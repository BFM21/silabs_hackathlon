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

// Connection monitoring
unsigned long lastWearableTime = 0;
unsigned long lastBedroomTime = 0;
unsigned long totalDevicesScanned = 0;
unsigned long totalSensorPacketsReceived = 0;

// Counter tracking for detecting restarts
uint16_t lastWearableCounter = 0;
uint16_t lastBedroomCounter = 0;
bool wearableCounterInitialized = false;
bool bedroomCounterInitialized = false;

// Track recent devices for debugging
struct RecentDevice {
  String name;
  String address;
  int rssi;
  bool hasManufData;
  int dataLength;
  unsigned long timestamp;
};

RecentDevice recentDevices[10];  // Track last 10 devices
int recentDeviceIndex = 0;

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

  // Initialize LED
  pinMode(LED_BUILTIN_1, OUTPUT);
  digitalWrite(LED_BUILTIN_1, LOW);

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
    totalDevicesScanned++;

    // Show ALL devices to understand what we're actually processing in real-time
    // Serial.print("REALTIME: Device #");
    // Serial.print(totalDevicesScanned);
    // Serial.print(" - '");
    // Serial.print(peripheral.localName());
    // Serial.print("' [");
    // Serial.print(peripheral.address());
    // Serial.println("]");

    // Track this device for debugging (do this first)
    recentDevices[recentDeviceIndex].name = peripheral.localName();
    recentDevices[recentDeviceIndex].address = peripheral.address();
    recentDevices[recentDeviceIndex].rssi = peripheral.rssi();
    recentDevices[recentDeviceIndex].hasManufData = peripheral.hasManufacturerData();
    recentDevices[recentDeviceIndex].dataLength = peripheral.hasManufacturerData() ? peripheral.manufacturerDataLength() : 0;
    recentDevices[recentDeviceIndex].timestamp = millis();
    // Index will be updated at end of loop

    // Priority processing for sensor devices
    String deviceName = peripheral.localName();
    String deviceAddress = peripheral.address();

    // Immediate detection and processing for any device with our target addresses or names
    if (deviceAddress == "34:25:b4:0d:45:f7" || deviceAddress == "8c:6f:b9:a7:e3:4a" ||
        deviceName == "Alva-Wearable" || deviceName == "Alva-Bedroom") {

      // Serial.print("DEBUG: FOUND SENSOR! Address: ");
      // Serial.print(deviceAddress);
      // Serial.print(", Name: '");
      // Serial.print(deviceName);
      // Serial.print("', RSSI: ");
      // Serial.print(peripheral.rssi());
      // Serial.println();
      // Serial.print("DEBUG: SENSOR device #");
      // Serial.print(totalSensorPacketsReceived + 1);
      // Serial.print(" - Name: '");
      // Serial.print(deviceName);
      // Serial.print("', Address: ");
      // Serial.print(deviceAddress);
      // Serial.print(", RSSI: ");
      // Serial.print(peripheral.rssi());
      // Serial.print(", Data: ");
      // Serial.print(peripheral.manufacturerDataLength());
      // Serial.print("b");
      // Serial.println();

      // Fast track sensor device processing
      if (peripheral.hasManufacturerData()) {
        processBroadcastData(peripheral);
        lastSensorAddress = peripheral.address();
        lastDataTime = millis();
        totalSensorPacketsReceived++;

        // Track individual device timing by address
        if (deviceName == "Alva-Wearable" || deviceAddress == "34:25:b4:0d:45:f7") {
          lastWearableTime = millis();
        } else if (deviceName == "Alva-Bedroom" || deviceAddress == "8c:6f:b9:a7:e3:4a") {
          lastBedroomTime = millis();
        }

        // Serial.print("DEBUG: Data processed successfully (packet #");
        // Serial.print(totalSensorPacketsReceived);
        // Serial.println(")");
      }
    }

    // Update device tracking index
    recentDeviceIndex = (recentDeviceIndex + 1) % 10;
  }

  // Show periodic status
  showPeriodicStatus();

  // Check if we need to restart BLE scanning (recovery mechanism)
  checkBLEHealth();

  // Output combined JSON at regular intervals
  outputCombinedJson();

  // Handle serial commands for LED control
  handleSerialCommands();

  delay(10);  // Reduced delay for more responsive scanning
}

bool isSensorDevice(BLEDevice& device) {
  // Check by name
  String deviceName = device.localName();
  String deviceAddress = device.address();

  Serial.print("DEBUG: Checking device - Name: '");
  Serial.print(deviceName);
  Serial.print("', Address: ");
  Serial.print(deviceAddress);
  Serial.print(", Data length: ");
  if (device.hasManufacturerData()) {
    Serial.print(device.manufacturerDataLength());
  } else {
    Serial.print("0");
  }
  Serial.println();

  // Check by name first
  for (int i = 0; i < 2; i++) {
    if (deviceName == knownSensorDevices[i]) {
      Serial.println("DEBUG: Device matched by name!");
      return true;
    }
  }

  // Check by previously known address
  if (lastSensorAddress.length() > 0 && deviceAddress == lastSensorAddress) {
    Serial.println("DEBUG: Device matched by previous address!");
    return true;
  }

  // Check if it has valid sensor data format (for unnamed devices)
  if (device.hasManufacturerData()) {
    int dataLength = device.manufacturerDataLength();
    Serial.print("DEBUG: Checking manufacturer data - Length: ");
    Serial.print(dataLength);
    Serial.print(", Expected bedroom: ");
    Serial.print(sizeof(BedroomSensorData));
    Serial.print(", Expected wearable: ");
    Serial.println(sizeof(WearableSensorData));

    if (dataLength == sizeof(BedroomSensorData) || dataLength == sizeof(WearableSensorData)) {
      // Check first byte for header
      uint8_t firstByte;
      device.manufacturerData(&firstByte, 1);
      Serial.print("DEBUG: First byte (header): 0x");
      Serial.println(firstByte, HEX);

      if (firstByte == 0xFF) {
        Serial.println("DEBUG: Device matched by manufacturer data format!");
        return true;
      }
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

    // Check for counter reset (device restart) or first packet after gap
    bool counterReset = false;
    if (bedroomCounterInitialized) {
      // If it's been more than 5 seconds since last update, force update
      if (currentTime - combinedData.sht30_timestamp > 5000) {
        counterReset = true;
      }
      // Also check for actual counter reset
      int counterDiff = (int)data.counter - (int)lastBedroomCounter;
      if (counterDiff < -10 || counterDiff > 32000) {
        counterReset = true;
      }
    } else {
      bedroomCounterInitialized = true;
      counterReset = true; // Force update on first packet
    }

    lastBedroomCounter = data.counter;

    // Store SHT30 data
    float newTemp = data.temperature / 100.0;
    float newHum = data.humidity / 100.0;

    // Check if values actually changed
    bool tempChanged = abs(newTemp - combinedData.sht30_temp) > 0.01;
    bool humChanged = abs(newHum - combinedData.sht30_hum) > 0.01;

    if (tempChanged || humChanged || counterReset) {
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

    // Check for counter reset (device restart) or first packet after gap
    bool counterReset = false;
    if (wearableCounterInitialized) {
      // If it's been more than 5 seconds since last update, force update
      if (currentTime - combinedData.si7021_timestamp > 5000) {
        counterReset = true;
      }
      // Also check for actual counter reset
      int counterDiff = (int)data.counter - (int)lastWearableCounter;
      if (counterDiff < -10 || counterDiff > 32000) {
        counterReset = true;
      }
    } else {
      wearableCounterInitialized = true;
      counterReset = true; // Force update on first packet
    }

    lastWearableCounter = data.counter;

    // Store SI7021 data
    float newSI7021Temp = (data.si7021_temp != -99900) ? data.si7021_temp/100.0 : -999;
    float newSI7021Hum = (data.si7021_hum != 0) ? data.si7021_hum/100.0 : -999;
    float newLight = (data.veml6035_light != 0) ? data.veml6035_light/100.0 : -999;

    // Serial.print("DEBUG: Raw data received - si7021_temp: ");
    // Serial.print(data.si7021_temp);
    // Serial.print(", si7021_hum: ");
    // Serial.print(data.si7021_hum);
    // Serial.print(", veml6035_light: ");
    // Serial.print(data.veml6035_light);
    // Serial.print(", counter: ");
    // Serial.println(data.counter);

    // Check if values actually changed
    bool tempChanged = abs(newSI7021Temp - combinedData.si7021_temp) > 0.01;
    bool humChanged = abs(newSI7021Hum - combinedData.si7021_hum) > 0.01;
    bool lightChanged = abs(newLight - combinedData.veml6035_light) > 0.01;

    // Serial.print("DEBUG: Data comparison - Temp: ");
    // Serial.print(newSI7021Temp, 2);
    // Serial.print(" vs ");
    // Serial.print(combinedData.si7021_temp, 2);
    // Serial.print(tempChanged ? " (CHANGED)" : " (same)");
    // Serial.print(", Hum: ");
    // Serial.print(newSI7021Hum, 2);
    // Serial.print(" vs ");
    // Serial.print(combinedData.si7021_hum, 2);
    // Serial.print(humChanged ? " (CHANGED)" : " (same)");
    // Serial.print(", Light: ");
    // Serial.print(newLight, 2);
    // Serial.print(" vs ");
    // Serial.print(combinedData.veml6035_light, 2);
    // Serial.println(lightChanged ? " (CHANGED)" : " (same)");

    if (tempChanged || humChanged || lightChanged || counterReset) {
      dataUpdated = true;
      // Serial.println("DEBUG: dataUpdated set to TRUE");
    }

    combinedData.si7021_temp = newSI7021Temp;
    combinedData.si7021_hum = newSI7021Hum;
    combinedData.si7021_timestamp = currentTime;

    // Store VEML6035 data
    combinedData.veml6035_light = newLight;
    combinedData.veml6035_timestamp = currentTime;

    // Store IMU data and check for changes
    bool imuChanged = (combinedData.imu_data.current_state != (ActivityState)data.imu_state) ||
                     (abs(combinedData.imu_data.movement_intensity - data.imu_intensity/100.0) > 0.01) ||
                     (combinedData.imu_data.movements_per_minute != data.movements_per_minute) ||
                     (combinedData.imu_data.is_likely_asleep != (data.is_likely_asleep == 1)) ||
                     (combinedData.imu_data.still_duration_minutes != data.still_duration_minutes);

    // Serial.print("DEBUG: IMU data - State: ");
    // Serial.print(data.imu_state);
    // Serial.print(", Intensity: ");
    // Serial.print(data.imu_intensity);
    // Serial.print(", MovPerMin: ");
    // Serial.print(data.movements_per_minute);
    // Serial.print(", Sleep: ");
    // Serial.print(data.is_likely_asleep);
    // Serial.print(", StillDur: ");
    // Serial.print(data.still_duration_minutes);
    // Serial.println(imuChanged ? " (IMU CHANGED)" : " (imu same)");

    if (imuChanged) {
      dataUpdated = true;
      // Serial.println("DEBUG: dataUpdated set to TRUE due to IMU change");
    }

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
  // if (currentTime - lastStatusTime > 10000) {
  //   Serial.println("===== CONNECTION STATUS =====");
  //   Serial.print("Total BLE devices scanned: ");
  //   Serial.println(totalDevicesScanned);
  //   Serial.print("Total sensor packets received: ");
  //   Serial.println(totalSensorPacketsReceived);

  //   Serial.print("Overall: ");
  //   Serial.print((currentTime - lastDataTime) / 1000);
  //   Serial.println("s since last sensor data");

  //   // Individual device status
  //   if (lastWearableTime > 0) {
  //     Serial.print("  Alva-Wearable: ");
  //     Serial.print((currentTime - lastWearableTime) / 1000);
  //     Serial.print("s ago");
  //     if (currentTime - lastWearableTime > 5000) Serial.print(" [STALE]");
  //     Serial.println();
  //   } else {
  //     Serial.println("  Alva-Wearable: NEVER SEEN");
  //   }

  //   if (lastBedroomTime > 0) {
  //     Serial.print("  Alva-Bedroom: ");
  //     Serial.print((currentTime - lastBedroomTime) / 1000);
  //     Serial.print("s ago");
  //     if (currentTime - lastBedroomTime > 5000) Serial.print(" [STALE]");
  //     Serial.println();
  //   } else {
  //     Serial.println("  Alva-Bedroom: NEVER SEEN");
  //   }

  //   // Show data age
  //   Serial.println("Data freshness:");
  //   if (combinedData.si7021_timestamp > 0) {
  //     Serial.print("  SI7021: ");
  //     Serial.print((currentTime - combinedData.si7021_timestamp) / 1000);
  //     Serial.println("s");
  //   }
  //   if (combinedData.sht30_timestamp > 0) {
  //     Serial.print("  SHT30: ");
  //     Serial.print((currentTime - combinedData.sht30_timestamp) / 1000);
  //     Serial.println("s");
  //   }
  //   if (combinedData.imu_timestamp > 0) {
  //     Serial.print("  IMU: ");
  //     Serial.print((currentTime - combinedData.imu_timestamp) / 1000);
  //     Serial.println("s");
  //   }
  //   // Show recent devices to help debug what we're actually seeing
  //   Serial.println("Recent BLE devices detected:");
  //   for (int i = 0; i < 10; i++) {
  //     int idx = (recentDeviceIndex + i) % 10;
  //     if (recentDevices[idx].timestamp > 0 && currentTime - recentDevices[idx].timestamp < 60000) {
  //       Serial.print("  ");
  //       Serial.print(recentDevices[idx].name.length() > 0 ? recentDevices[idx].name : "(unnamed)");
  //       Serial.print(" [");
  //       Serial.print(recentDevices[idx].address);
  //       Serial.print("] RSSI:");
  //       Serial.print(recentDevices[idx].rssi);
  //       Serial.print(" Data:");
  //       Serial.print(recentDevices[idx].hasManufData ? recentDevices[idx].dataLength : 0);
  //       Serial.print("b ");
  //       Serial.print((currentTime - recentDevices[idx].timestamp) / 1000);
  //       Serial.println("s ago");
  //     }
  //   }
  //   Serial.println("=============================");

  //   lastStatusTime = currentTime;
  // }
}

void checkBLEHealth() {
  unsigned long currentTime = millis();
  static unsigned long lastRestartTime = 0;
  static unsigned long lastHardRestartTime = 0;

  // Reset counter tracking if we haven't seen data for a while (transmission gap)
  if (currentTime - lastDataTime > 10000) { // 10 seconds without data
    wearableCounterInitialized = false;
    bedroomCounterInitialized = false;
  }

  // If no sensor data received for 15 seconds OR no real-time devices for 10 seconds, restart BLE scanning
  static unsigned long lastRealTimeDevice = 0;
  if (totalDevicesScanned > 0) {
    lastRealTimeDevice = currentTime;
  }

  bool needsRestart = ((currentTime - lastDataTime > 15000 && lastDataTime > 0) ||
                      (currentTime - lastRealTimeDevice > 10000 && lastRealTimeDevice > 0)) &&
                      (currentTime - lastRestartTime > 5000);  // 5 second cooldown

  if (needsRestart) {
    // Serial.println("WARNING: BLE scanning stuck - performing full restart...");

    // Full BLE stack restart
    BLE.stopScan();
    delay(200);
    BLE.end();
    delay(300);

    if (BLE.begin()) {
      delay(100);
      BLE.scan();
      // Serial.println("BLE stack fully restarted");
    } else {
      // Serial.println("ERROR: BLE restart failed!");
    }

    lastRestartTime = currentTime;
    // Reset counters to detect new activity
    totalDevicesScanned = 0;
  }

  // If we haven't seen any devices at all for 60 seconds, restart BLE entirely
  static unsigned long lastDeviceTime = 0;
  if (totalDevicesScanned > 0) {
    lastDeviceTime = currentTime;
  }

  if (currentTime - lastDeviceTime > 60000 && lastDeviceTime > 0 &&
      currentTime - lastHardRestartTime > 30000) {  // 30 second cooldown for hard restart
    // Serial.println("CRITICAL: No BLE devices for 60s - restarting BLE stack...");

    BLE.end();
    delay(500);

    if (BLE.begin()) {
      BLE.scan();
      // Serial.println("BLE stack restarted successfully");
    } else {
      // Serial.println("ERROR: Failed to restart BLE stack!");
    }

    // Reset counters
    totalDevicesScanned = 0;
    lastDeviceTime = currentTime;
    lastHardRestartTime = currentTime;
    lastRestartTime = currentTime;  // Also reset soft restart timer
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

  // Always output JSON every 5 seconds OR immediately when new data arrives
  if (dataUpdated || (currentTime - lastJsonOutput >= 5000)) {
    lastJsonOutput = currentTime;
    dataUpdated = false; // Reset the flag

    // Create combined JSON using the existing toJson function
    Serial.println(toJson(combinedData.si7021_temp, combinedData.si7021_hum,
                         combinedData.sht30_temp, combinedData.sht30_hum,
                         combinedData.veml6035_light, combinedData.imu_data));
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "LED_ON") {
      digitalWrite(LED_BUILTIN_1, HIGH);
    } else if (command == "LED_OFF") {
      digitalWrite(LED_BUILTIN_1, LOW);
    }
  }
}