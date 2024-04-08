#pragma once

///
/// メッシュの描画クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"

///
/// メッシュの描画クラス
///
class Mesh
{
  /// 頂点配列オブジェクト
  const GLuint vao;

public:

  ///
  /// コンストラクタ
  ///
  Mesh()
    : vao{ [] { GLuint vao; glGenVertexArrays(1, &vao); return vao; }() }
  {
    // 頂点配列オブジェクトが作れなかったら落とす
    assert(vao);
  }

  ///
  /// コピーコンストラクタは使用しない
  ///
  Mesh(const Mesh& mesh) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Mesh()
  {
    glDeleteVertexArrays(1, &vao);
  }

  ///
  /// 代入演算子は使用しない
  ///
  Mesh& operator=(const Mesh& mesh) = delete;

  ///
  /// 描画
  ///
  /// @param size 描画するメッシュの横と縦の格子点数
  ///
  void draw(const std::array<GLsizei, 2>& size) const
  {
    // 描画
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, size[0] * 2, size[1] - 1);
    glBindVertexArray(0);
  }
};
