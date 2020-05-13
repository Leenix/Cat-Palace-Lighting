#ifndef LAMP_CONTROL_LIB_H
#define LAMP_CONTROL_LIB_H

#include "Arduino.h"

/**
* Lamp Control Library
*
* Author: Leenix
* Date: 2018-08-18
*
* A revamp of some LED control code.
*/


class LampControl {
   public:
    LampControl(int control_pin, unsigned long timeout = DEFAULT_TIMEOUT_PERIOD);

    /**
    * Initialise the lamp
    */
    void apply_settings();
    void off();
    void activate_lamp(int target = -1, long long timeout = -1);
    void deactivate_lamp(int target = -1);
    void tick();

    const byte MAX_BRIGHTNESS = 255;
    const float DEFAULT_ACTIVE_OUTPUT_RATIO = 1.0;
    const float DEFAULT_INACTIVE_OUTPUT_RATIO = 0.0;

    bool is_active;
    unsigned long long active_start_time;
    unsigned long long timeout_period;
    unsigned long long default_timeout;

    unsigned int lamp_transition_up_period = 2;
    unsigned int lamp_transistion_down_period = 20;
    unsigned int lamp_transition_step_up = 1;
    unsigned int lamp_transition_step_down = lamp_transition_step_up;

    float active_output_ratio = DEFAULT_ACTIVE_OUTPUT_RATIO;
    float inactive_output_ratio = DEFAULT_INACTIVE_OUTPUT_RATIO;

    bool timeout_locked = false;
    bool timeout_enabled = false;

   private:
       const static long DEFAULT_TIMEOUT_PERIOD = 5000;
       const static long DEFAULT_SHORT_TIMEOUT = 1000;

       void set_lamp_target(int level);
       void write_lamp_level(int level);
       void transition_lamp();

       int pin;
       byte target;
       byte current_level;
       unsigned long long last_transition_time;
       bool settings_applied = false;

       byte active_output = constrain(MAX_BRIGHTNESS * active_output_ratio, 0, MAX_BRIGHTNESS);
       byte inactive_output = constrain(MAX_BRIGHTNESS * inactive_output_ratio, 0, MAX_BRIGHTNESS);
};
#endif
