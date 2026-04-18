package com.xteam.vicam

import androidx.compose.animation.*
import androidx.compose.animation.core.animateFloatAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.scale
import androidx.compose.ui.unit.dp

@OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
@Composable
fun EmergencyContactsScreen(
    onBack: () -> Unit
) {
    val contactManager = remember { getContactManager() }
    var allContacts by remember { mutableStateOf<List<EmergencyContact>>(emptyList()) }
    var searchQuery by remember { mutableStateOf("") }
    var isLoading by remember { mutableStateOf(true) }
    
    val originalSelectedIds = remember { DeviceManager.emergencyContacts.map { it.id }.toSet() }
    var currentSelectedIds by remember { mutableStateOf(originalSelectedIds) }
    
    val hasChanges = currentSelectedIds != originalSelectedIds
    var showDiscardDialog by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        try {
            allContacts = contactManager.getPhoneContacts()
        } catch (e: Exception) {
            e.printStackTrace()
        } finally {
            isLoading = false
        }
    }

    PlatformBackHandler(enabled = true) {
        if (hasChanges) {
            showDiscardDialog = true
        } else {
            onBack()
        }
    }

    val filteredGroupedContacts = remember(allContacts, searchQuery) {
        allContacts
            .asSequence()
            .filter { contact ->
                contact.name.contains(searchQuery, ignoreCase = true) ||
                contact.phoneNumber.contains(searchQuery)
            }
            .groupBy { it.name.firstOrNull()?.uppercaseChar() ?: '#' }
            .mapValues { entry -> entry.value.sortedBy { it.name } }
            .toList()
            .sortedBy { it.first }
    }

    val selectedContacts = remember(allContacts, currentSelectedIds) {
        allContacts.filter { it.id in currentSelectedIds }.sortedBy { it.name }
    }

    fun handleBack() {
        if (hasChanges) {
            showDiscardDialog = true
        } else {
            onBack()
        }
    }

    fun saveChanges() {
        DeviceManager.emergencyContacts.clear()
        allContacts.filter { currentSelectedIds.contains(it.id) }.forEach {
            DeviceManager.emergencyContacts.add(it)
        }
        DeviceManager.saveContacts()
        onBack()
    }

    if (showDiscardDialog) {
        AlertDialog(
            onDismissRequest = { showDiscardDialog = false },
            title = { Text("Discard changes?") },
            text = { Text("You have unsaved changes. Do you want to discard them and go back?") },
            confirmButton = {
                TextButton(onClick = onBack) {
                    Text("Discard")
                }
            },
            dismissButton = {
                TextButton(onClick = { showDiscardDialog = false }) {
                    Text("Keep Editing")
                }
            }
        )
    }

    Scaffold(
        topBar = {
            Surface(shadowElevation = 3.dp) {
                Column {
                    TopAppBar(
                        title = { 
                            Column {
                                Text("Emergency Contacts")
                                AnimatedVisibility(
                                    visible = currentSelectedIds.isNotEmpty(),
                                    enter = fadeIn() + expandVertically(),
                                    exit = fadeOut() + shrinkVertically()
                                ) {
                                    Text(
                                        "${currentSelectedIds.size} selected", 
                                        style = MaterialTheme.typography.labelMedium,
                                        color = MaterialTheme.colorScheme.primary
                                    )
                                }
                            }
                        },
                        navigationIcon = {
                            IconButton(onClick = ::handleBack) {
                                Icon(imageVector = Icons.Default.ArrowBack, contentDescription = "Back")
                            }
                        },
                        actions = {
                            AnimatedVisibility(
                                visible = hasChanges,
                                enter = scaleIn() + fadeIn(),
                                exit = scaleOut() + fadeOut()
                            ) {
                                IconButton(onClick = ::saveChanges) {
                                    Icon(imageVector = Icons.Default.Check, contentDescription = "Save")
                                }
                            }
                        }
                    )
                    OutlinedTextField(
                        value = searchQuery,
                        onValueChange = { searchQuery = it },
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(start = 16.dp, end = 16.dp, bottom = 12.dp),
                        placeholder = { Text("Search contacts...") },
                        leadingIcon = { Icon(Icons.Default.Search, contentDescription = null) },
                        singleLine = true,
                        shape = MaterialTheme.shapes.medium
                    )
                }
            }
        }
    ) { padding ->
        if (isLoading) {
            Box(modifier = Modifier.fillMaxSize().padding(padding), contentAlignment = Alignment.Center) {
                CircularProgressIndicator()
            }
        } else if (allContacts.isEmpty()) {
            Box(modifier = Modifier.fillMaxSize().padding(padding), contentAlignment = Alignment.Center) {
                Text("No contacts found. Make sure permissions are granted.")
            }
        } else {
            LazyColumn(modifier = Modifier.fillMaxSize().padding(padding)) {
                if (selectedContacts.isNotEmpty() && searchQuery.isEmpty()) {
                    item {
                        Text(
                            text = "Currently Selected",
                            style = MaterialTheme.typography.titleSmall,
                            color = MaterialTheme.colorScheme.primary,
                            modifier = Modifier.padding(start = 16.dp, top = 16.dp, bottom = 8.dp)
                        )
                    }
                    items(
                        items = selectedContacts, 
                        key = { "selected_${it.id}" },
                        contentType = { "contact" }
                    ) { contact ->
                        ContactItem(
                            modifier = Modifier.animateItem(),
                            contact = contact,
                            isSelected = true,
                            onToggle = {
                                currentSelectedIds = currentSelectedIds - contact.id
                            }
                        )
                    }
                    item {
                        HorizontalDivider(
                            modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
                            thickness = 1.dp,
                            color = MaterialTheme.colorScheme.outlineVariant
                        )
                    }
                }

                if (filteredGroupedContacts.isEmpty()) {
                    item {
                        Box(
                            modifier = Modifier
                                .fillParentMaxWidth()
                                .padding(top = 64.dp),
                            contentAlignment = Alignment.Center
                        ) {
                            Text("No contacts match your search.")
                        }
                    }
                } else {
                    filteredGroupedContacts.forEach { (initial, contactsInGroup) ->
                        stickyHeader(key = "header_$initial") {
                            Surface(
                                modifier = Modifier.fillMaxWidth(),
                                color = MaterialTheme.colorScheme.surfaceVariant
                            ) {
                                Text(
                                    text = initial.toString(),
                                    modifier = Modifier.padding(horizontal = 16.dp, vertical = 4.dp),
                                    style = MaterialTheme.typography.titleSmall,
                                    color = MaterialTheme.colorScheme.primary
                                )
                            }
                        }

                        items(
                            items = contactsInGroup, 
                            key = { it.id },
                            contentType = { "contact" }
                        ) { contact ->
                            val isSelected = currentSelectedIds.contains(contact.id)
                            ContactItem(
                                modifier = Modifier.animateItem(),
                                contact = contact,
                                isSelected = isSelected,
                                onToggle = {
                                    if (isSelected) {
                                        currentSelectedIds = currentSelectedIds - contact.id
                                    } else {
                                        currentSelectedIds = currentSelectedIds + contact.id
                                    }
                                }
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
fun ContactItem(
    modifier: Modifier = Modifier,
    contact: EmergencyContact,
    isSelected: Boolean,
    onToggle: () -> Unit
) {
    val scale by animateFloatAsState(
        targetValue = if (isSelected) 1.02f else 1f,
        animationSpec = tween(200)
    )

    Surface(
        modifier = modifier
            .fillMaxWidth()
            .padding(horizontal = 16.dp, vertical = 4.dp)
            .scale(scale)
            .clickable { onToggle() },
        shape = MaterialTheme.shapes.medium,
        color = animateColorAsState(
            targetValue = if (isSelected) MaterialTheme.colorScheme.primaryContainer else MaterialTheme.colorScheme.surface,
            animationSpec = tween(300)
        ).value,
        tonalElevation = if (isSelected) 0.dp else 1.dp
    ) {
        Row(
            modifier = Modifier.padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(
                    text = contact.name, 
                    style = MaterialTheme.typography.titleMedium,
                    maxLines = 1
                )
                Text(
                    text = contact.phoneNumber, 
                    style = MaterialTheme.typography.bodyMedium,
                    maxLines = 1
                )
            }
            Checkbox(
                checked = isSelected,
                onCheckedChange = { _ -> onToggle() }
            )
        }
    }
}
