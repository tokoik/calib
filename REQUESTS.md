# 作業指示および開発履歴

## 概要

本プロジェクトは、RICOH THETA V などの H.264 出力 Web カメラで `cv::VideoCapture` のキャプチャ・デコードが遅い（低フレームレート、数秒の遅延）問題を解消するため、Microsoft Media Foundation を直接利用するキャプチャモジュール `CamMf` を開発したところから始まった。その後、入力部の再設計、macOS・Raspberry Pi・Android・OpenXR への対応、自動キャリブレーションなどを追加している。現在の設計方針は [GEMINI.md](GEMINI.md) にまとめている。

## 作業履歴

### 1. OpenCV 依存の排除と Media Foundation 化

- **指示**: `cv::VideoCapture` によるキャプチャ・デコードが遅いため、Media Foundation で自前でキャプチャし、OpenCV に依存しない `CamMf` クラスを実装する。
- **対応**: `Camera.h` から OpenCV の依存を排除し、フレーム転送処理をテンプレート化した。

### 2. MFT による H.264 デコード

- **指示**: H.264 などの圧縮フォーマットから RGB32 への変換を高速化するため、MFT のデコーダとカラーコンバータを手動で構築する。
- **対応**: `CamMf` に H.264 デコーダとカラーコンバータを組み込み、CPU メモリへ直接出力するようにした。

### 3. 初期化遅延（フリーズ）の回避

- **指示**: カメラ認識時やフォーマット一覧取得時にデコーダが初期化され、数秒のフリーズやエラーが発生する問題を解決する。
- **対応**: 一覧取得時にはフォーマットを適用せず、「開始」時にだけ適用する遅延初期化を導入した。フォーマット選択 UI を解像度・コマ数・符号化方式の 3 段階のドロップダウンにした。

### 4. MFT バッファ管理と低遅延化

- **指示**: キャプチャ画像に 1～2 秒の遅延が発生する問題を解決する。
- **対応**: `MFT_OUTPUT_DATA_BUFFER` の扱いを修正して `E_FAIL` を回避し、`ICodecAPI` で `CODECAPI_AVLowLatencyMode` を設定した。

### 5. レイテンシ優先モードの導入

- **指示**: 古いフレームを捨てて常に最新フレームを使うモードを追加し、全フレーム処理と実行時に切り替えられるようにする。
- **対応**: `prioritizeLatency` フラグと UI を追加した。`CODECAPI_AVLowLatencyMode` を正しい型（`VT_UI4`）で設定し、`0x8007007A`（バッファ長の上書きによる表示停止）と `0x80070057`（属性取得エラー）を修正した。
- **結果**: 実用上十分な低遅延で滑らかに動作するようになった。MPC-HC（ハードウェアデコード + Zero-copy 描画）と比べると CPU メモリ展開分の数ミリ秒の差は残るが、現在の構成での到達点として完了とした。

### 6. コードのクリーンアップと堅牢性の向上

- **指示**: 冗長な記述を整理し、スレッド競合、メモリリーク、バッファオーバーフローの危険性を解消する。
- **対応**: MFT バッファ生成処理の集約、共有フラグの `std::atomic` 化、`enumerateFormats()` の COM オブジェクトのリーク修正（RAII 導入）、フレーム転送時のコピー長のクランプを行った。

### 7. 依存ライブラリ管理とビルド環境の更新

- **指示**: `libs` をジャンクションから CMake による自動ダウンロードへ変更し、Visual Studio 2022 以降を対象とする。
- **対応**: `CMakeLists.txt` と文書を更新した。

### 8. キャプチャ開始時に投影方式固有のパラメータが失われる問題の修正

- **原因**: `Menu::setSize()` が入力解像度だけでなく、投影方式の画角と中心位置まで汎用値で上書きしていた。
- **対応**: `setSize()` は入力解像度だけを更新するようにした（画角の扱いは第 12 項で再度変更）。

### 9. `Menu` を中心とした状態管理とクラス境界の整理

- **指示**: 度重なる修正で複雑化した `Menu.cpp` とクラス構造を点検し、改善する。
- **対応**:
  - `Config::load()` を、一時領域へ読み込んで検証後に置き換えるトランザクション型に変更し、再読込時の `preferenceList` への追記や、`Preference` のデストラクターによる共有シェーダーの消去を修正した。
  - 投影方式の同期を `Menu::selectPreference()` へ、キャプチャ開始を `Menu::startCapture()` へ集約した。
  - フォーマット情報を `CaptureFormat` として構造化し、表示文字列の再解析を廃止した。
  - `Menu::draw()` を描画関数へ分割し、`const_cast` と `friend class Menu` を廃止して公開 API へ置き換えた。

### 10. GStreamer 対応の廃止

- **対応**: `cv::CAP_GSTREAMER`、GStreamer パイプラインの入力処理、構成項目、文書の記述を削除した。

### 11. 入力画像の表示領域への自動フィット

- **指示**: 画像、動画、カメラを開いたとき、縦横比を維持して表示領域に収まる最大サイズで表示する。
- **対応**: 投影方式の `Intrinsics` と画面配置を分離し、`Texture::draw()` で表示矩形の頂点スケールを計算する contain 表示にした。BGRA から RGBA への変換は `draw.frag` で行い、HiDPI でも正しいサイズになるよう実 Framebuffer サイズを使うようにした。

### 12. 入力オープン時の初期画角計算の復元

- **指示**: 入力を開いたときは入力画像のサイズに合わせた初期画角とし、投影方式を選び直したときはその方式の設定画角へ戻す。
- **対応**: 入力オープン後の処理を `Menu::initializeInputIntrinsics()` に集約し、現在の焦点距離から `Intrinsics::setFov()` で初期画角を計算するようにした。中心位置は変更せず、無効な解像度や焦点距離では計算しない。

### 13. メンバ変数の初期化位置の統一

- **対応**: 全クラスのメンバ初期値をクラス定義内のデフォルトメンバ初期化子へ集約した。

### 14. `mfcapture` との命名規約・コメントの統一

- **対応**: 共通する変数名・関数名を `mfcapture` の命名に、コメントと Doxygen の表現を `calib` に統一し、教材資料（プレゼンテーション、ハンドブック）に C++ クラス設計・カプセル化方針を追記した。

### 15. `mfcapture` の GStreamer 関連コードの削除

- **対応**: `mfcapture` からも GStreamer 関連の処理を削除し、方針を同期した。

### 16. ChArUco Board のマス目数（縦横）設定の追加

- **指示**: ChArUco Board のマス目の大きさだけでなく、縦横の数も設定できるようにする。
- **対応**: `Settings` に `checkerSize`（既定値 `{ 10, 7 }`）を追加し、`Calibration` のコンストラクタ、`createBoard()`、`setDictionary()` で指定できるようにした。較正パネルに「升目数」を追加し（最小値 2）、構成ファイルの保存・読込に対応した。

### 17. `Camera` クラスと入力モジュールの再設計

- **指示**: 場当たり的に作られていた `Camera` クラスの設計を見直し、最適化する。
- **対応**:
  - `Camera.h` を OpenGL・OpenCV・GLFW に依存しない抽象インターフェースとし、NVI パターン（`start()` / `stop()` / `close()` と保護フック `onStart()` / `onStop()` / `onClose()`）を導入した。
  - `frame` と `image` の二重バッファを単一バッファへ集約し、`lockFrame()` による非ブロッキングのゼロコピー転送にした。
  - `Capture` の `dynamic_cast` を廃止し、`isStillImage()`, `getFormatList()`, `selectFormat()` の仮想関数で連携するようにした。

### 18. 終了時の純粋仮想関数呼び出し例外の解消

- **現象**: Windows で終了時に R6025（pure virtual function call）が発生する。
- **原因**: `~Camera()` から `close()` を介して純粋仮想関数 `onClose()` を呼んでいた。基底クラスのデストラクタ実行時には派生クラスは解体済みである。
- **対応**: `~Camera()` を `default` とし、各派生クラスのデストラクタで `close()` を呼ぶようにした。

### 19. OpenXR 対応の `GgApp::OpenXR` への統合

- **指示**: `calib-openxr` の OpenXR 対応を `calib-rpi` / `mfcapture` の `GgApp::OpenXR` と一致させ、rebase できるようにする。
- **対応**: 独自クラス `GgOpenXR` を廃止して `GgApp::OpenXR` の API で描画フローを再実装し、`GG_ENABLE_OPENXR` オプションと OpenXR SDK 1.1.61 の自動取得を移植した。MSVC Debug 構成で `openxr_loaderd.lib` をリンクするよう修正した。

### 20. Android スマートフォン対応

- **対応**: Camera2 NDK による `CamAndroid`、APK の `assets/` からの自動展開、Gradle プロジェクト（`android/`）と OpenCV Android SDK 4.11.0 の自動取得を実装した。UI 部分は第 24 項で Jetpack Compose に置き換えた。

### 21. macOS の AV Foundation（`CamAvf`）対応

- **指示**: `CamCv`（`cv::CAP_AVFOUNDATION`）ではカメラの一覧や特性を取得できないため、AV Foundation ネイティブの `CamAvf` を作成し、インターフェースを他のプラットフォームと統一する。
- **対応**: デバイス列挙（重複名は `名前##番号`）、`CaptureFormat` によるフォーマット列挙、NVI フック、BGRA 出力、`alwaysDiscardsLateVideoFrames` によるレイテンシ優先の切り替え、`dispatch_sync` による停止時の合流、遅延初期化を実装した。`Menu` のフォーマット選択 UI を Windows / Android と共通化した。

### 22. macOS ビルドエラーの解消と Homebrew 非依存化

- **対応**:
  - `openMovie()` に残っていた未定義変数 `backend`, `fileHistory` の参照を削除した。
  - OpenCV のビルド設定で Homebrew 由来の外部依存の探索を無効化し、Xcode Command Line Tools と CMake だけで自己完結ビルドできるようにした。
  - ImGui のグリフ範囲の追加とデバイス名のサニタイズ（`sanitizeDeviceName()`）で、カメラ名の文字化けに対処した。
  - `CamAvf.mm` に `CamMf.cpp` と同等の教育的コメントを追加した。

### 23. ドキュメントの整理と Doxygen マニュアルの作成

- **対応**: 文書の冗長な記述を整理した。pdfLaTeX（CJKutf8）でエラーになる特殊 Unicode 文字をコメントから除き、HTML と `docs/pdf/refman.pdf` を生成した。

### 24. Android 版の Jetpack Compose 移行と OpenGL / ImGui 依存の排除

- **対応**:
  - `NativeBridge.cpp` から EGL / OpenGL ES を除き、`ANativeWindow` への CPU 直接転送で描画するようにした。サーフェスの再生成やサイズ変更時はバッファジオメトリを再設定する。
  - Android ビルドから EGL、GLESv3、ImGui、OpenGL ラッパー群と `native_app_glue` を除外し、不要になった Gradle 設定、`glEsVersion` の宣言、シェーダーやフォントのアセットを削除した。
  - デスクトップ専用コードを `#if !defined(__ANDROID__)` で分離した。

### 25. Android 版の機能追加と較正ファイルへの解像度の記録

- **対応**:
  - Android 版でキャプチャ画像を縦横比を保って中央に表示するようにした。
  - Android 版でチェッカーボードのマス目長とマーカー長を設定できるようにした（マス目長 2～20 cm、マーカー長 1～10 cm）。マーカー長がマス目長以上になる場合はマス目長の 1/2 にする。
  - Android 版でカメラの解像度を選択できるようにした（`Menu::selectResolution()`）。
  - 較正ファイルに較正時の入力解像度（`size`）を記録し、保存時の既定ファイル名を `calibYYYYMMDDhhmm.json` とした。

### 26. 点検と最適化

- **指示**: `GEMINI.md` の方針と `REQUESTS.md` の経緯にもとづいてプロジェクトを点検・最適化し、文書を整理する。
- **対応**:
  - `Capture` のプラットフォーム別に重複していた `openDevice()` / `updateFormatList()` の処理を、ネイティブカメラ型 `NativeCamera` を用いて共通化した。未使用の `Capture::emptyFormatList` を削除した。
  - `Camera::stop()` で `running.exchange(false)` を使い、停止処理とスレッド合流が重複しないようにした。`close()` が `stop()` を含むため、`Menu` の `stop()` と `close()` の連続呼び出しを整理した。
  - `Menu::openDevice()` と `updateFormatDropdowns()` に重複していた既定フォーマットの選択を `findDefaultFormat()` に集約した。3 つのフォーマット選択ドロップダウンの同期処理を `Menu::selectFormatItem()` にまとめた。
  - `startCapture()` での初期画角計算の重複、非 Windows 分岐内の到達しない `_MSC_VER` 分岐、未使用の `fileHistory` と `<pwd.h>` などのインクルード、`saveImage()` 内で重複定義していたファイルフィルタを削除した。姿勢の更新と復帰を `updatePose()` / `resetPose()` に統一した。
  - `Config::initialImage` の定義を `Menu.cpp` から `Config.cpp` へ移した。
  - Android でビルドされない `calib.cpp` から Android 用の分岐を削除し、新しいフレームを取得したときだけテクスチャへ転送するようにした。
  - BOM が欠けていた C++ ソース（`CamImage.h`, `Capture.h`, `Capture.cpp`, `Config.h`, `Config.cpp`）に BOM を付与した。
  - `GEMINI.md`、`REQUESTS.md`、`README.md` から古い記述や重複を整理した。
  - Doxygen の HTML と `docs/pdf/refman.pdf`（1019 ページ）を更新した。
- **検証**: Windows の Debug / Release、Android の Debug APK のビルドが成功し、`git diff --check` とソースコードに関する Doxygen 警告がないことを確認した。
