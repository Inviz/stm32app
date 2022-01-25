#include "sensor.h"

static ODR_t sensor_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    input_sensor_t *sensor = stream->object;
    (void)sensor;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t sensor_validate(input_sensor_properties_t *properties) {
    return 0;
}

static app_signal_t sensor_construct(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static app_signal_t sensor_start(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static app_signal_t sensor_stop(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

// pass over adc value to the linked device
static app_signal_t sensor_receive(input_sensor_t *sensor, device_t *device, void *value, void *channel) {
    (void)channel; /*unused*/
    if (sensor->adc->device == device) {
        device_send(sensor->device, sensor->target_device, value, sensor->target_argument);
    }
    return 0;
}

static app_signal_t sensor_link(input_sensor_t *sensor) {
    return device_link(sensor->device, (void **)&sensor->adc, sensor->properties->adc_index,
                       (void *)(uint32_t)sensor->properties->adc_channel);
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

device_class_t input_sensor_class = {
    .type = INPUT_SENSOR,
    .size = sizeof(input_sensor_t),
    .phase_subindex = INPUT_SENSOR_PHASE,
    .validate = (app_method_t)sensor_validate,
    .construct = (app_method_t)sensor_construct,
    .link = (app_method_t)sensor_link,
    .start = (app_method_t)sensor_start,
    .stop = (app_method_t)sensor_stop,
    .on_link = (device_on_link_t)sensor_accept,
    .on_value = (device_on_value_t)sensor_receive,
    .on_phase = (device_on_phase_t)sensor_phase,
    .property_write = sensor_property_write,
};
