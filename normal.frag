#version 410

//
// テクスチャ座標の位置の画素色をそのまま使う
//

// テクスチャ
uniform sampler2D image;

// 境界色
uniform vec4 border;

// テクスチャ座標
in vec2 texcoord;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main(void)
{
  // テクスチャ座標の範囲
  vec4 code = vec4(texcoord, texcoord);
  bool allGt0 = all(greaterThan(code, vec4(0.0)));
  bool allLt1 = all(lessThan(code, vec4(1.0)));

  // 画素の陰影を求める
  fc = (allGt0 && allLt1) ? texture(image, texcoord) : border;
}
