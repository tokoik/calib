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
#include <atomic>
#include <algorithm>

///
/// キャプチャデバイスが対応するビデオフォーマットの表示・選択情報
///
/// @details
/// バックエンド固有のメディア型を UI が解釈し直さなくて済むよう、
/// 解像度、フレームレート、コーデックを表示用文字列として保持する。
/// index はバックエンドが保持する実フォーマットの選択に使用する。
///
struct CaptureFormat
{
  std::string resolution; ///< 解像度の表示文字列（例: "1920 x 1080"）
  std::string fps;        ///< フレームレートの表示文字列（例: "30.00"）
  std::string codec;      ///< コーデックの表示文字列（例: "NV12"）
  int index{ 0 };         ///< バックエンドのフォーマットリストにおける選択番号

  ///
  /// コンストラクタ
  ///
  /// @param resolution 解像度の表示文字列（例: "1920 x 1080"）
  /// @param fps フレームレートの表示文字列（例: "30.00"）
  /// @param codec コーデックの表示文字列（例: "NV12"）
  /// @param index バックエンドのフォーマットリストにおける選択番号
  ///
  CaptureFormat(const std::string& resolution, const std::string& fps,
    const std::string& codec, int index)
    : resolution{ resolution }
    , fps{ fps }
    , codec{ codec }
    , index{ index }
  {
  }
};

///
/// キャプチャデバイス関連の基底クラス
///
class Camera
{
protected:

  /// ムービーファイルの総フレーム数
  double total{ -1.0 };

  /// キャプチャした画像のフレーム間隔
  double interval{ 10.0 };

  /// 解像度（幅）
  int width{ 0 };

  /// 解像度（高さ）
  int height{ 0 };

  /// チャンネル数
  int channels{ 0 };

  /// キャプチャデバイスから取得したフレーム
  std::vector<GLubyte> frame;

  /// キャプチャしたフレームを GPU に送るために用いる一時メモリ
  std::vector<GLubyte> image;

  /// レイテンシを優先するなら true
  std::atomic<bool> prioritizeLatency{ false };

  /// 新しいフレームが取得されたら true
  std::atomic<bool> captured{ false };

  /// 再転送可能なフレーム（静止画像等）なら true
  bool reusableFrame{ false };

  /// キャプチャを非同期に行うためのスレッド
  std::thread thr;

  /// キャプチャを非同期に行うためのミューテックス
  std::mutex mtx;

  /// キャプチャスレッドが実行中なら true
  std::atomic<bool> running{ false };

  ///
  /// フレームをキャプチャする
  ///
  /// @details
  /// スレッドを起動するための仮想関数。
  ///
  virtual void capture()
  {
    // 継承されていなけれはスレッドを起動しない
    stop();
  }

public:

  /// ムービーファイルのインポイント
  double in{ -1.0 };

  /// ムービーファイルのアウトポイント
  double out{ -1.0 };

  ///
  /// コンストラクタ
  ///
  Camera() = default;

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
  virtual void stop()
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
  /// @return 新しいフレームを転送できたら true
  ///
  bool transmit(GLuint buffer)
  {
    // 新しいフレームが取得されているときカメラのロックが成功したら
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    if (lock.owns_lock() && captured)
    {
      // データの長さを計算して
      const auto expected_length{ static_cast<size_t>(width) * height * channels };
      const auto length{ std::min(image.size(), expected_length) * sizeof(GLubyte) };

      // フレームをピクセルバッファオブジェクトに転送して
      glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
      glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, length, image.data());
      glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

      // 動画・カメラ入力では次のフレームを待つ。静止画像は表示方式を
      // 切り替えた後も同じ内容を再転送できるよう取得済みの状態を維持する。
      if (!reusableFrame) captured = false;
      return true;
    }

    // カメラがロックできなかった
    return false;
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
    if (lock.owns_lock() && captured)
    {
      // データの長さを計算して
      const auto length{ image.size() };

      // 呼び出し先のサイズを合わせてから
      buffer.resize(length);

      // フレームを呼び出し元にコピーして
      memcpy(buffer.data(), image.data(), length);

      // 静止画像でなければ次のフレームの取得を待つ
      if (!reusableFrame) captured = false;
    }
  }

  ///
  /// キャプチャデバイスをロックしてフレームをメモリに転送する
  ///
  /// @param buffer 転送先のメモリ（cv::Matなど）
  /// @return 新しいフレームを転送できたら true
  ///
  template <typename MatType>
  bool transmit(MatType& buffer)
  {
    // 新しいフレームが取得されているときカメラのロックが成功したら
    std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    if (lock.owns_lock() && captured)
    {
      // 呼び出し元にコピーして
      // cv::Mat の型番号 (CV_8UC1〜CV_8UC4) は (channels - 1) << 3 で表されます
      buffer.create(height, width, ((channels - 1) << 3));
      const auto expected_length{ static_cast<size_t>(width) * height * channels };
      memcpy(buffer.data, image.data(), std::min(image.size(), expected_length));

      // 静止画像でなければ次のフレームの取得を待つ
      if (!reusableFrame) captured = false;
      return true;
    }

    // カメラがロックできなかった
    return false;
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
  bool isRunning() const
  {
    return running;
  }

  ///
  /// レイテンシ優先モードを設定する
  ///
  /// @param mode レイテンシを優先する場合は true
  ///
  void setPrioritizeLatency(bool mode)
  {
    prioritizeLatency = mode;
  }

  ///
  /// レイテンシ優先モードかどうか調べる
  ///
  /// @return レイテンシを優先する場合は true
  ///
  bool getPrioritizeLatency() const
  {
    return prioritizeLatency;
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
