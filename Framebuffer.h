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
  /// フレームバッファのカラーバッファのサイズ
  std::array<int, 2> size;

  /// フレームバッファのカラーバッファのチャネル数
  int channels;

  /// フレームバッファオブジェクト名
  GLuint name;

  /// フレームバッファオブジェクトのレンダーターゲット
  GLenum attachment;

  /// フレームバッファのカラーバッファに使うテクスチャ
  Texture& texture;

  ///
  /// フレームバッファオブジェクトを初期化する
  ///
  void initialize();

  ///
  /// フレームバッファオブジェクトを作成する
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  ///
  void createFramebuffer(GLuint texture);

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Framebuffer()
    : size{ 0, 0 }
    , channels{ 0 }
    , name{ 0 }
    , attachment{ GL_COLOR_ATTACHMENT0 }
    , texture { Texture{} }
  {
  }

  ///
  /// 指定したテクスチャをカラーバッファに使って
  /// フレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  ///
  Framebuffer(Texture& texture, GLenum attachment = GL_COLOR_ATTACHMENT0);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param framebuffer コピー元
  ///
  Framebuffer(const Framebuffer& framebuffer) = delete;

  ///
  /// ムーブコンストラクタ
  ///
  /// @param framebuffer ムーブ元
  ///
  Framebuffer(Framebuffer&& framebuffer) noexcept;

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
  /// ムーブ代入演算子
  ///
  /// @param framebuffer ムーブ代入元
  ///
  Framebuffer& operator=(Framebuffer&& framebuffer) noexcept;

  ///
  /// フレームバッファオブジェクトを破棄する
  ///
  void discard();

  ///
  /// テクスチャを得る
  ///
  /// @return テクスチャ名
  ///
  auto getFramebufferName() const
  {
    return name;
  }

  ///
  /// フレームバッファオブジェクトのサイズを得る
  ///
  const auto& getSize() const
  {
    return size;
  }

  ///
  /// フレームバッファオブジェクトの横の画素数を得る
  ///
  const auto getWidth() const
  {
    return size[0];
  }

  ///
  /// フレームバッファオブジェクトの縦の画素数を得る
  ///
  const auto getHeight() const
  {
    return size[1];
  }

  ///
  /// フレームバッファオブジェクトのチャネル数を得る
  ///
  const auto getChannels() const
  {
    return channels;
  }

  ///
  /// レンダリング先をフレームバッファオブジェクトに切り替える
  ///
  void use();

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
