#include "spi.h"

/* SPI must be within range */
static int transport_spi_validate(OD_entry_t *config_entry) {
    transport_spi_config_t *config = (transport_spi_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    return 0;
}

static int transport_spi_construct(transport_spi_t *spi, device_t *device) {
    spi->device = device;
    spi->config = (transport_spi_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);
    switch (spi->device->seq) {
    case 0:
        spi->clock = RCC_SPI1;
        spi->address = SPI1;
        break;
    case 1:
#ifdef SPI2_BASE
        spi->clock = RCC_SPI2;
        spi->address = SPI2;
        break;
#endif
        return 1;
    case 2:
#ifdef SPI3_BASE
        spi->clock = RCC_SPI3;
        spi->address = SPI3;
        break;
#endif
        return 1;
    case 3:
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

static int transport_spi_destruct(transport_spi_t *spi) {
    (void)spi;
    return 0;
}

static int transport_spi_start(transport_spi_t *spi) {
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
    uint8_t clock, polarity;
    switch (spi->config->mode) {
        case 3:
            clock = SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE;
            polarity = SPI_CR1_CPHA_CLK_TRANSITION_2;
            break;
        case 2:
            clock = SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE;
            polarity = SPI_CR1_CPHA_CLK_TRANSITION_1;
            break;
        case 1:
            clock = SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE;
            polarity = SPI_CR1_CPHA_CLK_TRANSITION_2;
            break;
        case 0:
            clock = SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE;
            polarity = SPI_CR1_CPHA_CLK_TRANSITION_1;
            break;
    }
    if (!spi->config->is_slave) {
        spi_init_master(spi->address, SPI_CR1_BAUDRATE_FPCLK_DIV_64, clock,
                        polarity, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    } else {
        return 1;
    }

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

static int transport_spi_stop(transport_spi_t *spi) {
    spi_reset(spi->clock);
    spi_disable(spi->clock);
    return 0;
}


void transport_spi_send(transport_spi_t *spi, uint8_t value) {
  spi_send(spi->address, value);
}

device_callbacks_t transport_spi_callbacks = {.validate = transport_spi_validate,
                                           .construct = (int (*)(void *, device_t *)) transport_spi_construct,
                                           .destruct = (int (*)(void *)) transport_spi_destruct,
                                           .start = (int (*)(void *)) transport_spi_start,
                                           .stop = (int (*)(void *)) transport_spi_stop};