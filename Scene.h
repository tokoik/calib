#pragma once

///
/// シーンの描画クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"
using namespace gg;

///
/// シーンの描画クラス
///
class Scene
{
  // シェーダ
  const GgSimpleShader shader;

  // モデル
  std::unique_ptr<const GgSimpleObj> model;

public:

  // 光源
  GgSimpleShader::LightBuffer light;

  ///
  /// コンストラクタ
  ///
  Scene();

  ///
  /// デストラクタ
  ///
  virtual ~Scene();

  ///
  /// モデルを選択する
  ///
  /// @param filename Wavefront OBJ 形式の形状データファイルのファイル名
  ///
  void set(const std::string& filename);

  ///
  /// シーンを描画する
  ///
  /// @param mp 投影変換行列
  /// @param mv モデルビュー変換行列
  ///
  void draw(const GgMatrix& mp, const GgMatrix& mv) const;
};
