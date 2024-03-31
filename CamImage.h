#pragma once

///
/// OpenCV を使って画像ファイルを読み込むクラス
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// カメラ関連の処理
#include "Camera.h"

// ファイル入出力
#include "fstream"

///
/// OpenCV を使って画像ファイルを読み込むクラス
///
class CamImage : public Camera
{
public:

  ///
  /// コンストラクタ
  ///
  CamImage()
  {
  }

  ///
  /// 画像ファイルを開いて読み込むコンストラクタ
  ///
  CamImage(std::string& filename, bool flip = false)
  {
    open(filename, flip);
  }

  ///
  /// デストラクタ
  ///
  virtual ~CamImage()
  {
  }

  ///
  /// 画像ファイルを開く
  ///
  /// @param filename 画像ファイル名
  /// @param flip 上下を反転するときは true
  /// @param repeat テクスチャを繰り返すときは true
  ///
  bool open(const std::string& filename, bool flip = false)
  {
    // 画像ファイルをバイナリモードで開いて読み込み位置を最後に移動する
    std::ifstream file(Utf8ToTChar(filename), std::ifstream::in | std::ifstream::binary | std::ifstream::ate);

    // 画像ファイルが開けなかったら戻る
    if (file.fail()) return false;

    // 画像ファイルを読み込むメモリを確保する
    std::vector<char> buffer(static_cast<std::vector<char>::size_type>(file.tellg()));

    // 画像ファイルを先頭から全部読み込む
    file.seekg(0, std::ifstream::beg);
    file.read(buffer.data(), buffer.size());

    // 画像ファイルの読み込みに失敗したら戻る
    if (file.fail()) return false;

    // 画像ファイルを閉じる
    file.close();

    // 読み込んだ画像データを復号する
    frame = cv::imdecode(buffer, cv::IMREAD_COLOR);

    // 画像データが復号できなかったら戻る
    if (frame.empty()) return false;

    // 必要なら上下を反転する
    if (flip) cv::flip(frame, frame, 1);

    // 転送用の一時メモリにデータを格納する
    copyFrame();

    // 画像が読み込まれたことを記録する
    captured = true;

    return true;
  }

  ///
  /// ファイルが開けたかどうか調べる
  ///
  /// @return ファイルが開けていたら true
  ///
  bool isOpened() const
  {
    // 画像が読み込めていたら true
    return !pixels.empty();
  }
};
