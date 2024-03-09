﻿#pragma once

///
/// 構成データクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date March 6, 2024
///

// アプリケーションの設定
#include "GgApp.h"

// キャプチャデバイスの構成
#include "Preference.h"

// 動画のキャプチャ
#include "CamCv.h"

///
/// 表示関連の設定データ
///
struct Settings
{
  /// 展開に使うメッシュのサンプル数
  int samples;

  /// キャプチャデバイスの姿勢のオイラー角
  std::array<float, 3> euler;

  /// キャプチャデバイスの姿勢のデフォルト値
  static constexpr decltype(euler) defaultEuler{ 0.0f, 0.0f, 0.0f };

  /// 描画時の焦点距離
  float focal;

  /// 描画時の焦点距離のデフォルト値
  static constexpr decltype(focal) defaultFocal{ 50.0f };

  /// 描画時の焦点距離の範囲
  std::array<float, 2> focalRange;

  /// 描画時の焦点距離の範囲のデフォルト値
  static constexpr decltype(focalRange) defaultFocalRange{ 10.0f, 200.0f };

  /// 使用中の ArUco Marker 辞書名
  std::string dictionary;

  ///
  /// コンストラクタ
  ///
  Settings(const std::string& dictionary)
    : samples{ 57600 }
    , euler{ defaultEuler }
    , focal{ defaultFocal }
    , focalRange{ defaultFocalRange }
    , dictionary{ dictionary }
  {}

  ///
  /// サンプル数からメッシュのメッシュの分割数を得る
  ///
  /// @param メッシュ分割の基準にする解像度
  /// 
  std::array<GLsizei, 2> getMeshResolution(std::array<GLsizei, 2>& size) const;

  ///
  /// 正規化デバイス座標系における焦点距離を求める
  ///
  auto getFocal() const
  {
    return focal / defaultFocal;
  }

  ///
  /// 正規化デバイス座標系における (スクリーンの高さ / 2)÷カメラの焦点距離を求める
  ///
  /// @note ディオプトリー, 焦点距離の逆数
  ///
  float getDiopter() const
  {
    return defaultFocal / focal;
  }
};

///
/// 構成データクラス
///
class Config
{
  /// プライベートメンバは Menu クラスで設定する
  friend class Menu;

  /// ウィンドウタイトル
  std::string title;

  /// ウィンドウの幅と高さ
  std::array<GLsizei, 2> windowSize;

  /// ウィンドウの背景色
  std::array<GLfloat, 4> background;

  /// メニューフォント
  std::string menuFont;

  /// メニューフォントサイズ
  float menuFontSize;

  /// 表示関連の設定
  Settings settings;

  /// バックエンドのリスト
  static const std::map<cv::VideoCaptureAPIs, const char*> backendList;

  /// コーデックのリスト
  static const std::vector<const char*> codecList;

  /// キャプチャデバイスのリスト
  static std::map <cv::VideoCaptureAPIs, std::vector<std::string>> deviceList;

  /// ArUco Marker 辞書のリスト
  static const std::map<const std::string, const cv::aruco::PredefinedDictionaryType> dictionaryList;

  /// 初期表示の画像ファイル名
  static std::string initialImage;

  /// すべての構成のリスト
  std::vector<Preference> preferenceList;

public:

  ///
  /// コンストラクタ
  ///
  /// @param filename 構成データの初期化に使う JSON 形式のファイル名
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
  auto& getInitialImage() const
  {
    return initialImage;
  }

  ///
  /// キャプチャデバイスのリストを取り出す
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @return キャプチャデバイスのリスト
  ///
  const auto& getDeviceList(cv::VideoCaptureAPIs api) const
  {
    return deviceList.at(api);
  }

  ///
  /// キャプチャデバイスの数を調べる
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @return キャプチャデバイスの数
  ///
  auto getDeviceCount(cv::VideoCaptureAPIs api) const
  {
    return static_cast<int>(deviceList.at(api).size());
  }

  ///
  /// キャプチャデバイスの名前を調べる
  ///
  /// @param api 使用しているバックエンドの API 名
  /// @param number キャプチャデバイスの番号
  /// @return キャプチャデバイスの名前
  ///
  const auto& getDeviceName(cv::VideoCaptureAPIs api, int number) const
  {
    static const std::string empty{};
    const auto& list{ deviceList.at(api) };
    return list.empty() ? empty : list[number];
  }

  ///
  /// ArUco Maker 辞書の種類を取得する
  ///
  /// @name ArUco Maker 辞書の名前
  /// @return ArUco Maker 辞書
  ///
  cv::aruco::PredefinedDictionaryType getPredfinedDictionaryType(const std::string& name) const
  {
    return dictionaryList.at(name);
  }
};