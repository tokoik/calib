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

// ピクセルバッファオブジェクトを使うとき
#define USE_PIXEL_BUFFER_OBJECT

///
/// バッファクラス
///
/// @description
/// フレームを格納するピクセルバッファオブジェクト
///
class Buffer
{
  /// バッファに格納されているフレームのサイズ
  std::array<int, 2> bufferSize{ 0, 0 };

  /// バッファに格納されているフレームのチャネル数
  int bufferChannels{ 0 };

#if defined(USE_PIXEL_BUFFER_OBJECT)
  /// バッファのデータ長
  GLsizei bufferLength{ 0 };

  /// フレームを格納するピクセルバッファオブジェクト名
  GLuint
#else
  /// フレームを格納するメモリ
  std::vector<GLubyte>
#endif
    bufferName{ 0 };

protected:

  ///
  /// チャネル数からフォーマットを求める
  ///
  /// @param channels フレームのチャネル数
  /// @return テクスチャのフォーマット
  ///
  auto channelsToFormat(int channels) const
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
  int formatToChannels(GLenum format) const;

  ///
  /// 既存のバッファにコピーする
  ///
  /// @param buffer コピー元のバッファ
  ///
  void copyBuffer(const Buffer& buffer) noexcept;

public:

  ///
  /// バッファのデフォルトコンストラクタ
  ///
  Buffer() = default;

  ///
  /// バッファを作成するコンストラクタ
  ///
  /// @param width 格納するフレームの横の画素数
  /// @param height 格納するフレームの縦の画素数
  /// @param channels 格納するフレームのチャネル数
  ///
  /// @note
  /// ピクセルバッファオブジェクトを作成して、
  /// そこにフレームのデータを格納する。
  ///
  Buffer(GLsizei width, GLsizei height, int channels)
  {
    Buffer::create(width, height, channels);
  }

  ///
  /// コピーコンストラクタ
  ///
  /// @param texture コピー元のバッファ
  ///
  Buffer(const Buffer& buffer)
  {
    Buffer::copy(buffer);
  }

  ///
  /// ムーブコンストラクタ
  ///
  /// @param texture ムーブ元のバッファ
  ///
  Buffer(Buffer&& buffer) noexcept
    : bufferSize{ buffer.bufferSize }
    , bufferChannels{ buffer.bufferChannels }
#if defined(USE_PIXEL_BUFFER_OBJECT)
    , bufferLength{ buffer.bufferLength }
    , bufferName{ buffer.bufferName }
#endif
  {
    buffer.bufferSize = { 0, 0 };
    buffer.bufferChannels = 0;
#if defined(USE_PIXEL_BUFFER_OBJECT)
    buffer.bufferLength = 0;

    // ムーブ元のデストラクタでバッファが削除されないよう 0 にする
    buffer.bufferName = 0;
#endif
  }

  ///
  /// デストラクタ
  ///
  virtual ~Buffer()
  {
    Buffer::discard();
  }

  ///
  /// 代入演算子
  ///
  /// @param buffer 代入元のバッファ
  /// @return 代入結果のバッファの参照
  ///
  Buffer& operator=(const Buffer& buffer);

  ///
  /// ムーブ代入演算子
  ///
  /// @param buffer ムーブ代入元のバッファ
  /// @return ムーブ代入結果のバッファの参照
  ///
  Buffer& operator=(Buffer&& buffer) noexcept;

  ///
  /// バッファを作成する
  ///
  /// @param width バッファに格納するフレームの横の画素数
  /// @param height バッファに格納するフレームの縦の画素数
  /// @param channels バッファに格納するフレームのチャネル数
  ///
  /// @note
  /// このバッファのサイズが引数で指定したサイズと異なれば、
  /// このバッファを削除して新しいバッファを作り直す。
  ///
  virtual void create(GLsizei width, GLsizei height, int channels);

  ///
  /// オブジェクトが保持するフレームのサイズを変更する
  ///
  /// @param width フレームの横の画素数
  /// @param height フレームの縦の画素数
  /// @param channels フレームのチャネル数
  ///
  /// @note
  /// このオブジェクトのサイズが引数で指定したオブジェクトのサイズと異なれば、
  /// このオブジェクトを削除して新しいオブジェクトを作り直す。
  ///
  void resize(GLsizei width, GLsizei height, int channels)
  {
    create(width, height, channels);
  }

  ///
  /// オブジェクトが保持するフレームのサイズを引数に指定したオブジェクトと同じにする
  ///
  /// @param buffer サイズの基準に用いるオブジェクト
  /// @param pixels 作成するオブジェクトに格納するフレームのデータのポインタ
  ///
  /// @note
  /// このオブジェクトのサイズが引数で指定したオブジェクトのサイズと異なれば、
  /// このオブジェクトを削除して新しいオブジェクトを作り直す。
  ///
  void resize(const Buffer& buffer)
  {
    create(buffer.getWidth(), buffer.getHeight(), buffer.getChannels());
  }

  ///
  /// バッファをコピーする
  ///
  /// @param buffer コピー元のバッファ
  ///
  /// @note
  /// このバッファのサイズがコピー元のバッファのサイズと異なれば、
  /// このバッファを削除して新しいバッファを作り直してコピーする。
  ///
  virtual void copy(const Buffer& buffer) noexcept;

  ///
  /// バッファを破棄する
  ///
  virtual void discard();

  ///
  /// バッファのピクセルバッファオブジェクト名を得る
  ///
  /// @return ピクセルバッファオブジェクト名
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
  /// 格納されているフレームのサイズを得る
  ///
  /// @return 格納されているフレームのサイズ
  ///
  virtual const std::array<GLsizei, 2>& getSize() const
  {
    return bufferSize;
  }

  ///
  /// 格納されているフレームの横の画素数を得る
  ///
  /// @return 格納されているフレームの横の画素数
  ///
  GLsizei getWidth() const
  {
    return getSize()[0];
  }

  ///
  /// 格納されているフレームの縦の画素数を得る
  ///
  /// @return 格納されているフレームの縦の画素数
  ///
  GLsizei getHeight() const
  {
    return getSize()[1];
  }

  ///
  /// 格納されているフレームのチャネル数を得る
  ///
  /// @return 格納されているフレームのチャネル数
  ///
  virtual int getChannels() const
  {
    return bufferChannels;
  }

  ///
  /// 格納されているフレームのフォーマットを得る
  ///
  /// @return 格納されているフレームのフォーマット
  ///
  auto getFormat() const
  {
    return channelsToFormat(getChannels());
  }

  ///
  /// 格納されているフレームの縦横比を得る
  ///
  /// @return 格納されているフレームの縦横比
  ///
  auto getAspect() const
  {
    return static_cast<GLfloat>(getWidth()) / static_cast<GLfloat>(getHeight());
  }

  ///
  /// バッファのピクセルバッファオブジェクトを結合する
  ///
  /// @param target 結合するターゲット
  ///
  void bindBuffer(GLenum target = GL_PIXEL_PACK_BUFFER) const
  {
#if defined(USE_PIXEL_BUFFER_OBJECT)
    glBindBuffer(target, bufferName);
#endif
  }

  ///
  /// バッファのピクセルバッファオブジェクトの結合を解除する
  ///
  /// @param target 結合を解除するターゲット
  ///
  void unbindBuffer(GLenum target = GL_PIXEL_PACK_BUFFER) const
  {
#if defined(USE_PIXEL_BUFFER_OBJECT)
    glBindBuffer(target, 0);
#endif
  }

  ///
  /// バッファのピクセルバッファオブジェクトをマップする
  ///
  /// @return ピクセルバッファオブジェクトをマップしたメモリ
  ///
  GLvoid* map()
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
    ;

  ///
  /// バッファのピクセルバッファオブジェクトをアンマップする
  ///
  void unmap() const;
};
