#version 410

//
// 正距円筒図法画像のサンプリング
//

// テクスチャ
uniform sampler2D image;

// テクスチャ上の投影像の半径と中心位置
uniform vec4 circle;

// 視線ベクトル
in vec3 vector;

// フラグメントの色
layout (location = 0) out vec4 fc;

void main(void)
{
  // 投影像のテクスチャ空間上のスケール (180/π = 57.2957795)
  vec2 scale = vec2(57.2957795, -57.2957795) / circle.st;

  // 投影像のテクスチャ空間上の中心位置
  vec2 center = circle.pq + 0.5;

  // テクスチャ座標を求める
  vec2 texcoord = atan(vector.xy, vec2(vector.z, length(vector.xz))) * scale + center;

  // 画素の陰影を求める
  fc = texture(image, texcoord);
}
