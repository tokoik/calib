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

// 標準ライブラリ
#include <algorithm>
#include <chrono>
#include <iostream>

// 構成ファイル名
#define CONFIG_FILE PROJECT_NAME "_config.json"

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
#if defined(GG_USE_OPENXR)
  // --openxr が指定されたときだけ HMD を起動する
  const bool useOpenXr{ std::find(argv + 1, argv + argc, std::string{ "--openxr" }) != argv + argc };
#endif

  // 構成ファイルを読み込む
  Config config{ CONFIG_FILE };

  // 構成にもとづいてウィンドウを作成する
  GgApp::Window window{ config.getTitle(), config.getWidth(), config.getHeight() };

#if defined(GG_USE_OPENXR)
  // OpenXR の初期化
  GgApp::OpenXR* openxr{ nullptr };
  if (useOpenXr)
  {
    try
    {
      openxr = &GgApp::OpenXR::initialize(window, XR_REFERENCE_SPACE_TYPE_STAGE, config.getTitle().c_str());
    }
    catch (const std::exception& e)
    {
      std::cerr << "OpenXR: " << e.what() << '\n';
      std::cerr << "OpenXR is not available; continuing with the desktop display.\n";
    }
  }
#endif

  // 開いたウィンドウに対して初期化処理を実行する
  config.initialize();
  
  // キャプチャデバイスを作る
  Capture capture;

  // 較正オブジェクトを作成する
  Calibration calibration{ config.getDictionaryName(), config.getCheckerSize(), config.getCheckerLength() };

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
    // フレーム間の実経過時間 (deltaTime) を計測する
    static auto lastFrameTime{ std::chrono::steady_clock::now() };
    const auto currentFrameTime{ std::chrono::steady_clock::now() };
    float deltaTime{ std::chrono::duration<float>(currentFrameTime - lastFrameTime).count() };
    lastFrameTime = currentFrameTime;
    if (deltaTime <= 0.0f || deltaTime > 0.5f) deltaTime = 0.033f;

    // 描画フレームレートの実測と診断出力 (1秒ごと)
    static auto lastRenderFpsReport{ std::chrono::steady_clock::now() };
    static int renderFrameCount{ 0 };
    ++renderFrameCount;
    const auto renderElapsed{ std::chrono::duration<double>(currentFrameTime - lastRenderFpsReport).count() };
    if (renderElapsed >= 2.0)
    {
      const double rFps{ renderFrameCount / renderElapsed };
      std::cout << "calib: Render FPS = " << rFps << std::endl;
      renderFrameCount = 0;
      lastRenderFpsReport = currentFrameTime;
    }

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

        // 自動キャプチャの静止・多様性判定および記録処理
        menu.updateAutoCapture(deltaTime);
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

#if defined(GG_USE_OPENXR)
    // OpenXR が実行中なら、各 view の swapchain にレンダリングする
    if (openxr && openxr->isRunning() && openxr->begin())
    {
      const auto viewCount{ openxr->getViewCount() };
      for (uint32_t view = 0; view < viewCount; ++view)
      {
        // 各眼の向きで入力画像を再展開する。単眼画像なので位置による視差は付けない。
        const auto& pose{ openxr->getPose(view) };
        const auto viewPose{ gg::ggQuaternionMatrix(gg::GgQuaternion{
          pose.orientation.x, pose.orientation.y,
          pose.orientation.z, pose.orientation.w }) };
        const auto&& xrSize{ menu.setup(openxr->getAspect(view), viewPose) };
        framebuffer.update(xrSize, frame);

        openxr->select(view);
        framebuffer.draw(openxr->getWidth(view), openxr->getHeight(view));
        openxr->commit(view);
      }
      openxr->submit(false);
    }
#endif

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
