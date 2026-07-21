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
#include <MFapi.h>
#include <Mfidl.h>
#include <MFtransform.h>
#include <MFreadwrite.h>
#include <Mferror.h>
#include <wmcodecdsp.h>

///
/// Microsoft Media Foundation を使ってビデオをキャプチャするクラス
///
class CamMf : public Camera
{
  ///
  /// ビデオフォーマットの詳細を保持する構造体
  ///
  struct VideoFormat
  {
    UINT32 width;     ///< 幅
    UINT32 height;    ///< 高さ
    UINT32 fpsNum;    ///< フレームレートの分子 (Numerator)
    UINT32 fpsDenom;  ///< フレームレートの分母 (Denominator)
    GUID subType;     ///< ピクセルフォーマット/コーデックの GUID

    ///
    /// コンストラクタ
    ///
    /// @param width 幅
    /// @param height 高さ
    /// @param fpsNum フレームレートの分子
    /// @param fpsDenom フレームレートの分母
    /// @param subType ピクセルフォーマット/コーデックの GUID
    ///
    VideoFormat(UINT32 width, UINT32 height,
      UINT32 fpsNum, UINT32 fpsDenom, GUID subType)
      : width{ width }
      , height{ height }
      , fpsNum{ fpsNum }
      , fpsDenom{ fpsDenom }
      , subType{ subType }
    {
    }
  };

  ///
  /// COM ライブラリの初期化と終了を行うクラス
  ///
  class ComInitializer
  {
    /// COM ライブラリの初期化と終了を行うオブジェクト (シングルトン)
    static ComInitializer instance;

    /// メディアソースのリスト
    IMFActivate** ppSourceActivate{ nullptr };

    /// メディアソースの数
    UINT32 cSourceActivate{ 0 };

    /// ビデオキャプチャデバイスの表示名のリスト
    std::vector<std::string> deviceList;

    /// COM ライブラリが初期化されていれば true
    bool coInitialized{ false };

    /// Media Foundation が起動されていれば true
    bool mfStarted{ false };

    ///
    /// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
    ///
    ComInitializer();

    ///
    /// COM ライブラリの初期化と終了を行うクラスのデストラクタ
    ///
    ~ComInitializer();

    ///
    /// COM ライブラリを初期化して Media Foundation を開始する
    ///
    /// @return エラーメッセージ（成功時は nullptr）
    ///
    const char* initialize();

  public:

    // シングルトンなのでコピーは作らせない
    ComInitializer(const ComInitializer& com) = delete;
    ComInitializer(ComInitializer&& com) = delete;
    ComInitializer& operator=(const ComInitializer& com) = delete;
    ComInitializer& operator=(ComInitializer&&) = delete;

    ///
    /// COM ライブラリのシングルトンインスタンスを返す
    ///
    /// @return COM ライブラリのシングルトンインスタンスへの参照
    ///
    static const ComInitializer& getInstance();

    ///
    /// キャプチャデバイスを有効化してメディアソースを作成する
    ///
    /// @param device デバイスの番号
    /// @param pMediaSource 作成したメディアソースを返すポインタへのポインタ
    /// @return 成功したら true
    ///
    static bool activate(int device, IMFMediaSource** pMediaSource);

    ///
    /// ビデオキャプチャデバイスの表示名のリストを返す
    ///
    /// @return デバイス名のリスト
    ///
    static const std::vector<std::string>& getDeviceList();
  };

  /// メディアソースのポインタ
  IMFMediaSource* pMediaSource{ nullptr };

  /// メディアソースのリーダーへのポインタ
  IMFSourceReader* pSourceReader{ nullptr };

  /// MFT デコーダへのポインタ (MJPG, H264 等デコード用)
  IMFTransform* pDecoder{ nullptr };

  /// MFT デコーダの出力フレームを保持するバッファ
  IMFMediaBuffer* pDecoderBuffer{ nullptr };

  /// MFT カラーコンバータへのポインタ (RGB32 変換用)
  IMFTransform* pConverter{ nullptr };

  /// MFT カラーコンバータの出力フレームを保持するバッファ
  IMFMediaBuffer* pConverterBuffer{ nullptr };

  /// 使用可能なビデオフォーマットのリスト
  std::vector<VideoFormat> availableFormats;

  /// 使用可能なビデオフォーマットの表示情報のリスト
  std::vector<CaptureFormat> formatList;

  ///
  /// 使用可能な解像度、フレームレート、コーデックのリストを作成する
  ///
  /// @return 成功したら true
  ///
  bool enumerateFormats();

  ///
  /// 指定されたサブタイプに対応するビデオデコーダを探す
  ///
  /// @param subtype ピクセルフォーマット/コーデックの GUID
  /// @param ppDecoder 見つかったデコーダを返すポインタへのポインタ
  /// @param bAllowAsync 非同期デコーダを許可するなら TRUE
  /// @param bAllowHardware ハードウェアデコーダを許可するなら TRUE
  /// @param bAllowTranscode トランスコード専用のデコーダを許可するなら TRUE
  /// @return 結果の HRESULT コード
  ///
  HRESULT findVideoDecoder(
    const GUID& subtype,
    IMFTransform** ppDecoder,
    BOOL bAllowAsync = FALSE,
    BOOL bAllowHardware = FALSE,
    BOOL bAllowTranscode = FALSE
  ) const;

  ///
  /// MFT のセットアップと接続を行う
  ///
  /// @param pTransform セットアップする MFT のポインタ
  /// @param format 出力フレームのフォーマット
  /// @param subType 出力フレームのピクセルフォーマット/コーデックの GUID
  /// @return 結果の HRESULT コード
  ///
  HRESULT setUpPipeline(IMFTransform* pTransform,
    const VideoFormat& format, const GUID& subType) const;

  ///
  /// MFT を解放する
  ///
  /// @param pTransform 解放する MFT のポインタのポインタ
  ///
  void cleanUpTransform(IMFTransform** pTransform) const;

  ///
  /// デコーダの出力バッファを作成する
  ///
  /// @return 結果の HRESULT コード
  ///
  HRESULT createDecoderBuffer();

  ///
  /// カラーコンバータの出力バッファを作成する
  ///
  /// @return 結果の HRESULT コード
  ///
  HRESULT createConverterBuffer();

  ///
  /// Source Reader の出力フォーマットを設定し基底クラスの frame を初期化する
  ///
  /// @param index 選択するフォーマットのリストインデックス
  /// @return 成功したら true
  ///
  bool setFormat(int index);

public:
    
  ///
  /// コンストラクタ
  ///
  CamMf() = default;

  ///
  /// デストラクタ
  ///
  virtual ~CamMf()
  {
    // カメラを閉じる
    close();
  }

  ///
  /// Media Foundation のビデオデバイスの一覧を返す
  ///
  /// @return デバイス名のリスト
  ///
  static const auto& getDeviceList()
  {
    return ComInitializer::getDeviceList();
  }

  ///
  /// カメラを開く
  ///
  /// @param device デバイスの番号
  /// @param setupFormat 最初のフォーマットを設定するかどうか (遅延初期化時は false を指定)
  /// @return 開くことができたら true
  ///
  bool open(int device, bool setupFormat = true);

  ///
  /// 使用可能なビデオフォーマットの表示・選択情報を返す
  ///
  /// @return enumerateFormats() で作成した構造化フォーマットのリスト
  ///
  const auto& getFormatList() const
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
  /// フレームをキャプチャする（別スレッドでループ実行される）
  ///
  void capture();

  ///
  /// キャプチャスレッドを停止する
  ///
  virtual void stop() override;

  ///
  /// カメラを閉じる
  ///
  void close();
};
