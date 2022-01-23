#include "sensor.h"


static ODR_t sensor_property_write(OD_stream_t *stream, const void *buf, OD_size_t count,
                                             OD_size_t *countWritten) {
    input_sensor_t *sensor = stream->object;
    (void)sensor;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t sensor_validate(input_sensor_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}

static app_signal_t sensor_phase_constructing(input_sensor_t *sensor) {
    return 0;
}

static app_signal_t sensor_phase_starting(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static app_signal_t sensor_phase_stoping(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static app_signal_t sensor_phase_pausing(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static app_signal_t sensor_phase_resuming(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}


// pass over adc value to the linked device
static app_signal_t sensor_receive(input_sensor_t *sensor, device_t *device, void *value, void *channel) {
    (void) channel; /*unused*/
    if (sensor->adc->device == device) {
        device_send(sensor->device, sensor->target_device, value, sensor->target_argument);
    }
    return 0;
}

static app_signal_t sensor_phase_linking(input_sensor_t *sensor) {
    return device_link(sensor->device, (void **)&sensor->adc, sensor->properties->adc_index, (void *) (uint32_t) sensor->properties->adc_channel);
}

static app_signal_t sensor_accept(input_sensor_t *sensor, device_t *target, void *argument) {
    sensor->target_device = target;
    sensor->target_argument = argument;
    return 0;
}

static app_signal_t sensor_phase(input_sensor_t *sensor, device_phase_t phase) {
    (void)sensor;
    (void)phase;
    return 0;
}

device_methods_t input_sensor_methods = {
    .validate = (app_method_t) sensor_validate,
    .phase_constructing = (app_method_t)sensor_phase_constructing,
    .phase_linking = (app_method_t) sensor_phase_linking,
    .phase_starting = (app_method_t) sensor_phase_starting,
    .phase_stoping = (app_method_t) sensor_phase_stoping,
    .phase_pausing = (app_method_t) sensor_phase_pausing,
    .phase_resuming = (app_method_t) sensor_phase_resuming,
    .callback_link = (app_signal_t (*)(void *, device_t *device, void *channel))sensor_accept,
    .callback_value = (app_signal_t (*)(void *, device_t *device, void *value, void *channel))sensor_receive,
    .callback_phase = (app_signal_t (*)(void *, device_phase_t phase))sensor_phase,
    .property_write = sensor_property_write};
