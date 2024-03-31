///
/// 構成データクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Config.h"

// 標準ライブラリ
#include <fstream>

//
// デフォルトのビデオデバイスの一覧を作る
//
void getAnyList(std::vector<std::string>& list)
{
  list.emplace_back("(any)");
  list.emplace_back("Device 1");
  list.emplace_back("Device 2");
  list.emplace_back("Device 3");
  list.emplace_back("Device 4");
  list.emplace_back("Device 5");
  list.emplace_back("Device 6");
  list.emplace_back("Device 7");
}

#if defined(_MSC_VER)
//
// Direct Show のビデオデバイスの一覧を作る
//
//   https://docs.microsoft.com/en-us/windows/win32/directshow/selecting-a-capture-device
//   https://www.geekpage.jp/programming/directshow/list-capture-device.php
//   http://www.antillia.com/sol9.2.0/4.25.html
//
#include <dshow.h>
#pragma comment(lib, "strmiids")

void getDirectShowList(std::vector<std::string>& list)
{
  // COM を開く
  HRESULT hr{ CoInitialize(nullptr) };
  if (FAILED(hr)) return;

  // デバイスの列挙子を作成する
  ICreateDevEnum* pDevEnum{ nullptr };
  hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
    reinterpret_cast<PVOID*>(&pDevEnum));

  // 列挙子が作れなかったら戻る
  if (FAILED(hr)) return;

  // デバイスの列挙子の異名を作成する
  IEnumMoniker* pEnumMoniker{ nullptr };
  hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);

  // デバイスの列挙子はもういらないので開放する
  pDevEnum->Release();

  // 列挙子の異名が作れなかったら戻る
  if (FAILED(hr)) return;

  // 列挙子の異名が一つもなければ戻る
  if (!pEnumMoniker) return;

  // 列挙子の異名の取り出し先
  IMoniker* pMoniker{ nullptr };

  // 列挙子の異名を一つずつ取り出す
  while (pEnumMoniker->Next(1, &pMoniker, nullptr) == S_OK)
  {
    // プロパティバッグの場所を取り出す
    IPropertyBag* pPropertyBag;
    hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<void**>(&pPropertyBag));

    // プロパティバッグの場所が取り出せなかった次に行く
    if (FAILED(hr))
    {
      pMoniker->Release();
      continue;
    }

    // FriendlyName の格納場所
    VARIANT friendlyName;
    VariantInit(&friendlyName);

    // FriendlyName を取得する
    hr = pPropertyBag->Read(L"FriendlyName", &friendlyName, 0);

    // Friendly Name を保存する
    list.emplace_back(FAILED(hr) ? "Unknown" : TCharToUtf8(friendlyName.bstrVal));

    // FriendlyName の格納場所を消去する
    VariantClear(&friendlyName);

    // プロパティバッグを解放する
    pMoniker->Release();
    pPropertyBag->Release();
  }

  // デバイスの列挙子の異名を開放する
  pEnumMoniker->Release();

  // COM を閉じる
  CoUninitialize();
}

//
// Media Foundation のビデオデバイスの一覧を作る
//
//   https://docs.microsoft.com/ja-jp/windows/win32/medfound/audio-video-capture-in-media-foundation
//
#include <Mfidl.h>
#include <Mfapi.h>
#include <Mferror.h>
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")

void getMediaFoundationList(std::vector<std::string>& list)
{
  // Create an attribute store to hold the search criteria.
  IMFAttributes* pConfig{ NULL };
  HRESULT hr{ MFCreateAttributes(&pConfig, 1) };

  // Request video capture devices.
  if (SUCCEEDED(hr))
  {
    hr = pConfig->SetGUID(
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
      MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
  }

  // Enumerate the devices,
  IMFActivate** ppDevices{ NULL };
  UINT32 count{ 0 };
  if (SUCCEEDED(hr))
  {
    hr = MFEnumDeviceSources(pConfig, &ppDevices, &count);
  }

  for (DWORD i = 0; i < count; i++)
  {
    // Try to get the display name.
    WCHAR* szFriendlyName{ NULL };
    UINT32 cchName{ 0 };
    HRESULT hr{
      ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
      &szFriendlyName, &cchName)
    };

    if (SUCCEEDED(hr))
    {
      list.emplace_back(TCharToUtf8(szFriendlyName));
    }
    CoTaskMemFree(szFriendlyName);
  }

  for (DWORD i = 0; i < count; i++)
  {
    ppDevices[i]->Release();
  }
  CoTaskMemFree(ppDevices);
}

// appData のパスを得るときに使う
#include <shlobj_core.h>

#else

#  if defined(__APPLE__)
//
// macOS のビデオデバイスの一覧を作る
//
void getAvFoundationList(std::vector<std::string>& list)
{
  // TODO: macOS の AV Foundation のビデオデバイスの一覧を得る方法に書き換える
  getAnyList(list);
}
#  endif

// パスワードのエントリからホームディレクトリの場所を得るときに使う
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#endif

//
// コンストラクタ
//
Config::Config(const std::string& filename)
  : title{ PROJECT_NAME }
  , windowSize{ 1280, 720 }
  , background{ 0.2f, 0.3f, 0.4f, 1.0f }
  , settings{ "DICT_4X4_50" }
  , menuFont{ "Mplus1-Regular.ttf" }
  , menuFontSize{ 20.0f }
{
  // バックエンドごとのキャプチャデバイスの一覧を初期化する
  for (auto& [api, name] : backendList)
  {
    // バックエンドごとに空のリストを追加する
    deviceList.emplace(api, std::vector<std::string>());
  }

  // キャプチャデバイスの一覧を作る
  getAnyList(deviceList.at(cv::CAP_ANY));
#if defined(_MSC_VER)
  getDirectShowList(deviceList.at(cv::CAP_DSHOW));
  getMediaFoundationList(deviceList.at(cv::CAP_MSMF));
#elif defined(__APPLE__)
  getAvFoundationList(deviceList.at(cv::CAP_AVFOUNDATION));
#endif

  // 構成ファイルの保存場所を決定する
#if defined(_DEBUG)
  const auto path{ Utf8ToTChar(filename) };
#else
#  if defined(_MSC_VER)
  // 構成ファイルの保存先のパス
  wchar_t appDataPath[MAX_PATH];

  // AppData のバスを取得する
  SHGetSpecialFolderPathW(NULL, appDataPath, CSIDL_APPDATA, 0);

  // AppData のパスに構成ファイル名を連結する
  const auto path{ CString(appDataPath) + TEXT("\\") + Utf8ToTChar(filename) };
#  else
  // パスワードファイルのエントリを取得する
  const passwd* pw{ getpwuid(getuid()) };

  // ホームディレクトリのパスに構成ファイル名を連結する
  const auto path{ std::string(pw->pw_dir) + "/." + filename };
#  endif
#endif

  // 構成ファイルを読み込む
  if (!load(path))
  {
    // デフォルトの設定を追加する
    if (!load(Utf8ToTChar(filename))) preferenceList.emplace_back();

    // デフォルトの設定を入れた構成ファイルを作成する
    save(path);
  }
}

//
// デストラクタ
//
Config::~Config()
{
}

//
// 構成データを初期化する
//
void Config::initialize()
{
  // 構成リストのすべて構成についてシェーダをビルドする
  for (auto& preference : preferenceList) preference.buildShader();

  // 背景色を設定する
  glClearColor(background[0], background[1], background[2], background[3]);
}

//
// 構成ファイルを読み込む
//
bool Config::load(const pathString& filename)
{
  // 構成ファイルの読み込み
  std::ifstream json{ filename };
  if (!json) return false;

  // JSON の読み込み
  picojson::value value;
  json >> value;
  json.close();

  // 構成内容の取り出し
  const auto& object{ value.get<picojson::object>() };

  // オブジェクトが空だったらエラー
  if (object.empty()) return false;

  // ウィンドウのサイズ
  getValue(object, "size", windowSize);

  // ウィンドウの背景色
  getValue(object, "background", background);

  // メッシュのサンプル数
  getValue(object, "samples", settings.samples);

  // キャプチャデバイスの姿勢
  getValue(object, "pose", settings.euler);

  // 描画時の焦点距離
  getValue(object, "focal", settings.focal);

  // 描画時の焦点距離の範囲
  getValue(object, "range", settings.focalRange);

  // ArUco Marker の辞書名
  getString(object, "dictionary", settings.dictionaryName);

  // 初期表示画像
  getString(object, "initial", initialImage);

  // FFmpeg のリスト
  getString(object, "ffmpeg", deviceList[cv::CAP_FFMPEG]);

  // GStreamer のリスト
  getString(object, "gstreamer", deviceList[cv::CAP_GSTREAMER]);

  // キャプチャデバイスの構成を探す
  const auto& camera{ object.find("camera") };

  // キャプチャデバイスの構成の配列が見つからなければエラー
  if (camera == object.end() || !camera->second.is<picojson::array>()) return false;

  // 配列の個々の要素について
  for (const auto& value : camera->second.get<picojson::array>())
  {
    // キャプチャデバイスの構成のオブジェクトを取り出す
    const auto& preference{ value.get<picojson::object>() };

    // キャプチャデバイスの構成をリストに追加
    preferenceList.emplace_back(preference);
  }

  // キャプチャデバイスの構成が一つも読み取れなければエラー
  return !preferenceList.empty();
}

//
// 構成ファイルを保存する
//
bool Config::save(const pathString& filename) const
{
  // 設定値を保存する
  std::ofstream config{ filename };
  if (!config) return false;

  // オブジェクト
  picojson::object object;

  // ウィンドウのサイズ
  setValue(object, "size", windowSize);

  // ウィンドウの背景色
  setValue(object, "background", background);

  // メッシュのサンプル数
  setValue(object, "samples", settings.samples);

  // キャプチャデバイスの姿勢
  setValue(object, "pose", settings.euler);

  // 描画時の焦点距離
  setValue(object, "focal", settings.focal);

  // 描画時の焦点距離の範囲
  setValue(object, "range", settings.focalRange);

  // ArUco Marker 辞書名
  setString(object, "dictionary", settings.dictionaryName);

  // 初期表示画像
  setString(object, "initial", initialImage);

  // FFmpeg のリスト
  setString(object, "ffmpeg", deviceList.at(cv::CAP_FFMPEG));

  // GStreamer のリスト
  setString(object, "gstreamer", deviceList.at(cv::CAP_GSTREAMER));

  // 配列
  picojson::array array;

  // 構成リストのすべて構成について
  for (const auto& preference : preferenceList)
  {
    // 構成の要素
    picojson::object camera;

    // 構成を JSON オブジェクトに格納する
    preference.setPreference(camera);

    // 要素を picojson::array に追加する
    array.emplace_back(picojson::value(camera));
  }

  // オブジェクトに追加する
  object.emplace("camera", array);

  // 構成をシリアライズして保存
  picojson::value v{ object };
  config << v.serialize(true);
  config.close();

  // 構成データの読み込み成功
  return true;
}

// バックエンドのリスト
const std::map<cv::VideoCaptureAPIs, const char*> Config::backendList
{
#if defined(_MSC_VER)
  { cv::CAP_MSMF, "Media Foundation" },
  { cv::CAP_DSHOW, "Direct Show" },
#elif defined(__APPLE__)
  { cv::CAP_AVFOUNDATION, "AV Foundation" },
#endif
  { cv::CAP_GSTREAMER, "GStreamer" },
  { cv::CAP_ANY, "(any)" },
  { cv::CAP_FFMPEG, u8"動画ファイル履歴" }
};

// コーデックのリスト
const std::vector<const char*> Config::codecList
{
  "(any)",
  "MJPG",
  "H264",
  "BGR3",
  "YUY2",
  "I420",
  "NV12"
};

// キャプチャデバイスのリスト
std::map <cv::VideoCaptureAPIs, std::vector<std::string>> Config::deviceList;

// 初期表示の画像ファイル名
std::string Config::initialImage{ "initial.jpg" };
