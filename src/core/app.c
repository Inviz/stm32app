#include "app.h"
#include "system/canopen.h"

// Count or initialize all devices in OD of given type
size_t app_device_type_enumerate(app_t *app, OD_t *od, device_class_t *class,
                                 device_t *destination, size_t offset) {
    size_t count = 0;

    for (size_t seq = 0; seq < 128; seq++) {
        OD_entry_t *properties = OD_find(od, class->type + seq);
        if (properties == NULL && seq >= 19)
            break;
        if (properties == NULL)
            continue;
        
        uint8_t *phase = OD_getPtr(properties, class->phase_subindex, 0, NULL);

        // compute struct offset for phase property
        // class->phase_offset = (void *) phase - OD_getPtr(properties, 0x00, 0, NULL);

        if (*phase != DEVICE_ENABLED || class->validate(OD_getPtr(properties, 0x00, 0, NULL)) != 0) {
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
        device->seq = seq;
        device->properties = properties;
        device->class = class;

        device->properties_extension.write = class->property_write == NULL ? OD_writeOriginal : class->property_write;
        device->properties_extension.read = class->property_read == NULL ? OD_readOriginal : class->property_read;

        OD_extension_init(properties, &device->properties_extension);
    }
    return count;
}

device_t *app_device_find(app_t *app, uint16_t index) {
    for (size_t i = 0; i < app->device_count; i++) {
        device_t *device = &app->device[i];
        if (device_index(device) == index) {
            return &app->device[i];
        }
    }
    return NULL;
}

device_t *app_device_find_by_type(app_t *app, uint16_t type) {
    for (size_t i = 0; i < app->device_count; i++) {
        device_t *device = &app->device[i];
        if (device->class->type == type) {
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
        return APP_SIGNAL_OUT_OF_MEMORY;
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
