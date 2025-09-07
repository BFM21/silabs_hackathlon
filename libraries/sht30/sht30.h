
#ifndef SHT30_H
#define SHT30_H

#include <Arduino.h>
#include <Wire.h>

// addresses are according to the sensor datasheet
// https://cdn-shop.adafruit.com/product-files/5064/5064_Sensirion_Humidity_Sensors_SHT3x_Datasheet_digital.pdf


// I2C addresses
#define SHT30_ADDRESS_A     0x44  // ADDR pin connected to logic low (default)
#define SHT30_ADDRESS_B     0x45  // ADDR pin connected to logic high

// Measurement commands (single shot mode)
#define SHT30_CMD_MEASURE_HIGH_REP_STRETCH    0x2C06  // High repeatability with clock stretching
#define SHT30_CMD_MEASURE_MEDIUM_REP_STRETCH  0x2C0D  // Medium repeatability with clock stretching
#define SHT30_CMD_MEASURE_LOW_REP_STRETCH     0x2C10  // Low repeatability with clock stretching
#define SHT30_CMD_MEASURE_HIGH_REP_NOSTRETCH  0x2400  // High repeatability without clock stretching
#define SHT30_CMD_MEASURE_MEDIUM_REP_NOSTRETCH 0x240B // Medium repeatability without clock stretching
#define SHT30_CMD_MEASURE_LOW_REP_NOSTRETCH   0x2416  // Low repeatability without clock stretching

// Other commands
#define SHT30_CMD_SOFT_RESET    0x30A2
#define SHT30_CMD_HEATER_ENABLE 0x306D
#define SHT30_CMD_HEATER_DISABLE 0x3066
#define SHT30_CMD_STATUS_REGISTER 0xF32D
#define SHT30_CMD_CLEAR_STATUS  0x3041

// Error codes
#define SHT30_OK                0
#define SHT30_ERROR_TIMEOUT     1
#define SHT30_ERROR_CRC         2
#define SHT30_ERROR_I2C         3
#define SHT30_ERROR_NO_DATA     4


struct SHT30_Repeatability {
  static const uint8_t HIGH = 0;
  static const uint8_t MEDIUM = 1;
  static const uint8_t LOW = 2;
};

class SHT30 {
private:
  uint8_t _address;
  uint8_t _lastError;
  
  
  uint8_t calculateCRC(const uint8_t* data, uint8_t length);
  int verifyCRC(const uint8_t* data, uint8_t length, uint8_t expectedCRC);
  uint8_t sendCommand(uint16_t command);
  uint8_t readData(uint8_t* buffer, uint8_t length);
  uint16_t getCommand(uint8_t repeatability, int clockStretching);

public:
  
  SHT30(uint8_t address = SHT30_ADDRESS_A);
  
  
  int init();
  
  
  uint8_t readTempHumidity(float* temperature, float* humidity, 
                          uint8_t repeatability = SHT30_Repeatability::HIGH,
                          int clockStretching = 1);
  
  
  uint8_t readTemperature(float* temperature, 
                         uint8_t repeatability = SHT30_Repeatability::HIGH,
                         int clockStretching = 1);
  
  uint8_t readHumidity(float* humidity, 
                      uint8_t repeatability = SHT30_Repeatability::HIGH,
                      int clockStretching = 1);
  
  
  uint8_t softReset();
  uint8_t enableHeater();
  uint8_t disableHeater();
  uint8_t readStatusRegister(uint16_t* status);
  uint8_t clearStatusRegister();
  
  
  int isConnected();
  uint8_t getLastError();
};

#endif 