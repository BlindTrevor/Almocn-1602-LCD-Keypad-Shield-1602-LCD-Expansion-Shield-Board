#ifndef BUTTON_TYPE_H
#define BUTTON_TYPE_H

// Button identifiers for the Almocn 1602 LCD Keypad Shield ADC keypad.
// This enum is kept in its own header so that the Arduino IDE preprocessor
// can see the type definition before it auto-generates function prototypes
// for readButton() and getPressEvent().
enum Button { BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT, BTN_NONE };

#endif // BUTTON_TYPE_H
