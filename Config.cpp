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

#if defined(_WIN32)
// Microsoft Media Foundation によるキャプチャ
#include "CamMf.h"
#endif

#if !defined(_DEBUG) && defined(_WIN32)
// appData のパスを得るときに使う
#include <shlobj_core.h>
#endif

//
// コンストラクタ
//
Config::Config(const std::string& filename)
#if defined(_WIN32)
  : deviceList{ CamMf::getDeviceList() }
#endif
{
  // 構成ファイルの保存場所を決定する
#if defined(_DEBUG)
  const auto path{ Utf8ToTChar(filename) };
#else
#  if defined(_WIN32)
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

  initialized = true;

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

  // ルートが JSON オブジェクトでなければ、既存構成を変更せず失敗とする
  if (!value.is<picojson::object>()) return false;

  // 構成内容の取り出し
  const auto& object{ value.get<picojson::object>() };

  // オブジェクトが空だったらエラー
  if (object.empty()) return false;

  // 読み込みに失敗しても現在の構成を壊さないよう、一時領域へ読み込む
  auto loadedWindowSize{ windowSize };
  auto loadedBackground{ background };
  auto loadedSettings{ settings };
  auto loadedInitialImage{ initialImage };
  std::vector<Preference> loadedPreferences;

  // ウィンドウのサイズ
  getValue(object, "size", loadedWindowSize);

  // ウィンドウの背景色
  getValue(object, "background", loadedBackground);

  // メッシュのサンプル数
  getValue(object, "samples", loadedSettings.samples);

  // キャプチャデバイスの姿勢
  getValue(object, "pose", loadedSettings.euler);

  // 描画時の焦点距離
  getValue(object, "focal", loadedSettings.focal);

  // 描画時の焦点距離の範囲
  getValue(object, "range", loadedSettings.focalRange);

  // ArUco Marker の辞書名
  getString(object, "dictionary", loadedSettings.dictionaryName);

  // 初期表示画像
  getString(object, "initial", loadedInitialImage);

  // キャプチャデバイスの構成を探す
  const auto& camera{ object.find("camera") };

  // キャプチャデバイスの構成の配列が見つからなければエラー
  if (camera == object.end() || !camera->second.is<picojson::array>()) return false;

  // 配列の個々の要素について
  for (const auto& value : camera->second.get<picojson::array>())
  {
    // 配列中に投影方式以外の値があれば、不完全な構成を適用せず失敗とする
    if (!value.is<picojson::object>()) return false;

    // キャプチャデバイスの構成のオブジェクトを取り出す
    const auto& preference{ value.get<picojson::object>() };

    // キャプチャデバイスの構成をリストに追加
    loadedPreferences.emplace_back(preference);
  }

  // キャプチャデバイスの構成が一つも読み取れなければエラー
  if (loadedPreferences.empty()) return false;

  // 実行時の再読み込みでは、新しい構成のシェーダも準備する
  if (initialized)
  {
    // OpenGL 初期化後の再読み込みなので、置換前に全投影方式を描画可能にする
    for (auto& preference : loadedPreferences) preference.buildShader();
  }

  // 読み込みと検証がすべて成功してから現在の構成を置き換える
  windowSize = loadedWindowSize;
  background = loadedBackground;
  settings = loadedSettings;
  initialImage = std::move(loadedInitialImage);
  preferenceList.swap(loadedPreferences);

  if (initialized)
  {
    // 実行中の再読み込みでは、新しい背景色を現在の OpenGL 状態にも反映する
    glClearColor(background[0], background[1], background[2], background[3]);
  }

  return true;
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

  // 構成データの書き込み成功
  return true;
}
