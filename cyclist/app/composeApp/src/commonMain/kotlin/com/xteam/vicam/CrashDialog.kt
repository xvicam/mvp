package com.xteam.vicam

import androidx.compose.animation.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.zIndex
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

@Composable
fun CrashDialog() {
    val crash = DeviceManager.activeCrash
    val emergencyManager = remember { getEmergencyManager() }
    val soundManager = remember { getSoundManager() }
    val scope = rememberCoroutineScope()

    if (crash != null) {
        var timeLeft by remember { mutableStateOf(10) }
        var isTimerActive by remember { mutableStateOf(true) }
        var isSending by remember { mutableStateOf(false) }

        // Play sound and manage timer when a crash appears
        LaunchedEffect(crash) {
            timeLeft = 10
            isTimerActive = true
            isSending = false
            
            soundManager.playAlarm()
            
            while (timeLeft > 0 && isTimerActive) {
                delay(1000)
                timeLeft--
            }
            
            if (timeLeft == 0 && isTimerActive) {
                isSending = true
                soundManager.stopAlarm()
                sendEmergencyAlert(crash, emergencyManager)
                DeviceManager.activeCrash = null
            }
        }

        // Ensure sound stops if the composable is disposed or crash becomes null
        DisposableEffect(Unit) {
            onDispose {
                soundManager.stopAlarm()
            }
        }

        Box(
            modifier = Modifier
                .fillMaxSize()
                .zIndex(9999f) // Ensure it's on top of everything
                .background(Color.Red)
                .padding(24.dp),
            contentAlignment = Alignment.Center
        ) {
            Column(
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Icon(
                    imageVector = Icons.Default.Warning,
                    contentDescription = null,
                    modifier = Modifier.size(100.dp),
                    tint = Color.White
                )
                
                Spacer(modifier = Modifier.height(24.dp))
                
                Text(
                    text = "CRASH DETECTED!",
                    style = MaterialTheme.typography.headlineLarge,
                    color = Color.White,
                    fontWeight = FontWeight.Bold
                )
                
                Spacer(modifier = Modifier.height(16.dp))
                
                Text(
                    text = "Emergency alert in:",
                    style = MaterialTheme.typography.titleMedium,
                    color = Color.White
                )
                
                Text(
                    text = "$timeLeft",
                    style = MaterialTheme.typography.displayLarge,
                    color = Color.White,
                    fontWeight = FontWeight.Black,
                    fontSize = 100.sp
                )
                
                Spacer(modifier = Modifier.height(48.dp))
                
                if (isSending) {
                    CircularProgressIndicator(color = Color.White)
                    Spacer(modifier = Modifier.height(8.dp))
                    Text("Sending SMS to contacts...", color = Color.White)
                } else {
                    Button(
                        onClick = {
                            isTimerActive = false
                            isSending = true
                            soundManager.stopAlarm()
                            scope.launch {
                                sendEmergencyAlert(crash, emergencyManager)
                                DeviceManager.activeCrash = null
                            }
                        },
                        modifier = Modifier.fillMaxWidth().height(70.dp),
                        colors = ButtonDefaults.buttonColors(containerColor = Color.White, contentColor = Color.Red),
                        shape = MaterialTheme.shapes.large
                    ) {
                        Text("I HAVE GOTTEN INTO A CRASH", fontWeight = FontWeight.ExtraBold, fontSize = 18.sp)
                    }
                    
                    Spacer(modifier = Modifier.height(20.dp))
                    
                    OutlinedButton(
                        onClick = {
                            isTimerActive = false
                            soundManager.stopAlarm()
                            DeviceManager.activeCrash = null
                        },
                        modifier = Modifier.fillMaxWidth().height(70.dp),
                        border = androidx.compose.foundation.BorderStroke(3.dp, Color.White),
                        colors = ButtonDefaults.outlinedButtonColors(contentColor = Color.White),
                        shape = MaterialTheme.shapes.large
                    ) {
                        Text("I HAVE NOT GOTTEN INTO A CRASH", fontWeight = FontWeight.Bold, fontSize = 18.sp)
                    }
                }
            }
        }
    }
}

private suspend fun sendEmergencyAlert(crash: CrashEvent, emergencyManager: EmergencyManager) {
    val lat = crash.gps?.lat ?: 0.0
    val lng = crash.gps?.lng ?: 0.0
    
    emergencyManager.sendEmergencyAlert(
        userName = DeviceManager.userName,
        lat = lat,
        lng = lng,
        contacts = DeviceManager.emergencyContacts
    )
}
