#if defined(__ANDROID__)

///
/// Android JNI ブリッジとレンダリングエンジンの実装 (OpenGL 非依存)
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///
#include "NativeBridge.h"

#include <fstream>
#include <vector>
#include <cmath>
#include <unistd.h>
#include <sys/stat.h>
#include <opencv2/imgproc.hpp>

#define LOG_TAG "calib-jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{
  constexpr float PI{ 3.14159265358979323846f };
  inline float degToRad(float deg) { return deg * (PI / 180.0f); }
  inline float radToDeg(float rad) { return rad * (180.0f / PI); }

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
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!internalPath) return;

    LOGI("Setting working directory to: %s", internalPath);
    if (chdir(internalPath) != 0)
    {
      LOGW("Failed to chdir to: %s", internalPath);
    }

    if (!assetManager) return;

    // 必須アセットを展開（シェーダー・フォントは不要）
    static const char* const requiredAssets[]{
      "calib_config.json",
      "castle.jpg",
      "initial.jpg",
      "sky.jpg"
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
    std::lock_guard<std::mutex> lock(engineMutex);
    if (capture && capture->isOpened() && !(*capture))
    {
      capture->start();
      return bool(*capture);
    }
    if (!menu) return false;
    return menu->startCapture();
  }

  void NativeEngine::stopCapture()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (capture)
    {
      capture->stop();
    }
  }

  bool NativeEngine::isCapturing() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!capture) return false;
    return bool(*capture);
  }

  // --- 画角・中心 ---
  float NativeEngine::getFovX() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getIntrinsics().fov[0] : 0.0f;
  }

  float NativeEngine::getFovY() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getIntrinsics().fov[1] : 0.0f;
  }

  void NativeEngine::setFov(float x, float y)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->getIntrinsics().fov = { x, y };
  }

  float NativeEngine::getCenterX() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getIntrinsics().center[0] : 0.0f;
  }

  float NativeEngine::getCenterY() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getIntrinsics().center[1] : 0.0f;
  }

  void NativeEngine::setCenter(float x, float y)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->getIntrinsics().center = { x, y };
  }

  // --- 姿勢 ---
  float NativeEngine::getEulerHeading() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? radToDeg(menu->getSettings().euler[1]) : 0.0f;
  }

  float NativeEngine::getEulerPitch() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? radToDeg(menu->getSettings().euler[0]) : 0.0f;
  }

  float NativeEngine::getEulerRoll() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? radToDeg(menu->getSettings().euler[2]) : 0.0f;
  }

  void NativeEngine::setEuler(float heading, float pitch, float roll)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu)
    {
      menu->getSettings().euler[1] = degToRad(heading);
      menu->getSettings().euler[0] = degToRad(pitch);
      menu->getSettings().euler[2] = degToRad(roll);
    }
  }

  // --- 焦点距離 ---
  float NativeEngine::getFocal() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().focal : 0.0f;
  }

  void NativeEngine::setFocal(float focal)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->getSettings().focal = focal;
  }

  float NativeEngine::getFocalMin() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().focalRange[0] : 100.0f;
  }

  float NativeEngine::getFocalMax() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().focalRange[1] : 5000.0f;
  }

  void NativeEngine::resetPose()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->resetPose();
  }

  // --- 較正 ---
  bool NativeEngine::isDetectingBoard() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->detectBoard : false;
  }

  void NativeEngine::setDetectBoard(bool enabled)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->detectBoard = enabled;
  }

  bool NativeEngine::recordSnapshot()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (calibration)
    {
      calibration->recordCorners();
      return true;
    }
    return false;
  }

  void NativeEngine::clearSnapshots()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (calibration) calibration->discardCorners();
  }

  int NativeEngine::getSampleCount() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return calibration ? calibration->getSampleCount() : 0;
  }

  double NativeEngine::calibrate()
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (calibration && calibration->calibrate())
    {
      return calibration->getReprojectionError();
    }
    return -1.0;
  }

  bool NativeEngine::isCalibrationFinished() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return calibration ? calibration->finished() : false;
  }

  double NativeEngine::getReprojectionError() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return calibration ? calibration->getReprojectionError() : 0.0;
  }

  bool NativeEngine::isAutoCaptureEnabled() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->autoCaptureEnabled : false;
  }

  void NativeEngine::setAutoCaptureEnabled(bool enabled)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu) menu->autoCaptureEnabled = enabled;
  }

  float NativeEngine::getAutoCaptureProgress() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return (calibration && menu) ? calibration->getStableProgress(menu->autoCaptureMinStableTime) : 0.0f;
  }

  bool NativeEngine::isAutoCaptureDiverse() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return calibration ? calibration->isDiverseEnough() : false;
  }

  bool NativeEngine::isAutoCaptureStable() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return calibration ? calibration->isStable() : false;
  }

  float NativeEngine::getCurrentMotion() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return calibration ? calibration->getCurrentMotion() : 999.0f;
  }

  std::string NativeEngine::getDictionaryName() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().dictionaryName : "";
  }

  void NativeEngine::setDictionary(const std::string& name)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu && calibration)
    {
      menu->getSettings().dictionaryName = name;
      calibration->setDictionary(name, menu->getSettings().checkerSize, menu->getSettings().checkerLength);
    }
  }

  int NativeEngine::getDictionaryCount() const
  {
    return static_cast<int>(Calibration::dictionaryList.size());
  }

  std::string NativeEngine::getDictionaryNameByIndex(int index) const
  {
    if (index < 0 || index >= static_cast<int>(Calibration::dictionaryList.size())) return "";
    auto it = Calibration::dictionaryList.begin();
    std::advance(it, index);
    return it->first;
  }

  int NativeEngine::getCheckerWidth() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().checkerSize[0] : 0;
  }

  int NativeEngine::getCheckerHeight() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().checkerSize[1] : 0;
  }

  void NativeEngine::setCheckerSize(int w, int h)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu && calibration)
    {
      menu->getSettings().checkerSize = { w, h };
      calibration->createBoard(menu->getSettings().checkerSize, menu->getSettings().checkerLength);
    }
  }

  float NativeEngine::getSquareLength() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().checkerLength[0] : 4.0f;
  }

  float NativeEngine::getMarkerLength() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getSettings().checkerLength[1] : 2.0f;
  }

  void NativeEngine::setCheckerLength(float squareLength, float markerLength)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (menu && calibration)
    {
      // ユーザー指定範囲 (チェッカー: 2cm～20cm, マーカー: 1cm～10cm) へのクランプ
      squareLength = std::clamp(squareLength, 2.0f, 20.0f);
      markerLength = std::clamp(markerLength, 1.0f, 10.0f);
      // マーカーサイズはチェッカーマス目サイズ未満でなければならない (基本はマス目の1/2)
      if (markerLength >= squareLength)
      {
        markerLength = squareLength * 0.5f;
      }
      menu->getSettings().checkerLength = { squareLength, markerLength };
      calibration->createBoard(menu->getSettings().checkerSize, menu->getSettings().checkerLength);
    }
  }

  bool NativeEngine::saveParameters(const std::string& filename) const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!calibration || !calibration->finished()) return false;
    return calibration->saveParameters(filename);
  }

  void NativeEngine::getStatus(float* outStatus, int count) const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!outStatus || count < 9) return;
    outStatus[0] = (capture && bool(*capture)) ? 1.0f : 0.0f;
    outStatus[1] = (menu && menu->detectBoard) ? 1.0f : 0.0f;
    outStatus[2] = (menu && menu->autoCaptureEnabled) ? 1.0f : 0.0f;
    outStatus[3] = (calibration && menu) ? calibration->getStableProgress(menu->autoCaptureMinStableTime) : 0.0f;
    outStatus[4] = (calibration && calibration->isDiverseEnough()) ? 1.0f : 0.0f;
    outStatus[5] = (calibration && calibration->isStable()) ? 1.0f : 0.0f;
    outStatus[6] = calibration ? static_cast<float>(calibration->getSampleCount()) : 0.0f;
    outStatus[7] = (calibration && calibration->finished()) ? 1.0f : 0.0f;
    outStatus[8] = calibration ? static_cast<float>(calibration->getReprojectionError()) : 0.0f;
    if (count >= 11)
    {
      outStatus[9] = static_cast<float>(frameWidth.load());
      outStatus[10] = static_cast<float>(frameHeight.load());
    }
  }

  int NativeEngine::getResolutionCount() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getResolutionCount() : 0;
  }

  std::string NativeEngine::getResolutionByIndex(int index) const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getResolutionByIndex(index) : "";
  }

  std::string NativeEngine::getCurrentResolution() const
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    return menu ? menu->getCurrentResolution() : "";
  }

  bool NativeEngine::selectResolution(const std::string& resolution)
  {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (!menu) return false;
    return menu->selectResolution(resolution);
  }

  void NativeEngine::renderLoop()
  {
    LOGI("renderLoop started (Direct ANativeWindow CPU Blit)");
    ANativeWindow* win{ nativeWindow.load() };
    if (!win)
    {
      LOGE("ANativeWindow is null in renderLoop");
      return;
    }

    {
      std::lock_guard<std::mutex> lock(engineMutex);
      if (!config || !capture || !calibration || !menu)
      {
        LOGE("Engine components are not initialized");
        return;
      }

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
          capture->start();
          menu->initializeInputIntrinsics(capture->getSize());
        }
      }
    }

    cv::Mat cpuFrame;
    cv::Mat displayFrame;
    ANativeWindow* currentWin{ nullptr };
    int lastW{ 0 }, lastH{ 0 };

    auto lastFrameTime{ std::chrono::steady_clock::now() };

    while (isRunning)
    {
      const auto currentFrameTime{ std::chrono::steady_clock::now() };
      float deltaTime{ std::chrono::duration<float>(currentFrameTime - lastFrameTime).count() };
      lastFrameTime = currentFrameTime;
      if (deltaTime <= 0.0f || deltaTime > 0.5f) deltaTime = 0.033f;

      ANativeWindow* win{ nativeWindow.load() };
      if (win != currentWin)
      {
        currentWin = win;
        lastW = 0;
        lastH = 0;
      }

      if (!currentWin)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        continue;
      }

      {
        std::lock_guard<std::mutex> lock(engineMutex);

        if (capture && *capture)
        {
          const bool hasNewFrame{ capture->retrieve(cpuFrame) };
          if (hasNewFrame && !cpuFrame.empty())
          {
            frameWidth = cpuFrame.cols;
            frameHeight = cpuFrame.rows;

            // 1. CPU 上で直接 ArUco / ChArUco 認識
            if (menu->detectBoard)
            {
              calibration->detectBoard(cpuFrame);
              menu->updateAutoCapture(deltaTime);
            }
            else if (menu->detectMarker)
            {
              calibration->detectMarkers(cpuFrame, menu->getMarkerLength());
            }

            // 2. ウィンドウバッファサイズ設定（解像度変更時）
            if (lastW != cpuFrame.cols || lastH != cpuFrame.rows)
            {
              ANativeWindow_setBuffersGeometry(currentWin, cpuFrame.cols, cpuFrame.rows, WINDOW_FORMAT_RGBA_8888);
              lastW = cpuFrame.cols;
              lastH = cpuFrame.rows;
              LOGI("ANativeWindow buffers geometry set to: %d x %d", lastW, lastH);
            }

            // 3. ANativeWindow へ直接描画 (BGRA -> RGBA)
            ANativeWindow_Buffer winBuf;
            if (ANativeWindow_lock(currentWin, &winBuf, nullptr) == 0)
            {
              if (cpuFrame.channels() == 4)
              {
                cv::cvtColor(cpuFrame, displayFrame, cv::COLOR_BGRA2RGBA);
              }
              else if (cpuFrame.channels() == 3)
              {
                cv::cvtColor(cpuFrame, displayFrame, cv::COLOR_BGR2RGBA);
              }
              else
              {
                displayFrame = cpuFrame;
              }

              const int copyRows{ std::min(winBuf.height, displayFrame.rows) };
              const int srcRowBytes{ displayFrame.cols * 4 };
              const int dstStrideBytes{ winBuf.stride * 4 };
              const uint8_t* srcBits{ displayFrame.data };
              uint8_t* dstBits{ static_cast<uint8_t*>(winBuf.bits) };

              if (winBuf.stride == displayFrame.cols)
              {
                std::memcpy(dstBits, srcBits, srcRowBytes * copyRows);
              }
              else
              {
                for (int y = 0; y < copyRows; ++y)
                {
                  std::memcpy(dstBits + y * dstStrideBytes, srcBits + y * srcRowBytes, srcRowBytes);
                }
              }

              ANativeWindow_unlockAndPost(currentWin);
            }
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    {
      std::lock_guard<std::mutex> lock(engineMutex);
      if (capture)
      {
        capture->stop();
        capture->close();
      }
    }

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

  // --- 投影方式 ---
  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetPreferenceCount(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getPreferenceCount();
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetPreferenceName(
    JNIEnv* env, jclass, jint index)
  {
    const std::string name{ calib::NativeEngine::getInstance().getPreferenceName(index) };
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetPreferenceIndex(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getPreferenceIndex();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSelectPreference(
    JNIEnv*, jclass, jint index)
  {
    calib::NativeEngine::getInstance().selectPreference(index);
  }

  // --- 画角・中心 ---
  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFovX(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFovX();
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFovY(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFovY();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetFov(
    JNIEnv*, jclass, jfloat x, jfloat y)
  {
    calib::NativeEngine::getInstance().setFov(x, y);
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetCenterX(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getCenterX();
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetCenterY(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getCenterY();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetCenter(
    JNIEnv*, jclass, jfloat x, jfloat y)
  {
    calib::NativeEngine::getInstance().setCenter(x, y);
  }

  // --- 姿勢 ---
  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetEulerHeading(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getEulerHeading();
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetEulerPitch(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getEulerPitch();
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetEulerRoll(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getEulerRoll();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetEuler(
    JNIEnv*, jclass, jfloat heading, jfloat pitch, jfloat roll)
  {
    calib::NativeEngine::getInstance().setEuler(heading, pitch, roll);
  }

  // --- 焦点距離 ---
  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFocal(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFocal();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetFocal(
    JNIEnv*, jclass, jfloat focal)
  {
    calib::NativeEngine::getInstance().setFocal(focal);
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFocalMin(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFocalMin();
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFocalMax(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFocalMax();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeResetPose(
    JNIEnv*, jclass)
  {
    calib::NativeEngine::getInstance().resetPose();
  }

  // --- 較正 ---
  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeIsDetectingBoard(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().isDetectingBoard() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetDetectBoard(
    JNIEnv*, jclass, jboolean enabled)
  {
    calib::NativeEngine::getInstance().setDetectBoard(enabled == JNI_TRUE);
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeRecordSnapshot(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().recordSnapshot() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeClearSnapshots(
    JNIEnv*, jclass)
  {
    calib::NativeEngine::getInstance().clearSnapshots();
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetSampleCount(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getSampleCount();
  }

  JNIEXPORT jdouble JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeCalibrate(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().calibrate();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeIsCalibrationFinished(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().isCalibrationFinished() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jdouble JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetReprojectionError(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getReprojectionError();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeIsAutoCaptureEnabled(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().isAutoCaptureEnabled() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetAutoCaptureEnabled(
    JNIEnv*, jclass, jboolean enabled)
  {
    calib::NativeEngine::getInstance().setAutoCaptureEnabled(enabled == JNI_TRUE);
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetAutoCaptureProgress(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getAutoCaptureProgress();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeIsAutoCaptureDiverse(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().isAutoCaptureDiverse() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeIsAutoCaptureStable(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().isAutoCaptureStable() ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetCurrentMotion(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getCurrentMotion();
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetDictionaryName(
    JNIEnv* env, jclass)
  {
    const std::string name{ calib::NativeEngine::getInstance().getDictionaryName() };
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetDictionary(
    JNIEnv* env, jclass, jstring nameStr)
  {
    const char* name{ env->GetStringUTFChars(nameStr, nullptr) };
    calib::NativeEngine::getInstance().setDictionary(name);
    env->ReleaseStringUTFChars(nameStr, name);
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetDictionaryCount(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getDictionaryCount();
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetDictionaryNameByIndex(
    JNIEnv* env, jclass, jint index)
  {
    const std::string name{ calib::NativeEngine::getInstance().getDictionaryNameByIndex(index) };
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetCheckerWidth(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getCheckerWidth();
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetCheckerHeight(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getCheckerHeight();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetCheckerSize(
    JNIEnv*, jclass, jint w, jint h)
  {
    calib::NativeEngine::getInstance().setCheckerSize(w, h);
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetSquareLength(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getSquareLength();
  }

  JNIEXPORT jfloat JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetMarkerLength(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getMarkerLength();
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSetCheckerLength(
    JNIEnv*, jclass, jfloat squareLength, jfloat markerLength)
  {
    calib::NativeEngine::getInstance().setCheckerLength(squareLength, markerLength);
  }

  JNIEXPORT void JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetStatus(
    JNIEnv* env, jclass, jfloatArray outStatus)
  {
    if (!outStatus) return;
    jsize len{ env->GetArrayLength(outStatus) };
    if (len < 9) return;
    const int count{ std::min(static_cast<int>(len), 11) };
    jfloat buf[11]{};
    calib::NativeEngine::getInstance().getStatus(buf, count);
    env->SetFloatArrayRegion(outStatus, 0, count, buf);
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFrameWidth(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFrameWidth();
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetFrameHeight(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getFrameHeight();
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSaveParameters(
    JNIEnv* env, jclass, jstring pathStr)
  {
    if (!pathStr) return JNI_FALSE;
    const char* path{ env->GetStringUTFChars(pathStr, nullptr) };
    bool ok{ calib::NativeEngine::getInstance().saveParameters(path) };
    env->ReleaseStringUTFChars(pathStr, path);
    return ok ? JNI_TRUE : JNI_FALSE;
  }

  JNIEXPORT jint JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetResolutionCount(
    JNIEnv*, jclass)
  {
    return calib::NativeEngine::getInstance().getResolutionCount();
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetResolutionByIndex(
    JNIEnv* env, jclass, jint index)
  {
    const std::string res{ calib::NativeEngine::getInstance().getResolutionByIndex(index) };
    return env->NewStringUTF(res.c_str());
  }

  JNIEXPORT jstring JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeGetCurrentResolution(
    JNIEnv* env, jclass)
  {
    const std::string res{ calib::NativeEngine::getInstance().getCurrentResolution() };
    return env->NewStringUTF(res.c_str());
  }

  JNIEXPORT jboolean JNICALL Java_net_wakayama_1u_tokoi_calib_NativeBridge_nativeSelectResolution(
    JNIEnv* env, jclass, jstring resStr)
  {
    if (!resStr) return JNI_FALSE;
    const char* res{ env->GetStringUTFChars(resStr, nullptr) };
    const bool ok{ calib::NativeEngine::getInstance().selectResolution(res) };
    env->ReleaseStringUTFChars(resStr, res);
    return ok ? JNI_TRUE : JNI_FALSE;
  }
}

#endif // defined(__ANDROID__)
