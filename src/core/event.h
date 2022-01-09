#ifndef INC_CORE_EVENT
#define INC_CORE_EVENT

#include "types.h"

#define APP_EVENT_NEEDS_TO_BE_CONSUMED 10
enum app_event_type {
    // Internal events that dont need subscription
    APP_EVENT_THREAD_START = 32, // Set up a schedule, prepare for work
    APP_EVENT_THREAD_STOP,       // Deallocate and destruct working objects
    APP_EVENT_THREAD_SCHEDULE,   // Wake up by software timer
 
    APP_EVENT_INPUT = 0, // Incoming data to be processed
    APP_EVENT_OUTPUT,    // Outgoing data to be processed

    APP_EVENT_MESSAGE_SPI = APP_EVENT_NEEDS_TO_BE_CONSUMED,
    APP_EVENT_MESSAGE_I2C,
    APP_EVENT_MESSAGE_USART,
    APP_EVENT_MESSAGE_MODBUS,
    APP_EVENT_MESSAGE_CANOPEN

};

/* Is event owned by some specific device */
enum app_event_status {
    APP_EVENT_WAITING,   // Event is waiting to be routed
    APP_EVENT_RECEIVED,  // Some devices receieved the event
    APP_EVENT_ADDRESSED, // A device that could handle event was busy, others still can claim it
    APP_EVENT_HANDLED,   // Device processed the event so no others will receive it
    APP_EVENT_DEFERRED   // A busy device wants this event exclusively
};

struct app_event {
    app_event_type_t type;     /* Kind of event*/
    app_event_status_t status; /* Status of events handling*/
    uint8_t *data;             /* Pointer to data package*/
    size_t size;               /* Size of data payload*/
    void *argument;            /* Optional argument */
    device_t *producer;        /* Where event originated at */
    device_t *consumer;        /* Device that handled the event*/
};

bool_t *app_event_needs_to_be_consuemd(app_event_t *event);

#endif