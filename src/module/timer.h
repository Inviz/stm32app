#ifndef INC_MODULE_TIMER
#define INC_MODULE_TIMER

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_UNITS 10

#include "core/device.h"
#include "core/types.h"

/* Start of autogenerated OD types */
/* 0x6100: Module Timer 1
   TIM1 timer */
typedef struct module_timer_properties {
    uint8_t parameter_count;
    uint8_t prescaler;
    uint8_t initial_subscriptions_count; // How many subscriptions can timer hold at once 
    uint32_t period; // Counter overflow value (usually max value, 16 or 32bit depending on counter) 
    uint32_t frequency; // How many times a second should timer fire 
    uint8_t phase;
} module_timer_properties_t;
/* End of autogenerated OD types */

typedef struct module_timer_subscription {
    device_t *device;
    uint32_t time;
    void *argument;
} module_timer_subscription_t;


struct module_timer {
    device_t *device;
    module_timer_properties_t *properties;

    uint32_t current_time; // timestamp that gets updated on every interrupt/timer subscription
    uint32_t next_time;    // next subscription time (-1 if there arent any)
    uint32_t next_tick;   // scheduled interval (usually equals to timer period)

    uint32_t address;
    uint16_t reset;
    uint16_t clock;
    uint8_t irq;
    uint8_t peripheral_clock;
    uint8_t source;

    #ifdef DEBUG
        uint32_t debug_stopper;
    #endif
    
    module_timer_subscription_t *subscriptions;
};


extern device_class_t module_timer_class;

// set/update alarm timeout in microseconds from now (per device/argument pair), start timer if needed
app_signal_t module_timer_timeout(module_timer_t *timer, device_t *device, void *argument, uint32_t timeout);

// clear alarm for given device/argument pair, deschedule ticks and stop timer if needed
app_signal_t module_timer_clear(module_timer_t *timer, device_t *device, void *argument);

/* Start of autogenerated OD accessors */
typedef enum module_timer_properties_properties {
  MODULE_TIMER_PRESCALER = 0x1,
  MODULE_TIMER_INITIAL_SUBSCRIPTIONS_COUNT = 0x2,
  MODULE_TIMER_PERIOD = 0x3,
  MODULE_TIMER_FREQUENCY = 0x4,
  MODULE_TIMER_PHASE = 0x5
} module_timer_properties_properties_t;

#define module_timer_set_phase(timer, value) OD_set_u8(timer->device->properties, MODULE_TIMER_PHASE, value, false)
#define module_timer_get_phase(timer) *((uint8_t *) OD_getPtr(timer->device->properties, MODULE_TIMER_PHASE, 0, NULL))
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif