#pragma once

///
/// テクスチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// バッファクラス
#include "Buffer.h"

///
/// テクスチャクラス
///
class Texture : public Buffer
{
  /// テクスチャに格納されているテクスチャのサイズ
  std::array<int, 2> size;

  /// テクスチャに格納されているテクスチャのチャネル数
  int channels;

  /// テクスチャ名
  GLuint name;

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Texture()
    : Buffer{}
    , size{ 0, 0 }
    , channels{ 0 }
    , name { 0 }
  {
  }

  ///
  /// テクスチャを作成するコンストラクタ
  ///
  /// @param width 作成するテクスチャの横の画素数
  /// @param height 作成するテクスチャの縦の画素数
  /// @param channels 作成するテクスチャのチャネル数
  /// @param pixels 作成するテクスチャに格納するデータのポインタ
  ///
  Texture(GLsizei width, GLsizei height, int channels, const GLvoid* pixels = nullptr);

  ///
  /// コピーコンストラクタ
  ///
  /// @param texture コピー元
  ///
  Texture(const Texture& texture) noexcept;

  ///
  /// ムーブコンストラクタ
  ///
  /// @param texture ムーブ元
  ///
  Texture(Texture&& texture) noexcept;

  ///
  /// デストラクタ
  ///
  virtual ~Texture();

  ///
  /// 代入演算子
  ///
  /// @param texture 代入元
  ///
  Texture& operator=(const Texture& texture);

  ///
  /// ムーブ代入演算子
  ///
  /// @param texture ムーブ代入元
  ///
  Texture& operator=(Texture&& texture) noexcept;

  ///
  /// 既存のテクスチャを破棄して新しいテクスチャを作成する
  ///
  /// @param width 作成するテクスチャの横の画素数
  /// @param height 作成するテクスチャの縦の画素数
  /// @param channels 作成するテクスチャのチャネル数
  /// @param pixels 作成するテクスチャに格納するデータのポインタ
  ///
  virtual void create(GLsizei width, GLsizei height, int channels,
    const GLvoid* pixels = nullptr);

  ///
  /// サイズが変更されていたら以前のテクスチャを削除して
  /// テクスチャを格納する新しいバッファを作成する
  ///
  /// @param width 格納するテクスチャの横の画素数
  /// @param height 格納するテクスチャの縦の画素数
  /// @param channels 格納するテクスチャのチャネル数
  /// @param pixels 格納するテクスチャに格納するデータのポインタ
  ///
  virtual void resize(GLsizei width, GLsizei height, int channels,
    const GLvoid* pixels = nullptr)
  {
    // 指定したサイズとバッファのサイズが違っていたら
    if (width != this->size[0]
      || height != this->size[1]
      || channels != this->channels)
    {
      // テクスチャを作り直す
      create(width, height, channels, pixels);
    }
  }

  ///
  /// 新しいテクスチャを作成して別のテクスチャのバッファをコピーする
  ///
  /// @param texture コピー元のテクスチャ
  ///
  void copy(const Texture& texture) noexcept;

  ///
  /// テクスチャを破棄する
  ///
  void discard();

  ///
  /// テクスチャ名を得る
  ///
  /// @return テクスチャ名
  ///
  auto getTextureName() const
  {
    return name;
  }

  ///
  /// テクスチャのサイズを得る
  ///
  const auto& getSize() const
  {
    return size;
  }

  ///
  /// テクスチャの横の画素数を得る
  ///
  const auto getWidth() const
  {
    return size[0];
  }

  ///
  /// テクスチャの縦の画素数を得る
  ///
  const auto getHeight() const
  {
    return size[1];
  }

  ///
  /// テクスチャのチャネル数を得る
  ///
  const auto getChannels() const
  {
    return channels;
  }

  ///
  /// テクスチャユニットを指定してテクスチャを結合する
  ///
  /// @param unit テクスチャユニット番号
  ///
  void bindTexture(int unit = 0) const
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, name);
  }

  ///
  /// テクスチャの結合を解除する
  ///
  void unbindTexture() const
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  ///
  /// テクスチャから指定したピクセルバッファオブジェクトにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のピクセルバッファオブジェクト名
  ///
  /// @note コピーするデータのサイズを確認する必要がある
  ///
  void readPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
    decltype(name) buffer) const
#else
    decltype(storage)& buffer)
#endif
    ;

  ///
  /// テクスチャから指定したバッファにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のバッファ
  ///
  /// @note コピーする先のバッファのサイズをテクスチャに合わせる
  ///
  void readPixels(Buffer& buffer)
  {
    // バッファのサイズをこのテクスチャに合わせる
    buffer.resize(size[0], size[1], channels);

    // このテクスチャからバッファにコピーする
    readPixels(buffer.getBufferName());
  }

  ///
  /// テクスチャからピクセルバッファオブジェクトにデータをコピーする
  ///
  /// @note バッファのサイズをテクスチャに合わせる
  ///
  void readPixels()
  {
    // このテクスチャからこのバッファにコピーする
    readPixels(*this);
  }

  ///
  /// 指定したピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  /// @param buffer コピーするテクスチャを格納したバッファ
  /// @param unit テクスチャのサンプリングに使うテクスチャユニット番号
  ///
  /// @note コピーするデータのサイズを確認する必要がある
  ///
  void drawPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
    decltype(name)
#else
    const decltype(storage)&
#endif
  buffer, int unit = 0) const;

  ///
  /// 指定したバッファからテクスチャにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のバッファ
  /// @param unit テクスチャのサンプリングに使うテクスチャユニット番号
  ///
  /// @note テクスチャのサイズをバッファに合わせる
  ///
  void drawPixels(Buffer& buffer, int unit = 0)
  {
    // このテクスチャのサイズをバッファに合わせる
    resize(buffer.getWidth(), buffer.getHeight(), buffer.getChannels());

    // バッファからこのテクスチャにコピーする
    drawPixels(buffer.getBufferName(), unit);
  }

  ///
  /// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  /// @param unit テクスチャのサンプリングに使うテクスチャユニット番号
  ///
  /// @note テクスチャのサイズをバッファに合わせる
  ///
  void drawPixels(int unit = 0)
  {
    // このバッファからこのテクスチャにコピーする
    drawPixels(*this, unit);
  }
};
