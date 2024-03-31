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
    // ストリップの頂点数
    const auto vertices{ size[0] * 2 };

    // ストリップ数
    const auto strips{ size[1] - 1 };

    // 描画
    glBindVertexArray(vao);
#if defined(DO_NOT_USE_INSTANCING)
    for (int i = 0; i < h; ++i)
    {
      glUniform1i(instanceLoc, i);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, w);
    }
#else
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, vertices, strips);
#endif
    glBindVertexArray(0);
  }
};
