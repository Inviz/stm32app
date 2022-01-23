#include "blank.h"


static ODR_t blank_property_write(OD_stream_t *stream, const void *buf, OD_size_t count,
                                            OD_size_t *countWritten) {
    device_blank_t *blank = stream->object;
    (void)blank;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t blank_validate(device_blank_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}

static app_signal_t blank_construct(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static app_signal_t blank_start(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static app_signal_t blank_stop(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static app_signal_t blank_link(device_blank_t *blank) {
    (void)blank;
    return 0;
}

static app_signal_t blank_phase(device_blank_t *blank, device_phase_t phase) {
    (void)blank;
    (void)phase;
    return 0;
}

device_methods_t device_blank_methods = {.validate = (app_method_t) blank_validate,
                                             .construct = (app_method_t)blank_construct,
                                             .link = (app_method_t) blank_link,
                                             .start = (app_method_t) blank_start,
                                             .stop = (app_method_t) blank_stop,
                                             .callback_phase = (device_callback_phase_t)blank_phase,
                                             .property_write = blank_property_write};
