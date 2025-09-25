#include <ArduinoBLE.h>
#include <Wire.h>
#include "sht30.h"

// SHT30 sensor instance
SHT30 sht30_ths;
bool sht30SetupFailed = false;

// Data variables
float temperature = 20.0;
float humidity = 50.0;
int counter = 0;
unsigned long previousMillis = 0;
const long interval = 2000; // Update every 2 seconds - more stable

const char DEVICE_NAME[] = "Alva-Bedroom";

// Advertising data structure
struct SensorData {
  uint8_t header = 0xFF;        // Custom data identifier
  uint16_t counter;             // 2 bytes
  int16_t temperature;          // 2 bytes (multiplied by 100 for precision)
  uint16_t humidity;            // 2 bytes (multiplied by 100 for precision)
  uint8_t battery = 85;         // 1 byte (battery percentage)
  uint8_t checksum;             // 1 byte
} __attribute__((packed));

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize I2C
  Wire.begin();

  // Initialize SHT30 sensor
  if (sht30_ths.init() != 0) {
    sht30SetupFailed = true;
    Serial.println("SHT30 Temperature & Humidity Sensor setup failed");
  } else {
    Serial.println("SHT30 Temperature & Humidity Sensor Ready");
  }

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  // Set device name (shows up in scans)
  BLE.setLocalName(DEVICE_NAME);
  
  Serial.println("Arduino BLE One-to-Many Broadcaster");
  Serial.println("Broadcasting sensor data...");
  Serial.println("Multiple devices can receive simultaneously!");
  
  // Start with initial broadcast
  broadcastData();
}

void loop() {
  unsigned long currentMillis = millis();

  // Update and broadcast data every interval
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Update sensor data
    updateSensorData();
    
    // Broadcast to all listening devices
    broadcastData();
    
    // Print to serial for monitoring
    printData();
  }
}

void updateSensorData() {
  // Read SHT30 sensor if available
  if (!sht30SetupFailed) {
    float sht30_t, sht30_h;
    uint8_t result = sht30_ths.readTempHumidity(&sht30_t, &sht30_h, SHT30_Repeatability::HIGH);

    if (result == SHT30_OK) {
      temperature = sht30_t;
      humidity = sht30_h;
    } else {
      Serial.print("SHT30 Error: ");
      Serial.println(result);
      // Use simulated data as fallback
      temperature = 22.0 + sin(millis() / 20000.0) * 5.0 + random(-50, 50) / 100.0;
      humidity = 60.0 + cos(millis() / 15000.0) * 15.0 + random(-100, 100) / 100.0;
    }
  } else {
    // Use simulated data if sensor failed to initialize
    temperature = 22.0 + sin(millis() / 20000.0) * 5.0 + random(-50, 50) / 100.0;
    humidity = 60.0 + cos(millis() / 15000.0) * 15.0 + random(-100, 100) / 100.0;
  }

  counter++;

  // Keep values in realistic ranges
  temperature = constrain(temperature, -40.0, 85.0);
  humidity = constrain(humidity, 0.0, 100.0);
}

void broadcastData() {
  // Prepare data structure
  SensorData data;
  data.header = 0xFF;  // Explicitly set header
  data.counter = counter;
  data.temperature = (int16_t)(temperature * 100);  // Store with 2 decimal precision
  data.humidity = (uint16_t)(humidity * 100);       // Store with 2 decimal precision

  // Calculate simple checksum
  data.checksum = (data.header + data.counter + data.temperature + data.humidity + data.battery) & 0xFF;

  // Stop advertising to update data
  // Always stop before updating manufacturer data
  BLE.stopAdvertise();
  delay(10); // Small delay to ensure stop completes

  // Set manufacturer data (this is where our sensor data goes)
  BLE.setManufacturerData((uint8_t*)&data, sizeof(data));

  // Set advertising parameters for better broadcasting
  BLE.setAdvertisingInterval(200); // 200ms intervals - less aggressive than 100ms

  // Start advertising again with new data
  if (!BLE.advertise()) {
    Serial.println("ERROR: Failed to start advertising!");
    // Try to recover
    BLE.end();
    delay(100);
    if (BLE.begin()) {
      BLE.setLocalName(DEVICE_NAME);
      BLE.setManufacturerData((uint8_t*)&data, sizeof(data));
      BLE.setAdvertisingInterval(200);
      BLE.advertise();
    }
  } else {
    Serial.println("DEBUG: Successfully advertising data");
  }
}

void printData() {
  Serial.println("=== BROADCASTING ===");
  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(humidity, 2);
  Serial.println(" %");
  
  Serial.print("Counter: ");
  Serial.println(counter);
  
  Serial.print("Data size: ");
  Serial.print(sizeof(SensorData));
  Serial.println(" bytes");
  
  Serial.println(">>> All nearby devices can receive this data! <<<");
  Serial.println("====================");
}