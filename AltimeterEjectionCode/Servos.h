#ifndef SERVOS_H_
#define SERVOS_H_

#include "ESP32Servo.h"

class Servos {
private:
  #define SERVO_PIN 23
  #define CLOSED_POSITION 110
  #define OPEN_POSITION 60
  Servo servo;
  
public:
  void Init();
  void openServo();
  void closeServo();
  void writeServo(int position);
};


#endif