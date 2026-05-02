// DmxMonitor/DmxMonitor.ino
// DMX512 monitor using a MAX485 module and the Almocn 1602 LCD Keypad Shield.
//
// ── MAX485 wiring ─────────────────────────────────────────────────────────
//   MAX485  │ Arduino    │ Notes
//   ────────┼────────────┼──────────────────────────────────────────────────
//   RO      │ D0 (RX)   │ Receiver Output → hardware UART RX
//   RE      │ D2        │ Receiver Enable (active LOW)  — held LOW = RX
//   DE      │ D11       │ Driver Enable   (active HIGH) — held LOW = RX
//   DI      │ —         │ Driver Input, not connected (receive only)
//   A       │ DMX +     │ Non-inverting RS-485 line (XLR 3-pin: pin 3)
//   B       │ DMX −     │ Inverting   RS-485 line (XLR 3-pin: pin 2)
//   VCC     │ 5 V       │
//   GND     │ GND       │
//
//   Optional: 120 Ω termination resistor across A and B at the last device.
//
// ⚠ D0 is shared with the USB–serial upload link.
//   Disconnect the MAX485 RO wire from D0 before uploading, then
//   reconnect it afterwards.
//
// Targets Arduino Uno / Nano (ATmega328P, 16 MHz).
// For Arduino Mega: connect MAX485 RO to D19 (Serial1 RX) instead of D0,
// then replace the UBRR0/UCSR0x/USART_RX registers with UBRR1/UCSR1x/
// USART1_RX in setupDMX() and the ISR.
//
// ── Normal view ───────────────────────────────────────────────────────────
//   Row 0:  Ch:001   Val:255
//   Row 1:  [##############]   ← 14-segment bar graph
//
// ── No-signal view ────────────────────────────────────────────────────────
//   Row 0:  Ch:001   Val:---
//   Row 1:   No DMX signal
//
// ── Keypad controls ───────────────────────────────────────────────────────
//   RIGHT / LEFT   channel ±1  (wraps at 1 and 512)
//   UP    / DOWN   channel ±10
//   SELECT         jump back to channel 1

#include <LiquidCrystal.h>
#include "ButtonType.h"

// ---------------------------------------------------------------------------
// Hardware pins
// ---------------------------------------------------------------------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN       = A0;
const int BACKLIGHT_PIN = 10;
const int RE_PIN        = 2;   // MAX485 RE (Receiver Enable, active LOW)
const int DE_PIN        = 11;  // MAX485 DE (Driver Enable, active HIGH)

// Set BACKLIGHT_ON_LEVEL to LOW if your shield uses an active-LOW backlight.
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = !BACKLIGHT_ON_LEVEL;

// ---------------------------------------------------------------------------
// Timing constants
// ---------------------------------------------------------------------------
const unsigned long DEBOUNCE_MS       = 40;    // button stability window (ms)
const unsigned long LOOP_DELAY_MS     = 80;    // display refresh interval (ms)
const unsigned long SPLASH_MS         = 1200;  // startup banner duration  (ms)
const unsigned long SIGNAL_TIMEOUT_MS = 1000;  // ms with no frame → signal lost

// ---------------------------------------------------------------------------
// DMX protocol
// ---------------------------------------------------------------------------
const int DMX_CHANNELS = 512;

// DMX receive state machine (runs inside ISR)
enum DmxRxState { DMX_IDLE, DMX_BREAK, DMX_DATA };

volatile DmxRxState dmxState      = DMX_IDLE;
volatile uint16_t   dmxBufIdx     = 0;   // write index into dmxBuf[]
volatile uint16_t   dmxFrameSize  = 0;   // byte count when frame was published
volatile bool       dmxFrameReady = false;
volatile uint8_t    dmxBuf[DMX_CHANNELS];  // working buffer (ISR writes here)
         uint8_t    dmxFrame[DMX_CHANNELS]; // stable copy for main-loop reads

// Updated in main loop only (avoids calling millis() from inside the ISR)
bool          dmxEverReceived = false;
unsigned long dmxLastFrameMs  = 0;

// ---------------------------------------------------------------------------
// UART / DMX initialisation
// ---------------------------------------------------------------------------
// Configures hardware UART0 for DMX512: 250 000 baud, 8 data bits,
// 2 stop bits, no parity.  Arduino's Serial library is NOT used here,
// so there is no conflict with its ISR — we install our own ISR below.
void setupDMX() {
  pinMode(RE_PIN, OUTPUT);
  digitalWrite(RE_PIN, LOW);   // MAX485 RE LOW → receiver enabled
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);   // MAX485 DE LOW → driver disabled (receive-only)

  // Baud rate register: UBRR = F_CPU / (16 × baud) − 1
  // At 16 MHz: 16 000 000 / (16 × 250 000) − 1 = 3
  UBRR0H = 0;
  UBRR0L = 3;

  UCSR0A = 0;                                          // no double-speed mode
  UCSR0B = _BV(RXEN0) | _BV(RXCIE0);                  // enable RX + RX-complete ISR
  UCSR0C = _BV(USBS0) | _BV(UCSZ01) | _BV(UCSZ00);   // 8 data bits, 2 stop bits
}

// ---------------------------------------------------------------------------
// UART RX-complete ISR — drives the DMX state machine
// ---------------------------------------------------------------------------
// The framing-error flag (FE0) is set when the UART detects a BREAK
// (the line is held LOW for longer than one full character frame).
// Reading UCSR0A BEFORE UDR0 captures the correct status for this byte;
// reading UDR0 clears the interrupt and advances the hardware FIFO.
ISR(USART_RX_vect) {
  bool    fe = (UCSR0A & _BV(FE0)) != 0;
  uint8_t b  = UDR0;   // must always be read to clear the interrupt

  if (fe) {
    // BREAK detected: end of the previous frame, start of a new one.
    // Publish whatever channel data we have collected so far.
    if (dmxState == DMX_DATA && dmxBufIdx > 0) {
      dmxFrameSize  = dmxBufIdx;
      dmxFrameReady = true;
    }
    dmxBufIdx = 0;
    dmxState  = DMX_BREAK;
    return;
  }

  switch (dmxState) {
    case DMX_BREAK:
      // First byte after BREAK is the DMX start code.
      // 0x00 = standard dimmer/controller protocol (DMX512-A).
      // Any other start code belongs to a different sub-protocol; skip it.
      dmxState = (b == 0x00) ? DMX_DATA : DMX_IDLE;
      break;

    case DMX_DATA:
      if (dmxBufIdx < (uint16_t)DMX_CHANNELS) {
        dmxBuf[dmxBufIdx++] = b;
      }
      // Publish as soon as all 512 channels have arrived.
      if (dmxBufIdx >= (uint16_t)DMX_CHANNELS) {
        dmxFrameSize  = DMX_CHANNELS;
        dmxFrameReady = true;
        dmxState      = DMX_IDLE;   // wait for the next BREAK
      }
      break;

    default:  // DMX_IDLE — discard byte, waiting for a BREAK
      break;
  }
}

// ---------------------------------------------------------------------------
// Copy the ISR buffer to the stable display buffer
// Returns true when new frame data was copied.
// ---------------------------------------------------------------------------
bool fetchDMXFrame() {
  noInterrupts();
  bool     ready = dmxFrameReady;
  uint16_t count = 0;
  if (ready) {
    count = dmxFrameSize;
    for (uint16_t i = 0; i < count; i++) {
      dmxFrame[i] = dmxBuf[i];
    }
    dmxFrameReady = false;
  }
  interrupts();

  if (ready) {
    dmxLastFrameMs  = millis();
    dmxEverReceived = true;
  }
  return ready;
}

// ---------------------------------------------------------------------------
// Button reading (identical mapping to all other sketches)
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
// Backlight
// ---------------------------------------------------------------------------
void setBacklight(bool on) {
  digitalWrite(BACKLIGHT_PIN, on ? BACKLIGHT_ON_LEVEL : BACKLIGHT_OFF_LEVEL);
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------
int viewChannel = 1;   // 1-indexed channel currently shown (1–512)

// Print a zero-padded three-digit integer.
void printThreeDigits(int val) {
  if (val < 100) lcd.print('0');
  if (val < 10)  lcd.print('0');
  lcd.print(val);
}

// Render the DMX monitor view.
//
// Signal present:
//   Row 0:  Ch:NNN   Val:VVV   (16 chars)
//   Row 1:  [## bar graph ##]  (16 chars — 14 fill segments inside brackets)
//
// No signal:
//   Row 0:  Ch:NNN   Val:---   (16 chars)
//   Row 1:   No DMX signal     (16 chars)
void drawMonitor(bool signalPresent) {
  // ── Row 0: channel number and raw decimal value ───────────────────────
  lcd.setCursor(0, 0);
  lcd.print("Ch:");
  printThreeDigits(viewChannel);      // "001" … "512"
  lcd.print("   Val:");
  if (signalPresent) {
    printThreeDigits((int)dmxFrame[viewChannel - 1]); // "000" … "255"
  } else {
    lcd.print("---");
  }

  // ── Row 1: bar graph or no-signal message ─────────────────────────────
  lcd.setCursor(0, 1);
  if (signalPresent) {
    uint8_t val    = dmxFrame[viewChannel - 1];
    int     filled = (int)val * 14 / 255;  // 0–14 filled segments
    lcd.print('[');
    for (int i = 0; i < 14; i++) lcd.print(i < filled ? '#' : ' ');
    lcd.print(']');
  } else {
    lcd.print(" No DMX signal  ");   // 16 chars
  }
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  pinMode(BACKLIGHT_PIN, OUTPUT);
  setBacklight(true);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("  DMX Monitor   ");   // 16 chars
  lcd.setCursor(0, 1);
  lcd.print("MAX485 / DMX512 ");   // 16 chars
  delay(SPLASH_MS);
  lcd.clear();

  memset(dmxFrame, 0, sizeof(dmxFrame));
  setupDMX();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  const unsigned long now     = millis();
  Button              pressed = getPressEvent();

  // Copy any newly completed DMX frame to the stable display buffer.
  fetchDMXFrame();

  // Determine whether a live DMX signal is present.
  bool signalPresent = dmxEverReceived && (now - dmxLastFrameMs < SIGNAL_TIMEOUT_MS);

  // ── Channel navigation ────────────────────────────────────────────────
  switch (pressed) {
    case BTN_RIGHT:
      viewChannel = (viewChannel < DMX_CHANNELS) ? viewChannel + 1 : 1;
      break;
    case BTN_LEFT:
      viewChannel = (viewChannel > 1) ? viewChannel - 1 : DMX_CHANNELS;
      break;
    case BTN_UP:
      viewChannel += 10;
      if (viewChannel > DMX_CHANNELS) viewChannel = DMX_CHANNELS;
      break;
    case BTN_DOWN:
      viewChannel -= 10;
      if (viewChannel < 1) viewChannel = 1;
      break;
    case BTN_SELECT:
      viewChannel = 1;
      break;
    default:
      break;
  }

  drawMonitor(signalPresent);

  delay(LOOP_DELAY_MS);
}
