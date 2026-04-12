package com.xteam.vicam

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.*
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import android.util.Log
import androidx.core.content.getSystemService
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import java.util.*

private const val TAG = "BluetoothScanner"

class AndroidBluetoothScanner(private val context: Context) : BluetoothScanner {
    private val bluetoothAdapter: BluetoothAdapter? = context.getSystemService<BluetoothManager>()?.adapter
    private val scanner = bluetoothAdapter?.bluetoothLeScanner
    private var bluetoothGatt: BluetoothGatt? = null

    // ESP32 UUIDs from the Arduino script
    private val SERVICE_UUID = UUID.fromString("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e1")
    private val CHARACTERISTIC_UUID = UUID.fromString("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e2")
    private val DESCRIPTOR_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    private val _isScanning = MutableStateFlow(false)
    override val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()

    private val _discoveredDevices = MutableStateFlow<List<BicycleDevice>>(emptyList())
    override val discoveredDevices: StateFlow<List<BicycleDevice>> = _discoveredDevices.asStateFlow()

    private val lastSeenMap = mutableMapOf<String, Long>()
    private val mainHandler = Handler(Looper.getMainLooper())

    private val cleanupRunnable = object : Runnable {
        override fun run() {
            if (!_isScanning.value) return
            
            val now = System.currentTimeMillis()
            val timeout = 8000L // 8 seconds timeout
            
            val expiredAddresses = lastSeenMap.filter { now - it.value > timeout }.keys
            if (expiredAddresses.isNotEmpty()) {
                expiredAddresses.forEach { lastSeenMap.remove(it) }
                _discoveredDevices.update { current ->
                    current.filter { it.address !in expiredAddresses }
                }
                Log.d(TAG, "Removed expired devices: $expiredAddresses")
            }
            
            mainHandler.postDelayed(this, 2000) // Check every 2 seconds
        }
    }

    private val bondStateReceiver = object : BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == BluetoothDevice.ACTION_BOND_STATE_CHANGED) {
                val device = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE, BluetoothDevice::class.java)
                } else {
                    @Suppress("DEPRECATION")
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
                }
                val bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR)
                
                Log.d(TAG, "Bond state changed for ${device?.address}: $bondState")
                
                if (bondState == BluetoothDevice.BOND_BONDED) {
                    Log.d(TAG, "Device bonded! Discovering services...")
                    bluetoothGatt?.discoverServices()
                }
            }
        }
    }

    init {
        val filter = IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
        context.registerReceiver(bondStateReceiver, filter)
    }

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val name = device.name ?: "Unknown Device"
            val scanRecord = result.scanRecord
            val serviceUuids = scanRecord?.serviceUuids?.map { it.uuid } ?: emptyList()
            
            val isTarget = name.contains("Cyclist", ignoreCase = true) || 
                           name.contains("VICAM", ignoreCase = true) ||
                           serviceUuids.contains(SERVICE_UUID)

            if (isTarget) {
                val now = System.currentTimeMillis()
                lastSeenMap[device.address] = now
                
                _discoveredDevices.update { current ->
                    val existingIndex = current.indexOfFirst { it.address == device.address }
                    if (existingIndex != -1) {
                        // Update RSSI and potentially name if it was unknown
                        val existing = current[existingIndex]
                        if (existing.rssi == result.rssi && existing.name == name) {
                            current
                        } else {
                            current.toMutableList().apply {
                                set(existingIndex, existing.copy(rssi = result.rssi, name = name))
                            }
                        }
                    } else {
                        current + BicycleDevice(name, device.address, result.rssi, SERVICE_UUID.toString())
                    }
                }
            }
        }
    }

    @SuppressLint("MissingPermission")
    override fun startScanning(filterUuid: String?) {
        if (_isScanning.value) return
        _discoveredDevices.value = emptyList()
        lastSeenMap.clear()
        
        val filters = mutableListOf<ScanFilter>()
        filters.add(ScanFilter.Builder().setServiceUuid(ParcelUuid(SERVICE_UUID)).build())
        
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
            
        scanner?.startScan(filters, settings, scanCallback)
        _isScanning.value = true
        
        mainHandler.removeCallbacks(cleanupRunnable)
        mainHandler.postDelayed(cleanupRunnable, 2000)
        
        Log.d(TAG, "Scanning for $SERVICE_UUID")
    }

    @SuppressLint("MissingPermission")
    override fun stopScanning() {
        if (!_isScanning.value) return
        scanner?.stopScan(scanCallback)
        _isScanning.value = false
        mainHandler.removeCallbacks(cleanupRunnable)
        Log.d(TAG, "Stopped scanning")
    }

    @SuppressLint("MissingPermission")
    override fun connect(device: BicycleDevice) {
        stopScanning()
        val remoteDevice = bluetoothAdapter?.getRemoteDevice(device.address) ?: return
        
        Log.d(TAG, "Connecting to ${device.address}. Current bond state: ${remoteDevice.bondState}")

        if (remoteDevice.bondState == BluetoothDevice.BOND_NONE) {
            Log.d(TAG, "Initiating bond...")
            remoteDevice.createBond()
        }
        
        bluetoothGatt = remoteDevice.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
    }

    private val gattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d(TAG, "GATT Connected")
                mainHandler.postDelayed({
                    gatt.discoverServices()
                }, 1000)
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.d(TAG, "GATT Disconnected")
            }
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                val service = gatt.getService(SERVICE_UUID)
                val characteristic = service?.getCharacteristic(CHARACTERISTIC_UUID)
                
                if (characteristic != null) {
                    Log.d(TAG, "Enabling notifications for $CHARACTERISTIC_UUID")
                    gatt.setCharacteristicNotification(characteristic, true)
                    
                    val descriptor = characteristic.getDescriptor(DESCRIPTOR_UUID)
                    if (descriptor != null) {
                        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                        gatt.writeDescriptor(descriptor)
                    }
                }
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (characteristic.uuid == CHARACTERISTIC_UUID) {
                val rawData = characteristic.value
                Log.d(TAG, "Received: ${rawData.contentToString()}")
            }
        }
    }
}

object BluetoothScannerProvider {
    lateinit var scanner: BluetoothScanner
}

actual fun getBluetoothScanner(): BluetoothScanner = BluetoothScannerProvider.scanner
