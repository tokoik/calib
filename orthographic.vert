#version 410

//
// 直交投影
//

// 投影像の半径と中心位置
uniform vec4 circle;

// スクリーンの大きさと中心位置
uniform vec4 screen;

// スクリーンまでの焦点距離 
uniform float focal;

// スクリーンを回転する変換行列
uniform mat4 rotation;

// スクリーンの格子間隔
uniform vec2 gap;

// テクスチャ座標
out vec2 texcoord;

void main(void)
{
  // 頂点位置
  //   各頂点において gl_VertexID が 0, 1, 2, 3, ... のように割り当てられるから、
  //     x = gl_VertexID >> 1      = 0, 0, 1, 1, 2, 2, 3, 3, ...
  //     y = 1 - (gl_VertexID & 1) = 1, 0, 1, 0, 1, 0, 1, 0, ...
  //   のように GL_TRIANGLE_STRIP 向けの頂点座標値が得られる。
  //   y に gl_InstaceID を足せば glDrawArrayInstanced() のインスタンスごとに y が変化する。
  //   これに格子の間隔 gap をかけて 1 を引けば縦横 [-1, 1] の範囲の位置 position が得られる。
  int x = gl_VertexID >> 1;
  int y = gl_InstanceID + 1 - (gl_VertexID & 1);
  vec2 position = vec2(x, y) * gap - 1.0;

  // 頂点位置をそのままラスタライザに送ればクリッピング空間全面に描く
  gl_Position = vec4(position, 0.0, 1.0);

  // スクリーン上の位置 (screen の縦と横を入れ替えている)
  vec2 p = mat2(rotation) * position * screen.ts + screen.pq;

  // 投影像のテクスチャ空間上のスケール (0.5π / 180 ≒ 0.00872664626)
  vec2 scale = 5.0 * tan(circle.st * 0.00872664626) / focal;

  // 投影像のテクスチャ空間上の中心位置
  vec2 center = circle.pq + 0.5;

  // テクスチャ座標
  texcoord = p * scale + center;
}
