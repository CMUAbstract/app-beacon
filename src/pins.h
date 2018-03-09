#ifndef PINS_H
#define PINS_H

#if BOARD_MAJOR == 1

#define PORT_DBG 3
#define PIN_DBG0 5
#define PIN_DBG1 6
#define PIN_DBG2 7

#if BOARD_MINOR == 0

#define PORT_RADIO_SW 3
#define PIN_RADIO_SW  2

//#define PORT_RADIO_RST ?
//#define PIN_RADIO_RST  ?
#error TODO: Define RADIO_RST pin for Capybara v1.0 (or is it not wired in HW?)

#elif BOARD_MINOR == 1
// GPIO extender pins
#define BIT_RADIO_RST (1 << 6)
#define BIT_RADIO_SW  (1 << 7)

#endif // BOARD_MINOR

#elif BOARD_MAJOR == 2

// Overloaded onto part of EDB AUX pins
#define PORT_DBG 1
#define PIN_DBG0 3
#define PIN_DBG1 4
#define PIN_DBG2 5

#define PORT_RADIO_SW 4
#define PIN_RADIO_SW  0

#define PORT_RADIO_RST 4
#define PIN_RADIO_RST  1

#else // BOARD_MAJOR
#error Unsupported board: do not have pin definitions (see BOARD var)
#endif // BOARD_MAJOR

#endif // PINS_H
