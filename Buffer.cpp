///
/// バッファクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Buffer.h"

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

  // コピー元のピクセルバッファオブジェクトのサイズを調べる
  GLint readSize;
  glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &readSize);

  // コピー先のピクセルバッファオブジェクト
  glBindBuffer(GL_COPY_WRITE_BUFFER, bufferName);

  // コピー先のピクセルバッファオブジェクトのサイズを調べる
  GLint writeSize;
  glGetBufferParameteriv(GL_COPY_WRITE_BUFFER, GL_BUFFER_SIZE, &writeSize);

  // バッファサイズが一致しているか確認する
  assert(readSize == writeSize);

  // バッファオブジェクトをコピーする
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
    0, 0, std::min(readSize, writeSize));
#else
  // フレームのデータをコピーする
  memcpy(bufferName.data(), buffer.bufferName.data(), bufferName.size());
#endif
}

//
// バッファを作成する
//
void Buffer::create(GLsizei width, GLsizei height, int channels,
  const GLvoid* pixels)
{
  // フレームの保存に必要なメモリ量を求める
  const auto size{ width * height * channels };

  // 指定したサイズが保持しているフレームのサイズと違えば
  if (width != bufferSize[0] || height != bufferSize[1]
    || channels != bufferChannels)
  {
    // フレームのサイズとチャンネル数を記録する
    bufferSize = std::array<int, 2>{ width, height };
    bufferChannels = channels;

#if defined(USE_PIXEL_BUFFER_OBJECT)
    // 新しいピクセルバッファオブジェクトを作成する
    if (bufferName == 0) glGenBuffers(1, &bufferName);
#else
    // メモリを確保する
    bufferName.resize(size);
#endif
  }

#if defined(USE_PIXEL_BUFFER_OBJECT)
  // ピクセルバッファオブジェクトのメモリを確保してデータを転送する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, bufferName);
  glBufferData(GL_PIXEL_PACK_BUFFER, size, pixels, GL_DYNAMIC_COPY);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
  // 確保したメモリにデータを転送する
  if (pixels) memcpy(bufferName.data(), pixels, size);
#endif
}

//
// バッファをコピーする
//
void Buffer::copy(const Buffer& buffer) noexcept
{
  // コピー元と同じサイズの空のバッファを作る
  Buffer::create(buffer.bufferSize[0], buffer.bufferSize[1],
    buffer.bufferChannels, nullptr);

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
  return glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
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
// ピクセルバッファオブジェクトのデータを読み出す
//
void Buffer::readBuffer(GLsizei size, GLvoid* pixels) const
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // コピー元のピクセルバッファオブジェクトを結合する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, bufferName);

  // コピー元のピクセルバッファオブジェクトのサイズを調べる
  GLint readSize;
  glGetBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &readSize);

  // コピー元とコピー先の小さい方のサイズをコピーする
  glGetBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, std::min(size, readSize), pixels);

  // コピー元のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#else
  // コピー元とコピー先の小さい方のサイズ
  const auto bufferSize{ static_cast<decltype(size)>(bufferName.size()) };
  if (size > bufferSize) size = bufferSize;

  // コピー元のメモリから引数に指定したコピー先の配列にコピーする
  memcpy(pixels, bufferName.data(), size);
#endif
}

//
// ピクセルバッファオブジェクトにデータを転送する
//
void Buffer::writeBuffer(GLsizei size, const GLvoid* pixels)
#if defined(USE_PIXEL_BUFFER_OBJECT)
const
#endif
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // コピー先のピクセルバッファオブジェクト
  glBindBuffer(GL_PIXEL_PACK_BUFFER, bufferName);

  // コピー先のピクセルバッファオブジェクトのサイズを調べる
  GLint writeSize;
  glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &writeSize);

  // コピー元とコピー先の小さい方のサイズをコピーする
  glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, std::min(size, writeSize), pixels);

  // コピー先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
  // コピー元とコピー先の小さい方のサイズ
  const auto bufferSize{ static_cast<decltype(size)>(bufferName.size()) };
  if (size > bufferSize) size = bufferSize;

  // 引数に指定したコピー元の配列からコピー先のメモリにコピーする
  memcpy(bufferName.data(), pixels, size);
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
