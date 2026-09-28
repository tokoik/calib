#pragma once

///
/// Android JNI ブリッジとレンダリングエンジンの定義 (OpenGL 非依存)
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

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

#include "Config.h"
#include "Capture.h"
#include "Calibration.h"
#include "Menu.h"

namespace calib
{
  ///
  /// Android レンダリングと画像処理を管理するエンジンクラス (Direct ANativeWindow CPU Blit)
  ///
  class NativeEngine
  {
    /// 排他制御用ミューテックス
    mutable std::mutex engineMutex;

    /// 設定情報
    std::unique_ptr<Config> config;

    /// キャプチャデバイス
    std::unique_ptr<Capture> capture;

    /// 較正処理
    std::unique_ptr<Calibration> calibration;

    /// メニュー（設定管理および自動キャプチャ）
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

    /// キャプチャフレームの幅
    std::atomic<int> frameWidth{ 1280 };

    /// キャプチャフレームの高さ
    std::atomic<int> frameHeight{ 720 };

    ///
    /// レンダリングループ本体 (ANativeWindow 直接描画)
    ///
    void renderLoop();

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
    /// キャプチャフレームの幅を取得する
    ///
    /// @return キャプチャフレームの幅 (px)
    ///
    int getFrameWidth() const { return frameWidth.load(); }

    ///
    /// キャプチャフレームの高さを取得する
    ///
    /// @return キャプチャフレームの高さ (px)
    ///
    int getFrameHeight() const { return frameHeight.load(); }

    // --- 投影方式 (Preferences: Android ではダミー・非変形) ---
    int getPreferenceCount() const { return 1; }
    std::string getPreferenceName(int) const { return "Default"; }
    int getPreferenceIndex() const { return 0; }
    void selectPreference(int) {}

    // --- 画角・中心・姿勢・焦点距離 ---
    float getFovX() const;
    float getFovY() const;
    void setFov(float x, float y);

    float getCenterX() const;
    float getCenterY() const;
    void setCenter(float x, float y);

    float getEulerHeading() const;
    float getEulerPitch() const;
    float getEulerRoll() const;
    void setEuler(float heading, float pitch, float roll);

    float getFocal() const;
    void setFocal(float focal);
    float getFocalMin() const;
    float getFocalMax() const;
    void resetPose();

    // --- 較正 (Calibration) ---
    bool isDetectingBoard() const;
    void setDetectBoard(bool enabled);

    bool recordSnapshot();
    void clearSnapshots();
    int getSampleCount() const;
    double calibrate();
    bool isCalibrationFinished() const;
    double getReprojectionError() const;

    bool isAutoCaptureEnabled() const;
    void setAutoCaptureEnabled(bool enabled);
    float getAutoCaptureProgress() const;
    bool isAutoCaptureDiverse() const;
    bool isAutoCaptureStable() const;
    float getCurrentMotion() const;

    std::string getDictionaryName() const;
    void setDictionary(const std::string& name);

    ///
    /// 利用可能なマーカー辞書の総数を取得
    ///
    /// @return マーカー辞書の数
    ///
    int getDictionaryCount() const;

    ///
    /// インデックス指定でマーカー辞書名を取得
    ///
    /// @param index 辞書インデックス (0 <= index < getDictionaryCount())
    /// @return 辞書名（範囲外の場合は空文字列）
    ///
    std::string getDictionaryNameByIndex(int index) const;

    int getCheckerWidth() const;
    int getCheckerHeight() const;
    void setCheckerSize(int w, int h);
    bool saveParameters(const std::string& filename) const;
    void getStatus(float* outStatus, int count) const;

    ///
    /// シングルトンインスタンスを取得
    ///
    static NativeEngine& getInstance();
  };
}

#endif // defined(__ANDROID__)
