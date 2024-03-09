#pragma once

///
/// テクスチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"

// OpenCV のデータ型
#include <opencv2/core/types.hpp>

///
/// テクスチャクラス
///
class Texture
{
  /// テクスチャ名
  GLuint texture;

  /// テクスチャの読み書きに使うピクセルバッファオブジェクト名
  GLuint buffer;

  /// テクスチャのフォーマット
  GLenum format;

  /// テクスチャのサイズ
  cv::Size2i size;

protected:

  ///
  /// 新しいテクスチャを作成する
  ///
  /// @param width 作成するテクスチャの横の画素数
  /// @param height 作成するテクスチャの縦の画素数
  /// @param channels 作成するテクスチャのチャネル数
  /// @param pixels 作成するテクスチャに格納するデータのポインタ
  /// @return 作成したテクスチャ名
  ///
  /// @note 以前のテクスチャやピクセルバッファオブジェクトは削除しない
  ///
  GLuint createTexture(GLsizei width, GLsizei height, int channels, const GLvoid* pixels);

  ///
  /// 新しいテクスチャを作成してそこに別のテクスチャをコピーする
  ///
  /// @param texture コピー元のテクスチャ
  ///
  GLuint copyTexture(const Texture& texture) noexcept;

  ///
  /// テクスチャを破棄する
  ///
  void discardTexture();

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Texture()
    : texture{ 0 }
    , buffer{ 0 }
    , format{ 0 }
    , size{ 0, 0 }
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
  /// 画像ファイルを読み込んでテクスチャを作成するコンストラクタ
  ///
  /// @param filename 画像ファイル名
  ///
  Texture(const std::string& filename);

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
  /// 既存のテクスチャを破棄して新しいテクスチャを作成する
  ///
  /// @param width 作成するテクスチャの横の画素数
  /// @param height 作成するテクスチャの縦の画素数
  /// @param channels 作成するテクスチャのチャネル数
  /// @param pixels 作成するテクスチャに格納するデータのポインタ
  /// @return テクスチャ名
  ///
  GLuint create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels = nullptr);

  ///
  /// テクスチャのサイズを得る
  ///
  const auto& getSize() const
  {
    return size;
  }

  ///
  /// テクスチャの縦横比を得る
  ///
  auto getAspect() const
  {
    return static_cast<GLfloat>(size.width) / static_cast<GLfloat>(size.height);
  }

  ///
  /// テクスチャを得る
  ///
  /// @return テクスチャ名
  ///
  auto getTexture() const
  {
    return texture;
  }

  ///
  /// ピクセルバッファオブジェクトを得る
  ///
  /// @return ピクセルバッファオブジェクト名
  ///
  auto getBuffer() const
  {
    return buffer;
  }

  ///
  /// テクスチャを指定する
  ///
  /// @param unit テクスチャユニット番号
  ///
  void bindTexture(int unit = 0) const
  {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
  }

  ///
  /// テクスチャの指定を解除する
  ///
  void unbindTexture() const
  {
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  ///
  /// ピクセルバッファオブジェクトを指定する
  ///
  void bindBuffer(GLenum target = GL_PIXEL_PACK_BUFFER) const
  {
    glBindBuffer(target, buffer);
  }

  ///
  /// ピクセルバッファオブジェクトの指定を解除する
  ///
  void unbindBuffer(GLenum target = GL_PIXEL_PACK_BUFFER) const
  {
    glBindBuffer(target, 0);
  }

  ///
  /// ピクセルバッファオブジェクトをマップする
  ///
  /// @return ピクセルバッファオブジェクトをマップしたメモリ
  ///
  auto mapBuffer() const
  {
    // ピクセルバッファオブジェクトを参照する
    glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer);

    // ピクセルバッファオブジェクトをマップする
    return glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
  }

  ///
  /// ピクセルバッファオブジェクトをアンマップする
  ///
  void unmapBuffer() const
  {
    // ピクセルバッファオブジェクトのマップを解除する
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

    // アップロード先のピクセルバッファオブジェクトの結合を解除する
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  }

  ///
  /// ピクセルバッファオブジェクトのデータを読み出す
  ///
  /// @param size 読み出すデータのサイズ
  /// @param data 読み出すデータのポインタ
  ///
  void readBuffer(GLsizeiptr size, GLvoid* data) const;

  ///
  /// ピクセルバッファオブジェクトにデータを転送する
  ///
  /// @param size 転送するデータのサイズ
  /// @param data 転送するデータのポインタ
  ///
  void drawBuffer(GLsizeiptr size, const GLvoid* data) const;

  ///
  /// テクスチャから指定したピクセルバッファオブジェクトにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする先のピクセルバッファオブジェクト
  ///
  void readPixels(GLuint buffer) const;

  ///
  /// テクスチャからピクセルバッファオブジェクトにデータをコピーする
  ///
  void readPixels() const
  {
    readPixels(buffer);
  }

  ///
  /// 指定したピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  void drawPixels(GLuint buffer) const;

  ///
  /// ピクセルバッファオブジェクトからテクスチャにデータをコピーする
  ///
  /// @param buffer テクスチャをコピーする元のピクセルバッファオブジェクト
  ///
  void drawPixels() const
  {
    drawPixels(buffer);
  }

  ///
  /// メディアファイルを読み込む
  ///
  /// @tparam ImageType Camera クラスの派生クラス
  /// @param filename メディアファイル名
  /// @return メディアファイルの読み込みに成功したら true
  ///
  template<typename ImageType>
  bool loadMedia(const std::string& filename);

  ///
  /// 画像ファイルを読み込む
  ///
  /// @param 読み込む画像ファイル名
  /// @return 画像ファイルの読み込みに成功したら true
  ///
  virtual bool loadImage(const std::string& filename);

  ///
  /// 動画ファイルを読み込む
  ///
  /// @param 読み込む動画ファイル名
  /// @return 動画ファイルの読み込みに成功したら true
  ///
  virtual bool loadMovie(const std::string& filename);
};
