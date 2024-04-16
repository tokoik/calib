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
GLuint Framebuffer::createFramebuffer(GLuint textureName, GLenum attachment)
{
  GLuint framebufferName;
  glGenFramebuffers(1, &framebufferName);
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, textureName, 0);
  glDrawBuffers(1, &attachment);
  glReadBuffer(attachment);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return framebufferName;
}

//
// フレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(GLsizei width, GLsizei height, int channels,
  GLenum attachment)
  : attachment{ attachment }
  , viewport{ 0, 0, 0, 0 }
{
  Framebuffer::create(width, height, channels);
}

///
/// コピーコンストラクタ
///
/// @param framebuffer コピー元のフレームバッファオブジェクト
///
Framebuffer::Framebuffer(const Framebuffer& framebuffer)
  : attachment{ framebuffer.attachment }
  , viewport{ framebuffer.viewport }
{
  Framebuffer::copy(framebuffer);
}

///
/// ムーブコンストラクタ
///
/// @param framebuffer ムーブ元
///
Framebuffer::Framebuffer(Framebuffer&& framebuffer) noexcept
{
  *this = std::move(framebuffer);
}

//
// デストラクタ
//
Framebuffer::~Framebuffer()
{
  discard();
}

//
// 代入演算子
//
Framebuffer& Framebuffer::operator=(const Framebuffer& framebuffer)
{
  // 代入元と代入先が同じでなければ
  if (&framebuffer != this)
  {
    // 引数のフレームバッファオブジェクトをコピーする
    Framebuffer::copy(framebuffer);
  }

  // このフレームバッファオブジェクトを返す
  return *this;
}

//
// ムーブ代入演算子
//
Framebuffer& Framebuffer::operator=(Framebuffer&& framebuffer) noexcept
{
  // 代入元と代入先が同じでなければ
  if (&framebuffer != this)
  {
    // ムーブ元のフレームバッファオブジェクトをコピーする
    Framebuffer::copy(framebuffer);

    // ムーブ元のバッファを破棄する
    framebuffer.Buffer::discard();

    // ムーブ元のテクスチャを破棄する
    framebuffer.Texture::discard();

    // ムーブ元のフレームバッファオブジェクトを破棄する
    framebuffer.Framebuffer::discard();
  }

  // このフレームバッファオブジェクトを返す
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
  glDeleteFramebuffers(1, &framebufferName);
  framebufferName = 0;

  // バッファオブジェクトのメンバを初期化する
  framebufferSize = std::array<int, 2>{ 0, 0 };
  framebufferChannels = 0;
  attachment = GL_COLOR_ATTACHMENT0;
  viewport = std::array<GLint, 4>{ 0, 0, 0, 0 };
}

//
// フレームバッファオブジェクトを作成する
//
void Framebuffer::create(GLsizei width, GLsizei height, int channels)
{
  // 既存のテクスチャを破棄して新しいテクスチャを作成する
  Texture::create(width, height, channels);

  // 指定したサイズがフレームバッファオブジェクトのサイズと同じなら何もしない
  if (width == framebufferSize[0] && height == framebufferSize[1]
    && channels == framebufferChannels) return;

  // フレームバッファオブジェクトのサイズとチャンネル数を記録する
  framebufferSize = std::array<int, 2>{ width, height };
  framebufferChannels = channels;

  // 以前のフレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebufferName);

  // フレームバッファオブジェクトを作成する
  glGenFramebuffers(1, &framebufferName);
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, getTextureName(), 0);
  glDrawBuffers(1, &attachment);
  glReadBuffer(attachment);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//
// フレームバッファオブジェクトをコピーする
//
void Framebuffer::copy(const Buffer& framebuffer) noexcept
{
  // コピー元と同じサイズの空のフレームバッファオブジェクトを作る
  Framebuffer::create(framebuffer.getWidth(), framebuffer.getHeight(),
    framebuffer.getChannels());

  // 作成したフレームバッファオブジェクトのバッファにコピーする
  copyBuffer(framebuffer);
}

//
// レンダリング先をフレームバッファオブジェクトに切り替える
//
void Framebuffer::bindFramebuffer()
{
  // 現在のビューポートを保存する
  glGetIntegerv(GL_VIEWPORT, viewport.data());

  // 描画先をフレームバッファオブジェクトに切り替える
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);

  // ビューポートをフレームバッファオブジェクトに合わせる
  glViewport(0, 0, framebufferSize[0], framebufferSize[1]);
}

//
// レンダリング先を通常のフレームバッファに戻す
//
void Framebuffer::unbindFramebuffer() const
{
  // 描画先を通常のフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 読み書きを通常のフレームバッファのバックバッファに対して行う
  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  // ビューポートを復帰する
  glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

//
// テクスチャを展開してフレームバッファオブジェクトを更新する
//
void Framebuffer::update(const std::array<int, 2>& size)
{
  // 描画先をフレームバッファオブジェクトに切り替える
  bindFramebuffer();

  // テクスチャをフレームバッファオブジェクトに展開する
  mesh.draw(size);

  // 描画先を通常のフレームバッファに戻す
  unbindFramebuffer();
}

//
// テクスチャを展開してフレームバッファオブジェクトを更新する
//
void Framebuffer::update(const std::array<int, 2>& size, const Texture& frame, int unit)
{
  // 展開するするテクスチャを指定する
  frame.bindTexture(unit);

  // テクスチャをフレームバッファオブジェクトに展開する
  update(size);

  // 展開するするテクスチャの指定を解除する
  frame.unbindTexture();
}

//
// フレームバッファオブジェクトの表示
//
void Framebuffer::draw(GLsizei width, GLsizei height) const
{
  // フレームバッファオブジェクトの縦横比
  const auto f{ static_cast<float>(getWidth() * height) };

  // ウィンドウの表示領域の縦横比
  const auto d{ static_cast<float>(getHeight() * width) };

  // 描画する領域
  GLint dx0, dy0, dx1, dy1;

  // 表示領域の右上端の位置を求める
  --width;
  --height;

  // フレームバッファオブジェクトの縦横比が大きかったら
  if (f > d)
  {
    // ディスプレイ上の描画する領域の高さを求める
    const auto h{ static_cast<GLint>(d / getWidth() + 0.5f) };

    // 表示が横長なので描画する領域の横幅いっぱいに表示する
    dx0 = 0;
    dx1 = width;

    // 高さは縦横比を維持して描画する領域の中央に描く
    dy0 = (height - h) / 2;
    dy1 = dy0 + h;
  }
  else
  {
    // ディスプレイ上の描画する領域の幅を求める
    const auto w{ static_cast<GLint>(f / getHeight() + 0.5f) };

    // 表示が縦長なので描画する領域の高さいっぱいに表示する
    dy0 = 0;
    dy1 = height;

    // 横幅は縦横比を維持して描画する領域の中央に描く
    dx0 = (width - w) / 2;
    dx1 = dx0 + w;
  }

  // フレームバッファオブジェクトを読み込み元にする
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferName);
  glReadBuffer(attachment);

  // 現在のフレームバッファを消去する
  glClear(GL_COLOR_BUFFER_BIT);

  // フレームバッファオブジェクトの内容を通常のフレームバッファに書き込む
  glBlitFramebuffer(0, 0, getWidth() - 1, getHeight() - 1,
    dx0, dy1, dx1, dy0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // 読み込み元を通常のフレームバッファに戻す
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
}
