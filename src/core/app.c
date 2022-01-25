#include "app.h"
#include "system/canopen.h"

// Count or initialize all actors in OD of given type
size_t app_actor_type_enumerate(app_t *app, OD_t *od, actor_class_t *class,
                                 actor_t *destination, size_t offset) {
    size_t count = 0;

    for (size_t seq = 0; seq < 128; seq++) {
        OD_entry_t *properties = OD_find(od, class->type + seq);
        if (properties == NULL && seq >= 19)
            break;
        if (properties == NULL)
            continue;
        
        uint8_t *phase = OD_getPtr(properties, class->phase_subindex, 0, NULL);

        // compute struct offset for phase property
        class->phase_offset = (void *) phase - OD_getPtr(properties, 0x00, 0, NULL);

        if (*phase != ACTOR_ENABLED || class->validate(OD_getPtr(properties, 0x00, 0, NULL)) != 0) {
            if (app != NULL && app->canopen != NULL) {
                app_error_report(app, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL, OD_getIndex(properties));
            }
            continue;
        }

        count++;
        if (destination == NULL) {
            continue;
        }

        actor_t *actor = &destination[offset + count - 1];
        actor->seq = seq;
        actor->entry = properties;
        actor->class = class;

        actor->entry_extension.write = class->property_write == NULL ? OD_writeOriginal : class->property_write;
        actor->entry_extension.read = class->property_read == NULL ? OD_readOriginal : class->property_read;

        OD_extension_init(properties, &actor->entry_extension);
    }
    return count;
}

actor_t *app_actor_find(app_t *app, uint16_t index) {
    for (size_t i = 0; i < app->actor_count; i++) {
        actor_t *actor = &app->actor[i];
        if (actor_index(actor) == index) {
            return &app->actor[i];
        }
    }
    return NULL;
}

actor_t *app_actor_find_by_type(app_t *app, uint16_t type) {
    for (size_t i = 0; i < app->actor_count; i++) {
        actor_t *actor = &app->actor[i];
        if (actor->class->type == type) {
            return &app->actor[i];
        }
    }
    return NULL;
}

actor_t *app_actor_find_by_number(app_t *app, uint8_t number) {
    return &app->actor[number];
}

uint8_t app_actor_find_number(app_t *app, actor_t *actor) {
    for (size_t i = 0; i < app->actor_count; i++) {
        if (&app->actor[i] == actor) {
            return i;
        }
    }
    return 255;
}

void app_set_phase(app_t *app, actor_phase_t phase) {
    log_printf("Devices - phase %s\n", get_actor_phase_name(phase));
    for (size_t i = 0; i < app->actor_count; i++) {
        if (app->actor[i].phase != ACTOR_DISABLED) {
            actor_set_phase(&app->actor[i], phase);
        }
    }
}

int app_allocate(app_t **app, OD_t *od, size_t (*enumerator)(app_t *app, OD_t *od, actor_t *actors)) {
    // count actors first to allocate specific size of an array
    size_t actor_count = enumerator(NULL, od, NULL);
    actor_t *actors = malloc(sizeof(actor_t) * actor_count);

    if (actors == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }
    // run actor constructors
    enumerator(*app, od, actors);

    for (size_t i = 0; i < actor_count; i++) {
        // allocate memory for actor struct
        int ret = actor_allocate(&actors[i]);
        if (ret != 0) {
            return ret;
        }

        // first actor must be app actor
        // store actor count
        if (i == 0) {
            *app = actors->object;
            (*app)->actor_count = actor_count;
        }

        // link actor to the app
        (&actors[i])->app = *app;
    }

    return 0;
}

int app_free(app_t **app) {
    for (size_t i = 0; i < (*app)->actor_count; i++) {
        int ret = actor_free(&(*app)->actor[i]);
        if (ret != 0)
            return ret;
    }
    (*app)->actor_count = 0;
    free((*app)->actor);
    return 0;
}
