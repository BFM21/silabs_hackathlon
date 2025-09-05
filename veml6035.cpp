#include "veml6035.h"

VEML6035::VEML6035() {
    // Constructor - nothing needed here since Wire is initialized externally
}

int VEML6035::init() {
    return init(DEFAULT_CONFIG);
}

int VEML6035::init(uint16_t configValue) {
    // Set configuration
    if (setConfig(configValue) != 0) {
        return 1;
    }
    
    // Set power save mode
    if (setPowerSaveMode(0x07) != 0) {
        return 1;
    }
    
    delay(100); // Wait for sensor to initialize
    return 0;
}

int VEML6035::setConfig(uint16_t configValue) {
    uint8_t configBytes[] = {
        (uint8_t)(configValue & 0xFF),        // LSB
        (uint8_t)((configValue >> 8) & 0xFF)  // MSB
    };
    
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_CONFIG_ADDRESS);
    Wire.write(configBytes, 2);
    if (Wire.endTransmission() != 0) {
        return 1;
    }
    
    return 0;
}

int VEML6035::setPowerSaveMode(uint8_t psm) {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_PSM_ADDRESS);
    Wire.write(psm);
    if (Wire.endTransmission() != 0) {
        return 1;
    }
    
    return 0;
}

float VEML6035::readAmbientLight() {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_ALS_OUTPUT);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(125); // Wait for data to be ready
    
    Wire.requestFrom(VEML6035_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawAmbientLight = Wire.read() << 8;
    rawAmbientLight |= Wire.read();
    
    float luxValue = (float)rawAmbientLight * LUX_FACTOR;
    return luxValue;
}

float VEML6035::readWhiteChannel() {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_WCH_OUTPUT);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(125); // Wait for data to be ready
    
    Wire.requestFrom(VEML6035_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawWhiteChannel = Wire.read() << 8;
    rawWhiteChannel |= Wire.read();
    
    float whiteValue = (float)rawWhiteChannel * LUX_FACTOR;
    return whiteValue;
}