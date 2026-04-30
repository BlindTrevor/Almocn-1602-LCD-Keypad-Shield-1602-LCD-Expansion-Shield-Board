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
7. [Troubleshooting](#troubleshooting)
8. [Contributing](#contributing)
9. [License](#license)

---

## Features

- **16×2 character LCD** driven by the standard Arduino `LiquidCrystal` library (no extra driver needed)
- **Five navigation buttons** — RIGHT, UP, DOWN, LEFT, SELECT — read through a single analog pin via a resistor ladder
- **Software-controlled backlight** on pin D10 with configurable active level (HIGH or LOW)
- **Automatic backlight timeout** to save power when the board is idle
- **Debounced button reading** for reliable key detection
- Compatible with **Arduino Uno, Nano, Mega**, and other 5 V AVR boards

---

## Hardware Requirements

| Item | Notes |
|------|-------|
| Arduino Uno / Nano / Mega | Any 5 V AVR board with matching headers |
| Almocn 1602 LCD Keypad Shield | Or any compatible 1602 shield sharing the same pinout |
| USB cable | For programming and power |

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

### 4. Open the sketch

Open `Demo/Demo.ino` in the Arduino IDE.

### 5. Upload

1. Connect the Arduino (with the shield seated) via USB.
2. Select the correct board under **Tools → Board**.
3. Select the correct port under **Tools → Port**.
4. Click **Upload** (→).

---

## Usage

After uploading, the LCD shows the currently pressed button name on the first row and the backlight state on the second row:

```
Btn: RIGHT
Backlight: ON
```

- Press any button to wake the backlight if it has timed out.
- If no button is pressed for **5 seconds** the backlight turns off automatically.

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

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| LCD shows garbled characters | Contrast too low/high | Adjust the contrast potentiometer on the back of the shield |
| LCD shows no characters (blank) | Contrast set to one extreme | Turn the contrast pot slowly until text appears |
| Backlight stays off | `BACKLIGHT_ON_LEVEL` is wrong | Change `BACKLIGHT_ON_LEVEL` from `HIGH` to `LOW` (or vice-versa) |
| Buttons read the wrong value | ADC thresholds don't match your board | Print raw ADC values in `loop()` and update the thresholds in `readButton()` |
| Upload fails | Wrong board/port selected | Re-check **Tools → Board** and **Tools → Port** |
| `LiquidCrystal.h` not found | Library not installed | Install via **Sketch → Include Library → Manage Libraries…** |

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