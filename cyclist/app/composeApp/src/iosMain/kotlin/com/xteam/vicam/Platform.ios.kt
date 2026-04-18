package com.xteam.vicam

import platform.UIKit.UIDevice
import platform.Foundation.NSUserDefaults

class IOSPlatform : Platform {
    override val name: String = UIDevice.currentDevice.systemName() + " " + UIDevice.currentDevice.systemVersion
}

actual fun getPlatform(): Platform = IOSPlatform()

class IosSettingsProvider : SettingsProvider {
    private val userDefaults = NSUserDefaults.standardUserDefaults

    override fun getString(key: String, defaultValue: String): String {
        return userDefaults.stringForKey(key) ?: defaultValue
    }

    override fun putString(key: String, value: String) {
        userDefaults.setObject(value, key)
    }
}

actual fun getSettingsProvider(): SettingsProvider = IosSettingsProvider()
