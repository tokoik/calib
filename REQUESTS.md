# ChArUco Board キャリブレーションプログラム 開発依頼・対応履歴

本ドキュメントは、これまでにユーザー様からご依頼いただいた内容と、それに対応するために実施したソースコードの修正、構成変更、およびドキュメント作成の履歴をまとめたものです。

---

## 1. 依頼内容と対応概要

| # | 依頼内容 | 主な対応内容 |
| :--- | :--- | :--- |
| 1 | `README.md` の作成（構造解説、Mermaidによる構成図、カメラオンライン/画像オフラインでのキャリブレーション詳細手順） | [README.md](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/README.md) を新規作成し、システムの解説と手順を網羅。 |
| 2 | プラットフォーム別のビルド手順の追加 | `README.md` に Windows (MSVC), macOS (Xcode), Ubuntu Linux のビルド方法を追記。 |
| 3 | アプリケーションのビルド・開発構成を定義した `GEMINI.md` の作成 | 各プラットフォームの依存ライブラリ構成や CMake 設計を記述した [GEMINI.md](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/GEMINI.md) を作成。 |
| 4 | `.gitignore` の整備（`build` に加え `lib` / `libs` も Git 管理除外とする） | [.gitignore](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/.gitignore) を見直し、自動生成物やダウンロード依存関係を除外する設定に更新。 |
| 5 | 依存ライブラリ配置ディレクトリを `lib` から `libs` へ統一 | すべての外部依存ファイルを `libs/` 配下に統合し、CMake 設定および Python 取得スクリプトを更新。 |
| 6 | Windows 版で `calib` を Visual Studio 2022 の既定スタートアッププロジェクトに設定 | CMakeLists.txt に `VS_STARTUP_PROJECT` 設定を追加。また、初回インポート時の `.vs` 設定キャッシュの影響と対策をドキュメント化。 |
| 7 | Windows 版でシェーダーファイル（`.vert`, `.frag`）を `Shader Files` フィルタフォルダ内に分類して表示 | CMakeLists.txt 内で `source_group` コマンドを使用し、ソリューションエクスプローラ上で綺麗に分類表示されるよう対応。 |
| 8 | OpenCV 4.12.0 (4.7+) 対応に伴う、廃止された旧 ArUco 姿勢推定関数の移行 | [Calibration.h](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/Calibration.h) のインクルードを現代的な `<opencv2/objdetect.hpp>` に修正。 |
| 9 | `estimatePoseSingleMarkers` のモック（代替ラッパー）を使わず、`cv::solvePnP` を用いた直接の実装への書き換え | 代替ラッパー定義を排除し、[Calibration.cpp](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/Calibration.cpp) 内の該当ループ箇所で直接 `cv::solvePnP` を呼び出すように変更。 |
| 10 | マーカー寸法計算における `markerLength * 0.5f` の定数化リファクタリング | 中心からの距離座標として直感的で分かりやすい `markerCenter` という定数（`const float`）に置き換え、可読性を向上。 |
| 11 | `cv::solvePnP` 呼び出し箇所への解説コメント追加 | 処理意図が1行で明確に伝わる簡潔な日本語コメントを挿入。 |
| 12 | Native File Dialog - Extended (NFDe) のマルチプラットフォームコンパイル設定の確認と GEMINI.md への反映 | `download_deps.py` と `CMakeLists.txt` によるプラットフォーム別（Windows: win, macOS: cocoa, Linux: gtk）のソース混成コンパイル設定のドキュメント化。 |

---

## 2. 具体的な対応詳細

### 2-1. ビルドシステムと依存関係の自動化
* **自動ダウンロードスクリプト ([download_deps.py](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/download_deps.py))**:
  Gitリポジトリを軽量に保つため、Dear ImGui, Native File Dialog - Extended, picojson, KHR/GL関連の OpenGL ヘッダー、および Windows 版 OpenCV / GLFW バイナリを自動ダウンロードして `libs/` ディレクトリに展開する Python スクリプトを作成しました。
* **近代的な CMakeLists 構成 ([CMakeLists.txt](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/CMakeLists.txt))**:
  * **Windows**: 静的リンク GLFW、動的リンク OpenCV を使用し、ビルド完了時にアセット（テクスチャ、3Dモデル、シェーダー）および OpenCV DLL を実行バイナリのフォルダへ自動コピー。VSデバッガの作業ディレクトリと実行パス環境変数を自動設定。ソリューションにセットアッププロジェクト (`.vdproj`) を統合。
  * **macOS**: OpenCV と GLFW をソース（`add_subdirectory`）から Xcode プロジェクトの一部としてビルドし、アセットや dynamic library (dylib) を内包した独立起動可能な App Bundle (`calib.app`) を生成。ロードパス (`RPATH`) を自動で書き換えるポストビルドコマンドを統合。
  * **Linux**: システムパッケージマネージャおよび `pkg-config` 経由で取得した OpenCV, GLFW3, GTK+-3.0 をリンク。

### 2-2. ソースコードの近代化とリファクタリング
* **文字コード規格の厳格化**:
  MSVCコンパイラによる `warning C4819` などの多言語文字化け警告を防ぐため、C++コード（`.h`, `.cpp`）はすべて **BOM付き UTF-8** に統一し、シェーダーコード（`.vert`, `.frag`）は互換性を高めるため **BOM無し UTF-8** に統一する Python チェッカースクリプトを用いて保守。
* **OpenCV 4.7+ の ArUco API への適合 ([Calibration.cpp](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/Calibration.cpp))**:
  削除された旧 API を使う代わりに、以下のように `cv::solvePnP` と正方形マーカー向けに最適化された `SOLVEPNP_IPPE_SQUARE` フラグを用いて、シンプルかつ直接的に姿勢推定を行う方式へリファクタリングしました。
  
  ```cpp
  // 各マーカに対応する３次元空間の点
  const float markerCenter{ markerLength * 0.5f };
  std::vector<cv::Point3f> markerObjPoints;
  markerObjPoints.push_back(cv::Point3f(-markerCenter, markerCenter, 0.0f));
  markerObjPoints.push_back(cv::Point3f(markerCenter, markerCenter, 0.0f));
  markerObjPoints.push_back(cv::Point3f(markerCenter, -markerCenter, 0.0f));
  markerObjPoints.push_back(cv::Point3f(-markerCenter, -markerCenter, 0.0f));

  // 個々のマーカーについて
  for (size_t i = 0; i < corners.size(); ++i)
  {
    cv::Vec3d rvec, tvec;
    // マーカーのコーナー検出位置から3次元姿勢（回転・平行移動）を推定する
    cv::solvePnP(markerObjPoints, corners[i], cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);

    // 座標軸を描く
    cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvec, tvec, markerLength);
  }
  ```

### 2-3. ファイルダイアログ（NFDe）のコンパイル設計
* プラットフォームに応じて OS ネイティブのダイアログを使用する `nativefiledialog-extended` をコンパイル対象として組み込んでいます。
  * **Windows**: `libs/ImGui/nfd_win.cpp` (COM / Shell API)
  * **macOS**: `libs/ImGui/nfd_cocoa.m` (Objective-C / Cocoa AppKit)
  * **Linux**: `libs/ImGui/nfd_gtk.cpp` (GTK+ 3.0)
* この設計を [GEMINI.md](file:///D:/Users/tokoi/Documents/Projects/worktrees/calib-wom/GEMINI.md) に追加・明記しました。
