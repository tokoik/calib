///
/// フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Framebuffer.h"

//
// フレームバッファオブジェクトを作成する
//
void Framebuffer::createFramebuffer()
{
  // 新しいフレームバッファオブジェクトを作成する
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.getTexture(), 0);
  glDrawBuffers(1, &attachment);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//
// テクスチャからフレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(const Texture& texture)
  : texture{ texture }
  , attachment{ GL_COLOR_ATTACHMENT0 }
{
  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();
}

//
// デストラクタ
//
Framebuffer::~Framebuffer()
{
  // フレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);

  // デフォルトのフレームバッファオブジェクトに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  framebuffer = 0;
}

//
// 現在のテクスチャをカラーバッファに使ってフレームバッファオブジェクトを作り直す
//
void Framebuffer::update()
{
  // 以前のフレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);

  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();
}

//
// レンダリング先をフレームバッファオブジェクトに切り替える
//
void Framebuffer::use() const
{
  // 描画先をフレームバッファオブジェクトに切り替える
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  // ビューポートをフレームバッファオブジェクトに合わせる
  const auto& size{ texture.getSize() };
  glViewport(0, 0, size.width, size.height);
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
  // テクスチャのサイズ
  const auto& size{ texture.getSize() };

  // フレームバッファオブジェクトのアスペクト比
  const auto f{ static_cast<float>(size.width * height) };

  // ウィンドウの表示領域のアスペクト比
  const auto d{ static_cast<float>(size.height * width) };

  // 描画する領域
  GLint dx0, dy0, dx1, dy1;

  // 表示領域の右上端の位置を求める
  --width;
  --height;

  // フレームバッファオブジェクトのアスペクト比が大きかったら
  if (f > d)
  {
    // ディスプレイ上の描画する領域の高さを求める
    const auto h{ static_cast<GLint>(d / size.width + 0.5f) };

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
    const auto w{ static_cast<GLint>(f / size.height + 0.5f) };

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
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
  glReadBuffer(attachment);

  // フレームバッファオブジェクトの内容を通常のフレームバッファに書き込む
  glBlitFramebuffer(0, 0, size.width - 1, size.height - 1,
    dx0, dy1, dx1, dy0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // 読み込み元を通常のフレームバッファに戻す
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
}
