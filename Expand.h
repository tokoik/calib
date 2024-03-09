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

  /// スクリーンの格子間隔の uniform 変数の場所
  const GLint gapLoc;

  /// スクリーンの投影範囲の uniform 変数の場所
  const GLint screenLoc;

  /// スクリーンまでの焦点距離の uniform 変数の場所
  const GLint focalLoc;

  /// スクリーンを回転する変換行列の uniform 変数の場所
  const GLint rotationLoc;

  /// レンズのイメージサークルの uniform 変数の場所
  const GLint circleLoc;

  /// 背景テクスチャのサンプラの uniform 変数の場所
  const GLint imageLoc;

#if defined(DO_NOT_USE_INSTANCING)
  /// インスタンス番号
  const GLint instanceLoc;
#endif

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
  Expand(const Expand& shader) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Expand();

  ///
  /// 代入演算子は使用しない
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
  /// 展開用シェーダの使用開始
  ///
  /// @param gap 展開するテクスチャをサンプリングする格子間隔 (2 / 解像度)]
  /// @param aspect 展開するテクスチャの縦横比
  /// @param focal 展開するテクスチャに対する焦点距離
  /// @param rotation 展開するテクスチャに対する回転
  /// @param circle 展開するテクスチャ上のイメージサークルの大きさと中心
  ///
  void use(const std::array<GLfloat, 2>& gap, GLfloat aspect, GLfloat focal,
    const gg::GgMatrix& rotation, const std::array<GLfloat, 4>& circle) const;
};
