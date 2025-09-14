#ifndef SILABS_IMU_H
#define SILABS_IMU_H

#include <Arduino.h>
#include <SPI.h>
#include <math.h>

#define SENSOR_ENABLE_PIN  PC9
#define IMU_CS_PIN         PA7

#define ICM20689_WHO_AM_I        0x75
#define ICM20689_PWR_MGMT_1      0x6B
#define ICM20689_ACCEL_XOUT_H    0x3B
#define ICM20689_WHO_AM_I_VAL    0x98
#define SPI_READ_BIT             0x80
#define SPI_WRITE_BIT            0x00

#define MOVEMENT_THRESHOLD       0.15
#define ACTIVE_THRESHOLD         0.5
#define SAMPLES_PER_MINUTE       300

enum ActivityState {
  STILL = 0,
  MOVING = 1,
  ACTIVE = 2
};

struct IMUReading {
  float accel_x, accel_y, accel_z;
  float total_acceleration;
};

struct MovementData {
  ActivityState current_state;
  float movement_intensity;
  int movements_per_minute;
  bool is_likely_asleep;
  unsigned long still_duration_minutes;
};

class SilabsIMU {
  private:
    IMUReading imu;
    MovementData movement;
    bool imuInitialized;

    int movement_count;
    int sample_count;
    float movement_sum;
    unsigned long last_movement_time;
    unsigned long last_minute_update;

    uint8_t readRegister(uint8_t reg);
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
    void writeRegister(uint8_t reg, uint8_t value);

  public:
    SilabsIMU();
    bool begin();
    bool initIMU();
    bool readIMU();
    void calculateMovement();
    void updateMovementState();
    void updateMinutelyStats();
    void printStatus();
    void printSummary();
    void incrementSampleCount();
    bool shouldUpdateMinutelyStats();

    IMUReading getIMUReading() const { return imu; }
    MovementData getMovementData() const { return movement; }
    bool isInitialized() const { return imuInitialized; }
};

#endif