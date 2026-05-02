#include <Arduino.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <esp_now.h>

// ===================== HARDWARE =====================

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

constexpr uint8_t RED_PIN = 26;
constexpr uint8_t GREEN_PIN = 13;
constexpr uint8_t BLUE_PIN = 14;
constexpr uint8_t MOTOR_PIN = 25;

constexpr uint8_t GPS_RX = 16;
constexpr uint8_t GPS_TX = 17;

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t GPS_BAUD = 9600;

constexpr uint32_t PWM_FREQ = 5000;
constexpr uint8_t PWM_RES = 8;

constexpr bool RGB_COMMON_ANODE = false;

// ===================== SETTINGS =====================

constexpr uint32_t GPS_MAX_AGE_MS = 3000;
constexpr uint32_t CYCLIST_TIMEOUT_MS = 3000;
constexpr uint32_t DEBUG_INTERVAL_MS = 500;

constexpr float MOVING_ON_KMPH = 3.0;
constexpr float MOVING_OFF_KMPH = 1.0;

constexpr double ALERT_DISTANCE_M = 30.0;
constexpr double WARNING_DISTANCE_M = 15.0;
constexpr double URGENT_DISTANCE_M = 8.0;

constexpr float MAX_REASONABLE_SPEED_KMPH = 80.0;

constexpr uint8_t MAX_CYCLISTS = 12;
constexpr size_t MAX_PACKET_SIZE = 250;

// ===================== MODES / STATES =====================

enum class Mode : uint8_t {
  REAL,
  MANUAL
};

enum class State : uint8_t {
  OFF,
  GPS_WAIT,
  IDLE,
  MOVING,
  ALERT,
  WARNING,
  URGENT
};

Mode currentMode = Mode::REAL;
State currentState = State::GPS_WAIT;
State manualState = State::OFF;

bool manualPowerOn = false;
bool vehicleMoving = false;

// ===================== CYCLIST DATA =====================

struct CyclistData {
  bool active = false;
  uint8_t mac[6] = {0};

  double lat = 0.0;
  double lng = 0.0;
  float speedKmph = 0.0;

  uint32_t lastSeenMs = 0;
};

CyclistData cyclists[MAX_CYCLISTS];

double closestDistanceM = -1.0;
int closestCyclistIndex = -1;

// ===================== PACKET BUFFER =====================

struct PendingPacket {
  bool available = false;
  bool overflow = false;
  uint8_t mac[6] = {0};
  char data[MAX_PACKET_SIZE];
  int len = 0;
};

PendingPacket pendingPacket;
portMUX_TYPE packetMux = portMUX_INITIALIZER_UNLOCKED;

// ===================== BASIC HELPERS =====================

const char* modeName(Mode mode) {
  return mode == Mode::REAL ? "REAL" : "MANUAL";
}

const char* stateName(State state) {
  switch (state) {
    case State::OFF: return "OFF";
    case State::GPS_WAIT: return "GPS_WAIT";
    case State::IDLE: return "IDLE";
    case State::MOVING: return "MOVING";
    case State::ALERT: return "ALERT";
    case State::WARNING: return "WARNING";
    case State::URGENT: return "URGENT";
    default: return "UNKNOWN";
  }
}

bool sameMac(const uint8_t a[6], const uint8_t b[6]) {
  return memcmp(a, b, 6) == 0;
}

void printMac(const uint8_t mac[6]) {
  char buffer[18];
  snprintf(
    buffer,
    sizeof(buffer),
    "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0],
    mac[1],
    mac[2],
    mac[3],
    mac[4],
    mac[5]
  );
  Serial.print(buffer);
}

bool validCoordinate(double lat, double lng) {
  return lat >= -90.0 && lat <= 90.0 && lng >= -180.0 && lng <= 180.0;
}

void writeLed(uint8_t pin, uint8_t value) {
  if (RGB_COMMON_ANODE && (pin == RED_PIN || pin == GREEN_PIN || pin == BLUE_PIN)) {
    value = 255 - value;
  }

  ledcWrite(pin, value);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  writeLed(RED_PIN, r);
  writeLed(GREEN_PIN, g);
  writeLed(BLUE_PIN, b);
}

void setMotor(uint8_t duty) {
  ledcWrite(MOTOR_PIN, duty);
}

void applyOutput(State state) {
  switch (state) {
    case State::OFF:
      setRGB(0, 0, 0);
      setMotor(0);
      break;

    case State::GPS_WAIT:
      setRGB(80, 0, 80);
      setMotor(0);
      break;

    case State::IDLE:
      setRGB(0, 255, 0);
      setMotor(0);
      break;

    case State::MOVING:
      setRGB(0, 0, 255);
      setMotor(0);
      break;

    case State::ALERT:
      setRGB(255, 255, 0);
      setMotor(80);
      break;

    case State::WARNING:
      setRGB(255, 120, 0);
      setMotor(160);
      break;

    case State::URGENT:
      setRGB(255, 0, 0);
      setMotor(255);
      break;
  }
}

// ===================== GPS =====================

bool vehicleGpsValid() {
  return gps.location.isValid() &&
         gps.location.age() <= GPS_MAX_AGE_MS &&
         validCoordinate(gps.location.lat(), gps.location.lng());
}

float vehicleSpeedKmph() {
  if (!gps.speed.isValid()) {
    return 0.0;
  }

  float speed = gps.speed.kmph();

  if (speed < 0.0 || speed > 250.0) {
    return 0.0;
  }

  return speed;
}

void updateVehicleMoving(float speedKmph) {
  if (!vehicleMoving && speedKmph >= MOVING_ON_KMPH) {
    vehicleMoving = true;
  } else if (vehicleMoving && speedKmph <= MOVING_OFF_KMPH) {
    vehicleMoving = false;
  }
}

// ===================== CYCLIST TRACKING =====================

int findCyclistByMac(const uint8_t mac[6]) {
  for (uint8_t i = 0; i < MAX_CYCLISTS; i++) {
    if (cyclists[i].active && sameMac(cyclists[i].mac, mac)) {
      return i;
    }
  }

  return -1;
}

int findFreeCyclistSlot() {
  for (uint8_t i = 0; i < MAX_CYCLISTS; i++) {
    if (!cyclists[i].active) {
      return i;
    }
  }

  return -1;
}

int findOldestCyclistSlot() {
  uint8_t oldestIndex = 0;
  uint32_t oldestTime = cyclists[0].lastSeenMs;

  for (uint8_t i = 1; i < MAX_CYCLISTS; i++) {
    if (cyclists[i].lastSeenMs < oldestTime) {
      oldestTime = cyclists[i].lastSeenMs;
      oldestIndex = i;
    }
  }

  return oldestIndex;
}

void removeExpiredCyclists() {
  uint32_t now = millis();

  for (uint8_t i = 0; i < MAX_CYCLISTS; i++) {
    if (cyclists[i].active && now - cyclists[i].lastSeenMs > CYCLIST_TIMEOUT_MS) {
      cyclists[i].active = false;
    }
  }
}

bool parseCyclistPacket(const uint8_t mac[6], const char* json) {
  StaticJsonDocument<256> doc;

  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    Serial.print("Bad JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  if (!doc["lat"].is<double>() || !doc["lng"].is<double>()) {
    Serial.println("Rejected packet: missing lat/lng");
    return false;
  }

  double lat = doc["lat"].as<double>();
  double lng = doc["lng"].as<double>();
  float speedKmph = doc["speed"] | 0.0;

  if (!validCoordinate(lat, lng)) {
    Serial.println("Rejected packet: invalid coordinates");
    return false;
  }

  if (speedKmph < 0.0 || speedKmph > MAX_REASONABLE_SPEED_KMPH) {
    Serial.println("Rejected packet: invalid speed");
    return false;
  }

  int index = findCyclistByMac(mac);

  if (index < 0) {
    index = findFreeCyclistSlot();
  }

  if (index < 0) {
    index = findOldestCyclistSlot();
  }

  cyclists[index].active = true;
  memcpy(cyclists[index].mac, mac, 6);
  cyclists[index].lat = lat;
  cyclists[index].lng = lng;
  cyclists[index].speedKmph = speedKmph;
  cyclists[index].lastSeenMs = millis();

  return true;
}

void processPendingPacket() {
  PendingPacket local;

  portENTER_CRITICAL(&packetMux);
  local = pendingPacket;
  pendingPacket.available = false;
  pendingPacket.overflow = false;
  portEXIT_CRITICAL(&packetMux);

  if (local.overflow) {
    Serial.println("Rejected ESP-NOW packet: too large");
    return;
  }

  if (!local.available) {
    return;
  }

  local.data[local.len] = '\0';

  Serial.print("RX from ");
  printMac(local.mac);
  Serial.print(": ");
  Serial.println(local.data);

  parseCyclistPacket(local.mac, local.data);
}

// ===================== REAL RISK CALCULATION =====================

State calculateRealState() {
  closestDistanceM = -1.0;
  closestCyclistIndex = -1;

  if (!vehicleGpsValid()) {
    return State::GPS_WAIT;
  }

  float vSpeed = vehicleSpeedKmph();
  updateVehicleMoving(vSpeed);

  removeExpiredCyclists();

  double vehicleLat = gps.location.lat();
  double vehicleLng = gps.location.lng();

  for (uint8_t i = 0; i < MAX_CYCLISTS; i++) {
    if (!cyclists[i].active) {
      continue;
    }

    double distanceM = TinyGPSPlus::distanceBetween(
      vehicleLat,
      vehicleLng,
      cyclists[i].lat,
      cyclists[i].lng
    );

    if (closestDistanceM < 0 || distanceM < closestDistanceM) {
      closestDistanceM = distanceM;
      closestCyclistIndex = i;
    }
  }

  if (closestCyclistIndex < 0) {
    return vehicleMoving ? State::MOVING : State::IDLE;
  }

  bool cyclistMoving = cyclists[closestCyclistIndex].speedKmph >= MOVING_ON_KMPH;
  bool relevantMotion = vehicleMoving || cyclistMoving;

  if (!relevantMotion) {
    return State::IDLE;
  }

  if (closestDistanceM <= URGENT_DISTANCE_M) {
    return State::URGENT;
  }

  if (closestDistanceM <= WARNING_DISTANCE_M) {
    return State::WARNING;
  }

  if (closestDistanceM <= ALERT_DISTANCE_M) {
    return State::ALERT;
  }

  return vehicleMoving ? State::MOVING : State::IDLE;
}

// ===================== SERIAL CONTROL =====================

void toggleMode() {
  if (currentMode == Mode::REAL) {
    currentMode = Mode::MANUAL;
    currentState = manualState;
  } else {
    currentMode = Mode::REAL;
  }

  Serial.print("Mode switched to: ");
  Serial.println(modeName(currentMode));
}

void handleManualCommand(char command) {
  if (command == '0') {
    manualPowerOn = !manualPowerOn;
    manualState = manualPowerOn ? State::IDLE : State::OFF;
  } else if (command == '1') {
    manualPowerOn = true;
    manualState = State::IDLE;
  } else if (command == '2') {
    manualPowerOn = true;
    manualState = State::ALERT;
  } else if (command == '3') {
    manualPowerOn = true;
    manualState = State::WARNING;
  } else if (command == '4') {
    manualPowerOn = true;
    manualState = State::URGENT;
  } else {
    return;
  }

  currentState = manualState;

  Serial.print("Manual state: ");
  Serial.println(stateName(manualState));
}

void processSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r' || c == ' ') {
      continue;
    }

    if (c == 'M' || c == 'm') {
      toggleMode();
      continue;
    }

    if (currentMode == Mode::MANUAL) {
      handleManualCommand(c);
    } else {
      Serial.println("Ignored command. Press M to enter MANUAL mode.");
    }
  }
}

// ===================== DEBUG =====================

uint8_t activeCyclistCount() {
  uint8_t count = 0;

  for (uint8_t i = 0; i < MAX_CYCLISTS; i++) {
    if (cyclists[i].active) {
      count++;
    }
  }

  return count;
}

void printDebug() {
  static uint32_t lastDebugMs = 0;

  if (millis() - lastDebugMs < DEBUG_INTERVAL_MS) {
    return;
  }

  lastDebugMs = millis();

  Serial.print("Mode: ");
  Serial.print(modeName(currentMode));

  Serial.print(" | State: ");
  Serial.print(stateName(currentState));

  Serial.print(" | GPS: ");
  Serial.print(vehicleGpsValid() ? "valid" : "invalid");

  Serial.print(" | Vehicle speed: ");
  Serial.print(vehicleSpeedKmph());
  Serial.print(" km/h");

  Serial.print(" | Moving: ");
  Serial.print(vehicleMoving ? "yes" : "no");

  Serial.print(" | Cyclists: ");
  Serial.print(activeCyclistCount());

  Serial.print(" | Closest: ");

  if (closestCyclistIndex >= 0 && closestDistanceM >= 0) {
    Serial.print(closestDistanceM);
    Serial.print(" m from ");
    printMac(cyclists[closestCyclistIndex].mac);
  } else {
    Serial.print("none");
  }

  if (gps.time.isValid()) {
    char timeBuffer[12];
    snprintf(
      timeBuffer,
      sizeof(timeBuffer),
      "%02d:%02d:%02d",
      gps.time.hour(),
      gps.time.minute(),
      gps.time.second()
    );

    Serial.print(" | UTC: ");
    Serial.print(timeBuffer);
  }

  Serial.println();
}

// ===================== ESP-NOW CALLBACK =====================

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (!info || !data || len <= 0) {
    return;
  }

  portENTER_CRITICAL_ISR(&packetMux);

  if (len >= MAX_PACKET_SIZE) {
    pendingPacket.overflow = true;
  } else {
    memcpy(pendingPacket.mac, info->src_addr, 6);
    memcpy(pendingPacket.data, data, len);
    pendingPacket.len = len;
    pendingPacket.available = true;
  }

  portEXIT_CRITICAL_ISR(&packetMux);
}

// ===================== SETUP / LOOP =====================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  ledcAttach(MOTOR_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(RED_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(GREEN_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(BLUE_PIN, PWM_FREQ, PWM_RES);

  applyOutput(State::GPS_WAIT);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed. Manual mode still available.");
    currentMode = Mode::MANUAL;
    manualState = State::OFF;
    currentState = State::OFF;
    applyOutput(currentState);
    return;
  }

  esp_now_register_recv_cb(onReceive);

  Serial.println("VICAM vehicle receiver started.");
  Serial.println("Press M to switch REAL / MANUAL mode.");
  Serial.println("Manual mode: 0=OFF/ON, 1=IDLE, 2=ALERT, 3=WARNING, 4=URGENT");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  processSerialInput();
  processPendingPacket();

  if (currentMode == Mode::REAL) {
    currentState = calculateRealState();
  } else {
    currentState = manualState;
  }

  applyOutput(currentState);
  printDebug();
}