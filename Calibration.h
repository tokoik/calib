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

// テクスチャ
#include "Texture.h"

// ArUco Maker
#include <opencv2/aruco.hpp>

// ChArUco
#include <opencv2/aruco/charuco.hpp>

// 標準ライブラリ
#include <map>

///
/// 較正クラス
///
class Calibration
{
  /// 較正に用いるピクセルバッファオブジェクトを備えたテクスチャ
  const Texture& texture;

  /// ArUco Marker 辞書
  cv::aruco::Dictionary dictionary;

  /// ArUco Marker 検出器
  cv::Ptr<cv::aruco::ArucoDetector> detector;

  /// ChArUco ボード
  cv::Ptr<cv::aruco::CharucoBoard> board;

  /// ChArUco ボード検出器
  cv::Ptr<cv::aruco::CharucoDetector> boardDetector;

  /// ChArUco ボードの全ての交点
  std::vector<cv::Mat> allCorners;

  /// ChArUco ボードの全ての交点の番号
  std::vector<cv::Mat> allIds;

  //std::vector<std::vector<cv::Point2f>> allCharucoCorners;
  //std::vector<std::vector<int>> allCharucoIds;
  //std::vector<std::vector<cv::Point2f>> allImagePoints;
  //std::vector<std::vector<cv::Point3f>> allObjectPoints;

  /// カメラの内部パラメータ行列
  cv::Mat cameraMatrix;

  /// カメラの歪み係数
  cv::Mat distCoeffs;

  /// 較正に使う残りフレーム数
  int frameCount;

  /// 較正に使うフレーム数
  int totalFrames;

public:

  ///
  /// コンストラクタ
  ///
  /// @param texture 較正に用いるフレームを格納したテクスチャ
  ///
  Calibration(const Texture& texture);

  ///
  /// コピーコンストラクタは使用しない
  ///
  Calibration(const Calibration& calibration) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Calibration();

  ///
  /// 代入演算子は使用しない
  ///
  Calibration& operator=(const Calibration& calibration) = delete;

  ///
  /// ArUco Marker の辞書と検出器を設定する
  ///
  /// @param dictionaryId ArUco Marker の辞書名
  ///
  void setDictionary(cv::aruco::PredefinedDictionaryType dictionaryId);

  ///
  /// ChArUco ボードを描く
  ///
  /// @param boardImage ChArUco ボードの画像の格納先
  /// @param width ChArUco ボードの横の画素数
  /// @param height ChAruCo ボードの縦の画素数
  ///
  void drawBoard(cv::Mat& boardImage, int width, int height);

  ///
  /// 較正開始
  ///
  /// @param frames 較正に用いるフレーム数
  ///
  void startCaribration(int frames);

  ///
  /// 較正停止
  ///
  void stopCalibration();

  ///
  /// マーカーを検出する
  ///
  /// @param image マーカーを検出するフレームの画像
  /// @param corners 検出されたマーカーのコーナーの２次元位置
  /// @param ids 検出されたマーカーの id
  /// @param poses id が検出されなかったマーカーのコーナーの２次元位置
  /// @return マーカーが見つかれば true
  /// 
  bool detect(cv::Mat& image,
    std::vector<std::vector<cv::Point2f>>& corners,
    std::vector<int>& ids,
    std::vector<std::vector<cv::Point2f>>& rejected
  );

  ///
  /// 較正する
  ///
  /// @param image マーカーを検出するフレームの画像
  /// @param corners 検出されたマーカーのコーナーの２次元位置
  /// @param ids 検出されたマーカーの id
  /// @param rejected id が検出されなかったマーカーのコーナーの２次元位置
  /// @param poses マーカーの３次元姿勢
  ///
  bool calibrate(cv::Mat& image,
    const std::vector<std::vector<cv::Point2f>>& corners,
    const std::vector<int>& ids,
    const std::vector<std::vector<cv::Point2f>>& rejected,
    std::map<int, GgMatrix>& poses
  );
};
