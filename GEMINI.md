# GEMINI.md - アプリケーションビルドおよび開発構成定義書

本ドキュメントは、ChArUco Board カメラキャリブレーションアプリケーションを近年の開発環境（Windows MSVC 2022, macOS Xcode, Ubuntu Linux）で正常にビルド・実行可能にするために実施したすべての設定、コード修正、およびライブラリ構成について記録したものです。

---

## 1. ソースコードの変更設定 (C++ コードの近代化・ビルドエラー対処)

### 1-1. OpenCV 4.13.0 (4.7+) への対応と ArUco API の移行
OpenCV 4.7.0 以降、ArUco モジュールは `opencv2/objdetect.hpp` に統合され、古いヘッダーや姿勢推定関数（`estimatePoseSingleMarkers`）は削除されました。これに対応するため以下の変更を行いました：
- **インクルードヘッダーの変更 ([Calibration.h](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/Calibration.h))**:
  - `opencv2/aruco.hpp` および `opencv2/aruco/charuco.hpp` のインクルードを廃止し、`<opencv2/objdetect.hpp>` に置き換えました。
- **マーカー姿勢推定の実装 ([Calibration.cpp](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/Calibration.cpp))**:
  - 削除された `cv::aruco::estimatePoseSingleMarkers` の呼び出し箇所（2箇所）を、`cv::solvePnP` を用いた直接の処理ループに置き換えました。これにより、代替用のモック関数を定義することなく、OpenCV 4.7+ の標準 API のみで処理を完結させています。
  ```cpp
  // 例: 各マーカのコーナーに対して個別に solvePnP を実行
  for (size_t i = 0; i < corners.size(); ++i)
  {
    cv::Vec3d rvec, tvec;
    cv::solvePnP(markerObjPoints, corners[i], cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);
    // 取得した rvec, tvec を用いて軸描画や座標変換行列の算出を行う
  }
  ```

### 1-2. OpenCV World リンク設定の反映 ([Camera.h](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/Camera.h))
`BUILD_opencv_world` を ON にした OpenCV パッケージを使用するため、ヘッダーに記述されている MSVC 用のライブラリ自動リンク設定を変更しました：
- `opencv_core`, `opencv_imgproc` などの個別モジュールへの `#pragma comment` を廃止し、`opencv_world` 一本に変更。
  ```cpp
  #  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
  ```

### 1-3. GLFW3 デバッグ版リンクの修正 ([gg.cpp](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/gg.cpp))
公式リリースバイナリには `glfw3d.lib`（デバッグ用）は含まれず `glfw3.lib` のみが提供されているため、MSVC デバッグビルド時でも `glfw3.lib` をリンクするよう修正しました：
  ```cpp
  #    pragma comment(lib, "glfw3.lib")
  ```

### 1-4. 文字コードと BOM の設定 (MSVC コンパイラ対応)
MSVC コンパイラが UTF-8 ソースファイルを Shift-JIS と誤認してコンパイル警告（`C4819`）を出すのを防ぐため、文字コード規則を以下のように厳格化しました：
- **C++ ソースファイル (`.h`, `.cpp`)**: すべて **BOM 付き UTF-8** で保存。
- **シェーダソースファイル (`.vert`, `.frag`)**: すべて **BOM 無し UTF-8** を維持。

---

## 2. 外部依存ライブラリの自動管理 ([download_deps.py](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/download_deps.py))

Git管理から除外されているサードパーティ製ライブラリを適切に配置するため、一括ダウンロードスクリプトを構築しました。

- **Dear ImGui (v1.92.8)**: GLFWおよびOpenGL3実装を `libs/ImGui/` 配下に展開。
- **Native File Dialog Extended (NFDe) (v1.2.1)**: ダイアログ用ソースファイルを `libs/ImGui/`、API定義ヘッダーを `libs/include/` に展開。
- **不足ヘッダーファイル**: `picojson.h` (JSON解析用)、`GL/glcorearb.h` (OpenGL コアAPI定義)、`KHR/khrplatform.h` (Khronos基本型定義) を `libs/include/` 配下に自動ダウンロード。
- **Windows 用バイナリ**: OpenCV 4.13.0 の公式インストーラー (SFX EXE) を取得して自動サイレント解凍し、GLFW 3.4 64bit バイナリとともに `libs/` 配下に格納。
- **macOS 用ソースコード**: OpenCV 4.13.0 および GLFW 3.4 のソースコードを `libs/` 配下に取得・解凍。

---

## 3. ビルド構成設定 ([CMakeLists.txt](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/CMakeLists.txt))

### 3-1. Windows 環境 (MSVC) での実行パスと作業ディレクトリの自動化
ビルドした実行ファイルをそのまま実行できるようにし、かつ Visual Studio 2022 上でそのままデバッグ実行 (F5) が行えるようにするため、以下の設定を追加しました。
- **DLL およびアセットのポストビルドコピー**:
  - 各種シェーダ（`.vert`, `.frag`）、アセット（`.json`, `.ttf`, `.obj`, `.mtl`, `.jpg`, `.gif` 等）を実行ファイル出力ディレクトリ（`$<TARGET_FILE_DIR:calib>`）へビルド完了時に自動コピー。
  - さらに、デバッグ時は `opencv_world4130d.dll`、リリース時は `opencv_world4130.dll` を自動的に実行ファイル出力ディレクトリへコピー。
- **VS プロジェクトのデバッグ環境設定**:
  - `VS_DEBUGGER_WORKING_DIRECTORY` プロパティを `$<TARGET_FILE_DIR:calib>`（出力ディレクトリ）に設定し、コピーされたアセットをデバッグ時に直接読み込めるように設定。
  - `VS_DEBUGGER_ENVIRONMENT` プロパティにて `PATH` 環境変数に出力ディレクトリを追加し、DLL のロードエラーを防止。
- **Windows Installer Project の統合**:
  - `include_external_msproject` コマンドを使用し、VSソリューションファイル (`calib.sln`) にセットアッププロジェクト `INSTALL/INSTALL.vdproj` を自動的にインクルード。
- **スタートアッププロジェクトの設定**:
  - `VS_STARTUP_PROJECT` ディレクトリプロパティを使用し、`calib.sln` ソリューションファイルを開いた際に `calib` が自動的にデフォルトのスタートアッププロジェクトに設定されるように構成。
- **ソリューションエクスプローラのフィルタ（フォルダ）分類**:
  - `source_group` コマンドを使用し、すべてのシェーダファイル（`.vert`, `.frag`）をソリューションエクスプローラ内の `Shader Files` というフィルタ（フォルダ）内に分類して表示。

### 3-2. macOS 環境でのスタンドアロンアプリケーション構成
Xcode上でビルドするだけで完全な Mac アプリケーションとして動作可能にするため、以下の設定を追加しました。
- **ソースビルドの統合**:
  - `libs/glfw-3.4` および `libs/opencv-4.13.0` を `add_subdirectory` にてビルドシステムに統合（OpenCV は `BUILD_opencv_world=ON` および必要モジュールのみをビルドする軽量構成に設定）。
- **App Bundle (`MACOSX_BUNDLE`) の設定**:
  - ウィンドウおよび各種リソースファイル（シェーダ、テクスチャ、モデル）を `Info.plist` を利用して App Bundle (`calib.app/Contents/Resources/`) 内に自動パッケージ。
- **dylib コピーとロードパス書き換え**:
  - ビルドされた `libopencv_world.dylib` および `libglfw.dylib` をバンドルの `Contents/Frameworks/` ディレクトリにコピーし、`install_name_tool` コマンドで実行ファイル内の依存先ロードパスを `@executable_path/../Frameworks/...` へ自動的に書き換え。

### 3-3. Linux 環境でのシステムライブラリ対応
- `find_package` (OpenCV) および `pkg-config` (GLFW3, GTK+-3.0) を用いてシステムにインストールされたライブラリをリンク。GTK+-3.0 は Linux における NFDe のダイアログ描画に必須であるため、依存関係として設定。

### 3-4. Native File Dialog Extended (NFDe) のマルチプラットフォームコンパイル設定
クロスプラットフォーム対応のネイティブファイルダイアログを構築するため、各プラットフォームに適した NFDe の実装ソースをビルド対象に選択的に追加しています。
- **Windows**: `libs/ImGui/nfd_win.cpp` をコンパイルし、Windows COM API 制御に必要なシステムライブラリ（`ole32`, `uuid`, `shell32`, `shlwapi`）をリンクします。
- **macOS**: `libs/ImGui/nfd_cocoa.m` を Objective-C ソースコードとしてビルド対象に含め、`Cocoa` および `AppKit` フレームワークとリンクします。
- **Linux**: `libs/ImGui/nfd_gtk.cpp` をコンパイルし、システムパッケージマネージャ経由で取得される `gtk+-3.0` ライブラリ（および関連ヘッダー）とリンクします。

