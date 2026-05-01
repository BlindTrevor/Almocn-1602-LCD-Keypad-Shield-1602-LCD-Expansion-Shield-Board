// PomodoroTimer/PomodoroTimer.ino
// Pomodoro productivity timer using the Almocn 1602 LCD Keypad Shield.
// Tracks work sessions, short breaks, and long breaks with adjustable durations.
//
// Normal view (STATE_IDLE / STATE_RUNNING / STATE_PAUSED):
//   Row 0:  WORK    25:00   (period label + MM:SS countdown)
//   Row 1:  #:00 SEL=start  (pomodoro count + control hint)
//
// Alert view (STATE_ALERT) — period just ended:
//   Row 0:  *  WORK DONE!  *   or   * BREAK DONE!  *
//   Row 1:  SEL=next RT=rst
//   Backlight flashes every 500 ms.
//   Buzzer behaviour depends on the Buzzer Mode setting:
//     OFF  — buzzer silent.
//     SYNC — pin 3 toggles in time with the backlight flash (every 500 ms).
//     BEEP — two short beeps every 2 s (watch-alarm style).
//   SELECT advances to the next period (work → break, break → work).
//   RIGHT resets everything to the start.
//
// Settings menu (enter with LEFT from IDLE):
//   RIGHT moves to the next page, LEFT moves back.
//   UP / DOWN changes the value on the current page.
//   Hold UP / DOWN to change the value faster (accelerates after 2 s).
//   SELECT saves all pages and returns to IDLE.
//
//   Page 1 — Work duration    (1 – 60 min, default 25)
//   Page 2 — Short break      (1 – 30 min, default 5)
//   Page 3 — Long break       (1 – 60 min, default 15)
//   Page 4 — Buzzer mode      (OFF / SYNC / BEEP, default SYNC)
//
// Button mapping (normal view):
//   SELECT — start / pause / resume the countdown
//   RIGHT  — reset to the beginning (clears the pomodoro count)
//   LEFT   — enter the settings menu (only from IDLE)

#include <EEPROM.h>
#include <LiquidCrystal.h>

// ---------------------------------------------------------------------------
// Hardware pins (same wiring as Demo.ino / Menu.ino / AlarmClock.ino)
// ---------------------------------------------------------------------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN       = A0;
const int BACKLIGHT_PIN = 10;
const int BUZZER_PIN    = 3;   // passive piezo between D3 and GND

// Set BACKLIGHT_ON_LEVEL to LOW if your shield uses an active-LOW backlight.
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = !BACKLIGHT_ON_LEVEL;

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------
const unsigned long DEBOUNCE_MS       = 40;   // button stability window (ms)
const unsigned long LOOP_DELAY_MS     = 80;   // display refresh interval (ms)
const unsigned long SPLASH_MS         = 1200; // startup splash duration (ms)
const unsigned long FLASH_INTERVAL_MS  = 500;  // backlight flash half-period (ms)
const unsigned long BUZZ_INTERVAL_MS   = 500;  // SYNC mode: buzzer toggle half-period (ms)

// BEEP mode timing — two short beeps repeated every BEEP_CYCLE_MS.
const unsigned long BEEP_ON_MS    = 120;  // duration of each beep (ms)
const unsigned long BEEP_GAP_MS   = 120;  // gap between the two beeps (ms)
const unsigned long BEEP_CYCLE_MS = 2000; // full beep-beep repetition period (ms)

// Hold-to-repeat for UP / DOWN in the settings menu.
const unsigned long HOLD_DELAY_MS      = 500;  // pause before repeat begins (ms)
const unsigned long HOLD_REPEAT_MS     = 200;  // slow repeat interval (ms)
const unsigned long HOLD_FAST_MS       = 80;   // fast repeat interval (ms)
const unsigned long HOLD_FAST_AFTER_MS = 2000; // ms of holding before switching to fast rate

// ---------------------------------------------------------------------------
// Buzzer mode
// ---------------------------------------------------------------------------
enum BuzzerMode {
  BUZZ_OFF   = 0,  // buzzer silent
  BUZZ_FLASH = 1,  // toggle in sync with the backlight flash
  BUZZ_BEEP  = 2   // watch-alarm style: two short beeps every BEEP_CYCLE_MS
};
const int      BUZZ_MODE_COUNT  = 3;             // total number of BuzzerMode values
const BuzzerMode DEFAULT_BUZZ_MODE = BUZZ_FLASH;

// ---------------------------------------------------------------------------
// Pomodoro defaults and limits
// ---------------------------------------------------------------------------
const int DEFAULT_WORK_MIN  = 25;
const int DEFAULT_SHORT_MIN = 5;
const int DEFAULT_LONG_MIN  = 15;
const int MIN_DURATION      = 1;
const int MAX_WORK_MIN      = 60;
const int MAX_SHORT_MIN     = 30;
const int MAX_LONG_MIN      = 60;
const int POMODOROS_PER_LONG = 4;  // long break after every Nth completed work session

// ---------------------------------------------------------------------------
// EEPROM layout
// A single magic byte at address 0 guards against reading uninitialised EEPROM.
// Addresses 1-4 store the four adjustable settings as single bytes.
// ---------------------------------------------------------------------------
const uint8_t EEPROM_MAGIC       = 0xA7; // change this value to force a reset to defaults
const int     EEPROM_ADDR_MAGIC  = 0;
const int     EEPROM_ADDR_WORK   = 1;
const int     EEPROM_ADDR_SHORT  = 2;
const int     EEPROM_ADDR_LONG   = 3;
const int     EEPROM_ADDR_BUZZ   = 4;   // BuzzerMode (0=OFF, 1=FLASH, 2=BEEP)

// Load workMin / shortMin / longMin / buzzerMode from EEPROM.
// Falls back to compile-time defaults when the magic byte is absent or wrong.
void loadSettings() {
  if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC) return; // use defaults

  const uint8_t w = EEPROM.read(EEPROM_ADDR_WORK);
  const uint8_t s = EEPROM.read(EEPROM_ADDR_SHORT);
  const uint8_t l = EEPROM.read(EEPROM_ADDR_LONG);
  const uint8_t b = EEPROM.read(EEPROM_ADDR_BUZZ);

  // Validate ranges — corrupt data reverts to defaults for that field.
  workMin    = (w >= MIN_DURATION && w <= MAX_WORK_MIN)  ? w : DEFAULT_WORK_MIN;
  shortMin   = (s >= MIN_DURATION && s <= MAX_SHORT_MIN) ? s : DEFAULT_SHORT_MIN;
  longMin    = (l >= MIN_DURATION && l <= MAX_LONG_MIN)  ? l : DEFAULT_LONG_MIN;
  buzzerMode = (b < (uint8_t)BUZZ_MODE_COUNT) ? (BuzzerMode)b : DEFAULT_BUZZ_MODE;
}

// Persist workMin / shortMin / longMin / buzzerMode to EEPROM.
// EEPROM.update() only writes a byte when its value has changed,
// reducing wear on cells rated for ~100 000 write cycles.
void saveSettings() {
  EEPROM.update(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);
  EEPROM.update(EEPROM_ADDR_WORK,  (uint8_t)workMin);
  EEPROM.update(EEPROM_ADDR_SHORT, (uint8_t)shortMin);
  EEPROM.update(EEPROM_ADDR_LONG,  (uint8_t)longMin);
  EEPROM.update(EEPROM_ADDR_BUZZ,  (uint8_t)buzzerMode);
}

// ---------------------------------------------------------------------------
// Button enum (same ADC thresholds as all other sketches)
// ---------------------------------------------------------------------------
enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

// ---------------------------------------------------------------------------
// Period type
// ---------------------------------------------------------------------------
enum PeriodType { PERIOD_WORK, PERIOD_SHORT_BREAK, PERIOD_LONG_BREAK };

// ---------------------------------------------------------------------------
// Application state machine
// ---------------------------------------------------------------------------
enum AppState {
  STATE_IDLE,        // timer ready but not running
  STATE_RUNNING,     // timer counting down
  STATE_PAUSED,      // timer paused mid-period
  STATE_ALERT,       // period just ended — flashing / beeping
  STATE_MENU_WORK,   // settings: adjust work duration
  STATE_MENU_SHORT,  // settings: adjust short-break duration
  STATE_MENU_LONG,   // settings: adjust long-break duration
  STATE_MENU_BUZZ    // settings: choose buzzer mode
};

// ---------------------------------------------------------------------------
// Runtime variables
// ---------------------------------------------------------------------------
int        workMin       = DEFAULT_WORK_MIN;
int        shortMin      = DEFAULT_SHORT_MIN;
int        longMin       = DEFAULT_LONG_MIN;
BuzzerMode buzzerMode    = DEFAULT_BUZZ_MODE;
int        pomodoroCount = 0;
PeriodType currentPeriod = PERIOD_WORK;
AppState   appState      = STATE_IDLE;
bool       backlightIsOn = true;
long       secsRemaining = (long)DEFAULT_WORK_MIN * 60;

// Tracks the last millis() tick so we can pause the countdown accurately.
unsigned long timerLastTickMs = 0;

// Temporary values used only while inside the settings menu.
int        editWork, editShort, editLong;
BuzzerMode editBuzz;

// ---------------------------------------------------------------------------
// Button reading (identical mapping to Demo.ino / Menu.ino / AlarmClock.ino)
// ---------------------------------------------------------------------------
Button readButton(int adc) {
  if (adc < 50)  return BTN_RIGHT;
  if (adc < 180) return BTN_UP;
  if (adc < 330) return BTN_DOWN;
  if (adc < 525) return BTN_LEFT;
  if (adc < 800) return BTN_SELECT;
  return BTN_NONE;
}

// Returns the button event for this loop iteration.
// For UP / DOWN: fires on first press and then repeats at an accelerating rate while held.
//   Rate schedule: HOLD_DELAY_MS initial pause → HOLD_REPEAT_MS slow phase
//                  → HOLD_FAST_MS fast phase (after HOLD_FAST_AFTER_MS total hold time).
// For all other buttons: fires once per press (rising-edge only, debounced).
Button getButtonEvent() {
  static Button        lastStable   = BTN_NONE;
  static unsigned long lastChangeMs = 0;
  static unsigned long heldSinceMs  = 0;
  static unsigned long lastRepeatMs = 0;
  static bool          holdActive   = false;

  const unsigned long now     = millis();
  Button              current = readButton(analogRead(KEY_PIN));

  if (current != lastStable) {
    if (now - lastChangeMs >= DEBOUNCE_MS) {
      lastStable   = current;
      lastChangeMs = now;
      holdActive   = false;
      if (lastStable != BTN_NONE) {
        if (lastStable == BTN_UP || lastStable == BTN_DOWN) {
          heldSinceMs  = now;
          lastRepeatMs = now;
          holdActive   = true;
        }
        return lastStable; // fire immediately on first press
      }
    }
  } else {
    lastChangeMs = now;
    if (holdActive) {
      const unsigned long heldMs   = now - heldSinceMs;
      if (heldMs >= HOLD_DELAY_MS) {
        const unsigned long interval = (heldMs >= HOLD_FAST_AFTER_MS)
                                       ? HOLD_FAST_MS : HOLD_REPEAT_MS;
        if (now - lastRepeatMs >= interval) {
          lastRepeatMs = now;
          return lastStable; // repeated fire while held
        }
      }
    }
  }
  return BTN_NONE;
}

// ---------------------------------------------------------------------------
// Backlight (identical to Demo.ino / Menu.ino / AlarmClock.ino)
// ---------------------------------------------------------------------------
void setBacklight(bool on) {
  digitalWrite(BACKLIGHT_PIN, on ? BACKLIGHT_ON_LEVEL : BACKLIGHT_OFF_LEVEL);
  backlightIsOn = on;
}

// ---------------------------------------------------------------------------
// Countdown tick — call every loop() iteration only while STATE_RUNNING
// ---------------------------------------------------------------------------
void tickTimer() {
  const unsigned long now = millis();
  if (now - timerLastTickMs >= 1000UL) {
    timerLastTickMs = now;
    if (secsRemaining > 0) secsRemaining--;
  }
}

// Load secsRemaining for the current period.
void loadPeriodTime() {
  switch (currentPeriod) {
    case PERIOD_WORK:        secsRemaining = (long)workMin  * 60; break;
    case PERIOD_SHORT_BREAK: secsRemaining = (long)shortMin * 60; break;
    case PERIOD_LONG_BREAK:  secsRemaining = (long)longMin  * 60; break;
  }
}

// Advance to the next period (called when the user acknowledges an alert).
// Increments the pomodoro count when a work session completes.
void advancePeriod() {
  if (currentPeriod == PERIOD_WORK) {
    pomodoroCount++;
    currentPeriod = (pomodoroCount % POMODOROS_PER_LONG == 0)
                    ? PERIOD_LONG_BREAK
                    : PERIOD_SHORT_BREAK;
  } else {
    currentPeriod = PERIOD_WORK;
  }
  loadPeriodTime();
}

// Full reset: clear the pomodoro count and return to the start of a work period.
void resetTimer() {
  pomodoroCount = 0;
  currentPeriod = PERIOD_WORK;
  loadPeriodTime();
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

// Print a zero-padded two-digit integer.
void printTwoDigits(int val) {
  if (val < 10) lcd.print('0');
  lcd.print(val);
}

// 6-character period label (space-padded to keep row 0 stable).
const char* periodLabel() {
  switch (currentPeriod) {
    case PERIOD_WORK:        return "WORK  ";
    case PERIOD_SHORT_BREAK: return "BREAK ";
    case PERIOD_LONG_BREAK:  return "L.BRK ";
  }
  return "      ";
}

// Normal timer view (IDLE / RUNNING / PAUSED).
// Row 0: "WORK    25:00   "  (6 + 2 + 2 + 1 + 2 + 3 = 16 chars)
// Row 1: "#:00 SEL=start  "  (2 + 2 + 1 + 11 = 16 chars)
void drawTimer() {
  const int mins = (int)(secsRemaining / 60);
  const int secs = (int)(secsRemaining % 60);

  lcd.setCursor(0, 0);
  lcd.print(periodLabel());   // 6 chars
  lcd.print("  ");            // 2 chars
  printTwoDigits(mins);       // 2 chars
  lcd.print(':');             // 1 char
  printTwoDigits(secs);       // 2 chars
  lcd.print("   ");           // 3 chars  — row 0 total: 16

  lcd.setCursor(0, 1);
  lcd.print("#:");                          // 2 chars
  printTwoDigits(pomodoroCount % 100);      // 2 chars (wraps at 99 for display)
  lcd.print(' ');                           // 1 char

  switch (appState) {
    case STATE_IDLE:    lcd.print("SEL=start  "); break; // 11 chars — row 1 total: 16
    case STATE_RUNNING: lcd.print("SEL=pause  "); break; // 11 chars
    case STATE_PAUSED:  lcd.print("SEL=resume "); break; // 11 chars
    default:            lcd.print("           "); break; // 11 chars
  }
}

// Alert view — period just ended.
// Row 0: "*  WORK DONE!  *"  or  "* BREAK DONE!  *"  (16 chars each)
// Row 1: "SEL=next RT=rst "  (16 chars)
void drawAlert() {
  lcd.setCursor(0, 0);
  if (currentPeriod == PERIOD_WORK) {
    lcd.print("*  WORK DONE!  *"); // 16 chars
  } else {
    lcd.print("* BREAK DONE!  *"); // 16 chars
  }
  lcd.setCursor(0, 1);
  lcd.print("SEL=next RT=rst "); // 16 chars
}

// Generic settings menu header — caller must supply an exactly 16-character string.
void drawMenuRow0(const char* label) {
  lcd.setCursor(0, 0);
  lcd.print(label);
}

// Settings menu value row for duration pages.
// Format: "< NN min UP/DN  "  (2 + 2 + 12 = 16 chars)
void drawMenuRow1(int val) {
  lcd.setCursor(0, 1);
  lcd.print("< ");           // 2 chars
  printTwoDigits(val);       // 2 chars
  lcd.print(" min UP/DN  "); // 12 chars  — row 1 total: 16
}

// Settings menu value row for the buzzer-mode page.
// Format: "< OFF    UP/DN  "  (2 + 7 + 7 = 16 chars)
void drawMenuRowBuzz(BuzzerMode mode) {
  lcd.setCursor(0, 1);
  lcd.print("< ");
  switch (mode) {
    case BUZZ_OFF:   lcd.print("OFF    "); break; // 7 chars
    case BUZZ_FLASH: lcd.print("SYNC   "); break; // 7 chars
    case BUZZ_BEEP:  lcd.print("BEEP   "); break; // 7 chars
  }
  lcd.print("UP/DN  ");                           // 7 chars — row 1 total: 16
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
  loadSettings();   // restore durations from EEPROM (falls back to defaults on first boot)
  loadPeriodTime(); // apply the loaded work duration to secsRemaining
  lcd.clear();
  lcd.print("Pomodoro Timer  "); // 16 chars
  lcd.setCursor(0, 1);
  lcd.print("SEL=go  LT=set  "); // 16 chars
  delay(SPLASH_MS);
  lcd.clear();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  const unsigned long now     = millis();
  Button              pressed = getButtonEvent();

  // Advance the countdown only while the timer is running.
  if (appState == STATE_RUNNING) {
    tickTimer();
    if (secsRemaining == 0) {
      // Period just finished — enter alert.
      appState = STATE_ALERT;
      lcd.clear();
    }
  }

  // ---- State machine -------------------------------------------------------
  switch (appState) {

    // ---- IDLE ---------------------------------------------------------------
    case STATE_IDLE:
      drawTimer();
      switch (pressed) {
        case BTN_SELECT:
          timerLastTickMs = millis(); // first tick 1 s from now, not immediately
          appState = STATE_RUNNING;
          break;
        case BTN_RIGHT:
          resetTimer();
          lcd.clear();
          break;
        case BTN_LEFT:
          editWork  = workMin;
          editShort = shortMin;
          editLong  = longMin;
          editBuzz  = buzzerMode;
          appState  = STATE_MENU_WORK;
          lcd.clear();
          break;
        default: break;
      }
      break;

    // ---- RUNNING ------------------------------------------------------------
    case STATE_RUNNING:
      drawTimer();
      switch (pressed) {
        case BTN_SELECT:
          appState = STATE_PAUSED;
          break;
        case BTN_RIGHT:
          resetTimer();
          appState = STATE_IDLE;
          lcd.clear();
          break;
        default: break;
      }
      break;

    // ---- PAUSED -------------------------------------------------------------
    case STATE_PAUSED:
      drawTimer();
      switch (pressed) {
        case BTN_SELECT:
          timerLastTickMs = millis(); // first tick 1 s from resume, not immediately
          appState = STATE_RUNNING;
          break;
        case BTN_RIGHT:
          resetTimer();
          appState = STATE_IDLE;
          lcd.clear();
          break;
        default: break;
      }
      break;

    // ---- ALERT --------------------------------------------------------------
    case STATE_ALERT: {
      // Flash backlight
      static unsigned long lastFlashMs = 0;
      if (now - lastFlashMs >= FLASH_INTERVAL_MS) {
        lastFlashMs = now;
        setBacklight(!backlightIsOn);
      }

      // Drive buzzer according to buzzerMode
      static unsigned long lastBuzzMs      = 0;
      static unsigned long lastBuzzCycleMs = 0;
      static bool          buzzerOn        = false;
      if (buzzerMode == BUZZ_FLASH) {
        if (now - lastBuzzMs >= BUZZ_INTERVAL_MS) {
          lastBuzzMs = now;
          buzzerOn   = !buzzerOn;
          digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
        }
      } else if (buzzerMode == BUZZ_BEEP) {
        unsigned long elapsed = now - lastBuzzCycleMs;
        if (elapsed >= BEEP_CYCLE_MS) {
          lastBuzzCycleMs = now;
          elapsed = 0;
        }
        const bool wantOn = (elapsed < BEEP_ON_MS) ||
                            (elapsed >= BEEP_ON_MS + BEEP_GAP_MS &&
                             elapsed <  2UL * BEEP_ON_MS + BEEP_GAP_MS);
        if (wantOn != buzzerOn) {
          buzzerOn = wantOn;
          digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
        }
      }
      // BUZZ_OFF: buzzer stays silent

      drawAlert();

      if (pressed == BTN_SELECT) {
        // Advance to the next period and return to IDLE.
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn = false;
        setBacklight(true);
        advancePeriod();
        appState = STATE_IDLE;
        lcd.clear();
      } else if (pressed == BTN_RIGHT) {
        // Full reset — clear count and return to a fresh work session.
        digitalWrite(BUZZER_PIN, LOW);
        buzzerOn = false;
        setBacklight(true);
        resetTimer();
        appState = STATE_IDLE;
        lcd.clear();
      }
      break;
    }

    // ---- MENU: Work duration ------------------------------------------------
    case STATE_MENU_WORK:
      switch (pressed) {
        case BTN_UP:   if (editWork < MAX_WORK_MIN) editWork++;  break;
        case BTN_DOWN: if (editWork > MIN_DURATION) editWork--;  break;
        case BTN_RIGHT:
          appState = STATE_MENU_SHORT;
          lcd.clear();
          break;
        case BTN_LEFT:
          // Cancel — exit menu without saving.
          appState = STATE_IDLE;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Save all pages and return to IDLE.
          workMin    = editWork;
          shortMin   = editShort;
          longMin    = editLong;
          buzzerMode = editBuzz;
          saveSettings();
          resetTimer();
          appState = STATE_IDLE;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Work Duration:  "); // 16 chars
      drawMenuRow1(editWork);
      break;

    // ---- MENU: Short break duration -----------------------------------------
    case STATE_MENU_SHORT:
      switch (pressed) {
        case BTN_UP:   if (editShort < MAX_SHORT_MIN) editShort++; break;
        case BTN_DOWN: if (editShort > MIN_DURATION)  editShort--; break;
        case BTN_RIGHT:
          appState = STATE_MENU_LONG;
          lcd.clear();
          break;
        case BTN_LEFT:
          appState = STATE_MENU_WORK;
          lcd.clear();
          break;
        case BTN_SELECT:
          workMin    = editWork;
          shortMin   = editShort;
          longMin    = editLong;
          buzzerMode = editBuzz;
          saveSettings();
          resetTimer();
          appState = STATE_IDLE;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Short Break:    "); // 16 chars
      drawMenuRow1(editShort);
      break;

    // ---- MENU: Long break duration ------------------------------------------
    case STATE_MENU_LONG:
      switch (pressed) {
        case BTN_UP:   if (editLong < MAX_LONG_MIN) editLong++; break;
        case BTN_DOWN: if (editLong > MIN_DURATION) editLong--; break;
        case BTN_LEFT:
          appState = STATE_MENU_SHORT;
          lcd.clear();
          break;
        case BTN_RIGHT:
          // Advance to the buzzer-mode page.
          appState = STATE_MENU_BUZZ;
          lcd.clear();
          break;
        case BTN_SELECT:
          workMin    = editWork;
          shortMin   = editShort;
          longMin    = editLong;
          buzzerMode = editBuzz;
          saveSettings();
          resetTimer();
          appState = STATE_IDLE;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Long Break:     "); // 16 chars
      drawMenuRow1(editLong);
      break;

    // ---- MENU: Buzzer mode --------------------------------------------------
    case STATE_MENU_BUZZ:
      switch (pressed) {
        case BTN_UP:
          editBuzz = (BuzzerMode)((editBuzz + 1) % BUZZ_MODE_COUNT);
          break;
        case BTN_DOWN:
          editBuzz = (BuzzerMode)((editBuzz + BUZZ_MODE_COUNT - 1) % BUZZ_MODE_COUNT);
          break;
        case BTN_LEFT:
          appState = STATE_MENU_LONG;
          lcd.clear();
          break;
        case BTN_RIGHT:
        case BTN_SELECT:
          // RIGHT on the last page saves and exits, same as SELECT.
          workMin    = editWork;
          shortMin   = editShort;
          longMin    = editLong;
          buzzerMode = editBuzz;
          saveSettings();
          resetTimer();
          appState = STATE_IDLE;
          lcd.clear();
          break;
        default: break;
      }
      drawMenuRow0("Buzzer Mode:    "); // 16 chars
      drawMenuRowBuzz(editBuzz);
      break;
  }

  delay(LOOP_DELAY_MS);
}
