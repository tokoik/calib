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
  // テクスチャ座標の範囲 (0 < texcoord < 1 なら code > 0) 
  vec4 code = vec4(texcoord, 1.0 - texcoord);

  // テクスチャ座標の範囲外は境界色にする
  fc = all(greaterThan(code, vec4(0.0))) ? texture(image, texcoord) : border;
}
