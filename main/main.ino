#include <Wire.h>

#include "si7021.h"
#include "veml6035.h"
#include "sht30.h"
#include "pins_arduino.h"

SI7021 si7021_ths;
VEML6035 veml6035_als;
SHT30 sht30_ths;

void setup() {

    Serial.begin(115200);

    Wire.begin();
    
    
    if (si7021_ths.init() != 0) {
        Serial.println("SI7021 Temperature & Humidity Sensor setup failed");
    } else {
        Serial.println("SI7021 Temperature & Humidity Sensor Ready");
    }

    
    if (sht30_ths.init() != 0) {
        Serial.println("SHT30 Temperature & Humidity Sensor setup failed");
    } else {
        Serial.println("SHT30 Temperature & Humidity Sensor Ready");
    }
    
    
    if (veml6035_als.init() != 0) {

        Serial.println("Ambient Light Sensor setup failed");

    } else {
        Serial.println("Ambient Light Sensor Ready");
    }
}

void loop() {
    
    float si7021_h = si7021_ths.readHumidity();
    float si7021_t = si7021_ths.readTemperature();
    
    
    if (si7021_h != -999 && si7021_t != -999) {
        Serial.print("SI7021 - ");
        Serial.print("Humidity: ");
        Serial.print(si7021_h, 2);
        Serial.print(" %RH, Temperature: ");
        Serial.print(si7021_t, 2);
        Serial.println(" °C");
    } else {
        Serial.println("Error reading temperature/humidity sensor data");
    }

    float sht30_h, sht30_t;
    uint8_t result = sht30_ths.readTempHumidity(&sht30_t, &sht30_h, SHT30_Repeatability::HIGH);

    if (result == SHT30_OK) {
        Serial.print("SHT30 - ");
        Serial.print("Humidity: ");
        Serial.print(sht30_h, 2);
        Serial.print(" %RH, Temperature: ");
        Serial.print(sht30_t, 2);
        Serial.println(" °C");
    } else {
        Serial.print("Error: ");
        Serial.println(result);
    }
  

    float light = veml6035_als.readAmbientLight();

    if (light != -999) {
        Serial.print("Light: ");
        Serial.print(light, 2);
        Serial.println(" lux");
    } else {
        Serial.println("Error reading light sensor data");
    }
    
    delay(1000);
}