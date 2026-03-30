#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Wire.h>

#include "DFRobot_GNSS.h"

#include "AccGyro.h"
#include "CrashDetector.h"

// Remove local override; use the board variant's LED_BUILTIN.
// #define LED_BUILTIN 2

namespace {
// ESP-NOW broadcast
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;
int messageCounter = 0;

DFRobot_GNSS_I2C gnss(&Wire, GNSS_DEVICE_ADDR);
AccGyro imu;
CrashDetector crashDetector;

// Simple scheduler
uint32_t lastImuMs = 0;
uint32_t lastRadioMs = 0;
constexpr uint32_t kImuPeriodMs = 20;   // 50 Hz
constexpr uint32_t kRadioPeriodMs = 250;

void initSerialAndLed() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void initI2C() {
  // Explicit I2C init before any I2C peripherals.
  Wire.begin();
}

void initGnss() {
  // Avoid hanging forever on boot if GNSS isn't connected.
  const uint32_t start = millis();
  while (!gnss.begin()) {
    Serial.println("Error initialising GNSS");
    delay(500);
    if (millis() - start > 5000) {
      Serial.println("GNSS init timed out; continuing without GNSS");
      return;
    }
  }

  gnss.enablePower();
  gnss.setGnss(eGPS_BeiDou_GLONASS);
}

bool initEspNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initialising ESP-NOW");
    return false;
  }

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return false;
  }

  return true;
}

bool initImu() {
  Serial.println("Adafruit LSM6DSOX test!");
  if (!imu.beginI2C(Wire)) {
    Serial.println("Failed to find LSM6DSOX chip");
    return false;
  }
  Serial.println("LSM6DSOX Found!");
  return true;
}

void blinkOk() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}

void sendEspNowHello() {
  char message[64];
  snprintf(message, sizeof(message), "Hello from FireBeetle! %d", messageCounter++);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)message, strlen(message));
  if (result == ESP_OK) blinkOk();
}

void printUtcHourIfAvailable() {
  // If GNSS wasn't initialised, this will likely return zeros; still safe.
  sTim_t utc = gnss.getUTC();
  Serial.println(utc.hour);
}

void readPrintAndDetectCrash() {
  ImuSample s;
  imu.readSample(s);

  ImuOrientation o = imu.computeOrientation(s);

  const uint32_t now = millis();
  CrashEvent ev = crashDetector.update(s, now);

  Serial.print("Orientation: ");
  Serial.print(o.orientationLabel);
  Serial.print(" | Pitch: ");
  Serial.print(o.pitchDeg);
  Serial.print("° | Roll: ");
  Serial.print(o.rollDeg);
  Serial.print("° | Status: ");
  Serial.print(o.isMoving ? "Moving" : "Stationary");

  if (ev.triggered) {
    Serial.print(" | CRASH DETECTED!");
    Serial.print(" a=");
    Serial.print(ev.accelMag);
    Serial.print(" jerk=");
    Serial.print(ev.jerk);
    Serial.print(" gyro=");
    Serial.print(ev.gyroMag);
    Serial.print(" peakA=");
    Serial.print(ev.peakAccelMag);
    Serial.print(" peakG=");
    Serial.print(ev.peakGyroMag);
  }

  Serial.println();
}
} // namespace

void setup() {
  initSerialAndLed();
  initI2C();

  initGnss();
  initEspNow();
  initImu();
}

void loop() {
  const uint32_t now = millis();

  if (now - lastRadioMs >= kRadioPeriodMs) {
    lastRadioMs = now;
    printUtcHourIfAvailable();
    sendEspNowHello();
  }

  if (now - lastImuMs >= kImuPeriodMs) {
    lastImuMs = now;
    readPrintAndDetectCrash();
  }
}
