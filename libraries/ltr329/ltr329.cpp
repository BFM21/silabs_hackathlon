#include "ltr329.h"
#include "pins_arduino.h"

// Calculations and initialization processes are based on the sensor datasheet
// https://cdn-shop.adafruit.com/product-files/5591/LTR-329ALS-01-Lite-On-datasheet-140998467.pdf

LTR329::LTR329() {
}

static void enableSensor(){
    pinMode(PIN_SENSOR_ENABLE, OUTPUT);
    digitalWrite(PIN_SENSOR_ENABLE, HIGH);
    delay(100);
}

int LTR329::init() {
    return init(DEFAULT_ALS_GAIN_CONFIG, DEFAULT_ALS_MEASURMENT_CONFIG);
}

int LTR329::init(uint8_t gain_config_value, uint8_t mr_and_it_config_value) {
    // enableSensor(); // Keep disabled to avoid conflict with built-in VEML6035
    if (setGain(gain_config_value) != 0) {
        return -1;
    }

    if (setMeasurementRate(mr_and_it_config_value) != 0) {
        return -1;
    }
    
    delay(100);
    
    // Read Part ID and Manufacturer ID to verify sensor presence
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_PART_ID);
    if (Wire.endTransmission() == 0 && Wire.requestFrom(LTR329_ADDRESS, 1) == 1) {
        uint8_t part_id = Wire.read();
        Serial.print("Part ID: 0x");
        Serial.println(part_id, HEX);
    }
    
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_MANUFAC_ID);
    if (Wire.endTransmission() == 0 && Wire.requestFrom(LTR329_ADDRESS, 1) == 1) {
        uint8_t manufac_id = Wire.read();
        Serial.print("Manufacturer ID: 0x");
        Serial.println(manufac_id, HEX);
    }
    
    // Read back control register to verify settings
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_CONTR);
    if (Wire.endTransmission() == 0 && Wire.requestFrom(LTR329_ADDRESS, 1) == 1) {
        uint8_t control_reg = Wire.read();
        Serial.print("Control Register: 0x");
        Serial.println(control_reg, HEX);
    }
    
    return 0;
}

int LTR329::setGain(uint8_t configValue) {
    Serial.print("Gain Config value: ");
    Serial.println(configValue);
    
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_CONTR);
    Wire.write(configValue);
    uint8_t result = Wire.endTransmission();
    if (result != 0) {
        Serial.print("I2C write error: ");
        Serial.println(result);
        return -1;
    }
    Serial.println("Gain setting written successfully");
    return 0;
}

int LTR329::setGain(char als_mode, char sw_reset, uint8_t gain) {
    uint8_t configValue = 0;
    
    configValue |= (als_mode & 0x01);
    configValue |= (sw_reset & 0x01) << 1;
    configValue |= (gain & 0x07) << 2;
    
    return setGain(configValue);
}

int LTR329::setMeasurementRate(uint8_t configValue){
    Serial.print("Measurement Rate Config value: ");
    Serial.println(configValue);

    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_MEAS_RATE);
    Wire.write(configValue);
    if (Wire.endTransmission() != 0) {
        return -1;
    }
    return 0;
}

int LTR329::setMeasurementRate(uint8_t als_meas_rate, uint8_t als_it){
    
   uint8_t configValue = 0;
    
    configValue |= (als_meas_rate & 0x07);
    configValue |= (als_it & 0x07) << 3;
    
    return setMeasurementRate(configValue);
}

int LTR329::isNewDataAndValid(){
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_STATUS);
    if (Wire.endTransmission() != 0) {
        return -1;
    }
    
    if (Wire.requestFrom(LTR329_ADDRESS, 1) != 1) {
        return -1;
    }
    
    uint8_t response = Wire.read();
    uint8_t status = (response & 0x04) >> 2;
    uint8_t valid = (response & 0x80) >> 7;
    Serial.print("ALS Status Data: ");
    Serial.print(response);
    Serial.print(" Status: ");
    Serial.print(status);
    Serial.print(" Valid: ");
    Serial.print(valid);
    Serial.println("");

    if(status == 1 && valid == 0){
        return 0;
    }
    
    return 1;
    
}

uint16_t LTR329::readASLChannel1(){
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_DATA_CH1_0);
    if (Wire.endTransmission() != 0) {
        return -1;
    }
    
    if (Wire.requestFrom(LTR329_ADDRESS, 1) != 1) {
        return -1;
    }
    uint8_t lowByte = Wire.read();
    
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_DATA_CH1_1);
    if (Wire.endTransmission() != 0) {
        return -1;
    }
    
    if (Wire.requestFrom(LTR329_ADDRESS, 1) != 1) {
        return -1;
    }
    uint8_t highByte = Wire.read();

    uint16_t value = (highByte << 8) | lowByte;
    
    
    return value;
    
}

uint16_t LTR329::readASLChannel0(){
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_DATA_CH0_0);
    if (Wire.endTransmission() != 0) {
        return -1;
    }
    
    if (Wire.requestFrom(LTR329_ADDRESS, 1) != 1) {
        return -1;
    }
    uint8_t lowByte = Wire.read();
    
    Wire.beginTransmission(LTR329_ADDRESS);
    Wire.write(LTR329_ALS_DATA_CH0_1);
    if (Wire.endTransmission() != 0) {
        return -1;
    }
    
    if (Wire.requestFrom(LTR329_ADDRESS, 1) != 1) {
        return -1;
    }
    uint8_t highByte = Wire.read();

    uint16_t value = (highByte << 8) | lowByte;
    
    
    return value;
    
}