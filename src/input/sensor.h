#ifndef INC_DEV_SENSOR
#define INC_DEV_SENSOR

#ifdef __cplusplus
extern "C" {
#endif

#include "core/device.h"
#include "module/adc.h"

/* Start of autogenerated OD types */
/* 0x8000: Input Sensor 1
   A sensor that measures a single analog value (i.e. current meter, tank level meter) */
typedef struct input_sensor_properties {
    uint8_t parameter_count;
    uint16_t disabled;
    uint8_t port;
    uint8_t pin;
    uint16_t adc_index;
    uint8_t adc_channel;
    uint8_t phase;
} input_sensor_properties_t;
/* End of autogenerated OD types */

struct input_sensor {
    device_t *device;
    input_sensor_properties_t *properties;
    device_t *target_device;
    void *target_argument;
    module_adc_t *adc;
};


extern device_methods_t input_sensor_methods;

/* Start of autogenerated OD accessors */
#define SUBIDX_SENSOR_DISABLED 0x1
#define SUBIDX_SENSOR_PORT 0x2
#define SUBIDX_SENSOR_PIN 0x3
#define SUBIDX_SENSOR_ADC_INDEX 0x4
#define SUBIDX_SENSOR_ADC_CHANNEL 0x5
#define SUBIDX_SENSOR_PHASE 0x6

ODR_t input_sensor_set_disabled(input_sensor_t *sensor, uint16_t value); // 0x80XX01: sensor properties disabled
uint16_t input_sensor_get_disabled(input_sensor_t *sensor); // 0x80XX01: sensor properties disabled
ODR_t input_sensor_set_port(input_sensor_t *sensor, uint8_t value); // 0x80XX02: sensor properties port
uint8_t input_sensor_get_port(input_sensor_t *sensor); // 0x80XX02: sensor properties port
ODR_t input_sensor_set_pin(input_sensor_t *sensor, uint8_t value); // 0x80XX03: sensor properties pin
uint8_t input_sensor_get_pin(input_sensor_t *sensor); // 0x80XX03: sensor properties pin
ODR_t input_sensor_set_adc_index(input_sensor_t *sensor, uint16_t value); // 0x80XX04: sensor properties adc_index
uint16_t input_sensor_get_adc_index(input_sensor_t *sensor); // 0x80XX04: sensor properties adc_index
ODR_t input_sensor_set_adc_channel(input_sensor_t *sensor, uint8_t value); // 0x80XX05: sensor properties adc_channel
uint8_t input_sensor_get_adc_channel(input_sensor_t *sensor); // 0x80XX05: sensor properties adc_channel
ODR_t input_sensor_set_phase(input_sensor_t *sensor, uint8_t value); // 0x80XX06: sensor properties phase
uint8_t input_sensor_get_phase(input_sensor_t *sensor); // 0x80XX06: sensor properties phase
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif