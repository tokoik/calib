///
/// フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Framebuffer.h"

//
// 新しいフレームバッファオブジェクトを作成する
//
GLuint Framebuffer::createFramebuffer()
{
  // 新しいフレームバッファオブジェクトを作成する
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture(GL_FRAMEBUFFER, attachment, getTexture(), 0);
  glDrawBuffers(1, &attachment);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // カラーバッファに使っているテクスチャを返す
  return getTexture();
}

///
/// 新しいフレームバッファオブジェクトを作成してそこに別のフレームバッファオブジェクトをコピーする
///
/// @param texture コピー元のテクスチャ
///
GLuint Framebuffer::copyFramebuffer(const Framebuffer& framebuffer) noexcept
{
  // コピー元のフレームバッファオブジェクトのテクスチャをコピーする
  copyTexture(framebuffer);

  // 新しいフレームバッファオブジェクトを作成する
  return createFramebuffer();
}

///
/// フレームバッファオブジェクトを破棄する
///
void Framebuffer::discardFramebuffer()
{
  // フレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &framebuffer);
}

//
// テクスチャからフレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(Texture& texture)
  : attachment{ GL_COLOR_ATTACHMENT0 }
{
  // テクスチャからフレームバッファオブジェクトを作成する
  create(texture);
}

//
// テクスチャを作成してフレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
  : Texture{ width, height, channels, pixels }
  , attachment{ GL_COLOR_ATTACHMENT0 }
{
  // フレームバッファオブジェクトを作成する
  createFramebuffer();
}

//
// 画像ファイルを読み込んでフレームバッファオブジェクトを作成するコンストラクタ
//
Framebuffer::Framebuffer(const std::string& filename)
  : Texture{ filename }
  , attachment{ GL_COLOR_ATTACHMENT0 }
{
  // フレームバッファオブジェクトを作成する
  createFramebuffer();
}

//
// コピーコンストラクタ
//
Framebuffer::Framebuffer(const Framebuffer& framebuffer) noexcept
  : attachment{ GL_COLOR_ATTACHMENT0 }
{
  // テクスチャからフレームバッファオブジェクトを作成する
  create(framebuffer);
}

//
// ムーブコンストラクタ
//
Framebuffer::Framebuffer(Framebuffer&& framebuffer) noexcept
  : attachment{ GL_COLOR_ATTACHMENT0 }
{
  // テクスチャからフレームバッファオブジェクトを作成する
  create(framebuffer);
}

//
// デストラクタ
//
Framebuffer::~Framebuffer()
{
  // フレームバッファオブジェクトを削除する
  discardFramebuffer();

  // デフォルトのフレームバッファオブジェクトに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  framebuffer = 0;
}

//
// 代入演算子
//
Framebuffer& Framebuffer::operator=(const Framebuffer& framebuffer)
{
  // 代入先のフレームバッファオブジェクトを削除する
  discardFramebuffer();

  // テクスチャからフレームバッファオブジェクトを作成する
  create(framebuffer);

  // このオブジェクトを返す
  return *this;
}

//
// テクスチャからフレームバッファオブジェクトを作成する
//
GLuint Framebuffer::create(const Texture& texture)
{
  // 引数のテクスチャーをカラーバッファに使う
  *static_cast<Texture*>(this) = texture;

  // フレームバッファオブジェクトを作成する
  return createFramebuffer();
}

//
// テクスチャを作成してフレームバッファオブジェクトを作成する
//
GLuint Framebuffer::create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels)
{
  // テクスチャを作成して入れ替える
  *static_cast<Texture*>(this) = Texture{ width, height, channels, pixels };

  // フレームバッファオブジェクトを作成する
  return createFramebuffer();
}

//
// 画像ファイルを読み込んでフレームバッファオブジェクトを作成する
//
GLuint Framebuffer::create(const std::string& filename)
{
  // 画像ファイルからテクスチャを作成して入れ替える
  *static_cast<Texture*>(this) = Texture{ filename };

  // フレームバッファオブジェクトを作成する
  return createFramebuffer();
}

//
// フレームバッファオブジェクトの表示
//
void Framebuffer::draw(GLsizei width, GLsizei height) const
{
  // テクスチャのサイズ
  const auto& size{ getSize() };

  // フレームバッファオブジェクトのアスペクト比
  const auto f{ static_cast<float>(size.width * height) };

  // ウィンドウの表示領域のアスペクト比
  const auto d{ static_cast<float>(size.height * width) };

  // 描画する領域
  GLint dx0, dy0, dx1, dy1;

  // 表示領域の右上端の位置を求める
  --width;
  --height;

  // フレームバッファオブジェクトのアスペクト比が大きかったら
  if (f > d)
  {
    // ディスプレイ上の描画する領域の高さを求める
    const auto h{ static_cast<GLint>(d / size.width + 0.5f) };

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
    const auto w{ static_cast<GLint>(f / size.height + 0.5f) };

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
  glBlitFramebuffer(0, 0, size.width - 1, size.height - 1,
    dx0, dy1, dx1, dy0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // 読み込み元を通常のフレームバッファに戻す
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  glReadBuffer(GL_BACK);
}

//
// 画像ファイルを読み込んでフレームバッファオブジェクトを更新する
//
bool Framebuffer::loadImage(const std::string& filename)
{
  if (!Texture::loadImage(filename)) return false;
  discardFramebuffer();
  createFramebuffer();
  return true;
}

//
// 動画ファイルを読み込んでフレームバッファオブジェクトを更新する
//
bool Framebuffer::loadMovie(const std::string& filename)
{
  if (!Texture::loadMovie(filename)) return false;
  discardFramebuffer();
  createFramebuffer();
  return true;
}
