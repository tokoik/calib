package net.wakayama_u.tokoi.calib

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // ネイティブエンジンのアセット・ストレージ初期化
        NativeBridge.nativeInit(assets, filesDir.absolutePath)

        setContent {
            MaterialTheme(
                colorScheme = darkColorScheme(
                    primary = Color(0xFF64B5F6),
                    secondary = Color(0xFF81C784),
                    background = Color(0xFF121212),
                    surface = Color(0xFF1E1E1E),
                    error = Color(0xFFE57373)
                )
            ) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    MainScreen()
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen() {
    val context = LocalContext.current
    var hasCameraPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.CAMERA
            ) == PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { isGranted ->
        hasCameraPermission = isGranted
    }

    LaunchedEffect(Unit) {
        if (!hasCameraPermission) {
            permissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    // キャプチャ状態
    var isCapturing by remember { mutableStateOf(false) }

    // ボトムシートの表示状態
    var showInputSheet by remember { mutableStateOf(false) }
    var showCalibSheet by remember { mutableStateOf(false) }

    // 較正・自動キャプチャ状態（定期更新）
    var isDetectingBoard by remember { mutableStateOf(false) }
    var autoCaptureEnabled by remember { mutableStateOf(false) }
    var autoProgress by remember { mutableStateOf(0.0f) }
    var isDiverse by remember { mutableStateOf(false) }
    var isStable by remember { mutableStateOf(false) }
    var sampleCount by remember { mutableStateOf(0) }
    var isCalibrated by remember { mutableStateOf(false) }
    var reprojectionError by remember { mutableStateOf(0.0) }

    // 定期ポーリングによる UI 状態の同期 (100ms ごと)
    LaunchedEffect(Unit) {
        while (true) {
            isCapturing = NativeBridge.nativeIsCapturing()
            isDetectingBoard = NativeBridge.nativeIsDetectingBoard()
            autoCaptureEnabled = NativeBridge.nativeIsAutoCaptureEnabled()
            autoProgress = NativeBridge.nativeGetAutoCaptureProgress()
            isDiverse = NativeBridge.nativeIsAutoCaptureDiverse()
            isStable = NativeBridge.nativeIsAutoCaptureStable()
            sampleCount = NativeBridge.nativeGetSampleCount()
            isCalibrated = NativeBridge.nativeIsCalibrationFinished()
            reprojectionError = NativeBridge.nativeGetReprojectionError()
            delay(100)
        }
    }

    Box(modifier = Modifier.fillMaxSize()) {
        if (hasCameraPermission) {
            // 最背面: C++ / OpenGL ES 3.1 レンダリングを行う SurfaceView
            AndroidView(
                factory = { ctx ->
                    SurfaceView(ctx).apply {
                        holder.addCallback(object : SurfaceHolder.Callback {
                            override fun surfaceCreated(holder: SurfaceHolder) {
                                NativeBridge.nativeSurfaceCreated(holder.surface)
                                isCapturing = NativeBridge.nativeIsCapturing()
                            }

                            override fun surfaceChanged(
                                holder: SurfaceHolder,
                                format: Int,
                                width: Int,
                                height: Int
                            ) {
                                NativeBridge.nativeSurfaceChanged(width, height)
                            }

                            override fun surfaceDestroyed(holder: SurfaceHolder) {
                                NativeBridge.nativeSurfaceDestroyed()
                                isCapturing = false
                            }
                        })
                    }
                },
                modifier = Modifier.fillMaxSize()
            )
        } else {
            // カメラ権限要求画面
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .background(Color.Black),
                contentAlignment = Alignment.Center
            ) {
                Column(horizontalAlignment = Alignment.CenterHorizontally) {
                    Text(
                        text = "カメラへのアクセス許可が必要です",
                        color = Color.White
                    )
                    Spacer(modifier = Modifier.height(16.dp))
                    Button(onClick = { permissionLauncher.launch(Manifest.permission.CAMERA) }) {
                        Text("許可をリクエスト")
                    }
                }
            }
        }

        // 最前面: アプリバー（半透明）
        TopAppBar(
            title = {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("calib", fontWeight = FontWeight.Bold, fontSize = 20.sp)
                    Spacer(modifier = Modifier.width(8.dp))
                    if (isCalibrated) {
                        Surface(
                            color = Color(0xFF2E7D32),
                            shape = RoundedCornerShape(12.dp)
                        ) {
                            Text(
                                text = "誤差: %.2f px".format(reprojectionError),
                                color = Color.White,
                                fontSize = 12.sp,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }
                    }
                }
            },
            actions = {
                // 入力設定ボタン
                IconButton(onClick = { showInputSheet = true }) {
                    Icon(
                        imageVector = Icons.Default.Tune,
                        contentDescription = "入力設定",
                        tint = Color.White
                    )
                }
                // 較正パネルボタン
                IconButton(onClick = { showCalibSheet = true }) {
                    Icon(
                        imageVector = Icons.Default.CameraAlt,
                        contentDescription = "較正設定",
                        tint = if (isDetectingBoard) Color(0xFF64B5F6) else Color.White
                    )
                }
            },
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = Color.Black.copy(alpha = 0.5f),
                titleContentColor = Color.White
            )
        )

        // 中央上部: 自動キャプチャの進捗および静止状態表示（ボード検出中のみ）
        if (isDetectingBoard && autoCaptureEnabled) {
            Surface(
                modifier = Modifier
                    .align(Alignment.TopCenter)
                    .padding(top = 70.dp, start = 16.dp, end = 16.dp)
                    .fillMaxWidth(0.9f),
                shape = RoundedCornerShape(8.dp),
                color = Color.Black.copy(alpha = 0.7f)
            ) {
                Column(modifier = Modifier.padding(12.dp)) {
                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.SpaceBetween,
                        verticalAlignment = Alignment.CenterVertically
                    ) {
                        Text(
                            text = if (isStable) {
                                if (isDiverse) "静止状態: 安定 (多様性あり)" else "静止状態: 安定 (前回姿勢と類似)"
                            } else {
                                "静止判定中..."
                            },
                            fontSize = 13.sp,
                            fontWeight = FontWeight.SemiBold,
                            color = if (isStable) {
                                if (isDiverse) Color(0xFF81C784) else Color(0xFFFFF176)
                            } else Color(0xFFB0BEC5)
                        )
                        Text(
                            text = "標本: %d 枚".format(sampleCount),
                            fontSize = 13.sp,
                            color = Color.White
                        )
                    }
                    Spacer(modifier = Modifier.height(6.dp))
                    LinearProgressIndicator(
                        progress = { autoProgress },
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(8.dp),
                        color = if (isDiverse) Color(0xFF81C784) else Color(0xFF64B5F6),
                        trackColor = Color.DarkGray
                    )
                }
            }
        }

        // 最下部: メイン操作バー
        Surface(
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .fillMaxWidth(),
            color = Color.Black.copy(alpha = 0.65f)
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 12.dp),
                horizontalArrangement = Arrangement.SpaceEvenly,
                verticalAlignment = Alignment.CenterVertically
            ) {
                // キャプチャ開始 / 停止ボタン
                Button(
                    onClick = {
                        if (isCapturing) {
                            NativeBridge.nativeStopCapture()
                            isCapturing = false
                        } else {
                            val ok = NativeBridge.nativeStartCapture()
                            isCapturing = ok
                        }
                    },
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (isCapturing) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.primary
                    ),
                    modifier = Modifier.height(48.dp)
                ) {
                    Icon(
                        imageVector = if (isCapturing) Icons.Default.Stop else Icons.Default.PlayArrow,
                        contentDescription = null
                    )
                    Spacer(modifier = Modifier.width(6.dp))
                    Text(if (isCapturing) "停止" else "開始")
                }

                // 標本手動記録ボタン (ボード検出時のみ)
                if (isDetectingBoard) {
                    FilledTonalButton(
                        onClick = {
                            NativeBridge.nativeRecordSnapshot()
                            sampleCount = NativeBridge.nativeGetSampleCount()
                        },
                        modifier = Modifier.height(48.dp)
                    ) {
                        Icon(imageVector = Icons.Default.AddPhotoAlternate, contentDescription = null)
                        Spacer(modifier = Modifier.width(6.dp))
                        Text("記録 (%d)".format(sampleCount))
                    }
                }

                // 較正実行ボタン (標本が3枚以上ある場合)
                if (sampleCount >= 3) {
                    Button(
                        onClick = {
                            reprojectionError = NativeBridge.nativeCalibrate()
                            isCalibrated = NativeBridge.nativeIsCalibrationFinished()
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF388E3C)),
                        modifier = Modifier.height(48.dp)
                    ) {
                        Icon(imageVector = Icons.Default.Check, contentDescription = null)
                        Spacer(modifier = Modifier.width(6.dp))
                        Text("較正実行")
                    }
                }
            }
        }
    }

    // --- 入力設定ボトムシート ---
    if (showInputSheet) {
        ModalBottomSheet(
            onDismissRequest = { showInputSheet = false },
            containerColor = MaterialTheme.colorScheme.surface
        ) {
            InputSettingsContent()
        }
    }

    // --- 較正設定ボトムシート ---
    if (showCalibSheet) {
        ModalBottomSheet(
            onDismissRequest = { showCalibSheet = false },
            containerColor = MaterialTheme.colorScheme.surface
        ) {
            CalibrationSettingsContent(
                sampleCount = sampleCount,
                onClearSamples = {
                    NativeBridge.nativeClearSnapshots()
                    sampleCount = 0
                    isCalibrated = false
                }
            )
        }
    }
}

//
// 入力設定ボトムシートの内容
//
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun InputSettingsContent() {
    val scrollState = rememberScrollState()

    var preferenceIndex by remember { mutableStateOf(NativeBridge.nativeGetPreferenceIndex()) }
    val preferenceCount = remember { NativeBridge.nativeGetPreferenceCount() }
    val preferenceNames = remember {
        (0 until preferenceCount).map { NativeBridge.nativeGetPreferenceName(it) }
    }

    var fovX by remember { mutableStateOf(NativeBridge.nativeGetFovX()) }
    var fovY by remember { mutableStateOf(NativeBridge.nativeGetFovY()) }
    var heading by remember { mutableStateOf(NativeBridge.nativeGetEulerHeading()) }
    var pitch by remember { mutableStateOf(NativeBridge.nativeGetEulerPitch()) }
    var roll by remember { mutableStateOf(NativeBridge.nativeGetEulerRoll()) }

    var focal by remember { mutableStateOf(NativeBridge.nativeGetFocal()) }
    val focalMin = remember { NativeBridge.nativeGetFocalMin() }
    val focalMax = remember { NativeBridge.nativeGetFocalMax() }

    var prefExpanded by remember { mutableStateOf(false) }

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 24.dp, vertical = 8.dp)
            .verticalScroll(scrollState)
    ) {
        Text(
            text = "入力・投影設定",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            color = Color.White
        )
        Spacer(modifier = Modifier.height(16.dp))

        // 投影方式ドロップダウン
        Text("投影方式", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Spacer(modifier = Modifier.height(4.dp))
        ExposedDropdownMenuBox(
            expanded = prefExpanded,
            onExpandedChange = { prefExpanded = !prefExpanded }
        ) {
            OutlinedTextField(
                value = preferenceNames.getOrElse(preferenceIndex) { "" },
                onValueChange = {},
                readOnly = true,
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = prefExpanded) },
                modifier = Modifier
                    .menuAnchor(MenuAnchorType.PrimaryNotEditable)
                    .fillMaxWidth()
            )
            ExposedDropdownMenu(
                expanded = prefExpanded,
                onDismissRequest = { prefExpanded = false }
            ) {
                preferenceNames.forEachIndexed { index, name ->
                    DropdownMenuItem(
                        text = { Text(name) },
                        onClick = {
                            preferenceIndex = index
                            NativeBridge.nativeSelectPreference(index)
                            fovX = NativeBridge.nativeGetFovX()
                            fovY = NativeBridge.nativeGetFovY()
                            prefExpanded = false
                        }
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(16.dp))

        // 画角 (FOV)
        Text("画角: X = %.1f°, Y = %.1f°".format(fovX, fovY), fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Text("水平画角 (X)", fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = fovX,
            onValueChange = {
                fovX = it
                NativeBridge.nativeSetFov(fovX, fovY)
            },
            valueRange = 10f..360f
        )
        Text("垂直画角 (Y)", fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = fovY,
            onValueChange = {
                fovY = it
                NativeBridge.nativeSetFov(fovX, fovY)
            },
            valueRange = 10f..360f
        )

        Spacer(modifier = Modifier.height(12.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(12.dp))

        // 姿勢 (Euler)
        Text("姿勢 (オイラー角)", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Text("方位 (Heading): %.1f°".format(heading), fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = heading,
            onValueChange = {
                heading = it
                NativeBridge.nativeSetEuler(heading, pitch, roll)
            },
            valueRange = -180f..180f
        )
        Text("仰角 (Pitch): %.1f°".format(pitch), fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = pitch,
            onValueChange = {
                pitch = it
                NativeBridge.nativeSetEuler(heading, pitch, roll)
            },
            valueRange = -180f..180f
        )
        Text("傾斜 (Roll): %.1f°".format(roll), fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = roll,
            onValueChange = {
                roll = it
                NativeBridge.nativeSetEuler(heading, pitch, roll)
            },
            valueRange = -180f..180f
        )

        Spacer(modifier = Modifier.height(12.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(12.dp))

        // 焦点距離
        Text("焦点距離: %.1f".format(focal), fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Slider(
            value = focal,
            onValueChange = {
                focal = it
                NativeBridge.nativeSetFocal(focal)
            },
            valueRange = focalMin..focalMax
        )

        Spacer(modifier = Modifier.height(12.dp))

        // 復帰ボタン
        OutlinedButton(
            onClick = {
                NativeBridge.nativeResetPose()
                heading = NativeBridge.nativeGetEulerHeading()
                pitch = NativeBridge.nativeGetEulerPitch()
                roll = NativeBridge.nativeGetEulerRoll()
                focal = NativeBridge.nativeGetFocal()
            },
            modifier = Modifier.fillMaxWidth()
        ) {
            Icon(Icons.Default.Refresh, contentDescription = null)
            Spacer(modifier = Modifier.width(6.dp))
            Text("姿勢・焦点距離を初期値へ戻す")
        }

        Spacer(modifier = Modifier.height(24.dp))
    }
}

//
// 較正設定ボトムシートの内容
//
@Composable
fun CalibrationSettingsContent(
    sampleCount: Int,
    onClearSamples: () -> Unit
) {
    val scrollState = rememberScrollState()

    var detectBoard by remember { mutableStateOf(NativeBridge.nativeIsDetectingBoard()) }
    var autoCapture by remember { mutableStateOf(NativeBridge.nativeIsAutoCaptureEnabled()) }

    var checkerW by remember { mutableStateOf(NativeBridge.nativeGetCheckerWidth()) }
    var checkerH by remember { mutableStateOf(NativeBridge.nativeGetCheckerHeight()) }

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .padding(horizontal = 24.dp, vertical = 8.dp)
            .verticalScroll(scrollState)
    ) {
        Text(
            text = "カメラ較正設定",
            fontSize = 20.sp,
            fontWeight = FontWeight.Bold,
            color = Color.White
        )
        Spacer(modifier = Modifier.height(16.dp))

        // ボード検出トグル
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text("ChArUco Board 検出", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
                Text("カメラ映像からボードを検出して較正標本を取得", fontSize = 12.sp, color = Color.Gray)
            }
            Switch(
                checked = detectBoard,
                onCheckedChange = {
                    detectBoard = it
                    NativeBridge.nativeSetDetectBoard(it)
                }
            )
        }

        Spacer(modifier = Modifier.height(12.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(12.dp))

        // 自動キャプチャトグル
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text("自動キャプチャ", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
                Text("ボードが静止した瞬間に自動で標本を記録", fontSize = 12.sp, color = Color.Gray)
            }
            Switch(
                checked = autoCapture,
                enabled = detectBoard,
                onCheckedChange = {
                    autoCapture = it
                    NativeBridge.nativeSetAutoCaptureEnabled(it)
                }
            )
        }

        Spacer(modifier = Modifier.height(12.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(12.dp))

        // マス目数設定
        Text("ボードのマス目数", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Text("横のマス数: %d".format(checkerW), fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = checkerW.toFloat(),
            onValueChange = {
                checkerW = it.toInt()
                NativeBridge.nativeSetCheckerSize(checkerW, checkerH)
            },
            valueRange = 3f..15f,
            steps = 11
        )

        Text("縦のマス数: %d".format(checkerH), fontSize = 12.sp, color = Color.Gray)
        Slider(
            value = checkerH.toFloat(),
            onValueChange = {
                checkerH = it.toInt()
                NativeBridge.nativeSetCheckerSize(checkerW, checkerH)
            },
            valueRange = 3f..15f,
            steps = 11
        )

        Spacer(modifier = Modifier.height(16.dp))
        HorizontalDivider()
        Spacer(modifier = Modifier.height(16.dp))

        // 標本クリアボタン
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("記録済み標本数: %d 枚".format(sampleCount), fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
            OutlinedButton(
                onClick = onClearSamples,
                colors = ButtonDefaults.outlinedButtonColors(contentColor = MaterialTheme.colorScheme.error)
            ) {
                Icon(Icons.Default.Delete, contentDescription = null)
                Spacer(modifier = Modifier.width(4.dp))
                Text("全削除")
            }
        }

        Spacer(modifier = Modifier.height(24.dp))
    }
}
