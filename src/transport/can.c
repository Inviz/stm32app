#include "can.h"

static ODR_t can_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    transport_can_t *can = stream->object;
    (void)can;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t can_validate(transport_can_properties_t *properties) {
    return properties->tx_port == 0 || properties->rx_port == 0;
}

static app_signal_t can_construct(transport_can_t *can) {
    return 0;
}

static app_signal_t can_start(transport_can_t *can) {
    log_printf("    > CAN%i TX ", can->device->seq + 1);
    gpio_configure_output(can->properties->tx_port, can->properties->tx_pin, 0);
    log_printf("    > CAN%i RX ", can->device->seq + 1);
    gpio_configure_input(can->properties->rx_port, can->properties->rx_pin);

#ifdef STM32F1
    if (can->properties->tx_port == 2 && can->properties->tx_pin == 8) {
        rcc_periph_clock_enable(RCC_AFIO);
        gpio_primary_remap(                   // Map CAN1 to use PB8/PB9
            AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, // Optional
            AFIO_MAPR_CAN1_REMAP_PORTB);
    }
#endif

    // Only configure CAN interface if it's not claimed by CANopenNode
    if (can->canopen == NULL) {
    }

    return 0;
}

static app_signal_t can_stop(transport_can_t *can) {
    (void)can;
    return 0;
}

static app_signal_t can_link(transport_can_t *can) {
    (void)can;
    return 0;
}

static app_signal_t can_phase(transport_can_t *can, device_phase_t phase) {
    (void)can;
    (void)phase;
    return 0;
}

// CANopenNode's driver configures its CAN interface so we dont have to
static app_signal_t can_accept(transport_can_t *can, device_t *origin, void *arg) {
    (void)arg;
    can->canopen = origin;
    return 0;
}

device_class_t transport_can_class = {
    .type = TRANSPORT_CAN,
    .size = sizeof(transport_can_t),
    .phase_subindex = TRANSPORT_CAN_PHASE,
    .validate = (app_method_t)can_validate,
    .construct = (app_method_t)can_construct,
    .link = (app_method_t)can_link,
    .start = (app_method_t)can_start,
    .stop = (app_method_t)can_stop,
    .callback_link = (device_callback_argument_t)can_accept,
    .callback_phase = (device_callback_phase_t)can_phase,
    .property_write = can_property_write,
};
