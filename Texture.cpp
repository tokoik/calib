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
  this->size = std::array<int, 2>{ width, height };
  this->channels = channels;

  // テクスチャァの読み書きに使うピクセルバッファオブジェクトを作成する
  Buffer::create(width, height, channels, pixels);

  // 以前のテクスチャを破棄する
  discard();
  
  // テクスチャを作成する
  glGenTextures(1, &name);
  glBindTexture(GL_TEXTURE_2D, name);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, getFormat(), GL_UNSIGNED_BYTE, pixels);
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
  const auto& size{ texture.getSize()};
  const auto& channels{ texture.getChannels() };
  create(size[0], size[1], channels, nullptr);

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
  glDeleteTextures(1, &name);
  name = 0;
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
  if (&texture != this) copy(texture);
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
    name = texture.name;

    // ムーブ元のテクスチャのメンバを初期化する
    texture.name = 0;
  }
  return *this;
}

//
// テクスチャからピクセルバッファオブジェクトにデータをコピーする
//
void Texture::readPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
  decltype(name) buffer) const
#else
decltype(storage)& buffer)
#endif
{
  // ダウンロード元のテクスチャを結合する
  glBindTexture(GL_TEXTURE_2D, name);

#if defined(USE_PIXEL_BUFFER_OBJECT)
  // ピクセルバッファオブジェクトをデータのダウンロード先に指定する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);

  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, getFormat(), GL_UNSIGNED_BYTE, 0);

  // ダウンロード先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

#else

  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, getFormat(), GL_UNSIGNED_BYTE, buffer.data());

#endif

  // ダウンロード元のテクスチャの結合を解除する
  glBindTexture(GL_TEXTURE_2D, 0);
}

//
// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
//
void Texture::drawPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
  decltype(name)
#else
  const decltype(storage)&
#endif
  buffer, int unit) const
{
  // テクスチャユニットを指定してアップロード先のテクスチャを結合する
  bindTexture(unit);

#if defined(USE_PIXEL_BUFFER_OBJECT)

  // ピクセルバッファオブジェクトをデータのアップロード元に指定する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);

  // ピクセルバッファオブジェクトの内容をテクスチャに書き込む
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], getFormat(), GL_UNSIGNED_BYTE, 0);

  // アップロード元のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

#else

  // ピクセルバッファオブジェクトの内容をテクスチャに書き込む
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], getFormat(), GL_UNSIGNED_BYTE, buffer.data());

#endif
}
