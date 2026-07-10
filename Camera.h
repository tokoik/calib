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
#include <vector>

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

  /// 解像度（幅）
  int width;

  /// 解像度（高さ）
  int height;

  /// チャンネル数
  int channels;

  /// キャプチャデバイスから取得したフレーム
  std::vector<GLubyte> frame;

  /// キャプチャしたフレームを GPU に送るために用いる一時メモリ
  std::vector<GLubyte> image;

  /// 新しいフレームが取得されたら true
  bool captured;

  /// キャプチャを非同期に行うためのスレッド
  std::thread thr;

  /// キャプチャを非同期に行うためのミューテックス
  std::mutex mtx;

  /// キャプチャスレッドが実行中なら true
  bool running;

  ///
  /// フレームをキャプチャする
  ///
  /// @description
  /// スレッドを起動するための仮想関数。
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
    , width{ 0 }
    , height{ 0 }
    , channels{ 0 }
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
    image.clear();
  }

  ///
  /// 代入演算子は使用しない
  ///
  /// @param camera 代入元のカメラ
  /// @return 代入後のこのカメラの参照
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
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    if (captured && lock.owns_lock())
    {
      // データの長さを計算して
      const auto length{ image.size() * sizeof(GLubyte) };

      // フレームをピクセルバッファオブジェクトに転送して
      glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
      glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, length, image.data());
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

       // 次のフレームの取得を待つ
      captured = false;
    }
  }

  ///
  /// キャプチャデバイスをロックしてフレームをメモリに転送する
  ///
  /// @param buffer 転送先のメモリ
  ///
  void transmit(std::vector<GLubyte>& buffer)
  {
    // 新しいフレームが取得されているときカメラのロックが成功したら
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    if (captured && lock.owns_lock())
    {
      // データの長さを計算して
      const auto length{ image.size() };

      // 呼び出し先のサイズを合わせてから
      buffer.resize(length);

      // フレームを呼び出し元にコピーして
      memcpy(buffer.data(), image.data(), length);

      // 次のフレームの取得を待つ
      captured = false;
    }
  }

  ///
  /// キャプチャデバイスをロックしてフレームをメモリに転送する
  ///
  /// @param buffer 転送先のメモリ（cv::Matなど）
  ///
  template <typename MatType>
  void transmit(MatType& buffer)
  {
    // 新しいフレームが取得されているときカメラのロックが成功したら
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    if (captured && lock.owns_lock())
    {
      // 呼び出し元にコピーして
      // cv::Mat の型番号 (CV_8UC1〜CV_8UC4) は (channels - 1) << 3 で表されます
      buffer.create(height, width, ((channels - 1) << 3));
      memcpy(buffer.data, image.data(), image.size());

      // 次のフレームの取得を待つ
      captured = false;
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
    return std::array<int, 2>{ width, height };
  }

  ///
  /// キャプチャしたフレームの横の画素数を得る
  ///
  /// @return キャプチャ中のフレームの横の画素数
  ///
  auto getWidth() const
  {
    return width;
  }

  ///
  /// キャプチャしたフレームの縦の画素数を得る
  ///
  /// @return キャプチャ中のフレームの縦の画素数
  ///
  auto getHeight() const
  {
    return height;
  }

  ///
  /// キャプチャしたフレームのチャネル数を調べる
  ///
  /// @return キャプチャしたフレームのチャネル数
  ///
  auto getChannels() const
  {
    return channels;
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
  virtual void increaseExposure()
  {
  }

  ///
  /// 露出を下げる
  ///
  virtual void decreaseExposure()
  {
  }

  ///
  /// 利得を上げる
  ///
  virtual void increaseGain()
  {
  }

  ///
  /// 利得を下げる
  ///
  virtual void decreaseGain()
  {
  }
};
