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
    private var bluetoothGatt: BluetoothGatt? = null

    // ESP32 UUIDs from the Arduino script
    private val SERVICE_UUID = UUID.fromString("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e1")
    private val CHARACTERISTIC_UUID = UUID.fromString("1b4d9b4b-9d59-4c4a-8ec6-4f0d8d5cc9e2")
    private val DESCRIPTOR_UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

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
        replay = 0,
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
                val bondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, BluetoothDevice.ERROR)
                val device = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE, BluetoothDevice::class.java)
                } else {
                    @Suppress("DEPRECATION")
                    intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE)
                }
                
                if (bondState == BluetoothDevice.BOND_BONDED && device?.address == pendingDeviceAddressForNotificationEnable) {
                    bluetoothGatt?.let { tryEnableNotifications(it) }
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
            val name = device.name ?: "Unknown Device"
            val scanRecord = result.scanRecord
            val serviceUuids = scanRecord?.serviceUuids?.map { it.uuid } ?: emptyList()
            
            if (name.contains("Cyclist", ignoreCase = true) || 
                name.contains("VICAM", ignoreCase = true) ||
                serviceUuids.contains(SERVICE_UUID)) {
                
                lastSeenMap[device.address] = System.currentTimeMillis()
                _discoveredDevices.update { current ->
                    val existingIndex = current.indexOfFirst { it.address == device.address }
                    if (existingIndex != -1) {
                        val existing = current[existingIndex]
                        if (existing.rssi == result.rssi && existing.name == name) current
                        else current.toMutableList().apply { set(existingIndex, existing.copy(rssi = result.rssi, name = name)) }
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
        stopScanning()
        val remoteDevice = bluetoothAdapter?.getRemoteDevice(device.address) ?: return
        bluetoothGatt = remoteDevice.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
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
        override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (descriptor.uuid == DESCRIPTOR_UUID && status != BluetoothGatt.GATT_SUCCESS) {
                if (status == BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION || status == BluetoothGatt.GATT_INSUFFICIENT_ENCRYPTION) {
                    pendingDeviceAddressForNotificationEnable = gatt.device.address
                    gatt.device.createBond()
                }
            }
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU && characteristic.uuid == CHARACTERISTIC_UUID) {
                @Suppress("DEPRECATION")
                handleNotifyBytes(characteristic.value)
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            if (characteristic.uuid == CHARACTERISTIC_UUID) {
                handleNotifyBytes(value)
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

    private fun handleNotifyBytes(rawData: ByteArray) {
        try {
            val chunk = rawData.toString(Charsets.UTF_8)
            if (chunk.isBlank()) return
            
            // If a new chunk starts with "{" and our buffer is already populated but incomplete,
            // it's likely a re-send. Clear the buffer.
            if (chunk.startsWith("{") && notifyTextBuffer.isNotEmpty() && !notifyTextBuffer.contains("}")) {
                Log.d(TAG, "Partial buffer discarded in favor of new message start: ${notifyTextBuffer.toString()}")
                notifyTextBuffer.setLength(0)
            }

            notifyTextBuffer.append(chunk)
            val buffered = notifyTextBuffer.toString()

            Log.d(TAG, "Notify chunk: $chunk")
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
                        _crashEvents.tryEmit(event)
                        mainHandler.post { 
                            Toast.makeText(context, "CRASH DETECTED (ID: ${event.crashId})", Toast.LENGTH_LONG).show() 
                        }
                        handledAny = true
                    } catch (t: Throwable) {
                        Log.w(TAG, "Failed to parse JSON: $candidate", t)
                    }
                }
            }

            // Agressive fallback: if we see "type":"crash" and the buffer is growing without a closing brace,
            // or if it's the exact 20-byte truncated message the user is seeing.
            if (!handledAny && buffered.contains("\"type\":\"crash\"")) {
                val trimmed = buffered.trim()
                // If it's the specific truncated 20-byte message or if it's just stuck.
                if (trimmed == "{\"type\":\"crash\",\"cra" || (trimmed.length > 30 && !trimmed.endsWith("}"))) {
                    Log.d(TAG, "Triggering emergency crash event from partial/truncated data")
                    val fallbackEvent = CrashEvent(crashId = -1, orientation = "TRUNCATED")
                    _crashEvents.tryEmit(fallbackEvent)
                    mainHandler.post { 
                        Toast.makeText(context, "CRASH DETECTED (Partial Signal)!", Toast.LENGTH_LONG).show() 
                    }
                    // Clear buffer to avoid spamming the fallback
                    notifyTextBuffer.setLength(0)
                }
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

object BluetoothScannerProvider {
    lateinit var scanner: BluetoothScanner
}

actual fun getBluetoothScanner(): BluetoothScanner = BluetoothScannerProvider.scanner
