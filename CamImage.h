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
  ///
  bool open(const std::string& filename, bool flip = false)
  {
    // 画像ファイルを読み込む
    if (!load(filename, frame)) return false;

    // 必要なら上下を反転する
    if (flip) cv::flip(frame, frame, 1);

    // 転送用の一時メモリにデータを格納する
    copyFrame();

    // 画像が読み込まれたことを記録する
    captured = true;

    // 画像ファイルが開けた
    return true;
  }

  ///
  /// 画像ファイルが開けたかどうか調べる
  ///
  /// @return ファイルが開けていたら true
  ///
  bool isOpened() const
  {
    // 画像が読み込めていたら true
    return !pixels.empty();
  }

  ///
  /// 画像ファイルを読み込む
  ///
  /// @param filename 読み込む画像ファイルのパス
  /// @param frame 読み込んだ画像
  /// @return 読み込みに成功したら true
  ///
  static bool load(const std::string& filename, cv::Mat& frame)
  {
    // 画像ファイルをバイナリモードで開いて読み込み位置を最後に移動する
    std::ifstream file(Utf8ToTChar(filename),
      std::ifstream::in |
      std::ifstream::binary |
      std::ifstream::ate);

    // 画像ファイルが開けたら
    if (file.good())
    {
      // 画像ファイルを読み込むメモリを確保する
      std::vector<char> buffer(static_cast<std::vector<char>::size_type>(file.tellg()));

      // 画像ファイルを先頭から全部読み込む
      file.seekg(0, std::ifstream::beg);
      file.read(buffer.data(), buffer.size());

      // 画像ファイルを閉じる
      file.close();

      // 画像ファイルが読み込めたら
      if (file.good())
      {
        // 読み込んだ画像データを復号して返す
        frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
        return true;
      }
    }

    // 読み込めなかった
    return false;
  }

  ///
  /// 配列に格納されている画像をファイルに保存する
  ///
  /// @param filename 保存する画像ファイルのパス
  /// @param image 保存する画像を保持している配列
  /// @return 保存に成功したら true
  ///
  static bool save(const std::string& filename, const cv::Mat& image)
  {
    // ファイル名の拡張子の場所を取り出す
    const auto pos{ filename.find_last_of('.') };

    // 拡張子があればそれを取り出し、無ければ ".png" にする
    const std::string ext{ pos != std::string::npos ? filename.substr(pos) : ".png" };
      
    // 画像の符号化に失敗したら戻る
    std::vector<uchar> buffer;
    if (!cv::imencode(ext, image, buffer)) return false;

    // 画像ファイルをバイナリモードで開く
    std::ofstream file(Utf8ToTChar(filename),
      std::ifstream::out |
      std::ifstream::binary);

    // 画像ファイルが開けなかったら戻る
    if (file.fail()) return false;

    // 画像データをファイルに書き込む
    const auto ptr{ reinterpret_cast<char*>(buffer.data()) };
    file.write(ptr, buffer.size());

    // 画像ファイルを閉じる
    file.close();

    // 読み込めたかどうかを返す
    return file.good();
  }
};
