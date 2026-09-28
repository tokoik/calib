#pragma once

///
/// Android Camera2 NDK を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date September 2026
///

#if defined(__ANDROID__)

// カメラ関連の基底クラス
#include "Camera.h"

// Android NDK Camera2 & Media API
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraMetadata.h>
#include <media/NdkImageReader.h>
#include <media/NdkImage.h>

// 標準ライブラリ
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

///
/// Android Camera2 NDK を使ってビデオをキャプチャするクラス
///
class CamAndroid : public Camera
{
  /// カメラマネージャ
  ACameraManager* cameraManager{ nullptr };

  /// カメラデバイス
  ACameraDevice* cameraDevice{ nullptr };

  /// キャプチャセッション
  ACameraCaptureSession* captureSession{ nullptr };

  /// キャプチャリクエスト
  ACaptureRequest* captureRequest{ nullptr };

  /// セッション出力コンテナ
  ACaptureSessionOutputContainer* outputContainer{ nullptr };

  /// セッション出力
  ACaptureSessionOutput* sessionOutput{ nullptr };

  /// 出力ターゲット
  ACameraOutputTarget* outputTarget{ nullptr };

  /// イメージリーダー
  AImageReader* imageReader{ nullptr };

  /// ネイティブウィンドウ
  ANativeWindow* imageWindow{ nullptr };

  /// セッションクローズ待機用ミューテックス
  std::mutex sessionMtx;

  /// セッションクローズ待機用条件変数
  std::condition_variable sessionCv;

  /// セッションがクローズされたら true
  std::atomic<bool> sessionClosed{ false };

  /// デバイス番号
  int deviceIndex{ 0 };

  /// 選択されているカメラ ID
  std::string selectedCameraId;

  /// カメラフォーマットのリスト
  std::vector<CaptureFormat> formatList;

  /// 使用可能なカメラデバイスのリスト
  static std::vector<std::string> deviceList;

  ///
  /// 利用可能なカメラフォーマットを列挙して formatList に格納する
  ///
  /// @param metadata カメラメタデータ
  ///
  void enumerateFormats(ACameraMetadata* metadata);

  ///
  /// 画像取得時のコールバック関数
  ///
  /// @param reader 画像を生成した AImageReader
  ///
  static void onImageAvailableCallback(void* context, AImageReader* reader);

  ///
  /// YUV420_888 形式の AImage を BGRA8888 形式に変換してバッファへ格納する
  ///
  /// @param img 変換元の AImage
  ///
  void convertYuvToBgra(AImage* img);

protected:

  ///
  /// キャプチャ開始処理を行う
  ///
  /// @return 正常に開始できたら true
  ///
  virtual bool onStart() override;

  ///
  /// キャプチャ停止処理を行う
  ///
  virtual void onStop() override;

  ///
  /// キャプチャデバイスを閉じる処理を行う
  ///
  virtual void onClose() override;

public:

  ///
  /// コンストラクタ
  ///
  /// @param deviceNumber カメラの番号 (0: 背面カメラ, 1: 前面カメラなど)
  /// @param initial_width 要求するフレームの横の画素数
  /// @param initial_height 要求するフレームの縦の画素数
  /// @param initial_fps 要求するフレームレート
  ///
  CamAndroid(int deviceNumber = 0, int initial_width = 0, int initial_height = 0, double initial_fps = 0.0);

  ///
  /// デストラクタ
  ///
  virtual ~CamAndroid();

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
  /// キャプチャデバイスが有効かどうか調べる
  ///
  /// @return 有効なら true
  ///
  bool isOpened() const
  {
    return cameraManager != nullptr && !selectedCameraId.empty();
  }

  ///
  /// 使用可能なカメラデバイスのリストを返す
  ///
  /// @return カメラ名一覧
  ///
  static const std::vector<std::string>& getDeviceList();

  ///
  /// フォーマット一覧を取得する
  ///
  /// @return サポートされているフォーマットのリスト
  ///
  const std::vector<CaptureFormat>& getFormatList() const override
  {
    return formatList;
  }

  ///
  /// フォーマットを選択する
  ///
  /// @param index 選択するフォーマット番号
  /// @return 正常に選択できたら true
  ///
  bool selectFormat(int index) override;
};

#endif // defined(__ANDROID__)
