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

// キャプチャデバイス
#include "Capture.h"

// 較正オブジェクト
#include "Calibration.h"

///
/// メニューの描画
///
class Menu
{
  /// オリジナルの構成データの参照
  const Config& config;

  /// 設定データのコピー
  Settings settings;

#if !defined(_WIN32)
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
  int deviceNumber;

#if defined(_WIN32)
  /// 選択しているビデオフォーマットの番号
  int formatNumber;

  /// ビデオフォーマットの詳細を保持する構造体
  struct FormatInfo
  {
    std::string resolution; // "640 x 480"
    std::string fps;        // "30.00"
    std::string codec;      // "NV12"
    int index;              // formatList のインデックス
  };

  /// パースされたビデオフォーマットのリスト
  std::vector<FormatInfo> parsedFormats;

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
  int lastDeviceNumber;

  /// 解像度、フレームレート、コーデックの選択リストを更新する
  void updateFormatDropdowns();
#else
  /// 選択しているコーデックの番号
  int codecNumber;

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs backend;
#endif

  /// 使用中の構成の番号
  int preferenceNumber;

  /// キャプチャデバイスの姿勢
  GgMatrix pose;

  /// メニューバーの高さ
  GLsizei menubarHeight;

  /// 入力パネルの表示
  bool showInputPanel;

  /// 較正パネルの表示
  bool showCalibrationPanel;

  /// 終了するなら true
  bool quit;

  /// エラーが無ければ nullptr
  mutable const char* errorMessage;

  ///
  /// キャプチャデバイスを開く
  ///
  bool openDevice();

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
  void loadParameters() const;

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
  /// 指定した番号の構成を調べる
  ///
  /// @param i 構成の番号
  ///
  const auto& getPreference(int i) const
  {
    return config.preferenceList[i];
  }

  ///
  /// 現在の構成を調べる
  ///
  const auto& getPreference() const
  {
    return getPreference(preferenceNumber);
  }

public:

  /// ArUco Marker を検出するなら true
  bool detectMarker;

  /// ChArUco Board を検出するなら true
  bool detectBoard;

  /// レイテンシを優先するなら true
  bool prioritizeLatency;

  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  /// @param capture 入力フレームを取得するキャプチャデバイス
  /// @param calibration 較正オブジェクト
  ///
  Menu(const Config& config, Capture& capture, Calibration& calibration);

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

  ///
  /// 正規化デバイス座標系における焦点距離を求める
  ///
  /// @return 図形の姿勢
  ///
  const auto& getPose() const
  {
    return pose;
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

  ///
  /// 解像度初期値を設定する
  ///
  /// @param size キャプチャされるフレームの解像度
  ///
  /// @note
  /// 解像度と現在の焦点距離 focal をもとに、
  /// 撮像系の縦横の画角の初期値も設定する。
  /// 投影面の中心位置も設定する。
  /// 
  void setSize(const std::array<int, 2>& size);

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
  /// メニューを描画する
  ///
  void draw();

  ///
  /// 画像の保存
  ///
  /// @param image 保存する画像データ
  /// @param filename 保存する画像ファイル名のテンプレート
  ///
  void saveImage(const cv::Mat& image, const std::string& filename = "*.jpg") const;
};
