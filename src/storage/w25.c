#include "w25.h"


static app_task_signal_t step_write_spi(app_task_t *task, uint8_t *data, size_t size) {
    app_publish(task->device->app, &((app_event_t){
                                       .type = APP_EVENT_WRITE,
                                       .consumer = ((storage_w25_t *)task->device->object)->spi,
                                       .producer = task->device,
                                       .data = data,
                                       .size = size,
                                   }));
    return APP_TASK_STEP_WAIT;
}
static app_task_signal_t step_read_spi(app_task_t *task, size_t size) {
    app_publish(task->device->app, &((app_event_t){
                                       .type = APP_EVENT_READ,
                                       .consumer = ((storage_w25_t *)task->device->object)->spi,
                                       .producer = task->device,
                                       .size = size,
                                   }));
    return APP_TASK_STEP_WAIT;
}

// Send command and receieve response
static app_task_signal_t step_fetch(app_task_t *task, uint8_t *data, size_t size, size_t reply_size) {
    switch (task->step_index) {
    case 0: return step_write_spi(task, data, size);
    case 1: return step_read_spi(task, reply_size);
    default: return APP_TASK_STEP_COMPLETE;
    }
}
// Send command without wait_until_readying for response
static app_task_signal_t step_send(app_task_t *task, uint8_t *data, size_t size) {
    switch (task->step_index) {
    case 0: return step_write_spi(task, data, size);
    default: return APP_TASK_STEP_COMPLETE;
    }
}

// Query SR signal in a loop to check if device is ready to accept commands
static app_task_signal_t step_wait_until_ready(app_task_t *task) {
    switch (task->step_index) {
    case 0: return step_write_spi(task, (uint8_t[]){W25_CMD_READ_SR1}, 1);
    case 1: return step_read_spi(task, 1);
    default: return (*task->awaited_event.data & W25_SR1_BUSY) ? APP_TASK_STEP_RETRY : APP_TASK_STEP_COMPLETE;
    }
}

// Send command and receieve response
static app_task_signal_t step_set_lock(app_task_t *task, bool_t state) {
    return step_send(task, (uint8_t[]){state ? W25_CMD_UNLOCK : W25_CMD_LOCK}, 1);
}

static app_task_signal_t step_write_in_pages(app_task_t *task, uint32_t address, uint8_t *data, size_t size, size_t page_size) {
    uint32_t bytes_on_page = get_number_of_bytes_intesecting_page(address + task->counter, page_size);
    switch (task->step_index) {
    case 0: return step_set_lock(task, false);
    case 2: return step_write_spi(task, data, bytes_on_page);
    default:
        task->counter += bytes_on_page;
        if (task->counter == size) {
            task->counter = 0;
            return APP_TASK_STEP_COMPLETE;
        } else {
            return APP_TASK_STEP_RETRY;
        }
    }
}

static app_task_signal_t w25_task_introspection(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_fetch(task, (uint8_t[]){W25_CMD_MANUF_DEVICE, 0xff, 0xff, 0x00}, 4, 2);
    default: return APP_TASK_COMPLETE;
    }
}

static app_task_signal_t w25_send_command(app_task_t *task, uint8_t command) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_send(task, (uint8_t[]){ command }, 1);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_lock(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_set_lock(task, true);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_unlock(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_set_lock(task, false);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_enable(app_task_t *task) {
    return w25_send_command(task, W25_CMD_PWR_ON);
}
static app_task_signal_t w25_task_disable(app_task_t *task) {
    return w25_send_command(task, W25_CMD_PWR_OFF);
}

static app_task_signal_t w25_task_write(app_task_t *task) {
    switch (task->phase_index) {
    case 1: return step_wait_until_ready(task);
    case 2:
        return step_write_in_pages(task, (uint32_t)task->inciting_event.argument, task->inciting_event.data, task->inciting_event.size,
                                   256);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_read(app_task_t *task) {
    switch (task->phase_index) {
    default: return APP_TASK_COMPLETE;
    }
}

static app_signal_t w25_tick_high_priority(storage_w25_t *w25, app_event_t *event, device_tick_t *tick, app_thread_t *thread) {
    (void) tick;
    (void) thread;
    if (event->type == APP_EVENT_THREAD_ALARM) {
        return app_task_execute(&w25->task);
    }
    return 0;
}

static ODR_t w25_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_w25_t *w25 = stream->object;
    (void)w25;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t w25_validate(storage_w25_properties_t *properties) {
    return properties->phase != DEVICE_ENABLED;
}

static app_signal_t w25_construct(storage_w25_t *w25) {
    return 0;
}

static app_signal_t w25_stop(storage_w25_t *w25) {
    return device_event_handle_and_start_task(w25->device, &((app_event_t){
        .type = APP_EVENT_DISABLE
    }), &w25->task, w25->device->app->threads->bg_priority, w25_task_disable);
}

static app_signal_t w25_start(storage_w25_t *w25) {
    return device_event_handle_and_start_task(w25->device, &((app_event_t){
        .type = APP_EVENT_ENABLE
    }), &w25->task, w25->device->app->threads->bg_priority, w25_task_enable);
}

/* Link w25 device with its spi module (i2c, spi, uart) */
static app_signal_t w25_link(storage_w25_t *w25) {
    device_link(w25->device, (void **)&w25->spi, w25->properties->spi_index, NULL);
    return 0;
}

static app_signal_t w25_phase(storage_w25_t *w25, device_phase_t phase) {
    (void)w25;
    (void)phase;
    return 0;
}


static app_signal_t w25_tick_input(storage_w25_t *w25, app_event_t *event, device_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_INTROSPECTION:
        return device_event_handle_and_start_task(w25->device, event, &w25->task, w25->device->app->threads->high_priority, w25_task_introspection);
    case APP_EVENT_WRITE:
        return device_event_handle_and_start_task(w25->device, event, &w25->task, w25->device->app->threads->high_priority, w25_task_write);
    case APP_EVENT_READ:
        return device_event_handle_and_start_task(w25->device, event, &w25->task, w25->device->app->threads->high_priority, w25_task_read);
    case APP_EVENT_LOCK:
        return device_event_handle_and_start_task(w25->device, event, &w25->task, w25->device->app->threads->high_priority, w25_task_lock);
    case APP_EVENT_UNLOCK:
        return device_event_handle_and_start_task(w25->device, event, &w25->task, w25->device->app->threads->high_priority, w25_task_unlock);
    case APP_EVENT_RESPONSE:
        if (event->producer == w25->spi && w25->task.handler != NULL) {
            return device_event_handle_and_pass_to_task(w25->device, event, &w25->task, w25->device->app->threads->high_priority,
                                                        w25->task.handler);
        }
        break;
    default: return 0;
    }
}

device_methods_t storage_w25_methods = {.validate = (app_method_t) w25_validate,
                                        .construct = (app_method_t)w25_construct,
                                        .link = (app_method_t)w25_link,
                                        .start = (app_method_t)w25_start,
                                        .stop = (app_method_t)w25_stop,
                                        .tick_input = (device_tick_callback_t)w25_tick_input,
                                        .tick_high_priority = (device_tick_callback_t)w25_tick_high_priority,
                                        .callback_phase = (device_callback_phase_t)w25_phase,
                                        .property_write = w25_property_write};
