package com.xteam.vicam

import kotlinx.coroutines.flow.StateFlow

interface BluetoothScanner {
    val isScanning: StateFlow<Boolean>
    val discoveredDevices: StateFlow<List<BicycleDevice>>
    fun startScanning()
    fun stopScanning()
}

expect fun getBluetoothScanner(): BluetoothScanner