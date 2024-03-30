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
#include "Camera.h"

// フレームバッファオブジェクト
#include "Framebuffer.h"

// 較正
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

  /// キャプチャしたフレームの表示に使うフレームバッファオブジェクト
  Framebuffer& framebuffer;

  /// 較正オブジェクト
  Calibration& calibration;

  /// 使用中の構成のキャプチャデバイス固有のパラメータのコピー
  Intrinsics intrinsics;

  /// 選択しているコーデックの番号
  int codecNumber;

  /// 選択しているキャプチャデバイスの番号
  int deviceNumber;

  /// 使用中の構成の番号
  int preferenceNumber;

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

  /// 選択しているキャプチャデバイスのポインタ
  std::unique_ptr<Camera> camera;

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs backend;

  /// ArUco Marker を検出するなら true
  bool detectMarker;

  /// ChArUco Board を検出するなら true
  bool detectBoard;

  /// 再投影誤差
  double repError;

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
  void loadImage();

  ///
  /// 動画ファイルを開く
  ///
  void loadMovie();

  ///
  /// キャプチャを開始する
  ///
  void startCapture();

  ///
  /// キャプチャを停止する
  ///
  void stopCapture();

public:

  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  /// @param framebuffer フレームの描画に用いるフレームバッファオブジェクト
  /// @param calibration フレームの構成に用いる較正オブジェクト
  ///
  Menu(const Config& config, Framebuffer& framebuffer, Calibration& calibration);

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
  /// メニューバーの高さを得る
  ///
  /// @return メニューバーの高さ
  /// 
  auto getMenubarHeight() const
  {
    return menubarHeight;
  }

  ///
  /// 正規化デバイス座標系における焦点距離を求める
  ///
  /// @return 図形の姿勢
  ///
  auto getPose() const
  {
    return ggRotateY(settings.euler[1]).rotateX(settings.euler[0]).rotateZ(settings.euler[2]);
  }

  ///
  /// フレームを取得する
  ///
  /// @param texture 取得したフレームを格納するテクスチャ
  ///
  void retriveFrame(Texture& texture) const;

  ///
  /// シェーダを設定する
  ///
  /// @param aspect 表示領域の縦横比
  /// @param pose カメラの姿勢
  ///
  void setup(GLfloat aspect, const GgMatrix& pose = ggIdentity()) const;

  ///
  /// メニューを描画する
  ///
  const Settings& draw();

  ///
  /// 画像の保存
  ///
  /// @param image 保存する画像データ
  /// @param filename 保存する画像ファイル名のテンプレート
  ///
  void saveImage(const cv::Mat& image, const std::string& filename = "*.jpg");
};
