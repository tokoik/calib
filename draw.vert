#version 410

//
// 表示領域全面に矩形を描く
//

// 表示矩形のスケール
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

  // 頂点番号 gl_VertexID から画像全体を参照する上下反転済みテクスチャ座標を求める
  texcoord = vec2(p.x, 1.0 - p.y);

  // 縦横比を維持する表示スケールを頂点位置に適用し、画像全体を表示領域内に収める
  gl_Position = vec4((p * 2.0 - 1.0) * scale, 0.0, 1.0);
}
