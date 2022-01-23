#ifndef INC_DEV_CIRCUIT
#define INC_DEV_CIRCUIT

#ifdef __cplusplus
extern "C" {
#endif

#include "input/sensor.h"
#include "core/device.h"
#include "module/adc.h"

/* Start of autogenerated OD types */
/* 0x3800: Device Circuit 1
   A relay that turns circuit on and off, possibly using PWM. Can be paired with Hall Sensor to measure current for taking metrics via ADC. Applying current limit will turn relay into a circuit breaker. */
typedef struct device_circuit_config {
    uint8_t parameter_count;
    int16_t disabled;
    uint8_t port;
    uint8_t pin;
    uint16_t limit_current;
    uint16_t limit_voltage;
    uint16_t psu_index;
    uint16_t sensor_index;
} device_circuit_config_t;
/* End of autogenerated OD types */

struct device_circuit {
    device_t *device;
    device_circuit_config_t *config;
    device_circuit_values_t *values;
    device_circuit_t *psu;
    input_sensor_t *current_sensor;
    module_adc_t *adc;;
};

extern device_methods_t device_circuit_methods;

void device_circuit_set_state(device_circuit_t *circuit, bool state);
bool_t device_circuit_get_state(device_circuit_t *circuit);

/* Start of autogenerated OD accessors */
#define SUBIDX_CIRCUIT_DISABLED 0x1
#define SUBIDX_CIRCUIT_PORT 0x2
#define SUBIDX_CIRCUIT_PIN 0x3
#define SUBIDX_CIRCUIT_LIMIT_CURRENT 0x4
#define SUBIDX_CIRCUIT_LIMIT_VOLTAGE 0x5
#define SUBIDX_CIRCUIT_PSU_INDEX 0x6
#define SUBIDX_CIRCUIT_SENSOR_INDEX 0x7

/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* objectAccessOD_H */