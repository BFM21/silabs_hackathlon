/*
 * Simple Movement Monitor for Silicon Labs xG24 Dev Kit
 * ICM-20689 6-Axis IMU with basic activity tracking
 *
 * Features:
 * - Movement intensity detection
 * - Basic activity level (Still, Moving, Active)
 * - Simple sleep/wake detection
 * - Movement counting
 */

#include <silabs_imu.h>

SilabsIMU imu;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Simple Movement Monitor - xG24 + ICM-20689");
  Serial.println("===========================================");

  if (imu.begin()) {
    Serial.println("IMU initialized successfully!");
  } else {
    Serial.println("Failed to initialize IMU!");
  }

  Serial.println("Time | State  | Intensity | Moves/min | Still(min) | Sleep?");
  Serial.println("-----|--------|-----------|-----------|------------|-------");
}

void loop() {
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
        imu.printStatus();
      }
    }
  }
}