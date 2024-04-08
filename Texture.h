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
  std::array<int, 2> textureSize;

  /// テクスチャに格納されているテクスチャのチャネル数
  int textureChannels;

  /// テクスチャ名
  GLuint textureName;

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Texture()
    : Buffer{}
    , textureSize{ 0, 0 }
    , textureChannels{ 0 }
    , textureName{ 0 }
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
    // 指定したサイズとバッファのサイズが同じなら何もしない
    if (width == textureSize[0] && height == textureSize[1]
      && channels == textureChannels) return;

    // 違っていたらテクスチャを作り直す
    Texture::create(width, height, channels, pixels);
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
    return textureName;
  }

  ///
  /// テクスチャのサイズを得る
  ///
  const auto& getTextureSize() const
  {
    return textureSize;
  }

  ///
  /// テクスチャの横の画素数を得る
  ///
  const auto getTextureWidth() const
  {
    return textureSize[0];
  }

  ///
  /// テクスチャの縦の画素数を得る
  ///
  const auto getTextureHeight() const
  {
    return textureSize[1];
  }

  ///
  /// テクスチャのチャネル数を得る
  ///
  const auto getTextureChannels() const
  {
    return textureChannels;
  }

  ///
  /// テクスチャのフォーマットを得る
  ///
  const auto getTextureFormat() const
  {
    return channelsToFormat(textureChannels);
  }

  ///
  /// テクスチャユニットを指定してテクスチャを結合する
  ///
  /// @param unit テクスチャユニット番号
  ///
  void bindTexture(int unit = 0) const
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureName);
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
    GLuint buffer) const
#else
    std::vector<GLubyte>& buffer)
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
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
  {
    // バッファのサイズをこのテクスチャに合わせる
    buffer.resize(textureSize[0], textureSize[1], textureChannels);

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
    GLuint
#else
    const std::vector<GLubyte>&
#endif
    buffer) const;

  ///
  /// 指定したバッファからテクスチャにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のバッファ
  /// @param unit テクスチャのサンプリングに使うテクスチャユニット番号
  ///
  /// @note テクスチャのサイズをバッファに合わせる
  ///
  void drawPixels(Buffer& buffer)
  {
    // このテクスチャのサイズをバッファに合わせる
    resize(buffer.getBufferWidth(), buffer.getBufferHeight(), buffer.getBufferChannels());

    // バッファからこのテクスチャにコピーする
    drawPixels(buffer.getBufferName());
  }

  ///
  /// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  /// @param unit テクスチャのサンプリングに使うテクスチャユニット番号
  ///
  /// @note テクスチャのサイズをバッファに合わせる
  ///
  void drawPixels()
  {
    // このバッファからこのテクスチャにコピーする
    drawPixels(*this);
  }
};
