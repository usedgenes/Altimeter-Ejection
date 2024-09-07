#include "Servos.h"

void Servos::Init() {
  servo.write(OPEN_POSITION);
  servo.attach(SERVO_PIN);
  servo.write(OPEN_POSITION);
}

void Servos::openServo() {
  servo.write(OPEN_POSITION);
  Serial.println("Opened Parachute");
}

void Servos::closeServo() {
  servo.write(CLOSED_POSITION);
  Serial.println("Closed Parachute");
}

void Servos::writeServo(int position) {
  servo.write(position);
}
