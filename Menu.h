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

  /// 使用中の構成のキャプチャデバイス固有のパラメータのコピー
  Intrinsics intrinsics;

  /// キャプチャデバイス
  Capture& capture;

  /// 較正オブジェクト
  Calibration& calibration;

  /// 選択しているコーデックの番号
  int codecNumber;

  /// 選択しているキャプチャデバイスの番号
  int deviceNumber;

  /// 使用中の構成の番号
  int preferenceNumber;

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs backend;

  /// キャプチャデバイスの姿勢
  GgMatrix pose;

  /// メニューバーの高さ
  GLsizei menubarHeight;

  /// コントロールパネルの表示
  bool showControlPanel;

  /// 情報パネルの表示
  bool showInformationPanel;

  /// 終了するなら true
  bool quit;

  /// エラーが無ければ nullptr
  const char* errorMessage;

  ///
  /// 構成ファイルを読み込む
  ///
  void loadConfig();

  ///
  /// 構成ファイルを保存する
  ///
  void saveConfig();

  ///
  /// 画像ファイルを開く
  ///
  void openImage();

  ///
  /// 動画ファイルを開く
  ///
  void openMovie();

  ///
  /// キャプチャデバイスを開く
  ///
  void openDevice();

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

  /// 再投影誤差
  double repError;

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
  /// @param menu 代入元
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
  /// 解像度と画角の調整値を設定する
  ///
  /// @param size キャプチャされるフレームの解像度
  /// @param 焦点距離に対する投影面の対角線長
  /// 
  void setSizeAndFov(const std::array<int, 2>& size, float tangent);

  ///
  /// シェーダを設定する
  ///
  /// @param aspect 表示領域の縦横比
  /// @return 描画すべきメッシュの横と縦の格子点数
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
  void saveImage(const cv::Mat& image, const std::string& filename = "*.jpg");
};
