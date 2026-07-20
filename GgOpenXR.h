#pragma once

///
/// OpenXR/OpenGL バックエンドの定義.
///
/// @file
/// @author Kohe Tokoi
/// @date July 20, 2026
///

// 標準ライブラリ
#include <array>
#include <cstddef>
#include <memory>
#include <string>

// GLFW ウィンドウの識別子を隠すための前方宣言
struct GLFWwindow;

// ゲームグラフィックス特論の宿題用補助プログラムの変換行列
namespace gg { class GgMatrix; }

///
/// OpenXR と OpenGL の相互運用を管理するバックエンド.
///
/// @note
/// GLFW ウィンドウ、OpenGL コンテキスト、OpenXR ランタイムの橋渡しだけを担当し、
/// アプリケーション固有の画像生成や姿勢補正は行わない。
/// `beginFrame()` と `endFrame()`、および成功した `beginView()` と `endView()` は
/// 必ず対にして呼び出すこと。
///
class GgOpenXR
{
  // OpenXR 型を公開ヘッダから隠し、無効ビルドでも同じ API を保つ実装クラス.
  class Impl;

  // OpenXR の状態と所有資源.
  std::unique_ptr<Impl> impl;

public:

  ///
  /// OpenXR の一つの view に対応する描画情報.
  ///
  struct View
  {
    /// 基準空間における眼の位置 [m].
    std::array<float, 3> position{};

    /// 基準空間における眼の向き (x, y, z, w).
    std::array<float, 4> orientation{ 0.0f, 0.0f, 0.0f, 1.0f };

    /// 視野角 (left, right, down, up) [rad].
    std::array<float, 4> fov{};

    /// OpenXR ランタイムが推奨する描画幅 [pixel].
    int width{ 0 };

    /// OpenXR ランタイムが推奨する描画高さ [pixel].
    int height{ 0 };
  };

  ///
  /// 基準空間における HMD 中央の姿勢.
  ///
  struct Pose
  {
    /// 基準空間における HMD 中央の位置 [m].
    std::array<float, 3> position{};

    /// 基準空間における HMD 中央の向き (x, y, z, w).
    std::array<float, 4> orientation{ 0.0f, 0.0f, 0.0f, 1.0f };
  };

  ///
  /// コンストラクタ.
  ///
  GgOpenXR();

  ///
  /// デストラクタ. 保持している OpenXR 資源を破棄する.
  ///
  ~GgOpenXR();

  ///
  /// コピーコンストラクタは使用しない.
  ///
  GgOpenXR(const GgOpenXR&) = delete;

  ///
  /// コピー代入演算子は使用しない.
  ///
  GgOpenXR& operator=(const GgOpenXR&) = delete;

  ///
  /// OpenXR と、現在の GLFW/OpenGL コンテキストを初期化する.
  ///
  /// @param window OpenGL コンテキストを所有する GLFW ウィンドウ.
  /// @param applicationName OpenXR ランタイムへ通知するアプリケーション名.
  /// @return 初期化に成功したら true.
  ///
  bool initialize(GLFWwindow* window, const std::string& applicationName);

  ///
  /// OpenXR 資源を依存関係の逆順で破棄する.
  ///
  void shutdown();

  ///
  /// OpenXR イベントを取得し、セッション状態を更新する.
  ///
  void pollEvents();

  ///
  /// ランタイムと同期し、現在フレームの view と HMD 姿勢を取得する.
  ///
  /// @return `xrBeginFrame()` まで成功し、`endFrame()` が必要なら true.
  ///
  bool beginFrame();

  ///
  /// 指定した view の swapchain image を取得し、描画先 FBO に設定する.
  ///
  /// @param view 描画する view の番号.
  /// @return 描画可能なら true. true の場合は `endView(view)` が必要.
  ///
  bool beginView(std::size_t view);

  ///
  /// 指定した view の描画を完了し、swapchain image をランタイムへ返す.
  ///
  /// @param view `beginView()` に渡した view の番号.
  ///
  void endView(std::size_t view);

  ///
  /// 現在フレームの projection layer をランタイムへ提出する.
  ///
  /// @return `xrEndFrame()` に成功したら true.
  ///
  bool endFrame();

  ///
  /// OpenXR が利用可能かどうかを調べる.
  ///
  /// @return OpenXR instance が作成されていれば true.
  ///
  bool available() const;

  ///
  /// OpenXR session が実行中かどうかを調べる.
  ///
  /// @return OpenXR session が実行中なら true.
  ///
  bool running() const;

  ///
  /// 現在フレームを描画するようランタイムが要求しているかどうかを調べる.
  ///
  /// @return 現在フレームを描画するようランタイムが要求していれば true.
  ///
  bool shouldRender() const;

  ///
  /// ランタイムからアプリケーションの終了が要求されているかどうかを調べる.
  ///
  /// @return ランタイムからアプリケーションの終了を要求されていれば true.
  ///
  bool shouldClose() const;

  ///
  /// PRIMARY_STEREO 構成で列挙された view の数を取得する.
  ///
  /// @return PRIMARY_STEREO 構成で列挙された view の数.
  ///
  std::size_t viewCount() const;

  ///
  /// 指定した view の現在情報を取得する.
  ///
  /// @param view view の番号.
  /// @return `beginFrame()` で取得した view 情報.
  /// @throws std::out_of_range view が範囲外の場合.
  ///
  const View& getView(std::size_t view) const;

  ///
  /// 現在フレームの HMD 中央姿勢が有効かどうかを調べる.
  ///
  /// @return 現在フレームの HMD 中央姿勢が有効なら true.
  ///
  bool headPoseValid() const;

  ///
  /// 現在フレームの HMD 中央姿勢を取得する.
  ///
  /// @return `beginFrame()` で取得した HMD 中央姿勢.
  ///
  const Pose& getHeadPose() const;

  ///
  /// HMD のローカル座標から基準空間への姿勢行列を取得する.
  ///
  /// @return `Translation(position) * Rotation(orientation)`.
  /// 姿勢が無効な場合は単位行列.
  ///
  gg::GgMatrix getHeadPoseMatrix() const;
};
