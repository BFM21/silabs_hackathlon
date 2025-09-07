#include "veml6035.h"
#include "pins_arduino.h"
// Calculations and initialization processes are based on the sensor datasheet
// https://www.vishay.com/docs/84889/veml6035.pdf

VEML6035::VEML6035() {
}

static void enableSensor(){
    pinMode(PIN_SENSOR_ENABLE, OUTPUT);
    digitalWrite(PIN_SENSOR_ENABLE, HIGH);
    delay(100);
}

int VEML6035::init() {
    return init(DEFAULT_CONFIG);
}

int VEML6035::init(uint16_t configValue) {
    enableSensor();
    if (setConfig(configValue) != 0) {
        return 1;
    }
     
    delay(100); 
    return 0;
}

int VEML6035::init(char sd, char int_en, char channel_en, char int_channel, uint8_t als_pers, uint8_t als_it, char gain, char dg, char sens) {
    uint16_t configValue = 0;
    
    // Bit assignments based on VEML6035 datasheet
    configValue |= (sd & 0x01);                    // Bit 0: Shutdown
    configValue |= ((int_en & 0x01) << 1);         // Bit 1: Interrupt enable
    configValue |= ((channel_en & 0x01) << 2);     // Bit 2: Channel enable
    configValue |= ((int_channel & 0x01) << 3);    // Bit 3: Interrupt channel
    configValue |= ((als_pers & 0x03) << 4);       // Bits 5:4: ALS persistence
    configValue |= ((als_it & 0x0F) << 6);         // Bits 9:6: ALS integration time
    configValue |= ((gain & 0x01) << 10);          // Bit 10: Gain
    configValue |= ((dg & 0x01) << 11);            // Bit 11: Digital gain
    configValue |= ((sens & 0x01) << 12);          // Bit 12: Sensitivity          
   
    return init(configValue);
}

int VEML6035::setConfig(uint16_t configValue) {
    config_value = configValue;  
    
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

uint16_t VEML6035::getConfig(){
    return config_value;
}

int VEML6035::setHighTresholdWindow(uint8_t htw) {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_HTW_ADDRESS);
    Wire.write(htw);
    if (Wire.endTransmission() != 0) {
        return 1;
    }
    delay(100); 

    return 0;
}

int VEML6035::setLowTresholdWindow(uint8_t ltw) {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_LTW_ADDRESS);
    Wire.write(ltw);
    if (Wire.endTransmission() != 0) {
        return 1;
    }
    delay(100); 

    
    return 0;
}

int VEML6035::setPowerSaveMode(char psm_enabled, uint8_t psm_wait_time) {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_PSM_ADDRESS);
    uint8_t psm = (psm_enabled & 0x01);
    psm |= (psm_wait_time & 0x03) << 1; 
    Wire.write(psm);
    if (Wire.endTransmission() != 0) {
        return 1;
    }
    
    delay(100); 

    
    return 0;
}

float VEML6035::readAmbientLight() {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_ALS_OUTPUT);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(125); 
    
    Wire.requestFrom(VEML6035_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawAmbientLight = Wire.read() << 8;
    rawAmbientLight |= Wire.read();
    float luxResolution = getLuxResolutionValue();
    float luxValue = (float)rawAmbientLight * luxResolution;
    return luxValue;
}

float VEML6035::readWhiteChannel() {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_WCH_OUTPUT);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(125); 
    
    Wire.requestFrom(VEML6035_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawWhiteChannel = Wire.read() << 8;
    rawWhiteChannel |= Wire.read();
    float luxResolution = getLuxResolutionValue();

    float whiteValue = (float)rawWhiteChannel * luxResolution;
    return whiteValue;
}

float VEML6035::readInterruptStatus() {
    Wire.beginTransmission(VEML6035_ADDRESS);
    Wire.write(VEML6035_INT_STATUS);
    if (Wire.endTransmission() != 0) {
        return ERROR_VALUE;
    }
    
    delay(10); 
    
    Wire.requestFrom(VEML6035_ADDRESS, 2);
    if (Wire.available() != 2) {
        return ERROR_VALUE;
    }
    
    uint16_t rawIntStatus = Wire.read() << 8;
    rawIntStatus |= Wire.read();
    
    return (float)rawIntStatus;
}

float VEML6035::getLuxResolutionValue() {
    uint8_t it = (config_value >> 6) & 0x0F;
    
    uint8_t gain = (config_value >> 10) & 0x01;
    
    uint8_t dg = (config_value >> 11) & 0x01;
    
    uint8_t sens = (config_value >> 12) & 0x01;
    
    // Base resolution values for DG=0, GAIN=0, SENS=0 
    float baseResolution;
    switch(it) {
        case IntegrationTime::MS_25:  baseResolution = 0.0512; break;  // 0.0512 lux/count 
        case IntegrationTime::MS_50:  baseResolution = 0.0256; break;  // 0.0256 lux/count 
        case IntegrationTime::MS_100: baseResolution = 0.0128; break;  // 0.0128 lux/count 
        case IntegrationTime::MS_200: baseResolution = 0.0064;  break;  // 0.0064 lux/count  
        case IntegrationTime::MS_400: baseResolution = 0.0032;  break;  // 0.0032 lux/count 
        case IntegrationTime::MS_800: baseResolution = 0.0016;  break;  // 0.0016 lux/count
        default:     baseResolution = 0.0128; break;  // Default to 100ms
    }
    
    // Apply GAIN adjustment 
    if (gain == 1) {        // GAIN=1 means double sensitivity (divide by 2)
        baseResolution /= 2;
    }
    
    // Apply SENS adjustment 
    if (sens == 1) {        // SENS=1 means low sensitivity (1/8x, multiply by 8)
        baseResolution *= 8;
    }
    
    // Apply DG adjustment 
    if (dg == 1) {          // DG=1 means double gain (divide by 2)
        baseResolution /= 2;
    }
    
    return baseResolution;
}