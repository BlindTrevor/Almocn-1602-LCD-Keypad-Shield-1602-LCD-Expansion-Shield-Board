#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN = A0;

// Many shields use D10 for backlight control.
// Some are active LOW (LOW = on), some active HIGH (HIGH = on).
const int BACKLIGHT_PIN = 10;

// Change this if your backlight works inverted:
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = LOW;

const unsigned long BACKLIGHT_TIMEOUT_MS = 5000;

enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

Button readButton(int adc) {
  // Thresholds tailored to your measured values:
  // RIGHT 0, UP 100, DOWN 256, LEFT 410, SELECT 640, NONE 1023
  if (adc < 50)    return BTN_RIGHT;
  if (adc < 180)   return BTN_UP;
  if (adc < 330)   return BTN_DOWN;
  if (adc < 525)   return BTN_LEFT;
  if (adc < 800)   return BTN_SELECT;
  return BTN_NONE;
}

const char* buttonName(Button b) {
  switch (b) {
    case BTN_RIGHT:  return "RIGHT";
    case BTN_UP:     return "UP";
    case BTN_DOWN:   return "DOWN";
    case BTN_LEFT:   return "LEFT";
    case BTN_SELECT: return "SELECT";
    default:         return "NONE";
  }
}

unsigned long lastActivityMs = 0;
bool backlightIsOn = true;

void setBacklight(bool on) {
  digitalWrite(BACKLIGHT_PIN, on ? BACKLIGHT_ON_LEVEL : BACKLIGHT_OFF_LEVEL);
  backlightIsOn = on;
}

void setup() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  setBacklight(true);
  lastActivityMs = millis();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Button + BL");
  lcd.setCursor(0, 1);
  lcd.print("timeout 5s");
  delay(800);
  lcd.clear();
}

void loop() {
  static Button lastStable = BTN_NONE;
  static unsigned long lastChangeMs = 0;

  const unsigned long now = millis();

  // Read current button
  int adc = analogRead(KEY_PIN);
  Button current = readButton(adc);

  // Simple debounce: detect a stable change lasting >= 40ms
  if (current != lastStable) {
    // Potential change; start timing
    if (now - lastChangeMs >= 40) {
      // Accept the change
      lastStable = current;
      lastChangeMs = now;

      // Treat any real button press (not NONE) as activity
      if (lastStable != BTN_NONE) {
        lastActivityMs = now;
        if (!backlightIsOn) setBacklight(true);
      }
    }
  } else {
    // No change; reset timer baseline for future changes
    lastChangeMs = now;
  }

  // Backlight timeout logic
  if (backlightIsOn && (now - lastActivityMs >= BACKLIGHT_TIMEOUT_MS)) {
    setBacklight(false);
  }

  // Display info (optional)
  lcd.setCursor(0, 0);
  lcd.print("Btn: ");
  lcd.print(buttonName(lastStable));
  lcd.print("       ");

  lcd.setCursor(0, 1);
  lcd.print(backlightIsOn ? "Backlight: ON " : "Backlight: OFF");

  delay(80);
}
