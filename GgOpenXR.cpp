///
/// OpenXR/OpenGL バックエンドの実装.
///
/// @file
/// @author Kohe Tokoi
/// @date July 20, 2026
///
#include "GgOpenXR.h"

// 補助プログラム
#include "gg.h"

// 標準ライブラリ
#include <algorithm>
#include <cstring>
#include <vector>

// OpenXR を使うとき
#if defined(GG_ENABLE_OPENXR)
#  define XR_USE_PLATFORM_WIN32
#  define XR_USE_GRAPHICS_API_OPENGL
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  include <openxr/openxr.h>
#  include <openxr/openxr_platform.h>
#  include <windows.h>
#endif

//
// OpenXR / OpenGL バックエンドの実装クラス.
//
class GgOpenXR::Impl
{
public:

  // OpenXR 型を含まない公開用データは
  // OpenXR 無効ビルドでも同じ API を提供できるよう
  // 条件付きコンパイルの外に置く。

  // PRIMARY_STEREO 構成で列挙された view の数.
  std::vector<View> publicViews;

  // 基準空間における HMD 中央の姿勢.
  Pose headPose;

  // HMD 中央の姿勢が有効かどうか.
  bool poseValid{ false };

  // ランタイムからアプリケーションの終了が要求されているかどうか.
  bool closeRequested{ false };

  // 現在フレームを描画するようランタイムが要求しているかどうか.
  bool renderRequested{ false };

#if defined(GG_ENABLE_OPENXR)
  // 一つの view に対応する swapchain と OpenGL 側の描画資源。
  // images はランタイム所有のテクスチャであり、fbos だけを本クラスが所有する。

  //
  // OpenXR の swapchain image.
  // ランタイムが所有するため、OpenGL 側で FBO を作成して描画する。
  //
  struct Swapchain
  {
    XrSwapchain handle{ XR_NULL_HANDLE };
    uint32_t width{ 0 };
    uint32_t height{ 0 };
    uint32_t imageIndex{ 0 };
    bool acquired{ false };
    std::vector<XrSwapchainImageOpenGLKHR> images;
    std::vector<GLuint> fbos;
  };

  // OpenXR 資源は instance -> session -> space/swapchain の親子関係を持つ。
  // destroy() では必ずこの逆順で破棄する。
  XrInstance instance{ XR_NULL_HANDLE };
  XrSystemId systemId{ XR_NULL_SYSTEM_ID };
  XrSession session{ XR_NULL_HANDLE };
  XrSpace space{ XR_NULL_HANDLE };
  XrSpace viewSpace{ XR_NULL_HANDLE };
  XrSessionState sessionState{ XR_SESSION_STATE_UNKNOWN };
  XrFrameState frameState{ XR_TYPE_FRAME_STATE };
  bool sessionRunning{ false };
  bool frameActive{ false };
  std::vector<XrView> views;
  std::vector<Swapchain> swapchains;

  //
  // OpenXR の呼び出し結果が成功かどうかを判定する.
  //
  bool check(XrResult result) const
  {
    return XR_SUCCEEDED(result);
  }

  //
  // OpenXR 資源を依存関係の逆順で破棄する.
  //
  void destroy()
  {
    // beginFrame() 後に例外的な終了経路へ入っても、OpenXR の呼び出し順序を
    // 壊さないよう空の xrEndFrame() で未完了フレームを閉じる。
    if (frameActive && session != XR_NULL_HANDLE)
    {
      // xrEndFrame() では、描画しない場合でも空の projection layer を提出する必要がある。
      XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };

      // projection layer を提出しない場合は、layerCount = 0 でよい。
      endInfo.displayTime = frameState.predictedDisplayTime;

      // ランタイムが要求する描画を行わない場合は、XR_ENVIRONMENT_BLEND_MODE_OPAQUE を指定する。
      endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

      // projection layer を提出しない場合は、layers = nullptr でよい。
      xrEndFrame(session, &endInfo);

      // 例外的な終了経路で xrEndFrame() を呼んだ場合は、次のフレームで beginFrame() が失敗する。
      frameActive = false;
    }

    // swapchain image をランタイムへ返してから swapchain を破棄する。
    for (auto& swapchain : swapchains)
    {
      // acquire 済み画像はランタイムへ返してから swapchain を破棄する。
      if (swapchain.acquired)
      {
        // acquire した画像を返すための構造体を作る。
        XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };

        // swapchain image をランタイムへ返す。
        xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
      }

      // swapchain image はランタイムが所有するため、FBO だけを破棄する。
      if (!swapchain.fbos.empty())
        glDeleteFramebuffers(static_cast<GLsizei>(swapchain.fbos.size()), swapchain.fbos.data());

      // swapchain は OpenXR が所有するため、OpenXR の関数で破棄する。
      if (swapchain.handle != XR_NULL_HANDLE) xrDestroySwapchain(swapchain.handle);
    }

    // OpenXR の資源を依存関係の逆順で破棄する。
    swapchains.clear();
    views.clear();
    publicViews.clear();

    // 子オブジェクトから親オブジェクトの順に解放する。
    if (viewSpace != XR_NULL_HANDLE) xrDestroySpace(viewSpace);
    viewSpace = XR_NULL_HANDLE;
    if (space != XR_NULL_HANDLE) xrDestroySpace(space);
    space = XR_NULL_HANDLE;
    if (sessionRunning && session != XR_NULL_HANDLE) xrEndSession(session);
    sessionRunning = false;
    if (session != XR_NULL_HANDLE) xrDestroySession(session);
    session = XR_NULL_HANDLE;
    if (instance != XR_NULL_HANDLE) xrDestroyInstance(instance);
    instance = XR_NULL_HANDLE;
  }
#endif
};

//
// コンストラクタ.
//
GgOpenXR::GgOpenXR()
  : impl{ std::make_unique<Impl>() }
{
}

//
// デストラクタ. 保持している OpenXR 資源を破棄する.
//
GgOpenXR::~GgOpenXR()
{
  shutdown();
}

//
// OpenXR と 現在の GLFW/OpenGL コンテキストを初期化する.
// 
bool GgOpenXR::initialize(GLFWwindow* window, const std::string& applicationName)
{
#if !defined(GG_ENABLE_OPENXR)
  // OpenXR が無効ビルドのときは、引数を使わずに false を返す。
  (void)window;
  (void)applicationName;
  return false;
#else
  // 再初期化時にも古い session や FBO を残さない。
  shutdown();

  // OpenGL の texture を swapchain image として受け取るために必要な拡張を有効化する。
  const char* extensions[]{ XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
  XrInstanceCreateInfo instanceInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
  std::strncpy(instanceInfo.applicationInfo.applicationName, applicationName.c_str(),
    XR_MAX_APPLICATION_NAME_SIZE - 1);
  std::strncpy(instanceInfo.applicationInfo.engineName, "GgApp", XR_MAX_ENGINE_NAME_SIZE - 1);
  instanceInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  instanceInfo.enabledExtensionCount = 1;
  instanceInfo.enabledExtensionNames = extensions;
  if (!impl->check(xrCreateInstance(&instanceInfo, &impl->instance))) return false;

  // 現在アクティブなランタイムから HMD 用 system を取得する。
  XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
  systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  if (!impl->check(xrGetSystem(impl->instance, &systemInfo, &impl->systemId)))
  {
    shutdown();
    return false;
  }

  // OpenGL graphics requirements は拡張関数なので、instance から関数ポインタを取得する。
  PFN_xrGetOpenGLGraphicsRequirementsKHR getRequirements{};
  if (!impl->check(xrGetInstanceProcAddr(impl->instance, "xrGetOpenGLGraphicsRequirementsKHR",
    reinterpret_cast<PFN_xrVoidFunction*>(&getRequirements))))
  {
    shutdown();
    return false;
  }
  XrGraphicsRequirementsOpenGLKHR requirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
  if (!impl->check(getRequirements(impl->instance, impl->systemId, &requirements)))
  {
    shutdown();
    return false;
  }

  // OpenXR session を現在の GLFW/WGL コンテキストへ結び付ける。
  // GetDC() で借用した HDC は xrCreateSession() の直後に返却する。  
  const HWND hwnd{ glfwGetWin32Window(window) };
  XrGraphicsBindingOpenGLWin32KHR binding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
  binding.hDC = GetDC(hwnd);
  binding.hGLRC = wglGetCurrentContext();
  if (!binding.hDC || !binding.hGLRC)
  {
    if (binding.hDC) ReleaseDC(hwnd, binding.hDC);
    shutdown();
    return false;
  }
  XrSessionCreateInfo sessionInfo{ XR_TYPE_SESSION_CREATE_INFO };
  sessionInfo.next = &binding;
  sessionInfo.systemId = impl->systemId;
  const XrResult sessionResult{ xrCreateSession(impl->instance, &sessionInfo, &impl->session) };
  ReleaseDC(hwnd, binding.hDC);
  if (!impl->check(sessionResult))
  {
    shutdown();
    return false;
  }

  // アプリケーションの基準空間には床基準の STAGE を優先し、
  // 利用できない場合はランタイムでは起動位置基準の LOCAL へフォールバックする。
  XrReferenceSpaceCreateInfo spaceInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
  spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
  if (XR_FAILED(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->space)))
  {
    spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    if (!impl->check(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->space)))
    {
      shutdown();
      return false;
    }
  }

  // HMD 中央姿勢を眼ごとの pose から推測せず取得するため、VIEW 空間も作成する。
  spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  if (!impl->check(xrCreateReferenceSpace(impl->session, &spaceInfo, &impl->viewSpace)))
  {
    shutdown();
    return false;
  }

  // PRIMARY_STEREO の view 数と、各 view の推奨解像度をランタイムから取得する。
  // view 数を 2 に固定しないことで、公開 API と資源管理を列挙結果に一致させる。
  uint32_t viewCount{};
  xrEnumerateViewConfigurationViews(impl->instance, impl->systemId,
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
  std::vector<XrViewConfigurationView> configurations(viewCount,
    { XR_TYPE_VIEW_CONFIGURATION_VIEW });
  if (!impl->check(xrEnumerateViewConfigurationViews(impl->instance, impl->systemId,
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, configurations.data())))
  {
    shutdown();
    return false;
  }

  // ランタイムが受け付ける swapchain format の中から、
  // 通常の OpenGL 描画と相互運用できる sRGB RGBA、RGBA の順で選択する。
  uint32_t formatCount{};
  xrEnumerateSwapchainFormats(impl->session, 0, &formatCount, nullptr);
  std::vector<int64_t> formats(formatCount);
  xrEnumerateSwapchainFormats(impl->session, formatCount, &formatCount, formats.data());
  const int64_t preferred[]{ GL_SRGB8_ALPHA8, GL_RGBA8 };
  int64_t format{};
  for (const auto candidate : preferred)
    if (std::find(formats.begin(), formats.end(), candidate) != formats.end()) { format = candidate; break; }
  if (!format)
  {
    shutdown();
    return false;
  }

  impl->views.assign(viewCount, { XR_TYPE_VIEW });
  impl->publicViews.resize(viewCount);
  impl->swapchains.resize(viewCount);
  for (uint32_t i{}; i < viewCount; ++i)
  {
    // view ごとに独立した swapchain を作り、その全 image に対応する FBO を用意する。
    // 実際に描画する image は毎フレーム xrAcquireSwapchainImage() が決定する。
    auto& swapchain{ impl->swapchains[i] };
    swapchain.width = configurations[i].recommendedImageRectWidth;
    swapchain.height = configurations[i].recommendedImageRectHeight;
    XrSwapchainCreateInfo createInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
    createInfo.format = format;
    createInfo.sampleCount = 1;
    createInfo.width = swapchain.width;
    createInfo.height = swapchain.height;
    createInfo.faceCount = 1;
    createInfo.arraySize = 1;
    createInfo.mipCount = 1;
    if (!impl->check(xrCreateSwapchain(impl->session, &createInfo, &swapchain.handle)))
    {
      shutdown();
      return false;
    }
    uint32_t imageCount{};
    xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr);
    swapchain.images.assign(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
    xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data()));
    swapchain.fbos.resize(imageCount);
    glGenFramebuffers(static_cast<GLsizei>(imageCount), swapchain.fbos.data());
    for (uint32_t image{}; image < imageCount; ++image)
    {
      // OpenXR が所有する OpenGL texture をカラー attachment として借用する。
      glBindFramebuffer(GL_FRAMEBUFFER, swapchain.fbos[image]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        swapchain.images[image].image, 0);
    }
    impl->publicViews[i].width = static_cast<int>(swapchain.width);
    impl->publicViews[i].height = static_cast<int>(swapchain.height);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return true;
#endif
}

//
// OpenXR 資源を依存関係の逆順で破棄する.
//
void GgOpenXR::shutdown()
{
#if defined(GG_ENABLE_OPENXR)
  impl->destroy();
#endif
  impl->closeRequested = false;
  impl->renderRequested = false;
  impl->poseValid = false;
}

//
// OpenXR イベントを取得してセッション状態を更新する.
//
void GgOpenXR::pollEvents()
{
#if defined(GG_ENABLE_OPENXR)
  if (impl->instance == XR_NULL_HANDLE) return;
  // OpenXR の session はイベント駆動で開始・停止する。READY になる前に
  // xrBeginSession() を呼ばず、STOPPING を受けたときだけ xrEndSession() する。
  XrEventDataBuffer event{ XR_TYPE_EVENT_DATA_BUFFER };
  while (xrPollEvent(impl->instance, &event) == XR_SUCCESS)
  {
    if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) impl->closeRequested = true;
    else if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
    {
      const auto& changed{ *reinterpret_cast<XrEventDataSessionStateChanged*>(&event) };
      impl->sessionState = changed.state;
      if (changed.state == XR_SESSION_STATE_READY && !impl->sessionRunning)
      {
        XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        impl->sessionRunning = XR_SUCCEEDED(xrBeginSession(impl->session, &beginInfo));
      }
      else if (changed.state == XR_SESSION_STATE_STOPPING && impl->sessionRunning)
      {
        xrEndSession(impl->session);
        impl->sessionRunning = false;
      }
      else if (changed.state == XR_SESSION_STATE_EXITING || changed.state == XR_SESSION_STATE_LOSS_PENDING)
        impl->closeRequested = true;
    }
    event = { XR_TYPE_EVENT_DATA_BUFFER };
  }
#endif
}

//
// ランタイムと同期して現在フレームの view と HMD 姿勢を取得する.
//
bool GgOpenXR::beginFrame()
{
#if !defined(GG_ENABLE_OPENXR)
  // OpenXR が無効ビルドのときは、引数を使わずに false を返す。
  return false;
#else
  // pollEvents() をここで呼んでいるのは
  // 呼び出し側が明示的なイベント取得を忘れても
  // session state が更新されるようにするため
  pollEvents();

  // session が実行中でない、または前フレームが閉じられていない場合は失敗する。
  if (!impl->sessionRunning || impl->frameActive) return false;

  // xrWaitFrame() がランタイムとのフレーム周期同期と予測表示時刻の取得を行う。
  // OpenXR が要求する wait -> begin -> end の順序をこのクラス内で維持する。
  XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  if (XR_FAILED(xrWaitFrame(impl->session, &waitInfo, &impl->frameState))) return false;
  XrFrameBeginInfo beginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  if (XR_FAILED(xrBeginFrame(impl->session, &beginInfo))) return false;

  // ここから beginFrame() が成功した状態で、endFrame() までの間に
  // OpenXR の呼び出し順序を壊すと、ランタイムがクラッシュする可能性がある。
  impl->frameActive = true;

  // ランタイムが描画を要求していない場合は、描画せずに endFrame() で空レイヤーを提出する。
  impl->renderRequested = impl->frameState.shouldRender == XR_TRUE;

  // 非表示中でも begin 済みフレームは endFrame() で空レイヤーとして閉じる必要がある。
  if (!impl->renderRequested) return true;

  // VIEW 空間をアプリケーション基準空間へ locate し、左右眼の中間を推測せず
  // 予測表示時刻における HMD 中央の位置・方向を取得する。
  XrSpaceLocation headLocation{ XR_TYPE_SPACE_LOCATION };
  if (XR_SUCCEEDED(xrLocateSpace(impl->viewSpace, impl->space,
    impl->frameState.predictedDisplayTime, &headLocation)) &&
    (headLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
    (headLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
  {
    const auto& pose{ headLocation.pose };
    impl->headPose.position = { pose.position.x, pose.position.y, pose.position.z };
    impl->headPose.orientation = { pose.orientation.x, pose.orientation.y,
      pose.orientation.z, pose.orientation.w };
    impl->poseValid = true;
  }
  else
  {
    impl->poseValid = false;
  }

  // 同じ予測表示時刻を使って、描画と projection layer 提出に必要な各眼の
  // pose と非対称 FOV を取得する。
  XrViewLocateInfo locateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
  locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  locateInfo.displayTime = impl->frameState.predictedDisplayTime;
  locateInfo.space = impl->space;
  XrViewState viewState{ XR_TYPE_VIEW_STATE };
  uint32_t count{ static_cast<uint32_t>(impl->views.size()) };
  if (XR_FAILED(xrLocateViews(impl->session, &locateInfo, &viewState, count, &count, impl->views.data())))
    impl->renderRequested = false;

  // ランタイムが要求する描画を行う場合は、取得した pose と FOV を公開用構造体にコピーする。
  for (uint32_t i{}; i < count; ++i)
  {
    const auto& source{ impl->views[i] };
    auto& target{ impl->publicViews[i] };
    target.position = { source.pose.position.x, source.pose.position.y, source.pose.position.z };
    target.orientation = { source.pose.orientation.x, source.pose.orientation.y,
      source.pose.orientation.z, source.pose.orientation.w };
    target.fov = { source.fov.angleLeft, source.fov.angleRight,
      source.fov.angleDown, source.fov.angleUp };
  }

  // 成功
  return true;
#endif
}

//
// 指定した view の swapchain image を取得し、描画先 FBO に設定する.
//
bool GgOpenXR::beginView(std::size_t view)
{
#if !defined(GG_ENABLE_OPENXR)
  // OpenXR が無効ビルドのときは、引数を使わずに false を返す。
  return false;
#else
  // ランタイムが描画を要求していない場合は、描画せずに false を返す。
  if (!impl->frameActive || !impl->renderRequested || view >= impl->swapchains.size()) return false;

  // ランタイムから一枚の image を借り、GPU が書き込み可能になるまで待機する。
  // acquire に成功した時点から endView() まで、本クラスが返却責任を持つ。
  auto& swapchain{ impl->swapchains[view] };
  XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
  if (XR_FAILED(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.imageIndex))) return false;
  swapchain.acquired = true;

  // GPU が書き込み可能になるまで待機する。タイムアウトは無限にする。
  XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
  waitInfo.timeout = XR_INFINITE_DURATION;

  // acquire した image が GPU で書き込み可能になるまで待機する。
  if (XR_FAILED(xrWaitSwapchainImage(swapchain.handle, &waitInfo)))
  {
    endView(view);
    return false;
  }

  // 呼び出し側は以降、通常の OpenGL 描画を行うだけで swapchain へ描画できる。
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, swapchain.fbos[swapchain.imageIndex]);
  glViewport(0, 0, swapchain.width, swapchain.height);
  glClear(GL_COLOR_BUFFER_BIT);

  // 成功
  return true;
#endif
}

//
// OpenXR が利用可能かどうかを調べる.
//
bool GgOpenXR::available() const
{
#if defined( GG_ENABLE_OPENXR)
  return impl->instance != XR_NULL_HANDLE;
#else
  return false;
#endif
}

//
// OpenXR session が実行中かどうかを調べる.
//
bool GgOpenXR::running() const
{
#if defined( GG_ENABLE_OPENXR)
  return impl->sessionRunning;
#else
  return false;
#endif
}
//
// 指定した view の swapchain image をランタイムへ返す.
//
void GgOpenXR::endView(std::size_t view)
{
#if defined(GG_ENABLE_OPENXR)
  if (view >= impl->swapchains.size()) return;
  auto& swapchain{ impl->swapchains[view] };
  if (!swapchain.acquired) return;

  // image をランタイムへ返す前に OpenGL コマンドを GPU へ送る。
  // release 後はアプリケーションがこの image を使用してはならない。
  glFlush();

  XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
  xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
  swapchain.acquired = false;
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#else
  // OpenXR が無効ビルドのときは、引数を使わずに何もしない。
  (void)view;
#endif
}

//
// ランタイムへ projection layer を提出してフレームを閉じる.
//
bool GgOpenXR::endFrame()
{
#if !defined(GG_ENABLE_OPENXR)
  // OpenXR が無効ビルドのときは、引数を使わずに false を返す。
  return false;
#else
  // beginFrame() で取得した predictedDisplayTime と、各 view の pose/FOV と swapchain を
  // 一つの projection layer にまとめ、同じ predictedDisplayTime で提出する。
  if (!impl->frameActive) return false;
  std::vector<XrCompositionLayerProjectionView> projectionViews;
  XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
  std::vector<const XrCompositionLayerBaseHeader*> layers;
  if (impl->renderRequested)
  {
    // beginFrame() で locate した pose/FOV と各 view の swapchain を
    // 一つの projection layer にまとめ、同じ predictedDisplayTime で提出する。
    projectionViews.assign(impl->views.size(), { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });
    for (std::size_t i{}; i < projectionViews.size(); ++i)
    {
      projectionViews[i].pose = impl->views[i].pose;
      projectionViews[i].fov = impl->views[i].fov;
      projectionViews[i].subImage.swapchain = impl->swapchains[i].handle;
      projectionViews[i].subImage.imageRect.extent =
        { static_cast<int32_t>(impl->swapchains[i].width), static_cast<int32_t>(impl->swapchains[i].height) };
    }
    layer.space = impl->space;
    layer.viewCount = static_cast<uint32_t>(projectionViews.size());
    layer.views = projectionViews.data();
    layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer));
  }
  /* shouldRender == false の場合は layerCount == 0 の空フレームとして提出する。 */
  XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
  endInfo.displayTime = impl->frameState.predictedDisplayTime;
  endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  endInfo.layerCount = static_cast<uint32_t>(layers.size());
  endInfo.layers = layers.data();
  impl->frameActive = false;
  return XR_SUCCEEDED(xrEndFrame(impl->session, &endInfo));
#endif
}

//
// ランタイムがアプリケーションの終了を要求しているかどうかを返す.
//
bool GgOpenXR::shouldRender() const
{
  return impl->renderRequested;
}

//
// ランタイムがアプリケーションの終了を要求しているかどうかを返す.
//
bool GgOpenXR::shouldClose() const
{
  return impl->closeRequested;
}

//
// PRIMARY_STEREO 構成で列挙された view の数を返す.
//
std::size_t GgOpenXR::viewCount() const
{
  return impl->publicViews.size();
}

//
// 指定した view の情報を返す.
//
const GgOpenXR::View& GgOpenXR::getView(std::size_t view) const
{
  return impl->publicViews.at(view);
}

//
// HMD 中央の姿勢が有効かどうかを返す。
//
bool GgOpenXR::headPoseValid() const
{
  return impl->poseValid;
}

//
// HMD 中央の姿勢を返す。
//
const GgOpenXR::Pose& GgOpenXR::getHeadPose() const
{
  return impl->headPose;
}

//
// HMD 中央の姿勢を GgMatrix 形式で返す。
//
gg::GgMatrix GgOpenXR::getHeadPoseMatrix() const
{
  // Pose 自体は OpenXR や gg に依存しない公開形式で保持し、
  // 必要な場合だけアプリケーションで扱いやすい GgMatrix の T * R へ変換する。
  if (!impl->poseValid) return gg::ggIdentity();
  const auto& position{ impl->headPose.position };
  const auto& orientation{ impl->headPose.orientation };
  return gg::ggTranslate(position[0], position[1], position[2]) *
    gg::GgQuaternion{ orientation[0], orientation[1], orientation[2], orientation[3] }.getMatrix();
}
