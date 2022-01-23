#include "module/adc.h"
#include "lib/dma.h"

/* ADC must be within range */
static app_signal_t adc_validate(module_adc_properties_t *properties) {
    return 0;
}

static app_signal_t adc_construct(module_adc_t *adc) {

    adc->dma_address = dma_get_address(adc->properties->dma_unit);
    if (adc->dma_address == 0) {
        return 0;
    }

    switch (adc->device->seq) {
    case 0:
        adc->address = ADC1;
        adc->clock = RCC_ADC1;
        break;
    case 1:
#ifdef ADC2_BASE
        adc->address = ADC2;
        adc->clock = RCC_ADC2;
        break;
#endif
        return 1;
    case 2:
#ifdef ADC3_BASE
        adc->address = ADC3;
        adc->clock = RCC_ADC3;
        break;
#endif
        return 1;
    case 3:
#ifdef ADC4_BASE
        adc->clock = RCC_ADC4;
        adc->address = ADC4;
        break;
#endif
        return 1;
    default: return 1;
    }

    return 0;
}

static app_signal_t adc_destruct(module_adc_t *adc) {
    (void)adc;
    return 0;
}

static app_signal_t adc_start(module_adc_t *adc) {
    if (adc->channel_count == 0) {
        return 1;
    }
    adc_channels_alloc(adc, adc->channel_count);

    // create array of used channels
    size_t channel_index = 0;
    for (size_t channel = 0; channel < DEVICE_ADC_MAX_CHANNELS; channel++) {
        if (adc->subscribers[channel] != NULL) {
            adc->channels[channel_index] = channel;
            channel_index++;
        }
    }

    rcc_periph_clock_enable(adc->clock);

    adc_power_off(adc->address);
#ifdef STMF1
    adc_set_dual_mode(ADC_CR1_DUALMOD_FIM);
    rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV6);
#else
    adc_set_multi_mode(ADC_CCR_MULTI_DUAL_REGULAR_SIMUL);
    // adc_set_clk_prescale(adc->address, ADC_CCR_CKMODE_DIV2); // FIXME: Which div?
#endif
    adc_enable_scan_mode(adc->address);
    adc_set_continuous_conversion_mode(adc->address);
    adc_disable_discontinuous_mode_regular(adc->address);
    adc_set_right_aligned(adc->address);

// Software trigger
#ifdef STMF1
    adc_enable_external_trigger_regular(adc->address, ADC_CR2_EXTSEL_SWSTART);
    adc_set_sample_time_on_all_channels(adc->address, ADC_SMPR_SMP_239DOT5CYC);
#else
    adc_enable_external_trigger_regular(adc->address, ADC_CR2_EXTSEL_TIM2_TRGO, ADC_CR2_EXTEN_RISING_EDGE);
    adc_set_sample_time_on_all_channels(adc->address, ADC_SMPR_SMP_144CYC);
#endif
    adc_power_on(adc->address);
    // device_set_temporary_phase(adc->device, DEVICE_PREPARING, 10000);

    return 0;
}

static app_signal_t adc_calibrate(module_adc_t *adc) {
#ifdef ADC_CALIBRATION_ENABLED
    adc_reset_calibration(adc->address);
    adc_calibrate(adc->address);
#endif

    adc_set_regular_sequence(adc->address, adc->channel_count, adc->channels);

    adc_dma_setup(adc);
    adc_enable_dma(adc->address);

    // device_set_temporary_phase(adc->device, DEVICE_CALIBRATING, ADC_CALIBRATION_DELAY);

    return 0;
}

static app_signal_t adc_run(module_adc_t *adc) {
    adc_start_conversion_regular(adc->address);
    device_set_phase(adc->device, DEVICE_RUNNING);
    return 0;
}

static app_signal_t adc_accept(module_adc_t *adc, device_t *device, void *channel) {
    adc->channel_count++;
    adc->subscribers[(size_t)channel] = device;
    return 0;
}

static app_signal_t adc_stop(module_adc_t *adc) {
    adc_disable_dma(adc->address);
    adc_power_off(adc->address);
    adc_channels_free(adc);
    return 0;
}

static app_signal_t adc_phase(module_adc_t *adc) {
    switch (adc->device->phase) {
    case DEVICE_PREPARING: return adc_calibrate(adc);

    case DEVICE_CALIBRATING: return adc_run(adc); break;
    default: break;
    }
    return 0;
}

static app_signal_t adc_receive(module_adc_t *adc, device_t *device, void *value, void *channel) {
    (void)device;
    (void)value;
    if (adc_integrate_samples(adc) == 0) {
        log_printf("ADC%i - Measurement ready %lu\n", adc->device->seq, adc->values[1]);
        for (size_t i = 0; i < adc->channel_count; i++) {
            size_t channelIndex = adc->channels[i];
            device_send(adc->device, adc->subscribers[channelIndex], (void *)adc->values[i], (void *)channel);
        }
    }
    return 0;
}

/*
static app_signal_t adc_high_priority(module_adc_t *adc), uint32_t time_passed, uint32_t *next_tick) {

}*/

device_class_t module_adc_class = {
    .type = MODULE_ADC,
    .size = sizeof(module_adc_t),
    .phase_subindex = MODULE_ADC_PHASE,
    .validate = (app_method_t)adc_validate,
    .construct = (app_method_t)adc_construct,
    .destruct = (app_method_t)adc_destruct,
    .callback_link = (device_callback_argument_t)adc_accept,
    .callback_value = (device_callback_value_t)adc_receive,
    //.high_priority = (int (*)(void *, uint32_t time_passed, uint32_t *next_tick))module_adc_high_priority,
    .callback_phase = (device_callback_phase_t)adc_phase,
    .start = (app_method_t)adc_start,
    .stop = (app_method_t)adc_stop,
};

void adc_dma_setup(module_adc_t *adc) {
    if (adc->dma_address == DMA1) {
        rcc_periph_clock_enable(RCC_DMA1);
        nvic_enable_irq(dma_get_interrupt_for_channel_or_stream(adc->properties->dma_unit, adc->properties->dma_stream));
    }
    dma_channel_select(adc->dma_address, adc->properties->dma_stream, adc->properties->dma_channel);
    dma_disable_channel_or_stream(adc->dma_address, adc->properties->dma_stream);

    dma_enable_circular_mode(adc->dma_address, adc->properties->dma_stream);
    dma_enable_memory_increment_mode(adc->dma_address, adc->properties->dma_stream);

    dma_set_peripheral_size(adc->dma_address, adc->properties->dma_stream, DMA_PSIZE_16BIT);
    dma_set_memory_size(adc->dma_address, adc->properties->dma_stream, DMA_MSIZE_16BIT);

    dma_set_read_from_peripheral(adc->dma_address, adc->properties->dma_stream);
    dma_set_peripheral_address(adc->dma_address, adc->properties->dma_stream, (uint32_t)&ADC_DR(adc->address));

    dma_set_memory_address(adc->dma_address, adc->properties->dma_stream, (uint32_t)adc->sample_buffer);
    dma_set_number_of_data(adc->dma_address, adc->properties->dma_stream, adc->sample_buffer_size);

    dma_enable_transfer_complete_interrupt(adc->dma_address, adc->properties->dma_stream);
    dma_enable_channel_or_stream(adc->dma_address, adc->properties->dma_stream);
}

size_t adc_integrate_samples(module_adc_t *adc) {
    if (adc->measurement_counter == 0) {
        memset(adc->accumulators, 0, adc->channel_count * sizeof(uint32_t));
        memset(adc->properties, 0, adc->channel_count * sizeof(uint32_t));
    }
    /* integrate samples from the buffer into accumulator buffer */
    size_t samples_left_total = adc->measurements_per_second - adc->measurement_counter;
    size_t samples_left_now = samples_left_total < adc->sample_buffer_size ? samples_left_total : adc->sample_buffer_size;
    for (size_t i = 0; i < samples_left_now; i += adc->channel_count) {
        for (size_t c = 0; c < adc->channel_count; c++) {
            adc->accumulators[c] += adc->sample_buffer[i + c];
        }
    }
    adc->measurement_counter += samples_left_now;
    if (adc->measurement_counter == adc->measurements_per_second) {
        adc->measurement_counter = 0;
        for (size_t c = 0; c < adc->channel_count; c++) {
            adc->values[c] = adc->accumulators[c] / (adc->measurements_per_second / adc->channel_count);
        }
    }
    return samples_left_total - samples_left_now;
}

void adc_channels_alloc(module_adc_t *adc, size_t channel_count) {
    adc->sample_buffer_size = adc->properties->sample_count_per_channel * channel_count;
    adc->measurements_per_second = (1000000 / (adc->properties->interval));
    adc->channels = malloc(channel_count * sizeof(size_t));
    adc->values = malloc(channel_count * sizeof(uint32_t));
    adc->accumulators = malloc(channel_count * sizeof(uint32_t));
    adc->sample_buffer = malloc(adc->sample_buffer_size * sizeof(uint16_t));
}

void adc_channels_free(module_adc_t *adc) {
    free(adc->sample_buffer);
    free(adc->values);
    free(adc->channels);
    free(adc->accumulators);
}
