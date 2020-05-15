////////////////////////////////////////////////////////////////////
// Cat Palace Lighting

///////////////////////////////////////////////////////////////////////////////
// Library imports

#include <Arduino.h>
#include <ArduinoLog.h>
#include <LampControl.h>
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <Wire.h>

#include "Ticker.h"

///////////////////////////////////////////////////////////////////////////////

bool get_time_condition();
bool get_ambient_light_condition();
void check_lamp_state();
// void update_lcd();
float get_current();
void check_serial();

///////////////////////////////////////////////////////////////////////////////

// Logging/Comms
const long SERIAL_BAUD = 57600;
const int LOGGER_LEVEL = LOG_LEVEL_VERBOSE;

// RTC
//* Wiring; VCC to LIGHT + (3V3), GND to LIGHT -, SDA to LIGHT ~ (A4), SCL to LIGHT C (A5)
RTC_DS3231 rtc;
const int ON_HOUR = 5;
const int ON_MINUTE = 30;
const int OFF_HOUR = 22;
const int OFF_MINUTE = 30;

bool lights_enabled;
long time_of_last_check;
const long TIME_BETWEEN_CHECKS = 1000;  //! ms CHANGE BACK TO 10000 AFTER TESTING

// LDR
const int LDR_PIN = A0;  //* Fork 1 ~
const int LDR_LOWER_THRESHOLD = 10;
const int LDR_UPPER_THRESHOLD = 30;
Ticker lamp_state_task(check_lamp_state, TIME_BETWEEN_CHECKS, 0, MILLIS);

/**
// Display
const int LCD_ENABLE_PIN = A2;
const int LCD_RS_PIN = A3;
const int LCD_D4 = 2;
const int LCD_D5 = 3;
const int LCD_D6 = 4;
const int LCD_D7 = A6; // Can go on D5
const int LCD_BACKLIGHT = 11;
LiquidCrystal lcd(LCD_RS_PIN, LCD_ENABLE_PIN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Ticker update_lcd_timer(update_lcd, TIME_BETWEEN_CHECKS, 0, MILLIS);

const int LCD_WIDTH = 16;
const int LCD_HEIGHT = 2;

*/

// Switches
const int LAMP_PIN = 13;  //! Keep this change from 10 to 13 so can see board led on when lamp on. Lamp switch is Q2
LampControl light(LAMP_PIN);
const int PUMP_PIN = 12;  //* Q1 gate

// Current meter
const int CURRENT_PIN = A1;  //* Fork 2 ~
const float CURRENT_SCALAR = 0.2;

///////////////////////////////////////////////////////////////////////////////
// Main Functions

void setup() {
    Serial.begin(SERIAL_BAUD);
    Log.begin(LOGGER_LEVEL, &Serial);
    Log.notice(F("Working...\n"));

    rtc.begin();
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(__DATE__, __TIME__));
        Log.warning(F("RTC lost power; setting time to compile time..."));
    }
    DateTime now = rtc.now();
    time_of_last_check = millis();

    char current_time[60];
    sprintf(current_time, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(),
            now.second());
    Log.notice(F("Time is \t%s\n"), current_time);

    char on_time[20];
    sprintf(on_time, "%02d:%02d:00", ON_HOUR, ON_MINUTE);
    char off_time[20];
    sprintf(off_time, "%02d:%02d:00", OFF_HOUR, OFF_MINUTE);
    Log.notice(F("On time: \t%s\n"), on_time);
    Log.notice(F("Off time: \t%s\n"), off_time);
    Log.notice(F("Ambient light thresholds: \tlow - %d\thigh - %d\n\n"), LDR_LOWER_THRESHOLD, LDR_UPPER_THRESHOLD);

    pinMode(LDR_PIN, INPUT);
    lamp_state_task.start();

    //! Can we add some lines that we can have // out then run when need to reset rtc to PC time

    // lcd.begin(LCD_WIDTH, LCD_HEIGHT);
    // update_lcd_timer.start();

    lights_enabled = false;
    light.deactivate_lamp();
}

void loop() {
    light.tick();
    lamp_state_task.update();
    check_serial();
}

///////////////////////////////////////////////////////////////////////////////
// Functions

/**
 * Check if the light is meant to be on, based on the time condition.
 * @return True if the light is meant to be active in the current time.
 */
bool get_time_condition() {
    DateTime now = rtc.now();

    bool is_correct_time = false;
    if (now.hour() > ON_HOUR or (now.hour() == ON_HOUR and now.minute() >= ON_MINUTE)) {
        if (now.hour() < OFF_HOUR or (now.hour() == OFF_HOUR and now.minute() < OFF_MINUTE)) {
            is_correct_time = true;
        }
    }

    char buffer[40];
    sprintf(buffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    Log.verbose(F("Time check:\t%s - %T\n"), buffer, is_correct_time);
    return is_correct_time;
}

/**
 * Check if the light is meant to be on, based on ambient light level.
 *
 * The ambient light threshold changes, depending if the lamp is active or not.
 * This adds a hysteresis to lamp transitions so the lamp doesn't flicker and do dumb shit.
 *
 * @return True if the light should be on, based on ambient light level.
 */
bool get_ambient_light_condition() {
    bool is_correct_light_level = false;

    int ldr_level = analogRead(LDR_PIN);

    // When the lights are on, switch off when the sky gets too bright
    if (lights_enabled) {
        if (ldr_level < LDR_UPPER_THRESHOLD) {
            is_correct_light_level = true;
        }
    }

    // When the lights are off, switch on when the sky gets too dark
    else {
        if (ldr_level < LDR_LOWER_THRESHOLD) {
            is_correct_light_level = true;
        }
    }

    Log.verbose(F("LDR level: %d - %T\n"), ldr_level, is_correct_light_level);
    return is_correct_light_level;
}

void check_lamp_state() {
    bool new_lamp_state = get_time_condition();
    new_lamp_state &= get_ambient_light_condition();

    if (lights_enabled != new_lamp_state) {
        if (new_lamp_state) {
            light.activate_lamp();
            Log.notice(F("Lamp activated\n"));
        } else {
            light.deactivate_lamp();
            Log.notice(F("Lamp deactivated\n"));
        }
        lights_enabled = new_lamp_state;
    }

    Log.verbose(F("Lamp state: %T\n\n"), lights_enabled);
}

void check_serial() {
    if (Serial.available()) {
        char command = Serial.read();
        switch (command) {
            case 'H':
                rtc.adjust(rtc.now() + TimeSpan(0, 1, 0, 0));
                break;
            case 'h':
                rtc.adjust(rtc.now() + TimeSpan(0, -1, 0, 0));
                break;
            case 'M':
                rtc.adjust(rtc.now() + TimeSpan(0, 0, 1, 0));
                break;
            case 'm':
                rtc.adjust(rtc.now() + TimeSpan(0, 0, -1, 0));
                break;
            case 'S':
                rtc.adjust(rtc.now() + TimeSpan(0, 0, 0, 1));
                break;
            case 's':
                rtc.adjust(rtc.now() + TimeSpan(0, 0, 0, -1));
                break;
        }
    }
}

/**
void update_lcd(){
  // TIME-HHMM C.C A
  // ON-HHMM OFF-HHMM

  lcd.setCursor(0, 0);

  // Current time
  DateTime now = rtc.now();
  lcd.write("TIME-");
  if (now.hour() < 10) {
    lcd.write('0');
  }
  lcd.write(now.hour());
  lcd.write(':');
  if (now.minute() < 10) {
    lcd.write('0');
  }
  lcd.write(now.minute());
  lcd.write(' ');

  // Current
  float current = get_current();
  lcd.write(int(current));
  lcd.write('.');
  lcd.write(int((current - int(current)) * 10));
  lcd.write(" A");

  lcd.setCursor(0, 1);

  // ON time
  lcd.write("ON-");
  if (ON_HOUR < 10) {
    lcd.write('0');
  }
  lcd.write(ON_HOUR);
  if (ON_MINUTE < 10) {
    lcd.write('0');
  }
  lcd.write(ON_MINUTE);

  // OFF time
  lcd.write("OFF-");
  if (OFF_HOUR < 10) {
    lcd.write('0');
  }
  lcd.write(OFF_HOUR);
  if (OFF_MINUTE < 10) {
    lcd.write('0');
  }
  lcd.write(OFF_MINUTE);

}



float get_current(){
  int read = analogRead(CURRENT_PIN);
  float current = read/float(1024) * CURRENT_SCALAR;
  return current;
  Log.verbose(F("Current: %d\n"), current);
}

*/
