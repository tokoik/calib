#version 410

//
// 直交投影
//

// テクスチャ上の投影像の半径と中心位置
uniform vec4 circle;

// スクリーンを回転する変換行列
uniform mat4 rotation;

// スクリーンの大きさと中心位置
uniform vec4 screen;

// 頂点の位置
layout(location = 0) in vec2 position;

// 頂点の法線
layout(location = 1) in vec3 normal;

// テクスチャ座標
out vec2 texcoord;

void main(void)
{
  // 頂点の位置はそのままラスタライザに送る
  gl_Position = rotation * vec4(position, 0.0, 1.0);

  // 背景テクスチャのテクスチャ空間上のスケール (0.5π / 180 ≒ 0.00872664626)
  vec2 scale = 1.0 / tan(circle.st * 0.00872664626);

  // 背景テクスチャのテクスチャ空間上の中心位置
  vec2 center = circle.pq + 0.5;

  // テクスチャ座標
  texcoord = (normal.xy * screen.st + screen.pq) * scale + center;
}
