package com.xteam.vicam

interface SoundManager {
    fun playAlarm()
    fun stopAlarm()
}

expect fun getSoundManager(): SoundManager
