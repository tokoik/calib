#pragma once

///
/// フレームバッファオブジェクトクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// テクスチャ
#include "Texture.h"

// 展開に用いるメッシュ
#include "Mesh.h"

///
/// フレームバッファオブジェクトクラス
///
class Framebuffer
{
  /// フレームバッファのカラーバッファのサイズ
  std::array<int, 2> framebufferSize;

  /// フレームバッファのカラーバッファのチャネル数
  int framebufferChannels;

  /// フレームバッファオブジェクト名
  GLuint framebufferName;

  /// フレームバッファオブジェクトのレンダーターゲット
  GLenum attachment;

  /// ビューポートの保存用
  std::array<GLint, 4> viewport;

  /// フレームバッファのカラーバッファに用いるテクスチャ
  Texture& texture;

  // 展開に用いるメッシュ
  Mesh mesh;

  ///
  /// フレームバッファオブジェクトを初期化する
  ///
  void initialize();

  ///
  /// フレームバッファオブジェクトを作成する
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに用いるテクスチャ
  ///
  void createFramebuffer(GLuint texture);

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Framebuffer()
    : framebufferSize{ 0, 0 }
    , framebufferChannels{ 0 }
    , framebufferName{ 0 }
    , attachment{ GL_COLOR_ATTACHMENT0 }
    , viewport{ 0, 0, 0, 0 }
    , texture { Texture{} }
  {
  }

  ///
  /// 指定したテクスチャをカラーバッファに使って
  /// フレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに用いるテクスチャ
  ///
  Framebuffer(Texture& texture, GLenum attachment = GL_COLOR_ATTACHMENT0);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param framebuffer コピー元
  ///
  Framebuffer(const Framebuffer& framebuffer) = delete;

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
  /// 代入演算子は使用しない
  ///
  /// @param framebuffer 代入元
  ///
  Framebuffer& operator=(const Framebuffer& framebuffer) = delete;

  ///
  /// ムーブ代入演算子
  ///
  /// @param framebuffer ムーブ代入元
  ///
  Framebuffer& operator=(Framebuffer&& framebuffer) noexcept;

  ///
  /// フレームバッファオブジェクトを破棄する
  ///
  void discard();

  ///
  /// テクスチャを得る
  ///
  /// @return テクスチャ名
  ///
  auto getFramebufferName() const
  {
    return framebufferName;
  }

  ///
  /// フレームバッファオブジェクトのサイズを得る
  ///
  const auto& getFramebufferSize() const
  {
    return framebufferSize;
  }

  ///
  /// フレームバッファオブジェクトの横の画素数を得る
  ///
  const auto getFramebufferWidth() const
  {
    return framebufferSize[0];
  }

  ///
  /// フレームバッファオブジェクトの縦の画素数を得る
  ///
  const auto getFramebufferHeight() const
  {
    return framebufferSize[1];
  }

  ///
  /// フレームバッファオブジェクトのチャネル数を得る
  ///
  const auto getFramebufferChannels() const
  {
    return framebufferChannels;
  }

  ///
  /// レンダリング先をフレームバッファオブジェクトに切り替える
  ///
  void use();

  ///
  /// レンダリング先を通常のフレームバッファに戻す
  ///
  void unuse() const;

  ///
  /// テクスチャを展開してフレームバッファオブジェクトを更新する
  ///
  /// @param size 展開に用いるメッシュの分割数
  ///
  void update(const std::array<int, 2>& size);

  ///
  /// テクスチャを展開してフレームバッファオブジェクトを更新する
  ///
  /// @param size 展開に用いるメッシュの分割数
  /// @param frame 展開するフレームを格納したテクスチャ
  /// @param unit テクスチャのマッピングに使うテクスチャユニットの番号
  ///
  void update(const std::array<int, 2>& size, const Texture& frame, int unit = 0);

  ///
  /// フレームバッファオブジェクトの内容を表示する
  ///
  /// @param width フレームバッファオブジェクトの内容を表示する横の画素数
  /// @param height フレームバッファオブジェクトの内容を表示する縦の画素数
  ///
  void draw(GLsizei width, GLsizei height) const;
};
