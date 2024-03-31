///
/// キャプチャデバイスの構成データクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Preference.h"

//
// キャプチャデバイスの構成データのデフォルトコンストラクタ
//
Preference::Preference()
  : Preference{ "Default", "orthographic.vert", "normal.frag" }
{
}

//
// シェーダのソースファイルを指定するときに使う
// キャプチャデバイスの構成データのコンストラクタ
//
Preference::Preference(const std::string& description,
  const std::string& vert, const std::string& frag,
  const Intrinsics& intrinsics)
  : description{ description }
  , source{ vert, frag }
  , intrinsics{ intrinsics }
  , shader{ nullptr }
{
}

//
// 構成ファイルを読み込むときに使う
// キャプチャデバイスの構成データのコンストラクタ
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
  setValue(object, "resolution", intrinsics.size);

  // キャプチャデバイスのフレームレート
  setValue(object, "fps", intrinsics.fps);
}

// すべての構成のシェーダーのリスト
std::map<std::string, Expand> Preference::shaderList;
