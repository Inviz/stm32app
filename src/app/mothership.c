#include "mothership.h"
//#include "device/circuit.h"
//#include "devices/modbus.h"
#include "system/mcu.h"
#include "system/canopen.h"
#include "input/sensor.h"
#include "module/adc.h"
#include "storage/w25.h"
//#include "screen/epaper.h"
//#include "transport/can.h"
//#include "transport/i2c.h"
#include "transport/spi.h"
//#include "transport/usart.h"

static ODR_t mothership_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    app_mothership_t *mothership = stream->object;
    (void)mothership;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);

    return result;
}

static app_signal_t mothership_validate(app_mothership_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}

static app_signal_t mothership_phase_constructing(app_mothership_t *mothership) {
    app_threads_allocate((app_t *)mothership);
    return 0;
}

static app_signal_t mothership_phase_starting(app_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static app_signal_t mothership_phase_stoping(app_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static app_signal_t mothership_phase_pausing(app_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static app_signal_t mothership_phase_resuming(app_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static app_signal_t mothership_phase_linking(app_mothership_t *mothership) {
    device_link(mothership->device, (void **)&mothership->mcu, mothership->properties->mcu_index, NULL);
    device_link(mothership->device, (void **)&mothership->canopen, mothership->properties->canopen_index, NULL);
    device_link(mothership->device, (void **)&mothership->timer, mothership->properties->timer_index, NULL);
    return 0;
}

static app_signal_t mothership_phase(app_mothership_t *mothership, device_phase_t phase) {
    (void)mothership;
    (void)phase;
    return 0;
}

// Initialize all devices of all types found in OD
// If devices is NULL, it will only count the devices
size_t app_mothership_enumerate_devices(app_t *app, OD_t *od, device_t *destination) {
    size_t count = 0;
    count += app_device_type_enumerate(app, od, APP, &app_mothership_methods, sizeof(app_mothership_t), destination, count);
    count += app_device_type_enumerate(app, od, SYSTEM_MCU, &system_mcu_methods, sizeof(system_mcu_t), destination, count);
    count += app_device_type_enumerate(app, od, SYSTEM_CANOPEN, &system_canopen_methods, sizeof(system_canopen_t), destination, count);
    count += app_device_type_enumerate(app, od, MODULE_TIMER, &module_timer_methods, sizeof(module_timer_t), destination, count);
    count += app_device_type_enumerate(app, od, MODULE_ADC, &module_adc_methods, sizeof(module_adc_t), destination, count);
    count += app_device_type_enumerate(app, od, TRANSPORT_CAN, &transport_can_methods, sizeof(transport_can_t), destination, count);
    count += app_device_type_enumerate(app, od, TRANSPORT_SPI, &transport_spi_methods, sizeof(transport_spi_t), destination, count);
    count += app_device_type_enumerate(app, od, STORAGE_W25, &storage_w25_methods, sizeof(storage_w25_t), destination, count);
    // count += app_device_type_enumerate(app, od, TRANSPORT_SPI, &transport_spi_methods, sizeof(transport_spi_t), destination, count);
    // count += app_device_type_enumerate(MODULE_USART, &transport_usart_methods, sizeof(transport_usart_t), destination, count);
    // count += app_device_type_enumerate(app, od, TRANSPORT_I2C, &transport_i2c_methods, sizeof(transport_i2c_t), destination, count);
    // count += app_device_type_enumerate(app, od, DEVICE_CIRCUIT, &device_circuit_methods, sizeof(device_circuit_t), destination, count);
    // count += app_device_type_enumerate(app, OD, SCREEN_EPAPER, &screen_epaper_methods, sizeof(screen_epaper_t), destination, count);
    // count += app_device_type_enumerate(app, od, INPUT_SENSOR, &input_sensor_methods, sizeof(input_sensor_t), destination, count);
    return count;
}

static app_signal_t mothership_high_priority(app_mothership_t *mothership, app_event_t *event, device_tick_t *tick, app_thread_t *thread) {
    (void)tick;
    (void)thread;
    if (event->type == APP_EVENT_THREAD_START) {
        module_timer_timeout(mothership->timer, mothership->device, (void *)123, 1000000);
        /*
        return app_publish(mothership->device->app, &((app_event_t){
            .type = APP_EVENT_INTROSPECTION,
            .producer = mothership->device,
            .consumer = app_device_find_by_type((app_t *) mothership, STORAGE_W25)
        }))*/
    }
    return 0;
}

static app_signal_t mothership_callback_signal(app_mothership_t mothership, device_t *device, uint32_t signal, void *argument) {
    log_printf("Got signal!\n");
}

device_methods_t app_mothership_methods = {
    .validate = (app_method_t)mothership_validate,
    .phase_constructing = (app_method_t)mothership_phase_constructing,
    .phase_linking = (app_method_t)mothership_phase_linking,
    .phase_starting = (app_method_t)mothership_phase_starting,
    .phase_stoping = (app_method_t)mothership_phase_stoping,
    .phase_pausing = (app_method_t)mothership_phase_pausing,
    .phase_resuming = (app_method_t)mothership_phase_resuming,
    //.accept = (int (*)(void *, device_t *device, void *channel))app_mothership_accept,
    .callback_phase = (app_signal_t(*)(void *, device_phase_t phase))mothership_phase,
    .callback_signal = (app_signal_t(*)(void *, device_t *device, uint32_t signal, void *argument))mothership_callback_signal,
    .tick_high_priority = (device_tick_callback_t)mothership_high_priority,
    .property_write = mothership_property_write,
};
