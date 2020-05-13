#include "LampControl.h"

LampControl::LampControl(int control_pin, unsigned long timeout){
  pin = control_pin;
  default_timeout = timeout;
  current_level = 0;
}

void LampControl::apply_settings() {
  /**
  * Start lamp control
  * The lamp will start off in the active state for debugging and quick testing
  */
    pinMode(pin, OUTPUT);

    active_output = constrain(MAX_BRIGHTNESS * active_output_ratio, 0, MAX_BRIGHTNESS);
    inactive_output = constrain(MAX_BRIGHTNESS * inactive_output_ratio, 0, MAX_BRIGHTNESS);
    settings_applied = true;

    activate_lamp(-1, DEFAULT_SHORT_TIMEOUT);
}

void LampControl::off(){
    /**
    * Stop lamp control.
    * Stop any active timers and turn the lamp off.
    */
    deactivate_lamp(0);
}

void LampControl::activate_lamp(int target, long long timeout) {
    /**
    * Switch the lamp to its active state and start the timeout
    */

    if (!settings_applied){
        apply_settings();
    }

    if (!timeout_locked){
        active_start_time =  millis();

        // Deal with default timeouts
        if (timeout == -1) {
            timeout_period = default_timeout;
        }else{
            timeout_period = timeout;
        }
    }
    is_active = true;

    // Handle default target optioon
    if (target == -1){
        target = active_output;
    }

    set_lamp_target(target);
}

void LampControl::deactivate_lamp(int target){
    /**
    * Stop any active timers and set the lamp to its inactive state.
    */

    if (target == -1) {
        target = inactive_output;
    }
    is_active = false;
    timeout_locked = false;
    set_lamp_target(target);
}

void LampControl::set_lamp_target(int level) {
  target = constrain(level, 0, MAX_BRIGHTNESS);
  transition_lamp();
}

void LampControl::write_lamp_level(int level) {
  /**
  * Write the lamp level; instantaneous change
  * :level: Lamp level in 8-bit (0-255; 255 == maximum)
  */
  current_level = constrain(level, 0, MAX_BRIGHTNESS);
  analogWrite(pin, current_level);
}

void LampControl::transition_lamp() {
  byte new_output;

  // If the output is already close to the target, instantly change the output to match
  if ((target >= current_level && (target - current_level) <= lamp_transition_step_up) ||
      (target <= current_level && (current_level - target) <= lamp_transition_step_down)) {
    write_lamp_level(target);
  }

  // Else, a transition is needed; find out what the new output and period needs to be
  else {
    if (target > current_level) {
      new_output = current_level + lamp_transition_step_up;
    } else {
      new_output = current_level - lamp_transition_step_down;
    }

    write_lamp_level(new_output);
  }
}

void LampControl::tick(){
    unsigned long now = millis();
    // Deactivate the lamp if it has already been active for the timeout period
    if (timeout_enabled && is_active && (now - active_start_time > timeout_period)){
            deactivate_lamp();
    }

    // Transition to target if needed
    if (current_level != target){
        unsigned long time_since_transition = now - last_transition_time;
        if (((current_level > target) && (time_since_transition >= lamp_transistion_down_period)) ||
            ((current_level < target) && (time_since_transition >= lamp_transition_up_period))){
                transition_lamp();
                last_transition_time = now;
            }
    }
}
