package com.xteam.vicam

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

object DeviceManager {
    val connectedDevices = mutableStateListOf<BicycleDevice>()
    var selectedDevice by mutableStateOf<BicycleDevice?>(null)
}