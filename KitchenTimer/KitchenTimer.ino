// KitchenTimer/KitchenTimer.ino
// Kitchen timer using the Almocn 1602 LCD Keypad Shield.
//
// Set time view (STATE_SET):
//   Row 0:  Set Timer  MM:SS
//   Row 1:  U/D=min LR=sec
//   UP / DOWN change the minutes (hold to repeat, accelerates after 2 s).
//   LEFT / RIGHT change the seconds (hold to repeat, accelerates after 2 s).
//   SELECT starts the countdown immediately.
//
// Running view (STATE_RUNNING):
//   Row 0:  Timer   MM:SS
//   Row 1:  SEL=pause RT=rst
//
// Paused view (STATE_PAUSED):
//   Row 0:  Timer   MM:SS
//   Row 1:  SEL=resm  RT=rst
//
// Done view (STATE_DONE) — timer reached zero:
//   Row 0:  ** TIME IS UP! *
//   Row 1:  SEL=reset
//   Backlight flashes every 500 ms.
//   Pin D3 (BUZZER_PIN) toggles HIGH/LOW every 500 ms.
//   SELECT or RIGHT stops the buzzer and returns to the set-time screen.

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
const unsigned long FLASH_INTERVAL_MS = 500;  // backlight flash half-period (ms)
const unsigned long BUZZ_INTERVAL_MS  = 500;  // buzzer toggle half-period (ms)

// Hold-to-repeat for all four directional buttons while setting the time.
const unsigned long HOLD_DELAY_MS      = 500;  // pause before repeat begins (ms)
const unsigned long HOLD_REPEAT_MS     = 200;  // slow repeat interval (ms)
const unsigned long HOLD_FAST_MS       = 80;   // fast repeat interval (ms)
const unsigned long HOLD_FAST_AFTER_MS = 2000; // ms of holding before switching to fast rate

// ---------------------------------------------------------------------------
// Timer limits
// ---------------------------------------------------------------------------
const int MAX_MINUTES = 99;
const int MAX_SECONDS = 59;

// ---------------------------------------------------------------------------
// Button enum (same ADC thresholds as all other sketches)
// ---------------------------------------------------------------------------
enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

// ---------------------------------------------------------------------------
// Application state machine
// ---------------------------------------------------------------------------
enum AppState {
  STATE_SET,      // user is setting minutes and seconds on one screen
  STATE_RUNNING,  // countdown is active
  STATE_PAUSED,   // countdown is paused mid-run
  STATE_DONE      // timer reached zero — buzzer is firing
};

// ---------------------------------------------------------------------------
// Runtime variables
// ---------------------------------------------------------------------------
int      setMinutes    = 5;    // last user-chosen minutes (persists between resets)
int      setSeconds    = 0;    // last user-chosen seconds (persists between resets)
long     secsRemaining = 0;    // remaining seconds while running or paused
bool     backlightIsOn = true;
AppState appState      = STATE_SET;

unsigned long timerLastTickMs = 0; // millis() snapshot of the last countdown tick

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

// Returns the button event for this loop() iteration.
// For UP / DOWN / LEFT / RIGHT: fires on the first press then repeats at an
//               accelerating rate while the button is held.
//   Rate schedule: HOLD_DELAY_MS initial pause → HOLD_REPEAT_MS slow phase
//                  → HOLD_FAST_MS fast phase (after HOLD_FAST_AFTER_MS total hold time).
// For SELECT: fires once per press (rising-edge only, debounced).
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
        if (lastStable == BTN_UP   || lastStable == BTN_DOWN ||
            lastStable == BTN_LEFT || lastStable == BTN_RIGHT) {
          heldSinceMs  = now;
          lastRepeatMs = now;
          holdActive   = true;
        }
        return lastStable; // fire immediately on the first press
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
    if (secsRemaining > 0) --secsRemaining;
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

// Setting view — combined minutes and seconds on one screen.
// Row 0:  Set Timer  MM:SS   (16 chars)
// Row 1:  U/D=min LR=sec     (16 chars)
void drawSetTimer() {
  lcd.setCursor(0, 0);
  lcd.print("Set Timer  ");
  printTwoDigits(setMinutes);
  lcd.print(':');
  printTwoDigits(setSeconds);
  lcd.setCursor(0, 1);
  lcd.print("U/D=min LR=sec  ");
}

// Countdown display — shared between STATE_RUNNING and STATE_PAUSED.
// Row 0:  Timer   MM:SS      (16 chars)
// Row 1:  <hint>             (caller must supply exactly 16 chars)
void drawTimer(const char* hint) {
  const int mm = (int)(secsRemaining / 60);
  const int ss = (int)(secsRemaining % 60);
  lcd.setCursor(0, 0);
  lcd.print("Timer   ");
  printTwoDigits(mm);
  lcd.print(':');
  printTwoDigits(ss);
  lcd.print("   ");
  lcd.setCursor(0, 1);
  lcd.print(hint);
}

// Alert view when the timer reaches zero.
// Row 0:  ** TIME IS UP! * (16 chars)
// Row 1:  SEL=reset        (16 chars)
void drawDone() {
  lcd.setCursor(0, 0);
  lcd.print("** TIME IS UP! *");
  lcd.setCursor(0, 1);
  lcd.print("SEL=reset       ");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Start the countdown from the current setMinutes / setSeconds values.
// Ignores the request if the total duration is zero.
void startTimer() {
  const long total = (long)setMinutes * 60 + setSeconds;
  if (total == 0) return;
  secsRemaining   = total;
  timerLastTickMs = millis();
  appState        = STATE_RUNNING;
  lcd.clear();
}

// Stop the buzzer and return to the set-time screen.
void resetToSet() {
  digitalWrite(BUZZER_PIN, LOW);
  setBacklight(true);
  appState = STATE_SET;
  lcd.clear();
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
  lcd.print(" Kitchen Timer  ");
  lcd.setCursor(0, 1);
  lcd.print("  SEL = start   ");
  delay(SPLASH_MS);
  lcd.clear();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  const unsigned long now     = millis();
  Button              pressed = getButtonEvent();

  switch (appState) {

    // ---- Set time (minutes and seconds on one screen) ----------------------
    case STATE_SET:
      switch (pressed) {
        case BTN_UP:
          setMinutes = (setMinutes < MAX_MINUTES) ? setMinutes + 1 : 0;
          break;
        case BTN_DOWN:
          setMinutes = (setMinutes > 0) ? setMinutes - 1 : MAX_MINUTES;
          break;
        case BTN_RIGHT:
          setSeconds = (setSeconds < MAX_SECONDS) ? setSeconds + 1 : 0;
          break;
        case BTN_LEFT:
          setSeconds = (setSeconds > 0) ? setSeconds - 1 : MAX_SECONDS;
          break;
        case BTN_SELECT:
          startTimer();
          break;
        default: break;
      }
      if (appState == STATE_SET) drawSetTimer();
      break;

    // ---- Running -----------------------------------------------------------
    case STATE_RUNNING:
      tickTimer();
      if (secsRemaining == 0) {
        appState = STATE_DONE;
        lcd.clear();
        break;
      }
      switch (pressed) {
        case BTN_SELECT:
          appState = STATE_PAUSED;
          lcd.clear();
          break;
        case BTN_RIGHT:
          resetToSet();
          break;
        default: break;
      }
      if (appState == STATE_RUNNING) drawTimer("SEL=pause RT=rst");
      break;

    // ---- Paused ------------------------------------------------------------
    case STATE_PAUSED:
      switch (pressed) {
        case BTN_SELECT:
          // Reset the tick reference so the first tick fires exactly 1 s later.
          timerLastTickMs = millis();
          appState        = STATE_RUNNING;
          lcd.clear();
          break;
        case BTN_RIGHT:
          resetToSet();
          break;
        default: break;
      }
      if (appState == STATE_PAUSED) drawTimer("SEL=resm  RT=rst");
      break;

    // ---- Done — timer reached zero -----------------------------------------
    case STATE_DONE: {
      // Flash backlight every FLASH_INTERVAL_MS
      static unsigned long lastFlashMs = 0;
      if (now - lastFlashMs >= FLASH_INTERVAL_MS) {
        lastFlashMs = now;
        setBacklight(!backlightIsOn);
      }

      // Toggle buzzer on D3 every BUZZ_INTERVAL_MS
      static unsigned long lastBuzzMs = 0;
      static bool          buzzerOn   = false;
      if (now - lastBuzzMs >= BUZZ_INTERVAL_MS) {
        lastBuzzMs = now;
        buzzerOn   = !buzzerOn;
        digitalWrite(BUZZER_PIN, buzzerOn ? HIGH : LOW);
      }

      drawDone();

      if (pressed == BTN_SELECT || pressed == BTN_RIGHT) {
        buzzerOn = false;
        resetToSet();
      }
      break;
    }
  }

  delay(LOOP_DELAY_MS);
}
