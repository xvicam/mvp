package com.xteam.vicam

import androidx.compose.runtime.Composable

@Composable
actual fun PlatformBackHandler(enabled: Boolean, onBack: () -> Unit) {
    // iOS doesn't have a hardware back button like Android. Navigation is handled via the UI.
}
