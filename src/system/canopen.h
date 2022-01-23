#ifndef INC_SYSTEM_CANOPEN
#define INC_SYSTEM_CANOPEN

#ifdef __cplusplus
extern "C" {
#endif

#include "core/app.h"
#include "transport/can.h"

#include <CANopen.h>
#include "OD.h"



#define NMT_CONTROL                                                                                                                        \
    CO_NMT_STARTUP_TO_OPERATIONAL                                                                                                          \
    | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION
#define FIRST_HB_TIME 501
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK true
#define OD_STATUS_BITS NULL

#define CO_GET_CNT(obj) OD_CNT_##obj
#define OD_GET(entry, index) OD_ENTRY_##entry

#define app_error_report(app, errorBit, errorCode, index) CO_errorReport(app->canopen->instance->em, errorBit, errorCode, index)
#define app_error_reset(app, errorBit, errorCode, index) CO_errorReset(app->canopen->instance->em, errorBit, errorCode, index)

#define device_error_report(device, errorBit, errorCode) CO_errorReport(device->app->canopen->instance->em, errorBit, errorCode, device->index)
#define device_error_reset(device, errorBit, errorCode) CO_errorReset(device->app->canopen->instance->em, errorBit, errorCode, device->index)


/* Start of autogenerated OD types */
/* 0x6010: System CANopen
   CANOpen framework */
typedef struct system_canopen_config {
    uint8_t parameter_count;
    uint32_t disabled;
    uint16_t can_index;
    uint8_t can_fifo_index;
    uint8_t green_led_port;
    uint8_t green_led_pin;
    uint8_t red_led_port;
    uint8_t red_led_pin;
    uint16_t first_hb_time;
    uint16_t sdo_server_timeout;
    uint16_t sdo_client_timeout;
} system_canopen_config_t;
/* End of autogenerated OD types */

struct system_canopen {
    device_t *device;
    system_canopen_config_t *config;
    system_canopen_values_t *values;
    transport_can_t *can;
    CO_t *instance;
} ;


extern device_methods_t system_canopen_methods;


/* Start of autogenerated OD accessors */
#define SUBIDX_CANOPEN_DISABLED 0x1
#define SUBIDX_CANOPEN_CAN_INDEX 0x2
#define SUBIDX_CANOPEN_CAN_FIFO_INDEX 0x3
#define SUBIDX_CANOPEN_GREEN_LED_PORT 0x4
#define SUBIDX_CANOPEN_GREEN_LED_PIN 0x5
#define SUBIDX_CANOPEN_RED_LED_PORT 0x6
#define SUBIDX_CANOPEN_RED_LED_PIN 0x7
#define SUBIDX_CANOPEN_FIRST_HB_TIME 0x8
#define SUBIDX_CANOPEN_SDO_SERVER_TIMEOUT 0x9
#define SUBIDX_CANOPEN_SDO_CLIENT_TIMEOUT 0x10

/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif