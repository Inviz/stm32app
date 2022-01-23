#ifndef INC_DEV_CAN
#define INC_DEV_CAN

#ifdef __cplusplus
extern "C" {
#endif
#include "core/app.h"

/* Start of autogenerated OD types */
/* 0x6200: Transport CAN 1null */
typedef struct transport_can_properties {
    uint8_t parameter_count;
    uint8_t tx_port;
    uint8_t tx_pin;
    uint8_t rx_port;
    uint8_t rx_pin;
    int16_t bitrate;
    uint16_t brp;
    uint8_t sjw;
    uint8_t prop;
    uint8_t ph_seg1;
    uint8_t ph_seg2;
    uint8_t phase;
} transport_can_properties_t;
/* End of autogenerated OD types */

struct transport_can {
    device_t *device;
    transport_can_properties_t *properties;
    device_t *canopen;
};


extern device_methods_t transport_can_methods;

/* Start of autogenerated OD accessors */
#define SUBIDX_CAN_TX_PORT 0x1
#define SUBIDX_CAN_TX_PIN 0x2
#define SUBIDX_CAN_RX_PORT 0x3
#define SUBIDX_CAN_RX_PIN 0x4
#define SUBIDX_CAN_BITRATE 0x5
#define SUBIDX_CAN_BRP 0x6
#define SUBIDX_CAN_SJW 0x7
#define SUBIDX_CAN_PROP 0x8
#define SUBIDX_CAN_PH_SEG1 0x9
#define SUBIDX_CAN_PH_SEG2 0x10
#define SUBIDX_CAN_PHASE 0x11

ODR_t transport_can_set_tx_port(transport_can_t *can, uint8_t value); // 0x62XX01: can properties tx_port
uint8_t transport_can_get_tx_port(transport_can_t *can); // 0x62XX01: can properties tx_port
ODR_t transport_can_set_tx_pin(transport_can_t *can, uint8_t value); // 0x62XX02: can properties tx_pin
uint8_t transport_can_get_tx_pin(transport_can_t *can); // 0x62XX02: can properties tx_pin
ODR_t transport_can_set_rx_port(transport_can_t *can, uint8_t value); // 0x62XX03: can properties rx_port
uint8_t transport_can_get_rx_port(transport_can_t *can); // 0x62XX03: can properties rx_port
ODR_t transport_can_set_rx_pin(transport_can_t *can, uint8_t value); // 0x62XX04: can properties rx_pin
uint8_t transport_can_get_rx_pin(transport_can_t *can); // 0x62XX04: can properties rx_pin
ODR_t transport_can_set_bitrate(transport_can_t *can, int16_t value); // 0x62XX05: can properties bitrate
int16_t transport_can_get_bitrate(transport_can_t *can); // 0x62XX05: can properties bitrate
ODR_t transport_can_set_brp(transport_can_t *can, uint16_t value); // 0x62XX06: can properties brp
uint16_t transport_can_get_brp(transport_can_t *can); // 0x62XX06: can properties brp
ODR_t transport_can_set_sjw(transport_can_t *can, uint8_t value); // 0x62XX07: can properties sjw
uint8_t transport_can_get_sjw(transport_can_t *can); // 0x62XX07: can properties sjw
ODR_t transport_can_set_prop(transport_can_t *can, uint8_t value); // 0x62XX08: can properties prop
uint8_t transport_can_get_prop(transport_can_t *can); // 0x62XX08: can properties prop
ODR_t transport_can_set_ph_seg1(transport_can_t *can, uint8_t value); // 0x62XX09: can properties ph_seg1
uint8_t transport_can_get_ph_seg1(transport_can_t *can); // 0x62XX09: can properties ph_seg1
ODR_t transport_can_set_ph_seg2(transport_can_t *can, uint8_t value); // 0x62XX0a: can properties ph_seg2
uint8_t transport_can_get_ph_seg2(transport_can_t *can); // 0x62XX0a: can properties ph_seg2
ODR_t transport_can_set_phase(transport_can_t *can, uint8_t value); // 0x62XX0b: can properties phase
uint8_t transport_can_get_phase(transport_can_t *can); // 0x62XX0b: can properties phase
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif




#ifdef CO_CAN_INTERFACE
#ifdef STMF32F1
#if CO_CAN_RX_FIFO_INDEX == 0 || !defined(CO_CAN_RX_FIFO_INDEX)
void usb_hp_can_tx_isr(void) { CO_CANTxInterrupt(CO->CANmodule); }
void usb_lp_can_rx0_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#else
void usb_lp_can_rx1_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#endif
#else
#if CO_CAN_INTERFACE == CAN1
#if CO_CAN_RX_FIFO_INDEX == 0
void can1_tx_isr(void) { CO_CANTxInterrupt(CO->CANmodule); }
void can1_rx0_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#else
void can1_rx1_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#endif
#else
#if CO_CAN_RX_FIFO_INDEX == 0
void can2_tx_isr(void) { CO_CANTxInterrupt(CO->CANmodule); }
void can2_rx0_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#else
void can2_rx1_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#endif
#endif
#endif
#endif
