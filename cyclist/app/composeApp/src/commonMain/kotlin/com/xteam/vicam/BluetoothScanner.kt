package com.xteam.vicam

import kotlinx.coroutines.flow.StateFlow

data class BicycleDevice(
    val name: String,
    val address: String,
    val rssi: Int,
    val serviceUuid: String? = null
)

interface BluetoothScanner {
    val isScanning: StateFlow<Boolean>
    val discoveredDevices: StateFlow<List<BicycleDevice>>
    fun startScanning(filterUuid: String? = null)
    fun stopScanning()
    fun connect(device: BicycleDevice)
}

expect fun getBluetoothScanner(): BluetoothScanner