#include "thread.h"


int device_tick_allocate(device_tick_t **destination, int (*callback)(void *object, device_tick_t *tick, app_thread_t *thread)) {
    if (callback != NULL) {
        *destination = malloc(sizeof(device_tick_t));
        if (*destination == NULL) {
            return CO_ERROR_OUT_OF_MEMORY;
        }
        (*destination)->callback = callback;
    }
    return 0;
}

void device_tick_free(device_tick_t **tick) {
    if (*tick != NULL) {
        free(*tick);
        *tick = NULL;
    }
}

int app_thread_allocate(app_thread_t **destination, void *app_or_object, void (*callback)(void *ptr), const char *const name, uint16_t stack_depth,
                        size_t priority, void *argument) {
    *destination = (app_thread_t *) malloc(sizeof(app_thread_t));
    app_thread_t *thread = *destination;
    thread->device = ((app_t *) app_or_object)->device;
    thread->semaphore = xSemaphoreCreateBinary();
    if (thread->semaphore == NULL) {
        return CO_ERROR_OUT_OF_MEMORY;
    }
    xTaskCreate(callback, name, stack_depth, (void *)thread, priority, (void *)&thread->task);
    if (thread->task == NULL) {
        return CO_ERROR_OUT_OF_MEMORY;
    }
    thread->argument = argument;
    return 0;
}

int app_thread_free(app_thread_t **thread) {
    free(*thread);
    free((*thread)->semaphore);
    *thread = NULL;
    return 0;
}

static inline device_tick_t *device_tick_by_index(device_t *device, size_t index) {
    device_tick_t *ticks = (device_tick_t *)&(device->ticks);
    return &ticks[index];
}

void app_thread_execute(void *thread_ptr) {
    app_thread_t *thread = (app_thread_t *) thread_ptr;
    thread->last_time = xTaskGetTickCount();

    app_t *app = thread->device->app;
    size_t handler_index = (uint32_t)thread->argument;
    while (true) {
        // Tick at least once every minute if no devices scheduled it sooner
        thread->current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        thread->next_time = thread->last_time + 60000;

        // Run all devices that are subscribed to the task
        for (size_t i = 0; i < app->device_count; i++) {
            device_t *device = &app->device[i];
            device_tick_t *tick = device_tick_by_index(device, handler_index);
            
            // Subscribed devices have corresponding tick handler
            if (tick == NULL) {
                continue;
            }
            
            // Ticks only run when time is right
            if (tick->next_time >= thread->current_time) {
              tick->callback(device->object, NULL, tick, thread);
              tick->last_time = thread->current_time;
            }

            // Ensure that thread wakes up by the next scheduled tick 
            if (tick->next_time <= thread->current_time && thread->next_time > tick->next_time) {
                thread->next_time = tick->next_time;
            }
        }

        xSemaphoreTake(thread->task, TIME_DIFF(thread->next_time, thread->current_time) / portTICK_PERIOD_MS);
    }
}

void app_thread_wake_from_isr(app_thread_t *thread) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(thread->semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void app_thread_wake(app_thread_t *thread) {
    xSemaphoreGive(thread->semaphore);
    portYIELD();
}


void device_tick_schedule(device_tick_t *tick, uint32_t next_time) {
  tick->next_time = next_time;
}

void device_tick_delay(device_tick_t *tick, app_thread_t *thread, uint32_t timeout) {
  device_tick_schedule(tick, thread->current_time + timeout);
}

int device_ticks_allocate(device_t *device) {
    device->ticks = (device_ticks_t *)malloc(sizeof(device_ticks_t));
    if (device->ticks == NULL || //
        device_tick_allocate(&device->ticks->input, device->callbacks->input_tick) ||
        device_tick_allocate(&device->ticks->output, device->callbacks->output_tick) ||
        device_tick_allocate(&device->ticks->async, device->callbacks->async_tick) ||
        device_tick_allocate(&device->ticks->poll, device->callbacks->poll_tick) ||
        device_tick_allocate(&device->ticks->idle, device->callbacks->idle_tick)) {
        return CO_ERROR_OUT_OF_MEMORY;
    }

    return 0;
}

int device_ticks_free(device_t *device) {
    device_tick_free(&device->ticks->input);
    device_tick_free(&device->ticks->output);
    device_tick_free(&device->ticks->async);
    device_tick_free(&device->ticks->poll);
    device_tick_free(&device->ticks->idle);
    free(device->ticks);
    return 0;
}

int app_threads_allocate(app_t *app) {
    if (app_thread_allocate(&app->threads->input, app, app_thread_execute, "App: Input", 200, 5, (void *)(uint32_t)0) ||
        app_thread_allocate(&app->threads->async, app, app_thread_execute, "App: Async", 200, 4, (void *)(uint32_t)1) ||
        app_thread_allocate(&app->threads->output, app, app_thread_execute, "App: Output", 200, 3, (void *)(uint32_t)2) ||
        app_thread_allocate(&app->threads->poll, app, app_thread_execute, "App: Poll", 200, 2, (void *)(uint32_t)3) ||
        app_thread_allocate(&app->threads->idle, app, app_thread_execute, "App: Idle", 200, 1, (void *)(uint32_t)4)) {
        return CO_ERROR_OUT_OF_MEMORY;
    } else {
        return 0;
    }
}

int app_threads_free(app_t *app) {
    app_thread_free(&app->threads->input);
    app_thread_free(&app->threads->output);
    app_thread_free(&app->threads->async);
    app_thread_free(&app->threads->poll);
    app_thread_free(&app->threads->idle);
    free(app->threads);
    return 0;
}
