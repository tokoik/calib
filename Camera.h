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
/// キャプチャデバイス関連の基底クラス
///
class Camera
{
protected:

  /// ムービーファイルの総フレーム数
  double total;

  /// キャプチャした画像のフレーム間隔
  double interval;

  /// OpenCV のキャプチャデバイスから取得したフレーム
  cv::Mat frame;

  /// キャプチャフレームを GPU に送るために用いる一時メモリ
  std::vector<GLubyte> pixels;

  /// 新しいフレームが取得されたら true
  bool captured;

  /// キャプチャを非同期に行うためのスレッド
  std::thread thr;

  /// キャプチャを非同期に行うためのミューテックス
  std::mutex mtx;

  /// キャプチャスレッドが実行中なら true
  bool running;

  ///
  /// フレームを一時メモリにコピーする
  ///
  /// @param frame コピーするフレーム
  ///
  void copyFrame()
  {
    // フレームの大きさを求める
    const auto length{ frame.cols * frame.rows * frame.channels() };

    // 転送用に必要なメモリサイズが以前と違ったらメモリを確保しなおす
    if (static_cast<int>(pixels.size()) != length) pixels.resize(length);

    // データをコピーする
    std::copy(frame.data, frame.data + length, pixels.data());
  }

  ///
  /// フレームをキャプチャする
  ///
  virtual void capture()
  {
    // 継承されていなけれはスレッドを起動しない
    stop();
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
    : total{ -1.0 }
    , interval{ 10.0 }
    , captured{ false }
    , running{ false }
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

    // キャプチャデバイスの使用を終了する
    close();

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
    running = true;

    // スレッドを起動する
    thr = std::thread([&] { this->capture(); });
  }

  ///
  /// キャプチャスレッドを停止する
  ///
  void stop()
  {
    // キャプチャスレッドが実行中なら
    if (running)
    {
      // キャプチャスレッドのループを止めて
      running = false;

      // 合流する
      thr.join();
    }
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
  /// キャプチャデバイスの使用を終了する
  ///
  virtual void close()
  {
    // フレームを取得していないことにする
    captured = false;
  }

  ///
  /// キャプチャスレッドが実行中かどうか調べる
  ///
  /// @return キャプチャ中なら true
  ///
  auto isRunning() const
  {
    return running;
  }

  ///
  /// キャプチャしたフレームのサイズを得る
  ///
  /// @return キャプチャしたフレームのサイズ
  ///
  std::array<int, 2> getSize() const
  {
    return std::array<int, 2>{ frame.cols, frame.rows };
  }

  ///
  /// キャプチャしたフレームの横の画素数を得る
  ///
  /// @return キャプチャ中のフレームの横の画素数
  ///
  auto getWidth() const
  {
    return frame.cols;
  }

  ///
  /// キャプチャしたフレームの縦の画素数を得る
  ///
  /// @return キャプチャ中のフレームの縦の画素数
  ///
  auto getHeight() const
  {
    return frame.rows;
  }

  ///
  /// キャプチャしたフレームのチャネル数を調べる
  ///
  /// @return キャプチャしたフレームのチャネル数
  ///
  auto getChannels() const
  {
    return frame.channels();
  }

  ///
  /// ムービーファイルの総フレーム数を得る
  ///
  /// @return ムービーファイルの総フレーム数
  ///
  auto getFrames() const
  {
    return total;
  }

  ///
  /// キャプチャデバイスのフレームレートを得る
  ///
  /// @return キャプチャデバイスのフレームレート
  ///
  virtual double getFps() const
  {
    return 1000.0 / interval;
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
