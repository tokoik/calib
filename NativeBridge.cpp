#if defined(__ANDROID__)

///
/// Android JNI ブリッジとレンダリングエンジンの実装
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///
#include "NativeBridge.h"
#include "gg.h"

#include <fstream>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>

#define LOG_TAG "calib-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern void* ggAndroidAssetManager;

namespace
{
  //
  // 単一アセットを内部ストレージへ展開する
  //
  bool extractSingleAsset(AAssetManager* assetMgr, const char* filename, const std::string& internalPath)
  {
    if (!assetMgr || !filename || filename[0] == '\0') return false;

    AAsset* asset{ AAssetManager_open(assetMgr, filename, AASSET_MODE_BUFFER) };
    if (!asset) return false;

    const std::string destPath{ internalPath + "/" + filename };
    const off_t size{ AAsset_getLength(asset) };

    struct stat st;
    if (stat(destPath.c_str(), &st) != 0 || st.st_size != size)
    {
      std::ofstream out(destPath, std::ios::binary);
      if (!out)
      {
        LOGE("Failed to open destination for write: %s", destPath.c_str());
        AAsset_close(asset);
        return false;
      }

      std::vector<char> buffer(65536);
      int bytesRead{ 0 };
      while ((bytesRead = AAsset_read(asset, buffer.data(), static_cast<int>(buffer.size()))) > 0)
      {
        out.write(buffer.data(), bytesRead);
      }
      out.close();
      LOGI("Extracted asset: %s (%ld bytes)", filename, static_cast<long>(size));
    }

    AAsset_close(asset);
    return true;
  }
}

namespace calib
{
  NativeEngine::NativeEngine() = default;

  NativeEngine::~NativeEngine()
  {
    onSurfaceDestroyed();
  }

  NativeEngine& NativeEngine::getInstance()
  {
    static NativeEngine instance;
    return instance;
  }

  void NativeEngine::init(AAssetManager* assetManager, const char* internalPath)
  {
    if (!internalPath) return;

    LOGI("Setting working directory to: %s", internalPath);
    if (chdir(internalPath) != 0)
    {
      LOGW("Failed to chdir to: %s", internalPath);
    }

    if (!assetManager) return;
    ggAndroidAssetManager = assetManager;

    // 必須アセットを展開
    static const char* const requiredAssets[]{
      "calib_config.json",
      "castle.jpg",
      "draw.frag",
      "draw.vert",
      "equidistance_up.vert",
      "equidistance.vert",
      "equirectangular.frag",
      "equirectangular.vert",
      "initial.jpg",
      "Mplus1-Regular.ttf",
      "normal.frag",
      "orthographic.vert",
      "sky.jpg",
      "stereographic_up.vert",
      "stereographic.vert",
      "theta.frag",
      "theta.vert"
    };

    const std::string pathStr{ internalPath };
    for (const auto* assetName : requiredAssets)
    {
      extractSingleAsset(assetManager, assetName, pathStr);
    }

    // 展開後に Config, Capture, Calibration, Menu を構築
    config = std::make_unique<Config>("calib_config.json");
    capture = std::make_unique<Capture>();
    calibration = std::make_unique<Calibration>(
      config->getDictionaryName(), config->getCheckerSize(), config->getCheckerLength());
    menu = std::make_unique<Menu>(*config, *capture, *calibration);
  }

  bool NativeEngine::initEgl(ANativeWindow* window)
  {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
    {
      LOGE("eglGetDisplay failed");
      return false;
    }

    if (!eglInitialize(display, nullptr, nullptr))
    {
      LOGE("eglInitialize failed");
      return false;
    }

    const EGLint attribs[]{
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_RED_SIZE, 8,
      EGL_DEPTH_SIZE, 24,
      EGL_NONE
    };

    EGLConfig eglConfig;
    EGLint numConfigs{ 0 };
    if (!eglChooseConfig(display, attribs, &eglConfig, 1, &numConfigs) || numConfigs <= 0)
    {
      LOGE("eglChooseConfig failed");
      return false;
    }

    const EGLint contextAttribs[]{
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
    };

    context = eglCreateContext(display, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT)
    {
      LOGE("eglCreateContext failed");
      return false;
    }

    surface = eglCreateWindowSurface(display, eglConfig, window, nullptr);
    if (surface == EGL_NO_SURFACE)
    {
      LOGE("eglCreateWindowSurface failed");
      return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context))
    {
      LOGE("eglMakeCurrent failed");
      return false;
    }

    return true;
  }

  void NativeEngine::destroyEgl()
  {
    if (display != EGL_NO_DISPLAY)
    {
      eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      if (surface != EGL_NO_SURFACE)
      {
        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
      }
      if (context != EGL_NO_CONTEXT)
      {
        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
      }
      eglTerminate(display);
      display = EGL_NO_DISPLAY;
    }
  }

  void NativeEngine::onSurfaceCreated(ANativeWindow* window)
  {
    LOGI("onSurfaceCreated");
    onSurfaceDestroyed();

    nativeWindow = window;
    isRunning = true;
    renderThread = std::thread(&NativeEngine::renderLoop, this);
  }

  void NativeEngine::onSurfaceChanged(int width, int height)
  {
    LOGI("onSurfaceChanged: %d x %d", width, height);
    windowWidth = width;
    windowHeight = height;
    sizeChanged = true;
  }

  void NativeEngine::onSurfaceDestroyed()
  {
    LOGI("onSurfaceDestroyed");
    isRunning = false;
    if (renderThread.joinable())
    {
      renderThread.join();
    }
    if (nativeWindow)
    {
      ANativeWindow_release(nativeWindow);
      nativeWindow = nullptr;
    }
  }

  bool NativeEngine::startCapture()
  {
    if (!menu) return false;
    return menu->startCapture();
  }

  void NativeEngine::stopCapture()
  {
    if (capture)
    {
      capture->stop();
    }
  }

  bool NativeEngine::isCapturing() const
  {
    if (!capture) return false;
    return capture->isOpened();
  }

  void NativeEngine::renderLoop()
  {
    LOGI("renderLoop started");
    ANativeWindow* win{ nativeWindow.load() };
    if (!win || !initEgl(win))
    {
      LOGE("Failed to initialize EGL in renderLoop");
      return;
    }

    // OpenGL 拡張機能および補助ライブラリの初期化
    gg::ggInit();

    if (!config || !capture || !calibration || !menu)
    {
      LOGE("Engine components are not initialized");
      destroyEgl();
      return;
    }

    // OpenGL コンテキスト作成後の設定初期化
    config->initialize();

    // 背面カメラの自動検索・開始
    bool cameraStarted{ false };
    const auto& deviceList{ config->getDeviceList() };
    int backCameraIndex{ -1 };

    for (int i = 0; i < static_cast<int>(deviceList.size()); ++i)
    {
      if (deviceList[i].find("Back") != std::string::npos || deviceList[i].find("back") != std::string::npos)
      {
        backCameraIndex = i;
        break;
      }
    }

    if (backCameraIndex < 0 && !deviceList.empty())
    {
      backCameraIndex = 0;
    }

    if (backCameraIndex >= 0)
    {
      menu->setDeviceNumber(backCameraIndex);
      cameraStarted = menu->startCapture();
    }

    if (!cameraStarted)
    {
      if (capture->openImage(config->getInitialImage()))
      {
        menu->initializeInputIntrinsics(capture->getSize());
      }
    }

    Texture frame;
    Framebuffer framebuffer{ config->getWidth(), config->getHeight() };

    auto lastFrameTime{ std::chrono::steady_clock::now() };

    while (isRunning)
    {
      const auto currentFrameTime{ std::chrono::steady_clock::now() };
      float deltaTime{ std::chrono::duration<float>(currentFrameTime - lastFrameTime).count() };
      lastFrameTime = currentFrameTime;
      if (deltaTime <= 0.0f || deltaTime > 0.5f) deltaTime = 0.033f;

      const int curW{ windowWidth.load() };
      const int curH{ windowHeight.load() };

      if (curW <= 0 || curH <= 0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        continue;
      }

      glViewport(0, 0, curW, curH);

      // フレーム取得
      if (*capture)
      {
        capture->retrieve(frame);
        frame.drawPixels();
        framebuffer.resize(frame);

        const auto&& size{ menu->setup(framebuffer.getAspect()) };
        framebuffer.update(size, frame);

        // ArUco / ChArUco 認識
        if (menu->detectMarker || menu->detectBoard)
        {
          framebuffer.readPixels();
          const auto imgSize{ cv::Size{ framebuffer.getWidth(), framebuffer.getHeight() } };
          cv::Mat image{ imgSize, CV_8UC(framebuffer.getChannels()), framebuffer.map() };

          if (menu->detectBoard)
          {
            calibration->detectBoard(image);
            menu->updateAutoCapture(deltaTime);
          }
          else
          {
            calibration->detectMarkers(image, menu->getMarkerLength());
          }

          framebuffer.unmap();
          framebuffer.drawPixels();
        }

        // 画面全体への中央 contain 描画
        framebuffer.draw(curW, curH);
      }
      else
      {
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }

      if (!eglSwapBuffers(display, surface))
      {
        LOGW("eglSwapBuffers returned false");
      }
    }

    capture->stop();
    capture->close();

    destroyEgl();
    LOGI("renderLoop exited cleanly");
  }
}

//
// JNI 関数エクスポート
//
extern "C"
{
  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeInit(
    JNIEnv* env, jclass, jobject assetManager, jstring internalPath)
  {
    AAssetManager* mgr{ AAssetManager_fromJava(env, assetManager) };
    const char* path{ env->GetStringUTFChars(internalPath, nullptr) };
    calib::NativeEngine::getInstance().init(mgr, path);
    env->ReleaseStringUTFChars(internalPath, path);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSurfaceCreated(
    JNIEnv* env, jclass, jobject surface)
  {
    ANativeWindow* window{ ANativeWindow_fromSurface(env, surface) };
    calib::NativeEngine::getInstance().onSurfaceCreated(window);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSurfaceChanged(
    JNIEnv*, jclass, jint width, jint height)
  {
    calib::NativeEngine::getInstance().onSurfaceChanged(width, height);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSurfaceDestroyed(
    JNIEnv*, jclass)
  {
    calib::NativeEngine::getInstance().onSurfaceDestroyed();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeStartCapture(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().startCapture() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeStopCapture(
    JNIEnv*, jclass)
  {
    calib::NativeEngine::getInstance().stopCapture();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeIsCapturing(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().isCapturing() ? JNI_TRUE : JNI_FALSE;
  }
}

#endif // defined(__ANDROID__)
