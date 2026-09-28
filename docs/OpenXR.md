# calib OpenXR バックエンド

## 概要

calib の OpenXR 対応は、アプリケーション基盤クラス `GgApp` に統合された `GgApp::OpenXR` クラスとして実装されています。

`GgApp::OpenXR` が担当する処理は次のとおりです。

- OpenXR instance、system、session の作成と破棄
- STAGE または LOCAL 基準空間の作成
- OpenXR のイベントおよび session state の処理（フォーカス状態やセッション状態の遷移）
- HMD の視野角（FOV）および視点姿勢（Pose）の取得
- コントローラ入力（Aim/Grip Pose、トリガー、グリップ、サムスティック、ボタン、ハプティクス）の追跡
- OpenGL swapchain image と FBO / Depth buffer の管理（sRGB フォーマット優先）
- `xrWaitFrame`、`xrBeginFrame`、`xrEndFrame` によるフレーム同期
- PC 画面へのミラー表示および安全な swapchain release 管理

画像の展開、歪み補正、ヘッドトラッキングの画像処理への反映はアプリケーション側（`calib.cpp` / `Menu` / `Framebuffer`）の責務です。

## ビルド

OpenXR 対応は既定では無効（`OFF`）です。有効にするには、CMake の構成時に `-DGG_ENABLE_OPENXR=ON` を指定します。
CMake は自動的に OpenXR SDK 1.1.61 を `libs/OpenXR-SDK-release-1.1.61` にダウンロードし、静的ローダーライブラリ `openxr_loader` をビルド・リンクします。

```powershell
# OpenXR を有効化して CMake を構成
cmake -S . -B build -DGG_ENABLE_OPENXR=ON

# Release 構成のビルド
cmake --build build --config Release

# Debug 構成のビルド
cmake --build build --config Debug
```

OpenXR を組み込まない通常ビルドは次のとおりです。

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## 実行

OpenXR を使用する場合は、コマンドライン引数に `--openxr` を指定します。

```powershell
build\Release\calib.exe --openxr
```

OpenXR ランタイムや HMD を利用できない場合、標準エラーに警告を出力し、デスクトップ表示だけで処理を継続します。`--openxr` を指定しなければ OpenXR の初期化は行われません。

## 基本的な利用方法

`GgApp::OpenXR` は、GLFW ウィンドウおよび OpenGL コンテキストが作成された後に初期化します。

```cpp
#if defined(GG_USE_OPENXR)
GgApp::OpenXR* openxr{ nullptr };
if (useOpenXr)
{
  try
  {
    openxr = &GgApp::OpenXR::initialize(window, XR_REFERENCE_SPACE_TYPE_STAGE, config.getTitle().c_str());
  }
  catch (const std::exception& e)
  {
    std::cerr << "OpenXR: " << e.what() << '\n';
  }
}
#endif
```

描画ループでの基本的なフレーム処理は次の順序です。

```cpp
#if defined(GG_USE_OPENXR)
if (openxr && openxr->isRunning() && openxr->begin())
{
  const auto viewCount{ openxr->getViewCount() };
  for (uint32_t view = 0; view < viewCount; ++view)
  {
    // 各眼の向き（orientation）を考慮して画像を準備
    const auto& pose{ openxr->getPose(view) };
    const auto viewPose{ gg::ggQuaternionMatrix(gg::GgQuaternion{
      pose.orientation.x, pose.orientation.y,
      pose.orientation.z, pose.orientation.w }) };
    const auto&& xrSize{ menu.setup(openxr->getAspect(view), viewPose) };
    framebuffer.update(xrSize, frame);

    // 描画対象の swapchain を選択して描画
    openxr->select(view);
    framebuffer.draw(openxr->getWidth(view), openxr->getHeight(view));
    openxr->commit(view);
  }

  // フレームをランタイムへ送信（mirror=false でデスクトップ上書きをスキップ）
  openxr->submit(false);
}
#endif
```

### 呼び出し順序とリソース管理のポイント

1. `begin()`:
   - 内部で `pollEvents()` を実行してセッション状態を更新し、`xrWaitFrame` と `xrBeginFrame` を呼び出します。
   - 描画すべきフレームでなければ内部で自動的に `endFrame()` を呼び出し、`false` を返します。`true` が返ったフレームのみ描画処理に進みます。
2. `select(eye)`:
   - スワップチェーン画像を取得（`xrAcquireSwapchainImage` / `xrWaitSwapchainImage`）し、対応する FBO にアタッチしてビューポートを設定します。
3. `commit(eye)`:
   - 当該の眼の描画を完了し、FBO のバインドを解除します。この時点ではスワップチェーン画像を解放しません。
4. `submit(mirror)`:
   - 必要に応じてウィンドウへのミラー blit（`blitMirror()`）を行い、取得していたすべてのスワップチェーン画像を安全に解放（`xrReleaseSwapchainImage`）した後、`endFrame()`（`xrEndFrame`）を呼び出してコンポジタへフレームを引き渡します。

## calib への統合

現在の calib は、デスクトップ表示とは別に、`openxr->getPose(view)` で得た各眼のクォータニオン回転行列を `Menu::setup(aspect, viewPose)` へ渡し、メニューの姿勢補正と合成して入力画像を再展開します。その展開結果を、`openxr->select(view)` が設定した swapchain FBO へ `Framebuffer::draw()` でレンダリングします。

入力映像は単眼パノラマ・全天球画像として扱うため、現在は眼の `position` による両眼視差は付与せず、頭部の回転姿勢に追従した自然な視野追従を実現しています。
