#include "thread.h"
#include "core/device.h"

#define MAX_THREAD_SLEEP_TIME_LONG (((uint32_t)-1) / 2)
#define MAX_THREAD_SLEEP_TIME 5000
#define IS_IN_ISR (SCB_ICSR & SCB_ICSR_VECTACTIVE)

static app_signal_t app_thread_event_dispatch(app_thread_t *thread, app_event_t *event, device_t *first_device, size_t tick_index);
static app_signal_t app_thread_event_device_dispatch(app_thread_t *thread, app_event_t *event, device_t *device, device_tick_t *tick);
static size_t app_thread_event_requeue(app_thread_t *thread, app_event_t *event, app_event_status_t previous_status);
static void app_thread_event_await(app_thread_t *thread, app_event_t *event);
static inline bool_t app_thread_event_queue_shift(app_thread_t *thread, app_event_t *event, size_t deferred_count);
static inline device_tick_t *device_tick_by_index(device_t *device, size_t index);

/* A generic RTOS task function that handles all typical propertiesurations of threads. It supports following features or combinations of
  them:
  - Waking up devices on individual timer alarms to simulate delays and periodical work
  - Aborting or speeding up the timer alarms (with 1ms precision of FreeRTOS)
  - Yielding execution to other devices and getting it at the end of event queue
  - Broadcasting events either to specific device, multiple listeners or first taker
  - Re-queuing events for later processing if a receiving device was busy
  - Maintaining queue of events or lightweight event event mailbox slot.

  An app has at least five of these threads with different priorities. Devices declare "ticks" which act as methods for a corresponding
  thread. This way devices can prioritize parts of their logic to play well in shared environment. For example devices may afford doing
  longer processing of data in background without blocking networking or handling inputs for the whole app. Another benefit of
  allowing device to run logic with different priorities is responsiveness and flexibility. An high-priority input event may tell device to
  abort longer task and do something else instead.

  A downside of multiple devices sharing the same thread, is that there is no automatic time-slicing of work. Whole thread (including
  processing of new events) is blocked until a device finishes its work, unless a thread with higher priority tells the device to stop.
  Devices still need to be mindful of blocking the cpu within a scope of its task priority. There can be a few solutions to this:
  - Devices can split their workload into chunks. Execution control would need to be yielded after each chunk. It can be done
  either by using alarm to schedule next tick in future (with 1ms resolution) or right after the thread goes through its queue of events
  (effectively simulating RTOS yield)
  - Devices can also create their own custom threads with own queues, leveraging all the dispatching and queue-management mechanisms
  available to app-wide threads. In this case RTOS will use time-slicing to periodically break up execution and allow other threads to work.
 */
void app_thread_execute(app_thread_t *thread) {
    thread->current_time = xTaskGetTickCount();
    thread->last_time = thread->current_time;
    thread->next_time = thread->current_time + MAX_THREAD_SLEEP_TIME;
    size_t tick_index = app_thread_get_tick_index(thread);      // Which device tick handles this thread
    size_t deferred_count = 0;                                  // Counter of re-queued events
    device_t *first_device = app_thread_filter_devices(thread); // linked list of devices declaring the tick
    app_event_t event = {.producer = thread->device->app->device, .type = APP_EVENT_THREAD_START};

    if (first_device == NULL) {
        log_printf("~ %s:  does not have any listener devices and will self-terminate\n", app_thread_get_name(thread));
        vTaskDelete(NULL);
        portYIELD();
    }

    while (true) {
        app_event_status_t previous_event_status = event.status;

        // route event to its listeners
        app_thread_event_dispatch(thread, &event, first_device, tick_index);

        // keep event for later processing if needed
        deferred_count += app_thread_event_requeue(thread, &event, previous_event_status);

        // move to the next event
        if (!app_thread_event_queue_shift(thread, &event, deferred_count)) {
            // wait for external notifications or scheduled wakeup
            app_thread_event_await(thread, &event);
        }
    }
}

/*
  Devices may indicate which types of events they want to be notified about. But there're also internal synthetic events that they receieve
  unconditionally.
*/
static inline bool_t app_thread_should_notify_device(app_thread_t *thread, app_event_t *event, device_t *device, device_tick_t *tick) {
    switch (event->type) {
    // Ticks get notified when thread starts and stops, so they can construct/destruct or schedule a periodical alarm
    case APP_EVENT_THREAD_START:
    case APP_EVENT_THREAD_STOP: return true;

    // Ticks that set up software alarm will recieve schedule event in time
    case APP_EVENT_THREAD_ALARM: return tick->next_time <= thread->current_time;

    default:
        if (event->consumer == device) {
            // Events targeted to a specific reciepeint will always be received
            return true;
        } else {
            // Devices have to subscribe to other types of events manually
            return device_event_is_subscribed(device, event);
        }
    }
}

/* Notify interested devices of a new event. Events may either be targeted to a specific device, or broadcasted to all/*/
static app_signal_t app_thread_event_dispatch(app_thread_t *thread, app_event_t *event, device_t *first_device, size_t tick_index) {

    // If event has no indicated reciepent, all devices that are subscribed to thread and event will need to be notified
    device_t *device = event->consumer ? event->consumer : first_device;
    device_tick_t *tick;
    app_signal_t signal;

    while (device) {
        tick = device_tick_by_index(device, tick_index);
        signal = app_thread_event_device_dispatch(thread, event, device, tick);

        // let tick know that it has events to catch up later
        if (signal == APP_SIGNAL_BUSY) {
            tick->catchup = thread;
        }

        // stop broadcasting if device wants this event for itself
        if (event->status >= APP_EVENT_HANDLED || event->consumer != NULL) {
            break;
        }

        device = tick->next_device;
    }

    return signal;
}

/* In response to event, device can request to:
 * - stop broadcasting event, so other devices will not receive it
 * - keep event in the queue, so this device can process it later
 * - keep event in the queue for later processing, but allow other devices to handle it before that
 * - wake up on software timer at specific time in future */

static app_signal_t app_thread_event_device_dispatch(app_thread_t *thread, app_event_t *event, device_t *device, device_tick_t *tick) {
    app_signal_t signal;

    if (app_thread_should_notify_device(thread, event, device, tick)) {
        log_printf("~ %s:  dispatching #%s from %s to %s\n", app_thread_get_name(thread), get_app_event_type_name(event->type),
                   get_device_type_name(event->producer->class->type), get_device_type_name(device->class->type));

        if (event->type == APP_EVENT_THREAD_ALARM) {
            tick->next_time = -1;
        }
        // Tick callback may change event status, set software timer or both
        signal = tick->callback(device->object, event, tick, thread);
        tick->last_time = thread->current_time;

        // Mark event as processed
        if (event->status == APP_EVENT_WAITING) {
            event->status = APP_EVENT_RECEIVED;
        }
    }

    // Device may request thread to wake up at specific time without waiting for external events by settings next_time of its ticks
    // - To wake up periodically device should re-schedule its tick after each run
    // - To yield control until other events are processed device should set schedule to current time of a thread
    /*if (tick->next_time != 0 && thread->next_time > tick->next_time) {
        thread->next_time = tick->next_time;
    }*/

    return signal;
}

/* Some threads may want to redirect their stale messages to another thread */
app_thread_t *app_thread_get_catchup_thread(app_thread_t *thread) {
    if (thread == thread->device->app->threads->input) {
        return thread->device->app->threads->catchup;
    } else {
        return thread;
    }
}

/* Find a queue to insert deferred message to so it doesn't get lost. A thread may put it back to its own queue at the end, put it into
 * another thread queue, or even into a queue not owned by any thread. This can be used to create lightweight threads that only have a
 * notification mailbox slot, but then can still retain events elsewhere.  */
static inline QueueHandle_t app_thread_get_catchup_queue(app_thread_t *thread) {
    return app_thread_get_catchup_thread(thread)->queue;
}

/*
  A broadcasted event may be claimed for exclusive processing by device that is too busy to process it right away. In that case event then
  gets pushed to the back of the queue, so the thread can keep on processing other events. When all is left in queue is deferred events, the
  thread will sleep and wait for either new events or notification from device that is now ready to process a deferred event.
*/
static size_t app_thread_event_requeue(app_thread_t *thread, app_event_t *event, app_event_status_t previous_status) {
    QueueHandle_t queue;

    switch (event->status) {
    case APP_EVENT_WAITING:
        if (event->type != APP_EVENT_THREAD_ALARM)
            log_printf("No devices are listening to event: #%s\n", get_app_event_type_name(event->type));
        break;

    // Some busy device wants to handle event later, so the event has to be re-queued
    case APP_EVENT_DEFERRED:
    case APP_EVENT_ADDRESSED:
        queue = app_thread_get_catchup_queue(thread);
        if (queue != NULL) {
            // If event is reinserted back into the same queue it came from, the thread has
            // to keep the records about that to avoid popping the event right back
            if (queue == thread->queue) {
                event->status = APP_EVENT_DEFERRED;
            } else {
                // Otherwise event status gets reset, so receiving thread has a chance to do its own bookkeeping
                // The thread will not wake up automatically, since that requires sending notification
                event->status = APP_EVENT_WAITING;
            }

            // If the target queue is full, event gets lost.
            if (xQueueSend(queue, &event, 0)) {
                if (event->status == APP_EVENT_DEFERRED && previous_status != APP_EVENT_DEFERRED) {
                    return 1;
                }
            } else {
                log_printf("The queue doesnt have any room for deferred event #%s\n", get_app_event_type_name(event->type));
            }
        } else {
            // Threads without a queue will leave event stored in the only available notification slot
            // New events will overwrite the deferred event that wasnt handled in time
        }
        break;

    // When a device catches up with deferred event, it has to mark it as processed or else the event
    // will keep on being requeued
    case APP_EVENT_HANDLED:
        if (previous_status == APP_EVENT_DEFERRED && thread->queue) {
            return -1;
        }
        break;

    case APP_EVENT_RECEIVED: break;
    }

    return 0;
}

/*
  Thread ingests new event from queue and dispatches it for devices to handle.

  Unlike others, input thread off-loads its deferred events to a queue of a `catchup` thread to keep its own queue clean. This happens
  transparently to devices, as they dont know of that special thread existance.
  */
static inline bool_t app_thread_event_queue_shift(app_thread_t *thread, app_event_t *event, size_t deferred_count) {
    // Threads without queues receieve their events via notification slot
    if (thread->queue == NULL) {
        return false;
    }

    // There is no need to shift queue if all events in the queue are deferred
    if (deferred_count > 0 && deferred_count == uxQueueMessagesWaiting(thread->queue)) {
        return false;
    }

    // Try getting an event out of the queue without blocking. Thread blocks on notification signal instead
    if (!xQueueReceive(thread->queue, event, 0)) {
        // clear out previous event
        // *event = (app_event_t) {};
        return false;
    }
    return true;
}

/*
  A thread goes to sleep to allow tasks with lower priority to run, when it does not have any events that can be processed right now.
  - Publishing new events also sends the notification for the thread to wake up. If an event was published to the back of the queue that had
  deferred events in it, the thread will need to rotate the whole queue to get to to the new events. This is due to FreeRTOS only allowing
  to take first event in the queue.
  - A device that was previously busy can send a `APP_SIGNAL_CATCHUP` notification, indicating that it is ready to catch up with events
    that it deferred previously. In that case thread will attempt to re-dispatch all the events in the queue.
*/
static inline void app_thread_event_await(app_thread_t *thread, app_event_t *event) {

    for (bool_t stop = false; stop == false;) {
        // task gets blocked expecting notification, with a timeout to simulate software alarm.
        int32_t timeout = thread->next_time - thread->current_time;

        // if notifications keep being published, the timeout may not happen in time causing alarm to lag behind.
        // in that case timer has to be adjusted to be fired after all events and notifications are processed.
        if (timeout < 0) {
            timeout = 0;
        }

        // threads are expected to receive external notifications in order to wake up
        if (timeout > 0) {
            log_printf("~ %s:  will sleep for %ims\n", app_thread_get_name(thread), (int)timeout);
        }

        uint32_t notification = ulTaskNotifyTake(true, pdMS_TO_TICKS((uint32_t)timeout));

#ifdef DEBUG
        if (thread->device->app->current_thread != thread) {
            if (thread->device->app->current_thread != NULL)
                log_printf("~ %s <= %s\n", app_thread_get_name(thread), app_get_current_thread_name(thread->device->app));
            thread->device->app->current_thread = thread;
        }
#endif

        switch (notification) {
        case APP_SIGNAL_OK:
            // if no notification was recieved (value of signal being zero) within scheduled time,
            // it means that thread is woken up by schedule so synthetic wake up event is broadcasted
            *event = (app_event_t){.producer = thread->device->app->device, .type = APP_EVENT_THREAD_ALARM};
            log_printf("~ %s:  woke up on alert after %lims\n", app_thread_get_name(thread),
                       ((xTaskGetTickCount() - thread->current_time) * portTICK_PERIOD_MS));
            stop = true;
            break;

        case APP_SIGNAL_RESCHEDULE:
            // something wants the thread to wake up earlier than its current schedule.
            // next_time must be already updated with new time, so the thread can just go into blocked state again
            log_printf("~ %s:  will reschedule\n", app_thread_get_name(thread));
            break;

        case APP_SIGNAL_CATCHUP:
            // something tells thread that it is now ready to process messages that it asked to keep deferred
            log_printf("~ %s:  got signal for catchup\n", app_thread_get_name(thread));
            __attribute__((fallthrough));
        default:
            // event publishers set address of event in notification slot instead of a signal
            // it is only read in threads that dont use queues
            if (thread->queue != NULL) {
                // threads with queue that receieve notification expect a new message in queue
                if (!xQueueReceive(thread->queue, event, 0)) {
                    log_printf("~ %s:  was woken up by notification but its queue is empty\n", app_thread_get_name(thread));
                    continue;
                }
            } else if (notification > 50) {
                // if thread doesnt have queue, event is passed as address in notification slot instead.
                // publisher will have to ensure that event memory survives until thread is ready for it.
                memcpy(event, (uint32_t *)notification, sizeof(app_event_t));
            }
            stop = true;
        }
    }

    // current time will only update after waking up
    thread->last_time = thread->current_time;
    thread->current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    // tell thread to sleep if no work co
    if (thread->next_time <= thread->current_time)
        thread->next_time = thread->current_time + MAX_THREAD_SLEEP_TIME;
}

bool_t app_thread_notify_generic(app_thread_t *thread, uint32_t value, bool_t overwrite) {
    log_printf("~ %s:  notifying #%s with %s\n", app_get_current_thread_name(thread->device->app),
               value < 50 ? get_app_signal_name(value) : (char)value, app_thread_get_name(thread));
    if (IS_IN_ISR) {
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        bool_t result = xTaskNotifyFromISR(thread->task, value, overwrite ? eSetValueWithOverwrite : eSetValueWithoutOverwrite,
                                           &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return result;
    } else {
        return xTaskNotify(thread->task, value, overwrite ? eSetValueWithOverwrite : eSetValueWithoutOverwrite);
    }
}

bool_t app_thread_publish_generic(app_thread_t *thread, app_event_t *event, bool_t to_front) {
    log_printf("~ %s: %s publishing #%s for %s\n", app_get_current_thread_name(thread->device->app),
               get_device_type_name(event->producer->class->type), get_app_event_type_name(event->type), app_thread_get_name(thread));
    if (IS_IN_ISR) {
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        bool_t result = false;
        if (thread->queue == NULL || xQueueGenericSendFromISR(thread->queue, event, &xHigherPriorityTaskWoken, to_front)) {
            result = xTaskNotifyFromISR(thread->task, (uint32_t)event, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        };
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return result;
    } else {
        if (thread->queue == NULL || xQueueGenericSend(thread->queue, event, 0, to_front)) {
            return xTaskNotify(thread->task, (uint32_t)event, eSetValueWithOverwrite);
        } else {
            return false;
        }
    }
}

void app_thread_schedule(app_thread_t *thread, uint32_t time) {
    if (thread->next_time > time)
        thread->next_time = time;
    if (eTaskGetState(thread->task) == eBlocked) {
        app_thread_reschedule(thread);
    }
}
void app_thread_tick_schedule(app_thread_t *thread, device_tick_t *tick, uint32_t time) {
    tick->next_time = time;
    app_thread_schedule(thread, time);
}

int device_tick_allocate(device_tick_t **destination, device_on_tick_t callback) {
    if (callback != NULL) {
        *destination = malloc(sizeof(device_tick_t));
        if (*destination == NULL) {
            return APP_SIGNAL_OUT_OF_MEMORY;
        }
        device_tick_initialize(*destination);
        (*destination)->callback = callback;
    }
    return 0;
}

void device_tick_initialize(device_tick_t *tick) {
    *tick = (device_tick_t){.next_time = -1};
}

void device_tick_free(device_tick_t **tick) {
    if (*tick != NULL) {
        free(*tick);
        *tick = NULL;
    }
}

int app_thread_allocate(app_thread_t **destination, void *app_or_object, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t queue_size, size_t priority, void *argument) {
    *destination = (app_thread_t *)malloc(sizeof(app_thread_t));
    app_thread_t *thread = *destination;
    thread->device = ((app_t *)app_or_object)->device;
    xTaskCreate(callback, name, stack_depth, (void *)thread, priority, (void *)&thread->task);
    if (thread->task == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }
    if (queue_size > 0) {
        thread->queue = xQueueCreate(queue_size, sizeof(app_event_t));
        if (thread->queue == NULL) {
            return APP_SIGNAL_OUT_OF_MEMORY;
        }
#if propertiesQUEUE_REGISTRY_SIZE > 0
        vQueueAddToRegistry(thread->queue, name);
#endif
    }

    thread->argument = argument;
    return 0;
}

int app_thread_free(app_thread_t **thread) {
    free(*thread);
    if ((*thread)->queue != NULL) {
        vQueueDelete((*thread)->queue);
    }
    *thread = NULL;
    return 0;
}

/* Returns specific member of app_threads_t struct by its numeric index*/
static inline device_tick_t *device_tick_by_index(device_t *device, size_t index) {
    return ((device_tick_t **)device->ticks)[index];
}

/* Returns specific member of device_ticks_t struct by its numeric index*/
static inline app_thread_t *app_thread_by_index(app_t *app, size_t index) {
    return ((app_thread_t **)app->threads)[index];
}

/* Returns device tick handling given thread */
device_tick_t *device_tick_for_thread(device_t *device, app_thread_t *thread) {
    return device_tick_by_index(device, app_thread_get_tick_index(thread));
}

device_t *app_thread_filter_devices(app_thread_t *thread) {
    app_t *app = thread->device->app;
    size_t tick_index = app_thread_get_tick_index(thread);
    device_t *first_device = NULL;
    device_t *last_device = NULL;
    for (size_t i = 0; i < app->device_count; i++) {
        device_t *device = &app->device[i];
        device_tick_t *tick = device_tick_by_index(device, tick_index);
        // Subscribed devices have corresponding tick handler
        if (tick == NULL) {
            continue;
        }

        // Return first device
        if (first_device == NULL) {
            first_device = device;
        }

        // double link the ticks for fast iteration
        if (last_device != NULL) {
            device_tick_by_index(last_device, tick_index)->next_device = device;
            tick->prev_device = device;
        }

        last_device = device;
    }

    return first_device;
}

size_t app_thread_get_tick_index(app_thread_t *thread) {
    for (size_t i = 0; i < sizeof(app_threads_t) / sizeof(app_thread_t *); i++) {
        if (app_thread_by_index(thread->device->app, i) == thread) {
            // Both input and immediate threads invoke same input tick
            if (i > 0) {
                return i - 1;
            } else {
                return i;
            }
        }
    }
    return -1;
}

int device_ticks_allocate(device_t *device) {
    device->ticks = malloc(sizeof(device_ticks_t));
    return device_tick_allocate(&device->ticks->input, device->class->tick_input) ||
           device_tick_allocate(&device->ticks->medium_priority, device->class->tick_medium_priority) ||
           device_tick_allocate(&device->ticks->high_priority, device->class->tick_high_priority) ||
           device_tick_allocate(&device->ticks->low_priority, device->class->tick_low_priority) ||
           device_tick_allocate(&device->ticks->bg_priority, device->class->tick_bg_priority);
}

int device_ticks_free(device_t *device) {
    device_tick_free(&device->ticks->input);
    device_tick_free(&device->ticks->medium_priority);
    device_tick_free(&device->ticks->high_priority);
    device_tick_free(&device->ticks->low_priority);
    device_tick_free(&device->ticks->bg_priority);
    free(device->ticks);
    return 0;
}

int app_threads_allocate(app_t *app) {
    app->threads = malloc(sizeof(app_threads_t));
    return app_thread_allocate(&app->threads->input, app, (void (*)(void *ptr))app_thread_execute, "Input", 200, 20, 5, NULL) ||
           app_thread_allocate(&app->threads->catchup, app, (void (*)(void *ptr))app_thread_execute, "Catchup", 200, 100, 5, NULL) ||
           app_thread_allocate(&app->threads->high_priority, app, (void (*)(void *ptr))app_thread_execute, "High P", 200, 1, 4, NULL) ||
           app_thread_allocate(&app->threads->medium_priority, app, (void (*)(void *ptr))app_thread_execute, "Medium P", 200, 1, 3, NULL) ||
           app_thread_allocate(&app->threads->low_priority, app, (void (*)(void *ptr))app_thread_execute, "Low P", 200, 1, 2, NULL) ||
           app_thread_allocate(&app->threads->bg_priority, app, (void (*)(void *ptr))app_thread_execute, "Bg P", 200, 0, 1, NULL);
}

char *app_thread_get_name(app_thread_t *thread) {
    return pcTaskGetTaskName(thread->task);
}

char *app_get_current_thread_name(app_t *app) {
    return IS_IN_ISR ? "ISR" : pcTaskGetTaskName(app->current_thread->task);
}

int app_threads_free(app_t *app) {
    app_thread_free(&app->threads->input);
    app_thread_free(&app->threads->medium_priority);
    app_thread_free(&app->threads->high_priority);
    app_thread_free(&app->threads->low_priority);
    app_thread_free(&app->threads->bg_priority);
    free(app->threads);
    return 0;
}