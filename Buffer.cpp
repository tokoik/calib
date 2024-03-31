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
int formatToChannels(GLenum format)
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
// フレームを格納するバッファを作成するコンストラクタ
//
Buffer::Buffer(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  create(width, height, channels, pixels);
}

//
// コピーコンストラクタ
//
Buffer::Buffer(const Buffer& buffer)
{
  copy(buffer);
}

//
// ムーブコンストラクタ
//
Buffer::Buffer(Buffer&& buffer) noexcept
{
  *this = std::move(buffer);
}

//
// デストラクタ
//
Buffer::~Buffer()
{
  discard();
}

//
// 代入演算子
//
Buffer& Buffer::operator=(const Buffer& buffer)
{
  if (&buffer != this) copy(buffer);
  return *this;
}

//
// ムーブ代入演算子
//
Buffer& Buffer::operator=(Buffer&& buffer) noexcept
{
  if (&buffer != this)
  {
    // ムーブ元のバッファのメンバをムーブ先にコピーする
    size = buffer.size;
    channels = buffer.channels;
    name = buffer.name;

    // ムーブ元のバッファのメンバを初期化する
    buffer.size = std::array<int, 2>{ 0, 0 };
    buffer.channels = 0;
    buffer.name = 0;
  }
  return *this;
}

//
// 以前のバッファを削除してフレームを格納する新しいバッファを作成する
//
void Buffer::create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  // フレームのサイズとフォーマットを記録する
  this->size = std::array<int, 2>{ width, height };
  this->channels = channels;

  // メモリ量を求める
  const auto size{ width * height * channels };

#if defined(USE_PIXEL_BUFFER_OBJECT)
  // 以前のピクセルバッファオブジェクトを削除する
  glDeleteBuffers(1, &name);

  // フレームバッファの読み出しに使うピクセルバッファオブジェクトを作成する
  glGenBuffers(1, &name);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, name);
  glBufferData(GL_PIXEL_PACK_BUFFER, size, pixels, GL_DYNAMIC_COPY);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
  // メモリを確保する
  storage.resize(size);

  // 確保したメモリにデータを転送する
  if (pixels) memcpy(storage.data(), pixels, size);
#endif
}

//
// 新しいピクセルバッファオブジェクトを作成して別のピクセルバッファオブジェクトをコピーする
//
void Buffer::copy(const Buffer& buffer) noexcept
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // コピー元のバッファと同じ大きさのバッファを作成する
  create(buffer.size[0], buffer.size[1], buffer.channels);

  // コピー元のピクセルバッファオブジェクト
  glBindBuffer(GL_COPY_READ_BUFFER, buffer.name);

  // コピー元のピクセルバッファオブジェクトのサイズを調べる
  GLint readSize;
  glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &readSize);

  // コピー先のピクセルバッファオブジェクト
  glBindBuffer(GL_COPY_WRITE_BUFFER, name);

  // コピー先のピクセルバッファオブジェクトのサイズを調べる
  GLint writeSize;
  glGetBufferParameteriv(GL_COPY_WRITE_BUFFER, GL_BUFFER_SIZE, &writeSize);

  // バッファサイズが一致しているか確認する
  assert(readSize == writeSize);

  // バッファオブジェクトをコピーする
  glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
    0, 0, std::min(readSize, writeSize));
#else
  // メモリサイズを修正する
  storage.resize(buffer.storage.size());

  // コピー元からコピー先に小さい方のサイズ分だけコピーする
  memcpy(storage.data(), buffer.storage.data(), buffer.storage.size());
#endif
}

//
// ピクセルバッファオブジェクトをマップする
//
GLvoid* Buffer::mapBuffer()
#if defined(USE_PIXEL_BUFFER_OBJECT)
const
#endif
{
#if defined(USE_PIXEL_BUFFER_OBJECT)
  // ピクセルバッファオブジェクトを参照する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, name);

  // ピクセルバッファオブジェクトをマップする
  return glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
#else
  // データの格納場所を返す
  return storage.data();
#endif
}

//
// ピクセルバッファオブジェクトをアンマップする
//
void Buffer::unmapBuffer() const
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
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, name);

  // コピー元のピクセルバッファオブジェクトのサイズを調べる
  GLint readSize;
  glGetBufferParameteriv(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &readSize);

  // コピー元とコピー先の小さい方のサイズをコピーする
  glGetBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, std::min(size, readSize), pixels);

  // コピー元のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
#else
  // コピー元とコピー先の小さい方のサイズ
  const auto bufferSize{ static_cast<decltype(size)>(storage.size()) };
  if (size > bufferSize) size = bufferSize;

  // コピー元のメモリから引数に指定したコピー先の配列にコピーする
  memcpy(pixels, storage.data(), size);
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
  glBindBuffer(GL_PIXEL_PACK_BUFFER, name);

  // コピー先のピクセルバッファオブジェクトのサイズを調べる
  GLint writeSize;
  glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &writeSize);

  // コピー元とコピー先の小さい方のサイズをコピーする
  glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, std::min(size, writeSize), pixels);

  // コピー先のピクセルバッファオブジェクトの結合を解除する
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
  // コピー元とコピー先の小さい方のサイズ
  const auto bufferSize{ static_cast<decltype(size)>(storage.size()) };
  if (size > bufferSize) size = bufferSize;

  // 引数に指定したコピー元の配列からコピー先のメモリにコピーする
  memcpy(storage.data(), pixels, size);
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
  glDeleteBuffers(1, &name);
  name = 0;
  size = std::array<int, 2>{ 0, 0 };
  channels = 0;
#else
  // メモリを消去する
  storage.clear();
#endif
}
