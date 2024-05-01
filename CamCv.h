#pragma once

///
/// OpenCV を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date December 27, 2022
///

// カメラ関連の処理
#include "Camera.h"

///
/// OpenCV を使ってビデオをキャプチャするクラス
///
class CamCv : public Camera
{
  /// OpenCV のキャプチャデバイス
  cv::VideoCapture camera;

  /// 現在のフレームの時刻
  double elapsedTime;

  /// 露出と利得
  int exposure, gain;

  ///
  /// キャプチャデバイスを初期化する
  ///
  /// @param initial_width キャプチャデバイスを開く際に期待するフレームの横の画素数
  /// @param initial_height キャプチャデバイスを開く際に期待するフレームの縦の画素数
  /// @param initial_fps キャプチャデバイスを開く際に期待するフレームフレームレート
  /// @param fourcc キャプチャデバイスを開く際に期待するコーデックの 4 文字
  /// @return キャプチャデバイスが使用可能なら true
  ///
  bool init(int initial_width, int initial_height, double initial_fps, const char* fourcc = "")
  {
    // カメラのコーデック・解像度・フレームレートを設定する
    if (fourcc[0] != '\0') camera.set(cv::CAP_PROP_FOURCC,
      cv::VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]));
    if (initial_width > 0) camera.set(cv::CAP_PROP_FRAME_WIDTH, initial_width);
    if (initial_height > 0) camera.set(cv::CAP_PROP_FRAME_HEIGHT, initial_height);
    if (initial_fps > 0.0) camera.set(cv::CAP_PROP_FPS, initial_fps);

    // fps が 0 なら逆数をカメラの遅延に使う
    const auto fps{ camera.get(cv::CAP_PROP_FPS) };
    if (fps > 0.0) interval = 1000.0 / fps;

    // ムービーファイルのインポイント・アウトポイントの初期値とフレーム数
    in = camera.get(cv::CAP_PROP_POS_FRAMES);
    out = total = camera.get(cv::CAP_PROP_FRAME_COUNT);

    // 経過時間
    elapsedTime = 0.0;

    // カメラから最初のフレームをキャプチャできなかったらカメラは使えない
    if (!camera.grab()) return false;

    // カメラの利得と露出を取得する
    gain = static_cast<GLsizei>(camera.get(cv::CAP_PROP_GAIN));
    exposure = static_cast<GLsizei>(camera.get(cv::CAP_PROP_EXPOSURE) * 10.0);

    // フレームを取り出してキャプチャ用のメモリを確保する
    camera.retrieve(frame);

#if defined(DEBUG)
    char codec[5]{ 0, 0, 0, 0, 0 };
    getCodec(codec);
    std::cerr << "in:" << in << ", out:" << out
      << ", width:" << frame.cols << ", height:" << frame.rows
      << ", fourcc: " << codec << "\n";
#endif

    // 取り出した転送用の一時メモリにデータを格納する
    copyFrame();

    // フレームがキャプチャされたことを記録する
    captured = true;

    // カメラが使える
    return true;
  }

  ///
  /// フレームをキャプチャする
  ///
  void capture()
  {
    // 再生開始時刻
    auto startTime{ glfwGetTime() };

    // スレッドが実行可の間
    while (running)
    {
      // フレームを取り出せたら true
      auto status{ (total <= 0.0 || camera.get(cv::CAP_PROP_POS_FRAMES) < out) && camera.grab() };

      // ムービーファイルでないかムービーファイルの終端でなければ次のフレームを取り出して
      if (status && camera.retrieve(frame))
      {
        // ピクセルバッファオブジェクトをロックしてから
        std::lock_guard lock{ mtx };

        // 転送用の一時メモリにデータを格納したら
        copyFrame();

        // 新しいフレームがキャプチャされたことを通知する
        captured = true;
      }

      // 遅延時間
      auto deferred{ 0.0 };

      // ムービーファイルから入力しているとき
      if (total > 0.0)
      {
        // ムービーファイルの終端に到達していたら
        if (!status)
        {
          // インポイントまで巻き戻す
          camera.set(cv::CAP_PROP_POS_FRAMES, in);

          // 経過時間を戻す
          elapsedTime = 0.0;

          // 開始時間を更新する
          startTime = glfwGetTime();
        }
        else
        {
          // 現在のフレームのインポイントからの経過時間
          const auto pos{ camera.get(cv::CAP_PROP_POS_MSEC) - in };

          // 再生位置の次のフレームの時刻に対する経過時間
          const auto now{ (elapsedTime + glfwGetTime() - startTime) * 1000.0 + interval };

          // 遅延時間はフレームの経過時間と現在の経過時間の差
          deferred = pos - now;
        }
      }

      // 遅延時間あれば
      if (deferred > 0.0)
      {
        // 待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(deferred)));
      }
    }

    // 再生停止までの経過時間を積算する
    elapsedTime += glfwGetTime() - startTime;
  }

public:

  ///
  /// コンストラクタ
  ///
  CamCv()
    : elapsedTime{ 0.0 }
    , exposure{ 0 }
    , gain{ 0 }
  {}

  ///
  /// デストラクタ
  ///
  virtual ~CamCv()
  {
  }

  ///
  /// キャプチャデバイスを開く
  ///
  /// @param device キャプチャデバイスの番号
  /// @param width キャプチャデバイスを開く際に期待するフレームの横の画素数, 0 ならお任せ
  /// @param height キャプチャデバイスを開く際に期待するフレームの縦の画素数, 0 ならお任せ
  /// @param fps キャプチャデバイスを開く際に期待するフレームフレームレート, 0 ならお任せ
  /// @param fourcc キャプチャデバイスを開く際に期待するコーデックの 4 文字, "" ならお任せ
  /// @return キャプチャデバイスが使用可能なら true
  ///
  auto open(int device, int width = 0, int height = 0, double fps = 0.0, const char* fourcc = "", int pref = cv::CAP_ANY)
  {
    // カメラを開いて初期化する
    return camera.open(device, pref) && init(width, height, fps, fourcc);
  }

  ///
  /// キャプチャデバイスの使用を終了する
  ///
  void close()
  {
    // キャプチャデバイスを開放する
    camera.release();

    // キャプチャデバイスを閉じる
    Camera::close();
  }

  ///
  /// ファイル / ネットワーク / GStreamer から入力する
  ///
  /// @param device 入力するファイルの名前
  /// @param width 入力するファイルを開く際に期待するフレームの横の画素数, 0 ならお任せ
  /// @param height 入力するファイルを開く際に期待するフレームの縦の画素数, 0 ならお任せ
  /// @param fps 入力するファイルを開く際に期待するフレームフレームレート, 0 ならお任せ
  /// @param fourcc 入力するファイルを開く際に期待するコーデックの 4 文字, "" ならお任せ
  /// @return 入力するファイルが使用可能なら true
  ///
  auto open(const std::string& file, int width = 0, int height = 0, double fps = 0.0, const char* fourcc = "", int pref = cv::CAP_ANY)
  {
    // ファイル／ネットワークを開いて初期化する
    return camera.open(file, pref) && init(width, height, fps, fourcc);
  }

  ///
  /// キャプチャしたフレームのフレームレートを得る
  ///
  /// @return キャプチャしたフレームのフレームレート
  ///
  virtual double getFps() const
  {
    return camera.get(cv::CAP_PROP_FPS);
  }

  ///
  /// コーデックを調べる
  ///
  /// @return 使用しているコーデックを表す 4 バイト
  ///
  auto getCodec() const
  {
    return static_cast<unsigned int>(camera.get(cv::CAP_PROP_FOURCC));
  }

  ///
  /// コーデックを調べる
  ///
  /// @param forcc 使用しているコーデックを表す 4 文字の格納先
  ///
  void getCodec(char* fourcc) const
  {
    auto cc{ getCodec() };
    for (int i = 0; i < 4; ++i)
    {
      fourcc[i] = static_cast<char>(cc & 0x7f);
      if (!isalnum(fourcc[i])) fourcc[i] = '?';
      cc >>= 8;
    }
  }

  ///
  /// ファイルから入力しているとき現在のフレーム番号を得る
  ///
  /// @return 現在入力しているフレーム番号
  ///
  auto getPosition() const
  {
    return camera.get(cv::CAP_PROP_POS_FRAMES);
  }

  ///
  /// ファイルから入力しているとき再生位置を指定する
  ///
  /// @param frame 再生位置
  ///
  void setPosition(double frame)
  {
    camera.set(cv::CAP_PROP_POS_FRAMES, frame);
  }

  ///
  /// 露出を設定する
  ///
  /// @param exposure 設定する露出
  ///
  void setExposure(double exposure)
  {
    if (camera.isOpened()) camera.set(cv::CAP_PROP_EXPOSURE, exposure);
  }

  ///
  /// 露出を一段階上げる
  ///
  void increaseExposure()
  {
    setExposure(++exposure * 0.1);
  }

  ///
  /// 露出を一段階下げる
  ///
  void decreaseExposure()
  {
    setExposure(--exposure * 0.1);
  }

  ///
  /// 利得を設定する
  ///
  /// @param gain 設定する利得
  ///
  void setGain(double gain)
  {
    if (camera.isOpened()) camera.set(cv::CAP_PROP_GAIN, gain);
  }

  ///
  /// 利得を一段階上げる
  ///
  void increaseGain()
  {
    setGain(++gain);
  }

  ///
  /// 利得を一段階下げる
  ///
  void decreaseGain()
  {
    setGain(--gain);
  }
};
