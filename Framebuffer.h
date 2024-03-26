#pragma once

///
/// フレームバッファオブジェクトクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// テクスチャ
#include "Texture.h"

///
/// フレームバッファオブジェクトクラス
///
class Framebuffer
{
  /// フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  const Texture& texture;

  /// フレームバッファオブジェクトのレンダーターゲット
  GLenum attachment;

  /// フレームバッファオブジェクト
  GLuint framebuffer;

  ///
  /// 現在のテクスチャをカラーバッファに使って新しいフレームバッファオブジェクトを作成する
  ///
  void createFramebuffer();

public:

  ///
  /// 指定したテクスチャをカラーバッファに使ってフレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  ///
  Framebuffer(const Texture& texture);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param framebuffer コピー元
  ///
  Framebuffer(const Framebuffer& framebuffer) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Framebuffer();

  ///
  /// 代入演算子は使用しない
  ///
  /// @param framebuffer 代入元
  ///
  Framebuffer& operator=(const Framebuffer& framebuffer) = delete;

  ///
  /// 現在のテクスチャをカラーバッファに使ってフレームバッファオブジェクトを作り直す
  ///
  void update();

  ///
  /// レンダリング先をフレームバッファオブジェクトに切り替える
  ///
  void use() const;

  ///
  /// レンダリング先を通常のフレームバッファに戻す
  ///
  void unuse() const;

  ///
  /// フレームバッファオブジェクトの内容を表示する
  ///
  /// @param width フレームバッファオブジェクトの内容を表示する横の画素数
  /// @param height フレームバッファオブジェクトの内容を表示する縦の画素数
  ///
  void draw(GLsizei width, GLsizei height) const;
};
