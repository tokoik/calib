#version 410

//
// 等距離射影方式の魚眼レンズ画像の平面展開
//

// テクスチャ
uniform sampler2D image;

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
  // 投影像のサイズ
  vec2 size = vec2(textureSize(image, 0));

  // 投影像のテクスチャ空間上のスケール (180/π = 57.2957795)
  vec2 scale = -57.2957795 * size.yx / (size.x * circle.st);

  // 投影像のテクスチャ空間上の中心位置
  vec2 center = circle.pq + 0.5;

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

  // スクリーン上の点の位置と視線ベクトル
  //   position にスクリーンの大きさ screen.st をかけて中心位置 screen.pq を足せば、
  //   スクリーン上の点の位置 p が得られるから、原点にある視点からこの点に向かう視線は、
  //   焦点距離 focal を Z 座標に用いて (p, -focal) となる。
  //   これを回転したあと正規化して、その方向の視線単位ベクトルを得る。
  vec2 p = screen.pq - position * screen.st;

  // スクリーンに向かう視線ベクトル
  vec3 vector = normalize(mat3(rotation) * vec3(p, -focal));

  // テクスチャ座標
  texcoord = acos(-vector.z) * normalize(vector.xy) * scale + center;
}
