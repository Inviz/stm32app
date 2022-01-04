#include "CO_application.h"
#include "devices/circuit.h"
#include "devices/devices.h"
#include "devices/epaper.h"
#include "devices/modbus.h"
#include "devices/sensor.h"
#include "modules/adc.h"
#include "modules/can.h"
#include "modules/mcu.h"
#include "modules/spi.h"

#include "OD.h"

// Initialize all devices of all types found in OD
// If devices is NULL, it will only count the devices
size_t devices_enumerate(device_t *destination) {
    size_t count = 0;
    count += devices_enumerate_type(MODULE_MCU, &module_mcu_callbacks, sizeof(module_mcu_t), destination, count);
    count += devices_enumerate_type(MODULE_CAN, &module_can_callbacks, sizeof(module_can_t), destination, count);
    count += devices_enumerate_type(MODULE_ADC, &module_adc_callbacks, sizeof(module_adc_t), destination, count);
    count += devices_enumerate_type(MODULE_SPI, &module_spi_callbacks, sizeof(module_spi_t), destination, count);
    count += devices_enumerate_type(DEVICE_CIRCUIT, &device_circuit_callbacks, sizeof(device_circuit_t), destination, count);
    count += devices_enumerate_type(DEVICE_EPAPER, &device_epaper_callbacks, sizeof(device_epaper_t), destination, count);
    count += devices_enumerate_type(DEVICE_SENSOR, &device_sensor_callbacks, sizeof(device_sensor_t), destination, count);
    return count;
}

/******************************************************************************/
CO_ReturnError_t app_programBoot() {
    devices_allocate();
    devices_set_phase(DEVICE_CONSTRUCTING);
    devices_set_phase(DEVICE_LINKING);
    devices_set_phase(DEVICE_STARTING);

    return 0;
}

/******************************************************************************/
CO_ReturnError_t app_programStart(uint16_t *bitRate, uint8_t *nodeId) {
    (void)bitRate; /* unused */
    (void)nodeId;  /* unused */

    return 0;
}

/******************************************************************************/
void app_communicationReset(CO_t *co) { (void)co; /* unused */ }


bool devices_initialized = false;

/******************************************************************************/
void app_programEnd(void) {}

/******************************************************************************/
void app_programAsync(CO_t *co, uint32_t time_passed, uint32_t *next_tick) {
    for (size_t i = 0; i < device_count; i++) {
        device_t *device = &devices[i];

        // handle temporary phases
        if (device->phase_delay != 0) {
            if (device_timeout_check(&device->phase_delay, time_passed, next_tick) == 0) {
                device_set_phase(device, device->phase);
            }
        }

        // run async logic
        if (device->callbacks->async != NULL) {
            device->callbacks->async(device, time_passed, next_tick);
        }
    }

    uint8_t LED_red, LED_green;

    LED_red = CO_LED_RED(co->LEDs, CO_LED_CANopen);
    LED_green = CO_LED_GREEN(co->LEDs, CO_LED_CANopen);

    (void)LED_red;
    (void)LED_green;

    // ensure at least 10hz
    if (next_tick != NULL && *next_tick > 100000)
        *next_tick = 100000;
}

/******************************************************************************/
void app_programRt(CO_t *co, uint32_t time_passed, uint32_t *next_tick) {
    (void)time_passed; /* unused */
    (void)co;          /* unused */

    // ensure at least 100hz
    if (next_tick != NULL && *next_tick > 10000)
        *next_tick = 10000;
}

/******************************************************************************/
void app_peripheralRead(CO_t *co, uint32_t timer1usDiff, uint32_t *timerNext_us) {
    (void)co;           /* unused */
    (void)timerNext_us; /* unused */
    devices_read(timer1usDiff);
    /*
      OD_RAM.x6001_circuit_1.current = (uint32_t) adc_samples[0];
      OD_RAM.x6002_circuit_2.current = (uint32_t) adc_samples[1];
      OD_requestTPDO(OD_6001_C1_flagsPDO, 1);

      log_printf("%6lu %6lu\r\n", adc_samples[0], adc_samples[1]);*/
}

/******************************************************************************/
void app_peripheralWrite(CO_t *co, uint32_t timer1usDiff, uint32_t *timerNext_us) {
    (void)co;           /* unused */
    (void)timerNext_us; /* unused */
    devices_write(timer1usDiff);
}