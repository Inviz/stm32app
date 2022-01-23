#include "stdbool.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define GPIOX(n) (GPIO_PORT_A_BASE + (n - 1) * 0x0400)

#define GPIO_SLOW 251
#define GPIO_MEDIUM 252
#define GPIO_FAST 253
#define GPIO_MAX 254

#define RCC_GPIOX(n) (RCC_GPIOA + n - 1)

#include "core/types.h"


void gpio_enable_port(uint8_t port);
void gpio_configure_input(uint8_t port, uint8_t pins);
void gpio_configure_output_generic(uint8_t port, uint8_t pin, uint8_t speed, uint8_t af_index, uint8_t opendrain, uint8_t pullup);
void gpio_configure_af_output(uint8_t port, uint8_t pins, uint8_t speed, bool_t pullup, uint8_t af_index);
void gpio_set_state(uint8_t port, uint8_t pins, uint8_t state);

#define gpio_configure_output(port, pin, speed) gpio_configure_output_generic(port, pin, speed, -1, 0, 0)
#define gpio_configure_output_pullup(port, pin, speed) gpio_configure_output_generic(port, pin, speed, -1, 0, 1)
#define gpio_configure_output_pulldown(port, pin, speed) gpio_configure_output_generic(port, pin, speed, -1, 0, 2)

#define gpio_configure_output_opendrain(port, pin, speed) gpio_configure_output_generic(port, pin, speed, -1, 1, 0)
#define gpio_configure_output_opendrain_pullup(port, pin, speed) gpio_configure_output_generic(port, pin, speed, -1, 1, 1)
#define gpio_configure_output_opendrain_pulldown(port, pin, speed) gpio_configure_output_generic(port, pin, speed, -1, 1, 2)

#define gpio_configure_output_af(port, pin, speed, af_index) gpio_configure_output_generic(port, pin, speed, af_index, 0, 0)
#define gpio_configure_output_af_pullup(port, pin, speed, af_index) gpio_configure_output_generic(port, pin, speed, af_index, 0, 1)
#define gpio_configure_output_af_pulldown(port, pin, speed, af_index) gpio_configure_output_generic(port, pin, speed, af_index, 0, 2)

#define gpio_configure_output_af_opendrain(port, pin, speed, af_index) gpio_configure_output_generic(port, pin, speed, af_index, 1, 0)
#define gpio_configure_output_af_opendrain_pullup(port, pin, speed, af_index)                                                              \
    gpio_configure_output_generic(port, pin, speed, af_index, 1, 1)
#define gpio_configure_output_af_opendrain_pulldown(port, pin, speed, af_index)                                                            \
    gpio_configure_output_generic(port, pin, speed, af_index, 1, 2)

/* EOF */