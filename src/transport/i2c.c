#include "i2c.h"
#include "lib/dma.h"


static ODR_t i2c_property_write(OD_stream_t *stream, const void *buf, OD_size_t count,
                                            OD_size_t *countWritten) {
    transport_i2c_t *i2c = stream->object;
    (void)i2c;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t i2c_validate(transport_i2c_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}

static void i2c_tx_dma_start(transport_i2c_t *i2c, uint8_t *data, uint16_t size) {
    i2c_tx_dma_stop(i2c);
    dma_periphery_tx_start((uint32_t) & (I2C_DR(i2c->address)), i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->properties->dma_tx_channel, data, size);
    i2c_enable_txdma(i2c->address);
}

static app_signal_t i2c_construct(transport_i2c_t *i2c) {
    return 0;
}

static app_signal_t i2c_start(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_stop(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_tick(transport_i2c_t *i2c, uint32_t time_passed, uint32_t *next_tick) {
    (void)i2c;
    (void)time_passed;
    (void)next_tick;
    return 0;
}

static app_signal_t i2c_link(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_phase(transport_i2c_t *i2c, device_phase_t phase) {
    (void)i2c;
    (void)phase;
    return 0;
}

device_methods_t transport_i2c_methods = {.validate = (app_method_t) i2c_validate,
                                             .construct = (app_method_t)i2c_construct,
                                             .link = (app_method_t) i2c_link,
                                             .start = (app_method_t) i2c_start,
                                             .stop = (app_method_t) i2c_stop,
                                             .callback_phase = (app_signal_t (*)(void *, device_phase_t phase))i2c_phase,
                                             .property_write = i2c_property_write};
