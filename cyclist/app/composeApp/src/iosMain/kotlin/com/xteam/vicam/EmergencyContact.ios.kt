package com.xteam.vicam
import platform.Contacts.*
import platform.Foundation.NSError
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import kotlinx.cinterop.ObjCObjectVar
class IosContactManager : ContactManager {
    @OptIn(ExperimentalForeignApi::class)
    override suspend fun getPhoneContacts(): List<EmergencyContact> {
        val store = CNContactStore()
        val keys = listOf(
            CNContactGivenNameKey,
            CNContactFamilyNameKey,
            CNContactPhoneNumbersKey
        )
        val request = CNContactFetchRequest(keysToFetch = keys)
        val contacts = mutableListOf<EmergencyContact>()
        memScoped {
            val error = alloc<ObjCObjectVar<NSError?>>()
            store.enumerateContactsWithFetchRequest(request, error.ptr) { contact, _ ->
                if (contact != null) {
                    val name = "${contact.givenName} ${contact.familyName}".trim()
                    val id = contact.identifier
                    val phoneNumbers = contact.phoneNumbers
                    if (phoneNumbers.isNotEmpty()) {
                        val number = (phoneNumbers.first() as? platform.Contacts.CNLabeledValue)?.value as? platform.Contacts.CNPhoneNumber
                        val phoneString = number?.stringValue ?: ""
                        contacts.add(EmergencyContact(id, name, phoneString))
                    }
                }
            }
        }
        return contacts.distinctBy { it.phoneNumber }
    }
}
actual fun getContactManager(): ContactManager = IosContactManager()
