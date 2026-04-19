package com.xteam.vicam

import android.app.Application
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch

class VICAMApplication : Application() {
    private val applicationScope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    override fun onCreate() {
        super.onCreate()
        Log.d("VICAMApplication", "Application onCreate")
        
        // Initialize singletons
        SettingsProviderHolder.provider = AndroidSettingsProvider(applicationContext)
        ContactManagerProvider.manager = AndroidContactManager(applicationContext)
        EmergencyManagerProvider.manager = AndroidEmergencyManager(applicationContext)
        SoundManagerProvider.manager = AndroidSoundManager(applicationContext)
        
        // Initialize scanner early
        val scanner = AndroidBluetoothScanner(applicationContext)
        BluetoothScannerProvider.scanner = scanner
        
        // Global listener for crash events to update the shared st`ate
        applicationScope.launch {
            scanner.crashEvents.collectLatest { event ->
                Log.d("VICAMApplication", "Global crash listener received: ${event.crashId}")
                DeviceManager.activeCrash = event
            }
        }
    }
}
