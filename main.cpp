///
/// ゲームグラフィックス特論宿題アプリケーション
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "GgApp.h"

// MessageBox の準備
#if defined(_WIN32)
#  include <atlstr.h>
#elif defined(__APPLE__)
#  include <CoreFoundation/CoreFoundation.h>
#else
#  include <iostream>
#endif
#define HEADER_STR "ゲームグラフィックス特論"

#if defined(__ANDROID__)
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <string>

extern void* ggAndroidAssetManager;

#define LOG_TAG "calib"
// 情報と警告はデバッグビルドだけに出力する (リリースビルドでは出力しない)
// if (false) で囲むのは、ログにしか使わない変数が未使用の警告にならないようにするため
#if defined(NDEBUG)
#define LOGI(...) do { if (false) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__); } while (0)
#define LOGW(...) do { if (false) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__); } while (0)
#else
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

//
// 単一アセットを内部ストレージへ書き出す
//
static bool extractSingleAsset(AAssetManager* assetMgr, const char* filename, const char* internalPath)
{
  if (!assetMgr || !filename || !internalPath) return false;

  AAsset* asset{ AAssetManager_open(assetMgr, filename, AASSET_MODE_STREAMING) };
  if (!asset)
  {
    LOGW("Asset not found in APK: %s", filename);
    return false;
  }

  const off_t size{ AAsset_getLength(asset) };
  const std::string destPath{ std::string(internalPath) + "/" + filename };

  // 既存ファイルのサイズを検査し、同一サイズなら展開をスキップする
  bool needWrite{ true };
  std::ifstream check(destPath, std::ios::binary | std::ios::ate);
  if (check.is_open())
  {
    if (check.tellg() == size && size > 0)
    {
      needWrite = false;
    }
    check.close();
  }

  if (needWrite)
  {
    std::ofstream out(destPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
      LOGE("Failed to open destination for write: %s", destPath.c_str());
      AAsset_close(asset);
      return false;
    }

    std::vector<char> buffer(65536);
    int bytesRead{ 0 };
    off_t totalWritten{ 0 };
    while ((bytesRead = AAsset_read(asset, buffer.data(), static_cast<int>(buffer.size()))) > 0)
    {
      out.write(buffer.data(), bytesRead);
      totalWritten += bytesRead;
    }
    out.close();

    LOGI("Extracted asset: %s (%ld / %ld bytes)", filename, static_cast<long>(totalWritten), static_cast<long>(size));
  }
  else
  {
    LOGI("Asset already up to date: %s", filename);
  }

  AAsset_close(asset);
  return true;
}

//
// アセットをアプリ内内部ストレージへ展開し、作業ディレクトリを設定する
//
static void setupAndroidAssetsAndStorage(struct android_app* state)
{
  if (!state || !state->activity)
  {
    LOGE("Invalid android_app state in setupAndroidAssetsAndStorage");
    return;
  }

  const char* internalPath{ state->activity->internalDataPath };
  if (!internalPath)
  {
    LOGE("Internal data path is null");
    return;
  }

  LOGI("Setting working directory to: %s", internalPath);
  if (chdir(internalPath) != 0)
  {
    LOGW("Failed to chdir to: %s", internalPath);
  }

  AAssetManager* assetMgr{ state->activity->assetManager };
  if (!assetMgr)
  {
    LOGE("AssetManager is null");
    return;
  }

  ggAndroidAssetManager = assetMgr;

  // 1. 走査可能な端末ではディレクトリ走査により全アセットを展開
  AAssetDir* assetDir{ AAssetManager_openDir(assetMgr, "") };
  if (assetDir)
  {
    const char* filename{ nullptr };
    while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr)
    {
      extractSingleAsset(assetMgr, filename, internalPath);
    }
    AAssetDir_close(assetDir);
  }

  // 2. ディレクトリ走査が 0 件を返す端末や圧縮環境に備え、必須アセットを個別展開
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

  for (const auto* assetName : requiredAssets)
  {
    extractSingleAsset(assetMgr, assetName, internalPath);
  }
}

//
// Android NativeActivity エントリーポイント
//
void android_main(struct android_app* state)
{
  LOGI("android_main started");
  GgApp::androidApp = state;

  // 入力イベントハンドラを登録
  state->onInputEvent = [](struct android_app*, AInputEvent* event) -> int32_t {
#if defined(GG_USE_IMGUI)
    if (ImGui_ImplAndroid_HandleInputEvent(event))
    {
      return 1;
    }
#endif
    return 0;
  };

  // アセットの展開と作業ディレクトリの設定
  setupAndroidAssetsAndStorage(state);

  try
  {
    LOGI("Initializing GgApp (OpenGL ES 3.1)...");
    GgApp app(3, 1);
    LOGI("Running GgApp main loop...");
    app.main(0, nullptr);
    LOGI("GgApp main loop finished.");
  }
  catch (const std::exception& e)
  {
    LOGE("Application error: %s", e.what());
  }
  catch (...)
  {
    LOGE("Unknown application exception occurred.");
  }

  // アプリケーション終了時に NativeActivity を安全に破棄する
  if (state && state->activity)
  {
    LOGI("Finishing ANativeActivity...");
    ANativeActivity_finish(state->activity);

    // Activity の完全な破棄 (APP_CMD_DESTROY) を待機
    int ident;
    int events;
    struct android_poll_source* source;
    while ((ident = ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0)
    {
      if (source != nullptr) source->process(state, source);
      if (state->destroyRequested != 0) break;
    }
    LOGI("android_main exited cleanly.");
  }
}

#else

//
// メインプログラム
//
int main(int argc, const char* const* argv) try
{
  // アプリケーションのオブジェクトを生成する
#if defined(GL_GLES_PROTOTYPES)
  GgApp app(3, 1);
#else
  GgApp app(4, 1);
#endif

  // アプリケーションを実行する
  return app.main(argc, argv);
}
catch (const std::runtime_error &e)
{
  // エラーメッセージを表示する
#if defined(_WIN32)
  MessageBox(NULL, CString(e.what()), TEXT(HEADER_STR), MB_ICONERROR);
#elif defined(__APPLE__)
  // the following code is copied from http://blog.jorgearimany.com/2010/05/messagebox-from-windows-to-mac.html
  // convert the strings from char* to CFStringRef
  CFStringRef msg_ref = CFStringCreateWithCString(NULL, e.what(), kCFStringEncodingUTF8);

  // result code from the message box
  CFOptionFlags result;

  //launch the message box
  CFUserNotificationDisplayAlert(
    0,                                 // no timeout
    kCFUserNotificationNoteAlertLevel, // change it depending message_type flags ( MB_ICONASTERISK.... etc.)
    NULL,                              // icon url, use default, you can change it depending message_type flags
    NULL,                              // not used
    NULL,                              // localization of strings
    CFSTR(HEADER_STR),                 // header text
    msg_ref,                           // message text
    NULL,                              // default "ok" text in button
    NULL,                              // alternate button title
    NULL,                              // other button title, null--> no other button
    &result                            // response flags
  );

  // Clean up the strings
  CFRelease(msg_ref);
#else
  std::cerr << HEADER_STR << ": " << e.what() << '\n';
#endif

  // ブログラムを終了する
  return EXIT_FAILURE;
}
#endif
