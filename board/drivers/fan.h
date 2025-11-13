#include "fan_declarations.h"

struct fan_state_t fan_state;

static const uint8_t FAN_TICK_FREQ = 8U;

void fan_set_power(uint8_t percentage) {
  fan_state.target_rpm = ((current_board->fan_max_rpm * CLAMP(percentage, 0U, 100U)) / 100U);
}

void llfan_init(void);
void fan_init(void) {
  fan_state.cooldown_counter = current_board->fan_enable_cooldown_time * FAN_TICK_FREQ;
  llfan_init();
}

// Call this at FAN_TICK_FREQ
void fan_tick(void) {
  const float FAN_I = 6.5f;

  if (current_board->fan_max_rpm > 0U) {
    // Measure fan RPM
    uint16_t fan_rpm_fast = fan_state.tach_counter * (60U * FAN_TICK_FREQ / 4U);   // 4 interrupts per rotation
    fan_state.tach_counter = 0U;
    fan_state.rpm = (fan_rpm_fast + (3U * fan_state.rpm)) / 4U;

    bool fan_stalled = false;
    if (fan_state.target_rpm > 0U) {
      if (fan_rpm_fast == 0U) {
        fan_state.stall_counter = MIN(fan_state.stall_counter + 1U, 254U);
      } else {
        fan_state.stall_counter = 0U;
      }
      // Simple 1 second stall threshold
      if (fan_state.stall_counter > FAN_TICK_FREQ) {
        fan_stalled = true;
        fan_state.stall_counter = 0U; // Reset counter to try again
      }
    } else {
      fan_state.stall_counter = 0U;
    }

    #ifdef DEBUG_FAN
      puth(fan_state.target_rpm);
      print(" "); puth(fan_rpm_fast);
      print(" "); puth(fan_state.power);
      print("\n");
    #endif

    // Cooldown counter
    if (fan_state.target_rpm > 0U) {
      fan_state.cooldown_counter = current_board->fan_enable_cooldown_time * FAN_TICK_FREQ;
    } else {
      if (fan_state.cooldown_counter > 0U) {
        fan_state.cooldown_counter--;
      }
    }

    // Update controller
    if (fan_stalled) {
      // Noctua fan needs 100% power to unstall
      fan_state.error_integral = current_board->fan_max_pwm;
    } else if (fan_state.target_rpm == 0U) {
      fan_state.error_integral = 0.0f;
    } else {
      float error = (fan_state.target_rpm - fan_rpm_fast) / ((float) current_board->fan_max_rpm);
      fan_state.error_integral += FAN_I * error;
    }
    fan_state.error_integral = CLAMP(fan_state.error_integral, 0U, current_board->fan_max_pwm);
    fan_state.power = fan_state.error_integral;

    // Determine final enable state
    bool fan_should_be_on = (fan_state.target_rpm > 0U) || (fan_state.cooldown_counter > 0U);

    pwm_set(TIM3, 3, fan_state.power);
    // fan_stalled cycles the enable line
    current_board->set_fan_enabled(fan_should_be_on && !fan_stalled);
  }
}
