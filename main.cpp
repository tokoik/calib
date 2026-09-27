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

extern void* ggAndroidAssetManager;

//
// アセットをアプリ内内部ストレージへ展開し、作業ディレクトリを設定する
//
static void setupAndroidAssetsAndStorage(struct android_app* state)
{
  if (!state || !state->activity) return;

  const char* internalPath{ state->activity->internalDataPath };
  if (!internalPath) return;

  // アプリの内部ストレージを作業ディレクトリに設定
  chdir(internalPath);

  AAssetManager* assetMgr{ state->activity->assetManager };
  if (!assetMgr) return;

  ggAndroidAssetManager = assetMgr;

  // アセット一覧を走査して内部ストレージへ展開
  AAssetDir* assetDir{ AAssetManager_openDir(assetMgr, "") };
  if (assetDir)
  {
    const char* filename{ nullptr };
    while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr)
    {
      AAsset* asset{ AAssetManager_open(assetMgr, filename, AASSET_MODE_BUFFER) };
      if (asset)
      {
        const off_t size{ AAsset_getLength(asset) };
        const std::string destPath{ std::string(internalPath) + "/" + filename };

        bool needWrite{ true };
        std::ifstream check(destPath, std::ios::binary | std::ios::ate);
        if (check.is_open())
        {
          if (check.tellg() == size) needWrite = false;
          check.close();
        }

        if (needWrite)
        {
          const void* buffer{ AAsset_getBuffer(asset) };
          std::ofstream out(destPath, std::ios::binary);
          if (out.is_open())
          {
            out.write(static_cast<const char*>(buffer), size);
            out.close();
          }
        }
        AAsset_close(asset);
      }
    }
    AAssetDir_close(assetDir);
  }
}

//
// Android NativeActivity エントリーポイント
//
void android_main(struct android_app* state)
{
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
    GgApp app(3, 1);
    app.main(0, nullptr);
  }
  catch (const std::exception& e)
  {
    __android_log_print(ANDROID_LOG_ERROR, "calib", "Application error: %s", e.what());
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
