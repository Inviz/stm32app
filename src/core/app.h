#ifndef INC_CORE_APP
#define INC_CORE_APP
/* Generic types for all apps.

Any app type can be cast to this type and get access to generic config values and objects */

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "CANopen.h"
#include "core/types.h"
#include "core/device.h"
#include "core/thread.h"

#define malloc(size) pvPortMalloc(size)
#define free(pointer) vPortFree(pointer)

typedef struct {
    int16_t disabled;
} app_config_t;

typedef struct {
    int16_t disabled;
} app_values_t;


struct app {
    device_t *device;
    size_t device_count;
    OD_t *dictionary;
    app_config_t *config;
    app_values_t *values;
    app_threads_t *threads;
    system_mcu_t *mcu;
    system_canopen_t *canopen;
};

enum app_signal {
    APP_SIGNAL_OK,
    APP_SIGNAL_TIMEOUT,
    APP_SIGNAL_TIMER,

    APP_SIGNAL_DMA_ERROR,
    APP_SIGNAL_DMA_TRANSFERRING,
    APP_SIGNAL_DMA_IDLE,

    APP_SIGNAL_RX_COMPLETE,
    APP_SIGNAL_TX_COMPLETE,

    APP_SIGNAL_CATCHUP,
    APP_SIGNAL_RESCHEDULE,
    APP_SIGNAL_INCOMING,
    APP_SIGNAL_BUSY
};


// Initialize array of all devices found in OD that can be initialized
int app_allocate(app_t **app, OD_t *od, size_t (*enumerator)(app_t *app, OD_t *od, device_t *devices));
// Destruct all devices and release memory
int app_free(app_t **app);
// Transition all devices to given state
void app_set_phase(app_t *app, device_phase_t phase);

size_t app_device_type_enumerate(app_t *app, OD_t *od, device_type_t type, device_methods_t *callbacks, size_t struct_size,
                                  device_t *destination, size_t offset);

/* Find device by index in the global list of registered devices */
device_t *app_device_find(app_t *app, uint16_t index);
/* Find device by type in the global list of registered devices */
device_t *app_device_find_by_type(app_t *app, uint16_t type);
/* Return device from a global array by its index */
device_t *app_device_find_by_number(app_t *app, uint8_t number);
/* Get numeric index of a device in a global array */
uint8_t app_device_find_number(app_t *app, device_t *device);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif