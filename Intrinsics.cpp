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
void Intrinsics::setFov(float focal)
{
  // 撮像面の画素数の対角線長×２
  const auto d{ hypotf(static_cast<float>(size[0]), static_cast<float>(size[1])) * 2.0f };

  // 撮像面の画素数の対角線長に対するスクリーンの幅と高さの比の 1/2 (d は対角線長の２倍だから)
  const auto w{ size[0] / d };
  const auto h{ size[1] / d };

  // 焦点距離が１のときの撮像面の対角線長
  const auto t{ sensorSize / focal };

  // 焦点距離が１のときの撮像面の対角線長をこの比で横と縦に振り分ける
  const auto u{ t * w };
  const auto v{ t * h };

  // 画角を求め単位を度に換算して設定する
  constexpr auto s{ 360.0f / 3.14159265359f };
  setFov(atan(u) * s, atan(v) * s);
}
