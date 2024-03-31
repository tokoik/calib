///
/// キャプチャデバイス固有のパラメータクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///
#include "Intrinsics.h"

//
// 構成ファイルを読み込むときに使う
// キャプチャデバイス固有のパラメータの構造体のコンストラクタ
//
Intrinsics::Intrinsics(const picojson::object& object)
{
  // キャプチャデバイスの画角
  getValue(object, "fov", fov);

  // キャプチャデバイスの中心位置
  getValue(object, "center", center);

  // キャプチャデバイスの解像度
  getValue(object, "size", size);

  // キャプチャデバイスの周波数
  getValue(object, "fps", fps);
}

//
// スクリーンの対角線長をもとにしてキャプチャデバイスのレンズの縦横の画角を変更する
//
void Intrinsics::setFov(float tangent)
{
  // 撮像面の画素数の対角線長×2
  const auto d{ hypotf(static_cast<float>(size[0]), static_cast<float>(size[1])) * 2.0f};

  // この対角線長に対するスクリーンの幅と高さの比の 1/2 (d を 2 倍にしているから)
  const auto w{ size[0] / d};
  const auto h{ size[1] / d};

  // 実際の対角線長をこの比で横と縦に振り分ける
  const auto u{ tangent * w };
  const auto v{ tangent * h };

  // 画角を度に換算して設定する
  constexpr auto s{ 360.0f / 3.14159265359f };
  setFov(atan(u) * s, atan(v) * s);
}
