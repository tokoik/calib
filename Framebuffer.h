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
class Framebuffer : public Texture
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

  // 展開に用いるメッシュ
  Mesh mesh;

  ///
  /// フレームバッファオブジェクトを作成する
  ///
  /// @param textureName フレームバッファオブジェクトのカラーバッファに使うテクスチャ名
  /// @param attachment フレームバッファオブジェクトのカラーバッファのアタッチメント
  /// @return 作成したフレームバッファオブジェクト名
  ///
  static GLuint createFramebuffer(GLuint textureName, GLenum attachment);

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Framebuffer()
    : Texture{}
    , framebufferSize{ 0, 0 }
    , framebufferChannels{ 0 }
    , framebufferName{ 0 }
    , attachment{ GL_COLOR_ATTACHMENT0 }
    , viewport{ 0, 0, 0, 0 }
  {
  }

  ///
  /// フレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param width 作成するフレームバッファオブジェクトの横の画素数
  /// @param height 作成するフレームバッファオブジェクトの縦の画素数
  /// @param channels 作成するフレームバッファオブジェクトのチャネル数
  /// @param attachment フレームバッファオブジェクトのカラーバッファのアタッチメント
  ///
  Framebuffer(GLsizei width, GLsizei height, int channels = 3,
    GLenum attachment = GL_COLOR_ATTACHMENT0);

  ///
  /// コピーコンストラクタ
  ///
  /// @param framebuffer コピー元のフレームバッファオブジェクト
  ///
  Framebuffer(const Framebuffer& framebuffer);

  ///
  /// ムーブコンストラクタ
  ///
  /// @param framebuffer ムーブ元のフレームバッファオブジェクト
  ///
  Framebuffer(Framebuffer&& framebuffer) noexcept;

  ///
  /// デストラクタ
  ///
  virtual ~Framebuffer();

  ///
  /// 代入演算子
  ///
  /// @param framebuffer 代入元のフレームバッファオブジェクト
  /// @return 代入結果のテクスチャ
  ///
  Framebuffer& operator=(const Framebuffer& framebuffer);

  ///
  /// ムーブ代入演算子
  ///
  /// @param framebuffer ムーブ代入元のフレームバッファオブジェクト
  /// @return ムーブ代入結果のレームバッファオブジェクト
  ///
  Framebuffer& operator=(Framebuffer&& framebuffer) noexcept;

  ///
  /// フレームバッファオブジェクトを作成する
  ///
  /// @param width 作成するフレームバッファオブジェクトの横の画素数
  /// @param height 作成するフレームバッファオブジェクトの縦の画素数
  /// @param channels 作成するフレームバッファオブジェクトのチャネル数
  ///
  /// @note
  /// このフレームバッファオブジェクトのサイズが引数で指定したサイズと異なれば、
  /// このフレームバッファオブジェクトを削除して、
  /// 新しいフレームバッファオブジェクトを作り直す。
  ///
  virtual void create(GLsizei width, GLsizei height, int channels);

  ///
  /// フレームバッファオブジェクトをコピーする
  ///
  /// @param framebuffer コピー元のフレームバッファオブジェクト
  ///
  /// @note
  /// このフレームバッファオブジェクトのサイズがコピー元と異なれば、
  /// このフレームバッファオブジェクトを削除して、
  /// 新しいフレームバッファオブジェクトを作り直してコピーする。
  /// 引数の型が Buffer なのは、このオブジェクトの型が Buffer のとき
  /// Buffer::copy()、Texture のとき Texture::copy()、
  /// Framebuffer のとき Framebuffer::copy() を呼ぶようにするため。
  ///
  virtual void copy(const Buffer& framebuffer) noexcept;

  ///
  /// フレームバッファオブジェクトを破棄する
  ///
  virtual void discard();

  ///
  /// フレームバッファオブジェクト名を得る
  ///
  /// @return フレームバッファオブジェクト名
  ///
  auto getFramebufferName() const
  {
    return framebufferName;
  }

  ///
  /// フレームバッファオブジェクトのサイズを得る
  ///
  /// @return フレームバッファオブジェクトのサイズ
  ///
  virtual const std::array<GLsizei, 2>& getSize() const
  {
    return framebufferSize;
  }

  ///
  /// フレームバッファオブジェクトのチャネル数を得る
  ///
  /// @return フレームバッファオブジェクトのチャネル数
  ///
  virtual int getChannels() const
  {
    return framebufferChannels;
  }

  ///
  /// レンダリング先をフレームバッファオブジェクトに切り替える
  ///
  /// @note
  /// この後から unbindFramebuffer() の前まで、
  /// レンダリング先がフレームバッファオブジェクトになる。
  /// この時点のビューポートを保存する。
  /// この間でレンダリング処理を実行する。
  ///
  void bindFramebuffer();

  ///
  /// レンダリング先を通常のフレームバッファに戻す
  ///
  /// @note
  /// bindFramebuffer() の後からこの前まで、
  /// レンダリング先がフレームバッファオブジェクトになる。
  /// 保存したビューポートはここで復帰する。
  /// この間でレンダリング処理を実行する。
  ///
  void unbindFramebuffer() const;

  ///
  /// テクスチャを展開してフレームバッファオブジェクトを更新する
  ///
  /// @param size 展開に用いるメッシュの分割数
  ///
  /// @note
  /// レンダリング先をフレームバッファオブジェクトに切り替えて、
  /// フレームバッファオブジェクト全体を覆うメッシュを描画する。
  /// これより前でシェーダの選択やテクスチャの結合を行う。
  ///
  void update(const std::array<int, 2>& size);

  ///
  /// テクスチャを展開してフレームバッファオブジェクトを更新する
  ///
  /// @param size 展開に用いるメッシュの分割数
  /// @param frame 展開するフレームを格納したテクスチャ
  /// @param unit テクスチャのマッピングに使うテクスチャユニットの番号
  ///
  /// @note
  /// レンダリング先をフレームバッファオブジェクトに切り替えて、
  /// テクスチャユニット unit を指定して frame に格納されたテクスチャを結合してから、
  /// フレームバッファオブジェクト全体を覆うメッシュを描画する。
  /// これより前でシェーダの選択を行う。
  /// unit には シェーダにおいてテクスチャのサンプラの uniform 変数に設定する
  /// GL_TEXTUREi の i と一致させること。
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
