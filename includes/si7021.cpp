#include "si7021.h"

SI7021::SI7021() {
    // Constructor - nothing needed here since Wire is initialized externally
}

int SI7021::init() {
    return reset();
}

int SI7021::reset() {
    Wire.beginTransmission(SI7021_ADDRESS);
    Wire.write(SI7021_RESET);
    if (Wire.endTransmission() != 0) {
        return 1;
    }
    delay(100); // Wait for reset to complete
    return 0;
}

float SI7021::readHumidity() {
    Wire.beginTransmission(SI7021_ADDRESS);
    Wire.write(SI7021_MEASURE_HUMIDITY_HOLD);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(30); // Wait for measurement
    
    Wire.requestFrom(SI7021_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawHumidity = Wire.read() << 8;
    rawHumidity |= Wire.read();
    
    float humidity = ((float)rawHumidity * 125.0 / 65536.0) - 6.0;
    
    // Clamp to valid range
    if (humidity < 0) humidity = 0;
    if (humidity > 100) humidity = 100;
    
    return humidity;
}

float SI7021::readTemperature() {
    Wire.beginTransmission(SI7021_ADDRESS);
    Wire.write(SI7021_MEASURE_TEMP_HOLD);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(30); // Wait for measurement
    
    Wire.requestFrom(SI7021_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawTemperature = Wire.read() << 8;
    rawTemperature |= Wire.read();
    
    float temperature = ((float)rawTemperature * 175.72 / 65536.0) - 46.85;
    
    return temperature;
}

float SI7021::readTemperatureFromHumidity() {
    Wire.beginTransmission(SI7021_ADDRESS);
    Wire.write(SI7021_READ_TEMP_FROM_RH);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    Wire.requestFrom(SI7021_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawTemperature = Wire.read() << 8;
    rawTemperature |= Wire.read();
    
    float temperature = ((float)rawTemperature * 175.72 / 65536.0) - 46.85;
    
    return temperature;
}