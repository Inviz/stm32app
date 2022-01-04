#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>



#define GPIOX(n) (GPIO_PORT_A_BASE + (n - 1) * 0x0400)

#define RCC_GPIOX(n) (RCC_GPIOA + n - 1)



void gpio_enable_port(uint8_t port);
void gpio_configure_input(uint8_t port, uint8_t pins);
void gpio_configure_output(uint8_t port, uint8_t pins);
void gpio_set_state(uint8_t port, uint8_t pins, uint8_t state);