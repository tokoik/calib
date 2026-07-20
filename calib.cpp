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

// OpenXR と OpenGL の相互運用
#include "GgOpenXR.h"

// 標準ライブラリ
#include <algorithm>
#include <iostream>

// 構成ファイル名
#define CONFIG_FILE PROJECT_NAME "_config.json"

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // --openxr が指定されたときだけ HMD を起動する
  const bool useOpenXr{ std::find(argv + 1, argv + argc, std::string{ "--openxr" }) != argv + argc };

  // 構成ファイルを読み込む
  Config config{ CONFIG_FILE };

  // 構成にもとづいてウィンドウを作成する
  GgApp::Window window{ config.getTitle(), config.getWidth(), config.getHeight() };

  // OpenXR は Window や calib に依存しない独立した描画バックエンドとして扱う
  GgOpenXR openxr;
  if (useOpenXr && !openxr.initialize(window.getNativeHandle(), config.getTitle()))
    std::cerr << "OpenXR is not available; continuing with the desktop display.\n";

  // 開いたウィンドウに対して初期化処理を実行する
  config.initialize();
  
  // キャプチャデバイスを作る
  Capture capture;

  // 較正オブジェクトを作成する
  Calibration calibration{ config.getDictionaryName(), config.getCheckerLength() };

  // メニューを作る
  Menu menu{ config, capture, calibration };

  // キャプチャデバイスで初期画像を開く
  if (!capture.openImage(config.getInitialImage())) throw std::runtime_error("Cannot open initial image.");

  // 初期画像の実解像度と焦点距離から、見やすい初期画角を設定する
  menu.initializeInputIntrinsics(capture.getSize());

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

      // ChArUco Board を認識するなら
      if (menu.detectBoard)
      {
        // ChArUco Board を検出する
        calibration.detectBoard(image);
      }
      else
      {
        // ArUco Marker を検出する
        calibration.detectMarkers(image, menu.getMarkerLength());
      }

      // ピクセルバッファオブジェクトのマップを解除する
      framebuffer.unmap();

      // ピクセルバッファオブジェクトの内容をフレームバッファオブジェクトに書き戻す
      framebuffer.drawPixels();
    }

    // 表示するウィンドウのビューポートを再設定する
    window.setMenubarHeight(menu.getMenubarHeight());

    // シェーダーでBGRAをRGBAへ変換し、縦横比を維持して実Framebuffer領域へ中央表示する
    framebuffer.draw(window.getFboWidth(), window.getFboHeight());

    // OpenXR が実行中なら、同じ展開結果を各 view の swapchain に転送する
    if (openxr.available())
    {
      openxr.pollEvents();
      if (openxr.shouldClose()) window.setClose(GLFW_TRUE);
      if (openxr.beginFrame())
      {
        if (openxr.shouldRender())
        {
          for (std::size_t view{}; view < openxr.viewCount(); ++view)
          {
            // 各眼の向きで入力画像を再展開する。単眼画像なので位置による視差は付けない。
            const auto& xrView{ openxr.getView(view) };
            const auto viewPose{ gg::ggQuaternionMatrix(gg::GgQuaternion{
              xrView.orientation[0], xrView.orientation[1],
              xrView.orientation[2], xrView.orientation[3] }) };
            const auto&& xrSize{ menu.setup(
              static_cast<GLfloat>(xrView.width) / static_cast<GLfloat>(xrView.height), viewPose) };
            framebuffer.update(xrSize, frame);

            if (!openxr.beginView(view)) continue;
            framebuffer.draw(xrView.width, xrView.height);
            openxr.endView(view);
          }
        }
        openxr.endFrame();
      }
    }

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
