package com.xteam.vicam

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp

@Composable
fun BicycleDashboard(
    device: BicycleDevice,
    onDisconnect: () -> Unit,
    onGoBack: () -> Unit = onDisconnect
) {
    val locationProvider = remember { LocationProviderFactory.create() }
    
    // Use the singleton so state survives activity recreation
    var useStaticGps by remember { mutableStateOf(StaticSensorState.useStaticGps) }
    var useStaticImu by remember { mutableStateOf(StaticSensorState.useStaticImu) }
    var useStaticSpeed by remember { mutableStateOf(StaticSensorState.useStaticSpeed) }
    var useStaticHeading by remember { mutableStateOf(StaticSensorState.useStaticHeading) }

    var staticLat by remember { mutableStateOf(StaticSensorState.lat) }
    var staticLng by remember { mutableStateOf(StaticSensorState.lng) }
    var staticAlt by remember { mutableStateOf(StaticSensorState.alt) }
    var staticAx by remember { mutableStateOf(StaticSensorState.ax) }
    var staticAy by remember { mutableStateOf(StaticSensorState.ay) }
    var staticAz by remember { mutableStateOf(StaticSensorState.az) }
    var staticGx by remember { mutableStateOf(StaticSensorState.gx) }
    var staticGy by remember { mutableStateOf(StaticSensorState.gy) }
    var staticGz by remember { mutableStateOf(StaticSensorState.gz) }
    var staticSpeed by remember { mutableStateOf(StaticSensorState.speed) }
    var staticHeading by remember { mutableStateOf(StaticSensorState.heading) }

    // Helper to persist field values back to singleton
    fun syncToSingleton() {
        StaticSensorState.useStaticGps = useStaticGps
        StaticSensorState.useStaticImu = useStaticImu
        StaticSensorState.useStaticSpeed = useStaticSpeed
        StaticSensorState.useStaticHeading = useStaticHeading
        StaticSensorState.lat = staticLat
        StaticSensorState.lng = staticLng
        StaticSensorState.alt = staticAlt
        StaticSensorState.ax = staticAx
        StaticSensorState.ay = staticAy
        StaticSensorState.az = staticAz
        StaticSensorState.gx = staticGx
        StaticSensorState.gy = staticGy
        StaticSensorState.gz = staticGz
        StaticSensorState.speed = staticSpeed
        StaticSensorState.heading = staticHeading
    }

    fun sendValues() {
        val scanner = BluetoothScannerProvider.scanner
        val cmd = "STATIC_VALS:$staticLat,$staticLng,$staticAlt,$staticAx,$staticAy,$staticAz,$staticGx,$staticGy,$staticGz,$staticSpeed,$staticHeading"
        scanner.sendCommand(device.address, cmd)
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.Top,
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Spacer(modifier = Modifier.height(32.dp))
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
        
        Spacer(modifier = Modifier.height(24.dp))

        // ── Sensor Simulation ──
        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Column(modifier = Modifier.padding(16.dp)) {
                Text("Sensor Simulation", style = MaterialTheme.typography.titleMedium)
                Spacer(modifier = Modifier.height(4.dp))
                Text("Toggle each sensor group independently.", style = MaterialTheme.typography.bodySmall)
                Spacer(modifier = Modifier.height(12.dp))

                // ── GPS ──
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Static GPS", modifier = Modifier.weight(1f))
                    Switch(
                        checked = useStaticGps,
                        onCheckedChange = { checked ->
                            useStaticGps = checked
                            syncToSingleton()
                            BluetoothScannerProvider.scanner.sendCommand(device.address, "STATIC_GPS:${if (checked) "1" else "0"}")
                        }
                    )
                }
                if (useStaticGps) {
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        OutlinedTextField(value = staticLat, onValueChange = { staticLat = it; syncToSingleton(); sendValues() }, label = { Text("Lat") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                        OutlinedTextField(value = staticLng, onValueChange = { staticLng = it; syncToSingleton(); sendValues() }, label = { Text("Lng") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                        OutlinedTextField(value = staticAlt, onValueChange = { staticAlt = it; syncToSingleton(); sendValues() }, label = { Text("Alt") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                    }
                    Spacer(modifier = Modifier.height(8.dp))
                    OutlinedButton(
                        onClick = {
                            locationProvider.getCurrentLocation(
                                onSuccess = { lat, lng, alt ->
                                    staticLat = lat.toString()
                                    staticLng = lng.toString()
                                    staticAlt = alt.toString()
                                    syncToSingleton()
                                    sendValues()
                                },
                                onError = { /* TODO: Show error message */ }
                            )
                        },
                        modifier = Modifier.fillMaxWidth()
                    ) {
                        Text("Use Phone GPS Location")
                    }
                }

                HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))

                // ── IMU (Accel + Gyro) ──
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Static IMU", modifier = Modifier.weight(1f))
                    Switch(
                        checked = useStaticImu,
                        onCheckedChange = { checked ->
                            useStaticImu = checked
                            syncToSingleton()
                            BluetoothScannerProvider.scanner.sendCommand(device.address, "STATIC_IMU:${if (checked) "1" else "0"}")
                        }
                    )
                }
                if (useStaticImu) {
                    Text("Accelerometer", style = MaterialTheme.typography.labelSmall)
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        OutlinedTextField(value = staticAx, onValueChange = { staticAx = it; syncToSingleton(); sendValues() }, label = { Text("Ax") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                        OutlinedTextField(value = staticAy, onValueChange = { staticAy = it; syncToSingleton(); sendValues() }, label = { Text("Ay") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                        OutlinedTextField(value = staticAz, onValueChange = { staticAz = it; syncToSingleton(); sendValues() }, label = { Text("Az") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                    }
                    Spacer(modifier = Modifier.height(4.dp))
                    Text("Gyroscope", style = MaterialTheme.typography.labelSmall)
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        OutlinedTextField(value = staticGx, onValueChange = { staticGx = it; syncToSingleton(); sendValues() }, label = { Text("Gx") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                        OutlinedTextField(value = staticGy, onValueChange = { staticGy = it; syncToSingleton(); sendValues() }, label = { Text("Gy") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                        OutlinedTextField(value = staticGz, onValueChange = { staticGz = it; syncToSingleton(); sendValues() }, label = { Text("Gz") }, modifier = Modifier.weight(1f), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                    }
                }

                HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))

                // ── Speed ──
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Static Speed", modifier = Modifier.weight(1f))
                    Switch(
                        checked = useStaticSpeed,
                        onCheckedChange = { checked ->
                            useStaticSpeed = checked
                            syncToSingleton()
                            BluetoothScannerProvider.scanner.sendCommand(device.address, "STATIC_SPEED:${if (checked) "1" else "0"}")
                        }
                    )
                }
                if (useStaticSpeed) {
                    OutlinedTextField(value = staticSpeed, onValueChange = { staticSpeed = it; syncToSingleton(); sendValues() }, label = { Text("Speed") }, modifier = Modifier.fillMaxWidth(), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                }

                HorizontalDivider(modifier = Modifier.padding(vertical = 8.dp))

                // ── Heading ──
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Static Heading", modifier = Modifier.weight(1f))
                    Switch(
                        checked = useStaticHeading,
                        onCheckedChange = { checked ->
                            useStaticHeading = checked
                            syncToSingleton()
                            BluetoothScannerProvider.scanner.sendCommand(device.address, "STATIC_HEADING:${if (checked) "1" else "0"}")
                        }
                    )
                }
                if (useStaticHeading) {
                    OutlinedTextField(value = staticHeading, onValueChange = { staticHeading = it; syncToSingleton(); sendValues() }, label = { Text("Heading (°)") }, modifier = Modifier.fillMaxWidth(), keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number), singleLine = true)
                }

                Spacer(modifier = Modifier.height(16.dp))
                Button(
                    onClick = {
                        syncToSingleton()
                        BluetoothScannerProvider.scanner.sendCommand(device.address, "MODE:OPERATING")
                        onGoBack()
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Start ESP-NOW Broadcast")
                }
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
        Spacer(modifier = Modifier.height(16.dp))
    }
}
