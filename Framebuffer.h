#pragma once

///
/// フレームバッファオブジェクトクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"

// テクスチャ
#include "Texture.h"

///
/// フレームバッファオブジェクトクラス
///
class Framebuffer : public Texture
{
  /// フレームバッファオブジェクト
  GLuint framebuffer;

  /// フレームバッファオブジェクトのレンダーターゲット
  GLenum attachment;

protected:

  ///
  /// 新しいフレームバッファオブジェクトを作成する
  ///
  /// @return フレームバッファオブジェクトのカラーバッファのテクスチャ名
  ///
  GLuint createFramebuffer();

  ///
  /// 新しいフレームバッファオブジェクトを作成してそこに別のフレームバッファオブジェクトをコピーする
  ///
  /// @param texture コピー元のテクスチャ
  ///
  GLuint copyFramebuffer(const Framebuffer& framebuffer) noexcept;

  ///
  /// フレームバッファオブジェクトを破棄する
  ///
  void discardFramebuffer();

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Framebuffer()
    : Texture{}
    , framebuffer{ 0 }
    , attachment{ GL_COLOR_ATTACHMENT0 }
  {
  }

  ///
  /// テクスチャからフレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  ///
  Framebuffer(Texture& texture);

  ///
  /// テクスチャを作成してフレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param width フレームバッファオブジェクトの横の画素数
  /// @param height フレームバッファオブジェクトの縦の画素数
  /// @param channels フレームバッファオブジェクトのカラーバッファのテクスチャのチャネル数
  /// @param pixels 作成するテクスチャに格納するデータのポインタ
  ///
  Framebuffer(GLsizei width, GLsizei height, int channels, const GLvoid* pixels = nullptr);

  ///
  /// 画像ファイルを読み込んでフレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param filename 画像ファイル名
  ///
  Framebuffer(const std::string& filename);

  ///
  /// コピーコンストラクタ
  ///
  /// @param framebuffer コピー元
  ///
  Framebuffer(const Framebuffer& framebuffer) noexcept;

  ///
  /// ムーブコンストラクタ
  ///
  /// @param framebuffer ムーブ元
  ///
  Framebuffer(Framebuffer&& framebuffer) noexcept;

  ///
  /// デストラクタ
  ///
  virtual ~Framebuffer();

  ///
  /// 代入演算子
  ///
  /// @param framebuffer 代入元
  ///
  Framebuffer& operator=(const Framebuffer& framebuffer);

  ///
  /// テクスチャからフレームバッファオブジェクトを作成する
  ///
  /// @param フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  /// @return フレームバッファオブジェクトのカラーバッファに使ったテクスチャ名
  ///
  GLuint create(const Texture& texture);

  ///
  /// テクスチャを作成してフレームバッファオブジェクトを作成する
  ///
  /// @param width 作成するフレームバッファオブジェクトの横の画素数
  /// @param height 作成するフレームバッファオブジェクトの縦の画素数
  /// @param channels 作成するフレームバッファオブジェクトのチャネル数
  /// @param pixels 作成するフレームバッファオブジェクトに格納するデータのポインタ
  /// @return フレームバッファオブジェクトのカラーバッファに使ったテクスチャ名
  ///
  GLuint create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels = nullptr);

  ///
  /// 画像ファイルを読み込んでフレームバッファオブジェクトを作成する
  ///
  /// @param filename 画像ファイル名
  /// @return フレームバッファオブジェクトのカラーバッファに使ったテクスチャ名
  ///
  GLuint create(const std::string& filename);

  ///
  /// レンダリング先をフレームバッファオブジェクトに切り替える
  ///
  void use() const
  {
    // 描画先をフレームバッファオブジェクトに切り替える
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // ビューポートをフレームバッファオブジェクトに合わせる
    const auto& size{ getSize() };
    glViewport(0, 0, size.width, size.height);
  }

  ///
  /// レンダリング先を通常のフレームバッファに戻す
  ///
  void unuse() const
  {
    // 描画先を通常のフレームバッファに戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 読み書きを通常のフレームバッファのバックバッファに対して行う
    glReadBuffer(GL_BACK);
    glDrawBuffer(GL_BACK);
  }

  ///
  /// フレームバッファオブジェクトの内容を表示する
  ///
  /// @param width フレームバッファオブジェクトの内容を表示する横の画素数
  /// @param height フレームバッファオブジェクトの内容を表示する縦の画素数
  ///
  void draw(GLsizei width, GLsizei height) const;

  ///
  /// 画像ファイルを読み込んでフレームバッファオブジェクトを更新する
  ///
  /// @param 読み込む画像ファイル名
  /// @return 画像ファイルの読み込みに成功したら true
  ///
  virtual bool loadImage(const std::string& filename);

  ///
  /// 動画ファイルを読み込んでフレームバッファオブジェクトを更新する
  ///
  /// @param 読み込む動画ファイル名
  /// @return 動画ファイルの読み込みに成功したら true
  ///
  virtual bool loadMovie(const std::string& filename);
};
