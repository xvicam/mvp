package com.xteam.vicam

import android.content.Context
import android.location.Geocoder
import android.telephony.SmsManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class AndroidEmergencyManager(private val context: Context) : EmergencyManager {
    override suspend fun sendEmergencyAlert(
        userName: String,
        lat: Double,
        lng: Double,
        contacts: List<EmergencyContact>
    ) {
        val address = getAddressFromLocation(lat, lng)
        val time = getCurrentTimeFormatted()
        val message = "$userName has got into a cyclist crash in $address at $time."
        
        withContext(Dispatchers.IO) {
            try {
                val smsManager = context.getSystemService(SmsManager::class.java)
                contacts.forEach { contact ->
                    try {
                        smsManager.sendTextMessage(contact.phoneNumber, null, message, null, null)
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    override suspend fun getAddressFromLocation(lat: Double, lng: Double): String {
        return withContext(Dispatchers.IO) {
            try {
                val geocoder = Geocoder(context, Locale.getDefault())
                val addresses = geocoder.getFromLocation(lat, lng, 1)
                if (!addresses.isNullOrEmpty()) {
                    addresses[0].getAddressLine(0) ?: "$lat, $lng"
                } else {
                    "$lat, $lng"
                }
            } catch (e: Exception) {
                "$lat, $lng"
            }
        }
    }

    override fun getCurrentTimeFormatted(): String {
        val sdf = SimpleDateFormat("HH:mm", Locale.getDefault())
        return sdf.format(Date())
    }
}

object EmergencyManagerProvider {
    lateinit var manager: EmergencyManager
}

actual fun getEmergencyManager(): EmergencyManager = EmergencyManagerProvider.manager
