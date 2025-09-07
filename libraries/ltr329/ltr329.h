#ifndef LTR329_H
#define LTR329_H

#include <Arduino.h>
#include <Wire.h>

// All addresses are according to the sensors' datasheet
// https://www.vishay.com/docs/84889/LTR329.pdf

/* 
 WARNING: 
 THIS SENSOR IS USING THE SAME I2C ADDRESS AS THE SI7021
 ON THE xG24 DEV KIT BOARD AND BECAUSE OF THE ADDRESS COLLISION 
 IT WON'T WORK WHEN CONNECTED
*/

// LTR329 I2C Address
#define LTR329_ADDRESS 0x29 

// LTR329 Register Addresses
#define LTR329_ALS_CONTR          0x80
#define LTR329_ALS_MEAS_RATE      0x85
#define LTR329_PART_ID            0x86
#define LTR329_MANUFAC_ID         0x87
#define LTR329_ALS_DATA_CH1_0     0x88
#define LTR329_ALS_DATA_CH1_1     0x89
#define LTR329_ALS_DATA_CH0_0     0x8A
#define LTR329_ALS_DATA_CH0_1     0x8B
#define LTR329_ALS_STATUS         0x8C

#define DEFAULT_RESET_VALUE 0x00
#define ALS_MEAS_RATE_RESET_VALUE 0x03
#define PART_ID_RESET_VALUE 0xA0
#define MANUFAC_ID_RESET_VALUE 0x05


struct ALSGain{
    static const uint8_t X_1 = 0x00;     // 1 lux to 64k lux (default)
    static const uint8_t X_2 = 0x01;     // 0.5 lux to 32k lux 
    static const uint8_t X_4 = 0x02;     // 0.25 lux to 16k lux 
    static const uint8_t X_8 = 0x03;     // 0.125 lux to 8k lux 
    static const uint8_t X_48 = 0x06;    // 0.02 lux to 1.3k lux 
    static const uint8_t X_96 = 0x07;    // 0.01 lux to 600 lux 
};

struct ALSIntegrationTime{
    static const uint8_t MS_50 = 0x01;       // 50 ms
    static const uint8_t MS_100 = 0x00;      // 100 ms (default)
    static const uint8_t MS_150 = 0x04;      // 150 ms
    static const uint8_t MS_200 = 0x02;      // 200 ms
    static const uint8_t MS_250 = 0x05;      // 250 ms
    static const uint8_t MS_300 = 0x06;      // 300 ms
    static const uint8_t MS_350 = 0x07;      // 350 ms
    static const uint8_t MS_400 = 0x03;      // 400 ms
};

struct ALSMeasurementRate{
    static const uint8_t MS_50 = 0x00;       // 50 ms
    static const uint8_t MS_100 = 0x01;      // 100 ms
    static const uint8_t MS_200 = 0x02;      // 200 ms
    static const uint8_t MS_500 = 0x03;      // 500 ms (default)
    static const uint8_t MS_1000 = 0x04;     // 1000 ms
    static const uint8_t MS_2000 = 0x05;     // 2000 ms - values 0x06 and 0x07 could be used also 
};


class LTR329 {
public:
    LTR329();
    
    int init();

    int init(uint8_t gain_config_value, uint8_t mr_and_it_config_value);

    int setGain(uint8_t gain_config_value);

    int setGain(char als_mode, char sw_reset, uint8_t gain);

    int setMeasurementRate(uint8_t mr_and_it_config_value);

    int setMeasurementRate(uint8_t measurement_rate, uint8_t integration_time);

    uint16_t readASLChannel0();

    uint16_t readASLChannel1();
    
    int isNewDataAndValid();

private:
    static constexpr uint8_t DEFAULT_ALS_GAIN_CONFIG = 0x03;
    static constexpr uint8_t DEFAULT_ALS_MEASURMENT_CONFIG = 0x03;


};

#endif