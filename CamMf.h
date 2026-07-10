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
#include <d3d11.h>
#include <wrl/client.h>

///
/// Microsoft Media Foundation を使ってビデオをキャプチャするクラス
///
class CamMf : public Camera
{
  //
  // ビデオフォーマットの詳細を保持する構造体
  //
  struct VideoFormat
  {
    UINT32 width;     // 幅
    UINT32 height;    // 高さ
    UINT32 fpsNum;    // フレームレートの分子 (Numerator)
    UINT32 fpsDenom;  // フレームレートの分母 (Denominator)
    GUID subType;     // ピクセルフォーマット/コーデックの GUID

    //
    // コンストラクタ
    //
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
    /// COM ライブラリの初期化と終了を行うオブジェクト
    static ComInitializer instance;

    /// メディアソースのリスト
    IMFActivate** ppSourceActivate;

    /// メディアソースの数
    UINT32 cSourceActivate;

    /// ビデオキャプチャデバイスの表示名のリスト
    std::vector<std::string> deviceList;

    /// COM ライブラリが初期化されていれば true
    bool coInitialized;

    /// Media Foundation が起動されていれば true
    bool mfStarted;

    /// Direct3D 11 デバイス
    Microsoft::WRL::ComPtr<ID3D11Device> pD3D11Device;

    /// DXGI デバイスマネージャー
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> pDeviceManager;

    /// デバイスマネージャーのリセットトークン
    UINT resetToken;

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
    /// DXGI デバイスマネージャーを返す
    ///
    /// @return デバイスマネージャーのポインタ
    ///
    static IMFDXGIDeviceManager* getDeviceManager();

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

  /// メディアソース
  Microsoft::WRL::ComPtr<IMFMediaSource> pMediaSource;

  /// メディアソースのリーダー
  Microsoft::WRL::ComPtr<IMFSourceReader> pSourceReader;

  /// 使用可能なビデオフォーマットのリスト
  std::vector<VideoFormat> availableFormats;

  /// 使用可能なビデオフォーマットの表示名のリスト
  std::vector<std::string> formatList;

  ///
  /// 使用可能な解像度、フレームレート、コーデックのリストを作成する
  ///
  /// @return 成功したら true
  ///
  bool enumerateFormats();

  ///
  /// Source Reader の出力フォーマットを設定し基底クラスの frame を初期化する
  ///
  /// @param index 選択するフォーマットのリストインデックス
  ///
  bool setFormat(int index);

public:
    
  ///
  /// コンストラクタ
  ///
  CamMf()
  {
  }

  ///
  /// デストラクタ
  ///
  virtual ~CamMf()
  {
    // カメラを閉じる
    close();
  }

  ///
  /// Media Foundation のビデオデバイスの一覧を作る
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
  ///
  bool open(int device);

  ///
  /// 使用可能なビデオフォーマットの表示名のリストを返す
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
  /// フレームをキャプチャする
  ///
  void capture();

  ///
  /// カメラを閉じる
  ///
  void close();
};
