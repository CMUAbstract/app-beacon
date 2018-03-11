#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

#include <libmsp/watchdog.h>
#include <libmsp/clock.h>
#include <libmsp/gpio.h>
#include <libmsp/periph.h>
#include <libmsp/uart.h>
#include <libmsp/sleep.h>
#include <libmsp/mem.h>
#include <libio/console.h>
#include <libcapybara/power.h>
#include <libcapybara/capybara.h>
#include <libmspuartlink/uartlink.h>

#include "pins.h"

#define BEACON_INTERVAL 8000 // ~2 seconds @ 32768/8 Hz

#define RADIO_ON_CYCLES   60 // ~12ms (a bundle of 2-3 pkts 25 payload bytes each on Pin=0)
#define RADIO_BOOT_CYCLES 60
#define RADIO_RST_CYCLES   1

typedef enum __attribute__((packed)) {
    RADIO_CMD_SET_ADV_PAYLOAD = 0,
} radio_cmd_t;

#define RADIO_PAYLOAD_LEN 4 /* chosen by the app */
typedef struct __attribute__((packed)) {
    radio_cmd_t cmd;
    uint8_t payload[RADIO_PAYLOAD_LEN];
} radio_pkt_t;

static radio_pkt_t radio_pkt;

static __nv uint8_t counter; // to generate dummy payload

static inline void radio_pin_init()
{
#if BOARD_MAJOR == 1 && BOARD_MINOR == 0
    GPIO(PORT_SENSE_SW, OUT) &= ~BIT(PIN_SENSE_SW);
    GPIO(PORT_SENSE_SW, DIR) |= BIT(PIN_SENSE_SW);

    GPIO(PORT_RADIO_SW, OUT) &= ~BIT(PIN_RADIO_SW);
    GPIO(PORT_RADIO_SW, DIR) |= BIT(PIN_RADIO_SW);
#elif BOARD_MAJOR == 1 && BOARD_MINOR == 1

    fxl_out(BIT_RADIO_SW | BIT_RADIO_RST);
    fxl_pull_up(BIT_CCS_WAKE);
    // SENSE_SW is present but is not electrically correct: do not use.
#elif BOARD_MAJOR == 2 && BOARD_MINOR == 0
    GPIO(PORT_RADIO_SW, OUT) &= ~BIT(PIN_RADIO_SW);
    GPIO(PORT_RADIO_SW, DIR) |= BIT(PIN_RADIO_SW);
#else // BOARD_{MAJOR,MINOR}
#error Unsupported board: do not know what pins to configure (see BOARD var)
#endif // BOARD_{MAJOR,MINOR}
}

static inline void radio_on()
{
#if (BOARD_MAJOR == 1 && BOARD_MINOR == 0) || (BOARD_MAJOR == 2 && BOARD_MINOR == 0)

#if PORT_RADIO_SW != PORT_RADIO_RST // we assume this below
#error Unexpected pin config: RAD_SW and RAD_RST not on same port
#endif // PORT_RADIO_SW != PORT_RADIO_RST

    GPIO(PORT_RADIO_SW, OUT) |= BIT(PIN_RADIO_SW) | BIT(PIN_RADIO_RST);
    GPIO(PORT_RADIO_RST, OUT) &= ~BIT(PIN_RADIO_RST);

#elif BOARD_MAJOR == 1 && BOARD_MINOR == 1
    // Assert reset and give it time before turning on power, to make sure that
    // radio doesn't power on while reset is (not yet) asserted and starts.
    fxl_set(BIT_RADIO_RST);
    msp_sleep(RADIO_RST_CYCLES);
    fxl_set(BIT_RADIO_SW);
    msp_sleep(RADIO_RST_CYCLES);
    fxl_clear(BIT_RADIO_RST);

#else // BOARD_{MAJOR,MINOR}
#error Unsupported board: do not know how to turn off radio (see BOARD var)
#endif // BOARD_{MAJOR,MINOR}
}

static inline void radio_off()
{
#if (BOARD_MAJOR == 1 && BOARD_MINOR == 0) || (BOARD_MAJOR == 2 && BOARD_MINOR == 0)
    GPIO(PORT_RADIO_RST, OUT) |= BIT(PIN_RADIO_RST); // reset for clean(er) shutdown
    msp_sleep(RADIO_RST_CYCLES);
    GPIO(PORT_RADIO_SW, OUT) &= ~BIT(PIN_RADIO_SW);
#elif BOARD_MAJOR == 1 && BOARD_MINOR == 1
    fxl_set(BIT_RADIO_RST); // reset for clean(er) shutdown
    msp_sleep(RADIO_RST_CYCLES);
    fxl_clear(BIT_RADIO_SW);
#else // BOARD_{MAJOR,MINOR}
#error Unsupported board: do not know how to turn on radio (see BOARD var)
#endif // BOARD_{MAJOR,MINOR}
}

int main() {
    msp_watchdog_disable();
    msp_gpio_unlock();

    __enable_interrupt();

    capybara_config_pins();
    capybara_wait_for_supply();
    capybara_shutdown_on_deep_discharge();

    msp_clock_setup();

    INIT_CONSOLE();
    LOG("CAPYBARA v1.0\r\n");

#if BOARD_MAJOR == 1 && BOARD_MINOR == 1
    LOG2("i2c init\r\n");
    i2c_setup();

    LOG2("fxl init\r\n");
    fxl_init();
#endif // BOARD == v1.1

    radio_pin_init();

    while (1) {

        radio_pkt.cmd = RADIO_CMD_SET_ADV_PAYLOAD;
        for (int j = 0; j < RADIO_PAYLOAD_LEN; ++j) {
            radio_pkt.payload[j] = counter++;
        }

        LOG2("TX PKT (cmd %u len %u):\r\n", radio_pkt.cmd, RADIO_PAYLOAD_LEN);
        int j;
        for (j = 0; j < RADIO_PAYLOAD_LEN; j += 16) {
            for (int c = 0; c < 16 && j + c < RADIO_PAYLOAD_LEN; ++c)
                LOG2("%02i ", (int)radio_pkt.payload[j + c]);
            LOG2("\r\n");
        }
        for (; j < RADIO_PAYLOAD_LEN % 16; ++j)
            LOG2("%i ", (int)radio_pkt.payload[j]);
        LOG2("\r\n");

        radio_on();

        msp_sleep(RADIO_BOOT_CYCLES); // ~15ms @ ACLK/8

        uartlink_open_tx();
        uartlink_send((uint8_t *)&radio_pkt, sizeof(radio_pkt));
        uartlink_close();

        // TODO: wait until radio is finished; for now, wait blindly
        msp_sleep(RADIO_ON_CYCLES);

        radio_off();

        msp_sleep(BEACON_INTERVAL);
    }
}

#define _THIS_PORT 2
ISR(GPIO_VECTOR(_THIS_PORT))
{
    switch (__even_in_range(INTVEC(_THIS_PORT), INTVEC_RANGE(_THIS_PORT))) {
#if BOARD_MAJOR == 1 && BOARD_MINOR == 0
#if LIBCAPYBARA_PORT_VBOOST_OK == _THIS_PORT
        case INTFLAG(LIBCAPYBARA_PORT_VBOOST_OK, LIBCAPYBARA_PIN_VBOOST_OK):
            capybara_vboost_ok_isr();
            break;
#else
#error Handler in wrong ISR: capybara_vboost_ok_isr
#endif // LIBCAPYBARA_PORT_VBOOST_OK
#endif // BOARD_{MAJOR,MINOR}
    }
}
#undef _THIS_PORT

#define _THIS_PORT 3
ISR(GPIO_VECTOR(_THIS_PORT))
{
    switch (__even_in_range(INTVEC(_THIS_PORT), INTVEC_RANGE(_THIS_PORT))) {
#if (BOARD_MAJOR == 1 && BOARD_MINOR == 1) || (BOARD_MAJOR == 2 && BOARD_MINOR == 0)
#if LIBCAPYBARA_PORT_VBOOST_OK == _THIS_PORT
        case INTFLAG(LIBCAPYBARA_PORT_VBOOST_OK, LIBCAPYBARA_PIN_VBOOST_OK):
            capybara_vboost_ok_isr();
            break;
#else
#error Handler in wrong ISR: capybara_vboost_ok_isr
#endif // LIBCAPYBARA_PORT_VBOOST_OK
#endif // BOARD_{MAJOR,MINOR}
    }
}
#undef _THIS_PORT
