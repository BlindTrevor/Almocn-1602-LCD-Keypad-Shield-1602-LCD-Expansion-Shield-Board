// ComponentIdentifier/ComponentIdentifier.ino
// Electronic component identifier using the Almocn 1602 LCD Keypad Shield.
//
// Identifies resistors, capacitors, silicon/germanium diodes, and LEDs
// automatically, then displays the component type and measured value on
// the 16×2 LCD.
//
// ── Required external hardware ────────────────────────────────────────────────
//
//   One 10 kΩ resistor (1/4 W, 1 % tolerance recommended)
//
// ── Wiring ────────────────────────────────────────────────────────────────────
//
//   Arduino             External             Component
//   ──────────────────────────────────────────────────────
//   D12 ────[10 kΩ]──── A1 (PROBE_PLUS)  ─────────┐
//                                                   │  unknown component
//   A2  (PROBE_MINUS, driven LOW) ──────────────────┘
//
//   ① Solder or clip a 10 kΩ resistor between D12 and A1.
//   ② Attach one probe wire to A1  (PROBE_PLUS,  yellow/red).
//   ③ Attach one probe wire to A2  (PROBE_MINUS, black).
//   ④ Connect the unknown component between the two probe wires.
//      For diodes and LEDs connect the ANODE to A1 and CATHODE to A2.
//
// ── Measurement techniques ────────────────────────────────────────────────────
//
//   Resistor  : Voltage divider with R_ref (10 kΩ).
//               R = R_ref × V_A1 / (5 V − V_A1).
//               Accurate range: ~100 Ω to ~1 MΩ.
//
//   Capacitor : RC charge-time method. C = τ / R_ref, where τ is the time
//               for A1 to rise from 0 V to 63.2 % of Vcc (≈ 3.16 V).
//               Measurable range: ~1 µF to ~200 µF.
//
//   Diode     : Stable Vf in a known forward-voltage band.
//                 Germanium  Vf ≈ 0.15 – 0.45 V
//                 Silicon    Vf ≈ 0.50 – 0.75 V
//   LED       : Stable Vf ≈ 1.60 – 3.50 V.
//   Short     : A1 reads < 0.05 V with D12 HIGH.
//   Open      : A1 reads > 4.95 V with D12 HIGH.
//
// ── User workflow ─────────────────────────────────────────────────────────────
//
//   1. Upload this sketch and seat the shield on the Arduino.
//   2. Wire the 10 kΩ reference resistor between D12 and A1.
//   3. Press SELECT to reach the "Connect component" prompt.
//   4. Connect the unknown component between A1 (PROBE_PLUS) and A2 (PROBE_MINUS).
//   5. Press SELECT to trigger measurement.
//   6. Read the result on the LCD.
//   7. Press SELECT to test another component, or RIGHT to return to the start.
//
// ── Navigation ────────────────────────────────────────────────────────────────
//
//   STATE_IDLE   : splash screen            SELECT → STATE_PROMPT
//   STATE_PROMPT : "Connect & press SEL"    SELECT → (measure) → STATE_RESULT
//   STATE_RESULT : type + value             SELECT → STATE_PROMPT
//                                           RIGHT  → STATE_IDLE

#include <LiquidCrystal.h>

// ── Hardware pins (same wiring as all other sketches) ─────────────────────────
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN       = A0;
const int BACKLIGHT_PIN = 10;

// Measurement circuit pins
const int REF_PIN      = 12;  // D12: driven HIGH through 10 kΩ reference resistor
const int PROBE_A_PIN  = A1;  // measurement node — one probe terminal (PROBE_PLUS)
const int PROBE_B_PIN  = A2;  // driven LOW   — other probe terminal (PROBE_MINUS)

// Reference resistor value (Ω). Use a 1 % 10 kΩ resistor for best accuracy.
const float R_REF_OHMS = 10000.0f;

// Nominal supply voltage (V). Adjust if your board differs from 5.00 V.
const float VCC = 5.0f;

// ── ADC classification thresholds ─────────────────────────────────────────────
// D12 HIGH → [10 kΩ] → A1 → component → A2 (GND)
// Voltage at A1: 0 V = short, 5 V = open, in between = component.
//
// ADC = V_A1 × 1023 / 5.0
const int ADC_SHORT_MAX = 10;   // < ~0.05 V  → short circuit / wire
const int ADC_GE_LOW    = 31;   // ≥ ~0.15 V  → germanium diode lower bound
const int ADC_GE_HIGH   = 92;   // ≤ ~0.45 V  → germanium diode upper bound
const int ADC_SI_LOW    = 102;  // ≥ ~0.50 V  → silicon diode lower bound
const int ADC_SI_HIGH   = 153;  // ≤ ~0.75 V  → silicon diode upper bound
const int ADC_LED_LOW   = 328;  // ≥ ~1.60 V  → LED lower bound
const int ADC_LED_HIGH  = 716;  // ≤ ~3.50 V  → LED upper bound
const int ADC_OPEN_MIN  = 1013; // > ~4.95 V  → open circuit (no component)

// If two ADC readings taken 500 ms apart differ by more than this value,
// the voltage is still changing → the component is a capacitor.
const int ADC_STABILITY_DELTA = 12;

// ADC level that equals 63.2 % of Vcc (one RC time constant).
const int ADC_CAP_THRESHOLD = 647;

// Maximum time to wait for a capacitor to charge to ADC_CAP_THRESHOLD (ms).
const unsigned long CAP_CHARGE_TIMEOUT_MS   = 10000UL;

// Maximum time to wait for a capacitor to discharge (ms).
const unsigned long CAP_DISCHARGE_TIMEOUT_MS = 15000UL;

// ── Backlight ─────────────────────────────────────────────────────────────────
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = !BACKLIGHT_ON_LEVEL;

// ── Timing constants ──────────────────────────────────────────────────────────
const unsigned long DEBOUNCE_MS   = 40;
const unsigned long LOOP_DELAY_MS = 80;
const unsigned long SPLASH_MS     = 1400;

// ── Button enum (identical to all other sketches) ─────────────────────────────
enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

// ── Application state machine ─────────────────────────────────────────────────
enum AppState {
  STATE_IDLE,    // splash / welcome screen
  STATE_PROMPT,  // "Connect component, press SELECT"
  STATE_RESULT   // displaying the measurement result
};

// ── Component type ────────────────────────────────────────────────────────────
enum CompType {
  COMP_OPEN,      // no component detected (open circuit)
  COMP_SHORT,     // short circuit / wire
  COMP_RESISTOR,  // resistive component
  COMP_CAPACITOR, // capacitive component
  COMP_DIODE_SI,  // silicon diode
  COMP_DIODE_GE,  // germanium diode
  COMP_LED,       // light-emitting diode
  COMP_UNKNOWN    // unclassified
};

// ── Runtime state ─────────────────────────────────────────────────────────────
AppState appState    = STATE_IDLE;
CompType resultType  = COMP_UNKNOWN;
float    resultValue = 0.0f;  // resistance (Ω) or capacitance (µF) or Vf (V)
bool     backlightIsOn = true;

// ── Button helpers (identical to Menu.ino / AlarmClock.ino / KitchenTimer.ino) ─
Button readButton(int adc) {
  if (adc < 50)  return BTN_RIGHT;
  if (adc < 180) return BTN_UP;
  if (adc < 330) return BTN_DOWN;
  if (adc < 525) return BTN_LEFT;
  if (adc < 800) return BTN_SELECT;
  return BTN_NONE;
}

// Returns the button that was just pressed (rising-edge, debounced).
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

// ── Backlight (identical to all other sketches) ───────────────────────────────
void setBacklight(bool on) {
  digitalWrite(BACKLIGHT_PIN, on ? BACKLIGHT_ON_LEVEL : BACKLIGHT_OFF_LEVEL);
  backlightIsOn = on;
}

// ── Measurement helpers ───────────────────────────────────────────────────────

// Return the average of `samples` ADC readings from PROBE_A_PIN.
int averageADC(int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(PROBE_A_PIN);
    delay(2);
  }
  return (int)(sum / samples);
}

// Drive REF_PIN HIGH and PROBE_B_PIN LOW to energise the measurement circuit.
void enableMeasurement() {
  pinMode(REF_PIN, OUTPUT);
  digitalWrite(REF_PIN, HIGH);
  pinMode(PROBE_B_PIN, OUTPUT);
  digitalWrite(PROBE_B_PIN, LOW);
}

// Set both drive pins to high-impedance (safe state, no measurement).
void disableMeasurement() {
  pinMode(REF_PIN, INPUT);
  pinMode(PROBE_B_PIN, INPUT);
}

// Discharge a capacitor by driving REF_PIN LOW (through the 10 kΩ reference).
// Waits until A1 falls below ~0.05 V, or until the timeout expires.
void dischargeCapacitor() {
  // Show progress on LCD row 1
  lcd.setCursor(0, 1);
  lcd.print("Discharging...  ");

  pinMode(REF_PIN, OUTPUT);
  digitalWrite(REF_PIN, LOW);   // discharge via 10 kΩ reference to GND

  unsigned long start = millis();
  while (analogRead(PROBE_A_PIN) > ADC_SHORT_MAX &&
         millis() - start < CAP_DISCHARGE_TIMEOUT_MS) {
    // wait
  }
}

// Return the component type based on a stable ADC reading.
// Returns COMP_UNKNOWN when the reading falls in the resistor voltage range.
CompType classifyByVoltage(int adc) {
  if (adc <= ADC_SHORT_MAX) return COMP_SHORT;
  if (adc >= ADC_OPEN_MIN)  return COMP_OPEN;
  if (adc >= ADC_GE_LOW  && adc <= ADC_GE_HIGH)  return COMP_DIODE_GE;
  if (adc >= ADC_SI_LOW  && adc <= ADC_SI_HIGH)  return COMP_DIODE_SI;
  if (adc >= ADC_LED_LOW && adc <= ADC_LED_HIGH) return COMP_LED;
  return COMP_UNKNOWN;
}

// Calculate resistance from a stable ADC reading using the voltage-divider
// formula:  R_x = R_ref × V_A1 / (Vcc − V_A1).
// Returns resistance in ohms.
float calcResistance(int adc) {
  if (adc <= 0)    return 0.0f;
  if (adc >= 1023) return 1.0e9f;
  return R_REF_OHMS * (float)adc / (1023.0f - (float)adc);
}

// Measure capacitance using the RC charge-time method.
// Assumes the capacitor has already been discharged.
// Returns capacitance in µF, or -1.0 if the charge timeout expires.
float measureCapacitance() {
  lcd.setCursor(0, 1);
  lcd.print("Charging...     ");

  enableMeasurement();
  unsigned long startMs = millis();

  // Wait until A1 reaches 63.2 % of Vcc (one time constant).
  while (analogRead(PROBE_A_PIN) < ADC_CAP_THRESHOLD) {
    if (millis() - startMs > CAP_CHARGE_TIMEOUT_MS) {
      disableMeasurement();
      return -1.0f;  // capacitor too large to measure in this time window
    }
  }
  unsigned long tauMs = millis() - startMs;
  disableMeasurement();

  // C (µF) = τ (ms) × 1000 / R (Ω)  =  τ (ms) / R (kΩ)
  return (float)tauMs * 1000.0f / R_REF_OHMS;
}

// ── Main measurement routine ──────────────────────────────────────────────────
// Blocks until identification is complete. Updates the LCD with progress
// messages. Sets resultType and resultValue before returning.
void runMeasurement() {
  resultType  = COMP_UNKNOWN;
  resultValue = 0.0f;

  lcd.setCursor(0, 0);
  lcd.print("Measuring...    ");
  lcd.setCursor(0, 1);
  lcd.print("Please wait...  ");

  // Step 1 — Energise the circuit and allow the voltage to settle.
  enableMeasurement();
  delay(80);
  int adc1 = averageADC(16);  // first reading

  // Step 2 — Second reading 500 ms later to detect a charging capacitor.
  delay(500);
  int adc2 = averageADC(16);

  // Step 3 — If the reading is still changing, the component is a capacitor.
  if (abs(adc2 - adc1) > ADC_STABILITY_DELTA) {
    disableMeasurement();
    dischargeCapacitor();
    resultType  = COMP_CAPACITOR;
    resultValue = measureCapacitance();
    return;
  }

  // Step 4 — Stable reading: classify by voltage.
  CompType ct = classifyByVoltage(adc2);

  if (ct == COMP_DIODE_SI || ct == COMP_DIODE_GE || ct == COMP_LED) {
    // Store the forward voltage in volts.
    resultValue = (float)adc2 * VCC / 1023.0f;
    resultType  = ct;
    disableMeasurement();
    return;
  }

  if (ct != COMP_UNKNOWN) {
    resultType = ct;
    disableMeasurement();
    return;
  }

  // Step 5 — Resistor: calculate R from the voltage-divider equation.
  resultType  = COMP_RESISTOR;
  resultValue = calcResistance(adc2);
  disableMeasurement();
}

// ── Display helpers ───────────────────────────────────────────────────────────

// Print resistance with appropriate unit onto the current LCD cursor position.
// Always writes exactly 16 characters (pads with spaces).
void printResistance(float ohms) {
  char num[8];
  char buf[17];

  if (ohms < 1.0f) {
    lcd.print("< 1 ohm         ");
  } else if (ohms < 1000.0f) {
    snprintf(buf, sizeof(buf), "%-5d ohm        ", (int)(ohms + 0.5f));  // 0.5f rounds to nearest integer
    lcd.print(buf);
  } else if (ohms < 1000000.0f) {
    dtostrf(ohms / 1000.0f, 5, 2, num);
    snprintf(buf, sizeof(buf), "%s kohm       ", num);
    buf[16] = '\0';
    lcd.print(buf);
  } else {
    dtostrf(ohms / 1000000.0f, 5, 2, num);
    snprintf(buf, sizeof(buf), "%s Mohm       ", num);
    buf[16] = '\0';
    lcd.print(buf);
  }
}

// Print capacitance in µF (or ">" if over range).
void printCapacitance(float uf) {
  char num[8];
  char buf[17];

  if (uf < 0.0f) {
    lcd.print(">200 uF         ");
  } else if (uf < 1000.0f) {
    dtostrf(uf, 5, 1, num);
    snprintf(buf, sizeof(buf), "%s uF          ", num);
    buf[16] = '\0';
    lcd.print(buf);
  } else {
    dtostrf(uf / 1000.0f, 5, 3, num);
    snprintf(buf, sizeof(buf), "%s mF          ", num);
    buf[16] = '\0';
    lcd.print(buf);
  }
}

// Print forward voltage in volts, with an "A1=+" polarity note.
void printForwardVoltage(float vf) {
  char num[6];
  char buf[17];
  dtostrf(vf, 4, 2, num);
  snprintf(buf, sizeof(buf), "Vf=%sV  A1=+  ", num);
  buf[16] = '\0';
  lcd.print(buf);
}

void drawIdle() {
  lcd.setCursor(0, 0);
  lcd.print("Component Tester");
  lcd.setCursor(0, 1);
  lcd.print("  SEL = start   ");
}

void drawPrompt() {
  lcd.setCursor(0, 0);
  lcd.print("Connect A1 & A2 ");
  lcd.setCursor(0, 1);
  lcd.print("  SEL = measure ");
}

void drawResult() {
  switch (resultType) {

    case COMP_OPEN:
      lcd.setCursor(0, 0); lcd.print("Open / No comp. ");
      lcd.setCursor(0, 1); lcd.print("Check connection");
      break;

    case COMP_SHORT:
      lcd.setCursor(0, 0); lcd.print("SHORT / WIRE    ");
      lcd.setCursor(0, 1); lcd.print("< 1 ohm         ");
      break;

    case COMP_RESISTOR:
      lcd.setCursor(0, 0); lcd.print("RESISTOR        ");
      lcd.setCursor(0, 1); printResistance(resultValue);
      break;

    case COMP_CAPACITOR:
      lcd.setCursor(0, 0); lcd.print("CAPACITOR       ");
      lcd.setCursor(0, 1); printCapacitance(resultValue);
      break;

    case COMP_DIODE_SI:
      lcd.setCursor(0, 0); lcd.print("DIODE  Silicon  ");
      lcd.setCursor(0, 1); printForwardVoltage(resultValue);
      break;

    case COMP_DIODE_GE:
      lcd.setCursor(0, 0); lcd.print("DIODE  Germanium");
      lcd.setCursor(0, 1); printForwardVoltage(resultValue);
      break;

    case COMP_LED:
      lcd.setCursor(0, 0); lcd.print("LED             ");
      lcd.setCursor(0, 1); printForwardVoltage(resultValue);
      break;

    default:
      lcd.setCursor(0, 0); lcd.print("Unknown         ");
      lcd.setCursor(0, 1); lcd.print("Check wiring    ");
      break;
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  setBacklight(true);

  // Leave measurement pins in high-impedance until a measurement begins.
  pinMode(REF_PIN, INPUT);
  pinMode(PROBE_B_PIN, INPUT);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Component Tester");
  lcd.setCursor(0, 1);
  lcd.print("Initializing... ");
  delay(SPLASH_MS);
  lcd.clear();
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
  Button pressed = getPressEvent();

  switch (appState) {

    // ---- Welcome / idle screen -----------------------------------------------
    case STATE_IDLE:
      drawIdle();
      if (pressed == BTN_SELECT) {
        appState = STATE_PROMPT;
        lcd.clear();
      }
      break;

    // ---- "Connect component" prompt ------------------------------------------
    case STATE_PROMPT:
      drawPrompt();
      if (pressed == BTN_SELECT) {
        lcd.clear();
        runMeasurement();   // blocking; updates LCD with progress messages
        appState = STATE_RESULT;
        lcd.clear();
      }
      break;

    // ---- Result display -------------------------------------------------------
    case STATE_RESULT:
      drawResult();
      if (pressed == BTN_SELECT) {
        // Test another component
        appState = STATE_PROMPT;
        lcd.clear();
      } else if (pressed == BTN_RIGHT) {
        // Return to the welcome screen
        appState = STATE_IDLE;
        lcd.clear();
      }
      break;
  }

  delay(LOOP_DELAY_MS);
}
