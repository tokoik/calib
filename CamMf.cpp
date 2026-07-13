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
#include <codecapi.h>

// Microsoft Media Foundation
#pragma comment(lib, "MF.lib")
#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFuuid.lib")
#pragma comment(lib, "MFreadwrite.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

// COM ライブラリの初期化と終了を行うオブジェクト
CamMf::ComInitializer CamMf::ComInitializer::instance;

//
// メモリの解放
//
template <class T> void SafeRelease(T** ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

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
    for (DWORD i = 0; i < cSourceActivate; ++i) SafeRelease(&ppSourceActivate[i]);

    // メディアソースのリストに使ったメモリを解放して
    CoTaskMemFree(ppSourceActivate);
    ppSourceActivate = nullptr;

    // ビデオキャプチャデバイスの表示名のリストを空にする
    deviceList.clear();
  }

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
  // エラーメッセージ
  const char* message{ nullptr };

  // 検索条件を保持する属性ストア
  IMFAttributes* pAttributes{ nullptr };

  // COM ライブラリを初期化する
  if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
  {
    // COM ライブラリの初期化に失敗した
    message = "Failed to initialize COM library.";
    goto done;
  }

  // COM ライブラリの初期化に成功した
  coInitialized = true;

  // Media Foundation を起動する
  if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL)))
  {
    // Media Foundation の起動に失敗した
    message = "Failed to start Media Foundation.";
    goto done;
  }

  // Media Foundation の起動に成功した
  mfStarted = true;

  // 検索条件を保持する属性ストアを作成する
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    // 属性ストアの作成失敗した
    message = "Failed to create attribute store.";
    goto done;
  }

  // 属性ストアにビデオキャプチャデバイスの属性を設定する
  if (FAILED(pAttributes->SetGUID(
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
  {
    // ビデオキャプチャデバイスの属性の設定に失敗した
    message = "Failed to set attribute for video capture device.";
    goto done;
  }

  // メディアソースを列挙する
  if (FAILED(MFEnumDeviceSources(pAttributes,
    &ppSourceActivate, &cSourceActivate)))
  {
    // メディアソースの列挙に失敗した
    message = "Failed to enumerate media sources.";
    goto done;
  }

  // すべてのメディアソースについて
  for (DWORD i = 0; i < cSourceActivate; ++i)
  {
    // メディアソースの表示名の文字列ポインタ
    WCHAR* szFriendlyName{ nullptr };

    // メディアソースの表示名の文字数
    UINT32 cFriendlyName{ 0 };

    // メディアソースの表示名を取得する
    if (SUCCEEDED(ppSourceActivate[i]->GetAllocatedString(
      MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cFriendlyName)))
    {
      // ビデオキャプチャデバイス名を作る
      std::stringstream ss;
      ss << TCharToUtf8(szFriendlyName) << "##" << i;

      // ビデオキャプチャデバイス名をリストに追加する
      deviceList.emplace_back(ss.str());
    }

    // 表示名の文字列に使ったメモリを解放する
    CoTaskMemFree(szFriendlyName);
  }

done:

  // 属性ストアはもう使わないので解放する
  SafeRelease(&pAttributes);

  // エラーメッセージを返す
  return message;
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
  // ビデオキャプチャデバイスの表示名のリストを返す
  return getInstance().deviceList;
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
  IMFMediaType* pMediaType{ nullptr };
  for (DWORD dwMediaTypeIndex = 0;
    SUCCEEDED(pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      dwMediaTypeIndex, &pMediaType));
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
    if (FAILED(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height))) continue;

    // フレームレートを取得する
    UINT32 numerator{ 0 }, denominator{ 0 };
    if (FAILED(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &numerator, &denominator))) continue;

    // コーデックの文字列を取り出す
    const auto& codecName{ SubTypeToName(subType) };

    // 対応できないコーデックかフレームレートに問題があれば次へ
    if (codecName.empty() || denominator == 0 || numerator == 0) continue;

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

    // メディアタイプの取得に使ったメモリを解放する
    SafeRelease(&pMediaType);
  }

  // リストが空でなければ成功
  return !formatList.empty();
}

//
// 指定されたサブタイプに対応するビデオデコーダを探す
//
HRESULT CamMf::findVideoDecoder(
  const GUID& subtype,
  IMFTransform** ppDecoder,
  BOOL bAllowAsync,
  BOOL bAllowHardware,
  BOOL bAllowTranscode
) const
{
  // 結果
  HRESULT hr{ S_OK };

  // 見つかったデコーダの数
  UINT32 count{ 0 };

  // デコーダのアクティベーションオブジェクトのリスト
  IMFActivate** ppActivate{ nullptr };

  // 検索条件
  MFT_REGISTER_TYPE_INFO inputInfo{ MFMediaType_Video, subtype };
  MFT_REGISTER_TYPE_INFO outputInfo{ MFMediaType_Video, MFVideoFormat_NV12 };

  // 列挙のフラグ
  UINT32 unFlags
  {
    MFT_ENUM_FLAG_SYNCMFT
    | MFT_ENUM_FLAG_LOCALMFT
    | MFT_ENUM_FLAG_SORTANDFILTER
  };

  // 非同期デコーダも含めるなら
  if (bAllowAsync) unFlags |= MFT_ENUM_FLAG_ASYNCMFT;

  // ハードウェアデコーダも含めるなら
  if (bAllowHardware) unFlags |= MFT_ENUM_FLAG_HARDWARE;

  // トランスコード専用のデコーダも含めるなら
  if (bAllowTranscode) unFlags |= MFT_ENUM_FLAG_TRANSCODE_ONLY;

  // 条件に合う MFT のアクティベーションオブジェクトを列挙する
  hr = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER,
    unFlags, &inputInfo, &outputInfo, &ppActivate, &count);

  // デコーダが見つからなかったらエラー
  if (SUCCEEDED(hr) && count == 0) hr = MF_E_TOPO_CODEC_NOT_FOUND;

  // リストの最初のデコーダをインスタンス化する
  if (SUCCEEDED(hr)) hr = ppActivate[0]->ActivateObject(IID_PPV_ARGS(ppDecoder));

  // デコーダのアクティベーションオブジェクトのリストを解放する
  for (UINT32 i = 0; i < count; i++) ppActivate[i]->Release();

  // デコーダのアクティベーションオブジェクトのリストに使ったメモリを解放する
  CoTaskMemFree(ppActivate);

  // 結果を返す
  return hr;
}

//
// MFT のセットアップと接続を行う
//
HRESULT CamMf::setUpPipeline(IMFTransform* pTransform, const VideoFormat& format, const GUID& subType) const
{
#if defined(_DEBUG)
  std::cerr << SubTypeToName(format.subType) << " -> " << SubTypeToName(subType) << std::endl;
#endif

  // 入力と出力のメディアタイプ
  IMFMediaType* pInputType{ nullptr };
  IMFMediaType* pOutputType{ nullptr };

  // 結果
  HRESULT hr{ S_OK };

  // MFT の入力タイプの設定
  hr = MFCreateMediaType(&pInputType);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_SUBTYPE, format.subType);
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, format.width, format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, format.fpsNum, format.fpsDenom);

  // MFT に入力タイプを設定する
  if (SUCCEEDED(hr)) hr = pTransform->SetInputType(0, pInputType, 0);
  if (FAILED(hr)) goto done;

  // MFT の出力タイプの設定
  hr = MFCreateMediaType(&pOutputType);
  if (SUCCEEDED(hr)) hr = pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pOutputType->SetGUID(MF_MT_SUBTYPE, subType);
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, format.width, format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, format.fpsNum, format.fpsDenom);

  // MFT に出力タイプを設定する
  if (SUCCEEDED(hr)) hr = pTransform->SetOutputType(0, pOutputType, 0);
  if (FAILED(hr)) goto done;

  // MFT をアクティブにする
  hr = pTransform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
  if (SUCCEEDED(hr)) hr = pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
  if (SUCCEEDED(hr)) hr = pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);

done:

  // メディアタイプの取得に使ったメモリを解放する
  SafeRelease(&pInputType);
  SafeRelease(&pOutputType);

  // 結果を返す
  return hr;
}

//
// MFT を解放する
//
void CamMf::cleanUpTransform(IMFTransform** pTransform) const
{
  // MFT が作成されていれば
  if (*pTransform)
  {
    // MFT のストリーミングの終了を通知する
    (*pTransform)->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, NULL);
    (*pTransform)->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, NULL);

    // MFT を解放する
    (*pTransform)->Release();
    *pTransform = nullptr;
  }
}

//
// Source Reader の出力フォーマットを設定し、基底クラスの frame を初期化する
//
bool CamMf::setFormat(int index)
{
  // 使用可能なフォーマットのリストから選択されたフォーマットを取得する
  VideoFormat selectedFormat{ availableFormats[index] };

  // デコード方法
  enum class DecodeMethod { None = 0, Convert, Decode } decodeMethod
  {
    selectedFormat.subType == MFVideoFormat_MJPG ? DecodeMethod::Decode :
    selectedFormat.subType == MFVideoFormat_H264 ? DecodeMethod::Decode :
    selectedFormat.subType == MFVideoFormat_NV12 ? DecodeMethod::Convert :
    selectedFormat.subType == MFVideoFormat_YUY2 ? DecodeMethod::Convert :
    DecodeMethod::None
  };

  // デコーダとカラーコンバータを未設定にする
  cleanUpTransform(&pDecoder);
  cleanUpTransform(&pConverter);

  // 結果
  HRESULT hr{ S_OK };

  // 新しいメディアタイプを作成する
  IMFMediaType* pMediaType{ nullptr };
  hr = MFCreateMediaType(&pMediaType);
  if (FAILED(hr)) goto done;

  // メジャータイプにビデオを指定する
  hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (FAILED(hr)) goto done;

  // ビデオフォーマットを指定する
  hr = pMediaType->SetGUID(MF_MT_SUBTYPE, selectedFormat.subType);
  if (FAILED(hr)) goto done;

  // 解像度を設定する
  hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, selectedFormat.width, selectedFormat.height);
  if (FAILED(hr)) goto done;

  // フレームレートを設定する
  hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, selectedFormat.fpsNum, selectedFormat.fpsDenom);
  if (FAILED(hr)) goto done;

  // Source Reader の出力メディアタイプを設定する
  hr = pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType);
  if (FAILED(hr)) goto done;

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

  // デコードまたはカラー変換が必要なとき
  if (decodeMethod != DecodeMethod::None)
  {
    // デコードが必要なら
    if (decodeMethod == DecodeMethod::Decode)
    {
      // 選択されたビデオフォーマットのデコーダを探す
      hr = findVideoDecoder(selectedFormat.subType, &pDecoder);
      if (FAILED(hr)) goto done;

      // H.264 等で遅延を防ぐため、Low Latency Mode を有効にする
      {
        ICodecAPI* pCodecAPI{ nullptr };
        if (SUCCEEDED(pDecoder->QueryInterface(IID_PPV_ARGS(&pCodecAPI))))
        {
          VARIANT var;
          VariantInit(&var);
          var.vt = VT_BOOL;
          var.boolVal = VARIANT_TRUE;
          pCodecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);
          SafeRelease(&pCodecAPI);
        }
      }

      // MFT デコーダのセットアップと接続を行う
      hr = setUpPipeline(pDecoder, selectedFormat, MFVideoFormat_NV12);
      if (FAILED(hr)) goto done;

      // デコード後のビデオフォーマットは NV12 にしている
      selectedFormat.subType = MFVideoFormat_NV12;

      // デコーダの出力バッファのサイズを計算する
      MFT_OUTPUT_STREAM_INFO decoderStreamInfo{ 0 };
      if (SUCCEEDED(pDecoder->GetOutputStreamInfo(0, &decoderStreamInfo))) {}

      UINT32 alignedWidth = (width + 15) & ~15;
      UINT32 alignedHeight = (height + 15) & ~15;
      UINT32 cbDecoderCalc = (alignedWidth * alignedHeight * 3) / 2;
      UINT32 cbDecoder = (decoderStreamInfo.cbSize > cbDecoderCalc) ? decoderStreamInfo.cbSize : cbDecoderCalc;
      DWORD alignmentDecoder = (decoderStreamInfo.cbAlignment > 0) ? (decoderStreamInfo.cbAlignment - 1) : 63;

      // デコーダの出力バッファを作成する
      hr = MFCreateAlignedMemoryBuffer(cbDecoder, alignmentDecoder, &pDecoderBuffer);
      if (FAILED(hr)) goto done;
    }

    // MFT をインスタンス化する
    hr = CoCreateInstance(CLSID_CColorConvertDMO, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pConverter));
    if (FAILED(hr)) goto done;

    // MFT カラーコンバータのセットアップと接続を行う
    hr = setUpPipeline(pConverter, selectedFormat, MFVideoFormat_RGB32);
    if (FAILED(hr)) goto done;

    // カラーコンバータの出力バッファのサイズを計算する
    MFT_OUTPUT_STREAM_INFO converterStreamInfo{ 0 };
    if (SUCCEEDED(pConverter->GetOutputStreamInfo(0, &converterStreamInfo))) {}

    UINT32 cbConverterCalc{ 0 };
    MFCalculateImageSize(MFVideoFormat_RGB32, width, height, &cbConverterCalc);
    UINT32 cbConverter = (converterStreamInfo.cbSize > cbConverterCalc) ? converterStreamInfo.cbSize : cbConverterCalc;
    DWORD alignmentConverter = (converterStreamInfo.cbAlignment > 0) ? (converterStreamInfo.cbAlignment - 1) : 63;

    // カラー変換用の出力バッファを作成する
    hr = MFCreateAlignedMemoryBuffer(cbConverter, alignmentConverter, &pConverterBuffer);
    if (FAILED(hr)) goto done;
  }

done:

  // メディアタイプはもう使わないので解放する
  SafeRelease(&pMediaType);

  // 成功したら true を返す
  if (SUCCEEDED(hr)) return true;

  // 失敗したときはカメラを閉じて
  close();

  // false を返す
  return false;
}

//
// カメラを開く
//
bool CamMf::open(int device, bool setupFormat)
{
  // メディアソースを作成する
  if (!ComInitializer::activate(device, &pMediaSource)) return false;

  // 結果
  HRESULT hr{ S_OK };

  // Source Reader の属性ストアを作成する
  IMFAttributes* pAttributes{ nullptr };
  hr = MFCreateAttributes(&pAttributes, 1);
  if (FAILED(hr)) goto done;

  // Source Reader の属性ストアにデコード能力を設定する
  pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
  pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, FALSE);

  // Source Reader の解放時に Media Source をシャットダウンするようにする
  pAttributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);

  // Source Reader を作成する
  hr = MFCreateSourceReaderFromMediaSource(pMediaSource, pAttributes, &pSourceReader);
  if (FAILED(hr)) goto done;

  // 属性ストアはもう必要ないので解放する
  SafeRelease(&pAttributes);

  // 使用可能なフォーマットを列挙して最初のフォーマットを選択する
  if (enumerateFormats())
  {
    // 要求された場合のみ最初のフォーマットを設定する
    if (!setupFormat || setFormat(0)) return true;
  }

done:

  // フォーマットの設定に失敗したら全てを解放して戻る
  SafeRelease(&pSourceReader);
  SafeRelease(&pAttributes);
  availableFormats.clear();
  formatList.clear();

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
    IMFSample* pSample{ nullptr };
    if (FAILED(pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0, &dwStreamIndex, &dwStreamFlags, &llTimestamp, &pSample))) continue;

    // サンプルが取得できていなければ戻る
    if (!pSample) continue;

    // サンプルから取り出したメディアバッファ
    IMFMediaBuffer* pBuffer{ nullptr };

    // デコーダの出力フレームを保持するサンプル
    IMFSample* pDecodedSample{ nullptr };

    // カラーコンバータの出力フレームを保持するサンプル
    IMFSample* pConvertedSample{ nullptr };

    // ProcessOutput の呼び出しの状態
    DWORD dwStatus{ 0 };

    // デコーダが設定されていれば (MJPG か H264 の場合)
    if (pDecoder)
    {
      // サンプルをデコーダに渡す
      HRESULT hr{ pDecoder->ProcessInput(0, pSample, 0) };

      // デコーダへの入力に失敗したら
      if (FAILED(hr))
      {
#if defined(_DEBUG)
        if (hr != MF_E_NOTACCEPTING)
        {
          std::cerr << "Decoder process input failed: ";
          switch (hr)
          {
          case E_INVALIDARG:
            std::cerr << "Invalid argument." << std::endl; break;
          case E_UNEXPECTED:
            std::cerr << "Unexpected error." << std::endl; break;
          case E_FAIL:
            std::cerr << "Unspecified error." << std::endl; break;
          case MF_E_INVALIDSTREAMNUMBER:
            std::cerr << "Invalid stream number." << std::endl; break;
          case MF_E_NO_SAMPLE_DURATION:
            std::cerr << "No sample duration." << std::endl; break;
          case MF_E_NO_SAMPLE_TIMESTAMP:
            std::cerr << "No sample timestamp." << std::endl; break;
          case MF_E_NOTACCEPTING:
            std::cerr << "Not accepting input." << std::endl; break;
          case MF_E_TRANSFORM_TYPE_NOT_SET:
            std::cerr << "Transform type not set." << std::endl; break;
          case MF_E_UNSUPPORTED_D3D_TYPE:
            std::cerr << "Unsupported D3D type." << std::endl; break;
          default:
            std::cerr << "Code: " << std::hex << hr << std::endl; break;
          }
        }
#endif
        goto done;
      }

      // デコーダの出力ストリーム情報を取得し、誰がサンプルを提供するべきかを判定する
      MFT_OUTPUT_STREAM_INFO streamInfo{};
      bool mftProvidesSamples{ false };
      if (SUCCEEDED(pDecoder->GetOutputStreamInfo(0, &streamInfo)))
      {
        mftProvidesSamples = (streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
      }

      // デコーダから出力サンプルを取り出す
      IMFSample* pLatestDecodedSample{ nullptr };
      bool hasOutput{ false };

      while (true)
      {
        if (mftProvidesSamples)
        {
          // MFT が自前でサンプルを割り当てるので nullptr を渡す
          pDecodedSample = nullptr;
        }
        else
        {
          // 呼び出し側がサンプルを提供する
          hr = MFCreateSample(&pDecodedSample);
          if (FAILED(hr)) goto done;

          // クラスメンバの pDecoderBuffer が有効であることを確認
          if (!pDecoderBuffer)
          {
            MFT_OUTPUT_STREAM_INFO streamInfo{ 0 };
            if (SUCCEEDED(pDecoder->GetOutputStreamInfo(0, &streamInfo))) {}
            
            UINT32 alignedWidth = (width + 15) & ~15;
            UINT32 alignedHeight = (height + 15) & ~15;
            UINT32 cbDecoderCalc = (alignedWidth * alignedHeight * 3) / 2;
            UINT32 cbDecoder = (streamInfo.cbSize > cbDecoderCalc) ? streamInfo.cbSize : cbDecoderCalc;
            DWORD alignmentDecoder = (streamInfo.cbAlignment > 0) ? (streamInfo.cbAlignment - 1) : 63;

            hr = MFCreateAlignedMemoryBuffer(cbDecoder, alignmentDecoder, &pDecoderBuffer);
            if (FAILED(hr))
            {
              SafeRelease(&pDecodedSample);
              goto done;
            }
          }

          // バッファの有効データ長を 0 に設定して、MFT が先頭から書き込めるようにする
          pDecoderBuffer->SetCurrentLength(0);

          hr = pDecodedSample->AddBuffer(pDecoderBuffer);
          if (FAILED(hr))
          {
            SafeRelease(&pDecodedSample);
            goto done;
          }
        }

        MFT_OUTPUT_DATA_BUFFER decodedBuffer{ 0, pDecodedSample, 0, nullptr };
        hr = pDecoder->ProcessOutput(0, 1, &decodedBuffer, &dwStatus);

        // ストリームのフォーマットが変化した場合の処理
        if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
        {
#if defined(_DEBUG)
          std::cerr << "Transform stream change." << std::endl;
#endif
          // 1. 利用可能な出力タイプから NV12 を探索・選択する
          IMFMediaType* pNewOutputType{ nullptr };
          GUID subtype{ 0 };
          DWORD dwTypeIndex = 0;
          bool foundNV12 = false;
          hr = S_OK; // hr を S_OK にリセット
          while (SUCCEEDED(pDecoder->GetOutputAvailableType(0, dwTypeIndex++, &pNewOutputType)))
          {
            GUID subType{};
            if (SUCCEEDED(pNewOutputType->GetGUID(MF_MT_SUBTYPE, &subType)) && subType == MFVideoFormat_NV12)
            {
              foundNV12 = true;
              subtype = subType;
#if defined(_DEBUG)
              std::cerr << "Found NV12 output type at index " << (dwTypeIndex - 1) << std::endl;
#endif
              break;
            }
            SafeRelease(&pNewOutputType);
          }

          // 見つからなかった場合はインデックス 0 を取得してサブタイプを NV12 に上書きする
          if (!foundNV12)
          {
#if defined(_DEBUG)
            std::cerr << "NV12 output type not found. Forcing fallback to index 0." << std::endl;
#endif
            hr = pDecoder->GetOutputAvailableType(0, 0, &pNewOutputType);
            if (SUCCEEDED(hr))
            {
              hr = pNewOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
              subtype = MFVideoFormat_NV12;
            }
          }

          // 2. 現在の入力メディアタイプから解像度などの属性をコピーする
          if (SUCCEEDED(hr) && pNewOutputType)
          {
            IMFMediaType* pInputType{ nullptr };
            if (SUCCEEDED(pDecoder->GetInputCurrentType(0, &pInputType)))
            {
              UINT32 w{ 0 }, h{ 0 };
              if (SUCCEEDED(MFGetAttributeSize(pInputType, MF_MT_FRAME_SIZE, &w, &h)))
              {
                MFSetAttributeSize(pNewOutputType, MF_MT_FRAME_SIZE, w, h);
              }

              UINT32 fpsNum{ 0 }, fpsDenom{ 0 };
              if (SUCCEEDED(MFGetAttributeRatio(pInputType, MF_MT_FRAME_RATE, &fpsNum, &fpsDenom)))
              {
                MFSetAttributeRatio(pNewOutputType, MF_MT_FRAME_RATE, fpsNum, fpsDenom);
              }

              UINT32 aspectNum{ 0 }, aspectDenom{ 0 };
              if (SUCCEEDED(MFGetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, &aspectNum, &aspectDenom)))
              {
                MFSetAttributeRatio(pNewOutputType, MF_MT_PIXEL_ASPECT_RATIO, aspectNum, aspectDenom);
              }

              UINT32 interlace{ 0 };
              if (SUCCEEDED(pInputType->GetUINT32(MF_MT_INTERLACE_MODE, &interlace)))
              {
                pNewOutputType->SetUINT32(MF_MT_INTERLACE_MODE, interlace);
              }

              SafeRelease(&pInputType);
            }

            // 新しい出力メディアタイプを設定する
            hr = pDecoder->SetOutputType(0, pNewOutputType, 0);
#if defined(_DEBUG)
            if (FAILED(hr)) std::cerr << "pDecoder->SetOutputType failed: " << std::hex << hr << std::endl;
            else std::cerr << "pDecoder->SetOutputType succeeded." << std::endl;
#endif
          }
          else if (SUCCEEDED(hr))
          {
            hr = E_FAIL;
#if defined(_DEBUG)
            std::cerr << "No output type available to set!" << std::endl;
#endif
          }

          // 3. 設定された出力タイプから属性を取得してバッファとカラーコンバータを更新する
          if (SUCCEEDED(hr) && pNewOutputType)
          {
            UINT32 newWidth{ 0 }, newHeight{ 0 };
            if (SUCCEEDED(MFGetAttributeSize(pNewOutputType, MF_MT_FRAME_SIZE, &newWidth, &newHeight)) && newWidth > 0 && newHeight > 0)
            {
              width = newWidth;
              height = newHeight;
            }

            UINT32 fpsNum{ 0 }, fpsDenom{ 0 };
            MFGetAttributeRatio(pNewOutputType, MF_MT_FRAME_RATE, &fpsNum, &fpsDenom);

            // デコーダの出力バッファ要件の取得と再作成
            MFT_OUTPUT_STREAM_INFO streamInfo{ 0 };
            if (SUCCEEDED(pDecoder->GetOutputStreamInfo(0, &streamInfo))) {}
            
            UINT32 alignedWidth = (width + 15) & ~15;
            UINT32 alignedHeight = (height + 15) & ~15;
            UINT32 cbDecoderCalc = (alignedWidth * alignedHeight * 3) / 2;
            UINT32 cbDecoder = (streamInfo.cbSize > cbDecoderCalc) ? streamInfo.cbSize : cbDecoderCalc;
            DWORD alignmentDecoder = (streamInfo.cbAlignment > 0) ? (streamInfo.cbAlignment - 1) : 63;

            SafeRelease(&pDecoderBuffer);
            hr = MFCreateAlignedMemoryBuffer(cbDecoder, alignmentDecoder, &pDecoderBuffer);

            // カラーコンバータの再設定
            if (SUCCEEDED(hr) && pConverter)
            {
              VideoFormat newFormat{ static_cast<UINT32>(width), static_cast<UINT32>(height), fpsNum, fpsDenom, subtype };

              // カラーコンバータのセットアップ
              hr = setUpPipeline(pConverter, newFormat, MFVideoFormat_RGB32);

              if (SUCCEEDED(hr))
              {
                MFT_OUTPUT_STREAM_INFO convStreamInfo{ 0 };
                if (SUCCEEDED(pConverter->GetOutputStreamInfo(0, &convStreamInfo))) {}
                
                UINT32 cbConverterCalc{ 0 };
                MFCalculateImageSize(MFVideoFormat_RGB32, width, height, &cbConverterCalc);
                UINT32 cbConverter = (convStreamInfo.cbSize > cbConverterCalc) ? convStreamInfo.cbSize : cbConverterCalc;
                DWORD alignmentConverter = (convStreamInfo.cbAlignment > 0) ? (convStreamInfo.cbAlignment - 1) : 63;

                SafeRelease(&pConverterBuffer);
                hr = MFCreateAlignedMemoryBuffer(cbConverter, alignmentConverter, &pConverterBuffer);
              }
            }

            // 基底クラスのバッファリサイズ
            if (SUCCEEDED(hr))
            {
              frame.resize(width * height * channels);
              image.resize(width * height * channels);
            }
          }

          // 出力サンプルとイベントを破棄する
          if (!mftProvidesSamples) SafeRelease(&pDecodedSample);
          else SafeRelease(&decodedBuffer.pSample);
          SafeRelease(&decodedBuffer.pEvents);

          // リソースの解放
          SafeRelease(&pNewOutputType);

          if (FAILED(hr))
          {
#if defined(_DEBUG)
            std::cerr << "Failed to handle stream change: " << std::hex << hr << std::endl;
#endif
            goto done;
          }

          // ストリーム情報を再取得する（ストリームチェンジによってフラグが変わる可能性があるため）
          if (SUCCEEDED(pDecoder->GetOutputStreamInfo(0, &streamInfo)))
          {
            mftProvidesSamples = (streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
#if defined(_DEBUG)
            std::cerr << "After stream change, mftProvidesSamples = " << (mftProvidesSamples ? "true" : "false") << ", cbSize = " << streamInfo.cbSize << std::endl;
#endif
          }

#if defined(_DEBUG)
          std::cerr << "Stream change handled successfully, retrying ProcessOutput." << std::endl;
#endif
          // ストリーム変更を行ったので、今回の ProcessOutput はやり直す
          continue;
        }

        // デコーダがさらなる入力を要求している場合 (正常動作)
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
          if (!mftProvidesSamples) SafeRelease(&pDecodedSample);
          else SafeRelease(&decodedBuffer.pSample);
          break;
        }

        // その他のエラー
        if (FAILED(hr))
        {
#if defined(_DEBUG)
          std::cerr << "Decoder process output failed (loop bottom): " << std::hex << hr << std::endl;
#endif
          if (!mftProvidesSamples) SafeRelease(&pDecodedSample);
          else SafeRelease(&decodedBuffer.pSample);
          SafeRelease(&decodedBuffer.pEvents);
          goto done;
        }

        // デコード成功！
        SafeRelease(&pLatestDecodedSample);
        if (mftProvidesSamples)
        {
          pLatestDecodedSample = decodedBuffer.pSample;
        }
        else
        {
          pLatestDecodedSample = pDecodedSample;
        }
        pDecodedSample = nullptr;
        SafeRelease(&decodedBuffer.pEvents);
        hasOutput = true;
        break;
      }

      if (hasOutput && pLatestDecodedSample)
      {
        // 元のサンプルはもう使わないので解放する
        pSample->Release();

        // デコードに成功したらデコードされたサンプルを使う
        pSample = pLatestDecodedSample;
        pLatestDecodedSample = nullptr;
      }
      else
      {
        // 今回の入力ではデコード完了フレームが得られなかったため、
        // 入力サンプルを解放して次のフレームの読み込みに進む
        pSample->Release();
        pSample = nullptr;
        continue;
      }
    }

    // カラーコンバータが設定されていれば (NV12 か YUY2 か MJPG が H264 の場合)
    if (pConverter)
    {
      // サンプルをカラーコンバータに渡す
      HRESULT hr{ pConverter->ProcessInput(0, pSample, 0) };
      if (FAILED(hr)) goto done;

      // カラーコンバータの出力サンプルを作成する
      hr = MFCreateSample(&pConvertedSample);
      if (FAILED(hr)) goto done;

      // バッファの長さを 0 に設定して書き込み可能にする
      pConverterBuffer->SetCurrentLength(0);

      // カラーコンバータの出力サンプルにバッファを追加する
      hr = pConvertedSample->AddBuffer(pConverterBuffer);
      if (FAILED(hr)) goto done;

      // カラーコンバータの出力バッファ
      MFT_OUTPUT_DATA_BUFFER convertedBuffer{ 0, pConvertedSample, 0, nullptr };

      // カラー変換処理を実行
      hr = pConverter->ProcessOutput(0, 1, &convertedBuffer, &dwStatus);
      if (FAILED(hr)) goto done;

      // 元のサンプルはもう使わないので解放する
      pSample->Release();

      // コンバートに成功したらコンバートされたサンプルを使う
      pSample = pConvertedSample;
      pConvertedSample = nullptr;
    }

    // サンプルからメディアバッファを取得して
    if (FAILED(pSample->GetBufferByIndex(0, &pBuffer))) goto done;

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
        std::lock_guard<std::mutex> lock{ mtx };

        // キャプチャしたデータを一時メモリにコピーしたら
        if (image.size() < cbDataLength) image.resize(cbDataLength);
        memcpy(image.data(), pData, cbDataLength);

        // 新しいフレームがキャプチャされたことを通知して
        captured = true;

        // メディアバッファのロックを解除する
        pBuffer->Unlock();
      }

      // メディアバッファを解放する
      pBuffer->Release();
      pBuffer = nullptr;
    }

  done:

    // デコーダの出力サンプルとバッファを解放する
    if (pDecodedSample) pDecodedSample->Release();

    // カラーコンバータの出力サンプルバッファを解放する
    if (pConvertedSample) pConvertedSample->Release();

    // サンプルを解放する
    pSample->Release();
    pSample = nullptr;

    // CPU負荷軽減のための短いスリープ
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

//
// キャプチャスレッドを停止する
//
void CamMf::stop()
{
  // キャプチャスレッドが実行中なら
  if (running)
  {
    // キャプチャスレッドのループを止めて
    running = false;

    // ReadSample のブロッキングを解除する
    if (pSourceReader)
    {
      pSourceReader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    }

    // 合流する
    if (thr.joinable())
    {
      thr.join();
    }
  }
}

//
// カメラを閉じる
//
void CamMf::close()
{
  // MFT デコーダを解放する
  cleanUpTransform(&pDecoder);

  // MFT カラーコンバータを解放する
  cleanUpTransform(&pConverter);

  // MFT デコーダのバッファを解放する
  SafeRelease(&pDecoderBuffer);

  // MFT カラーコンバータのバッファを解放する
  SafeRelease(&pConverterBuffer);

  // Source Reader を解放する
  SafeRelease(&pSourceReader);

  // Media Source 解放する
  SafeRelease(&pMediaSource);

  // フォーマットリストをクリアする
  availableFormats.clear();
  formatList.clear();

  // 基底クラスの close を呼び出す
  Camera::close();
}
