package com.xteam.vicam

import androidx.compose.animation.*
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.tooling.preview.Preview

enum class Screen {
    List, Connect, Dashboard
}

@Composable
@Preview
fun App() {
    MaterialTheme {
        var currentScreen by remember { mutableStateOf(Screen.List) }
        val connectedDevices = remember { mutableStateListOf<BicycleDevice>() }
        var selectedDevice by remember { mutableStateOf<BicycleDevice?>(null) }

        Surface(
            modifier = Modifier.fillMaxSize(),
            color = MaterialTheme.colorScheme.background
        ) {
            AnimatedContent(
                targetState = currentScreen,
                transitionSpec = {
                    if (targetState == Screen.Connect || targetState == Screen.Dashboard) {
                        slideInHorizontally { width -> width } + fadeIn() togetherWith
                                slideOutHorizontally { width -> -width } + fadeOut()
                    } else {
                        slideInHorizontally { width -> -width } + fadeIn() togetherWith
                                slideOutHorizontally { width -> width } + fadeOut()
                    }.using(
                        SizeTransform(clip = false)
                    )
                },
                label = "screen_transition"
            ) { screen ->
                when (screen) {
                    Screen.List -> {
                        BicycleListScreen(
                            connectedDevices = connectedDevices,
                            onAddClick = { currentScreen = Screen.Connect },
                            onDeviceClick = { device ->
                                selectedDevice = device
                                currentScreen = Screen.Dashboard
                            }
                        )
                    }
                    Screen.Connect -> {
                        BicycleConnectScreen(
                            onConnect = { device ->
                                if (!connectedDevices.any { it.address == device.address }) {
                                    connectedDevices.add(device)
                                }
                                currentScreen = Screen.List
                            },
                            onBack = { currentScreen = Screen.List }
                        )
                    }
                    Screen.Dashboard -> {
                        BicycleDashboard(
                            device = selectedDevice!!,
                            onDisconnect = {
                                connectedDevices.remove(selectedDevice)
                                selectedDevice = null
                                currentScreen = Screen.List
                            }
                        )
                    }
                }
            }
        }
    }
}

@Composable
fun BicycleDashboard(
    device: BicycleDevice,
    onDisconnect: () -> Unit
) {
    Column(
        modifier = Modifier.fillMaxSize().padding(16.dp),
        verticalArrangement = Arrangement.Center,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text(
            text = "Connected to",
            style = MaterialTheme.typography.labelLarge
        )
        Text(
            text = device.name,
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )
        Spacer(modifier = Modifier.height(24.dp))
        
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Column(modifier = Modifier.padding(16.dp)) {
                Text("Device Info", style = MaterialTheme.typography.titleSmall)
                Text("Address: ${device.address}", style = MaterialTheme.typography.bodyMedium)
                Text("Signal Strength: ${device.rssi} dBm", style = MaterialTheme.typography.bodyMedium)
            }
        }
        
        Spacer(modifier = Modifier.height(32.dp))
        
        Button(
            onClick = onDisconnect,
            colors = ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.error
            )
        ) {
            Text("Disconnect")
        }
    }
}
