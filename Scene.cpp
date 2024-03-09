///
/// 展開図の描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Scene.h"

//
// コンストラクタ
//
Scene::Scene(const Config& config, int slices, int stacks, int instances)
  : config{ config }
  , slices{ slices }
  , stacks{ stacks }
  , instances{ instances }
  , vao{ [] { GLuint vao; glGenVertexArrays(1, &vao); return vao; }() }
  , vbo{ [] { GLuint buffer; glGenBuffers(1, &buffer); return buffer; }() }
  , vertexCount{ (slices + 1) * (stacks + 1) * instances }
  , ibo{ [] { GLuint buffer; glGenBuffers(1, &buffer); return buffer; }() }
  , drawCount{ stacks * instances }
  , indexCount{ (slices + 1) * 2 * drawCount }
{
  // 頂点配列オブジェクト
  glBindVertexArray(vao);

  // 頂点の番号のバッファオブジェクトを作成する
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLushort), nullptr, GL_STATIC_DRAW);

  // 頂点の番号のバッファオブジェクトに番号を格納する
  auto index{ static_cast<GLushort*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY)) };

  // メッシュの１ライン分の頂点数
  const auto vertex_count_of_a_line{ slices + 1 };

  // メッシュの１ストリップ分の頂点数
  const auto vertex_count_of_a_strip{ vertex_count_of_a_line * 2 };

  // ストリップとラインのそれぞれの最初の頂点番号
  int strip{ 0 }, line{ 0 };

  // すべての面について
  for (int k = 0; k < instances; ++k)
  {
    // すべてのストリップについて
    for (int j = 0; j < stacks; ++j)
    {
      // そのストリップの最初の頂点の番号をバッファオブジェクトに保存する
      indices.emplace_back(static_cast<GLushort*>(0) + strip);

      // そのストリップの頂点数を保存する
      count.emplace_back(vertex_count_of_a_strip);

      // そのストリップの個々の頂点について
      for (int i = 0; i < vertex_count_of_a_line; ++i)
      {
        // ストリップの下側のラインの頂点番号
        *index++ = line + i;

        // ストリップの上側のラインの頂点番号
        *index++ = line + i + vertex_count_of_a_line;
      }

      // 次のストリップの最初の頂点番号
      strip += vertex_count_of_a_strip;

      // 次のラインの最初の頂点番号
      line += vertex_count_of_a_line;
    }

    // 次の面の最初のラインの最初の頂点番号
    line += vertex_count_of_a_line;
  }

  // 頂点の番号のバッファオブジェクトに番号を格納を終了する
  glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

  // 頂点の位置のバッファオブジェクト
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), nullptr, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetptr(Vertex, position));
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetptr(Vertex, normal));
  glEnableVertexAttribArray(1);
}

//
// デストラクタ
//
Scene::~Scene()
{
  glBindVertexArray(0);
  glDeleteVertexArrays(1, &vao);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &ibo);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vbo);
}

//
// 展開する
//
void Scene::expand(const GgApp::Window& window, Calibration& framebuffer)
{
  // テクスチャを更新する
  texture->update(border, repeat);

  // 隠面消去処理はしない
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // 現在の構成を取り出す
  const auto& selection{ queryPreference() };

  // メッシュの格子点を設定して間隔を得る
  const auto& gap{ framebuffer.getGap() };

  // フレームバッファオブジェクトのアスペクト比
  const auto& aspect{ framebuffer.getAspect() };

  // 回転の変換行列を取り出す
  const auto direction{ ggRotateY(eyePose[1]).rotateX(eyePose[0]).rotateZ(eyePose[2]) };

  // 図形のプロジェクション変換行列
  const auto mp{ ggPerspective(fovy, window.getAspect(), 0.1f, 20.0f) };

  // シェーダを指定する
  //   画角 90°なら focal = 1.0
  selection.getShader()->use(gap, aspect, 1.0, direction, circle);

  // フレームバッファオブジェクトを指定する
  framebuffer.use();

  // フレームバッファオブジェクトを更新する
  framebuffer.update();

  if (detect)
  {
    // フレームバッファオブジェクトの内容をメモリに保存する
    framebuffer.read();

    // マーカーを検出してキャリブレーションする
    calibrate = !framebuffer.detect(markerPoses);

#if defined(_DEBUG)
    for (const auto& [id, pose] : markerPoses)
      std::cerr << id << " => " << pose[12] << ", " << pose[13] << ", " << pose[14] << "\n";
#endif
  }

  // 通常のフレームバッファへの描画に戻す
  framebuffer.unuse();

  // ビューポートを元に戻す
  window.restoreViewport();

  // 隠面消去処理を行う
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // ウィンドウを消去する
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // 図形のモデルビュー変換行列
  const auto mv{ origin * window.getTranslationMatrix(GLFW_MOUSE_BUTTON_2)
    * window.getRotationMatrix(GLFW_MOUSE_BUTTON_1) };

  // 描画
  cubic->use(mp, mv, framebuffer.get(), static_cast<int>(type));
}

//
// 描画する
//
void Scene::draw(float tangent, float aspect) const
{
  // メッシュを描画する
  glBindVertexArray(vao);
  glMultiDrawElements(GL_TRIANGLE_STRIP, count.data(), GL_UNSIGNED_SHORT, indices.data(), drawCount);
}
