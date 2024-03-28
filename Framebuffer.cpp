///
/// フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Framebuffer.h"

//
// 現在のテクスチャをカラーバッファに使って新しいフレームバッファオブジェクトを作成する
//
void Framebuffer::createFramebuffer()
{
  // 新しいフレームバッファオブジェクトを作成する
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, getName(), 0);
  glDrawBuffers(1, &attachment);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//
// デフォルトコンストラクタ
//
Framebuffer::Framebuffer()
  : Texture{}
  , attachment{ GL_COLOR_ATTACHMENT0 }
  , framebuffer{ 0 }
{
}

//
// 指定したテクスチャをカラーバッファに使ってフレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(const Texture& texture)
  : Texture{ texture }
  , attachment{ GL_COLOR_ATTACHMENT0 }
{
  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();
}

//
// 画像ファイルを読み込んでテクスチャを作成するコンストラクタ
//
Framebuffer::Framebuffer(const std::string& filename)
  : Texture{ filename }
  , attachment{ GL_COLOR_ATTACHMENT0 }
{
  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();
}

//
// デストラクタ
//
Framebuffer::~Framebuffer()
{
  // フレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);

  // デフォルトのフレームバッファオブジェクトに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  framebuffer = 0;
}

//
// 既存のフレームバッファオブジェクトを破棄して新しいフレームバッファオブジェクトを作成する
//
GLuint Framebuffer::create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  // 既存のテクスチャを破棄して新しいテクスチャを作る
  Texture::create(width, height, channels, pixels);

  // 既存のフレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);

  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();

  // テクスチャ名を返す
  return getName();
}

//
// 既存のフレームバッファオブジェクトを破棄して新しいフレームバッファオブジェクトに画像ファイルを読み込む
//
bool Framebuffer::loadImage(const std::string& filename)
{
  // 既存のテクスチャを破棄して新しいテクスチャに画像ファイルを読み込む
  Texture::loadImage(filename);

  // 既存のフレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);

  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();

  // テクスチャ名を返す
  return getName();
}

//
// 既存のフレームバッファオブジェクトを破棄して新しいフレームバッファオブジェクトに動画ファイルを読み込む
//
bool Framebuffer::loadMovie(const std::string& filename)
{
  // 既存のテクスチャを破棄して新しいテクスチャに動画ファイルを読み込む
  Texture::loadMovie(filename);

  // 既存のフレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);

  // 新しいフレームバッファオブジェクトを作成する
  createFramebuffer();

  // テクスチャ名を返す
  return getName();
}

//
// レンダリング先をフレームバッファオブジェクトに切り替える
//
void Framebuffer::use() const
{
  // 描画先をフレームバッファオブジェクトに切り替える
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  // ビューポートをフレームバッファオブジェクトに合わせる
  glViewport(0, 0, getSize().width, getSize().height);
}

//
// レンダリング先を通常のフレームバッファに戻す
//
void Framebuffer::unuse() const
{
  // 描画先を通常のフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 読み書きを通常のフレームバッファのバックバッファに対して行う
  glReadBuffer(GL_BACK);
  glDrawBuffer(GL_BACK);
}

//
// フレームバッファオブジェクトの表示
//
void Framebuffer::draw(GLsizei width, GLsizei height) const
{
  // フレームバッファオブジェクトのアスペクト比
  const auto f{ static_cast<float>(getSize().width * height) };

  // ウィンドウの表示領域のアスペクト比
  const auto d{ static_cast<float>(getSize().height * width) };

  // 描画する領域
  GLint dx0, dy0, dx1, dy1;

  // 表示領域の右上端の位置を求める
  --width;
  --height;

  // フレームバッファオブジェクトのアスペクト比が大きかったら
  if (f > d)
  {
    // ディスプレイ上の描画する領域の高さを求める
    const auto h{ static_cast<GLint>(d / getSize().width + 0.5f) };

    // 表示が横長なので描画する領域の横幅いっぱいに表示する
    dx0 = 0;
    dx1 = width;

    // 高さはアスペクト比を維持して描画する領域の中央に描く
    dy0 = (height - h) / 2;
    dy1 = dy0 + h;
  }
  else
  {
    // ディスプレイ上の描画する領域の幅を求める
    const auto w{ static_cast<GLint>(f / getSize().height + 0.5f) };

    // 表示が縦長なので描画する領域の高さいっぱいに表示する
    dy0 = 0;
    dy1 = height;

    // 横幅はアスペクト比を維持して描画する領域の中央に描く
    dx0 = (width - w) / 2;
    dx1 = dx0 + w;
  }

  // 書き込み先を通常のフレームバッファにする
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  // フレームバッファオブジェクトを読み込み元にする
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
  glReadBuffer(attachment);

  // フレームバッファオブジェクトの内容を通常のフレームバッファに書き込む
  glBlitFramebuffer(0, 0, getSize().width - 1, getSize().height - 1,
    dx0, dy1, dx1, dy0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // 読み込み元を通常のフレームバッファに戻す
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
}
