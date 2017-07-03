#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#include <libmsp/watchdog.h>
#include <libmsp/clock.h>
#include <libmsp/gpio.h>
#include <libmsp/periph.h>
#include <libio/console.h>

#include "pins.h"

int main() {
    msp_watchdog_disable();
    msp_gpio_unlock();

    __enable_interrupt();

    msp_clock_setup();

    INIT_CONSOLE();
    LOG("CAPYBARA v1.0\r\n");

    while(1) {
        __bis_SR_register(LPM4_bits);
    }
}
