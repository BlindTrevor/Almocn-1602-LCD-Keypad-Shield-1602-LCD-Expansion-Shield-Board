// AlarmClock/AlarmClock.ino
// Alarm clock using the Almocn 1602 LCD Keypad Shield.
// Time is tracked via millis() — no RTC or WiFi required.
//
// Normal view (STATE_CLOCK):
//   Row 0:  12:34:56 [SET]
//   Row 1:  ALM 07:00  ON
//
// When alarm fires (STATE_ALARMING):
//   Row 0:  ** ALARM TIME **
//   Row 1:  SEL=off  RT=snz
//   Backlight flashes every 500 ms.
//   Pin 3 (BUZZER_PIN) toggles HIGH/LOW every 500 ms.
//   SELECT dismisses the alarm.
//   RIGHT snoozes for SNOOZE_MINUTES (shifts the alarm time forward).
//
// Settings menu (enter with SELECT from normal view):
//   RIGHT moves to the next page, LEFT moves back.
//   UP / DOWN changes the value on the current page.
//   SELECT saves the current page and returns to the clock.
//
//   Page 1 — Set Clock Hour   (< HH > UP/DN > )
//   Page 2 — Set Clock Minute (< MM > UP/DN > )
//   Page 3 — Set Alarm Hour   (< HH > UP/DN > )
//   Page 4 — Set Alarm Minute (< MM > UP/DN > )
//   Page 5 — Alarm On/Off     (ENABLED / DISABLED, UP/DN toggles)

#include <LiquidCrystal.h>

// ---------------------------------------------------------------------------
// Hardware pins
// ---------------------------------------------------------------------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN       = A0;
const int BACKLIGHT_PIN = 10;
const int BUZZER_PIN    = 3;   // buzzer between pin 3 and GND

// Set BACKLIGHT_ON_LEVEL to LOW if your shield uses an active-LOW backlight.
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = !BACKLIGHT_ON_LEVEL;

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------
const unsigned long DEBOUNCE_MS       = 40;   // button stability window (ms)
const unsigned long LOOP_DELAY_MS     = 80;   // display refresh interval (ms)
const unsigned long SPLASH_MS         = 1200; // startup splash duration (ms)
const unsigned long FLASH_INTERVAL_MS = 500;  // backlight flash half-period (ms)
const unsigned long BUZZ_INTERVAL_MS  = 500;  // buzzer toggle half-period (ms)
const int           SNOOZE_MINUTES    = 5;    // snooze duration in minutes

// ---------------------------------------------------------------------------
// Button enum (same thresholds as Demo.ino and Menu.ino)
// ---------------------------------------------------------------------------
enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

// ---------------------------------------------------------------------------
// Application state machine
// ---------------------------------------------------------------------------
enum AppState {
  STATE_CLOCK,         // normal clock view
  STATE_ALARMING,      // alarm is firing
  STATE_MENU_TIME_H,   // setting clock hour
  STATE_MENU_TIME_M,   // setting clock minute
  STATE_MENU_ALARM_H,  // setting alarm hour
  STATE_MENU_ALARM_M,  // setting alarm minute
  STATE_MENU_ALARM_EN  // enabling / disabling the alarm
};

// ---------------------------------------------------------------------------
// Runtime variables
// ---------------------------------------------------------------------------
int      clockH = 0, clockM = 0, clockS = 0; // current time
int      alarmH = 6, alarmM = 30;            // alarm time (default 06:30)
bool     alarmEnabled = true;
bool     alarmFired   = false;  // prevents re-trigger within the same minute
bool     backlightIsOn = true;
AppState appState = STATE_CLOCK;

// Temporary values used only while in the settings menu
int editClockH, editClockM;
int editAlarmH, editAlarmM;

// ---------------------------------------------------------------------------
// Button reading (identical mapping to Demo.ino / Menu.ino)
// ---------------------------------------------------------------------------
Button readButton(int adc) {
  if (adc < 50)   return BTN_RIGHT;
  if (adc < 180)  return BTN_UP;
  if (adc < 330)  return BTN_DOWN;
  if (adc < 525)  return BTN_LEFT;
  if (adc < 800)  return BTN_SELECT;
  return BTN_NONE;
}

// Returns the button that was just pressed (rising-edge only, debounced).
Button getPressEvent() {
  static Button        lastStable   = BTN_NONE;
  static unsigned long lastChangeMs = 0;

  const unsigned long now     = millis();
  Button              current = readButton(analogRead(KEY_PIN));

  if (current != lastStable) {
    if (now - lastChangeMs >= DEBOUNCE_MS) {
      lastStable   = current;
      lastChangeMs = now;
      if (lastStable != BTN_NONE) return lastStable;
    }
  } else {
    lastChangeMs = now;
  }
  return BTN_NONE;
}

// ---------------------------------------------------------------------------
// Backlight (identical to Demo.ino / Menu.ino)
// ---------------------------------------------------------------------------
void setBacklight(bool on) {
  digitalWrite(BACKLIGHT_PIN, on ? BACKLIGHT_ON_LEVEL : BACKLIGHT_OFF_LEVEL);
  backlightIsOn = on;
}

// ---------------------------------------------------------------------------
// Software clock — call once per loop() iteration
// ---------------------------------------------------------------------------
void tickClock() {
  static unsigned long lastTickMs = 0;
  const unsigned long  now        = millis();
  if (now - lastTickMs >= 1000UL) {
    lastTickMs = now;
    if (++clockS >= 60) {
      clockS = 0;
      if (++clockM >= 60) {
        clockM = 0;
        if (++clockH >= 24) clockH = 0;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

// Print a zero-padded two-digit integer.
void printTwoDigits(int val) {
  if (val < 10) lcd.print('0');
  lcd.print(val);
}

// Normal clock view.
// Row 0:  HH:MM:SS [SET]
// Row 1:  ALM HH:MM  ON   (or OFF)
void drawClock() {
  lcd.setCursor(0, 0);
  printTwoDigits(clockH); lcd.print(':');
  printTwoDigits(clockM); lcd.print(':');
  printTwoDigits(clockS);
  lcd.print(" [SET]  ");   // 8 chars — total row 0: 16

  lcd.setCursor(0, 1);
  lcd.print("ALM ");
  printTwoDigits(alarmH); lcd.print(':');
  printTwoDigits(alarmM);
  lcd.print(alarmEnabled ? "  ON   " : "  OFF  "); // 7 chars — total row 1: 16
}

// Alarm-firing view.
void drawAlarming() {
  lcd.setCursor(0, 0);
  lcd.print("** ALARM TIME **"); // 16 chars
  lcd.setCursor(0, 1);
  lcd.print("SEL=off  RT=snz "); // 16 chars
}

// Generic menu header (exactly 16 chars — caller must supply a padded string).
void drawMenuRow0(const char* label) {
  lcd.setCursor(0, 0);
  lcd.print(label);
}

// Generic menu value row.
// Format: "< HH > UP/DN >  " (16 chars)
void drawMenuRow1(int val) {
  lcd.setCursor(0, 1);
  lcd.print("< ");
  printTwoDigits(val);
  lcd.print(" > UP/DN >  "); // 12 chars — total row 1: 16
}

// Alarm enable/disable menu page.
void drawMenuAlarmEn() {
  lcd.setCursor(0, 0);
  lcd.print("Alarm:          "); // 16 chars
  lcd.setCursor(0, 1);
  lcd.print(alarmEnabled ? "ENABLED  UP/DN  " : "DISABLED UP/DN  "); // 16 chars
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  setBacklight(true);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("  Alarm Clock   "); // 16 chars
  lcd.setCursor(0, 1);
  lcd.print(" SELECT=menu    "); // 16 chars
  delay(SPLASH_MS);
  lcd.clear();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  const unsigned long now     = millis();
  Button              pressed = getPressEvent();

  // Always advance the software clock so it keeps time even while in menus.
  tickClock();

  // Trigger the alarm (only from the normal clock view).
  if (appState == STATE_CLOCK && alarmEnabled) {
    if (clockH == alarmH && clockM == alarmM && clockS == 0 && !alarmFired) {
      alarmFired = true;
      appState   = STATE_ALARMING;
      lcd.clear();
    }
  }

  // Allow the alarm to fire again once we move past the alarm minute.
  if (clockH != alarmH || clockM != alarmM) {
    alarmFired = false;
  }

  // ---- State machine -------------------------------------------------------
  switch (appState) {

    // ---- Normal clock view -------------------------------------------------
    case STATE_CLOCK:
      drawClock();
      if (pressed == BTN_SELECT) {
        editClockH = clockH;
        editClockM = clockM;
        appState   = STATE_MENU_TIME_H;
        lcd.clear();
      }
      break;

    // ---- Alarm firing ------------------------------------------------------
    case STATE_ALARMING: {
      // Flash backlight
      static unsigned long lastFlashMs = 0;
      if (now - lastFlashMs >= FLASH_INTERVAL_MS) {
        lastFlashMs = now;
        setBacklight(!backlightIsOn);
      }

      // Toggle buzzer (pin 3)
      static unsigned long lastBuzzMs = 0;
      static bool          buzzerOn   = false;
      if (now - lastBuzzMs >= BUZZ_INTERVAL_MS) {
        lastBuzzMs = now;
        buzzerOn   = !buzzerOn;
        digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
      }

      drawAlarming();

      if (pressed == BTN_SELECT) {
        // Dismiss alarm
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn = false;
        setBacklight(true);
        alarmFired = true;  // prevent re-trigger until next alarm minute
        appState   = STATE_CLOCK;
        lcd.clear();
      } else if (pressed == BTN_RIGHT) {
        // Snooze: shift alarm time forward by SNOOZE_MINUTES
        alarmM += SNOOZE_MINUTES;
        if (alarmM >= 60) { alarmM -= 60; alarmH = (alarmH + 1) % 24; }
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn   = false;
        setBacklight(true);
        alarmFired = false;  // let the new (snoozed) alarm time re-trigger
        appState   = STATE_CLOCK;
        lcd.clear();
      }
      break;
    }

    // ---- Set clock hour ----------------------------------------------------
    case STATE_MENU_TIME_H:
      switch (pressed) {
        case BTN_UP:     editClockH = (editClockH + 1) % 24;  break;
        case BTN_DOWN:   editClockH = (editClockH + 23) % 24; break;
        case BTN_RIGHT:
          appState = STATE_MENU_TIME_M;
          lcd.clear();
          break;
        case BTN_LEFT:
          // Cancel — return to clock without saving
          appState = STATE_CLOCK;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Save and return to clock
          clockH   = editClockH;
          clockM   = editClockM;
          clockS   = 0;
          appState = STATE_CLOCK;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Set Clock Hour: "); // 16 chars
      drawMenuRow1(editClockH);
      break;

    // ---- Set clock minute --------------------------------------------------
    case STATE_MENU_TIME_M:
      switch (pressed) {
        case BTN_UP:    editClockM = (editClockM + 1) % 60;  break;
        case BTN_DOWN:  editClockM = (editClockM + 59) % 60; break;
        case BTN_RIGHT:
          // Commit clock time, load alarm values, move to alarm hour page
          clockH     = editClockH;
          clockM     = editClockM;
          clockS     = 0;
          editAlarmH = alarmH;
          editAlarmM = alarmM;
          appState   = STATE_MENU_ALARM_H;
          lcd.clear();
          break;
        case BTN_LEFT:
          appState = STATE_MENU_TIME_H;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Save and return to clock
          clockH   = editClockH;
          clockM   = editClockM;
          clockS   = 0;
          appState = STATE_CLOCK;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Set Clock Min:  "); // 16 chars
      drawMenuRow1(editClockM);
      break;

    // ---- Set alarm hour ----------------------------------------------------
    case STATE_MENU_ALARM_H:
      switch (pressed) {
        case BTN_UP:    editAlarmH = (editAlarmH + 1) % 24;  break;
        case BTN_DOWN:  editAlarmH = (editAlarmH + 23) % 24; break;
        case BTN_RIGHT:
          appState = STATE_MENU_ALARM_M;
          lcd.clear();
          break;
        case BTN_LEFT:
          // Go back to clock-minute page, reload the committed clock values
          editClockH = clockH;
          editClockM = clockM;
          appState   = STATE_MENU_TIME_M;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Save and return to clock
          alarmH   = editAlarmH;
          alarmM   = editAlarmM;
          appState = STATE_CLOCK;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Set Alarm Hour: "); // 16 chars
      drawMenuRow1(editAlarmH);
      break;

    // ---- Set alarm minute --------------------------------------------------
    case STATE_MENU_ALARM_M:
      switch (pressed) {
        case BTN_UP:    editAlarmM = (editAlarmM + 1) % 60;  break;
        case BTN_DOWN:  editAlarmM = (editAlarmM + 59) % 60; break;
        case BTN_RIGHT:
          // Commit alarm time, move to enable/disable page
          alarmH   = editAlarmH;
          alarmM   = editAlarmM;
          appState = STATE_MENU_ALARM_EN;
          lcd.clear();
          break;
        case BTN_LEFT:
          appState = STATE_MENU_ALARM_H;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Save and return to clock
          alarmH   = editAlarmH;
          alarmM   = editAlarmM;
          appState = STATE_CLOCK;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Set Alarm Min:  "); // 16 chars
      drawMenuRow1(editAlarmM);
      break;

    // ---- Alarm enable / disable --------------------------------------------
    case STATE_MENU_ALARM_EN:
      switch (pressed) {
        case BTN_UP:
        case BTN_DOWN:
          alarmEnabled = !alarmEnabled;
          break;
        case BTN_LEFT:
          // Go back to alarm-minute page
          editAlarmH = alarmH;
          editAlarmM = alarmM;
          appState   = STATE_MENU_ALARM_M;
          lcd.clear();
          break;
        case BTN_RIGHT:
        case BTN_SELECT:
          appState = STATE_CLOCK;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuAlarmEn();
      break;
  }

  delay(LOOP_DELAY_MS);
}
