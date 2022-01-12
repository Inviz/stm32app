#include "task.h"


app_signal_t app_task_execute(app_task_t *task) {
  configASSERT(task);
  app_task_signal_t task_signal = app_task_advance(task);
  switch (task_signal) {
    case APP_TASK_COMPLETE:
    case APP_TASK_HALT:
      app_task_finalize(task);
      device_tick_catchup(task->device, task->tick);
      break;
    case APP_TASK_STEP_WAIT:
      device_event_finalize(task->device, &task->awaited_event);  // free up room for a new event
      break;
    default:
      break;
  }
  return task_signal;
}

app_task_signal_t app_task_advance(app_task_t *task) {
  app_task_signal_t signal = task->handler(task);
  switch (signal) {
    case APP_TASK_CONTINUE:
    case APP_TASK_COMPLETE:
      task->phase_index++;
      break;
      
    case APP_TASK_RETRY:
      task->step_index = 0;
      task->phase_index = 0;
      break;
      
    case APP_TASK_HALT:
      task->phase_index = APP_TASK_HALT_INDEX;
      break;

    case APP_TASK_STEP_RETRY:
      task->step_index = 0;
      break;

    case APP_TASK_STEP_WAIT:
    case APP_TASK_STEP_COMPLETE:
      task->step_index++;
      break;
      
    case APP_TASK_STEP_HALT:
      task->step_index = APP_TASK_HALT_INDEX;
      break;
  }
  return signal;
}

app_signal_t app_task_finalize(app_task_t *task) {
  if (task->device->methods->callback_task != NULL) {
    task->device->methods->callback_task(task->device->object, task);
  }
  device_event_finalize(task->device, &task->inciting_event);
  device_event_finalize(task->device, &task->awaited_event);
}
