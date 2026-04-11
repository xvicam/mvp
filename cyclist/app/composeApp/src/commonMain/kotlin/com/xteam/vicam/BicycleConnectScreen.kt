package com.xteam.vicam

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun BicycleConnectScreen(
    onConnect: (BicycleDevice) -> Unit,
    onBack: () -> Unit
) {
    val scanner = remember { getBluetoothScanner() }
    val isScanning by scanner.isScanning.collectAsState()
    val devices by scanner.discoveredDevices.collectAsState()

    LaunchedEffect(Unit) {
        scanner.startScanning()
    }

    DisposableEffect(Unit) {
        onDispose {
            scanner.stopScanning()
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Connect to Bike") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(imageVector = Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    TextButton(onClick = {
                        if (isScanning) scanner.stopScanning() else scanner.startScanning()
                    }) {
                        Text(if (isScanning) "Stop" else "Refresh")
                    }
                }
            )
        }
    ) { padding ->
        Column(modifier = Modifier.fillMaxSize().padding(padding)) {
            if (isScanning) LinearProgressIndicator(modifier = Modifier.fillMaxWidth())

            if (devices.isEmpty()) {
                Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Text(if (isScanning) "Searching for VICAM devices..." else "No devices found")
                }
            } else {
                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = PaddingValues(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(devices) { device ->
                        BicycleDeviceItem(device = device, onConnect = { 
                            scanner.connect(device)
                            onConnect(device) 
                        })
                    }
                }
            }
        }
    }
}

@Composable
fun BicycleDeviceItem(device: BicycleDevice, onConnect: () -> Unit) {
    Card(
        modifier = Modifier.fillMaxWidth().clickable { onConnect() },
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Row(modifier = Modifier.padding(16.dp).fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Column(modifier = Modifier.weight(1f)) {
                Text(text = device.name, style = MaterialTheme.typography.titleLarge)
                Text(text = device.address, style = MaterialTheme.typography.bodySmall)
            }
            Button(onClick = onConnect) { Text("Connect") }
        }
    }
}
