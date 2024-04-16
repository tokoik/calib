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
  /// テクスチャのデフォルトコンストラクタ
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
  ///
  Texture(GLsizei width, GLsizei height, int channels)
  {
    Texture::create(width, height, channels);
  }

  ///
  /// コピーコンストラクタ
  ///
  /// @param texture コピー元のテクスチャ
  ///
  Texture(const Texture& texture)
  {
    Texture::copy(texture);
  }

  ///
  /// ムーブコンストラクタ
  ///
  /// @param texture ムーブ元のテクスチャ
  ///
  Texture(Texture&& texture) noexcept
  {
    *this = std::move(texture);
  }

  ///
  /// デストラクタ
  ///
  virtual ~Texture()
  {
    Texture::discard();
  }

  ///
  /// 代入演算子
  ///
  /// @param texture 代入元のテクスチャ
  /// @return 代入結果のテクスチャ
  ///
  Texture& operator=(const Texture& texture);

  ///
  /// ムーブ代入演算子
  ///
  /// @param texture ムーブ代入元のテクスチャ
  /// @return ムーブ代入結果のテクスチャ
  ///
  Texture& operator=(Texture&& texture) noexcept;

  ///
  /// テクスチャを作成する
  ///
  /// @param width 作成するテクスチャの横の画素数
  /// @param height 作成するテクスチャの縦の画素数
  /// @param channels 作成するテクスチャのチャネル数
  ///
  /// @note
  /// このテクスチャのサイズが引数で指定したサイズと異なれば、
  /// このテクスチャを削除して新しいテクスチャを作り直す。
  ///
  virtual void create(GLsizei width, GLsizei height, int channels);

  ///
  /// テクスチャをコピーする
  ///
  /// @param texture コピー元のテクスチャ
  ///
  /// @note
  /// このテクスチャのサイズがコピー元と異なれば、
  /// このテクスチャを削除して新しいテクスチャを作り直してコピーする。
  /// 引数の型が Buffer なのは、このオブジェクトの型が Buffer のとき
  /// Buffer::copy()、Texture のとき Texture::copy()、
  /// Framebuffer のとき Framebuffer::copy() を呼ぶようにするため。
  ///
  virtual void copy(const Buffer& texture) noexcept;

  ///
  /// テクスチャを破棄する
  ///
  virtual void discard();

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
  virtual const std::array<GLsizei, 2>& getSize() const
  {
    return textureSize;
  }

  ///
  /// テクスチャのチャネル数を得る
  ///
  virtual int getChannels() const
  {
    return textureChannels;
  }

  ///
  /// テクスチャユニットを指定してテクスチャを結合する
  ///
  /// @param unit テクスチャユニット番号
  ///
  /// @note
  /// unit には シェーダにおいてテクスチャのサンプラの uniform 変数に設定する
  /// GL_TEXTUREi の i と一致させること。
  ///
  void bindTexture(int unit = 0) const
  {
    // テクスチャユニットを指定する
    glActiveTexture(GL_TEXTURE0 + unit);

    // テクスチャを指定する
    glBindTexture(GL_TEXTURE_2D, textureName);
  }

  ///
  /// テクスチャの結合を解除する
  ///
  void unbindTexture() const
  {
    // デフォルトのテクスチャユニットに戻す
    glActiveTexture(GL_TEXTURE0);

    // デフォルトのテクスチャに戻す
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  ///
  /// テクスチャから指定したピクセルバッファオブジェクトにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のピクセルバッファオブジェクト名
  ///
  /// @note
  /// コピーするデータのサイズが一致している必要がある。
  ///
  void readPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
    GLuint buffer) const
#else
    std::vector<GLubyte>& buffer)
#endif
    ;

  ///
  /// テクスチャから指定したオブジェクトのバッファにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のオブジェクト
  ///
  /// @note
  /// コピーする先のオブジェクトのサイズをテクスチャに合わせる。
  ///
  void readPixels(Buffer& buffer)
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
  {
    // このテクスチャのバッファから引数のオブジェクトのバッファにコピーする
    readPixels();

    // 引数のオブジェクトのサイズをこのテクスチャに合わせてコピーする
    buffer.Buffer::copy(*this);
  }

  ///
  /// テクスチャからピクセルバッファオブジェクトにデータをコピーする
  ///
  void readPixels()
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
  {
    // このテクスチャからこのバッファにコピーする
    readPixels(getBufferName());
  }

  ///
  /// 指定したピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  /// @param buffer コピーするテクスチャを格納したピクセルバッファオブジェクト名
  ///
  /// @note
  /// コピーするデータのサイズが一致している必要がある。
  ///
  void drawPixels(
#if defined(USE_PIXEL_BUFFER_OBJECT)
    GLuint
#else
    const std::vector<GLubyte>&
#endif
    buffer) const;

  ///
  /// 指定したテクスチャからからデータをコピーする
  ///
  /// @param texture コピー元のテクスチャ
  ///
  /// @note
  /// テクスチャのサイズをコピー元のテクスチャに合わせる。
  ///
  void drawPixels(const Texture& texture)
  {
    // このテクスチャのサイズを引数のオブジェクトのサイズに合わせてコピーする
    copy(texture);

    // バッファからこのテクスチャにコピーする
    drawPixels();
  }

  ///
  /// 指定したオブジェクトからテクスチャにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のオブジェクト
  ///
  /// @note
  /// テクスチャのサイズをコピー元のオブジェクトのバッファに合わせる。
  ///
  void drawPixels(const Buffer& buffer)
  {
    // このテクスチャのサイズを引数のオブジェクトのサイズに合わせてコピーする
    copy(buffer);

    // バッファからこのテクスチャにコピーする
    drawPixels();
  }

  ///
  /// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  void drawPixels()
#if defined(USE_PIXEL_BUFFER_OBJECT)
    const
#endif
  {
    // このバッファからこのテクスチャにコピーする
    drawPixels(getBufferName());
  }
};
