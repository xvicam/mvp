package com.xteam.vicam

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview

@Composable
@Preview
fun App() {
    MaterialTheme {
        Surface(
            modifier = Modifier.fillMaxSize(),
            color = MaterialTheme.colorScheme.background
        ) {
            BicycleListScreen(
                connectedDevices = DeviceManager.connectedDevices,
                onAddClick = { /* Handle navigation in platform-specific MainActivity */ },
                onDeviceClick = { /* Handle navigation in platform-specific MainActivity */ }
            )
        }
    }
}
