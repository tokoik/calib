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

// キャプチャデバイス固有のパラメータ
#include "Intrinsics.h"

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
  /// シェーダのソースファイルを指定するときに使う
  /// キャプチャデバイスの構成データのコンストラクタ
  ///
  /// @param description 説明
  /// @param vert バーテックスシェーダのソースファイル名
  /// @param frag フラグメントシェーダのソースファイル名
  /// @param intrinsics キャプチャデバイス固有のパラメータ
  ///
  Preference(const std::string& description,
    const std::string& vert, const std::string& frag,
    const Intrinsics& intrinsics = Intrinsics{});

  ///
  /// 構成ファイルを読み込むときに使う
  /// キャプチャデバイスの構成データのコンストラクタ
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
