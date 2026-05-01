#include "board/drivers/drivers.h"

struct fan_state_t fan_state;

static const uint8_t FAN_TICK_FREQ = 8U;

void fan_set_power(uint8_t percentage) {
  if (percentage > 0U) {
    fan_state.power = CLAMP(percentage, 20U, 100U);
  } else {
    fan_state.power = 0U;
  }
}

void fan_init(void) {
  fan_state.cooldown_counter = current_board->fan_enable_cooldown_time * FAN_TICK_FREQ;
  llfan_init();
}

// Call this at FAN_TICK_FREQ
void fan_tick(void) {
  if (current_board->has_fan) {
    // Measure fan RPM
    uint16_t fan_rpm_fast = fan_state.tach_counter * (60U * FAN_TICK_FREQ / 4U);   // 4 interrupts per rotation
    fan_state.tach_counter = 0U;
    fan_state.rpm = (fan_rpm_fast + (3U * fan_state.rpm)) / 4U;

    #ifdef DEBUG_FAN
      puth(fan_state.target_rpm);
      print(" "); puth(fan_rpm_fast);
      print(" "); puth(fan_state.power);
      print("\n");
    #endif

    // Cooldown counter to prevent noise on tachometer line.
    if (fan_state.power > 0U) {
      fan_state.cooldown_counter = current_board->fan_enable_cooldown_time * FAN_TICK_FREQ;
    } else {
      if (fan_state.cooldown_counter > 0U) {
        fan_state.cooldown_counter--;
      }
    }

    // Anti-stall: if fan is commanded on but no raw tach pulses for 1 second,
    // blip to 100% power and cycle the enable line to reset the fan controller
    // (Noctua NF-A4x10 on Comma 3X workaround).
    bool fan_stalled = false;
    uint8_t effective_power = fan_state.power;
    if (fan_state.power > 0U) {
      if (fan_rpm_fast == 0U) {
        fan_state.stall_counter = MIN(fan_state.stall_counter + 1U, 254U);
        if (fan_state.stall_counter > FAN_TICK_FREQ) {
          fan_stalled = true;
          fan_state.stall_counter = 0U;
          effective_power = 100U;
        }
      } else {
        fan_state.stall_counter = 0U;
      }
    } else {
      fan_state.stall_counter = 0U;
    }

    // Set PWM and enable line. Cycling enable off on stall resets the fan controller.
    pwm_set(TIM3, 3, effective_power);
    current_board->set_fan_enabled(((fan_state.power > 0U) || (fan_state.cooldown_counter > 0U)) && !fan_stalled);
  }
}
