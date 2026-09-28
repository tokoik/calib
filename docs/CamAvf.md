# `CamAvf` クラス実装の完全解説

本ドキュメントでは、macOS 環境において **AV Foundation**、**Core Video**、**Core Media**、および **Grand Central Dispatch (GCD)** を使用してビデオキャプチャを行う `CamAvf` クラスの実装について、ソースコード ([CamAvf.h](../CamAvf.h), [CamAvf.mm](../CamAvf.mm)) と突き合わせながら詳細に解説します。

また、実装内で利用されている Objective-C クラスおよび C 言語 API について、引数や戻り値を含めた包括的なリファレンスを掲載しています。

---

## 目次

- [1. 概要とプロジェクトにおける役割](#1-概要とプロジェクトにおける役割)
  - [1.1 AV Foundation とは](#11-av-foundation-とは)
  - [1.2 OpenCV (cv::VideoCapture) の制約と CamAvf の開発意図](#12-opencv-cvvideocapture-の制約と-camavf-の開発意図)
  - [1.3 主な役割と設計方針](#13-主な役割と設計方針)
- [2. クラス構造とコンポーネント](#2-クラス構造とコンポーネント)
  - [2.1 ネストクラス・構造体](#21-ネストクラス構造体)
  - [2.2 内部デリゲートクラス (CamAvfDelegate)](#22-内部デリゲートクラス-camavfdelegate)
  - [2.3 内部実装構造体 (CamAvf::Impl - PIMPL)](#23-内部実装構造体-camavfimpl---pimpl)
  - [2.4 補助関数](#24-補助関数)
- [3. 主要処理フローとソースコード解説](#3-主要処理フローとソースコード解説)
  - [3.1 TCC 権限確認とデバイス列挙 (`getDeviceList`)](#31-tcc-権限確認とデバイス列挙-getdevicelist)
  - [3.2 カメラオープンと遅延初期化 (`open`)](#32-カメラオープンと遅延初期化-open)
  - [3.3 フォーマット列挙とコーデック・FPS 抽出 (`enumerateFormats`)](#33-フォーマット列挙とコーデックfps-抽出-enumerateformats)
  - [3.4 フォーマット選択とハードウェア設定 (`select`)](#34-フォーマット選択とハードウェア設定-select)
  - [3.5 キャプチャ開始とセッション構築 (`onStart`)](#35-キャプチャ開始とセッション構築-onstart)
  - [3.6 サンプルバッファ受信とストライド・色変換 (`handleSampleBuffer`)](#36-サンプルバッファ受信とストライド色変換-handlesamplebuffer)
  - [3.7 キャプチャ停止とクリーンアップ (`onStop`, `onClose`, `~Impl`)](#37-キャプチャ停止とクリーンアップ-onstop-onclose-impl)
- [4. AV Foundation / Core Media / Core Video / GCD 完全リファレンス](#4-av-foundation--core-media--core-video--gcd-完全リファレンス)
  - [4.1 AV Foundation (Objective-C クラス)](#41-av-foundation-objective-c-クラス)
  - [4.2 Core Media / Core Video (C API)](#42-core-media--core-video-c-api)
  - [4.3 Grand Central Dispatch (GCD)](#43-grand-central-dispatch-gcd)
- [5. まとめ](#5-まとめ)

---

## 1. 概要とプロジェクトにおける役割

`CamAvf` クラスは、基底クラス [Camera](../Camera.h) を継承し、macOS 環境において内蔵 FaceTime HD カメラ、外付け USB Web カメラ、および iPhone をワイヤレスカメラとして利用する「連係カメラ (Continuity Camera)」から高品質・低遅延にビデオフレームを取得・処理するクラスです。

```
+-------------------------------------------------------------------------------+
|                            calib / mfcapture                                  |
|                                                                               |
|  +--------------------+    +--------------------+    +---------------------+  |
|  |     CamMf (Win)    |    |   CamAvf (macOS)   |    |  CamLibcam (RPi)    |  |
|  +---------+----------+    +---------+----------+    +----------+----------+  |
|            |                         |                          |             |
|            +-------------------------+--------------------------+             |
|                                      |                                        |
|                                      v                                        |
|                         +--------------------------+                          |
|                         |    Camera (基底クラス)   |                          |
|                         +------------+-------------+                          |
+--------------------------------------|----------------------------------------+
                                       |
                                       v
                    +------------------------------------+
                    |        Capture / Texture           |
                    | (PBO / GPU テクスチャ / GLSL展開)  |
                    +------------------------------------+
```

### 1.1 AV Foundation とは

**AV Foundation** は、macOS および iOS におけるマルチメディア（オーディオおよびビデオ）の再生、録画、編集、およびリアルタイムストリーミングを司る Apple の標準 Objective-C / Swift フレームワークです。
旧来の QuickTime フレームワークに代わり導入され、カメラキャプチャのハードウェア制御（露出、フォーカス、ホワイトバランス、解像度、フレームレート）を詳細に管理できます。下位レイヤである **Core Media**（時間軸管理とメディアパイプライン）や **Core Video**（GPU/CPU メモリパイプライン）と直接連携し、極めてオーバーヘッドの少ないパイプラインを構築できます。

### 1.2 OpenCV (cv::VideoCapture) の制約と CamAvf の開発意図

macOS における `cv::VideoCapture` (AVFoundation バックエンド) には以下の深刻な制約が存在します：
1. **デバイス名の取得不可**: カメラ一覧を取得する API が OpenCV に存在せず、インデックス番号（0, 1, 2...）でしか指定できないため、UI にカメラ名（例: "FaceTime HD Camera", "Logitech BRIO"）を表示できません。
2. **フォーマット選択の不自由さ**: カメラがサポートするすべての解像度、フレームレート、コーデックの組み合わせを列挙できず、任意のフォーマットへ確実に切り替えることが困難です。
3. **不要な色変換と遅延**: OpenCV 内部で独自の色変換や二重バッファリングが行われ、レイテンシが増大します。
4. **macOS 14 (Sonoma) 以降の連係カメラ／外部カメラ分離への未対応**: 最新 macOS ではプライバシー保護とカメラカテゴリの細分化が行われていますが、古い API を用いると外部カメラが認識されない問題が発生します。

`CamAvf` は、macOS のネイティブ API を直接利用することでこれらすべての問題を解決し、Windows の `CamMf` や Raspberry Pi の `CamLibcam` と同等の高機能性と低遅延性能を実現しています。

### 1.3 主な役割と設計方針

1. **AV Foundation 固有処理の隠蔽と PIMPL パターン**
   - Objective-C / AV Foundation のヘッダや型（`AVCaptureSession`, `AVCaptureDevice` 等）を [CamAvf.h](../CamAvf.h) に一切露出させず、内部実装構造体 `CamAvf::Impl` (PIMPL) に閉じ込めます。これにより、純粋な C++17 ソースファイルからインクルード可能です。
2. **ゼロコピー色変換 (`kCVPixelFormatType_32BGRA`)**
   - `AVCaptureVideoDataOutput` の `videoSettings` に `kCVPixelFormatType_32BGRA` を指定することで、カメラからの YUV / MJPG 信号を OS / ハードウェアデコーダ内部で直接 32-bit BGRA へ高速変換します。CPU ソフトウェアによる無駄なピクセル走査・色変換を完全に排除します。
3. **NVI (Non-Virtual Interface) によるライフサイクルの一元管理**
   - 基底クラス `Camera::start()`, `Camera::stop()`, `Camera::close()` が排他制御、スレッド状態フラグ（`running`）、およびスレッド合流（`thr.join()`）などの共通ライフサイクルを一元管理し、`CamAvf` は保護フック関数 `onStart()`, `onStop()`, `onClose()` にセッションの開始・停止のみを記述します。
4. **遅延初期化 (Lazy Initialization)**
   - アプリ起動時やフォーマット列挙時にはセッション開始やハードウェア確保を行わず、ユーザーが「開始」を指示したタイミングで初めて入出力を構築・起動します。
5. **レイテンシ優先と全フレーム処理の動的制御**
   - `alwaysDiscardsLateVideoFrames` プロパティと連携し、レイテンシ優先モードではキューに滞留した古いフレームを即座に破棄して最新フレームを提供します。
6. **macOS TCC (Transparency, Consent, and Control) 権限の自動ハンドリング**
   - 初回起動時にカメラのアクセス許可が決定されていない場合、自動的にシステム許可リクエストダイアログをトリガーし、ユーザーの応答を同期的に待機します。
7. **デバイス名サニタイズによる ImGui 文字化け防止**
   - カメラ名に含まれる可能性のある制御文字の空白置換、タイポグラフィック引用符（‘, ’, “, ”）の標準 ASCII（', "）への正規化、フォント未収録の 4 バイト絵文字の除外を行い、ImGui での表示崩れを防ぎます。

---

## 2. クラス構造とコンポーネント

### 2.1 ネストクラス・構造体

#### `CamAvf::VideoFormat` (内部構造体)
デバイスがサポートする解像度、フレームレート、コーデック、および AV Foundation の内部インデックスを保持します。
```cpp
struct VideoFormat
{
  int width{ 0 };        ///< 幅
  int height{ 0 };       ///< 高さ
  double fps{ 0.0 };     ///< フレームレート
  std::string codec;     ///< コーデック名 (NV12, YUY2, BGRA, MJPG, H264 等)
  int formatIndex{ 0 };  ///< AVCaptureDevice.formats のインデックス
  int rangeIndex{ 0 };   ///< videoSupportedFrameRateRanges のインデックス
};
```

---

### 2.2 内部デリゲートクラス (`CamAvfDelegate`)

- **対象ソース**: [CamAvf.mm:L45-L89](../CamAvf.mm#L45-L89)

```objc
@interface CamAvfDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
  CamAvf* parent; ///< キャプチャを処理する C++ クラスへのポインタ
}
- (id)initWithParent:(CamAvf*)p;
@end
```
AV Foundation では、新しいフレームは `AVCaptureVideoDataOutputSampleBufferDelegate` プロトコルの `captureOutput:didOutputSampleBuffer:fromConnection:` を介してコールバックされます。
`CamAvfDelegate` はこの Objective-C デリゲートを受け取り、C++ インスタンスの `handleSampleBuffer()` へ橋渡しします。

---

### 2.3 内部実装構造体 (`CamAvf::Impl` - PIMPL)

- **対象ソース**: [CamAvf.mm:L97-L150](../CamAvf.mm#L97-L150)

```cpp
struct CamAvf::Impl
{
  AVCaptureSession* session{ nil };           ///< キャプチャデータフローを統括するセッション
  AVCaptureDevice* device{ nil };             ///< 選択されたビデオキャプチャデバイス
  AVCaptureDeviceInput* input{ nil };         ///< セッションへデバイスを接続する入力
  AVCaptureVideoDataOutput* output{ nil };    ///< ビデオフレームを取得する出力
  CamAvfDelegate* delegate{ nil };           ///< フレーム受信を中継するデリゲート
  dispatch_queue_t queue{ nil };              ///< フレームキャプチャ専用のシリアルディスパッチキュー
  int deviceIndex{ -1 };                      ///< 選択されているデバイスのインデックス番号
};
```
デストラクタ内で `stopRunning`、`removeInput`、`removeOutput`、`setSampleBufferDelegate:nil` を順序通りに実行し、Objective-C ARC 環境下における循環参照やリソースリークを完全に防ぎます。

---

### 2.4 補助関数

#### `getVideoDevices()`
- **対象ソース**: [CamAvf.mm:L161-L191](../CamAvf.mm#L161-L191)
macOS 14 (Sonoma) 以降で導入された `AVCaptureDeviceTypeExternal`、`AVCaptureDeviceTypeContinuityCamera`、および macOS 13 以前の `AVCaptureDeviceTypeExternalUnknown` を `@available(macOS 14.0, *)` で動的に切り替え、内蔵カメラ、USB カメラ、iPhone 連係カメラのすべてを漏れなく検索します。

#### `sanitizeDeviceName(const std::string& name)`
- **対象ソース**: [CamAvf.mm:L242-L350](../CamAvf.mm#L242-L350)
UTF-8 バイト列を走査し、0x00〜0x1F の制御文字をスペースへ置換、スマートクォート（U+2018/2019, U+201C/201D）を標準の `'` や `"` へ置換、4 バイト絵文字（0xF0〜0xF7）を除去します。

---

## 3. 主要処理フローとソースコード解説

### 3.1 TCC 権限確認とデバイス列挙 (`getDeviceList`)
- **対象ソース**: [CamAvf.mm:L355-L399](../CamAvf.mm#L355-L399)

```cpp
const std::vector<std::string>& CamAvf::getDeviceList()
```
1. `[AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]` を呼び出し、現在のカメラアクセス許可状態を確認します。
2. 初回起動などで状態が `AVAuthorizationStatusNotDetermined` の場合、`dispatch_semaphore_t` を用いて同期ブロックを形成し、`requestAccessForMediaType:completionHandler:` によりシステム許可ダイアログを表示してユーザーの選択を待機します。
3. `getVideoDevices()` で得られた `AVCaptureDevice` の配列を走査し、`dev.localizedName` を `sanitizeDeviceName` で正規化します。
4. 重複名称を UI で一意に識別するため、`"デバイス名##インデックス"` の書式で `deviceList` に格納します。

---

### 3.2 カメラオープンと遅延初期化 (`open`)
- **対象ソース**: [CamAvf.mm:L404-L453](../CamAvf.mm#L404-L453)

```cpp
bool CamAvf::open(int deviceNumber, bool setupFormat)
```
1. `getDeviceByIndex(deviceNumber)` で目的の `AVCaptureDevice` を取得します。
2. `impl = std::make_unique<Impl>()` で内部状態を割り当て、デバイスを保持します。
3. `enumerateFormats()` を呼び出し、カメラがサポートする全フォーマットを解析します。
4. `setupFormat` が `false`（遅延初期化）の場合、重いセッション初期化は行わず、先頭フォーマットの幅・高さ・FPS・画像バッファサイズのみを準備して復帰します。

---

### 3.3 フォーマット列挙とコーデック・FPS 抽出 (`enumerateFormats`)
- **対象ソース**: [CamAvf.mm:L458-L578](../CamAvf.mm#L458-L578)

```cpp
bool CamAvf::enumerateFormats()
```
1. `impl->device.formats`（`AVCaptureDeviceFormat` の配列）を走査します。
2. `CMVideoFormatDescriptionGetDimensions(format.formatDescription)` から解像度（幅・高さ）を取得します。
3. `CMFormatDescriptionGetMediaSubType` から FourCC コーデック（`kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange` ＝ NV12、`kCMVideoCodecType_JPEG` ＝ MJPG 等）を判定します。
4. `format.videoSupportedFrameRateRanges` を走査し、`range.maxFrameRate` が 5.0 fps 以上のものを抽出します。
5. 解像度、FPS、コーデック名で重複を除外し、UI 用の `formatList`（`CaptureFormat`）および内部用の `availableFormats` に登録します。

---

### 3.4 フォーマット選択とハードウェア設定 (`select`)
- **対象ソース**: [CamAvf.mm:L583-L638](../CamAvf.mm#L583-L638)

```cpp
bool CamAvf::select(int index)
```
1. キャプチャ実行中の場合は一度 `stop()` します。
2. `[impl->device lockForConfiguration:&error]` でデバイスの排他設定権限限を取得します。
3. `impl->device.activeFormat = fmt;` により選択フォーマットを適用します。
4. `CMTimeMake(1, fpsInt)` を生成し、`activeVideoMinFrameDuration` および `activeVideoMaxFrameDuration` に同一のフレーム期間を設定して FPS を固定します。
5. `[impl->device unlockForConfiguration]` で設定を確定・解放します。
6. `width`, `height`, `image` バッファを更新し、以前実行中だった場合は `start()` を再開します。

---

### 3.5 キャプチャ開始とセッション構築 (`onStart`)
- **対象ソース**: [CamAvf.mm:L660-L742](../CamAvf.mm#L660-L742)

```cpp
bool CamAvf::onStart()
```
1. `impl->session` が未作成の場合、`AVCaptureSession` をインスタンス化し、`beginConfiguration` を呼び出します。
2. `AVCaptureDeviceInput` を生成し、`[impl->session addInput:impl->input]` で接続します。
3. `AVCaptureVideoDataOutput` を生成し、`videoSettings` に `kCVPixelFormatType_32BGRA` を設定します。
4. `alwaysDiscardsLateVideoFrames` を `prioritizeLatency` フラグに応じて設定します。
5. 専用のシリアルキュー `"CamAvfQueue"` を作成し、`setSampleBufferDelegate:queue:` でデリゲートを登録します。
6. `[impl->session addOutput:impl->output]` で出力を接続し、`commitConfiguration` で確定します。
7. デバイスの排他ロックを取得して選択フォーマットと FPS を最終適用します。
8. `[impl->session startRunning]` を呼び出してバックグラウンドキャプチャを開始します。

---

### 3.6 サンプルバッファ受信とストライド・色変換 (`handleSampleBuffer`)
- **対象ソース**: [CamAvf.mm:L788-L856](../CamAvf.mm#L788-L856)

```cpp
void CamAvf::handleSampleBuffer(const void* sampleBufferRef)
```
1. 全フレーム処理モード (`prioritizeLatency == false`) の場合、メインスレッドが前フレームを消費する（`captured == false`）まで待機ループを回し、取りこぼしを防ぎます。
2. `CMSampleBufferGetImageBuffer` で `CVImageBufferRef` (`CVPixelBufferRef`) を取得します。
3. `CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly)` で CPU アドレスをロックします。
4. `CVPixelBufferGetBytesPerRow` で行あたりのバイト数（ストライド）を取得します。
5. メインスレッドとの排他ロック `std::lock_guard<std::mutex> lock{ mtx }` を取得します。
6. **ストライド処理**:
   - `bytesPerRow == width * 4` の場合：パディングがないため単一の `std::memcpy` で一括コピー。
   - `bytesPerRow > width * 4` の場合：各行の末尾にアライメントパディングが含まれるため、行ごとに安全に `std::memcpy` して連続バッファ `image` へ再配置。
7. `captured = true` を設定し、`CVPixelBufferUnlockBaseAddress` でバッファをアンロックします。

---

### 3.7 キャプチャ停止とクリーンアップ (`onStop`, `onClose`, `~Impl`)
- **対象ソース**: [CamAvf.mm:L747-L784](../CamAvf.mm#L747-L784)

```cpp
void CamAvf::onStop()
void CamAvf::onClose()
```
1. `onStop()`: `[impl->session stopRunning]` を呼び出し、`dispatch_sync(impl->queue, ^{})` でディスパッチキュー内に残っているフレームコールバックの完了を確実に同期待機します。これにより、停止直後の不正メモリアクセスを防止します。
2. `onClose()`: `impl.reset()` により `Impl` のデストラクタが走り、セッションの入出力解除とリソース破棄が行われます。

---

## 4. AV Foundation / Core Media / Core Video / GCD 完全リファレンス

### 4.1 AV Foundation (Objective-C クラス)

#### `AVCaptureDeviceDiscoverySession`
- **`+discoverySessionWithDeviceTypes:mediaType:position:`**
  - **概要**: 条件に一致するキャプチャデバイスを検索・列挙するセッションを作成します。
  - **引数**:
    - `deviceTypes` (`NSArray<AVCaptureDeviceType>*`): 検索対象のデバイスタイプ（`BuiltInWideAngleCamera`, `External`, `ContinuityCamera` 等）。
    - `mediaType` (`AVMediaType`): メディア種別（`AVMediaTypeVideo`）。
    - `position` (`AVCaptureDevicePosition`): カメラの位置（`AVCaptureDevicePositionUnspecified`）。
  - **戻り値**: 発見セッションオブジェクト。

#### `AVCaptureDevice`
- **`+authorizationStatusForMediaType:`**
  - **概要**: 指定メディアタイプに対するアプリのアクセス許可状態（TCC）を取得します。
  - **引数**: `mediaType` (`AVMediaType`): `AVMediaTypeVideo`。
  - **戻り値** (`AVAuthorizationStatus`): `Authorized`, `Denied`, `Restricted`, `NotDetermined`。
- **`+requestAccessForMediaType:completionHandler:`**
  - **概要**: ユーザーにアクセス許可を求めるダイアログを表示し、非同期に結果を返します。
- **`-lockForConfiguration:` / `-unlockForConfiguration`**
  - **概要**: デバイスのプロパティ（フォーマット、FPS 等）を変更するための排他設定ロックを取得／解除します。
  - **引数**: `error` (`NSError**`): エラー情報を受け取るポインタ。
  - **戻り値** (`BOOL`): 成功時 `YES`。
- **`-activeFormat`**
  - **概要**: 現在ハードウェアに適用されている `AVCaptureDeviceFormat` を取得・設定します。
- **`-activeVideoMinFrameDuration` / `-activeVideoMaxFrameDuration`**
  - **概要**: フレーム更新の間隔（`CMTime`）の最小値・最大値を設定します。両者に同一値を設定することで固定フレームレート化します。

#### `AVCaptureSession`
- **`-beginConfiguration` / `-commitConfiguration`**
  - **概要**: 入出力の追加・削除などのセッション変更をバッチでアトミックに実行します。
- **`-addInput:` / `-removeInput:`**
  - **概要**: デバイス入力（`AVCaptureDeviceInput`）をセッションへ接続／切断します。
- **`-addOutput:` / `-removeOutput:`**
  - **概要**: データ出力（`AVCaptureVideoDataOutput`）をセッションへ接続／切断します。
- **`-startRunning` / `-stopRunning`**
  - **概要**: キャプチャデータフローを開始／停止します。

#### `AVCaptureVideoDataOutput`
- **`-videoSettings`**
  - **概要**: 出力ピクセル形式を定義するディクショナリを設定します。
  - **CamAvf での指定**: `@{ (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA) }`。
- **`-alwaysDiscardsLateVideoFrames`**
  - **概要**: キューが混雑した際に遅延フレームを破棄するかどうかを制御します。レイテンシ優先時は `YES`、全フレーム処理時は `NO`。
- **`-setSampleBufferDelegate:queue:`**
  - **概要**: フレーム受信コールバックを呼び出すデリゲートオブジェクトとディスパッチキューを設定します。

---

### 4.2 Core Media / Core Video (C API)

#### `CMVideoFormatDescriptionGetDimensions`
- **シグネチャ**: `CMVideoDimensions CMVideoFormatDescriptionGetDimensions(CMVideoFormatDescriptionRef videoDesc)`
- **概要**: ビデオフォーマット記述子から幅と高さ（ピクセル単位）を取得します。

#### `CMFormatDescriptionGetMediaSubType`
- **シグネチャ**: `FourCharCode CMFormatDescriptionGetMediaSubType(CMMediaFormatDescriptionRef desc)`
- **概要**: フォーマットの FourCC コード（`'2vuy'`, `'yuvs'`, `'BGRA'`, `'jpeg'`, `'avc1'` 等）を取得します。

#### `CMTimeMake`
- **シグネチャ**: `CMTime CMTimeMake(int64_t value, int32_t timescale)`
- **概要**: 分子 `value`、分母 `timescale` から有理数表現の時間構造体 `CMTime` を作成します（例: 30 fps は `CMTimeMake(1, 30)`）。

#### `CMSampleBufferGetImageBuffer`
- **シグネチャ**: `CVImageBufferRef CMSampleBufferGetImageBuffer(CMSampleBufferRef sbuf)`
- **概要**: サンプルバッファから画像ピクセルバッファ（`CVPixelBufferRef`）への参照を取得します。

#### `CVPixelBufferLockBaseAddress` / `CVPixelBufferUnlockBaseAddress`
- **シグネチャ**: `CVReturn CVPixelBufferLockBaseAddress(CVPixelBufferRef pixelBuffer, CVPixelBufferLockFlags lockFlags)`
- **概要**: ピクセルバッファの物理メモリアドレスをロックして直接アクセス可能にします。`kCVPixelBufferLock_ReadOnly` を指定して読み取り専用ロックを取得します。

#### `CVPixelBufferGetBaseAddress`
- **シグネチャ**: `void* CVPixelBufferGetBaseAddress(CVPixelBufferRef pixelBuffer)`
- **概要**: ピクセルデータの先頭メモリアドレスポインタを取得します。

#### `CVPixelBufferGetBytesPerRow`
- **シグネチャ**: `size_t CVPixelBufferGetBytesPerRow(CVPixelBufferRef pixelBuffer)`
- **概要**: 画像の 1 行あたりの総バイト数（パディングを含むストライド幅）を取得します。

---

### 4.3 Grand Central Dispatch (GCD)

#### `dispatch_queue_create`
- **シグネチャ**: `dispatch_queue_t dispatch_queue_create(const char *label, dispatch_queue_attr_t attr)`
- **概要**: 専用のシリアルキュー（`DISPATCH_QUEUE_SERIAL`）を生成し、フレーム受信コールバックが順序通り排他的に実行されることを保証します。

#### `dispatch_sync`
- **シグネチャ**: `void dispatch_sync(dispatch_queue_t queue, dispatch_block_t block)`
- **概要**: 指定したキューに空ブロックを投入し、その実行完了を同期的に待機します。キャプチャ停止時（`onStop`）にキュー内の処理残りをフラッシュするために使用します。

---

## 5. まとめ

`CamAvf` クラスは、macOS ネイティブのマルチメディア基盤である AV Foundation の性能を最大限に引き出しつつ、以下の堅牢なアーキテクチャを実現しています：
- **PIMPL イディオムによる純粋な C++ ヘッダの維持**
- **ハードウェア BGRA 出力によるゼロコピー化**
- **遅延初期化と高速フォーマット列挙**
- **最新 macOS (Sonoma 以降) の外部・連係カメラへの完全適応**
- **TCC カメラ権限の自動ハンドリングと ImGui 表示用の文字列サニタイズ**

これにより、macOS プラットフォーム上において、OpenCV の制約を受けない極めて安定した超低遅延ビデオキャプチャ環境を提供しています。
