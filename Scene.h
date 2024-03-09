#pragma once

///
/// シーンの描画クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 構成データ
#include "Config.h"

///
/// 構造体やクラスのメンバのバイトオフセットをポインタで得る
///
/// @param type 構造体名／クラス名
/// @param member メンバ名
///
#define offsetptr(type, member) (static_cast<const char*>(0) + offsetof(type, member))

///
/// シーンの描画
///
class Scene
{
protected:

  // 編集する構成データ
  const Config& config;

  // メッシュの分割数
  const int slices, stacks, instances;

  // 頂点配列オブジェクト
  const GLuint vao;

  // 頂点属性のバッファオブジェクト
  const GLuint vbo;

  // 全頂点数
  const GLsizei vertexCount;

  // 頂点番号のバッファオブジェクト
  const GLuint ibo;

  // ストリップの数
  const GLsizei drawCount;

  // 全番号数
  const GLsizei indexCount;

  // ストリップごとの頂点番号の先頭位置
  std::vector<const void*> indices;

  // ストリップごとの頂点数
  std::vector<GLsizei> count;

public:


  ///
  /// コンストラクタ
  ///
  /// @param config 構成データ
  ///
  Scene(const Config& config);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param scene コピー元
  ///
  Scene(const Scene& scene) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Scene();

  ///
  /// 代入演算子は使用しない
  ///
  /// @param scene 代入元
  ///
  Scene& operator=(const Scene& scene) = delete;

  ///
  /// 展開する
  ///
  /// @param window ウィンドウ
  /// @param framebuffer フレームバッファオブジェクト
  ///
  void expand(const GgApp::Window& window, Calibration& framebuffer);

  ///
  /// 描画する
  ///
  /// @param tangent カメラの焦点距離に対するスクリーンの高さ/ 2
  /// @param aspect 表示領域の縦横比
  ///
  virtual void draw(float tangent, float aspect) const;
};
