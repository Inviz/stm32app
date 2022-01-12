#include "device.h"
#include "system/canopen.h"

char *string_from_phase(device_phase_t phase) {
    static char *phases[] = {
        "ENABLED",    "CONSTRUCTING", "LINKING",

        "STARTING",   "CALIBRATING",  "PREPARING", "RUNNING",

        "REQUESTING", "RESPONDING",

        "BUSY",       "RESETTING",

        "PAUSING",    "PAUSED",       "RESUMING",

        "STOPPING",   "STOPPED",

        "DISABLED",   "DESTRUCTING",
    };
    return phases[phase];
}

int device_send(device_t *device, device_t *origin, void *value, void *argument) {
    if (device->methods->callback_value == NULL) {
        return 1;
    }
    return device->methods->callback_value(device->object, origin, value, argument);
}

int device_signal(device_t *device, device_t *origin, uint32_t value, void *argument) {
    if (device->methods->callback_signal == NULL) {
        return 1;
    }
    return device->methods->callback_signal(device->object, origin, value, argument);
}

int device_phase_linking(device_t *device, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
    device_t *target = app_device_find(device->app, index);
    if (target != NULL) {
        *destination = target->object;
        if (target->methods->callback_link != NULL) {
            target->methods->callback_link(target->object, device, argument);
        }

        return 0;
    } else {
        *destination = NULL;
        log_printf("    ! Device 0x%x could not find device 0x%x\n", device->index, index);
        device_set_phase(device, DEVICE_DISABLED);
        device_error_report(device, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL);
        return 1;
    }
}

int device_allocate(device_t *device) {
    device->object = malloc(device->struct_size);
    // by convention each object struct has pointer to device as its first member
    memcpy(device->object, &device, sizeof(device_t *));
    if (device->object == NULL) {
        return CO_ERROR_OUT_OF_MEMORY;
    }
    return device_ticks_allocate(device);
}

int device_free(device_t *device) {
    free(device->object);
    return device_ticks_free(device);
}

int device_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick) {
    if (*clock > time_since_last_tick) {
        *clock -= time_since_last_tick;
        if (*next_tick > *clock) {
            *next_tick = *clock;
        }
        return 1;
    } else {
        *clock = 0;
        return 0;
    }
}

void device_gpio_configure_input(char *name, uint8_t port, uint16_t pin) {
    /* may be unused*/ (void)name;
    log_printf("    > %s GPIO Analog input: %i %i\n", name, port, pin);
    gpio_enable_port(port);
    gpio_configure_input(port, pin);
}

void device_gpio_configure_output(char *name, uint8_t port, uint16_t pin) {
    /* may be unused*/ (void)name;
    log_printf("    > %s GPIO Push Pull: %i %i\n", name, port, pin);
    gpio_enable_port(port);
    gpio_configure_output(port, pin);
}

void device_gpio_configure_output_with_value(char *name, uint8_t port, uint16_t pin, uint8_t value) {
    device_gpio_configure_output(name, port, pin);
    gpio_set_state(port, pin, value);
}

void device_set_phase(device_t *device, device_phase_t phase) { device_set_temporary_phase(device, phase, 0); }

app_signal_t device_event_accept_and_process_generic(device_t *device, app_event_t *event, app_event_t *destination,
                                                     app_event_status_t ready_status, app_event_status_t busy_status,
                                                     app_event_handler_t handler) {
    // Check if destination can store the incoming event, otherwise device will be considered busy
    if (destination != NULL && destination->type != APP_EVENT_IDLE) {
        event->consumer = device;
        event->status = ready_status;
        memcpy(destination, event, sizeof(app_event_t));
        if (handler != NULL) {
            return handler(device->object, destination);
        }
        return APP_SIGNAL_OK;
    } else {
        event->status = busy_status;
        return APP_SIGNAL_BUSY;
    }
}

app_signal_t device_event_accept_and_start_task_generic(device_t *device, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                  app_task_handler_t handler, app_event_status_t ready_status,
                                                  app_event_status_t busy_status) {
    app_signal_t signal = device_event_accept_and_process_generic(device, event, &task->inciting_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        task->device = device;
        task->handler = handler;
        memcpy(&task->inciting_event, event, sizeof(app_event_t));
        memset(&task->awaited_event, 0, sizeof(app_event_t));
        task->step_index = task->phase_index = 0;
        task->thread = thread;
        task->tick = NULL;
        task->counter = 0;
        app_thread_device_schedule(thread, device, thread->current_time);
    }
    return signal;
}

app_signal_t device_event_accept_and_pass_to_task_generic(device_t *device, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                  app_task_handler_t handler, app_event_status_t ready_status,
                                                  app_event_status_t busy_status) {
    if (task->handler != handler) {
        event->status = busy_status;
        return APP_SIGNAL_BUSY;
    }
    app_signal_t signal = device_event_accept_and_process_generic(device, event, &task->awaited_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        app_thread_device_schedule(thread, device, thread->current_time);
    }
    return signal;
}

/* Attempt to store event in a memory destination if it's not occupied yet */
void device_set_temporary_phase(device_t *device, device_phase_t phase, uint32_t delay) {
    if (device->phase != phase || delay != 0) {
        log_printf(delay != 0 ? "  - Device phase: 0x%x %s <= %s (over %umS)\n" : "  - Device phase: 0x%x %s <= %s\n", device->index,
                   string_from_phase(phase), string_from_phase(device->phase), delay / 1000);
    }
    device->phase = phase;
    device->phase_delay = delay;

    if (delay != 0)
        return;

    switch (phase) {
    case DEVICE_CONSTRUCTING:
        if (device->methods->phase_constructing != NULL) {
            if (device->methods->phase_constructing(device->object, device) != 0) {
                return device_set_phase(device, DEVICE_DISABLED);
            }
        }
        break;
    case DEVICE_DESTRUCTING:
        if (device->methods->phase_destructing != NULL) {
            device->methods->phase_destructing(device->object);
        }
        break;
    case DEVICE_LINKING:
        if (device->methods->phase_linking != NULL) {
            device->methods->phase_linking(device->object);
        }
        break;
    case DEVICE_STARTING:
        if (device->methods->phase_starting != NULL) {
            device->methods->phase_starting(device->object);
            if (device->phase == DEVICE_STARTING) {
                device_set_phase(device, DEVICE_RUNNING);
            }
        }
        break;
    case DEVICE_STOPPING:
        if (device->methods->phase_stoping != NULL) {
            device->methods->phase_stoping(device->object);
            if (device->phase == DEVICE_STOPPING) {
                device_set_phase(device, DEVICE_STOPPED);
            }
        }
        break;
    default:
        break;
    }

    if (device->methods->callback_phase != NULL) {
        device->methods->callback_phase(device->object, phase);
    }
}

bool_t device_event_is_subscribed(device_t *device, app_event_t *event) { return device->event_subscriptions & event->type; }

void device_event_subscribe(device_t *device, app_event_type_t type) { device->event_subscriptions |= type; }

app_signal_t device_event_report(device_t *device, app_event_t *event) {
    if (event->producer && event->producer->methods->callback_event) {
        return event->producer->methods->callback_event(event->producer->object, event);
    } else {
        return APP_SIGNAL_OK;
    }
}

app_signal_t device_event_finalize(device_t *device, app_event_t *event) {
    if (event != NULL && event->type != APP_EVENT_IDLE) {
        device_event_report(device, event);
        memset(event, 0, sizeof(app_event_t));
    }
    return APP_SIGNAL_OK;
}

app_signal_t device_tick_catchup(device_t *device, device_tick_t *tick) {
    app_thread_t *thread = tick->catchup;
    if (thread) {
        tick->catchup = NULL;
        return app_thread_catchup(thread);
    }
    return APP_SIGNAL_OK;
}
inline void device_gpio_set(uint8_t port, uint8_t pin) { return gpio_set(GPIOX(port), pin); }
inline void device_gpio_clear(uint8_t port, uint8_t pin) { return gpio_clear(GPIOX(port), pin); }
inline uint32_t device_gpio_get(uint8_t port, uint8_t pin) { return gpio_get(GPIOX(port), pin); }