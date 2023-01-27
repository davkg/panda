#include "safety_volkswagen_common.h"

// Transmit of LS_01 is allowed on bus 0 and 2 to keep compatibility with gateway and camera integration
const CanMsg VOLKSWAGEN_MLB_TX_MSGS[] = {{MSG_HCA_01, 0, 8}, {MSG_LS_01, 0, 8}, {MSG_LS_01, 2, 8}};
#define VOLKSWAGEN_MLB_TX_MSGS_LEN (sizeof(VOLKSWAGEN_MLB_TX_MSGS) / sizeof(VOLKSWAGEN_MLB_TX_MSGS[0]))

// FIXME: temporarily disabling checksum verification
AddrCheckStruct volkswagen_mlb_addr_checks[] = {
  {.msg = {{MSG_ESP_03, 0, 8, .check_checksum = false, .max_counter = 15U, .expected_timestep = 20000U}, { 0 }, { 0 }}},
  {.msg = {{MSG_LH_EPS_03, 0, 8, .check_checksum = false, .max_counter = 15U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  {.msg = {{MSG_ESP_05, 0, 8, .check_checksum = false, .max_counter = 15U, .expected_timestep = 20000U}, { 0 }, { 0 }}},
  {.msg = {{MSG_TSK_02, 0, 8, .check_checksum = false, .max_counter = 15U, .expected_timestep = 20000U}, { 0 }, { 0 }}},
  {.msg = {{MSG_MOTOR_03, 0, 8, .check_checksum = false, .max_counter = 15U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
};
#define VOLKSWAGEN_MLB_ADDR_CHECKS_LEN (sizeof(volkswagen_mlb_addr_checks) / sizeof(volkswagen_mlb_addr_checks[0]))
addr_checks volkswagen_mlb_rx_checks = {volkswagen_mlb_addr_checks, VOLKSWAGEN_MLB_ADDR_CHECKS_LEN};


static uint32_t volkswagen_mlb_compute_crc(CANPacket_t *to_push) {
  int addr = GET_ADDR(to_push);
  int len = GET_LEN(to_push);
  uint8_t crc;
  uint8_t counter;

  // TODO: review this, lifted directly from SBieger. Does the ESP_05 checksum method really change?
  // TODO: consolidate into common

  switch(addr) {
    case MSG_LH_EPS_03:
      counter = volkswagen_mxb_get_counter(to_push);
      crc = 0xFFU;
      for (int i = 1; i < len; i++) {
        crc ^= (uint8_t)GET_BYTE(to_push, i);
        crc = volkswagen_crc8_lut_8h2f[crc];
      }
      crc ^= (uint8_t[]){0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5,0xF5}[counter];
      crc = volkswagen_crc8_lut_8h2f[crc];
      crc ^= 0xFFU;
      break;
    default: // MSG_ESP_03, MSG_ESP_05, MSG_TSK_02, MSG_MOTOR_03 and most likely others
      crc = 0x00U;
      for (int i = 1; i < len; i++) {
        crc ^= (uint8_t)GET_BYTE(to_push, i);
      }
      crc ^= (addr >> 8) & 0xFF;
      crc ^= addr & 0xFF;
      break;
  }

  return crc;
}

static const addr_checks* volkswagen_mlb_init(uint16_t param) {
  UNUSED(param);

  volkswagen_set_button_prev = false;
  volkswagen_resume_button_prev = false;
  volkswagen_brake_pedal_switch = false;
  volkswagen_brake_pressure_detected = false;

  gen_crc_lookup_table_8(0x2F, volkswagen_crc8_lut_8h2f);
  return &volkswagen_mlb_rx_checks;
}

static int volkswagen_mlb_rx_hook(CANPacket_t *to_push) {

  bool valid = addr_safety_check(to_push, &volkswagen_mlb_rx_checks,
                                 volkswagen_mxb_get_checksum, volkswagen_mlb_compute_crc, volkswagen_mxb_get_counter);

  if (valid && (GET_BUS(to_push) == 0U)) {
    int addr = GET_ADDR(to_push);

    // Update in-motion state by sampling wheel speeds
    // Signals: ESP_03.ESP_[VL,VR,HL,HR]_Radgeschw in scaled km/h
    if (addr == MSG_ESP_03) {
      int wheel_speed_fl = (GET_BYTE(to_push, 2) | (GET_BYTE(to_push, 3) << 8)) & 0xFFF;
      int wheel_speed_fr = ((GET_BYTE(to_push, 3) >> 4) | (GET_BYTE(to_push, 4) << 4)) & 0xFFF;
      int wheel_speed_rl = (GET_BYTE(to_push, 4) | (GET_BYTE(to_push, 5) << 8)) & 0xFFF;
      int wheel_speed_rr = ((GET_BYTE(to_push, 5) >> 4) | (GET_BYTE(to_push, 6) << 4)) & 0xFFF;
      vehicle_moving = (wheel_speed_fl + wheel_speed_fr + wheel_speed_rl + wheel_speed_rr) > 0;
    }

    // Update driver input torque samples
    // Signal: LH_EPS_03.EPS_Lenkmoment (absolute torque)
    // Signal: LH_EPS_03.EPS_VZ_Lenkmoment (direction)
    if (addr == MSG_LH_EPS_03) {
      int torque_driver_new = GET_BYTE(to_push, 5) | ((GET_BYTE(to_push, 6) & 0x1FU) << 8);
      int sign = (GET_BYTE(to_push, 6) & 0x80U) >> 7;
      if (sign == 1) {
        torque_driver_new *= -1;
      }
      update_sample(&torque_driver, torque_driver_new);
    }

    if (addr == MSG_TSK_02) {
      // When using stock ACC, enter controls on rising edge of stock ACC engage, exit on disengage
      // Always exit controls on main switch off
      // Signal: TSK_02.TSK_Status_GRA_ACC_01
      bool cruise_engaged = (GET_BYTE(to_push, 2) & 0x3) == 1;
      pcm_cruise_check(cruise_engaged);
    }

    if (addr == MSG_LS_01) {
      // Always exit controls on rising edge of Cancel
      // Signal: LS_01.LS_Abbrechen
      if (GET_BIT(to_push, 13U) == 1U) {
        controls_allowed = false;
      }
    }

    // Signal: Motor_03.MO_Fahrpedalrohwert_01
    if (addr == MSG_MOTOR_03) {
      gas_pressed = GET_BYTE(to_push, 6) != 0;
      volkswagen_brake_pedal_switch = GET_BIT(to_push, 35U);
    }

    // Signal: ESP_05.ESP_Fahrer_bremst (ESP detected driver brake pressure above platform specified threshold)
    if (addr == MSG_ESP_05) {
      volkswagen_brake_pressure_detected = (GET_BYTE(to_push, 3) & 0x4U) >> 2;
    }

    brake_pressed = volkswagen_brake_pedal_switch || volkswagen_brake_pressure_detected;

    generic_rx_checks((addr == MSG_HCA_01));
  }
  return valid;
}

static int volkswagen_mlb_tx_hook(CANPacket_t *to_send) {
  int addr = GET_ADDR(to_send);
  int tx = 1;

  tx = msg_allowed(to_send, VOLKSWAGEN_MLB_TX_MSGS, sizeof(VOLKSWAGEN_MLB_TX_MSGS) / sizeof(VOLKSWAGEN_MLB_TX_MSGS[0]));

  // Safety check for HCA_01 Heading Control Assist torque
  // Signal: HCA_01.Assist_Torque (absolute torque)
  // Signal: HCA_01.Assist_VZ (direction)
  if (addr == MSG_HCA_01) {
    int desired_torque = GET_BYTE(to_send, 2) | ((GET_BYTE(to_send, 3) & 0x3FU) << 8);
    int sign = (GET_BYTE(to_send, 3) & 0x80U) >> 7;
    if (sign == 1) {
      desired_torque *= -1;
    }

    if (steer_torque_cmd_checks(desired_torque, -1, VOLKSWAGEN_MXB_STEERING_LIMITS)) {
      tx = 0;
    }
  }

  // FORCE CANCEL: ensuring that only the cancel button press is sent when controls are off.
  // This avoids unintended engagements while still allowing resume spam
  if ((addr == MSG_LS_01) && !controls_allowed) {
    // disallow resume and set: bits 16 and 19
    if ((GET_BYTE(to_send, 2) & 0x9U) != 0U) {
      tx = 0;
    }
  }

  // 1 allows the message through
  return tx;
}

static int volkswagen_mlb_fwd_hook(int bus_num, CANPacket_t *to_fwd) {
  int addr = GET_ADDR(to_fwd);
  int bus_fwd = -1;

  switch (bus_num) {
    case 0:
      // Forward all traffic from the Extended CAN onward
      bus_fwd = 2;
      break;
    case 2:
      if (addr == MSG_HCA_01) {
        // openpilot takes over LKAS steering control and related HUD messages from the camera
        bus_fwd = -1;
      } else {
        // Forward all remaining traffic from Extended CAN devices to J533 gateway
        bus_fwd = 0;
      }
      break;
    default:
      // No other buses should be in use; fallback to do-not-forward
      bus_fwd = -1;
      break;
  }

  return bus_fwd;
}

const safety_hooks volkswagen_mlb_hooks = {
  .init = volkswagen_mlb_init,
  .rx = volkswagen_mlb_rx_hook,
  .tx = volkswagen_mlb_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = volkswagen_mlb_fwd_hook,
};
