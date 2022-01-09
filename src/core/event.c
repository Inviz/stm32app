#include "event.h"



app_event_t app_event_new(app_event_type_t type, device_t *device) {
  (app_event_t) { .producer = device };

  app_event_t event = {
    .type = type,
    .producer = device,
  };
  return event;
}


bool_t *app_event_needs_to_be_consuemd(app_event_t *event) {
  return event->type >= APP_EVENT_NEEDS_TO_BE_CONSUMED && !event->consumer;
}