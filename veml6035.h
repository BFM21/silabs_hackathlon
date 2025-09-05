#ifndef VEML6035_H
#define VEML6035_H

#include <Arduino.h>
#include <Wire.h>

// VEML6035 I2C Address
#define VEML6035_ADDRESS 0x29

// VEML6035 Register Addresses
#define VEML6035_CONFIG_ADDRESS 0x00
#define VEML6035_HTW_ADDRESS    0x01
#define VEML6035_LTW_ADDRESS    0x02
#define VEML6035_PSM_ADDRESS    0x03
#define VEML6035_ALS_OUTPUT     0x04
#define VEML6035_WCH_OUTPUT     0x05
#define VEML6035_INT_STATUS     0x06

class VEML6035 {
public:
    VEML6035();
    
    // Initialize the sensor with default configuration
    int init();
    
    // Initialize the sensor with custom configuration
    int init(uint16_t configValue);
    
    // Read ambient light in lux
    float readAmbientLight();
    
    // Read white channel data
    float readWhiteChannel();
    
    // Set power save mode
    int setPowerSaveMode(uint8_t psm);
    
    // Set configuration register
    int setConfig(uint16_t configValue);

private:
    // Error value returned when reading fails
    static constexpr float ERROR_VALUE = -999.0;
    
    // Default configuration value (your working config)
    static constexpr uint16_t DEFAULT_CONFIG = 0x818;
    
    // Conversion factor for lux calculation
    static constexpr float LUX_FACTOR = 0.1024;
};

#endif // VEML6035_H