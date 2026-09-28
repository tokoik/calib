#pragma once

///
/// メニューの描画クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 構成データ
#include "Config.h"

// 内部パラメータ
#include "Intrinsics.h"

// キャプチャデバイス
#include "Capture.h"

// 較正オブジェクト
#include "Calibration.h"

///
/// メニューの描画
///
class Menu
{
  /// 読み込み・保存の対象となる構成データへの参照
  Config& config;

  /// 設定データのコピー
  Settings settings;

#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(__APPLE__)
  /// バックエンドのリスト
  static const std::map<cv::VideoCaptureAPIs, const char*> backendList;

  /// コーデックのリスト
  static const std::vector<const char*> codecList;

  /// キャプチャデバイスのリスト
  static std::map <cv::VideoCaptureAPIs, std::vector<std::string>> deviceList;

  /// 読み込む動画ファイル名の履歴
  std::vector<std::string> fileHistory;

  ///
  /// キャプチャデバイスのリストを取り出す
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @return キャプチャデバイスのリスト
  ///
  const auto& getDeviceList(cv::VideoCaptureAPIs api) const
  {
    return deviceList.at(api);
  }

  ///
  /// キャプチャデバイスの数を調べる
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @return キャプチャデバイスの数
  ///
  auto getDeviceCount(cv::VideoCaptureAPIs api) const
  {
    return static_cast<int>(deviceList.at(api).size());
  }

  ///
  /// キャプチャデバイスの名前を調べる
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @param number キャプチャデバイスの番号
  /// @return キャプチャデバイスの名前
  ///
  const auto& getDeviceName(cv::VideoCaptureAPIs api, int number) const
  {
    static const std::string empty{};
    const auto& list{ deviceList.at(api) };
    return list.empty() ? empty : list[number];
  }
#endif

  /// 使用中の構成のキャプチャデバイス固有のパラメータのコピー
  Intrinsics intrinsics;

  /// キャプチャデバイス
  Capture& capture;

  /// 較正オブジェクト
  Calibration& calibration;

  /// 選択しているキャプチャデバイスの番号
  int deviceNumber{ 0 };

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  /// 選択しているビデオフォーマットの番号
  int formatNumber{ -1 };

  /// 使用可能なビデオフォーマットのリスト
  std::vector<CaptureFormat> availableFormats;

  /// 重複のない解像度のリスト
  std::vector<std::string> uniqueResolutions;

  /// 重複のないフレームレートのリスト
  std::vector<std::string> uniqueFpsList;

  /// 重複のないコーデックのリスト
  std::vector<std::string> uniqueCodecs;

  /// 現在選択されている解像度
  std::string currentRes;

  /// 現在選択されているフレームレート
  std::string currentFps;

  /// 現在選択されているコーデック
  std::string currentCodec;

  /// 最後に処理したデバイスの番号
  int lastDeviceNumber{ -1 };

  ///
  /// 構造化フォーマットから解像度、フレームレート、コーデックの選択肢を更新する
  ///
  void updateFormatDropdowns();
#else
  /// 選択しているコーデックの番号
  int codecNumber{ 0 };

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs backend{ cv::CAP_ANY };
#endif

#if !defined(__ANDROID__)
  /// 使用中の構成の番号
  int preferenceNumber{ 0 };

  /// キャプチャデバイスの姿勢
  GgMatrix pose{ ggIdentity() };

  /// メニューバーの高さ
  GLsizei menubarHeight{ 0 };
#endif

  /// 入力パネルの表示
  bool showInputPanel{ true };

  /// 較正パネルの表示
  bool showCalibrationPanel{ true };

  /// 終了するなら true
  bool quit{ false };

  /// エラーが無ければ nullptr
  mutable const char* errorMessage{ nullptr };

  ///
  /// キャプチャデバイスを開く
  ///
  /// @return 選択中のデバイスとフォーマットを適用できたら true
  bool openDevice();

#if !defined(__ANDROID__)
  ///
  /// 画像ファイルを開く
  ///
  void openImage();

  ///
  /// 動画ファイルを開く
  ///
  void openMovie();

  ///
  /// 構成ファイルを読み込む
  ///
  void loadConfig();

  ///
  /// 構成ファイルを保存する
  ///
  void saveConfig() const;

  ///
  /// 較正ファイルを読み込む
  ///
  void loadCalibration();

  ///
  /// 較正ファイルを保存する
  ///
  void saveParameters() const;

  ///
  /// 較正用の画像ファイルを取得する (複数選択)
  ///
  void recordFileCorners() const;

  ///
  /// 較正用の ChArUco Board を作成する
  ///
  void createCharuco() const;

  ///
  /// メインメニューバーを描画し、ファイル操作とパネル表示の要求を処理する
  ///
  void drawMainMenuBar();

  ///
  /// 投影方式と入力デバイスを設定する入力パネルを描画する
  ///
  void drawInputPanel();

  ///
  /// マーカー検出とカメラ較正を操作する較正パネルを描画する
  ///
  void drawCalibrationPanel();

  ///
  /// 保留中のエラーメッセージをダイアログとして描画する
  ///
  void drawErrorDialog();
#endif

public:

  /// レイテンシを優先するなら true
  bool prioritizeLatency{ false };

  /// ArUco Marker を検出するなら true
  bool detectMarker{ false };

  /// ChArUco Board を検出するなら true
  bool detectBoard{ false };

  /// 自動キャプチャを行うなら true
  bool autoCaptureEnabled{ false };

  /// 静止判定に必要な継続時間 (秒)
  float autoCaptureMinStableTime{ 0.6f };

  /// 姿勢変更クールダウン時間 (秒)
  float autoCaptureCooldown{ 1.5f };

  /// 姿勢変更クールダウンタイマー (秒)
  float autoCaptureCooldownTimer{ 0.0f };

  /// 自動キャプチャ時の音響フィードバックを行うなら true
  bool autoCaptureBeep{ true };

  /// 自動キャプチャのステータスメッセージ
  std::string autoCaptureStatusMessage;

  ///
  /// 自動キャプチャ処理を更新する
  ///
  /// @param deltaTime 前フレームからの経過時間 (秒)
  /// @return 自動記録が行われたら true
  ///
  bool updateAutoCapture(float deltaTime);

  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  /// @param capture 入力フレームを取得するキャプチャデバイス
  /// @param calibration 較正オブジェクト
  ///
  Menu(Config& config, Capture& capture, Calibration& calibration);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param menu コピー元
  ///
  Menu(const Menu& menu) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Menu();

  ///
  /// 代入演算子は使用しない
  ///
  /// @param menu 代入元のメニュー
  /// @return 代入後のこのメニューの参照
  ///
  Menu& operator=(const Menu& menu) = delete;

  ///
  /// 処理を継続するかどうか調べる
  ///
  /// @return 処理を継続するなら true
  /// 
  explicit operator bool() const
  {
    return !quit;
  }

#if !defined(__ANDROID__)
  ///
  /// キャプチャデバイスの姿勢を得る
  ///
  /// @return 図形の姿勢
  ///
  const auto& getPose() const
  {
    return pose;
  }
#endif

  ///
  /// 選択するキャプチャデバイスの番号を設定する
  ///
  /// @param number デバイス番号
  ///
  void setDeviceNumber(int number)
  {
    deviceNumber = number;
  }

  ///
  /// 選択されているキャプチャデバイスの番号を得る
  ///
  /// @return デバイス番号
  ///
  int getDeviceNumber() const
  {
    return deviceNumber;
  }

  ///
  /// 選択中の入力設定を適用してキャプチャを開始する
  ///
  /// @return デバイスを開いてキャプチャを開始できたら true
  ///
  bool startCapture();

  ///
  /// 設定データを得る
  ///
  /// @return 設定データへの参照
  ///
  const Settings& getSettings() const { return settings; }
  Settings& getSettings() { return settings; }

  ///
  /// 内部パラメータを得る
  ///
  /// @return 内部パラメータへの参照
  ///
  const Intrinsics& getIntrinsics() const { return intrinsics; }
  Intrinsics& getIntrinsics() { return intrinsics; }

#if !defined(__ANDROID__)
  ///
  /// 選択中の投影方式とその内部パラメータを同期する
  ///
  /// @param index 新しく選択する投影方式の番号
  /// @details 投影方式固有の画角と中心位置を反映し、入力が開いている場合は
  /// 実際のキャプチャ解像度だけを維持する。入力オープン時に計算した初期画角は
  /// この操作によって投影方式の設定値へ戻る。
  ///
  void selectPreference(int index);

  ///
  /// 指定した番号の構成を調べる
  ///
  /// @param i 構成の番号
  /// @return 指定した投影方式への読み取り専用参照
  ///
  const auto& getPreference(int i) const
  {
    return config.getPreferences()[i];
  }

  ///
  /// 現在選択中の投影方式を調べる
  ///
  /// @return 現在選択中の投影方式への読み取り専用参照
  ///
  const auto& getPreference() const
  {
    return getPreference(preferenceNumber);
  }

  ///
  /// 現在選択されている投影方式の番号を得る
  ///
  /// @return 投影方式の番号
  ///
  int getPreferenceNumber() const { return preferenceNumber; }

  ///
  /// 投影方式の総数を得る
  ///
  /// @return 投影方式の数
  ///
  int getPreferenceCount() const { return static_cast<int>(config.getPreferences().size()); }

  ///
  /// 姿勢の回転行列を更新する
  ///
  void updatePose()
  {
    pose = ggRotateY(settings.euler[1]).rotateX(settings.euler[0]).rotateZ(settings.euler[2]);
  }

  ///
  /// 姿勢と焦点距離を設定値から初期値へ復帰する
  ///
  void resetPose()
  {
    settings.euler = config.getSettings().euler;
    settings.focal = config.getSettings().focal;
    settings.focalRange = config.getSettings().focalRange;
    updatePose();
  }

  ///
  /// メニューバーの高さを得る
  ///
  /// @return メニューバーの高さ
  /// 
  auto getMenubarHeight() const
  {
    return menubarHeight;
  }
#else
  int getPreferenceNumber() const { return 0; }
  int getPreferenceCount() const { return 0; }
  void updatePose() {}
  void resetPose()
  {
    settings.euler = config.getSettings().euler;
    settings.focal = config.getSettings().focal;
    settings.focalRange = config.getSettings().focalRange;
  }
  auto getMenubarHeight() const { return 0; }
#endif

  ///
  /// 入力画像に合わせて内部パラメータを初期化する
  ///
  /// @param size 開かれた入力フレームの解像度
  ///
  /// @details 実解像度を反映し、現在の焦点距離から画像全体が見やすい初期画角を
  /// 計算する。中心位置は投影方式の設定値を維持する。
  /// 
  void initializeInputIntrinsics(const std::array<int, 2>& size);

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  ///
  /// 利用可能なカメラ解像度の総数を取得する
  ///
  /// @return 解像度の数
  ///
  int getResolutionCount() const
  {
    return static_cast<int>(uniqueResolutions.size());
  }

  ///
  /// インデックス指定で利用可能なカメラ解像度を取得する
  ///
  /// @param index 解像度インデックス (0 <= index < getResolutionCount())
  /// @return 解像度文字列 (例: "1280 x 720"、範囲外なら空文字列)
  ///
  std::string getResolutionByIndex(int index) const
  {
    return (index >= 0 && index < static_cast<int>(uniqueResolutions.size())) ? uniqueResolutions[index] : std::string{};
  }

  ///
  /// 現在選択されているカメラ解像度を取得する
  ///
  /// @return 現在の解像度文字列 (例: "1280 x 720")
  ///
  const std::string& getCurrentResolution() const
  {
    return currentRes;
  }

  ///
  /// カメラ解像度を選択する
  ///
  /// @param resolution 選択する解像度文字列 (例: "1280 x 720")
  /// @return 変更に成功したら true
  ///
  bool selectResolution(const std::string& resolution);
#endif

  ///
  /// 検出する ChArUco Board のマス目の横と縦の数を得る
  ///
  /// @return 検出する ChArUco Board のマス目の横と縦の数
  ///
  const auto& getCheckerSize() const
  {
    return settings.checkerSize;
  }

  ///
  /// 検出する ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さを得る
  ///
  /// @return 検出する ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さ
  ///
  const auto& getCheckerLength() const
  {
    return settings.checkerLength;
  }

  ///
  /// 検出する ArUco Marker の一辺の長さを得る
  ///
  /// @return 検出する ArUco Marker の一辺の長さ
  ///
  auto getMarkerLength() const
  {
    return settings.markerLength;
  }

#if !defined(__ANDROID__)
  ///
  /// シェーダを設定する
  ///
  /// @param aspect 表示領域の縦横比
  /// @return 描画すべきメッシュの横と縦の格子点数
  ///
  /// @note
  /// 格子点数は画角 aspect と展開用メッシュのサンプル点数 samples から求める。
  ///
  std::array<GLsizei, 2> setup(GLfloat aspect) const;

  ///
  /// 指定した姿勢でシェーダを設定する
  ///
  /// @param aspect 表示領域の縦横比
  /// @param viewPose メニューの補正姿勢へ追加する視点姿勢
  /// @return 描画すべきメッシュの横と縦の格子点数
  ///
  std::array<GLsizei, 2> setup(GLfloat aspect, const gg::GgMatrix& viewPose) const;

  ///
  /// メニューを描画する
  ///
  void draw();
#endif

  ///
  /// 画像の保存
  ///
  /// @param image 保存する画像データ
  /// @param filename 保存する画像ファイル名のテンプレート
  ///
  void saveImage(const cv::Mat& image, const std::string& filename = "*.jpg") const;
};
