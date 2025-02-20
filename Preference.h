#pragma once

///
/// キャプチャデバイスの構成データクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date January 23, 2025
///

// 平面展開用のシェーダ
#include "Expand.h"

// キャプチャデバイス固有のパラメータ
#include "Intrinsics.h"

///
/// キャプチャデバイスの構成データ
///
/// キャプチャデバイスごとの特性を記述する.
/// キャプチャデバイス固有のパラメータはカメラの内部パラメータなどである.
/// キャプチャデバイスの投影方式は平面展開用のシェーダとして記述する.
///
class Preference
{
  /// このキャプチャデバイスの説明の文字列
  std::string description;

  /// このキャプチャデバイスの入力画像の平面展開用のシェーダのソースファイル名
  std::array<std::string, 2> source;

  /// このキャプチャデバイス固有のパラメータ
  const Intrinsics intrinsics;

  /// このキャプチャデバイスの入力画像の平面展開用のシェーダへのポインタ
  const Expand* shader;

  /// すべてのキャプチャデバイスの平面展開用のシェーダのリスト
  static std::map<std::string, Expand> shaderList;

public:

  ///
  /// キャプチャデバイスの構成データのデフォルトコンストラクタ
  ///
  /// @note 構成ファイルが読めなかったときにしか使わない.
  ///
  Preference();

  ///
  /// シェーダのソースファイルを指定するときに使う
  /// キャプチャデバイスの構成データのコンストラクタ
  ///
  /// @param description このキャプチャデバイスの説明の文字列
  /// @param vert 展開用のバーテックスシェーダのソースファイル名
  /// @param frag 展開用のフラグメントシェーダのソースファイル名
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
  /// 展開用のシェーダをビルドする
  ///
  void buildShader();

  ///
  /// 説明の文字列を取り出す
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
