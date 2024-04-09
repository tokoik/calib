#pragma once

///
/// 展開用シェーダクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"

///
/// 展開用シェーダクラス
///
class Expand
{
  /// プログラムオブジェクト
  const GLuint program;

  /// 投影像のサンプラの uniform 変数の場所
  const GLint imageLoc;

  /// スクリーンの投影範囲の uniform 変数の場所
  const GLint screenLoc;

  /// スクリーンまでの焦点距離の uniform 変数の場所
  const GLint focalLoc;

  /// スクリーンを回転する変換行列の uniform 変数の場所
  const GLint rotationLoc;

  /// レンズのイメージサークルの uniform 変数の場所
  const GLint circleLoc;

  /// 境界色の uniform 変数の場所
  const GLint borderLoc;

  /// スクリーンの格子間隔の uniform 変数の場所
  const GLint gapLoc;

public:

  ///
  /// コンストラクタ
  ///
  /// @param vert バーテックスシェーダのソースファイル名
  /// @param frag フラグメントシェーダのソースファイル名
  ///
  Expand(const std::string& vert, const std::string& frag);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param shader コピー元のシェーダ
  ///
  Expand(const Expand& shader) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Expand();

  ///
  /// 代入演算子は使用しない
  ///
  /// @param shader 代入元のシェーダ
  ///
  Expand& operator=(const Expand& shader) = delete;

  ///
  /// 展開用シェーダのプログラムオブジェクトを得る
  ///
  /// @return 展開用シェーダのプログラムオブジェクト
  ///
  auto getProgram() const
  {
    return program;
  }

  ///
  /// 展開
  ///
  /// @param samples 展開するテクスチャをサンプリングする数
  /// @param aspect 展開するテクスチャの縦横比
  /// @param pose サンプリングに用いるカメラの姿勢
  /// @param fov サンプリングに用いるカメラの相対画角（単位は度）
  /// @param center サンプリングに用いるカメラの撮像面上の中心 (主点) 位置
  /// @oaram focal サンプリングに用いるカメラの主点とスクリーンの距離
  /// @param border 展開後のフレームの境界色
  /// @param unit テクスチャユニット番号
  /// @return 描画すべきメッシュの横と縦の格子点数
  ///
  /// @note 展開するテクスチャをマッピングするメッシュの解像度を
  ///       samples と aspect から個々のメッシュが正方形に近くな
  ///       るよう決定し、それをもとに格子の間隔を求める
  ///
  std::array<GLsizei, 2> setup(int samples, GLfloat aspect,  const gg::GgMatrix& pose,
    const std::array<GLfloat, 2>& fov, const std::array<GLfloat, 2>& center, GLfloat focal,
    const std::array<GLfloat, 4>& border, int unit = 0) const;
};
