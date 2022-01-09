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
    QueueHandle_t queue;
    void *argument;
};

struct app_threads {
    app_thread_t *input;   /* Thread with queue of events that need immediate response*/
    app_thread_t *catchup; /* Allow devices that were busy to catch up with input events  */
    app_thread_t *async;   /* Logic that is scheduled by devices themselves */
    app_thread_t *output;  /* A queue of events that concerns outputting  outside */
    app_thread_t *poll;    /* Logic that runs periodically that is not very important */
    app_thread_t *idle;    /* A background thread of sorts for work that can be done in free time */
};

struct device_tick {
    uint32_t last_time;    /* Previous time the tick fired */
    uint32_t next_time;    /* Next time tick will fire */
    device_t *next_device; /* Next device having the same tick */
    device_t *prev_device; /* Previous device handling the same tick*/
    int (*callback)(void *object, void *argument, device_tick_t *tick, app_thread_t *thread);
};

struct device_ticks {
    device_tick_t *input;  /* Logic that processes input and immediate work */
    device_tick_t *output; /* Asynchronous work that needs to be done later */
    device_tick_t *async;  /* Asynchronous work that needs to be done later */
    device_tick_t *poll;   /* Logic that runs periodically even when there is no input*/
    device_tick_t *idle;   /* Lowest priority work that is done when there isn't anything more important */
};

int device_tick_allocate(device_tick_t **destination,
                         int (*callback)(void *object, app_event_t *event, device_tick_t *tick, app_thread_t *thread));
void device_tick_free(device_tick_t **tick);

int device_ticks_allocate(device_t *device);
int device_ticks_free(device_t *device);

void device_tick_schedule(device_tick_t *tick, uint32_t next_time);
void device_tick_delay(device_tick_t *tick, app_thread_t *thread, uint32_t timeout);

int app_thread_allocate(app_thread_t **thread, void *app_or_object, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t queue_size, size_t priority, void *argument);

int app_thread_free(app_thread_t **thread);

/* Find out numeric index of a tick that given standard thread handles  */
size_t app_thread_get_tick_index(app_thread_t *thread);

int app_threads_allocate(app_t *app);

int app_threads_free(app_t *app);

/* Find app devices that subscribe to given thread. Joins matching device tick handlers in a linked list, a returns first device as a list
 * head*/
device_t *app_thread_filter_devices(app_thread_t *thread);

/*
Callback methods that is invoked as FreeRTOS tasks.
Typical task flow: Allow devices to schedule next tick, or be woken up on semaphore
*/
void app_thread_execute(app_thread_t *thread);

/* Wake up thread to run asap without waiting for next tick*/
bool_t app_thread_publish_generic(app_thread_t *thread, app_event_t *event, bool_t to_front);
/* Wake up thread from ISR */
bool_t app_thread_publish_generic_from_isr(app_thread_t *thread, app_event_t *event, bool_t to_front);

#define app_thread_publish_to_front_from_isr(thread, event) app_thread_publish_generic_from_isr(thread, event, 1)
#define app_thread_publish_from_isr(thread, event) app_thread_publish_generic_from_isr(thread, event, 0)
#define app_thread_publish_to_front(thread, event) app_thread_publish_generic(thread, event, 1)
#define app_thread_publish(thread, event) app_thread_publish_generic(thread, event, 0)
#define app_publish(app, event) app_thread_publish_generic(app->threads->input, event, 0);
#ifdef __cplusplus
}
#endif

#endif