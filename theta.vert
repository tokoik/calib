#version 410

//
// RICOH THETA S の二重魚眼画像の平面展開
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
out vec2 texcoord_b;
out vec2 texcoord_f;

// 前後のテクスチャの混合比
out float blend;

void main(void)
{
  // 投影像のサイズ
  vec2 size = vec2(textureSize(image, 0));

  // 投影像の後方カメラ像のテクスチャ空間上の半径（画角×π/360/2）と中心
  vec2 radius_b = circle.st * vec2(-0.25, 0.25 * size.x / size.y) * 0.00436332313;
  vec2 center_b = vec2(circle.p - radius_b.s, radius_b.t - circle.q);

  // 投影像の前方カメラ像のテクスチャ空間上の半径と中心
  vec2 radius_f = vec2(-radius_b.s, radius_b.t);
  vec2 center_f = vec2(center_b.s + 0.5, center_b.t);

  // 頂点位置
  //   各頂点において gl_VertexID が 0, 1, 2, 3, ... のように割り当てられるから、
  //     x = gl_VertexID >> 1      = 0, 0, 1, 1, 2, 2, 3, 3, ...
  //     y = 1 - (gl_VertexID & 1) = 1, 0, 1, 0, 1, 0, 1, 0, ...
  //   のように GL_TRIANGLE_STRIP 向けの頂点座標値が得られる。
  //   y に gl_InstaceID を足せば glDrawArrayInstanced() のインスタンスごとに y が変化する。
  //   これに格子の間隔 gap をかけて 1 を引けば縦横 [-1, 1] の範囲の点群 position が得られる。
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
  vec2 p = position * screen.st + screen.pq;

  // スクリーンに向かう視線ベクトル
  vec3 vector = normalize(mat3(rotation) * vec3(p, -focal));

  // この方向ベクトルの相対的な仰角
  //   acos(-vector.z) * 2 / π - 1 → [正面:1, 背面:-1]
  float angle = acos(vector.z) * 0.63661977 - 1.0;

  // 前後のテクスチャの混合比
  blend = smoothstep(-0.02, 0.02, angle);

  // この方向ベクトルの yx 上での方向ベクトル
  vec2 orientation = normalize(vector.yx) * 0.885;

  // テクスチャ座標
  texcoord_b = (1.0 + angle) * orientation * radius_b + center_b;
  texcoord_f = (1.0 - angle) * orientation * radius_f + center_f;
}
