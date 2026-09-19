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
#pragma warning(disable:4819)
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

// 標準ライブラリ
#include <fstream>
#include <numeric>

// cv::Rodrigues() を使う
#define USE_RODRIGUES


//
// コンストラクタ
//
Calibration::Calibration(const std::string& dictionaryName,
  const std::array<int, 2>& checkerSize, const std::array<float, 2>& checkerLength)
{
  // ArUco Marker の辞書を選択する
  setDictionary(dictionaryName, checkerSize, checkerLength);
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
void Calibration::createBoard(const std::array<int, 2>& checkerSize,
  const std::array<float, 2>& checkerLength)
{
  // キャリブレーション用の ChArUco Board を作成する
  board = new cv::aruco::CharucoBoard(cv::Size{ checkerSize[0], checkerSize[1] },
    checkerLength[0] * 0.01f, checkerLength[1] * 0.01f, dictionary);

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
void Calibration::setDictionary(const std::string& dictionaryName,
  const std::array<int, 2>& checkerSize, const std::array<float, 2>& checkerLength)
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
  createBoard(checkerSize, checkerLength);
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

  // 4 チャンネル画像の場合は一時的に3チャンネル画像を作成する
  cv::Mat tempImage;
  const auto isFourChannels{ image.channels() == 4 };
  if (isFourChannels)
  {
    cv::cvtColor(image, tempImage, cv::COLOR_BGRA2BGR);
  }
  else
  {
    tempImage = image;
  }

  // ChArUco Board のコーナーを検出する
  boardDetector->detectBoard(tempImage, charucoCorners, charucoIds);

  // コーナーが見つからなかったら何もしない
  if (charucoCorners.empty()) return;

  // ChArUco Board のコーナーの位置を表示に描き込む
  cv::aruco::drawDetectedCornersCharuco(tempImage, charucoCorners, charucoIds, cv::Scalar(0, 0, 255));

  // 4 チャンネル画像の場合は結果を書き戻す
  if (isFourChannels)
  {
    cv::cvtColor(tempImage, image, cv::COLOR_BGR2BGRA);
  }
}

//
// ArUco Marker を検出する
//
void Calibration::detectMarkers(cv::Mat& image, float markerLength)
{
  // 4 チャンネル画像の場合は一時的に3チャンネル画像を作成する
  cv::Mat tempImage;
  const auto isFourChannels{ image.channels() == 4 };
  if (isFourChannels)
  {
    cv::cvtColor(image, tempImage, cv::COLOR_BGRA2BGR);
  }
  else
  {
    tempImage = image;
  }

  // ArUco Marker のコーナーを検出する
  detector->detectMarkers(tempImage, corners, ids, rejected);

  // コーナーが見つからなければ戻る
  if (corners.empty()) return;

  // キャリブレーションが完了していれば
  if (finished())
  {
    // 各マーカに対応する３次元空間の点
    const float markerCenter{ markerLength * 0.5f };
    std::vector<cv::Point3f> markerObjPoints;
    markerObjPoints.push_back(cv::Point3f(-markerCenter, markerCenter, 0.0f));
    markerObjPoints.push_back(cv::Point3f(markerCenter, markerCenter, 0.0f));
    markerObjPoints.push_back(cv::Point3f(markerCenter, -markerCenter, 0.0f));
    markerObjPoints.push_back(cv::Point3f(-markerCenter, -markerCenter, 0.0f));

    // 個々のマーカーについて
    for (size_t i = 0; i < corners.size(); ++i)
    {
      // マーカーのコーナー検出位置から3次元姿勢（回転・平行移動）を推定する
      cv::Vec3d rvec, tvec;
      cv::solvePnP(markerObjPoints, corners[i], cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);

      // 座標軸を描く
      cv::drawFrameAxes(tempImage, cameraMatrix, distCoeffs, rvec, tvec, markerLength);
    }
  }
  else
  {
    // ArUco Marker の場所に矩形と番号を描き込む
    cv::aruco::drawDetectedMarkers(tempImage, corners, ids);
  }

  // 4 チャンネル画像の場合は結果を書き戻す
  if (isFourChannels)
  {
    cv::cvtColor(tempImage, image, cv::COLOR_BGR2BGRA);
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

      // 記録したショットの幾何特徴を保存する (多様性チェック用)
      lastRecordedFeatures = computeFeatures(charucoCorners);
      hasRecordedShot = true;
      stableDuration = 0.0f;
      isCurrentlyStable = false;
    }
  }
#if defined(_DEBUG)
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

  // 自動キャプチャ関連の状態をリセットする
  hasRecordedShot = false;
  stableDuration = 0.0f;
  isCurrentlyStable = false;
  currentMotion = 999.0f;
  prevCharucoCorners.clear();
  prevCharucoIds.clear();
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

    // ChArUco Board の姿勢 (使わないので捨ててしまう)
    std::vector<cv::Mat> boardRvecs, boardTvecs;

    // 取得した全てのコーナーからカメラパラメータを推定する
    repError = cv::calibrateCamera(allObjectPoints, allImagePoints, size,
      cameraMatrix, distCoeffs, boardRvecs, boardTvecs, cv::noArray(),
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
// 回転ベクトルから姿勢の変換行列を求める
//
GgMatrix Calibration::RvecTvecToPose(const cv::Vec3d& rvec, const cv::Vec3d& tvec)
{
#if defined(USE_RODRIGUES)
  // 回転軸と回転角から回転の変換行列を求める
  cv::Mat_<double> r(3, 3);
  cv::Rodrigues(rvec, r);

  // 姿勢の変換行列
  return GgMatrix
  {
    static_cast<GLfloat>(r[0][0]),
    static_cast<GLfloat>(r[1][0]),
    static_cast<GLfloat>(r[2][0]),
    0.0f,
    static_cast<GLfloat>(r[0][1]),
    static_cast<GLfloat>(r[1][1]),
    static_cast<GLfloat>(r[2][1]),
    0.0f,
    static_cast<GLfloat>(r[0][2]),
    static_cast<GLfloat>(r[1][2]),
    static_cast<GLfloat>(r[2][2]),
    0.0f,
    static_cast<GLfloat>(tvec[0]),
    static_cast<GLfloat>(tvec[1]),
    static_cast<GLfloat>(tvec[2]),
    1.0f
  };
#else
  // 回転角
  const auto d{ cv::norm(rvec) };

  // 回転軸ベクトル
  const auto rx{ static_cast<GLfloat>(rvec[0] / d) };
  const auto ry{ static_cast<GLfloat>(rvec[1] / d) };
  const auto rz{ static_cast<GLfloat>(rvec[2] / d) };

  // 平行移動量
  const auto tx{ static_cast<GLfloat>(tvec[0]) };
  const auto ty{ static_cast<GLfloat>(tvec[1]) };
  const auto tz{ static_cast<GLfloat>(tvec[2]) };

  // 姿勢の変換行列
  return ggTranslate(tx, ty, tz).rotate(rx, ry, rz, static_cast<GLfloat>(d));
#endif
}

//
// ArUco Marker の３次元姿勢の変換行列を求める
//
void Calibration::getAllMarkerPoses(float markerLength, std::map<int, GgMatrix>& poses)
{
  // 各マーカに対応する３次元空間の点
  const float markerCenter = markerLength * 0.5f;
  std::vector<cv::Point3f> markerObjPoints;
  markerObjPoints.push_back(cv::Point3f(-markerCenter, markerCenter, 0.0f));
  markerObjPoints.push_back(cv::Point3f(markerCenter, markerCenter, 0.0f));
  markerObjPoints.push_back(cv::Point3f(markerCenter, -markerCenter, 0.0f));
  markerObjPoints.push_back(cv::Point3f(-markerCenter, -markerCenter, 0.0f));

  // 個々のマーカについて
  for (size_t i = 0; i < corners.size(); ++i)
  {
    // マーカーのコーナー検出位置から3次元姿勢（回転・平行移動）を推定する
    cv::Vec3d rvec, tvec;
    cv::solvePnP(markerObjPoints, corners[i], cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE);

    // 各マーカの姿勢の変換行列を求める
    poses[ids[i]] = RvecTvecToPose(rvec, tvec);
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
  std::ifstream json{ Utf8ToTChar(filename) };
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
  std::ofstream config{ Utf8ToTChar(filename) };
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

//
// コーナー群から幾何特徴を計算する
//
// 【目的】
//   ChArUco ボードの画像内での配置状態（位置、見かけの大きさ、傾き）を
//   統計的モーメントを用いてロバストに要約し、特徴量として抽出する。
//   直前に記録した標本の特徴量と比較することで、サンプルの多様性（位置移動、
//   カメラとの距離変化、回転・傾き）を判定するために利用する。
//
Calibration::BoardPoseFeatures Calibration::computeFeatures(const std::vector<cv::Point2f>& corners)
{
  BoardPoseFeatures features;

  // 4点未満では面としての幾何特徴（慣性楕円など）を安定して算出できないため除外
  if (corners.size() >= 4)
  {
    // コーナー点群の統計モーメントを算出
    //   m00: 点の総数 (質量)
    //   m10, m01: 1次モーメント (座標総和)
    //   mu20, mu02, mu11: 重心まわりの2次中心モーメント (分散・共分散)
    cv::Moments m = cv::moments(corners);
    if (m.m00 > 0.0)
    {
      // 1. 重心位置 (Centroid): 画像内でのボードの中心座標
      features.centroid = cv::Point2f(static_cast<float>(m.m10 / m.m00), static_cast<float>(m.m01 / m.m00));

      const double mu20 = m.mu20 / m.m00;
      const double mu02 = m.mu02 / m.m00;
      const double mu11 = m.mu11 / m.m00;

      // 2. 慣性半径 (Spread / Scale): コーナー点群の重心からの平均的な広がり
      //    カメラとボード間の距離（見かけの大きさ）を表す指標となる。
      //    単純な外接矩形や一部コーナーのオクルージョンに比べ外れ値に強い。
      features.spread = static_cast<float>(std::sqrt(std::max(0.0, mu20 + mu02)));

      // 3. 主軸角度 (Orientation Angle): 慣性主軸の傾き角度 (度数法, -90° ～ +90°)
      //    画像平面内におけるボードの回転・傾き状態を表す指標となる。
      features.angleDeg = static_cast<float>(0.5 * std::atan2(2.0 * mu11, mu20 - mu02) * 180.0 / CV_PI);
    }
  }
  return features;
}

//
// モーション状態（静止判定）を更新する
//
// 【目的】
//   直前フレームと同一の ChArUco コーナー ID を照合してフレーム間の変位量を追跡し、
//   作業者の手振れやボード移動に伴う「モーションブラー（ブレ）」のない
//   安定した静止状態が一定時間継続しているかを判定する。
//
void Calibration::updateMotion(float deltaTime, float motionThresholdPx, float minStableTime, int minCorners)
{
  // 検出コーナー数が最低必要数未満、または直前フレームのデータが存在しない場合
  if (charucoCorners.size() < static_cast<size_t>(minCorners) || prevCharucoCorners.empty())
  {
    // 移動中または未検出とみなし、変位量を大きく設定して静止時間をリセット
    currentMotion = 999.0f;
    stableDuration = 0.0f;
    isCurrentlyStable = false;
    prevCharucoCorners = charucoCorners;
    prevCharucoIds = charucoIds;
    return;
  }

  // 直前フレームと現在フレームで共通するコーナー ID を探索し、
  // ピクセル座標の移動量（ユークリッド距離）を合算する
  float totalDist = 0.0f;
  int commonCount = 0;

  for (size_t i = 0; i < charucoIds.size(); ++i)
  {
    const int id = charucoIds[i];
    for (size_t j = 0; j < prevCharucoIds.size(); ++j)
    {
      if (prevCharucoIds[j] == id)
      {
        totalDist += static_cast<float>(cv::norm(charucoCorners[i] - prevCharucoCorners[j]));
        ++commonCount;
        break;
      }
    }
  }

  // 共通コーナーが一定数以上見つかった場合は平均変位量 (px) を算出
  if (commonCount >= std::min(4, minCorners))
  {
    currentMotion = totalDist / commonCount;
  }
  else
  {
    // 共通点が少なすぎる場合はトラッキング不可（大きな移動があった）とみなす
    currentMotion = 999.0f;
  }

  // 平均変位量が許容閾値（motionThresholdPx、例: 2.0px）以下なら静止中と判定
  if (currentMotion <= motionThresholdPx)
  {
    // 静止継続時間 (秒) を積算
    stableDuration += deltaTime;
  }
  else
  {
    // 動いている場合は静止タイマーを即座にリセット
    stableDuration = 0.0f;
  }

  // 静止継続時間が必要時間（minStableTime、例: 0.6秒）に達していれば安定と判定
  isCurrentlyStable = (stableDuration >= minStableTime);

  // 次フレームの変位計算用に現在のコーナー情報を保持
  prevCharucoCorners = charucoCorners;
  prevCharucoIds = charucoIds;
}

//
// 直前に記録された標本に対して十分な姿勢・位置の多様性があるかを調べる
//
// 【目的】
//   静止が検知された場合でも、直前に記録したサンプルとほぼ同じ場所・距離・角度の
//   重複標本が連続して保存されるのを防止する。
//   画面内の平行移動、カメラとの距離変化、ボードの傾き変化のいずれかが
//   閾値以上ある場合のみ「多様性あり」と判定して記録を許可する。
//
bool Calibration::isDiverseEnough(float minDistanceRatio, float minScaleRatio, float minAngleDeg) const
{
  // 初回記録時、または過去の標本がない場合は比較対象がないため常に合格
  if (!hasRecordedShot || allCorners.empty()) return true;

  // コーナーが少なすぎる場合は判定不可
  if (charucoCorners.size() < 4) return false;

  // 現在フレームの幾何特徴を算出
  const auto cur = computeFeatures(charucoCorners);

  // 1. 重心移動量の比率 (画像対角線長に対する割合)
  //    ボードが画面の中央、四隅、端など異なる位置に移動したかを判定
  const float diag = std::hypot(static_cast<float>(size.width), static_cast<float>(size.height));
  const float dist = static_cast<float>(cv::norm(cur.centroid - lastRecordedFeatures.centroid));
  if (diag > 0.0f && (dist / diag) >= minDistanceRatio) return true;

  // 2. サイズ（慣性半径・スケール）の変化率
  //    カメラに近づいたか、遠ざかったか（奥行き方向のバリエーション）を判定
  if (lastRecordedFeatures.spread > 0.0f)
  {
    const float scaleDiff = std::abs(cur.spread - lastRecordedFeatures.spread) / lastRecordedFeatures.spread;
    if (scaleDiff >= minScaleRatio) return true;
  }

  // 3. 主軸角度の変化 (度)
  //    ボードが回転または斜めに傾けられたかを判定 (周期性を考慮して 0°～90° の最小差を算出)
  float angleDiff = std::abs(cur.angleDeg - lastRecordedFeatures.angleDeg);
  while (angleDiff > 90.0f) angleDiff = std::abs(180.0f - angleDiff);
  if (angleDiff >= minAngleDeg) return true;

  // いずれの幾何変化も閾値に満たない場合は「同一姿勢の重複」とみなして不合格
  return false;
}
