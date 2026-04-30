// Menu/Menu.ino
// Based on Demo/Demo.ino — adds a user menu for configuring backlight
// timeout duration and auto-off enable/disable without recompiling.
//
// Navigation:
//   Normal view  →  press SELECT to enter the menu
//   In menu      →  LEFT / RIGHT to move between menu items
//                   UP   / DOWN  to change the highlighted setting
//                   SELECT       to leave the menu and return to normal view
//
// Menu items:
//   1. Timeout  — choose from preset durations (5s, 10s, 15s, 30s, 1min, 2min, 5min)
//   2. Auto-Off — ENABLED or DISABLED

#include <LiquidCrystal.h>

// ---------------------------------------------------------------------------
// Hardware constants — match Demo.ino pin assignments
// ---------------------------------------------------------------------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN      = A0;
const int BACKLIGHT_PIN = 10;

// Set BACKLIGHT_ON_LEVEL to LOW if your shield uses an active-LOW backlight.
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = !BACKLIGHT_ON_LEVEL;  // automatically the opposite level

// ---------------------------------------------------------------------------
// Button enum (same as Demo.ino)
// ---------------------------------------------------------------------------
enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

// ---------------------------------------------------------------------------
// Menu state machine
// ---------------------------------------------------------------------------
// STATE_NORMAL     – shows current button + backlight status (like Demo.ino)
// STATE_MENU_TIMEOUT – lets the user pick a timeout duration
// STATE_MENU_AUTOOFF – lets the user enable/disable auto-off
enum MenuState { STATE_NORMAL, STATE_MENU_TIMEOUT, STATE_MENU_AUTOOFF };

// ---------------------------------------------------------------------------
// Timeout presets
// ---------------------------------------------------------------------------
const unsigned long TIMEOUT_OPTIONS[] = {
  5000UL,    //  5 seconds
  10000UL,   // 10 seconds
  15000UL,   // 15 seconds
  30000UL,   // 30 seconds
  60000UL,   //  1 minute
  120000UL,  //  2 minutes
  300000UL   //  5 minutes
};
const char* TIMEOUT_LABELS[] = {
  " 5 sec", "10 sec", "15 sec", "30 sec",
  " 1 min", " 2 min", " 5 min"
};
const int TIMEOUT_COUNT = 7;

// Timing constants
const unsigned long DEBOUNCE_DELAY_MS    = 40;   // ms a button must be stable before accepted
const unsigned long SPLASH_SCREEN_DELAY_MS = 1200; // ms to show the startup message
const unsigned long LOOP_DELAY_MS        = 80;   // ms between each loop iteration (refresh rate)

// ---------------------------------------------------------------------------
// Runtime state
// ---------------------------------------------------------------------------
int           timeoutIndex   = 0;     // index into TIMEOUT_OPTIONS / TIMEOUT_LABELS
bool          autoOffEnabled = true;  // whether backlight auto-off is active
unsigned long lastActivityMs = 0;     // millis() of last button press
bool          backlightIsOn  = true;
MenuState     menuState      = STATE_NORMAL;

// ---------------------------------------------------------------------------
// Helper: read and debounce the keypad
// ---------------------------------------------------------------------------
Button readButton(int adc) {
  // Thresholds match Demo.ino:
  // RIGHT ~0, UP ~100, DOWN ~256, LEFT ~410, SELECT ~640, NONE ~1023
  if (adc < 50)   return BTN_RIGHT;
  if (adc < 180)  return BTN_UP;
  if (adc < 330)  return BTN_DOWN;
  if (adc < 525)  return BTN_LEFT;
  if (adc < 800)  return BTN_SELECT;
  return BTN_NONE;
}

// Returns the button that was just pressed (rising edge only).
// Call once per loop iteration to get at most one press event.
Button getPressEvent() {
  static Button lastStable  = BTN_NONE;
  static unsigned long lastChangeMs = 0;

  const unsigned long now = millis();
  Button current = readButton(analogRead(KEY_PIN));

  if (current != lastStable) {
    if (now - lastChangeMs >= DEBOUNCE_DELAY_MS) {
      lastStable    = current;
      lastChangeMs  = now;
      // A press event fires when a real button becomes active (not NONE)
      if (lastStable != BTN_NONE) {
        return lastStable;
      }
    }
  } else {
    lastChangeMs = now;
  }
  return BTN_NONE;
}

// ---------------------------------------------------------------------------
// Backlight control (identical to Demo.ino)
// ---------------------------------------------------------------------------
void setBacklight(bool on) {
  digitalWrite(BACKLIGHT_PIN, on ? BACKLIGHT_ON_LEVEL : BACKLIGHT_OFF_LEVEL);
  backlightIsOn = on;
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

// Normal view: row 0 = last button pressed, row 1 = backlight state
void drawNormal(Button lastBtn) {
  lcd.setCursor(0, 0);
  lcd.print("Btn: ");
  switch (lastBtn) {
    case BTN_RIGHT:  lcd.print("RIGHT  "); break;
    case BTN_UP:     lcd.print("UP     "); break;
    case BTN_DOWN:   lcd.print("DOWN   "); break;
    case BTN_LEFT:   lcd.print("LEFT   "); break;
    case BTN_SELECT: lcd.print("SELECT "); break;
    default:         lcd.print("NONE   "); break;
  }
  lcd.setCursor(0, 1);
  lcd.print(backlightIsOn ? "BL:ON  [MENU]   " : "BL:OFF [MENU]   ");
}

// Timeout menu: shows current selection with navigation hints
// Row 0: "Timeout:"
// Row 1: "< 10 sec UP/DN >"   (arrows appear when more items exist)
void drawMenuTimeout() {
  lcd.setCursor(0, 0);
  lcd.print("Timeout:        ");

  lcd.setCursor(0, 1);
  // Left arrow when not at first item
  lcd.print(timeoutIndex > 0 ? "<" : " ");
  lcd.print(TIMEOUT_LABELS[timeoutIndex]);
  lcd.print(" UP/DN ");
  // Right arrow when not at last item
  lcd.print(timeoutIndex < TIMEOUT_COUNT - 1 ? ">" : " ");
}

// Auto-Off menu
// Row 0: "Auto-Off:"
// Row 1: "< ENABLED UP/DN >"
void drawMenuAutoOff() {
  lcd.setCursor(0, 0);
  lcd.print("Auto-Off:       ");

  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.print(autoOffEnabled ? "ENABLED  UP/DN  " : "DISABLED UP/DN  ");
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  setBacklight(true);
  lastActivityMs = millis();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("BL Timeout Menu");
  lcd.setCursor(0, 1);
  lcd.print("SELECT=open menu");
  delay(SPLASH_SCREEN_DELAY_MS);
  lcd.clear();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  static Button lastDisplayedBtn = BTN_NONE;

  const unsigned long now = millis();

  // Get a single press event this iteration (debounced)
  Button pressed = getPressEvent();

  if (pressed != BTN_NONE) {
    // Any key press wakes / resets the backlight timeout
    lastActivityMs = now;
    if (!backlightIsOn) {
      setBacklight(true);
      // When waking from off, consume the press so we don't also navigate
      pressed = BTN_NONE;
    }
  }

  // ---- State machine -------------------------------------------------------
  switch (menuState) {

    // ---- Normal view --------------------------------------------------------
    case STATE_NORMAL:
      if (pressed != BTN_NONE) {
        lastDisplayedBtn = pressed;
      }
      if (pressed == BTN_SELECT) {
        // Enter first menu item
        menuState = STATE_MENU_TIMEOUT;
        lcd.clear();
      }
      drawNormal(lastDisplayedBtn);
      break;

    // ---- Timeout setting menu -----------------------------------------------
    case STATE_MENU_TIMEOUT:
      switch (pressed) {
        case BTN_UP:
          // Increase timeout (wrap at max)
          if (timeoutIndex < TIMEOUT_COUNT - 1) timeoutIndex++;
          break;
        case BTN_DOWN:
          // Decrease timeout (clamp at 0)
          if (timeoutIndex > 0) timeoutIndex--;
          break;
        case BTN_RIGHT:
          // Move to next menu item (Auto-Off)
          menuState = STATE_MENU_AUTOOFF;
          lcd.clear();
          break;
        case BTN_LEFT:
          // Already at first menu item; exit menu
          menuState = STATE_NORMAL;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Confirm and exit menu
          menuState = STATE_NORMAL;
          lcd.clear();
          break;
        default:
          break;
      }
      drawMenuTimeout();
      break;

    // ---- Auto-Off menu ------------------------------------------------------
    case STATE_MENU_AUTOOFF:
      switch (pressed) {
        case BTN_UP:
        case BTN_DOWN:
          // Toggle auto-off
          autoOffEnabled = !autoOffEnabled;
          break;
        case BTN_LEFT:
          // Go back to timeout menu
          menuState = STATE_MENU_TIMEOUT;
          lcd.clear();
          break;
        case BTN_RIGHT:
          // Already at last menu item; exit menu
          menuState = STATE_NORMAL;
          lcd.clear();
          break;
        case BTN_SELECT:
          // Confirm and exit menu
          menuState = STATE_NORMAL;
          lcd.clear();
          break;
        default:
          break;
      }
      drawMenuAutoOff();
      break;
  }

  // ---- Backlight auto-off logic --------------------------------------------
  // Only applies when outside the menu (to avoid turning off mid-edit)
  // and only when auto-off is enabled.
  if (menuState == STATE_NORMAL && autoOffEnabled && backlightIsOn) {
    if (now - lastActivityMs >= TIMEOUT_OPTIONS[timeoutIndex]) {
      setBacklight(false);
    }
  }

  delay(LOOP_DELAY_MS);
}
