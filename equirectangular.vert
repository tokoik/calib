#version 410

//
// 正距円筒図法画像の平面展開
//

// スクリーンを回転する変換行列
uniform mat4 rotation;

// スクリーンの大きさと中心位置
uniform vec4 screen;

// 頂点の位置
layout(location = 0) in vec2 position;

// 頂点の法線
layout(location = 1) in vec3 normal;

// 視線ベクトル
out vec3 vector;

void main(void)
{
  // 頂点の位置はそのままラスタライザに送る
  gl_Position = vec4(position, 0.0, 1.0);

  // 法線を縦横比でスケーリングする
  vec3 n = vec3(normal.xy * screen.st + screen.pq, normal.z);

  // 回転した頂点の法線を視線ベクトルに使う
  vector = normalize(mat3(rotation) * n);
}
