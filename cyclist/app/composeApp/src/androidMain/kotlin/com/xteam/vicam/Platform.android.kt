package com.xteam.vicam

import android.content.Context
import android.os.Build

class AndroidPlatform : Platform {
    override val name: String = "Android ${Build.VERSION.SDK_INT}"
}

actual fun getPlatform(): Platform = AndroidPlatform()

class AndroidSettingsProvider(private val context: Context) : SettingsProvider {
    private val prefs = context.getSharedPreferences("vicam_prefs", Context.MODE_PRIVATE)

    override fun getString(key: String, defaultValue: String): String {
        return prefs.getString(key, defaultValue) ?: defaultValue
    }

    override fun putString(key: String, value: String) {
        prefs.edit().putString(key, value).apply()
    }
}

object SettingsProviderHolder {
    lateinit var provider: SettingsProvider
}

actual fun getSettingsProvider(): SettingsProvider = SettingsProviderHolder.provider
