#version 410

//
// 表示領域全面に矩形を描く
//

// テクスチャのスケール
uniform vec2 scale;

// テクスチャ座標
out vec2 texcoord;

void main(void)
{
  // 頂点位置
  //   各頂点において gl_VertexID が 0, 1, 2, 3 と割り当てられるから、
  //     x = gl_VertexID >> 1 = 0, 0, 1, 1
  //     y = gl_VertexID & 1  = 0, 1, 0, 1
  //   のように GL_TRIANGLE_STRIP 向けの [0, 1] の範囲のテクスチャ座標 texcoord が得られる。
  //   これに格子の間隔 2 をかけて 1 を引けば縦横 [-1, 1] の範囲の位置 position が得られる。
  vec2 p = vec2(gl_VertexID >> 1, gl_VertexID & 1);

  // 頂点番号 gl_VertexID から上下を反転したテクスチャ座標を求める
  texcoord = vec2(p.x - 0.5, 0.5 - p.y) * scale + 0.5;

  // テクスチャ座標から求めた頂点位置をラスタライザに送ればクリッピング空間全面に描く
  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
