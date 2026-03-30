package com.xteam.vicam

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import androidx.core.content.getSystemService
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

class AndroidBluetoothScanner(context: Context) : BluetoothScanner {
    private val bluetoothManager = context.getSystemService<BluetoothManager>()
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager?.adapter
    private val scanner = bluetoothAdapter?.bluetoothLeScanner

    private val _isScanning = MutableStateFlow(false)
    override val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()

    private val _discoveredDevices = MutableStateFlow<List<BicycleDevice>>(emptyList())
    override val discoveredDevices: StateFlow<List<BicycleDevice>> = _discoveredDevices.asStateFlow()

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val name = device.name ?: "Unknown Device"
            val address = device.address
            val rssi = result.rssi

            // For this app, let's assume cyclists have "Cyclist" in their name or use a specific service UUID
            // For now, just filtering by name "Cyclist" to make it easy to test
            if (name.contains("Cyclist", ignoreCase = true) || name.contains("ESP32", ignoreCase = true)) {
                _discoveredDevices.update { current ->
                    val existing = current.find { it.address == address }
                    if (existing != null) {
                        current.map { if (it.address == address) it.copy(rssi = rssi) else it }
                    } else {
                        current + BicycleDevice(name, address, rssi)
                    }
                }
            }
        }
    }

    @SuppressLint("MissingPermission")
    override fun startScanning() {
        if (_isScanning.value) return
        _discoveredDevices.value = emptyList()
        scanner?.startScan(scanCallback)
        _isScanning.value = true
    }

    @SuppressLint("MissingPermission")
    override fun stopScanning() {
        if (!_isScanning.value) return
        scanner?.stopScan(scanCallback)
        _isScanning.value = false
    }
}

// We need a way to provide the context to the scanner. 
// For simplicity in this demo, we'll use a static reference set in MainActivity.
object BluetoothScannerProvider {
    lateinit var scanner: BluetoothScanner
}

actual fun getBluetoothScanner(): BluetoothScanner = BluetoothScannerProvider.scanner
