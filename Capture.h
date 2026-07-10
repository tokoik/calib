#pragma once

///
/// キャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date Aplil 3, 2023
///

// バッファクラス
#include "Buffer.h"

// OpenCV による画像ファイルの入力
#include "CamImage.h"

// OpenCV による動画の入力
#include "CamCv.h"

// Windows のみ Microsoft Media Foundation による動画の入力
#if defined(_WIN32)
#include "CamMf.h"
#endif

///
/// キャプチャクラス
///
class Capture
{
  /// 選択しているキャプチャデバイスのポインタ
  std::unique_ptr<Camera> camera;

#if defined(_WIN32)
  /// 使用可能なビデオフォーマットの表示名のリスト
  const std::vector<std::string>* formatList;

  /// 一時取得したキャプチャデバイスのビデオフォーマットの表示名のリスト
  std::vector<std::string> deviceFormatList;

  /// 空のビデオフォーマットの表示名のリスト
  static const std::vector<std::string> emptyFormatList;
#endif

public:

  ///
  /// キャプチャーオブジェクトのデフォルトコンストラクタ
  ///
  Capture()
#if defined(_WIN32)
    : camera{ nullptr }
    , formatList{ &deviceFormatList }
#endif
  {
  }

  ///
  /// キャプチャするファイルを指定するコンストラクタ
  ///
  /// @param filename キャプチャするファイルのパス名
  ///
  Capture(const std::string& filename)
#if defined(_WIN32)
    : camera{ nullptr }
    , formatList{ &emptyFormatList }
#endif
  {
    openImage(filename);
  }

  ///
  /// デストラクタ
  ///
  virtual ~Capture()
  {
    close();
  }

  ///
  /// 画像ファイルを開く
  ///
  /// @param 開く画像ファイル名
  /// @return 開くことができたら true
  ///
  bool openImage(const std::string& filename);

  ///
  /// 動画ファイルを開く
  ///
  /// @param filename 開く動画ファイル名
  /// @param backend バックエンドの種類
  /// @return 開くことができたら true
  ///
  bool openMovie(const std::string& filename,
#if defined(_WIN32)
    cv::VideoCaptureAPIs backend = cv::CAP_ANY);
#else
    cv::VideoCaptureAPIs backend = cv::CAP_FFMPEG);
#endif

#if defined(_WIN32)
  ///
  /// キャプチャデバイスを開く (Windows用: MSMF)
  ///
  /// @param deviceNumber 開くデバイス番号
  /// @return 開くことができたら true
  ///
  bool openDevice(int deviceNumber);

  ///
  /// 使用可能なビデオフォーマットの表示名のリストを得る
  ///
  /// @return 使用可能なビデオフォーマットの表示名のリスト
  ///
  const auto& getFormatList() const
  {
    return *formatList;
  }

  ///
  /// ビデオフォーマット選択
  ///
  /// @param index 選択するビデオフォーマットのインデックス
  ///
  bool select(int index);

  ///
  /// キャプチャデバイスのビデオフォーマットの表示名のリストを一時的に更新する
  ///
  /// @param deviceNumber 一時的に開くデバイス番号
  ///
  void updateFormatList(int deviceNumber);
#else
  ///
  /// キャプチャデバイスを開く (Windows以外用: OpenCV)
  ///
  /// @param deviceNumber 開くデバイス番号
  /// @param size キャプチャデバイスのフレームの解像度
  /// @param fps キャプチャデバイスのフレームレート
  /// @param backend バックエンドの種類
  /// @param fourcc コーデックの 4 文字
  /// @return 開くことができたら true
  ///
  bool openDevice(int deviceNumber,
    std::array<int, 2>& size, double& fps,
    cv::VideoCaptureAPIs backend,
    char* fourcc);
#endif

  ///
  /// キャプチャ開始
  ///
  void start();

  ///
  /// キャプチャ終了
  ///
  void stop();

  ///
  /// キャプチャデバイスを閉じる
  ///
  void close();

  ///
  /// キャプチャ中か否か
  ///
  /// @return キャプチャ中なら true
  ///
  explicit operator bool() const
  {
    return camera && camera->isRunning();
  }

  ///
  /// キャプチャデバイスが有効かどうか
  ///
  bool isOpend() const
  {
    return bool(camera);
  }

  ///
  /// 現在開いているのが静止画像かどうか
  ///
  bool isImage() const
  {
    return camera && dynamic_cast<const CamImage*>(camera.get()) != nullptr;
  }

  ///
  /// キャプチャデバイスのフレームの解像度を得る
  /// 
  /// @return キャプチャデバイスのフレームの解像度
  ///
  std::array<int, 2> getSize() const;

  ///
  /// キャプチャデバイスのフレームレートを得る
  /// 
  /// @return キャプチャデバイスのフレームレート
  ///
  double getFps() const;

  ///
  /// フレームを取得する
  ///
  /// @param buffer 取得したフレームを格納するバッファ
  ///
  void retrieve(Buffer& buffer);
};
