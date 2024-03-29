#pragma once

///
/// 較正クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

// テクスチャ
#include "Texture.h"

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
  cv::Mat charucoCorners, charucoIds;
  //std::vector<cv::Point2f> currentCharucoCorners;
  //std::vector<int> currentCharucoIds;
  //std::vector<cv::Point3f> currentObjectPoints;
  //std::vector<cv::Point2f> currentImagePoints;

  /// ChArUco Board の検出結果の記録
  std::vector<cv::Mat> allCorners, allIds;
  //std::vector<std::vector<cv::Point2f>> allCharucoCorners;
  //std::vector<std::vector<int>> allCharucoIds;
  //std::vector<std::vector<cv::Point2f>> allImagePoints;
  //std::vector<std::vector<cv::Point3f>> allObjectPoints;

  /// カメラの内部パラメータ行列
  cv::Mat cameraMatrix;

  /// カメラの歪み係数
  cv::Mat distCoeffs;

public:

  ///
  /// コンストラクタ
  ///
  /// @param texture 較正に用いるフレームを格納したテクスチャ
  /// @param dictionaryName ArUco Marker の辞書名
  ///
  Calibration(const std::string& dictionaryName);

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
  /// @param length ChArUco Board の正方形の一辺の長さとマーカの一辺の長さ (cm)
  ///
  void createBoard(std::array<float, 2>& length);

  ///
  /// ArUco Marker の辞書と検出器を設定する
  ///
  /// @param dictionaryName ArUco Marker の辞書名
  ///
  void setDictionary(const std::string& dictionaryName);

  ///
  /// ChArUco Board を描く
  ///
  /// @param boardImage ChArUco Board の画像の格納先
  /// @param width ChArUco Board の横の画素数
  /// @param height ChArUco Board の縦の画素数
  ///
  void drawBoard(cv::Mat& boardImage, int width, int height);

  ///
  /// ArUco Marker を検出する
  ///
  /// @param ArUco Marker を検出するフレームを格納したテクスチャ
  /// @param detectBoard ChArUco Board を検出するなら true
  /// @return ArUco Marker が見つかれば true
  /// 
  bool detect(Texture& texture, bool detectBoard);

  ///
  /// 検出数を取得する
  ///
  /// @return 検出された角の数
  ///
  const auto getCornersCount() const
  {
    return corners.size();
  }

  ///
  /// 標本を取得する
  ///
  void extractSample();

  ///
  /// 標本数を取得する
  ///
  /// @return 保存された標本の数
  ///
  const auto getSampleCount() const
  {
    return allCorners.size();
  }

  ///
  /// 標本を破棄する
  ///
  void discardSamples();

  ///
  /// 較正する
  ///
  /// @param size ArUco Marker のサイズ
  /// @return 再投影誤差
  ///
  double calibrate(const cv::Size& size);

  ///
  /// 較正が完了したかどうかを調べる
  ///
  /// @return 較正が完了していたら true
  /// 
  auto finished()
  {
    return cameraMatrix.total() == 9;
  }

  ///
  /// 較正結果を使って ArUco Marker の座標軸を描く
  ///
  /// @param texture ArUco Marker の座標軸を描き込むフレームを格納したテクスチャ
  /// @param poses ArUco Marker の３次元姿勢
  ///
  void drawFrameAxes(Texture& texture, std::map<int, GgMatrix>& poses);

  /// ArUco Marker 辞書のリスト
  static const std::map<const std::string, const cv::aruco::PredefinedDictionaryType> dictionaryList;
};
