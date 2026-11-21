package org.levimc.launcher.util

import android.content.Context
import android.content.SharedPreferences
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.Process
import android.util.Log
import org.json.JSONObject
import org.levimc.launcher.core.minecraft.LauncherApplication
import androidx.core.content.edit
import android.widget.Toast
import android.view.Gravity
import org.levimc.launcher.R
import androidx.annotation.Keep

@Keep
object NativeBridgeHelper {
    @Volatile
    private var gxcoreLoaded = false

    @Volatile
    @JvmStatic
    var lastGxCoreError: Throwable? = null
        private set

    @JvmStatic
    fun getAppContext(): Context {
        return LauncherApplication.context
    }

    @JvmStatic
    fun ensureGxCoreLoaded(): Boolean {
        if (gxcoreLoaded) {
            return true
        }
        return synchronized(this) {
            if (gxcoreLoaded) {
                true
            } else {
                try {
                    System.loadLibrary("gxcore")
                    gxcoreLoaded = true
                    lastGxCoreError = null
                    true
                } catch (error: Throwable) {
                    lastGxCoreError = error
                    Log.e("EON", "Failed to load gxcore", error)
                    logNativeLibraryDiagnostics()
                    false
                }
            }
        }
    }

    private fun logNativeLibraryDiagnostics() {
        val context = getAppContext()
        val nativeLibraryDir = context.applicationInfo.nativeLibraryDir
        val libraries = java.io.File(nativeLibraryDir)
            .listFiles()
            ?.filter { it.isFile && it.name.endsWith(".so") }
            ?.map { it.name }
            ?.sorted()
            ?: emptyList()
        val primaryAbi = if (Process.is64Bit()) {
            Build.SUPPORTED_64_BIT_ABIS.firstOrNull()
        } else {
            Build.SUPPORTED_32_BIT_ABIS.firstOrNull()
        }
        Log.e(
            "EON",
            "gxcore load diagnostics: nativeLibraryDir=$nativeLibraryDir, " +
                "libraries=$libraries, primaryAbi=$primaryAbi"
        )
    }

    @JvmStatic
    fun scanImage(inputPath: String): Boolean {
        if (!ensureGxCoreLoaded()) {
            return false
        }
        return nativeScanImage(inputPath)
    }

    @JvmStatic
    fun rewriteImage(inputPath: String, outputPath: String): Boolean {
        if (!ensureGxCoreLoaded()) {
            return false
        }
        return nativeRewriteImage(inputPath, outputPath)
    }

    @JvmStatic
    fun bootstrapGxCore(): Boolean {
        if (!ensureGxCoreLoaded()) {
            return false
        }
        return try {
            nativeBootstrapGxCore()
            true
        } catch (_: Throwable) {
            false
        }
    }

    @JvmStatic
    fun loadMinecraftRuntime(libraryPath: String) {
        if (!ensureGxCoreLoaded()) {
            throw UnsatisfiedLinkError("Failed to load gxcore for the Minecraft runtime")
        }
        nativeLoadMinecraftRuntime(libraryPath)?.let { error ->
            throw UnsatisfiedLinkError(error)
        }
    }

    @JvmStatic
    fun getPrefs(name: String): SharedPreferences {
        return getAppContext().getSharedPreferences(name, Context.MODE_PRIVATE)
    }

    @JvmStatic
    fun getBoolean(prefsName: String, key: String, defaultValue: Boolean = false): Boolean {
        return try {
            getPrefs(prefsName).getBoolean(key, defaultValue)
        } catch (t: Throwable) {
            defaultValue
        }
    }

    @JvmStatic
    fun putBoolean(prefsName: String, key: String, value: Boolean) {
        try {
            getPrefs(prefsName).edit { putBoolean(key, value) }
        } catch (_: Throwable) {
        }
    }

    @JvmStatic
    fun getString(prefsName: String, key: String, defaultValue: String? = null): String? {
        return try {
            getPrefs(prefsName).getString(key, defaultValue)
        } catch (t: Throwable) {
            defaultValue
        }
    }

    @JvmStatic
    fun putString(prefsName: String, key: String, value: String) {
        try {
            getPrefs(prefsName).edit { putString(key, value) }
        } catch (_: Throwable) {
        }
    }

    @JvmStatic
    fun isLauncherManagedLoginEnabled(): Boolean {
        return try {
            val json = getString("feature_settings", "settings_json", null)
            if (!json.isNullOrEmpty()) {
                val obj = JSONObject(json)
                obj.optBoolean("launcherManagedMcLoginEnabled", false)
            } else {
                false
            }
        } catch (t: Throwable) {
            false
        }
    }

    @JvmStatic
    fun showInvalidLicenseOverlay() {
        val ctx = getAppContext()
        try {
            val handler = Handler(Looper.getMainLooper())
            handler.post {
                Toast.makeText(
                    ctx,
                    ctx.getString(R.string.invalid_license_detected_toast),
                    Toast.LENGTH_LONG
                ).apply {
                    setGravity(Gravity.TOP or Gravity.CENTER_HORIZONTAL, 0, 100)
                    show()
                }
            }
        } catch (t: Throwable) {
            t.printStackTrace()
        }
    }

    @JvmStatic
    private external fun nativeScanImage(inputPath: String): Boolean

    @JvmStatic
    private external fun nativeRewriteImage(inputPath: String, outputPath: String): Boolean

    @JvmStatic
    private external fun nativeBootstrapGxCore()

    @JvmStatic
    private external fun nativeLoadMinecraftRuntime(libraryPath: String): String?
}
