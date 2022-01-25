#ifndef INC_CORE_ACTOR
#define INC_CORE_ACTOR

#ifdef __cplusplus
extern "C" {
#endif

#include "301/CO_ODinterface.h"
#include "core/app.h"
#include "core/thread.h"
#include "lib/gpio.h"


/*#define OD_ACCESSORS(OD_TYPE, NAME, SUBTYPE, PROPERTY, SUBINDEX, TYPE, SHORT_TYPE) \
    static inline ODR_t OD_TYPE##_##NAME##_set_##PROPERTY(OD_TYPE##_##NAME##_t *NAME, TYPE value) {                                        \
        return OD_set_##SHORT_TYPE(NAME->actor->SUBTYPE, SUBINDEX, value, false);                                                         \
    }                                                                                                                                      \
    static inline TYPE OD_TYPE##_##NAME##_get_##PROPERTY(OD_TYPE##_##NAME##_t *NAME) {                                                     \
        TYPE value;                                                                                                                        \
        OD_get_##SHORT_TYPE(NAME->actor->SUBTYPE, SUBINDEX, &value, false);                                                               \
        return value;                                                                                                                      \
    }*/

enum actor_phase {
    ACTOR_ENABLED,
    ACTOR_CONSTRUCTING,
    ACTOR_LINKING,

    ACTOR_STARTING,
    ACTOR_CALIBRATING,
    ACTOR_PREPARING,
    ACTOR_RUNNING,

    ACTOR_REQUESTING,
    ACTOR_RESPONDING,

    ACTOR_WORKING,
    ACTOR_IDLE,

    ACTOR_BUSY,
    ACTOR_RESETTING,

    ACTOR_PAUSING,
    ACTOR_PAUSED,
    ACTOR_RESUMING,

    ACTOR_STOPPING,
    ACTOR_STOPPED,

    ACTOR_DISABLED,
    ACTOR_DESTRUCTING
};

enum actor_type {
    APP = 0x3000,

    // custom actors
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

    // input actors
    INPUT_SENSOR = 0x8000,

    // control periphery
    CONTROL_TOUCHSCREEN = 0x8100,

    // output actors
    SCREEN_EPAPER = 0x9000,
    INDICATOR_LED = 0x9800,
};

struct actor {
    void *object;                   /* Pointer to the actor own struct */
    uint8_t seq;                    /* Sequence number of the actor in its family  */
    int16_t index;                  /* Actual OD address of this actor */
    actor_phase_t phase;           /* Current lifecycle phase of the actor */
    actor_class_t *class;          /* Per-class methods and callbacks */
    actor_ticks_t *ticks;          /* Per-actor thread subscription */
    app_t *app;                     /* Reference to root actor */
    OD_entry_t *entry;              /* OD entry containing propertiesuration for actor*/
    OD_extension_t entry_extension; /* OD IO handlers for properties changes */
    uint32_t event_subscriptions;   /* Mask for different event types that actor recieves */
    #if DEBUG
        actor_phase_t previous_phase;
    #endif
};

struct actor_class {
    actor_type_t type;     /* OD index of a first actor of this type */
    uint8_t phase_subindex; /* OD subindex containing phase property*/
    uint8_t phase_offset;   /* OD subindex containing phase property*/
    size_t size;            /* Memory requirements for actor struct */

    app_signal_t (*validate)(void *properties); /* Check if properties has all properties */
    app_signal_t (*construct)(void *object);    /* Initialize actor at given pointer*/
    app_signal_t (*link)(void *object);         /* Link related actors together*/
    app_signal_t (*destruct)(void *object);     /* Destruct actor at given pointer*/
    app_signal_t (*start)(void *object);        /* Prepare periphery and run initial logic */
    app_signal_t (*stop)(void *object);         /* Reset periphery and deinitialize */
    app_signal_t (*pause)(void *object);        /* Put actor to sleep temporarily */
    app_signal_t (*resume)(void *object);       /* Wake actor up from sleep */

    app_signal_t (*on_task)(void *object, app_task_t *task);                                   /* Task has been complete */
    app_signal_t (*on_event)(void *object, app_event_t *event);                                /* Somebody processed the event */
    app_signal_t (*on_phase)(void *object, actor_phase_t phase);                              /* Handle phase change */
    app_signal_t (*on_signal)(void *object, actor_t *origin, app_signal_t signal, void *arg); /* Send signal to actor */
    app_signal_t (*on_value)(void *object, actor_t *actor, void *value, void *arg);          /* Accept value from linked actor */
    app_signal_t (*on_link)(void *object, actor_t *origin, void *arg);                        /* Accept linking request*/

    app_signal_t (*tick_input)(void *p, app_event_t *e, actor_tick_t *tick, app_thread_t *t);         /* Processing input events asap */
    app_signal_t (*tick_high_priority)(void *o, app_event_t *e, actor_tick_t *tick, app_thread_t *t); /* Important work that isnt input */
    app_signal_t (*tick_medium_priority)(void *p, app_event_t *e, actor_tick_t *tick, app_thread_t *t); /* Medmium importance periphery */
    app_signal_t (*tick_low_priority)(void *p, app_event_t *e, actor_tick_t *tick, app_thread_t *t);    /* Low-importance periodical */
    app_signal_t (*tick_bg_priority)(void *p, app_event_t *e, actor_tick_t *tick, app_thread_t *t);     /* Lowest priority work that i*/

    ODR_t (*property_read)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*property_write)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
    uint32_t property_read_handlers;  // bit masks of properties that have custom reader logic
    uint32_t property_write_handlers; // bit mask of properties that have custom writer logic
};

// Faster version of OD_set_value that has assumptions:
// - Value is not streamed
// - Address is record
// - There're no gaps in record definition
// - If value hasnt changed, there is no need to run callback
ODR_t actor_set_property(actor_t *actor, void *value, size_t size, uint8_t index);
void *actor_get_property_pointer(actor_t *actor, void *value, size_t size, uint8_t index);
ODR_t actor_set_property_numeric(actor_t *actor, uint32_t value, size_t size, uint8_t index);

#define actor_class_property_mask(class, property) (property - class->phase_subindex + 1)

// Compute actor's own index
#define actor_index(actor) (actor->seq + actor->class->type)
// Get struct containing actors OD values
#define actor_get_properties(actor) ((uint8_t *)((app_t *)actor->object)->properties)
// Optimized getter for actor phase
#define actor_get_phase(actor) actor_get_properties(actor)[actor->class->phase_offset]
// Optimized setter for actor phase (will not trigger observers)
#define actor_set_phase(actor, phase) actor_set_property_numeric(actor, (uint32_t) phase, sizeof(actor_phase_t), (actor)->class->phase_subindex) 
// Default state machine for basic phases
void actor_on_phase_change(actor_t *actor, actor_phase_t phase);

int actor_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick);

/* Find actor with given index and write it to the given pointer */
int actor_link(actor_t *actor, void **destination, uint16_t index, void *arg);

/* Send value from actor to another */
int actor_send(actor_t *actor, actor_t *target, void *value, void *argument);

/* Send signal from actor to another*/
int actor_signal(actor_t *actor, actor_t *origin, app_signal_t value, void *argument);

int actor_allocate(actor_t *actor);
int actor_free(actor_t *actor);

void actor_gpio_set(uint8_t port, uint8_t pin);
void actor_gpio_clear(uint8_t port, uint8_t pin);
uint32_t actor_gpio_get(uint8_t port, uint8_t pin);

/* Check if event will invoke input tick on this actor */
bool_t actor_event_is_subscribed(actor_t *actor, app_event_t *event);
void actor_event_subscribe(actor_t *actor, app_event_type_t type);

/* Attempt to store event in a memory destination if it's not occupied yet */
app_signal_t actor_event_accept_and_process_generic(actor_t *actor, app_event_t *event, app_event_t *destination,
                                                     app_event_status_t ready_status, app_event_status_t busy_status,
                                                     actor_on_event_t handler);

app_signal_t actor_event_accept_and_start_task_generic(actor_t *actor, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                        actor_on_task_t handler, app_event_status_t ready_status,
                                                        app_event_status_t busy_status);

app_signal_t actor_event_accept_and_pass_to_task_generic(actor_t *actor, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                          actor_on_task_t handler, app_event_status_t ready_status,
                                                          app_event_status_t busy_status);

/* Consume event if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle(actor, event, destination)                                                                                    \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_DEFERRED, NULL)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_process(actor, event, destination, handler)                                                               \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_DEFERRED, (actor_on_event_t)handler)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_start_task(actor, event, task, thread, handler)                                                           \
    actor_event_accept_and_start_task_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_pass_to_task(actor, event, task, thread, handler)                                                         \
    actor_event_accept_and_pass_to_task_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

/* Consume event if not busy, otherwise allow actors to process it if */
#define actor_event_accept(actor, event, destination)                                                                                    \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED, NULL)
/* Consume event with a given handler if not busy, otherwise allow actors to process it if */
#define actor_event_accept_and_process(actor, event, destination, handler)                                                               \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED, (app_event_t)handler)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_accept_and_start_task(actor, event, task, thread, handler)                                                           \
    actor_event_accept_and_start_task_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_accept_and_pass_to_task(actor, event, task, thread, handler)                                                         \
    actor_event_accept_and_pass_to_task_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

/* Process event and let others receieve it too */
#define actor_event_receive(actor, event, destination)                                                                                   \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_RECEIEVED, APP_EVENT_RECEIEVED, NULL)
/* Process event with a given handler and let others receieve it too */
#define actor_event_receive_and_process(actor, event, destination, handler)                                                              \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_RECEIEVED, APP_EVENT_RECEIEVED, (app_event_t)handler)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_receive_and_start_task(actor, event, task, thread, handler)                                                          \
    actor_event_accept_and_start_task_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_receive_and_pass_to_task(actor, event, task, thread, handler)                                                        \
    actor_event_accept_and_pass_to_task_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

app_signal_t actor_event_report(actor_t *actor, app_event_t *event);
app_signal_t actor_event_finalize(actor_t *actor, app_event_t *event);

app_signal_t actor_tick_catchup(actor_t *actor, actor_tick_t *tick);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif