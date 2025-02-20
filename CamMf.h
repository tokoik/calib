#pragma once

///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///

// カメラ関連の処理
#include "Camera.h"

// Microsoft Media Foundation
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>

///
/// Microsoft Media Foundation を使ってビデオをキャプチャするクラス
///
class CamMf : public Camera
{
  // ビデオキャプチャデバイスの表示名のリスト
  static std::vector<std::string> deviceList;

  // メディアソースのリスト
  static IMFActivate** ppSourceActivate;

  // メディアソースの数
  static UINT32 cSourceActivate;

  IMFSourceReader* pSourceReader;
  IMFMediaSource* pMediaSource;
  HWND hwndVideo;

public:

  ///
  /// コンストラクタ
  ///
  CamMf()
    : pSourceReader{ nullptr }
    , pMediaSource{ nullptr }
    , hwndVideo{ nullptr }
  {
  }

  ///
  /// デストラクタ
  ///
  virtual ~CamMf()
  {
    // Source Reader を解放する
    if (pSourceReader) pSourceReader->Release();

    // Media Source 解放する
    if (pMediaSource) pMediaSource->Release();

    // Media Foundation をシャットダウンする
    MFShutdown();
  }

  ///
  /// 初期化
  ///
  static bool init();

  ///
  /// 後始末
  ///
  static void cleanup();

  ///
  /// カメラを開く
  ///
  /// @param device デバイスの番号
  ///
  bool open(int device);

  ///
  /// Media Foundation のビデオデバイスの一覧を作る
  ///
  /// @return デバイス名のリスト
  ///
  const std::vector<std::string>& getDeviceList()
  {
    return deviceList;
  }
};
