#ifndef SI7021_H
#define SI7021_H

#include <Arduino.h>
#include <Wire.h>

// addresses are according to the sensor datasheet
// https://www.silabs.com/documents/public/data-sheets/Si7021-A20.pdf


// SI7021 I2C Address
#define SI7021_ADDRESS 0x40

// SI7021 Commands
#define SI7021_MEASURE_HUMIDITY_HOLD     0xE5
#define SI7021_MEASURE_HUMIDITY_NO_HOLD  0xF5
#define SI7021_MEASURE_TEMP_HOLD         0xE3
#define SI7021_MEASURE_TEMP_NO_HOLD      0xF3
#define SI7021_READ_TEMP_FROM_RH         0xE0
#define SI7021_RESET                     0xFE

class SI7021 {
public:
    SI7021();
    
    
    int init();
    
    float readHumidity();
    
    
    float readTemperature();
    
    
    float readTemperatureFromHumidity();
    
    
    int reset();

private:
    
    static constexpr float ERROR_VALUE = -999.0;
};

#endif