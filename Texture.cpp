///
/// テクスチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Texture.h"

//
// テクスチャを作成する
//
void Texture::create(GLsizei width, GLsizei height, int channels,
  const GLvoid* pixels)
{
  // 既存のバッファを破棄して新しいバッファを作成する
  Buffer::create(width, height, channels, pixels);
  
  // 指定したサイズが保持しているテクスチャのサイズと違えば
  if (width != textureSize[0] || height != textureSize[1]
    || channels != textureChannels)
  {
    // テクスチャのサイズとチャンネル数を記録する
    textureSize = std::array<int, 2>{ width, height };
    textureChannels = channels;

    // 以前のテクスチャを削除する
    glDeleteTextures(1, &textureName);

    // テクスチャを作成する
    glGenTextures(1, &textureName);
  }

  // テクスチャのメモリを確保してデータを転送する
  glBindTexture(GL_TEXTURE_2D, textureName);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
    channelsToFormat(channels), GL_UNSIGNED_BYTE, pixels);
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
    texture.getChannels(), nullptr);

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
  glGetTexImage(GL_TEXTURE_2D, 0, getFormat(), GL_UNSIGNED_BYTE, 0);

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
    getFormat(), GL_UNSIGNED_BYTE, 0);

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
