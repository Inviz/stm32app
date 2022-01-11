#include "w25.h"

/* Start of autogenerated OD accessors */

/* End of autogenerated OD accessors */

static ODR_t OD_write_storage_w25_property(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_w25_t *w25 = stream->object;
    (void)w25;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t w25_validate(OD_entry_t *config_entry) {
    storage_w25_config_t *config = (storage_w25_config_t *)OD_getPtr(config_entry, 0x01, 0, NULL);
    (void)config;
    if (false) {
        return CO_ERROR_OD_PARAMETERS;
    }
    return 0;
}

static app_signal_t w25_construct(storage_w25_t *w25, device_t *device) {
    w25->config = (storage_w25_config_t *)OD_getPtr(device->config, 0x01, 0, NULL);
    return w25->config->disabled;
}
static app_signal_t w25_start(storage_w25_t *w25) {
    return storage_w25_command(w25, 0xAB);
}

static app_signal_t w25_stop(storage_w25_t *w25) {
    return storage_w25_command(w25, 0xB9);
}

static app_signal_t w25_pause(storage_w25_t *w25) {
    (void)w25;
    return 0;
}

static app_signal_t w25_resume(storage_w25_t *w25) {
    (void)w25;
    return 0;
}

static app_signal_t w25_tick(storage_w25_t *w25, uint32_t time_passed, uint32_t *next_tick) {
    (void)w25;
    (void)time_passed;
    (void)next_tick;
    return 0;
}

/* Link w25 device with its spi module (i2c, spi, uart) */
static app_signal_t w25_link(storage_w25_t *w25) {
    device_link(w25->device, (void **)&w25->spi, w25->config->spi_index, NULL);
    return 0;
}

static app_signal_t w25_phase(storage_w25_t *w25, device_phase_t phase) {
    (void)w25;
    (void)phase;
    return 0;
}

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

#define min(a, b) (a > b ? b : a)
#define max(a, b) (a < b ? b : a)

uint32_t get_number_of_bytes_intesecting_page(address, page_size) {
    size_t current_address = address;
    size_t page_start_offset = current_address % page_size;
    size_t last_byte_on_page = min(current_address + page_size, address + page_size);
    return last_byte_on_page - page_start_offset;
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
    default: return *task->resulting_event->data & W25_SR1_BUSY ? APP_TASK_STEP_RETRY : APP_TASK_STEP_COMPLETE;
    }
}

// Send command and receieve response
static app_task_signal_t step_set_lock(app_task_t *task, bool_t state) {
    return step_send(task, (uint8_t[]){state ? W25_CMD_UNLOCK : W25_CMD_LOCK}, 1);
}

static app_task_signal_t step_write_in_pages(app_task_t *task, uint32_t address, uint8_t *data, size_t size, size_t page_size) {
    uint32_t bytes_on_the_page = get_number_of_bytes_intesecting_page(address + task->counter, page_size);
    switch (task->step_index) {
    case 0: return step_set_lock(task, false);
    case 2: return step_send_spi(task, data, bytes_on_the_page);
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

static app_task_signal_t task_introspection(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_fetch(task, (uint8_t[]){W25_CMD_MANUF_DEVICE, 0xff, 0xff, 0x00}, 4, 2);
    default: return APP_TASK_COMPLETE;
    }
}

static app_task_signal_t task_lock(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_set_lock(task, true);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t task_unlock(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_set_lock(task, false);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t task_write(app_task_t *task) {
    switch (task->phase_index) {
    case 1: return step_wait_until_ready(task);
    case 2: return wstep_rite_in_pages(task, (uint32_t)task->issuing_event->argument, task->issuing_event->data, task->issuing_event->size, 256);
    default: return APP_TASK_COMPLETE;
    }
}

static app_signal_t w25_tick_async(storage_w25_t *w25, app_event_t *event, device_tick_t *tick, app_thread_t *thread) {
    if (event->type == APP_EVENT_THREAD_ALARM) {
        return app_task_execute(&w25->task);
    }
    return 0;
}

static app_signal_t w25_tick_input(storage_w25_t *w25, app_event_t *event, device_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_READ: break;
    case APP_EVENT_INTROSPECTION:
        return device_event_handle_and_start_task(w25->device, event, &w25->task, w25->device->app->threads->async,
                                            storage_w25_task_introspection);
    case APP_EVENT_RESPONSE:
        if (event->producer == w25->spi) {
            return device_event_handle_and_pass_to_task(w25->device, event, &w25->task, w25->device->app->threads->async,
                                            storage_w25_task_introspection);
        }
        break;
    default: return 0;
    }
}

device_callbacks_t storage_w25_callbacks = {.validate = storage_w25_validate,
                                                .construct = (int (*)(void *, device_t *))storage_w25_construct,
                                                .link = (int (*)(void *))storage_w25_link,
                                                .start = (int (*)(void *))storage_w25_start,
                                                .stop = (int (*)(void *))storage_w25_stop,
                                                .pause = (int (*)(void *))storage_w25_pause,
                                                .resume = (int (*)(void *))storage_w25_resume,
                                                //.accept = (int (*)(void *, device_t *device, void *channel))storage_w25_accept,
                                                .tick_input = (device_tick_callback_t)w25_tick_input,
                                                .tick_async = (device_tick_callback_t)w25_tick_async,
                                                .phase = (int (*)(void *, device_phase_t phase))storage_w25_phase,
                                                .write_values = OD_write_storage_w25_property};
