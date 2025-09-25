#include <ArduinoBLE.h>
#include "silabs_imu.h"

#include "pins_arduino.h"

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

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== ArduinoBLE Multi-Sensor Client ===");

  if (!BLE.begin())
  {
    Serial.println("ERROR: BLE.begin() failed!");
    while (1)
      ;
  }

  // Start scanning for the server
  BLE.scanForName(DEVICE_NAME);
  Serial.println("Scanning for server...");
}

void loop()
{

  BLEDevice server = BLE.available();

  if (server)
  {
    Serial.print("Found server: ");
    Serial.println(server.localName());

    // Stop scanning
    BLE.stopScan();

    Serial.println("Connecting...");
    if (server.connect())
    {
      Serial.println("Connected to server");

      // Discover service and characteristics
      if (server.discoverAttributes())
      {
        Serial.println("Service and characteristics discovered");

        BLECharacteristic si7021_t_char = server.characteristic(SI7021_T_UUID);
        BLECharacteristic si7021_h_char = server.characteristic(SI7021_H_UUID);
        BLECharacteristic sht30_t_char = server.characteristic(SHT30_T_UUID);
        BLECharacteristic sht30_h_char = server.characteristic(SHT30_H_UUID);
        BLECharacteristic veml6035_char = server.characteristic(VEML6035_UUID);
        BLECharacteristic imu_cs_char = server.characteristic(IMU_CS_UUID);
        BLECharacteristic imu_mi_char = server.characteristic(IMU_MI_UUID);
        BLECharacteristic imu_mpm_char = server.characteristic(IMU_MPM_UUID);
        BLECharacteristic imu_ila_char = server.characteristic(IMU_ILA_UUID);
        BLECharacteristic imu_sdm_char = server.characteristic(IMU_SDM_UUID);

        if (!si7021_t_char || !si7021_h_char || !sht30_t_char || !sht30_h_char || !veml6035_char || !imu_cs_char || !imu_mi_char || !imu_mpm_char || !imu_ila_char || !imu_sdm_char)
        {
          Serial.println("Failed to find characteristics!");
          server.disconnect();
          BLE.scanForName(DEVICE_NAME);
          return;
        }

        // Enable notifications
        si7021_t_char.subscribe();
        si7021_h_char.subscribe();
        sht30_t_char.subscribe();
        sht30_h_char.subscribe();
        veml6035_char.subscribe();
        imu_cs_char.subscribe();
        imu_mi_char.subscribe();
        imu_mpm_char.subscribe();
        imu_ila_char.subscribe();
        imu_sdm_char.subscribe();

        Serial.println("Subscribed to sensor notifications");
        String command = "";
        while (server.connected())
        {
          
          BLE.poll(); // keep stack responsive

          // Check for new values
          if (si7021_t_char.valueUpdated() || si7021_h_char.valueUpdated() ||
              sht30_t_char.valueUpdated() || sht30_h_char.valueUpdated() ||
              veml6035_char.valueUpdated() || imu_cs_char.valueUpdated() || imu_mi_char.valueUpdated() || imu_mpm_char.valueUpdated() || imu_ila_char.valueUpdated() || imu_sdm_char.valueUpdated())
          {

            float si7021_t, si7021_h, sht30_t, sht30_h, veml6035_l;
            uint8_t imu_cs;
            float imu_mi;
            int imu_mpm;
            bool imu_ila;
            unsigned long imu_sdm;

            si7021_t_char.readValue((byte *)&si7021_t, sizeof(si7021_t));
            si7021_h_char.readValue((byte *)&si7021_h, sizeof(si7021_h));
            sht30_t_char.readValue((byte *)&sht30_t, sizeof(sht30_t));
            sht30_h_char.readValue((byte *)&sht30_h, sizeof(sht30_h));
            veml6035_char.readValue((byte *)&veml6035_l, sizeof(veml6035_l));
            imu_cs_char.readValue((byte *)&imu_cs, sizeof(imu_cs));
            imu_mi_char.readValue((byte *)&imu_mi, sizeof(imu_mi));
            imu_mpm_char.readValue((byte *)&imu_mpm, sizeof(imu_mpm));
            imu_ila_char.readValue((byte *)&imu_ila, sizeof(imu_ila));
            imu_sdm_char.readValue((byte *)&imu_sdm, sizeof(imu_sdm));

            // Send one JSON object
            MovementData imu_data;
            imu_data.current_state = (ActivityState)imu_cs;
            imu_data.movement_intensity = imu_mi;
            imu_data.movements_per_minute = imu_mpm;
            imu_data.is_likely_asleep = imu_ila;
            imu_data.still_duration_minutes = imu_sdm;
            String json = toJson(si7021_t, si7021_h, sht30_t, sht30_h, veml6035_l, imu_data);
            Serial.println(json);
          }
        }

        Serial.println("Server disconnected, scanning again...");
        BLE.scanForName(DEVICE_NAME);
      }
    }
    else
    {
      Serial.println("Connection failed, scanning again...");
      BLE.scanForName(DEVICE_NAME);
    }
  }
}

String toJson(float si7021_t, float si7021_h,
              float sht30_t, float sht30_h,
              float veml6035_l,
              MovementData imu_data)
{
  String json = "{";
  json += "\"si7021_temp\":" + String(si7021_t, 2) + ",";
  json += "\"si7021_hum\":" + String(si7021_h, 2) + ",";
  // json += "\"sht30_temp\":"  + String(sht30_t, 2) + ",";
  // json += "\"sht30_hum\":"   + String(sht30_h, 2) + ",";
  json += "\"veml6035\":" + String(veml6035_l, 2) + ",";

  // IMU data as nested object
  json += "\"imu_data\":{";
  json += "\"current_state\":" + String((int)imu_data.current_state) + ",";
  json += "\"movement_intensity\":" + String(imu_data.movement_intensity, 2) + ",";
  json += "\"movements_per_minute\":" + String(imu_data.movements_per_minute) + ",";
  json += "\"is_likely_asleep\":" + String(imu_data.is_likely_asleep ? "true" : "false") + ",";
  json += "\"still_duration_minutes\":" + String(imu_data.still_duration_minutes);
  json += "}";

  json += "}";
  return json;
}