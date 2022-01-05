#include "devices.h"
#include "CO_application.h"
#include "OD.h"
#include "module/adc.h"
#include "transport/spi.h"

device_t *devices = NULL;
size_t device_count = 0;

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

// Count or initialize all devices in OD of given type
size_t devices_enumerate_type(device_type_t type, device_callbacks_t *callbacks, size_t struct_size, device_t *destination, size_t offset) {
    size_t count = 0;
    for (size_t seq = 0; seq < 128; seq++) {
        OD_entry_t *config = OD_find(OD, type + seq);
        if (config == NULL)
            break;

        // Skip devices disabled by OD (first index)
        uint16_t disabled = 0;
        OD_get_u16(config, 0x01, &disabled, true);
        if (disabled != 0)
            break;
        OD_entry_t *values = OD_find(OD, type + DEVICES_VALUES_OFFSET + seq);

        if (callbacks->validate(config) != 0 && destination == NULL) {
            CO_errorReport(CO->em, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL, OD_getIndex(config));
            continue;
        }

        count++;
        if (destination == NULL) {
            continue;
        }

        device_t *device = &destination[offset + count - 1];
        device->type = type;
        device->seq = seq;
        device->index = type + seq;
        device->struct_size = struct_size;
        device->config = config;
        device->callbacks = callbacks;
        device->values = values;

        device->config_extension.write = callbacks->write_config == NULL ? OD_writeOriginal : callbacks->write_config;
        device->config_extension.read = callbacks->read_config == NULL ? OD_readOriginal : callbacks->read_config;

        device->values_extension.write = callbacks->write_values == NULL ? OD_writeOriginal : callbacks->write_values;
        device->values_extension.read = callbacks->read_values == NULL ? OD_readOriginal : callbacks->read_values;

        OD_extension_init(config, &device->config_extension);
        OD_extension_init(values, &device->values_extension);
    }
    return count;
}

device_t *find_device(uint16_t index) {
    for (size_t i = 0; i < device_count; i++) {
        if (devices[i].index == index) {
            return &devices[i];
        }
    }
    return NULL;
}

device_t *find_device_by_type(uint16_t type) {
    for (size_t i = 0; i < device_count; i++) {
        if (devices[i].type == type) {
            return &devices[i];
        }
    }
}

device_t *get_device_by_number(uint8_t number) { return &devices[number]; }

uint8_t get_device_number(device_t *device) {
    for (size_t i = 0; i < device_count; i++) {
        if (&devices[i] == device) {
            return i;
        }
    }
    return 255;
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
    device_t *target = find_device(index);
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
        CO_errorReport(CO->em, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL, index);
        return 1;
    }
}

static int device_allocate(device_t *device) {
    device->object = pvPortMalloc(device->struct_size);
    return device->object == NULL ? CO_ERROR_OUT_OF_MEMORY : 0;
}

static int device_free(device_t *device) {
    vPortFree(device->object);
    return 0;
}

void devices_set_phase(device_phase_t phase) {
    log_printf("Devices - phase %s\n", string_from_phase(phase));
    for (size_t i = 0; i < device_count; i++) {
        if (devices[i].phase != DEVICE_DISABLED) {
            device_set_phase(&devices[i], phase);
        }
    }
}

int devices_allocate(void) {
    // count devices first to allocate specific size of an array
    device_count = devices_enumerate(NULL);
    devices = pvPortMalloc(sizeof(device_t) * device_count);

    if (devices == NULL) {
        return CO_ERROR_OUT_OF_MEMORY;
    }
    // actually allocate devices
    devices_enumerate(devices);

    // allocate nested structs
    for (size_t i = 0; i < device_count; i++) {
        int ret = device_allocate(&devices[i]);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

int devices_free(void) {
    for (size_t i = 0; i < device_count; i++) {
        int ret = device_free(&devices[i]);
        if (ret != 0)
            return ret;
    }
    device_count = 0;
    vPortFree(devices);
    return 0;
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

void devices_read(uint32_t timer1usDiff) { (void)timer1usDiff; /* unused */ }
void devices_write(uint32_t timer1usDiff) { (void)timer1usDiff; /* unused */ }

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