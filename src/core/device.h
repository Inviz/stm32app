#ifndef INC_CORE_DEVICE
#define INC_CORE_DEVICE

#ifdef __cplusplus
extern "C" {
#endif

#include "301/CO_ODinterface.h"
#include "core/app.h"
#include "core/thread.h"
#include "lib/gpio.h"

#define DEVICES_VALUES_OFFSET 4096
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

enum device_phase {
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
    DEVICE_DESTRUCTING
};

enum device_signal {
    SIGNAL_TIMER,
    SIGNAL_TIMEOUT,

    SIGNAL_DMA_ERROR,
    SIGNAL_DMA_TRANSFERRING,
    SIGNAL_DMA_IDLE,

    SIGNAL_RX_COMPLETE,
    SIGNAL_TX_COMPLETE

    SIGNAL_CATCHUP
};

char *string_from_phase(device_phase_t phase);

enum device_type {
    APP = 0x3000,

    // custom devices
    DEVICE_CIRCUIT = 0x3800,

    // basic features
    SYSTEM_MCU = 0x6000,
    SYSTEM_CANOPEN = 0x6020,
    SYSTEM_RTOS = 0x6040,

    // internal mcu modules
    MODULE_TIMER = 0x6100,
    MODULE_ADC = 0x6120,

    // communication modules
    TRANSPORT_CAN = 0x6100,
    TRANSPORT_SPI = 0x6120,
    TRANSPORT_USART = 0x6140,
    TRANSPORT_I2C = 0x6160,
    TRANSPORT_MODBUS = 0x6180,

    // input devices
    INPUT_SENSOR = 0x6800,

    // control periphery
    CONTROL_TOUCHSCREEN = 0x6900,

    // output devices
    SCREEN_EPAPER = 0x7000,
};

#define device_error_report(device, errorBit, errorCode)                                                                                   \
    CO_errorReport(device->app->canopen->instance->em, errorBit, errorCode, device->index)
#define device_error_reset(device, errorBit, errorCode)                                                                                    \
    CO_errorReset(device->app->canopen->instance->em, errorBit, errorCode, device->index)

struct device {
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
    device_callbacks_t *callbacks;   /* Per-class methods and callbacks */
    device_ticks_t *ticks;           /* Per-device thread subscription */
    app_t *app;                      /* Reference to root device */
    uint32_t event_subscriptions;    /* Mask for different event types that device recieves */
    bool_t *h;
};

struct device_callbacks {
    int (*validate)(OD_entry_t *config);                                            /* Check if config has all values */
    int (*construct)(void *object, device_t *device);                               /* Initialize device at given pointer*/
    int (*link)(void *object);                                                      /* Link related devices together*/
    int (*destruct)(void *object);                                                  /* Destruct device at given pointer*/
    int (*start)(void *object);                                                     /* Prepare periphery and run initial logic */
    int (*stop)(void *object);                                                      /* Reset periphery and deinitialize */
    int (*pause)(void *object);                                                     /* Put device to sleep temporarily */
    int (*resume)(void *object);                                                    /* Wake device up from sleep */
    int (*phase)(void *object, device_phase_t phase);                               /* Handle phase change */
    int (*receive)(void *object, device_t *device, void *value, void *argument);    /* Accept value from linked device */
    int (*accept)(void *object, device_t *origin, void *argument);                  /* Accept linking request*/
    int (*signal)(void *object, device_t *origin, uint32_t signal, void *argument); /* Send signal to device */

    int (*input_tick)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread);  /* Processing input events asap */
    int (*async_tick)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread);  /* Work that needs to be done later */
    int (*output_tick)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread); /* Medmium importance periphery work*/
    int (*poll_tick)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread);   /* Low-importance periodical work*/
    int (*idle_tick)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread);   /* Lowest priority work that i*/

    ODR_t (*read_config)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*write_config)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
    ODR_t (*read_values)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*write_values)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
};
/* Configure numeric port/pin combination as input */
void device_gpio_configure_input(char *name, uint8_t port, uint16_t pin);
/* Configure numeric port/pin combination as output */
void device_gpio_configure_output(char *name, uint8_t port, uint16_t pin);
/* Configure numeric port/pin combination as output, and set value accordingly */
void device_gpio_configure_output_with_value(char *name, uint8_t port, uint16_t pin, uint8_t value);

int device_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick);

/* Find device with given index and write it to the given pointer */
int device_link(device_t *device, void **destination, uint16_t index, void *arg);

/* Send value from device to another */
int device_send(device_t *device, device_t *target, void *value, void *argument);

/* Send signal from device to another*/
int device_signal(device_t *device, device_t *origin, uint32_t value, void *argument);

int device_allocate(device_t *device);
int device_free(device_t *device);

void device_set_phase(device_t *device, device_phase_t phase);
void device_set_temporary_phase(device_t *device, device_phase_t phase, uint32_t delay);

void device_gpio_set(uint8_t port, uint8_t pin);
void device_gpio_clear(uint8_t port, uint8_t pin);
uint32_t device_gpio_get(uint8_t port, uint8_t pin);

/* Check if event will invoke input tick on this device */
bool_t device_can_handle_event(device_t *device, app_event_t *event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif