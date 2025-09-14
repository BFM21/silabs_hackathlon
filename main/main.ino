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
const char IMU_CS_UUID[] = "12345678-1234-5678-1234-56789abcdef6";
const char IMU_MI_UUID[] = "12345678-1234-5678-1234-56789abcdef7";
const char IMU_MPM_UUID[] = "12345678-1234-5678-1234-56789abcdef8";
const char IMU_ILA_UUID[] = "12345678-1234-5678-1234-56789abcdef9";
const char IMU_SDM_UUID[] = "12345678-1234-5678-1234-56789abcdee0";

// BLE service and characteristics
BLEService sensorService(SERVICE_UUID);
BLECharacteristic si7021_t_char(SI7021_T_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic si7021_h_char(SI7021_H_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic sht30_t_char(SHT30_T_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic sht30_h_char(SHT30_H_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic veml6035_char(VEML6035_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic imu_cs_char(IMU_CS_UUID, BLERead | BLENotify, sizeof(uint8_t));
BLECharacteristic imu_mi_char(IMU_MI_UUID, BLERead | BLENotify, sizeof(float));
BLECharacteristic imu_mpm_char(IMU_MPM_UUID, BLERead | BLENotify, sizeof(int));
BLECharacteristic imu_ila_char(IMU_ILA_UUID, BLERead | BLENotify, sizeof(bool));
BLECharacteristic imu_sdm_char(IMU_SDM_UUID, BLERead | BLENotify, sizeof(unsigned long));

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 5000;




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
  sensorService.addCharacteristic(imu_cs_char);
  sensorService.addCharacteristic(imu_mi_char);
  sensorService.addCharacteristic(imu_mpm_char);
  sensorService.addCharacteristic(imu_ila_char);
  sensorService.addCharacteristic(imu_sdm_char);
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

    if (si7021_h == -999 || si7021_t == -999)
    {
     Serial.println("Error reading temperature/humidity sensor data");
    }
   
  }

  if (!sht30SetupFailed)
  {

    uint8_t result = sht30_ths.readTempHumidity(&sht30_t, &sht30_h, SHT30_Repeatability::HIGH);

    if (result != SHT30_OK)
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

    if (veml6035_l == -999)
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
          Serial.print("SI7021 Temperature: ");
          Serial.println(si7021_t);
        }
        if (si7021_h != -999.0)
        {
          si7021_h_char.writeValue((byte *)&si7021_h, sizeof(si7021_h));
          Serial.print("SI7021 Humidity: ");
          Serial.println(si7021_h);
        }
        if (sht30_t != -999.0)
        {
          sht30_t_char.writeValue((byte *)&sht30_t, sizeof(sht30_t));
          Serial.print("SHT30 Temperature: ");
          Serial.println(sht30_t);
        }
        if (sht30_h != -999.0)
        {
          sht30_h_char.writeValue((byte *)&sht30_h, sizeof(sht30_h));
          Serial.print("SHT30 Humidity: ");
          Serial.println(sht30_h);
        }
        if (veml6035_l != -999.0)
        {
          veml6035_char.writeValue((byte *)&veml6035_l, sizeof(veml6035_l));
          Serial.print("VEML6035 Light: ");
          Serial.println(veml6035_l);
        }

        // Send IMU data individually
        if (!imuSetupFailed)
        {
          uint8_t currentState = (uint8_t)movementData.current_state;
          imu_cs_char.writeValue((byte *)&currentState, sizeof(currentState));
          Serial.print("IMU Current State: ");
          Serial.println(currentState);

          imu_mi_char.writeValue((byte *)&movementData.movement_intensity, sizeof(movementData.movement_intensity));
          Serial.print("IMU Movement Intensity: ");
          Serial.println(movementData.movement_intensity);

          imu_mpm_char.writeValue((byte *)&movementData.movements_per_minute, sizeof(movementData.movements_per_minute));
          Serial.print("IMU Movements Per Minute: ");
          Serial.println(movementData.movements_per_minute);

          imu_ila_char.writeValue((byte *)&movementData.is_likely_asleep, sizeof(movementData.is_likely_asleep));
          Serial.print("IMU Is Likely Asleep: ");
          Serial.println(movementData.is_likely_asleep);

          imu_sdm_char.writeValue((byte *)&movementData.still_duration_minutes, sizeof(movementData.still_duration_minutes));
          Serial.print("IMU Still Duration Minutes: ");
          Serial.println(movementData.still_duration_minutes);
        }
      }
    }
    else
    {
      Serial.println("Central disconnected");
    }
  }
}