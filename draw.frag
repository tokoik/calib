#version 410

//
// 表示領域全面にメッシュを描く
//

// テクスチャ
uniform sampler2D image;

// テクスチャ座標
in vec2 texcoord;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main(void)
{
  // テクスチャ座標の範囲 (0 < texcoord < 1) 外ならフラグメントを捨てる 
  if (any(lessThan(vec4(texcoord, 1.0 - texcoord), vec4(0.0)))) discard;

  // フラグメントの色
  fc = texture(image, texcoord);
}
