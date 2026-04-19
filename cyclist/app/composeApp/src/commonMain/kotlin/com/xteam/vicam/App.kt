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

    Box(modifier = Modifier.fillMaxSize()) {
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

        CrashDialog()
    }
}
