#include "event.h"

app_event_t *app_event_from_vpool(app_event_t *event, struct vpool *vpool) {
    vpool_export(vpool, &event->data, &event->size);
    return event;
}