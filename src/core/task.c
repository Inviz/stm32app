#include "task.h"


void app_task_execute(app_task_t *task) {
  switch (task->handler(task)) {
    case APP_TASK_CONTINUE:
      if (task->step_index == 0) {
        task->phase_index++; 
      } else {
        task->step_index++;
      }
      break;

    case APP_TASK_COMPLETE:
      task->step_index++;
      break;
      
    case APP_TASK_RETRY:
      task->step_index = 0;
      task->phase_index = 0;
      break;
      
    case APP_TASK_HALT:
      task->phase_index = -1;
      break;

    case APP_TASK_STEP_RETRY:
      task->step_index = 0;
      break;

    case APP_TASK_STEP_WAIT:
      break;
      
    case APP_TASK_STEP_COMPLETE:
      task->step_index++;
      break;
      
    case APP_TASK_STEP_HALT:
      task->step_index = 0;
      task->phase_index++;
      break;
  }
}