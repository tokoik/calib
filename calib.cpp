﻿///
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
  
  // 初期画像を読み込んでフレームバッファオブジェクトを作成する
  Framebuffer framebuffer{ config.getInitialImage() };

  // 較正オブジェクトを作成する
  Calibration calibration{ config.getDictionaryName() };

  // メニューを作る
  Menu menu{ config, framebuffer, calibration };

  // ウィンドウが開いている間繰り返す
  while (window && menu)
  {
    // 選択しているキャプチャデバイスから１フレーム取得する
    menu.retriveFrame(framebuffer);

    // メニューを表示して更新された設定を得る
    const Settings& settings{ menu.draw() };

    // シェーダの設定を行う
    menu.setup(window.getAspect());

    // ピクセルバッファオブジェクトの内容をテクスチャにコピーする
    framebuffer.drawPixels();

    // フレームバッファオブジェクトの内容を表示する
    framebuffer.draw(window.getWidth(), window.getHeight());

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  return 0;
}
