#pragma once

///
/// キャプチャデバイス関連の基底クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"

// OpenCV
#include <opencv2/opencv.hpp>
#if defined(_MSC_VER)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_core" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_imgproc" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_imgcodecs" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_videoio" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_calib3d" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_aruco" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_objdetect" CV_VERSION_STR CV_EXT_STR)
#endif

// 非同期処理
#include <thread>
#include <mutex>

///
/// チャネル数からフォーマットを求める
///
/// @param channels フレームのチャネル数
/// @return テクスチャのフォーマット
///
inline auto channelsToFormat(int channels)
{
  // OpenGL のテクスチャフォーマットのリスト
  static constexpr GLenum toFormat[]{ GL_RED, GL_RG, GL_RGB, GL_RGBA };

  // フォーマットを返す
  return toFormat[(channels - 1) & 3];
}

///
/// フォーマットからチャネル数を求める
///
/// @param format テクスチャのフォーマット
/// @return フレームのチャネル数
///
inline auto formatToChannels(GLenum format)
{
  switch (format)
  {
  case GL_RED:
    return 1;
  case GL_RG:
    return 2;
  case GL_RGB:
    return 3;
  case GL_RGBA:
    return 4;
  default:
    assert(false);
    break;
  }

  return 0;
};

///
/// キャプチャデバイス関連の基底クラス
///
class Camera
{
protected:

  /// キャプチャしたフレームの画素数
  std::array<GLsizei, 2> resolution;

  /// ムービーファイルの総フレーム数
  double frames;

  /// キャプチャした画像のフレーム間隔
  double interval;

  /// キャプチャするムービーのチャネル数
  int channels;

  /// フレームを GPU に送るために使う一時メモリ
  std::vector<GLubyte> pixels;

  /// 新しいフレームが取得されたら true
  bool captured;

  /// キャプチャを非同期に行うためのスレッド
  std::thread thr;

  /// キャプチャを非同期に行うためのミューテックス
  std::mutex mtx;

  /// キャプチャスレッドが実行中なら true
  bool run;

  ///
  /// フレームを一時メモリにコピーする
  ///
  /// @param frame コピーするフレーム
  ///
  void copyFrame(const cv::Mat& frame)
  {
    // 取り出したフレームのチャネル数を保存する
    channels = frame.channels();

    // 転送用の一時メモリのサイズを求める
    const auto size{ resolution[0] * resolution[1] * channels };

    // フレームの大きさを求める
    const auto length{ frame.cols * frame.rows * channels };

    // コピーするサイズを決める
    const auto count{ std::min(size, length) };

    // 転送用に必要なメモリサイズが以前と違ったらメモリを確保しなおす
    if (pixels.size() != static_cast<decltype(pixels.size())>(count)) pixels.resize(count);

    // R と B を入れ替えてコピーする
    if (channels > 0)
    {
      if (channels < 3)
        std::copy(frame.data, frame.data + count, pixels.data());
      else
      {
        for (int i = 0; i < count; i += channels)
        {
          pixels.data()[i + 0] = frame.data[i + 2];
          pixels.data()[i + 1] = frame.data[i + 1];
          pixels.data()[i + 2] = frame.data[i + 0];
          if (channels >= 3) pixels.data()[i + 3] = 1;
        }
      }
    }
  }

  ///
  /// フレームをキャプチャする
  ///
  virtual void capture()
  {
  }

public:

  /// ムービーファイルのインポイント
  double in;

  /// ムービーファイルのアウトポイント
  double out;

  ///
  /// コンストラクタ
  ///
  Camera()
    : resolution{ 640, 480 }
    , frames{ -1.0 }
    , interval{ 10.0 }
    , channels{ 3 }
    , captured{ false }
    , run{ false }
    , in{ -1.0 }
    , out{ -1.0 }
  {
  }

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param camera コピー元
  ///
  Camera(const Camera& camera) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Camera()
  {
    // キャプチャスレッドを停止する
    stop();

    // 一時メモリを空にする
    pixels.clear();
  }

  ///
  /// 代入演算子は使用しない
  ///
  /// @param camera 代入元
  ///
  Camera& operator=(const Camera& camera) = delete;

  ///
  /// キャプチャスレッドを起動する
  ///
  void start()
  {
    // スレッドが起動状態であることを記録しておく
    run = true;

    // スレッドを起動する
    thr = std::thread([&] { this->capture(); });
  }

  ///
  /// キャプチャスレッドを停止する
  ///
  void stop()
  {
    // キャプチャスレッドが実行中なら
    if (run)
    {
      // キャプチャスレッドのループを止めて
      run = false;

      // 合流する
      thr.join();
    }

    // フレームを取得していないことにする
    captured = false;
  }

  ///
  /// キャプチャデバイスをロックしてフレームをピクセルバッファオブジェクトに転送する
  ///
  /// @param buffer 転送先のピクセルバッファオブジェクト
  ///
  void transmit(GLuint buffer)
  {
    // 新しいフレームが取得されているときカメラのロックが成功したら
    if (captured && mtx.try_lock())
    {
      // フレームをピクセルバッファオブジェクトに転送して
      glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
      glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, pixels.size(), pixels.data());
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

      // 次のフレームの取得を待つ
      captured = false;

      // キャプチャデバイスのロックを解除する
      mtx.unlock();
    }
  }

  ///
  /// キャプチャデバイスをロックしてフレームをメモリに転送する
  ///
  /// @param buffer 転送先のメモリ
  ///
  void transmit(decltype(pixels)& buffer)
  {
    // 新しいフレームが取得されているときカメラのロックが成功したら
    if (captured && mtx.try_lock())
    {
      // フレームをメモリに転送して
      const auto size{ std::max(pixels.size(), buffer.size()) };
      memcpy(buffer.data(), pixels.data(), size);

      // 次のフレームの取得を待つ
      captured = false;

      // キャプチャデバイスのロックを解除する
      mtx.unlock();
    }
  }

  ///
  /// キャプチャスレッドが実行中かどうか調べる
  ///
  /// @return キャプチャ中なら true
  ///
  auto isRunning() const
  {
    return run;
  }

  ///
  /// キャプチャ中のフレームの横の画素数を得る
  ///
  /// @return キャプチャ中のフレームの横の画素数
  ///
  auto getWidth() const
  {
    return resolution[0];
  }

  ///
  /// キャプチャ中のフレームの縦の画素数を得る
  ///
  /// @return キャプチャ中のフレームの縦の画素数
  ///
  auto getHeight() const
  {
    return resolution[1];
  }

  ///
  /// ムービーファイルの総フレーム数を得る
  ///
  /// @return ムービーファイルの総フレーム数
  ///
  auto getFrames() const
  {
    return frames;
  }

  ///
  /// キャプチャデバイスのフレームレートを得る
  ///
  /// @return キャプチャデバイスのフレームレート
  ///
  auto getFps() const
  {
    return 1000.0 / interval;
  }

  ///
  /// キャプチャしたフレームのチャネル数を調べる
  ///
  /// @return キャプチャ下フレームのチャネル数
  ///
  auto getChannels() const
  {
    return channels;
  }

  ///
  /// 露出を上げる
  ///
  virtual void increaseExposure() {}

  ///
  /// 露出を下げる
  ///
  virtual void decreaseExposure() {}

  ///
  /// 利得を上げる
  ///
  virtual void increaseGain() {}

  ///
  /// 利得を下げる
  ///
  virtual void decreaseGain() {}
};
