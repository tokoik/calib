﻿///
/// メニューの描画クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Menu.h"

// 動画のキャプチャデバイス
#include "CamCv.h"

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// ファイルダイアログ
#include "nfd.h"

//
// 構成ファイルを読み込む
//
void Menu::loadConfig()
{
  // JSON ファイル名のフィルタ
  constexpr nfdfilteritem_t jsonFilter[]{ { "JSON", "json" } };

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
void Menu::saveConfig()
{
  // JSON ファイル名のフィルタ
  constexpr nfdfilteritem_t jsonFilter[]{ { "JSON", "json" } };

  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_SaveDialog(&filepath, jsonFilter, 1, NULL, "*.json") == NFD_OKAY)
  {
    // 現在の設定で構成を更新して保存する
    const_cast<Config&>(config).settings = settings;
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
// 画像ファイルを開く
//
void Menu::openImage()
{
  // 画像ファイル名のフィルタ
  constexpr nfdfilteritem_t imageFilter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };

  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, imageFilter, 1, NULL) == NFD_OKAY)
  {
    // ダイアログで指定した画像ファイルが開けたら
    if (capture.openImage(filepath))
    {
      // 画像の解像度を構成データに設定する
      intrinsics.size = capture.getSize();
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
  // 動画ファイル名のフィルタ
  constexpr nfdfilteritem_t movieFilter[]{ "Movies", "mp4,m4v,mpg,mov,avi,ogg,mkv" };

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
      //フレームの解像度を構成データに設定する
      intrinsics.size = capture.getSize();
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

///
/// キャプチャデバイスを開く
///
void Menu::openDevice()
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
    }
  }
  else if (backend != cv::CAP_FFMPEG)
  {
    // ダイアログで指定したキャプチャデバイスが開けなかったら
    if (!capture.openDevice(deviceNumber, intrinsics.size, intrinsics.fps, backend,
      codecNumber > 0 ? config.codecList[codecNumber] : ""))
    {
      // 開けなかった
      errorMessage = u8"キャプチャデバイスが開けません";
    }
  }
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
  , detectMarker{ false }
  , detectBoard{ false }
  , repError{ 0.0 }
  , showControlPanel{ true }
  , showInformationPanel{ false }
  , quit{ false }
  , errorMessage{ nullptr }
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
// 解像度と画角の調整値を設定する
//
void Menu::setSizeAndFov(const std::array<int, 2>& size, float tangent)
{
  // 解像度の調整値を設定する
  intrinsics.size = size;

  // 画角を設定する
  intrinsics.setFov(tangent);

  // 対角 35mm のフィルムに 50mm のレンズで投影したとき
  intrinsics.setFov(35.0f / 50.0f);
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
      // 構成ファイルを開く
      if (ImGui::MenuItem(u8"構成ファイルを開く")) loadConfig();

      // 構成ファイルを保存する
      if (ImGui::MenuItem(u8"構成ファイルを保存")) saveConfig();

      // 画像ファイルを開く
      if (ImGui::MenuItem(u8"画像ファイルを開く")) openImage();

      // 動画ファイルを開く
      if (ImGui::MenuItem(u8"動画ファイルを開く")) openMovie();

      // ChArUco Board の作成
      if (ImGui::MenuItem(u8"ボード画像を保存"))
      {
        // ChArUco Board の画像を作成する
        cv::Mat boardImage;
        calibration.drawBoard(boardImage, 1000, 700);

        // ファイルに保存する
        saveImage(boardImage, "ChArUcoBoard.png");
      }

      // 終了
      quit = ImGui::MenuItem(u8"終了");

      // File メニュー修了
      ImGui::EndMenu();
    }

    // ウィンドウメニュー
    if (ImGui::BeginMenu(u8"ウィンドウ"))
    {
      // コントロールパネルの表示
      ImGui::MenuItem(u8"コントロールパネル", NULL, &showControlPanel);

      // 情報パネルの表示
      ImGui::MenuItem(u8"情報", NULL, &showInformationPanel);

      // File メニュー修了
      ImGui::EndMenu();
    }

    // メニューバーの高さを保存しておく
    menubarHeight = static_cast<GLsizei>(ImGui::GetWindowHeight());

    // メインメニューバー終了
    ImGui::EndMainMenuBar();
  }

  // コントロールパネル
  if (showControlPanel)
  {
    // ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(2.0f, 2.0f + menubarHeight), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(231, 640), ImGuiCond_Once);
    ImGui::Begin(u8"コントロールパネル", &showControlPanel);

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

    // 較正関連項目
    ImGui::Separator();

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
          calibration.setDictionary(settings.dictionaryName);
        }

        // この選択を次にコンボボックスを開いたときのデフォルトにしておく
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // ChArUco Board の大きさ
    if (ImGui::InputFloat2(u8"ボード長", settings.boardSize.data(), "%.2f cm"))
    {
      // ChArUco Board を作り直す
      calibration.createBoard(settings.boardSize);
    }

    // ArUco Marker の検出
    ImGui::Checkbox(u8"マーカ検出", &detectMarker);

    // ArUco Marker を検出するなら
    if (detectMarker)
    {
      // ChArUco Board の検出
      ImGui::SameLine();
      ImGui::Checkbox(u8"ボード検出", &detectBoard);
    }
    else
    {
      // ArUco Marker を検出していなければ ChArUco Board は検出しない
      detectBoard = false;
    }

    // 「標本」ボタンをクリックしたとき ChArUco Board の検出中なら
    if (ImGui::Button(u8"標本") && detectBoard) calibration.extractSample();

    // １つでも標本を取得していれば
    if (calibration.getSampleCount() > 0)
    {
      // 標本の「消去」ボタンを表示する
      ImGui::SameLine();
      if (ImGui::Button(u8"消去")) calibration.discardSamples();

      // 標本を６つ以上取得していれば
      if (calibration.getSampleCount() >= 6)
      {
        // 「較正」ボタンを表示する
        ImGui::SameLine();
        if (ImGui::Button(u8"較正")) repError = calibration.calibrate();

        // 較正が完了していれば
        if (calibration.finished())
        {
          // 「完了」を表示する
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.0f, 1.0f), "%s", u8"完了");
        }
      }
    }

    // 装置関連項目
    ImGui::Separator();
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
        // キャプチャデバイスを開いてから
        openDevice();

        // キャプチャスレッドを動かす
        capture.start();
      }
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "%s", u8"停止中");
    }
    ImGui::End();
  }

  // 情報パネル
  if (showInformationPanel)
  {
    // ウィンドウの位置とサイズ
    ImGui::SetNextWindowPos(ImVec2(235.0f, 2.0f + menubarHeight), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(214, 134), ImGuiCond_Once);
    ImGui::Begin(u8"情報", &showInformationPanel);

    // フレームレートの表示
    ImGui::Text("Frame rate: %6.2f fps", ImGui::GetIO().Framerate);

    // 検出数の表示
    ImGui::Text("Detected corners: %d", calibration.getCornersCount());

    // 標本数の表示
    ImGui::Text("Sampled corners: %d", calibration.getSampleCount());

    // 再投影誤差の表示
    ImGui::Text("Reprojection error: %.6f", repError);

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

  // ImGui のフレームに描画する
  ImGui::Render();
}

//
// 画像の保存
//
void Menu::saveImage(const cv::Mat& image, const std::string& filename)
{
  // 画像ファイル名のフィルタ
  constexpr nfdfilteritem_t imageFilter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };

  // ファイルダイアログから得るパス
  nfdchar_t* filepath;

  // ファイルダイアログを開く
  if (NFD_SaveDialog(&filepath, imageFilter, 1, NULL, filename.c_str()) == NFD_OKAY)
  {
    // ファイルに保存する
    if (!cv::imwrite(filepath, image)) errorMessage = u8"ファイルが保存できませんでした";

    // ファイルパスの取り出しに使ったメモリを開放する
    NFD_FreePath(filepath);
  }
}
