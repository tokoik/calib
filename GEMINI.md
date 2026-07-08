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
