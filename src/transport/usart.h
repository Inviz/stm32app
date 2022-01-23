#ifndef INC_DEV_USART
#define INC_DEV_USART

#ifdef __cplusplus
extern "C" {
#endif

#include "core/device.h"
#include <libopencm3/stm32/usart.h>

#define USART_RX_BUFFER_SIZE 64

/* Start of autogenerated OD types */
/* 0x6240: Transport USART 1
   Serial protocol */
typedef struct transport_usart_properties {
    uint8_t parameter_count;
    uint8_t dma_rx_unit;
    uint8_t dma_rx_stream;
    uint8_t dma_rx_channel;
    uint8_t dma_rx_buffer_size;
    uint8_t dma_tx_unit;
    uint8_t dma_tx_stream;
    uint8_t dma_tx_channel;
    uint32_t baudrate;
    uint8_t databits;
    uint8_t phase;
} transport_usart_properties_t;
/* End of autogenerated OD types */

struct transport_usart {
    device_t *device;
    transport_usart_properties_t *properties;
    uint32_t clock;
    uint32_t address;
    uint32_t dma_tx_address;
    uint32_t dma_rx_address;
    device_t *target_device;
    void *target_argument;

    uint8_t *dma_rx_buffer;
};

extern device_methods_t transport_usart_methods;

int transport_usart_send(transport_usart_t *usart, char *data, int size);


/* Start of autogenerated OD accessors */
#define SUBIDX_USART_DMA_RX_UNIT 0x1
#define SUBIDX_USART_DMA_RX_STREAM 0x2
#define SUBIDX_USART_DMA_RX_CHANNEL 0x3
#define SUBIDX_USART_DMA_RX_BUFFER_SIZE 0x4
#define SUBIDX_USART_DMA_TX_UNIT 0x5
#define SUBIDX_USART_DMA_TX_STREAM 0x6
#define SUBIDX_USART_DMA_TX_CHANNEL 0x7
#define SUBIDX_USART_BAUDRATE 0x8
#define SUBIDX_USART_DATABITS 0x9
#define SUBIDX_USART_PHASE 0x10

ODR_t transport_usart_set_dma_rx_unit(transport_usart_t *usart, uint8_t value); // 0x62XX01: usart properties dma_rx_unit
uint8_t transport_usart_get_dma_rx_unit(transport_usart_t *usart); // 0x62XX01: usart properties dma_rx_unit
ODR_t transport_usart_set_dma_rx_stream(transport_usart_t *usart, uint8_t value); // 0x62XX02: usart properties dma_rx_stream
uint8_t transport_usart_get_dma_rx_stream(transport_usart_t *usart); // 0x62XX02: usart properties dma_rx_stream
ODR_t transport_usart_set_dma_rx_channel(transport_usart_t *usart, uint8_t value); // 0x62XX03: usart properties dma_rx_channel
uint8_t transport_usart_get_dma_rx_channel(transport_usart_t *usart); // 0x62XX03: usart properties dma_rx_channel
ODR_t transport_usart_set_dma_rx_buffer_size(transport_usart_t *usart, uint8_t value); // 0x62XX04: usart properties dma_rx_buffer_size
uint8_t transport_usart_get_dma_rx_buffer_size(transport_usart_t *usart); // 0x62XX04: usart properties dma_rx_buffer_size
ODR_t transport_usart_set_dma_tx_unit(transport_usart_t *usart, uint8_t value); // 0x62XX05: usart properties dma_tx_unit
uint8_t transport_usart_get_dma_tx_unit(transport_usart_t *usart); // 0x62XX05: usart properties dma_tx_unit
ODR_t transport_usart_set_dma_tx_stream(transport_usart_t *usart, uint8_t value); // 0x62XX06: usart properties dma_tx_stream
uint8_t transport_usart_get_dma_tx_stream(transport_usart_t *usart); // 0x62XX06: usart properties dma_tx_stream
ODR_t transport_usart_set_dma_tx_channel(transport_usart_t *usart, uint8_t value); // 0x62XX07: usart properties dma_tx_channel
uint8_t transport_usart_get_dma_tx_channel(transport_usart_t *usart); // 0x62XX07: usart properties dma_tx_channel
ODR_t transport_usart_set_baudrate(transport_usart_t *usart, uint32_t value); // 0x62XX08: usart properties baudrate
uint32_t transport_usart_get_baudrate(transport_usart_t *usart); // 0x62XX08: usart properties baudrate
ODR_t transport_usart_set_databits(transport_usart_t *usart, uint8_t value); // 0x62XX09: usart properties databits
uint8_t transport_usart_get_databits(transport_usart_t *usart); // 0x62XX09: usart properties databits
ODR_t transport_usart_set_phase(transport_usart_t *usart, uint8_t value); // 0x62XX0a: usart properties phase
uint8_t transport_usart_get_phase(transport_usart_t *usart); // 0x62XX0a: usart properties phase
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif