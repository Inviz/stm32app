#include "CO_application.h"
#include "devices.h"
#include "device/circuit.h"
#include "screen/epaper.h"
//#include "devices/modbus.h"
#include "input/sensor.h"
#include "module/adc.h"
#include "module/mcu.h"
#include "transport/can.h"
#include "transport/spi.h"
#include "transport/usart.h"
#include "transport/i2c.h"

#include "OD.h"
#include "CANopen.h"

// Initialize all devices of all types found in OD
// If devices is NULL, it will only count the devices
size_t devices_enumerate(device_t *destination) {
    size_t count = 0;
    count += devices_enumerate_type(MODULE_MCU, &module_mcu_callbacks, sizeof(module_mcu_t), destination, count);
    count += devices_enumerate_type(MODULE_ADC, &module_adc_callbacks, sizeof(module_adc_t), destination, count);
    count += devices_enumerate_type(TRANSPORT_CAN, &transport_can_callbacks, sizeof(transport_can_t), destination, count);
    count += devices_enumerate_type(TRANSPORT_SPI, &transport_spi_callbacks, sizeof(transport_spi_t), destination, count);
    //count += devices_enumerate_type(MODULE_USART, &transport_usart_callbacks, sizeof(transport_usart_t), destination, count);
    count += devices_enumerate_type(TRANSPORT_I2C, &transport_i2c_callbacks, sizeof(transport_i2c_t), destination, count);
    count += devices_enumerate_type(DEVICE_CIRCUIT, &device_circuit_callbacks, sizeof(device_circuit_t), destination, count);
    count += devices_enumerate_type(SCREEN_EPAPER, &screen_epaper_callbacks, sizeof(screen_epaper_t), destination, count);
    count += devices_enumerate_type(INPUT_SENSOR, &input_sensor_callbacks, sizeof(input_sensor_t), destination, count);
    return count;
}

/******************************************************************************/
CO_ReturnError_t app_programConfigure(CO_t *co) {
    devices_allocate();
    devices_set_phase(DEVICE_CONSTRUCTING);
    devices_set_phase(DEVICE_LINKING);
    devices_set_phase(DEVICE_STARTING);


    /* Find CAN module that is used for CANopen*/
    transport_can_t *can;
    for (size_t i = 0; i < device_count; i++) {
        if (devices[i].type == TRANSPORT_CAN && devices[i].phase != DEVICE_DISABLED) {
            can = ((transport_can_t *) devices[i].object);
            if (!can->config->canopen) {
                can->config = NULL;
            } else {
                break;
            }
        }
    }

    /* Use that module to configure parameters*/
    if (can == NULL) {
        return 1;
    } else {
        co->CANmodule->port = can->device->seq == 0 ? CAN1 : CAN2;
        co->CANmodule->rxFifoIndex = can->config->canopen_fifo_index;
        co->CANmodule->sjw = can->config->sjw;
        co->CANmodule->prop = can->config->prop;
        co->CANmodule->brp = can->config->brp;
        co->CANmodule->ph_seg1 = can->config->ph_seg1;
        co->CANmodule->ph_seg2 = can->config->ph_seg2;
        co->CANmodule->bitrate = can->config->bitrate;
        return 0;
    }

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