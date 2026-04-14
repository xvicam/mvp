package com.xteam.vicam

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)

        // Initialize the scanner if not already done
        try {
            BluetoothScannerProvider.scanner
        } catch (e: UninitializedPropertyAccessException) {
            BluetoothScannerProvider.scanner = AndroidBluetoothScanner(applicationContext)
        }

        // Request permissions
        val permissions = mutableListOf(
            Manifest.permission.ACCESS_FINE_LOCATION
        )
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions.add(Manifest.permission.BLUETOOTH_SCAN)
            permissions.add(Manifest.permission.BLUETOOTH_CONNECT)
        }

        val requestPermissionLauncher = registerForActivityResult(
            ActivityResultContracts.RequestMultiplePermissions()
        ) { permissionsMap ->
            // In a real app, handle permission denial
        }

        if (permissions.any { ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED }) {
            requestPermissionLauncher.launch(permissions.toTypedArray())
        }

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    Box {
                        BicycleListScreen(
                            connectedDevices = DeviceManager.connectedDevices,
                            onAddClick = {
                                startActivity(Intent(this@MainActivity, BicycleConnectActivity::class.java))
                            },
                            onDeviceClick = { device ->
                                DeviceManager.selectedDevice = device
                                startActivity(Intent(this@MainActivity, BicycleDashboardActivity::class.java))
                            }
                        )
                        CrashDialog()
                    }
                }
            }
        }
    }
}

@Preview
@Composable
fun AppAndroidPreview() {
    MaterialTheme {
        BicycleListScreen(
            connectedDevices = emptyList(),
            onAddClick = {},
            onDeviceClick = {}
        )
    }
}
