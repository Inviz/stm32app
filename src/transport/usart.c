#include "usart.h"
#include "lib/dma.h"

/* USART must be within range */
static app_signal_t usart_validate(transport_usart_properties_t *properties) {
    return 0;
}

static app_signal_t usart_construct(transport_usart_t *usart) {

    usart->dma_rx_address = dma_get_address(usart->properties->dma_rx_unit);
    usart->dma_tx_address = dma_get_address(usart->properties->dma_tx_unit);

    usart->dma_rx_buffer = malloc(usart->properties->dma_rx_buffer_size);

    if (usart->dma_rx_address == 0 || usart->dma_tx_address == 0) {
        return 0;
    }

    switch (usart->device->seq) {
    case 0:
        usart->address = USART1;
        usart->clock = RCC_USART1;
        break;
    case 1:
#ifdef USART2_BASE
        usart->address = USART2;
        usart->clock = RCC_USART2;
        break;
#endif
        return 1;
    case 2:
#ifdef USART3_BASE
        usart->address = USART3;
        usart->clock = RCC_USART3;
        break;
#endif
        return 1;
    case 3:
#ifdef USART4_BASE
        usart->address = USART4;
        usart->clock = RCC_USART4;
        break;
#endif
        return 1;
    default: return 1;
    }
    return 0;
}

int transport_usart_send(transport_usart_t *usart, char *data, int size) {
    (void)usart;
    (void)data;
    (void)size;
    return 0;
}

static uint16_t transport_usart_get_buffer_size(transport_usart_t *usart) {
    return usart->properties->dma_rx_buffer_size;
}

static uint16_t transport_usart_get_buffer_size_left(transport_usart_t *usart) {
    return dma_get_number_of_data(usart->properties->dma_rx_unit, usart->properties->dma_rx_stream);
}

static uint16_t transport_usart_get_buffer_size_written(transport_usart_t *usart) {
    return transport_usart_get_buffer_size(usart) - transport_usart_get_buffer_size_left(usart);
}

static app_signal_t usart_accept(transport_usart_t *usart, device_t *target, void *argument) {
    usart->target_device = target;
    usart->target_argument = argument;
    return 0;
}

static app_signal_t usart_destruct(transport_usart_t *usart) {
    free(usart->dma_rx_buffer);
    return 0;
}

static void transport_usart_tx_dma_stop(transport_usart_t *usart) {
    device_dma_tx_stop(usart->properties->dma_tx_unit, usart->properties->dma_tx_stream, usart->properties->dma_tx_channel);
    usart_disable_tx_dma(usart->address);
    // usart_disable_tx_complete_interrupt(usart->address);
}

/* Configure memory -> usart transfer*/
static void transport_usart_tx_dma_start(transport_usart_t *usart, uint8_t *data, uint16_t size) {
    transport_usart_tx_dma_stop(usart);
    device_dma_tx_start((uint32_t) & (USART_DR(usart->address)), usart->properties->dma_tx_unit, usart->properties->dma_tx_stream,
                        usart->properties->dma_tx_channel, data, size);
    usart_enable_tx_dma(usart->address);
}

static void transport_usart_rx_dma_stop(transport_usart_t *usart) {
    device_dma_rx_stop(usart->properties->dma_tx_unit, usart->properties->dma_tx_stream, usart->properties->dma_tx_channel);
    usart_disable_rx_dma(usart->address);
    // usart_disable_tx_complete_interrupt(usart->address);
}

/* Configure memory <- usart transfer*/
static void transport_usart_rx_dma_start(transport_usart_t *usart, uint8_t *data, uint16_t size) {
    transport_usart_rx_dma_stop(usart);
    device_dma_rx_start((uint32_t) & (USART_DR(usart->address)), usart->properties->dma_tx_unit, usart->properties->dma_tx_stream,
                        usart->properties->dma_tx_channel, data, size);
    usart_enable_rx_dma(usart->address);
}

static app_signal_t usart_start(transport_usart_t *usart) {
    usart_set_baudrate(usart->address, usart->properties->baudrate);
    usart_set_databits(usart->address, usart->properties->databits);
    usart_set_stopbits(usart->address, USART_STOPBITS_1);
    usart_set_mode(usart->address, USART_MODE_TX_RX);
    usart_set_parity(usart->address, USART_PARITY_NONE);
    usart_set_flow_control(usart->address, USART_FLOWCONTROL_NONE);

    usart_enable_idle_interrupt(usart->address);
    rcc_periph_clock_enable(usart->clock);

    usart_enable(usart->address);

    // transport_usart_rx_dma_start(usart);

    return 0;
}

static app_signal_t usart_stop(transport_usart_t *usart) {
    (void)usart;
    return 0;
}
static app_signal_t usart_signal(transport_usart_t *usart, device_t *device, app_signal_t signal, void *source) {
    (void)device;
    (void)source;
    switch (signal) {
    case APP_SIGNAL_TX_COMPLETE: // DMA_TCIF, transfer complete
        device_signal(usart->target_device, usart->device, APP_SIGNAL_TX_COMPLETE, usart->target_argument);
        transport_usart_tx_dma_stop(usart);
        break;
    case APP_SIGNAL_RX_COMPLETE: // USART IDLE, probably complete transfer
        device_signal(usart->target_device, usart->device, APP_SIGNAL_RX_COMPLETE, usart->target_argument);
        break;
    default:
        break;
    }

    return 0;
}

device_class_t transport_usart_class = {
    .type = TRANSPORT_USART,
    .size = sizeof(transport_usart_t),
    .phase_subindex = TRANSPORT_USART_PHASE,

    .validate = (app_method_t)usart_validate,
    .construct = (app_method_t)usart_construct,
    .destruct = (app_method_t)usart_destruct,
    .start = (app_method_t)usart_start,
    .stop = (app_method_t)usart_stop,
    .on_signal = (device_on_signal_t)usart_signal,
    .on_link = (device_on_link_t)usart_accept,
};
