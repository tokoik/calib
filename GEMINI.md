# GEMINI.md - アプリケーションビルドおよび開発構成定義書

本書は、ChArUco Board を用いたカメラキャリブレーションアプリケーションにおける、ビルド構成、プラットフォーム別の依存関係、および `CMakeLists.txt` の設計方針について記録・定義したものである。

---

## 1. アプリケーション開発・ビルド構成の定義

本プログラムはクロスプラットフォーム (Windows, macOS, Ubuntu Linux) でのビルドおよび実行をサポートし、以下の開発構成に従う。

### 1-1. 文字コード・改行コード仕様
- **C++ ソースファイル (`.cpp`, `.h`)**: 
  - 文字コード: **BOM 付き UTF-8** (Visual Studio 等での文字化け防止のため)
  - 改行コード: **CRLF**
- **GLSL シェーダファイル (`.vert`, `.frag`, `.geom`, `.comp`)**: 
  - 文字コード: **BOM 無し UTF-8** (一部のグラフィックスドライバ/コンパイラでのエラー回避のため)
  - 改行コード: **CRLF**

### 1-2. ビルドフレームワーク
- **CMake**: 最小要件バージョン `3.13`
- **C++規格**: `C++17` を必須とする。

---

## 2. 各プラットフォームの依存ライブラリ構成

外部依存ライブラリは、システムグローバルにインストールする Linux を除き、cmake 実行時に自動的にダウンロードおよび展開され、プロジェクトルート直下の `libs/` ディレクトリ配下に配置される。

| ライブラリ / パッケージ | Windows (MSVC 2022) | macOS (Xcode) | Ubuntu Linux (GCC) |
| :--- | :--- | :--- | :--- |
| **OpenGL API ヘッダ** | cmake 時に Khronos レジストリよりダウンロードし `libs/include` 内の `GL`, `KHR` に配置 | cmake 時に Khronos レジストリよりダウンロードし `libs/include` 内の `GL`, `KHR` に配置 | システムパッケージ `libgl-dev` を使用 |
| **GLFW** | cmake 時に最新バイナリを `libs/` に展開、`lib-vc2022/glfw3.lib` をリンク | cmake 時に最新バイナリを `libs/` に展開、ビルドシステム用スタティックライブラリをリンク | システムパッケージ `libglfw3-dev` を使用 |
| **Dear ImGui** | cmake 時に最新ソースを `libs/ImGui` に展開、本体と同時コンパイル・リンク | cmake 時に最新ソースを `libs/ImGui` に展開、本体と同時コンパイル・リンク | cmake 時に最新ソースを `libs/ImGui` に展開、本体と同時コンパイル・リンク |
| **Native File Dialog Extended** | cmake 時にソースファイルを `libs/ImGui/` 内に展開し、ImGui と同時にビルド | cmake 時にソースファイルを `libs/ImGui/` 内に展開し、ImGui と同時にビルド | システムパッケージ `libgtk-3-dev` と `nfd_gtk.cpp` を使用 |
| **picojson** | cmake 時に `picojson.h` をダウンロードし `libs/include` に配置 | cmake 時に `picojson.h` をダウンロードし `libs/include` に配置 | cmake 時に `picojson.h` をダウンロードし `libs/include` に配置 |
| **OpenCV (v4)** | cmake 時に Windows 用バイナリを `libs/opencv` に展開しリンク | cmake 時に最新ソースを `libs/` に展開し、高速化のため主要モジュールのみ CMake サブプロジェクトビルド | システムパッケージ `libopencv-dev` を使用 |

---

## 3. CMakeLists.txt の設計方針

`CMakeLists.txt` は、各OSで標準的に用いられる統合開発環境 (IDE) での作業効率を最大化する設計となっている。

### 3-1. IDE 向けフィルタ・グループ設計 (Source Grouping)
Windows (Visual Studio) および macOS (Xcode) のプロジェクト生成時に、ファイルが論理的に整理されて表示されるよう、グローバルに `source_group` を定義している。

- **Header Files**: プロジェクト内のすべての `.h` ファイルをグループ化。
- **Shader Files**: すべての `.vert`, `.frag` 等のシェーダコードをグループ化。
- **Resource Files**: 3Dモデル (`.obj`/`.mtl`)、フォント (`.ttf`) , アセット画像 (`.jpg`/`.png`/`.gif`)、コンフィグ (`.json`)、Windowsリソース定義 (`.rc`) をグループ化。
- **ImGui**: `libs/ImGui` 配下のソース・ヘッダ、および Native File Dialog Extended のファイルをグループ化。

### 3-2. Windows 開発環境向け設計
- **スタートアッププロジェクト設定**: `calib` ターゲットをスタートアッププロジェクトに自動設定。
- **作業ディレクトリの自動設定**: Visual Studio 内から F5キーで直接デバッグ実行できるよう、作業ディレクトリを実行ファイルの出力フォルダ (`$<TARGET_FILE_DIR:calib>`) に自動設定。
- **アセット・DLL の自動同期**: ビルド後に自動でシェーダ、アセットファイル、および構成 (Debug/Release) に応じた OpenCV DLL (`opencv_world4130[d].dll`) を実行ディレクトリにコピー。

### 3-3. macOS 開発環境向け設計
- **App Bundle (`.app`) 作成**: macOS標準のアプリケーションバンドル形式で出力。
- **リソースの同梱**: すべてのアセットとシェーダファイルをバンドル内の `Contents/Resources` に自動パッケージ化。
- **Frameworks と RPATH の調整**: 依存する動的ライブラリ (`opencv_world`) を `Contents/Frameworks/` にコピーし、実行ファイルからのリンク参照パス (`@executable_path/../Frameworks/...`) を `install_name_tool` で自動修正。(GLFWはスタティックライブラリとしてリンクするため、コピーおよびパス修正は不要)

### 3-4. 自動ダウンロード機能
- **cmake による自動ダウンロード**: ビルドの依存ライブラリは、Python などの外部スクリプトを使用せず、`CMakeLists.txt` 内で `file(DOWNLOAD)` と `cmake -E tar` コマンドを用いて自動的にダウンロード・展開・配置される。

---

## 4. ビデオキャプチャデバイス（UVCカメラ）の制御・初期化方針

Windows (Media Foundation: MSMF) における UVC カメラの制御は、アプリの起動速度および操作の応答性を損なわないよう、以下の初期化ライフサイクル方針に従う。

### 4-1. フォーマットリスト取得とデバイスオープンの軽量化
- **デコーダ初期化の回避**: 起動時やデバイス変更時のフォーマットリスト取得（`updateFormatList`）、およびデバイスのオープン（`openDevice`）の時点では、`setupFormat = false` として `CamMf::open` を呼び出し、`setFormat(0)` を実行しない。
- **目的**: H.264 などの圧縮フォーマットをサポートするカメラにおいて、一覧取得時やデバイス認識時に「H.264デコーダ MFT」や GPU 連携のハードウェアデコーダが不要に初期化されるのを防ぎ、初期化の競合エラー（`MF_E_VIDEO_DEVICE_LOCKED` 等）による起動失敗や数秒に及ぶフリーズ（物理遅延）を防止する。

### 4-2. キャプチャ開始時の遅延評価（Lazy Initialization）
- **ピンポイントのフォーマット適用**: カメラに解像度・フレームレート・ピクセルフォーマットを実際に適用する `setFormat()` 処理は、ユーザーが「開始」ボタンを押して `capture.select(formatNumber)` が明示的に呼び出されたタイミングでのみ、選択されたフォーマットに対して 1 度だけ実行する。

### 4-3. 手動デコードによる CPU 高速処理方式（AVP自動変換の廃止）
- **遅延・フリーズの回避**: RICOH THETA V や Logi C615 など、H.264 や MJPG などの圧縮フォーマットを出力する Web カメラにおいて、OpenCV の AVP (Advanced Video Processing) 自動変換による物理遅延や列挙フリーズを回避するため、`CamMf` クラスに自前でデコーダ（`IMFTransform`）とカラーコンバータ（`CLSID_CColorConvertDMO`）を直接構築し、NV12 から RGB32 へのデコード処理を手動で行う方式を移植。

### 4-4. 安全なフォーマットリスト動的取得設計（生ポインタ管理の廃止）
- **状態不整合の解消**: 起動時に `openImage` や他のカメラオープン処理が失敗した際、表示用フォーマットリストの生ポインタが空リスト（`emptyFormatList`）を指したまま戻らなくなるバグを解消。生ポインタ `formatList` を完全に廃止し、`getFormatList()` がカメラの有効状態に応じて適切なリスト（`deviceFormatList` または有効なカメラオブジェクト内のリスト）を動的に安全に返すように設計を変更。

### 4-5. GPUゼロコピー化（WGL DX Interop）の方針断念と、PBOを用いたCPUメモリ経由転送の維持
- **断念理由**: 本アプリケーションの目的は、キャプチャしたフレームに対して OpenCV による CPU 上の画像処理（ChArUco コーナー検出など）を施し、その結果を描画することである。GPU ゼロコピー化（VRAM上でのテクスチャ直接共有）を行うと、画素データが GPU 内部から CPU に戻ってこないため画像処理を適用できなくなり、逆に戻す処理 (Readback) を入れると大幅なオーバーヘッドとなるため、画像処理を前提とした設計と完全に競合することから、ゼロコピー化は不採用とし、PBO (Pixel Buffer Object) を用いた最速の CPU メモリ経由転送・テクスチャ同期を維持することとした。

### 4-6. ArUco/ChArUco 検出描画時の4チャンネル画像対応（例外回避）
- **例外の回避**: OpenCV の ArUco 検出描画関数（`cv::aruco::drawDetectedMarkers`, `cv::aruco::drawDetectedCornersCharuco` 等）は 1 または 3 チャンネル画像のみをサポートし、4 チャンネル画像が渡されるとアサーション例外をスローする。
- **対処法**: パフォーマンス上の理由から PBO 転送や表示処理に 4 チャンネル（BGRA/RGB32）画像を維持しつつ、`Calibration` 内の検出・描画処理（`detectBoard`, `detectMarkers`）の直前で一時的に 3 チャンネル（BGR）にダウンサンプリング（`cv::cvtColor`）して処理を実行し、描画結果を元の 4 チャンネル画像に書き戻す方法を採用して例外を回避した。

### 4-7. MFT デコーダのバッファ管理と低遅延（Low Latency）化
- **バッファ書き込みの最適化**: MFT (Media Foundation Transform) が出力用の自前サンプルを持たない場合、アプリ側で確保した `MFT_OUTPUT_DATA_BUFFER` を渡す。この際、バッファの `CurrentLength` を最大サイズではなく `0` に設定して渡すことで、MFT 側がバッファ満杯と誤認して `E_FAIL (80004005)` を返す問題を回避している。
- **低遅延デコード設定**: H.264 デコーダは、デフォルトで再生のスムーズさを優先し数秒分のフレームをバッファリングする仕様がある。これを回避するため、`ICodecAPI` を通じて `CODECAPI_AVLowLatencyMode` を `VARIANT_TRUE` に設定し、リアルタイムキャプチャにおける数秒の遅延（レイテンシ）を解消している。

---

## 5. UI（メニュー表示）の改善方針

### 5-1. ビデオフォーマット選択UIの分割と最適化
- **3分割ドロップダウン化**: 従来の「解像度 @ フレームレート fps (コーデック)」という結合表示から、「解像度」「コマ数」「符号化」の3つの独立したドロップダウンメニューに分割した。これによりUI幅（231px）の制限内に収まり、選択しやすさが向上した。存在しない組み合わせの場合は「フォーマットが存在しません」と警告して開始ボタンを非表示化する。
- **動的生成の最適化**: 毎フレームのメニュー描画時にドロップダウン項目の配列を動的に作成するのをやめ、キャプチャデバイス変更時（および初期オープン時）にのみユニークな項目のパースとキャッシュ生成（`Menu::updateFormatDropdowns`）を行うよう変更し、描画パフォーマンスを改善した。

