///
/// フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Framebuffer.h"

//
// フレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(GLsizei width, GLsizei height, int channels,
  GLenum attachment)
  : attachment{ attachment }
{
  // フレームバッファオブジェクトを作る
  Framebuffer::create(width, height, channels);
}

///
/// コピーコンストラクタ
///
/// @param framebuffer コピー元のフレームバッファオブジェクト
///
Framebuffer::Framebuffer(const Framebuffer& framebuffer)
  : attachment{ framebuffer.attachment }
{
  // フレームバッファオブジェクトをコピーして作成する
  Framebuffer::copy(framebuffer);
}

//
// デストラクタ
//
Framebuffer::~Framebuffer()
{
  // フレームバッファオブジェクトを破棄する
  Framebuffer::discard();
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
  // ムーブ代入元とムーブ代入先が同じなら何もしない
  if (&framebuffer == this) return *this;

  // ムーブ代入元の基底クラスをムーブする
  static_cast<Texture&>(*this) = std::move(framebuffer);

  // ムーブ代入元のテクスチャのメンバをムーブする
  framebufferSize = framebuffer.framebufferSize;
  framebuffer.framebufferSize = { 0, 0 };
  framebufferChannels = framebuffer.framebufferChannels;
  framebuffer.framebufferChannels = 0;
  framebuffer.attachment = GL_COLOR_ATTACHMENT0;

  // ムーブ代入先のフレームバッファを削除する
  glDeleteFramebuffers(1, &framebufferName);

  // ムーブ代入元のフレームバッファ名をムーブ代入先に移す
  framebufferName = framebuffer.framebufferName;

  // ムーブ代入元のデストラクタでフレームバッファが削除されないよう 0 にする
  framebuffer.framebufferName = 0;

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
}

//
// フレームバッファオブジェクトを作成する
//
void Framebuffer::create(GLsizei width, GLsizei height, int channels)
{
  // 既存のテクスチャを破棄して新しいテクスチャを作成する
  Texture::create(width, height, channels);

  // まだメッシュの頂点配列オブジェクトが作られていなければ作る
  if (!mesh) mesh = std::make_shared<Mesh>();

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
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, getTextureName(), 0);
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
  static constexpr GLenum bufs{
#if defined(GL_GLES_PROTOTYPES)
    GL_BACK
#else
    GL_BACK_LEFT
#endif
  };
  glDrawBuffers(1, &bufs);
  glReadBuffer(GL_BACK);
}

//
// テクスチャを展開してフレームバッファオブジェクトを更新する
//
void Framebuffer::update(const std::array<int, 2>& size)
{
  // 描画先をフレームバッファオブジェクトに切り替える
  bindFramebuffer();

  // テクスチャをフレームバッファオブジェクトに展開する
  mesh->drawMesh(size);

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
