# `CamAndroid` クラス実装の完全解説

本ドキュメントでは、Android 環境において **Camera2 NDK API** および **MediaNDK (AImageReader)** を使用してビデオキャプチャを行う `CamAndroid` クラスの実装について、ソースコード ([CamAndroid.h](../CamAndroid.h), [CamAndroid.cpp](../CamAndroid.cpp)) と突き合わせながら詳細に解説します。

また、実装内で利用されている Android C/C++ ネイティブ API について、引数や戻り値を含めた包括的なリファレンスを掲載しています。

---

## 目次

- [1. 概要とプロジェクトにおける役割](#1-概要とプロジェクトにおける役割)
  - [1.1 Android Camera2 NDK とは](#11-android-camera2-ndk-とは)
  - [1.2 主な役割と設計方針](#12-主な役割と設計方針)
- [2. クラス構造とコンポーネント](#2-クラス構造とコンポーネント)
  - [2.1 主要メンバ変数とその責務](#21-主要メンバ変数とその責務)
  - [2.2 コールバック機構](#22-コールバック機構)
- [3. 主要処理フローとソースコード解説](#3-主要処理フローとソースコード解説)
  - [3.1 カメラデバイスの列挙と向き判定 (`getDeviceList`)](#31-カメラデバイスの列挙と向き判定-getdevicelist)
  - [3.2 カメラオープンと遅延初期化 (`open`)](#32-カメラオープンと遅延初期化-open)
  - [3.3 メタデータ解析とフォーマット列挙 (`enumerateFormats`)](#33-メタデータ解析とフォーマット列挙-enumerateformats)
  - [3.4 フォーマット選択 (`selectFormat`)](#34-フォーマット選択-selectformat)
  - [3.5 キャプチャ開始と Camera2 パイプライン構築 (`onStart`)](#35-キャプチャ開始と-camera2-パイプライン構築-onstart)
  - [3.6 画像取得コールバックと YUV420_888 → BGRA 色変換 (`onImageAvailableCallback`, `convertYuvToBgra`)](#36-画像取得コールバックと-yuv420_888--bgra-色変換-onimageavailablecallback-convertyuvtobgra)
  - [3.7 キャプチャ停止とリソース解放 (`onStop`, `onClose`)](#37-キャプチャ停止とリソース解放-onstop-onclose)
- [4. Android Camera2 NDK & MediaNDK 完全リファレンス](#4-android-camera2-ndk--mediandk-完全リファレンス)
  - [4.1 ACameraManager](#41-acameramanager)
  - [4.2 ACameraMetadata](#42-acamerametadata)
  - [4.3 ACameraDevice](#43-acameradevice)
  - [4.4 ACaptureRequest](#44-acapturerequest)
  - [4.5 ACameraCaptureSession](#45-acameracapturesession)
  - [4.6 AImageReader & AImage (MediaNDK)](#46-aimagereader--aimage-mediandk)
- [5. まとめ](#5-まとめ)

---

## 1. 概要とプロジェクトにおける役割

`CamAndroid` クラスは、基底クラス [Camera](../Camera.h) を継承し、Android スマートフォンやタブレットの実機において、Android NDK のネイティブカメラスタックである **Camera2 NDK** と **MediaNDK** を利用してリアルタイムにビデオフレームを取得・処理するクラスです。

```
+-------------------------------------------------------------------------------+
|                            calib / mfcapture                                  |
|                                                                               |
|  +--------------------+    +--------------------+    +---------------------+  |
|  |     CamMf (Win)    |    |   CamAvf (macOS)   |    | CamAndroid (Android)|  |
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

### 1.1 Android Camera2 NDK とは

Android 5.0 (API Level 21) 以降で導入された **Camera2 API** は、旧来の Camera API (Camera1) を刷新したパイプラインベースの低レイヤカメラ制御フレームワークです。各フレームのリクエストパラメータ（露出、感度、焦点距離、ホワイトバランスなど）と画像出力サーフェスを柔軟にバインドできます。

Android NDK では、API Level 24 (Android 7.0) 以降で **Camera2 NDK** (`<camera/NdkCamera*.h>`) および **MediaNDK** (`<media/NdkImage*.h>`) が標準提供されており、Java / JNI レイヤを一切介さずに、C++ ネイティブコードのみでカメラの列挙・オープン・ストリーム構成・フレーム取得を行えます。本プロジェクトのような `NativeActivity` ベースの高速 OpenGL ES アプリケーションに最適化されています。

### 1.2 主な役割と設計方針

1. **JNI 非依存の純粋な C++ 実装**
   - Java VM や JNI 環境ポインタ (`JNIEnv*`)、Java クラスの呼び出しを必要とせず、標準 C++ と Android NDK C API のみで完結しています。
2. **NVI (Non-Virtual Interface) によるライフサイクルの一元管理**
   - 基底クラス `Camera::start()`, `Camera::stop()`, `Camera::close()` が排他制御、スレッド状態フラグ（`running`）、およびスレッド合流（`thr.join()`）などの共通ライフサイクルを一元管理し、`CamAndroid` は保護フック関数 `onStart()`, `onStop()`, `onClose()` にハードウェアセッション固有の処理のみをオーバーライドします。
3. **遅延初期化 (Lazy Initialization)**
   - カメラ認識時やフォーマット列挙時にはハードウェアのオープンやバッファ確保を行わず、メタデータ (`ACameraMetadata`) の静的解析のみを実施します。ユーザーが「開始」を指示したタイミングで初めて `AImageReader` と `ACameraDevice` を構築します。
4. **レイテンシ優先モードと全フレーム処理モードの切り替え**
   - `prioritizeLatency` フラグと連携し、低遅延モードでは `AImageReader_acquireLatestImage` を使用してバッファ内に滞留した古いフレームを自動的に破棄し、常に最新の映像を取得します。
   - 全フレーム処理モードでは `AImageReader_acquireNextImage` を使用して取りこぼしのないフレーム受信を行います。
5. **高速な YUV420_888 → BGRA8888 整数色変換**
   - Android のカメラ出力標準形式である `AIMAGE_FORMAT_YUV_420_888` から、OpenGL / OpenCV が要求する 32-bit BGRA への変換を、固定小数点演算（ビットシフト `>> 8`）とピクセルストライド追従により CPU 上で高速に実行します。
6. **非ブロッキング排他ロックによるゼロコピー連携**
   - フレーム変換時は `std::unique_lock<std::mutex> lock(mtx, std::try_to_lock)` による非ブロッキングロックを行い、メイン描画スレッドが `Camera::lockFrame()` 経由で PBO やテクスチャへ転送している最中は無駄なロック待機を回避して最新フレームを追従します。

---

## 2. クラス構造とコンポーネント

### 2.1 主要メンバ変数とその責務

- **対象ソース**: [CamAndroid.h:L36-L73](../CamAndroid.h#L36-L73)

| メンバ変数 | 型 | 役割・責務 |
|:---|:---|:---|
| `cameraManager` | `ACameraManager*` | カメラデバイスの列挙、メタデータ取得、デバイスオープンを統括するマネージャ |
| `cameraDevice` | `ACameraDevice*` | 接続された物理カメラデバイス（オープン時に生成） |
| `captureSession` | `ACameraCaptureSession*` | キャプチャリクエストと出力サーフェス間のストリームデータフローを管理 |
| `captureRequest` | `ACaptureRequest*` | 連続キャプチャパラメータ（`TEMPLATE_PREVIEW`）を定義したリクエスト |
| `outputContainer` | `ACaptureSessionOutputContainer*` | セッションに追加する出力ターゲットを保持するコンテナ |
| `sessionOutput` | `ACaptureSessionOutput*` | 単一の出力エンドポイント |
| `outputTarget` | `ACameraOutputTarget*` | キャプチャリクエストに関連付けられた出力先 |
| `imageReader` | `AImageReader*` | CPU メモリでフレームバッファを直接受け取るサーフェスレシーバ |
| `imageWindow` | `ANativeWindow*` | `imageReader` から取得したネイティブ描画サーフェス |
| `selectedCameraId` | `std::string` | 選択されているカメラ ID（"0", "1" 等） |
| `formatList` | `std::vector<CaptureFormat>` | UI 表示および選択用の構造化フォーマットリスト |
| `deviceList` | `std::vector<std::string>` | 列挙されたカメラデバイスの表示名リスト |

---

### 2.2 コールバック機構

#### `AImageReader_ImageListener`
新しい画像フレームが `imageReader` に到着した際に、Android 内部スレッドから `onImageAvailableCallback()` が非同期に呼び出されます。
```cpp
AImageReader_ImageListener listener;
listener.context = this;
listener.onImageAvailable = onImageAvailableCallback;
AImageReader_setImageListener(imageReader, &listener);
```

#### `ACameraDevice_stateCallbacks`
カメラデバイスの接続切断やハードウェアエラー発生時に通知を受け取ります。
```cpp
static ACameraDevice_stateCallbacks devCallbacks;
devCallbacks.context = this;
devCallbacks.onDisconnected = [](void*, ACameraDevice*) {};
devCallbacks.onError = [](void*, ACameraDevice*, int error) {
  LOGE("CameraDevice error: %d", error);
};
```

---

## 3. 主要処理フローとソースコード解説

### 3.1 カメラデバイスの列挙と向き判定 (`getDeviceList`)
- **対象ソース**: [CamAndroid.cpp:L41-L86](../CamAndroid.cpp#L41-L86)

```cpp
const std::vector<std::string>& CamAndroid::getDeviceList()
```
1. `ACameraManager_create()` でマネージャインスタンスを生成します。
2. `ACameraManager_getCameraIdList(mgr, &idList)` を呼び出し、利用可能なすべてのカメラ ID 配列を取得します。
3. 各カメラ ID に対し、`ACameraManager_getCameraCharacteristics(mgr, id, &chars)` を呼び出して特性メタデータを取得します。
4. `ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &entry)` を取得し、カメラの物理的な向きを判定します：
   - `ACAMERA_LENS_FACING_BACK`: 背面カメラ (`"Back"`)
   - `ACAMERA_LENS_FACING_FRONT`: 前面カメラ (`"Front"`)
   - `ACAMERA_LENS_FACING_EXTERNAL`: 外付けカメラ (`"External"`)
5. UI 表示用に `"0: Camera 0 (Back)"` のような識別文字列を構築して `deviceList` に追加します。
6. `ACameraManager_deleteCameraIdList()` および `ACameraManager_delete()` で列挙用リソースを即座に解放します。

---

### 3.2 カメラオープンと遅延初期化 (`open`)
- **対象ソース**: [CamAndroid.cpp:L91-L145](../CamAndroid.cpp#L91-L145)

```cpp
bool CamAndroid::open(int deviceNumber, int initial_width, int initial_height, double initial_fps)
```
1. 既存の接続があれば `close()` で終了します。
2. `ACameraManager_create()` を呼び出し、永続マネージャを保持します。
3. 指定インデックスのカメラ ID を `selectedCameraId` として保持します。
4. `ACameraManager_getCameraCharacteristics()` を取得し、`enumerateFormats(chars)` を呼び出してサポート解像度の一覧を構築します。
5. 解像度・FPS・チャンネル数（BGRA: 4）の既定値をプロパティへ設定します。
6. **遅延初期化**: この時点ではハードウェアデバイスのオープン（`ACameraManager_openCamera`）は行わず、軽量なメタデータ取得のみで復帰します。

---

### 3.3 メタデータ解析とフォーマット列挙 (`enumerateFormats`)
- **対象ソース**: [CamAndroid.cpp:L150-L205](../CamAndroid.cpp#L150-L205)

```cpp
void CamAndroid::enumerateFormats(ACameraMetadata* metadata)
```
1. `ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry)` を取得します。
2. このタグのデータ配列は、`(format, width, height, isInput)` の 4 要素が 1 組として格納されています。
3. `fmt == AIMAGE_FORMAT_YUV_420_888` かつ `isInput == 0`（出力ストリーム）のエントリのみを抽出します。
4. 重複解像度を除外し、解像度の降順（面積 `w * h` の大きい順）にソートして `formatList`（`CaptureFormat` 構造体）に登録します。

---

### 3.4 フォーマット選択 (`selectFormat`)
- **対象ソース**: [CamAndroid.cpp:L210-L228](../CamAndroid.cpp#L210-L228)

```cpp
bool CamAndroid::selectFormat(int index)
```
1. 指定されたインデックスから解像度文字列（例: `"1920 x 1080"`）をパースします。
2. キャプチャ実行中の場合は一度 `stop()` します。
3. `width`, `height` を更新し、実行中だった場合は `start()` を呼び出してパイプラインを再構築します。

---

### 3.5 キャプチャ開始と Camera2 パイプライン構築 (`onStart`)
- **対象ソース**: [CamAndroid.cpp:L233-L320](../CamAndroid.cpp#L233-L320)

```cpp
bool CamAndroid::onStart()
```
本メソッドで Camera2 NDK の一連のパイプラインオブジェクトを順序通りに構築します：

1. **`AImageReader` の生成**:
   - `AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, 4, &imageReader)` を呼び出します（最大同時バッファ数: 4）。
   - `AImageReader_setImageListener` でリスナーを登録します。
   - `AImageReader_getWindow(imageReader, &imageWindow)` により、サーフェスとなる `ANativeWindow` を取得します。
2. **`ACameraDevice` のオープン**:
   - `ACameraManager_openCamera(cameraManager, selectedCameraId.c_str(), &devCallbacks, &cameraDevice)` を呼び出します。
   - ※カメラ権限が付与されていない場合、ここで `ACAMERA_ERROR_PERMISSION_DENIED` となり失敗します。
3. **`ACaptureRequest` の作成**:
   - `ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &captureRequest)` を呼び出し、リアルタイムプレビュー用の連続キャプチャテンプレートを作成します。
4. **出力ターゲットの関連付け**:
   - `ACameraOutputTarget_create(imageWindow, &outputTarget)` でターゲットを生成し、`ACaptureRequest_addTarget(captureRequest, outputTarget)` でリクエストに出力を紐付けます。
5. **セッション出力コンテナの構成**:
   - `ACaptureSessionOutputContainer_create(&outputContainer)`
   - `ACaptureSessionOutput_create(imageWindow, &sessionOutput)`
   - `ACaptureSessionOutputContainer_add(outputContainer, sessionOutput)`
6. **`ACameraCaptureSession` の作成**:
   - `ACameraDevice_createCaptureSession(cameraDevice, outputContainer, &sessionCallbacks, &captureSession)` を呼び出します。
7. **連続リクエストの開始**:
   - `ACameraCaptureSession_setRepeatingRequest(captureSession, nullptr, 1, &captureRequest, nullptr)` を呼び出して、カメラからの連続フレームキャプチャを開始します。

---

### 3.6 画像取得コールバックと YUV420_888 → BGRA 色変換 (`onImageAvailableCallback`, `convertYuvToBgra`)
- **対象ソース**: [CamAndroid.cpp:L400-L505](../CamAndroid.cpp#L400-L505)

#### コールバック処理
```cpp
void CamAndroid::onImageAvailableCallback(void* context, AImageReader* reader)
```
- `prioritizeLatency == true` の場合：
  `AImageReader_acquireLatestImage(reader, &image)` を呼び出し、最新フレームのみを取得して古いフレームを破棄します。
- `prioritizeLatency == false` の場合：
  `AImageReader_acquireNextImage(reader, &image)` を呼び出し、到着順にすべてのフレームを取得します。
- 取得した `AImage` を `convertYuvToBgra(image)` へ引き渡し、処理完了後に `AImage_delete(image)` で確実に解放します。

#### YUV420_888 → BGRA8888 変換ループ
`AImage` の YUV420_888 形式は、Y/U/V の各プレーンが個別のバッファポインタとストライドを持っています（端末によって U/V がインターリーブされている Semi-Planar / NV21 構造や、独立した Planar 構造が存在します）。

`CamAndroid` では各プレーンの `RowStride` および `PixelStride` を取得して直接インデックス計算を行うため、あらゆる端末の YUV 配置に自動適応します：

```cpp
AImage_getPlaneData(img, 0, &yPlane, &yLen);
AImage_getPlaneRowStride(img, 0, &yRowStride);

AImage_getPlaneData(img, 1, &uPlane, &uLen);
AImage_getPlaneRowStride(img, 1, &uRowStride);
AImage_getPlanePixelStride(img, 1, &uPixelStride); // セミプラナーなら 2、プラナーなら 1

AImage_getPlaneData(img, 2, &vPlane, &vLen);
AImage_getPlaneRowStride(img, 2, &vRowStride);
AImage_getPlanePixelStride(img, 2, &vPixelStride);
```

ITU-R BT.601 規格に基づく整数固定小数点変換式：
```cpp
const int C = Y - 16;
const int D = U - 128;
const int E = V - 128;

int R = (298 * C + 409 * E + 128) >> 8;
int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
int B = (298 * C + 516 * D + 128) >> 8;

dstRow[x * 4 + 0] = static_cast<uint8_t>(std::clamp(B, 0, 255));
dstRow[x * 4 + 1] = static_cast<uint8_t>(std::clamp(G, 0, 255));
dstRow[x * 4 + 2] = static_cast<uint8_t>(std::clamp(R, 0, 255));
dstRow[x * 4 + 3] = 255; // Alpha
```
変換結果は単一の BGRA バッファ `image` へ書き込まれ、`captured = true` が通知されます。

---

### 3.7 キャプチャ停止とリソース解放 (`onStop`, `onClose`)
- **対象ソース**: [CamAndroid.cpp:L326-L395](../CamAndroid.cpp#L326-L395)

パイプラインの依存関係の逆順で安全にリソースを解放します：
1. `ACameraCaptureSession_stopRepeating(captureSession)` で連続キャプチャを停止
2. `ACameraCaptureSession_close(captureSession)`
3. `ACaptureRequest_removeTarget` & `ACaptureRequest_free(captureRequest)`
4. `ACameraOutputTarget_free(outputTarget)`
5. `ACaptureSessionOutputContainer_remove` & `ACaptureSessionOutput_free(sessionOutput)`
6. `ACaptureSessionOutputContainer_free(outputContainer)`
7. `ACameraDevice_close(cameraDevice)` でカメラデバイスを閉じる
8. `AImageReader_setImageListener(imageReader, nullptr)` でリスナー解除
9. `AImageReader_delete(imageReader)` でリーダーとウィンドウを解放
10. `onClose()` ではさらに `ACameraManager_delete(cameraManager)` を実行

---

## 4. Android Camera2 NDK & MediaNDK 完全リファレンス

### 4.1 ACameraManager

#### `ACameraManager_create` / `ACameraManager_delete`
- **概要**: カメラマネージャのインスタンスを生成／破棄します。
- **シグネチャ**: `ACameraManager* ACameraManager_create()` / `void ACameraManager_delete(ACameraManager* manager)`

#### `ACameraManager_getCameraIdList` / `ACameraManager_deleteCameraIdList`
- **概要**: 利用可能なカメラデバイス ID のリストを取得／解放します。
- **シグネチャ**: `camera_status_t ACameraManager_getCameraIdList(ACameraManager* manager, ACameraIdList** cameraIdList)`

#### `ACameraManager_getCameraCharacteristics`
- **概要**: 指定したカメラ ID の静的メタデータ（特性）を取得します。
- **シグネチャ**: `camera_status_t ACameraManager_getCameraCharacteristics(ACameraManager* manager, const char* cameraId, ACameraMetadata** characteristics)`

#### `ACameraManager_openCamera`
- **概要**: カメラデバイスを非同期でオープンします。
- **シグネチャ**: `camera_status_t ACameraManager_openCamera(ACameraManager* manager, const char* cameraId, ACameraDevice_stateCallbacks* callback, ACameraDevice** device)`

---

### 4.2 ACameraMetadata

#### `ACameraMetadata_getConstEntry`
- **概要**: メタデータから特定のタグに対応するエントリを取得します。
- **シグネチャ**: `camera_status_t ACameraMetadata_getConstEntry(const ACameraMetadata* metadata, uint32_t tag, ACameraMetadata_const_entry* entry)`
- **CamAndroid で使用する主要タグ**:
  - `ACAMERA_LENS_FACING`: カメラの向き（Front/Back/External）
  - `ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS`: 利用可能な出力ストリーム形式、幅、高さ

#### `ACameraMetadata_free`
- **概要**: 取得したメタデータオブジェクトを解放します。
- **シグネチャ**: `void ACameraMetadata_free(ACameraMetadata* metadata)`

---

### 4.3 ACameraDevice

#### `ACameraDevice_createCaptureRequest`
- **概要**: 指定したテンプレート（用途）に基づいてキャプチャリクエストを作成します。
- **引数**: `templateId`: `TEMPLATE_PREVIEW`（リアルタイムプレビュー用）。
- **シグネチャ**: `camera_status_t ACameraDevice_createCaptureRequest(ACameraDevice* device, ACameraDevice_request_template templateId, ACaptureRequest** request)`

#### `ACameraDevice_createCaptureSession`
- **概要**: 出力コンテナを指定してキャプチャセッションを作成します。
- **シグネチャ**: `camera_status_t ACameraDevice_createCaptureSession(ACameraDevice* device, const ACaptureSessionOutputContainer* outputs, const ACameraCaptureSession_stateCallbacks* callbacks, ACameraCaptureSession** session)`

#### `ACameraDevice_close`
- **概要**: オープン中のカメラデバイスを閉じます。
- **シグネチャ**: `camera_status_t ACameraDevice_close(ACameraDevice* device)`

---

### 4.4 ACaptureRequest

#### `ACaptureRequest_addTarget` / `ACaptureRequest_removeTarget`
- **概要**: キャプチャリクエストに出力ターゲット（`ACameraOutputTarget`）を追加／削除します。

#### `ACaptureRequest_free`
- **概要**: キャプチャリクエストオブジェクトを解放します。

---

### 4.5 ACameraCaptureSession

#### `ACameraCaptureSession_setRepeatingRequest`
- **概要**: 連続キャプチャ（プレビューなど）のリクエストを設定します。フレーム到着ごとに自動的に新しいキャプチャが行われます。
- **シグネチャ**: `camera_status_t ACameraCaptureSession_setRepeatingRequest(ACameraCaptureSession* session, ACameraCaptureSession_captureCallbacks* callbacks, int numRequests, ACaptureRequest** requests, int* captureSequenceId)`

#### `ACameraCaptureSession_stopRepeating`
- **概要**: 連続キャプチャ要求を停止します。

#### `ACameraCaptureSession_close`
- **概要**: キャプチャセッションを閉じます。

---

### 4.6 AImageReader & AImage (MediaNDK)

#### `AImageReader_new`
- **概要**: 画像バッファ受信用に新しい `AImageReader` を作成します。
- **引数**: `width`, `height`, `format` (`AIMAGE_FORMAT_YUV_420_888`), `maxImages` (バッファキュー最大数)。
- **シグネチャ**: `media_status_t AImageReader_new(int32_t width, int32_t height, int32_t format, int32_t maxImages, AImageReader** reader)`

#### `AImageReader_setImageListener`
- **概要**: フレーム到着コールバック（`AImageReader_ImageListener`）を登録します。

#### `AImageReader_getWindow`
- **概要**: `AImageReader` からサーフェスとなる `ANativeWindow*` を取得します。カメラの出力先としてバインドします。

#### `AImageReader_acquireLatestImage`
- **概要**: キュー内の最新画像を取得し、未処理の古い画像を破棄します（レイテンシ優先時に使用）。

#### `AImageReader_acquireNextImage`
- **概要**: キュー内の次の画像を取得します（全フレーム処理時に使用）。

#### `AImage_getPlaneData` / `AImage_getPlaneRowStride` / `AImage_getPlanePixelStride`
- **概要**: 指定プレーン（0: Y, 1: U, 2: V）の先頭メモリアドレス、行ストライド、ピクセル間隔を取得します。

#### `AImage_delete`
- **概要**: 取得した `AImage` を解放し、バッファを `AImageReader` のキューへ返却します。

---

## 5. まとめ

`CamAndroid` クラスは、Android NDK の提供する低レイヤマルチメディア基盤を最大限に活用し、以下の優れた特性を備えています：
- **JNI を排除した完全ネイティブ C++ パイプライン**
- **遅延初期化による軽量な起動とフォーマット列挙**
- **最新フレーム破棄（`acquireLatestImage`）による超低遅延プレビュー**
- **あらゆる SoC の YUV プレーン配置（Semi-Planar / Planar）に自動適応する高速 BGRA 変換**
- **NVI 設計に準拠した堅牢なライフサイクル・リソース管理**

これにより、Android の `NativeActivity` 上で動作する画像処理およびコンピュータビジョンパイプラインに対し、安定した高品質なカメラフレームをリアルタイムに供給します。
