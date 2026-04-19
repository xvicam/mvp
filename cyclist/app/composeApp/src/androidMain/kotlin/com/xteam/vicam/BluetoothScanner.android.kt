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
import android.widget.Toast
import androidx.core.content.getSystemService
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.json.Json
import java.util.*

private const val TAG = "BluetoothScanner"

class AndroidBluetoothScanner(private val context: Context) : BluetoothScanner {
    private val bluetoothAdapter: BluetoothAdapter? = context.getSystemService<BluetoothManager>()?.adapter
    private val scanner = bluetoothAdapter?.bluetoothLeScanner
    private val activeGatts = mutableMapOf<String, BluetoothGatt>()
    private val SERVICE_UUID = UUID.fromString("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e1")
    private val CHARACTERISTIC_UUID = UUID.fromString("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e2")
    private val DESCRIPTOR_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    
    private var lastProcessedCrashId: Long = -1L
    
    private val _isScanning = MutableStateFlow(false)
    override val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()

    private val _discoveredDevices = MutableStateFlow<List<BicycleDevice>>(emptyList())
    override val discoveredDevices: StateFlow<List<BicycleDevice>> = _discoveredDevices.asStateFlow()

    private val json = Json {
        ignoreUnknownKeys = true
        isLenient = true
        explicitNulls = false
    }

    private val _crashEvents = MutableSharedFlow<CrashEvent>(
        replay = 1, // 1 to ensure UI gets the latest event if it starts late
        extraBufferCapacity = 16
    )
    override val crashEvents: SharedFlow<CrashEvent> = _crashEvents

    private val notifyTextBuffer = StringBuilder()
    private val lastSeenMap = mutableMapOf<String, Long>()
    private val mainHandler = Handler(Looper.getMainLooper())

    private val cleanupRunnable = object : Runnable {
        override fun run() {
            if (!_isScanning.value) return
            val now = System.currentTimeMillis()
            val timeout = 8000L
            val expiredAddresses = lastSeenMap.filter { now - it.value > timeout }.keys
            if (expiredAddresses.isNotEmpty()) {
                expiredAddresses.forEach { lastSeenMap.remove(it) }
                _discoveredDevices.update { current ->
                    current.filter { it.address !in expiredAddresses }
                }
            }
            mainHandler.postDelayed(this, 2000)
        }
    }

    private val bondStateReceiver = object : BroadcastReceiver() {
        @SuppressLint("MissingPermission")
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == BluetoothDevice.ACTION_BOND_STATE_CHANGED) {
                val bondState =
                    intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR)
                val device = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    intent.getParcelableExtra(
                        BluetoothDevice.EXTRA_DEVICE,
                        BluetoothDevice::class.java
                    )
                } else {
                    @Suppress("DEPRECATION")
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
                }

                if (bondState == BluetoothDevice.BOND_BONDED && device != null && device.address == pendingDeviceAddressForNotificationEnable) {
                    activeGatts[device.address]?.let { tryEnableNotifications(it) }
                    pendingDeviceAddressForNotificationEnable = null
                }
            }
        }
    }

    init {
        val filter = IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED)
        context.registerReceiver(bondStateReceiver, filter)
    }

    private var pendingDeviceAddressForNotificationEnable: String? = null

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val scanRecord = result.scanRecord
            val deviceNameFallback = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                @Suppress("MissingPermission")
                device.alias ?: device.name
            } else {
                @Suppress("MissingPermission")
                device.name
            }
            val rawName = scanRecord?.deviceName ?: deviceNameFallback ?: "Unknown Device"
            val serviceUuids = scanRecord?.serviceUuids?.map { it.uuid } ?: emptyList()

            val name = if (rawName == "Unknown Device" && serviceUuids.contains(SERVICE_UUID)) {
                "ESP32 (Cyclist)"
            } else {
                rawName
            }

            val isAlreadyDiscovered = lastSeenMap.containsKey(device.address)
            if (isAlreadyDiscovered ||
                name.contains("Cyclist", ignoreCase = true) ||
                name.contains("VICAM", ignoreCase = true) ||
                name.contains("ESP32", ignoreCase = true) ||
                serviceUuids.contains(SERVICE_UUID)) {

                lastSeenMap[device.address] = System.currentTimeMillis()
                _discoveredDevices.update { current ->
                    val existingIndex = current.indexOfFirst { it.address == device.address }
                    if (existingIndex != -1) {
                        val existing = current[existingIndex]
                        val updatedName = if (name != "Unknown Device") name else existing.name
                        if (existing.rssi == result.rssi && existing.name == updatedName) current
                        else current.toMutableList().apply { set(existingIndex, existing.copy(rssi = result.rssi, name = updatedName)) }
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
        val filters = listOf(ScanFilter.Builder().setServiceUuid(ParcelUuid(SERVICE_UUID)).build())
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        scanner?.startScan(filters, settings, scanCallback)
        _isScanning.value = true
        mainHandler.postDelayed(cleanupRunnable, 2000)
    }

    @SuppressLint("MissingPermission")
    override fun stopScanning() {
        if (!_isScanning.value) return
        scanner?.stopScan(scanCallback)
        _isScanning.value = false
        mainHandler.removeCallbacks(cleanupRunnable)
    }

    @SuppressLint("MissingPermission")
    override fun connect(device: BicycleDevice) {
        val remoteDevice = bluetoothAdapter?.getRemoteDevice(device.address) ?: return
        val gatt = remoteDevice.connectGatt(context, true, gattCallback, BluetoothDevice.TRANSPORT_LE)
        activeGatts[device.address] = gatt
    }

    private val gattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d(TAG, "GATT Connected. Requesting MTU 517...")
                gatt.requestMtu(517)
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.d(TAG, "GATT Disconnected")
            }
        }

        @SuppressLint("MissingPermission")
        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            Log.d(TAG, "MTU changed to $mtu, status=$status")
            gatt.discoverServices()
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                tryEnableNotifications(gatt)
            }
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (characteristic.uuid == CHARACTERISTIC_UUID) {
                @Suppress("DEPRECATION")
                handleNotifyBytes(gatt, characteristic.value)
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            if (characteristic.uuid == CHARACTERISTIC_UUID) {
                handleNotifyBytes(gatt, value)
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun tryEnableNotifications(gatt: BluetoothGatt) {
        val characteristic = gatt.getService(SERVICE_UUID)?.getCharacteristic(CHARACTERISTIC_UUID) ?: return
        gatt.setCharacteristicNotification(characteristic, true)
        val descriptor = characteristic.getDescriptor(DESCRIPTOR_UUID) ?: return
        @Suppress("DEPRECATION")
        descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        gatt.writeDescriptor(descriptor)
    }

    private fun handleNotifyBytes(gatt: BluetoothGatt, rawData: ByteArray) {
        // Run on Main thread to avoid concurrent access to notifyTextBuffer
        mainHandler.post {
            try {
                val chunk = rawData.toString(Charsets.UTF_8)
                if (chunk.isBlank()) return@post

                Log.d(TAG, "Notify chunk: $chunk")

                if (chunk.startsWith("{") && notifyTextBuffer.isNotEmpty() && !notifyTextBuffer.contains("}")) {
                    Log.d(TAG, "Partial buffer discarded in favor of new message start")
                    notifyTextBuffer.setLength(0)
                }

                notifyTextBuffer.append(chunk)
                val buffered = notifyTextBuffer.toString()
                Log.d(TAG, "Notify buffered: $buffered")

                var start = -1
                var depth = 0
                var inString = false
                var escape = false
                val complete = mutableListOf<Pair<Int, Int>>()
                for (i in buffered.indices) {
                    val c = buffered[i]
                    if (inString) {
                        if (escape) escape = false else if (c == '\\') escape = true else if (c == '"') inString = false
                        continue
                    }
                    when (c) {
                        '"' -> inString = true
                        '{' -> { if (depth == 0) start = i; depth++ }
                        '}' -> { if (depth > 0) depth--; if (depth == 0 && start != -1) { complete += start to i; start = -1 } }
                    }
                }

                var handledAny = false
                var lastEnd = -1
                for ((s, e) in complete) {
                    val candidate = buffered.substring(s, e + 1).trim()
                    lastEnd = maxOf(lastEnd, e)
                    if (candidate.contains("\"crash\"")) {
                        try {
                            val event = json.decodeFromString<CrashEvent>(candidate)
                            Log.d(TAG, "Parsed CrashEvent: ID=${event.crashId}")

                            if (event.crashId != lastProcessedCrashId) {
                                lastProcessedCrashId = event.crashId
                                Log.d(TAG, "Emitting new crash event to flow")
                                _crashEvents.tryEmit(event)
                                Toast.makeText(context, "CRASH DETECTED (ID: ${event.crashId})", Toast.LENGTH_LONG).show()
                            }

                            sendAck(gatt)
                            handledAny = true
                        } catch (t: Throwable) {
                            Log.w(TAG, "Failed to parse JSON: $candidate", t)
                        }
                    }
                }

                // Fallback for truncated/malformed but obviously crash data
                if (!handledAny && buffered.contains("\"type\":\"crash\"") && buffered.length > 50 && !buffered.endsWith("}")) {
                    Log.d(TAG, "Triggering emergency fallback for partial data")
                    if (lastProcessedCrashId != -999L) {
                        lastProcessedCrashId = -999L
                        _crashEvents.tryEmit(CrashEvent(crashId = -999L, orientation = "TRUNCATED"))
                    }
                    sendAck(gatt)
                    notifyTextBuffer.setLength(0)
                    handledAny = true
                }

                if (lastEnd >= 0) {
                    val remainder = if (lastEnd + 1 < buffered.length) buffered.substring(lastEnd + 1) else ""
                    notifyTextBuffer.setLength(0)
                    notifyTextBuffer.append(remainder)
                }
            } catch (t: Throwable) {
                Log.w(TAG, "Error in handleNotifyBytes", t)
                notifyTextBuffer.setLength(0)
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun sendAck(gatt: BluetoothGatt) {
        try {
            val char = gatt.getService(SERVICE_UUID)?.getCharacteristic(CHARACTERISTIC_UUID)
            if (char != null) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    gatt.writeCharacteristic(char, "ACK".toByteArray(), BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
                } else {
                    @Suppress("DEPRECATION")
                    char.value = "ACK".toByteArray()
                    @Suppress("DEPRECATION")
                    gatt.writeCharacteristic(char)
                }
                Log.d(TAG, "Sent ACK back to ESP32")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to send ACK", e)
        }
    }
}

object BluetoothScannerProvider {
    lateinit var scanner: BluetoothScanner
}

actual fun getBluetoothScanner(): BluetoothScanner = BluetoothScannerProvider.scanner
