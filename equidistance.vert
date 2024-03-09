#version 410

//
// 等距離射影方式の魚眼レンズ画像の平面展開
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
out vec2 texcoord;

void main(void)
{
  // 頂点の位置はそのままラスタライザに送る
  gl_Position = vec4(position, 0.0, 1.0);

  // 法線を縦横比でスケーリングする
  vec3 n = vec3(normal.xy * screen.st + screen.pq, normal.z);

  // 回転した法線を視線ベクトルに使う
  vec3 vector = normalize(mat3(rotation) * n);

  // 背景テクスチャのサイズ
  vec2 size = textureSize(image, 0);

  // 背景テクスチャのテクスチャ空間上のスケール (180/π = 57.2957795)
  vec2 scale = 57.2957795 * size.yx / (size.x * circle.st);

  // 背景テクスチャのテクスチャ空間上の中心位置
  vec2 center = circle.pq + 0.5;

  // テクスチャ座標
  texcoord = acos(vector.z) * normalize(vector.xy) * scale + center;
}
