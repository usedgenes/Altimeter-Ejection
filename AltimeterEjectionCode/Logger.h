#ifndef LOGGER_H_
#define LOGGER_H_

#include "SD.h"
#include "SPI.h"

enum LogType {
  Altitude,
  Accelerometer,
  Gyroscope,
  Events,
};

class Logger {
#define SD_SCK 5
#define SD_MISO 16
#define SD_MOSI 17
#define SD_CS 18

private:
SPIClass vspi;
public:
  bool Init(SPIClass & vspi);
  void log(LogType type, String _message, unsigned long time);
};

#endif