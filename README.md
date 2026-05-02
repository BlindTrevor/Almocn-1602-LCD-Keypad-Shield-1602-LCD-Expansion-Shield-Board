# Almocn 1602 LCD Keypad Shield / LCD Expansion Shield Board

An Arduino library demo and reference project for the **Almocn 1602 LCD Keypad Shield** — a 16×2 character LCD display with five built-in navigation buttons (RIGHT, UP, DOWN, LEFT, SELECT) and a software-controllable backlight, designed to plug directly onto an Arduino Uno/Mega header.

---

## Table of Contents

1. [Features](#features)
2. [Hardware Requirements](#hardware-requirements)
3. [Pinout & Connections](#pinout--connections)
4. [Installation](#installation)
5. [Usage](#usage)
6. [Demo Sketch Walkthrough](#demo-sketch-walkthrough)
7. [Menu Sketch Walkthrough](#menu-sketch-walkthrough)
8. [Alarm Clock Sketch Walkthrough](#alarm-clock-sketch-walkthrough)
9. [Pomodoro Timer Sketch Walkthrough](#pomodoro-timer-sketch-walkthrough)
10. [Kitchen Timer Sketch Walkthrough](#kitchen-timer-sketch-walkthrough)
11. [DMX Monitor Sketch Walkthrough](#dmx-monitor-sketch-walkthrough)
12. [Troubleshooting](#troubleshooting)
13. [Contributing](#contributing)
14. [License](#license)

---

## Features

- **16×2 character LCD** driven by the standard Arduino `LiquidCrystal` library (no extra driver needed)
- **Five navigation buttons** — RIGHT, UP, DOWN, LEFT, SELECT — read through a single analog pin via a resistor ladder
- **Software-controlled backlight** on pin D10 with configurable active level (HIGH or LOW)
- **Automatic backlight timeout** to save power when the board is idle
- **Debounced button reading** for reliable key detection
- **Runtime backlight menu** (`Menu/Menu.ino`) — change the timeout duration and enable/disable auto-off on-device without recompiling
- **Alarm clock** (`AlarmClock/AlarmClock.ino`) — manual time setting, configurable alarm with LCD flash and buzzer output on pin D3, snooze, and enable/disable toggle
- **Pomodoro timer** (`PomodoroTimer/PomodoroTimer.ino`) — focused work/break sessions with adjustable durations, pomodoro count, and end-of-period buzzer alert
- **Kitchen timer** (`KitchenTimer/KitchenTimer.ino`) — simple countdown timer: set minutes and seconds, start/pause/reset with keypad buttons, buzzer alert on D3 when time is up
- **DMX monitor** (`DmxMonitor/DmxMonitor.ino`) — receive and display live DMX512 channel values via a MAX485 RS-485 module; navigate all 512 channels with the keypad and view a real-time bar graph
- Compatible with **Arduino Uno, Nano, Mega**, and other 5 V AVR boards

---

## Hardware Requirements

| Item | Notes |
|------|-------|
| Arduino Uno / Nano / Mega | Any 5 V AVR board with matching headers |
| Almocn 1602 LCD Keypad Shield | Or any compatible 1602 shield sharing the same pinout |
| USB cable | For programming and power |
| MAX485 module *(DMX Monitor only)* | RS-485 transceiver module; required only for `DmxMonitor/DmxMonitor.ino` |
| DMX source and XLR cable *(DMX Monitor only)* | Any DMX512 controller or dimmer pack, standard 3- or 5-pin XLR cable |

The shield plugs directly onto the Arduino header — no extra wiring is required.

---

## Pinout & Connections

### LCD (LiquidCrystal 4-bit mode)

| Shield Signal | Arduino Pin |
|---------------|-------------|
| RS            | D8          |
| E (Enable)    | D9          |
| D4            | D4          |
| D5            | D5          |
| D6            | D6          |
| D7            | D7          |

### Buttons (resistor ladder)

| Signal    | Arduino Pin |
|-----------|-------------|
| KEY (ADC) | A0          |

ADC thresholds used by the demo sketch:

| Button  | Approximate ADC value |
|---------|-----------------------|
| RIGHT   | ~0                    |
| UP      | ~100                  |
| DOWN    | ~256                  |
| LEFT    | ~410                  |
| SELECT  | ~640                  |
| NONE    | ~1023                 |

### Backlight

| Signal     | Arduino Pin |
|------------|-------------|
| Backlight  | D10         |

> **Note:** Some shields drive the backlight active-HIGH (write `HIGH` to turn on); others are active-LOW. The `BACKLIGHT_ON_LEVEL` constant in the sketch lets you flip this without changing any other code.

### Available Pins for User Projects

The following Arduino pins are **not used by the shield** and are free for your own circuitry:

| Pin  | Notes                                          |
|------|------------------------------------------------|
| D0   | Available — **used by DMX Monitor (MAX485 RO); disconnect before uploading when wired** |
| D1   | Available                                      |
| D2   | Available — **used by DMX Monitor (MAX485 DE/RE control)** |
| D3   | Available — **buzzer connected by default; use with caution** |
| D11  | Available                                      |
| D12  | Available                                      |
| A1   | Available                                      |
| A2   | Available                                      |
| A3   | Available                                      |
| A4   | Available                                      |
| A5   | Available                                      |

> **⚠️ Warning:** Pin D3 has an onboard buzzer attached. Driving this pin with a conflicting signal or leaving it floating may produce unexpected noise. If you do not need the buzzer, you can desolder it or simply avoid using pin D3.

---

## Installation

### 1. Install the Arduino IDE

Download and install the [Arduino IDE](https://www.arduino.cc/en/software) (version 2.0 or later is recommended).

### 2. Install the LiquidCrystal Library

The `LiquidCrystal` library ships with the Arduino IDE. If it is missing:

1. Open **Sketch → Include Library → Manage Libraries…**
2. Search for **LiquidCrystal**
3. Click **Install**

### 3. Download this project

```bash
git clone https://github.com/BlindTrevor/Almocn-1602-LCD-Keypad-Shield-1602-LCD-Expansion-Shield-Board.git
```

Or download the ZIP from GitHub and extract it.

### 4. Open a sketch

| Sketch        | File                                  | Description |
|---------------|---------------------------------------|-------------|
| Demo          | `Demo/Demo.ino`                       | Basic button display and fixed 5 s backlight timeout |
| Menu          | `Menu/Menu.ino`                       | Runtime menu to set timeout and toggle auto-off |
| AlarmClock    | `AlarmClock/AlarmClock.ino`           | Bedside alarm clock with manual time setting, alarm, snooze, and buzzer |
| PomodoroTimer | `PomodoroTimer/PomodoroTimer.ino`     | Pomodoro productivity timer with adjustable work/break durations, pomodoro count, and end-of-period alert |
| KitchenTimer  | `KitchenTimer/KitchenTimer.ino`       | Countdown kitchen timer: set MM:SS with keypad, start/pause/reset, buzzer alert on D3 |
| DmxMonitor    | `DmxMonitor/DmxMonitor.ino`           | DMX512 monitor: receive live channel values via MAX485, display value + bar graph, navigate all 512 channels |

Open whichever sketch you'd like to try in the Arduino IDE.

### 5. Upload

1. Connect the Arduino (with the shield seated) via USB.
2. Select the correct board under **Tools → Board**.
3. Select the correct port under **Tools → Port**.
4. Click **Upload** (→).

---

## Usage

### Demo sketch

After uploading `Demo/Demo.ino`, the LCD shows the currently pressed button name on the first row and the backlight state on the second row:

```
Btn: RIGHT
Backlight: ON
```

- Press any button to wake the backlight if it has timed out.
- If no button is pressed for **5 seconds** the backlight turns off automatically.

### Menu sketch

After uploading `Menu/Menu.ino`, the LCD starts in the same normal view but shows a menu hint on the second row:

```
Btn: NONE
BL:ON  [MENU]
```

Press **SELECT** to open the settings menu. Two items are available, navigated with **LEFT / RIGHT**:

```
Timeout:
< 5 sec UP/DN >
```

```
Auto-Off:
 ENABLED  UP/DN
```

| Key | Action |
|-----|--------|
| **SELECT** (normal view) | Enter the menu |
| **LEFT / RIGHT** | Move between menu items |
| **UP / DOWN** | Change the highlighted setting |
| **SELECT** (in menu) | Save and return to normal view |

- The backlight stays on while you are in the menu.
- Pressing any button while the backlight is off wakes it; that press is then consumed so you don't accidentally navigate.

### Alarm Clock sketch

After uploading `AlarmClock/AlarmClock.ino`, the LCD shows the current time on the first row and the alarm setting on the second row:

```
12:34:56 [SET]
ALM 06:30  ON
```

Press **SELECT** to enter the settings menu. Five pages are available, navigated with **LEFT / RIGHT**:

```
Set Clock Hour:
< 12 > UP/DN >
```

```
Set Clock Min:
< 34 > UP/DN >
```

```
Set Alarm Hour:
< 06 > UP/DN >
```

```
Set Alarm Min:
< 30 > UP/DN >
```

```
Alarm:
ENABLED  UP/DN
```

| Key | Action |
|-----|--------|
| **SELECT** (normal view) | Enter the settings menu |
| **RIGHT** | Advance to the next settings page |
| **LEFT** | Go back to the previous settings page (or return to clock from first page) |
| **UP / DOWN** | Increase / decrease the displayed value (or toggle alarm on/off on the last page) |
| **SELECT** (in menu) | Save the current value and return to the clock |

When the alarm fires the display switches to:

```
** ALARM TIME **
SEL=off  RT=snz
```

The backlight flashes and pin **D3** toggles HIGH/LOW to drive a buzzer (connect a passive buzzer between D3 and GND).

| Key | Action |
|-----|--------|
| **SELECT** | Dismiss the alarm |
| **RIGHT** | Snooze — silences the alarm and re-triggers after 5 minutes |

> **Hardware note:** The UNO has no real-time clock. Time is tracked with `millis()`, so the clock resets to 00:00:00 each time the board is powered on. Set the correct time after every power cycle using the settings menu.

### Pomodoro Timer sketch

After uploading `PomodoroTimer/PomodoroTimer.ino`, the LCD shows a splash screen then the normal timer view:

```
WORK    25:00
#:00 SEL=start
```

The first row shows the current period label (`WORK`, `BREAK`, or `L.BRK`) and the remaining time as `MM:SS`. The second row shows the number of completed pomodoros (`#:NN`) and a hint for the primary button action.

| Key | Action |
|-----|--------|
| **SELECT** (IDLE) | Start the countdown |
| **SELECT** (RUNNING) | Pause the countdown |
| **SELECT** (PAUSED) | Resume the countdown |
| **RIGHT** | Reset — stops the timer and clears the pomodoro count |
| **LEFT** (IDLE only) | Enter the settings menu |

**Period sequence:**

1. Work session counts down.
2. When it reaches zero the display flashes and the buzzer beeps.
3. Press **SELECT** to acknowledge and start the next break automatically (`BREAK` after sessions 1–3, `L.BRK` after every 4th session).
4. When the break ends, press **SELECT** again to return to a fresh work session.

**Alert view** (end of any period):

```
*  WORK DONE!  *
SEL=next RT=rst
```

or, at the end of a break:

```
* BREAK DONE!  *
SEL=next RT=rst
```

The backlight flashes every 500 ms; the buzzer behaviour depends on the **Buzzer Mode** setting (see Settings menu below).

| Key | Action |
|-----|--------|
| **SELECT** | Advance to the next period |
| **RIGHT** | Full reset — clear the pomodoro count and restart from scratch |

**Settings menu** (press **LEFT** from the idle timer view):

```
Work Duration:
< 25 min UP/DN
```

```
Short Break:
< 05 min UP/DN
```

```
Long Break:
< 15 min UP/DN
```

```
Buzzer Mode:
< SYNC   UP/DN
```

The **Buzzer Mode** page cycles through three options with UP / DOWN:

| Display | Behaviour |
|---------|-----------|
| `OFF`  | Buzzer silent — only the backlight flashes |
| `SYNC` | Buzzer toggles every 500 ms in time with the backlight flash (default) |
| `BEEP` | Two short beeps every 2 s, watch-alarm style |

| Key | Action |
|-----|--------|
| **RIGHT** | Advance to the next settings page |
| **LEFT** | Go back to the previous page (or exit the menu from the first page without saving) |
| **UP / DOWN** | Increase / decrease the displayed duration; **hold** to repeat at an accelerating rate (slow for the first 2 s, then fast) |
| **SELECT** | Save all pages and return to the timer |

> **Hardware note:** Connect a passive piezo buzzer (or a transistor-driven active buzzer) between **D3** and **GND** for end-of-period audio feedback. The sketch works without a buzzer — only the backlight flash fires if no buzzer is wired.

### Kitchen Timer sketch

After uploading `KitchenTimer/KitchenTimer.ino`, the LCD shows a splash screen then the set-time screen, where minutes and seconds are displayed together:

```
Set Timer  05:00
U/D=min LR=sec
```

| Key | Action |
|-----|--------|
| **UP / DOWN** | Increase / decrease the **minutes** (0–99); **hold** to repeat at an accelerating rate |
| **RIGHT / LEFT** | Increase / decrease the **seconds** (0–59); **hold** to repeat at an accelerating rate |
| **SELECT** | Start the countdown immediately |

Once started, the display shows the live countdown:

```
Timer   05:00
SEL=pause RT=rst
```

| Key | Action |
|-----|--------|
| **SELECT** | Pause the countdown |
| **RIGHT** | Reset — stops the timer and returns to the set-time screen |

When paused:

```
Timer   04:47
SEL=resm  RT=rst
```

| Key | Action |
|-----|--------|
| **SELECT** | Resume the countdown |
| **RIGHT** | Reset — returns to the set-time screen |

When the timer reaches zero the display switches to the alert view:

```
** TIME IS UP! *
SEL=reset
```

The backlight flashes and pin **D3** toggles HIGH/LOW to drive the buzzer.

| Key | Action |
|-----|--------|
| **SELECT** | Stop the buzzer and return to the set-time screen |
| **RIGHT** | Stop the buzzer and return to the set-time screen |

> **Hardware note:** Connect a passive piezo buzzer (or a transistor-driven active buzzer) between **D3** and **GND**. The sketch works without a buzzer — only the backlight flashes if no buzzer is wired.

### DMX Monitor sketch

After uploading `DmxMonitor/DmxMonitor.ino` and connecting the MAX485 module (see wiring below), the LCD shows a splash screen then switches to the live monitor view:

```
Ch:001   Val:255
[##############]
```

The first row shows the currently selected DMX channel number and its raw value (0–255). The second row shows a 14-segment bar graph scaled from 0 to full.

If no DMX signal is being received the display changes to:

```
Ch:001   Val:---
 No DMX signal
```

The monitor re-acquires the signal automatically as soon as a valid DMX frame arrives.

| Key | Action |
|-----|--------|
| **RIGHT** | Next channel (+1, wraps 512 → 1) |
| **LEFT** | Previous channel (−1, wraps 1 → 512) |
| **UP** | Jump forward 10 channels |
| **DOWN** | Jump backward 10 channels |
| **SELECT** | Return to channel 1 |

### Adapting the sketch to your project

```cpp
#include <LiquidCrystal.h>

// Match these pins to your shield version
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const int KEY_PIN        = A0;
const int BACKLIGHT_PIN  = 10;

// Set HIGH if your backlight is active-HIGH, LOW if active-LOW
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = LOW;

// Milliseconds before the backlight turns off automatically
const unsigned long BACKLIGHT_TIMEOUT_MS = 5000;
```

Change `BACKLIGHT_ON_LEVEL` to `LOW` if your backlight turns on with a LOW signal.  
Adjust `BACKLIGHT_TIMEOUT_MS` to any number of milliseconds that suits your application.

---

## Demo Sketch Walkthrough

```
Demo/
└── Demo.ino   — standalone Arduino sketch
```

Key functions in `Demo.ino`:

| Function | Purpose |
|----------|---------|
| `readButton(int adc)` | Maps a raw ADC reading to a `Button` enum value using fixed thresholds |
| `buttonName(Button b)` | Returns a human-readable string for display |
| `setBacklight(bool on)` | Writes to `BACKLIGHT_PIN` using the configured active level |
| `setup()` | Initialises the LCD, backlight, and prints a startup message |
| `loop()` | Debounces button input, manages the backlight timeout, and refreshes the display |

---

## Menu Sketch Walkthrough

```
Menu/
└── Menu.ino   — standalone Arduino sketch
```

`Menu.ino` extends `Demo.ino` with a runtime settings menu so you can change the backlight behaviour without recompiling.

### Key functions

| Function | Purpose |
|----------|---------|
| `readButton(int adc)` | Same ADC-to-enum mapping as `Demo.ino` |
| `getPressEvent()` | Edge-detecting wrapper around `readButton` — returns the button only on the rising edge so each physical press fires exactly once |
| `setBacklight(bool on)` | Identical to `Demo.ino` |
| `drawNormal(Button)` | Renders the normal status view |
| `drawMenuTimeout()` | Renders the timeout selection screen |
| `drawMenuAutoOff()` | Renders the auto-off toggle screen |
| `setup()` | Initialises the LCD and shows a brief splash screen |
| `loop()` | Runs the menu state machine and backlight auto-off logic |

### Menu state machine

| State | Description |
|-------|-------------|
| `STATE_NORMAL` | Default view — shows last pressed button and backlight status |
| `STATE_MENU_TIMEOUT` | Timeout picker — UP/DOWN cycles through seven presets |
| `STATE_MENU_AUTOOFF` | Auto-off toggle — UP/DOWN flips between ENABLED and DISABLED |

### Timeout presets

| Label  | Duration  |
|--------|-----------|
| 5 sec  | 5 000 ms  |
| 10 sec | 10 000 ms |
| 15 sec | 15 000 ms |
| 30 sec | 30 000 ms |
| 1 min  | 60 000 ms |
| 2 min  | 120 000 ms |
| 5 min  | 300 000 ms |

### Key constants

```cpp
// Adjust BACKLIGHT_ON_LEVEL only; OFF level is derived automatically
const bool BACKLIGHT_ON_LEVEL  = HIGH;
const bool BACKLIGHT_OFF_LEVEL = !BACKLIGHT_ON_LEVEL;

const unsigned long DEBOUNCE_DELAY_MS     = 40;   // button stability window
const unsigned long SPLASH_SCREEN_DELAY_MS = 1200; // startup message duration
const unsigned long LOOP_DELAY_MS         = 80;   // display refresh interval
```

---

## Alarm Clock Sketch Walkthrough

```
AlarmClock/
└── AlarmClock.ino   — standalone Arduino sketch
```

`AlarmClock.ino` turns the shield into a bedside alarm clock. Because the Arduino Uno has no real-time clock module or network connection, time is tracked entirely in software using `millis()`.

### Key functions

| Function | Purpose |
|----------|---------|
| `readButton(int adc)` | Same ADC-to-enum mapping as `Demo.ino` and `Menu.ino` |
| `getPressEvent()` | Rising-edge button detector — fires once per physical press |
| `setBacklight(bool on)` | Controls backlight via `BACKLIGHT_PIN` |
| `tickClock()` | Increments `clockS`, `clockM`, `clockH` once per second using `millis()` |
| `printTwoDigits(int val)` | Prints a zero-padded two-digit integer to the LCD |
| `drawClock()` | Renders the normal clock view |
| `drawAlarming()` | Renders the alarm-firing view |
| `drawMenuRow0(label)` | Prints a 16-character header row for menu pages |
| `drawMenuRow1(val)` | Prints the value row (`< HH > UP/DN >`) for menu pages |
| `drawMenuAlarmEn()` | Renders the alarm enable/disable menu page |
| `setup()` | Initialises pins, LCD, and shows a splash screen |
| `loop()` | Ticks the clock, checks the alarm, and runs the state machine |

### Application state machine

| State | Description |
|-------|-------------|
| `STATE_CLOCK` | Normal view — shows `HH:MM:SS` and alarm summary |
| `STATE_ALARMING` | Alarm is firing — flashes backlight, toggles buzzer on D3 |
| `STATE_MENU_TIME_H` | Settings page 1 — adjust clock hour |
| `STATE_MENU_TIME_M` | Settings page 2 — adjust clock minute |
| `STATE_MENU_ALARM_H` | Settings page 3 — adjust alarm hour |
| `STATE_MENU_ALARM_M` | Settings page 4 — adjust alarm minute |
| `STATE_MENU_ALARM_EN` | Settings page 5 — enable or disable the alarm |

### Alarm behaviour

- The alarm fires once per day when `clockH == alarmH` and `clockM == alarmM` at the start of that minute (i.e. when `clockS` rolls to `0`).
- While firing, the backlight flashes every 500 ms and pin **D3** toggles every 500 ms.
- Pressing **SELECT** dismisses the alarm for that minute.
- Pressing **RIGHT** snoozes: the alarm time is shifted forward by `SNOOZE_MINUTES` (default 5) and the alarm fires again at the new time.

### Key constants

```cpp
const int  BUZZER_PIN       = 3;    // buzzer between D3 and GND
const int  SNOOZE_MINUTES   = 5;    // snooze duration in minutes
const unsigned long FLASH_INTERVAL_MS = 500; // backlight flash half-period
const unsigned long BUZZ_INTERVAL_MS  = 500; // buzzer toggle half-period
```

### Wiring the buzzer

Connect a passive piezo buzzer (or a transistor-driven active buzzer) between **D3** and **GND**. No additional components are needed for a passive piezo; for louder output use an NPN transistor (e.g. 2N2222) with a 1 kΩ base resistor.

---

## Pomodoro Timer Sketch Walkthrough

```
PomodoroTimer/
└── PomodoroTimer.ino   — standalone Arduino sketch
```

`PomodoroTimer.ino` implements the [Pomodoro Technique](https://en.wikipedia.org/wiki/Pomodoro_Technique): focused work sessions separated by short breaks, with a longer break after every fourth completed session. All durations are adjustable at runtime through the built-in settings menu.

### Key functions

| Function | Purpose |
|----------|---------|
| `readButton(int adc)` | Same ADC-to-enum mapping as all other sketches |
| `getButtonEvent()` | Button event detector — fires once per press for all buttons; fires repeatedly at an accelerating rate while UP / DOWN are held |
| `setBacklight(bool on)` | Controls backlight via `BACKLIGHT_PIN` |
| `tickTimer()` | Decrements `secsRemaining` by 1 once per second using `millis()` |
| `loadSettings()` | Reads work/short/long durations and buzzer mode from EEPROM; falls back to compile-time defaults on first boot or if the magic byte is wrong |
| `saveSettings()` | Writes the four settings to EEPROM using `EEPROM.update()` (write-only-if-changed) |
| `loadPeriodTime()` | Sets `secsRemaining` to the configured duration for `currentPeriod` |
| `advancePeriod()` | Increments the pomodoro count (on work completion) and switches to the next period |
| `resetTimer()` | Clears the pomodoro count and restarts from a fresh work session |
| `printTwoDigits(int val)` | Prints a zero-padded two-digit integer to the LCD |
| `periodLabel()` | Returns the 6-character padded label for the current period |
| `drawTimer()` | Renders the normal timer view (IDLE / RUNNING / PAUSED) |
| `drawAlert()` | Renders the end-of-period alert view |
| `drawMenuRow0(label)` | Prints a 16-character header row for settings pages |
| `drawMenuRow1(val)` | Prints the value row (`< NN min UP/DN  `) for duration settings pages |
| `drawMenuRowBuzz(mode)` | Prints the value row (`< SYNC   UP/DN  ` etc.) for the buzzer-mode page |
| `setup()` | Initialises pins, LCD, and shows a splash screen |
| `loop()` | Ticks the countdown, checks for period completion, and runs the state machine |

### Application state machine

| State | Description |
|-------|-------------|
| `STATE_IDLE` | Timer ready — shows countdown at period start; SELECT starts it |
| `STATE_RUNNING` | Countdown active — SELECT pauses, RIGHT resets |
| `STATE_PAUSED` | Countdown frozen — SELECT resumes, RIGHT resets |
| `STATE_ALERT` | Period ended — backlight flashes, buzzer fires per mode; SELECT advances, RIGHT resets |
| `STATE_MENU_WORK` | Settings page 1 — adjust work duration (1–60 min) |
| `STATE_MENU_SHORT` | Settings page 2 — adjust short-break duration (1–30 min) |
| `STATE_MENU_LONG` | Settings page 3 — adjust long-break duration (1–60 min) |
| `STATE_MENU_BUZZ` | Settings page 4 — choose buzzer mode (OFF / SYNC / BEEP) |

### Period sequence

```
WORK → SHORT_BREAK → WORK → SHORT_BREAK → WORK → SHORT_BREAK → WORK → LONG_BREAK → (repeat)
  1          1          2          2          3          3          4         4
```

`POMODOROS_PER_LONG` (default `4`) controls how many completed work sessions trigger a long break.

### Pause accuracy

`timerLastTickMs` is reset to `millis()` every time the timer starts or resumes. This ensures the first tick after a resume fires exactly one second later rather than immediately, keeping the countdown accurate across any number of pause/resume cycles.

### Key constants

```cpp
const int DEFAULT_WORK_MIN   = 25;  // default work session length (minutes)
const int DEFAULT_SHORT_MIN  = 5;   // default short break length (minutes)
const int DEFAULT_LONG_MIN   = 15;  // default long break length (minutes)
const int POMODOROS_PER_LONG = 4;   // work sessions before a long break
const int BUZZER_PIN         = 3;   // buzzer between D3 and GND
const unsigned long FLASH_INTERVAL_MS  = 500;  // backlight flash half-period
const unsigned long BUZZ_INTERVAL_MS   = 500;  // SYNC mode: buzzer toggle half-period
const unsigned long BEEP_ON_MS    = 120;  // BEEP mode: duration of each beep
const unsigned long BEEP_GAP_MS   = 120;  // BEEP mode: gap between the two beeps
const unsigned long BEEP_CYCLE_MS = 2000; // BEEP mode: full repetition period
const unsigned long HOLD_DELAY_MS      = 500;  // pause before UP/DOWN repeat begins
const unsigned long HOLD_REPEAT_MS     = 200;  // slow repeat interval
const unsigned long HOLD_FAST_MS       = 80;   // fast repeat interval
const unsigned long HOLD_FAST_AFTER_MS = 2000; // switch to fast rate after this many ms
```

### EEPROM persistence

The four adjustable settings (work, short break, long break, buzzer mode) are saved to internal EEPROM whenever the settings menu is confirmed. They are restored automatically on every power-on, so your custom settings survive resets and power cycles.

| EEPROM address | Content |
|----------------|---------|
| 0 | Magic byte (`0xA7`) — marks the EEPROM as written by this sketch |
| 1 | Work duration (minutes) |
| 2 | Short-break duration (minutes) |
| 3 | Long-break duration (minutes) |
| 4 | Buzzer mode (`0`=OFF, `1`=SYNC, `2`=BEEP) |

`saveSettings()` uses `EEPROM.update()`, which only writes a byte when its stored value has changed. This minimises wear on EEPROM cells (rated for approximately 100 000 write cycles).

On first boot (or after flashing to a different board), the magic byte at address 0 will not match `EEPROM_MAGIC`, so the sketch silently falls back to the compile-time defaults. Each stored value is also range-checked individually: if a byte is outside the allowed range for that setting, only that field reverts to its default.

> **Factory reset:** To force the sketch to revert to defaults, change `EEPROM_MAGIC` to any different value (e.g. `0xA8`) and re-upload. The mismatched magic byte will cause `loadSettings()` to ignore the stored values on the next boot.

---

## Kitchen Timer Sketch Walkthrough

```
KitchenTimer/
└── KitchenTimer.ino   — standalone Arduino sketch
```

`KitchenTimer.ino` turns the shield into a simple kitchen countdown timer. The user sets a duration in minutes and seconds using the keypad, starts the countdown, and is alerted with a flashing backlight and buzzer when time is up.

### Key functions

| Function | Purpose |
|----------|---------|
| `readButton(int adc)` | Same ADC-to-enum mapping as all other sketches |
| `getButtonEvent()` | Button event detector — fires once per press for SELECT; fires repeatedly at an accelerating rate while UP / DOWN / LEFT / RIGHT are held |
| `setBacklight(bool on)` | Controls backlight via `BACKLIGHT_PIN` |
| `tickTimer()` | Decrements `secsRemaining` by 1 once per second using `millis()` |
| `printTwoDigits(int val)` | Prints a zero-padded two-digit integer to the LCD |
| `drawSetTimer()` | Renders the set-time screen showing `MM:SS` with button hints |
| `drawTimer(hint)` | Renders the live countdown view (RUNNING / PAUSED) |
| `drawDone()` | Renders the alert view when the timer reaches zero |
| `startTimer()` | Loads `secsRemaining` from `setMinutes`/`setSeconds` and transitions to `STATE_RUNNING` |
| `resetToSet()` | Silences the buzzer, restores the backlight, and returns to `STATE_SET` |
| `setup()` | Initialises pins, LCD, and shows a splash screen |
| `loop()` | Ticks the countdown, handles button events, and runs the state machine |

### Application state machine

| State | Description |
|-------|-------------|
| `STATE_SET` | Set-time screen — UP/DOWN adjust minutes (0–99), LEFT/RIGHT adjust seconds (0–59), SELECT starts |
| `STATE_RUNNING` | Countdown active — SELECT pauses, RIGHT resets |
| `STATE_PAUSED` | Countdown frozen — SELECT resumes, RIGHT resets |
| `STATE_DONE` | Timer reached zero — backlight flashes, buzzer fires; SELECT or RIGHT resets |

### Alert behaviour

- When `secsRemaining` reaches `0` the sketch transitions to `STATE_DONE`.
- The backlight flashes every `FLASH_INTERVAL_MS` (500 ms).
- Pin **D3** toggles every `BUZZ_INTERVAL_MS` (500 ms).
- Both the backlight and buzzer stop as soon as SELECT or RIGHT is pressed.

### Pause accuracy

`timerLastTickMs` is reset to `millis()` every time the timer starts or resumes. This ensures the first tick after a resume fires exactly one second later, keeping the displayed time accurate across any number of pause/resume cycles.

### Key constants

```cpp
const int BUZZER_PIN             = 3;    // buzzer between D3 and GND
const int MAX_MINUTES            = 99;   // maximum settable minutes
const int MAX_SECONDS            = 59;   // maximum settable seconds
const unsigned long FLASH_INTERVAL_MS = 500;  // backlight flash half-period
const unsigned long BUZZ_INTERVAL_MS  = 500;  // buzzer toggle half-period
const unsigned long HOLD_DELAY_MS      = 500;  // pause before directional-button repeat begins
const unsigned long HOLD_REPEAT_MS     = 200;  // slow repeat interval
const unsigned long HOLD_FAST_MS       = 80;   // fast repeat interval
const unsigned long HOLD_FAST_AFTER_MS = 2000; // switch to fast rate after this many ms
```

### Wiring the buzzer

Connect a passive piezo buzzer (or a transistor-driven active buzzer) between **D3** and **GND**. No additional components are needed for a passive piezo; for louder output use an NPN transistor (e.g. 2N2222) with a 1 kΩ base resistor.

---

## DMX Monitor Sketch Walkthrough

```
DmxMonitor/
└── DmxMonitor.ino   — standalone Arduino sketch
```

`DmxMonitor.ino` turns the shield into a live DMX512 signal monitor. It receives the RS-485 differential signal through a MAX485 module connected to the Arduino hardware UART, then displays each channel's value and a proportional bar graph on the LCD. All 512 channels are accessible with the keypad.

### MAX485 wiring

| MAX485 pin | Arduino pin | Notes |
|------------|-------------|-------|
| RO         | D0 (RX)     | Receiver Output → hardware UART RX |
| RE         | D2          | Receiver Enable (active LOW) — tie to DE |
| DE         | D2          | Driver Enable (active HIGH) — tie to RE; both held LOW = receive-only |
| DI         | —           | Driver Input, not connected (receive only) |
| A          | DMX+        | Non-inverting RS-485 line (XLR 3-pin: pin 3) |
| B          | DMX−        | Inverting RS-485 line (XLR 3-pin: pin 2) |
| VCC        | 5 V         | |
| GND        | GND         | |

> **Termination:** Fit a 120 Ω resistor across the A and B terminals on the last device on the DMX run.
>
> **XLR pinout (3-pin):** pin 1 = GND/shield, pin 2 = DMX− (B), pin 3 = DMX+ (A).
>
> **Upload warning:** D0 is shared with the USB–serial upload link. Disconnect the MAX485 RO wire from D0 **before** clicking Upload in the Arduino IDE, then reconnect it once upload completes.

### Arduino Mega note

On a Mega, connect MAX485 RO to **D19** (Serial1 RX) so the USB serial port remains free. In `setupDMX()` replace `UBRR0H`, `UBRR0L`, `UCSR0A`, `UCSR0B`, `UCSR0C` with `UBRR1H`, `UBRR1L`, `UCSR1A`, `UCSR1B`, `UCSR1C`, and change the ISR name from `USART_RX_vect` to `USART1_RX_vect`.

### How DMX512 reception works

DMX512 is a unidirectional RS-485 serial protocol running at **250 000 baud** (8 data bits, 2 stop bits, no parity). Each universe packet begins with a **BREAK** — the transmitter holds the line LOW for at least 88 µs, long enough for the UART to report a **framing error** (FE0 bit in UCSR0A). The sketch detects this framing error as the start-of-packet signal, reads the **start code** byte (0x00 for standard DMX), then collects up to 512 channel bytes.

The UART is configured directly via the AVR hardware registers (not through Arduino's `Serial` library) so the framing error flag can be read reliably before each byte is consumed.

### Key functions

| Function | Purpose |
|----------|---------|
| `setupDMX()` | Configures UART0 for 250 000 baud, 8N2, receiver-only; drives DE/RE LOW |
| `ISR(USART_RX_vect)` | Fires on every received byte; checks FE0 for BREAK, then runs the DMX state machine |
| `fetchDMXFrame()` | Atomically copies the ISR buffer to the stable display buffer when a new frame is ready |
| `readButton(int adc)` | Same ADC-to-enum mapping as all other sketches |
| `getPressEvent()` | Rising-edge button detector — fires once per physical press |
| `setBacklight(bool on)` | Controls backlight via `BACKLIGHT_PIN` |
| `printThreeDigits(int val)` | Prints a zero-padded three-digit integer to the LCD |
| `drawMonitor(bool signalPresent)` | Renders row 0 (channel number + value) and row 1 (bar graph or no-signal message) |
| `setup()` | Initialises pins, LCD, shows splash screen, and starts the DMX receiver |
| `loop()` | Calls `fetchDMXFrame()`, handles button navigation, and refreshes the display |

### Application state machine

The DMX receiver uses an internal three-state machine, driven entirely inside the ISR:

| State | Description |
|-------|-------------|
| `DMX_IDLE` | Waiting for the next BREAK; incoming bytes are discarded |
| `DMX_BREAK` | BREAK received — next byte is the start code |
| `DMX_DATA` | Collecting channel bytes into `dmxBuf[]` until 512 bytes arrive or a new BREAK occurs |

The main loop runs independently: it calls `fetchDMXFrame()` each iteration to atomically snapshot the ISR buffer, checks `dmxLastFrameMs` to decide whether the signal is live, and redraws the LCD every `LOOP_DELAY_MS` (80 ms).

### Double-buffer design

| Buffer | Who writes | Who reads | Notes |
|--------|------------|-----------|-------|
| `dmxBuf[512]` (volatile) | ISR | `fetchDMXFrame()` | Filled byte-by-byte as the frame arrives |
| `dmxFrame[512]` | `fetchDMXFrame()` | `drawMonitor()` | Stable snapshot; only updated on a complete or partial-but-ended frame |

`fetchDMXFrame()` disables interrupts (`noInterrupts()`) while copying and clearing the ready flag, preventing the ISR from modifying `dmxBuf[]` mid-copy.

### Signal-loss detection

`dmxLastFrameMs` is updated in the main loop every time `fetchDMXFrame()` returns a new frame. If `millis() − dmxLastFrameMs` exceeds `SIGNAL_TIMEOUT_MS` (1 000 ms, approximately 44 missed frames), `signalPresent` becomes false and the LCD switches to the no-signal view. The monitor recovers immediately when frames resume.

### Key constants

```cpp
const int              DMX_CHANNELS      = 512;    // number of DMX channel slots
const unsigned long    SIGNAL_TIMEOUT_MS = 1000;   // ms without a frame → signal lost
const unsigned long    LOOP_DELAY_MS     = 80;     // LCD refresh interval
// UBRR0L = 3 → 250 000 baud at 16 MHz (adjust for other clock speeds)
```

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| LCD shows garbled characters | Contrast too low/high | Adjust the contrast potentiometer on the back of the shield |
| LCD shows no characters (blank) | Contrast set to one extreme | Turn the contrast pot slowly until text appears |
| Backlight stays off | `BACKLIGHT_ON_LEVEL` is wrong | Change `BACKLIGHT_ON_LEVEL` from `HIGH` to `LOW` (or vice-versa) |
| Buttons read the wrong value | ADC thresholds don't match your board | Print raw ADC values in `loop()` and update the thresholds in `readButton()` |
| Upload fails | Wrong board/port selected | Re-check **Tools → Board** and **Tools → Port** |
| `LiquidCrystal.h` not found | Library not installed | Install via **Sketch → Include Library → Manage Libraries…** |
| Buzzer silent when alarm fires | No buzzer wired to D3 | Connect a passive piezo between D3 and GND |
| Buzzer silent at end of Pomodoro period | No buzzer wired to D3 | Connect a passive piezo between D3 and GND |
| Buzzer silent when kitchen timer expires | No buzzer wired to D3 | Connect a passive piezo between D3 and GND |
| Clock drifts noticeably | `millis()` accuracy limitation (no RTC) | Re-set the time after long periods; consider adding a DS3231 RTC module for precision |
| DMX Monitor shows "No DMX signal" | No DMX frame received / D0 not wired | Check MAX485 A/B wiring and XLR connections; verify the DMX source is transmitting; confirm the RO wire is connected to D0 after uploading |
| DMX Monitor upload fails or hangs | MAX485 RO still connected to D0 | Disconnect the RO wire from D0 before uploading; reconnect afterwards |
| DMX values stuck at 0 or wrong | Incorrect baud rate (non-16 MHz board) | Recalculate `UBRR0L` using `F_CPU / (16 × 250000) − 1` for your clock speed |
| DMX values flickering / garbled | Missing 120 Ω termination resistor | Add a 120 Ω resistor across MAX485 A and B terminals |

---

## Contributing

Contributions are welcome! To contribute:

1. **Fork** the repository on GitHub.
2. Create a feature branch: `git checkout -b feature/my-improvement`
3. Commit your changes with a clear message: `git commit -m "Add: description of change"`
4. Push to your fork: `git push origin feature/my-improvement`
5. Open a **Pull Request** against the `main` branch and describe what you changed and why.

Please keep sketches compatible with the Arduino IDE and avoid introducing additional dependencies unless necessary.

---

## License

This project is released under the **MIT License**:

> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files, to deal in the software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the software.

---

*Happy making! If you find a bug or have a suggestion, please open an [issue](https://github.com/BlindTrevor/Almocn-1602-LCD-Keypad-Shield-1602-LCD-Expansion-Shield-Board/issues).*