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

// キャプチャデバイス
#include "Capture.h"

// 較正
#include "Calibration.h"

// メニュー
#include "Menu.h"

// フレームバッファオブジェクト
#include "Framebuffer.h"

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
  
  // キャプチャデバイスを作る
  Capture capture;

  // 較正オブジェクトを作成する
  Calibration calibration{ config.getDictionaryName() };

  // メニューを作る
  Menu menu{ config, capture, calibration };

  // キャプチャデバイスで初期画像を開く
  capture.openImage(config.getInitialImage());

  // 解像度と画角の調整値の初期値を初期画像に合わせる
  menu.setSizeAndFov(capture.getSize(), Settings::defaultFocal);

  // キャプチャしたフレームを保持するテクスチャ
  Texture frame;

  // フレームバッファオブジェクトのカラーバッファに用いるテクスチャ
  Texture color{ 1280, 720, 3 };

  // 画像の展開に用いるフレームバッファオブジェクト
  Framebuffer framebuffer{ color };

  // ウィンドウが開いている間繰り返す
  while (window && menu)
  {
    // メニューを表示して設定を更新する
    menu.draw();

    // 選択しているキャプチャデバイスから１フレーム取得する
    capture.retrieve(frame);

    // ピクセルバッファオブジェクトの内容をテクスチャに転送する
    frame.drawPixels();

    // シェーダの設定を行う
    const auto&& size{ menu.setup(window.getAspect()) };

    // フレームバッファオブジェクトにフレームを展開する
    framebuffer.update(size, frame);

    // フレームバッファオブジェクトの内容をピクセルバッファオブジェクトに転送する
    color.readPixels();

    // ArUco Marker を検出するなら ArUco Marker と ArUco Board を検出する
    if (menu.detectMarker) calibration.detect(color, menu.detectBoard);

    // ピクセルバッファオブジェクトの内容をフレームバッファオブジェクトに書き戻す
    color.drawPixels();

    // フレームバッファオブジェクトの内容を表示する
    framebuffer.draw(window.getWidth(), window.getHeight());

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
