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
}
