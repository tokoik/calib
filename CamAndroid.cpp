///
/// Android Camera2 NDK を使ったビデオキャプチャクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 2026
///

#if defined(__ANDROID__)

#include "CamAndroid.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <limits>
#include <chrono>

#define LOG_TAG "CamAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{
  //
  // 0〜255 の範囲にクランプする高速ヘルパー関数
  //
  inline uint8_t clampToUint8(int val)
  {
    return static_cast<uint8_t>((val < 0) ? 0 : ((val > 255) ? 255 : val));
  }
}

std::vector<std::string> CamAndroid::deviceList;

//
// コンストラクタ
//
CamAndroid::CamAndroid(int deviceNumber, int initial_width, int initial_height, double initial_fps)
{
  open(deviceNumber, initial_width, initial_height, initial_fps);
}

//
// デストラクタ
//
CamAndroid::~CamAndroid()
{
  close();
}

//
// 使用可能なカメラデバイスのリストを返す
//
const std::vector<std::string>& CamAndroid::getDeviceList()
{
  deviceList.clear();

  ACameraManager* mgr{ ACameraManager_create() };
  if (!mgr) return deviceList;

  ACameraIdList* idList{ nullptr };
  camera_status_t status{ ACameraManager_getCameraIdList(mgr, &idList) };
  if (status == ACAMERA_OK && idList)
  {
    for (int i = 0; i < idList->numCameras; ++i)
    {
      const char* id{ idList->cameraIds[i] };
      ACameraMetadata* chars{ nullptr };
      status = ACameraManager_getCameraCharacteristics(mgr, id, &chars);

      std::string facingStr{ "External" };
      if (status == ACAMERA_OK && chars)
      {
        ACameraMetadata_const_entry entry;
        if (ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &entry) == ACAMERA_OK)
        {
          auto facing{ entry.data.u8[0] };
          if (facing == ACAMERA_LENS_FACING_BACK)
          {
            facingStr = "Back";
          }
          else if (facing == ACAMERA_LENS_FACING_FRONT)
          {
            facingStr = "Front";
          }
        }
        ACameraMetadata_free(chars);
      }

      std::ostringstream oss;
      oss << i << ": Camera " << id << " (" << facingStr << ")";
      deviceList.push_back(oss.str());
    }
    ACameraManager_deleteCameraIdList(idList);
  }

  ACameraManager_delete(mgr);
  return deviceList;
}

//
// キャプチャデバイスを開く
//
bool CamAndroid::open(int deviceNumber, int initial_width, int initial_height, double initial_fps)
{
  close();

  cameraManager = ACameraManager_create();
  if (!cameraManager)
  {
    LOGE("Failed to create ACameraManager");
    return false;
  }

  ACameraIdList* idList{ nullptr };
  camera_status_t status{ ACameraManager_getCameraIdList(cameraManager, &idList) };
  if (status != ACAMERA_OK || !idList || deviceNumber < 0 || deviceNumber >= idList->numCameras)
  {
    if (idList) ACameraManager_deleteCameraIdList(idList);
    LOGE("Invalid camera device number: %d", deviceNumber);
    return false;
  }

  selectedCameraId = idList->cameraIds[deviceNumber];
  deviceIndex = deviceNumber;
  ACameraManager_deleteCameraIdList(idList);

  // カメラ特性から解像度一覧を取得
  ACameraMetadata* chars{ nullptr };
  status = ACameraManager_getCameraCharacteristics(cameraManager, selectedCameraId.c_str(), &chars);
  if (status == ACAMERA_OK && chars)
  {
    enumerateFormats(chars);
    ACameraMetadata_free(chars);
  }

  // デフォルト解像度の決定 (リアルタイム処理に最適な 1280x720 を優先)
  if (initial_width > 0 && initial_height > 0)
  {
    width = initial_width;
    height = initial_height;
  }
  else if (!formatList.empty())
  {
    int defaultIndex{ 0 };
    int bestScore{ std::numeric_limits<int>::max() };
    const int targetW{ 1280 };
    const int targetH{ 720 };
    const int targetArea{ targetW * targetH };

    for (size_t i = 0; i < formatList.size(); ++i)
    {
      int fw{ 0 }, fh{ 0 };
      if (sscanf(formatList[i].resolution.c_str(), "%d x %d", &fw, &fh) == 2)
      {
        if (fw == targetW && fh == targetH)
        {
          defaultIndex = static_cast<int>(i);
          break;
        }

        // 1920x1080 を超える超高解像度はリアルタイム処理で遅延するためペナルティ
        int score{ std::abs(fw * fh - targetArea) };
        if (fw * fh > 1920 * 1080)
        {
          score += 10000000;
        }

        if (score < bestScore)
        {
          bestScore = score;
          defaultIndex = static_cast<int>(i);
        }
      }
    }

    selectFormat(defaultIndex);
  }
  else
  {
    width = 1280;
    height = 720;
  }

  channels = 4; // BGRA
  interval = (initial_fps > 0.0) ? (1000.0 / initial_fps) : 33.3;

  LOGI("Opened camera %s (%d x %d)", selectedCameraId.c_str(), width, height);
  return true;
}

//
// 利用可能なカメラフォーマットを列挙する
//
void CamAndroid::enumerateFormats(ACameraMetadata* metadata)
{
  formatList.clear();

  ACameraMetadata_const_entry entry;
  camera_status_t status{ ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry) };
  if (status != ACAMERA_OK) return;

  // 各エントリは (format, width, height, isInput) の 4 要素
  int formatIndex{ 0 };
  for (uint32_t i = 0; i < entry.count; i += 4)
  {
    int32_t fmt{ entry.data.i32[i + 0] };
    int32_t w{ entry.data.i32[i + 1] };
    int32_t h{ entry.data.i32[i + 2] };
    int32_t isInput{ entry.data.i32[i + 3] };

    // YUV_420_888 の出力ストリームのみ対象
    if (fmt == AIMAGE_FORMAT_YUV_420_888 && isInput == 0)
    {
      std::ostringstream resOss;
      resOss << w << " x " << h;
      std::string resStr{ resOss.str() };

      // 重複チェック
      bool exists{ false };
      for (const auto& item : formatList)
      {
        if (item.resolution == resStr)
        {
          exists = true;
          break;
        }
      }

      if (!exists)
      {
        formatList.emplace_back(resStr, "30.00", "YUV420", formatIndex++);
      }
    }
  }

  // 解像度の降順にソート
  std::sort(formatList.begin(), formatList.end(), [](const CaptureFormat& a, const CaptureFormat& b) {
    int wA = 0, hA = 0, wB = 0, hB = 0;
    sscanf(a.resolution.c_str(), "%d x %d", &wA, &hA);
    sscanf(b.resolution.c_str(), "%d x %d", &wB, &hB);
    return (wA * hA) > (wB * hB);
  });

  // index の再割り当て
  for (size_t i = 0; i < formatList.size(); ++i)
  {
    formatList[i].index = static_cast<int>(i);
  }
}

//
// フォーマットを選択する
//
bool CamAndroid::selectFormat(int index)
{
  if (index < 0 || index >= static_cast<int>(formatList.size())) return false;

  int w{ 0 }, h{ 0 };
  if (sscanf(formatList[index].resolution.c_str(), "%d x %d", &w, &h) == 2)
  {
    bool restart{ running };
    if (restart) stop();

    width = w;
    height = h;

    if (restart) start();
    return true;
  }

  return false;
}

//
// キャプチャ開始処理
//
bool CamAndroid::onStart()
{
  if (!cameraManager || selectedCameraId.empty()) return false;

  // AImageReader の作成
  media_status_t mStatus{ AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, 4, &imageReader) };
  if (mStatus != AMEDIA_OK || !imageReader)
  {
    LOGE("Failed to create AImageReader (%d x %d)", width, height);
    return false;
  }

  AImageReader_ImageListener listener;
  listener.context = this;
  listener.onImageAvailable = onImageAvailableCallback;
  AImageReader_setImageListener(imageReader, &listener);

  mStatus = AImageReader_getWindow(imageReader, &imageWindow);
  if (mStatus != AMEDIA_OK || !imageWindow)
  {
    LOGE("Failed to get ANativeWindow from AImageReader");
    AImageReader_delete(imageReader);
    imageReader = nullptr;
    return false;
  }

  // カメラデバイスのオープン
  static ACameraDevice_stateCallbacks devCallbacks;
  devCallbacks.context = this;
  devCallbacks.onDisconnected = [](void*, ACameraDevice*) {};
  devCallbacks.onError = [](void*, ACameraDevice*, int error) {
    LOGE("CameraDevice error: %d", error);
  };

  camera_status_t cStatus{ ACameraManager_openCamera(cameraManager, selectedCameraId.c_str(), &devCallbacks, &cameraDevice) };
  if (cStatus != ACAMERA_OK || !cameraDevice)
  {
    LOGE("Failed to open camera %s", selectedCameraId.c_str());
    AImageReader_delete(imageReader);
    imageReader = nullptr;
    imageWindow = nullptr;
    return false;
  }

  // キャプチャリクエスト作成
  cStatus = ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &captureRequest);
  if (cStatus != ACAMERA_OK || !captureRequest)
  {
    LOGE("Failed to create capture request");
    onStop();
    return false;
  }

  // ターゲットの関連付け
  ACameraOutputTarget_create(imageWindow, &outputTarget);
  ACaptureRequest_addTarget(captureRequest, outputTarget);

  // セッション出力コンテナ
  ACaptureSessionOutputContainer_create(&outputContainer);
  ACaptureSessionOutput_create(imageWindow, &sessionOutput);
  ACaptureSessionOutputContainer_add(outputContainer, sessionOutput);

  // キャプチャセッション作成
  static ACameraCaptureSession_stateCallbacks sessionCallbacks;
  sessionCallbacks.context = this;
  sessionCallbacks.onActive = [](void*, ACameraCaptureSession*) {};
  sessionCallbacks.onClosed = [](void* context, ACameraCaptureSession*) {
    auto* self{ static_cast<CamAndroid*>(context) };
    if (self)
    {
      {
        std::lock_guard<std::mutex> lock(self->sessionMtx);
        self->sessionClosed = true;
      }
      self->sessionCv.notify_all();
    }
  };
  sessionCallbacks.onReady = [](void*, ACameraCaptureSession*) {};

  cStatus = ACameraDevice_createCaptureSession(cameraDevice, outputContainer, &sessionCallbacks, &captureSession);
  if (cStatus != ACAMERA_OK || !captureSession)
  {
    LOGE("Failed to create capture session");
    onStop();
    return false;
  }

  // プレビューの連続キャプチャ要求開始
  cStatus = ACameraCaptureSession_setRepeatingRequest(captureSession, nullptr, 1, &captureRequest, nullptr);
  if (cStatus != ACAMERA_OK)
  {
    LOGE("Failed to set repeating request");
    onStop();
    return false;
  }

  LOGI("Camera capture started successfully: %s (%d x %d)", selectedCameraId.c_str(), width, height);
  return true;
}

//
// キャプチャ停止処理
//
void CamAndroid::onStop()
{
  // 1. AImageReader のリスナーを即座に解除して新規フレームのコールバックを停止
  if (imageReader)
  {
    AImageReader_setImageListener(imageReader, nullptr);
  }

  // 2. キャプチャセッションのリクエストを停止・中止し、安全にクローズ
  if (captureSession)
  {
    ACameraCaptureSession_stopRepeating(captureSession);
    ACameraCaptureSession_abortCaptures(captureSession);

    sessionClosed = false;
    ACameraCaptureSession_close(captureSession);

    // セッションのクローズ完了通知 (onClosed) を最大 500ms 待機
    std::unique_lock<std::mutex> lock(sessionMtx);
    sessionCv.wait_for(lock, std::chrono::milliseconds(500), [this] {
      return sessionClosed.load();
    });

    captureSession = nullptr;
  }

  // 3. キャプチャリクエストおよびターゲットの解放
  if (captureRequest)
  {
    if (outputTarget)
    {
      ACaptureRequest_removeTarget(captureRequest, outputTarget);
    }
    ACaptureRequest_free(captureRequest);
    captureRequest = nullptr;
  }

  if (outputTarget)
  {
    ACameraOutputTarget_free(outputTarget);
    outputTarget = nullptr;
  }

  if (sessionOutput && outputContainer)
  {
    ACaptureSessionOutputContainer_remove(outputContainer, sessionOutput);
  }

  if (sessionOutput)
  {
    ACaptureSessionOutput_free(sessionOutput);
    sessionOutput = nullptr;
  }

  if (outputContainer)
  {
    ACaptureSessionOutputContainer_free(outputContainer);
    outputContainer = nullptr;
  }

  // 4. カメラデバイスのクローズ (セッション終了後に呼ぶことでエラー code 3 を防止)
  if (cameraDevice)
  {
    ACameraDevice_close(cameraDevice);
    cameraDevice = nullptr;
  }

  // 5. イメージリーダーの解放
  if (imageReader)
  {
    AImageReader_delete(imageReader);
    imageReader = nullptr;
    imageWindow = nullptr;
  }

  LOGI("Camera capture stopped");
}

//
// キャプチャデバイスを閉じる処理
//
void CamAndroid::onClose()
{
  onStop();

  if (cameraManager)
  {
    ACameraManager_delete(cameraManager);
    cameraManager = nullptr;
  }

  selectedCameraId.clear();
}

//
// 画像取得時のコールバック
//
void CamAndroid::onImageAvailableCallback(void* context, AImageReader* reader)
{
  auto* self{ static_cast<CamAndroid*>(context) };
  if (!self || !self->running) return;

  AImage* image{ nullptr };
  media_status_t status;

  if (self->prioritizeLatency)
  {
    // レイテンシ優先時は最新フレームを取得し、古いフレームを破棄
    status = AImageReader_acquireLatestImage(reader, &image);
  }
  else
  {
    status = AImageReader_acquireNextImage(reader, &image);
  }

  if (status == AMEDIA_OK && image)
  {
    if (self->running)
    {
      self->convertYuvToBgra(image);
    }
    AImage_delete(image);
  }
}

//
// YUV420_888 から BGRA8888 への高速カラー変換
//
void CamAndroid::convertYuvToBgra(AImage* img)
{
  if (!running) return;

  int32_t w{ 0 }, h{ 0 };
  AImage_getWidth(img, &w);
  AImage_getHeight(img, &h);
  if (w <= 0 || h <= 0) return;

  uint8_t* yPlane{ nullptr };
  uint8_t* uPlane{ nullptr };
  uint8_t* vPlane{ nullptr };
  int yLen{ 0 }, uLen{ 0 }, vLen{ 0 };
  int yRowStride{ 0 }, uRowStride{ 0 }, vRowStride{ 0 };
  int uPixelStride{ 0 }, vPixelStride{ 0 };

  AImage_getPlaneData(img, 0, &yPlane, &yLen);
  AImage_getPlaneRowStride(img, 0, &yRowStride);

  AImage_getPlaneData(img, 1, &uPlane, &uLen);
  AImage_getPlaneRowStride(img, 1, &uRowStride);
  AImage_getPlanePixelStride(img, 1, &uPixelStride);

  AImage_getPlaneData(img, 2, &vPlane, &vLen);
  AImage_getPlaneRowStride(img, 2, &vRowStride);
  AImage_getPlanePixelStride(img, 2, &vPixelStride);

  if (!yPlane || !uPlane || !vPlane) return;

  const size_t outSize{ static_cast<size_t>(w * h * 4) };

  // フレームバッファの排他更新
  std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
  if (!lock.owns_lock() || !running)
  {
    // メインスレッドが読み出し中または停止中はスキップ
    return;
  }

  if (image.size() != outSize)
  {
    image.resize(outSize);
    width = w;
    height = h;
    channels = 4;
  }

  uint8_t* dst{ image.data() };

  // YUV420 -> BGRA 高速変換ループ (2 画素単位で UV 演算を共有)
  for (int y = 0; y < h; ++y)
  {
    const uint8_t* yRow{ yPlane + y * yRowStride };
    const uint8_t* uRow{ uPlane + (y / 2) * uRowStride };
    const uint8_t* vRow{ vPlane + (y / 2) * vRowStride };
    uint8_t* dstRow{ dst + y * w * 4 };

    for (int x = 0; x < w; x += 2)
    {
      const int uvIdx{ x / 2 };
      const int U{ uRow[uvIdx * uPixelStride] };
      const int V{ vRow[uvIdx * vPixelStride] };

      const int D{ U - 128 };
      const int E{ V - 128 };

      const int rCoeff{ 409 * E + 128 };
      const int gCoeff{ -100 * D - 208 * E + 128 };
      const int bCoeff{ 516 * D + 128 };

      // 画素 1 (x)
      {
        const int C{ (yRow[x] - 16) * 298 };
        const int R{ (C + rCoeff) >> 8 };
        const int G{ (C + gCoeff) >> 8 };
        const int B{ (C + bCoeff) >> 8 };

        dstRow[x * 4 + 0] = clampToUint8(B);
        dstRow[x * 4 + 1] = clampToUint8(G);
        dstRow[x * 4 + 2] = clampToUint8(R);
        dstRow[x * 4 + 3] = 255;
      }

      // 画素 2 (x + 1)
      if (x + 1 < w)
      {
        const int C{ (yRow[x + 1] - 16) * 298 };
        const int R{ (C + rCoeff) >> 8 };
        const int G{ (C + gCoeff) >> 8 };
        const int B{ (C + bCoeff) >> 8 };

        dstRow[(x + 1) * 4 + 0] = clampToUint8(B);
        dstRow[(x + 1) * 4 + 1] = clampToUint8(G);
        dstRow[(x + 1) * 4 + 2] = clampToUint8(R);
        dstRow[(x + 1) * 4 + 3] = 255;
      }
    }
  }

  captured = true;
}

#endif // defined(__ANDROID__)
