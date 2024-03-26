///
/// テクスチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Texture.h"

// 動画のキャプチャ
#include "CamCv.h"

// 静止画のキャプチャ
#include "CamImage.h"

// 標準ライブラリ
#include <algorithm>

//
// 新しいテクスチャを作成する
//
GLuint Texture::createTexture(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  // テクスチャのサイズとフォーマットを記録する
  size = cv::Size2i{ width, height };
  format = channelsToFormat(channels);

  // テクスチャを作成する
  glGenTextures(1, &name);
  glBindTexture(GL_TEXTURE_2D, name);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // フレームバッファの読み出しに使うピクセルバッファオブジェクトを作成する
  glGenBuffers(1, &buffer);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
  glBufferData(GL_PIXEL_PACK_BUFFER, width * height * channels, nullptr, GL_DYNAMIC_COPY);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  // テクスチャ名を返す
  return getName();
}

//
// 新しいテクスチャを作成してそのピクセルバッファオブジェクトに別のテクスチャをコピーする
//
GLuint Texture::copyTexture(const Texture& texture) noexcept
{
  // コピー元のテクスチャと同じサイズのテクスチャを作る
  const auto& size{ texture.size };
  const auto& channels{ formatToChannels(texture.format) };
  createTexture(size.width, size.height, channels, nullptr);

  // コピー元のテクスチャの内容をこのピクセルバッファオブジェクトにコピーする
  texture.readPixels(buffer);

  // テクスチャ名を返す
  return getName();
}

//
// メディアファイルをテクスチャのピクセルバッファオブジェクトに読み込む
//
template<typename MediaType>
bool Texture::loadMedia(const std::string& filename)
{
  // キャプチャデバイスを作成して
  auto media{ std::make_unique<MediaType>() };

  // キャプチャデバイスが使えなかったら戻る
  if (!media->open(filename)) return false;

  // 画像のサイズとフォーマットを取り出して
  const auto width{ media->getWidth() };
  const auto height{ media->getHeight() };
  const auto channels{ media->getChannels() };

  // 描画するフレームを格納するテクスチャを作成したら
  create(width, height, channels, nullptr);

  // そこに読み込んだ画像をピクセルバッファオブジェクトに転送する
  media->transmit(buffer);

  // 読み込み成功
  return true;
}

//
// テクスチャを破棄する
//
void Texture::discardTexture()
{
  // テクスチャを削除する
  glDeleteTextures(1, &name);

  // ピクセルバッファオブジェクトを削除する
  glDeleteBuffers(1, &buffer);
}

//
// コンストラクタ
//
Texture::Texture(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  createTexture(width, height, channels, pixels);
}

//
// コピーコンストラクタ
//
Texture::Texture(const Texture& texture) noexcept
{
  copyTexture(texture);
}

//
// ムーブコンストラクタ
//
Texture::Texture(Texture&& texture) noexcept
{
  copyTexture(texture);
}

//
// 画像ファイルを読み込んでテクスチャを作成するコンストラクタ
//
Texture::Texture(const std::string& filename)
{
  loadImage(filename);
}

//
// デストラクタ
//
Texture::~Texture()
{
  // 既に OpenGL のコンテキストが作成されていれば
  if (glDeleteTextures && glDeleteBuffers)
  {
    // テクスチャを削除する
    discardTexture();

    // デフォルトのテクスチャに戻す
    glBindTexture(GL_TEXTURE_2D, 0);
    name = 0;

    // ピクセルバッファオブジェクトの指定を解除する
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    buffer = 0;
  }
}

//
// 代入演算子
//
Texture& Texture::operator=(const Texture& texture)
{
  // 代入先のテクスチャを破棄する
  discardTexture();

  // テクスチャをコピーする
  copyTexture(texture);

  // このオブジェクトを返す
  return *this;
}

//
// 既存のテクスチャを破棄して新しいテクスチャを作成する
//
GLuint Texture::create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  // 以前のテクスチャを破棄する
  discardTexture();

  // 新しいテクスチャを作成する
  return createTexture(width, height, channels, pixels);
}

//
// ピクセルバッファオブジェクトのデータを読み出す
//
void Texture::readBuffer(GLsizeiptr size, GLvoid* data) const
{
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);
  glGetBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, size, data);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

//
// ピクセルバッファオブジェクトにデータを転送する
//
void Texture::drawBuffer(GLsizeiptr size, const GLvoid* data) const
{
  glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);
  glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, size, data);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

//
// テクスチャからピクセルバッファオブジェクトにデータをコピーする
//
void Texture::readPixels(GLuint buffer) const
{
  // ダウンロード元のテクスチャを結合する
  glBindTexture(GL_TEXTURE_2D, name);

  // ピクセルバッファオブジェクトをデータのダウンロード先に指定する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);

  // テクスチャの内容をピクセルバッファオブジェクトに書き込む
  glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, 0);

  // ダウンロード先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  // ダウンロード元のテクスチャの結合を解除する
  glBindTexture(GL_TEXTURE_2D, 0);
}

//
// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
//
void Texture::drawPixels(GLuint buffer) const
{
  // アップロード先のテクスチャを結合する
  glBindTexture(GL_TEXTURE_2D, name);

  // ピクセルバッファオブジェクトをデータのアップロード元に指定する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);

  // ピクセルバッファオブジェクトの内容をテクスチャに書き込む
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width, size.height, format, GL_UNSIGNED_BYTE, 0);

  // アップロード元のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // アップロード先のテクスチャの結合を解除する
  glBindTexture(GL_TEXTURE_2D, 0);
}

//
// 画像ファイルを読み込む
//
bool Texture::loadImage(const std::string& filename)
{
  return loadMedia<CamImage>(filename);
}

//
// 動画ファイルを読み込む
//
bool Texture::loadMovie(const std::string& filename)
{
  return loadMedia<CamCv>(filename);
}
