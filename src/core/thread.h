#ifndef INC_CORE_THREAD
#define INC_CORE_THREAD

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "core/device.h"
#include "core/types.h"

#define TIME_DIFF(now, last) (last > now ? ((uint32_t)-1) - last + now : now - last)

struct app_thread {
    device_t *device;
    uint32_t last_time;
    uint32_t current_time;
    uint32_t next_time;
    size_t index; /* Corresponding handler in device_ticks struct */
    TaskHandle_t *task;
    SemaphoreHandle_t semaphore;
    void *argument;
};

struct app_threads {
    app_thread_t *input;
    app_thread_t *async;
    app_thread_t *output;
    app_thread_t *poll;
    app_thread_t *idle;
};

struct device_tick {
    uint32_t last_time; /* Previous time the tick fired */
    uint32_t next_time; /* Next time tick will fire */
    uint32_t interval;  /* Optional period of ticks firing*/
    void *argument;     /* Startup-time argument */
    int (*callback)(void *object, void *argument, device_tick_t *tick, app_thread_t *thread);
};

struct device_ticks {
    device_tick_t *input;  /* Logic that needs immediate response, like processing input */
    device_tick_t *output; /* Asynchronous work that needs to be done later */
    device_tick_t *async;  /* Asynchronous work that needs to be done later */
    device_tick_t *poll;   /* Logic that runs periodically even when there is no input*/
    device_tick_t *idle;   /* Lowest priority work that is done when there isn't anything more important */
};

int device_tick_allocate(device_tick_t **destination,
                         int (*callback)(void *object, void *argument, device_tick_t *tick, app_thread_t *thread));
void device_tick_free(device_tick_t **tick);

int device_ticks_allocate(device_t *device);
int device_ticks_free(device_t *device);

void device_tick_schedule(device_tick_t *tick, uint32_t next_time);
void device_tick_delay(device_tick_t *tick, app_thread_t *thread, uint32_t timeout);

int app_thread_allocate(app_thread_t **destination, void *app_or_object, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t priority, void *argument);

int app_thread_free(app_thread_t **thread);
int app_threads_allocate(app_t *app);
int app_threads_free(app_t *app);

/*
Callback methods that is invoked as FreeRTOS tasks.
Typical task flow: Allow devices to schedule next tick, or be woken up on semaphore
*/
void app_thread_execute(void *thread_ptr);

/* Wake up thread to run asap without waiting for next tick*/
void app_thread_wake(app_thread_t *thread);

/* Wake up thread from ISR */
void app_thread_wake_from_isr(app_thread_t *thread);

#ifdef __cplusplus
}
#endif

#endif