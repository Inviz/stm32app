#include "spi.h"

/* SPI must be within range */
static int module_spi_validate(OD_entry_t *config_entry) {
    module_spi_config_t *config = (module_spi_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    return config->index == 0;
}

static int module_spi_construct(module_spi_t *spi, device_t *device) {
    spi->device = device;
    spi->config = (module_spi_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);
    switch (spi->config->index) {
    case 1:
        spi->clock = RCC_SPI1;
        spi->address = SPI1;
        break;
    case 2:
#ifdef SPI2_BASE
        spi->clock = RCC_SPI2;
        spi->address = SPI2;
        break;
#endif
        return 1;
    case 3:
#ifdef SPI3_BASE
        spi->clock = RCC_SPI3;
        spi->address = SPI3;
        break;
#endif
        return 1;
    case 4:
#ifdef SPI4_BASE
        spi->clock = RCC_SPI4;
        spi->address = SPI4;
        break;
#endif
        return 1;
    default:
        return 1;
    }
    return 0;
}

static int module_spi_destruct(module_spi_t *spi) {
    (void)spi;
    return 0;
}

static int module_spi_start(module_spi_t *spi) {
    (void)spi;
    rcc_periph_clock_enable(spi->clock);

    device_gpio_configure_input("MISO", spi->config->miso_port, spi->config->miso_pin);
    device_gpio_configure_output_with_value("SCK", spi->config->sck_port, spi->config->sck_pin, 0);
    device_gpio_configure_output_with_value("SS", spi->config->ss_port, spi->config->ss_pin, 0);
    device_gpio_configure_output_with_value("MOSI", spi->config->mosi_port, spi->config->mosi_pin, 0);

    /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
    spi_reset(spi->address);

    /* Set up SPI in Master mode with:
     * Clock baud rate: 1/64 of peripheral clock frequency
     * Clock polarity: Idle High
     * Clock phase: Data valid on 2nd clock pulse
     * Data frame format: 8-bit
     * Frame format: MSB First
     */
    spi_init_master(spi->address, SPI_CR1_BAUDRATE_FPCLK_DIV_64, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

    /*
     * Set NSS management to software.
     *
     * Note:
     * Setting nss high is very important, even if we are controlling the GPIO
     * ourselves this bit needs to be at least set to 1, otherwise the spi
     * peripheral will not send any data out.
     */
    spi_enable_software_slave_management(spi->address);
    spi_set_nss_high(spi->address);

    /* Enable SPI1 periph. */
    spi_enable(spi->address);
    return 0;
}

static int module_spi_stop(module_spi_t *spi) {
    spi_reset(spi->clock);
    spi_disable(spi->clock);
    return 0;
}


void module_spi_send(module_spi_t *spi, uint8_t value) {
  spi_send(spi->address, value);
}

device_callbacks_t module_spi_callbacks = {.validate = module_spi_validate,
                                           .construct = (int (*)(void *, device_t *)) module_spi_construct,
                                           .destruct = (int (*)(void *)) module_spi_destruct,
                                           .start = (int (*)(void *)) module_spi_start,
                                           .stop = (int (*)(void *)) module_spi_stop};