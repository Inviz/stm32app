#include "gpio.h"

void gpio_enable_port(uint8_t port) {
    rcc_periph_clock_enable(RCC_GPIOX(port));
}

void gpio_configure_input(uint8_t port, uint8_t pins) {
    #ifdef STM32F1
        gpio_set_mode(GPIOX(port), GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, pins);
    #else
        gpio_mode_setup(GPIOX(port), GPIO_MODE_INPUT, GPIO_MODE_ANALOG, pins);
    #endif
}

void gpio_configure_output(uint8_t port, uint8_t pins) {
    #ifdef STM32F1
        gpio_set_mode(GPIOX(port), GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, pins);
    #else
        gpio_mode_setup(GPIOX(port), GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, pins);
    #endif
}

void gpio_set_state(uint8_t port, uint8_t pins, uint8_t state) {
    if (state == 0) {
        gpio_clear(GPIOX(port), 1 << pins);
    } else {
        gpio_set(GPIOX(port), 1 << pins);
    }
}

