///
/// メニューの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Menu.h"

// ImGui
#if !defined(__ANDROID__)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

// ファイルダイアログ
#if !defined(__ANDROID__)
#include "nfd.h"

namespace
{
  // JSON ファイル名のフィルタ
  constexpr nfdfilteritem_t jsonFilter[]{ { "JSON", "json" } };

  // 画像ファイル名のフィルタ
  constexpr nfdfilteritem_t imageFilter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };

  // 動画ファイル名のフィルタ
  constexpr nfdfilteritem_t movieFilter[]{ "Movies", "mp4,m4v,mpg,mov,avi,ogg,mkv" };
}
#else
#include <filesystem>
#endif

// 標準ライブラリ
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <limits>
#include <algorithm>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
namespace
{
  //
  // 解像度の表示文字列 (例: "1280 x 720") から幅と高さを取り出す
  //
  bool parseResolution(const std::string& resolution, int& width, int& height)
  {
    return std::sscanf(resolution.c_str(), "%d x %d", &width, &height) == 2;
  }

  //
  // フォーマットが未選択のときに使う既定のフォーマットを選ぶ
  //
  // 1280 x 720 があればそれを選び、なければ画素数が 1280 x 720 に近いものを選ぶ。
  // 1920 x 1080 を超える解像度はデコード負荷が大きいため優先度を下げる。
  //
  std::vector<CaptureFormat>::const_iterator findDefaultFormat(const std::vector<CaptureFormat>& formats)
  {
    constexpr long long targetArea{ 1280LL * 720LL };
    constexpr long long largeArea{ 1920LL * 1080LL };
    constexpr long long largePenalty{ 10000000LL };

    auto best{ formats.begin() };
    long long bestScore{ std::numeric_limits<long long>::max() };
    for (auto it = formats.begin(); it != formats.end(); ++it)
    {
      int width{ 0 }, height{ 0 };
      if (!parseResolution(it->resolution, width, height)) continue;
      if (width == 1280 && height == 720) return it;

      const long long area{ static_cast<long long>(width) * height };
      const long long score{ std::llabs(area - targetArea) + (area > largeArea ? largePenalty : 0LL) };
      if (score < bestScore)
      {
        bestScore = score;
        best = it;
      }
    }
    return best;
  }
}
#endif

#if !defined(_WIN32) && !defined(__ANDROID__) && !defined(__APPLE__)
// バックエンドのリスト
const std::map<cv::VideoCaptureAPIs, const char*> Menu::backendList
{
#  if defined(USE_LIBCAMERA)
  { CAP_LIBCAMERA, "libcamera" },
#  endif
  { cv::CAP_ANY, "(any)" },
#  if defined(__linux__)
  { cv::CAP_V4L2, "V4L2" },
#  endif
  { cv::CAP_FFMPEG, u8"動画ファイル履歴" }
};

// コーデックのリスト
const std::vector<const char*> Menu::codecList
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
std::map <cv::VideoCaptureAPIs, std::vector<std::string>> Menu::deviceList;

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

#  if defined(__linux__)
#    include <filesystem>
#    include <fstream>
#    include <fcntl.h>
#    include <unistd.h>
#    include <sys/ioctl.h>
#    include <linux/videodev2.h>

//
// Linux (V4L2) のビデオデバイスの一覧を作る
//
void getV4L2List(std::vector<std::string>& list)
{
  namespace fs = std::filesystem;
  std::error_code ec;

  // /sys/class/video4linux ディレクトリを走査する
  const fs::path v4l2Path{ "/sys/class/video4linux" };
  if (fs::exists(v4l2Path, ec))
  {
    std::map<int, std::string> cameraDevices;
    std::map<int, std::string> otherDevices;

    for (const auto& entry : fs::directory_iterator(v4l2Path, ec))
    {
      const auto filename{ entry.path().filename().string() };
      // "video" で始まるノード (video0, video1, ...)
      if (filename.rfind("video", 0) == 0)
      {
        try
        {
          const int index{ std::stoi(filename.substr(5)) };
          const std::string devPath{ "/dev/" + filename };

          // デバイスファイルを開いてケーパビリティを調べる
          const int fd{ ::open(devPath.c_str(), O_RDONLY | O_NONBLOCK) };
          if (fd >= 0)
          {
            v4l2_capability cap{};
            if (::ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0)
            {
              const uint32_t caps{ cap.device_caps ? cap.device_caps : cap.capabilities };
              const std::string card{ reinterpret_cast<const char*>(cap.card) };
              const std::string driver{ reinterpret_cast<const char*>(cap.driver) };

              // キャプチャ機能 (VIDEO_CAPTURE) を持ち、出力 (OUTPUT) や M2M ではないこと
              const bool isCapture{ (caps & (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE)) != 0 };
              const bool isM2M{ (caps & (V4L2_CAP_VIDEO_M2M | V4L2_CAP_VIDEO_M2M_MPLANE)) != 0 };
              const bool isOutput{ (caps & (V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_VIDEO_OUTPUT_MPLANE)) != 0 };
              const bool isMeta{ (caps & (V4L2_CAP_META_CAPTURE | V4L2_CAP_META_OUTPUT)) != 0 };

              // Raspberry Pi の bcm2835-codec や bcm2835-isp, pisp 等の SoC 内部処理用ノードは除外
              const bool isSoCInternal{
                driver.find("bcm2835") != std::string::npos ||
                card.find("bcm2835") != std::string::npos ||
                driver.find("pisp") != std::string::npos ||
                card.find("pisp") != std::string::npos
              };

              std::string displayName{ filename + ": " + (card.empty() ? driver : card) };

              if (isCapture && !isM2M && !isOutput && !isMeta && !isSoCInternal)
              {
                cameraDevices[index] = displayName;
              }
              else
              {
                otherDevices[index] = displayName;
              }
            }
            ::close(fd);
          }
        }
        catch (...)
        {
        }
      }
    }

    // カメラデバイスがあればそれを登録
    for (const auto& [idx, devName] : cameraDevices)
    {
      list.emplace_back(devName);
    }

    // カメラデバイスが見つからなかった場合はフォールバックとしてその他を登録
    if (list.empty())
    {
      for (const auto& [idx, devName] : otherDevices)
      {
        list.emplace_back(devName);
      }
    }
  }

  // デバイスが取得できなかった場合はフォールバック
  if (list.empty())
  {
    getAnyList(list);
  }
}
#  endif
#endif // !defined(_WIN32) && !defined(__ANDROID__) && !defined(__APPLE__)

//
// キャプチャデバイスを開く
//
bool Menu::openDevice()
{
#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  // 何のデバイスも接続されていなければ戻る
  if (deviceNumber < 0) return false;

  // 前に開いていたキャプチャデバイスを (キャプチャスレッドを止めてから) 閉じる
  capture.close();

  // 選択したキャプチャデバイスを開く
  if (capture.openDevice(deviceNumber))
  {
    // フォーマットの選択肢を作り直す (未選択なら既定のフォーマットを選ぶ)
    updateFormatDropdowns();

    // フォーマットを指定して開始できるように準備する
    if (capture.select(formatNumber))
    {
      // 実解像度と焦点距離から、この入力を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
      return true;
    }
  }

  // 開けなかった
  errorMessage = u8"デバイスが開けません";
  return false;
#else
  // コーデック
  char codec[5]{};
  if (codecNumber > 0) strncpy(codec, codecList[codecNumber], 5);

  // 実際のデバイス番号を決定する
  int actualDeviceNumber{ deviceNumber };
#  if defined(__linux__)
#    if defined(USE_LIBCAMERA)
  if (backend == CAP_LIBCAMERA)
  {
    actualDeviceNumber = deviceNumber;
  }
  else
#    endif
  if (backend == cv::CAP_V4L2)
  {
    const auto& name{ getDeviceName(backend, deviceNumber) };
    if (name.rfind("video", 0) == 0)
    {
      try
      {
        actualDeviceNumber = std::stoi(name.substr(5));
      }
      catch (...)
      {
      }
    }
  }
#  endif

  // ダイアログで指定したキャプチャデバイスが開けなかったら
  if (!capture.openDevice(actualDeviceNumber,
    intrinsics.size, intrinsics.fps, backend, codec))
  {
    // 開けなかった
    errorMessage = u8"デバイスが開けません";
    return false;
  }

  // 実解像度と焦点距離から、この入力を見やすく表示する初期画角を設定する
  initializeInputIntrinsics(capture.getSize());

  // 使うことになったコーデックの番号を調べる
  for (size_t i = 0; i < codecList.size(); ++i)
  {
    if (strncmp(codec, codecList[i], 4) == 0)
    {
      // コーデックが分かった
      codecNumber = static_cast<int>(i);
      return true;
    }
  }

  // コーデックが分からない
  codecNumber = 0;
  return true;
#endif
}

#if !defined(__ANDROID__)
//
// 画像ファイルを開く
//
void Menu::openImage()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, imageFilter, 1, NULL) == NFD_OKAY)
  {
    // スレッドが動作中なら停止する
    capture.stop();

    // ダイアログで指定した画像ファイルが開けたら
    if (capture.openImage(filepath))
    {
      // 実解像度と焦点距離から、この画像を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
    }
    else
    {
      // 開けなかった
      errorMessage = u8"画像ファイルが開けません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 動画ファイルを開く
//
void Menu::openMovie()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, movieFilter, 1, NULL) == NFD_OKAY)
  {
    // スレッドが動作中なら停止する
    capture.stop();

    // ダイアログで指定した動画ファイルが開けたら
    if (capture.openMovie(filepath))
    {
      // 実解像度と焦点距離から、この動画を見やすく表示する初期画角を設定する
      initializeInputIntrinsics(capture.getSize());
    }
    else
    {
      // 開けなかった
      errorMessage = u8"動画ファイルが開けません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}
#endif

#if !defined(__ANDROID__)
//
// 構成ファイルを読み込む
//
void Menu::loadConfig()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, jsonFilter, 1, NULL) == NFD_OKAY)
  {
    // 現在の構成を構成ファイルの内容にする
    if (config.load(filepath))
    {
      // 現在の設定に反映する
      settings = config.getSettings();

      // 選択番号を有効範囲に収め、新しい投影方式の内部パラメータを反映する
      selectPreference(preferenceNumber < static_cast<int>(config.getPreferences().size())
        ? preferenceNumber : 0);

      // 較正設定も新しい構成に同期する
      calibration.setDictionary(settings.dictionaryName, settings.checkerSize, settings.checkerLength);
    }
    else
    {
      // 読み込めなかった
      errorMessage = u8"構成ファイルが読み込めません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 構成ファイルを保存する
//
void Menu::saveConfig() const
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_SaveDialog(&filepath, jsonFilter, 1, NULL, "*.json") == NFD_OKAY)
  {
    // 現在の設定で構成を更新する
    config.setSettings(settings);

    // 現在の構成を保存する
    if (!config.save(filepath))
    {
      // 保存できなかった
      errorMessage = u8"構成ファイルが保存できません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 較正ファイルを読み込む
//
void Menu::loadCalibration()
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, jsonFilter, 1, NULL) == NFD_OKAY)
  {
    // 現在のキャリブレーションパラメータを較正ファイルの内容にする
    if (!calibration.loadParameters(filepath))
    {
      // 読み込めなかった
      errorMessage = u8"較正ファイルが読み込めません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 較正ファイルを保存する
//
void Menu::saveParameters() const
{
  // キャリブレーションが完了していなければ戻る
  if (!calibration.finished()) return;

  // 現在時刻の取得
  const auto now{ std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
  const std::tm* const localTime{ std::localtime(&now) };

  // 時刻を文字列に変換 (例: calib202609282345.json)
  const auto timeString{ std::put_time(localTime, "calib%Y%m%d%H%M.json") };
  const auto pathString{ static_cast<std::ostringstream&&>(std::ostringstream() << timeString).str() };

  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_SaveDialog(&filepath, jsonFilter, 1, NULL, pathString.c_str()) == NFD_OKAY)
  {
    // 現在のキャリブレーションパラメータを構成ファイルに保存する
    if (!calibration.saveParameters(filepath))
    {
      // 保存できなかった
      errorMessage = u8"較正ファイルが保存できません";
    }

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}

//
// 較正用の画像ファイルを取得する (複数選択)
//
void Menu::recordFileCorners() const
{
  // ファイルダイアログから得るパス
  const nfdpathset_t* outPaths;

  // ファイルダイアログを開く
  if (NFD_OpenDialogMultiple(&outPaths, imageFilter, 1, NULL) == NFD_OKAY)
  {
    // ファイルパスの一覧を得る
    nfdpathsetenum_t enumerator;
    NFD_PathSet_GetEnum(outPaths, &enumerator);

    // ファイルパスの一覧からファイルパスを一つずつ取り出して
    for (nfdchar_t* path = NULL; NFD_PathSet_EnumNext(&enumerator, &path) && path;)
    {
      // 画像の読み出し
      CamImage image;

      // 画像ファイルが読み出せたら
      if (image.open(path))
      {
        // データのコピー先
        cv::Mat frame;

        // データをコピーして
        image.lockFrame([&frame](const std::uint8_t* data, size_t length, int width, int height, int channels) {
          frame.create(height, width, ((channels - 1) << 3));
          std::memcpy(frame.data, data, std::min(static_cast<size_t>(frame.total() * frame.elemSize()), length));
        });

        // ボードを検出して
        calibration.detectBoard(frame);

        // コーナーを記録する
        calibration.recordCorners();

        // 画像の読み出しを終わる
        image.close();
      }

      // ファイルパスの取り出しに使ったメモリを開放する
      NFD_PathSet_FreePath(path);
    }

    // ファイルパスの一覧に使ったメモリを開放する
    NFD_PathSet_FreeEnum(&enumerator);

    // フォルダのパスに使ったメモリを開放する
    NFD_PathSet_Free(outPaths);
  }
}

//
// 較正用の ChArUco Board を作成する
//
void Menu::createCharuco() const
{
  // ChArUco Board の画像を作成する
  cv::Mat boardImage;
  calibration.drawBoard(boardImage, 980, 692);

  // ファイルに保存する
  saveImage(boardImage, "ChArUcoBoard.png");
}
#endif

//
// コンストラクタ
//
Menu::Menu(Config& config, Capture& capture, Calibration& calibration)
  : config{ config }
  , settings{ config.getSettings() }
#if !defined(__ANDROID__)
  , intrinsics{ config.getPreferences().front().getIntrinsics() }
#endif
  , capture{ capture }
  , calibration{ calibration }
{
  // ファイルダイアログ (Native File Dialog Extended) を初期化する
#if !defined(__ANDROID__)
  NFD_Init();
#endif

  // Dear ImGui の入力デバイス
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // キーボードコントロールを使う
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // ゲームパッドを使う

  // Dear ImGui のスタイル
  //ImGui::StyleColorsDark();                                 // 暗めのスタイル
  //ImGui::StyleColorsClassic();                              // 以前のスタイル

#if !defined(__ANDROID__)
  // 日本語を表示できるメニューフォントを読み込む
  // 基本の日本語グリフセット（常用・人名用漢字、ひらがな、カタカナ、英数字）に加え、
  // デバイス名等に含まれる一般句読点（引用符、ダッシュ等）や文字様記号（商標記号等）を追加する
  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesJapanese());

  // 追加の Unicode 範囲 (2要素で1範囲、0終端)
  static const ImWchar additionalRanges[] =
  {
    0x2000, 0x206F, // General Punctuation (引用符、ダッシュ、リーダー等)
    0x2100, 0x214F, // Letterlike Symbols (商標記号 TM 等)
    0x2190, 0x21FF, // Arrows (矢印記号等)
    0x2460, 0x24FF, // Enclosed Alphanumerics (丸数字・囲み英数字等)
    0x25A0, 0x25FF, // Geometric Shapes (幾何学模様・図形記号等)
    0,
  };
  builder.AddRanges(additionalRanges);

  // フォントアトラス構築時までメモリを維持するため static で保持する
  static ImVector<ImWchar> glyphRanges;
  builder.BuildRanges(&glyphRanges);

  if (!ImGui::GetIO().Fonts->AddFontFromFileTTF(config.getMenuFont().c_str(), config.getMenuFontSize(),
    nullptr, glyphRanges.Data))
  {
    // メニューフォントが読み込めなかったらエラーにする
    throw std::runtime_error("Cannot find any menu fonts.");
  }
#endif

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
  // 初期状態で最初のデバイスのフォーマットリストを取得しておく
  if (!config.getDeviceList().empty())
  {
    capture.updateFormatList(deviceNumber);
  }
#else
  // バックエンドごとのキャプチャデバイスの一覧を初期化する
  for (auto& [api, name] : backendList)
  {
    // バックエンドごとに空のリストを追加する
    deviceList.emplace(api, std::vector<std::string>());
  }

  // キャプチャデバイスの一覧を作る
  getAnyList(deviceList.at(cv::CAP_ANY));
#  if defined(__linux__)
#    if defined(USE_LIBCAMERA)
  deviceList.at(CAP_LIBCAMERA) = CamLibcam::getDeviceList();
  if (!deviceList.at(CAP_LIBCAMERA).empty())
  {
    backend = CAP_LIBCAMERA;
  }
#    endif
  getV4L2List(deviceList.at(cv::CAP_V4L2));
#  endif
#endif
}

//
// デストラクタ
//
Menu::~Menu()
{
  // 前に開いていたキャプチャデバイスを (キャプチャスレッドを止めてから) 閉じる
  capture.close();

  // ファイルダイアログ (Native File Dialog Extended) を終了する
#if !defined(__ANDROID__)
  NFD_Quit();
#endif
}

//
// 入力画像に合わせて内部パラメータを初期化する
//
void Menu::initializeInputIntrinsics(const std::array<int, 2>& size)
{
  // 実際の入力解像度を処理系へ反映する
  intrinsics.size = size;

  // 無効な入力によるゼロ除算を避け、有効な場合だけ焦点距離から初期画角を求める
  if (size[0] > 0 && size[1] > 0 && settings.focal > 0.0f)
  {
    intrinsics.setFov(settings.focal);
  }
}

//
// 選択中の入力設定を適用してキャプチャを開始する
//
bool Menu::startCapture()
{
  // オープン、フォーマット適用、初期画角の設定を一つの入口に集約し、失敗時は開始処理を中断する
  if (!openDevice()) return false;

  // デバイスが確定してから動作モードを反映し、取得スレッドを開始する
  capture.setPrioritizeLatency(prioritizeLatency);
  capture.start();
  return true;
}

#if !defined(__ANDROID__)
//
// 選択中の投影方式とその内部パラメータを同期する
//
void Menu::selectPreference(int index)
{
  // 不正な選択番号では現在の投影状態を変更しない
  if (index < 0 || index >= static_cast<int>(config.getPreferences().size())) return;

  // 入力中の実解像度を、投影方式の構成値で上書きしないため退避する
  const auto size{ intrinsics.size };
  preferenceNumber = index;
  intrinsics = getPreference().getIntrinsics();

  // 入力中は実解像度だけを戻し、画角と中心位置は選択した投影方式の設定値を使用する
  if (capture.isOpened()) intrinsics.size = size;
}
#endif

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
//
// 解像度、フレームレート、コーデックの選択リストを更新する
//
void Menu::updateFormatDropdowns()
{
  const auto& formatList{ capture.getFormatList() };

  // デバイス列挙結果を UI 側にコピーし、各ドロップダウンの候補を作り直す
  availableFormats = formatList;
  uniqueResolutions.clear();
  uniqueFpsList.clear();
  uniqueCodecs.clear();

  // 構造化されたフォーマット情報から選択肢を作成する
  for (const auto& info : availableFormats)
  {
    if (std::find(uniqueResolutions.begin(), uniqueResolutions.end(), info.resolution) == uniqueResolutions.end())
      uniqueResolutions.push_back(info.resolution);
    if (std::find(uniqueFpsList.begin(), uniqueFpsList.end(), info.fps) == uniqueFpsList.end())
      uniqueFpsList.push_back(info.fps);
    if (std::find(uniqueCodecs.begin(), uniqueCodecs.end(), info.codec) == uniqueCodecs.end())
      uniqueCodecs.push_back(info.codec);
  }

  // 現在の formatNumber のフォーマットに同期する
  auto selected{ std::find_if(availableFormats.cbegin(), availableFormats.cend(),
    [this](const CaptureFormat& info) { return info.index == formatNumber; }) };

  // formatNumber が未設定または範囲外の場合は既定のフォーマットを自動選択する
  if (selected == availableFormats.cend() && !availableFormats.empty())
  {
    selected = findDefaultFormat(availableFormats);
    formatNumber = selected->index;
  }

  if (selected != availableFormats.cend())
  {
    currentRes = selected->resolution;
    currentFps = selected->fps;
    currentCodec = selected->codec;
  }
  else
  {
    currentRes.clear();
    currentFps.clear();
    currentCodec.clear();
  }
  lastDeviceNumber = deviceNumber;
}

//
// 解像度、フレームレート、コーデックのいずれかを選択し、実在する組み合わせに同期する
//
void Menu::selectFormatItem(std::string CaptureFormat::* field, const std::string& value)
{
  // 選択した項目だけを変更し、他の項目は現在の選択を維持した組み合わせを作る
  CaptureFormat wanted{ currentRes, currentFps, currentCodec, formatNumber };
  wanted.*field = value;

  // 選択した項目が一致するフォーマットなら真
  const auto sameItem{ [field, &value](const CaptureFormat& f) { return f.*field == value; } };

  // 組み合わせがそのまま実在すればそれを使い、なければ解像度を維持できるもの、
  // それもなければ選択した項目が一致する最初のフォーマットへ同期する
  auto it{ std::find_if(availableFormats.cbegin(), availableFormats.cend(), [&wanted](const CaptureFormat& f)
    { return f.resolution == wanted.resolution && f.fps == wanted.fps && f.codec == wanted.codec; }) };
  if (it == availableFormats.cend())
  {
    it = std::find_if(availableFormats.cbegin(), availableFormats.cend(), [&](const CaptureFormat& f)
      { return sameItem(f) && f.resolution == wanted.resolution; });
  }
  if (it == availableFormats.cend())
  {
    it = std::find_if(availableFormats.cbegin(), availableFormats.cend(), sameItem);
  }

  // 同期先が見つからなければ選択した値だけを反映する (「フォーマットが存在しません」と表示される)
  const auto& result{ it != availableFormats.cend() ? *it : wanted };
  currentRes = result.resolution;
  currentFps = result.fps;
  currentCodec = result.codec;
}
#endif

#if !defined(__ANDROID__)
//
// シェーダを設定する
//
std::array<GLsizei, 2> Menu::setup(GLfloat aspect) const
{
  // シェーダを設定する
  return config.getPreferences()[preferenceNumber].getShader().setup(settings.samples, aspect,
    pose, intrinsics.fov, intrinsics.center, settings.getFocal(), config.getBackground());
}

//
// 指定した視点姿勢を加えてシェーダを設定する
//
std::array<GLsizei, 2> Menu::setup(GLfloat aspect, const gg::GgMatrix& viewPose) const
{
  // メニューの補正を基準空間からカメラ空間への変換として先に適用する
  return config.getPreferences()[preferenceNumber].getShader().setup(settings.samples, aspect,
    pose * viewPose, intrinsics.fov, intrinsics.center, settings.getFocal(), config.getBackground());
}

//
// メインメニューバーの描画
//
void Menu::drawMainMenuBar()
{
  if (ImGui::BeginMainMenuBar())
  {
    // ファイルメニュー
    if (ImGui::BeginMenu(u8"ファイル"))
    {
      // 画像ファイルを開く
      if (ImGui::MenuItem(u8"画像ファイルを開く")) openImage();

      // 動画ファイルを開く
      if (ImGui::MenuItem(u8"動画ファイルを開く")) openMovie();

      // 構成ファイルを開く
      if (ImGui::MenuItem(u8"構成ファイルを開く")) loadConfig();

      // 構成ファイルを保存する
      if (ImGui::MenuItem(u8"構成ファイルを保存")) saveConfig();

      // キャリブレーションパラメータファイルを開く
      if (ImGui::MenuItem(u8"較正ファイルを開く")) loadCalibration();

      // キャリブレーションパラメータファイルを保存する
      if (ImGui::MenuItem(u8"較正ファイルを保存")) saveParameters();

      // フォルダ内の画像ファイルを使って較正する
      if (ImGui::MenuItem(u8"較正用画像から取得")) recordFileCorners();

      // ChArUco Board の作成
      if (ImGui::MenuItem(u8"ChArUco 画像作成")) createCharuco();

      // 終了
      quit = ImGui::MenuItem(u8"終了");

      // File メニュー修了
      ImGui::EndMenu();
    }

    // ウィンドウメニュー
    if (ImGui::BeginMenu(u8"ウィンドウ"))
    {
      // 入力パネルの表示
      ImGui::MenuItem(u8"入力", NULL, &showInputPanel);

      // 較正パネルの表示
      ImGui::MenuItem(u8"較正", NULL, &showCalibrationPanel);

      // File メニュー修了
      ImGui::EndMenu();
    }

    // メニューバーの高さを保存しておく
    menubarHeight = static_cast<GLsizei>(ImGui::GetWindowHeight());

    // メインメニューバー終了
    ImGui::EndMainMenuBar();
  }
}

//
// 入力パネルの描画
//
void Menu::drawInputPanel()
{
  // 入力パネル
  if (showInputPanel)
  {
    // ウィンドウの位置とサイズ (メニューバーを除いた表示領域内に高さを収める)
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    const float displayW{ ImGui::GetIO().DisplaySize.x };
    const float displayH{ ImGui::GetIO().DisplaySize.y };

    const float posY{ 2.0f + menubarHeight };
    const float targetH{ 517.0f * uiScale };
    const float maxH{ (displayH > posY + 10.0f) ? (displayH - posY - 4.0f) : targetH };
    const float winW{ (displayW > 10.0f) ? std::min(231.0f * uiScale, displayW - 4.0f) : 231.0f * uiScale };
    const float winH{ std::min(targetH, maxH) };

    ImGui::SetNextWindowPos(ImVec2(2.0f, posY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f * uiScale, 100.0f * uiScale), ImVec2(displayW, maxH));
    ImGui::Begin(u8"入力", &showInputPanel);

    // 投影方式の選択
    if (ImGui::BeginCombo(u8"投影方式", getPreference().getDescription().c_str()))
    {
      // すべての投影方式について
      for (int i = 0; i < static_cast<int>(config.getPreferences().size()); ++i)
      {
        // その投影方式が選択されていれば真
        const bool selected{ i == preferenceNumber };

        // 投影方式を（それが現在の投影方式ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(getPreference(i).getDescription().c_str(), selected))
        {
          // 表示した投影方式が選択されていたらそれを現在の選択とする
          selectPreference(i);
        }

        // この選択を次にコンボボックスを開いたときのデフォルトにしておく
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // レンズの画角を設定する
    ImGui::DragFloat2(u8"画角", intrinsics.fov.data(), 0.1f, -360.0f, 360.0f, "%.2f");

    // レンズの中心位置
    ImGui::DragFloat2(u8"中心", intrinsics.center.data(), 0.001f, -1.0f, 1.0f, "%.4f");

    // キャプチャデバイス固有のパラメータを元に戻す
    if (ImGui::Button(u8"回復"))
    {
      // 選択した投影方式のキャプチャデバイス固有のパラメータを回復する
      intrinsics = getPreference().getIntrinsics();
    }

    // 姿勢
    ImGui::SliderAngle(u8"方位", &settings.euler[1], -180.0f, 180.0f, "%.2f");
    ImGui::SliderAngle(u8"仰角", &settings.euler[0], -180.0f, 180.0f, "%.2f");
    ImGui::SliderAngle(u8"傾斜", &settings.euler[2], -180.0f, 180.0f, "%.2f");

    // 焦点距離
    ImGui::SliderFloat(u8"焦点距離", &settings.focal, settings.focalRange[0], settings.focalRange[1], "%.1f");

    // 姿勢を元に戻す
    if (ImGui::Button(u8"復帰")) resetPose();

    // スライダーまたは復帰で変更された姿勢を回転行列へ反映する
    updatePose();

    ImGui::Separator();

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
    // キャプチャデバイスが存在するとき
    if (!config.getDeviceList().empty())
    {
      // 装置関連項目
      ImGui::Text("%s", u8"以下の変更は [開始] で反映します");

      // キャプチャデバイスの選択コンボボックス
      if (ImGui::BeginCombo(u8"装置", config.getDeviceName(deviceNumber).c_str()))
      {
        // すべてのキャプチャデバイスについて
        for (int i = 0; i < static_cast<int>(config.getDeviceList().size()); ++i)
        {
          // キャプチャデバイス名を (それを選択していればハイライトして) コンボボックスに表示する
          if (ImGui::Selectable(config.getDeviceName(i).c_str(), i == deviceNumber))
          {
            // キャプチャデバイスが変わったら
            if (deviceNumber != i)
            {
              // 前に開いていたキャプチャデバイスを (キャプチャスレッドを止めてから) 閉じる
              capture.close();

              // 表示したキャプチャデバイスが選択されていたらそのキャプチャデバイスを選択する
              deviceNumber = i;

              // キャプチャデバイスが変わったので最初のビデオフォーマットを選択する
              formatNumber = 0;

              // 開始ボタンが押されるまでは、フォーマットリストだけを一時取得して更新する
              capture.updateFormatList(deviceNumber);

              // 選択可能なドロップダウンのリストを更新する
              updateFormatDropdowns();
            }

            // この選択を次にコンボボックスを開いたときのデフォルトにしておく
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      // 使用可能なビデオフォーマットの表示名のリスト
      const auto& formatList{ capture.getFormatList() };

      // 使用可能なビデオフォーマットが存在するなら
      if (!formatList.empty())
      {
        // 必要な場合（デバイス番号の不一致やリスト未作成時）にリストを更新する
        if (lastDeviceNumber != deviceNumber || availableFormats.empty())
        {
          updateFormatDropdowns();
        }

        // 1. 解像度の選択コンボボックス
        if (ImGui::BeginCombo(u8"解像度", currentRes.c_str()))
        {
          for (const auto& res : uniqueResolutions)
          {
            if (ImGui::Selectable(res.c_str(), currentRes == res))
            {
              selectFormatItem(&CaptureFormat::resolution, res);
            }
          }
          ImGui::EndCombo();
        }

        // 2. フレームレートの選択コンボボックス
        std::string fpsLabel = currentFps + " fps";
        if (ImGui::BeginCombo(u8"コマ数", fpsLabel.c_str()))
        {
          for (const auto& fpsVal : uniqueFpsList)
          {
            std::string valLabel = fpsVal + " fps";
            if (ImGui::Selectable(valLabel.c_str(), currentFps == fpsVal))
            {
              selectFormatItem(&CaptureFormat::fps, fpsVal);
            }
          }
          ImGui::EndCombo();
        }

        // 3. コーデックの選択コンボボックス
        if (ImGui::BeginCombo(u8"符号化", currentCodec.c_str()))
        {
          for (const auto& cod : uniqueCodecs)
          {
            if (ImGui::Selectable(cod.c_str(), currentCodec == cod))
            {
              selectFormatItem(&CaptureFormat::codec, cod);
            }
          }
          ImGui::EndCombo();
        }

        // 選択された組み合わせが availableFormats に存在するか探す
        const auto found{ std::find_if(availableFormats.cbegin(), availableFormats.cend(),
          [this](const CaptureFormat& info)
          { return info.resolution == currentRes && info.fps == currentFps && info.codec == currentCodec; }) };
        const bool formatExists{ found != availableFormats.cend() };

        // 存在する組み合わせに変わったら取得を止め、次の [開始] でそのフォーマットを適用する
        if (formatExists && formatNumber != found->index)
        {
          capture.stop();
          formatNumber = found->index;
        }

        // 4. レイテンシ優先のチェックボックス
        if (ImGui::Checkbox(u8"レイテンシ優先", &prioritizeLatency))
        {
          if (capture) capture.setPrioritizeLatency(prioritizeLatency);
        }

        // キャプチャの開始と停止
        if (capture)
        {
          // キャプチャスレッドが動いているので止める
          if (ImGui::Button(u8"停止")) capture.stop();
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "%s", u8"取得中");
        }
        else
        {
          if (formatExists)
          {
            // 「開始」ボタンをクリックしたときデバイスが選択されていれば
            if (ImGui::Button(u8"開始") && deviceNumber >= 0)
            {
              startCapture();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"停止中");
          }
          else
          {
            // 存在しない組み合わせの時はメッセージを表示する
            ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"フォーマットが存在しません");
          }
        }
      }
      else
      {
        // キャプチャデバイスが開けなかった
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"デバイスが開けません");
      }
    }
    else
    {
      // キャプチャデバイスが存在しないとき
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"デバイスが見つかりません");
    }
#else
    // 装置関連項目
    ImGui::Text("%s", u8"以下の変更は [開始] で反映します");

    // デバイスプリファレンスを選択する
    if (ImGui::BeginCombo(u8"装置特性", backendList.at(backend)))
    {
      // すべての表示方式について
      for (auto& [apiId, apiName] : backendList)
      {
        // その表示方式が選択されていれば真
        const bool selected{ apiId == backend };

        // 装置特性を（それが現在の装置特性ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(apiName, selected))
        {
          // 表示した装置特性が選択されていたらそれを現在の選択とする
          backend = apiId;

          // 切り替え前の装置特性のデバイスが存在しなければ最初のデバイスの番号を選択する
          if (deviceNumber < 0) deviceNumber = 0;

          // 選択されているデバイスの番号が接続されたキャプチャデバイスの数を超えないようにする
          const int count{ getDeviceCount(backend) };
          if (deviceNumber >= count) deviceNumber = count - 1;
        }

        // この選択を次にコンボボックスを開いたときのデフォルトにしておく
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // キャプチャデバイスが存在すれば
    if (deviceNumber >= 0)
    {
      // キャプチャデバイスの選択コンボボックス
      if (ImGui::BeginCombo(u8"入力源", getDeviceName(backend, deviceNumber).c_str()))
      {
        // すべてのキャプチャデバイスについて
        for (int i = 0; i < static_cast<int>(getDeviceList(backend).size()); ++i)
        {
          // キャプチャデバイス名を（それを選択していればハイライトして）コンボボックスに表示する
          if (ImGui::Selectable(getDeviceName(backend, i).c_str(), i == deviceNumber))
          {
            // 表示したキャプチャデバイスが選択されていたらそのキャプチャデバイスを選択する
            deviceNumber = i;

            // この選択を次にコンボボックスを開いたときのデフォルトにしておく
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
    }
    else
    {
      // 使えるキャプチャデバイスがない
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"デバイスが見つかりません");
    }

    // キャプチャするサイズとフレームレート
    ImGui::InputInt2(u8"解像度", intrinsics.size.data());
    ImGui::InputDouble(u8"周波数", &intrinsics.fps, 1.0f, 1.0f, "%.1f");

    // コーデックを選択する
    if (ImGui::BeginCombo(u8"符号化", codecList[codecNumber]))
    {
      // すべてのコーデックについて
      for (int i = 0; i < static_cast<int>(codecList.size()); ++i)
      {
        // コーデックを（それを選択していればハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(codecList[i], i == codecNumber))
        {
          // 表示したキャプチャデバイスが選択されていたらそのキャプチャデバイスを選択する
          codecNumber = i;

          // この選択を次にコンボボックスを開いたときのデフォルトにしておく
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    // キャプチャの開始と停止
    if (capture)
    {
      // キャプチャスレッドが動いているので止める
      if (ImGui::Button(u8"停止")) capture.stop();
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "%s", u8"取得中");
    }
    else
    {
      // キャプチャスレッドが止まっているので
      if (ImGui::Button(u8"開始") && deviceNumber >= 0)
      {
        startCapture();
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"停止中");
    }
#endif
    ImGui::End();
  }
}

//
// 較正パネルの描画
//
void Menu::drawCalibrationPanel()
{
  if (showCalibrationPanel)
  {
    // ウィンドウの位置とサイズ
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    const float displayW{ ImGui::GetIO().DisplaySize.x };
    const float displayH{ ImGui::GetIO().DisplaySize.y };

    const float panelW{ 218.0f * uiScale };
    float posX{ 235.0f * uiScale };
    float posY{ 2.0f + menubarHeight };
    if (posX + panelW > displayW && displayW > 0.0f)
    {
      posX = 20.0f * uiScale;
      posY += 30.0f * uiScale;
    }
    const float targetH{ 359.0f * uiScale };
    const float maxH{ (displayH > posY + 10.0f) ? (displayH - posY - 4.0f) : targetH };
    const float winW{ (displayW > 10.0f) ? std::min(panelW, displayW - 4.0f) : panelW };
    const float winH{ std::min(targetH, maxH) };

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(100.0f * uiScale, 100.0f * uiScale), ImVec2(displayW, maxH));
    ImGui::Begin(u8"較正", &showCalibrationPanel);

    // 辞書の選択
    if (ImGui::BeginCombo(u8"辞書", settings.dictionaryName.c_str()))
    {
      // すべての辞書について
      for (auto d = calibration.dictionaryList.begin(); d != calibration.dictionaryList.end(); ++d)
      {
        // その設定が現在選択されている設定なら真
        const bool selected{ d->first == settings.dictionaryName };

        // 設定を（それが現在の設定ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(d->first.c_str(), selected))
        {
          // 表示した設定が選択されていたらそれを現在の選択とする
          settings.dictionaryName = d->first;

          // 選択した ArUco Marker の辞書を設定する
          calibration.setDictionary(settings.dictionaryName, settings.checkerSize, settings.checkerLength);
        }

        // この選択を次にコンボボックスを開いたときのデフォルトにしておく
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    ImGui::Separator();

    // ArUco Marker の検出
    if (ImGui::Checkbox(u8"ArUco Marker 検出", &detectMarker) && detectMarker) detectBoard = false;

    // ArUco Marker の大きさ
    ImGui::InputFloat(u8"マーカ長", &settings.markerLength, 0.0f, 0.0f, "%.2f cm");

    ImGui::Separator();

    // 較正
    if (ImGui::Checkbox(u8"ChArUco Board 検出", &detectBoard) && detectBoard) detectMarker = false;

    // ChArUco Board のマス目の数 (横, 縦)
    if (ImGui::InputInt2(u8"升目数", settings.checkerSize.data()))
    {
      // 升目の数は 2 以上とする
      settings.checkerSize[0] = std::max(2, settings.checkerSize[0]);
      settings.checkerSize[1] = std::max(2, settings.checkerSize[1]);

      // ChArUco Board を作り直す
      calibration.createBoard(settings.checkerSize, settings.checkerLength);
    }

    // ChArUco Board の大きさ
    if (ImGui::InputFloat2(u8"升目長", settings.checkerLength.data(), "%.2f cm"))
    {
      // ChArUco Board を作り直す
      calibration.createBoard(settings.checkerSize, settings.checkerLength);
    }

    // 「取得」ボタンをクリックしたとき ChArUco Board の検出中なら
    if (ImGui::Button(u8"取得") && detectBoard)
    {
      // 検出したコーナーを記録する
      calibration.recordCorners();
    }

    // １つでも標本を取得していれば
    if (calibration.getSampleCount() > 0)
    {
      // 標本の「消去」ボタンを表示する
      ImGui::SameLine();
      if (ImGui::Button(u8"消去"))
      {
        calibration.discardCorners();
        autoCaptureStatusMessage.clear();
      }

      // 標本を６つ以上取得していれば
      if (calibration.getSampleCount() >= 6)
      {
        // 「較正」ボタンを表示する
        ImGui::SameLine();
        if (ImGui::Button(u8"較正") && !calibration.calibrate())
        {
          // 較正失敗
          errorMessage = u8"較正に失敗しました";
        }

        // 較正が完了していれば
        if (calibration.finished())
        {
          // 「完了」を表示する
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "%s", u8"完了");
        }
      }
    }

    // 自動キャプチャ機能 (ChArUco Board 検出中のみ有効)
    if (detectBoard)
    {
      ImGui::Separator();
      ImGui::Checkbox(u8"自動キャプチャ", &autoCaptureEnabled);
      if (autoCaptureEnabled)
      {
        ImGui::SameLine();
        ImGui::Checkbox(u8"音", &autoCaptureBeep);

        ImGui::SliderFloat(u8"静止時間", &autoCaptureMinStableTime, 0.3f, 1.5f, "%.2f s");

        // クールダウンまたは静止プログレスバー表示
        if (autoCaptureCooldownTimer > 0.0f)
        {
          const float cdRatio{ autoCaptureCooldownTimer / autoCaptureCooldown };
          ImGui::ProgressBar(cdRatio, ImVec2(-1, 0), u8"姿勢変更待機中...");
        }
        else
        {
          const float prog{ calibration.getStableProgress(autoCaptureMinStableTime) };
          std::string progText{ u8"静止検知: " + std::to_string(static_cast<int>(prog * 100)) + "%" };
          if (prog >= 1.0f)
          {
            progText = calibration.isDiverseEnough() ? u8"記録可能 (多様性OK)" : u8"多様性不足 (動かしてください)";
          }
          ImGui::ProgressBar(prog, ImVec2(-1, 0), progText.c_str());
        }

        // 変位量・静止状態テキスト
        const float motion{ calibration.getCurrentMotion() };
        if (calibration.isStable())
        {
          if (calibration.isDiverseEnough())
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", u8"静止状態: 安定 (多様性あり)");
          else
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "%s", u8"静止状態: 安定 (前回姿勢と類似)");
        }
        else
        {
          if (motion < 100.0f)
            ImGui::Text(u8"変位量: %.1f px (進捗: %.0f%%)", motion, calibration.getStableProgress(autoCaptureMinStableTime) * 100.0f);
          else
            ImGui::Text("%s", u8"変位量: -- px (コーナー不足)");
        }

        // 自動記録ステータスメッセージがあれば表示
        if (!autoCaptureStatusMessage.empty())
        {
          ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", autoCaptureStatusMessage.c_str());
        }
      }
    }

    ImGui::Separator();

    // フレームレートの表示
    ImGui::Text(u8"フレームレート: %6.2f fps", ImGui::GetIO().Framerate);

    // 検出数の表示
    ImGui::Text(u8"コーナー検出数: %d", calibration.getCornersCount());

    // 標本数の表示
    ImGui::Text(u8"サンプル取得数: %d (%d)", calibration.getSampleCount(), calibration.getTotalCount());

    // 再投影誤差の表示
    ImGui::Text(u8"再投影誤差: %.4f", calibration.getReprojectionError());

    ImGui::End();
  }
}

//
// エラーダイアログの描画
//
void Menu::drawErrorDialog()
{
  // エラーメッセージが設定されていたら
  if (errorMessage)
  {
    // ウィンドウの位置・サイズとタイトル
    const float uiScale{ ImGui::GetIO().FontGlobalScale };
    ImGui::SetNextWindowPos(ImVec2(60.0f * uiScale, 60.0f * uiScale), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(240.0f * uiScale, 92.0f * uiScale), ImGuiCond_Always);

    // ウィンドウを表示するとき true
    bool status{ true };

    // エラーメッセージウィンドウを表示する
    ImGui::Begin(u8"エラー", &status);

    // エラーメッセージの表示
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", errorMessage);

    // クローズボックスか「閉じる」ボタンをクリックしたら
    if (!status || ImGui::Button(u8"閉じる"))
    {
      // エラーメッセージを消去する
      errorMessage = nullptr;
    }
    ImGui::End();
  }
}

//
// メニューの描画
//
void Menu::draw()
{
  // 各ウィンドウの描画責務を分離し、この関数では一フレーム分の呼び出し順だけを管理する
  drawMainMenuBar();
  drawInputPanel();
  drawCalibrationPanel();
  drawErrorDialog();

  // ChArUco Board の検出中にスペースバーをタイプしたなら
  if (detectBoard && ImGui::IsKeyPressed(ImGuiKey_Space))
  {
    // 検出したコーナーを記録する
    calibration.recordCorners();
  }
}
#endif

//
// 画像の保存
//
void Menu::saveImage(const cv::Mat& image, const std::string& filename) const
{
#if !defined(__ANDROID__)
  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_SaveDialog(&filepath, imageFilter, 1, NULL, filename.c_str()) == NFD_OKAY)
  {
    // ファイルに保存する
    if (!CamImage::save(filepath, image)) errorMessage = u8"ファイルが保存できませんでした";

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
#else
  if (!CamImage::save(filename, image)) errorMessage = u8"ファイルが保存できませんでした";
#endif
}

//
// 自動キャプチャ処理を更新する
//
// 【目的】
//   ChArUco ボードの静止状態および幾何多様性を判定し、
//   条件を満たした場合に自動的に標本（コーナー）を記録する。
//
bool Menu::updateAutoCapture(float deltaTime)
{
  // ChArUco Board 検出中でなければ何もしない
  if (!detectBoard) return false;

  // 1. 静止判定（フレーム間変位追跡）を更新
  //    motionThresholdPx = 2.0px, minStableTime = autoCaptureMinStableTime, minCorners = 6
  calibration.updateMotion(deltaTime, 2.0f, autoCaptureMinStableTime, 6);

  // 2. 姿勢変更クールダウンタイマーの更新
  if (autoCaptureCooldownTimer > 0.0f)
  {
    autoCaptureCooldownTimer -= deltaTime;
    if (autoCaptureCooldownTimer < 0.0f) autoCaptureCooldownTimer = 0.0f;
  }

  // 3. 自動キャプチャが無効、またはクールダウン中なら記録判定をスキップ
  if (!autoCaptureEnabled || autoCaptureCooldownTimer > 0.0f) return false;

  // 4. 静止判定かつ多様性チェック
  if (calibration.isStable() && calibration.isDiverseEnough())
  {
    // 標本を記録する
    calibration.recordCorners();

    // 標本番号
    const int sampleIndex{ calibration.getSampleCount() };
    autoCaptureStatusMessage = u8"[自動] 標本 #" + std::to_string(sampleIndex) + u8" を記録しました";

    // 姿勢変更のためのクールダウンを開始
    autoCaptureCooldownTimer = autoCaptureCooldown;

    // 5. 音響フィードバック
    if (autoCaptureBeep)
    {
#if defined(_WIN32)
      std::thread([] { Beep(1200, 100); }).detach();
#elif !defined(__ANDROID__)
      std::cout << '\a' << std::flush;
#endif
    }

    return true;
  }

  return false;
}

#if defined(_WIN32) || defined(__ANDROID__) || defined(__APPLE__)
//
// カメラ解像度を選択する
//
bool Menu::selectResolution(const std::string& resolution)
{
  if (resolution == currentRes && bool(capture)) return true;

  for (const auto& item : availableFormats)
  {
    if (item.resolution == resolution)
    {
      currentRes = resolution;
      currentFps = item.fps;
      currentCodec = item.codec;
      formatNumber = item.index;

      if (capture.isOpened())
      {
        const bool wasRunning{ bool(capture) };
        if (wasRunning) capture.stop();
        if (capture.select(formatNumber))
        {
          initializeInputIntrinsics(capture.getSize());
          if (wasRunning) capture.start();
          return true;
        }
      }
      break;
    }
  }

  return false;
}
#endif
