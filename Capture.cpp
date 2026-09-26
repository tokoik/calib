///
/// キャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date Aplil 3, 2023
///
#include "Capture.h"

#if defined(_WIN32)
/// フォーマットを提供できない場合に返す空のリスト
const std::vector<CaptureFormat> Capture::emptyFormatList;
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

  // ビデオフォーマットを選択する
  return camera->selectFormat(index);
}

void Capture::updateFormatList(int deviceNumber)
{
  // 実際の入力状態を変更せずに選択肢だけ取得する一時カメラ
  CamMf temp;

  // デバイスを遅延初期化で開く
  if (temp.open(deviceNumber, false))
  {
    // 開けたら列挙されたフォーマットリストを保存する
    deviceFormatList = temp.getFormatList();
    temp.close();
  }
  else
  {
    // 開けなかったらフォーマットリストを空にする
    deviceFormatList.clear();
  }
}

//
// フォーマットリストを取り出す
//
const std::vector<CaptureFormat>& Capture::getFormatList() const
{
  // 現在開いているカメラが有効かつフォーマットを保持している場合はその一覧を返し、
  // 静止画像 (CamImage) 表示中やカメラ未開始時は事前取得済みのデバイスフォーマット一覧を返す
  if (camera && !camera->getFormatList().empty())
  {
    return camera->getFormatList();
  }
  return deviceFormatList;
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

#if defined(USE_LIBCAMERA)
  // libcamera バックエンドの場合
  if (backend == CAP_LIBCAMERA)
  {
    auto camLibcam{ std::make_unique<CamLibcam>(deviceNumber, size[0], size[1], fps) };
    if (camLibcam->isOpened())
    {
      size[0] = camLibcam->getWidth();
      size[1] = camLibcam->getHeight();
      fps = camLibcam->getFps();
      if (fourcc)
      {
        const auto name{ camLibcam->getPixelFormatName() };
        std::strncpy(fourcc, name.c_str(), 4);
        fourcc[4] = '\0';
      }
      camera = std::move(camLibcam);
      return true;
    }
    return false;
  }
#endif

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
// 新しいフレームを GPU の PBO に取得する
//
bool Capture::retrieve(Buffer& buffer)
{
  // キャプチャデバイスが無効なら失敗
  if (!camera) return false;

  // フレームデータをロックして PBO に転送する
  return camera->lockFrame([&buffer](const std::uint8_t* data, size_t length, int width, int height, int channels) {
    buffer.create(width, height, channels);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer.getBufferName());
    glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, length, data);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  });
}

//
// 新しいフレームを CPU のメモリに取得する
//
bool Capture::retrieve(cv::Mat& frame)
{
  // OpenCV 補正を選んだときだけ使用し、PBO へ送る前の画像を cv::Mat として得る。
  if (!camera) return false;

  // フレームデータをロックして cv::Mat にコピーする
  return camera->lockFrame([&frame](const std::uint8_t* data, size_t length, int width, int height, int channels) {
    frame.create(height, width, ((channels - 1) << 3));
    const auto copySize{ std::min(static_cast<size_t>(frame.total() * frame.elemSize()), length) };
    std::memcpy(frame.data, data, copySize);
  });
}
