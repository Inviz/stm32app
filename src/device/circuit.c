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
    case SUBIDX_CIRCUIT_DUTY_CYCLE:
        log_printf("OD - Circuit [%X] duty cycle:  %i\n", circuit->device->seq, circuit->properties->duty_cycle);
        break;
    case SUBIDX_CIRCUIT_CONSUMERS: log_printf("OD - Circuit [%X] consumers: %i\n", circuit->device->seq, circuit->properties->consumers); break;
    }
    return result;
}

/* Circuit needs its relay GPIO configured */
static app_signal_t circuit_validate(device_circuit_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED || properties->port == 0 || properties->pin == 0;
}

static app_signal_t circuit_phase_constructing(device_circuit_t *circuit) {
    return 0;
}

static app_signal_t circuit_phase_linking(device_circuit_t *circuit) {
    return device_link(circuit->device, (void **)&circuit->current_sensor, circuit->properties->sensor_index, NULL) +
           device_link(circuit->device, (void **)&circuit->psu, circuit->properties->psu_index, NULL);
}

// receive value from current sensor
static app_signal_t circuit_receive(device_circuit_t *circuit, device_t *device, void *value, void *argument) {
    (void)argument;
    if (circuit->current_sensor->device == device) {
        device_circuit_set_current(circuit, (uint16_t)((uint32_t)value));
    }
    return 1;
}

static app_signal_t circuit_phase_starting(device_circuit_t *circuit) {
    device_gpio_configure("Relay", circuit->properties->port, circuit->properties->pin, 0);
    //device_gpio_set_state(device_circuit_get_state(circuit));

    return 0;
}

static app_signal_t circuit_phase_stoping(device_circuit_t *circuit) {
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

device_methods_t device_circuit_methods = {
    .validate = (app_method_t) circuit_validate,
    .phase_constructing = (app_method_t)circuit_phase_constructing,
    .phase_starting = (app_method_t) circuit_phase_starting,
    .phase_stoping = (app_method_t) circuit_phase_stoping,
    .phase_linking = (app_method_t) circuit_phase_linking,
    .callback_value = (app_signal_t (*)(void *, device_t *device, void *value, void *argument))circuit_receive,
    .property_write = circuit_property_write,
};
