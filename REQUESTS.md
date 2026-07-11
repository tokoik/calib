# REQUESTS.md - アプリケーションビルドおよび開発構成定義書

このプログラムは、ChArUco Board を使ったカメラキャリブレーションアプリケーションである。

---

## 1. ソースコード

- C++ のバージョンは C++17、OpenGL のバージョンは 4.1 とする。
- C++ のソースファイル (.cpp .h) の文字コードは BOM 付き utf-8 とする。
- GLSL のシェーダのソースファイル (.vert .frag .geom .comp) の文字コードは BOM 無し utf-8 とする。
- 改行コードは CRLF とする。
- Windows の場合は、Visual Studio のソリューションエクスプローラーに、次のようにしてファイル名を表示する。
  - C++ のヘッダファイル (.h) は Header Files というフィルタに表示する。
  - GLSL のシェーダのソースファイル (.vert .frag .geom .comp) は Shader Files というフィルタに表示する。
  - 画像ファイルなど、その他のファイルは Resource Files というフィルタに表示する。
- macOS の場合は Xcode のナビゲーターエリアに、次のようにしてファイル名を表示する。
  - C++ のヘッダファイル (.h) は Header Files というグループ内に表示する。
  - GLSL のシェーダのソースファイル (.vert .frag .geom .comp) は Shader Files というグループ内に表示する。
  - 画像ファイルなど、その他のファイルは Resource Files というグループ内に表示する。

## 2. CMakeLists.txt の設定

- cmake を使って、このプロジェクトを Windows、macOS、Ubuntu Linux の環境でビルドできるようにする。
  - Windows の場合は、Visual Studio のソリューションファイルを作成する。
    - ソリューションファイル名と同じプロジェクトを、単一のスタートアッププロジェクトに設定する。
  - macOS の場合は、Xcode のプロジェクトファイルを作成する。
  - Linux の場合は、Makefile を作成する。

### 2-2. 外部ライブラリの配置場所

- 外部ライブラリは cmake 時にダウンロードして、ソースのディレクトリの libs ディレクトリ以下に展開して配置する。
- ダウンロードには、ソースプログラムの配布先が Python を使用していないことを想定して、Python を使用せず、プラットフォームごとの標準的な手法を用いる。
- Visual Studio でこのディレクトリ以下のパスを指定する場合は、$(SolutionDir)/libs で始まるパスを使用する。

### 2-3. OpenGL 関連のヘッダファイル

- Ubuntu Linux の場合は OpenGL の開発パッケージ libgl-dev を用いる。
- Windows と macOS の場合は、以下の手順でダウンロードする。
  - libs の下に include というディレクトリを作る
  - include 中に GL および KHR というディレクトリを作る
  - GL の中に https://registry.khronos.org/OpenGL/api/GL/ から必要なものをダウンロードして配置する。
  - KHR の中に https://registry.khronos.org/EGL/api/KHR/ から必要なものをダウンロードして配置する。

### 2-4. GLFW

- フレームワークとして GLFW を用いる。
- Ubuntu Linux の場合は、GLFW バージョン 3 の開発パッケージ libglfw3-dev を用いる。
- Windows の場合は、最新バージョン (現時点で 3.4) のリリース版の Windows 64bit 用のバイナリファイル https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip をダウンロードして libs 以下に展開し、Visual Studio の最新のものに対応したスタティックライブラリファイル (現時点で lib-vc2022) をリンクする。
- macOS の場合は、最新バージョン (現時点で 3.4) のリリース版のバイナリファイル https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.MACOS.zip をダウンロードして libs 以下に展開し、ビルドするシステムに対応したスタティックライブラリファイルをリンクする。
- macOS の場合は、OpenGL.framework のほか、Cocoa.framework など OpenGL を使ったプログラムの実行に必要なフレームワークをリンクする。

### 2-5. Dear ImGui

- GUI には Dear ImGui を用いる。
  - 最新リリース (現時点で v1.92.8) のソースファイル https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.zip をダウンロードし、libs 以下の ImGui というディレクトリに展開する。ただし、OpenGL バージョン3 以降と GLFW の組み合わせに対応したものだけを配置すればよい。
  - 展開したファイルをプログラム本体と一緒にコンパイル・リンクする。
  - Windows の場合は、Visual Studio のソリューションエクスプローラーの ImGui というフィルタの中に ImGui のソースファイル名 (.cpp) を表示する。
  - macOS の場合は、Xcode のナビゲーターエリアの ImGui というグループ内に ImGui のソースファイル名 (.cpp .m) を表示する。

### 2-6. Native File Dialog Extended

- ファイルダイアログには Native File Dialog Extended を用いる
  - Windows と macOS の場合は、最新リリース (現時点では v1.3.0) のソースファイル https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.3.0.zip をダウンロードして、プラットフォームに合わせたソースファイルを ImGui と同じ libs ディレクトリ内の ImGui ディレクトリに配置する。
  - Windows の場合は、Visual Studio のソリューションエクスプローラーの ImGui というフィルタの中に、ImGui のソースファイル名と一緒に Native File Dialog Extended のソースファイル名を表示する。
  - macOS の場合は、Visual Studio のソリューションエクスプローラーの ImGui というグループ内に、ImGui のソースファイル名と一緒に Native File Dialog Extended のソースファイル名を表示する。
  - Ubuntu Linux の場合は、GTK+3 も必要になるので、開発パッケージ libgtk-3-dev および関連のパッケージを用いる。
  
### 2-7. picojson

- JSON ファイルの解析には picojson を用いる。
  - picojson の最新リリース (現時点では 1.3.0) のソースファイル https://github.com/kazuho/picojson/archive/refs/tags/v1.3.0.zip をダウンロードして、picojson.h を libs ディレクトリの中の include ディレクトリに配置する。

### 2-8. OpenCV

- OpenCV はバージョン 4 の最新リリース (現時点では 4.13.0) を使用する。
- Ubuntu Linux の場合は、OpenCV バージョン 4 の開発パッケージ libopencv-dev を用いる。
- Windows の場合は、リリース版の Windows 用バイナリインストーラ https://github.com/opencv/opencv/releases/download/4.13.0/opencv-4.13.0-windows.exe をダウンロードして libs 以下に展開してリンクする。
- macOS の場合は、ソースファイル https://github.com/opencv/opencv/archive/refs/tags/4.13.0.zip をダウンロードし、libs フォルダ内に展開してビルドし、作成されたライブラリファイルをこのプログラムにリンクする。
  - cmake のバージョンの違いによるエラーを回避するようにする。

### 2-9. 実行ファイル

- Windows および Ubuntu Linux では、実行ファイルと同じディレクトリに DLL や GLSL のソースファイル、フォントファイルなど、実行に必要なファイルをコピーする。
- Windows では、Visual Studio 内で実行（デバッグ）できるように、実行時の作業ディレクトリを実行ファイルを置いたところに移すようにする。
- macOS では、アプリケーションバンドル (.app) を作成し、その中に実行に必要なファイルをコピーする。またこれを、Xcode 内から実行できるようにする。

## 3. README.md, GEMINI.md, .gitignore

- README.md を作成すること。
  - プログラムの構造を解説すること。
  - Mermaidによる構成図を添付すること。
  - カメラオンライン/画像オフラインでのキャリブレーション詳細手順を説明すること。
  - プラットフォーム別のビルド手順を説明すること。
- GEMINI.md を作成すること。
  - アプリケーションのビルド・開発構成の定義を記録すること。
  - 各プラットフォームの依存ライブラリ構成を記録すること。
  - CMakeLists.txt の設計方針を記録すること。
- .gitignore を整備すること。
  - リポジトリに含める必要のないファイルやディレクトリを追加すること。
  - バイナリディレクトリ build に加え libs も Git 管理除外とすること。

## 4. ビデオキャプチャデバイスの設計意図

### 4-1. UVC カメラ初期化におけるデコーダ初期化の分離
- H.264 コーデックをサポートするカメラでのフリーズや起動失敗を避けるため、フォーマットリストの取得時およびキャプチャ「開始」ボタンを押す前のデバイスオープン時には、カメラフォーマットの適用（デコーダのロードを含む `setFormat`）を一切行わないこと。
- フォーマットの適用は、ユーザーが画面上で形式を選択し、キャプチャを明示的に開始する瞬間にのみ 1 回だけ行われるように設計し、無駄なハードウェアデコーダの初期化ロード（特に H.264 ハードウェア MFT の初期化）による物理遅延やエラー発生を防ぐこと。

### 4-2. 手動デコードによる CPU 高速処理方式（AVP自動変換の廃止）
- RICOH THETA V や Logi C615 など、H.264 や MJPG などの圧縮フォーマットを出力する Web カメラにおいて、OpenCV の AVP (Advanced Video Processing) 自動変換による物理遅延や列挙フリーズを回避するため、自前で `IMFTransform` デコーダ・カラーコンバータを構築して RGB32 にデコードする処理を移植し、OpenCV に依存しない Pure Media Foundation キャプチャ化を達成すること。

### 4-3. 安全なフォーマットリスト動的取得設計（生ポインタ管理の廃止）
- `Capture` クラス内でフォーマットリストの参照先を切り替える生ポインタ管理を廃止すること。カメラオブジェクトの有効状態を判定して安全にリストを返す `getFormatList()` を実装し、フォーマットリストが UI 上で空のまま更新されなくなる不具合を解消すること。

## 5. UI（メニュー表示）の改善要請

### 5-1. ビデオフォーマット選択ドロップダウンの3分割表示と最適化
- ビデオフォーマットの表示名（解像度 @ フレームレート fps (コーデック)）を、「解像度」「コマ数（フレームレート）」「符号化（コーデック）」の3つのドロップダウンリストに分割して表示すること。
- ドロップダウンリストの各項目（ユニークな解像度・フレームレート・コーデック）は毎フレーム再生成するのではなく、キャプチャデバイスの切り替え時および初期オープン時のみ作成するようにして、レンダリング負荷を最小限に抑えること。
- 存在しない組み合わせ（例えば、ある解像度で指定したフレームレートやコーデックがサポートされていないとき）が選択された場合は、「フォーマットが存在しません」という警告を表示し、キャプチャ「開始」ボタンを非表示にすること。

## 6. GPUゼロコピー化の断念とPBOの維持

### 6-1. OpenCVによる画像処理設計との整合性
- WGL DX Interop を用いた GPU ゼロコピー化（Direct3D11テクスチャからOpenGLテクスチャへの直接マッピング）は、画素データをGPU上に留めるため、OpenCVで行うChArUcoボードのコーナー検出や座標計算などのCPU上の画像処理設計と競合する。
- したがって、GPUゼロコピー化の実装は行わず、従来のPBO（Pixel Buffer Object）を用いた最速のCPU -> GPU画像転送機構を維持すること。

