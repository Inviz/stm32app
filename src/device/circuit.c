#include "circuit.h"

static ODR_t circuit_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    device_circuit_t *circuit = stream->object;
    bool was_on = device_circuit_get_state(circuit);
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    bool is_on = device_circuit_get_state(circuit);
    if (is_on != was_on) {
        device_circuit_set_state(circuit, is_on);
    }
    switch (stream->subIndex) {
    case DEVICE_CIRCUIT_DUTY_CYCLE:
        log_printf("OD - Circuit [%X] duty cycle:  %i\n", circuit->actor->seq, circuit->properties->duty_cycle);
        break;
    case DEVICE_CIRCUIT_CONSUMERS:
        log_printf("OD - Circuit [%X] consumers: %i\n", circuit->actor->seq, circuit->properties->consumers);
        break;
    }
    return result;
}

/* Circuit needs its relay GPIO configured */
static app_signal_t circuit_validate(device_circuit_properties_t *properties) {
    return properties->port == 0 || properties->pin == 0;
}

static app_signal_t circuit_construct(device_circuit_t *circuit) {
    (void)circuit;
    return 0;
}

static app_signal_t circuit_link(device_circuit_t *circuit) {
    return actor_link(circuit->actor, (void **)&circuit->current_sensor, circuit->properties->sensor_index, NULL) +
           actor_link(circuit->actor, (void **)&circuit->psu, circuit->properties->psu_index, NULL);
}

// receive value from current sensor
static app_signal_t circuit_on_value(device_circuit_t *circuit, actor_t *actor, void *value, void *argument) {
    (void)argument;
    if (circuit->current_sensor->actor == actor) {
        device_circuit_set_current(circuit, (uint16_t)((uint32_t)value));
    }
    return 1;
}

static app_signal_t circuit_start(device_circuit_t *circuit) {
    actor_gpio_configure_input("Relay", circuit->properties->port, circuit->properties->pin, 0);
    // actor_gpio_set_state(device_circuit_get_state(circuit));

    return 0;
}

static app_signal_t circuit_stop(device_circuit_t *circuit) {
    (void)circuit;
    return 0;
}

bool_t device_circuit_get_state(device_circuit_t *circuit) {
    return circuit->properties->duty_cycle > 0 || circuit->properties->consumers > 0;
}

void device_circuit_set_state(device_circuit_t *circuit, bool state) {
    gpio_set_state(circuit->properties->port, circuit->properties->pin, state);
    if (circuit->psu) {
        device_circuit_set_consumers(circuit->psu, circuit->psu->properties->consumers + (state ? 1 : -1));
    }
}

actor_class_t device_circuit_class = {
    .type = DEVICE_CIRCUIT,
    .size = sizeof(device_circuit_t),
    .phase_subindex = DEVICE_CIRCUIT_PHASE,
    .validate = (app_method_t)circuit_validate,
    .construct = (app_method_t)circuit_construct,
    .start = (app_method_t)circuit_start,
    .stop = (app_method_t)circuit_stop,
    .link = (app_method_t)circuit_link,
    .on_value = (actor_on_value_t)circuit_on_value,
    .property_write = circuit_property_write,
};
