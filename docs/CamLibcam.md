# `CamLibcam` クラス実装の完全解説

本ドキュメントでは、`mfcapture` プロジェクトにおいて Raspberry Pi や組み込み Linux 環境でネイティブカメラスタックである **libcamera** を使用してビデオキャプチャを行う `CamLibcam` クラスの実装について、ソースコード ([CamLibcam.h](../CamLibcam.h), [CamLibcam.cpp](../CamLibcam.cpp)) と突き合わせながら詳細に解説します。

また、実装内で利用されている **libcamera C++ API** および **Linux POSIX システムコール**の各関数・インターフェースについて、引数や戻り値を含めた包括的なリファレンスを掲載しています。

---

## 目次

- [1. 概要とプロジェクトにおける役割](#1-概要とプロジェクトにおける役割)
  - [1.1 libcamera とは](#11-libcamera-とは)
  - [1.2 主な役割と設計方針](#12-主な役割と設計方針)
- [2. クラス構造とコンポーネント](#2-クラス構造とコンポーネント)
  - [2.1 ネストクラス・構造体](#21-ネストクラス構造体)
  - [2.2 主要メンバ変数とその責務](#22-主要メンバ変数とその責務)
- [3. 主要処理フローとソースコード解説](#3-主要処理フローとソースコード解説)
  - [3.1 CameraManager の初期化とデバイス列挙 (`Manager`)](#31-cameramanager-の初期化とデバイス列挙-manager)
  - [3.2 カメラのオープンとストリーム設定 (`open`)](#32-カメラのオープンとストリーム設定-open)
  - [3.3 バッファ確保と dmabuf メモリマッピング (`allocate` / `mmap`)](#33-バッファ確保と-dmabuf-メモリマッピング-allocate--mmap)
  - [3.4 キャプチャ開始と露出制御 (`onStart`)](#34-キャプチャ開始と露出制御-onstart)
  - [3.5 リクエスト完了コールバックと色空間変換 (`requestComplete`)](#35-リクエスト完了コールバックと色空間変換-requestcomplete)
  - [3.6 停止とクリーンアップ (`onStop`, `onClose`, `unmapBuffers`)](#36-停止とクリーンアップ-onstop-onclose-unmapbuffers)
- [4. libcamera C++ API & Linux システムコール完全リファレンス](#4-libcamera-c-api--linux-システムコール完全リファレンス)
  - [4.1 libcamera::CameraManager](#41-libcameracameramanager)
  - [4.2 libcamera::Camera](#42-libcameracamera)
  - [4.3 libcamera::CameraConfiguration & StreamConfiguration](#43-libcameracameraconfiguration--streamconfiguration)
  - [4.4 libcamera::FrameBufferAllocator & FrameBuffer](#44-libcameraframebufferallocator--framebuffer)
  - [4.5 libcamera::Request](#45-libcamerarequest)
  - [4.6 libcamera コントロール (`libcamera::controls`)](#46-libcamera-コントロール-libcameracontrols)
  - [4.7 Linux POSIX メモリマッピング API (`mmap`, `munmap`)](#47-linux-posix-メモリマッピング-api-mmap-munmap)

---

## 1. 概要とプロジェクトにおける役割

`CamLibcam` クラスは、基底クラス [Camera](../Camera.h) を継承し、Raspberry Pi OS (Bookworm 以降など) の Linux 環境において、Raspberry Pi 公式カメラモジュール (Camera Module v1/v2/v3, High Quality Camera, Global Shutter Camera 等) や各種 MIPI-CSI カメラから高速かつ低遅延に映像フレームを取得・処理するクラスです。

```
+-------------------------------------------------------------------------+
|                              mfcapture                                  |
|                                                                         |
|  +--------------------+    +--------------------+    +---------------+  |
|  |     CamMf (Win)    |    |   CamCv (Linux/Mac)|    |CamLibcam (RPi)|  |
|  +---------+----------+    +---------+----------+    +-------+-------+  |
|            |                         |                       |          |
|            +-------------------------+-----------------------+          |
|                                      |                                  |
|                                      v                                  |
|                         +--------------------------+                    |
|                         |    Camera (基底クラス)   |                    |
|                         +------------+-------------+                    |
+--------------------------------------|----------------------------------+
                                       |
                                       v
                    +------------------------------------+
                    |        Capture / Texture           |
                    | (PBO / GPU テクスチャ / GLSL展開)  |
                    +------------------------------------+
```

### 1.1 libcamera とは

従来の Linux におけるカメラアクセスは **V4L2 (Video for Linux 2)** が標準でした。しかし、近年のスマートフォンや Raspberry Pi などの組み込み SoC では、カメラセンサが直接 ISP (Image Signal Processor) に接続されており、オートフォーカス (AF)、自動露出 (AE)、オートホワイトバランス (AWB) などの 3A アルゴリズムを CPU 上のソフトウェアパイプラインで複雑に協調動作させる必要があります。

V4L2 の単純なデバイスドライバモデルではこれらを制御しきれなくなったため、オープンソースの新しいカメラスタックとして設計されたのが **libcamera** です。Raspberry Pi OS では Bullseye / Bookworm 以降、従来の Raspberry Pi カメラスタック (MMAL, Raspicam) に代わり、libcamera が標準となっています。

### 1.2 主な役割と設計方針

1. **libcamera 固有処理のカプセル化と NVI 設計**
   - `libcamera::CameraManager`、`libcamera::Camera`、`CameraConfiguration`、リクエスト・バッファキューなどの複雑なオブジェクト群を `CamLibcam` 内部に完全に隠蔽します。
   - NVI (Non-Virtual Interface) パターンを採用し、`Camera::start()`, `Camera::stop()`, `Camera::close()` が共通ライフサイクルを管理し、`CamLibcam` は保護フック `onStart()`, `onStop()`, `onClose()` をオーバーライドします。
   - 上位の `Capture` クラスからは、従来の `Camera` 基底クラスの共通インターフェース (`open`, `start`, `stop`, `close`, `retrieve`) を通じて透過的に扱えます。
2. **ゼロコピーと超高速バッファ変換**
   - libcamera が提供する DMA バッファ (`dmabuf`) ファイルディスクリプタを Linux の `mmap()` システムコールでユーザー空間メモリに直接マッピングし、不要なカーネル・ユーザー空間間コピーを排除します。
   - ピクセルフォーマットとして `XBGR8888` / `BGRX8888` (4バイト BGRA 互換) が利用可能な場合は、色変換処理を行わず単一の `std::memcpy()` だけで内部バッファへ高速転送します。
   - `Camera::lockFrame()` により、上位層へのデータ受け渡しにおいても不要な中間バッファや二重コピーを排除しています。
3. **幅広いセンサ・ピクセルフォーマットの自動適応**
   - カラーカメラだけでなく、グローバルシャッターモノクロセンサ (OV9281 等の `R8`)、一般的なカラーフォーマット (`BGR888`, `RGB888`)、YCbCr/YUV 形式 (`YUYV`, `NV12`, `YUV420`) に対応し、内部で共通の 4 チャンネル BGRA 形式に変換します。
4. **イベント駆動・非同期キャプチャとフレームレート維持**
   - libcamera のシグナル／スロット機構 (`requestCompleted` シグナル) を用いた非同期イベント駆動型モデルを採用しています。
   - 完了したリクエストは `request->reuse(ReuseBuffers)` を用いてバッファを再利用し、即座にキューへ再投入することでフレーム落ちやディレイを防ぎます。
   - `controls::FrameDurationLimits` コントロールを設定することで、AE (自動露出) がフレームレートを勝手に低下させる現象を抑止し、安定した FPS を維持します。
5. **シングルトンによる CameraManager の一元管理**
   - `libcamera::CameraManager` はプロセス内で単一のインスタンスとして動作することが推奨されます。ネストした内部クラス `Manager` をシングルトンとして設計し、複数回のオープン／クローズやデバイス列挙において安全なライフサイクル管理を実現しています。

---

## 2. クラス構造とコンポーネント

### 2.1 ネストクラス・構造体

#### `CamLibcam::Manager` (内部シングルトンクラス)
プロセス全体で単一の `libcamera::CameraManager` を生成・起動・停止するシングルトンクラスです。
```cpp
class Manager
{
  std::unique_ptr<libcamera::CameraManager> cm; ///< libcamera CameraManager
  std::vector<std::string> deviceList;          ///< 列挙されたデバイス表示名
  bool started{ false };                        ///< 起動成功フラグ

  Manager();
  ~Manager();

public:
  Manager(const Manager&) = delete;
  Manager& operator=(const Manager&) = delete;

  static Manager& getInstance();
  libcamera::CameraManager* get() { return cm.get(); }
  const std::vector<std::string>& getDeviceList();
};
```

#### `CamLibcam::MappedPlane` (内部構造体)
`dmabuf` のプレーンごとに `mmap()` されたメモリアドレスとサイズを管理する構造体です。
```cpp
struct MappedPlane
{
  void* address{ nullptr }; ///< mmap された仮想アドレス
  size_t length{ 0 };       ///< バッファ長 (バイト数)
};
```

### 2.2 主要メンバ変数とその責務

| メンバ変数 | 型 | 責務・役割 |
| :--- | :--- | :--- |
| `camera` | `std::shared_ptr<libcamera::Camera>` | 操作対象のカメラデバイス本体の制御ハンドル |
| `config` | `std::unique_ptr<libcamera::CameraConfiguration>` | ストリーム設定 (解像度、フォーマット、バッファ数など) |
| `allocator` | `std::unique_ptr<libcamera::FrameBufferAllocator>` | カメラデバイスに紐付くフレームバッファの確保・解放 |
| `requests` | `std::vector<std::unique_ptr<libcamera::Request>>` | キャプチャパイプラインに投入するリクエストオブジェクト群 |
| `mappedBuffers` | `std::map<const FrameBuffer*, std::vector<MappedPlane>>` | 各 FrameBuffer に対応する mmap アドレスのキャッシュ |
| `stream` | `libcamera::Stream*` | 構成されたアクティブなビデオストリームのポインタ |
| `pixelFormat` | `libcamera::PixelFormat` | センサ/ISP から出力される実際のピクセルフォーマット |
| `stride` | `unsigned int` | 1 行あたりのバイト幅 (パディングを含む) |
| `frameDurationUs` | `int64_t` | 目標フレーム時間 (マイクロ秒)。FPS 制御に使用 |
| `cv` / `frameReady` | `std::condition_variable` / `bool` | メインスレッドとコールバック間のフレーム同期用 |

---

## 3. 主要処理フローとソースコード解説

### 3.1 CameraManager の初期化とデバイス列挙 (`Manager`)

- **対象ソース**: [CamLibcam.cpp:L27-L66](CamLibcam.cpp#L27-L66)

```cpp
CamLibcam::Manager::Manager()
  : cm{ std::make_unique<libcamera::CameraManager>() }
{
  const int ret{ cm->start() };
  started = (ret == 0);
  if (!started)
  {
    std::cerr << "Failed to start libcamera CameraManager: " << ret << std::endl;
  }
}
```

1. **`cm->start()` の実行**
   - `CameraManager::start()` を呼び出すことで、システム内のすべてのパイプラインハンドラ (Raspberry Pi の場合は `rpi/vc4` または `rpi/pisp`) を初期化し、接続されているカメラモジュールを列挙します。
2. **`getDeviceList()` による一覧取得**
   - `cm->cameras()` から検出されたカメラ一覧を取得し、`"Camera 0: /base/soc/i2c0mux/..."` のような一意な ID 文字列を UI 表示用のリストとして構築します。

---

### 3.2 カメラのオープンとストリーム設定 (`open`)

- **対象ソース**: [CamLibcam.cpp:L81-L222](CamLibcam.cpp#L81-L222)

```cpp
bool CamLibcam::open(int deviceNumber, int initial_width, int initial_height, double initial_fps)
```

1. **カメラの獲得 (`acquire`)**
   ```cpp
   camera = cameras[deviceNumber];
   if (camera->acquire() != 0) { ... return false; }
   ```
   - 指定インデックスのカメラを排他ロックします。他プロセスがカメラを使用中の場合は失敗します。
2. **StreamRole のフォールバック試行**
   - カメラの構成を生成する際、`Viewfinder` (低遅延プレビュー)、`VideoRecording` (動画記録)、`Raw` の順で試行し、最適な構成を自動探索します。
3. **フォーマット優先順位の選定**
   ```cpp
   const std::vector<libcamera::PixelFormat> preferenceList = {
     libcamera::formats::R8,       // モノクロ 8-bit (OV9281 等)
     libcamera::formats::XBGR8888, // 4-byte BGRA 互換 (memcpy可能・最速)
     libcamera::formats::BGRX8888,
     libcamera::formats::XRGB8888,
     libcamera::formats::RGBX8888,
     libcamera::formats::BGR888,
     libcamera::formats::RGB888,
     libcamera::formats::YUYV,
     libcamera::formats::NV12,
     libcamera::formats::YUV420,
   };
   ```
   - センサがサポートするピクセルフォーマット一覧から、プロジェクト内部フォーマット (BGRA) と相性の良いフォーマットを優先的に選択します。特に `XBGR8888` が利用できる場合は後述の高速コピーが可能になります。
4. **設定の検証と適用 (`validate` / `configure`)**
   - `config->validate()` を実行し、ハードウェアが要求設定をそのまま扱えるか、または近傍の解像度に自動調整 (`Adjusted`) されたかを検証します。
   - `camera->configure(config.get())` により、ハードウェア ISP およびセンサにストリーム設定を反映します。

---

### 3.3 バッファ確保と dmabuf メモリマッピング (`allocate` / `mmap`)

- **対象ソース**: [CamLibcam.cpp:L228-L276](CamLibcam.cpp#L228-L276)

libcamera では、フレームバッファはカーネルの `dmabuf` (Direct Memory Access Buffer) としてアロケートされます。CPU からその画素データを参照するには、ファイルディスクリプタをメモリマップする必要があります。

```cpp
// フレームバッファアロケータの作成
allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
allocator->allocate(stream);

// バッファのメモリマッピング
for (const auto& buffer : allocator->buffers(stream))
{
  std::vector<MappedPlane> planes;
  for (const auto& plane : buffer->planes())
  {
    void* memory{ ::mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset) };
    if (memory == MAP_FAILED) memory = nullptr;
    planes.push_back({ memory, plane.length });
  }
  mappedBuffers[buffer.get()] = std::move(planes);

  // リクエストの作成とバッファの関連付け
  auto request{ camera->createRequest() };
  request->addBuffer(stream, buffer.get());
  requests.push_back(std::move(request));
}

// 完了シグナルに関数をバインド
camera->requestCompleted.connect(this, &CamLibcam::requestComplete);
```

- 各フレームバッファ (`FrameBuffer`) に対して `Request` を 1:1 で事前作成し、`mappedBuffers` にマップ先アドレスを記録しておくことで、実行時の動的確保をゼロに抑えています。

---

### 3.4 キャプチャ開始と露出制御 (`onStart`)

- **対象ソース**: [CamLibcam.cpp:L397-L424](CamLibcam.cpp#L397-L424)

```cpp
bool CamLibcam::onStart()
{
  if (!camera) return false;

  libcamera::ControlList startControls;
  // 自動露出 (AE) を有効化し、フレーム時間を指定
  startControls.set(libcamera::controls::AeEnable, true);
  startControls.set(libcamera::controls::FrameDurationLimits,
    libcamera::Span<const int64_t, 2>({ frameDurationUs, frameDurationUs }));

  if (camera->start(&startControls) < 0)
  {
    std::cerr << "libcamera: Failed to start camera" << std::endl;
    return false;
  }

  // すべてのリクエストをキューに投入
  for (auto& request : requests)
  {
    camera->queueRequest(request.get());
  }

  return true;
}
```

- **AE ハンティングと低フレームレートの防止**:
  - `libcamera::controls::FrameDurationLimits` に目標フレーム時間（例: 30fps なら 33,333µs）の下限と上限を同一値として設定します。これにより、暗所などで ISP の自動露出が勝手に露出時間を伸ばしてフレームレートが低下するのを防ぎます。
  - 事前に確保したすべてのリクエストを `camera->queueRequest()` でパイプラインに投入し、非同期ストリーミングを開始します。

---

### 3.5 リクエスト完了コールバックと色空間変換 (`requestComplete`)

- **対象ソース**: [CamLibcam.cpp:L351-L546](CamLibcam.cpp#L351-L546)

フレームがセンサから読み取られ、ISP 処理が完了すると、libcamera の内部ワーカースレッドから `requestComplete` コールバックが呼び出されます。

#### 1. 高速メモリコピー (`XBGR8888` / `BGRX8888`)
```cpp
else if (pixelFormat == libcamera::formats::XBGR8888 || pixelFormat == libcamera::formats::BGRX8888)
{
  if (stride == static_cast<unsigned int>(width * 4))
  {
    std::memcpy(dst, src, static_cast<size_t>(width * height * 4));
  }
  else
  {
    for (size_t y = 0; y < static_cast<size_t>(height); ++y)
    {
      std::memcpy(dst + y * width * 4, src + y * stride, width * 4);
    }
  }
}
```
パディングがない場合、フレーム全体を単一の `memcpy` でコピーし、Raspberry Pi の限られた CPU リソース消費を極小化します。

#### 2. モノクロ 8-bit (`R8`) 展開 (OV9281 等)
```cpp
else if (pixelFormat == libcamera::formats::R8)
{
  for (size_t y = 0; y < static_cast<size_t>(height); ++y)
  {
    const uint8_t* rowSrc{ src + y * stride };
    uint8_t* rowDst{ dst + y * width * 4 };
    for (size_t x = 0; x < static_cast<size_t>(width); ++x)
    {
      const uint8_t val = *rowSrc++;
      rowDst[0] = val; // B
      rowDst[1] = val; // G
      rowDst[2] = val; // R
      rowDst[3] = 255; // A
      rowDst += 4;
    }
  }
}
```
グローバルシャッターモノクロセンサの輝度値を BGRA の各色成分へコピーし、グレースケールとして表示します。

#### 3. YUYV 4:2:2 から BGRA への整数演算色変換
```cpp
const int y0 = rowSrc[0] - 16;
const int u  = rowSrc[1] - 128;
const int y1 = rowSrc[2] - 16;
const int v  = rowSrc[3] - 128;

rowDst[0] = static_cast<uint8_t>(std::clamp((298 * y0 + 516 * u + 128) >> 8, 0, 255)); // B
rowDst[1] = static_cast<uint8_t>(std::clamp((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255)); // G
rowDst[2] = static_cast<uint8_t>(std::clamp((298 * y0 + 409 * v + 128) >> 8, 0, 255)); // R
rowDst[3] = 255;
```
ITU-R BT.601 規格の YUV-RGB 変換式をビットシフトと整数演算を用いて高速に行います。

#### 4. リクエストの再利用 (`reuse`)
```cpp
if (running)
{
  request->reuse(libcamera::Request::ReuseBuffers);
  camera->queueRequest(request);
}
```
リクエストとバッファを再割り当てすることなく、そのままパイプラインへ再投入します。

---

### 3.6 停止とクリーンアップ (`onStop`, `onClose`, `unmapBuffers`)

- **対象ソース**: [CamLibcam.cpp:L358-L393, L427-L437](CamLibcam.cpp#L358-L393)

1. **`onStop()`**:
   - `camera->stop()` を呼び出してハードウェアストリーミングを停止します。
   - `running = false;` の設定や状態管理は基底クラス `Camera::stop()` が担当します。
2. **`onClose()`**:
   - `camera->requestCompleted.disconnect()` でシグナル接続を解除します。
   - `requests.clear()` でリクエストオブジェクトを破棄します。
   - `unmapBuffers()` で各プレーンの `::munmap()` を呼び出し、仮想メモリ空間を解放します。
   - `allocator.reset()` および `config.reset()` によりカーネル DMA バッファと構成オブジェクトを解放します。
   - `camera->release()` でカメラデバイスの排他ロックを解放します。
   - バッファのクリアやパラメータ初期化は基底クラス `Camera::close()` が一括管理します。

---

## 4. libcamera C++ API & Linux システムコール完全リファレンス

### 4.1 libcamera::CameraManager

| メソッド | 引数 | 戻り値 | 説明 |
| :--- | :--- | :--- | :--- |
| `start()` | なし | `int` (0: 成功, 負値: エラー) | カメラマネージャを起動し、システム内のパイプラインハンドラとカメラを走査する |
| `stop()` | なし | `void` | カメラマネージャを停止し、管理リソースを解放する |
| `cameras()` | なし | `std::vector<std::shared_ptr<Camera>>` | 検出されたすべてのカメラのリストを返す |
| `get(id)` | `const std::string& id` | `std::shared_ptr<Camera>` | カメラの一意な識別子文字列から該当カメラインスタンスを取得する |

### 4.2 libcamera::Camera

| メソッド / メンバ | 引数 | 戻り値 | 説明 |
| :--- | :--- | :--- | :--- |
| `id()` | なし | `const std::string&` | カメラの一意なデバイスパス/ID文字列を取得する |
| `acquire()` | なし | `int` (0: 成功, 負値: エラー) | カメラの排他制御権を獲得する |
| `release()` | なし | `int` (0: 成功, 負値: エラー) | カメラの排他制御権を解放する |
| `generateConfiguration()` | `const std::vector<StreamRole>&` | `std::unique_ptr<CameraConfiguration>` | 指定したストリーム役割に応じた既定設定を生成する |
| `configure()` | `CameraConfiguration*` | `int` (0: 成功, 負値: エラー) | 検証済み設定をカメラハードウェアに適用する |
| `createRequest()` | `uint64_t cookie = 0` | `std::unique_ptr<Request>` | キャプチャパイプラインへ送信用リクエストを生成する |
| `start()` | `const ControlList* controls = nullptr` | `int` (0: 成功, 負値: エラー) | カメラのビデオキャプチャを開始する |
| `stop()` | なし | `int` (0: 成功, 負値: エラー) | カメラのビデオキャプチャを停止する |
| `queueRequest()` | `Request*` | `int` (0: 成功, 負値: エラー) | リクエストをキューへ投入してフレーム取得を要求する |
| `requestCompleted` | (シグナル) | `Signal<Request*>` | フレーム処理完了時に発行されるシグナル |

### 4.3 libcamera::CameraConfiguration & StreamConfiguration

#### `CameraConfiguration`
- `validate()`:
  - 戻り値: `CameraConfiguration::Status` (`Valid`, `Adjusted`, `Invalid`)
  - 要求設定がハードウェアでそのまま実行可能か、調整されたか、非対応かを検証します。
- `at(index)` / `operator[](index)`:
  - ストリーム設定 (`StreamConfiguration&`) を取得します。

#### `StreamConfiguration` 主要フィールド
- `size`: 解像度 (`libcamera::Size { width, height }`)
- `pixelFormat`: ピクセルフォーマット (`libcamera::PixelFormat`)
- `stride`: 1 行あたりのバイトストライド幅
- `frameSize`: フレーム全体のバイトサイズ
- `stream()`: 割り当てられた `Stream*` ポインタ

### 4.4 libcamera::FrameBufferAllocator & FrameBuffer

#### `FrameBufferAllocator`
- `allocate(Stream*)`: 指定ストリームに対してカーネル側 DMA バッファを確保します。
- `buffers(Stream*)`: 確保された `const std::vector<std::unique_ptr<FrameBuffer>>&` を取得します。

#### `FrameBuffer`
- `planes()`: 各カラープレーンのディスクリプタ情報 (`const std::vector<Plane>&`) を取得します。
- `Plane::fd`: DMA バッファのファイルディスクリプタ (`SharedFD`)。
- `Plane::offset`: プレーンの先頭オフセット (バイト)。
- `Plane::length`: プレーンのサイズ (バイト)。

### 4.5 libcamera::Request

| メソッド | 引数 | 戻り値 | 説明 |
| :--- | :--- | :--- | :--- |
| `addBuffer()` | `const Stream*, FrameBuffer*` | `int` (0: 成功) | リクエストにストリームとバッファを関連付ける |
| `buffers()` | なし | `const BufferMap&` | 完了したリクエストに含まれるストリームとバッファのマップ |
| `status()` | なし | `Request::Status` | リクエストの状態 (`RequestPending`, `RequestComplete`, `RequestCancelled`) |
| `controls()` | なし | `ControlList&` | リクエスト個別に適用する制御パラメータのリスト |
| `reuse()` | `ReuseFlag flags` | `void` | 完了リクエストを再利用可能にする (`ReuseBuffers` でバッファ保持) |

### 4.6 libcamera コントロール (`libcamera::controls`)

| コントロール名 | 型 | 説明 |
| :--- | :--- | :--- |
| `controls::AeEnable` | `bool` | オート露出 (Auto Exposure) の有効/無効 |
| `controls::FrameDurationLimits` | `Span<const int64_t, 2>` | フレーム時間の下限・上限 (マイクロ秒単位)。FPSを一定に維持するために指定 |
| `controls::ExposureTime` | `int32_t` | マニュアル露出時間 (マイクロ秒) |
| `controls::AnalogueGain` | `float` | アナログゲイン倍率 |
| `controls::AwbEnable` | `bool` | オートホワイトバランスの有効/無効 |

### 4.7 Linux POSIX メモリマッピング API (`mmap`, `munmap`)

#### `mmap`
```c
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
```
- **引数**:
  - `addr`: 配置希望アドレス (`NULL` でカーネルに一任)。
  - `length`: マッピングするバイト数 (`plane.length`)。
  - `prot`: メモリ保護フラグ (`PROT_READ`: 読み込み専用)。
  - `flags`: マッピング属性 (`MAP_SHARED`: 他プロセス/ドライバと共有)。
  - `fd`: マッピング対象のファイルディスクリプタ (`plane.fd.get()`)。
  - `offset`: ファイル/バッファ先頭からのオフセット (`plane.offset`)。
- **戻り値**: 成功時はマッピングされた仮想メモリアドレス、失敗時は `MAP_FAILED` (`(void*)-1`)。

#### `munmap`
```c
int munmap(void* addr, size_t length);
```
- **引数**:
  - `addr`: `mmap` で得られた先頭アドレス。
  - `length`: 解除するバイト数。
- **戻り値**: 成功時 0、失敗時 -1。
