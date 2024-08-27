//change BMI088 G range
#include "Bluetooth.h"
#include "IMU.h"
#include "Servos.h"
#include "Altimeter.h"
#include "Logger.h"
#include "Utilities.h"

#define SPI1_SCK 5
#define SPI1_MISO 16
#define SPI1_MOSI 17
#define SPI1_CS 21
#define SPI2_SCK 27
#define SPI2_MISO 25
#define SPI2_MOSI 26
#define SPI2_CS 33

#define TARGET_ALTITUDE 240.8
#define LAUNCH_ALTITUDE_THRESHOLD_METERS 4
#define MOTOR_BURN_TIME_MILLISECONDS 3000
#define PARACHUTE_EJECTION_VELOCITY_THRESHOLD 3
#define RECOVERY_ALTITUDE_THRESHOLD_METERS 1000

#define ON_PAD_DATA_FREQUENCY 1000
#define MOTOR_ACTIVE_DATA_FREQUENCY 1000
#define COASTING_DATA_FREQUENCY 1000
#define PARACHUTE_OUT_DATA_FREQUENCY 1000
#define ON_GROUND_DATA_FREQUENCY 1000

SPIClass* vspi = NULL;
SPIClass* hspi = NULL;

Bluetooth bluetooth;
IMU imu;
Servos servos;
Altimeter altimeter;
Logger logger;
Utilities utilities;

unsigned long currentTime = 0;
unsigned long previousTime = 0;
unsigned long loopTime = 0;

unsigned long lastDataLog = 0;

float currentAltitude = 0;
float currentTemperature = 0;
float currentPressure = 0;
float launchAltitude = 0;
unsigned long motorIgnitionTime = 0;
float previousAltitude = 0;

float accelerometer[] = { 0, 0, 0 };
float gyroscope[] = { 0, 0, 0 };
float apogee = 0;

bool sendBluetoothData = false;
bool bluetoothConnected = false;
bool armed = false;
bool bluetoothBypassOnPad = false;
bool bluetoothBypassMotorActive = false;
bool bluetoothBypassCoasting = false;
bool bluetoothBypassParachuteOut = false;
bool sendLoopTime = false;
bool sendBluetoothBMI088 = false;
bool sendBluetoothAltimeter = false;
bool ejectedAltimeter = false;

void setup() {
  Serial.begin(115200);
  bluetooth.Init(&servos, &imu, &altimeter,&armed, &bluetoothConnected, &sendLoopTime, &sendBluetoothBMI088, &sendBluetoothAltimeter, &bluetoothBypassOnPad, &bluetoothBypassMotorActive, &bluetoothBypassCoasting, &bluetoothBypassParachuteOut);
  while (!bluetoothConnected) {
    delay(1000);
  }
  vspi = new SPIClass(VSPI);
  hspi = new SPIClass(HSPI);
  vspi->begin(SPI1_SCK, SPI1_MISO, SPI1_MOSI, SPI1_CS);
  hspi->begin(SPI2_SCK, SPI2_MISO, SPI2_MOSI, SPI2_CS);
  utilities.Init();
  servos.Init();

  if (!imu.Init(*hspi)) {
    bluetooth.writeUtilitiesNotifications("IMU Initialization Error");
    Serial.println("IMU Initialization Error");
    while (1) {}
  }
  if (!altimeter.Init(vspi)) {
    bluetooth.writeUtilitiesNotifications("Altimeter Initialization Error");
    Serial.println("BMP390 Initialization Error");
    while (1) {}
  }
  if (!logger.Init(*vspi)) {
    bluetooth.writeUtilitiesNotifications("SD Initialization Error");
    Serial.println("SD Initialization Error");
  }

  utilities.initialized();

  while (!armed) {
    dataLoop();
    logData(ON_PAD_DATA_FREQUENCY);
    delay(100);
  }

  previousTime = 0;
  launchAltitude = altimeter.getAltitude();
  bluetooth.writeUtilitiesNotifications("Launch Altitude: " + String(launchAltitude));
  logger.log(Events, "Launch Altitude: " + String(launchAltitude), millis());
  Serial.println("Launch Altitude: " + String(launchAltitude));

  utilities.armed();
  bluetooth.writeUtilitiesEvents("On Pad");
  logger.log(Events, "On Pad", millis());
  Serial.println("On Pad");
  while (altimeter.getFilteredAltitude() - launchAltitude < LAUNCH_ALTITUDE_THRESHOLD_METERS && !bluetoothBypassOnPad) {
    onPad();
  }

  motorIgnitionTime = millis();
  bluetooth.writeUtilitiesEvents("Motor Active");
  logger.log(Events, "Motor Active", millis());
  Serial.println("Motor Vector Active");
  while (millis() - motorIgnitionTime < MOTOR_BURN_TIME_MILLISECONDS && !bluetoothBypassMotorActive) {
    motorActive();
  }

  bluetooth.writeUtilitiesEvents("Coasting");
  logger.log(Events, "Coasting", millis());
  Serial.println("Coasting");
  while ((altimeter.getFilteredAltitude() - previousAltitude) / loopTime < PARACHUTE_EJECTION_VELOCITY_THRESHOLD && !bluetoothBypassCoasting) {
    coasting();
    if(currentAltitude >= TARGET_ALTITUDE && !ejectedAltimeter) {
      ejectAltimeter();
      ejectedAltimeter = true;
      logger.log(Events, "Deploying Parachute", millis());
    }
  }

  bluetooth.writeUtilitiesEvents("Parachute Out");
  logger.log(Events, "Parachute Out", millis());
  Serial.println("Parachute Out");
  while (altimeter.getFilteredAltitude() - launchAltitude > RECOVERY_ALTITUDE_THRESHOLD_METERS && !bluetoothBypassParachuteOut) {
    parachuteOut();
  }

  bluetooth.writeUtilitiesEvents("Touchdown");
  logger.log(Events, "Touchdown", millis());
  Serial.println("Touchdown");
  while (true) {
    touchdown();
  }
}

void loop() {
}

void onPad() {
  dataLoop();
  logData(ON_PAD_DATA_FREQUENCY);
}

void motorActive() {
  dataLoop();
  logData(MOTOR_ACTIVE_DATA_FREQUENCY);
}

void coasting() {
  dataLoop();
  logData(COASTING_DATA_FREQUENCY);
}

void ejectAltimeter() {
  servos.openServo();
  logger.log(Events, "Altimeter Ejected", currentTime);
}

void parachuteOut() {
  dataLoop();
  logData(PARACHUTE_OUT_DATA_FREQUENCY);
}

void touchdown() {
  dataLoop();
  logData(ON_GROUND_DATA_FREQUENCY);
  utilities.readApogee(apogee);
}

void dataLoop() {
  currentTime = millis();
  loopTime = millis() - previousTime;
  previousTime = currentTime;

  imu.getIMUData(accelerometer, gyroscope);
  currentAltitude = altimeter.getAltitude();
  altimeter.getTempAndPressure(currentTemperature, currentPressure);
}

void logData(int dataLoggingFrequencyInMilliseconds) {
  if (millis() - lastDataLog >= dataLoggingFrequencyInMilliseconds) {
    lastDataLog = millis();
    if (sendBluetoothBMI088) {
      Serial.println("Sending Bluetooth BMI088");
      bluetooth.writeIMU("90" + String(accelerometer[0]));
      bluetooth.writeIMU("91" + String(accelerometer[1]));
      bluetooth.writeIMU("92" + String(accelerometer[2]));
      bluetooth.writeIMU("93" + String(gyroscope[0]));
      bluetooth.writeIMU("94" + String(gyroscope[1]));
      bluetooth.writeIMU("95" + String(gyroscope[2]));
    }
    if (sendBluetoothAltimeter) {
      Serial.println("Sending Bluetooth Altimeter");
      bluetooth.writeAltimeter("90" + String(currentAltitude));
      bluetooth.writeAltimeter("91" + String(currentTemperature));
      bluetooth.writeAltimeter("92" + String(currentPressure));
    }
    if (sendLoopTime) {
      Serial.println("Sending Bluetooth Loop Time");
      bluetooth.writeUtilities("90" + String(loopTime));
    }
    logger.log(Accelerometer, String(accelerometer[0]) + "\t" + String(accelerometer[1]) + "\t" + String(accelerometer[2]), currentTime);
    logger.log(Gyroscope, String(gyroscope[0]) + "\t" + String(gyroscope[1]) + "\t" + String(gyroscope[2]), currentTime);
    logger.log(Altitude, String(currentAltitude), currentTime);
    printToSerial();
  }
}

void printToSerial() {
  Serial.println("Accelerometer: " + String(accelerometer[0]) + "\t" + String(accelerometer[1]) + "\t" + String(accelerometer[2]));
  Serial.println("Gyroscope: " + String(gyroscope[0]) + "\t" + String(gyroscope[1]) + "\t" + String(gyroscope[2]));
  Serial.println("Altitude: " + String(currentAltitude) + "\tTemperature: " + String(currentTemperature) + "\tPressure: " + String(currentPressure));
}