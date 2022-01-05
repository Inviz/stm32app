#ifndef CO_DEVICES_H
#define CO_DEVICES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "helpers/gpio.h"
#define DEVICES_VALUES_OFFSET 4096

void devices_init(uint32_t *errInfo);
void devices_setup(uint32_t *errInfo);
void devices_setup_adc(uint32_t *errInfo);
void devices_setup_spi(uint32_t *errInfo);
void devices_destroy(void);

void devices_read(uint32_t timer1usDiff);
void devices_write(uint32_t timer1usDiff);

/* define convenient getters and setters */
#define OD_ACCESSORS(OD_TYPE, NAME, SUBTYPE, PROPERTY, SUBINDEX, TYPE, SHORT_TYPE)                                                         \
    ODR_t OD_TYPE##_##NAME##_set_##PROPERTY(OD_TYPE##_##NAME##_t *NAME, TYPE value) {                                                      \
        return OD_set_##SHORT_TYPE(NAME->device->SUBTYPE, SUBINDEX, value, false);                                                         \
    }                                                                                                                                      \
    TYPE OD_TYPE##_##NAME##_get_##PROPERTY(OD_TYPE##_##NAME##_t *NAME) {                                                                   \
        TYPE value;                                                                                                                        \
        OD_get_##SHORT_TYPE(NAME->device->SUBTYPE, SUBINDEX, &value, false);                                                               \
        return value;                                                                                                                      \
    }   

typedef enum {
    DEVICE_ENABLED,
    DEVICE_CONSTRUCTING,
    DEVICE_LINKING,

    DEVICE_STARTING,
    DEVICE_CALIBRATING,
    DEVICE_PREPARING,
    DEVICE_RUNNING,

    DEVICE_REQUESTING,
    DEVICE_RESPONDING,

    DEVICE_BUSY,
    DEVICE_RESETTING,

    DEVICE_PAUSING,
    DEVICE_PAUSED,
    DEVICE_RESUMING,

    DEVICE_STOPPING,
    DEVICE_STOPPED,

    DEVICE_DISABLED,
    DEVICE_DESTRUCTING,
} device_phase_t;

typedef enum { DEVICE_TX_DONE, DEVICE_RX_DONE, DEVICE_TIMER } device_signal_t;

char *string_from_phase(device_phase_t phase);

typedef enum {
    // custom devices
    DEVICE_CIRCUIT = 0x3800,

    // basic features
    MODULE_MCU = 0x6000,
    MODULE_TIMER = 0x6020,
    MODULE_ADC = 0x6020,

    // communication modules
    TRANSPORT_CAN = 0x6100,
    TRANSPORT_SPI = 0x6120,
    TRANSPORT_USART = 0x6140,
    TRANSPORT_I2C = 0x6160,
    TRANSPORT_MODBUS = 0x6180,

    // input devices
    INPUT_SENSOR = 0x6800,

    CONTROL_TOUCHSCREEN = 0x6900,

    // output devices
    SCREEN_EPAPER = 0x7000,
} device_type_t;

typedef struct device_callbacks_t device_callbacks_t;

typedef struct {
    device_type_t type;              /* OD index of a first device of this type */
    uint8_t seq;                     /* Sequence number of the device in its family  */
    int16_t index;                   /* Actual OD address of this device */
    device_phase_t phase;            /* Current lifecycle phase of the device */
    uint32_t phase_delay;            /* Current lifecycle phase of the device */
    void *object;                    /* Pointer to the device own struct */
    size_t struct_size;              /* Memory requirements for device struct */
    OD_entry_t *config;              /* OD entry containing configuration for device*/
    OD_extension_t config_extension; /* OD IO handlers for config changes */
    OD_entry_t *values;              /* OD entry containing mutable values (optinal) */
    OD_extension_t values_extension; /* OD IO handlers for mutable changes */
    device_callbacks_t *callbacks;   /* Pointers to functions that work on device struct*/
} device_t;

struct device_callbacks_t {
    int (*validate)(OD_entry_t *config);                                         /* Check if config has all values */
    int (*construct)(void *object, device_t *device);                            /* Initialize device at given pointer*/
    int (*link)(void *object);                                                   /* Link related devices together*/
    int (*destruct)(void *object);                                               /* Destruct device at given pointer*/
    int (*start)(void *object);                                                  /* Prepare periphery and run initial logic */
    int (*stop)(void *object);                                                   /* Reset periphery and deinitialize */
    int (*pause)(void *object);                                                  /* Put device to sleep temporarily */
    int (*resume)(void *object);                                                 /* Wake device up from sleep */
    int (*tick)(void *object, uint32_t time_passed, uint32_t *next_tick);        /* Run periodical tick */
    int (*phase)(void *object, device_phase_t phase);                            /* Handle phase change */
    int (*receive)(void *object, device_t *device, void *value, void *argument); /* Accept value from linked device */
    int (*accept)(void *object, device_t *origin, void *arg);                    /* Accept linking request*/
    int (*signal)(void *object, device_t *origin, uint32_t signal, void *argument);   /* Send signal to device */
    int (*async)(void *object, uint32_t time_passed, uint32_t *next_tick);  /* Run asynchronous logic (>10hz) */
    int (*period)(void *object, uint32_t time_passed, uint32_t *next_tick); /* Run realtime logic (>100hz) */
    ODR_t (*read_config)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*write_config)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
    ODR_t (*read_values)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*write_values)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
};

extern device_t *devices;
extern size_t device_count;

// Initialize array of all devices found in OD that can be initialized
int devices_allocate(void);
// Destruct all devices and release memory
int devices_free(void);
// Transition all devices to given state
void devices_set_phase(device_phase_t phase);

/* Configure numeric port/pin combination as input */
void device_gpio_configure_input(char *name, uint8_t port, uint16_t pin);
/* Configure numeric port/pin combination as output */
void device_gpio_configure_output(char *name, uint8_t port, uint16_t pin);
/* Configure numeric port/pin combination as output, and set value accordingly */
void device_gpio_configure_output_with_value(char *name, uint8_t port, uint16_t pin, uint8_t value);

int device_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick);

size_t devices_enumerate_type(device_type_t type, device_callbacks_t *callbacks, size_t struct_size, device_t *destination, size_t offset);

/* Find device with given index and write it to the given pointer */
int device_link(device_t *device, void **destination, uint16_t index, void *arg);

/* Send value from device to another */
int device_send(device_t *device, device_t *target, void *value, void *argument);

/* Find device by index in the global list of registered devices */
device_t *find_device(uint16_t index);
/* Find device by type in the global list of registered devices */
device_t *find_device_by_type(uint16_t type);
/* Get numeric index of a device in a global array */
uint8_t get_device_number(device_t *device);
/* Return device from a global array by its index */
device_t *get_device_by_number(uint8_t number);
void device_set_phase(device_t *device, device_phase_t phase);
void device_set_temporary_phase(device_t *device, device_phase_t phase, uint32_t delay);

void device_gpio_set(uint8_t port, uint8_t pin);
void device_gpio_clear(uint8_t port, uint8_t pin);
uint32_t device_gpio_get(uint8_t port, uint8_t pin);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* objectAccessOD_H */