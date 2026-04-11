package com.xteam.vicam

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

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