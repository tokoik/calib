///
/// AV Foundation を使ったビデオキャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 2026
///

#if defined(__APPLE__)

#include "CamAvf.h"

// AV Foundation
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>

// 標準ライブラリ
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <thread>

// キャプチャデバイス名のリスト
std::vector<std::string> CamAvf::deviceList;

///
/// AV Foundation のサンプルバッファデリゲート
///
@interface CamAvfDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
  CamAvf* parent;
}
- (id)initWithParent:(CamAvf*)p;
@end

@implementation CamAvfDelegate

- (id)initWithParent:(CamAvf*)p
{
  self = [super init];
  if (self)
  {
    parent = p;
  }
  return self;
}

- (void)captureOutput:(AVCaptureOutput*)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection*)connection
{
  if (parent)
  {
    parent->handleSampleBuffer(sampleBuffer);
  }
}

@end

///
/// CamAvf の内部実装構造体
///
struct CamAvf::Impl
{
  AVCaptureSession* session{ nil };
  AVCaptureDevice* device{ nil };
  AVCaptureDeviceInput* input{ nil };
  AVCaptureVideoDataOutput* output{ nil };
  CamAvfDelegate* delegate{ nil };
  dispatch_queue_t queue{ nil };
  int deviceIndex{ -1 };

  ~Impl()
  {
    if (session)
    {
      if (session.isRunning)
      {
        [session stopRunning];
      }
      [session beginConfiguration];
      if (input)
      {
        [session removeInput:input];
        input = nil;
      }
      if (output)
      {
        [output setSampleBufferDelegate:nil queue:NULL];
        [session removeOutput:output];
        output = nil;
      }
      [session commitConfiguration];
      session = nil;
    }
    delegate = nil;
    queue = nil;
    device = nil;
  }
};

namespace
{
  ///
  /// 利用可能なビデオキャプチャデバイスをすべて取得する
  ///
  /// @return デバイスの配列
  ///
  NSArray<AVCaptureDevice*>* getVideoDevices()
  {
    NSMutableArray<AVCaptureDeviceType>* types = [NSMutableArray array];
    [types addObject:AVCaptureDeviceTypeBuiltInWideAngleCamera];
#if defined(__MAC_14_0)
    if (@available(macOS 14.0, *))
    {
      [types addObject:AVCaptureDeviceTypeExternal];
      [types addObject:AVCaptureDeviceTypeContinuityCamera];
    }
    else
#endif
    {
      [types addObject:AVCaptureDeviceTypeExternalUnknown];
    }

    AVCaptureDeviceDiscoverySession* discoverySession = [AVCaptureDeviceDiscoverySession
      discoverySessionWithDeviceTypes:types
      mediaType:AVMediaTypeVideo
      position:AVCaptureDevicePositionUnspecified];

    return discoverySession.devices;
  }

  ///
  /// インデックスからデバイスを取得する
  ///
  /// @param index デバイス番号
  /// @return デバイスオブジェクト（存在しない場合は nil）
  ///
  AVCaptureDevice* getDeviceByIndex(int index)
  {
    if (index < 0) return nil;
    NSArray<AVCaptureDevice*>* devices = getVideoDevices();
    if (static_cast<NSUInteger>(index) < devices.count)
    {
      return devices[index];
    }
    return nil;
  }
}

//
// コンストラクタ
//
CamAvf::CamAvf()
{
}

//
// デストラクタ
//
CamAvf::~CamAvf()
{
  close();
}

//
// ビデオデバイスの一覧を取得する
//
const std::vector<std::string>& CamAvf::getDeviceList()
{
  deviceList.clear();

  @autoreleasepool {
    // カメラの利用権限を確認し、未決定の場合はリクエストする
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (status == AVAuthorizationStatusNotDetermined)
    {
      dispatch_semaphore_t sem = dispatch_semaphore_create(0);
      [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
        dispatch_semaphore_signal(sem);
      }];
      dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    }

    NSArray<AVCaptureDevice*>* devices = getVideoDevices();
    for (NSUInteger i = 0; i < devices.count; ++i)
    {
      AVCaptureDevice* dev = devices[i];
      std::string name = [dev.localizedName UTF8String];
      // 同一名称のカメラが存在する場合に識別できるよう "名前##番号" とする
      name += "##" + std::to_string(i);
      deviceList.push_back(name);
    }
  }

  return deviceList;
}

//
// カメラを開く
//
bool CamAvf::open(int deviceNumber, bool setupFormat)
{
  close();

  @autoreleasepool {
    AVCaptureDevice* dev = getDeviceByIndex(deviceNumber);
    if (!dev) return false;

    impl = std::make_unique<Impl>();
    impl->device = dev;
    impl->deviceIndex = deviceNumber;

    if (!enumerateFormats())
    {
      close();
      return false;
    }

    if (setupFormat)
    {
      if (!select(0))
      {
        close();
        return false;
      }
    }
    else
    {
      // 遅延初期化の場合でも、初期パラメータを保持しておく
      if (!availableFormats.empty())
      {
        selectedFormatIndex = 0;
        const auto& vFmt = availableFormats[0];
        width = vFmt.width;
        height = vFmt.height;
        channels = 4;
        interval = vFmt.fps > 0.0 ? 1000.0 / vFmt.fps : 10.0;
        image.resize(static_cast<size_t>(width) * height * channels);
      }
    }
  }

  return true;
}

//
// 使用可能な解像度、フレームレート、コーデックのリストを作成する
//
bool CamAvf::enumerateFormats()
{
  formatList.clear();
  availableFormats.clear();
  if (!impl || !impl->device) return false;

  @autoreleasepool {
    NSArray<AVCaptureDeviceFormat*>* formats = impl->device.formats;
    for (NSUInteger fIdx = 0; fIdx < formats.count; ++fIdx)
    {
      AVCaptureDeviceFormat* format = formats[fIdx];
      CMVideoFormatDescriptionRef formatDesc = format.formatDescription;
      CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(formatDesc);
      int w = dims.width;
      int h = dims.height;
      FourCharCode subType = CMFormatDescriptionGetMediaSubType(formatDesc);

      std::string codecName;
      switch (subType)
      {
      case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
      case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        codecName = "NV12";
        break;
      case kCVPixelFormatType_422YpCbCr8:
        codecName = "2vuy";
        break;
      case kCVPixelFormatType_422YpCbCr8_yuvs:
        codecName = "YUY2";
        break;
      case kCVPixelFormatType_32BGRA:
        codecName = "BGRA";
        break;
      case kCVPixelFormatType_32ARGB:
        codecName = "ARGB";
        break;
      case kCMVideoCodecType_JPEG:
        codecName = "MJPG";
        break;
      case kCMVideoCodecType_H264:
        codecName = "H264";
        break;
      case kCMVideoCodecType_HEVC:
        codecName = "HEVC";
        break;
      default:
      {
        char fcc[5] = {
          static_cast<char>((subType >> 24) & 0xFF),
          static_cast<char>((subType >> 16) & 0xFF),
          static_cast<char>((subType >> 8) & 0xFF),
          static_cast<char>(subType & 0xFF),
          '\0'
        };
        if (std::isprint(static_cast<unsigned char>(fcc[0])) &&
            std::isprint(static_cast<unsigned char>(fcc[1])) &&
            std::isprint(static_cast<unsigned char>(fcc[2])) &&
            std::isprint(static_cast<unsigned char>(fcc[3])))
        {
          codecName = fcc;
        }
        else
        {
          std::ostringstream oss;
          oss << "0x" << std::hex << subType;
          codecName = oss.str();
        }
        break;
      }
      }

      for (NSUInteger rIdx = 0; rIdx < format.videoSupportedFrameRateRanges.count; ++rIdx)
      {
        AVFrameRateRange* range = format.videoSupportedFrameRateRanges[rIdx];
        double maxFps = range.maxFrameRate;
        if (maxFps < 5.0) continue;

        std::ostringstream resOss;
        resOss << w << " x " << h;
        std::ostringstream fpsOss;
        fpsOss << std::fixed << std::setprecision(2) << maxFps;

        // 重複チェック (同一解像度、同一fps、同一codec のエントリはスキップ)
        bool duplicate = false;
        for (const auto& fmt : formatList)
        {
          if (fmt.resolution == resOss.str() && fmt.fps == fpsOss.str() && fmt.codec == codecName)
          {
            duplicate = true;
            break;
          }
        }
        if (!duplicate)
        {
          formatList.push_back({ resOss.str(), fpsOss.str(), codecName, static_cast<int>(availableFormats.size()) });
          availableFormats.emplace_back(w, h, maxFps, codecName, static_cast<int>(fIdx), static_cast<int>(rIdx));
        }
      }
    }
  }

  return !formatList.empty();
}

//
// フォーマットを選択して設定する
//
bool CamAvf::select(int index)
{
  if (!impl || !impl->device || index < 0 || index >= static_cast<int>(availableFormats.size()))
  {
    return false;
  }

  const bool restart{ running };
  if (restart) stop();

  selectedFormatIndex = index;
  const auto& vFmt = availableFormats[index];

  @autoreleasepool {
    NSError* error = nil;
    if ([impl->device lockForConfiguration:&error])
    {
      if (static_cast<NSUInteger>(vFmt.formatIndex) < impl->device.formats.count)
      {
        AVCaptureDeviceFormat* fmt = impl->device.formats[vFmt.formatIndex];
        impl->device.activeFormat = fmt;

        int32_t fpsInt = static_cast<int32_t>(std::round(vFmt.fps));
        if (fpsInt > 0)
        {
          impl->device.activeVideoMinFrameDuration = CMTimeMake(1, fpsInt);
          impl->device.activeVideoMaxFrameDuration = CMTimeMake(1, fpsInt);
        }
      }
      [impl->device unlockForConfiguration];
    }
  }

  width = vFmt.width;
  height = vFmt.height;
  channels = 4;
  interval = vFmt.fps > 0.0 ? 1000.0 / vFmt.fps : 10.0;
  image.resize(static_cast<size_t>(width) * height * channels);

  if (restart) start();

  return true;
}

//
// レイテンシ優先モードを設定する
//
void CamAvf::setPrioritizeLatency(bool mode)
{
  Camera::setPrioritizeLatency(mode);
  if (impl && impl->output)
  {
    impl->output.alwaysDiscardsLateVideoFrames = mode ? YES : NO;
  }
}

//
// キャプチャ開始処理を行う
//
bool CamAvf::onStart()
{
  if (!impl || !impl->device) return false;

  @autoreleasepool {
    if (!impl->session)
    {
      impl->session = [[AVCaptureSession alloc] init];
      [impl->session beginConfiguration];

      NSError* error = nil;
      impl->input = [AVCaptureDeviceInput deviceInputWithDevice:impl->device error:&error];
      if (!impl->input || ![impl->session canAddInput:impl->input])
      {
        [impl->session commitConfiguration];
        impl->session = nil;
        return false;
      }
      [impl->session addInput:impl->input];

      impl->output = [[AVCaptureVideoDataOutput alloc] init];
      // 出力フォーマットを BGRA8888 に固定し、高速なゼロコピー・ハードウェア変換を利用する
      impl->output.videoSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
      };
      impl->output.alwaysDiscardsLateVideoFrames = prioritizeLatency ? YES : NO;

      impl->delegate = [[CamAvfDelegate alloc] initWithParent:this];
      impl->queue = dispatch_queue_create("CamAvfQueue", DISPATCH_QUEUE_SERIAL);
      [impl->output setSampleBufferDelegate:impl->delegate queue:impl->queue];

      if (![impl->session canAddOutput:impl->output])
      {
        [impl->session commitConfiguration];
        impl->session = nil;
        return false;
      }
      [impl->session addOutput:impl->output];

      [impl->session commitConfiguration];
    }

    // セッション構成後にフォーマットを適用する
    if (selectedFormatIndex >= 0 && selectedFormatIndex < static_cast<int>(availableFormats.size()))
    {
      const auto& vFmt = availableFormats[selectedFormatIndex];
      NSError* error = nil;
      if ([impl->device lockForConfiguration:&error])
      {
        if (static_cast<NSUInteger>(vFmt.formatIndex) < impl->device.formats.count)
        {
          impl->device.activeFormat = impl->device.formats[vFmt.formatIndex];
          int32_t fpsInt = static_cast<int32_t>(std::round(vFmt.fps));
          if (fpsInt > 0)
          {
            impl->device.activeVideoMinFrameDuration = CMTimeMake(1, fpsInt);
            impl->device.activeVideoMaxFrameDuration = CMTimeMake(1, fpsInt);
          }
        }
        [impl->device unlockForConfiguration];
      }
    }

    [impl->session startRunning];
    return impl->session.isRunning;
  }
}

//
// キャプチャ停止処理を行う
//
void CamAvf::onStop()
{
  if (impl)
  {
    if (impl->session && impl->session.isRunning)
    {
      @autoreleasepool {
        [impl->session stopRunning];
      }
    }
    // デリゲートキューに残っているフレーム処理の完了を待機する
    if (impl->queue)
    {
      dispatch_sync(impl->queue, ^{});
    }
  }
}

//
// キャプチャデバイスを閉じる処理を行う
//
void CamAvf::onClose()
{
  if (impl)
  {
    impl.reset();
  }
  availableFormats.clear();
  formatList.clear();
  selectedFormatIndex = 0;
}

//
// サンプルバッファからフレームデータを取得してバッファを更新する
//
void CamAvf::handleSampleBuffer(const void* sampleBufferRef)
{
  if (!running) return;

  // 全フレーム処理モードなら、メインスレッドがフレームを取得するまで待機する
  if (!prioritizeLatency)
  {
    while (running && !prioritizeLatency && captured)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!running) return;
  }

  CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)sampleBufferRef;
  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (!imageBuffer) return;

  CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

  const size_t w = CVPixelBufferGetWidth(imageBuffer);
  const size_t h = CVPixelBufferGetHeight(imageBuffer);
  const size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
  const uint8_t* src = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(imageBuffer));

  if (src && w > 0 && h > 0)
  {
    std::lock_guard<std::mutex> lock{ mtx };

    if (width != static_cast<int>(w) || height != static_cast<int>(h) || channels != 4)
    {
      width = static_cast<int>(w);
      height = static_cast<int>(h);
      channels = 4;
      image.resize(static_cast<size_t>(width) * height * channels);
    }

    const size_t rowBytes = static_cast<size_t>(width * channels);
    if (bytesPerRow == rowBytes)
    {
      std::memcpy(image.data(), src, std::min(image.size(), rowBytes * h));
    }
    else
    {
      const size_t copyWidth = std::min(rowBytes, bytesPerRow);
      for (size_t y = 0; y < h; ++y)
      {
        std::memcpy(image.data() + y * rowBytes, src + y * bytesPerRow, copyWidth);
      }
    }
    captured = true;
  }

  CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
}

#endif // defined(__APPLE__)
