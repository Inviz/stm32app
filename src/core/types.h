#ifndef INC_TYPES
#define INC_TYPES

typedef struct app app_t;                           /* Generic application object, acts as a global root */
typedef struct device device_t;                     /* Generic object corresponding to Object Dictionary entry */
typedef struct app_thread app_thread_t;             /* A FreeRTOS thread that handles multiple devices*/
typedef struct app_threads app_threads_t;           /* List of built in threads in order corresponding to device ticks*/
typedef struct device_tick device_tick_t;           /* A device callback running within specific app thread*/
typedef struct device_ticks device_ticks_t;         /* List of device tick handlers in order corresponding to app threads */
typedef enum device_type device_type_t;             /* List of device groups found in Object Dictionary*/
typedef enum device_phase device_phase_t;           /* All phases that device can be in*/
typedef enum device_signal device_signal_t;         /* Things that device tell each other */
typedef struct device_callbacks device_callbacks_t; /* Method list to essentially subclass devices */

#endif