#include <Arduino.h>

#define MOTOR_PIN   25

#define RED_PIN     16
#define GREEN_PIN   17
#define BLUE_PIN    18

#define PWM_FREQ    5000
#define PWM_RES     8     // 0-255

void setup() {
  ledcAttach(MOTOR_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(RED_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(GREEN_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(BLUE_PIN, PWM_FREQ, PWM_RES);
}

void setRGB(int r, int g, int b) {
  ledcWrite(RED_PIN, r);
  ledcWrite(GREEN_PIN, g);
  ledcWrite(BLUE_PIN, b);
}

void loop() {
  // 1. Blue, Motor OFF
  setRGB(0, 0, 255);
  ledcWrite(MOTOR_PIN, 0);
  delay(2000);

  // 2. Yellow, Motor LOW
  setRGB(255, 255, 0);
  ledcWrite(MOTOR_PIN, 80);
  delay(2000);

  // 3. Orange, Motor MID
  setRGB(255, 120, 0);
  ledcWrite(MOTOR_PIN, 160);
  delay(2000);

  // 4. Red, Motor HIGH
  setRGB(255, 0, 0);
  ledcWrite(MOTOR_PIN, 255);
  delay(2000);
}