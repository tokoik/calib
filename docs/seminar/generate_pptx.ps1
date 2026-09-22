# calib 勉強会 PowerPoint スライド自動生成スクリプト (pwsh 対応版)
$ErrorActionPreference = "Stop"

# Office / PowerPoint 定数
$msoTrue = -1
$msoFalse = 0
$msoShapeRectangle = 1
$ppLayoutBlank = 12
$ppAlignCenter = 2
$ppAlignLeft = 1

Write-Host "PowerPoint アプリケーションを起動しています..."
$ppt = New-Object -ComObject PowerPoint.Application

try {
    $presentation = $ppt.Presentations.Add()
    # 16:9 ワイドスクリーン設定 (960 x 540 pt)
    $presentation.PageSetup.SlideWidth = 960
    $presentation.PageSetup.SlideHeight = 540

    # スライド作成ヘルパー
    function Add-MySlide {
        param(
            [string]$Category,
            [string]$Title,
            [string]$LeftTitle,
            [string]$LeftText,
            [string]$RightTitle,
            [string]$RightText,
            [string]$Notes,
            [switch]$IsTitleSlide,
            [switch]$IsSectionSlide,
            [string]$BottomText = ""
        )

        $slide = $presentation.Slides.Add($presentation.Slides.Count + 1, $ppLayoutBlank)
        $slide.FollowMasterBackground = $msoFalse
        $slide.Background.Fill.Solid()
        $slide.Background.Fill.ForeColor.RGB = 0x1A0F0F # BGR: 0x0F172A

        if ($IsTitleSlide) {
            # タイトルスライド
            $tb = $slide.Shapes.AddTextbox(1, 80, 140, 800, 260)
            $tf = $tb.TextFrame
            $tf.WordWrap = $msoTrue

            $p1 = $tf.TextRange.Paragraphs(1)
            $p1.Text = $Title
            $p1.Font.Name = "Meiryo"
            $p1.Font.Size = 40
            $p1.Font.Bold = $msoTrue
            $p1.Font.Color.RGB = 0xF8BD38 # BGR of 0x38BDF8
            $p1.ParagraphFormat.Alignment = $ppAlignCenter

            $p2 = $tf.TextRange.InsertAfter("`n" + $LeftTitle)
            $p2.Font.Name = "Meiryo"
            $p2.Font.Size = 22
            $p2.Font.Color.RGB = 0xFCFAF8
            $p2.ParagraphFormat.Alignment = $ppAlignCenter

            $p3 = $tf.TextRange.InsertAfter("`n`n" + $LeftText)
            $p3.Font.Name = "Meiryo"
            $p3.Font.Size = 14
            $p3.Font.Color.RGB = 0xB8A394
            $p3.ParagraphFormat.Alignment = $ppAlignCenter

        } elseif ($IsSectionSlide) {
            # セクション扉スライド
            $tb = $slide.Shapes.AddTextbox(1, 80, 160, 800, 220)
            $tf = $tb.TextFrame
            $tf.WordWrap = $msoTrue

            $p0 = $tf.TextRange.Paragraphs(1)
            $p0.Text = $Category
            $p0.Font.Name = "Meiryo"
            $p0.Font.Size = 16
            $p0.Font.Bold = $msoTrue
            $p0.Font.Color.RGB = 0x99D334
            $p0.ParagraphFormat.Alignment = $ppAlignCenter

            $p1 = $tf.TextRange.InsertAfter("`n" + $Title)
            $p1.Font.Name = "Meiryo"
            $p1.Font.Size = 34
            $p1.Font.Bold = $msoTrue
            $p1.Font.Color.RGB = 0xFCFAF8
            $p1.ParagraphFormat.Alignment = $ppAlignCenter

            $p2 = $tf.TextRange.InsertAfter("`n`n" + $LeftText)
            $p2.Font.Name = "Meiryo"
            $p2.Font.Size = 15
            $p2.Font.Color.RGB = 0xF8BD38
            $p2.ParagraphFormat.Alignment = $ppAlignCenter

        } else {
            # 通常スライドヘッダー
            $hdrBox = $slide.Shapes.AddTextbox(1, 50, 25, 860, 60)
            $htf = $hdrBox.TextFrame
            $htf.MarginLeft = 0; $htf.MarginTop = 0; $htf.MarginRight = 0; $htf.MarginBottom = 0

            $hp1 = $htf.TextRange.Paragraphs(1)
            $hp1.Text = $Category.ToUpper()
            $hp1.Font.Name = "Meiryo"
            $hp1.Font.Size = 11
            $hp1.Font.Bold = $msoTrue
            $hp1.Font.Color.RGB = 0x99D334 # Accent

            $hp2 = $htf.TextRange.InsertAfter("`n" + $Title)
            $hp2.Font.Name = "Meiryo"
            $hp2.Font.Size = 24
            $hp2.Font.Bold = $msoTrue
            $hp2.Font.Color.RGB = 0xF8BD38 # Primary Title

            # コンテンツ配置 (2カラムまたは1カラム)
            if ($RightTitle -or $RightText) {
                # 2カラム
                # 左カード
                $card1 = $slide.Shapes.AddShape($msoShapeRectangle, 50, 100, 415, 360)
                $card1.Fill.Solid()
                $card1.Fill.ForeColor.RGB = 0x3B291E # BGR of 0x1E293B
                $card1.Line.ForeColor.RGB = 0x554133
                $card1.Line.Weight = 1
                $ctf1 = $card1.TextFrame
                $ctf1.MarginLeft = 15; $ctf1.MarginTop = 15; $ctf1.MarginRight = 15; $ctf1.MarginBottom = 15
                $ctf1.WordWrap = $msoTrue
                $cp1 = $ctf1.TextRange.Paragraphs(1)
                $cp1.Text = $LeftTitle
                $cp1.Font.Name = "Meiryo"
                $cp1.Font.Size = 16
                $cp1.Font.Bold = $msoTrue
                $cp1.Font.Color.RGB = 0xF8BD38
                $cp2 = $ctf1.TextRange.InsertAfter("`n`n" + $LeftText)
                $cp2.Font.Name = "Meiryo"
                $cp2.Font.Size = 13
                $cp2.Font.Color.RGB = 0xFCFAF8

                # 右カード
                $card2 = $slide.Shapes.AddShape($msoShapeRectangle, 495, 100, 415, 360)
                $card2.Fill.Solid()
                $card2.Fill.ForeColor.RGB = 0x3B291E
                $card2.Line.ForeColor.RGB = 0x554133
                $card2.Line.Weight = 1
                $ctf2 = $card2.TextFrame
                $ctf2.MarginLeft = 15; $ctf2.MarginTop = 15; $ctf2.MarginRight = 15; $ctf2.MarginBottom = 15
                $ctf2.WordWrap = $msoTrue
                $cp21 = $ctf2.TextRange.Paragraphs(1)
                $cp21.Text = $RightTitle
                $cp21.Font.Name = "Meiryo"
                $cp21.Font.Size = 16
                $cp21.Font.Bold = $msoTrue
                $cp21.Font.Color.RGB = 0x99D334
                $cp22 = $ctf2.TextRange.InsertAfter("`n`n" + $RightText)
                $cp22.Font.Name = "Meiryo"
                $cp22.Font.Size = 13
                $cp22.Font.Color.RGB = 0xFCFAF8

            } else {
                # 1カラムワイド
                $height = if ($BottomText) { 260 } else { 380 }
                $card = $slide.Shapes.AddShape($msoShapeRectangle, 50, 100, 860, $height)
                $card.Fill.Solid()
                $card.Fill.ForeColor.RGB = 0x3B291E
                $card.Line.ForeColor.RGB = 0x554133
                $card.Line.Weight = 1
                $ctf = $card.TextFrame
                $ctf.MarginLeft = 20; $ctf.MarginTop = 20; $ctf.MarginRight = 20; $ctf.MarginBottom = 20
                $ctf.WordWrap = $msoTrue
                $cp1 = $ctf.TextRange.Paragraphs(1)
                $cp1.Text = $LeftTitle
                $cp1.Font.Name = "Meiryo"
                $cp1.Font.Size = 17
                $cp1.Font.Bold = $msoTrue
                $cp1.Font.Color.RGB = 0xF8BD38
                $cp2 = $ctf.TextRange.InsertAfter("`n`n" + $LeftText)
                $cp2.Font.Name = "Meiryo"
                $cp2.Font.Size = 13.5
                $cp2.Font.Color.RGB = 0xFCFAF8

                if ($BottomText) {
                    $bcard = $slide.Shapes.AddShape($msoShapeRectangle, 50, 380, 860, 100)
                    $bcard.Fill.Solid()
                    $bcard.Fill.ForeColor.RGB = 0x2A1F14
                    $bcard.Line.ForeColor.RGB = 0x99D334
                    $bcard.Line.Weight = 1.5
                    $bctf = $bcard.TextFrame
                    $bctf.MarginLeft = 20; $bctf.MarginTop = 15; $bctf.MarginRight = 20; $bctf.MarginBottom = 15
                    $bctf.WordWrap = $msoTrue
                    $bp = $bctf.TextRange.Paragraphs(1)
                    $bp.Text = $BottomText
                    $bp.Font.Name = "Meiryo"
                    $bp.Font.Size = 13
                    $bp.Font.Color.RGB = 0xFCFAF8
                }
            }
        }

        # スピーカーノート追加
        if ($Notes) {
            $slide.NotesPage.Shapes.Placeholders.Item(2).TextFrame.TextRange.Text = $Notes
        }
    }

    Write-Host "スライドを生成しています..."

    # 1. 表紙
    Add-MySlide -IsTitleSlide `
        -Title "calib 勉強会" `
        -LeftTitle "ChArUco 標本自動取得と先進的カメラキャプチャの設計と実装" `
        -LeftText "画像処理プログラミング勉強会（120分） | C++17 / OpenCV / Media Foundation / libcamera" `
        -Notes "【開始の挨拶】カメラ較正ツール calib を題材として、高品質な標本自動取得アルゴリズムと、Windows / Linux それぞれの低レイヤカメラキャプチャ実装を詳しく解説します。"

    # 2. タイムテーブル
    Add-MySlide `
        -Category "Introduction" `
        -Title "勉強会のタイムテーブル (120分)" `
        -LeftTitle "前半: 理論とアルゴリズム (50分)" `
        -LeftText "・00-15分: イントロダクションと全体構成`n・15-50分: ChArUco Board と標本自動取得`n  - なぜ ChArUco なのか？`n  - 手動較正の課題（ブレ・重複）`n  - フレーム間変位追跡 (updateMotion)`n  - 統計モーメント多様性 (isDiverseEnough)`n  - クールダウンと音響フィードバック" `
        -RightTitle "後半: 低レイヤキャプチャ (70分)" `
        -RightText "・50-85分: Media Foundation (CamMf)`n  - COM/MF 初期化と遅延初期化`n  - MFT デコーダと低遅延強制設定`n  - 動的ストリームチェンジへの対応`n・85-115分: libcamera (CamLibcam)`n  - Linux カメラスタックの変遷`n  - DMA バッファ確保と mmap`n  - XBGR8888 ゼロコピー最適化`n・115-120分: 比較まとめと質疑応答" `
        -Notes "本日の2時間の構成です。前半でアルゴリズムと数学的背景、後半で Windows と Linux のネイティブカメラキャプチャの神髄を学びます。"

    # 3. アーキテクチャ
    Add-MySlide `
        -Category "Architecture" `
        -Title "calib の全体像: CPU と GPU の役割分担" `
        -LeftTitle "CPU 側 (OpenCV とキャプチャ)" `
        -LeftText "・入力取得: CamMf / CamLibcam / CamCv による非同期取得`n・ChArUco 検出: cv::aruco::CharucoDetector`n・自動取得判定: 変位追跡 ＋ 幾何モーメント解析`n・較正計算: calibrateCamera (カメラ行列 K, 歪み係数 D)" `
        -RightTitle "GPU 側 (OpenGL / GLSL)" `
        -RightText "・第1パス (歪み補正): undistortion.frag による補正描画`n・第2パス (展開表示): 正射影、立体射影、正距円筒射影`n・最終描画: Dear ImGui UI 描画 ＋ contain 方式中央表示" `
        -Notes "プラットフォーム固有の型を UI や描画ループに露出させず、CPU 上の BGRA バッファを経由して OpenGL テクスチャへ転送する疎結合設計です。"

    # 4. 第1部 扉
    Add-MySlide -IsSectionSlide `
        -Category "Part 1" `
        -Title "ChArUco Board によるカメラ較正と`n標本自動取得アルゴリズム" `
        -LeftText "モーション検出・画像モーメント・幾何多様性チェック" `
        -Notes "第1部に入ります。キャリブレーションの品質と作業効率を飛躍的に高める自動取得の仕組みです。"

    # 5. なぜ ChArUco なのか？
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "なぜ ChArUco Board なのか？" `
        -LeftTitle "Chessboard と ArUco の融合" `
        -LeftText "・チェッカーボード:`n  交点精度は極めて高いが、一部が見切れると全交点の認識に失敗し 1 点も検出できない。`n`n・ArUco マーカー:`n  固有 ID を持ち隠れに強いが、四隅のコーナー精度が低く較正には不向き。`n`n・ChArUco Board:`n  マーカーから各交点の絶対 ID を特定するため、画面端で見切れても検出可能！交点自体は高精度なチェッカー交点を採用。" `
        -RightTitle "歪み補正における決定的な利点" `
        -RightText "広角・魚眼レンズでは、画面の最周辺部（四隅）で歪みが最大になります。`n`n従来のチェッカーボードでは四隅にボードを配置すると見切れて検出不能になりがちでしたが、ChArUco なら半分画面外にはみ出しても周辺部の標本を確実に採取できます。" `
        -Notes "ChArUco Board を採用することで、周辺部の歪みパラメータの推定精度が飛躍的に向上します。"

    # 6. ワンマン較正の課題
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "手動較正の課題: 「ブレ」と「重複」" `
        -LeftTitle "課題 1: モーションブラー (ブレ)" `
        -LeftText "・片手でボードを持ち、もう片手でキーボードやマウスを押すと必ず手振れが発生。`n・ブレた画像ではコーナー検出位置が数ピクセル狂い、再投影誤差が大幅に悪化する。" `
        -RightTitle "課題 2: 構図の偏り (重複姿勢)" `
        -RightText "・画面中央ばかり、あるいは同じ距離・同じ傾きで何枚記録しても較正行列のランクは上がらない。`n・周辺部で歪み補正が破綻する過学習を引き起こす。" `
        -Notes "この2大課題をアルゴリズムで解決するのが、本日のハイライトである自動取得アルゴリズムです。"

    # 7. 自動取得シーケンス
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "標本自動取得の毎フレーム評価シーケンス" `
        -LeftTitle "処理パイプライン (Menu::updateAutoCapture)" `
        -LeftText "1. ChArUco 検出: コーナー数が 6 点未満なら何もしない`n2. 静止判定 (updateMotion):`n   - 直前フレームと共通するコーナー ID を照合`n   - 平均変位量 ≦ 2.0px が 0.6 秒継続したか？`n3. 幾何多様性判定 (isDiverseEnough):`n   - 直前記録ショットと比べ「位置」「距離」「傾き」に十分な差異があるか？`n4. 自動トリガー:`n   - recordCorners() で標本保存 ＋ クールダウン (1.5s) ＋ 音響フィードバック" `
        -Notes "作業者は画面を見ずに、カメラの前でボードを色々な姿勢で止めて音を聞くだけで較正が完了します。"

    # 8. 静止判定 updateMotion
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "静止判定: フレーム間変位追跡 (updateMotion)" `
        -LeftTitle "アルゴリズムの実装ポイント (Calibration.cpp)" `
        -LeftText "・共通 ID の探索: charucoIds と prevCharucoIds を照合し、同一交点のみを変位計算に使用。`n・平均変位量の算出: 共通コーナーのユークリッド距離合計 / 共通点数。`n・即時リセット: 変位 ≦ 2.0px なら stableDuration += deltaTime。1 フレームでも超えたら即 0.0 にリセット。`n・安定判定: stableDuration ≧ 0.6 秒 で isCurrentlyStable = true。" `
        -RightTitle "設計のこだわり" `
        -RightText "画像差分法やオプティカルフローと異なり、コーナーのサブピクセル座標を直接追跡するため、微小な手の震えを 100% 検知できます。`n`n共通点が 4 点未満の場合は追跡不能として変位量 999.0px をセットします。" `
        -Notes "直前フレームと現在のフレームの共通 ID を照合することで、ボードの移動による ID の入れ替わりにも頑健に対応します。"

    # 9. 幾何モーメント解析
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "幾何多様性判定: 統計モーメントの活用" `
        -LeftTitle "なぜ画像モーメントなのか？" `
        -LeftText "外接矩形 (Bounding Box) は、ボードの一部が見切れたり手で隠れたりすると中心やサイズが大きく狂います。`n`nコーナー点群の 2 次モーメントを用いることで、点群の幾何学的広がり（重心・スケール・傾き）を統計的かつロバストに要約できます。" `
        -RightTitle "3 つの幾何特徴量" `
        -RightText "1. 重心位置 (Centroid):`n   x = m10 / m00,  y = m01 / m00`n   → 画像内の配置位置を表す。`n`n2. 慣性半径 (Spread / Scale):`n   R = sqrt(mu20 + mu02)`n   → カメラとボードの距離（見かけの大きさ）を表す。`n`n3. 主軸角度 (Orientation Angle):`n   theta = 0.5 * atan2(2*mu11, mu20 - mu02)`n   → 画像平面内での傾きを表す。" `
        -Notes "cv::moments を使って得られる重心、慣性半径、主軸角度の3つの値でボードの姿勢を要約します。"

    # 10. computeFeatures コード
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "幾何特徴抽出: computeFeatures 実装" `
        -LeftTitle "Calibration::computeFeatures (Calibration.cpp)" `
        -LeftText "cv::Moments m = cv::moments(corners);`nif (m.m00 > 0.0) {`n  // 1. 重心位置`n  features.centroid = cv::Point2f(m.m10 / m.m00, m.m01 / m.m00);`n`n  const double mu20 = m.mu20 / m.m00;`n  const double mu02 = m.mu02 / m.m00;`n  const double mu11 = m.mu11 / m.m00;`n`n  // 2. 慣性半径 (スケール)`n  features.spread = static_cast<float>(std::sqrt(std::max(0.0, mu20 + mu02)));`n`n  // 3. 主軸角度 (-90° 〜 +90°)`n  features.angleDeg = static_cast<float>(0.5 * std::atan2(2.0 * mu11, mu20 - mu02) * 180.0 / CV_PI);`n}" `
        -Notes "点の総数 m00 で正規化された 2 次中心モーメントから、慣性半径と傾き角度を算出しています。"

    # 11. isDiverseEnough コード
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "多様性判定: isDiverseEnough 実装" `
        -LeftTitle "いずれか 1 つの条件を満たせば合格 (OR 判定)" `
        -LeftText "1. 重心移動量比率:`n   移動距離 / 画像対角線長 ≧ 5% (minDistanceRatio)`n   → 画面の中央、四隅、端など別の場所へ移動したか？`n`n2. スケール変化率:`n   |現在半径 - 直前半径| / 直前半径 ≧ 12% (minScaleRatio)`n   → カメラにグッと近づいたか、遠ざかったか？`n`n3. 主軸角度変化:`n   |現在角度 - 直前角度| ≧ 8° (minAngleDeg)`n   → ボードを斜めに傾けたか？ (周期性考慮)" `
        -RightTitle "無駄なサンプルの完全排除" `
        -RightText "作業者が同じ場所でボードを静止させ続けても、多様性チェックでブロックされるため重複記録されません。`n`nわずかに位置、距離、傾きのいずれかを変えるだけで即座に次の標本として承認されます。" `
        -Notes "3つの幾何変化のいずれかが閾値を超えれば OK とする OR 条件が、直感的な操作感を生み出しています。"

    # 12. クールダウンとフィードバック
    Add-MySlide `
        -Category "Part 1: ChArUco Calibration" `
        -Title "クールダウン制御と音響フィードバック" `
        -LeftTitle "姿勢変更クールダウン (1.5秒)" `
        -LeftText "・標本を記録した直後、タイマーを 1.5 秒にセット。`n・作業者が次の姿勢へボードを動かす最中に、誤って中間フレームが記録されるのを完全に防止。" `
        -RightTitle "非同期音響フィードバック" `
        -RightText "#if defined(_WIN32)`n  // メインスレッドを止めないよう別スレッド再生`n  std::thread([] { Beep(1200, 100); }).detach();`n#else`n  // Linux は端末ベル文字`n  std::cout << '\a' << std::flush;`n#endif" `
        -BottomText "成果: 作業者は画面を見ずに、音を聞きながら 15〜20 姿勢ほど静止させるだけで、わずか 30〜40 秒で再投影誤差 0.2〜0.4 px の極めて高精度な較正が完了します。" `
        -Notes "Beep を別スレッドで鳴らすことで、UI や描画ループのフレーム落ちを防いでいます。"

    # 13. 第2部 扉
    Add-MySlide -IsSectionSlide `
        -Category "Part 2" `
        -Title "Microsoft Media Foundation を使った`nキャプチャクラス CamMf" `
        -LeftText "COM / MFT デコーダ / 低遅延設定 / 動的ストリームチェンジ" `
        -Notes "第2部に入ります。Windows ネイティブのマルチメディア基盤である Media Foundation の詳細解説です。"

    # 14. なぜ MF なのか？
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "なぜ OpenCV VideoCapture から脱却したのか？" `
        -LeftTitle "OpenCV VideoCapture の課題" `
        -LeftText "・内部バッファリングにより 50〜100ms の遅延が発生。`n・MJPG / H.264 のデコードが CPU 処理となり高負荷。`n・デバイス列挙時に重い初期化が走り UI がフリーズ。" `
        -RightTitle "Media Foundation 直接制御の強み" `
        -RightText "・CODECAPI_AVLowLatencyMode で遅延を極小化。`n・GPU / DXVA 支援の MFT ハードウェアデコーダを活用。`n・メタデータ列挙時はデコーダを作らない遅延初期化。" `
        -BottomText "アーキテクチャ方針: 基底クラス Camera から OpenCV 依存を排除し、生のバイト配列 (BGRA) に統一することでモジュール間の疎結合を実現。" `
        -Notes "教材としても、Windows の本格的なマルチメディア API の扱い方を学べる貴重なサンプルになっています。"

    # 15. CamMf パイプライン
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "CamMf のクラス構成と MFT パイプライン" `
        -LeftTitle "MFT 処理パイプラインの構成" `
        -LeftText "1. 非圧縮フォーマット (YUY2 / NV12):`n   Source Reader → MFT Color Converter → RGB32`n`n2. 圧縮フォーマット (MJPG / H.264):`n   Source Reader → MFT Decoder (NV12) → MFT Color Converter → RGB32`n`n・pDecoder: MJPG/H264 から NV12 へデコード`n・pConverter: NV12/YUY2 から RGB32 (BGRA) へ変換" `
        -RightTitle "クラス構成と基底クラス" `
        -RightText "・Camera: 解像度、バッファ、スレッド同期を管理する抽象基底クラス。`n・CamMf: Media Foundation 固有のポインタ (IMFSourceReader, IMFTransform) をカプセル化。`n・ComInitializer: プロセス全体での COM/MF 初期化を行うシングルトン。" `
        -Notes "MJPGやH264の場合はデコーダとコンバータの2段、非圧縮の場合はコンバータの1段パイプラインになります。"

    # 16. ComInitializer
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "ライフサイクル管理: ComInitializer シングルトン" `
        -LeftTitle "初期化シーケンス (initialize)" `
        -LeftText "1. CoInitializeEx(nullptr, COINIT_MULTITHREADED): マルチスレッド COM 初期化`n2. MFStartup(MF_VERSION, MFSTARTUP_FULL): Media Foundation 起動`n3. MFCreateAttributes ＋ MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID`n4. MFEnumDeviceSources でカメラ一覧を取得" `
        -RightTitle "RAII による確実な終了処理 (~ComInitializer)" `
        -RightText "1. SafeRelease で全メディアソースを解放`n2. CoTaskMemFree(ppSourceActivate)`n3. MFShutdown()`n4. CoUninitialize()`n`n→ プロセス終了時にリソースリークを一切残さない完全な RAII 設計。" `
        -Notes "COMの初期化とMedia Foundationのシャットダウンはプロセスで1回だけ適切に行う必要があります。"

    # 17. 遅延初期化
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "フォーマット列挙と遅延初期化 (Lazy Init)" `
        -LeftTitle "高速な列挙: enumerateFormats()" `
        -LeftText "・カメラを開いた直後は、Source Reader からネイティブメディアタイプ (GetNativeMediaType) のメタデータのみを走査。`n・解像度、fps、サブタイプ (GUID) を取得し、UI 用構造体 formatList に格納。`n・この段階ではデコーダの作成やバッファ確保を一切行わないため、UI が一瞬で起動！" `
        -RightTitle "遅延構築: setFormat(index)" `
        -RightText "・ユーザーが UI で明示的にフォーマットを選択した時、またはキャプチャ開始時に初めて呼び出される。`n・選択されたフォーマットに応じて MFT デコーダとカラーコンバータを生成・接続。" `
        -Notes "フォーマット列挙時に重い初期化を行わないのが、UI の軽快さを保つ決定的な工夫です。"

    # 18. 低遅延設定
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "究極の低遅延化: CODECAPI_AVLowLatencyMode" `
        -LeftTitle "1. Source Reader 属性での要求" `
        -LeftText "pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);`npAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);`n`n→ リーダーに対して内部バッファリングを最小化し、ハードウェアアクセラレーションを許可。" `
        -RightTitle "2. MFT デコーダへの低遅延強制" `
        -RightText "ICodecAPI* pCodecAPI{ nullptr };`npDecoder->QueryInterface(IID_PPV_ARGS(&pCodecAPI));`n`nVARIANT var; VariantInit(&var);`nvar.vt = VT_UI4; var.ulVal = 1; // Low Latency Mode`npCodecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);`n`n→ デコーダ内部のフレーム蓄積を禁止し、1入力即1出力を強制！" `
        -Notes "CODECAPI_AVLowLatencyMode を設定することで、Webカメラの映像遅延が劇的に短縮されます。"

    # 19. 動的ストリームチェンジ
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "動的ストリームチェンジ (MF_E_TRANSFORM_STREAM_CHANGE)" `
        -LeftTitle "商用レベル MFT の最重要例外処理" `
        -LeftText "MJPG/H.264 デコーダは、最初のフレームヘッダーを読み込んだ瞬間、ProcessOutput が MF_E_TRANSFORM_STREAM_CHANGE を返して出力メディアタイプの再設定を要求します。`n`nこれに対応しないと映像が一切流れません。" `
        -RightTitle "CamMf での対応手順 (CamMf.cpp L818-L980)" `
        -RightText "1. デコーダの利用可能な出力タイプから NV12 を再検索`n2. 入力解像度・アスペクト比を出力タイプへコピー`n3. pDecoder->SetOutputType で新タイプを確定`n4. createDecoderBuffer() で 64B アラインバッファを再作成`n5. 後段のカラーコンバータを再セットアップして ProcessOutput 再試行" `
        -Notes "ストリームチェンジ処理を完璧に実装している点が、CamMf のコードの非常に高い堅牢性を支えています。"

    # 20. メモリアラインメント
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "高速化の要: 64バイトアラインメントバッファ" `
        -LeftTitle "createDecoderBuffer / createConverterBuffer" `
        -LeftText "const auto alignedWidth  = (width + 15) & ~15;`nconst auto alignedHeight = (height + 15) & ~15;`nconst auto cbDecoderCalc = (alignedWidth * alignedHeight * 3) / 2;`n`nconst auto alignment = (streamInfo.cbAlignment > 0) ? (streamInfo.cbAlignment - 1) : 63;`n`n// 64 バイト境界に整列されたバッファを作成`nMFCreateAlignedMemoryBuffer(cbDecoder, alignment, &pDecoderBuffer);" `
        -RightTitle "なぜアラインメントが必要なのか？" `
        -RightText "ハードウェアデコーダや MFT カラーコンバータは内部で AVX2 / SSE4 などの SIMD 命令を駆使します。`n`nメモリが 64 バイト境界に整列されていない場合、非整列アクセス例外や大幅な速度低下を招きます。" `
        -Notes "MFCreateAlignedMemoryBuffer を使うことで、ハードウェアが要求するアラインメントを確実に満たせます。"

    # 21. キャプチャループ
    Add-MySlide `
        -Category "Part 2: Media Foundation (CamMf)" `
        -Title "キャプチャスレッドループ (CamMf::capture)" `
        -LeftTitle "スレッドループの基本ステップ" `
        -LeftText "1. ReadSample でサンプルを取得`n2. pDecoder->ProcessInput / ProcessOutput (MJPG/H264 の場合)`n3. pConverter->ProcessInput / ProcessOutput (RGB32 へ変換)`n4. メディアバッファを Lock() して CPU メモリへ memcpy`n5. captured = true を立てて通知" `
        -RightTitle "動作モードの切り替え" `
        -RightText "・低遅延優先 (prioritizeLatency == true):`n  デコーダバッファに溜まった古いフレームを捨て、常に最新フレームのみを採用。`n`n・全フレーム処理 (prioritizeLatency == false):`n  メインスレッドが前フレームを消費するまで std::this_thread::yield() で待機。" `
        -Notes "排他制御には std::mutex を使い、最小限のクリティカルセクションで memcpy を行っています。"

    # 22. 第3部 扉
    Add-MySlide -IsSectionSlide `
        -Category "Part 3" `
        -Title "libcamera を使った`nキャプチャクラス CamLibcam" `
        -LeftText "Linux / Raspberry Pi ネイティブ / DMA バッファ / 非同期リクエスト" `
        -Notes "第3部に入ります。Linux / Raspberry Pi 環境における libcamera ネイティブ実装の解説です。"

    # 23. Linux カメラスタック
    Add-MySlide `
        -Category "Part 3: libcamera (CamLibcam)" `
        -Title "Linux / Raspberry Pi カメラスタックの変遷" `
        -LeftTitle "世代交代の歴史" `
        -LeftText "・第1世代 (V4L2): UVC には良いが、SoC ネイティブカメラの複雑な ISP 制御が標準化されていなかった。`n`n・第2世代 (Raspicam / MMAL): Broadcom GPU 依存の独自 API。Raspberry Pi OS Bookworm で完全廃止。`n`n・第3世代 (libcamera): Linux 標準のオープンフレームワーク。C++17 準拠、オープンな ISP 制御 (IPA) を実現。" `
        -RightTitle "なぜネイティブ API なのか？" `
        -RightText "libcamerify 経由の V4L2 エミュレーションでは余分なプロセス間コピーやフォーマット変換が挟まり、30〜50% の性能低下が生じます。`n`nCamLibcam は libcamera C++ API をネイティブに叩くことで極限のパフォーマンスを発揮します。" `
        -Notes "Raspberry Pi 4 や 5 でカメラを扱う場合、libcamera ネイティブ対応が現在のデファクトスタンダードです。"

    # 24. コア概念
    Add-MySlide `
        -Category "Part 3: libcamera (CamLibcam)" `
        -Title "libcamera の基本オブジェクトと関係性" `
        -LeftTitle "主要オブジェクト" `
        -LeftText "・CameraManager: 全カメラの監視・列挙（シングルトン）`n・Camera: 個別のカメラデバイス。acquire() でロック`n・StreamRole: Viewfinder, VideoRecording, Raw`n・CameraConfiguration: パイプライン設定。validate() で自動調整`n・FrameBufferAllocator: DMA バッファの確保`n・Request: 撮影リクエスト。バッファをバインドして投入" `
        -RightTitle "非同期リクエスト駆動モデル" `
        -RightText "同期的に「画像をくれ」と待つのではなく、事前に確保した複数の DMA バッファをリクエストに載せてキューへ投入 (queueRequest)。`n`n撮影完了時にシグナル (requestCompleted) が発火し、コールバック関数で処理するイベント駆動アーキテクチャ。" `
        -Notes "libcamera の最大の特徴は、この Request-Buffer モデルによる完全な非同期パイプラインです。"

    # 25. ゼロコピー最適化
    Add-MySlide `
        -Category "Part 3: libcamera (CamLibcam)" `
        -Title "フォーマット優先順位とゼロコピー最適化" `
        -LeftTitle "優先選択リスト (CamLibcam.cpp)" `
        -LeftText "const std::vector<libcamera::PixelFormat> preferenceList = {`n  libcamera::formats::R8,       // 8-bit モノクロ (OV9281 等)`n  libcamera::formats::XBGR8888, // ★ 4バイト BGRA 互換`n  libcamera::formats::BGRX8888,`n  libcamera::formats::RGB888,`n  libcamera::formats::YUYV,`n  libcamera::formats::NV12,`n};" `
        -RightTitle "XBGR8888 の絶大な威力" `
        -RightText "カメラハードウェア（ISP）が XBGR8888 を出力できる場合、内部バッファとメモリ配置が完全に一致するため、ピクセルごとの色変換ループが一切不要！`n`n一括 std::memcpy だけで転送が完了し、CPU 使用率はほぼゼロになります。" `
        -Notes "組込みボードでは、色変換をハードウェア（ISP）に任せられるかどうかが極めて重要です。"

    # 26. DMA バッファ確保と mmap
    Add-MySlide `
        -Category "Part 3: libcamera (CamLibcam)" `
        -Title "DMA バッファ確保と mmap マッピング" `
        -LeftTitle "バッファ割り当てと mmap 手順" `
        -LeftText "1. FrameBufferAllocator でストリームにバッファを割り当て`n2. 各バッファプレーンのファイル記述子 (plane.fd) から ::mmap を実行:`n   void* memory = ::mmap(NULL, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), plane.offset);`n3. マッピングしたアドレスを mappedBuffers に保持`n4. camera->createRequest() にバッファを追加してリクエスト生成" `
        -RightTitle "カーネル DMA 直結のメリット" `
        -RightText "ファイル記述子は Linux カーネルの DMA-BUF です。`n`nカメラハードウェアが物理メモリへ直接書き込み、アプリケーションはそれを mmap で直接読み取るため、カーネルからユーザー空間へのコピーコストが完全にゼロになります。" `
        -Notes "物理メモリ直結の DMA-BUF を mmap することで、高フレームレートでも CPU に負荷をかけません。"

    # 27. requestComplete
    Add-MySlide `
        -Category "Part 3: libcamera (CamLibcam)" `
        -Title "完了コールバック (requestComplete) と再利用" `
        -LeftTitle "コールバック内での処理" `
        -LeftText "void CamLibcam::requestComplete(libcamera::Request* request) {`n  if (request->status() == RequestCancelled) return;`n`n  // 完了バッファからアドレス取得`n  const uint8_t* src = mappedBuffers[buffer][0].address;`n  {`n    std::lock_guard<std::mutex> lock(mtx);`n    copyOrConvertFrame(image.data(), src);`n    captured = true;`n  }`n`n  // ★ リクエストを再利用してカメラキューへ再投入！`n  if (running) {`n    request->reuse(Request::ReuseBuffers);`n    camera->queueRequest(request);`n  }`n}" `
        -RightTitle "リングバッファの循環ループ" `
        -RightText "・リクエストを使い捨てず、reuse() でバッファを保持したまま再利用。`n・メモリ確保・解放オーバーヘッドがゼロ。`n・3〜4 個のリクエストがハードウェアとアプリ間を永久に循環する。" `
        -Notes "reuse() して queueRequest() に戻すのが、libcamera でフレームを取りこぼさない必須テクニックです。"

    # 28. YUYV 整数演算展開
    Add-MySlide `
        -Category "Part 3: libcamera (CamLibcam)" `
        -Title "組込み向け最適化: YUYV 整数演算展開" `
        -LeftTitle "固定小数点ビットシフトによる高速変換" `
        -LeftText "const int y0 = rowSrc[0] - 16;`nconst int u  = rowSrc[1] - 128;`nconst int y1 = rowSrc[2] - 16;`nconst int v  = rowSrc[3] - 128;`n`n// 浮動小数点を使わず 32bit 整数乗算と >> 8 で高速展開`nrowDst[0] = std::clamp((298 * y0 + 516 * u + 128) >> 8, 0, 255); // B`nrowDst[1] = std::clamp((298 * y0 - 100 * u - 208 * v + 128) >> 8, 0, 255); // G`nrowDst[2] = std::clamp((298 * y0 + 409 * v + 128) >> 8, 0, 255); // R`nrowDst[3] = 255; // A" `
        -RightTitle "ARM コンパイラ最適化との親和性" `
        -RightText "浮動小数点演算を完全に排除することで、GCC / Clang が ARM NEON SIMD 命令へ自動ベクトル化しやすい素直なコードになっています。" `
        -Notes "YUYV は USB カメラ等で最も一般的なフォーマットですが、この整数変換で CPU 負荷を抑えています。"

    # 29. 第4部 扉
    Add-MySlide -IsSectionSlide `
        -Category "Part 4" `
        -Title "キャプチャバックエンドの比較まとめ &`nトラブルシューティング" `
        -LeftText "CamCv vs CamMf vs CamLibcam / 現場でのノウハウ" `
        -Notes "第4部、各バックエンドの比較とトラブルシューティングです。"

    # 30. 総合比較表
    Add-MySlide `
        -Category "Part 4: Summary & Comparison" `
        -Title "キャプチャバックエンド総合比較" `
        -LeftTitle "Media Foundation (CamMf)" `
        -LeftText "・ターゲット: Windows 10/11`n・API: 低水準 COM / MFT`n・レイテンシ: 極小 (CODECAPI_AVLowLatencyMode)`n・ハードウェア支援: DXVA / GPU デコーダ活用`n・メモリ管理: 64B アラインバッファ`n・用途: Windows PC での超低遅延リアルタイム較正" `
        -RightTitle "libcamera (CamLibcam)" `
        -RightText "・ターゲット: Linux / Raspberry Pi 4/5`n・API: 低水準 libcamera C++ API`n・レイテンシ: 極小 (DMA 直結キューイング)`n・ハードウェア支援: SoC ISP ハードウェア直結`n・メモリ管理: DMA-BUF + mmap (ゼロコピー)`n・用途: 組込み・自律ロボット・エッジ環境" `
        -BottomText "OpenCV (CamCv) は動画ファイル再生やクロスプラットフォーム環境のフォールバックとして位置づけられます。" `
        -Notes "それぞれのプラットフォームに最適化されたバックエンドを用意することで、最高のパフォーマンスを発揮します。"

    # 31. トラブルシューティング
    Add-MySlide `
        -Category "Part 4: Summary & Comparison" `
        -Title "よくある質問とトラブルシューティング" `
        -LeftTitle "Q1. 自動取得が反応しない" `
        -LeftText "・コーナー数が 6 点未満になっていませんか？（ボード全体を画面内に収める）`n・平均変位が 2.0px を超えていませんか？（机に置くか両手でしっかりホールド）`n・直前の標本と構図が近すぎませんか？（位置・距離・傾きのいずれかを大きく変える）" `
        -RightTitle "Q2. 再投影誤差が小さくならない" `
        -RightText "・画面四隅に見切れさせた標本が不足していませんか？（ChArUco の強みを活かして周辺部を攻める）`n・至近距離（大）と遠距離（小）の差をつけていますか？`n・ボードを 30度程度しっかり傾けていますか？" `
        -Notes "参加者が実際にキャリブレーションを行う際につまずきやすいポイントを整理しています。"

    # 32. まとめ
    Add-MySlide -IsTitleSlide `
        -Title "まとめ & 質疑応答" `
        -LeftTitle "本日の重要ポイント" `
        -LeftText "1. ChArUco 標本自動取得: 変位追跡と統計モーメントにより、ワンマンでも最高精度の較正を実現`n2. Media Foundation (CamMf): MFT 直接制御と低遅延設定で Windows の極限性能を引き出す`n3. libcamera (CamLibcam): DMA バッファ mmap とリクエスト再利用で組込み Linux のゼロコピーを達成`n`nご清聴ありがとうございました。" `
        -Notes "以上で講義を終了します。質疑応答に移ります。"

    $outputPath = "D:\Users\tokoi\Documents\Projects\worktrees\calib\docs\seminar\calib_seminar_slides.pptx"
    Write-Host "プレゼンテーションを保存しています: $outputPath"
    $presentation.SaveAs($outputPath)
    Write-Host "PowerPoint スライドの生成に成功しました！"

} catch {
    Write-Error "エラーが発生しました: $_"
} finally {
    if ($presentation) {
        $presentation.Close()
    }
    $ppt.Quit()
    [System.Runtime.Interopservices.Marshal]::ReleaseComObject($ppt) | Out-Null
    [System.GC]::Collect()
    [System.GC]::WaitForPendingFinalizers()
}
