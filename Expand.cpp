//
// 展開用シェーダ
//
#include "Expand.h"

// 標準ライブラリ
#include <cassert>

// コンストラクタ
Expand::Expand(const std::string& vert, const std::string& frag)
  : program{ gg::ggLoadShader(vert, frag) }
  , gapLoc{ glGetUniformLocation(program, "gap") }
  , screenLoc{ glGetUniformLocation(program, "screen") }
  , focalLoc{ glGetUniformLocation(program, "focal") }
  , rotationLoc{ glGetUniformLocation(program, "rotation") }
  , circleLoc{ glGetUniformLocation(program, "circle") }
  , imageLoc{ glGetUniformLocation(program, "image") }
#if defined(DO_NOT_USE_INSTANCING)
  , instanceLoc{ glGetUniformLocation(program, "instance") }
#endif
{
  assert(program);
}

// デストラクタ
Expand::~Expand()
{
  glDeleteProgram(program);
}

void Expand::use(const std::array<GLfloat, 2>& gap, GLfloat aspect, GLfloat focal,
  const gg::GgMatrix& rotation, const std::array<GLfloat, 4>& circle) const
{
  // プログラムオブジェクトの指定
  glUseProgram(program);

  // スクリーンの格子間隔
  //   クリッピング空間全体を埋める四角形は [-1, 1] の範囲すなわち縦横 2
  //   の大きさだから、 それを縦横の (格子数 - 1) で割って格子の間隔を求める.
  glUniform2fv(gapLoc, 1, gap.data());

  // スクリーンのサイズと中心位置
  //   screen[0] = (right - left) / 2
  //   screen[1] = (top - bottom) / 2
  //   screen[2] = (right + left) / 2
  //   screen[3] = (top + bottom) / 2
  const GLfloat screen[]{ aspect, 1.0f, 0.0f, 0.0f };
  glUniform4fv(screenLoc, 1, screen);

  // スクリーンまでの焦点距離
  //   35mm フィルムの対角線長を 43.3mm としたときの焦点距離 h に対する
  //   このプログラムの焦点距離 focal = 2 * h / 43.3
  glUniform1f(focalLoc, focal * 0.046189376f);

  // 背景に対する視線の回転行列
  glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, rotation.get());

  // レンズの画角と中心位置
  glUniform4fv(circleLoc, 1, circle.data());

  // テクスチャユニットの指定
  glUniform1i(imageLoc, 0);
  glActiveTexture(GL_TEXTURE0);
}
