///
/// AV Foundation を使ったビデオキャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 2026
///
/// @details macOS の AV Foundation ネイティブ API を直接使用し、
/// カメラデバイスの列挙、フォーマット（解像度・フレームレート・コーデック）の取得、
/// および高速なビデオフレームのキャプチャを行います。
/// OpenCV の VideoCapture におけるデバイス名取得不可やフォーマット変更不可といった
/// 制約を回避し、Windows (CamMf) と同等の低遅延・高機能なキャプチャ機能を提供します。
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
/// @details AVCaptureVideoDataOutputSampleBufferDelegate プロトコルを実装し、
/// 新しいビデオフレーム（CMSampleBuffer）が到着したときにディスパッチキューから
/// 非同期にコールバックを受け取ります。
/// Objective-C のデリゲートコールバックを C++ の CamAvf クラスのメンバ関数
/// handleSampleBuffer() へ安全に中継します。
///
@interface CamAvfDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
  CamAvf* parent; ///< キャプチャを処理する C++ クラスへのポインタ
}

///
/// 初期化
///
/// @param p CamAvf クラスのインスタンスへのポインタ
///
- (id)initWithParent:(CamAvf*)p;

@end

@implementation CamAvfDelegate

//
// 初期化
//
- (id)initWithParent:(CamAvf*)p
{
  self = [super init];
  if (self)
  {
    // 親となる C++ オブジェクトのポインタを保持する
    parent = p;
  }
  return self;
}

//
// 新しいビデオフレームが届いたときに呼ばれるコールバック
//
- (void)captureOutput:(AVCaptureOutput*)output
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection*)connection
{
  // 親オブジェクトが存在していればフレーム処理関数を呼び出す
  if (parent)
  {
    parent->handleSampleBuffer(sampleBuffer);
  }
}

@end

///
/// CamAvf の内部実装構造体 (PIMPL)
///
/// @details Objective-C / AV Foundation 固有の型やオブジェクトをカプセル化し、
/// CamAvf.h が純粋な C++ ヘッダとして他のソースファイルからインクルードできるようにします。
///
struct CamAvf::Impl
{
  AVCaptureSession* session{ nil };           ///< キャプチャデータフローを統括するセッション
  AVCaptureDevice* device{ nil };             ///< 選択されたビデオキャプチャデバイス
  AVCaptureDeviceInput* input{ nil };         ///< セッションへデバイスを接続する入力
  AVCaptureVideoDataOutput* output{ nil };    ///< ビデオフレームを取得する出力
  CamAvfDelegate* delegate{ nil };           ///< フレーム受信を中継するデリゲート
  dispatch_queue_t queue{ nil };              ///< フレームキャプチャ専用のシリアルディスパッチキュー
  int deviceIndex{ -1 };                      ///< 選択されているデバイスのインデックス番号

  ///
  /// デストラクタ
  ///
  /// @details セッションの停止、入出力の解除、およびデリゲートの破棄を確実に行います。
  ///
  ~Impl()
  {
    if (session)
    {
      // セッションが動作中であれば停止する
      if (session.isRunning)
      {
        [session stopRunning];
      }

      // セッションの構成変更を開始する
      [session beginConfiguration];

      // セッションから入力を削除する
      if (input)
      {
        [session removeInput:input];
        input = nil;
      }

      // デリゲートを解除してからセッションから出力を削除する
      if (output)
      {
        [output setSampleBufferDelegate:nil queue:NULL];
        [session removeOutput:output];
        output = nil;
      }

      // セッションの構成変更を確定する
      [session commitConfiguration];
      session = nil;
    }

    // デリゲートとキューの参照をクリアする
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
  /// @details macOS のバージョンに応じて適切なデバイスタイプを指定し、
  /// 内蔵カメラ、外部 USB Web カメラ、および連係カメラ (Continuity Camera) を検索します。
  ///
  NSArray<AVCaptureDevice*>* getVideoDevices()
  {
    // 検索対象とするデバイスタイプのリストを作成する
    NSMutableArray<AVCaptureDeviceType>* types = [NSMutableArray array];

    // 内蔵カメラ (FaceTime HD カメラ等) を追加する
    [types addObject:AVCaptureDeviceTypeBuiltInWideAngleCamera];

#if defined(__MAC_14_0)
    if (@available(macOS 14.0, *))
    {
      // macOS 14 (Sonoma) 以降では外部 UVC カメラと連係カメラが個別に分類される
      [types addObject:AVCaptureDeviceTypeExternal];
      [types addObject:AVCaptureDeviceTypeContinuityCamera];
    }
    else
#endif
    {
      // macOS 13 以前では外部カメラ全般を ExternalUnknown で検索する
      [types addObject:AVCaptureDeviceTypeExternalUnknown];
    }

    // 指定されたタイプのビデオ入力デバイスを検索・列挙するセッションを作成する
    AVCaptureDeviceDiscoverySession* discoverySession = [AVCaptureDeviceDiscoverySession
      discoverySessionWithDeviceTypes:types
      mediaType:AVMediaTypeVideo
      position:AVCaptureDevicePositionUnspecified];

    // 発見されたデバイスの配列を返す
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
    // 不正なインデックスの場合は nil を返す
    if (index < 0) return nil;

    // デバイス一覧を取得する
    NSArray<AVCaptureDevice*>* devices = getVideoDevices();

    // 範囲内であれば該当するデバイスを返す
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

///
/// デバイス名文字列を ImGui 表示用にサニタイズする
///
/// @param name サニタイズ対象のデバイス名 (UTF-8)
/// @return サニタイズ後のデバイス名
/// @details カメラ名に含まれる可能性のある制御文字（0x00-0x1F, 0x7F）の空白置換、
/// 特殊な引用符（‘, ’, “, ”）の標準 ASCII 記号（\', \"）への正規化、
/// フォントに含まれない 4 バイト絵文字（U+10000 以上）の除去、
/// および連続空白の圧縮と前後の余白除去を行い、ImGui での表示崩れを防ぎます。
///
static std::string sanitizeDeviceName(const std::string& name)
{
  std::string result;
  result.reserve(name.size());

  const auto len{ name.size() };
  for (size_t i = 0; i < len; )
  {
    const auto c{ static_cast<unsigned char>(name[i]) };

    // 1 バイト (ASCII: 0x00 - 0x7F)
    if (c <= 0x7F)
    {
      if (c < 0x20 || c == 0x7F)
      {
        result += ' '; // 制御文字は空白に置換
      }
      else
      {
        result += static_cast<char>(c);
      }
      ++i;
    }
    // 2 バイト文字 (0xC2 - 0xDF)
    else if ((c & 0xE0) == 0xC0)
    {
      if (i + 1 < len)
      {
        result += name.substr(i, 2);
        i += 2;
      }
      else
      {
        break; // 不正なバイト列
      }
    }
    // 3 バイト文字 (0xE0 - 0xEF)
    else if ((c & 0xF0) == 0xE0)
    {
      if (i + 2 < len)
      {
        const auto b1{ static_cast<unsigned char>(name[i + 1]) };
        const auto b2{ static_cast<unsigned char>(name[i + 2]) };

        // 特殊なクォーテーションの正規化
        // U+2018 (‘: E2 80 98) -> '
        // U+2019 (’: E2 80 99) -> '
        if (c == 0xE2 && b1 == 0x80 && (b2 == 0x98 || b2 == 0x99))
        {
          result += '\'';
        }
        // U+201C (“: E2 80 9C) -> "
        // U+201D (”: E2 80 9D) -> "
        else if (c == 0xE2 && b1 == 0x80 && (b2 == 0x9C || b2 == 0x9D))
        {
          result += '"';
        }
        else
        {
          result += name.substr(i, 3);
        }
        i += 3;
      }
      else
      {
        break; // 不正なバイト列
      }
    }
    // 4 バイト文字 (0xF0 - 0xF7: U+10000 以上の絵文字等)
    else if ((c & 0xF8) == 0xF0)
    {
      // フォントに収録されておらず ImGui で表示できないためスキップする
      i += (i + 4 <= len) ? 4 : (len - i);
    }
    else
    {
      // 不正なバイトはスキップ
      ++i;
    }
  }

  // 連続する空白文字を 1 つにまとめ、前後の空白を除去する
  std::string trimmed;
  trimmed.reserve(result.size());
  bool inSpace{ false };
  for (char ch : result)
  {
    if (ch == ' ')
    {
      if (!inSpace && !trimmed.empty())
      {
        trimmed += ' ';
        inSpace = true;
      }
    }
    else
    {
      trimmed += ch;
      inSpace = false;
    }
  }
  if (!trimmed.empty() && trimmed.back() == ' ')
  {
    trimmed.pop_back();
  }

  // 万一すべて除去されて空になった場合は既定の名前を使用する
  return trimmed.empty() ? "Camera" : trimmed;
}

//
// ビデオデバイスの一覧を取得する
//
const std::vector<std::string>& CamAvf::getDeviceList()
{
  // 以前のリストをクリアする
  deviceList.clear();

  // Objective-C の一時オブジェクトを即座に解放するための自動解放プール
  @autoreleasepool {
    // カメラのアクセス許可状態を確認する (macOS TCC: Transparency, Consent, and Control)
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    // アクセス権限が未決定 (初回起動時など) の場合は許可をリクエストし、ユーザーの選択を待機する
    if (status == AVAuthorizationStatusNotDetermined)
    {
      dispatch_semaphore_t sem = dispatch_semaphore_create(0);
      [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
        dispatch_semaphore_signal(sem);
      }];
      dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    }

    // 利用可能なデバイスの一覧を取得する
    NSArray<AVCaptureDevice*>* devices = getVideoDevices();

    // 各デバイスの表示名を取得してリストに追加する
    for (NSUInteger i = 0; i < devices.count; ++i)
    {
      AVCaptureDevice* dev = devices[i];

      // デバイスのローカライズ表示名を取り出す
      const char* rawName = [dev.localizedName UTF8String];

      // ImGui 表示用にサニタイズする
      std::string name = sanitizeDeviceName(rawName ? rawName : "");

      // 同一名称のカメラが存在する場合に識別できるよう "名前##番号" とする
      name += "##" + std::to_string(i);

      // リストに追加する
      deviceList.push_back(name);
    }
  }

  // デバイス名のリストを返す
  return deviceList;
}

//
// カメラを開く
//
bool CamAvf::open(int deviceNumber, bool setupFormat)
{
  // 既に開かれているデバイスがあれば閉じる
  close();

  @autoreleasepool {
    // 指定された番号のデバイスを取得する
    AVCaptureDevice* dev = getDeviceByIndex(deviceNumber);
    if (!dev) return false;

    // 内部実装オブジェクトを作成し、デバイス情報を設定する
    impl = std::make_unique<Impl>();
    impl->device = dev;
    impl->deviceIndex = deviceNumber;

    // デバイスがサポートするフォーマット一覧を列挙する
    if (!enumerateFormats())
    {
      close();
      return false;
    }

    if (setupFormat)
    {
      // 即時初期化モード: 既定（先頭）のフォーマットを適用する
      if (!select(0))
      {
        close();
        return false;
      }
    }
    else
    {
      // 遅延初期化モード: カメラ認識時にはハードウェアストリームやセッションを起動せず、
      // 既定フォーマットの解像度・フレームレート等のメタ情報のみを保持しておく
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
  // 以前のリストをクリアする
  formatList.clear();
  availableFormats.clear();

  // デバイスが有効でなければ戻る
  if (!impl || !impl->device) return false;

  @autoreleasepool {
    // デバイスがサポートするすべてのフォーマットを走査する
    NSArray<AVCaptureDeviceFormat*>* formats = impl->device.formats;
    for (NSUInteger fIdx = 0; fIdx < formats.count; ++fIdx)
    {
      AVCaptureDeviceFormat* format = formats[fIdx];

      // フォーマット記述子から解像度を取得する
      CMVideoFormatDescriptionRef formatDesc = format.formatDescription;
      CMVideoDimensions dims = CMVideoFormatDescriptionGetDimensions(formatDesc);
      int w = dims.width;
      int h = dims.height;

      // フォーマットのメディアサブタイプ（FourCC コーデックコード）を取得する
      FourCharCode subType = CMFormatDescriptionGetMediaSubType(formatDesc);

      // サブタイプに対応するコーデック名文字列を判定する
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
        // 既知の定数以外は FourCC 文字列または 16 進数値へ変換する
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

      // 各フォーマットがサポートするフレームレート範囲を走査する
      for (NSUInteger rIdx = 0; rIdx < format.videoSupportedFrameRateRanges.count; ++rIdx)
      {
        AVFrameRateRange* range = format.videoSupportedFrameRateRanges[rIdx];
        double maxFps = range.maxFrameRate;

        // 実用的なフレームレート（5 fps 以上）のみを対象とする
        if (maxFps < 5.0) continue;

        // UI 表示用の文字列を作成する
        std::ostringstream resOss;
        resOss << w << " x " << h;
        std::ostringstream fpsOss;
        fpsOss << std::fixed << std::setprecision(2) << maxFps;

        // 重複チェック (同一解像度、同一 fps、同一コーデックのエントリは重複して追加しない)
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
          // UI 表示用の構造体を追加する
          formatList.push_back({ resOss.str(), fpsOss.str(), codecName, static_cast<int>(availableFormats.size()) });

          // ハードウェア設定用の詳細フォーマット情報を追加する
          availableFormats.emplace_back(w, h, maxFps, codecName, static_cast<int>(fIdx), static_cast<int>(rIdx));
        }
      }
    }
  }

  // 1 つ以上のフォーマットが取得できていれば成功
  return !formatList.empty();
}

//
// フォーマットを選択して設定する
//
bool CamAvf::select(int index)
{
  // インデックスの妥当性を検査する
  if (!impl || !impl->device || index < 0 || index >= static_cast<int>(availableFormats.size()))
  {
    return false;
  }

  // キャプチャが実行中であれば一時停止する
  const bool restart{ running };
  if (restart) stop();

  // 選択されたフォーマットのインデックスを保持する
  selectedFormatIndex = index;
  const auto& vFmt = availableFormats[index];

  @autoreleasepool {
    NSError* error = nil;

    // デバイスの設定を変更するために排他ロックを取得する
    if ([impl->device lockForConfiguration:&error])
    {
      if (static_cast<NSUInteger>(vFmt.formatIndex) < impl->device.formats.count)
      {
        // 選択されたフォーマットをアクティブフォーマットとして適用する
        AVCaptureDeviceFormat* fmt = impl->device.formats[vFmt.formatIndex];
        impl->device.activeFormat = fmt;

        // フレームレートを CMTime 形式で設定する（最小・最大フレーム期間を指定）
        int32_t fpsInt = static_cast<int32_t>(std::round(vFmt.fps));
        if (fpsInt > 0)
        {
          impl->device.activeVideoMinFrameDuration = CMTimeMake(1, fpsInt);
          impl->device.activeVideoMaxFrameDuration = CMTimeMake(1, fpsInt);
        }
      }

      // デバイス設定の排他ロックを解除する
      [impl->device unlockForConfiguration];
    }
  }

  // カメラ基底クラスのプロパティを更新する
  width = vFmt.width;
  height = vFmt.height;
  channels = 4;
  interval = vFmt.fps > 0.0 ? 1000.0 / vFmt.fps : 10.0;

  // 画像バッファのメモリ領域を確保する
  image.resize(static_cast<size_t>(width) * height * channels);

  // 以前に実行中だった場合はキャプチャを再開する
  if (restart) start();

  return true;
}

//
// レイテンシ優先モードを設定する
//
void CamAvf::setPrioritizeLatency(bool mode)
{
  // 基底クラスの設定を更新する
  Camera::setPrioritizeLatency(mode);

  // AV Foundation 出力側のフレーム破棄ポリシーに反映する
  if (impl && impl->output)
  {
    // レイテンシ優先時は遅延した古いフレームを自動破棄 (YES) し、
    // 全フレーム処理時はフレームを破棄せず順番通りに処理 (NO) する
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
    // セッションがまだ作成されていなければ初期化する
    if (!impl->session)
    {
      // キャプチャセッションを生成する
      impl->session = [[AVCaptureSession alloc] init];

      // セッションの構成変更を開始する
      [impl->session beginConfiguration];

      // デバイス入力を作成してセッションに追加する
      NSError* error = nil;
      impl->input = [AVCaptureDeviceInput deviceInputWithDevice:impl->device error:&error];
      if (!impl->input || ![impl->session canAddInput:impl->input])
      {
        [impl->session commitConfiguration];
        impl->session = nil;
        return false;
      }
      [impl->session addInput:impl->input];

      // ビデオデータ出力を生成する
      impl->output = [[AVCaptureVideoDataOutput alloc] init];

      // 出力フォーマットを 32-bit BGRA に指定する
      // OS / ハードウェア側で BGRA への変換が自動的に行われ、CPU 側での無駄な色変換を排除する
      impl->output.videoSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
      };

      // レイテンシ優先設定に応じてフレーム破棄を制御する
      impl->output.alwaysDiscardsLateVideoFrames = prioritizeLatency ? YES : NO;

      // フレーム受信用のデリゲートとシリアルディスパッチキューを作成する
      impl->delegate = [[CamAvfDelegate alloc] initWithParent:this];
      impl->queue = dispatch_queue_create("CamAvfQueue", DISPATCH_QUEUE_SERIAL);
      [impl->output setSampleBufferDelegate:impl->delegate queue:impl->queue];

      // セッションに出力を追加する
      if (![impl->session canAddOutput:impl->output])
      {
        [impl->session commitConfiguration];
        impl->session = nil;
        return false;
      }
      [impl->session addOutput:impl->output];

      // セッションの構成変更を確定する
      [impl->session commitConfiguration];
    }

    // セッション構成後に、選択されたフォーマットをデバイスに適用する
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

    // キャプチャセッションを開始する
    [impl->session startRunning];

    // セッションが動作中であれば成功を返す
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
    // キャプチャセッションが動作中であれば停止する
    if (impl->session && impl->session.isRunning)
    {
      @autoreleasepool {
        [impl->session stopRunning];
      }
    }

    // ディスパッチキューに残っているフレーム処理の完了を待機する
    // これにより、停止後にコールバックが実行されて不正なメモリアクセスが発生するのを防ぐ
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
  // 内部実装オブジェクトを解放する（セッション停止とリソース解放が行われる）
  if (impl)
  {
    impl.reset();
  }

  // フォーマット情報をクリアする
  availableFormats.clear();
  formatList.clear();
  selectedFormatIndex = 0;
}

//
// サンプルバッファからフレームデータを取得してバッファを更新する
//
void CamAvf::handleSampleBuffer(const void* sampleBufferRef)
{
  // キャプチャが停止していれば何もしない
  if (!running) return;

  // 全フレーム処理モードの場合、メインスレッドが前フレームを処理（取得）するまで待機する
  if (!prioritizeLatency)
  {
    while (running && !prioritizeLatency && captured)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!running) return;
  }

  // CMSampleBuffer から画像バッファ (CVImageBufferRef) を取得する
  CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)sampleBufferRef;
  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (!imageBuffer) return;

  // 画像バッファのアドレスを読み取り専用でロックする
  CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

  // ピクセルバッファのサイズとストライド（1行あたりのバイト数）、先頭ポインタを取得する
  const size_t w = CVPixelBufferGetWidth(imageBuffer);
  const size_t h = CVPixelBufferGetHeight(imageBuffer);
  const size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
  const uint8_t* src = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(imageBuffer));

  if (src && w > 0 && h > 0)
  {
    // メインスレッドとのバッファ競合を防ぐためにミューテックスをロックする
    std::lock_guard<std::mutex> lock{ mtx };

    // 解像度やチャンネル数が変更されていれば内部バッファをリサイズする
    if (width != static_cast<int>(w) || height != static_cast<int>(h) || channels != 4)
    {
      width = static_cast<int>(w);
      height = static_cast<int>(h);
      channels = 4;
      image.resize(static_cast<size_t>(width) * height * channels);
    }

    // 1行あたりの有効バイト数を計算する (width * 4)
    const size_t rowBytes = static_cast<size_t>(width * channels);

    // アライメントパディングの有無を判定してコピーする
    if (bytesPerRow == rowBytes)
    {
      // パディングがない場合は連続領域として一括コピーする
      std::memcpy(image.data(), src, std::min(image.size(), rowBytes * h));
    }
    else
    {
      // 各行末尾にパディングが含まれる場合は行ごとにコピーする
      const size_t copyWidth = std::min(rowBytes, bytesPerRow);
      for (size_t y = 0; y < h; ++y)
      {
        std::memcpy(image.data() + y * rowBytes, src + y * bytesPerRow, copyWidth);
      }
    }

    // 新しいフレームが取得されたことを通知するフラグを立てる
    captured = true;
  }

  // 画像バッファのアドレスロックを解除する
  CVPixelBufferUnlockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);
}

#endif // defined(__APPLE__)
