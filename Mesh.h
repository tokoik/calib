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

// 標準ライブラリ
#include <stdexcept>

///
/// メッシュの描画クラス
///
class Mesh
{
  /// 頂点配列オブジェクト
  const GLuint array;

  /// シェーダ
  const GLuint program;

  /// サンプラの uniform 変数の場所
  const GLint imageLoc;

  /// テクスチャのスケールの uniform 変数の場所
  const GLint scaleLoc;

public:

  ///
  /// コンストラクタ
  ///
  Mesh()
    : array{ [] { GLuint array; glGenVertexArrays(1, &array); return array; }() }
    , program{ gg::ggLoadShader("draw.vert", "draw.frag") }
    , imageLoc{ glGetUniformLocation(program, "image") }
    , scaleLoc{ glGetUniformLocation(program, "scale") }
  {
    // 頂点配列オブジェクトが作れなかったら落とす
    assert(array);

    // シェーダが作れなかったら落とす
    if (program == 0) throw std::runtime_error("Cannot create the shader for the mesh.");
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
    glDeleteVertexArrays(1, &array);
    glDeleteProgram(program);
  }

  ///
  /// 代入演算子は使用しない
  ///
  Mesh& operator=(const Mesh& mesh) = delete;

  ///
  /// シェーダを指定して矩形を描画する
  ///
  /// @param scale マッピングするテクスチャのスケール
  /// @param unit 使用するテクスチャユニット
  ///
  /// @note
  /// この前にテクスチャユニット GL_TEXTURE0 + unit を指定し、
  /// テクスチャを結合しておく必要がある。
  ///
  void draw(const std::array<GLfloat, 2>& scale, GLint unit = 0) const
  {
    // 表示領域を背景色で塗りつぶす
    glClear(GL_COLOR_BUFFER_BIT);

    // シェーダの使用開始
    glUseProgram(program);

    // サンプラの指定
    glUniform1i(imageLoc, unit);

    // スケールの設定
    glUniform2fv(scaleLoc, 1, scale.data());

    // 描画
    glBindVertexArray(array);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // シェーダの仕様終了
    glUseProgram(0);
  }

  ///
  /// メッシュを描画する
  ///
  /// @param size 描画するメッシュの横と縦の格子点数
  ///
  /// @note
  /// この前に別途シェーダを指定し、
  /// メッシュの格子点数を得ておく必要がある。
  ///
  void drawMesh(const std::array<GLsizei, 2>& size) const
  {
    // 描画
    glBindVertexArray(array);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, size[0] * 2, size[1] - 1);
    glBindVertexArray(0);
  }
};
