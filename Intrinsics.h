#pragma once

///
/// キャプチャデバイス固有のパラメータクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// 構成ファイルの読み取り補助
#include "parseconfig.h"

// 標準ライブラリ
#include <array>

///
/// キャプチャデバイス固有のパラメータ
///
struct Intrinsics
{
  /// キャプチャデバイスのレンズの縦横の画角
  std::array<float, 2> fov;

  /// キャプチャデバイスのレンズの中心 (主点) の位置
  std::array<float, 2> center;

  /// キャプチャデバイスの解像度
  std::array<int, 2> size;

  /// キャプチャデバイスのフレームレート
  double fps;

  ///
  /// キャプチャデバイス固有のパラメータの構造体のデフォルトコンストラクタ
  ///
  /// @note 構成ファイルが読めなかったときにしか使わない
  ///
  Intrinsics()
    : Intrinsics{ { 50.03f, 38.58f }, { 0.0f, 0.0f }, { 640, 480 }, 30.0 }
  {
  }

  ///
  /// パラメータを指定するときに使う
  /// キャプチャデバイス固有のパラメータの構造体のコンストラクタ
  ///
  /// @param fov キャプチャデバイスのレンズの縦横の画角
  /// @param center キャプチャデバイスのレンズの中心 (主点) の位置
  /// @param resolution キャプチャデバイスの解像度
  /// @param fps キャプチャデバイスのフレームレート
  ///
  Intrinsics(const std::array<float, 2>& fov, const std::array<float, 2>& center,
    const std::array<int, 2> size, double fps)
    : fov{ fov }
    , center{ center }
    , size{ size }
    , fps{ fps }
  {
  }

  ///
  /// 構成ファイルを読み込むときに使う
  /// キャプチャデバイス固有のパラメータの構造体のコンストラクタ
  ///
  Intrinsics(const picojson::object& object);

  /// 展開時のセンサーサイズ
  static constexpr auto sensorSize{ 35.0f };

  ///
  /// スクリーンの高さをもとにしてキャプチャデバイスのレンズの縦横の画角を変更する
  ///
  /// @param focal 撮像面の対角線長が sensorSize のときの焦点距離
  ///
  void setFov(float focal);

  ///
  /// キャプチャデバイスのレンズの縦横の画角を変更する
  ///
  /// @param fovx キャプチャデバイスのレンズの fovx 方向の画角
  /// @param fovy キャプチャデバイスのレンズの fovy 方向の画角
  ///
  void setFov(float fovx, float fovy)
  {
    // キャプチャデバイスのレンズの縦横の画角を設定する
    fov[0] = fovx;
    fov[1] = fovy;
  }

  ///
  /// キャプチャデバイスのレンズの中心の位置を変更する
  ///
  /// @param x キャプチャデバイスのレンズの中心の位置の x 座標
  /// @param y キャプチャデバイスのレンズの中心の位置の y 座標
  ///
  void setCenter(float x, float y)
  {
    // キャプチャデバイスのレンズの中心の位置を設定する
    center[0] = x;
    center[1] = y;
  }


  ///
  /// キャプチャデバイスの解像度を変更する
  ///
  /// @param width キャプチャデバイスのフレームの横の画素数
  /// @param height キャプチャデバイスのフレームの縦の画素数
  ///
  void setSize(int width, int height)
  {
    // 装置の解像度を設定する
    size[0] = width;
    size[1] = height;
  }


  ///
  /// キャプチャデバイスのフレームレートを変更する
  ///
  /// @param frequency フレームレート
  ///
  void setFps(double frequency)
  {
    // 装置のフレームレートを設定する
    fps = frequency;
  }
};
