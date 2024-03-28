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

///
/// フレームバッファオブジェクトクラス
///
class Framebuffer : public Texture
{
  /// フレームバッファオブジェクトのレンダーターゲット
  GLenum attachment;

  /// フレームバッファオブジェクト
  GLuint framebuffer;

  ///
  /// 現在のテクスチャをカラーバッファに使って新しいフレームバッファオブジェクトを作成する
  ///
  void createFramebuffer();

public:

  ///
  /// デフォルトコンストラクタ
  ///
  Framebuffer();

  ///
  /// 指定したテクスチャをカラーバッファに使ってフレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param texture フレームバッファオブジェクトのカラーバッファに使うテクスチャ
  ///
  Framebuffer(const Texture& texture);

  ///
  /// 画像ファイルを読み込んでフレームバッファオブジェクトを作成するコンストラクタ
  ///
  /// @param filename 画像ファイル名
  ///
  Framebuffer(const std::string& filename);

  ///
  /// コピーコンストラクタは使用しない
  ///
  /// @param framebuffer コピー元
  ///
  Framebuffer(const Framebuffer& framebuffer) = delete;

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
  /// 既存のフレームバッファオブジェクトを破棄して新しいフレームバッファオブジェクトを作成する
  ///
  /// @param width 作成するフレームバッファオブジェクトの横の画素数
  /// @param height 作成するフレームバッファオブジェクトの縦の画素数
  /// @param channels 作成するフレームバッファオブジェクトのチャネル数
  /// @param pixels 作成するフレームバッファオブジェクトに格納するデータのポインタ
  /// @return フレームバッファオブジェクトのカラーバッファに使っているテクスチャ名
  ///
  virtual GLuint create(GLsizei width, GLsizei height, int channels, const GLvoid* pixels = nullptr);

  ///
  /// 既存のフレームバッファオブジェクトを破棄して新しいフレームバッファオブジェクトに画像ファイルを読み込む
  ///
  /// @param 読み込む画像ファイル名
  /// @return 画像ファイルの読み込みに成功したら true
  ///
  virtual bool loadImage(const std::string& filename);

  ///
  /// 既存のフレームバッファオブジェクトを破棄して新しいフレームバッファオブジェクトに動画ファイルを読み込む
  ///
  /// @param 読み込む動画ファイル名
  /// @return 動画ファイルの読み込みに成功したら true
  ///
  virtual bool loadMovie(const std::string& filename);

  ///
  /// レンダリング先をフレームバッファオブジェクトに切り替える
  ///
  void use() const;

  ///
  /// レンダリング先を通常のフレームバッファに戻す
  ///
  void unuse() const;

  ///
  /// フレームバッファオブジェクトの内容を表示する
  ///
  /// @param width フレームバッファオブジェクトの内容を表示する横の画素数
  /// @param height フレームバッファオブジェクトの内容を表示する縦の画素数
  ///
  void draw(GLsizei width, GLsizei height) const;
};
