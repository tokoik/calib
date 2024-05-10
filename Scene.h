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
  const GgSimpleShader& shader;

  // 光源
  const GgSimpleShader::LightBuffer& light;

  // モデル
  const GgSimpleObj& model;

public:

  ///
  /// コンストラクタ
  ///
  Scene();

  ///
  /// デストラクタ
  ///
  virtual ~Scene();

  ///
  /// シーンを描画する
  ///
  void draw(const GgMatrix& mp, const GgMatrix& mv) const;
};
