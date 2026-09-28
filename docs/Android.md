# calib Android 版の実機テストとビルドガイド

## 概要

`calib` の Android 実装は、Android NDK の `NativeActivity`（`android_main`）をエントリーポイントとし、EGL / OpenGL ES 3.1、Camera2 NDK (`CamAndroid`)、OpenCV Android SDK、および Dear ImGui を組み合わせて動作します。

本ドキュメントでは、Android Studio を用いた実機へのインストール、実行、および動作確認・デバッグの手順を解説します。

---

## 開発環境と必要なコンポーネント

- **統合開発環境**: Android Studio
- **SDK Platforms**:
  - Android 14.0 ("UpsideDownCake") (API Level 34)
- **SDK Tools**:
  - Android SDK Build-Tools: 34.0.0
  - NDK (Side by side): 27.0.12077973（または 26.x 以降）
  - CMake: 3.22.1
- **ターゲットアーキテクチャ**: `arm64-v8a`（64bit ARM 実機）

---

## 実機（Android 端末）の事前設定

1. **開発者向けオプションの有効化**:
   - 端末の「設定」→「デバイス情報（または端末情報）」を開きます。
   - 「ビルド番号」項目を **7回連続でタップ** します（「これでデベロッパーになりました」と表示されます）。
2. **USB デバッグの有効化**:
   - 「設定」→「システム」→「開発者向けオプション」を開きます。
   - **「USB デバッグ」** を ON にします。
   - *(※Xiaomi / OPPO / vivo 等の一部端末では「USB 経由でインストール」等の追加項目も ON にする必要があります)*
3. **PC との接続とデバッグ許可**:
   - PC と端末を USB ケーブルで接続します。
   - 端末画面に「**USB デバッグを許可しますか？**」と表示されたら、「このパソコンからの接続を常に許可する」にチェックを入れて「許可」をタップします。

---

## Android Studio でのビルドと実機転送

1. **プロジェクトを開く**:
   - Android Studio を起動し、「Open」から **`calib/android`** フォルダを選択して開きます。
   - *(※リポジトリのルートフォルダではなく、必ず `android` フォルダを開いてください)*
2. **Gradle Sync と依存ライブラリの自動取得**:
   - プロジェクトを開くと自動的に Gradle Sync が始まります。
   - `CMakeLists.txt` の記述により、初回のみ `OpenCV Android SDK 4.11.0`（約80MB）の自動ダウンロード・展開が行われます（インターネット接続が必要です）。
   - *(補足) もし Gradle Sync やビルド時に NDK バージョン不一致のエラーが出た場合は、`app/build.gradle` の `android { ... }` 内に `ndkVersion '27.0.12077973'` を明記してください。*
3. **実機へのインストールと実行**:
   - Android Studio 上部ツールバーのデバイス選択ドロップダウンに、接続した実機端末（例: `Google Pixel 7` など）が表示されていることを確認します。
   - **「Run 'app'」** ボタン（緑の再生マーク ▶、または `Shift + F10`）をクリックします。
   - ネイティブライブラリのコンパイルと APK のビルドが行われ、実機へ自動転送・インストール・起動されます。

---

## 【重要】カメラ権限の手動許可

本アプリは Java 側のフレームワークを持たない純粋な `NativeActivity` (`hasCode="false"`) で動作しているため、**初回起動時に OS 標準のカメラ許可ダイアログが表示されません**。

権限が付与されていない状態では Camera2 NDK の初期化に失敗しカメラ映像が表示されないため、初回起動後に以下の **いずれかの方法** でカメラ権限を許可してください。

### 方法 A: 端末の設定アプリから許可（推奨）
1. 端末の「設定」→「アプリ」→「calib」を開きます。
2. 「権限」（または「アプリの権限」）→「カメラ」をタップします。
3. **「アプリの使用中のみ許可」** を選択します。
4. アプリに戻るか、一度タスクを終了して再起動します。

### 方法 B: adb コマンドによる即時許可
PC の PowerShell または Android Studio の「Terminal」タブから以下を実行します：

```powershell
adb shell pm grant net.wakayama_u.tokoi.calib android.permission.CAMERA
```

---

## 動作確認と操作方法

- **タッチ操作**:
  - Dear ImGui のメニューバー、パネル、ボタン、スライダー等はタッチ操作に対応しています。
- **カメラ入力 (`CamAndroid`)**:
  - 「入力」パネルのデバイスリストに、実機の「Back（背面カメラ）」や「Front（前面カメラ）」が表示されます。
  - フォーマットを選択して「開始」を押すことで、Camera2 NDK を経由したリアルタイムキャプチャが行われます。
- **ファイルダイアログ**:
  - Android ではネイティブファイルダイアログの代わりに、ImGui によるモーダルダイアログ（`drawFileModal`）が開きます。
  - アプリ起動時に内部ストレージへ自動展開された画像ファイル（`castle.jpg`, `sky.jpg` 等）や JSON 構成ファイルを読み込めます。
- **キャリブレーション**:
  - ChArUco Board の検出や、一人での作業を支援する静止検知付き自動キャプチャも実機上で動作します。

---

## デバッグとログ確認 (Logcat)

Android Studio 下部の **「Logcat」** タブを開き、検索バーにフィルタを指定することで動作ログやエラーを確認できます。

- `tag:calib`: アプリケーション本体のエラーやライフサイクルログ
- `tag:CamAndroid`: Camera2 NDK のカメラ検出、ストリーム設定、フレーム取得ログ
- `package:net.wakayama_u.tokoi.calib`: アプリ全体のログ

---

## コマンドラインでのビルド (Gradle Wrapper)

Android Studio の GUI を使わずにコマンドラインからビルド・インストールすることも可能です：

```powershell
# ビルド (APK 生成)
cd android
.\gradlew.bat assembleDebug

# 実機へインストール
adb install -r app/build/outputs/apk/debug/app-debug.apk

# カメラ権限を付与
adb shell pm grant net.wakayama_u.tokoi.calib android.permission.CAMERA
```
