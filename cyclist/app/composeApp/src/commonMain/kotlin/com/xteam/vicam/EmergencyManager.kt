package com.xteam.vicam

interface EmergencyManager {
    suspend fun sendEmergencyAlert(userName: String, lat: Double, lng: Double, contacts: List<EmergencyContact>)
    suspend fun getAddressFromLocation(lat: Double, lng: Double): String
    fun getCurrentTimeFormatted(): String
}

expect fun getEmergencyManager(): EmergencyManager
