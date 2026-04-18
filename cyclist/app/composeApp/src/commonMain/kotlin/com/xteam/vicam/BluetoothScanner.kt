package com.xteam.vicam

import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

data class BicycleDevice(
    val name: String,
    val address: String,
    val rssi: Int,
    val serviceUuid: String? = null
)

interface BluetoothScanner {
    val isScanning: StateFlow<Boolean>
    val discoveredDevices: StateFlow<List<BicycleDevice>>

    /**
     * Stream of crash notifications sent by the ESP32 over the notify characteristic.
     *
     * Expected JSON example:
     * {"type":"crash","crashId":1,"peakDyn":12.34,"pitch":-1.2,"roll":3.4,"orient":"UPRIGHT","moving":true,"gps":{"lat":52.4862,"lng":-1.8904}}
     */
    val crashEvents: Flow<CrashEvent>

    fun startScanning(filterUuid: String? = null)
    fun stopScanning()
    fun connect(device: BicycleDevice)
}

@Serializable
data class CrashEvent(
    val type: String = "crash",
    val crashId: Long = 0,
    @SerialName("peakDyn") val peakDynamicMps2: Double = 0.0,
    val pitch: Double = 0.0,
    val roll: Double = 0.0,
    @SerialName("orient") val orientation: String = "UNKNOWN",
    @SerialName("moving") val isMoving: Boolean = false,
    val gps: GpsLocation? = null
)

@Serializable
data class GpsLocation(
    val lat: Double = 0.0,
    val lng: Double = 0.0
)

expect fun getBluetoothScanner(): BluetoothScanner