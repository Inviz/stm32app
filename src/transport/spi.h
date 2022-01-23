#ifndef INC_SPI
#define INC_SPI

#ifdef __cplusplus
extern "C" {
#endif

#include "core/device.h"
#include "lib/dma.h"
#include "lib/vpool.h"
#include "lib/vpool.h"
#include "module/timer.h"
#include <libopencm3/stm32/spi.h>

/* Start of autogenerated OD types */
/* 0x6220: Transport SPI 1
   ADC Unit used for high-volume sampling of analog signals */
typedef struct transport_spi_properties {
    uint8_t parameter_count;
    uint8_t is_slave;
    uint8_t software_ss_control;
    uint8_t mode;
    uint8_t dma_rx_unit;
    uint8_t dma_rx_stream;
    uint8_t dma_rx_channel;
    int16_t dma_rx_idle_timeout;
    uint16_t rx_buffer_size;
    uint16_t rx_pool_max_size;
    uint16_t rx_pool_initial_size;
    uint16_t rx_pool_block_size;
    uint8_t dma_tx_unit;
    uint8_t dma_tx_stream;
    uint8_t dma_tx_channel;
    uint8_t af_index;
    uint8_t ss_port;
    uint8_t ss_pin;
    uint8_t sck_port;
    uint8_t sck_pin;
    uint8_t miso_port;
    uint8_t miso_pin;
    uint8_t mosi_port;
    uint8_t mosi_pin;
    uint8_t phase;
} transport_spi_properties_t;
/* End of autogenerated OD types */

struct transport_spi {
    device_t *device;
    transport_spi_properties_t *properties;
    uint32_t clock;
    uint32_t address;
    module_timer_t *timer;
    app_event_t reading;      // current reading job
    app_event_t writing;      // current writing job
    uint8_t *rx_buffer;        // circular buffer for DMA
    uint16_t rx_buffer_cursor; // current ingested position in rx buffer
    struct vpool rx_pool;      // pool that allocates growing memory chunk for recieved messages
};

extern device_class_t transport_spi_class;

/* Initiate Tx transmission */
int transport_spi_write(transport_spi_t *spi, device_t *writer, void *argument, uint8_t *tx_buffer, uint16_t tx_size);

/* Initiate Rx transmission */
int transport_spi_read(transport_spi_t *spi, device_t *reader, void *argument);



/* Start of autogenerated OD accessors */
typedef enum transport_spi_properties_properties {
  TRANSPORT_SPI_IS_SLAVE = 0x1,
  TRANSPORT_SPI_SOFTWARE_SS_CONTROL = 0x2,
  TRANSPORT_SPI_MODE = 0x3,
  TRANSPORT_SPI_DMA_RX_UNIT = 0x4,
  TRANSPORT_SPI_DMA_RX_STREAM = 0x5,
  TRANSPORT_SPI_DMA_RX_CHANNEL = 0x6,
  TRANSPORT_SPI_DMA_RX_IDLE_TIMEOUT = 0x7,
  TRANSPORT_SPI_RX_BUFFER_SIZE = 0x8,
  TRANSPORT_SPI_RX_POOL_MAX_SIZE = 0x9,
  TRANSPORT_SPI_RX_POOL_INITIAL_SIZE = 0x10,
  TRANSPORT_SPI_RX_POOL_BLOCK_SIZE = 0x11,
  TRANSPORT_SPI_DMA_TX_UNIT = 0x12,
  TRANSPORT_SPI_DMA_TX_STREAM = 0x13,
  TRANSPORT_SPI_DMA_TX_CHANNEL = 0x14,
  TRANSPORT_SPI_AF_INDEX = 0x15,
  TRANSPORT_SPI_SS_PORT = 0x16,
  TRANSPORT_SPI_SS_PIN = 0x17,
  TRANSPORT_SPI_SCK_PORT = 0x18,
  TRANSPORT_SPI_SCK_PIN = 0x19,
  TRANSPORT_SPI_MISO_PORT = 0x20,
  TRANSPORT_SPI_MISO_PIN = 0x21,
  TRANSPORT_SPI_MOSI_PORT = 0x22,
  TRANSPORT_SPI_MOSI_PIN = 0x23,
  TRANSPORT_SPI_PHASE = 0x24
} transport_spi_properties_properties_t;

#define transport_spi_set_phase(spi, value) OD_set_u8(spi->device->properties, TRANSPORT_SPI_PHASE, value, false)
#define transport_spi_get_phase(spi) *((uint8_t *) OD_getPtr(spi->device->properties, TRANSPORT_SPI_PHASE, 0, NULL))
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_SPI_H */