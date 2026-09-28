# calib

## 概要

本プログラムは、Webカメラ、動画ファイル、静止画像から映像を取得し、選択した投影方式で画像を展開しながら、ArUco Markerの検出とChArUco Boardを用いたカメラ較正を行うC++アプリケーションです。

カメラ入力には、Windowsでは Microsoft Media Foundation (`CamMf`)、macOSでは AV Foundation (`CamAvf`)、Raspberry Pi では `libcamera` (`CamLibcam`)、Android では Camera2 NDK (`CamAndroid`) を直接使用し、カメラデバイスの一覧や特性（解像度・フレームレート・コーデック）をシステムから取得して制御します。その他の動画・静止画像の入力にはOpenCVを使用します。デスクトップおよびRaspberry Piの描画とUIにはOpenGL / OpenGL ES 3.1、GLFW、Dear ImGuiを使用し、Android版は Jetpack Compose のUIと `ANativeWindow` へのCPU直接描画により、OpenGLやImGuiに依存しない構成としています。

## 主な機能

- Webカメラ、動画ファイル、静止画像からの入力
- Orthographic、Equirectangular、Equidistance、Stereographicなどの投影方式による展開
- 投影方式ごとの画角、主点、既定解像度の管理
- 各プラットフォームのネイティブAPIによるカメラ入力と、解像度・フレームレート・コーデックの組み合わせ選択
- 全フレーム処理とレイテンシ優先処理の切り替え
- ArUco Marker検出、ChArUco Board検出、較正パラメータの保存・読込（較正時の解像度も記録）
- 静止検知と幾何多様性判定による自動キャリブレーション（一人での較正作業を支援）
- JSON構成ファイルによる投影方式、表示、較正設定の管理
- OpenXRによるHMD表示（Windows、オプション）

## プログラムの処理の流れ

デスクトップ版の`calib.cpp`のメインループでは、概ね次の順序で一フレームを処理します（Android版では`NativeBridge.cpp`がフレーム取得、検出、描画を行います）。

1. `Menu`がUIを描画し、投影方式、入力、較正設定を更新する。
2. `Capture`が現在の入力源から新しいフレームを取得できたら、OpenGLテクスチャへ転送する。
3. `Menu::setup()`が選択中の`Preference`に対応する展開シェーダーを設定する。
4. `Framebuffer`が投影変換後の画像を生成する。
5. 必要に応じて変換後の画像をCPU側へ戻し、`Calibration`がマーカーまたはボードを検出する。
6. 最終画像とImGuiのUIをデスクトップ画面へ描画する。
7. OpenXRが有効なら、各viewのorientationを姿勢へ合成して画像を再展開し、HMDのswapchainへ描画する。

投影方式を変更すると、その方式に保存された画角と中心位置が`Intrinsics`へ反映されます。一方、入力源やビデオフォーマットを変更した直後は、実際のフレーム解像度と現在の焦点距離から、その入力を見やすく表示する初期画角を計算します。このとき中心位置は維持され、投影方式を選び直すと画角もその方式の設定値へ戻ります。

最終画像は、実Framebufferの縦横比に応じて表示矩形の頂点を縮小し、幅または高さの一方を表示領域いっぱいに合わせます。テクスチャ座標は常に画像全体を参照し、`draw.frag`でCPU/OpenCV側のBGRA配列を画面表示用のRGBAへ変換します。

## 主要クラスと責務

### `Camera`と入力実装

`Camera`はキャプチャスレッド、フレームバッファ、排他制御、レイテンシ優先フラグを管理する基底クラスです。OpenGLやOpenCVに依存せず、標準C++ライブラリだけで構成されています。

- **NVI (Non-Virtual Interface) パターン**: 公開関数 `start()`, `stop()`, `close()` がスレッドの状態管理、排他制御、スレッド合流を一元管理し、派生クラスは保護フック `onStart()`, `onStop()`, `onClose()` にハードウェア固有の処理だけを実装します。
- **単一バッファとゼロコピー転送**: フレームはCPUメモリ上の単一バッファ（`std::vector<std::uint8_t>`）に保持し、`lockFrame(F&& func)` が非ブロッキングロックに成功したときだけデータをコールバックへ渡して、PBOへの直接転送や`cv::Mat`へのコピーを行います。
- **仮想関数による疎結合**: 上位層は `isStillImage()`, `getFormatList()`, `selectFormat()` などの仮想関数を通じて入力を操作し、派生クラスへのダウンキャストを行いません。

| クラス | 役割 |
| --- | --- |
| `CamMf` | Windows Media Foundationによるカメラ入力 |
| `CamAvf` | macOS AV Foundationによるカメラ入力 |
| `CamAndroid` | Android Camera2 NDKによるカメラ入力 |
| `CamLibcam` | Raspberry Pi の libcamera によるカメラ入力 |
| `CamCv` | OpenCVによるカメラ、動画、ネットワーク入力 |
| `CamImage` | 静止画像入力 |
| `Capture` | 上記入力実装の所有、切り替え、開始・停止を行う窓口 |

ネイティブAPIのフォーマット列挙結果は`CaptureFormat`として構造化され、解像度、fps、コーデック、選択番号を`Menu`へ渡します。UIはプラットフォーム固有型や表示文字列の解析に依存しません。

### `Preference`、`Intrinsics`、`Config`

- `Preference`: 一つの投影方式について、説明、シェーダーファイル、固有の`Intrinsics`を保持する。
- `Intrinsics`: 画角、中心位置、解像度、フレームレートを保持する。
- `Config`: JSON構成ファイルの読込・保存、投影方式一覧、表示設定、較正設定を管理する。

構成ファイルの再読込では、一時領域へ全内容を読み込んで検証し、成功した場合だけ現在の構成を置き換えます。実行中に追加された投影方式についても、置換前に展開シェーダーを構築します。

### `Menu`

`Menu`はImGuiによる操作画面と、UI操作を各機能へ伝える処理を担当します。描画処理は次の単位に分離されています。

- `draw()`: 一フレーム分の描画順序の管理
- `drawMainMenuBar()`: ファイル操作とパネル表示
- `drawInputPanel()`: 投影方式、姿勢、焦点距離、入力源、フォーマット、開始・停止
- `drawCalibrationPanel()`: 辞書、検出モード、標本取得、自動キャプチャ、較正
- `drawErrorDialog()`: エラーメッセージ表示

投影方式の同期は`selectPreference()`、キャプチャ開始は`startCapture()`、フォーマットの選択肢の同期は`updateFormatDropdowns()`と`selectFormatItem()`に集約しています。

### `Calibration`

ArUco MarkerとChArUco Boardの検出、較正用コーナーの記録、自動キャプチャのための静止検知と幾何多様性の判定、カメラ行列と歪み係数の計算、較正パラメータの読込・保存を担当します。

## 低遅延キャプチャ

### Windows (`CamMf`)

Media Foundation Source Readerから圧縮フレームを取得し、MFTデコーダとカラーコンバータでCPUメモリ上のRGB画像へ変換します。

- フォーマット列挙時にはデコーダを作成せず、「開始」時に選択フォーマットを適用する遅延初期化を行う。
- `CODECAPI_AVLowLatencyMode`を設定し、デコーダ内部のバッファリングを抑制する。
- レイテンシ優先時は古いフレームを待たず、常に新しいフレームを共有バッファへ反映する。全フレーム処理時は、メインスレッドが取得するまで次のフレームで上書きしない。

専用プレイヤーが使用するDXVAとGPU上のZero-copy描画に比べると、CPUメモリへの展開とOpenGLへの転送分の遅延は残ります。

### macOS (`CamAvf`)

- `AVCaptureDeviceDiscoverySession` で接続されたカメラ（内蔵カメラ、USB Webカメラ、連係カメラ等）を列挙し、`AVCaptureDeviceFormat` から解像度、最大フレームレート、コーデックを取り出します。
- キャプチャ開始時にだけフォーマット適用とセッション初期化を行います。
- 出力フォーマットに `kCVPixelFormatType_32BGRA` を指定してOS側でBGRAへ変換し、行パディングを考慮して単一バッファへ格納します。
- `alwaysDiscardsLateVideoFrames` により、レイテンシ優先時は遅れたフレームを破棄します。

詳細は [docs/CamMf.md](docs/CamMf.md)、[docs/CamAvf.md](docs/CamAvf.md) を参照してください。

## OpenXR 対応

- OpenXR バックエンドは既定では無効です。Windowsで使用する場合は CMake の構成時に `-DGG_ENABLE_OPENXR=ON` を指定してください。
- OpenXR ランタイムと HMD を使用する場合は、コマンドラインに `--openxr` を指定してください。初期化できない場合はデスクトップ表示だけで動作を継続します。
- HMD表示では各viewの向きを画像展開へ反映します。入力を単眼画像として扱うため、眼の位置による視差は付けません。
- API、フレーム処理、リソース管理については [OpenXRマニュアル](docs/OpenXR.md) を参照してください。

## 基本操作

1. 「入力」パネルで投影方式を選択する。
2. 必要に応じて画角、中心、姿勢、焦点距離を調整する。
3. カメラ装置を選択する。
4. WindowsおよびmacOSでは解像度、コマ数、符号化方式を選択する（未選択時は1280×720に近いものが選ばれる）。Android版では解像度を選択できる。
5. 必要に応じて「レイテンシ優先」を有効にする。
6. 「開始」を押してキャプチャを開始する。

較正する場合は、「較正」パネルで辞書、ChArUco Boardのマス目数（横・縦）および寸法を設定し、ChArUco Board検出を有効にして標本を取得します。6標本以上取得すると較正を実行できます。較正結果は「ファイル」メニューの「較正ファイルを保存」で、既定名 `calibYYYYMMDDhhmm.json` として保存できます。

一人で較正作業を行う場合は「自動キャプチャ」を有効にすると、カメラの前でボードを静止させるだけで、ブレのない安定状態が自動判定され、十分な幾何変化（重心位置・距離・傾き）が確認された瞬間に標本が自動記録されます。記録直後は姿勢変更のためのクールダウンに入り、合図音（Windows: ビープ音、その他: ターミナルベル）が鳴ります。手動の「取得」ボタンやスペースキーでの記録も併用できます。

## 構成ファイル

既定の構成ファイルは`calib_config.json`です。主な項目は次のとおりです。

- ウィンドウサイズと背景色
- 展開メッシュのサンプル数
- カメラ姿勢、焦点距離、焦点距離範囲
- ArUco辞書とChArUco Boardマス目数・寸法
- 初期表示画像
- 投影方式ごとの説明、画角、中心、解像度、fps、シェーダー

構成ファイルを保存すると、UIで変更した`Settings`が保存対象へ反映されます。投影方式ごとの固有パラメータは`Preference`が管理します。

## 開発環境とビルド

- C++17
- CMake 3.13以降
- Visual Studio 2022以降（Windows、x64）
- Clang / Xcode Command Line Tools（macOS、arm64 / x64）
- OpenCV 4.13.0（Android は OpenCV Android SDK 4.11.0）
- GLFW 3.4
- Dear ImGui 1.92.8
- Native File Dialog Extended 1.3.0

### Windows

```powershell
cmake -S . -B build
cmake --build build --config Debug
cmake --build build --config Release
```

初回のCMake構成時には、`CMakeLists.txt`が必要な依存ライブラリを`libs`以下へ取得します。ビルド後は、シェーダー、構成ファイル、画像、フォント、OpenCV DLLが実行ファイルのディレクトリへコピーされます。

### macOS

Xcode Command Line Tools（`xcode-select --install`）と CMake があれば、Homebrew などの外部パッケージマネージャに依存せずビルドできます。依存ライブラリは CMake が `libs` へ取得・ビルドし、`AVFoundation` と `CoreMedia` フレームワークを自動的にリンクします。

```bash
cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)
./build/calib
```

### Raspberry Pi (Linux ARM)

Raspberry Pi OS (Bookworm / Bullseye) では、標準のパッケージマネージャから開発パッケージを導入してビルドします。ARM環境では OpenGL ES 3.1 (`USE_GLES`) と libcamera バックエンド (`USE_LIBCAMERA`) が既定で有効になります。

```bash
sudo apt update
sudo apt install -y build-essential cmake libopencv-dev libglfw3-dev libgtk-3-dev libgles2-mesa-dev libegl1-mesa-dev libcamera-dev
cmake -B build
cmake --build build -j$(nproc)
./build/calib
```

Raspberry Pi Camera Module を V4L2 経由で使用する場合は `libcamerify ./build/calib` で起動します。macOS と Linux でも、ビルド後にシェーダー、構成ファイル、画像が `build/` へコピーされます。

### Android

Android Studio で `android` フォルダを開いてビルド・実行します。実機の事前設定、カメラ権限、Logcat によるデバッグ手順は [Android 実機テストガイド](docs/Android.md) を参照してください。コマンドラインでは次のようにビルドします。

```powershell
cd android
.\gradlew.bat assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## 開発時の確認事項

設計方針と検証方針の詳細は [GEMINI.md](GEMINI.md) にまとめています。特に次の点に注意してください。

- 入力を開いた直後は実解像度と焦点距離に基づく初期画角となり、投影方式を選び直すとその方式の画角と中心位置へ戻ること。
- フォーマット選択が、解像度・fps・コーデックの実在する組み合わせを指していること。
- プラットフォーム固有処理を `Capture` と各 `Cam*` クラスに閉じ込め、`Menu` に固有型を露出させないこと。
- 構成再読込に失敗した場合、現在の構成が部分的に変更されないこと。
- `mfcapture` と共通のファイルは同一内容に保つこと。
- C++ ソースは BOM 付き UTF-8、GLSL ソースは BOM なし UTF-8 とすること。
- コメントとDoxygenを実装変更と同時に更新すること。

## ドキュメント・関連資料

### 開発・管理ドキュメント

- [GEMINI.md](GEMINI.md): プロジェクト開発方針と環境定義
- [REQUESTS.md](REQUESTS.md): 変更要求と対応履歴

### プラットフォーム・機能別ガイド

- [docs/OpenXR.md](docs/OpenXR.md): OpenXR バックエンド実装マニュアル
- [docs/Android.md](docs/Android.md): Android 実機テストとビルドガイド
- [docs/CamMf.md](docs/CamMf.md): Windows Media Foundation ビデオキャプチャクラス `CamMf` の解説
- [docs/CamAvf.md](docs/CamAvf.md): macOS AV Foundation ビデオキャプチャクラス `CamAvf` の解説
- [docs/CamAndroid.md](docs/CamAndroid.md): Android Camera2 NDK ビデオキャプチャクラス `CamAndroid` の解説
- [docs/CamLibcam.md](docs/CamLibcam.md): Raspberry Pi ネイティブカメラキャプチャクラス `CamLibcam` の解説

### 勉強会資料

- [docs/seminar/calib_seminar_handout.md](docs/seminar/calib_seminar_handout.md): 勉強会ハンドアウト
- [docs/seminar/calib_seminar_slides.html](docs/seminar/calib_seminar_slides.html): 勉強会スライド (HTML)
