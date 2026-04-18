package com.xteam.vicam

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.flow.collectLatest

@Composable
fun App() {
    var showEmergencyContacts by remember { mutableStateOf(false) }
    val scanner = remember { getBluetoothScanner() }

    LaunchedEffect(scanner) {
        scanner.crashEvents.collectLatest { event ->
            println("Crash event received in UI: id=${event.crashId}")
            DeviceManager.activeCrash = event
        }
    }

    if (showEmergencyContacts) {
        EmergencyContactsScreen(
            onBack = { showEmergencyContacts = false }
        )
    } else {
        BicycleListScreen(
            connectedDevices = DeviceManager.connectedDevices,
            onAddClick = {
                // For a fully shared app we would route, but this is a stub.
            },
            onDeviceClick = { device ->
                // For a fully shared app we would route, but this is a stub.
            },
            onEmergencyContactsClick = {
                showEmergencyContacts = true
            }
        )
    }

    val crash = DeviceManager.activeCrash
    if (crash != null) {
        AlertDialog(
            onDismissRequest = { DeviceManager.activeCrash = null },
            title = { Text("Crash detected") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                    Text("Crash ID: ${crash.crashId}")
                    Text("Peak dynamic: ${crash.peakDynamicMps2.asFixed(2)} m/s²")
                    Text("Pitch: ${crash.pitch.asFixed(1)}°")
                    Text("Roll: ${crash.roll.asFixed(1)}°")
                    Text("Orientation: ${crash.orientation}")
                    Text("Moving: ${if (crash.isMoving) "Yes" else "No"}")
                    val gps = crash.gps
                    if (gps != null) {
                        Text("GPS: ${gps.lat.asFixed(4)}, ${gps.lng.asFixed(4)}")
                    }
                }
            },
            confirmButton = {
                TextButton(onClick = { DeviceManager.activeCrash = null }) { Text("OK") }
            },
            dismissButton = {
                TextButton(onClick = { DeviceManager.activeCrash = null }) { Text("Dismiss") }
            }
        )
    }
}

private fun Double.asFixed(decimals: Int): String {
    val clamped = decimals.coerceIn(0, 4)
    val factor = when (clamped) {
        0 -> 1.0
        1 -> 10.0
        2 -> 100.0
        3 -> 1000.0
        else -> 10000.0
    }
    val v = kotlin.math.round(this * factor) / factor
    return v.toString()
}
