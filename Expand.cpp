//
// 展開用シェーダ
//
#include "Expand.h"

// 標準ライブラリ
#include <cassert>

//
//  コンストラクタ
//
Expand::Expand(const std::string& vert, const std::string& frag)
  : program{ gg::ggLoadShader(vert, frag) }
  , imageLoc{ glGetUniformLocation(program, "image") }
  , screenLoc{ glGetUniformLocation(program, "screen") }
  , focalLoc{ glGetUniformLocation(program, "focal") }
  , rotationLoc{ glGetUniformLocation(program, "rotation") }
  , circleLoc{ glGetUniformLocation(program, "circle") }
  , borderLoc{ glGetUniformLocation(program, "border") }
  , gapLoc{ glGetUniformLocation(program, "gap") }
#if defined(DO_NOT_USE_INSTANCING)
  , instanceLoc{ glGetUniformLocation(program, "instance") }
#endif
{
  // プログラムオブジェクトが作れなかったら落とす
  assert(program);
}

//
//  デストラクタ
//
Expand::~Expand()
{
  glDeleteProgram(program);
}

///
/// 展開
///
std::array<int, 2> Expand::setup(int samples, GLfloat aspect, const gg::GgMatrix& pose,
  const std::array<GLfloat, 2>& fov, const std::array<GLfloat, 2>& center, GLfloat focal,
  const std::array<GLfloat, 4>& border, int unit) const
{
  // プログラムオブジェクトの指定
  glUseProgram(program);

  // テクスチャユニットの指定
  glUniform1i(imageLoc, unit);

  // 境界色
  glUniform4fv(borderLoc, 1, border.data());

  // 投影像の画角（度）と中心位置
  glUniform4f(circleLoc, fov[0], fov[1], center[0], center[1]);

  // スクリーンのサイズと中心位置
  //   screen[0] = (right - left) / 2
  //   screen[1] = (top - bottom) / 2
  //   screen[2] = (right + left) / 2
  //   screen[3] = (top + bottom) / 2
  const GLfloat screen[]{ aspect, 1.0f, 0.0f, 0.0f };
  glUniform4fv(screenLoc, 1, screen);

  // レンズの主点とスクリーンの距離
  glUniform1f(focalLoc, focal);

  // 背景に対する視線の回転行列
  glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, pose.get());

  // メッシュの横の格子点数
  //   標本点の数 (頂点数) samples = w * h とするとき、これにアスペクト比
  //   aspect = w / h をかければ aspect * samples = w * w となるから、
  //   w = sqrt(aspect * samples), h = samples / w で求められる
  const auto w{ static_cast<int>(sqrt(aspect * samples)) };
  const auto h{ samples / w };

  // 格子間隔
  //   クリッピング空間全体を埋める四角形は [-1, 1] の範囲すなわち
  //   縦横 2 の大きさだから、 それを縦横の (格子数 - 1) で割る
  std::array<GLfloat, 2> gap{ 2.0f / (w - 1), 2.0f / (h - 1) };
  glUniform2fv(gapLoc, 1, gap.data());

  // 描画するメッシュの横と縦の格子点数を返す
  return std::array<int, 2>{ w, h };
}
