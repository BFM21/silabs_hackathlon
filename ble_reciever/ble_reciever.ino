#include <ArduinoBLE.h>
#include "silabs_imu.h"


// Broadcast data structure for Alva-Bedroom (SHT30 data)
struct BedroomSensorData {
  uint8_t header;
  uint16_t counter;
  int16_t temperature;
  uint16_t humidity;
  uint8_t battery;
  uint8_t checksum;
} __attribute__((packed));

// Broadcast data structure for Alva-Wearable (multiple sensors) - must match main.ino exactly
struct WearableSensorData {
  uint8_t header = 0xFF;        // Data identifier
  uint16_t counter;             // Packet counter
  int16_t si7021_temp;          // SI7021 temperature * 100
  uint16_t si7021_hum;          // SI7021 humidity * 100
  uint16_t veml6035_light;      // Light sensor * 100
  uint8_t imu_state;            // Movement state
  uint16_t imu_intensity;       // Movement intensity * 100
  int16_t movements_per_minute; // Movements per minute
  uint8_t is_likely_asleep;     // Sleep state (0/1)
  uint16_t still_duration_minutes; // Still duration in minutes
  uint8_t checksum;             // Simple checksum
} __attribute__((packed));

// Keep track of known sensor devices
String knownSensorDevices[] = {"Alva-Wearable", "Alva-Bedroom"};
String lastSensorAddress = "";
unsigned long lastDataTime = 0;
unsigned long lastStatusTime = 0;

// Combined sensor data storage
struct CombinedSensorData {
  // SI7021 data (from Alva-Wearable)
  float si7021_temp = -999;
  float si7021_hum = -999;

  // SHT30 data (from Alva-Bedroom)
  float sht30_temp = -999;
  float sht30_hum = -999;

  // VEML6035 data (from Alva-Wearable)
  float veml6035_light = -999;

  // IMU data (from Alva-Wearable)
  MovementData imu_data = {STILL, -999.0, -999, false, 0};

  // Timestamps for data freshness
  unsigned long si7021_timestamp = 0;
  unsigned long sht30_timestamp = 0;
  unsigned long veml6035_timestamp = 0;
  unsigned long imu_timestamp = 0;
};

CombinedSensorData combinedData;
unsigned long lastJsonOutput = 0;
const unsigned long jsonOutputInterval = 1000; // Output combined JSON every 1 second
bool dataUpdated = false; // Flag to track if new data was received

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize BLE
  if (!BLE.begin()) {
    // Serial.println("Starting BLE failed!");
    while (1);
  }

  // Serial.println("=== Simplified BLE Receiver ===");
  // Serial.println("Continuously scanning for broadcast data...");
  // Serial.println("");

  // Start scanning continuously
  BLE.scan();
  lastDataTime = millis();
  lastStatusTime = millis();
}

void loop() {
  // Check for any BLE device
  BLEDevice peripheral = BLE.available();
  
  if (peripheral) {
    // Check if it's a sensor broadcaster
    if (isSensorDevice(peripheral)) {
      if (peripheral.hasManufacturerData()) {
        processBroadcastData(peripheral);
        lastSensorAddress = peripheral.address();
        lastDataTime = millis();
      }
    }
    
  }
  
  // Show periodic status
  showPeriodicStatus();

  // Output combined JSON at regular intervals
  outputCombinedJson();

  delay(50);
}

bool isSensorDevice(BLEDevice& device) {
  // Check by name
  String deviceName = device.localName();

  for (int i = 0; i < 2; i++) {
    if (deviceName == knownSensorDevices[i]) {
      return true;
    }
  }

  // Check by previously known address
  if (lastSensorAddress.length() > 0 && device.address() == lastSensorAddress) {
    return true;
  }

  // Check if it has valid sensor data format (for unnamed devices)
  if (deviceName.length() == 0 && device.hasManufacturerData()) {
    int dataLength = device.manufacturerDataLength();
    if (dataLength == sizeof(BedroomSensorData) || dataLength == sizeof(WearableSensorData)) {
      BedroomSensorData testData;
      device.manufacturerData((uint8_t*)&testData, sizeof(testData));
      return (testData.header == 0xFF);
    }
  }

  return false;
}

void processBroadcastData(BLEDevice& device) {
  int dataLength = device.manufacturerDataLength();
  String deviceName = device.localName();
  unsigned long currentTime = millis();

  // Handle Alva-Bedroom device (SHT30 data)
  if (deviceName == "Alva-Bedroom" && dataLength == sizeof(BedroomSensorData)) {
    BedroomSensorData data;
    device.manufacturerData((uint8_t*)&data, sizeof(data));

    // Validate header
    if (data.header != 0xFF) {
      return;
    }

    // Store SHT30 data
    float newTemp = data.temperature / 100.0;
    float newHum = data.humidity / 100.0;

    // Check if values actually changed
    if (abs(newTemp - combinedData.sht30_temp) > 0.01 || abs(newHum - combinedData.sht30_hum) > 0.01) {
      dataUpdated = true;
    }

    combinedData.sht30_temp = newTemp;
    combinedData.sht30_hum = newHum;
    combinedData.sht30_timestamp = currentTime;
  }
  // Handle Alva-Wearable device (multiple sensors) - accept any reasonable size
  else if (deviceName == "Alva-Wearable" && (dataLength >= sizeof(WearableSensorData) && dataLength <= 20)) {
    WearableSensorData data;
    device.manufacturerData((uint8_t*)&data, sizeof(data));

    // Validate header
    if (data.header != 0xFF) {
      return;
    }

    // Store SI7021 data
    float newSI7021Temp = (data.si7021_temp != -99900) ? data.si7021_temp/100.0 : -999;
    float newSI7021Hum = (data.si7021_hum != 0) ? data.si7021_hum/100.0 : -999;
    float newLight = (data.veml6035_light != 0) ? data.veml6035_light/100.0 : -999;

    // Check if values actually changed
    if (abs(newSI7021Temp - combinedData.si7021_temp) > 0.01 ||
        abs(newSI7021Hum - combinedData.si7021_hum) > 0.01 ||
        abs(newLight - combinedData.veml6035_light) > 0.01) {
      dataUpdated = true;
    }

    combinedData.si7021_temp = newSI7021Temp;
    combinedData.si7021_hum = newSI7021Hum;
    combinedData.si7021_timestamp = currentTime;

    // Store VEML6035 data
    combinedData.veml6035_light = newLight;
    combinedData.veml6035_timestamp = currentTime;

    // Store IMU data
    combinedData.imu_data.current_state = (ActivityState)data.imu_state;
    combinedData.imu_data.movement_intensity = data.imu_intensity / 100.0;
    combinedData.imu_data.movements_per_minute = data.movements_per_minute;
    combinedData.imu_data.is_likely_asleep = (data.is_likely_asleep == 1);
    combinedData.imu_data.still_duration_minutes = data.still_duration_minutes;
    combinedData.imu_timestamp = currentTime;
  }
}


void showPeriodicStatus() {
  unsigned long currentTime = millis();
  
  // Show status every 10 seconds
  if (currentTime - lastStatusTime > 10000) {
    // Serial.print("🔍 Scanning... (");
    // Serial.print((currentTime - lastDataTime) / 1000);
    // Serial.print("s since last data");
    // if (lastSensorAddress.length() > 0) {
    //   Serial.print(", last sensor: ");
    //   Serial.print(lastSensorAddress);
    // }
    // Serial.println(")");
    
    lastStatusTime = currentTime;
  }
}

String toJson(float si7021_t, float si7021_h,
              float sht30_t, float sht30_h,
              float veml6035_l,
              MovementData imu_data)
{
  String json = "{";
  bool first = true;
  
  // Helper function to add comma if not first element
  auto addCommaIfNeeded = [&]() {
    if (!first) json += ",";
    first = false;
  };
  
  // Check and add si7021 temperature
  if (si7021_t != -999) {
    addCommaIfNeeded();
    json += "\"si7021_temp\":" + String(si7021_t, 2);
  }
  
  // Check and add si7021 humidity
  if (si7021_h != -999) {
    addCommaIfNeeded();
    json += "\"si7021_hum\":" + String(si7021_h, 2);
  }
  
  // Check and add sht30 temperature
  if (sht30_t != -999) {
    addCommaIfNeeded();
    json += "\"sht30_temp\":" + String(sht30_t, 2);
  }
  
  // Check and add sht30 humidity
  if (sht30_h != -999) {
    addCommaIfNeeded();
    json += "\"sht30_hum\":" + String(sht30_h, 2);
  }
  
  // Check and add light sensor data
  if (veml6035_l != -999) {
    addCommaIfNeeded();
    json += "\"veml6035\":" + String(veml6035_l, 2);
  }
  
  // IMU data object - always include, but check individual values
  addCommaIfNeeded();
  json += "\"imu_data\":{";
  bool imu_first = true;
  
  auto addImuCommaIfNeeded = [&]() {
    if (!imu_first) json += ",";
    imu_first = false;
  };
  
  // Add current_state only if valid
  if ((int)imu_data.current_state != -999) {
    addImuCommaIfNeeded();
    json += "\"current_state\":" + String((int)imu_data.current_state);
  }
  
  // Add movement_intensity only if valid
  if (imu_data.movement_intensity != -999) {
    addImuCommaIfNeeded();
    json += "\"movement_intensity\":" + String(imu_data.movement_intensity, 2);
  }
  
  // Add movements_per_minute only if valid
  if (imu_data.movements_per_minute != -999) {
    addImuCommaIfNeeded();
    json += "\"movements_per_minute\":" + String(imu_data.movements_per_minute);
  }
  
  // Add is_likely_asleep - boolean, always include
  addImuCommaIfNeeded();
  json += "\"is_likely_asleep\":" + String(imu_data.is_likely_asleep ? "true" : "false");
  
  // Add still_duration_minutes only if valid
  if (imu_data.still_duration_minutes != -999) {
    addImuCommaIfNeeded();
    json += "\"still_duration_minutes\":" + String(imu_data.still_duration_minutes);
  }
  
  json += "}";
  
  json += "}";
  return json;
}

void outputCombinedJson() {
  unsigned long currentTime = millis();

  // Output combined JSON when new data is received OR as backup every 5 seconds if no updates
  if (dataUpdated || (currentTime - lastJsonOutput >= 5000)) {
    lastJsonOutput = currentTime;
    dataUpdated = false; // Reset the flag

    // Create combined JSON using the existing toJson function
    Serial.println(toJson(combinedData.si7021_temp, combinedData.si7021_hum,
                         combinedData.sht30_temp, combinedData.sht30_hum,
                         combinedData.veml6035_light, combinedData.imu_data));
  }
}