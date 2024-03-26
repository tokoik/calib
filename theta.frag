#version 410

//
// RICOH THETA S の二重魚眼画像の平面展開
//

// テクスチャ
uniform sampler2D image;

// テクスチャ座標
in vec2 texcoord_b;
in vec2 texcoord_f;

// 前後のテクスチャの混合比
in float blend;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main(void)
{
  // 前後のテクスチャの色をサンプリングする
  vec4 color_b = texture(image, texcoord_b);
  vec4 color_f = texture(image, texcoord_f);

  // サンプリングした色をブレンドしてフラグメントの色を求める
  fc = mix(color_f, color_b, blend);
}
