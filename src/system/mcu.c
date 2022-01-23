#include "mcu.h"


static ODR_t mcu_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    system_mcu_t *mcu = stream->object;
    (void)mcu;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t mcu_validate(system_mcu_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}


static app_signal_t mcu_phase_constructing(system_mcu_t *mcu) {
    return 0;
}

static app_signal_t mcu_phase_starting(system_mcu_t *mcu) {
#if defined(STM32F1)
    mcu->clock = &rcc_hsi_propertiess[RCC_CLOCK_HSE8_72MHZ];
#elif defined(STM32F4)
    mcu->clock = &rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ];
#endif
    rcc_clock_setup_pll(mcu->clock);
    return 0;
}

static app_signal_t mcu_phase_stoping(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static app_signal_t mcu_phase_pausing(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static app_signal_t mcu_phase_resuming(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static app_signal_t mcu_phase_linking(system_mcu_t *mcu) {
    (void)mcu;
    return device_link(mcu->device, (void **)&mcu->storage, mcu->properties->storage_index, NULL);
}

static app_signal_t mcu_phase(system_mcu_t *mcu, device_phase_t phase) {
    (void)mcu;
    (void)phase;
    return 0;
}

device_methods_t system_mcu_methods = {.validate = (app_method_t) mcu_validate,
                                           .phase_constructing = (app_method_t)mcu_phase_constructing,
                                           .phase_linking = (app_method_t) mcu_phase_linking,
                                           .phase_starting = (app_method_t) mcu_phase_starting,
                                           .phase_stoping = (app_method_t) mcu_phase_stoping,
                                           .phase_pausing = (app_method_t) mcu_phase_pausing,
                                           .phase_resuming = (app_method_t) mcu_phase_resuming,
                                           //.accept = (int (*)(void *, device_t *device, void *channel))system_mcu_accept,
                                           .callback_phase = (app_signal_t (*)(void *, device_phase_t phase))mcu_phase,
                                           .property_write = mcu_property_write};
