///
/// 較正用フレームバッファオブジェクトクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date February 20, 2024
///
#include "Calibration.h"

// 構成ファイルの読み取り補助
#include "parseconfig.h"

// OpenCV
#include <opencv2/calib3d.hpp>

// 標準ライブラリ
#include <fstream>
#include <numeric>

// cv::Rodrigues() を使う
#define USE_RODRIGUES

//
// コンストラクタ
//
Calibration::Calibration(const std::string& dictionaryName, const std::array<float, 2>& length)
  : size{ 0, 0 }
  , repError{ 0.0 }
  , totalCorners{ 0 }
  , calibrationFlags
    {
      0
      //| cv::CALIB_USE_INTRINSIC_GUESS // cameraMatrix contains valid initial values of fx, fy, cx, cy that are optimized further.Otherwise, (cx, cy) is initially set to the image center(imageSize is used), and focal distances are computed in a least - squares fashion.Note, that if intrinsic parameters are known, there is no need to use this function just to estimate extrinsic parameters.Use solvePnP instead.
      //| cv::CALIB_FIX_PRINCIPAL_POINT // The principal point is not changed during the global optimization.It stays at the center or at a different location specified when CALIB_USE_INTRINSIC_GUESS is set too.
      //| cv::CALIB_FIX_ASPECT_RATIO    // The functions consider only fy as a free parameter.The ratio fx / fy stays the same as in the input cameraMatrix.When CALIB_USE_INTRINSIC_GUESS is not set, the actual input values of fx and fy are ignored, only their ratio is computed and used further.
      //| cv::CALIB_ZERO_TANGENT_DIST   // Tangential distortion coefficients(p1, p2) are set to zeros and stay zero.
      //| cv::CALIB_FIX_FOCAL_LENGTH    // The focal length is not changed during the global optimization if CALIB_USE_INTRINSIC_GUESS is set.
      //| cv::CALIB_FIX_K1              // , ..., CALIB_FIX_K6 The corresponding radial distortion coefficient is not changed during the optimization.If CALIB_USE_INTRINSIC_GUESS is set, the coefficient from the supplied distCoeffs matrix is used.Otherwise, it is set to 0.
      //| cv::CALIB_RATIONAL_MODEL      // Coefficients k4, k5, and k6 are enabled.To provide the backward compatibility, this extra flag should be explicitly specified to make the calibration function use the rational model and return 8 coefficients or more.
      //| cv::CALIB_THIN_PRISM_MODEL    //  Coefficients s1, s2, s3 and s4 are enabled.To provide the backward compatibility, this extra flag should be explicitly specified to make the calibration function use the thin prism model and return 12 coefficients or more.
      //| cv::CALIB_FIX_S1_S2_S3_S4     // The thin prism distortion coefficients are not changed during the optimization.If CALIB_USE_INTRINSIC_GUESS is set, the coefficient from the supplied distCoeffs matrix is used.Otherwise, it is set to 0.
      //| cv::CALIB_TILTED_MODEL        // Coefficients tauX and tauY are enabled.To provide the backward compatibility, this extra flag should be explicitly specified to make the calibration function use the tilted sensor model and return 14 coefficients.
      //| cv::CALIB_FIX_TAUX_TAUY       // The coefficients of the tilted sensor model are not changed during the optimization.If CALIB_USE_INTRINSIC_GUESS is set, the coefficient from the supplied distCoeffs matrix is used.Otherwise, it is set to 0.
    }
{
  // ArUco Marker の辞書を選択する
  setDictionary(dictionaryName, length);
}

//
//  デストラクタ
//
Calibration::~Calibration()
{
}

//
// ChArUco Board を作成する
//
void Calibration::createBoard(const std::array<float, 2>& length)
{
  // キャリブレーション用の ChArUco Board を作成する
  board = new cv::aruco::CharucoBoard(cv::Size{ 10, 7 },
    length[0] * 0.01f, length[1] * 0.01f, dictionary);

  // キャリブレーション用の ChArUco Board の検出器を作成する
  boardDetector = new cv::aruco::CharucoDetector(*board);

  // board は boardDetector->getBoard() で取り出すことができるが
  // 実行中に board を作り直すことがあるので cv::Ptr に持たせる

  // 較正結果を再利用しない
  calibrationFlags &= ~cv::CALIB_USE_INTRINSIC_GUESS;
}

//
//  ArUco Marker の辞書と検出器を設定する
//
void Calibration::setDictionary(const std::string& dictionaryName, const std::array<float, 2>& length)
{
  // ArUco Marker の辞書を検索する
  auto dictionaryItem{ dictionaryList.find(dictionaryName) };

  // ArUco Marker の辞書が見つからなかったら辞書リストの最初の辞書を使う
  if (dictionaryItem == dictionaryList.end()) dictionaryItem = dictionaryList.begin();

  // ArUco Marker の辞書を設定する
  dictionary = cv::aruco::getPredefinedDictionary(dictionaryItem->second);

  // ArUco Marker の検出器を作成する
  cv::aruco::DetectorParameters detectorParams = cv::aruco::DetectorParameters();
  detector = new cv::aruco::ArucoDetector(dictionary, detectorParams);

  // キャリブレーション用の ChArUco Board を作成する
  createBoard(length);
}

//
// ChArUco Board を描く
//
void Calibration::drawBoard(cv::Mat& boardImage, int width, int height)
{
  board->generateImage(cv::Size{ width, height }, boardImage, 10, 1);
}

//
// ChArUco Board を検出する
//
void Calibration::detectBoard(cv::Mat& image)
{
  // 画像のサイズを保存しておく
  size = image.size();

  // ChArUco Board のコーナーを検出する
  boardDetector->detectBoard(image, charucoCorners, charucoIds);

  // コーナーが見つからなかったら何もしない
  if (charucoCorners.empty()) return;

  // ChArUco Board のコーナーの位置を表示に描き込む
  cv::aruco::drawDetectedCornersCharuco(image, charucoCorners, charucoIds, cv::Scalar(0, 0, 255));
}

//
// ArUco Marker を検出する
//
void Calibration::detectMarkers(cv::Mat& image, float markerLength)
{
  // ArUco Marker のコーナーを検出する
  detector->detectMarkers(image, corners, ids, rejected);

  // コーナーが見つからなかったら何もしない
  if (corners.empty()) return;

  // 較正が完了していれば
  if (finished())
  {
    /// マーカの姿勢
    std::vector<cv::Vec3d> rvecs, tvecs;

    // 全てのマーカの姿勢を推定して
    cv::aruco::estimatePoseSingleMarkers(corners, markerLength,
      cameraMatrix, distCoeffs, rvecs, tvecs);

    // 個々のマーカーについて
    for (size_t i = 0; i < rvecs.size(); ++i)
    {
      // 座標軸を描く
      cv::drawFrameAxes(image, cameraMatrix, distCoeffs, rvecs[i], tvecs[i], markerLength);
    }
  }
  else
  {
    // ArUco Marker の位置に座標軸を描き込む
    cv::aruco::drawDetectedMarkers(image, corners, ids);
  }
}

//
// 標本を取得する
//
void Calibration::recordCorners()
{
  // ChArUco Board のコーナーが４つ以上見つかれば
  if (charucoCorners.size() >= 4)
  {
    // ChArUco Board のレイアウトと検出されたコーナーから
    // ChArUco Board 上の点と対応する画像上の点を求める
    board->matchImagePoints(charucoCorners, charucoIds, objectPoints, imagePoints);

    // ChArUco Board 上の点と対応する画像上の点が見つかれば
    if (!imagePoints.empty() && !objectPoints.empty())
    {
      // ChArUco Board のコーナーを記録する
      allCorners.push_back(charucoCorners);
      allIds.push_back(charucoIds);
      allImagePoints.push_back(imagePoints);
      allObjectPoints.push_back(objectPoints);

      // 記録したコーナーの数の合計を求める
      totalCorners += static_cast<int>(charucoCorners.size());
    }
  }
#if defined(DEBUG)
  std::cerr << "charucoCorners = " << charucoCorners.size()
    << ", allCorners = " << allCorners.size() << "\n";
#endif
}

//
// 標本と較正結果を破棄する
//
void Calibration::discardCorners()
{
  // 記録した標本を消去する
  allCorners.clear();
  allIds.clear();

  // 較正結果を消去する
  cameraMatrix.release();
  distCoeffs.release();

  // 検出したコーナー数の合計を 0 にする
  totalCorners = 0;

  // 較正の計算結果を再利用しない
  calibrationFlags &= ~cv::CALIB_USE_INTRINSIC_GUESS;
}

//
// 較正する
//
bool Calibration::calibrate()
{
  // 再投影誤差はとりあえず 0 にしておく
  repError = 0.0f;

  // コーナーを合計６つ以上検出できていれば
  if (allCorners.size() >= 6) try
  {
    // CALIB_USE_INTRINSIC_GUESS が設定されていない場合に、
    // fx と fy をしてしたアスペクト比に強制する
    if (calibrationFlags & cv::CALIB_FIX_ASPECT_RATIO)
    {
      const auto aspect{ static_cast<double>(size.width) / size.height };
      cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
      cameraMatrix.at<double>(0, 0) = aspect;
    }

    // ChArUco Board の姿勢
    std::vector<cv::Mat> boardRvecs, boardTvecs;

    // 取得した全てのコーナーからカメラパラメータを推定する
    repError = cv::calibrateCamera(allObjectPoints, allImagePoints, size,
      cameraMatrix, distCoeffs, boardRvecs, boardRvecs, cv::noArray(),
      cv::noArray(), cv::noArray(), calibrationFlags);
  }
  catch (const cv::Exception&)
  {
    // 較正に失敗した場合は計算結果を捨てる
    discardCorners();

    // 較正に失敗したことを報告する
    return false;
  }

  // 較正の計算結果を再利用する
  calibrationFlags |= cv::CALIB_USE_INTRINSIC_GUESS;

  // 較正に成功したことを報告する
  return true;
}

//
// ArUco Marker の３次元姿勢の変換行列を求める
//
void Calibration::getAllMarkerPoses(float markerLength, std::map<int, GgMatrix>& poses)
{
  /// マーカの姿勢
  std::vector<cv::Vec3d> rvecs, tvecs;

  // 全てのマーカの姿勢を推定して
  cv::aruco::estimatePoseSingleMarkers(corners, markerLength,
    cameraMatrix, distCoeffs, rvecs, tvecs);

  // 個々のマーカについて
  for (size_t i = 0; i < rvecs.size(); ++i)
  {
#if defined(USE_RODRIGUES)
    // 回転軸と回転角から回転の変換行列を求める
    cv::Mat_<double> r(3, 3);
    cv::Rodrigues(rvecs[i], r);

    // 各マーカの姿勢の変換行列を求める
    GgMatrix& m{ poses[ids[i]] };
    m[ 0] = static_cast<GLfloat>(r[0][0]);
    m[ 1] = static_cast<GLfloat>(r[1][0]);
    m[ 2] = static_cast<GLfloat>(r[2][0]);
    m[ 3] = 0.0f;
    m[ 4] = static_cast<GLfloat>(r[0][1]);
    m[ 5] = static_cast<GLfloat>(r[1][1]);
    m[ 6] = static_cast<GLfloat>(r[2][1]);
    m[ 7] = 0.0f;
    m[ 8] = static_cast<GLfloat>(r[0][2]);
    m[ 9] = static_cast<GLfloat>(r[1][2]);
    m[10] = static_cast<GLfloat>(r[2][2]);
    m[11] = 0.0f;
    m[12] = static_cast<GLfloat>(tvecs[i][0]);
    m[13] = static_cast<GLfloat>(tvecs[i][1]);
    m[14] = static_cast<GLfloat>(tvecs[i][2]);
    m[15] = 1.0f;
#else
    // 回転角を求める
    const auto r{ cv::norm(rvecs[i]) };

    // 回転軸ベクトルを正規化する
    rvecs[i] /= r;

    // 回転軸ベクトル
    const auto rx{ static_cast<GLfloat>(rvecs[i][0]) };
    const auto ry{ static_cast<GLfloat>(rvecs[i][1]) };
    const auto rz{ static_cast<GLfloat>(rvecs[i][2]) };

    // 平行移動量
    const auto tx{ static_cast<GLfloat>(tvecs[i][0]) };
    const auto ty{ static_cast<GLfloat>(tvecs[i][1]) };
    const auto tz{ static_cast<GLfloat>(tvecs[i][2]) };

    // 各マーカの姿勢を求める
    poses[ids[i]] = ggTranslate(tx, ty, tz).rotate(rx, ry, rz, static_cast<GLfloat>(r));
#endif
  }
}

//
// カメラパラメータの JSON オブジェクトから数値の配列を取得する
//
static bool getMatrix(const picojson::object& object,
  const std::string& key, cv::Mat& mat, int cols, int rows)
{
  // key に一致するオブジェクトを探す
  const auto&& value{ object.find(key) };

  // オブジェクトが無いか配列でなかったら戻る
  if (value == object.end() || !value->second.is<picojson::array>()) return false;

  // 配列を取り出す
  const auto& array{ value->second.get<picojson::array>() };

  // 配列の要素数とデータの格納先の行列の要素数が一致していなければ戻る
  if (static_cast<size_t>(cols) * rows != array.size()) return false;

  // カメラ行列の要素を確保する
  mat = cv::Mat::zeros(rows, cols, CV_64F);

  // 配列の要素について
  for (size_t i = 0; i < array.size(); ++i)
  {
    // 行列の要素の位置
    const auto x{ static_cast<int>(i % cols) };
    const auto y{ static_cast<int>(i / cols) };

    // 要素が数値なら格納する
    if (array[i].is<double>()) mat.at<double>(y, x) = array[i].get<double>();
  }

  return true;
}

//
// カメラパラメータの JSON オブジェクトから数値の配列を取得する
//
static void setMatrix(picojson::object& object,
  const std::string& key, const cv::Mat& mat)
{
  // picojson の配列
  picojson::array array;

  // 配列の要素について
  for (size_t i = 0; i < mat.total(); ++i)
  {
    // 行列の要素の位置
    const auto x{ static_cast<int>(i % mat.cols) };
    const auto y{ static_cast<int>(i / mat.cols) };

    // 要素を picojson::array に追加する
    array.emplace_back(picojson::value(mat.at<double>(y, x)));
  }

  // オブジェクトに追加する
  object.emplace(key, array);
}

//
// ファイルからキャリブレーションパラメータを読み込む
//
bool Calibration::loadParameters(const std::string& filename)
{
  // パラメータファイルの読み込み
  std::ifstream json{ filename };
  if (!json) return false;

  // JSON の読み込み
  picojson::value value;
  json >> value;
  json.close();

  // 構成内容の取り出し
  const auto& object{ value.get<picojson::object>() };

  // オブジェクトが空だったらエラー
  if (object.empty()) return false;

  // カメラ行列
  if (!getMatrix(object, "camera matrix", cameraMatrix, 3, 3)) return false;

  // 歪み定数
  if (!getMatrix(object, "distortion", distCoeffs, 5, 1)) return false;

  // 再投影誤差
  getValue(object, "error", repError);

  // 較正の計算結果を再利用しない
  calibrationFlags &= ~cv::CALIB_USE_INTRINSIC_GUESS;

  // キャリブレーションパラメータの読み込み
  return true;
}

//
// キャリブレーションパラメータをファイルに保存する
//
bool Calibration::saveParameters(const std::string& filename) const
{
  // 設定値を保存する
  std::ofstream config{ filename };
  if (!config) return false;

  // オブジェクト
  picojson::object object;

  // カメラ行列
  setMatrix(object, "camera matrix", cameraMatrix);

  // 歪み定数
  setMatrix(object, "distortion", distCoeffs);

  // 再投影誤差
  setValue(object, "error", repError);

  // 構成をシリアライズして保存
  picojson::value v{ object };
  config << v.serialize(true);
  config.close();

  // キャリブレーションパラメータの書き込み
  return true;
}

// ArUco Marker 辞書のリスト
const std::map<const std::string, const cv::aruco::PredefinedDictionaryType> Calibration::dictionaryList
{
  { "DICT_4X4_50", cv::aruco::DICT_4X4_50 },
  { "DICT_4X4_100", cv::aruco::DICT_4X4_100 },
  { "DICT_4X4_250", cv::aruco::DICT_4X4_250 },
  { "DICT_4X4_1000", cv::aruco::DICT_4X4_1000 },
  { "DICT_5X5_50", cv::aruco::DICT_5X5_50 },
  { "DICT_5X5_100", cv::aruco::DICT_5X5_100 },
  { "DICT_5X5_250", cv::aruco::DICT_5X5_250 },
  { "DICT_5X5_1000", cv::aruco::DICT_5X5_1000 },
  { "DICT_6X6_50", cv::aruco::DICT_6X6_50 },
  { "DICT_6X6_100", cv::aruco::DICT_6X6_100 },
  { "DICT_6X6_250", cv::aruco::DICT_6X6_250 },
  { "DICT_6X6_1000", cv::aruco::DICT_6X6_1000 },
  { "DICT_7X7_50", cv::aruco::DICT_7X7_50 },
  { "DICT_7X7_100", cv::aruco::DICT_7X7_100 },
  { "DICT_7X7_250", cv::aruco::DICT_7X7_250 },
  { "DICT_7X7_1000", cv::aruco::DICT_7X7_1000 },
  { "DICT_ARUCO_ORIGINAL", cv::aruco::DICT_ARUCO_ORIGINAL },
  { "DICT_APRILTAG_16h5", cv::aruco::DICT_APRILTAG_16h5 },
  { "DICT_APRILTAG_25h9", cv::aruco::DICT_APRILTAG_25h9 },
  { "DICT_APRILTAG_36h10", cv::aruco::DICT_APRILTAG_36h10 },
  { "DICT_APRILTAG_36h11", cv::aruco::DICT_APRILTAG_36h11 }
};
