///
/// メニューの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Menu.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// ファイルダイアログ
#include "nfd.h"

// JSON ファイル名のフィルタ
constexpr nfdfilteritem_t jsonFilter[]{ { "JSON", "json" } };

// 画像ファイル名のフィルタ
constexpr nfdfilteritem_t imageFilter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };

// 動画ファイル名のフィルタ
constexpr nfdfilteritem_t movieFilter[]{ "Movies", "mp4,m4v,mpg,mov,avi,ogg,mkv" };

// 標準ライブラリ
#include <iomanip>
#include <sstream>
#include <chrono>

///
/// キャプチャデバイスを開く
///
bool Menu::openDevice()
{
  // バックエンドが GStreamer なら
  if (backend == cv::CAP_GSTREAMER)
  {
    // ファイルのリストを取り出し
    const auto& pipeline{ config.deviceList.at(backend)[deviceNumber] };

    // ダイアログで指定したパイプラインが開けなかったら
    if (!capture.openMovie(pipeline, backend))
    {
      // 開けなかった
      errorMessage = u8"パイプラインが開けません";
      return false;
    }

    // GStreamer が使える
    return true;
  }

  // コーデック
  char codec[5]{};
  if (codecNumber > 0) strncpy(codec, config.codecList[codecNumber], 5);

  // ダイアログで指定したキャプチャデバイスが開けなかったら
  if (!capture.openDevice(deviceNumber,
    intrinsics.size, intrinsics.fps, backend, codec))
  {
    // 開けなかった
    errorMessage = u8"キャプチャデバイスが開けません";
    return false;
  }

  // 使うことになったコーデックの番号を調べる
  for (size_t i = 0; i < config.codecList.size(); ++i)
  {
    if (strncmp(codec, config.codecList[i], 4) == 0)
    {
      // コーデックが分かった
      codecNumber = static_cast<int>(i);
      return true;
    }
  }

  // コーデックが分からない
  codecNumber = 0;
  return true;
}

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
    // ダイアログで指定した画像ファイルが開けたら
    if (capture.openImage(filepath))
    {
      // 構成データの解像度と画角を開いた画像に合わせる
      setSize(capture.getSize());
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
    // 入力特性をファイルに切り替えて
    backend = cv::CAP_FFMPEG;

    // ファイルのリストを取り出し
    auto& fileList{ config.deviceList.at(backend) };
    const auto fileListLength{ static_cast<int>(fileList.size()) };

    // ファイルのリストの各ファイルについて
    for (deviceNumber = 0; deviceNumber < fileListLength; ++deviceNumber)
    {
      // 選択したファイルと同じものがあればそれを選択する
      if (fileList[deviceNumber] == filepath) break;
    }

    // 選択したファイルがファイルのリストの中になければ
    if (deviceNumber == fileListLength)
    {
      // その先頭にファイルパスを挿入して
      fileList.insert(fileList.begin(), filepath);

      // そのエントリを選択する
      deviceNumber = 0;
    }

    // ダイアログで指定した動画ファイルが開けたら
    if (capture.openMovie(filepath, backend))
    {
      // 構成データの解像度と画角を開いた画像に合わせる
      setSize(capture.getSize());
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
    if (const_cast<Config&>(config).load(filepath))
    {
      // 読み込んだ構成の数が現在の選択よりも少ないときは最初の項目の構成にする
      if (preferenceNumber > static_cast<int>(config.preferenceList.size()))
        preferenceNumber = 0;

      // 現在の設定に反映する
      settings = config.settings;
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
    const_cast<Config&>(config).settings = settings;

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
void Menu::loadParameters() const
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

  // 時刻を文字列に変換
  const auto timeString{ std::put_time(localTime, "cal_%Y%m%d_%H%M%S.json") };
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
        image.transmit(frame);

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

//
// コンストラクタ
//
Menu::Menu(const Config& config, Capture& capture, Calibration& calibration)
  : config{ config }
  , settings{ config.settings }
  , capture{ capture }
  , calibration{ calibration }
  , deviceNumber{ 0 }
  , codecNumber{ 0 }
  , preferenceNumber{ 0 }
  , backend{ cv::CAP_ANY }
  , pose{ ggIdentity() }
  , menubarHeight{ 0 }
  , showInputPanel{ true }
  , showCalibrationPanel{ true }
  , quit{ false }
  , errorMessage{ nullptr }
  , detectMarker{ false }
  , detectBoard{ false }
{
  // ファイルダイアログ (Native File Dialog Extended) を初期化する
  NFD_Init();

  // Dear ImGui の入力デバイス
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // キーボードコントロールを使う
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // ゲームパッドを使う

  // Dear ImGui のスタイル
  //ImGui::StyleColorsDark();                                 // 暗めのスタイル
  //ImGui::StyleColorsClassic();                              // 以前のスタイル

  // 日本語を表示できるメニューフォントを読み込む
  if (!ImGui::GetIO().Fonts->AddFontFromFileTTF(config.menuFont.c_str(), config.menuFontSize,
    nullptr, ImGui::GetIO().Fonts->GetGlyphRangesJapanese()))
  {
    // メニューフォントが読み込めなかったらエラーにする
    throw std::runtime_error("Cannot find any menu fonts.");
  }
}

//
// デストラクタ
//
Menu::~Menu()
{
  // ファイルダイアログ (Native File Dialog Extended) を終了する
  NFD_Quit();
}

//
// 解像度の初期値を設定する
//
void Menu::setSize(const std::array<int, 2>& size)
{
  // 解像度の調整値を設定する
  intrinsics.size = size;

  // 投影像の画角と中心位置を設定する
  intrinsics.setFov(settings.focal);
  intrinsics.setCenter(0.0f, 0.0f);
}

//
// シェーダを設定する
//
std::array<GLsizei, 2> Menu::setup(GLfloat aspect) const
{
  // シェーダを設定する
  return config.preferenceList[preferenceNumber].getShader().setup(settings.samples, aspect,
    pose, intrinsics.fov, intrinsics.center, settings.getFocal(), config.background);
}

//
// メニューの描画
//
void Menu::draw()
{
  // ImGui のフレームを準備する
  ImGui::NewFrame();

  // メインメニューバー
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
      if (ImGui::MenuItem(u8"較正ファイルを開く")) loadParameters();

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

  // 入力パネル
  if (showInputPanel)
  {
    // ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(2.0f, 2.0f + menubarHeight), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(231, 516), ImGuiCond_Once);
    ImGui::Begin(u8"入力", &showInputPanel);

    // 投影方式の選択
    if (ImGui::BeginCombo(u8"投影方式", getPreference().getDescription().c_str()))
    {
      // すべての投影方式について
      for (int i = 0; i < static_cast<int>(config.preferenceList.size()); ++i)
      {
        // その投影方式が選択されていれば真
        const bool selected{ i == preferenceNumber };

        // 投影方式を（それが現在の投影方式ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(getPreference(i).getDescription().c_str(), selected))
        {
          // 表示した投影方式が選択されていたらそれを現在の選択とする
          preferenceNumber = i;

          // 選択した投影方式のキャプチャデバイス固有のパラメータをコピーする
          intrinsics = getPreference().getIntrinsics();
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
    pose = ggRotateY(settings.euler[1]).rotateX(settings.euler[0]).rotateZ(settings.euler[2]);

    // 焦点距離
    ImGui::SliderFloat(u8"焦点距離", &settings.focal, settings.focalRange[0], settings.focalRange[1], "%.1f");

    // 姿勢を元に戻す
    if (ImGui::Button(u8"復帰"))
    {
      settings.euler = config.settings.euler;
      settings.focal = config.settings.focal;
      settings.focalRange = config.settings.focalRange;
    }

    ImGui::Separator();

    // 装置関連項目
    ImGui::Text("%s", u8"以下の変更は [開始] で反映します");

    // デバイスプリファレンスを選択する
    if (ImGui::BeginCombo(u8"装置特性", config.backendList.at(backend)))
    {
      // すべての表示方式について
      for (auto& [apiId, apiName] : config.backendList)
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
          const int count{ config.getDeviceCount(backend) };
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
      if (ImGui::BeginCombo(u8"入力源", config.getDeviceName(backend, deviceNumber).c_str()))
      {
        // すべてのキャプチャデバイスについて
        for (int i = 0; i < static_cast<int>(config.getDeviceList(backend).size()); ++i)
        {
          // キャプチャデバイス名を（それを選択していればハイライトして）コンボボックスに表示する
          if (ImGui::Selectable(config.getDeviceName(backend, i).c_str(), i == deviceNumber))
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
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"キャプチャデバイスが見つかりません");
    }

    // キャプチャするサイズとフレームレート
    ImGui::InputInt2(u8"解像度", intrinsics.size.data());
    ImGui::InputDouble(u8"周波数", &intrinsics.fps, 1.0f, 1.0f, "%.1f");

    // コーデックを選択する
    if (ImGui::BeginCombo(u8"符号化", config.codecList[codecNumber]))
    {
      // すべてのコーデックについて
      for (int i = 0; i < static_cast<int>(config.codecList.size()); ++i)
      {
        // コーデックを（それを選択していればハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(config.codecList[i], i == codecNumber))
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
        // キャプチャデバイスが開けたらキャプチャスレッドを動かす
        if (openDevice()) capture.start();
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"停止中");
    }
    ImGui::End();
  }

  // 較正パネル
  if (showCalibrationPanel)
  {
    // ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(235.0f, 2.0f + menubarHeight), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(222, 326), ImGuiCond_Once);
    ImGui::Begin(u8"較正", &showCalibrationPanel);

    // 辞書の選択
    if (ImGui::BeginCombo(u8"辞書", settings.dictionaryName.c_str()))
    {
      // すべての辞書について
      for (auto d = calibration.dictionaryList.begin(); d != calibration.dictionaryList.end(); ++d)
      {
        // その設定が現在選択されている設定なら真
        const bool selected(d->first == settings.dictionaryName);

        // 設定を（それが現在の設定ならハイライトして）コンボボックスに表示する
        if (ImGui::Selectable(d->first.c_str(), d->first == settings.dictionaryName))
        {
          // 表示した設定が選択されていたらそれを現在の選択とする
          settings.dictionaryName = d->first;

          // 選択した ArUco Marker の辞書を設定する
          calibration.setDictionary(settings.dictionaryName, settings.checkerLength);
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

    // ChArUco Board の大きさ
    if (ImGui::InputFloat2(u8"升目長", settings.checkerLength.data(), "%.2f cm"))
    {
      // ChArUco Board を作り直す
      calibration.createBoard(settings.checkerLength);
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
      if (ImGui::Button(u8"消去")) calibration.discardCorners();

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

  // エラーメッセージが設定されていたら
  if (errorMessage)
  {
    // ウィンドウの位置・サイズとタイトル
    ImGui::SetNextWindowPos(ImVec2(60, 60), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(240, 92), ImGuiCond_Always);

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

  // ChArUco Board の検出中にスペースバーをタイプしたなら
  if (detectBoard && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space)))
  {
    // 検出したコーナーを記録する
    calibration.recordCorners();
  }

  // ImGui のフレームに描画する
  ImGui::Render();
}

//
// 画像の保存
//
void Menu::saveImage(const cv::Mat& image, const std::string& filename) const
{
  // 画像ファイル名のフィルタ
  constexpr nfdfilteritem_t imageFilter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };

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
}
