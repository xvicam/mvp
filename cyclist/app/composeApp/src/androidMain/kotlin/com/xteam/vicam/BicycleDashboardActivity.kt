package com.xteam.vicam

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier

class BicycleDashboardActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        
        val device = DeviceManager.selectedDevice
        if (device == null) {
            finish()
            return
        }

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    Box {
                        BicycleDashboard(
                            device = device,
                            onDisconnect = {
                                DeviceManager.connectedDevices.remove(device)
                                AppPreferences.save(this@BicycleDashboardActivity)
                                DeviceManager.selectedDevice = null
                                finish()
                            }
                        )
                        CrashDialog()
                    }
                }
            }
        }
    }
}
