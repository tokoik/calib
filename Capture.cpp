///
/// キャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date Aplil 3, 2023
///
#include "Capture.h"

#if defined(_WIN32)
/// 空のビデオフォーマットの表示名のリスト
const std::vector<std::string> Capture::emptyFormatList;
#endif

//
// 画像ファイルを開く
//
bool Capture::openImage(const std::string& filename)
{
  // 新しいキャプチャデバイスを作成したら
  auto camImage{ std::make_unique<CamImage>() };

  // キャプチャデバイスを開く
  if (camImage->open(filename))
  {
    // このキャプチャデバイスを使うことにする
    camera = std::move(camImage);
    return true;
  }

  // 開けなかった
  return false;
}

//
// 動画ファイルを開く
//
bool Capture::openMovie(const std::string& filename,
  cv::VideoCaptureAPIs backend)
{
  // 新しいキャプチャデバイスを作成したら
  auto camCv{ std::make_unique<CamCv>() };

  // キャプチャデバイスを開く
  if (camCv->open(filename, 0, 0, 0.0, "", backend))
  {
    // このキャプチャデバイスを使うことにする
    camera = std::move(camCv);
    return true;
  }

  // 開けなかった
  return false;
}

#if defined(_WIN32)
//
// デバイスを開く (Windows用: MSMF)
//
bool Capture::openDevice(int deviceNumber)
{
  // 既にカメラが有効なら一旦閉じる
  if (camera) camera->close();

  // 新しいキャプチャデバイスを作成したら
  auto camMf{ std::make_unique<CamMf>() };

  // このデバイスをデバイス番号で開いて
  if (camMf->open(deviceNumber, false))
  {
    // このキャプチャデバイスを使うことにする
    camera = std::move(camMf);

    // 開けた
    return true;
  }

  // カメラを無効にしておく
  camera.reset();

  // 開けなかった
  return false;
}

//
// ビデオフォーマット選択
//
bool Capture::select(int index)
{
  // カメラが有効でなければ戻る
  if (!camera) return false;

  // バックエンドが Microsoft Media Foundation でなければ戻る
  auto camMf{ dynamic_cast<CamMf*>(camera.get()) };
  if (!camMf) return true; // Media Foundation 以外なら常に true
  
  // ビデオフォーマットを選択する
  return camMf->select(index);
}

void Capture::updateFormatList(int deviceNumber)
{
  CamMf tempCam;
  if (tempCam.open(deviceNumber, false))
  {
    deviceFormatList = tempCam.getFormatList();
    tempCam.close();
  }
  else
  {
    deviceFormatList.clear();
  }
}

const std::vector<std::string>& Capture::getFormatList() const
{
  auto camMf{ dynamic_cast<const CamMf*>(camera.get()) };
  return camMf ? camMf->getFormatList() : deviceFormatList;
}
#else
//
// デバイスを開く (Windows以外用: OpenCV)
//
bool Capture::openDevice(int deviceNumber, std::array<int, 2>& size, double& fps,
  cv::VideoCaptureAPIs backend, char* fourcc)
{
  // 既にカメラが有効なら一旦閉じる
  if (camera) camera->close();

  // 新しいキャプチャデバイスを作成したら
  auto camCv{ std::make_unique<CamCv>() };

  // このデバイスをデバイス番号で開いて
  if (camCv->open(deviceNumber, size[0], size[1], fps, fourcc, backend))
  {
    // 実際に開いた設定を書き戻す
    size[0] = camCv->getWidth();
    size[1] = camCv->getHeight();
    fps = camCv->getFps();
    camCv->getCodec(fourcc);

    // このキャプチャデバイスを使うことにする
    camera = std::move(camCv);
    return true;
  }

  // 開けなかった
  return false;
}
#endif

//
// キャプチャ開始
//
void Capture::start()
{
  // キャプチャデバイスが有効ならキャプチャスレッドを起動する
  if (camera) camera->start();
}

//
// キャプチャ終了
//
void Capture::stop()
{
  // キャプチャデバイスが有効ならキャプチャスレッドを停止する
  if (camera) camera->stop();
}

//
// キャプチャデバイスを閉じる
//
void Capture::close()
{
  // キャプチャデバイスが有効ならキャプチャスレッドを停止する
  if (camera)
  {
    camera->close();
    camera.reset();
  }
}

//
// キャプチャデバイスの解像度とフレームレートを取得する
// 
std::array<int, 2> Capture::getSize() const
{
  // キャプチャデバイスが有効ならその解像度を返す
  return camera ? camera->getSize() : std::array<int, 2>{ 0, 0 };
}

//
// キャプチャデバイスのフレームレートを得る
// 
double Capture::getFps() const
{
  // キャプチャデバイスが有効ならそのフレームレートを返す
  return camera ? camera->getFps() : 0.0;
}

//
// フレームを取得する
//
void Capture::retrieve(Buffer& buffer)
{
  // キャプチャデバイスが有効なら
  if (camera)
  {
    // バッファのサイズを取得したフレームのサイズに合わせて
    buffer.create(camera->getWidth(), camera->getHeight(), camera->getChannels());

    // バッファのピクセルバッファオブジェクトにフレームを転送する
    camera->transmit(buffer.getBufferName());
  }
}
