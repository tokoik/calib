#pragma once

///
/// AV Foundation を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date September 2026
///

#if defined(__APPLE__)

// カメラ関連の処理
#include "Camera.h"

// 標準ライブラリ
#include <vector>
#include <string>
#include <memory>

///
/// AV Foundation を使ってビデオをキャプチャするクラス
///
class CamAvf : public Camera
{
  ///
  /// ビデオフォーマットの詳細を保持する内部構造体
  ///
  struct VideoFormat
  {
    int width{ 0 };        ///< 幅
    int height{ 0 };       ///< 高さ
    double fps{ 0.0 };     ///< フレームレート
    std::string codec;     ///< コーデック名
    int formatIndex{ 0 };  ///< AVCaptureDevice.formats のインデックス
    int rangeIndex{ 0 };   ///< videoSupportedFrameRateRanges のインデックス

    ///
    /// コンストラクタ
    ///
    /// @param width 幅
    /// @param height 高さ
    /// @param fps フレームレート
    /// @param codec コーデック名
    /// @param formatIndex フォーマットのインデックス
    /// @param rangeIndex レンジのインデックス
    ///
    VideoFormat(int width, int height, double fps, const std::string& codec,
      int formatIndex, int rangeIndex)
      : width{ width }
      , height{ height }
      , fps{ fps }
      , codec{ codec }
      , formatIndex{ formatIndex }
      , rangeIndex{ rangeIndex }
    {
    }
  };

  /// 内部実装オブジェクト (PIMPL)
  struct Impl;
  std::unique_ptr<Impl> impl;

  /// 使用可能なビデオフォーマットの詳細リスト
  std::vector<VideoFormat> availableFormats;

  /// 使用可能なビデオフォーマットの表示情報のリスト
  std::vector<CaptureFormat> formatList;

  /// 選択されているフォーマットのインデックス
  int selectedFormatIndex{ 0 };

  /// 使用可能なカメラデバイスの表示名のリスト
  static std::vector<std::string> deviceList;

  ///
  /// 使用可能な解像度、フレームレート、コーデックのリストを作成する
  ///
  /// @return 成功したら true
  ///
  bool enumerateFormats();

protected:

  ///
  /// キャプチャ開始処理を行う
  ///
  /// @return 正常に開始できたら true
  ///
  virtual bool onStart() override;

  ///
  /// キャプチャ停止処理を行う
  ///
  virtual void onStop() override;

  ///
  /// キャプチャデバイスを閉じる処理を行う
  ///
  virtual void onClose() override;

public:

  ///
  /// コンストラクタ
  ///
  CamAvf();

  ///
  /// デストラクタ
  ///
  virtual ~CamAvf();

  ///
  /// AV Foundation のビデオデバイスの一覧を返す
  ///
  /// @return デバイス名のリスト
  ///
  static const std::vector<std::string>& getDeviceList();

  ///
  /// カメラを開く
  ///
  /// @param deviceNumber デバイスの番号
  /// @param setupFormat 最初のフォーマットを設定するかどうか (遅延初期化時は false を指定)
  /// @return 開くことができたら true
  ///
  bool open(int deviceNumber, bool setupFormat = true);

  ///
  /// 使用可能なビデオフォーマットの表示・選択情報を返す
  ///
  /// @return 構造化フォーマットのリスト
  ///
  virtual const std::vector<CaptureFormat>& getFormatList() const override
  {
    return formatList;
  }

  ///
  /// フォーマットを選択して設定する
  ///
  /// @param index 選択するフォーマットのリストインデックス
  /// @return 成功したら true
  ///
  bool select(int index);

  ///
  /// ビデオフォーマットを選択して設定する
  ///
  /// @param index 選択するフォーマットのインデックス
  /// @return 正常に設定できたら true
  ///
  virtual bool selectFormat(int index) override
  {
    return select(index);
  }

  ///
  /// レイテンシ優先モードを設定する
  ///
  /// @param mode レイテンシを優先する場合は true
  ///
  virtual void setPrioritizeLatency(bool mode) override;

  ///
  /// サンプルバッファからフレームデータを取得してバッファを更新する
  ///
  /// @param sampleBuffer CMSampleBufferRef の不透明ポインタ
  ///
  void handleSampleBuffer(const void* sampleBuffer);
};

#endif // defined(__APPLE__)
