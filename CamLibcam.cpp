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
// Manager 実装
//
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

CamLibcam::Manager::~Manager()
{
  if (started)
  {
    cm->stop();
  }
}

CamLibcam::Manager& CamLibcam::Manager::getInstance()
{
  static Manager instance;
  return instance;
}

const std::vector<std::string>& CamLibcam::Manager::getDeviceList()
{
  deviceList.clear();
  if (started)
  {
    int index{ 0 };
    for (const auto& cam : cm->cameras())
    {
      std::string name{ "Camera " + std::to_string(index) + ": " + cam->id() };
      deviceList.emplace_back(name);
      ++index;
    }
  }
  return deviceList;
}

//
// CamLibcam 実装
//
CamLibcam::CamLibcam(int deviceNumber, int initial_width, int initial_height, double initial_fps)
{
  open(deviceNumber, initial_width, initial_height, initial_fps);
}

CamLibcam::~CamLibcam()
{
  close();
}

bool CamLibcam::open(int deviceNumber, int initial_width, int initial_height, double initial_fps)
{
  close();

  auto* cm{ Manager::getInstance().get() };
  if (!cm) return false;

  const auto cameras{ cm->cameras() };
  if (deviceNumber < 0 || deviceNumber >= static_cast<int>(cameras.size()))
  {
    std::cerr << "libcamera: Invalid camera index " << deviceNumber << std::endl;
    return false;
  }

  camera = cameras[deviceNumber];
  if (!camera) return false;

  if (camera->acquire() != 0)
  {
    std::cerr << "libcamera: Failed to acquire camera " << deviceNumber << " (" << camera->id() << ")" << std::endl;
    camera.reset();
    return false;
  }

  std::cout << "libcamera: Opened camera " << deviceNumber << ": " << camera->id() << std::endl;

  // ストリームロールを順次試行（Viewfinder -> VideoRecording -> Raw）
  const std::vector<libcamera::StreamRole> roles = {
    libcamera::StreamRole::Viewfinder,
    libcamera::StreamRole::VideoRecording,
    libcamera::StreamRole::Raw
  };

  bool configured{ false };

  for (const auto role : roles)
  {
    config = camera->generateConfiguration({ role });
    if (!config || config->empty()) continue;

    auto& streamConfig{ config->at(0) };

    // 希望解像度
    if (initial_width > 0 && initial_height > 0)
    {
      streamConfig.size.width = initial_width;
      streamConfig.size.height = initial_height;
    }

    // サポートされているフォーマット一覧を取得
    const auto supportedFormats{ streamConfig.formats().pixelformats() };
    std::cout << "libcamera: Supported pixel formats: ";
    for (const auto& fmt : supportedFormats)
    {
      std::cout << fmt.toString() << " ";
    }
    std::cout << std::endl;

    // 優先順位リストに基づいてフォーマットを選択
    const std::vector<libcamera::PixelFormat> preferenceList = {
      libcamera::formats::BGR888,
      libcamera::formats::RGB888,
      libcamera::formats::XBGR8888,
      libcamera::formats::BGRX8888,
      libcamera::formats::XRGB8888,
      libcamera::formats::RGBX8888,
      libcamera::formats::R8,       // モノクロ 8-bit (OV9281 等)
      libcamera::formats::YUYV,
      libcamera::formats::NV12,
      libcamera::formats::YUV420,
    };

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

    if (!formatSelected && !supportedFormats.empty())
    {
      streamConfig.pixelFormat = supportedFormats.front();
    }

    // 設定の検証
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

  if (!configured)
  {
    std::cerr << "libcamera: Failed to configure camera with any stream role" << std::endl;
    camera->release();
    camera.reset();
    return false;
  }

  auto& streamConfig{ config->at(0) };
  stream = streamConfig.stream();
  pixelFormat = streamConfig.pixelFormat;
  stride = streamConfig.stride;
  width = streamConfig.size.width;
  height = streamConfig.size.height;
  channels = 4; // calib 内部バッファは常に BGRA 4チャンネル

  std::cout << "libcamera: Final configuration - format: " << pixelFormat.toString()
            << ", resolution: " << width << "x" << height
            << ", stride: " << stride << std::endl;

  if (initial_fps > 0.0)
  {
    interval = 1000.0 / initial_fps;
  }
  else
  {
    interval = 33.3; // 既定 30fps
  }

  // バッファメモリ確保
  const size_t bufferSize{ static_cast<size_t>(width * height * channels) };
  frame.resize(bufferSize, 255);
  image.resize(bufferSize, 255);

  // フレームバッファアロケータの作成
  allocator = std::make_unique<libcamera::FrameBufferAllocator>(camera);
  if (allocator->allocate(stream) < 0)
  {
    std::cerr << "libcamera: Failed to allocate buffers" << std::endl;
    close();
    return false;
  }

  // バッファのメモリマッピング
  for (const auto& buffer : allocator->buffers(stream))
  {
    std::vector<MappedPlane> planes;
    for (const auto& plane : buffer->planes())
    {
      void* memory{ ::mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset) };
      if (memory == MAP_FAILED)
      {
        std::cerr << "libcamera: Failed to mmap plane" << std::endl;
        memory = nullptr;
      }
      planes.push_back({ memory, plane.length });
    }
    mappedBuffers[buffer.get()] = std::move(planes);

    // リクエストの作成
    auto request{ camera->createRequest() };
    if (!request)
    {
      std::cerr << "libcamera: Failed to create request" << std::endl;
      close();
      return false;
    }
    if (request->addBuffer(stream, buffer.get()) < 0)
    {
      std::cerr << "libcamera: Failed to add buffer to request" << std::endl;
      close();
      return false;
    }

    // 自動露出 (AE) を有効化
    request->controls().set(libcamera::controls::AeEnable, true);

    requests.push_back(std::move(request));
  }

  // コールバック接続
  camera->requestCompleted.connect(this, &CamLibcam::requestComplete);

  captured = false;
  return true;
}

void CamLibcam::unmapBuffers()
{
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
  mappedBuffers.clear();
}

void CamLibcam::close()
{
  stop();

  if (camera)
  {
    camera->requestCompleted.disconnect(this, &CamLibcam::requestComplete);
  }

  requests.clear();
  unmapBuffers();
  allocator.reset();
  config.reset();

  if (camera)
  {
    camera->release();
    camera.reset();
  }

  width = 0;
  height = 0;
  stream = nullptr;
}

void CamLibcam::start()
{
  if (!camera || running) return;

  if (camera->start() < 0)
  {
    std::cerr << "libcamera: Failed to start camera" << std::endl;
    return;
  }

  running = true;

  // すべてのリクエストをキューに入れる
  for (auto& request : requests)
  {
    camera->queueRequest(request.get());
  }
}

void CamLibcam::stop()
{
  if (!camera || !running) return;

  running = false;
  camera->stop();
}

void CamLibcam::requestComplete(libcamera::Request* request)
{
  if (request->status() == libcamera::Request::RequestCancelled)
  {
    return;
  }

  auto bufferMap{ request->buffers() };
  auto it{ bufferMap.find(stream) };
  if (it != bufferMap.end())
  {
    libcamera::FrameBuffer* buffer{ it->second };
    const auto& planes{ mappedBuffers[buffer] };

    if (!planes.empty() && planes[0].address)
    {
      // 全フレーム処理モードなら消費されるまで待機
      if (!prioritizeLatency)
      {
        while (running && captured)
        {
          std::this_thread::yield();
        }
      }

      const uint8_t* src{ static_cast<const uint8_t*>(planes[0].address) };
      std::lock_guard<std::mutex> lock(mtx);

      uint8_t* dst{ image.data() };

      if (pixelFormat == libcamera::formats::BGR888)
      {
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
            rowSrc += 4;
            rowDst += 4;
          }
        }
      }
      else if (pixelFormat == libcamera::formats::XRGB8888 || pixelFormat == libcamera::formats::RGBX8888)
      {
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
        // 8-bit モノクロ (OV9281 等)
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
        // YUYV 4:2:2
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

            rowDst[0] = static_cast<uint8_t>(std::clamp((298 * y0 + 516 * u + 128) >> 8, 0, 255));
            rowDst[1] = static_cast<uint8_t>(std::clamp((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255));
            rowDst[2] = static_cast<uint8_t>(std::clamp((298 * y0 + 409 * v + 128) >> 8, 0, 255));
            rowDst[3] = 255;
            rowDst += 4;

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
        // 輝度プレーン (Y) を展開
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
        // 未知のフォーマットに対するフォールバック: 輝度として展開
        for (size_t y = 0; y < static_cast<size_t>(height); ++y)
        {
          const uint8_t* rowSrc{ src + y * stride };
          uint8_t* rowDst{ dst + y * width * 4 };
          for (size_t x = 0; x < static_cast<size_t>(width); ++x)
          {
            const uint8_t val = *rowSrc++;
            rowDst[0] = val;
            rowDst[1] = val;
            rowDst[2] = val;
            rowDst[3] = 255;
            rowDst += 4;
          }
        }
      }

      captured = true;
    }
  }

  // 実行中であればリクエストを再キューイング
  if (running)
  {
    request->reuse(libcamera::Request::ReuseBuffers);
    camera->queueRequest(request);
  }
}

std::string CamLibcam::getPixelFormatName() const
{
  return pixelFormat.toString();
}

const std::vector<std::string>& CamLibcam::getDeviceList()
{
  return Manager::getInstance().getDeviceList();
}

#endif // USE_LIBCAMERA
