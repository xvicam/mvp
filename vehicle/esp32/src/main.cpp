#include <Arduino.h>

#define MOTOR_PIN 25  // GPIO connected to MOSFET gate

void setup() {
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
}

void loop() {
  digitalWrite(MOTOR_PIN, HIGH); // turn ON (MOSFET conducts)
  delay(2000);

  digitalWrite(MOTOR_PIN, LOW);  // turn OFF
  delay(2000);
}