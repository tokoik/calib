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
  Calibration calibration{ config.getDictionaryName(), config.getCheckerLength() };

  // メニューを作る
  Menu menu{ config, capture, calibration };

  // キャプチャデバイスで初期画像を開く
  capture.openImage(config.getInitialImage());

  // 解像度と画角の調整値の初期値を初期画像に合わせる
  menu.setSize(capture.getSize());

  // キャプチャしたフレームを保持するテクスチャ
  Texture frame;

  // 画像の展開に用いるフレームバッファオブジェクトのサイズを初期ウィンドウに合わせる
  Framebuffer framebuffer{ config.getWidth(), config.getHeight() };

  // ウィンドウが開いている間繰り返す
  while (window && menu)
  {
    // メニューを表示して設定を更新する
    menu.draw();

    // 選択しているキャプチャデバイスから１フレーム取得する
    capture.retrieve(frame);

    // ピクセルバッファオブジェクトの内容をテクスチャに転送する
    frame.drawPixels();

    // フレームバッファオブジェクトのサイズをキャプチャしたフレームに合わせる
    framebuffer.resize(frame);

    // シェーダの設定を行う
    const auto&& size{ menu.setup(framebuffer.getAspect()) };

    // フレームバッファオブジェクトにフレームを展開する
    framebuffer.update(size, frame);

    // ArUco Marker を検出するなら
    if (menu.detectMarker || menu.detectBoard)
    {
      // フレームバッファオブジェクトの内容をピクセルバッファオブジェクトに転送する
      framebuffer.readPixels();

      // 入力画像のサイズを調べる
      const auto size{ cv::Size{ framebuffer.getWidth(), framebuffer.getHeight() } };

      // ピクセルバッファオブジェクトを CPU のメモリ空間にマップする
      cv::Mat image{ size, CV_8UC(framebuffer.getChannels()), framebuffer.map() };

      if (menu.detectBoard)
      {
        // ChArUco Board を検出する
        calibration.detectBoard(image);
      }
      else
      {
        // ArUco Marker を検出する
        calibration.detectMarker(image, menu.getMarkerLength());
      }

      // ピクセルバッファオブジェクトのマップを解除する
      framebuffer.unmap();

      // ピクセルバッファオブジェクトの内容をフレームバッファオブジェクトに書き戻す
      framebuffer.drawPixels();
    }

    // 表示するウィンドウのビューポートを再設定する
    window.setMenubarHeight(menu.getMenubarHeight());

    // フレームバッファオブジェクトの内容を表示する
    //framebuffer.show(window.getWidth(), window.getHeight());
    framebuffer.draw(window.getWidth(), window.getHeight());

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
