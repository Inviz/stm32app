#ifndef INC_TYPES
#define INC_TYPES

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Global variable is only used for interrupts

typedef struct app app_t;                           /* Generic application object, acts as a global root */
typedef struct app_thread app_thread_t;             /* A FreeRTOS thread that handles multiple devices*/
typedef struct app_threads app_threads_t;           /* List of built in threads in order corresponding to device ticks*/
typedef struct app_event app_event_t;               /* Data sent between devices placed into a bus */
typedef enum app_event_status app_event_status_t;   /* Status of event interaction with device */
typedef enum app_event_type app_event_type_t;       /* Possible types of messages */
typedef struct device device_t;                     /* Generic object container correspondig to Object Dictionary entry */
typedef struct device_tick device_tick_t;           /* A device callback running within specific app thread*/
typedef struct device_ticks device_ticks_t;         /* List of device tick handlers in order corresponding to app threads */
typedef struct device_methods device_methods_t; /* Method list to essentially subclass devices */
typedef enum device_type device_type_t;             /* List of device groups found in Object Dictionary*/
typedef enum device_phase device_phase_t;           /* All phases that device can be in*/
typedef enum app_signal app_signal_t;               /* Things that device tell each other */
typedef struct app_task app_task_t;                 /* State machine dealing with async commands*/
typedef enum app_task_signal app_task_signal_t;     /* Commands to advance step machine*/

typedef struct system_canopen system_canopen_t;
typedef struct system_mcu system_mcu_t;

typedef app_signal_t(*app_method_t)(void *);
typedef app_signal_t (*device_tick_callback_t)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread);
typedef app_signal_t (*app_event_handler_t)(void *, app_event_t *);
typedef app_task_signal_t (*app_task_handler_t)(app_task_t *task);
#ifdef DEBUG
#include <stdio.h>
#define log_printf printf
#define error_printf printf
#else
#define log_printf(...)
#define error_printf(...)
#endif

#ifdef DEBUG
#include <stdio.h>
#define log_printf printf
#define error_printf printf
#else
#define log_printf(...)
#define error_printf(...)
#endif

#endif