package com.xteam.vicam

import kotlinx.serialization.Serializable

@Serializable
data class EmergencyContact(
    val id: String,
    val name: String,
    val phoneNumber: String,
    val isFavorite: Boolean = false,
    val isSelected: Boolean = false
)

interface ContactManager {
    suspend fun getPhoneContacts(): List<EmergencyContact>
}

expect fun getContactManager(): ContactManager
