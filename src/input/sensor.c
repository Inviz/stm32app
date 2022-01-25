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

// pass over adc value to the linked actor
static app_signal_t sensor_receive(input_sensor_t *sensor, actor_t *actor, void *value, void *channel) {
    (void)channel; /*unused*/
    if (sensor->adc->actor == actor) {
        actor_send(sensor->actor, sensor->target_actor, value, sensor->target_argument);
    }
    return 0;
}

static app_signal_t sensor_link(input_sensor_t *sensor) {
    return actor_link(sensor->actor, (void **)&sensor->adc, sensor->properties->adc_index,
                       (void *)(uint32_t)sensor->properties->adc_channel);
}

static app_signal_t sensor_accept(input_sensor_t *sensor, actor_t *target, void *argument) {
    sensor->target_actor = target;
    sensor->target_argument = argument;
    return 0;
}

static app_signal_t sensor_phase(input_sensor_t *sensor, actor_phase_t phase) {
    (void)sensor;
    (void)phase;
    return 0;
}

actor_class_t input_sensor_class = {
    .type = INPUT_SENSOR,
    .size = sizeof(input_sensor_t),
    .phase_subindex = INPUT_SENSOR_PHASE,
    .validate = (app_method_t)sensor_validate,
    .construct = (app_method_t)sensor_construct,
    .link = (app_method_t)sensor_link,
    .start = (app_method_t)sensor_start,
    .stop = (app_method_t)sensor_stop,
    .on_link = (actor_on_link_t)sensor_accept,
    .on_value = (actor_on_value_t)sensor_receive,
    .on_phase = (actor_on_phase_t)sensor_phase,
    .property_write = sensor_property_write,
};
