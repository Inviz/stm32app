#include "actor.h"
#include "301/CO_ODinterface.h"
#include "system/canopen.h"

int actor_send(actor_t *actor, actor_t *origin, void *value, void *argument) {
    if (actor->class->on_value == NULL) {
        return 1;
    }
    return actor->class->on_value(actor->object, origin, value, argument);
}

int actor_signal(actor_t *actor, actor_t *origin, app_signal_t signal, void *argument) {
    if (actor->class->on_signal == NULL) {
        return 1;
    }
    return actor->class->on_signal(actor->object, origin, signal, argument);
}

int actor_link(actor_t *actor, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
    actor_t *target = app_actor_find(actor->app, index);
    if (target != NULL) {
        *destination = target->object;
        if (target->class->on_link != NULL) {
            target->class->on_link(target->object, actor, argument);
        }

        return 0;
    } else {
        *destination = NULL;
        log_printf("    ! Device 0x%x (%s) could not find actor 0x%x\n", actor_index(actor), get_actor_type_name(actor->class->type),
                   index);
        actor_set_phase(actor, ACTOR_DISABLED);
        actor_error_report(actor, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL);
        return 1;
    }
}

int actor_allocate(actor_t *actor) {
    actor->object = malloc(actor->class->size);
    if (actor->object == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }
    // cast to app object which is following actor conventions
    app_t *obj = actor->object;
    // by convention each object struct has pointer to actor as its first member
    obj->actor = actor;
    // second member is `properties` poiting to memory struct in OD
    obj->properties = OD_getPtr(actor->entry, 0x00, 0, NULL);

    actor->entry_extension.object = actor->object;

    return actor_ticks_allocate(actor);
}

int actor_free(actor_t *actor) {
    free(actor->object);
    return actor_ticks_free(actor);
}

int actor_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick) {
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

app_signal_t actor_event_accept_and_process_generic(actor_t *actor, app_event_t *event, app_event_t *destination,
                                                     app_event_status_t ready_status, app_event_status_t busy_status,
                                                     actor_on_event_t handler) {
    // Check if destination can store the incoming event, otherwise actor will be considered busy
    if (destination != NULL || destination->type != APP_EVENT_IDLE) {
        event->consumer = actor;
        event->status = ready_status;
        memcpy(destination, event, sizeof(app_event_t));
        if (handler != NULL) {
            return handler(actor->object, destination);
        }
        return APP_SIGNAL_OK;
    } else {
        event->status = busy_status;
        return APP_SIGNAL_BUSY;
    }
}

app_signal_t actor_event_accept_and_start_task_generic(actor_t *actor, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                        actor_on_task_t handler, app_event_status_t ready_status,
                                                        app_event_status_t busy_status) {
    app_signal_t signal = actor_event_accept_and_process_generic(actor, event, &task->inciting_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        task->actor = actor;
        task->handler = handler;
        memcpy(&task->inciting_event, event, sizeof(app_event_t));
        memset(&task->awaited_event, 0, sizeof(app_event_t));
        task->step_index = task->phase_index = 0;
        task->thread = thread;
        task->tick = NULL;
        task->counter = 0;
        log_printf("~ %s: New task for %s via #%s\n", get_actor_type_name(actor->class->type), app_thread_get_name(thread), get_app_event_type_name(event->type));
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

app_signal_t actor_event_accept_and_pass_to_task_generic(actor_t *actor, app_event_t *event, app_task_t *task, app_thread_t *thread,
                                                          actor_on_task_t handler, app_event_status_t ready_status,
                                                          app_event_status_t busy_status) {
    if (task->handler != handler) {
        event->status = busy_status;
        return APP_SIGNAL_BUSY;
    }
    app_signal_t signal = actor_event_accept_and_process_generic(actor, event, &task->awaited_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

void actor_on_phase_change(actor_t *actor, actor_phase_t phase) {
    #if DEBUG
    log_printf("  - Device phase: 0x%x %s %s <= %s\n", actor_index(actor), get_actor_type_name(actor->class->type),
               get_actor_phase_name(phase), get_actor_phase_name(actor->previous_phase));
        actor->previous_phase = phase;
    #endif

    switch (phase) {
    case ACTOR_CONSTRUCTING:
        if (actor->class->construct != NULL) {
            if (actor->class->construct(actor->object)) {
                return actor_set_phase(actor, ACTOR_DISABLED);
            }
        }
        break;
    case ACTOR_DESTRUCTING:
        if (actor->class->destruct != NULL) {
            actor->class->destruct(actor->object);
        }
        break;
    case ACTOR_LINKING:
        if (actor->class->link != NULL) {
            actor->class->link(actor->object);
        }
        break;
    case ACTOR_STARTING:
        actor->class->start(actor->object);
        if (actor_get_phase(actor) == ACTOR_STARTING) {
            return actor_set_phase(actor, ACTOR_RUNNING);
        }
        break;
    case ACTOR_STOPPING:
        actor->class->stop(actor->object);
        if (actor_get_phase(actor) == ACTOR_STOPPING) {
            return actor_set_phase(actor, ACTOR_STOPPED);
        }
        break;
    case ACTOR_PAUSING:
        if (actor->class->pause == NULL) {
            return actor_set_phase(actor, ACTOR_STOPPING);
        } else {
            actor->class->pause(actor->object);
            if (actor_get_phase(actor) == ACTOR_PAUSING) {
                return actor_set_phase(actor, ACTOR_PAUSED);
            }
        }
        break;
    case ACTOR_RESUMING:
        if (actor->class->resume == NULL) {
            return actor_set_phase(actor, ACTOR_STARTING);
        } else {
            actor->class->resume(actor->object);
            if (actor_get_phase(actor) == ACTOR_RESUMING) {
                return actor_set_phase(actor, ACTOR_RUNNING);
            }
        }
        break;
    default: break;
    }

    if (actor->class->on_phase != NULL) {
        actor->class->on_phase(actor->object, phase);
    }
}

bool_t actor_event_is_subscribed(actor_t *actor, app_event_t *event) {
    return actor->event_subscriptions & event->type;
}

void actor_event_subscribe(actor_t *actor, app_event_type_t type) {
    actor->event_subscriptions |= type;
}

app_signal_t actor_event_report(actor_t *actor, app_event_t *event) {
    (void)actor;
    if (event->producer && event->producer->class->on_event) {
        return event->producer->class->on_event(event->producer->object, event);
    } else {
        return APP_SIGNAL_OK;
    }
}

app_signal_t actor_event_finalize(actor_t *actor, app_event_t *event) {
    if (event != NULL && event->type != APP_EVENT_IDLE) {

        log_printf("~ %s: %s finalizing #%s of %s\n", app_get_current_thread_name(actor->app),
                get_actor_type_name(actor->class->type), get_app_event_type_name(event->type), get_actor_type_name(event->producer->class->type));
                
        actor_event_report(actor, event);
        memset(event, 0, sizeof(app_event_t));
    }
    return APP_SIGNAL_OK;
}

app_signal_t actor_tick_catchup(actor_t *actor, actor_tick_t *tick) {
    (void)actor;
    app_thread_t *thread = tick->catchup;
    if (thread) {
        tick->catchup = NULL;
        return app_thread_catchup(thread);
    }
    return APP_SIGNAL_OK;
}
inline void actor_gpio_set(uint8_t port, uint8_t pin) {
    return gpio_set(GPIOX(port), 1 << pin);
}
inline void actor_gpio_clear(uint8_t port, uint8_t pin) {
    return gpio_clear(GPIOX(port), 1 << pin);
}
inline uint32_t actor_gpio_get(uint8_t port, uint8_t pin) {
    return gpio_get(GPIOX(port), 1 << pin);
}

typedef struct {
    void *dataOrig;       /**< Pointer to data */
    uint8_t subIndex;     /**< Sub index of element. */
    OD_attr_t attribute;  /**< Attribute bitfield, see @ref OD_attributes_t */
    OD_size_t dataLength; /**< Data length in bytes */
} OD_obj_record_t;

ODR_t actor_set_property(actor_t *actor, void *value, size_t size, uint8_t index) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];

    // bail out quickly if value hasnt changed
    if (memcmp(odo->dataOrig, value, size) == 0) {
        return 0;
    }

    // special case of phase handler
    if (index == actor->class->phase_subindex) {
        memcpy(odo->dataOrig, value, size);
        actor_on_phase_change(actor, *((actor_phase_t *) value));
        return ODR_OK;
    }

    // quickly copy the value if there is no custom observer
    if (actor->class->property_write == NULL) {
        memcpy(odo->dataOrig, value, size);
        return ODR_OK;
    }
 
    OD_size_t count_written = 0;
    OD_stream_t stream = {
        .dataOrig = odo->dataOrig,
        .dataLength = odo->dataLength,
        .attribute = odo->attribute,
        .object = actor->object,
        .subIndex = index,
        .dataOffset = 0,
    };

    return actor->class->property_write(&stream, value, size, &count_written);
}

ODR_t actor_set_property_numeric(actor_t *actor, uint32_t value, size_t size, uint8_t index) {
    return actor_set_property(actor, &value, size, index);
}

void *actor_get_property_pointer(actor_t *actor, void *value, size_t size, uint8_t index) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    if (actor->class->property_read == NULL) {
        return odo->dataOrig;
    }
    OD_size_t count_read = 0;
    OD_stream_t stream = {
        .dataOrig = odo->dataOrig,
        .dataLength = odo->dataLength,
        .attribute = odo->attribute,
        .object = actor->object,
        .subIndex = index,
        .dataOffset = 0,
    };
    actor->class->property_read(&stream, value, size, &count_read);
    return value;
}