#pragma once

///
/// キャプチャデバイスの構成データクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// 展開用シェーダ
#include "Expand.h"

// 構成ファイルの読み取り補助
#include "parseconfig.h"

///
/// キャプチャデバイス固有のパラメータ
///
struct Intrinsics
{
  /// キャプチャデバイスのレンズの縦横の画角
  std::array<float, 2> fov;

  /// キャプチャデバイスのレンズの中心 (主点) の位置
  std::array<float, 2> center;

  /// キャプチャデバイスの解像度
  std::array<int, 2> resolution;

  /// キャプチャデバイスのフレームレート
  double fps;

  ///
  /// キャプチャデバイス固有のパラメータの構造体のデフォルトコンストラクタ
  ///
  /// @note 構成ファイルが読めなかったときにしか使わない
  ///
  Intrinsics();

  ///
  /// パラメータを指定するときに使うキャプチャデバイス固有のパラメータの構造体のコンストラクタ
  ///
  /// @param fov キャプチャデバイスのレンズの縦横の画角
  /// @param center キャプチャデバイスのレンズの中心 (主点) の位置
  /// @param resolution キャプチャデバイスの解像度
  /// @param fps キャプチャデバイスのフレームレート
  ///
  Intrinsics(const std::array<float, 2>& fov, const std::array<float, 2>& center, const std::array<int, 2> resolution, double fps);

  ///
  /// 構成ファイルを読み込むときに使うキャプチャデバイス固有のパラメータの構造体のコンストラクタ
  ///
  Intrinsics(const picojson::object& object);

  ///
  /// キャプチャデバイスのレンズの縦横の画角を変更する
  ///
  /// @param fovx キャプチャデバイスのレンズの fovx 方向の画角
  /// @param fovy キャプチャデバイスのレンズの fovy 方向の画角
  ///
  void Intrinsics::setFov(float fovx, float fovy);

  ///
  /// スクリーンの高さをもとにしてキャプチャデバイスのレンズの縦横の画角を変更する
  ///
  /// @param tangent (スクリーンの高さ/2)÷焦点距離＝焦点距離が1の時のスクリーンの高さ/2
  ///
  void setFov(float tangent);

  ///
  /// キャプチャデバイスのレンズの中心の位置を変更する
  ///
  /// @param x キャプチャデバイスのレンズの中心の位置の x 座標
  /// @param y キャプチャデバイスのレンズの中心の位置の y 座標
  ///
  void setCenter(float x, float y);

  ///
  /// キャプチャデバイスの解像度を変更する
  ///
  /// @param width キャプチャデバイスのフレームの横の画素数
  /// @param height キャプチャデバイスのフレームの縦の画素数
  ///
  void setResolution(int width, int height);

  ///
  /// キャプチャデバイスのフレームレートを変更する
  ///
  /// @param frequency フレームレート
  ///
  void setFps(double frequency);
};

///
/// キャプチャデバイスの構成データ
///
class Preference
{
  /// 説明
  std::string description;

  /// シェーダのソースファイル名
  std::array<std::string, 2> source;

  /// キャプチャデバイス固有のパラメータ
  const Intrinsics intrinsics;

  /// この構成の展開用シェーダへのポインタ
  const Expand* shader;

  /// すべての構成の展開用シェーダのリスト
  static std::map<std::string, Expand> shaderList;

public:

  ///
  /// キャプチャデバイスの構成データのデフォルトコンストラクタ
  ///
  /// @note 構成ファイルが読めなかったときにしか使わない
  ///
  Preference();

  ///
  /// シェーダのソースファイルを指定するときに使うキャプチャデバイスの構成データのコンストラクタ
  ///
  /// @param description 説明
  /// @param vert バーテックスシェーダのソースファイル名
  /// @param frag フラグメントシェーダのソースファイル名
  /// @param intrinsics キャプチャデバイス固有のパラメータ
  ///
  Preference(const std::string& description, const std::string& vert, const std::string& frag,
    const Intrinsics& intrinsics = Intrinsics{});

  ///
  /// 構成ファイルを読み込むときに使うキャプチャデバイスの構成データのコンストラクタ
  ///
  Preference(const picojson::object& object);

  ///
  /// デストラクタ
  ///
  virtual ~Preference();

  ///
  /// シェーダをビルドする
  ///
  void buildShader();

  ///
  /// 説明を取り出す
  ///
  /// @return 構成の説明文
  ///
  const auto& getDescription() const
  {
    return description;
  }

  ///
  /// キャプチャデバイス固有のパラメータを取り出す
  ///
  /// @return キャプチャデバイス固有のパラメータ
  ///
  const auto& getIntrinsics() const
  {
    return intrinsics;
  }

  ///
  /// シェーダを取り出す
  ///
  /// @return この構成のシェーダの参照
  ///
  const auto& getShader() const
  {
    return *shader;
  }

  ///
  /// JSON オブジェクトに構成を格納する
  ///
  /// @param object 格納先の JSON オブジェクト
  ///
  void setPreference(picojson::object& object) const;
};
