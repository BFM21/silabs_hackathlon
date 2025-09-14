#include "sht30.h"

// CRC-8 polynomial: 0x31 (x^8 + x^5 + x^4 + 1)
const uint8_t CRC8_POLYNOMIAL = 0x31;

SHT30::SHT30(uint8_t address) {
  _address = address;
  _lastError = SHT30_OK;
}

int SHT30::init() {
  if (isConnected() != 0) {
    _lastError = SHT30_ERROR_I2C;
    return SHT30_ERROR_I2C;
  }
  
  return softReset();
}

uint8_t SHT30::readTempHumidity(float* temperature, float* humidity, 
                               uint8_t repeatability, int clockStretching) {
  uint8_t buffer[6];
  uint16_t command = getCommand(repeatability, clockStretching);
  
  if (sendCommand(command) != SHT30_OK) {
    return _lastError;
  }
  
  // Wait for measurement to complete based on repeatability
  uint16_t measurementTime;
  switch (repeatability) {
    case SHT30_Repeatability::HIGH:
      measurementTime = clockStretching ? 0 : 15;  // 15ms for high repeatability
      break;
    case SHT30_Repeatability::MEDIUM:
      measurementTime = clockStretching ? 0 : 6;   // 6ms for medium repeatability
      break;
    case SHT30_Repeatability::LOW:
      measurementTime = clockStretching ? 0 : 4;   // 4ms for low repeatability
      break;
    default:
      measurementTime = 15;
      break;
  }
  
  if (measurementTime > 0) {
    delay(measurementTime);
  }
  
  // Read measurement data (6 bytes: temp MSB, temp LSB, temp CRC, hum MSB, hum LSB, hum CRC)
  if (readData(buffer, 6) != SHT30_OK) {
    return _lastError;
  }
  
  if (verifyCRC(&buffer[0], 2, buffer[2]) != 0) {
    _lastError = SHT30_ERROR_CRC;
    return _lastError;
  }
  
  if (verifyCRC(&buffer[3], 2, buffer[5]) != 0) {
    _lastError = SHT30_ERROR_CRC;
    return _lastError;
  }
  
  uint16_t rawTemp = (buffer[0] << 8) | buffer[1];
  uint16_t rawHum = (buffer[3] << 8) | buffer[4];
  
  *temperature = -45.0 + 175.0 * ((float)rawTemp / 65535.0);
  
  *humidity = 100.0 * ((float)rawHum / 65535.0);
  
  _lastError = SHT30_OK;
  return _lastError;
}

uint8_t SHT30::readTemperature(float* temperature, uint8_t repeatability, int clockStretching) {
  float dummy_humidity;
  return readTempHumidity(temperature, &dummy_humidity, repeatability, clockStretching);
}

uint8_t SHT30::readHumidity(float* humidity, uint8_t repeatability, int clockStretching) {
  float dummy_temperature;
  return readTempHumidity(&dummy_temperature, humidity, repeatability, clockStretching);
}

uint8_t SHT30::softReset() {
  uint8_t result = sendCommand(SHT30_CMD_SOFT_RESET);
  if (result == SHT30_OK) {
    delay(2);  // Wait 2ms for reset to complete
  }
  return result;
}

uint8_t SHT30::enableHeater() {
  return sendCommand(SHT30_CMD_HEATER_ENABLE);
}

uint8_t SHT30::disableHeater() {
  return sendCommand(SHT30_CMD_HEATER_DISABLE);
}

uint8_t SHT30::readStatusRegister(uint16_t* status) {
  uint8_t buffer[3];
  
  if (sendCommand(SHT30_CMD_STATUS_REGISTER) != SHT30_OK) {
    return _lastError;
  }
  
  if (readData(buffer, 3) != SHT30_OK) {
    return _lastError;
  }
  
  if (verifyCRC(&buffer[0], 2, buffer[2]) != 0) {
    _lastError = SHT30_ERROR_CRC;
    return _lastError;
  }
  
  *status = (buffer[0] << 8) | buffer[1];
  _lastError = SHT30_OK;
  return _lastError;
}

uint8_t SHT30::clearStatusRegister() {
  return sendCommand(SHT30_CMD_CLEAR_STATUS);
}

int SHT30::isConnected() {
  Wire.beginTransmission(_address);
  return Wire.endTransmission();
}

uint8_t SHT30::getLastError() {
  return _lastError;
}


uint8_t SHT30::calculateCRC(const uint8_t* data, uint8_t length) {
  uint8_t crc = 0xFF;  // Initial value
  
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t bit = 8; bit > 0; --bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }
  
  return crc;
}

int SHT30::verifyCRC(const uint8_t* data, uint8_t length, uint8_t expectedCRC) {
  return (calculateCRC(data, length) == expectedCRC) ? 0 : 1;
}

uint8_t SHT30::sendCommand(uint16_t command) {
  Wire.beginTransmission(_address);
  Wire.write(command >> 8);    // MSB
  Wire.write(command & 0xFF);  // LSB
  
  uint8_t result = Wire.endTransmission();
  if (result != 0) {
    _lastError = SHT30_ERROR_I2C;
    return _lastError;
  }
  
  delay(1);  // Minimum waiting time between commands
  _lastError = SHT30_OK;
  return _lastError;
}

uint8_t SHT30::readData(uint8_t* buffer, uint8_t length) {
  uint8_t bytesRead = Wire.requestFrom(_address, length);
  
  if (bytesRead != length) {
    _lastError = SHT30_ERROR_NO_DATA;
    return _lastError;
  }
  
  for (uint8_t i = 0; i < length; i++) {
    if (Wire.available()) {
      buffer[i] = Wire.read();
    } else {
      _lastError = SHT30_ERROR_NO_DATA;
      return _lastError;
    }
  }
  
  _lastError = SHT30_OK;
  return _lastError;
}

uint16_t SHT30::getCommand(uint8_t repeatability, int clockStretching) {
  if (clockStretching) {
    switch (repeatability) {
      case SHT30_Repeatability::HIGH:
        return SHT30_CMD_MEASURE_HIGH_REP_STRETCH;
      case SHT30_Repeatability::MEDIUM:
        return SHT30_CMD_MEASURE_MEDIUM_REP_STRETCH;
      case SHT30_Repeatability::LOW:
        return SHT30_CMD_MEASURE_LOW_REP_STRETCH;
      default:
        return SHT30_CMD_MEASURE_HIGH_REP_STRETCH;
    }
  } else {
    switch (repeatability) {
      case SHT30_Repeatability::HIGH:
        return SHT30_CMD_MEASURE_HIGH_REP_NOSTRETCH;
      case SHT30_Repeatability::MEDIUM:
        return SHT30_CMD_MEASURE_MEDIUM_REP_NOSTRETCH;
      case SHT30_Repeatability::LOW:
        return SHT30_CMD_MEASURE_LOW_REP_NOSTRETCH;
      default:
        return SHT30_CMD_MEASURE_HIGH_REP_NOSTRETCH;
    }
  }
}