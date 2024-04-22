///
/// テクスチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Texture.h"

//
// テクスチャを作成するコンストラクタ
//
Texture::Texture(GLsizei width, GLsizei height, int channels)
{
  // テクスチャを作る
  Texture::create(width, height, channels);
}

//
// コピーコンストラクタ
//
Texture::Texture(const Texture& texture)
{
  // テクスチャをコピーして作成する
  Texture::copy(texture);
}

//
// ムーブコンストラクタ
//
Texture::Texture(Texture&& texture) noexcept
{
  // テクスチャをムーブして作成する
  *this = std::move(texture);
}

//
// デストラクタ
//
Texture::~Texture()
{
  // テクスチャを破棄する
  Texture::discard();
}

//
// テクスチャを作成する
//
void Texture::create(GLsizei width, GLsizei height, int channels)
{
  // 既存のバッファを破棄して新しいバッファを作成する
  Buffer::create(width, height, channels);

  // まだメッシュの頂点配列オブジェクトが作られていなければ作る
  if (!mesh) mesh = std::make_shared<Mesh>();

  // 指定したサイズが保持しているテクスチャのサイズと同じなら何もしない
  if (width == textureSize[0] && height == textureSize[1]
    && channels == textureChannels) return;

  // テクスチャのサイズとチャンネル数を記録する
  textureSize = std::array<int, 2>{ width, height };
  textureChannels = channels;

  // 以前のテクスチャを削除する
  glDeleteTextures(1, &textureName);

  // テクスチャを作成する
  glGenTextures(1, &textureName);

  // テクスチャのメモリを確保してデータを転送する
  glBindTexture(GL_TEXTURE_2D, textureName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
    channelsToFormat(channels), GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

//
// テクスチャをコピーする
//
void Texture::copy(const Buffer& texture) noexcept
{
  // コピー元と同じサイズの空のテクスチャを作る
  Texture::create(texture.getWidth(), texture.getHeight(),
    texture.getChannels());

  // 作成したテクスチャのバッファにコピーする
  copyBuffer(texture);
}

//
// 代入演算子
//
Texture& Texture::operator=(const Texture& texture)
{
  // 代入元と代入先が同じでなければ
  if (&texture != this)
  {
    // 引数のテクスチャをこのテクスチャにコピーする
    Texture::copy(texture);
  }

  // このテクスチャを返す
  return *this;
}

//
// ムーブ代入演算子
//
Texture& Texture::operator=(Texture&& texture) noexcept
{
  // 代入元と代入先が同じでなければ
  if (&texture != this)
  {
    // 引数のテクスチャをこのテクスチャにコピーする
    Texture::copy(texture);

    // ムーブ元のバッファを破棄する
    texture.Buffer::discard();

    // ムーブ元のテクスチャを破棄する
    texture.Texture::discard();
  }

  // このテクスチャを返す
  return *this;
}

//
// テクスチャを破棄する
//
void Texture::discard()
{
  // デフォルトのテクスチャに戻す
  glBindTexture(GL_TEXTURE_2D, 0);

  // テクスチャを削除する
  glDeleteTextures(1, &textureName);
  textureName = 0;

  // テクスチャのサイズを 0 にする
  textureSize = std::array<int, 2>{ 0, 0 };
  textureChannels = 0;
}

//
// このテクスチャをマッピングしてメッシュを描画する
//
void Texture::draw(GLsizei width, GLsizei height, int unit) const
{
  // テクスチャの縦横比
  const auto t{ static_cast<float>(textureSize[0] * height)};

  // 表示領域の縦横比
  const auto d{ static_cast<float>(textureSize[1] * width)};

  // テクスチャのスケール
  const std::array<GLfloat, 2> scale{ t > d // テクスチャの縦横比の方が
    ? std::array<GLfloat, 2>{ 1.0f, t / d } // 大きければ横方向いっぱいに描く
    : std::array<GLfloat, 2>{ d / t, 1.0f } // それ以外は縦方向いっぱいに描く
  };

  // このオブジェクトのテクスチャを指定する
  bindTexture(unit);

  // メッシュを描画する
  mesh->draw(scale, unit);

  // テクスチャの指定を解除する
  unbindTexture();
}

//
// テクスチャからピクセルバッファオブジェクトにデータをコピーする
//
void Texture::readPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
  GLuint buffer) const
#else
std::vector<GLubyte>& buffer)
#endif
{
  // 読み出し元のテクスチャを結合する
  glBindTexture(GL_TEXTURE_2D, textureName);

#if defined(USE_PIXEL_BUFFER_OBJECT)
  // 書き込み先のピクセルバッファオブジェクトを指定する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);

#if defined(GL_GLES_PROTOTYPES)
  // 現在使っているフレームバッファオブジェクトを調べる
  GLuint currentFbo;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, reinterpret_cast<GLint*>(&currentFbo));

  // テクスチャをカラーバッファに使ってフレームバッファオブジェクトを作る
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  static const GLenum attachment{ GL_COLOR_ATTACHMENT0 };
  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, textureName, 0);
  glDrawBuffers(1, &attachment);
  glReadBuffer(attachment);

  // フレームバッファオブジェクトからピクセルバッファオブジェクトに読み出す
  glReadPixels(0, 0, textureSize[0], textureSize[1], channelsToFormat(textureChannels),
    GL_UNSIGNED_BYTE, 0);

  // 元のフレームバッファオブジェクトに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, currentFbo);

  // 作ったフレームバッファオブジェクトは削除する
  glDeleteFramebuffers(1, &fbo);
#else
  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, getFormat(), GL_UNSIGNED_BYTE, 0);
#endif

  // 書き込み先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

#else

  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, getFormat(), GL_UNSIGNED_BYTE, buffer.data());

#endif

  // 読み出し元のテクスチャの結合を解除する
  glBindTexture(GL_TEXTURE_2D, 0);
}

//
// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
//
void Texture::drawPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
  GLuint
#else
  const std::vector<GLubyte>&
#endif
  buffer) const
{
  // 書き込み先のテクスチャを結合する
  glBindTexture(GL_TEXTURE_2D, textureName);

#if defined(USE_PIXEL_BUFFER_OBJECT)

  // 読み出し元のピクセルバッファオブジェクトを指定する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);

  // ピクセルバッファオブジェクトの内容をテクスチャに書き込む
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize[0], textureSize[1],
    getFormat(), GL_UNSIGNED_BYTE, 0);

  // 読み出し元のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

#else

  // ピクセルバッファオブジェクトの内容をテクスチャに書き込む
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize[0], textureSize[1],
    getFormat(), GL_UNSIGNED_BYTE, buffer.data());

#endif

  // 書き込み先のテクスチャの結合を解除する
  glBindTexture(GL_TEXTURE_2D, 0);
}

// 展開に用いるメッシュの頂点配列オブジェクト
std::shared_ptr<Mesh> Texture::mesh;
