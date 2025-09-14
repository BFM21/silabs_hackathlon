#include "silabs_imu.h"

SilabsIMU::SilabsIMU() {
  imuInitialized = false;
  movement_count = 0;
  sample_count = 0;
  movement_sum = 0;
  last_movement_time = 0;
  last_minute_update = 0;

  movement.current_state = STILL;
  movement.movement_intensity = 0;
  movement.movements_per_minute = 0;
  movement.is_likely_asleep = false;
  movement.still_duration_minutes = 0;
}

bool SilabsIMU::begin() {
  pinMode(SENSOR_ENABLE_PIN, OUTPUT);
  digitalWrite(SENSOR_ENABLE_PIN, HIGH);
  delay(100);

  pinMode(IMU_CS_PIN, OUTPUT);
  digitalWrite(IMU_CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider(16);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  if (initIMU()) {
    imuInitialized = true;
    last_minute_update = millis();
    return true;
  }

  return false;
}

bool SilabsIMU::initIMU() {
  uint8_t whoAmI = readRegister(ICM20689_WHO_AM_I);
  if (whoAmI != ICM20689_WHO_AM_I_VAL) {
    return false;
  }

  writeRegister(ICM20689_PWR_MGMT_1, 0x80);
  delay(100);
  writeRegister(ICM20689_PWR_MGMT_1, 0x00);
  delay(50);

  return true;
}

bool SilabsIMU::readIMU() {
  uint8_t buffer[6];

  if (!readRegisters(ICM20689_ACCEL_XOUT_H, buffer, 6)) {
    return false;
  }

  int16_t accel_x_raw = (buffer[0] << 8) | buffer[1];
  int16_t accel_y_raw = (buffer[2] << 8) | buffer[3];
  int16_t accel_z_raw = (buffer[4] << 8) | buffer[5];

  imu.accel_x = accel_x_raw / 16384.0f;
  imu.accel_y = accel_y_raw / 16384.0f;
  imu.accel_z = accel_z_raw / 16384.0f;

  return true;
}

void SilabsIMU::calculateMovement() {
  imu.total_acceleration = sqrt(
    imu.accel_x * imu.accel_x +
    imu.accel_y * imu.accel_y +
    imu.accel_z * imu.accel_z
  );

  float movement_magnitude = fabs(imu.total_acceleration - 1.0);

  movement.movement_intensity = movement_magnitude;
  movement_sum += movement_magnitude;
}

void SilabsIMU::updateMovementState() {
  unsigned long current_time = millis();

  if (movement.movement_intensity < MOVEMENT_THRESHOLD) {
    movement.current_state = STILL;
  } else if (movement.movement_intensity < ACTIVE_THRESHOLD) {
    movement.current_state = MOVING;
    movement_count++;
    last_movement_time = current_time;
  } else {
    movement.current_state = ACTIVE;
    movement_count++;
    last_movement_time = current_time;
  }

  if (movement.current_state == STILL) {
    movement.still_duration_minutes = (current_time - last_movement_time) / 60000;
  } else {
    movement.still_duration_minutes = 0;
  }

  movement.is_likely_asleep = (movement.still_duration_minutes > 1);
}

void SilabsIMU::updateMinutelyStats() {
  movement.movements_per_minute = movement_count;

  if (sample_count > 0) {
    movement.movement_intensity = movement_sum / sample_count;
  }

  movement_count = 0;
  sample_count = 0;
  movement_sum = 0;
}

void SilabsIMU::printStatus() {
  unsigned long elapsed_seconds = millis() / 1000;

  Serial.print(elapsed_seconds);
  Serial.print("s | ");

  switch (movement.current_state) {
    case STILL:
      Serial.print("STILL  ");
      break;
    case MOVING:
      Serial.print("MOVING ");
      break;
    case ACTIVE:
      Serial.print("ACTIVE ");
      break;
  }

  Serial.print(" | ");
  Serial.print(movement.movement_intensity, 3);
  Serial.print("     | ");
  Serial.print(movement.movements_per_minute);
  Serial.print("       | ");
  Serial.print(movement.still_duration_minutes);
  Serial.print("        | ");
  Serial.print(movement.is_likely_asleep ? "YES" : "NO");
  Serial.println();
}

void SilabsIMU::printSummary() {
  Serial.println("\n--- 10 Minute Summary ---");

  if (movement.is_likely_asleep) {
    Serial.println("Status: Likely sleeping");
    Serial.print("Still for: ");
    Serial.print(movement.still_duration_minutes);
    Serial.println(" minutes");
  } else {
    Serial.println("Status: Awake/Active");
    Serial.print("Recent movements: ");
    Serial.print(movement.movements_per_minute);
    Serial.println(" per minute");
  }

  Serial.print("Current activity: ");
  switch (movement.current_state) {
    case STILL:
      Serial.println("Still/Resting");
      break;
    case MOVING:
      Serial.println("Light movement");
      break;
    case ACTIVE:
      Serial.println("Active movement");
      break;
  }

  Serial.println("------------------------\n");
}

uint8_t SilabsIMU::readRegister(uint8_t reg) {
  digitalWrite(IMU_CS_PIN, LOW);
  SPI.transfer(reg | SPI_READ_BIT);
  uint8_t data = SPI.transfer(0x00);
  digitalWrite(IMU_CS_PIN, HIGH);
  return data;
}

bool SilabsIMU::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
  digitalWrite(IMU_CS_PIN, LOW);
  SPI.transfer(reg | SPI_READ_BIT);
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = SPI.transfer(0x00);
  }
  digitalWrite(IMU_CS_PIN, HIGH);
  return true;
}

void SilabsIMU::writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(IMU_CS_PIN, LOW);
  SPI.transfer(reg | SPI_WRITE_BIT);
  SPI.transfer(value);
  digitalWrite(IMU_CS_PIN, HIGH);
  delayMicroseconds(10);
}

void SilabsIMU::incrementSampleCount() {
  sample_count++;
}

bool SilabsIMU::shouldUpdateMinutelyStats() {
  unsigned long current_time = millis();
  if (current_time - last_minute_update >= 10000) {
    last_minute_update = current_time;
    return true;
  }
  return false;
}