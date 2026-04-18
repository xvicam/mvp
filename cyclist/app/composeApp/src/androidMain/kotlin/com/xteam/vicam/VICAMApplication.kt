package com.xteam.vicam

import android.app.Application

class VICAMApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        BluetoothScannerProvider.scanner = AndroidBluetoothScanner(this)
        ContactManagerProvider.manager = AndroidContactManager(this)
    }
}