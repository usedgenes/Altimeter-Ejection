#include "IMU.h"

bool IMU::Init(SPIClass & _hspi) {
  hspi = _hspi;
  accel = new Bmi088Accel(hspi, ACCEL_CS);
  gyro = new Bmi088Gyro(hspi, GYRO_CS);
  int status;
  status = accel->begin();
  if (status < 0) {
    return false;
  }
  status = gyro->begin();
  if (status < 0) {
    return false;
  }
  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(GYRO_CS, HIGH);
  return true;
}

void IMU::getIMUData(float accelerometer[], float gyroscope[]) {
  digitalWrite(ACCEL_CS, LOW);
  digitalWrite(GYRO_CS, LOW);
  while(accel->begin() < 0) {
    delay(10);
  }
  while(gyro->begin() < 0) {
    delay(10);
  }

  accel->readSensor();
  gyro->readSensor();

  accelerometer[0] = accel->getAccelX_mss();
  accelerometer[1] = accel->getAccelY_mss();
  accelerometer[2] = accel->getAccelZ_mss();

  gyroscope[0] = gyro->getGyroX_rads();
  gyroscope[1] = gyro->getGyroY_rads();
  gyroscope[2] = gyro->getGyroZ_rads();
  digitalWrite(ACCEL_CS, HIGH);
  digitalWrite(GYRO_CS, HIGH);
}


