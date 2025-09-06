#include <Wire.h>
#include "si7021.h"
#include "veml6035.h"

// PIN DEFINITIONS
#define PIN_SENSOR_ENABLE 17  
#define PIN_WIRE_SDA      6   
#define PIN_WIRE_SCL      11  

// Create sensor objects
SI7021 tempHumiditySensor;
VEML6035 lightSensor;

void setup() {
    Serial.begin(115200);
    
    // Enable sensors
    pinMode(PIN_SENSOR_ENABLE, OUTPUT);
    digitalWrite(PIN_SENSOR_ENABLE, HIGH);
    delay(100);
    
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
        lightSensor.setPowerSaveMode(1, S_04);
    }
}

void loop() {
    // Read sensors
    float humidity = tempHumiditySensor.readHumidity();
    float temperature = tempHumiditySensor.readTemperature();
    float light = lightSensor.readAmbientLight();
    
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
    
    // Display light
    if (light != -999) {
        Serial.print("Light: ");
        Serial.print(light, 2);
        Serial.println(" lux");
    } else {
        Serial.println("Error reading light sensor data");
    }
    
    delay(1000);
}