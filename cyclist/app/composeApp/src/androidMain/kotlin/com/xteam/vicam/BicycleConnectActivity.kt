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

class BicycleConnectActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    Box {
                        BicycleConnectScreen(
                            onConnect = { device ->
                                if (!DeviceManager.connectedDevices.any { it.address == device.address }) {
                                    DeviceManager.connectedDevices.add(device)
                                    AppPreferences.save(this@BicycleConnectActivity)
                                }
                                finish()
                            },
                            onBack = { finish() }
                        )
                        CrashDialog()
                    }
                }
            }
        }
    }
}