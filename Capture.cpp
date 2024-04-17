///
/// キャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date Aplil 3, 2023
///
#include "Capture.h"

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

//
// デバイスを開く
//
bool Capture::openDevice(int deviceNumber, std::array<int, 2>& size, double fps,
  cv::VideoCaptureAPIs backend, const char* fourcc)
{
  // 既にカメラが有効なら一旦閉じる
  if (camera) camera->close();

  // 新しいキャプチャデバイスを作成したら
  auto camCv{ std::make_unique<CamCv>() };

  // このデバイスをデバイス番号で開いて
  if (camCv->open(deviceNumber, size[0], size[1], fps, fourcc, backend))
  {
    // このキャプチャデバイスを使うことにする
    camera = std::move(camCv);
    return true;
  }

  // 開けなかった
  return false;
}

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
