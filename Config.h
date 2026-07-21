#pragma once

///
/// 構成データクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// キャプチャデバイスの構成
#include "Preference.h"

///
/// 表示関連の設定データ
///
struct Settings
{
  /// 展開に用いるメッシュのサンプル数
  int samples{ 57600 };

  /// キャプチャデバイスの姿勢のオイラー角
  std::array<float, 3> euler{ defaultEuler };

  /// キャプチャデバイスの姿勢のデフォルト値
  static constexpr decltype(euler) defaultEuler{ 0.0f, 0.0f, 0.0f };

  /// 描画時の焦点距離
  float focal{ defaultFocal };

  /// 展開時の焦点距離のデフォルト値
  static constexpr decltype(focal) defaultFocal{ 50.0f };

  /// 描画時の焦点距離の範囲
  std::array<float, 2> focalRange{ defaultFocalRange };

  /// 描画時の焦点距離の範囲のデフォルト値
  static constexpr decltype(focalRange) defaultFocalRange{ 10.0f, 200.0f };

  /// 使用中の ArUco Marker 辞書名
  std::string dictionaryName{ "DICT_4X4_50" };

  /// 検出する ChArUco Board のマス目一辺の長さと ArUco Marker の一辺の長さ (単位 cm)
  std::array<float, 2> checkerLength{ 4.0f, 2.0f };

  /// 検出する ArUco Marker の一辺の長さ (単位 cm)
  float markerLength{ 5.0f };

  ///
  /// 正規化デバイス座標系における焦点距離を求める
  ///
  auto getFocal() const
  {
    // 投影面の対角線長は 35mm (17.5mm × 2) とする
    return focal / 17.5f;
  }
};

///
/// 構成データクラス
///
class Config
{
  /// ウィンドウタイトル
  std::string title{ PROJECT_NAME };

  /// ウィンドウの幅と高さ
  std::array<GLsizei, 2> windowSize{ 1280, 720 };

  /// ウィンドウの背景色
  std::array<GLfloat, 4> background{ 0.2f, 0.3f, 0.4f, 1.0f };

  /// 表示関連の設定
  Settings settings{};

  /// メニューフォント
  std::string menuFont{ "Mplus1-Regular.ttf" };

  /// メニューフォントサイズ
  float menuFontSize{ 20.0f };

  /// 初期表示の画像ファイル名
  static std::string initialImage;

  /// すべての構成のリスト
  std::vector<Preference> preferenceList;

  /// OpenGL コンテキスト作成後の初期化が完了していれば true
  bool initialized{ false };

#if defined(_WIN32)
  /// キャプチャデバイスのリスト
  const std::vector<std::string>& deviceList;
#endif

public:

  ///
  /// コンストラクタ
  ///
  /// @param filename 構成データの初期化に用いる JSON 形式のファイル名
  ///
  Config(const std::string& filename);

  ///
  /// デストラクタ
  ///
  virtual ~Config();

  ///
  /// 構成データを初期化する
  ///
  /// @note OpenGL のレンダリングコンテキスト作成後に実行する
  ///
  void initialize();

  ///
  /// 構成ファイルを読み込む
  ///
  /// @param filename 構成データの JSON 形式のファイル名
  /// @return 構成ファイルの読み込みに成功したら true
  /// @details
  /// ファイル内容を一時領域へ読み込んで検証し、成功した場合だけ現在の構成を置き換える。
  /// initialize() 実行後の再読み込みでは、新しい投影方式のシェーダーも構築する。
  ///
  bool load(const pathString& filename);

  ///
  /// 構成ファイルを保存する
  ///
  /// @param filename 構成データの JSON 形式のファイル名
  /// @return 構成ファイルの保存に成功したら true
  ///
  bool save(const pathString& filename) const;

  ///
  /// 現在の表示・較正設定を得る
  ///
  /// @return 設定データへの読み取り専用参照
  ///
  const Settings& getSettings() const
  {
    return settings;
  }

  ///
  /// 保存対象の表示・較正設定を更新する
  ///
  /// @param value 新しい設定データ
  ///
  void setSettings(const Settings& value)
  {
    settings = value;
  }

  ///
  /// 利用可能な投影方式の一覧を得る
  ///
  /// @return 投影方式一覧への読み取り専用参照
  ///
  const auto& getPreferences() const
  {
    return preferenceList;
  }

  ///
  /// 背景色を得る
  ///
  /// @return RGBA 形式の背景色への読み取り専用参照
  ///
  const auto& getBackground() const
  {
    return background;
  }

  ///
  /// メニュー用フォントのファイル名を得る
  ///
  /// @return フォントファイル名への読み取り専用参照
  ///
  const auto& getMenuFont() const
  {
    return menuFont;
  }

  ///
  /// メニュー用フォントサイズを得る
  ///
  /// @return フォントサイズ
  ///
  auto getMenuFontSize() const
  {
    return menuFontSize;
  }

  ///
  /// ウィンドウタイトルの文字列を得る
  ///
  /// @return ウィンドウのタイトル文字列
  ///
  const auto& getTitle() const
  {
    return title;
  }

  ///
  /// ウィンドウの横幅を得る
  ///
  /// @return ウィンドウの横幅
  ///
  auto getWidth() const
  {
    return windowSize[0];
  }

  ///
  /// ウィンドウの高さを得る
  ///
  /// @return ウィンドウの高さ
  ///
  auto getHeight() const
  {
    return windowSize[1];
  }

  ///
  /// 初期画像ファイル名を得る
  ///
  /// @return 初期画像ファイル名
  ///
  const auto& getInitialImage() const
  {
    return initialImage;
  }

  ///
  /// 使用中の ArUco Marker の辞書名を得る
  ///
  /// @return 使用中の ArUco Marker の辞書名
  ///
  const auto& getDictionaryName() const
  {
    return settings.dictionaryName;
  }

  ///
  /// 検出する ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さを得る
  ///
  /// @return 検出する ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さ
  ///
  const auto& getCheckerLength() const
  {
    return settings.checkerLength;
  }

  ///
  /// 検出する ArUco Marker の一辺の長さを得る
  ///
  /// @return 検出する ArUco Marker の一辺の長さ
  ///
  auto getMarkerLength() const
  {
    return settings.markerLength;
  }

#if defined(_WIN32)
  ///
  /// キャプチャデバイスのリストを取り出す
  ///
  /// @return キャプチャデバイスのリスト
  ///
  const auto& getDeviceList() const
  {
    return deviceList;
  }

  ///
  /// キャプチャデバイスの名前を調べる
  ///
  /// @param number キャプチャデバイスの番号
  /// @return キャプチャデバイスの名前
  ///
  const auto& getDeviceName(int number) const
  {
    static const std::string empty{};
    return deviceList.empty() ? empty : deviceList[number];
  }
#endif
};
