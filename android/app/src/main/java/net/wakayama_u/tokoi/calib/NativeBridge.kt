package net.wakayama_u.tokoi.calib

import android.content.res.AssetManager
import android.view.Surface

object NativeBridge {
    init {
        try {
            System.loadLibrary("calib")
        } catch (e: UnsatisfiedLinkError) {
            e.printStackTrace()
        }
    }

    external fun nativeInit(assetManager: AssetManager, internalPath: String)
    external fun nativeSurfaceCreated(surface: Surface)
    external fun nativeSurfaceChanged(width: Int, height: Int)
    external fun nativeSurfaceDestroyed()
    external fun nativeStartCapture(): Boolean
    external fun nativeStopCapture()
    external fun nativeIsCapturing(): Boolean

    // --- 投影方式 (Preferences) ---
    external fun nativeGetPreferenceCount(): Int
    external fun nativeGetPreferenceName(index: Int): String
    external fun nativeGetPreferenceIndex(): Int
    external fun nativeSelectPreference(index: Int)

    // --- 画角・中心 ---
    external fun nativeGetFovX(): Float
    external fun nativeGetFovY(): Float
    external fun nativeSetFov(x: Float, y: Float)

    external fun nativeGetCenterX(): Float
    external fun nativeGetCenterY(): Float
    external fun nativeSetCenter(x: Float, y: Float)

    // --- 姿勢 ---
    external fun nativeGetEulerHeading(): Float
    external fun nativeGetEulerPitch(): Float
    external fun nativeGetEulerRoll(): Float
    external fun nativeSetEuler(heading: Float, pitch: Float, roll: Float)

    // --- 焦点距離 ---
    external fun nativeGetFocal(): Float
    external fun nativeSetFocal(focal: Float)
    external fun nativeGetFocalMin(): Float
    external fun nativeGetFocalMax(): Float
    external fun nativeResetPose()

    // --- 較正 (Calibration) ---
    external fun nativeIsDetectingBoard(): Boolean
    external fun nativeSetDetectBoard(enabled: Boolean)

    external fun nativeRecordSnapshot(): Boolean
    external fun nativeClearSnapshots()
    external fun nativeGetSampleCount(): Int
    external fun nativeCalibrate(): Double
    external fun nativeIsCalibrationFinished(): Boolean
    external fun nativeGetReprojectionError(): Double

    external fun nativeIsAutoCaptureEnabled(): Boolean
    external fun nativeSetAutoCaptureEnabled(enabled: Boolean)
    external fun nativeGetAutoCaptureProgress(): Float
    external fun nativeIsAutoCaptureDiverse(): Boolean
    external fun nativeIsAutoCaptureStable(): Boolean
    external fun nativeGetCurrentMotion(): Float

    external fun nativeGetDictionaryName(): String
    external fun nativeSetDictionary(name: String)
    external fun nativeGetDictionaryCount(): Int
    external fun nativeGetDictionaryNameByIndex(index: Int): String

    external fun nativeGetCheckerWidth(): Int
    external fun nativeGetCheckerHeight(): Int
    external fun nativeSetCheckerSize(w: Int, h: Int)

    // 一括状態取得 (Mutex 競合解消用)
    external fun nativeGetStatus(outStatus: FloatArray)

    // フレーム解像度
    external fun nativeGetFrameWidth(): Int
    external fun nativeGetFrameHeight(): Int

    // 較正パラメータ保存
    external fun nativeSaveParameters(path: String): Boolean
}
