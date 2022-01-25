#include "gpio.h"

void gpio_enable_port(uint8_t port) {
    rcc_periph_clock_enable(RCC_GPIOX(port));
}
uint8_t gpio_get_speed_setting(uint8_t speed) {
    switch (speed) {
    case GPIO_MAX:
    case 100: return GPIO_OSPEED_100MHZ;
    case GPIO_FAST:
    case 50: return GPIO_OSPEED_50MHZ;
    case GPIO_MEDIUM:
    case 25: return GPIO_OSPEED_25MHZ;
    case GPIO_SLOW:
    case 2:
    default: return GPIO_OSPEED_2MHZ;
    }
}

void gpio_configure_input(uint8_t port, uint8_t pin) {
    log_printf("\t[%c%u]\tinput\n", (char)(65 + port - 1), pin);
    gpio_enable_port(port);
#ifdef STM32F1
    gpio_set_mode(GPIOX(port), GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, pins);
#else
    gpio_mode_setup(GPIOX(port), GPIO_MODE_INPUT, GPIO_MODE_ANALOG, 1 << pin);
#endif
}

void gpio_configure_output_generic(uint8_t port, uint8_t pin, uint8_t speed, uint8_t af_index, uint8_t opendrain, uint8_t pullup) {
    log_printf("\t[%c%u]\toutput\t%s\t%s%c\t%s\n", 
               (char)(65 + port - 1), pin,
               opendrain ? "Open Drain" : "Push Pull", af_index == (uint8_t)-1 ? "" : "AF",
               af_index == (uint8_t)-1 ? ' ' : (char)(48 + af_index),
               pullup == 0   ? ""
               : pullup == 1 ? "Pullup"
                             : "Pulldown");
#ifdef STM32F1
    gpio_set_mode(GPIOX(port), speed, GPIO_CNF_OUTPUT_PUSHPULL, pins);
#else
    gpio_enable_port(port);
    gpio_mode_setup(GPIOX(port), af_index == (uint8_t)-1 ? GPIO_MODE_OUTPUT : GPIO_MODE_AF, pullup ? GPIO_PUPD_PULLUP : GPIO_PUPD_PULLDOWN,
                    1 << pin);
    if (af_index != (uint8_t)-1) {
        gpio_set_af(GPIOX(port), af_index, 1 << pin);
    }
    gpio_set_output_options(GPIOX(port), opendrain ? GPIO_OTYPE_OD : GPIO_OTYPE_PP, gpio_get_speed_setting(speed), 1 << pin);
#endif
}

void gpio_set_state(uint8_t port, uint8_t pin, uint8_t state) {
    if (state == 0) {
        gpio_clear(GPIOX(port), 1 << pin);
    } else {
        gpio_set(GPIOX(port), 1 << pin);
    }
}
