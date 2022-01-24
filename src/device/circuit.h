#ifndef INC_DEV_CIRCUIT
#define INC_DEV_CIRCUIT

#ifdef __cplusplus
extern "C" {
#endif

#include "input/sensor.h"
#include "core/device.h"
#include "module/adc.h"

/* Start of autogenerated OD types */
/* 0x4000: Device Circuit 1
   A relay that turns circuit on and off, possibly using PWM. Can be paired with Hall Sensor to measure current for taking metrics via ADC. Applying current limit will turn relay into a circuit breaker. */
typedef struct device_circuit_properties {
    uint8_t parameter_count;
    uint8_t port; // Relay GPIO port (1 for A, 2 for B, etc), required 
    uint8_t pin; // Relay GPIO pin, required 
    uint16_t limit_current; // Relay will turn off if measured current surpasses this limit 
    uint16_t limit_voltage; // Currently there is no way to enforce voltage limit 
    uint16_t psu_index; // PSU circuit is turned on when one of their dependends is turned on 
    uint16_t sensor_index; // Hall sensor measures current and allows circuit to become a circuit breaker 
    uint8_t phase; // Phase x 
    uint16_t duty_cycle;
    uint16_t current; // Current  x 
    uint16_t voltage;
    uint8_t consumers;
} device_circuit_properties_t;
/* End of autogenerated OD types */

struct device_circuit {
    device_t *device;
    device_circuit_properties_t *properties;
    device_circuit_t *psu;
    input_sensor_t *current_sensor;
    module_adc_t *adc;;
};

extern device_class_t device_circuit_class;

void device_circuit_set_state(device_circuit_t *circuit, bool state);
bool_t device_circuit_get_state(device_circuit_t *circuit);

/* Start of autogenerated OD accessors */
typedef enum device_circuit_properties_properties {
  DEVICE_CIRCUIT_PORT = 0x1,
  DEVICE_CIRCUIT_PIN = 0x2,
  DEVICE_CIRCUIT_LIMIT_CURRENT = 0x3,
  DEVICE_CIRCUIT_LIMIT_VOLTAGE = 0x4,
  DEVICE_CIRCUIT_PSU_INDEX = 0x5,
  DEVICE_CIRCUIT_SENSOR_INDEX = 0x6,
  DEVICE_CIRCUIT_PHASE = 0x7,
  DEVICE_CIRCUIT_DUTY_CYCLE = 0x8,
  DEVICE_CIRCUIT_CURRENT = 0x9,
  DEVICE_CIRCUIT_VOLTAGE = 0x10,
  DEVICE_CIRCUIT_CONSUMERS = 0x11
} device_circuit_properties_properties_t;

/* 0x40XX07: Phase x */
#define device_circuit_set_phase(circuit, value) OD_set_u8(circuit->device->properties, DEVICE_CIRCUIT_PHASE, value, false)
#define device_circuit_get_phase(circuit) *((uint8_t *) OD_getPtr(circuit->device->properties, DEVICE_CIRCUIT_PHASE, 0, NULL))
/* 0x40XX08: null */
#define device_circuit_set_duty_cycle(circuit, value) OD_set_u16(circuit->device->properties, DEVICE_CIRCUIT_DUTY_CYCLE, value, false)
#define device_circuit_get_duty_cycle(circuit) *((uint16_t *) OD_getPtr(circuit->device->properties, DEVICE_CIRCUIT_DUTY_CYCLE, 0, NULL))
/* 0x40XX09: Current  x */
#define device_circuit_set_current(circuit, value) OD_set_u16(circuit->device->properties, DEVICE_CIRCUIT_CURRENT, value, false)
#define device_circuit_get_current(circuit) *((uint16_t *) OD_getPtr(circuit->device->properties, DEVICE_CIRCUIT_CURRENT, 0, NULL))
/* 0x40XX0a: null */
#define device_circuit_set_voltage(circuit, value) OD_set_u16(circuit->device->properties, DEVICE_CIRCUIT_VOLTAGE, value, false)
#define device_circuit_get_voltage(circuit) *((uint16_t *) OD_getPtr(circuit->device->properties, DEVICE_CIRCUIT_VOLTAGE, 0, NULL))
/* 0x40XX0b: null */
#define device_circuit_set_consumers(circuit, value) OD_set_u8(circuit->device->properties, DEVICE_CIRCUIT_CONSUMERS, value, false)
#define device_circuit_get_consumers(circuit) *((uint8_t *) OD_getPtr(circuit->device->properties, DEVICE_CIRCUIT_CONSUMERS, 0, NULL))
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* objectAccessOD_H */