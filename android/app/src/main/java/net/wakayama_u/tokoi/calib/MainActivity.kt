package net.wakayama_u.tokoi.calib

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.core.content.FileProvider
import java.io.File
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

    // オーバーレイ（UIバー）の表示・非表示フラグ（画面タップでトグル）
    var showOverlay by remember { mutableStateOf(true) }

    // ボトムシートの表示状態（較正設定のみ）
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

    // 一括ポーリングによる UI 状態の同期 (Mutex 競合を解消)
    val statusArray = remember { FloatArray(9) }
    LaunchedEffect(Unit) {
        while (true) {
            NativeBridge.nativeGetStatus(statusArray)
            isCapturing = statusArray[0] > 0.5f
            isDetectingBoard = statusArray[1] > 0.5f
            autoCaptureEnabled = statusArray[2] > 0.5f
            autoProgress = statusArray[3]
            isDiverse = statusArray[4] > 0.5f
            isStable = statusArray[5] > 0.5f
            sampleCount = statusArray[6].toInt()
            isCalibrated = statusArray[7] > 0.5f
            reprojectionError = statusArray[8].toDouble()
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

            // 画面タップ検知用の透明レイヤー（SurfaceView の前面、UI コントロールの背面）
            Box(
                modifier = Modifier
                    .fillMaxSize()
                    .clickable(
                        interactionSource = remember { MutableInteractionSource() },
                        indication = null
                    ) {
                        showOverlay = !showOverlay
                    }
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

        // 最前面: アプリバー（半透明、タップでトグル表示）
        AnimatedVisibility(
            visible = showOverlay,
            enter = fadeIn() + slideInVertically(initialOffsetY = { -it }),
            exit = fadeOut() + slideOutVertically(targetOffsetY = { -it }),
            modifier = Modifier.align(Alignment.TopCenter)
        ) {
            TopAppBar(
                title = {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Text("calib", fontWeight = FontWeight.Bold, fontSize = 20.sp)

                        // 標本枚数バッジ
                        Surface(
                            color = Color(0xFF333333),
                            shape = RoundedCornerShape(12.dp)
                        ) {
                            Text(
                                text = "標本: %d 枚".format(sampleCount),
                                color = Color.White,
                                fontSize = 12.sp,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }

                        // ボード検出状態バッジ
                        Surface(
                            color = if (isDetectingBoard) Color(0xFF1976D2) else Color(0xFF424242),
                            shape = RoundedCornerShape(12.dp)
                        ) {
                            Text(
                                text = if (isDetectingBoard) "ボード検出中" else "ボード未検出",
                                color = Color.White,
                                fontSize = 12.sp,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }

                        // 較正誤差バッジ（較正済みの場合）
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
                    // 標本手動記録ボタン (ボード検出時)
                    if (isDetectingBoard) {
                        FilledTonalButton(
                            onClick = {
                                NativeBridge.nativeRecordSnapshot()
                                sampleCount = NativeBridge.nativeGetSampleCount()
                            },
                            modifier = Modifier.height(38.dp),
                            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp)
                        ) {
                            Icon(
                                imageVector = Icons.Default.AddPhotoAlternate,
                                contentDescription = null,
                                modifier = Modifier.size(18.dp)
                            )
                            Spacer(modifier = Modifier.width(4.dp))
                            Text("記録", fontSize = 13.sp, lineHeight = 16.sp)
                        }
                    }

                    // 較正実行ボタン (標本が3枚以上ある場合)
                    if (sampleCount >= 3) {
                        Spacer(modifier = Modifier.width(6.dp))
                        Button(
                            onClick = {
                                reprojectionError = NativeBridge.nativeCalibrate()
                                isCalibrated = NativeBridge.nativeIsCalibrationFinished()
                            },
                            colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF388E3C)),
                            modifier = Modifier.height(38.dp),
                            contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp)
                        ) {
                            Icon(
                                imageVector = Icons.Default.Check,
                                contentDescription = null,
                                modifier = Modifier.size(18.dp)
                            )
                            Spacer(modifier = Modifier.width(4.dp))
                            Text("較正実行", fontSize = 13.sp, lineHeight = 16.sp)
                        }
                    }

                    // 較正完了時の共有・保存ボタン (mfcapture 連携用)
                    if (isCalibrated) {
                        Spacer(modifier = Modifier.width(4.dp))
                        IconButton(onClick = {
                            try {
                                val file = File(context.cacheDir, "calibration.json")
                                val ok = NativeBridge.nativeSaveParameters(file.absolutePath)
                                if (ok && file.exists()) {
                                    val uri = FileProvider.getUriForFile(
                                        context,
                                        "${context.packageName}.fileprovider",
                                        file
                                    )
                                    val shareIntent = Intent(Intent.ACTION_SEND).apply {
                                        type = "application/json"
                                        putExtra(Intent.EXTRA_STREAM, uri)
                                        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                                    }
                                    context.startActivity(Intent.createChooser(shareIntent, "較正パラメータを保存・共有"))
                                }
                            } catch (e: Exception) {
                                e.printStackTrace()
                            }
                        }) {
                            Icon(
                                imageVector = Icons.Default.Share,
                                contentDescription = "較正パラメータを保存・共有",
                                tint = Color(0xFF81C784)
                            )
                        }
                    }

                    Spacer(modifier = Modifier.width(4.dp))

                    // 較正設定ボタン
                    IconButton(onClick = { showCalibSheet = true }) {
                        Icon(
                            imageVector = Icons.Default.CameraAlt,
                            contentDescription = "較正設定",
                            tint = if (isDetectingBoard) Color(0xFF64B5F6) else Color.White
                        )
                    }
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = Color.Black.copy(alpha = 0.55f),
                    titleContentColor = Color.White
                )
            )
        }

        // 中央上部: 自動キャプチャの進捗および静止状態表示（ボード検出中のみ）
        AnimatedVisibility(
            visible = showOverlay && isDetectingBoard && autoCaptureEnabled,
            enter = fadeIn(),
            exit = fadeOut(),
            modifier = Modifier
                .align(Alignment.TopCenter)
                .padding(top = 64.dp, start = 16.dp, end = 16.dp)
                .fillMaxWidth(0.7f)
        ) {
            Surface(
                shape = RoundedCornerShape(8.dp),
                color = Color.Black.copy(alpha = 0.7f)
            ) {
                Column(modifier = Modifier.padding(horizontal = 12.dp, vertical = 6.dp)) {
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
                            fontSize = 12.sp,
                            fontWeight = FontWeight.SemiBold,
                            color = if (isStable) {
                                if (isDiverse) Color(0xFF81C784) else Color(0xFFFFF176)
                            } else Color(0xFFB0BEC5)
                        )
                    }
                    Spacer(modifier = Modifier.height(4.dp))
                    LinearProgressIndicator(
                        progress = { autoProgress },
                        modifier = Modifier
                            .fillMaxWidth()
                            .height(6.dp),
                        color = if (isDiverse) Color(0xFF81C784) else Color(0xFF64B5F6),
                        trackColor = Color.DarkGray
                    )
                }
            }
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
// 較正設定ボトムシートの内容
//
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun CalibrationSettingsContent(
    sampleCount: Int,
    onClearSamples: () -> Unit
) {
    val scrollState = rememberScrollState()

    var detectBoard by remember { mutableStateOf(NativeBridge.nativeIsDetectingBoard()) }
    var autoCapture by remember { mutableStateOf(NativeBridge.nativeIsAutoCaptureEnabled()) }

    var dictIndex by remember {
        val current = NativeBridge.nativeGetDictionaryName()
        val count = NativeBridge.nativeGetDictionaryCount()
        val names = (0 until count).map { NativeBridge.nativeGetDictionaryNameByIndex(it) }
        val idx = names.indexOf(current)
        mutableStateOf(if (idx >= 0) idx else 0)
    }
    val dictCount = remember { NativeBridge.nativeGetDictionaryCount() }
    val dictNames = remember {
        (0 until dictCount).map { NativeBridge.nativeGetDictionaryNameByIndex(it) }
    }
    var dictExpanded by remember { mutableStateOf(false) }

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

        // マーカー辞書選択
        Text("マーカー辞書", fontWeight = FontWeight.SemiBold, fontSize = 14.sp)
        Spacer(modifier = Modifier.height(4.dp))
        ExposedDropdownMenuBox(
            expanded = dictExpanded,
            onExpandedChange = { dictExpanded = !dictExpanded }
        ) {
            OutlinedTextField(
                value = dictNames.getOrElse(dictIndex) { "" },
                onValueChange = {},
                readOnly = true,
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = dictExpanded) },
                modifier = Modifier
                    .menuAnchor(MenuAnchorType.PrimaryNotEditable)
                    .fillMaxWidth()
            )
            ExposedDropdownMenu(
                expanded = dictExpanded,
                onDismissRequest = { dictExpanded = false }
            ) {
                dictNames.forEachIndexed { index, name ->
                    DropdownMenuItem(
                        text = { Text(name) },
                        onClick = {
                            dictIndex = index
                            NativeBridge.nativeSetDictionary(name)
                            dictExpanded = false
                        }
                    )
                }
            }
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
