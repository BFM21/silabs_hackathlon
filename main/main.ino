#include <Wire.h>
#include <ArduinoBLE.h>
#include "si7021.h"
#include "veml6035.h"
#include "sht30.h"
#include "pins_arduino.h"
#include <silabs_imu.h>

SilabsIMU imu;
SI7021 si7021_ths;
VEML6035 veml6035_als;
SHT30 sht30_ths;
MovementData movementData;

bool imuSetupFailed = false;
bool si7021SetupFailed = false;
bool veml6035SetupFailed = false;
bool sht30SetupFailed = false;

// Device name to appear in scans
const char DEVICE_NAME[] = "Hackathlon";

// Custom service UUID
const char SERVICE_UUID[] = "12345678-1234-5678-1234-56789abcdef0";

// Custom characteristic UUIDs
const char SI7021_H_UUID[] = "12345678-1234-5678-1234-56789abcdef1";
const char SI7021_T_UUID[] = "12345678-1234-5678-1234-56789abcdef2";
const char SHT30_H_UUID[] = "12345678-1234-5678-1234-56789abcdef3";
const char SHT30_T_UUID[] = "12345678-1234-5678-1234-56789abcdef4";
const char VEML6035_UUID[] = "12345678-1234-5678-1234-56789abcdef5";
const char IMU_UUID[] = "12345678-1234-5678-1234-56789abcdef6";

// BLE service and characteristics
BLEService sensorService(SERVICE_UUID);
BLECharacteristic si7021_t_char(SI7021_T_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic si7021_h_char(SI7021_H_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic sht30_t_char(SHT30_T_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic sht30_h_char(SHT30_H_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic veml6035_char(VEML6035_UUID, BLERead | BLENotify, sizeof(float));
// Increased buffer size for JSON string
BLECharacteristic imu_char(IMU_UUID, BLERead | BLENotify, 200);

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 2000;

// Function to convert ActivityState enum to string
const char *activityStateToString(ActivityState state)
{
  switch (state)
  {
  case STILL:
    return "STILL";
  case MOVING:
    return "MOVING";
  case ACTIVE:
    return "ACTIVE";
  default:
    return "UNKNOWN";
  }
}

// Function to create JSON string from MovementData
String createMovementDataJSON(const MovementData &data)
{
  String json = "{";
  json += "\"sensor\":\"imu\",";
  json += "\"current_state\":\"" + String(activityStateToString(data.current_state)) + "\",";
  json += "\"movement_intensity\":" + String(data.movement_intensity, 3) + ",";
  json += "\"movements_per_minute\":" + String(data.movements_per_minute) + ",";
  json += "\"is_likely_asleep\":" + String(data.is_likely_asleep ? "true" : "false") + ",";
  json += "\"still_duration_minutes\":" + String(data.still_duration_minutes);
  json += "}";
  return json;
}

void setup()
{

  Serial.begin(115200);

  Wire.begin();

  if (imu.begin())
  {
    Serial.println("IMU initialized successfully!");
  }
  else
  {
    imuSetupFailed = true;

    Serial.println("Failed to initialize IMU!");
  }

  if (si7021_ths.init() != 0)
  {
    si7021SetupFailed = true;

    Serial.println("SI7021 Temperature & Humidity Sensor setup failed");
  }
  else
  {
    Serial.println("SI7021 Temperature & Humidity Sensor Ready");
  }

  if (sht30_ths.init() != 0)
  {
    sht30SetupFailed = true;

    Serial.println("SHT30 Temperature & Humidity Sensor setup failed");
  }
  else
  {
    Serial.println("SHT30 Temperature & Humidity Sensor Ready");
  }

  if (veml6035_als.init() != 0)
  {
    veml6035SetupFailed = true;

    Serial.println("Ambient Light Sensor setup failed");
  }
  else
  {
    Serial.println("Ambient Light Sensor Ready");
  }

  Serial.println("=== ArduinoBLE Multi-Sensor Server ===");

  if (!BLE.begin())
  {
    Serial.println("ERROR: BLE.begin() failed!");
    while (1)
      ;
  }

  BLE.setLocalName(DEVICE_NAME);
  BLE.setDeviceName(DEVICE_NAME);
  BLE.setAdvertisedService(sensorService);

  // Add characteristics
  sensorService.addCharacteristic(si7021_t_char);
  sensorService.addCharacteristic(si7021_h_char);
  sensorService.addCharacteristic(sht30_t_char);
  sensorService.addCharacteristic(sht30_h_char);
  sensorService.addCharacteristic(veml6035_char);
  sensorService.addCharacteristic(imu_char);
  BLE.addService(sensorService);

  if (!BLE.advertise())
  {
    Serial.println("ERROR: BLE.advertise() failed!");
  }
  else
  {
    Serial.print("Advertising as: ");
    Serial.println(DEVICE_NAME);
  }
}

void loop()
{
  float si7021_h = -999.0, si7021_t = -999.0, sht30_h = -999.0, sht30_t = -999.0, veml6035_l = -999.0;
  if (!si7021SetupFailed)
  {
    si7021_h = si7021_ths.readHumidity();
    si7021_t = si7021_ths.readTemperature();

    if (si7021_h != -999 && si7021_t != -999)
    {
      Serial.print("SI7021 - ");
      Serial.print("Humidity: ");
      Serial.print(si7021_h, 2);
      Serial.print(" %RH, Temperature: ");
      Serial.print(si7021_t, 2);
      Serial.println(" °C");
    }
    else
    {
      Serial.println("Error reading temperature/humidity sensor data");
    }
  }

  if (!sht30SetupFailed)
  {

    uint8_t result = sht30_ths.readTempHumidity(&sht30_t, &sht30_h, SHT30_Repeatability::HIGH);

    if (result == SHT30_OK)
    {
      Serial.print("SHT30 - ");
      Serial.print("Humidity: ");
      Serial.print(sht30_h, 2);
      Serial.print(" %RH, Temperature: ");
      Serial.print(sht30_t, 2);
      Serial.println(" °C");
    }
    else
    {
      sht30_h = -999.0;
      sht30_t = -999.0;
      Serial.print("Error: ");
      Serial.println(result);
    }
  }

  if (!veml6035SetupFailed)
  {

    veml6035_l = veml6035_als.readAmbientLight();

    if (veml6035_l != -999)
    {
      Serial.print("Light: ");
      Serial.print(veml6035_l, 2);
      Serial.println(" lux");
    }
    else
    {
      Serial.println("Error reading light sensor data");
    }
  }

  if (!imuSetupFailed)
  {

    static unsigned long last_sample = 0;
    unsigned long current_time = millis();

    // Sample at 5Hz (every 200ms)
    if (current_time - last_sample >= 200)
    {
      last_sample = current_time;

      if (imu.readIMU())
      {
        imu.calculateMovement();
        imu.updateMovementState();
        imu.incrementSampleCount();

        // Update every minute (10 seconds for demo)
        if (imu.shouldUpdateMinutelyStats())
        {
          imu.updateMinutelyStats();
          imu.printStatus();
          movementData = imu.getMovementData();
        }
      }
    }
  }

  BLEDevice central = BLE.central();

  // Poll BLE stack
  BLE.poll();

  if (central)
  {
    if (central.connected())
    {
      // Update sensor values at intervals

      unsigned long now = millis();
      if (now - lastUpdate >= updateInterval)
      {
        lastUpdate = now;

        // Write values
        if (si7021_t != -999.0)
        {
          si7021_t_char.writeValue((byte *)&si7021_t, sizeof(si7021_t));
        }
        if (si7021_h != -999.0)
        {
          si7021_h_char.writeValue((byte *)&si7021_h, sizeof(si7021_h));
        }
        if (sht30_t != -999.0)
        {
          sht30_t_char.writeValue((byte *)&sht30_t, sizeof(sht30_t));
        }
        if (sht30_h != -999.0)
        {
          sht30_h_char.writeValue((byte *)&sht30_h, sizeof(sht30_h));
        }
        if (veml6035_l != -999.0)
        {
          veml6035_char.writeValue((byte *)&veml6035_l, sizeof(veml6035_l));
        }

        // Send IMU data as JSON
        String imuJson = createMovementDataJSON(movementData);
        Serial.print("Sending IMU JSON: ");
        Serial.println(imuJson);

        // Convert String to byte array and send
        const char *jsonCStr = imuJson.c_str();
        imu_char.writeValue((byte *)jsonCStr, imuJson.length());
      }
    }
    else
    {
      Serial.println("Central disconnected");
    }
  }
}