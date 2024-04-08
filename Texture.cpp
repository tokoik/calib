///
/// テクスチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Texture.h"

//
// 新しいテクスチャを作成する
//
void Texture::create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  // テクスチャのサイズとフォーマットを記録する
  textureSize = std::array<int, 2>{ width, height };
  textureChannels = channels;

  // テクスチャァの読み書きに用いるピクセルバッファオブジェクトを作成する
  Buffer::resize(width, height, channels, pixels);

  // 以前のテクスチャを破棄する
  glDeleteTextures(1, &textureName);

  // テクスチャを作成する
  glGenTextures(1, &textureName);
  glBindTexture(GL_TEXTURE_2D, textureName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, getBufferFormat(), GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

//
// 新しいテクスチャを作成してそのピクセルバッファオブジェクトに別のテクスチャをコピーする
//
void Texture::copy(const Texture& texture) noexcept
{
  // コピー元のテクスチャと同じサイズのテクスチャを作る
  Texture::resize(texture.getTextureWidth(), texture.getTextureHeight(),
    texture.getTextureChannels(), nullptr);

  // コピー元のテクスチャの内容をこのピクセルバッファオブジェクトにコピーする
  Buffer::copy(texture);
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
}

//
// コンストラクタ
//
Texture::Texture(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  create(width, height, channels, pixels);
}

//
// コピーコンストラクタ
//
Texture::Texture(const Texture& texture) noexcept
{
  copy(texture);
}

//
// ムーブコンストラクタ
//
Texture::Texture(Texture&& texture) noexcept
{
  *this = std::move(texture);
}

//
// デストラクタ
//
Texture::~Texture()
{
  // テクスチャを削除する
  discard();
}

//
// 代入演算子
//
Texture& Texture::operator=(const Texture& texture)
{
  // 代入元と代入先が同じでなければコピーする
  if (&texture != this) copy(texture);

  // このオブジェクトを返す
  return *this;
}

//
// ムーブ代入演算子
//
Texture& Texture::operator=(Texture&& texture) noexcept
{
  if (&texture != this)
  {
    // ムーブ元のテクスチャのメンバをムーブ先にコピーする
    *static_cast<Buffer*>(this) = std::move(static_cast<Buffer&>(texture));
    textureName = texture.textureName;

    // ムーブ元のテクスチャのメンバを初期化する
    texture.textureName = 0;
  }
  return *this;
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

  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, getBufferFormat(), GL_UNSIGNED_BYTE, 0);

  // 書き込み先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

#else

  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, getBufferFormat(), GL_UNSIGNED_BYTE, buffer.data());

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
    getTextureFormat(), GL_UNSIGNED_BYTE, 0);

  // 読み出し元のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

#else

  // ピクセルバッファオブジェクトの内容をテクスチャに書き込む
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, textureSize[0], textureSize[1],
    getTextureFormat(), GL_UNSIGNED_BYTE, buffer.data());

#endif

  // 書き込み先のテクスチャの結合を解除する
  glBindTexture(GL_TEXTURE_2D, 0);
}
