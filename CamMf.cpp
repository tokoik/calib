///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///
#include "CamMf.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// Microsoft Media Foundation
#pragma comment(lib, "MF.lib")
#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFuuid.lib")
#pragma comment(lib, "MFreadwrite.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "d3d11.lib")

// COM ライブラリの初期化と終了を行うオブジェクト
CamMf::ComInitializer CamMf::ComInitializer::instance;

//
// GUID から人間が読める形式の名前を返すヘルパー関数
//
std::string SubTypeToName(const GUID& subType)
{
  if (subType == MFVideoFormat_YUY2) return "YUY2";
  if (subType == MFVideoFormat_NV12) return "NV12";
  if (subType == MFVideoFormat_MJPG) return "MJPG";
  if (subType == MFVideoFormat_H264) return "H264";

  // TODO: 他に使用するフォーマットがあればここに追加
  //if (subType == MFVideoFormat_RGB24) return "RGB24";
  if (subType == MFVideoFormat_RGB32) return "RGB32";

  return "";
}

//
// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
//
CamMf::ComInitializer::ComInitializer()
  : deviceList{}
  , ppSourceActivate{ nullptr }
  , cSourceActivate{ 0 }
  , coInitialized{ false }
  , mfStarted{ false }
  , pD3D11Device{ nullptr }
  , pDeviceManager{ nullptr }
  , resetToken{ 0 }
{
}

//
// COM ライブラリの初期化と終了を行うクラスのデストラクタ
//
CamMf::ComInitializer::~ComInitializer()
{
  // メディアソースのリストを取得していれば
  if (ppSourceActivate)
  {
    // すべてのメディアソースを解放して
    for (DWORD i = 0; i < cSourceActivate; ++i)
    {
      if (ppSourceActivate[i]) ppSourceActivate[i]->Release();
    }

    // メディアソースのリストに使ったメモリを解放して
    CoTaskMemFree(ppSourceActivate);
    ppSourceActivate = nullptr;

    // ビデオキャプチャデバイスの表示名のリストを空にする
    deviceList.clear();
  }

  // D3D11 デバイスとマネージャーを解放する
  pDeviceManager.Reset();
  pD3D11Device.Reset();

  // Media Foundation が起動していればシャットダウンする
  if (mfStarted) MFShutdown();

  // COM ライブラリが初期化されていれば終了する
  if (coInitialized) CoUninitialize();
}

//
// 初期化
//
const char* CamMf::ComInitializer::initialize()
{
  // 検索条件を保持する属性ストア
  Microsoft::WRL::ComPtr<IMFAttributes> pAttributes;

  // COM ライブラリを初期化する
  if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
  {
    // COM ライブラリの初期化に失敗した
    return "Failed to initialize COM library.";
  }

  coInitialized = true;

  // Media Foundation を起動する
  if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL)))
  {
    // Media Foundation の起動に失敗した
    return "Failed to start Media Foundation.";
  }

  // Media Foundation が起動されたことを記録しておく
  mfStarted = true;

  // D3D11 デバイスの作成に使うフラグと機能レベルのリスト
  UINT creationFlags
  {
    D3D11_CREATE_DEVICE_VIDEO_SUPPORT |
    D3D11_CREATE_DEVICE_BGRA_SUPPORT
  };

  // D3D11 デバイスの機能レベルのリスト
  D3D_FEATURE_LEVEL featureLevels[]
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3
  };

  // D3D11 デバイスが作成できたら
  if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
    creationFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION,
    &pD3D11Device, nullptr, nullptr)))
  {
    // マルチスレッド保護を有効にする
    Microsoft::WRL::ComPtr<ID3D10Multithread> pMultithread;
    if (SUCCEEDED(pD3D11Device.As(&pMultithread)))
    {
      pMultithread->SetMultithreadProtected(TRUE);
    }

    // DXGI デバイスマネージャーを作成して D3D11 デバイスを登録する
    if (SUCCEEDED(MFCreateDXGIDeviceManager(&resetToken, &pDeviceManager)))
    {
      pDeviceManager->ResetDevice(pD3D11Device.Get(), resetToken);
    }
  }

  // 検索条件を保持する属性ストアを作成する
  if (FAILED(MFCreateAttributes(pAttributes.GetAddressOf(), 1)))
  {
    return "Failed to create attribute store.";
  }

  // 属性ストアにビデオキャプチャデバイスの属性を設定する
  if (FAILED(pAttributes->SetGUID(
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
  {
    return "Failed to set attribute for video capture device.";
  }

  // メディアソースを列挙する
  if (FAILED(MFEnumDeviceSources(pAttributes.Get(),
    &ppSourceActivate, &cSourceActivate)))
  {
    return "Failed to enumerate media sources.";
  }

  // すべてのメディアソースについて
  for (DWORD i = 0; i < cSourceActivate; ++i)
  {
    WCHAR* szFriendlyName{ nullptr };
    UINT32 cFriendlyName{ 0 };
    if (SUCCEEDED(ppSourceActivate[i]->GetAllocatedString(
      MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cFriendlyName)))
    {
      std::stringstream ss;
      ss << TCharToUtf8(szFriendlyName) << "##" << i;
      deviceList.emplace_back(ss.str());
    }
    CoTaskMemFree(szFriendlyName);
  }

  return nullptr;
}

//
// COM ライブラリのシングルトンインスタンスを返す
//
const CamMf::ComInitializer& CamMf::ComInitializer::getInstance()
{
  // COM ライブラリが初期化され Media Foundation が起動されていなければ
  if (!instance.ppSourceActivate)
  {
    // COM ライブラリを初期化して Media Foundation を起動する
    auto message{ instance.initialize() };

    // COM ライブラリの初期化と Media Foundation の起動に失敗したら例外を投げる
    if (message) throw std::runtime_error(message);
  }

  // COM ライブラリのシングルトンインスタンスへの参照を返す
  return instance;
}

//
// 有効化
//
bool CamMf::ComInitializer::activate(int device, IMFMediaSource** pMediaSource)
{
  // メディアソースを作成して結果を返す
  return device >= 0 && static_cast<UINT32>(device) < instance.cSourceActivate
    && SUCCEEDED(instance.ppSourceActivate[device]->ActivateObject(IID_PPV_ARGS(pMediaSource)))
    && pMediaSource;
}

//
// ビデオキャプチャデバイスの表示名のリストを返す
//
const std::vector<std::string>& CamMf::ComInitializer::getDeviceList()
{
  return getInstance().deviceList;
}

//
// DXGI デバイスマネージャーを返す
//
IMFDXGIDeviceManager* CamMf::ComInitializer::getDeviceManager()
{
  return getInstance().pDeviceManager.Get();
}

//
// 使用可能な解像度、フレームレート、コーデックのリストを作成する
//
bool CamMf::enumerateFormats()
{
  // Source Reader が作成されていなければ戻る
  if (!pSourceReader) return false;

  // 以前に作成したリストをクリアする
  availableFormats.clear();
  formatList.clear();

  // ネイティブメディアタイプを一つずつ列挙する
  Microsoft::WRL::ComPtr<IMFMediaType> pMediaType;
  for (DWORD dwMediaTypeIndex = 0;
    SUCCEEDED(pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      dwMediaTypeIndex, pMediaType.ReleaseAndGetAddressOf()));
    ++dwMediaTypeIndex
    )
  {
    // メディアタイプを取得する
    GUID majorType{};
    if (FAILED(pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType))) continue;

    // メディアタイプがビデオで無ければ次へ
    if (majorType != MFMediaType_Video) continue;

    // ビデオフォーマットを取得する
    GUID subType{};
    if (FAILED(pMediaType->GetGUID(MF_MT_SUBTYPE, &subType))) continue;

    // 解像度を取得する
    UINT32 width{ 0 }, height{ 0 };
    if (FAILED(MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height))) continue;

    // フレームレートを取得する
    UINT32 numerator{ 0 }, denominator{ 0 };
    if (FAILED(MFGetAttributeRatio(pMediaType.Get(), MF_MT_FRAME_RATE, &numerator, &denominator))) continue;

    // コーデックの文字列を取り出す
    const auto& codecName{ SubTypeToName(subType) };

    // 対応できないコーデックかフレームレートに問題があれば次へ
    if (codecName.empty() || denominator <= 0 || numerator <= 0) continue;

    // フレームレートを求める
    const double fps{ static_cast<double>(numerator) / static_cast<double>(denominator) };

    // フレームレートが 5 未満なら次へ
    if (fps < 5.0) continue;

    // 使用可能なビデオフォーマットの表示名を作成する
    std::stringstream ss;
    ss << width << " x " << height << " @ "
      << std::fixed << std::setprecision(2) << fps
      << " fps (" << codecName << ")##" << dwMediaTypeIndex;

    // 使用可能なビデオフォーマットの表示名をリストに追加する
    formatList.emplace_back(ss.str());

    // 使用可能なビデオフォーマットのリストに追加する
    availableFormats.emplace_back(width, height, numerator, denominator, subType);
  }

  // リストが空でなければ成功
  return !formatList.empty();
}

//
// Source Reader の出力フォーマットを設定し、基底クラスの frame を初期化する
//
bool CamMf::setFormat(int index)
{
  // 使用可能なフォーマットのリストから選択されたフォーマットを取得する
  VideoFormat selectedFormat{ availableFormats[index] };

  // 新しいメディアタイプを作成する
  Microsoft::WRL::ComPtr<IMFMediaType> pMediaType;
  if (FAILED(MFCreateMediaType(pMediaType.GetAddressOf()))) { close(); return false; }

  // メジャータイプにビデオを指定する
  if (FAILED(pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))) { close(); return false; }

  // 出力ビデオフォーマットとして RGB32 を要求する (Advanced Video Processing が自動変換する)
  if (FAILED(pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32))) { close(); return false; }

  // 解像度を設定する
  if (FAILED(MFSetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, selectedFormat.width, selectedFormat.height))) { close(); return false; }

  // フレームレートを設定する
  if (FAILED(MFSetAttributeRatio(pMediaType.Get(), MF_MT_FRAME_RATE, selectedFormat.fpsNum, selectedFormat.fpsDenom))) { close(); return false; }

  // Source Reader の出力メディアタイプを設定する
  if (FAILED(pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType.Get()))) { close(); return false; }

  // 基底クラスの解像度とチャンネル数を設定し、バッファサイズを設定する
  width = selectedFormat.width;
  height = selectedFormat.height;
  channels = 4;
  frame.resize(width * height * channels);
  image.resize(width * height * channels);

  // インターバルを計算する
  interval = (selectedFormat.fpsDenom != 0)
    ? (1000.0 * selectedFormat.fpsDenom / selectedFormat.fpsNum)
    : 10.0;

  return true;
}

//
// カメラを開く
//
bool CamMf::open(int device)
{
  // メディアソースを作成する
  if (!ComInitializer::activate(device, pMediaSource.ReleaseAndGetAddressOf())) return false;

  // Source Reader の属性ストアを作成する
  Microsoft::WRL::ComPtr<IMFAttributes> pAttributes;
  if (FAILED(MFCreateAttributes(pAttributes.GetAddressOf(), 4))) goto done;

  // Source Reader の属性ストアにデコード能力を設定する
  //
  //  | 設定                                                        | 効果                                      |
  //  | ----------------------------------------------------------- | ----------------------------------------- |
  //  | `MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING = TRUE`  | 色変換・デインターレース・スケーリングが  |
  //  | `MF_READWRITE_DISABLE_CONVERTERS = FALSE`                   | 自動的に行われる (もっとも簡単な自動処理) |
  //  | ----------------------------------------------------------- | ----------------------------------------- |
  //  | `MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING = FALSE` | シンプルなGPUデコードパス                 |
  //  | `MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS = TRUE`            | (カラースペース変換は限定)                |
  //  | ----------------------------------------------------------- | ----------------------------------------- |
  //  | `MF_READWRITE_DISABLE_CONVERTERS = TRUE`                    | 自動処理なし。自前でデコード／変換する    |
  //
  pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
  pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
  pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, FALSE);
  if (auto pManager = ComInitializer::getDeviceManager())
  {
    pAttributes->SetUnknown(MF_SOURCE_READER_D3D_MANAGER, pManager);
  }

  // Source Reader の解放時に Media Source をシャットダウンするようにする
  pAttributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);

  // 低遅延モードを有効にする
  pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);

  // Source Reader を作成する
  if (FAILED(MFCreateSourceReaderFromMediaSource(pMediaSource.Get(), pAttributes.Get(), pSourceReader.ReleaseAndGetAddressOf()))) goto done;

  // 使用可能なフォーマットを列挙して最初のフォーマットを選択する
  if (enumerateFormats() && setFormat(0)) return true;

done:

  // フォーマットの設定に失敗したら全てを解放して戻る
  pSourceReader.Reset();
  availableFormats.clear();
  formatList.clear();

  // フォーマットの設定に失敗したので false を返す
  return false;
}

//
// フォーマットを選択して設定する
//
bool CamMf::select(int index)
{
  // カメラが開かれていないかインデックスが範囲外なら戻る
  if (!pSourceReader || index < 0 || index >= availableFormats.size()) return false;

  // キャプチャスレッドが実行中であれば安全のために停止する
  stop();

  // 選択されたフォーマットを設定する
  return setFormat(index);
}

//
// フレームをキャプチャする
//
void CamMf::capture()
{
  // スレッドが実行可の間
  while (running)
  {
    // サンプルを読み出して
    DWORD dwStreamIndex{ 0 };
    DWORD dwStreamFlags{ 0 };
    LONGLONG llTimestamp{ 0 };
    Microsoft::WRL::ComPtr<IMFSample> pSample;
    if (FAILED(pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0, &dwStreamIndex, &dwStreamFlags, &llTimestamp, pSample.GetAddressOf()))) continue;

    // サンプルが取得できていなければ戻る
    if (!pSample) continue;

    // サンプルから取り出したメディアバッファ
    Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;

    // サンプルからメディアバッファを取得して
    if (FAILED(pSample->GetBufferByIndex(0, pBuffer.GetAddressOf()))) continue;

    // メディアバッファが取得できていれば
    if (pBuffer)
    {
      // メディアバッファから取り出したデータ
      BYTE* pData{ nullptr };

      // メディアバッファから取り出したデータの長さ
      DWORD cbDataLength{ 0 };

      // メディアバッファからフレームの情報を取得できたら
      if (SUCCEEDED(pBuffer->Lock(&pData, nullptr, &cbDataLength)) && pData)
      {
        // 一時メモリをロックして
        std::lock_guard<std::mutex> lock(mtx);

        // 基底クラスの frame バッファにデータをコピーする
        if (frame.size() < cbDataLength) frame.resize(cbDataLength);
        memcpy(frame.data(), pData, cbDataLength);

        // キャプチャしたデータを一時メモリにコピーしたら
        if (image.size() < cbDataLength) image.resize(cbDataLength);
        memcpy(image.data(), frame.data(), cbDataLength);

        // 新しいフレームがキャプチャされたことを通知して
        captured = true;

        // メディアバッファのロックを解除する
        pBuffer->Unlock();
      }
    }
  }
}

//
// カメラを閉じる
//
void CamMf::close()
{
  // Source Reader を解放する
  pSourceReader.Reset();

  // Media Source 解放する
  pMediaSource.Reset();

  // フォーマットリストをクリアする
  availableFormats.clear();
  formatList.clear();

  // 基底クラスの close を呼び出す
  Camera::close();
}
