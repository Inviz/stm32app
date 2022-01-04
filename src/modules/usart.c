#include "usart.h"
#include "helpers/dma.h"

module_usart_t *modules_usart[OD_CNT_MODULE_USART];

/* USART must be within range */
static int module_usart_validate(OD_entry_t *config_entry) {
    module_usart_config_t *config = (module_usart_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    return config->index == 0;
}

static int module_usart_construct(module_usart_t *usart, device_t *device) {
    usart->device = device;
    usart->config = (module_usart_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);

    usart->dma_rx_address = dma_get_address(usart->config->dma_rx_unit);
    usart->dma_tx_address = dma_get_address(usart->config->dma_tx_unit);

    usart->dma_rx_buffer = pvPortMalloc(usart->config->dma_rx_buffer_size);

    if (usart->dma_rx_address == 0 || usart->dma_tx_address == 0) {
        return 0;
    }

    switch (usart->config->index) {
    case 1:
        usart->address = USART1;
        usart->clock = RCC_USART1;
        break;
    case 2:
#ifdef USART2_BASE
        usart->address = USART2;
        usart->clock = RCC_USART2;
        break;
#endif
        return 1;
    case 3:
#ifdef USART3_BASE
        usart->address = USART3;
        usart->clock = RCC_USART3;
        break;
#endif
        return 1;
    case 4:
#ifdef USART4_BASE
        usart->address = USART4;
        usart->clock = RCC_USART4;
        break;
#endif
        return 1;
    default:
        return 1;
    }
    modules_usart[usart->config->index - 1] = usart;

    return 0;
}

int module_usart_send(module_usart_t *usart, char *data, int size) {
    (void) usart;
    (void) data;
    (void) size;
    return 0;
}


static uint16_t module_usart_get_buffer_size(module_usart_t *usart) {
    return usart->config->dma_rx_buffer_size;
}

static uint16_t module_usart_get_buffer_size_left(module_usart_t *usart) {
    return dma_get_number_of_data(usart->config->dma_rx_unit, usart->config->dma_rx_stream);
}

static uint16_t module_usart_get_buffer_size_written(module_usart_t *usart) {
    return module_usart_get_buffer_size(usart) - module_usart_get_buffer_size_left(usart);
}

static int module_usart_accept(module_usart_t *usart, device_t *target, void *argument) {
    usart->target_device = target;
    usart->target_argument = argument;
    return 0;
}

static int module_usart_destruct(module_usart_t *usart) {
    vPortFree(usart->dma_rx_buffer);
    return 0;
}

static void module_usart_rx_dma_start(module_usart_t *usart) {
    rcc_periph_clock_enable(dma_get_clock_address(usart->config->dma_rx_unit));
    nvic_set_priority(nvic_dma_get_channel_base(usart->config->dma_rx_unit) + usart->config->dma_rx_stream, 0);
    nvic_enable_irq(nvic_dma_get_channel_base(usart->config->dma_rx_unit) + usart->config->dma_rx_stream);

    usart_enable_rx_dma(usart->address);
    usart_enable_idle_interrupt(usart->address);
}

static void module_usart_tx_dma_stop(module_usart_t *usart) {
    dma_disable_channel_or_stream(usart->dma_tx_address, usart->config->dma_tx_stream);
    dma_reset_channel_or_stream(usart->dma_tx_address, usart->config->dma_tx_stream);
    nvic_disable_irq(nvic_dma_get_channel_base(usart->config->dma_tx_unit) + usart->config->dma_tx_stream);
    usart_disable_tx_dma(usart->address);
    //usart_disable_tx_complete_interrupt(usart->address);
}

/* Configure memory -> usart transfer*/
static void module_usart_tx_dma_start(module_usart_t *usart, uint8_t *data, uint16_t size) {
    module_usart_tx_dma_stop(usart);
    
    dma_channel_select(usart->dma_tx_address, usart->config->dma_tx_stream, usart->config->dma_tx_channel);
    dma_set_peripheral_address(usart->dma_tx_address, usart->config->dma_tx_stream,
                               (uint32_t) & (USART_DR(usart->address)));
    dma_set_memory_address(usart->dma_tx_address, usart->config->dma_tx_stream, (uint32_t)data);
    dma_set_number_of_data(usart->dma_tx_address, usart->config->dma_tx_stream, size);

    dma_set_read_from_memory(usart->dma_tx_address, usart->config->dma_tx_stream);
    dma_enable_memory_increment_mode(usart->dma_tx_address, usart->config->dma_tx_stream);

    dma_set_peripheral_size(usart->dma_tx_address, usart->config->dma_tx_stream, DMA_PSIZE_8BIT);
    dma_set_memory_size(usart->dma_tx_address, usart->config->dma_tx_stream, DMA_MSIZE_8BIT);
    dma_set_priority(usart->dma_tx_address, usart->config->dma_tx_stream, DMA_PL_VERY_HIGH);

    dma_enable_transfer_complete_interrupt(usart->dma_tx_address, usart->config->dma_tx_stream);
    dma_enable_channel_or_stream(usart->dma_tx_address, usart->config->dma_tx_stream);

    rcc_periph_clock_enable(dma_get_clock_address(usart->config->dma_tx_unit));
    nvic_set_priority(nvic_dma_get_channel_base(usart->config->dma_tx_unit) + usart->config->dma_tx_stream, 1);
    nvic_enable_irq(nvic_dma_get_channel_base(usart->config->dma_tx_unit) + usart->config->dma_tx_stream);

    usart_enable_tx_dma(usart->address);
    dma_enable_transfer_complete_interrupt(usart->address, usart->config->dma_tx_stream);
}

static int module_usart_start(module_usart_t *usart) {
    usart_set_baudrate(usart->address, usart->config->baudrate);
    usart_set_databits(usart->address, usart->config->databits);
    usart_set_stopbits(usart->address, USART_STOPBITS_1);
    usart_set_mode(usart->address, USART_MODE_TX_RX);
    usart_set_parity(usart->address, USART_PARITY_NONE);
    usart_set_flow_control(usart->address, USART_FLOWCONTROL_NONE);

    usart_enable_idle_interrupt(usart->address);
    rcc_periph_clock_enable(usart->clock);

    usart_enable(usart->address);

    module_usart_rx_dma_start(usart);

    return 0;
}

static int module_usart_stop(module_usart_t *usart) {
    (void)usart;
    return 0;
}
static int module_usart_signal(module_usart_t *usart, device_t *device, int interrupt, char *source) {
    (void)device;
    (void)source;
    switch (interrupt) {
    case DMA_TCIF: // DMA_TCIF, transfer complete
        usart->target_device->callbacks->signal(usart->target_device->object, usart->device, DEVICE_TX_DONE, usart->target_argument);
        module_usart_tx_dma_stop(usart);
        break;
    case USART_CR1_IDLEIE: // USART IDLE, probably complete transfer
        usart->target_device->callbacks->signal(usart->target_device->object, usart->device, DEVICE_RX_DONE, usart->target_argument);
        break;
    }

    return 0;
}

device_callbacks_t module_usart_callbacks = {
    .validate = module_usart_validate,
    .construct = (int (*)(void *, device_t *))module_usart_construct,
    .destruct = (int (*)(void *))module_usart_destruct,
    .signal = (int (*)(void *, device_t *device, uint32_t signal, void *channel))module_usart_signal,
    .start = (int (*)(void *))module_usart_start,
    .accept = (int (*)(void *, device_t *device, void *channel)) module_usart_accept,
    .stop = (int (*)(void *))module_usart_stop};
