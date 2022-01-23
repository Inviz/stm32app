#include "spi.h"

/* SPI must be within range */
static app_signal_t spi_validate(transport_spi_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}

static app_signal_t spi_construct(transport_spi_t *spi) {
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
#else
        return 1;
#endif
    case 2:
#ifdef SPI3_BASE
        spi->clock = RCC_SPI3;
        spi->address = SPI3;
        break;
#else
        return 1;
#endif
    case 3:
#ifdef SPI4_BASE
        spi->clock = RCC_SPI4;
        spi->address = SPI4;
        break;
#else
        return 1;
#endif
    case 4:
#ifdef SPI5_BASE
        spi->clock = RCC_SPI5;
        spi->address = SPI5;
        break;
#else
        return 1;
#endif
    case 5:
#ifdef SPI6_BASE
        spi->clock = RCC_SPI6;
        spi->address = SPI6;
        break;
#else
        return 1;
#endif
    default: return 1;
    }
    return 0;
}

static app_signal_t spi_destruct(transport_spi_t *spi) {
    (void)spi;
    return 0;
}

static app_signal_t spi_start(transport_spi_t *spi) {
    (void)spi;
    rcc_periph_clock_enable(spi->clock);

    //log_printf("    > SPI%i SS\n", spi->device->seq + 1);
    //gpio_configure_output_af_pullup(spi->properties->ss_port, spi->properties->ss_pin, GPIO_SLOW, 5);
    log_printf("    > SPI%i SCK\n", spi->device->seq + 1);
    gpio_configure_output_af_pullup (spi->properties->sck_port, spi->properties->sck_pin, GPIO_SLOW, 5);
    log_printf("    > SPI%i MOSI\n", spi->device->seq + 1);
    gpio_configure_output_af_pullup(spi->properties->mosi_port, spi->properties->mosi_pin, GPIO_SLOW, 5);
    log_printf("    > SPI%i MISO\n", spi->device->seq + 1);
    gpio_configure_input(spi->properties->miso_port, spi->properties->miso_pin);

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
    switch (spi->properties->mode) {
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
    default:
        clock = SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE;
        polarity = SPI_CR1_CPHA_CLK_TRANSITION_1;
        break;
    }
    if (!spi->properties->is_slave) {
        spi_init_master(spi->address, SPI_CR1_BAUDRATE_FPCLK_DIV_256, clock, polarity, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
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

static app_signal_t spi_stop(transport_spi_t *spi) {
    spi_reset(spi->clock);
    spi_disable(spi->clock);
    return 0;
}

static app_signal_t spi_allocate_rx_buffer(transport_spi_t *spi) {
    spi->rx_buffer = malloc(spi->properties->rx_buffer_size);
    spi->rx_buffer_cursor = 0;
    return spi->rx_buffer == NULL;
}

/* Set timer to signal device in specified amount of time */
static app_signal_t spi_schedule_rx_timeout(transport_spi_t *spi) {
    return 0;
    return module_timer_timeout(spi->timer, spi->device, spi->properties->dma_rx_idle_timeout, DEVICE_REQUESTING);
}

/* Check if DMAs circular buffer position is still the same */
static app_signal_t spi_read_is_idle(transport_spi_t *spi) {
    return device_dma_get_buffer_position(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->properties->rx_buffer_size) ==
           spi->rx_buffer_cursor;
}
static app_signal_t spi_on_write(transport_spi_t *spi, app_event_t *event) {
    device_dma_tx_start((uint32_t) & (SPI_DR(spi->address)), spi->properties->dma_tx_unit, spi->properties->dma_tx_stream,
                                 spi->properties->dma_tx_channel, event->data, event->size);
    return APP_SIGNAL_OK;
}

static app_signal_t spi_on_read(transport_spi_t *spi, app_event_t *event) {
    if (spi->rx_buffer == NULL) {
        int error = spi_allocate_rx_buffer(spi);
        if (error != 0) {
            return error;
        }
    }
    device_dma_rx_start((uint32_t) & (SPI_DR(spi->address)), spi->properties->dma_rx_unit, spi->properties->dma_rx_stream,
                                 spi->properties->dma_rx_channel, spi->rx_buffer, spi->properties->rx_buffer_size);
    // schedule timeout to detect end of rx transmission
    spi_schedule_rx_timeout(spi);
    return APP_SIGNAL_OK;
}

// todo: Read DR register
static app_signal_t spi_write_complete(transport_spi_t *spi) {
    device_event_finalize(spi->device, &spi->writing);
    device_tick_catchup(spi->device, spi->device->ticks->input);
}

/* Send the resulting read contents back via a queue */
static app_signal_t spi_read_complete(transport_spi_t *spi) {
    app_event_t *response = app_event_from_vpool(
        &(app_event_t){.type = APP_EVENT_RESPONSE, .producer = spi->device, .consumer = spi->reading.producer}, &spi->rx_pool);
    device_event_finalize(spi->device, &spi->reading);
    app_publish(spi->device->app, &response);
    device_tick_catchup(spi->device, spi->device->ticks->input);
}

static app_signal_t spi_signal(transport_spi_t *spi, device_t *device, app_signal_t signal, void *source) {
    switch (signal) {
    case APP_SIGNAL_DMA_IDLE: spi_write_complete(spi); break;
    case APP_SIGNAL_TIMEOUT:
        if ((uint32_t)source == DEVICE_REQUESTING) {
            if (spi_read_is_idle(spi)) {
                spi_read_complete(spi);
            } else {
                spi_schedule_rx_timeout(spi);
            }
        }
        break;
    case APP_SIGNAL_DMA_TRANSFERRING: // transfer (half) complete
        // If it's RX DMA, we still want to wait until IDLE signal
        if (device_dma_match_source(source, spi->properties->dma_rx_unit, spi->properties->dma_rx_stream)) {
            device_dma_ingest(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->rx_buffer, spi->properties->rx_buffer_size,
                              &spi->rx_buffer_cursor, &spi->rx_pool);
            spi_schedule_rx_timeout(spi);
        } else {
            spi_write_complete(spi);
        }
        break;
    default:
        break;
    }

    return 0;
}

static app_signal_t spi_tick_input(transport_spi_t *spi, app_event_t *event, device_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_READ: return device_event_handle_and_process(spi->device, event, &spi->reading, spi_on_read);
    case APP_EVENT_WRITE: return device_event_handle_and_process(spi->device, event, &spi->writing, spi_on_write);
    default: return 0;
    }
}

device_methods_t transport_spi_methods = {.validate = (app_method_t) spi_validate,
                                          .construct = (app_method_t)spi_construct,
                                          .destruct = (app_method_t) spi_destruct,
                                          .start = (app_method_t) spi_start,
                                          .tick_input = (device_tick_callback_t)spi_tick_input,
                                          .callback_signal = (device_callback_signal_t) spi_signal,
                                          .stop = (app_method_t) spi_stop};