#ifndef VEML6035_H
#define VEML6035_H

#include <Arduino.h>
#include <Wire.h>

// All addresses are according to the sensors' datasheet
// https://www.vishay.com/docs/84889/veml6035.pdf

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

enum IntegrationTime{
    MS_25 = 0x0C,
    MS_50 = 0x08,
    MS_100 = 0x00,
    MS_200 = 0x01,
    MS_400 = 0x02,
    MS_800 = 0x03
};

enum PersistenceSettings{
    ONE = 0x00,
    TWO = 0x01,
    FOUR = 0x02,
    EIGHT = 0x03
};

enum PowerSafeModeWaitTime{
    S_04 = 0x00,
    S_08 = 0x01,
    S_16 = 0x02,
    S_32 = 0x03
};


class VEML6035 {
public:
    VEML6035();
    
    // Initialize the sensor with default configuration
    int init();
    
    // Initialize the sensor with custom configuration
    int init(uint16_t config_value);

    // Initialize by setting individual configuration values
    int init(char sd, char int_en, char channel_en, char int_channel, PersistenceSettings als_pers, IntegrationTime als_it, char gain, char dg, char sens);
    
    // Set power save mode
    int setHighTresholdWindow(uint8_t htw);

    // Set power save mode
    int setLowTresholdWindow(uint8_t ltw);

    // Set power save mode
    int setPowerSaveMode(char psm_enabled, PowerSafeModeWaitTime psm_wait_time);
    
    // Set configuration register
    int setConfig(uint16_t config_value);

    // Read ambient light in lux
    float readAmbientLight();
    
    // Read white channel data
    float readWhiteChannel();

    // Read white channel data
    float readInterruptStatus();
    
    // Get lux resolution value based on config
    float getLuxResolutionValue();

    uint16_t getConfig();
    

private:

    uint16_t config_value = 0;

    // Error value returned when reading fails
    static constexpr float ERROR_VALUE = -999.0;
    
    // Default configuration value (your working config)
    static constexpr uint16_t DEFAULT_CONFIG = 0x1030;
};

#endif // VEML6035_H