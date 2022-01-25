#ifndef INC_CORE_TASK
#define INC_CORE_TASK

#include "app.h"
#include "core/event.h"

#define APP_TASK_HALT_INDEX 64
#define APP_TASK_SUCCESS_INDEX -1

struct app_task {
  char *name;

  device_t *device;
  device_tick_t *tick;
  app_thread_t *thread;

  app_event_t inciting_event;
  app_event_t awaited_event;

  uint32_t counter;
  
  size_t phase_index;
  size_t step_index;

  device_on_task_t handler;
};

enum app_task_signal {
  APP_TASK_CONTINUE,
  APP_TASK_COMPLETE,
  APP_TASK_RETRY,
  APP_TASK_HALT,
  APP_TASK_STEP_RETRY,
  APP_TASK_STEP_WAIT,
  APP_TASK_STEP_COMPLETE,
  APP_TASK_STEP_HALT
};

app_signal_t app_task_execute(app_task_t *task);
app_task_signal_t app_task_advance(app_task_t *task);
app_signal_t app_task_finalize(app_task_t *task);

#endif