#pragma once

///
/// バッファクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date Aplil 3, 2023
///

// 補助プログラム
#include "gg.h"
using namespace gg;

// OpenCV のデータ型
#include <opencv2/core/types.hpp>

// ピクセルバッファオブジェクトを使うとき
#define USE_PIXEL_BUFFER_OBJECT

///
/// チャネル数からフォーマットを求める
///
/// @param channels フレームのチャネル数
/// @return テクスチャのフォーマット
///
inline auto channelsToFormat(int channels)
{
  // OpenGL のテクスチャフォーマットのリスト
  static constexpr GLenum toFormat[]{ GL_RED, GL_RG, GL_RGB, GL_RGBA };

  // フォーマットを返す
  return toFormat[(channels - 1) & 3];
}

///
/// フォーマットからチャネル数を求める
///
/// @param format テクスチャのフォーマット
/// @return フレームのチャネル数
///
extern int formatToChannels(GLenum format);

///
/// バッファクラス
///
/// @description
/// フレームを格納するピクセルバッファオブジェクト
///
class Buffer
{
  /// バッファに格納されているフレームのサイズ
  std::array<int, 2> bufferSize;

  /// バッファに格納されているフレームのチャネル数
  int bufferChannels;

#if defined(USE_PIXEL_BUFFER_OBJECT)
  /// フレームを格納するピクセルバッファオブジェクト名
  GLuint
#else
  /// フレームを格納するメモリ
  std::vector<GLubyte>
#endif
    bufferName;

public:

  ///
  /// フレームを格納するバッファのデフォルトコンストラクタ
  ///
  Buffer()
    : bufferSize{ 0, 0 }
    , bufferChannels{ 0 }
#if defined(USE_PIXEL_BUFFER_OBJECT)
    , bufferName{ 0 }
#endif
  {
  }

  ///
  /// フレームを格納するバッファを作成するコンストラクタ
  ///
  /// @param width 格納するフレームの横の画素数
  /// @param height 格納するフレームの縦の画素数
  /// @param channels 格納するフレームのチャネル数
  /// @param pixels 格納するフレームに格納するデータのポインタ
  ///
  Buffer(GLsizei width, GLsizei height, int channels,
    const GLvoid* pixels = nullptr);

  ///
  /// コピーコンストラクタ
  ///
  /// @param texture コピー元
  ///
  Buffer(const Buffer& buffer);

  ///
  /// ムーブコンストラクタ
  ///
  /// @param texture ムーブ元
  ///
  Buffer(Buffer&& buffer) noexcept;

  ///
  /// デストラクタ
  ///
  virtual ~Buffer();

  ///
  /// 代入演算子
  ///
  /// @param buffer 代入元
  /// @return 代入結果
  ///
  Buffer& operator=(const Buffer& buffer);

  ///
  /// ムーブ代入演算子
  ///
  /// @param buffer 代入元
  /// @return ムーブ代入結果
  ///
  Buffer& operator=(Buffer&& buffer) noexcept;

  ///
  /// 以前のバッファを削除してフレームを格納する新しいバッファを作成する
  ///
  /// @param width 格納するフレームの横の画素数
  /// @param height 格納するフレームの縦の画素数
  /// @param channels 格納するフレームのチャネル数
  /// @param pixels 格納するフレームに格納するデータのポインタ
  ///
  virtual void create(GLsizei width, GLsizei height, int channels,
    const GLvoid* pixels = nullptr);

  ///
  /// サイズが変更されていたら以前のバッファを削除して
  /// フレームを格納する新しいバッファを作成する
  ///
  /// @param width 格納するフレームの横の画素数
  /// @param height 格納するフレームの縦の画素数
  /// @param channels 格納するフレームのチャネル数
  /// @param pixels 格納するフレームに格納するデータのポインタ
  ///
  virtual void resize(GLsizei width, GLsizei height, int channels,
    const GLvoid* pixels = nullptr)
  {
    // 指定したサイズがバッファのサイズと同じなら何もしない
    if (width == bufferSize[0] && height == bufferSize[1]
      && channels == bufferChannels) return;

    // 違っていたらバッファを作り直す
    Buffer::create(width, height, channels, pixels);
  }

  ///
  /// 新しいバッファを作成して別のバッファをコピーする
  ///
  /// @param buffer コピー元のバッファ
  ///
  void copy(const Buffer& buffer) noexcept;

  ///
  /// バッファを破棄する
  ///
  void discard();

  ///
  /// ピクセルバッファオブジェクト名を得る
  ///
#if defined(USE_PIXEL_BUFFER_OBJECT)
  auto getBufferName() const
#else
  auto& getBufferName()
#endif
  {
    return bufferName;
  }

  ///
  /// ピクセルバッファオブジェクトに格納されているフレームのサイズを得る
  ///
  const auto& getBufferSize() const
  {
    return bufferSize;
  }

  ///
  /// ピクセルバッファオブジェクトに格納されているフレームの横の画素数を得る
  ///
  const auto getBufferWidth() const
  {
    return bufferSize[0];
  }

  ///
  /// ピクセルバッファオブジェクトに格納されているフレームの縦の画素数を得る
  ///
  const auto getBufferHeight() const
  {
    return bufferSize[1];
  }

  ///
  /// ピクセルバッファオブジェクトに格納されているフレームのチャネル数を得る
  ///
  const auto getBufferChannels() const
  {
    return bufferChannels;
  }

  ///
  /// ピクセルバッファオブジェクトに格納されているフレームのフォーマットを得る
  ///
  const auto getBufferFormat() const
  {
    return channelsToFormat(bufferChannels);
  }

  ///
  /// ピクセルバッファオブジェクトに格納されているフレームの縦横比を得る
  ///
  auto getBufferAspect() const
  {
    return static_cast<GLfloat>(bufferSize[0]) / static_cast<GLfloat>(bufferSize[1]);
  }

  ///
  /// ピクセルバッファオブジェクトを結合する
  ///
  void bindBuffer(GLenum target = GL_PIXEL_PACK_BUFFER) const
  {
#if defined(USE_PIXEL_BUFFER_OBJECT)
    glBindBuffer(target, bufferName);
#endif
  }

  ///
  /// ピクセルバッファオブジェクトの結合を解除する
  ///
  void unbindBuffer(GLenum target = GL_PIXEL_PACK_BUFFER) const
  {
#if defined(USE_PIXEL_BUFFER_OBJECT)
    glBindBuffer(target, 0);
#endif
  }

  ///
  /// ピクセルバッファオブジェクトをマップする
  ///
  /// @return ピクセルバッファオブジェクトをマップしたメモリ
  ///
  GLvoid* map()
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
    ;

  ///
  /// ピクセルバッファオブジェクトをアンマップする
  ///
  void unmap() const;

  ///
  /// ピクセルバッファオブジェクトのデータを読み出す
  ///
  /// @param size 読み出すデータのサイズ
  /// @param pixels 読み出すデータのポインタ
  ///
  void readBuffer(GLsizei size, GLvoid* pixels) const;

  ///
  /// ピクセルバッファオブジェクトにデータを転送する
  ///
  /// @param size 転送するデータのサイズ
  /// @param pixels 転送するデータのポインタ
  ///
  void writeBuffer(GLsizei size, const GLvoid* pixels)
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
    ;
};
