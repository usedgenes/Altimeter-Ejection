#ifndef SERVOS_H_
#define SERVOS_H_

#include "ESP32Servo.h"

class Servos {
private:
  #define SERVO_PIN 13
  #define BLOCKING_POSITION 90
  #define OPEN_POSITION 50
  Servo servo;
  
public:
  void Init();
  void openServo();
  void closeServo();
};


#endif