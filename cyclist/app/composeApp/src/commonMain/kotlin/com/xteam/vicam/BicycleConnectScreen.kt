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
import androidx.compose.ui.tooling.preview.Preview

data class BicycleDevice(
    val name: String,
    val address: String,
    val rssi: Int
)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun BicycleListScreen(
    connectedDevices: List<BicycleDevice>,
    onAddClick: () -> Unit,
    onDeviceClick: (BicycleDevice) -> Unit
) {
    Scaffold(
        topBar = {
            TopAppBar(title = { Text("My Bicycles") })
        },
        floatingActionButton = {
            FloatingActionButton(onClick = onAddClick) {
                Text("+", style = MaterialTheme.typography.headlineMedium)
            }
        }
    ) { padding ->
        if (connectedDevices.isEmpty()) {
            Box(modifier = Modifier.fillMaxSize().padding(padding), contentAlignment = Alignment.Center) {
                Text("No bikes connected. Tap + to add one.")
            }
        } else {
            LazyColumn(
                modifier = Modifier.fillMaxSize().padding(padding),
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                item {
                    Text(
                        "Connected Bicycles",
                        style = MaterialTheme.typography.titleMedium,
                        modifier = Modifier.padding(bottom = 8.dp)
                    )
                }
                items(connectedDevices) { device ->
                    BicycleDeviceItem(device = device, onConnect = { onDeviceClick(device) })
                }
            }
        }
    }
}

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
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = "Back"
                        )
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
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            if (isScanning) {
                LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
            }

            if (devices.isEmpty()) {
                Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    Column(horizontalAlignment = Alignment.CenterHorizontally) {
                        Text(
                            text = if (isScanning) "Searching for bicycles..." else "No bicycles found",
                            style = MaterialTheme.typography.bodyLarge
                        )
                        if (!isScanning) {
                            Button(onClick = { scanner.startScanning() }, modifier = Modifier.padding(16.dp)) {
                                Text("Start Scanning")
                            }
                        }
                    }
                }
            } else {
                LazyColumn(
                    modifier = Modifier.fillMaxSize(),
                    contentPadding = PaddingValues(16.dp),
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    item {
                        Text(
                            "Available Bicycles",
                            style = MaterialTheme.typography.titleMedium,
                            modifier = Modifier.padding(bottom = 8.dp)
                        )
                    }
                    items(devices) { device ->
                        BicycleDeviceItem(device = device, onConnect = { onConnect(device) })
                    }
                }
            }
        }
    }
}

@Composable
fun BicycleDeviceItem(
    device: BicycleDevice,
    onConnect: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable { onConnect() },
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Row(
            modifier = Modifier
                .padding(16.dp)
                .fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1.0f)) {
                Text(text = device.name, style = MaterialTheme.typography.titleLarge)
                Text(text = device.address, style = MaterialTheme.typography.bodySmall)
            }
            Text(
                text = "${device.rssi} dBm",
                style = MaterialTheme.typography.bodySmall,
                modifier = Modifier.padding(end = 8.dp)
            )
            Button(onClick = onConnect) {
                Text("Connect")
            }
        }
    }
}
