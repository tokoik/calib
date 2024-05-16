#pragma once

///
/// 較正クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// 補助プログラム
#include "gg.h"
using namespace gg;

// ArUco Maker
#include <opencv2/aruco.hpp>

// ChArUco Board
#include <opencv2/aruco/charuco.hpp>

// 標準ライブラリ
#include <map>

///
/// 較正クラス
///
class Calibration
{
  /// ArUco Marker 辞書
  cv::aruco::Dictionary dictionary;

  /// ArUco Marker 検出器
  cv::Ptr<cv::aruco::ArucoDetector> detector;

  /// ChArUco Board
  cv::Ptr<cv::aruco::CharucoBoard> board;

  /// ChArUco Board 検出器
  cv::Ptr<cv::aruco::CharucoDetector> boardDetector;

  /// ArUco Marker の検出結果
  std::vector<std::vector<cv::Point2f>> corners, rejected;
  std::vector<int> ids;

  /// ChArUco Board の検出結果
  std::vector<cv::Point2f> charucoCorners;
  std::vector<int> charucoIds;
  std::vector<cv::Point3f> objectPoints;
  std::vector<cv::Point2f> imagePoints;

  /// ChArUco Board の検出結果の記録
  std::vector<std::vector<cv::Point2f>> allCorners;
  std::vector<std::vector<int>> allIds;
  std::vector<std::vector<cv::Point2f>> allImagePoints;
  std::vector<std::vector<cv::Point3f>> allObjectPoints;

  /// カメラの内部パラメータ行列
  cv::Mat cameraMatrix;

  /// カメラの歪み係数
  cv::Mat distCoeffs;

  /// 入力画像のサイズ
  cv::Size size;

  /// 再投影誤差
  double repError;

  /// 取得したコーナーの合計
  int totalCorners;

  /// 較正の設定
  int calibrationFlags;

public:

  ///
  /// 較正オブジェクトのコンストラクタ
  ///
  /// @param dictionaryName ArUco Marker の辞書名
  /// @param length ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さ (単位 cm)
  ///
  Calibration(const std::string& dictionaryName, const std::array<float, 2>& length);

  ///
  /// コピーコンストラクタは使用しない
  /// 
  /// @param calibration コピー元
  ///
  Calibration(const Calibration& calibration) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Calibration();

  ///
  /// 代入演算子は使用しない
  /// 
  /// @param calibration 代入元
  ///
  Calibration& operator=(const Calibration& calibration) = delete;

  ///
  /// ChArUco Board を作成する
  ///
  /// @param length ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さ (単位 cm)
  ///
  void createBoard(const std::array<float, 2>& length);

  ///
  /// ArUco Marker の辞書と検出器を設定する
  ///
  /// @param dictionaryName ArUco Marker の辞書名
  /// @param length ChArUco Board のマス目の一辺の長さと ArUco Marker の一辺の長さ (単位 cm)
  ///
  void setDictionary(const std::string& dictionaryName, const std::array<float, 2>& length);

  ///
  /// ChArUco Board を描く
  ///
  /// @param boardImage ChArUco Board を描き込む画像
  /// @param width ChArUco Board の横の画素数
  /// @param height ChArUco Board の縦の画素数
  ///
  void drawBoard(cv::Mat& boardImage, int width, int height);

  ///
  /// ChArUco Board を検出する
  ///
  /// @param image ChArUco Board を検出する画像
  /// 
  void detectBoard(cv::Mat& image);

  ///
  /// ArUco Marker を検出する
  ///
  /// @param image ArUco Marker を検出する画像
  /// @param markerLength ArUco Marker の一辺の長さ (単位 cm)
  /// 
  void detectMarkers(cv::Mat& image, float markerLength);

  ///
  /// 標本を取得する
  ///
  void recordCorners();

  ///
  /// 取得した標本と較正結果を破棄する
  ///
  void discardCorners();

  ///
  /// 較正する
  ///
  /// @return 較正に成功したら true
  ///
  bool calibrate();

  ///
  /// 検出数を取得する
  ///
  /// @return 検出されたコーナーの数
  ///
  auto getCornersCount() const
  {
    return static_cast<int>(corners.size());
  }

  ///
  /// 標本数を取得する
  ///
  /// @return 保存された標本の数
  ///
  auto getSampleCount() const
  {
    return static_cast<int>(allCorners.size());
  }

  ///
  /// 検出数の合計を取得する
  ///
  /// @return 検出されたコーナーの数
  ///
  auto getTotalCount() const
  {
    return totalCorners;
  }

  ///
  /// カメラ行列を取り出す
  ///
  /// @return カメラ行列
  /// 
  const auto& getCameraMatrix() const
  {
    return cameraMatrix;
  }

  ///
  /// 歪パラメータを取り出す
  ///
  /// @return 歪パラメータ
  ///
  const auto& getDistortionCoefficients() const
  {
    return distCoeffs;
  }

  ///
  /// 再投影誤差を得る
  ///
  /// @return 再投影誤差
  ///
  auto getReprojectionError() const
  {
    return repError;
  }

  ///
  /// 較正が完了したかどうかを調べる
  ///
  /// @return 較正が完了していたら true
  /// 
  auto finished()
  {
    return cameraMatrix.total() == 9 && distCoeffs.total() == 5;
  }

  ///
  /// ArUco Marker の３次元姿勢の変換行列を求める
  ///
  /// @param markerLength ArUco Marker の一辺の長さ (単位 cm)
  /// @param poses 推定した ArUco Marker の３次元姿勢
  ///
  /// @note
  /// これはキャリブレーション終了後に単独マーカの位置推定に用いる
  ///
  void getAllMarkerPoses(float markerLength, std::map<int, GgMatrix>& poses);

  ///
  /// ファイルからキャリブレーションパラメータを読み込む
  ///
  /// @param filename 保存するファイル名
  /// @return パラメータの読み込みに成功したら true
  ///
  bool loadParameters(const std::string& filename);

  ///
  /// キャリブレーションパラメータをファイルに保存する
  ///
  /// @param filename 保存するファイル名
  /// @return パラメータの書き込みに成功したら true
  ///
  bool saveParameters(const std::string& filename) const;

  /// ArUco Marker 辞書のリスト
  static const std::map<const std::string, const cv::aruco::PredefinedDictionaryType> dictionaryList;
};
