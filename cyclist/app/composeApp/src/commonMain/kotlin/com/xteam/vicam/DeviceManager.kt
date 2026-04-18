package com.xteam.vicam

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json

interface SettingsProvider {
    fun getString(key: String, defaultValue: String): String
    fun putString(key: String, value: String)
}

expect fun getSettingsProvider(): SettingsProvider

object DeviceManager {
    private const val CONTACTS_KEY = "emergency_contacts_json"
    private val settings = getSettingsProvider()

    val connectedDevices = mutableStateListOf<BicycleDevice>()
    var selectedDevice by mutableStateOf<BicycleDevice?>(null)
    
    // Global crash state to be shown across any active screen
    var activeCrash by mutableStateOf<CrashEvent?>(null)

    // Globally selected emergency contacts
    val emergencyContacts = mutableStateListOf<EmergencyContact>()

    init {
        loadContacts()
    }

    fun saveContacts() {
        try {
            val json = Json.encodeToString(emergencyContacts.toList())
            settings.putString(CONTACTS_KEY, json)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun loadContacts() {
        try {
            val json = settings.getString(CONTACTS_KEY, "")
            if (json.isNotEmpty()) {
                val contacts = Json.decodeFromString<List<EmergencyContact>>(json)
                emergencyContacts.clear()
                emergencyContacts.addAll(contacts)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}