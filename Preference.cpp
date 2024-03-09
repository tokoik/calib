///
/// キャプチャデバイスの構成データクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Preference.h"

//
// キャプチャデバイス固有のパラメータの構造体のデフォルトコンストラクタ
//
Intrinsics::Intrinsics()
  : Intrinsics{ { 50.03f, 38.58f }, { 0.0f, 0.0f }, { 640, 480 }, 30.0 }
{
}

//
// パラメータを指定するときに使うキャプチャデバイス固有のパラメータの構造体のコンストラクタ
//
Intrinsics::Intrinsics(const std::array<float, 2>& fov, const std::array<float, 2>& center, const std::array<int, 2> resolution, double fps)
  : fov{ fov }
  , center{ center }
  , resolution{ resolution }
  , fps{ fps }
{
}

//
// 構成ファイルを読み込むときに使うキャプチャデバイス固有のパラメータの構造体のコンストラクタ
//
Intrinsics::Intrinsics(const picojson::object& object)
{
  // キャプチャデバイスの画角
  getValue(object, "fov", fov);

  // キャプチャデバイスの中心位置
  getValue(object, "center", center);

  // キャプチャデバイスの解像度
  getValue(object, "resolution", resolution);

  // キャプチャデバイスの周波数
  getValue(object, "fps", fps);
}

//
// キャプチャデバイスのレンズの縦横の画角を変更する
//
void Intrinsics::setFov(float fovx, float fovy)
{
  // キャプチャデバイスのレンズの縦横の画角を設定する
  fov[0] = fovx;
  fov[1] = fovy;
}

//
// スクリーンの高さをもとにしてキャプチャデバイスのレンズの縦横の画角を変更する
//
void Intrinsics::setFov(float tangent)
{
  // キャプチャデバイスのフレームの縦横比
  const auto aspect{ static_cast<float>(resolution[0]) / static_cast<float>(resolution[1]) };

  // 画角を求めてキャプチャデバイスのレンズの縦横の画角を設定する
  setFov(atan(tangent * aspect) * 114.59156f, atan(tangent) * 114.59156f);
}

//
// キャプチャデバイスのレンズの中心の位置を変更する
//
void Intrinsics::setCenter(float x, float y)
{
  // キャプチャデバイスのレンズの中心の位置を設定する
  center[0] = x;
  center[1] = y;
}

//
// キャプチャデバイスの解像度を変更する
//
void Intrinsics::setResolution(int width, int height)
{
  // 装置の解像度を設定する
  resolution[0] = width;
  resolution[1] = height;
}

//
// キャプチャデバイスのフレームレートを変更する
//
void Intrinsics::setFps(double frequency)
{
  // 装置のフレームレートを設定する
  fps = frequency;
}

//
// キャプチャデバイスの構成データのデフォルトコンストラクタ
//
Preference::Preference()
  : Preference{ "Default", "orthographic.vert", "normal.frag" }
{
}

//
// シェーダのソースファイルを指定するときに使うキャプチャデバイスの構成データのコンストラクタ
//
Preference::Preference(const std::string& description, const std::string& vert, const std::string& frag, const Intrinsics& intrinsics)
  : description{ description }
  , source{ vert, frag }
  , intrinsics{ intrinsics }
  , shader{ nullptr }
{
}

//
// 構成ファイルを読み込むときに使うキャプチャデバイスの構成データのコンストラクタ
//
Preference::Preference(const picojson::object& object)
  : intrinsics{ object }
  , shader{ nullptr }
{
  // 説明の文字列
  getString(object, "description", description);

  // 展開用シェーダのファイル名
  getString(object, "shader", source);
}

//
// 構成データのデストラクタ
//
Preference::~Preference()
{
  // static メンバにしている std::map の中身を先に消去しておく
  shaderList.clear();
}

//
// シェーダをビルドする
//
void Preference::buildShader()
{
  // ソースファイル名をつないでシェーダを検索するキーを作る
  const auto key{ source[0] + "\t" + source[1] };

  // キーがシェーダリストに無ければシェーダを構築して追加する
  shader = &shaderList.try_emplace(key, source[0], source[1]).first->second;
}

//
// 構成を JSON オブジェクトに格納する
//
void Preference::setPreference(picojson::object& object) const
{
  // 説明の文字列
  setString(object, "description", description);

  // 展開用シェーダのファイル名
  setString(object, "shader", source);

  // キャプチャデバイスのレンズの縦横の画角
  setValue(object, "fov", intrinsics.fov);

  // キャプチャデバイスのレンズの中心（主点）位置
  setValue(object, "center", intrinsics.center);

  // キャプチャデバイスの解像度
  setValue(object, "resolution", intrinsics.resolution);

  // キャプチャデバイスのフレームレート
  setValue(object, "fps", intrinsics.fps);
}

// すべての構成のシェーダーのリスト
std::map<std::string, Expand> Preference::shaderList;
