///
/// シーンの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Scene.h"

//
// コンストラクタ
//
Scene::Scene() :
  light{ *menu.light.get() },
  model{ *menu.model.get() },
  shader{ *menu.shader.get() }
{
}

//
// デストラクタ
//
Scene::~Scene()
{
}

//
// 描画する
//
void Scene::draw(const GgMatrix& mp, const GgMatrix& mv) const
{
  // シェーダプログラムを指定する
  shader.use(mp, mv, light);

  // 図形を描画する
  model.draw();
}
