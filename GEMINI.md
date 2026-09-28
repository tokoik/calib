# プロジェクト開発方針と環境定義 (GEMINI.md)

本ドキュメントは、`calib` の設計方針、実装上維持すべき条件、および開発・動作環境を定義します。変更の経緯は [REQUESTS.md](REQUESTS.md) に記録します。

## 1. 開発環境

- **統合開発環境**: Visual Studio 2022 以降（Windows）、VS Code / Clang（macOS）、VS Code / GCC（Linux）、Android Studio（Android）
- **開発言語**: C++17
- **ターゲット**: 64 bit（x64 / arm64 / aarch64）
- **ビルドシステム**: CMake 3.13 以降。生成物はソース直下へ置かず、out-of-source build とします。
- **外部依存ライブラリ**: OpenCV、GLFW、Dear ImGui、Native File Dialog Extended 等は CMake がプロジェクト直下の `libs` へ自動取得します。`libs` を別ディレクトリへのジャンクションにしません。インクルードパス、ライブラリパス、DLL コピー、Visual Studio のデバッグ環境（作業ディレクトリ、`PATH`）は `CMakeLists.txt` に集約します。
- **文字コード**:
  - C++ / Objective-C++ ソース（`.h`, `.cpp`, `.mm`）: UTF-8、**BOM 付き**
  - GLSL ソース（`.vert`, `.frag`, `.comp`）: UTF-8、**BOM なし**
  - pdfLaTeX（CJKutf8）で Doxygen の PDF を生成できるよう、コメントに丸数字・特殊引用符・商標記号などの特殊 Unicode 文字を使いません。

## 2. 入力とキャプチャ

### 2.1 入力バックエンド

| 入力 | クラス | 備考 |
| --- | --- | --- |
| Windows のカメラ | `CamMf` | Media Foundation Source Reader + MFT デコーダ／カラーコンバータ |
| macOS のカメラ | `CamAvf` | AV Foundation、OS 側で BGRA 変換（第 8 節） |
| Android のカメラ | `CamAndroid` | Camera2 NDK、RGBA_8888（第 7 節） |
| Raspberry Pi のカメラ | `CamLibcam` | libcamera（第 5 節） |
| その他のカメラ・動画・ネットワーク | `CamCv` | OpenCV（Linux では `cv::CAP_V4L2`） |
| 静止画像 | `CamImage` | OpenCV |

- `cv::VideoCapture` による性能低下やデバイス情報の欠落を避けるため、Windows・macOS・Android のカメラ入力は OpenCV から独立させます。
- GStreamer パイプライン入力はサポート対象外とし、構成ファイルや UI に GStreamer 固有の設定を追加しません。
- `Capture` はプラットフォームのネイティブカメラ実装を `NativeCamera` 型として一つだけ選び、`openDevice()` と `updateFormatList()` の処理を共通化します。

### 2.2 `Camera` 基底クラス

- `Camera.h` は純粋なフレーム取得レイヤとし、OpenGL（`gg.h`）、OpenCV、GLFW に依存せず標準 C++ ライブラリだけで構成します。
- NVI（Non-Virtual Interface）パターンを採用します。公開関数 `start()`, `stop()`, `close()` が排他制御、スレッド状態フラグ `running`、スレッド合流を一元管理し、派生クラスは保護フック `onStart()`, `onStop()`, `onClose()` にハードウェア固有処理だけを実装します。
  - `stop()` は `running` を `exchange(false)` で一度だけ遷移させ、停止処理と `join()` の重複を防ぎます。
  - `close()` は内部で `stop()` を呼ぶため、呼び出し側で `stop()` と `close()` を続けて呼びません。
  - 基底クラスのデストラクタでは純粋仮想関数を呼べないため、`~Camera()` は `default` とし、各派生クラスのデストラクタで `close()` を呼びます。
- フレームは単一バッファ `std::vector<std::uint8_t> image` に保持します。
- 上位層へのフレーム転送はテンプレート関数 `lockFrame(F&& func)` によるコールバック方式とし、非ブロッキングロック（`try_to_lock`）に成功したときだけデータを渡して、PBO への直接転送や `cv::Mat` へのコピーを行います。
- 上位層は `dynamic_cast` を使わず、`isStillImage()`, `getFormatList()`, `selectFormat()`, `setPrioritizeLatency()` 等の仮想関数を介して連携します。

### 2.3 フォーマット選択と遅延初期化

- デバイス認識時やフォーマット一覧取得時には重い初期化（デコーダ生成、セッション開始）を行わず、「開始」を指示したときに選択フォーマットを適用します。
- バックエンドが持つ解像度・fps・コーデックは `CaptureFormat` 構造体で `Menu` へ渡し、表示用に連結した文字列を再解析して制御データへ戻す設計にしません。
- フォーマットが未選択のときは `findDefaultFormat()` が 1280 x 720 を優先し、なければ画素数の近いもの（1920 x 1080 超は優先度を下げる）を選びます。この選択は `Menu::updateFormatDropdowns()` だけで行います。
- 解像度・コマ数・符号化方式のドロップダウンは `Menu::selectFormatItem()` で実在する組み合わせへ同期します。

### 2.4 フレーム処理モードとスレッド安全性

- 「レイテンシ優先（古いフレームを破棄して常に最新フレームを使う）」と「全フレーム処理（取得前のフレームを上書きしない）」を実行時に切り替えられるようにします。
- `CamMf` のレイテンシ優先時は同期キャプチャループを最高速で回し、MFT デコーダの `CODECAPI_AVLowLatencyMode` で内部バッファリングを抑制します。
- スレッド間で共有する状態（`prioritizeLatency`, `running`, `captured` 等）は `std::atomic`、mutex、condition variable を用途に応じて使い、データレースと待機漏れを防ぎます。
- COM オブジェクト、MFT バッファ、OpenGL オブジェクト、libcamera の要求オブジェクト等は RAII または明示的な対称処理で解放し、早期 return や `continue` でも漏らしません。
- バッファ間のコピー長は入力と出力の実容量から `std::min` 等で決め、境界を越えないようにします。

## 3. 状態管理とクラス境界

- **投影方式と入力初期化の分離**:
  - 投影方式を選択したときは、その方式固有の画角と中心位置を `Preference` から `Intrinsics` へ反映します（`Menu::selectPreference()`）。
  - カメラ、動画、静止画像を開いたときは、実解像度と現在の焦点距離から見やすい初期画角を計算し、中心位置は維持します（`Menu::initializeInputIntrinsics()`）。
  - 両者の状態遷移を混在させません。
- **キャプチャ開始の一元化**: UI からのキャプチャ開始は `Menu::startCapture()` を使い、デバイスのオープン、フォーマット適用、初期画角の設定は `Menu::openDevice()` に集約します。失敗した時点で後続処理を中止し、原因に対応するエラーを通知します。
- **構成ファイルの安全な再読込**: 一時領域へ読み込み、必須項目と型を検証し、全処理が成功した場合だけ現在の構成と置き換えます。実行中の再読込では、新しい投影方式のシェーダー構築と、選択番号、`Intrinsics`、較正設定の再同期を行います。部分的に更新された状態や、未初期化のシェーダーを参照できる状態を作りません。
- **明示的な公開 API とカプセル化**:
  - `const_cast` や `friend` でクラスの不変条件を迂回せず、`getSettings()`, `setSettings()` 等の目的が分かる公開 API を用意します。
  - メンバ変数の初期値はコンストラクタの初期化子リストではなく、クラス定義内のデフォルトメンバ初期化子に集約します。静的メンバの定義は、そのクラスの実装ファイルに置きます。
- **`mfcapture` との共通化**: 共通する変数名・関数名は `mfcapture` の命名に、コメントおよび Doxygen の表現は `calib` に統一します。`Camera`、`Capture`、各 `Cam*` クラス、`Buffer`、`Texture`、`Framebuffer`、`Intrinsics`、`gg`、`GgApp` などの共通ファイルは両プロジェクトで同一内容に保ちます。

## 4. 較正処理

- **ChArUco Board の設定**: 辞書、マス目数（横・縦）、マス目長・マーカー長は `Settings` と `Config` で一元管理します。変更時は `Calibration::createBoard()` または `Calibration::setDictionary()` で検出器を再構築します。
- **較正ファイル**: 較正結果にはカメラ行列、歪み係数に加えて較正時の入力解像度を記録します。保存時の既定ファイル名は日時を含む `calibYYYYMMDDhhmm.json` とします。
- **自動キャリブレーション（静止検知と幾何多様性）**:
  - 前フレームとの共通コーナー変位（閾値 2.0 px）を追跡し、ブレのない状態が指定秒数続いたときだけ静止と判定します。
  - `cv::moments` から求めた重心位置、慣性半径（スケール）、主軸角度を直前の記録と比較し、平行移動・距離・傾きのいずれかに十分な差があるときだけ記録します。
  - 記録直後は約 1.5 秒のクールダウンを設け、音（Windows: `Beep()`、その他のデスクトップ: ターミナルベル `\a`）で完了を知らせます。

## 5. UI と描画

- `Menu::draw()` は一フレーム内の描画順序だけを管理し、メニューバー、入力パネル、較正パネル、エラーダイアログはそれぞれ独立した描画関数に分けます。
- UI ウィジェットの描画と、投影方式変更やキャプチャ開始などの状態遷移を分離します。複数の UI から同じ状態遷移が必要な場合は共通の補助関数を呼びます。
- プラットフォーム固有処理は `Capture` と各 `Cam*` クラスに閉じ込め、`Menu` 内の `#if` 範囲を拡大しません。
- 画像の画面配置は投影方式の `Intrinsics` を変更せず、表示矩形の頂点スケールとして実装します（contain 方式）。倍率は論理ウィンドウサイズではなく実 Framebuffer サイズで計算します。
- CPU/OpenCV 側では画像を BGRA で保持するため、最終表示は `draw.frag` で RGBA へ変換します。`GL_TEXTURE_SWIZZLE_RGBA` が反映されない `glBlitFramebuffer()` を最終表示に使いません。
- メインループでは、新しいフレームを取得できたときだけ PBO からテクスチャへ転送します。
- ImGui のフォントには日本語グリフに加えて一般句読点（`0x2000-0x206F`）、文字様記号（`0x2100-0x214F`）、矢印（`0x2190-0x21FF`）、囲み英数字（`0x2460-0x24FF`）、幾何学模様（`0x25A0-0x25FF`）を登録します。カメラ名は `CamMf` / `CamAvf` の `sanitizeDeviceName()` で制御文字の置換、特殊引用符の ASCII 化、4 バイト文字の除去を行います。

## 6. Raspberry Pi / Linux 対応

- **OpenGL ES 3.1**: CMake オプション `USE_GLES` を提供し、ARM 環境（`arm|aarch64`）では既定値を `ON` とします。
- **シェーダーの一元管理**: シェーダーファイルは `#version 330` で記述し、`GL_GLES_PROTOTYPES` 有効時は `ggCreateShader()` が `#version 310 es` への置換と精度修飾子の付与を行います。GLES 用の複製を作りません。
- **カメラ入力**: `CamLibcam`（libcamera）と `cv::CAP_V4L2` を提供します。`/sys/class/video4linux` を走査して、SoC 内部処理ノード（bcm2835-codec, bcm2835-isp, pisp 等）を除いたカメラを選択肢にします。
- **アセット配置**: POST_BUILD コマンドでシェーダー、構成ファイル、画像を実行ファイルのディレクトリへ配置します。

## 7. Android 対応

- **UI と描画**: UI は Jetpack Compose（`MainActivity.kt`）とし、ネイティブ層は EGL / OpenGL ES / ImGui に依存しません。`NativeBridge.cpp` は `SurfaceView` の `ANativeWindow` へ `ANativeWindow_lock` / `ANativeWindow_unlockAndPost` で直接描画し、サーフェスの再生成やサイズ変更時はバッファジオメトリを再設定して縦横比を保った中央配置にします。
- **エントリポイント**: Android 版の起動・フレーム処理は `NativeBridge.cpp` が担い、デスクトップ用の `calib.cpp` と `main.cpp` はビルドしません。デスクトップ専用コードは `#if !defined(__ANDROID__)` で分離します。
- **カメラ入力**: `CamAndroid` が Camera2 NDK（`ACameraManager`, `ACameraDevice`, `ACaptureRequest`, `AImageReader` 等）で `AIMAGE_FORMAT_RGBA_8888` のフレームを取得します。解像度はデスクトップと同じ `CaptureFormat` と `Menu::selectResolution()` で選択します。
- **UI 状態連携**: 較正パラメータ（マス目数、マス目長、マーカー長）、手動／自動記録、較正の実行、設定・較正データの保存と読み込みは JNI を介して C++ 側と同期します。マス目長を変更したとき、マーカー長がマス目長以上になる場合はマス目長の 1/2 にします。
- **ファイルとアセット**: 入出力はアプリ内部ストレージ（`context.filesDir`）を起点とし、APK の `assets/` に同梱した構成 JSON や初期画像は起動時に `AAssetManager` で展開します。
- **ビルド**: `android/` の Gradle プロジェクトからトップレベルの `CMakeLists.txt` を参照します。OpenCV Android SDK 4.11.0 を自動取得し、`OpenCV_LIBS`, `android`, `log`, `camera2ndk`, `mediandk` だけをリンクします。

## 8. macOS 対応

- **カメラ入力**: `CamAvf` が `AVCaptureDeviceDiscoverySession` でデバイスを列挙し、`AVCaptureDeviceFormat` から解像度・最大フレームレート・コーデックを `CaptureFormat` に集約します。`AVCaptureVideoDataOutput` で `kCVPixelFormatType_32BGRA` を指定し、行パディングを考慮して単一バッファへ格納します。`alwaysDiscardsLateVideoFrames` でレイテンシ優先を切り替え、停止時は `dispatch_sync` でデリゲートキューの処理完了を待ちます。
- **フレームワーク**: `CMakeLists.txt` で `AVFoundation` と `CoreMedia` をリンクします。
- **Homebrew 非依存の自己完結ビルド**: OpenCV のビルド設定で外部依存（Protobuf, FFmpeg, GStreamer, VTK, OpenEXR, libavif, Eigen, OpenJPEG, JasPer, Qt, TBB, IPP 等）の探索を無効化し、組み込み 3rdparty ライブラリ（ZLIB, JPEG, PNG, TIFF, WEBP）を強制します。

## 9. OpenXR 対応

- OpenXR の初期化、セッション管理、フレーム同期、スワップチェーン管理は `GgApp::OpenXR` に一元化し、独自クラスを作りません。
- `calib.cpp` は `--openxr` 指定時だけ `GgApp::OpenXR::initialize()` を呼び、利用できないときはデスクトップ表示を維持します。
- HMD の回転姿勢は `Menu::setup(aspect, viewPose)` で展開シェーダーのモデル変換へ合成します。入力は単眼画像として扱い、視差は付けません。
- CMake オプション `GG_ENABLE_OPENXR`（既定値 `OFF`）で OpenXR SDK 1.1.61 の取得と静的ローダーのリンクを行います。

## 10. コメントと Doxygen

- コメントにはコードの言い換えではなく、「何のために」「どの状態を保つために」処理するかを書きます。
- 状態同期、所有権、リソース寿命、早期終了、プラットフォーム差分など、コードだけでは意図が分かりにくいブロックに実装コメントを付けます。
- 公開型と公開関数、および複数の状態を更新する重要な非公開関数に Doxygen コメントを付け、`@param` は宣言の引数名と一致させ、戻り値がある関数には `@return` を書きます。
- 実装を変更したときは、コメント、Doxygen、README、必要なら REQUESTS を同時に更新します。
- ソースコードに関する Doxygen 警告（引数名の不一致、未記載引数、未知のコマンド）を残しません。

## 11. 検証方針

- Windows の Debug と Release の両構成をビルドします。
- 共通ファイルや `Menu`、`Capture` を変更したときは、Android の Debug APK（`android\gradlew.bat assembleDebug`）もビルドします。
- `git diff --check` で差分の空白エラーを確認します。
- Doxygen を実行し、ソースコードのコメント警告がないことを確認します。
- C++ ソースの BOM 付き UTF-8、GLSL ソースの BOM なし UTF-8 を確認します。
