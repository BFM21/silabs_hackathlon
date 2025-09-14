#include <ArduinoBLE.h>

// Device name to appear in scans
const char DEVICE_NAME[] = "Hackathlon";

// Custom service UUID
const char SERVICE_UUID[] = "12345678-1234-5678-1234-56789abcdef0";

// Custom characteristic UUIDs
const char SI7021_H_UUID[]  = "12345678-1234-5678-1234-56789abcdef1";
const char SI7021_T_UUID[]  = "12345678-1234-5678-1234-56789abcdef2";
const char SHT30_H_UUID[]   = "12345678-1234-5678-1234-56789abcdef3";
const char SHT30_T_UUID[]   = "12345678-1234-5678-1234-56789abcdef4";
const char VEML6035_UUID[] = "12345678-1234-5678-1234-56789abcdef5";



void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== ArduinoBLE Multi-Sensor Client ===");

  if (!BLE.begin()) {
    Serial.println("ERROR: BLE.begin() failed!");
    while (1);
  }

  // Start scanning for the server
  BLE.scanForName(DEVICE_NAME);
  Serial.println("Scanning for server...");
}

void loop() {
  BLEDevice server = BLE.available();

  if (server) {
    Serial.print("Found server: ");
    Serial.println(server.localName());

    // Stop scanning
    BLE.stopScan();

    Serial.println("Connecting...");
    if (server.connect()) {
      Serial.println("Connected to server");

      // Discover service and characteristics
      if (server.discoverAttributes()) {
        Serial.println("Service and characteristics discovered");

        BLECharacteristic si7021_t_char = server.characteristic(SI7021_T_UUID);
        BLECharacteristic si7021_h_char  = server.characteristic(SI7021_H_UUID);
        BLECharacteristic sht30_t_char = server.characteristic(SHT30_T_UUID);
        BLECharacteristic sht30_h_char = server.characteristic(SHT30_H_UUID);
        BLECharacteristic veml6035_char = server.characteristic(VEML6035_UUID);

        if (!si7021_t_char || !si7021_h_char || !sht30_t_char|| !sht30_h_char || !veml6035_char) {
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



        Serial.println("Subscribed to sensor notifications");

        while (server.connected()) {
          BLE.poll(); // keep stack responsive

          // Check for new values
          if (si7021_t_char.valueUpdated()) {
            float t;
            si7021_t_char.readValue((byte*)&t, sizeof(t));
            Serial.print("SI7021: "); Serial.print(t); Serial.println(" °C");
          }
          if (si7021_h_char.valueUpdated()) {
            float h;
            si7021_h_char.readValue((byte*)&h, sizeof(h));
            Serial.print("SI7021: "); Serial.print(h); Serial.println(" %");
          }
          if (sht30_t_char.valueUpdated()) {
            float t;
            sht30_t_char.readValue((byte*)&t, sizeof(t));
            Serial.print("SHT30: "); Serial.print(t); Serial.println(" °C");
          }
          if (sht30_h_char.valueUpdated()) {
            float h;
            sht30_h_char.readValue((byte*)&h, sizeof(h));
            Serial.print("SHT30: "); Serial.print(h); Serial.println(" %");
          }
          if (veml6035_char.valueUpdated()) {
            float l;
            veml6035_char.readValue((byte*)&l, sizeof(l));
            Serial.print("VEML6035: "); Serial.print(l); Serial.println(" lux");
          }
        }

        Serial.println("Server disconnected, scanning again...");
        BLE.scanForName(DEVICE_NAME);
      }
    } else {
      Serial.println("Connection failed, scanning again...");
      BLE.scanForName(DEVICE_NAME);
    }
  }
}



#include <ArduinoBLE.h>

// Device name to appear in scans
const char DEVICE_NAME[] = "Hackathlon";

// Custom service UUID
const char SERVICE_UUID[] = "12345678-1234-5678-1234-56789abcdef0";

// Custom characteristic UUIDs
const char TEMP_UUID[]  = "12345678-1234-5678-1234-56789abcdef1";
const char HUM_UUID[]   = "12345678-1234-5678-1234-56789abcdef2";
const char LIGHT_UUID[] = "12345678-1234-5678-1234-56789abcdef3";

// BLE service and characteristics
BLEService sensorService(SERVICE_UUID);
BLECharacteristic tempChar(TEMP_UUID,  BLERead | BLENotify, sizeof(float));
BLECharacteristic humChar(HUM_UUID,   BLERead | BLENotify, sizeof(float));
BLECharacteristic lightChar(LIGHT_UUID, BLERead | BLENotify, sizeof(int));

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000; // 1 second

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("=== ArduinoBLE Multi-Sensor Server ===");

  if (!BLE.begin()) {
    Serial.println("ERROR: BLE.begin() failed!");
    while (1);
  }

  BLE.setLocalName(DEVICE_NAME);
  BLE.setDeviceName(DEVICE_NAME);
  BLE.setAdvertisedService(sensorService);

  // Add characteristics
  sensorService.addCharacteristic(tempChar);
  sensorService.addCharacteristic(humChar);
  sensorService.addCharacteristic(lightChar);
  BLE.addService(sensorService);

  // Initial values
  float initTemp = 20.0f;
  float initHum  = 50.0f;
  int initLight  = 100;
  tempChar.writeValue((byte*)&initTemp, sizeof(initTemp));
  humChar.writeValue((byte*)&initHum, sizeof(initHum));
  lightChar.writeValue((byte*)&initLight, sizeof(initLight));

  // Start advertising once
  if (!BLE.advertise()) {
    Serial.println("ERROR: BLE.advertise() failed!");
  } else {
    Serial.print("Advertising as: ");
    Serial.println(DEVICE_NAME);
  }
}

void loop() {
  BLEDevice central = BLE.central();

  // Poll BLE stack
  BLE.poll();

  if (central) {
    if (central.connected()) {
      // Update sensor values at intervals
      unsigned long now = millis();
      if (now - lastUpdate >= updateInterval) {
        lastUpdate = now;

        // Simulated sensor values
        float tempVal = 20.0f + (now % 5000) / 100.0f;   // 20.0 – 70.0
        float humVal  = 40.0f + (now % 3000) / 100.0f;   // 40.0 – 70.0
        int lightVal  = (now / 100) % 1000;              // 0 – 999

        // Write values
        tempChar.writeValue((byte*)&tempVal, sizeof(tempVal));
        humChar.writeValue((byte*)&humVal, sizeof(humVal));
        lightChar.writeValue((byte*)&lightVal, sizeof(lightVal));

        Serial.print("Temp: "); Serial.print(tempVal);
        Serial.print(" °C | Hum: "); Serial.print(humVal);
        Serial.print(" % | Light: "); Serial.println(lightVal);
      }
    } else {
      Serial.println("Central disconnected");
    }
  }
}