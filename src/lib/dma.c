#include "dma.h"
#include "lib/vpool.h"

uint32_t dma_get_address(uint8_t index) {
    switch (index) {
#ifdef DMA3_BASE
    case 3:
        return DMA3;
#endif
#ifdef DMA2_BASE
    case 2:
        return DMA2;
#endif
    default:
        return DMA1;
    }
}

uint32_t dma_get_clock_address(uint8_t index) {
    switch (index) {
#ifdef DMA3_BASE
    case 3:
        return RCC_DMA3;
#endif
#ifdef DMA2_BASE
    case 2:
        return RCC_DMA2;
#endif
    default:
        return RCC_DMA1;
    }
}

uint32_t nvic_dma_get_channel_base(uint8_t index) {
    switch (index) {
#ifdef DMA2_BASE
    case 2:
#ifdef NVIC_DMA2_CHANNEL1_IRQ
        return NVIC_DMA2_CHANNEL1_IRQ - 1;
#else
        return NVIC_DMA2_STREAM0_IRQ;
#endif
#endif
#ifdef DMA3_BASE
    case 3:
#ifdef NVIC_DMA3_CHANNEL1_IRQ
        return NVIC_DMA3_CHANNEL1_IRQ - 1;
#else
        return NVIC_DMA3_STREAM0_IRQ;
#endif
#endif
    default:
#ifdef NVIC_DMA1_CHANNEL1_IRQ
        return NVIC_DMA1_CHANNEL1_IRQ - 1;
#else
        return NVIC_DMA1_STREAM0_IRQ;
#endif
    }
}

void dma_enable_channel_or_stream(uint32_t dma, uint8_t index) {
#ifdef DMA_CHANNEL1

    dma_enable_channel(dma, index);
#else
    dma_enable_stream(dma, index);
#endif
}

void dma_reset_channel_or_stream(uint32_t dma, uint8_t index) {
#ifdef DMA_CHANNEL1

    dma_channel_reset(dma, index);
#else
    dma_stream_reset(dma, index);
#endif
}

void dma_disable_channel_or_stream(uint32_t dma, uint8_t index) {
#ifdef DMA_CHANNEL1
    dma_disable_channel(dma, index);
#else
    dma_disable_stream(dma, index);
#endif
}


uint8_t dma_get_interrupt_for_channel_or_stream(uint32_t dma, uint8_t index) {
#ifdef DMA_CHANNEL1
    switch (dma) {
        case 2:
            return NVIC_DMA2_CHANNEL1_IRQ + index - 1;
        default:
            return NVIC_DMA1_CHANNEL1_IRQ + index - 1;
    }
#else
    switch (dma) {
        case 2:
            return NVIC_DMA2_STREAM1_IRQ + index - 1;
        default:
            return NVIC_DMA1_STREAM1_IRQ + index - 1;
    }

#endif
}

uint8_t devices_dma[DMA_BUFFER_SIZE] = {DMA_UNREGISTERED_INDEX};

void device_register_dma(uint8_t unit, uint8_t index, device_t *device) {

    devices_dma[DMA_INDEX(unit, index)] = get_device_number(device);
};

void device_unregister_dma(uint8_t unit, uint8_t index) { devices_dma[DMA_INDEX(unit, index)] = DMA_UNREGISTERED_INDEX; };

void devices_dma_notify(uint8_t unit, uint8_t index) {
    uint8_t number = devices_dma[DMA_INDEX(unit, index)];
    if (number != DMA_UNREGISTERED_INDEX) {
        device_t *device = get_device_by_number(number - 1);
        void *source = (void *)(uint32_t)((unit << 0) + (index << 16));
        if (dma_get_interrupt_flag(dma_get_address(unit), index, DMA_TEIF | DMA_DMEIF | DMA_FEIF)) {
            error_printf("DMA Error in channel %i", index);
            if (device->callbacks->signal(device->object, NULL, APP_SIGNAL_DMA_ERROR, device_dma_pack_source(unit, index))) {
                dma_clear_interrupt_flags(dma_get_address(unit), index, DMA_TEIF | DMA_DMEIF | DMA_FEIF);
            }
        } else if (dma_get_interrupt_flag(dma_get_address(unit), index, DMA_HTIF | DMA_TCIF)) {
            if (device->callbacks->signal(device->object, NULL, APP_SIGNAL_DMA_TRANSFERRING, device_dma_pack_source(unit, index)) == 0) {
                dma_clear_interrupt_flags(dma_get_address(unit), index, DMA_HTIF | DMA_TCIF);
            }
        }
    }
}

void *device_dma_pack_source(uint8_t unit, uint8_t index) {
    return (void *) (uint32_t) ((unit << 0) + (index << 8));
}

bool_t device_dma_match_source(void *source, uint8_t unit, uint8_t index) {
    return unit == (((uint32_t)source) & 0xff) && index == (((uint32_t)source) >> 8 & 0xff);
}

uint16_t device_dma_get_buffer_position(uint8_t unit, uint8_t index, uint16_t buffer_size) {
    return buffer_size - dma_get_number_of_data(dma_get_address(unit), index);
}

void device_dma_ingest(uint8_t unit, uint8_t index, uint8_t *buffer, uint16_t buffer_size, uint16_t *cursor, struct vpool *pool) {
    /* Calculate current position in buffer and check for new data available */
    uint16_t pos = device_dma_get_buffer_position(unit, index, buffer_size);
    if (pos != *cursor) {                       /* Check change in received data */
        if (pos > *cursor) {   
            vpool_insert(pool, UINT16_MAX, buffer[*cursor], pos - *cursor);
        } else {
            vpool_insert(pool, UINT16_MAX, buffer[*cursor], buffer_size - *cursor);
            if (pos > 0) {
                vpool_insert(pool, UINT16_MAX, buffer[0], pos);
            }
        }
        *cursor = pos;                          /* Save current position as old for next transfers */
    }
}

void device_dma_rx_stop(uint8_t unit, uint8_t stream, uint8_t channel) {
    uint32_t dma_address = dma_get_address(unit);
    dma_disable_channel_or_stream(dma_address, stream);
    dma_reset_channel_or_stream(dma_address, stream);
    nvic_disable_irq(nvic_dma_get_channel_base(unit) + stream);
}

void device_dma_rx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *data, size_t size) {
	uint32_t dma_address = dma_get_address(unit);

	dma_channel_reset(dma_address, stream);

	dma_set_peripheral_address(dma_address, stream, periphery_address);
	dma_set_memory_address(dma_address, stream, (uint32_t)data);
	dma_set_number_of_data(dma_address, stream, size);
	dma_set_read_from_peripheral(dma_address, stream);
	dma_enable_memory_increment_mode(dma_address, stream);
	dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_8BIT);
	dma_set_memory_size(dma_address, stream, DMA_MSIZE_8BIT);
	dma_set_priority(dma_address, stream, DMA_PL_VERY_HIGH);

    rcc_periph_clock_enable(dma_get_clock_address(unit));
	dma_enable_transfer_complete_interrupt(dma_address, stream);

	dma_enable_channel(DMA1, stream);
}

void device_dma_tx_stop(uint8_t unit, uint8_t stream, uint8_t channel) {
    uint32_t dma_address = dma_get_address(unit);
    dma_disable_channel_or_stream(dma_address, stream);
    dma_reset_channel_or_stream(dma_address, stream);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);
}


void device_dma_tx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *data, size_t size) {
    uint32_t dma_address = dma_get_address(unit);

    dma_channel_select(dma_address, stream, channel);
    dma_set_peripheral_address(dma_address, stream, periphery_address);
    dma_set_memory_address(dma_address, stream, (uint32_t)data);
    dma_set_number_of_data(dma_address, stream, size);

    dma_set_read_from_memory(dma_address, stream);
    dma_enable_memory_increment_mode(dma_address, stream);

    dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_8BIT);
    dma_set_memory_size(dma_address, stream, DMA_MSIZE_8BIT);
    dma_set_priority(dma_address, stream, DMA_PL_VERY_HIGH);

    dma_enable_transfer_complete_interrupt(dma_address, stream);

    rcc_periph_clock_enable(dma_get_clock_address(unit));
    nvic_set_priority(nvic_dma_get_channel_base(unit) + stream, 1);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);

    dma_enable_channel_or_stream(dma_address, stream);
}

#ifdef DMA_CHANNEL1
void dma_channel_select(uint32_t dma, uint8_t stream, uint8_t channel) {}
#else
void dma_set_read_from_peripheral(uint32_t dma, uint8_t stream) {
    dma_set_transfer_mode(dma, stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
};
void dma_set_read_from_memory(uint32_t dma, uint8_t stream) {
    dma_set_transfer_mode(dma, stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
};
#endif

#ifdef DMA_CHANNEL1

void dma1_channel1_isr(void) { devices_dma_notify(1, 1); }
void dma1_channel2_isr(void) { devices_dma_notify(1, 2); }
void dma1_channel3_isr(void) { devices_dma_notify(1, 3); }
void dma1_channel4_isr(void) { devices_dma_notify(1, 4); }
void dma1_channel5_isr(void) { devices_dma_notify(1, 5); }
void dma1_channel6_isr(void) { devices_dma_notify(1, 6); }
void dma1_channel7_isr(void) { devices_dma_notify(1, 7); }
void dma1_channel8_isr(void) { devices_dma_notify(1, 8); }

void dma2_channel1_isr(void) { devices_dma_notify(2, 1); }
void dma2_channel2_isr(void) { devices_dma_notify(2, 2); }
void dma2_channel3_isr(void) { devices_dma_notify(2, 3); }
void dma2_channel4_isr(void) { devices_dma_notify(2, 4); }
void dma2_channel5_isr(void) { devices_dma_notify(2, 5); }
void dma2_channel6_isr(void) { devices_dma_notify(2, 6); }
void dma2_channel7_isr(void) { devices_dma_notify(2, 7); }
void dma2_channel8_isr(void) { devices_dma_notify(2, 8); }

#else

void dma1_stream0_isr(void) { devices_dma_notify(1, 0); }
void dma1_stream1_isr(void) { devices_dma_notify(1, 1); }
void dma1_stream2_isr(void) { devices_dma_notify(1, 2); }
void dma1_stream3_isr(void) { devices_dma_notify(1, 3); }
void dma1_stream4_isr(void) { devices_dma_notify(1, 4); }
void dma1_stream5_isr(void) { devices_dma_notify(1, 5); }
void dma1_stream6_isr(void) { devices_dma_notify(1, 6); }
void dma1_stream7_isr(void) { devices_dma_notify(1, 7); }

void dma2_stream0_isr(void) { devices_dma_notify(2, 0); }
void dma2_stream1_isr(void) { devices_dma_notify(2, 1); }
void dma2_stream2_isr(void) { devices_dma_notify(2, 2); }
void dma2_stream3_isr(void) { devices_dma_notify(2, 3); }
void dma2_stream4_isr(void) { devices_dma_notify(2, 4); }
void dma2_stream5_isr(void) { devices_dma_notify(2, 5); }
void dma2_stream6_isr(void) { devices_dma_notify(2, 6); }
void dma2_stream7_isr(void) { devices_dma_notify(2, 7); }
#endif