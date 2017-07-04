#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#include <libmsp/watchdog.h>
#include <libmsp/clock.h>
#include <libmsp/gpio.h>
#include <libmsp/periph.h>
#include <libmsp/uart.h>
#include <libmsp/sleep.h>
#include <libio/console.h>

#include "pins.h"

#define MAX_PAYLOAD_LEN 25
uint8_t pkt[1 + MAX_PAYLOAD_LEN];

int main() {
    msp_watchdog_disable();
    msp_gpio_unlock();

    __enable_interrupt();

    msp_clock_setup();

    INIT_CONSOLE();
    LOG("CAPYBARA v1.0\r\n");

    msp_uart_open();

    uint8_t len = 1;

    while (1) {
        msp_sleep(16000);

        if (len == MAX_PAYLOAD_LEN)
            len = 1;

        pkt[0] = len;
        for (int i = 1; i < len + 1; ++i)
            pkt[i] = '0' + (i % 10);

        msp_uart_send_sync(pkt, len + 1);

        ++len;

    }

    msp_uart_close();

    while(1) {
        __bis_SR_register(LPM4_bits);
    }
}
