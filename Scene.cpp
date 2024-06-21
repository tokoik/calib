///
/// シーンの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Scene.h"

// 標準ライブラリ
#include <stdexcept>

// 光源データ
static constexpr GgSimpleShader::Light defaultLight
{
  { 0.8f, 0.8f, 0.8f, 1.0f },
  { 0.8f, 0.8f, 0.8f, 0.0f },
  { 0.2f, 0.2f, 0.2f, 0.0f },
  { 3.0f, 4.0f, 5.0f, 1.0f }
};

//
// コンストラクタ
//
Scene::Scene()
  : shader{ "simple.vert", "simple.frag" }
  , model{ std::make_unique<const GgSimpleObj>("axis.obj") }
  , light{ defaultLight }
{
  // シェーダが作れなかったら落とす
  if (shader.get() == 0) throw std::runtime_error("Cannot create the simple shader.");

  // 図形が作れなかったら落とす
  if (!model) throw std::runtime_error("Cannot load the axis object.");
}

//
// デストラクタ
//
Scene::~Scene()
{
}

//
// モデルを選択する
//
void Scene::set(const std::string& filename)
{
  model = std::make_unique<const GgSimpleObj>(filename);
}

//
// 描画する
//
void Scene::draw(const GgMatrix& mp, const GgMatrix& mv) const
{
  // シェーダプログラムを指定する
  shader.use(mp, mv, light);

  // 図形を描画する
  model->draw();
}
