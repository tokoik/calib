# calib OpenXR バックエンド

## 概要

calib の OpenXR 対応は、GLFW ウィンドウを管理する `GgApp::Window` とは独立した
`GgOpenXR` クラスとして実装されています。

`GgOpenXR` が担当する処理は次のとおりです。

- OpenXR instance、system、session の作成と破棄
- STAGE または LOCAL 基準空間と VIEW 空間の作成
- OpenXR のイベントおよび session state の処理
- view と HMD 中央姿勢の取得
- OpenGL swapchain image と FBO の管理
- `xrWaitFrame`、`xrBeginFrame`、`xrEndFrame` によるフレーム同期

画像の生成、カメラ補正、ヘッドトラッキングの画像処理への反映はアプリケーション側の
責務です。`GgOpenXR` は `Menu`、`Expand`、`Framebuffer` などの calib 固有クラスを
参照しません。

## ビルド

OpenXR 対応は既定では無効です。有効にすると、CMake は OpenXR SDK 1.1.61を
`libs/OpenXR-SDK-release-1.1.61` に取得し、static loaderをビルドします。

```powershell
cmake -S . -B build -DGG_ENABLE_OPENXR=ON
cmake --build build --config Debug --target calib -- /m
```

OpenXRを組み込まない通常ビルドは次のように構成します。

```powershell
cmake -S . -B build
```

現在のOpenGL graphics bindingはWindows用です。OpenXRを無効にしたビルドでは、
`GgOpenXR` の公開APIは維持されますが、`initialize()` は `false` を返します。

## 実行

OpenXRを使用する場合は `--openxr` を指定します。

```powershell
build\Debug\calib.exe --openxr
```

OpenXRランタイムやHMDを利用できない場合、メッセージを標準エラーへ出力し、
デスクトップ表示だけで処理を継続します。`--openxr` を指定しなければOpenXRの
初期化は行いません。

## 基本的な利用方法

`GgOpenXR` は有効なOpenGLコンテキストが作成された後に初期化します。

```cpp
GgApp::Window window{ title, width, height };
GgOpenXR openxr;

if (!openxr.initialize(window.getNativeHandle(), title))
{
  // 通常のデスクトップ表示を継続できる
}
```

基本的なフレーム処理は次の順序です。

```cpp
openxr.pollEvents();

if (openxr.beginFrame())
{
  if (openxr.shouldRender())
  {
    for (std::size_t view{}; view < openxr.viewCount(); ++view)
    {
      const auto& currentView{ openxr.getView(view) };
      prepareImageForView(currentView);

      if (!openxr.beginView(view)) continue;
      drawPreparedImage(currentView.width, currentView.height);
      openxr.endView(view);
    }
  }

  openxr.endFrame();
}
```

呼び出し順序に関する注意点:

1. `beginFrame()` が `true` を返した場合は、描画しないフレームでも `endFrame()` を呼びます。
2. `beginView()` が `true` を返したviewだけ描画し、対応する `endView()` を呼びます。
3. `beginView()` はOpenXRのswapchain FBOを `GL_DRAW_FRAMEBUFFER` に設定します。
4. `endView()` は描画コマンドをflushし、swapchain imageをランタイムへ返します。
5. `endView()` 後はデフォルトのdraw framebufferへ戻ります。

## view情報

`getView()` は、予測表示時刻における各viewの情報を返します。

```cpp
const GgOpenXR::View& view = openxr.getView(index);
```

`View` の内容:

| メンバ | 内容 |
|---|---|
| `position` | 基準空間における眼の位置、単位はメートル |
| `orientation` | 眼の向きを表す `(x, y, z, w)` 四元数 |
| `fov` | left、right、down、upの順の視野角、単位はラジアン |
| `width` | swapchain imageの推奨幅 |
| `height` | swapchain imageの推奨高さ |

`View` は `beginFrame()` 内の `xrLocateViews()` が成功したときに更新されます。

## HMD中央姿勢

HMD中央の姿勢は、OpenXRのVIEW空間をSTAGEまたはLOCAL基準空間に対して
`xrLocateSpace()` した結果です。予測表示時刻は、そのフレームの
`XrFrameState::predictedDisplayTime` と一致します。

まず有効性を確認してください。

```cpp
if (openxr.headPoseValid())
{
  const auto& pose = openxr.getHeadPose();
}
```

`Pose::position` の単位はメートル、`Pose::orientation` は `(x, y, z, w)` の
四元数です。

`GgMatrix` が必要な場合は次のメソッドを使用できます。

```cpp
const gg::GgMatrix headPose = openxr.getHeadPoseMatrix();
```

返される行列は次の変換です。

```text
headPose = Translation(position) * Rotation(orientation)
```

これは「HMDのローカル座標からOpenXR基準空間への姿勢」です。ワールド座標から
HMDのview座標へ変換する行列が必要な場合は、この行列の逆変換を使用してください。
姿勢がまだ有効でない場合、`getHeadPoseMatrix()` は単位行列を返します。

姿勢は `beginFrame()` の呼び出し中に更新されます。このため、描画前に
`beginFrame()` を呼び、同じフレーム内で姿勢を使用してください。

## セッションと終了要求

- `available()` はOpenXR instanceが作成済みかを返します。
- `running()` はsessionが実行状態かを返します。
- `shouldRender()` は現在のフレームを描画すべきかを返します。
- `shouldClose()` はランタイムから終了またはinstance lossが通知されたことを表します。
- `shutdown()` はOpenXR資源を破棄します。デストラクタからも自動的に呼ばれます。

アプリケーション終了へ連動させる例:

```cpp
openxr.pollEvents();
if (openxr.shouldClose()) window.setClose(GLFW_TRUE);
```

## 現在のcalibへの統合

現在のcalibは、デスクトップ表示とは別に、`beginFrame()`で得た各viewの
`orientation`を回転行列へ変換し、`Menu::setup(aspect, viewPose)`でメニューの補正姿勢と
合成して入力画像を再展開します。その展開結果を、`beginView()`が設定した各viewの
swapchain FBOへ`Framebuffer::draw()`で描画します。

入力映像は単眼画像として扱うため、現在はviewの`position`を使用せず、両眼の位置差に
よる視差は付けません。`GgOpenXR`自身は画像の回転や投影を行わず、view情報の画像処理への
反映はcalib側が担当します。HMD中央姿勢を別の処理で必要とする場合は、
`getHeadPose()`または`getHeadPoseMatrix()`を使用できます。

## エラー時の方針

初期化中に必要なOpenXR機能を利用できない場合、`initialize()` は `false` を返し、
作成済み資源を破棄します。フレーム処理中の一時的な失敗では、そのフレームまたは
viewの描画を中止します。アプリケーションはデスクトップ表示とGLFWのイベント処理を
継続できます。
