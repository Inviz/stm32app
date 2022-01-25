#include "led.h"

static ODR_t led_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    indicator_led_t *led = stream->object;
    (void)led;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);

    switch (stream->subIndex) {
    // allow state be changed over network
    case INDICATOR_LED_PHASE: actor_on_phase_change(led->actor, led->properties->phase); break;
    // actors may request changing duty cycle
    case INDICATOR_LED_DUTY_CYCLE:
        if (led->properties->duty_cycle == 0) {
            actor_set_phase(led->actor, ACTOR_IDLE);
            actor_gpio_clear(led->properties->port, led->properties->pin);
        } else {
            actor_set_phase(led->actor, ACTOR_RUNNING);
            actor_gpio_set(led->properties->port, led->properties->pin);
        }
        break;
    default: break;
    }
    return result;
}

static app_signal_t led_validate(indicator_led_properties_t *properties) {
    return properties->port == 0 || properties->pin == 0;
}

static app_signal_t led_construct(indicator_led_t *led) {
    (void)led;
    return 0;
}

static app_signal_t led_start(indicator_led_t *led) {
    log_printf("    > LED%i", led->actor->seq + 1);
    gpio_configure_output_pulldown(led->properties->port, led->properties->pin, GPIO_MEDIUM);
    (void)led;
    return 0;
}

static app_signal_t led_stop(indicator_led_t *led) {
    (void)led;
    return 0;
}

static app_signal_t led_on_phase(indicator_led_t *led, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_STARTING: actor_set_phase(led->actor, ACTOR_IDLE); break;
    default: break;
    }
    return 0;
}

actor_class_t indicator_led_class = {.type = INDICATOR_LED,
                                      .size = sizeof(indicator_led_t),
                                      .phase_subindex = INDICATOR_LED_PHASE,
                                      .validate = (app_method_t)led_validate,
                                      .construct = (app_method_t)led_construct,
                                      .start = (app_method_t)led_start,
                                      .stop = (app_method_t)led_stop,
                                      .on_phase = (actor_on_phase_t)led_on_phase,
                                      .property_write = led_property_write,
                                      .property_write_handlers = (1 << (INDICATOR_LED_DUTY_CYCLE - INDICATOR_LED_PHASE + 1))};
