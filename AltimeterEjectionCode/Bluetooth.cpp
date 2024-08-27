#include "Bluetooth.h"

void Bluetooth::Init(Servos *_servos, IMU *_imu, Altimeter *_altimeter, bool *_armed, bool *_bluetoothConnected, bool *_sendLoopTime, bool *_sendBluetoothBMI088, bool *_sendBluetoothAltimeter, bool *_bluetoothBypassOnPad, bool *_bluetoothBypassMotorActive, bool *_bluetoothBypassCoasting, bool *_bluetoothBypassParachuteOut) {
  servos = _servos;
  imu = _imu;
  altimeter = _altimeter;
  armed = _armed;
  bluetoothConnected = _bluetoothConnected;
  sendLoopTime = _sendLoopTime;
  sendBluetoothBMI088 = _sendBluetoothBMI088;
  sendBluetoothAltimeter = _sendBluetoothAltimeter;
  bluetoothBypassOnPad = _bluetoothBypassOnPad;
  bluetoothBypassMotorActive = _bluetoothBypassMotorActive;
  bluetoothBypassCoasting = _bluetoothBypassCoasting;
  bluetoothBypassParachuteOut = _bluetoothBypassParachuteOut;

  String devName = DEVICE_NAME;
  String chipId = String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
  devName += '_';
  devName += chipId;

  BLEDevice::init(devName.c_str());
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks(bluetoothConnected));
  BLEService* pService = pServer->createService(SERVICE_UUID);

  pServo = pService->createCharacteristic(SERVO_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  pServo->setCallbacks(new ServoCallbacks(servos));

  pBMI088 = pService->createCharacteristic(BMI088_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  pBMI088->setCallbacks(new BMI088Callbacks(imu));

  pBMP390 = pService->createCharacteristic(BMP390_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  pBMP390->setCallbacks(new BMP390Callbacks(altimeter));

  pUtilities = pService->createCharacteristic(UTILITIES_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  pUtilities->setCallbacks(new UtilitiesCallbacks(armed, sendLoopTime, sendBluetoothBMI088, sendBluetoothAltimeter, bluetoothBypassOnPad, bluetoothBypassMotorActive, bluetoothBypassCoasting, bluetoothBypassParachuteOut));

  pService->start();

  BLEAdvertising* pAdvertising = pServer->getAdvertising();

  BLEAdvertisementData adv;
  adv.setName(devName.c_str());
  pAdvertising->setAdvertisementData(adv);

  BLEAdvertisementData adv2;
  adv2.setCompleteServices(BLEUUID(SERVICE_UUID));
  pAdvertising->setScanResponseData(adv2);

  pAdvertising->start();
}

void Bluetooth::writeUtilitiesNotifications(String message) {
  pUtilities->setValue("1" + message);
  pUtilities->notify();
}

void Bluetooth::writeUtilitiesEvents(String message) {
  pUtilities->setValue("2" + message);
  pUtilities->notify();
}

void Bluetooth::writeUtilities(String message) {
  pUtilities->setValue(message);
  pUtilities->notify();
}

void Bluetooth::writeIMU(String message) {
  pBMI088->setValue(message);
  pBMI088->notify();
}

void Bluetooth::writeAltimeter(String message) {
  pBMP390->setValue(message);
  pBMP390->notify();
}

