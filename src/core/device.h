#ifndef INC_CORE_DEVICE
#define INC_CORE_DEVICE

#ifdef __cplusplus
extern "C" {
#endif

#include "301/CO_ODinterface.h"
#include "core/app.h"
#include "core/thread.h"
#include "lib/gpio.h"

#define OD_ACCESSORS(OD_TYPE, NAME, SUBTYPE, PROPERTY, SUBINDEX, TYPE, SHORT_TYPE)                                                         \
    __attribute__((weak)) ODR_t OD_TYPE##_##NAME##_set_##PROPERTY(OD_TYPE##_##NAME##_t *NAME, TYPE value);                                 \
    __attribute__((weak)) ODR_t OD_TYPE##_##NAME##_set_##PROPERTY(OD_TYPE##_##NAME##_t *NAME, TYPE value) {                                \
        return OD_set_##SHORT_TYPE(NAME->device->SUBTYPE, SUBINDEX, value, false);                                                         \
    }                                                                                                                                      \
    __attribute__((weak)) TYPE OD_TYPE##_##NAME##_get_##PROPERTY(OD_TYPE##_##NAME##_t *NAME);                                              \
    __attribute__((weak)) TYPE OD_TYPE##_##NAME##_get_##PROPERTY(OD_TYPE##_##NAME##_t *NAME) {                                             \
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

    DEVICE_WORKING,
    DEVICE_IDLE,

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

enum device_type {
    APP = 0x3000,

    // custom devices
    DEVICE_CIRCUIT = 0x4000,

    // basic features
    SYSTEM_MCU = 0x6000,
    SYSTEM_CANOPEN = 0x6020,

    // internal mcu modules
    MODULE_TIMER = 0x6100,

    // communication modules
    TRANSPORT_CAN = 0x6200,
    TRANSPORT_SPI = 0x6220,
    TRANSPORT_USART = 0x6240,
    TRANSPORT_I2C = 0x6260,
    TRANSPORT_MODBUS = 0x6280,

    MODULE_ADC = 0x6300,

    STORAGE_W25 = 0x7100,

    // input devices
    INPUT_SENSOR = 0x8000,

    // control periphery
    CONTROL_TOUCHSCREEN = 0x8100,

    // output devices
    SCREEN_EPAPER = 0x9000,
};

struct device {
    device_type_t type;                  /* OD index of a first device of this type */
    uint8_t seq;                         /* Sequence number of the device in its family  */
    int16_t index;                       /* Actual OD address of this device */
    device_phase_t phase;                /* Current lifecycle phase of the device */
    void *object;                        /* Pointer to the device own struct */
    size_t struct_size;                  /* Memory requirements for device struct */
    OD_entry_t *properties;              /* OD entry containing propertiesuration for device*/
    OD_extension_t properties_extension; /* OD IO handlers for properties changes */
    device_methods_t *methods;           /* Per-class methods and methods */
    device_ticks_t *ticks;               /* Per-device thread subscription */
    app_t *app;                          /* Reference to root device */
    uint32_t event_subscriptions;        /* Mask for different event types that device recieves */
};

struct device_methods {
    app_signal_t (*validate)(void *properties);       /* Check if properties has all properties */
    app_signal_t (*construct)(void *object); /* Initialize device at given pointer*/
    app_signal_t (*link)(void *object);      /* Link related devices together*/
    app_signal_t (*destruct)(void *object);  /* Destruct device at given pointer*/
    app_signal_t (*start)(void *object);     /* Prepare periphery and run initial logic */
    app_signal_t (*stop)(void *object);      /* Reset periphery and deinitialize */
    app_signal_t (*pause)(void *object);      /* Put device to sleep temporarily */
    app_signal_t (*resume)(void *object);     /* Wake device up from sleep */

    app_signal_t (*callback_task)(void *object, app_task_t *task);                                    /* Task has been complete */
    app_signal_t (*callback_event)(void *object, app_event_t *event);                                 /* Somebody processed the event */
    app_signal_t (*callback_phase)(void *object, device_phase_t phase);                               /* Handle phase change */
    app_signal_t (*callback_signal)(void *object, device_t *origin, app_signal_t signal, void *argument); /* Send signal to device */
    app_signal_t (*callback_value)(void *object, device_t *device, void *value, void *argument);      /* Accept value from linked device */
    app_signal_t (*callback_link)(void *object, device_t *origin, void *argument);                    /* Accept linking request*/

    app_signal_t (*tick_input)(void *p, app_event_t *e, device_tick_t *tick, app_thread_t *t);         /* Processing input events asap */
    app_signal_t (*tick_high_priority)(void *o, app_event_t *e, device_tick_t *tick, app_thread_t *t); /* Important work that isnt input */
    app_signal_t (*tick_medium_priority)(void *p, app_event_t *e, device_tick_t *tick, app_thread_t *t); /* Medmium importance periphery */
    app_signal_t (*tick_low_priority)(void *p, app_event_t *e, device_tick_t *tick, app_thread_t *t);    /* Low-importance periodical */
    app_signal_t (*tick_bg_priority)(void *p, app_event_t *e, device_tick_t *tick, app_thread_t *t);     /* Lowest priority work that i*/

    ODR_t (*property_read)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*property_write)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
};

int device_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick);

/* Find device with given index and write it to the given pointer */
int device_link(device_t *device, void **destination, uint16_t index, void *arg);

/* Send value from device to another */
int device_send(device_t *device, device_t *target, void *value, void *argument);

/* Send signal from device to another*/
int device_signal(device_t *device, device_t *origin, app_signal_t value, void *argument);

int device_allocate(device_t *device);
int device_free(device_t *device);

void device_set_phase(device_t *device, device_phase_t phase);

void device_gpio_set(uint8_t port, uint8_t pin);
void device_gpio_clear(uint8_t port, uint8_t pin);
uint32_t device_gpio_get(uint8_t port, uint8_t pin);

/* Check if event will invoke input tick on this device */
bool_t device_event_is_subscribed(device_t *device, app_event_t *event);
void device_event_subscribe(device_t *device, app_event_type_t type);

/* Attempt to store event in a memory destination if it's not occupied yet */
app_signal_t device_event_accept_and_process_generic(device_t *device, app_event_t *event, app_event_t *destination,
                                                     app_event_status_t ready_status, app_event_status_t busy_status,
                                                     device_event_handler_t handler);

app_signal_t device_event_accept_and_start_task_generic(device_t *device, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                        device_task_t handler, app_event_status_t ready_status,
                                                        app_event_status_t busy_status);

app_signal_t device_event_accept_and_pass_to_task_generic(device_t *device, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                          device_task_t handler, app_event_status_t ready_status,
                                                          app_event_status_t busy_status);

/* Consume event if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_handle(device, event, destination)                                                                                    \
    device_event_accept_and_process_generic(device, event, destination, APP_EVENT_HANDLED, APP_EVENT_DEFERRED, NULL)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_handle_and_process(device, event, destination, handler)                                                               \
    device_event_accept_and_process_generic(device, event, destination, APP_EVENT_HANDLED, APP_EVENT_DEFERRED, (device_event_handler_t)handler)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_handle_and_start_task(device, event, task, thread, handler)                                                           \
    device_event_accept_and_start_task_generic(device, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_handle_and_pass_to_task(device, event, task, thread, handler)                                                         \
    device_event_accept_and_pass_to_task_generic(device, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

/* Consume event if not busy, otherwise allow devices to process it if */
#define device_event_accept(device, event, destination)                                                                                    \
    device_event_accept_and_process_generic(device, event, destination, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED, NULL)
/* Consume event with a given handler if not busy, otherwise allow devices to process it if */
#define device_event_accept_and_process(device, event, destination, handler)                                                               \
    device_event_accept_and_process_generic(device, event, destination, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED,                            \
                                            (app_event_t)handler)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_accept_and_start_task(device, event, task, thread, handler)                                                           \
    device_event_accept_and_start_task_generic(device, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_accept_and_pass_to_task(device, event, task, thread, handler)                                                         \
    device_event_accept_and_pass_to_task_generic(device, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

/* Process event and let others receieve it too */
#define device_event_receive(device, event, destination)                                                                                   \
    device_event_accept_and_process_generic(device, event, destination, APP_EVENT_RECEIEVED, APP_EVENT_RECEIEVED, NULL)
/* Process event with a given handler and let others receieve it too */
#define device_event_receive_and_process(device, event, destination, handler)                                                              \
    device_event_accept_and_process_generic(device, event, destination, APP_EVENT_RECEIEVED, APP_EVENT_RECEIEVED,                          \
                                            (app_event_t)handler)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_receive_and_start_task(device, event, task, thread, handler)                                                          \
    device_event_accept_and_start_task_generic(device, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define device_event_receive_and_pass_to_task(device, event, task, thread, handler)                                                        \
    device_event_accept_and_pass_to_task_generic(device, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

app_signal_t device_event_report(device_t *device, app_event_t *event);
app_signal_t device_event_finalize(device_t *device, app_event_t *event);

app_signal_t device_tick_catchup(device_t *device, device_tick_t *tick);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif