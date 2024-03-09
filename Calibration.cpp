///
/// 較正用フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Calibration.h"

// OpenCV
#include <opencv2/calib3d.hpp>

// 標準ライブラリ
#include <iostream>
#include <numeric>

//
// コンストラクタ
//
Calibration::Calibration(const Texture& texture)
  : texture{ texture }
  , frameCount{ 0 }
  , totalFrames{ 0 }
{
}

//
//  デストラクタ
//
Calibration::~Calibration()
{
}

// ArUco Marker の辞書と検出器を設定する
void Calibration::setDictionary(cv::aruco::PredefinedDictionaryType dictionaryId)
{
  // ArUco Marker の辞書を設定する
  dictionary = cv::aruco::getPredefinedDictionary(dictionaryId);

  // ArUco Marker の検出器を作成する
  cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
  detector = new cv::aruco::ArucoDetector(dictionary, detectorParams);

  // キャリブレーション用のボードを作成する
  board = new cv::aruco::CharucoBoard(cv::Size(10, 7), 0.04f, 0.02f, dictionary);

  // キャリブレーション用のボードの検出器を作成する
  boardDetector = new cv::aruco::CharucoDetector(*board);
}

//
// ChArUco ボードを描く
//
void Calibration::drawBoard(cv::Mat& boardImage, int width, int height)
{
  boardDetector->getBoard().generateImage(cv::Size(width, height), boardImage, 10, 1);
}


//
// 較正開始
//
void Calibration::startCaribration(int frames)
{
  // 較正に使うフレーム数を記録しておく
  totalFrames = frames;

  // 較正に使う残りのフレーム数を初期化する
  frameCount = frames;

  // 較正データを消去する
  allCorners.clear();
  allIds.clear();
  cameraMatrix.release();
  distCoeffs.release();
}

//
// 較正停止
//
void Calibration::stopCalibration()
{
  // 較正に使う残りのフレーム数を 0 にする
  frameCount = 0;
}

//
// マーカーを検出する
//
bool Calibration::detect(cv::Mat& image,
  std::vector<std::vector<cv::Point2f>>& corners,
  std::vector<int>& ids,
  std::vector<std::vector<cv::Point2f>>& rejected
)
{
  // マーカーを検出する
  detector->detectMarkers(image, corners, ids, rejected);
  detector->refineDetectedMarkers(image, *board, corners, ids, rejected);

  // マーカーが見つかれば true を返す
  return ids.size() > 0;
}

//
// 較正する
//
bool Calibration::calibrate(cv::Mat& image,
  const std::vector<std::vector<cv::Point2f>>& corners,
  const std::vector<int>& ids,
  const std::vector<std::vector<cv::Point2f>>& rejected,
  std::map<int, GgMatrix>& poses)
{
  // マーカーを使ってチェスボード画像の交点を検出する
  cv::Mat charucoCorners, charucoIds;
  cv::aruco::interpolateCornersCharuco(corners, ids, image, board, charucoCorners, charucoIds);

  //std::vector<cv::Point2f> currentCharucoCorners;
  //std::vector<int> currentCharucoIds;
  //std::vector<cv::Point3f> currentObjectPoints;
  //std::vector<cv::Point2f> currentImagePoints;
  //boardDetector->detectBoard(buffer, currentCharucoCorners, currentCharucoIds);
  //boardDetector->getBoard().matchImagePoints(
  //  currentCharucoCorners, currentCharucoIds,
  //  currentObjectPoints, currentImagePoints
  //);

    //チェスボード交点位置を表示に描き込む
  cv::aruco::drawDetectedCornersCharuco(image, charucoCorners, charucoIds, cv::Scalar(0, 0, 255));

  // チェスボードの交点が４つ以上見つかれば
  if (charucoCorners.total() >= 4)
  {
    // チェスボードの交点を記録する
    allCorners.push_back(charucoCorners);
    allIds.push_back(charucoIds);

    // 較正枚数分のフレームを取得後に交点を合計６つ以上検出できていれば
    if (--frameCount == 0 && allCorners.size() >= 6) try
    {
      // ボードの姿勢
      std::vector<cv::Mat> boardRvecs, boardTvecs;

      // 取得した全ての交点からカメラパラメータを推定する
      const auto repError 
      {
        cv::aruco::calibrateCameraCharuco(allCorners, allIds, board, texture.getSize(),
        cameraMatrix, distCoeffs, boardRvecs, boardRvecs)
      };

      //int calibrationFlags = 0
      //  //| cv::CALIB_USE_INTRINSIC_GUESS // cameraMatrix contains valid initial values of fx, fy, cx, cy that are optimized further.Otherwise, (cx, cy) is initially set to the image center(imageSize is used), and focal distances are computed in a least - squares fashion.Note, that if intrinsic parameters are known, there is no need to use this function just to estimate extrinsic parameters.Use solvePnP instead.
      //  //| cv::CALIB_FIX_PRINCIPAL_POINT // The principal point is not changed during the global optimization.It stays at the center or at a different location specified when CALIB_USE_INTRINSIC_GUESS is set too.
      //  //| cv::CALIB_FIX_ASPECT_RATIO    // The functions consider only fy as a free parameter.The ratio fx / fy stays the same as in the input cameraMatrix.When CALIB_USE_INTRINSIC_GUESS is not set, the actual input values of fx and fy are ignored, only their ratio is computed and used further.
      //  //| cv::CALIB_ZERO_TANGENT_DIST   // Tangential distortion coefficients(p1, p2) are set to zeros and stay zero.
      //  //| cv::CALIB_FIX_FOCAL_LENGTH    // The focal length is not changed during the global optimization if CALIB_USE_INTRINSIC_GUESS is set.
      //  //| cv::CALIB_FIX_K1              // , ..., CALIB_FIX_K6 The corresponding radial distortion coefficient is not changed during the optimization.If CALIB_USE_INTRINSIC_GUESS is set, the coefficient from the supplied distCoeffs matrix is used.Otherwise, it is set to 0.
      //  //| cv::CALIB_RATIONAL_MODEL      // Coefficients k4, k5, and k6 are enabled.To provide the backward compatibility, this extra flag should be explicitly specified to make the calibration function use the rational model and return 8 coefficients or more.
      //  //| cv::CALIB_THIN_PRISM_MODEL    //  Coefficients s1, s2, s3 and s4 are enabled.To provide the backward compatibility, this extra flag should be explicitly specified to make the calibration function use the thin prism model and return 12 coefficients or more.
      //  //| cv::CALIB_FIX_S1_S2_S3_S4     // The thin prism distortion coefficients are not changed during the optimization.If CALIB_USE_INTRINSIC_GUESS is set, the coefficient from the supplied distCoeffs matrix is used.Otherwise, it is set to 0.
      //  //| cv::CALIB_TILTED_MODEL        // Coefficients tauX and tauY are enabled.To provide the backward compatibility, this extra flag should be explicitly specified to make the calibration function use the tilted sensor model and return 14 coefficients.
      //  //| cv::CALIB_FIX_TAUX_TAUY       // The coefficients of the tilted sensor model are not changed during the optimization.If CALIB_USE_INTRINSIC_GUESS is set, the coefficient from the supplied distCoeffs matrix is used.Otherwise, it is set to 0.
      //  ;
      //auto repError
      //{ cv::calibrateCamera(
      //  allObjectPoints, allImagePoints, getSize(),
      //  cameraMatrix, distCoeffs, boardRvecs, boardRvecs, cv::noArray(),
      //  cv::noArray(), cv::noArray(), calibrationFlags)
      //};
#if defined(_DEBUG)
      std::cerr << "reprojection error = " << repError << "\n";
#endif
    }
    catch (const cv::Exception)
    {
      // 収束しなかった場合はデータを捨てる
      allCorners.clear();
      allIds.clear();
      cameraMatrix.release();
      distCoeffs.release();

      // 最初からデータを撮り直す
      frameCount = totalFrames;
    }

    // キャリブレーションが完了してカメラ行列ができていれば
    else if (cameraMatrix.total() > 0)
    {
      // 全てのマーカーの姿勢を推定して
      std::vector<cv::Vec3d> rvecs, tvecs;
      cv::aruco::estimatePoseSingleMarkers(corners, 0.05f, cameraMatrix, distCoeffs, rvecs, tvecs);

      // 個々のマーカーについて
      for (decltype(rvecs.size()) n = 0; n < rvecs.size(); ++n)
      {
        // 座標軸を描く
        cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvecs[n], tvecs[n], 0.1f);

        // 回転角を求める
        const auto r{ cv::norm(rvecs[n]) };

        // 回転軸ベクトルを正規化する
        rvecs[n] /= r;

        // 回転軸ベクトル
        const auto rx{ static_cast<GLfloat>(rvecs[n][0]) };
        const auto ry{ static_cast<GLfloat>(rvecs[n][1]) };
        const auto rz{ static_cast<GLfloat>(rvecs[n][2]) };

        // 平行移動量
        const auto tx{ static_cast<GLfloat>(tvecs[n][0]) };
        const auto ty{ static_cast<GLfloat>(tvecs[n][1]) };
        const auto tz{ static_cast<GLfloat>(tvecs[n][2]) };

        // ６面の各面ごとにマーカーの姿勢を求める
        poses[ids[n]] = ggTranslate(tx, ty, tz).rotate(rx, ry, rz, static_cast<GLfloat>(r));
      }
    }
  }

  // ピクセルバッファオブジェクトのマップを解除する
  texture.unmapBuffer();

  // ピクセルバッファオブジェクトの内容をテクスチャに転送する
  texture.drawPixels();

  // ボードの読み込みが完了したら true を返す
  return frameCount == 0;
}
