#include "modules/adc.h"
#include "helpers/dma.h"

module_adc_t *modules_adc[OD_CNT_MODULE_ADC];

/* ADC must be within range */
static int module_adc_validate(OD_entry_t *config_entry) {
    module_adc_config_t *config = (module_adc_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    return config->index == 0;
}

static int module_adc_construct(module_adc_t *adc, device_t *device) {
    adc->device = device;
    adc->config = (module_adc_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);

    adc->dma_address = dma_get_address(adc->config->dma_unit);
    if (adc->dma_address == 0) {
        return 0;
    }

    switch (adc->config->index) {
    case 1:
        adc->address = ADC1;
        adc->clock = RCC_ADC1;
        break;
    case 2:
#ifdef ADC2_BASE
        adc->address = ADC2;
        adc->clock = RCC_ADC2;
        break;
#endif
        return 1;
    case 3:
#ifdef ADC3_BASE
        adc->address = ADC3;
        adc->clock = RCC_ADC3;
        break;
#endif
        return 1;
    case 4:
#ifdef ADC4_BASE
        adc->clock = RCC_ADC4;
        adc->address = ADC4;
        break;
#endif
        return 1;
    default:
        return 1;
    }
    modules_adc[adc->config->index - 1] = adc;

    return 0;
}

static int module_adc_destruct(module_adc_t *adc) {
    (void)adc;
    return 0;
}

static int module_adc_start(module_adc_t *adc) {
    module_adc_channels_alloc(adc, adc->channel_count);

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
    device_set_temporary_phase(adc->device, DEVICE_PREPARING, 10000);

    return 0;
}

static int module_adc_calibrate(module_adc_t *adc) {
#ifdef ADC_CALIBRATION_ENABLED
    adc_reset_calibration(adc->address);
    adc_calibrate(adc->address);
#endif

    adc_set_regular_sequence(adc->address, adc->channel_count, adc->channels);

    module_adc_dma_setup(adc);
    adc_enable_dma(adc->address);

    device_set_temporary_phase(adc->device, DEVICE_CALIBRATING, ADC_CALIBRATION_DELAY);

    return 0;
}

static int module_adc_run(module_adc_t *adc) {
    adc_start_conversion_regular(adc->address);
    device_set_phase(adc->device, DEVICE_RUNNING);
    return 0;
}

static int module_adc_accept(module_adc_t *adc, device_t *device, void *channel) {
    adc->channel_count++;
    adc->subscribers[(size_t)channel] = device;
    return 0;
}

static int module_adc_stop(module_adc_t *adc) {
    adc_disable_dma(adc->address);
    adc_power_off(adc->address);
    module_adc_channels_free(adc);
    return 0;
}

static int module_adc_phase(module_adc_t *adc) {
    switch (adc->device->phase) {
    case DEVICE_PREPARING:
        return module_adc_calibrate(adc);

    case DEVICE_CALIBRATING:
        return module_adc_run(adc);
        break;
    default:
        break;
    }
    return 0;
}

static int module_adc_receive(module_adc_t *adc, device_t *device, void *value, void *channel) {
    (void)device;
    (void)value;
    if (module_adc_integrate_samples(adc) == 0) {
        log_printf("ADC%i - Measurement ready %i\n", adc->config->index, adc->values[1]);
        for (size_t i = 0; i < adc->channel_count; i++) {
            size_t channelIndex = adc->channels[i];
            adc->subscribers[channelIndex]->callbacks->receive(adc->subscribers[channelIndex]->object, adc->device, (void *)adc->values[i],
                                                               (void *)channel);
        }
    }
    return 0;
}

/*
static int module_adc_async(module_adc_t *adc), uint32_t time_passed, uint32_t *next_tick) {

}*/

device_callbacks_t module_adc_callbacks = {.validate = module_adc_validate,
                                           .construct = (int (*)(void *, device_t *))module_adc_construct,
                                           .destruct = (int (*)(void *))module_adc_destruct,
                                           .accept = (int (*)(void *, device_t *device, void *channel))module_adc_accept,
                                           .receive = (int (*)(void *, device_t *device, void *value, void *channel))module_adc_receive,
                                           //.async = (int (*)(void *, uint32_t time_passed, uint32_t *next_tick))module_adc_async,
                                           .phase = (int (*)(void *, device_phase_t phase))module_adc_phase,
                                           .start = (int (*)(void *))module_adc_start,
                                           .stop = (int (*)(void *))module_adc_stop};

void module_adc_dma_setup(module_adc_t *adc) {
    if (adc->dma_address == DMA1) {
        rcc_periph_clock_enable(RCC_DMA1);
        nvic_enable_irq(dma_get_interrupt_for_channel_or_stream(adc->config->dma_unit, adc->config->dma_stream));
    }
    dma_channel_select(adc->dma_address, adc->config->dma_stream, adc->config->dma_channel);
    dma_disable_channel_or_stream(adc->dma_address, adc->config->dma_stream);

    dma_enable_circular_mode(adc->dma_address, adc->config->dma_stream);
    dma_enable_memory_increment_mode(adc->dma_address, adc->config->dma_stream);

    dma_set_peripheral_size(adc->dma_address, adc->config->dma_stream, DMA_PSIZE_16BIT);
    dma_set_memory_size(adc->dma_address, adc->config->dma_stream, DMA_MSIZE_16BIT);

    dma_set_read_from_peripheral(adc->dma_address, adc->config->dma_stream);
    dma_set_peripheral_address(adc->dma_address, adc->config->dma_stream, (uint32_t)&ADC_DR(adc->address));

    dma_set_memory_address(adc->dma_address, adc->config->dma_stream, (uint32_t)adc->sample_buffer);
    dma_set_number_of_data(adc->dma_address, adc->config->dma_stream, adc->sample_buffer_size);

    dma_enable_transfer_complete_interrupt(adc->dma_address, adc->config->dma_stream);
    dma_enable_channel_or_stream(adc->dma_address, adc->config->dma_stream);
}

size_t module_adc_integrate_samples(module_adc_t *adc) {
    if (adc->measurement_counter == 0) {
        memset(adc->accumulators, 0, adc->channel_count * sizeof(uint32_t));
        memset(adc->values, 0, adc->channel_count * sizeof(uint32_t));
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

void module_adc_channels_alloc(module_adc_t *adc, size_t channel_count) {
    adc->sample_buffer_size = adc->config->sample_count_per_channel * channel_count;
    adc->measurements_per_second = (1000000 / (adc->config->interval));
    adc->channels = pvPortMalloc(channel_count * sizeof(size_t));
    adc->values = pvPortMalloc(channel_count * sizeof(uint32_t));
    adc->accumulators = pvPortMalloc(channel_count * sizeof(uint32_t));
    adc->sample_buffer = pvPortMalloc(adc->sample_buffer_size * sizeof(uint16_t));
}

void module_adc_channels_free(module_adc_t *adc) {
    vPortFree(adc->sample_buffer);
    vPortFree(adc->values);
    vPortFree(adc->channels);
    vPortFree(adc->accumulators);
}
