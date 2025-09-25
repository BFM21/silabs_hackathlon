#include <Wire.h>
#include <ArduinoBLE.h>
#include "si7021.h"
#include "veml6035.h"
#include "pins_arduino.h"
#include <silabs_imu.h>

SilabsIMU imu;
SI7021 si7021_ths;
VEML6035 veml6035_als;
MovementData movementData;

bool imuSetupFailed = false;
bool si7021SetupFailed = false;
bool veml6035SetupFailed = false;

// Device name to appear in scans
const char DEVICE_NAME[] = "Alva-Wearable";

// Broadcast data structure - optimized for advertising packet size limit
struct SensorData {
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

unsigned long lastBroadcast = 0;
const unsigned long broadcastInterval = 2000; // Broadcast every 2 seconds
uint16_t packetCounter = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Initialize sensors
  if (imu.begin()) {
    Serial.println("IMU initialized successfully!");
  } else {
    imuSetupFailed = true;
    Serial.println("Failed to initialize IMU!");
  }

  if (si7021_ths.init() != 0) {
    si7021SetupFailed = true;
    Serial.println("SI7021 Temperature & Humidity Sensor setup failed");
  } else {
    Serial.println("SI7021 Temperature & Humidity Sensor Ready");
  }


  if (veml6035_als.init() != 0) {
    veml6035SetupFailed = true;
    Serial.println("Ambient Light Sensor setup failed");
  } else {
    Serial.println("Ambient Light Sensor Ready");
  }

  Serial.println("=== Multi-Sensor BLE Broadcaster ===");

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("ERROR: BLE.begin() failed!");
    while (1);
  }

  // Set device name
  BLE.setLocalName(DEVICE_NAME);
  
  Serial.print("Broadcasting as: ");
  Serial.println(DEVICE_NAME);
  Serial.println("Multiple devices can receive data simultaneously!");
}

void loop() {
  // Read sensor data
  float si7021_h = -999.0, si7021_t = -999.0;
  float veml6035_l = -999.0;

  // Read SI7021
  if (!si7021SetupFailed) {
    si7021_h = si7021_ths.readHumidity();
    si7021_t = si7021_ths.readTemperature();

    if (si7021_h == -999 || si7021_t == -999) {
      Serial.println("Error reading SI7021 sensor data");
    }
  }


  // Read VEML6035
  if (!veml6035SetupFailed) {
    veml6035_l = veml6035_als.readAmbientLight();

    if (veml6035_l == -999) {
      Serial.println("Error reading light sensor data");
    }
  }

  // Process IMU data
  if (!imuSetupFailed) {
    static unsigned long last_sample = 0;
    unsigned long current_time = millis();

    // Sample at 5Hz (every 200ms)
    if (current_time - last_sample >= 200) {
      last_sample = current_time;

      if (imu.readIMU()) {
        imu.calculateMovement();
        imu.updateMovementState();
        imu.incrementSampleCount();

        // Update every minute (10 seconds for demo)
        if (imu.shouldUpdateMinutelyStats()) {
          imu.updateMinutelyStats();
          movementData = imu.getMovementData();
        }
      }
    }
  }

  // Broadcast data at intervals
  unsigned long now = millis();
  if (now - lastBroadcast >= broadcastInterval) {
    lastBroadcast = now;
    
    broadcastSensorData(si7021_t, si7021_h, veml6035_l);
  }

  delay(50);
}

void broadcastSensorData(float si7021_t, float si7021_h, float veml6035_l) {
  SensorData data;

  // Pack data into structure
  data.header = 0xFF;  // Explicitly set header
  data.counter = packetCounter++;

  // Convert floats to integers for compact transmission
  data.si7021_temp = (si7021_t != -999.0) ? (int16_t)(si7021_t * 100) : -99900;
  data.si7021_hum = (si7021_h != -999.0) ? (uint16_t)(si7021_h * 100) : 0;
  data.veml6035_light = (veml6035_l != -999.0) ? (uint16_t)(veml6035_l * 100) : 0;

  // IMU data
  if (!imuSetupFailed) {
    data.imu_state = (uint8_t)movementData.current_state;
    data.imu_intensity = (uint16_t)(movementData.movement_intensity * 100);
    data.movements_per_minute = movementData.movements_per_minute;
    data.is_likely_asleep = movementData.is_likely_asleep ? 1 : 0;
    data.still_duration_minutes = (uint16_t)movementData.still_duration_minutes;
  } else {
    data.imu_state = 0;
    data.imu_intensity = 0;
    data.movements_per_minute = 0;
    data.is_likely_asleep = 0;
    data.still_duration_minutes = 0;
  }

  // Calculate checksum
  data.checksum = calculateChecksum(data);

  // Stop advertising to update data
  // Always stop before updating manufacturer data
  BLE.stopAdvertise();
  delay(10); // Small delay to ensure stop completes

  // Set manufacturer data with sensor readings
  BLE.setManufacturerData((uint8_t*)&data, sizeof(data));

  // Set advertising parameters
  BLE.setAdvertisingInterval(200); // 200ms intervals for frequent updates

  // Start advertising with new data
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
    Serial.println("DEBUG: Successfully advertising wearable data");
  }

  // Print data for debugging
  printSensorData(data, si7021_t, si7021_h, veml6035_l);
}

uint8_t calculateChecksum(const SensorData& data) {
  uint32_t sum = (uint32_t)data.header +
                 (uint32_t)data.counter +
                 (uint32_t)data.si7021_temp +
                 (uint32_t)data.si7021_hum +
                 (uint32_t)data.veml6035_light +
                 (uint32_t)data.imu_state +
                 (uint32_t)data.imu_intensity +
                 (uint32_t)data.movements_per_minute +
                 (uint32_t)data.is_likely_asleep +
                 (uint32_t)data.still_duration_minutes;
  return sum & 0xFF;
}

void printSensorData(const SensorData& data, float si7021_t, float si7021_h, float veml6035_l) {
  Serial.println("=== BROADCASTING SENSOR DATA ===");
  Serial.print("Packet #");
  Serial.println(data.counter);
  
  if (si7021_t != -999.0) {
    Serial.print("SI7021 - Temp: ");
    Serial.print(si7021_t, 2);
    Serial.print("°C, Humidity: ");
    Serial.print(si7021_h, 2);
    Serial.println("%");
  }
  
  
  if (veml6035_l != -999.0) {
    Serial.print("VEML6035 - Light: ");
    Serial.print(veml6035_l, 2);
    Serial.println(" lux");
  }
  
  if (!imuSetupFailed) {
    Serial.print("IMU - State: ");
    Serial.print(movementData.current_state);
    Serial.print(", Intensity: ");
    Serial.print(movementData.movement_intensity, 2);
    Serial.print(", Movements/min: ");
    Serial.print(movementData.movements_per_minute);
    Serial.print(", Asleep: ");
    Serial.print(movementData.is_likely_asleep ? "Yes" : "No");
    Serial.print(", Still duration: ");
    Serial.print(movementData.still_duration_minutes);
    Serial.println(" min");
    Serial.print("Transmitted - State: ");
    Serial.print(data.imu_state);
    Serial.print(", Intensity: ");
    Serial.print(data.imu_intensity);
    Serial.print(", MovPerMin: ");
    Serial.print(data.movements_per_minute);
    Serial.print(", Sleep: ");
    Serial.print(data.is_likely_asleep);
    Serial.print(", StillDur: ");
    Serial.println(data.still_duration_minutes);
  }
  
  Serial.print("Data size: ");
  Serial.print(sizeof(data));
  Serial.println(" bytes");
  Serial.println(">>> Multiple receivers can get this data! <<<");
  Serial.println("=====================================");
}