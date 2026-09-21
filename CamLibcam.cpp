///
/// libcamera を使ったビデオキャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date March 2026
///

#include "CamLibcam.h"

#if defined(USE_LIBCAMERA)

// libcamera formats
#include <libcamera/formats.h>
#include <libcamera/control_ids.h>

// POSIX
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <algorithm>

//
// libcamera の CameraManager を管理するクラスのコンストラクタ
//
CamLibcam::Manager::Manager()
  : cm{ std::make_unique<libcamera::CameraManager>() }
{
  // カメラマネージャを起動する
  const int ret{ cm->start() };

  // 起動に成功したかどうかを記録する
  started = (ret == 0);

  // 起動に失敗した場合はエラーメッセージを出力する
  if (!started)
  {
    std::cerr << "Failed to start libcamera CameraManager: " << ret << std::endl;
  }
}

//
// libcamera の CameraManager を管理するクラスのデストラクタ
//
CamLibcam::Manager::~Manager()
{
  // カメラマネージャが起動していれば停止する
  if (started)
  {
    cm->stop();
  }
}

//
// CameraManager シングルトンインスタンスへの参照を返す
//
CamLibcam::Manager& CamLibcam::Manager::getInstance()
{
  // シングルトンインスタンス
  static Manager instance;

  // インスタンスへの参照を返す
  return instance;
}

//
// 検出されたカメラデバイスの表示名リストを返す
//
const std::vector<std::string>& CamLibcam::Manager::getDeviceList()
{
  // デバイスリストを初期化する
  deviceList.clear();

  // カメラマネージャが正常に起動していれば
  if (started)
  {
    // カメラデバイスのインデックス
    int index{ 0 };

    // システムに存在するカメラデバイスを順次走査する
    for (const auto& cam : cm->cameras())
    {
      // デバイスインデックスとカメラ ID を組み合わせた表示名を作成する
      std::string name{ "Camera " + std::to_string(index) + ": " + cam->id() };

      // 表示名リストに追加する
      deviceList.emplace_back(name);
      ++index;
    }
  }

  // デバイスリストを返す
  return deviceList;
}

//
// コンストラクタ
//
CamLibcam::CamLibcam(int deviceNumber, int initial_width, int initial_height, double initial_fps)
{
  // 指定されたパラメータでカメラを開く
  open(deviceNumber, initial_width, initial_height, initial_fps);
}

//
// デストラクタ
//
CamLibcam::~CamLibcam()
{
  // カメラを閉じてリソースを解放する
  close();
}

//
// カメラを開く
//
bool CamLibcam::open(int deviceNumber, int initial_width, int initial_height, double initial_fps)
{
  // 既に開いているカメラがあれば閉じる
  close();

  // カメラマネージャを取得する
  auto* cm{ Manager::getInstance().get() };
  if (!cm) return false;

  // 利用可能なカメラ一覧を取得する
  const auto cameras{ cm->cameras() };

  // デバイス番号が有効な範囲内にあるか検証する
  if (deviceNumber < 0 || deviceNumber >= static_cast<int>(cameras.size()))
  {
    std::cerr << "libcamera: Invalid camera index " << deviceNumber << std::endl;
    return false;
  }

  // 指定されたインデックスのカメラを取得する
  camera = cameras[deviceNumber];
  if (!camera) return false;

  // カメラデバイスの排他制御権を獲得する
  if (camera->acquire() != 0)
  {
    std::cerr << "libcamera: Failed to acquire camera " << deviceNumber << " (" << camera->id() << ")" << std::endl;
    camera.reset();
    return false;
  }

  std::cout << "libcamera: Opened camera " << deviceNumber << ": " << camera->id() << std::endl;

  // ストリームロールを順次試行する（Viewfinder -> VideoRecording -> Raw）
  const std::vector<libcamera::StreamRole> roles = {
    libcamera::StreamRole::Viewfinder,
    libcamera::StreamRole::VideoRecording,
    libcamera::StreamRole::Raw
  };

  // 設定成功フラグ
  bool configured{ false };

  // 各ロールについて設定生成を試行する
  for (const auto role : roles)
  {
    // ロールに応じた既定設定を生成する
    config = camera->generateConfiguration({ role });
    if (!config || config->empty()) continue;

    // ストリーム設定の参照を取得する
    auto& streamConfig{ config->at(0) };

    // 希望解像度が指定されている場合は設定に反映する
    if (initial_width > 0 && initial_height > 0)
    {
      streamConfig.size.width = initial_width;
      streamConfig.size.height = initial_height;
    }

    // サポートされているピクセルフォーマット一覧を取得する
    const auto supportedFormats{ streamConfig.formats().pixelformats() };
    std::cout << "libcamera: Supported pixel formats: ";
    for (const auto& fmt : supportedFormats)
    {
      std::cout << fmt.toString() << " ";
    }
    std::cout << std::endl;

    // 優先順位リストに基づいてピクセルフォーマットを選択する (XBGR8888 は memcpy 可能で最速)
    const std::vector<libcamera::PixelFormat> preferenceList = {
      libcamera::formats::R8,       // モノクロ 8-bit (OV9281 等)
      libcamera::formats::XBGR8888, // 4-byte BGRA 互換 (memcpy可能・超高速)
      libcamera::formats::BGRX8888,
      libcamera::formats::XRGB8888,
      libcamera::formats::RGBX8888,
      libcamera::formats::BGR888,
      libcamera::formats::RGB888,
      libcamera::formats::YUYV,
      libcamera::formats::NV12,
      libcamera::formats::YUV420,
    };

    // 優先順位順にサポートフォーマットを探索する
    bool formatSelected{ false };
    for (const auto& pref : preferenceList)
    {
      if (std::find(supportedFormats.begin(), supportedFormats.end(), pref) != supportedFormats.end())
      {
        streamConfig.pixelFormat = pref;
        formatSelected = true;
        break;
      }
    }

    // 優先フォーマットが見つからなければサポート一覧の先頭を採用する
    if (!formatSelected && !supportedFormats.empty())
    {
      streamConfig.pixelFormat = supportedFormats.front();
    }

    // パイプラインによる設定の検証と調整を行う
    const auto status = config->validate();
    if (status == libcamera::CameraConfiguration::Invalid)
    {
      std::cerr << "libcamera: Configuration invalid for role, trying next..." << std::endl;
      continue;
    }
    else if (status == libcamera::CameraConfiguration::Adjusted)
    {
      std::cout << "libcamera: Configuration was adjusted by pipeline" << std::endl;
    }

    // カメラに設定を適用する
    if (camera->configure(config.get()) == 0)
    {
      configured = true;
      break;
    }
    else
    {
      std::cerr << "libcamera: configure() failed for role, trying next..." << std::endl;
    }
  }

  // いずれのロールでも設定に失敗した場合はカメラを解放して戻る
  if (!configured)
  {
    std::cerr << "libcamera: Failed to configure camera with any stream role" << std::endl;
    camera->release();
    camera.reset();
    return false;
  }

  // 確定したストリーム設定からパラメータを取得する
  auto& streamConfig{ config->at(0) };
  stream = streamConfig.stream();
  pixelFormat = streamConfig.pixelFormat;
  stride = streamConfig.stride;
  width = streamConfig.size.width;
  height = streamConfig.size.height;
  channels = 4; // 内部バッファは常に BGRA 4チャンネル

  std::cout << "libcamera: Final configuration - format: " << pixelFormat.toString()
            << ", resolution: " << width << "x" << height
            << ", stride: " << stride << std::endl;

  // 要求フレームレートからフレーム時間（マイクロ秒）とインターバルを計算する
  if (initial_fps > 0.0)
  {
    frameDurationUs = static_cast<int64_t>(1000000.0 / initial_fps);
    interval = 1000.0 / initial_fps;
  }
  else
  {
    frameDurationUs = 33333; // 既定 30fps
    interval = 33.3;
  }

  // フレームデータ格納用のバッファメモリを確保する
  const size_t bufferSize{ static_cast<size_t>(width * height * channels) };
  frame.resize(bufferSize, 255);
  image.resize(bufferSize, 255);

  // フレームバッファアロケータを作成しストリームにバッファを割り当てる
  allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
  if (allocator->allocate(stream) < 0)
  {
    std::cerr << "libcamera: Failed to allocate buffers" << std::endl;
    close();
    return false;
  }

  // 割り当てられたバッファの各プレーンをプロセスのアドレス空間にマッピングする
  for (const auto& buffer : allocator->buffers(stream))
  {
    std::vector<MappedPlane> planes;
    for (const auto& plane : buffer->planes())
    {
      // ファイルディスクリプタからメモリマップを作成する
      void* memory{ ::mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset) };
      if (memory == MAP_FAILED)
      {
        std::cerr << "libcamera: Failed to mmap plane" << std::endl;
        memory = nullptr;
      }
      planes.push_back({ memory, plane.length });
    }
    mappedBuffers[buffer.get()] = std::move(planes);

    // キャプチャ用リクエストを作成する
    auto request{ camera->createRequest() };
    if (!request)
    {
      std::cerr << "libcamera: Failed to create request" << std::endl;
      close();
      return false;
    }

    // リクエストにバッファを割り当てる
    if (request->addBuffer(stream, buffer.get()) < 0)
    {
      std::cerr << "libcamera: Failed to add buffer to request" << std::endl;
      close();
      return false;
    }

    // リクエストリストに追加する
    requests.push_back(std::move(request));
  }

  // フレーム完了通知コールバックを接続する
  camera->requestCompleted.connect(this, &CamLibcam::requestComplete);

  // キャプチャフラグを初期化する
  captured = false;
  return true;
}

//
// メモリマッピングされたバッファを解放する
//
void CamLibcam::unmapBuffers()
{
  // すべてのマッピング済みプレーンについて munmap を実行する
  for (auto& [buf, planes] : mappedBuffers)
  {
    for (auto& plane : planes)
    {
      if (plane.address && plane.address != MAP_FAILED)
      {
        ::munmap(plane.address, plane.length);
      }
    }
  }

  // マッピングテーブルをクリアする
  mappedBuffers.clear();
}

//
// カメラを閉じる
//
void CamLibcam::close()
{
  // キャプチャを停止する
  stop();

  // コールバック接続を解除する
  if (camera)
  {
    camera->requestCompleted.disconnect(this, &CamLibcam::requestComplete);
  }

  // リクエスト一覧をクリアする
  requests.clear();

  // メモリマッピングを解除する
  unmapBuffers();

  // アロケータと設定オブジェクトを解放する
  allocator.reset();
  config.reset();

  // カメラデバイスの排他制御権を解放する
  if (camera)
  {
    camera->release();
    camera.reset();
  }

  // 内部パラメータをリセットする
  width = 0;
  height = 0;
  stream = nullptr;
}

//
// キャプチャを開始する
//
void CamLibcam::start()
{
  // カメラが存在しないか既に実行中であれば何もしない
  if (!camera || running) return;

  // キャプチャ開始時のコントロールパラメータを設定する
  libcamera::ControlList startControls;

  // 自動露出 (AE) を有効化し、フレームレート（フレーム時間）を設定する
  startControls.set(libcamera::controls::AeEnable, true);
  startControls.set(libcamera::controls::FrameDurationLimits,
    libcamera::Span<const int64_t, 2>({ frameDurationUs, frameDurationUs }));

  // カメラのストリーミングを開始する
  if (camera->start(&startControls) < 0)
  {
    std::cerr << "libcamera: Failed to start camera" << std::endl;
    return;
  }

  // 実行中フラグを立てる
  running = true;

  // 準備したすべてのリクエストをカメラのキューに投入する
  for (auto& request : requests)
  {
    camera->queueRequest(request.get());
  }
}

//
// キャプチャを停止する
//
void CamLibcam::stop()
{
  // カメラが存在しないか既に停止中であれば何もしない
  if (!camera || !running) return;

  // 実行中フラグを解除する
  running = false;

  // カメラのストリーミングを停止する
  camera->stop();
}

//
// フレーム完了コールバック
//
void CamLibcam::requestComplete(libcamera::Request* request)
{
  // リクエストがキャンセルされた場合は何もしない
  if (request->status() == libcamera::Request::RequestCancelled)
  {
    return;
  }

  // 完了したリクエストのバッファマップを取得する
  auto bufferMap{ request->buffers() };
  auto it{ bufferMap.find(stream) };
  if (it != bufferMap.end())
  {
    // ストリームのフレームバッファとマッピングプレーンを取得する
    libcamera::FrameBuffer* buffer{ it->second };
    const auto& planes{ mappedBuffers[buffer] };

    // バッファアドレスが有効であれば画像データを変換・格納する
    if (!planes.empty() && planes[0].address)
    {
      // ソースメモリの先頭アドレス
      const uint8_t* src{ static_cast<const uint8_t*>(planes[0].address) };

      // バッファアクセスをスレッドセーフに行うための排他ロック
      std::lock_guard<std::mutex> lock(mtx);

      // 格納先バッファの先頭アドレス
      uint8_t* dst{ image.data() };

      // ピクセルフォーマットに応じて BGRA 4チャンネルへ変換する
      if (pixelFormat == libcamera::formats::BGR888)
      {
        // 3バイト BGR から 4バイト BGRA へ展開する
        for (size_t y = 0; y < static_cast<size_t>(height); ++y)
        {
          const uint8_t* rowSrc{ src + y * stride };
          uint8_t* rowDst{ dst + y * width * 4 };
          for (size_t x = 0; x < static_cast<size_t>(width); ++x)
          {
            rowDst[0] = rowSrc[0]; // B
            rowDst[1] = rowSrc[1]; // G
            rowDst[2] = rowSrc[2]; // R
            rowDst[3] = 255;       // A
            rowSrc += 3;
            rowDst += 4;
          }
        }
      }
      else if (pixelFormat == libcamera::formats::RGB888)
      {
        // 3バイト RGB から 4バイト BGRA へ色順序を入れ替えて展開する
        for (size_t y = 0; y < static_cast<size_t>(height); ++y)
        {
          const uint8_t* rowSrc{ src + y * stride };
          uint8_t* rowDst{ dst + y * width * 4 };
          for (size_t x = 0; x < static_cast<size_t>(width); ++x)
          {
            rowDst[0] = rowSrc[2]; // B
            rowDst[1] = rowSrc[1]; // G
            rowDst[2] = rowSrc[0]; // R
            rowDst[3] = 255;       // A
            rowSrc += 3;
            rowDst += 4;
          }
        }
      }
      else if (pixelFormat == libcamera::formats::XBGR8888 || pixelFormat == libcamera::formats::BGRX8888)
      {
        // BGRA 互換フォーマットの高速コピー
        if (stride == static_cast<unsigned int>(width * 4))
        {
          // ストライドが幅×4と一致する場合は全体を一括 memcpy する
          std::memcpy(dst, src, static_cast<size_t>(width * height * 4));
        }
        else
        {
          // ストライドパディングが存在する場合は行単位で memcpy する
          for (size_t y = 0; y < static_cast<size_t>(height); ++y)
          {
            std::memcpy(dst + y * width * 4, src + y * stride, width * 4);
          }
        }
      }
      else if (pixelFormat == libcamera::formats::XRGB8888 || pixelFormat == libcamera::formats::RGBX8888)
      {
        // 4バイト RGBX から 4バイト BGRA へ色順序を入れ替えて展開する
        for (size_t y = 0; y < static_cast<size_t>(height); ++y)
        {
          const uint8_t* rowSrc{ src + y * stride };
          uint8_t* rowDst{ dst + y * width * 4 };
          for (size_t x = 0; x < static_cast<size_t>(width); ++x)
          {
            rowDst[0] = rowSrc[2]; // B
            rowDst[1] = rowSrc[1]; // G
            rowDst[2] = rowSrc[0]; // R
            rowDst[3] = 255;       // A
            rowSrc += 4;
            rowDst += 4;
          }
        }
      }
      else if (pixelFormat == libcamera::formats::R8)
      {
        // 8-bit モノクロ（OV9281 等）の輝度値を BGR 各チャンネルに複製して展開する
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
      else if (pixelFormat == libcamera::formats::YUYV)
      {
        // YUYV 4:2:2 パッカブルフォーマットから BGRA へ色空間変換する
        for (size_t y = 0; y < static_cast<size_t>(height); ++y)
        {
          const uint8_t* rowSrc{ src + y * stride };
          uint8_t* rowDst{ dst + y * width * 4 };
          for (size_t x = 0; x < static_cast<size_t>(width); x += 2)
          {
            const int y0 = rowSrc[0] - 16;
            const int u  = rowSrc[1] - 128;
            const int y1 = rowSrc[2] - 16;
            const int v  = rowSrc[3] - 128;
            rowSrc += 4;

            // 1 画素目の RGB 変換
            rowDst[0] = static_cast<uint8_t>(std::clamp((298 * y0 + 516 * u + 128) >> 8, 0, 255));
            rowDst[1] = static_cast<uint8_t>(std::clamp((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255));
            rowDst[2] = static_cast<uint8_t>(std::clamp((298 * y0 + 409 * v + 128) >> 8, 0, 255));
            rowDst[3] = 255;
            rowDst += 4;

            // 2 画素目の RGB 変換
            rowDst[0] = static_cast<uint8_t>(std::clamp((298 * y1 + 516 * u + 128) >> 8, 0, 255));
            rowDst[1] = static_cast<uint8_t>(std::clamp((298 * y1 - 100 * u - 208 * v + 128) >> 8, 0, 255));
            rowDst[2] = static_cast<uint8_t>(std::clamp((298 * y1 + 409 * v + 128) >> 8, 0, 255));
            rowDst[3] = 255;
            rowDst += 4;
          }
        }
      }
      else if (pixelFormat == libcamera::formats::NV12 || pixelFormat == libcamera::formats::YUV420)
      {
        // 輝度プレーン (Y) を BGRA の各チャンネルへ複製して展開する
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
      else
      {
        // 未知のフォーマットに対するフォールバック（輝度として展開）
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

      // フレーム取得完了フラグを立てる
      captured = true;

      // キャプチャフレームレートの実測と診断出力 (1秒ごと)
      static auto lastFpsReport{ std::chrono::steady_clock::now() };
      static int capturedFrameCount{ 0 };

      ++capturedFrameCount;
      const auto now{ std::chrono::steady_clock::now() };
      const auto elapsed{ std::chrono::duration<double>(now - lastFpsReport).count() };
      if (elapsed >= 1.0)
      {
        const double currentFps{ capturedFrameCount / elapsed };
        std::cout << "libcamera: Capture FPS = " << currentFps << std::endl;
        capturedFrameCount = 0;
        lastFpsReport = now;
      }
    }
  }

  // キャプチャ実行中であればリクエストを再利用してカメラキューに再投入する
  if (running)
  {
    request->reuse(libcamera::Request::ReuseBuffers);
    camera->queueRequest(request);
  }
}

//
// 現在のピクセルフォーマット名を返す
//
std::string CamLibcam::getPixelFormatName() const
{
  // ピクセルフォーマットの文字列表現を返す
  return pixelFormat.toString();
}

//
// ビデオキャプチャデバイスの表示名リストを返す
//
const std::vector<std::string>& CamLibcam::getDeviceList()
{
  // カメラマネージャからデバイスリストを取得して返す
  return Manager::getInstance().getDeviceList();
}

#endif // USE_LIBCAMERA
