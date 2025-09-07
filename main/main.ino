#include <Wire.h>
#include "si7021.h"
#include "veml6035.h"
#include "ltr329.h"
#include "pins_arduino.h"

// Create sensor objects
SI7021 tempHumiditySensor;
VEML6035 lightSensor;

void setup() {
    Serial.begin(115200);
    
    // Initialize I2C
    Wire.begin();
    
    // Initialize temperature & humidity sensor
    if (tempHumiditySensor.init() != 0) {
        Serial.println("Temperature & Humidity Sensor setup failed");
    } else {
        Serial.println("Temperature & Humidity Sensor Ready");
    }
    
    // Initialize light sensor
    if (lightSensor.init() != 0) {

        Serial.println("Ambient Light Sensor setup failed");

    } else {
        Serial.println("Ambient Light Sensor Ready");
    }
}

void loop() {
    // Read sensors
    float humidity = tempHumiditySensor.readHumidity();
    float temperature = tempHumiditySensor.readTemperature();
    
    // Display temperature and humidity
    if (humidity != -999 && temperature != -999) {
        Serial.print("Humidity: ");
        Serial.print(humidity, 2);
        Serial.print(" %RH, Temperature: ");
        Serial.print(temperature, 2);
        Serial.println(" °C");
    } else {
        Serial.println("Error reading temperature/humidity sensor data");
    }

    // IF USING VEML60635

    float light = lightSensor.readAmbientLight();

    // // Display light
    if (light != -999) {
        Serial.print("Light: ");
        Serial.print(light, 2);
        Serial.println(" lux");
    } else {
        Serial.println("Error reading light sensor data");
    }
    
    // IF USING LTR329

    // int isDataAvailable = lightSensor.isNewDataAndValid();
    // uint16_t light = 0;
    // if(isDataAvailable == 0){
    //     light = lightSensor.readASLChannel1();
    //     if(light != -1){
    //         Serial.print("ALS value from CH1: ");
    //         Serial.println(light);
    //     }else{
    //         Serial.println("Error reading ALS data from CH1");
    //     }
    //     light = lightSensor.readASLChannel0();
    //     if(light != -1){
    //         Serial.print("ALS value from CH0: ");
    //         Serial.println(light);
    //     }else{
    //         Serial.println("Error reading ALS data from CH0");
    //     }
    // }else{
    //     Serial.println("ALS data is old or invalid");
    // }

    delay(1000);
}