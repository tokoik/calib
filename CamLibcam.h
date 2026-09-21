#pragma once

///
/// libcamera を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///

#if defined(USE_LIBCAMERA)

// カメラ関連の基底クラス
#include "Camera.h"

// libcamera C++ API
#include <libcamera/libcamera.h>

// 標準ライブラリ
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <condition_variable>

///
/// libcamera を使ってビデオをキャプチャするクラス
///
class CamLibcam : public Camera
{
  /// libcamera CameraManager のシングルトン／共有管理
  class Manager
  {
    std::unique_ptr<libcamera::CameraManager> cm;
    std::vector<std::string> deviceList;
    bool started{ false };

    Manager();
    ~Manager();

  public:
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    static Manager& getInstance();
    libcamera::CameraManager* get() { return cm.get(); }
    const std::vector<std::string>& getDeviceList();
  };

  /// カメラオブジェクト
  std::shared_ptr<libcamera::Camera> camera;

  /// カメラ設定
  std::unique_ptr<libcamera::CameraConfiguration> config;

  /// フレームバッファアロケータ
  std::unique_ptr<libcamera::FrameBufferAllocator> allocator;

  /// 生成したリクエストのリスト
  std::vector<std::unique_ptr<libcamera::Request>> requests;

  /// メモリーマップされたバッファプレーンの情報
  struct MappedPlane
  {
    void* address{ nullptr };
    size_t length{ 0 };
  };

  /// フレームバッファとマップ済みプレーンの対応マップ
  std::map<const libcamera::FrameBuffer*, std::vector<MappedPlane>> mappedBuffers;

  /// キャプチャストリーム
  libcamera::Stream* stream{ nullptr };

  /// 選択されているピクセルフォーマット
  libcamera::PixelFormat pixelFormat;

  /// ストライド幅
  unsigned int stride{ 0 };

  /// フレーム時間（マイクロ秒）
  int64_t frameDurationUs{ 33333 };

  /// キャプチャループ同期用
  std::condition_variable cv;
  bool frameReady{ false };

  ///
  /// リクエスト完了コールバック
  ///
  /// @param request 完了したリクエスト
  ///
  void requestComplete(libcamera::Request* request);

  ///
  /// バッファのメモリマッピングを解除する
  ///
  void unmapBuffers();

public:

  ///
  /// コンストラクタ
  ///
  /// @param deviceNumber カメラの番号
  /// @param initial_width 要求するフレームの横の画素数
  /// @param initial_height 要求するフレームの縦の画素数
  /// @param initial_fps 要求するフレームレート
  ///
  CamLibcam(int deviceNumber, int initial_width = 0, int initial_height = 0, double initial_fps = 0.0);

  ///
  /// デストラクタ
  ///
  virtual ~CamLibcam();

  ///
  /// キャプチャを開始する
  ///
  void start() override;

  ///
  /// キャプチャを停止する
  ///
  void stop() override;

  ///
  /// キャプチャデバイスを開く
  ///
  /// @param deviceNumber カメラの番号
  /// @param initial_width 要求するフレームの横の画素数
  /// @param initial_height 要求するフレームの縦の画素数
  /// @param initial_fps 要求するフレームレート
  /// @return 成功すれば true
  ///
  bool open(int deviceNumber, int initial_width = 0, int initial_height = 0, double initial_fps = 0.0);

  ///
  /// キャプチャデバイスを閉じる
  ///
  void close() override;

  ///
  /// キャプチャデバイスが有効かどうか調べる
  ///
  /// @return 有効なら true
  ///
  bool isOpened() const
  {
    return bool(camera);
  }

  ///
  /// 使用可能なカメラデバイスのリストを返す
  ///
  /// @return カメラ名一覧
  ///
  static const std::vector<std::string>& getDeviceList();

  ///
  /// 使用しているピクセルフォーマットの文字列表現を返す
  ///
  /// @return フォーマット名
  ///
  std::string getPixelFormatName() const;
};

#endif // USE_LIBCAMERA
