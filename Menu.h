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

// フレームバッファオブジェクト
#include "Framebuffer.h"

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

  /// 選択しているコーデックの番号
  int codecNumber;

  /// 選択しているキャプチャデバイスの番号
  int deviceNumber;

  /// 選択しているキャプチャデバイスのポインタ
  std::unique_ptr<Camera> camera;

  /// キャプチャしたフレームの表示に使うフレームバッファオブジェクト
  Framebuffer& framebuffer;

  /// 使用中の構成
  const Preference* preference;

  /// デバイスプリファレンス
  cv::VideoCaptureAPIs apiPreference;

  /// メニューバーの高さ
  GLsizei menubarHeight;

  /// コントロールパネルの表示
  bool showControlPanel;

  /// 終了するなら true
  bool continuing;

  /// エラーが無ければ nullptr
  const char* errorMessage;

public:

  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  ///
  Menu(const Config& config, Framebuffer& framebuffer);

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

  /// マーカーを検出するなら true
  bool detect;

  /// 構成するなら true
  bool calibrate;

  ///
  /// 処理を継続するかどうか調べる
  ///
  /// @return 処理を継続するなら true
  /// 
  explicit operator bool() const
  {
    return continuing;
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
  /// キャプチャを開始する
  ///
  void startCapture();

  ///
  /// キャプチャを停止する
  ///
  void stopCapture();

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
};
