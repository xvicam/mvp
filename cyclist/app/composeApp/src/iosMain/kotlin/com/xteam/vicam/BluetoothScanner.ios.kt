package com.xteam.vicam

import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.emptyFlow

private class IosBluetoothScanner : BluetoothScanner {
    override val isScanning = MutableStateFlow(false)
    override val discoveredDevices = MutableStateFlow<List<BicycleDevice>>(emptyList())
    override val crashEvents: Flow<CrashEvent> = emptyFlow()

    override fun startScanning(filterUuid: String?) {
// Todo: implement
    }

    override fun stopScanning() {
        // No-op
    }

    override fun connect(device: BicycleDevice) {
        // No-op
    }
}

actual fun getBluetoothScanner(): BluetoothScanner = IosBluetoothScanner()

