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
/// メッシュの描画
///
class Mesh
{
  /// 頂点配列オブジェクト
  const GLuint vao;

  /// 格子点の数
  std::array<GLsizei, 2> size;

  /// 格子点の間隔
  std::array<GLfloat, 2> gap;

public:

  ///
  /// コンストラクタ
  ///
  /// @param slices メッシュの横の格子点数
  /// @param stacks メッシュの縦の格子点数
  ///
  Mesh(GLsizei slices = 2, GLsizei stacks = 2)
    : vao{ [] { GLuint vao; glGenVertexArrays(1, &vao); return vao; }() }
    , size{ slices, stacks }
    , gap{ 2.0f / static_cast<GLfloat>(size[0] - 1), 2.0f / static_cast<GLfloat>(size[1] - 1) }
  {
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
  /// 格子点の数と間隔の設定
  ///
  /// @param slices メッシュの横の格子点数
  /// @param stacks メッシュの縦の格子点数
  ///
  void setSize(GLsizei slices, GLsizei stacks)
  {
    // 格子点の数を設定する
    size[0] = slices;
    size[1] = stacks;

    // 格子点の間隔を設定する
    // 
    //   クリッピング空間全体を埋める四角形は [-1, 1] の範囲すなわち縦横 2
    //   の大きさだから、 それを縦横の (格子数 - 1) で割って格子の間隔を求める.
    //
    gap[0] = 2.0f / static_cast<GLfloat>(size[0] - 1);
    gap[1] = 2.0f / static_cast<GLfloat>(size[1] - 1);
  }

  ///
  /// 標本点数と縦横比をもとにした格子点の数と間隔の設定
  ///
  /// @param samples メッシュの格子点数 (標本点数)
  /// @param aspect メッシュの縦横比
  ///
  void setSamples(GLsizei samples, GLfloat aspect)
  {
    // 格子点の数を求める
    // 
    //   標本点の数 (頂点数) n = x * y とするとき、これにアスペクト比 a = x / y
    //   をかければ、 a * n = x * x となるから x = sqrt(a * n), y = n / x;
    //   で求められる.
    //   この方法は頂点属性を持っていないので実行中に標本点の数やアスペクト比の変更が容易.
    //
    const auto slices{ static_cast<GLsizei>(sqrt(aspect * samples)) };
    const auto stacks{ samples / slices };

    // 格子点の数を設定する
    setSize(slices, stacks);
  }

  ///
  /// 格子点の間隔を得る
  ///
  /// @return 格子点 (標本点) の間隔
  ///
  const std::array<GLfloat, 2>& getGap() const
  {
    return gap;
  }

  ///
  /// 描画
  ///
  /// @param n インスタンス数
  ///
  void draw(GLsizei n = 1) const
  {
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, size[0] * 2, (size[1] - 1) * n);
  }
};
