#pragma once

///
/// Android JNI ブリッジとレンダリングエンジンの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///
#if defined(__ANDROID__)

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES3/gl31.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>

#include "Config.h"
#include "Capture.h"
#include "Calibration.h"
#include "Menu.h"
#include "Texture.h"
#include "Framebuffer.h"

namespace calib
{
  ///
  /// Android レンダリングと画像処理を管理するエンジンクラス
  ///
  class NativeEngine
  {
    /// 設定情報
    std::unique_ptr<Config> config;

    /// キャプチャデバイス
    std::unique_ptr<Capture> capture;

    /// 較正処理
    std::unique_ptr<Calibration> calibration;

    /// メニュー（内部パラメータおよび展開シェーダーの管理）
    std::unique_ptr<Menu> menu;

    /// レンダリング用スレッド
    std::thread renderThread;

    /// 動作中フラグ
    std::atomic<bool> isRunning{ false };

    /// 現在のネイティブウィンドウ
    std::atomic<ANativeWindow*> nativeWindow{ nullptr };

    /// 描画領域の幅
    std::atomic<int> windowWidth{ 0 };

    /// 描画領域の高さ
    std::atomic<int> windowHeight{ 0 };

    /// ウィンドウサイズ更新フラグ
    std::atomic<bool> sizeChanged{ false };

    /// EGL ディスプレイ
    EGLDisplay display{ EGL_NO_DISPLAY };

    /// EGL サーフェス
    EGLSurface surface{ EGL_NO_SURFACE };

    /// EGL コンテキスト
    EGLContext context{ EGL_NO_CONTEXT };

    ///
    /// レンダリングループ本体
    ///
    void renderLoop();

    ///
    /// EGL の初期化
    ///
    /// @param window 初期化対象の ANativeWindow
    /// @return 初期化に成功したら true
    ///
    bool initEgl(ANativeWindow* window);

    ///
    /// EGL の破棄
    ///
    void destroyEgl();

  public:

    ///
    /// コンストラクタ
    ///
    NativeEngine();

    ///
    /// デストラクタ
    ///
    ~NativeEngine();

    ///
    /// 初期化（アセット展開および作業ディレクトリ設定）
    ///
    /// @param assetManager Android AssetManager
    /// @param internalPath アプリ内部ストレージのパス
    ///
    void init(AAssetManager* assetManager, const char* internalPath);

    ///
    /// Surface が生成されたときの処理
    ///
    /// @param window 対象の ANativeWindow
    ///
    void onSurfaceCreated(ANativeWindow* window);

    ///
    /// Surface のサイズが変更されたときの処理
    ///
    /// @param width 新しい幅
    /// @param height 新しい高さ
    ///
    void onSurfaceChanged(int width, int height);

    ///
    /// Surface が破棄されたときの処理
    ///
    void onSurfaceDestroyed();

    ///
    /// キャプチャを開始する
    ///
    /// @return 開始に成功したら true
    ///
    bool startCapture();

    ///
    /// キャプチャを停止する
    ///
    void stopCapture();

    ///
    /// キャプチャ中かどうかを判定する
    ///
    /// @return キャプチャ中なら true
    ///
    bool isCapturing() const;

    ///
    /// Menu インスタンスへのポインタを取得
    ///
    Menu* getMenu() { return menu.get(); }

    ///
    /// Calibration インスタンスへのポインタを取得
    ///
    Calibration* getCalibration() { return calibration.get(); }

    ///
    /// シングルトンインスタンスを取得
    ///
    static NativeEngine& getInstance();
  };
}

#endif // defined(__ANDROID__)
