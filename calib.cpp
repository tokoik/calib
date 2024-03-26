///
/// ChArUco Board によるカメラキャリブレーション
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// ウィンドウ関連の処理
#include "GgApp.h"

// 構成データ
#include "Config.h"

// メニュー
#include "Menu.h"

// テクスチャ
#include "Texture.h"

// フレームバッファオブジェクト
#include "Framebuffer.h"

// 較正
#include "Calibration.h"

// 構成ファイル名
#define CONFIG_FILE PROJECT_NAME "_config.json"

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // 構成ファイルを読み込む
  Config config{ CONFIG_FILE };

  // 構成にもとづいてウィンドウを作成する
  GgApp::Window window{ config.getTitle(), config.getWidth(), config.getHeight() };

  // 開いたウィンドウに対して初期化処理を実行する
  config.initialize();

  // 初期画像を読み込んでテクスチャを作成する
  Texture texture{ config.getInitialImage() };

  // 作成したテクスチャをカラーバッファに使ってフレームバッファオブジェクトを作成する
  Framebuffer framebuffer{ texture };

  // 作成したテクスチャの較正オブジェクトを作成する
  Calibration calibration{ texture };

  // メニューを作る
  Menu menu{ config, texture, framebuffer, calibration };

  // ウィンドウが開いている間繰り返す
  while (window)
  {
    // メニューを表示して更新された設定を得る
    const Settings& settings{ menu.draw() };

    // シェーダの設定を行う
    menu.setup(window.getAspect());

    // テクスチャの内容をピクセルバッファオブジェクトに転送する
    //texture.readPixels();

    // ピクセルバッファオブジェクトを CPU のメモリ空間にマップする
    //cv::Mat image{ texture.getSize(), CV_8UC3, texture.mapBuffer() };

    std::vector<std::vector<cv::Point2f>> corners, rejected;
    std::vector<int> ids;

    // 検出結果を表示に描き込む
    //cv::aruco::drawDetectedMarkers(image, corners, ids);

    // フレームバッファオブジェクトの内容を表示する
    framebuffer.draw(window.getWidth(), window.getHeight());

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
