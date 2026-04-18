package com.xteam.vicam

import android.content.Context
import android.provider.ContactsContract
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class AndroidContactManager(private val context: Context) : ContactManager {
    override suspend fun getPhoneContacts(): List<EmergencyContact> = withContext(Dispatchers.IO) {
        val contactsList = mutableListOf<EmergencyContact>()
        val projection = arrayOf(
            ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
            ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME,
            ContactsContract.CommonDataKinds.Phone.NUMBER
        )
        val cursor = context.contentResolver.query(
            ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
            projection,
            null,
            null,
            ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME + " ASC"
        )
        
        val seenContactIds = mutableSetOf<String>()
        
        cursor?.use {
            val idIndex = it.getColumnIndex(ContactsContract.CommonDataKinds.Phone.CONTACT_ID)
            val nameIndex = it.getColumnIndex(ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME)
            val numberIndex = it.getColumnIndex(ContactsContract.CommonDataKinds.Phone.NUMBER)
            while (it.moveToNext()) {
                val contactId = it.getString(idIndex)
                // Only take the first phone number for each unique contact ID
                // to avoid the "duplicates" issue and ensure unique IDs for the list keys.
                if (seenContactIds.add(contactId)) {
                    val name = it.getString(nameIndex) ?: ""
                    val number = it.getString(numberIndex) ?: ""
                    contactsList.add(EmergencyContact(contactId, name, number))
                }
            }
        }
        // Further deduplicate by phone number in case the same number is stored under different names
        contactsList.distinctBy { it.phoneNumber }
    }
}

object ContactManagerProvider {
    lateinit var manager: ContactManager
}

actual fun getContactManager(): ContactManager = ContactManagerProvider.manager
