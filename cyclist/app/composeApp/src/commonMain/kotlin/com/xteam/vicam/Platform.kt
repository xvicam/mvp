package com.xteam.vicam

interface Platform {
    val name: String
}

expect fun getPlatform(): Platform