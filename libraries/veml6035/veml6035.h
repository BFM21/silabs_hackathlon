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

struct IntegrationTime{
    static const uint8_t MS_25 = 0x0C;
    static const uint8_t MS_50 = 0x08;
    static const uint8_t MS_100 = 0x00;
    static const uint8_t MS_200 = 0x01;
    static const uint8_t MS_400 = 0x02;
    static const uint8_t MS_800 = 0x03;
};

struct PersistenceSettings{
    static const uint8_t ONE = 0x00;
    static const uint8_t TWO = 0x01;
    static const uint8_t FOUR = 0x02;
    static const uint8_t EIGHT = 0x03;
};

struct PowerSafeModeWaitTime{
    static const uint8_t S_04 = 0x00;
    static const uint8_t S_08 = 0x01;
    static const uint8_t S_16 = 0x02;
    static const uint8_t S_32 = 0x03;
};


class VEML6035 {
public:
    VEML6035();
    
    
    int init();
    
    
    int init(uint16_t config_value);

    
    int init(char sd, char int_en, char channel_en, char int_channel, uint8_t als_pers, uint8_t als_it, char gain, char dg, char sens);
    
    
    int setHighTresholdWindow(uint8_t htw);

    
    int setLowTresholdWindow(uint8_t ltw);

    
    int setPowerSaveMode(char psm_enabled, uint8_t psm_wait_time);
    
    
    int setConfig(uint16_t config_value);

    
    float readAmbientLight();
    
    
    float readWhiteChannel();

    
    float readInterruptStatus();
    
    
    float getLuxResolutionValue();

    uint16_t getConfig();
    

private:

    uint16_t config_value = 0;

    
    static constexpr float ERROR_VALUE = -999.0;
    
    
    static constexpr uint16_t DEFAULT_CONFIG = 0x1030;
};

#endif 