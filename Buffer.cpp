///
/// バッファクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Buffer.h"

// 標準ライブラリ
#include <cstring>

//
// フォーマットからチャネル数を求める
//
int Buffer::formatToChannels(GLenum format) const
{
  switch (format)
  {
  case GL_RED:
    return 1;
  case GL_RG:
    return 2;
  case GL_RGB:
    return 3;
  case GL_RGBA:
    return 4;
  default:
    assert(false);
    break;
  }

  return 0;
};

//
// 既存のバッファにコピーする
//
void Buffer::copyBuffer(const Buffer& buffer) noexcept
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // コピー元のピクセルバッファオブジェクト
  glBindBuffer(GL_COPY_READ_BUFFER, buffer.bufferName);

  // コピー先のピクセルバッファオブジェクト
  glBindBuffer(GL_COPY_WRITE_BUFFER, bufferName);

  // バッファオブジェクトをコピーする
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
    0, 0, std::min(bufferLength, buffer.bufferLength));
#else
  // フレームのデータをコピーする
  memcpy(bufferName.data(), buffer.bufferName.data(), bufferName.size());
#endif
}

//
// バッファを作成する
//
void Buffer::create(GLsizei width, GLsizei height, int channels)
{
  // 指定したサイズが保持しているフレームのサイズと同じなら何もしない
  if (width == bufferSize[0] && height == bufferSize[1]
    && channels == bufferChannels) return;

  // フレームのサイズとチャンネル数を記録する
  bufferSize = std::array<int, 2>{ width, height };
  bufferChannels = channels;

  // フレームの保存に必要なメモリ量を求める
  bufferLength = width * height * channels;

#if defined(USE_PIXEL_BUFFER_OBJECT)
  // 以前のピクセルバッファオブジェクトを破棄する
  glDeleteBuffers(1, &bufferName);

  // 新しいピクセルバッファオブジェクトを作成する
  glGenBuffers(1, &bufferName);

  // ピクセルバッファオブジェクトのメモリを確保してデータを転送する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, bufferName);
  glBufferData(GL_PIXEL_PACK_BUFFER, bufferLength, nullptr, GL_DYNAMIC_COPY);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
  // メモリを確保する
  bufferName.resize(size);
#endif
}

//
// バッファをコピーする
//
void Buffer::copy(const Buffer& buffer) noexcept
{
  // コピー元と同じサイズの空のバッファを作る
  Buffer::create(buffer.bufferSize[0], buffer.bufferSize[1],
    buffer.bufferChannels);

  // 作成されたバッファにコピーする
  copyBuffer(buffer);
}

//
// 代入演算子
//
Buffer& Buffer::operator=(const Buffer& buffer)
{
  // 代入元と代入先が同じでなければ
  if (&buffer != this)
  {
    // 引数のバッファをバッファをこのバッファにコピーする
    Buffer::copy(buffer);
  }

  // このバッファを返す
  return *this;
}

//
// ムーブ代入演算子
//
Buffer& Buffer::operator=(Buffer&& buffer) noexcept
{
  // 代入元と代入先が同じでなければ
  if (&buffer != this)
  {
    // 引数のバッファをバッファをこのバッファにコピーする
    Buffer::copy(buffer);

    // 引数のバッファを破棄する
    buffer.Buffer::discard();
  }

  // このバッファを返す
  return *this;
}

//
// ピクセルバッファオブジェクトをマップする
//
GLvoid* Buffer::map()
#if defined(USE_PIXEL_BUFFER_OBJECT)
const
#endif
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // ピクセルバッファオブジェクトを参照する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, bufferName);

  // ピクセルバッファオブジェクトをマップする
  return glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bufferLength,
    GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
#else
  // データの格納場所を返す
  return bufferName.data();
#endif
}

//
// ピクセルバッファオブジェクトをアンマップする
//
void Buffer::unmap() const
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // ピクセルバッファオブジェクトのマップを解除する
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

  // アップロード先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#endif
}

//
// バッファを破棄する
//
void Buffer::discard()
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // ピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  // ピクセルバッファオブジェクトを削除する
  glDeleteBuffers(1, &bufferName);
  bufferName = 0;
#else
  // メモリを消去する
  bufferName.clear();
#endif

  // バッファのサイズを 0 にする
  bufferSize = std::array<int, 2>{ 0, 0 };
  bufferChannels = 0;
}
