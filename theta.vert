#version 410

//
// RICOH THETA S の二重魚眼画像の平面展開
//

// テクスチャ
uniform sampler2D image;

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
out vec2 texcoord_b;
out vec2 texcoord_f;

// 前後のテクスチャの混合比
out float blend;

void main(void)
{
  // 頂点の位置はそのままラスタライザに送る
  gl_Position = vec4(position, 0.0, 1.0);

  // 法線を縦横比でスケーリングする
  vec3 n = vec3(normal.xy * screen.st + screen.pq, normal.z);

  // 回転した法線を視線ベクトルに使う
  vec3 vector = normalize(mat3(rotation) * n);

  // この方向ベクトルの相対的な仰角
  //   1 - acos(vector.z) * 2 / π → [-1, 1]
  float angle = 1.0 - acos(vector.z) * 0.63661977;

  // 前後のテクスチャの混合比
  blend = smoothstep(-0.02, 0.02, angle);

  // この方向ベクトルの yx 上での方向ベクトル
  vec2 orientation = -0.885 * normalize(vector.yx);

  // 背景テクスチャのサイズ
  vec2 size = textureSize(image, 0);

  // 背景テクスチャの後方カメラ像のテクスチャ空間上の半径（画角×π/360/2）と中心
  vec2 radius_b = circle.st * vec2(-0.25, 0.25 * size.x / size.y) * 0.00436332313;
  vec2 center_b = vec2(radius_b.s - circle.p + 0.5, radius_b.t - circle.q);

  // 背景テクスチャの前方カメラ像のテクスチャ空間上の半径と中心
  vec2 radius_f = vec2(-radius_b.s, radius_b.t);
  vec2 center_f = vec2(center_b.s + 0.5, center_b.t);

  // テクスチャ座標
  texcoord_b = (1.0 - angle) * orientation * radius_b + center_b;
  texcoord_f = (1.0 + angle) * orientation * radius_f + center_f;
}
