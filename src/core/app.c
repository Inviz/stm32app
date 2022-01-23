#include "app.h"
#include "system/canopen.h"

// Count or initialize all devices in OD of given type
size_t app_device_type_enumerate(app_t *app, OD_t *od, device_type_t type, device_methods_t *methods, size_t struct_size,
                                 device_t *destination, size_t offset) {
    size_t count = 0;
    for (size_t seq = 0; seq < 128; seq++) {
        OD_entry_t *properties = OD_find(od, type + seq);
        if (properties == NULL && seq >= 10)
            break;
        if (properties == NULL)
            continue;

        // Skip devices disabled by OD (first index)
        uint16_t disabled = 0;
        OD_get_u16(properties, 0x01, &disabled, true);
        if (disabled != 0)
            break;

        if (methods->validate(properties) != 0 && destination == NULL) {
            if (app != NULL && app->canopen != NULL) {
                app_error_report(app, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL, OD_getIndex(properties));
            }
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
        device->properties = properties;
        device->methods = methods;

        device->properties_extension.write = methods->property_write == NULL ? OD_writeOriginal : methods->property_write;
        device->properties_extension.read = methods->property_read == NULL ? OD_readOriginal : methods->property_read;

        OD_extension_init(properties, &device->properties_extension);
    }
    return count;
}

device_t *app_device_find(app_t *app, uint16_t index) {
    for (size_t i = 0; i < app->device_count; i++) {
        if (app->device[i].index == index) {
            return &app->device[i];
        }
    }
    return NULL;
}

device_t *app_device_find_by_type(app_t *app, uint16_t type) {
    for (size_t i = 0; i < app->device_count; i++) {
        if (app->device[i].type == type) {
            return &app->device[i];
        }
    }
    return NULL;
}

device_t *app_device_find_by_number(app_t *app, uint8_t number) {
    return &app->device[number];
}

uint8_t app_device_find_number(app_t *app, device_t *device) {
    for (size_t i = 0; i < app->device_count; i++) {
        if (&app->device[i] == device) {
            return i;
        }
    }
    return 255;
}

void app_set_phase(app_t *app, device_phase_t phase) {
    log_printf("Devices - phase %s\n", get_device_phase_name(phase));
    for (size_t i = 0; i < app->device_count; i++) {
        if (app->device[i].phase != DEVICE_DISABLED) {
            device_set_phase(&app->device[i], phase);
        }
    }
}

int app_allocate(app_t **app, OD_t *od, size_t (*enumerator)(app_t *app, OD_t *od, device_t *devices)) {
    // count devices first to allocate specific size of an array
    size_t device_count = enumerator(NULL, od, NULL);
    device_t *devices = malloc(sizeof(device_t) * device_count);

    if (devices == NULL) {
        return CO_ERROR_OUT_OF_MEMORY;
    }
    // run device constructors
    enumerator(*app, od, devices);

    for (size_t i = 0; i < device_count; i++) {
        // allocate memory for device struct
        int ret = device_allocate(&devices[i]);
        if (ret != 0) {
            return ret;
        }

        // first device must be app device
        // store device count
        if (i == 0) {
            *app = devices->object;
            (*app)->device_count = device_count;
        }

        // link device to the app
        (&devices[i])->app = *app;
    }

    return 0;
}

int app_free(app_t **app) {
    for (size_t i = 0; i < (*app)->device_count; i++) {
        int ret = device_free(&(*app)->device[i]);
        if (ret != 0)
            return ret;
    }
    (*app)->device_count = 0;
    free((*app)->device);
    return 0;
}
