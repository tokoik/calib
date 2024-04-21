#pragma once

///
/// 画像処理クラスの定義（コンピュートシェーダ版）
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"
using namespace gg;

///
/// 画像処理クラスの定義（コンピュートシェーダ版）
///
class Compute
{
  /// 計算用のシェーダプログラム
  const GLuint program;

public:

  ///
  /// コンストラクタ
  ///
  /// @param comp コンピュートシェーダのソースファイル名
  ///
  Compute(const char* comp)
    : program(ggLoadComputeShader(comp))
  {
  }

  ///
  /// デストラクタ
  ///
  virtual ~Compute()
  {
    // シェーダプログラムを削除する
    glDeleteShader(program);
  }

  ///
  /// シェーダプログラムを得る
  ///
  /// @return シェーダのプログラムオブジェクト
  ///
  auto get() const
  {
    return program;
  }

  ///
  /// 計算用のシェーダプログラムの使用を開始する
  ///
  void use() const
  {
    glUseProgram(program);
  }

  ///
  /// 計算を実行する
  ///
  /// @param width 画像の横の画素数
  /// @param height 画像の縦の画素数
  /// @param local_size_x ワークグループの横方向のスレッド数
  /// @param local_size_y ワークグループの縦方向のスレッド数
  ///
  void execute(GLuint width, GLuint height, GLuint local_size_x = 1, GLuint local_size_y = 1) const
  {
    glDispatchCompute((width + local_size_x - 1) / local_size_x, (height + local_size_y - 1) / local_size_y, 1);
  }
};
