#include "timer.h"
#include "system/mcu.h"
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

volatile module_timer_t *module_timers[TIMER_UNITS];

static ODR_t timer_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    module_timer_t *timer = stream->object;
    (void)timer;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t timer_validate(module_timer_properties_t *properties) {
    return 0;
}

static app_signal_t timer_construct(module_timer_t *timer) {
    timer->subscriptions = malloc(sizeof(module_timer_subscription_t) * timer->properties->initial_subscriptions_count);
    timer->next_time = -1;
    timer->next_tick = timer->properties->period;

    module_timers[timer->actor->seq] = timer;

    switch (timer->actor->seq) {
    case 0:
        timer->clock = RCC_TIM1;
        timer->irq = NVIC_TIM1_CC_IRQ;
        timer->address = TIM1;
        timer->reset = RCC_TIM1;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 0;
#endif
#endif
#if defined(RCC_APB1ENR_TIM1EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM1EN;
#elif defined(RCC_APB2ENR_TIM1EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM1EN;
#endif
        break;
#ifdef TIM2
    case 1:
        timer->clock = RCC_TIM2;
        timer->irq = NVIC_TIM2_IRQ;
        timer->address = TIM2;
        timer->reset = RCC_TIM2;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 0;
#endif
#endif
#if defined(RCC_APB1ENR_TIM2EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM2EN;
#elif defined(RCC_APB2ENR_TIM2EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM2EN;
#endif
        break;
#endif
#ifdef TIM3
    case 2:
        timer->clock = RCC_TIM3;
        timer->irq = NVIC_TIM3_IRQ;
        timer->address = TIM3;
        timer->reset = RCC_TIM3;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 1;
#endif
#endif
#if defined(RCC_APB1ENR_TIM3EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM3EN;
#elif defined(RCC_APB2ENR_TIM3EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM3EN;
#endif
        break;
#endif
#ifdef TIM4
    case 3:
        timer->clock = RCC_TIM4;
        timer->irq = NVIC_TIM4_IRQ;
        timer->address = TIM4;
        timer->reset = RCC_TIM4;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 2;
#endif
#endif
#if defined(RCC_APB1ENR_TIM4EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM4EN;
#elif defined(RCC_APB2ENR_TIM4EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM4EN;
#endif
        break;
#endif
#ifdef TIM5
    case 4:
        timer->clock = RCC_TIM5;
        timer->irq = NVIC_TIM5_IRQ;
        timer->address = TIM5;
        timer->reset = RCC_TIM5;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 3;
#endif
#endif
#if defined(RCC_APB1ENR_TIM5EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM5EN;
#elif defined(RCC_APB2ENR_TIM5EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM5EN;
#endif
        break;
#endif
#ifdef TIM6
    case 5:
        timer->clock = RCC_TIM6;
        timer->irq = NVIC_TIM6_DAC_IRQ;
        timer->address = TIM6;
        timer->reset = RCC_TIM6;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 4;
#endif
#endif
#if defined(RCC_APB1ENR_TIM6EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM6EN;
#elif defined(RCC_APB2ENR_TIM6EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM6EN;
#endif
        break;
#endif
#ifdef TIM7
    case 6:
        timer->clock = RCC_TIM7;
        timer->irq = NVIC_TIM7_IRQ;
        timer->address = TIM7;
        timer->reset = RCC_TIM7;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 5;
#endif
#endif
#if defined(RCC_APB1ENR_TIM7EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM7EN;
#elif defined(RCC_APB2ENR_TIM7EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM7EN;
#endif
        break;
#endif
#ifdef TIM8
    case 7:
        timer->clock = RCC_TIM8;
        timer->irq = NVIC_TIM8_CC_IRQ;
        timer->address = TIM8;
        timer->reset = RCC_TIM8;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 6;
#endif
#endif
#if defined(RCC_APB1ENR_TIM8EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM8EN;
#elif defined(RCC_APB2ENR_TIM8EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM8EN;
#endif
        break;
#endif
#ifdef TIM9
    case 8:
        timer->clock = RCC_TIM9;
        timer->irq = NVIC_TIM1_BRK_TIM9_IRQ;
        timer->address = TIM9;
        timer->reset = RCC_TIM9;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 1;
#endif
#endif
#if defined(RCC_APB1ENR_TIM9EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM9EN;
#elif defined(RCC_APB2ENR_TIM9EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM9EN;
#endif
        break;
#endif
#ifdef TIM10
    case 9:
        timer->clock = RCC_TIM10;
        timer->irq = NVIC_TIM1_UP_TIM10_IRQ;
        timer->address = TIM10;
        timer->reset = RCC_TIM10;
#ifdef DEBUG
#if defined(STM32F4)
        timer->debug_stopper = 1 << 16;
#endif
#endif
#if defined(RCC_APB1ENR_TIM10EN)
        timer->source = 0;
        timer->peripheral_clock = RCC_APB1ENR_TIM10EN;
#elif defined(RCC_APB2ENR_TIM10EN)
        timer->source = 1;
        timer->peripheral_clock = RCC_APB2ENR_TIM10EN;
#endif
        break;
#endif
    default: error_printf("TIM%i is not supported by this actor", timer->actor->seq + 1);
    }
    return 0;
}

// find subscription that is closest to the current time
static module_timer_subscription_t *timer_get_next_subscription(module_timer_t *timer) {
    module_timer_subscription_t *result = NULL;
    int difference = 0x7FFFFFFF;
    for (size_t i = 0; i < timer->properties->initial_subscriptions_count; i++) {
        module_timer_subscription_t *subscription = &timer->subscriptions[i];
        if (subscription->actor == NULL) {
            continue;
        }
        int d = subscription->time - timer->next_time;
        if (difference > d) {
            difference = d;
            result = subscription;
        }
    }
    return result;
}

static app_signal_t timer_schedule(module_timer_t *timer, uint32_t next_time) {
    if (timer->next_time > next_time) {
        timer->next_time = next_time;
    }

    uint32_t diff = (uint32_t)((int32_t)timer->next_time - (int32_t)timer->current_time);
    // when next tick is closer than a full period timer, needs to be advanced
    if (diff < timer->properties->period) {
        timer->next_tick = diff;
        timer_set_counter(timer->address, timer->properties->period - timer->next_tick);
    } else {
        timer->next_tick = timer->properties->period;
    }
    return 0;
}

static app_signal_t timer_notify(module_timer_t *timer) {
    for (size_t i = 0; i < timer->properties->initial_subscriptions_count; i++) {
        module_timer_subscription_t *subscription = &timer->subscriptions[i];
        if (subscription->actor == NULL) {
            continue;
        }
        int diff = subscription->time - timer->current_time;
        if (diff <= 0) {
            actor_t *actor = subscription->actor;
            void *argument = subscription->argument;
            log_printf("~ Timeout for 0x%x %s (argument: %lu)\n", actor_index(actor), get_actor_type_name(actor->class->type), (uint32_t)argument);
            *subscription = (module_timer_subscription_t){};
            actor_signal(actor, timer->actor, APP_SIGNAL_TIMEOUT, argument);
        }
    }
    return 0;
}

static app_signal_t timer_advance(module_timer_t *timer, uint32_t time) {
    // start the timer if it was not ticking
    if (actor_get_phase(timer->actor) != ACTOR_RUNNING) {
        actor_set_phase(timer->actor, ACTOR_RUNNING);
    }
    // add passed time to current_time, and adjust timer
    timer->current_time += time;
    timer_set_counter(timer->address, 0);

    // notify subscribers if time for alarm has come
    if (timer->next_time <= timer->current_time) {
        timer_notify(timer);

        // stop the timer if there aren't any more subscriptions left
        module_timer_subscription_t *next = timer_get_next_subscription(timer);
        if (next == NULL) {
            actor_set_phase(timer->actor, ACTOR_IDLE);
            return 0;
        } else {
            timer->next_time = next->time;
        }
    }

    // schedule next period
    return timer_schedule(timer, timer->next_time);
}

static void timer_interrupt(size_t index) {
    module_timer_t *timer = module_timers[index];
    if (timer_get_flag(timer->address, TIM_DIER_UIE)) {
        timer_clear_flag(timer->address, TIM_DIER_UIE);
        timer_advance(timer, timer->next_tick);
    }
}

// find subscription slot for actor/argument pair
static module_timer_subscription_t *timer_get_subscription(module_timer_t *timer, actor_t *actor, void *argument) {
    size_t i = 0;
    size_t free = -1;
    // find subscription that matches actor and argument
    for (; i < timer->properties->initial_subscriptions_count; i++) {
        module_timer_subscription_t *subscription = &timer->subscriptions[i];
        if (subscription->actor == actor && subscription->argument == argument) {
            return subscription;
        } else if (subscription->actor == NULL && free == (uint32_t)-1) {
            free = i;
        }
    }

    // reuse subscription slot
    if (free != (size_t)-1) {
        return &timer->subscriptions[free];
    }

    // too many subscriptions were requested. Should reallocate?
    if (i == timer->properties->initial_subscriptions_count) {
        return NULL;
    }

    return &timer->subscriptions[i];
}

app_signal_t module_timer_timeout(module_timer_t *timer, actor_t *actor, void *argument, uint32_t timeout) {
    // ensure that current_time of a timer is up to date, adjust timers accordingly
    timer_advance(timer, timer_get_counter(timer->address) - (timer->properties->period - timer->next_tick));

    // update subscription
    module_timer_subscription_t *subscription = timer_get_subscription(timer, actor, argument);
    if (subscription == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }
    subscription->actor = actor;
    subscription->argument = argument;

    bool_t was_next = timer->next_time == (uint32_t)-1 || timer->next_time == subscription->time;
    bool_t was_closer = subscription->time != 0 && subscription->time <= timer->current_time + timeout;

    subscription->time = timer->current_time + timeout;

    if (was_next) {
        // if subscription being updated was the one closest to next tick, and it was pushed further away,
        // find if there is any another that is scheduled before that
        module_timer_subscription_t *next = was_closer ? timer_get_next_subscription(timer) : subscription;
        return timer_schedule(timer, next->time);
    }
    return 0;
}

app_signal_t module_timer_clear(module_timer_t *timer, actor_t *actor, void *argument) {
    module_timer_subscription_t *subscription = timer_get_subscription(timer, actor, argument);
    if (subscription == NULL)
        return 0;
    bool_t was_next = timer->next_time == subscription->time;

    *subscription = (module_timer_subscription_t){};

    if (was_next) {
        module_timer_subscription_t *next = timer_get_next_subscription(timer);
        if (next != NULL) {
            return timer_schedule(timer, next->time);
        }
    }
    return 0;
}

static app_signal_t timer_stop_counting(module_timer_t *timer) {
    timer->next_time = -1;
    timer->next_tick = timer->properties->period;
    timer_disable_counter(timer->address);
    return 0;
}

static app_signal_t timer_start_counting(module_timer_t *timer) {
    timer_enable_counter(timer->address);
    return 0;
}

static app_signal_t timer_stop(module_timer_t *timer) {
    if (actor_get_phase(timer->actor) == ACTOR_RUNNING)
        return timer_stop_counting(timer);
    return 0;
}

static app_signal_t timer_start(module_timer_t *timer) {
    uint32_t source_frequency;

    rcc_periph_clock_enable(timer->clock);
    rcc_periph_reset_pulse(timer->reset);

    if (timer->source == 0) {
        source_frequency = timer->actor->app->mcu->clock->apb1_frequency;
        rcc_peripheral_enable_clock(&RCC_APB1ENR, timer->peripheral_clock);
#ifdef DEBUG
        rcc_peripheral_enable_clock(&RCC_APB2ENR, 1 << 22);
        MMIO32(DBGMCU_BASE + 0x8) |= timer->debug_stopper;
#endif

    } else {
        source_frequency = timer->actor->app->mcu->clock->apb2_frequency;
        rcc_peripheral_enable_clock(&RCC_APB2ENR, timer->peripheral_clock);
#ifdef DEBUG
        rcc_peripheral_enable_clock(&RCC_APB2ENR, 1 << 22);
        MMIO32(DBGMCU_BASE + 0xc) |= timer->debug_stopper;
#endif
    }

    timer_set_mode(timer->address, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP); // defaults
    timer_set_prescaler(timer->address, source_frequency / timer->properties->frequency - 1);
    timer_set_clock_division(timer->address, 0);
    timer_set_period(timer->address, timer->properties->period);

    timer_disable_preload(timer->address);
    timer_continuous_mode(timer->address);
    timer_update_on_overflow(timer->address);

    nvic_enable_irq(timer->irq);
    timer_enable_irq(timer->address, TIM_DIER_UIE);
    // timer_enable_irq(timer->address, TIM_DIER_TIE);
    // timer_enable_irq(timer->address, TIM_DIER_BIE);

    return 0;
}

static app_signal_t timer_link(module_timer_t *timer) {
    (void)timer;
    return 0;
}

static app_signal_t timer_on_phase(module_timer_t *timer, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_RUNNING: return timer_start_counting(timer); break;

    case ACTOR_IDLE: return timer_stop_counting(timer); break;

    case ACTOR_STARTING:
    case ACTOR_RESUMING:
        if (timer_get_next_subscription(timer) == NULL) {
            actor_set_phase(timer->actor, ACTOR_IDLE);
        }
        break;

    default: break;
    }
    return 0;
}

actor_class_t module_timer_class = {
    .type = MODULE_TIMER,
    .size = sizeof(module_timer_t),
    .phase_subindex = MODULE_TIMER_PHASE,
    .validate = (app_method_t)timer_validate,
    .construct = (app_method_t)timer_construct,
    .link = (app_method_t)timer_link,
    .start = (app_method_t)timer_start,
    .stop = (app_method_t)timer_stop,
    .on_phase = (actor_on_phase_t)timer_on_phase,
    .property_write = timer_property_write,
};

void tim1_isr(void) {
    timer_interrupt(0);
}
void tim2_isr(void) {
    timer_interrupt(1);
}
void tim3_isr(void) {
    timer_interrupt(2);
}
void tim4_isr(void) {
    timer_interrupt(3);
}
void tim5_isr(void) {
    timer_interrupt(4);
}
void tim6_isr(void) {
    timer_interrupt(5);
}
void tim7_isr(void) {
    timer_interrupt(6);
}
void tim8_isr(void) {
    timer_interrupt(7);
}
void tim9_isr(void) {
    timer_interrupt(8);
}
void tim10_isr(void) {
    timer_interrupt(9);
}