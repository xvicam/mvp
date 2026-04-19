package com.xteam.vicam

import android.content.Context
import android.media.AudioAttributes
import android.media.Ringtone
import android.media.RingtoneManager
import android.os.Build

class AndroidSoundManager(private val context: Context) : SoundManager {
    private var ringtone: Ringtone? = null

    override fun playAlarm() {
        try {
            if (ringtone?.isPlaying == true) return

            val notification = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)
            ringtone = RingtoneManager.getRingtone(context, notification)
            
            ringtone?.apply {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    audioAttributes = AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_ALARM)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                        .build()
                }
                
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                    isLooping = true
                }
                
                play()
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun stopAlarm() {
        try {
            ringtone?.stop()
            ringtone = null
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}

object SoundManagerProvider {
    lateinit var manager: SoundManager
}

actual fun getSoundManager(): SoundManager = SoundManagerProvider.manager
