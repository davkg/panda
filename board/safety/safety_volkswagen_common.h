#ifndef SAFETY_VOLKSWAGEN_COMMON_H
#define SAFETY_VOLKSWAGEN_COMMON_H

// CAN arb ID space is non-conflicting and partially shared between MQB and MLB
// PQ CAN arb ID space is entirely different and defined at the PQ level
#define MSG_ESP_19      0x0B2   // RX from ABS, for wheel speeds (MQB)
#define MSG_LH_EPS_03   0x09F   // RX from EPS, for driver steering torque (shared)
#define MSG_ESP_03      0x103   // RX from ABS, for wheel speeds (MLB)
#define MSG_MOTOR_03    0x105   // RX from ECU, for driver throttle input (MLB)
#define MSG_ESP_05      0x106   // RX from ABS, for brake switch state (shared)
#define MSG_LS_01       0x10B   // TX by OP, ACC control buttons for cancel/resume (MLB)
#define MSG_TSK_02      0x10C   // RX from ECU, for ACC status from drivetrain coordinator (MLB)
#define MSG_TSK_06      0x120   // RX from ECU, for ACC status from drivetrain coordinator (MQB)
#define MSG_MOTOR_20    0x121   // RX from ECU, for driver throttle input (MQB)
#define MSG_ACC_06      0x122   // TX by OP, ACC control instructions to the drivetrain coordinator (MQB)
#define MSG_HCA_01      0x126   // TX by OP, Heading Control Assist steering torque (shared)
#define MSG_GRA_ACC_01  0x12B   // TX by OP, ACC control buttons for cancel/resume (MQB)
#define MSG_ACC_07      0x12E   // TX by OP, ACC control instructions to the drivetrain coordinator (MQB)
#define MSG_ACC_02      0x30C   // TX by OP, ACC HUD data to the instrument cluster (shared)
#define MSG_MOTOR_14    0x3BE   // RX from ECU, for brake switch status (???)
#define MSG_LDW_02      0x397   // TX by OP, Lane line recognition and text alerts (???)

// Shared MLB and MQB lateral limits
const SteeringLimits VOLKSWAGEN_MXB_STEERING_LIMITS = {
  .max_steer = 300,              // 3.0 Nm (EPS side max of 3.0Nm with fault if violated)
  .max_rt_delta = 75,            // 4 max rate up * 50Hz send rate * 250000 RT interval / 1000000 = 50 ; 50 * 1.5 for safety pad = 75
  .max_rt_interval = 250000,     // 250ms between real time checks
  .max_rate_up = 4,              // 2.0 Nm/s RoC limit (EPS rack has own soft-limit of 5.0 Nm/s)
  .max_rate_down = 10,           // 5.0 Nm/s RoC limit (EPS rack has own soft-limit of 5.0 Nm/s)
  .driver_torque_allowance = 80,
  .driver_torque_factor = 3,
  .type = TorqueDriverLimited,
};

// Shared MLB and MQB longitudinal limits
// acceleration in m/s2 * 1000 to avoid floating point math
const LongitudinalLimits VOLKSWAGEN_MXB_LONG_LIMITS = {
  .max_accel = 2000,
  .min_accel = -3500,
  .inactive_accel = 3010,  // VW sends one increment above the max range when inactive
};

// Shared MLB and MQB checksum and counter sizes and offsets
static uint32_t volkswagen_mxb_get_checksum(CANPacket_t *to_push) {
  return (uint8_t)GET_BYTE(to_push, 0);
}

static uint8_t volkswagen_mxb_get_counter(CANPacket_t *to_push) {
  return (uint8_t)GET_BYTE(to_push, 1) & 0xFU;
}

const uint16_t FLAG_VOLKSWAGEN_LONG_CONTROL = 1;
uint8_t volkswagen_crc8_lut_8h2f[256]; // Static lookup table for CRC8 poly 0x2F, aka 8H2F/AUTOSAR

bool volkswagen_longitudinal = false;
bool volkswagen_set_button_prev = false;
bool volkswagen_resume_button_prev = false;
bool volkswagen_brake_pedal_switch = false;
bool volkswagen_brake_pressure_detected = false;

#endif
