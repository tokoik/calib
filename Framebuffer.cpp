﻿///
/// フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Framebuffer.h"

//
// フレームバッファオブジェクトを初期化する
//
void Framebuffer::initialize()
{
  // バッファオブジェクトのメンバを初期化する
  size = std::array<int, 2>{ 0, 0 };
  channels = 0;
  name = 0;
  attachment = GL_COLOR_ATTACHMENT0;
}

//
// フレームバッファオブジェクトを作成する
//
void Framebuffer::createFramebuffer(GLuint texture)
{
  // 新しいフレームバッファオブジェクトを作成する
  glGenFramebuffers(1, &name);
  glBindFramebuffer(GL_FRAMEBUFFER, name);
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture, 0);
  glDrawBuffers(1, &attachment);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//
// 指定したテクスチャをカラーバッファに使って
// フレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(Texture& texture, GLenum attachment)
  : size{ texture.getSize() }
  , channels{ texture.getChannels() }
  , attachment{ attachment }
  , texture{ texture }
{
  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer(texture.getTextureName());
}

///
/// ムーブコンストラクタ
///
/// @param framebuffer ムーブ元
///
Framebuffer::Framebuffer(Framebuffer&& framebuffer) noexcept
  : size{ framebuffer.getSize() }
  , channels{ framebuffer.getChannels() }
  , name{ framebuffer.name }
  , attachment{ framebuffer.attachment }
  , texture{ framebuffer.texture }
{
  // ムーブ元のフレームバッファオブジェクトのメンバを初期化する
  framebuffer.initialize();
}

//
// デストラクタ
//
Framebuffer::~Framebuffer()
{
  discard();
}

//
// ムーブ代入演算子
//
Framebuffer& Framebuffer::operator=(Framebuffer&& framebuffer) noexcept
{
  if (&framebuffer != this)
  {
    // ムーブ元のフレームバッファオブジェクトのメンバをムーブ先にコピーする
    size = framebuffer.size;
    channels = framebuffer.channels;
    name = framebuffer.name;
    attachment = framebuffer.attachment;
    texture = std::move(framebuffer.texture);

    // ムーブ元のフレームバッファオブジェクトのメンバを初期化する
    framebuffer.initialize();
  }

  return *this;
}

//
// フレームバッファオブジェクトを破棄する
//
void Framebuffer::discard()
{
  // デフォルトのフレームバッファオブジェクトに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // フレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &name);

  // フレームバッファオブジェクトのメンバを初期化する
  initialize();
}

//
// レンダリング先をフレームバッファオブジェクトに切り替える
//
void Framebuffer::use()
{
  // フレームバッファオブジェクトのサイズがカラーバッファと違っていたら
  if (texture.getWidth() != size[0]
    || texture.getHeight() != size[1]
    || texture.getChannels() != channels)
  {
    // 以前のフレームバッファオブジェクトを削除する
    glDeleteFramebuffers(1, &name);

    // カラーバッファに使うテクスチャのサイズを記録する
    size = texture.getSize();
    channels = texture.getChannels();

    // 新しいフレームバッファオブジェクトを作成する
    createFramebuffer(texture.getTextureName());
  }

  // 描画先をフレームバッファオブジェクトに切り替える
  glBindFramebuffer(GL_FRAMEBUFFER, name);

  // ビューポートをフレームバッファオブジェクトに合わせる
  glViewport(0, 0, texture.getWidth(), texture.getHeight());
}

//
// レンダリング先を通常のフレームバッファに戻す
//
void Framebuffer::unuse() const
{
  // 描画先を通常のフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 読み書きを通常のフレームバッファのバックバッファに対して行う
  glReadBuffer(GL_BACK);
  glDrawBuffer(GL_BACK);
}

//
// フレームバッファオブジェクトの表示
//
void Framebuffer::draw(GLsizei width, GLsizei height) const
{
  // フレームバッファオブジェクトのアスペクト比
  const auto f{ static_cast<float>(texture.getWidth() * height) };

  // ウィンドウの表示領域のアスペクト比
  const auto d{ static_cast<float>(texture.getHeight() * width) };

  // 描画する領域
  GLint dx0, dy0, dx1, dy1;

  // 表示領域の右上端の位置を求める
  --width;
  --height;

  // フレームバッファオブジェクトのアスペクト比が大きかったら
  if (f > d)
  {
    // ディスプレイ上の描画する領域の高さを求める
    const auto h{ static_cast<GLint>(d / texture.getWidth() + 0.5f)};

    // 表示が横長なので描画する領域の横幅いっぱいに表示する
    dx0 = 0;
    dx1 = width;

    // 高さはアスペクト比を維持して描画する領域の中央に描く
    dy0 = (height - h) / 2;
    dy1 = dy0 + h;
  }
  else
  {
    // ディスプレイ上の描画する領域の幅を求める
    const auto w{ static_cast<GLint>(f / texture.getHeight() + 0.5f) };

    // 表示が縦長なので描画する領域の高さいっぱいに表示する
    dy0 = 0;
    dy1 = height;

    // 横幅はアスペクト比を維持して描画する領域の中央に描く
    dx0 = (width - w) / 2;
    dx1 = dx0 + w;
  }

  // 書き込み先を通常のフレームバッファにする
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  // フレームバッファオブジェクトを読み込み元にする
  glBindFramebuffer(GL_READ_FRAMEBUFFER, name);
  glReadBuffer(attachment);

  // フレームバッファオブジェクトの内容を通常のフレームバッファに書き込む
  glBlitFramebuffer(0, 0, texture.getWidth() - 1, texture.getHeight() - 1,
    dx0, dy1, dx1, dy0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // 読み込み元を通常のフレームバッファに戻す
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
}
