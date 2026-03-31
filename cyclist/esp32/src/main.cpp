#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// Simple ESP-NOW broadcaster for ESP32
namespace espNow{
  // Broadcast MAC address (all 0xFF -> broadcast)
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // Peer info struct for ESP-NOW operations
  esp_now_peer_info_t peerInfo;

  // Last time we sent a message (ms)
  uint32_t lastSendMs = 0;
  // How often to send messages (milliseconds)
  constexpr uint32_t kSendPeriodMs = 250;
  // Simple message counter to vary payload
  uint32_t messageCounter = 0;

  // Initialize serial port and built-in LED
  void initSerialAndLed() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Blink the on-board LED briefly to indicate a successful send
  void blinkOk() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }

  // Blink the on-board LED slowly to indicate an error
  void blinkError() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }

  // Configure WiFi for STA mode and initialise ESP-NOW
  bool initEspNow() {
    WiFi.mode(WIFI_STA);
    // Use long-range protocol and set TX power
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    esp_wifi_set_max_tx_power(78);

    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initialising ESP-NOW");
      return false;
    }

    // Prepare a broadcast peer entry
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Register the broadcast peer
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add broadcast peer");
      return false;
    }

    return true;
  }

  // Build a small message and send it via ESP-NOW to broadcast address
  void sendEspNowBroadcast() {
    char message[64];
    snprintf(message, sizeof(message), "Hello ESP-NOW %lu", static_cast<unsigned long>(messageCounter++));

    // Send including terminating null (hence +1)
    esp_err_t result = esp_now_send(broadcastAddress, reinterpret_cast<const uint8_t*>(message), strlen(message) + 1);
    if (result == ESP_OK) {
      // Indicate success with a short LED blink
      blinkOk();
    } else {
      Serial.print("ESP-NOW send failed: ");
      Serial.println(static_cast<int>(result));
      blinkError();
    }
  }
}

void setup() {
  // Prepare debugging output and LED
  espNow::initSerialAndLed();
  // Initialise ESP-NOW (peer added inside)
  espNow::initEspNow();
}

void loop() {
  // Periodically send messages, non-blocking timing
  const uint32_t now = millis();
  if (now - espNow::lastSendMs >= espNow::kSendPeriodMs) {
    espNow::lastSendMs = now;
    espNow::sendEspNowBroadcast();
  }
}