#include "device.h"

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
    if (device->callbacks->receive == NULL) {
        return 1;
    }
    return device->callbacks->receive(device->object, origin, value, argument);
}

int device_signal(device_t *device, device_t *origin, uint32_t value, void *argument) {
    if (device->callbacks->signal == NULL) {
        return 1;
    }
    return device->callbacks->signal(device->object, origin, value, argument);
}

int device_link(device_t *device, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
    device_t *target = app_device_find(device->app, index);
    if (target != NULL) {
        *destination = target->object;
        if (target->callbacks->accept != NULL) {
            target->callbacks->accept(target->object, device, argument);
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
        if (device->callbacks->construct != NULL) {
            if (device->callbacks->construct(device->object, device) != 0) {
                return device_set_phase(device, DEVICE_DISABLED);
            }
        }
        break;
    case DEVICE_DESTRUCTING:
        if (device->callbacks->destruct != NULL) {
            device->callbacks->destruct(device->object);
        }
        break;
    case DEVICE_LINKING:
        if (device->callbacks->link != NULL) {
            device->callbacks->link(device->object);
        }
        break;
    case DEVICE_STARTING:
        if (device->callbacks->start != NULL) {
            device->callbacks->start(device->object);
            if (device->phase == DEVICE_STARTING) {
                device_set_phase(device, DEVICE_RUNNING);
            }
        }
        break;
    case DEVICE_STOPPING:
        if (device->callbacks->stop != NULL) {
            device->callbacks->stop(device->object);
            if (device->phase == DEVICE_STOPPING) {
                device_set_phase(device, DEVICE_STOPPED);
            }
        }
        break;
    default:
        break;
    }

    if (device->callbacks->phase != NULL) {
        device->callbacks->phase(device->object, phase);
    }
}

inline void device_gpio_set(uint8_t port, uint8_t pin) { return gpio_set(GPIOX(port), pin); }
inline void device_gpio_clear(uint8_t port, uint8_t pin) { return gpio_clear(GPIOX(port), pin); }
inline uint32_t device_gpio_get(uint8_t port, uint8_t pin) { return gpio_get(GPIOX(port), pin); }