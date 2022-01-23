#include "modbus.h"
#define _MMODBUS_RTU 1
#define _MMODBUS_TXDMA


static ODR_t modbus_property_write(OD_stream_t *stream, const void *buf, OD_size_t count,
                                             OD_size_t *countWritten) {
    transport_modbus_t *modbus = stream->object;
    (void)modbus;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t modbus_validate(transport_modbus_properties_t *properties) {
    return 0;
}

static app_signal_t modbus_construct(transport_modbus_t *modbus) {
    modbus->rx_buffer = malloc(modbus->properties->rx_buffer_size);
    return 0;
}

static app_signal_t modbus_destruct(transport_modbus_t *modbus) {
    free(modbus->rx_buffer);
    return 0;
}

static app_signal_t modbus_start(transport_modbus_t *modbus) {
    (void)modbus;
    device_gpio_clear(modbus->properties->rts_port, modbus->properties->rts_pin);
    return 0;
}

static app_signal_t modbus_stop(transport_modbus_t *modbus) {
    (void)modbus;
    device_gpio_clear(modbus->properties->rts_port, modbus->properties->rts_pin);
    return 0;
}

static app_signal_t modbus_link(transport_modbus_t *modbus) {
    return device_link(modbus->device, (void **)&modbus->usart, modbus->properties->usart_index, NULL);
}

static app_signal_t modbus_phase(transport_modbus_t *modbus, device_phase_t phase) {
    // if this callback is called, it means the request timed out
    switch (phase) {
    case DEVICE_REQUESTING:
    case DEVICE_RESPONDING:
        transport_modbus_cancel(modbus);
        break;
    default:
        break;
    }
    return 0;
}

// int transport_modbus_send(transport_modbus_t *modbus, uint8_t *data, uint8_t length) { return 0; }

static app_signal_t modbus_validate_message(transport_modbus_t *modbus, uint8_t *data) {
    // check message crc
    if (modbus->ab(*(uint16_t *)(&data[5])) != transport_modbus_crc16(data, data[2] + 3)) {
        return 1;
    }

    return 0;
}

/* Copy memory from the circular buffer */
static void transport_modbus_ingest_buffer(transport_modbus_t *modbus) {
    size_t pos = transport_usart_get_buffer_size_written(modbus->usart);
    if (pos != modbus->rx_position) {    /* Check change in received data */
        if (pos > modbus->rx_position) { /* Current position is over previous one */
            memcpy(modbus->usart->dma_rx_buffer, modbus->rx_buffer[modbus->rx_position], pos - modbus->rx_position);
        } else {
            memcpy(modbus->usart->dma_rx_buffer, modbus->rx_buffer[modbus->rx_position],
                   transport_usart_get_buffer_size(modbus->usart) - modbus->rx_position);
            memcpy(modbus->usart->dma_rx_buffer, modbus->rx_buffer[0], pos);
        }
        modbus->rx_position = pos; /* Save current position as old for next transfers */
    }
}

static app_signal_t modbus_signal(transport_modbus_t *modbus, device_t *device, app_signal_t signal, char *source) {
    switch (signal) {
        /* usart is idle, need to wait 3.5 characters to start reading */
        case APP_SIGNAL_RX_COMPLETE:
            module_timer_set(modbus->timer, modbus->device, modbus->idle_timeout, APP_SIGNAL_RX_COMPLETE);
            break;
        /* 3.5 characters delay time is over, ready to process messages in buffer */
        case APP_SIGNAL_TIMEOUT:
            transport_modbus_ingest_buffer(modbus);
            break;
        default:
            break;
    }
}

int transport_modbus_read(transport_modbus_t *modbus, uint8_t *data) {
    transport_modbus_request_t *request;

    // check message crc
    if (modbus->ab(*(uint16_t *)(&data[5])) != transport_modbus_crc16(data, data[2] + 3)) {
        return 1;
    }

    // check if it's a response to current request
    if (device_get_phase(modbus->device) == DEVICE_REQUESTING) {
        request = &modbus->request;
        if (request->type == MODBUS_WRITE_SINGLE_COIL || request->type == MODBUS_WRITE_SINGLE_REGISTER) {
            return memcmp(request, data, 8);
        } else {
            if (data[0] != request->recipient)
                return 1;
            if (data[1] != request->type)
                return 1;
        }

    } else if (data[0] != modbus->properties->slave_address) {
        return 0;

        // copy incoming request
    } else {
        request = transport_modbus_allocate_request(modbus);
        memcpy(request, &data, 6);
    }

    // copy and byte-swap the response
    if (request->response != NULL) {
        for (uint8_t i = 0; i < data[2]; i += 2) {
            uint8_t H = data[i + 3];
            data[i + 3] = data[i + 3 + 1];
            data[i + 3 + 1] = H;
        }
        memcpy(request->response, &data[3], data[2]);
    }

    // respond to request
    if (device_get_phase(modbus->device) != DEVICE_REQUESTING) {
        return transport_modbus_respond(modbus, request);
    }

    // finish with request
    device_set_phase(modbus->device, DEVICE_RUNNING);
    return 0;
}
int transport_modbus_respond(transport_modbus_t *modbus, transport_modbus_request_t *request) {
    (void)request;
    //device_set_temporary_phase(modbus->device, DEVICE_RESPONDING, modbus->properties->timeout);
    device_gpio_clear(modbus->properties->rts_port, modbus->properties->rts_pin);
    return 0;
}

int transport_modbus_cancel(transport_modbus_t *modbus) {
    device_set_phase(modbus->device, DEVICE_RUNNING);
    device_gpio_clear(modbus->properties->rts_port, modbus->properties->rts_pin);
    return 0;
}

// actually does not allocate any memory and reuses the same static struct
transport_modbus_request_t *transport_modbus_allocate_request(transport_modbus_t *modbus) { return &modbus->request; }

int transport_modbus_request(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint8_t type, uint16_t length,
                          uint8_t *response) {
    transport_modbus_request_t *request = transport_modbus_allocate_request(modbus);

    if (device_get_phase(modbus->device) != DEVICE_RUNNING) {
        return 1;
    }
    request->recipient = recipient;
    request->type = type;
    request->index = index;
    request->value = length;
    request->crc = transport_modbus_crc16((uint8_t *)&request, 6);
    request->response = response;

    //device_set_temporary_phase(modbus->device, DEVICE_REQUESTING, modbus->properties->timeout);
    device_gpio_set(modbus->properties->rts_port, modbus->properties->rts_pin);
    return transport_usart_send(modbus->usart, (char *)&request, 8);
}

/* Read coils */
int transport_modbus_read_coil(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint8_t *data) {
    return transport_modbus_read_coils(modbus, recipient, index, 1, data);
}

int transport_modbus_read_coils(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                             uint8_t *data) {
    return transport_modbus_request(modbus, recipient, index, MODBUS_READ_COIL_STATUS, length, data);
}

int transport_modbus_read_discrete_input(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint8_t *data) {
    return transport_modbus_read_discrete_inputs(modbus, recipient, index, 1, data);
}

/* Read discrete inouts */
int transport_modbus_read_discrete_inputs(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                                       uint8_t *data) {
    return transport_modbus_request(modbus, recipient, index, MODBUS_READ_DISCRETE_INPUTS, length, data);
}

/* Reading input registers */
int transport_modbus_read_input_registers_8i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                                          uint8_t *data) {
    return transport_modbus_request(modbus, recipient, index, MODBUS_READ_INPUT_REGISTERS, length, data);
}

int transport_modbus_read_input_registers_16i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                                           uint16_t *data) {
    if (transport_modbus_read_input_registers_8i(modbus, recipient, index, length * 1, (uint8_t *)data)) {
        return 1;
    }
    for (uint16_t i = 0; i < length; i++) {
        data[i] = modbus->ab(data[i]);
    }
    return 0;
}

int transport_modbus_read_input_register_16i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t *data) {
    return transport_modbus_read_input_registers_16i(modbus, recipient, index, 1, data);
}

int transport_modbus_read_input_registers_32f(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                                           float *data) {
    if (!transport_modbus_read_input_registers_8i(modbus, recipient, index, length * 2, (uint8_t *)data)) {
        return 1;
    }
    for (uint32_t i = 0; i < length; i++) {
        data[i] = modbus->abcd(data[i]);
    }
    return 0;
}

int transport_modbus_read_input_register_32f(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, float *data) {
    return transport_modbus_read_input_registers_32f(modbus, recipient, index, 1, data);
}

int transport_modbus_read_input_register_32i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint32_t *data) {
    return transport_modbus_read_input_registers_32i(modbus, recipient, index, 1, data);
}

int transport_modbus_read_input_registers_32i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                                           uint32_t *data) {
    return transport_modbus_read_input_registers_32f(modbus, recipient, index, length, (float *)data);
}

/*  Reading holding registers */

int transport_modbus_read_holding_registers_8i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t length,
                                            uint8_t *data) {
    return transport_modbus_request(modbus, recipient, index, MODBUS_READ_HOLDING_REGISTERS, length, data);
}

int transport_modbus_read_holding_registers_16i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index,
                                             uint16_t length, uint16_t *data) {
    if (!transport_modbus_read_holding_registers_8i(modbus, recipient, index, length * 1, (uint8_t *)data)) {
        return 1;
    }
    for (uint16_t i = 0; i < length; i++) {
        data[i] = modbus->ab(data[i]);
    }
    return 0;
}

int transport_modbus_read_holding_register16i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t *data) {
    return transport_modbus_read_holding_registers_16i(modbus, recipient, index, 1, data);
}

int transport_modbus_read_holding_registers_32f(transport_modbus_t *modbus, uint8_t recipient, uint16_t index,
                                             uint16_t length, float *data) {
    if (!transport_modbus_read_holding_registers_8i(modbus, recipient, index, length * 2, (uint8_t *)data)) {
        return 1;
    }
    for (uint32_t i = 0; i < length; i++) {
        data[i] = modbus->abcd(data[i]);
    }
    return 0;
}

int transport_modbus_read_holding_register32f(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, float *data) {
    return transport_modbus_read_holding_registers_32f(modbus, recipient, index, 1, data);
}

int transport_modbus_read_holding_register32i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint32_t *data) {
    return transport_modbus_read_holding_registers_32i(modbus, recipient, index, 1, data);
}

int transport_modbus_read_holding_registers_32i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index,
                                             uint16_t length, uint32_t *data) {
    return transport_modbus_read_holding_registers_32f(modbus, recipient, index, length, (float *)data);
}

/* Writers */

int transport_modbus_write_holding_register16i(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint16_t data) {
    return transport_modbus_request(modbus, recipient, index, MODBUS_WRITE_SINGLE_REGISTER, data, NULL);
}

int transport_modbus_write_coil(transport_modbus_t *modbus, uint8_t recipient, uint16_t index, uint8_t data) {
    return transport_modbus_request(modbus, recipient, index, MODBUS_WRITE_SINGLE_COIL, data, NULL);
}

static const uint16_t wCRCTable[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241, 0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1,
    0XC481, 0X0440, 0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40, 0X0A00, 0XCAC1, 0XCB81, 0X0B40,
    0XC901, 0X09C0, 0X0880, 0XC841, 0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40, 0X1E00, 0XDEC1,
    0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41, 0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040, 0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1,
    0XF281, 0X3240, 0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441, 0X3C00, 0XFCC1, 0XFD81, 0X3D40,
    0XFF01, 0X3FC0, 0X3E80, 0XFE41, 0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840, 0X2800, 0XE8C1,
    0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41, 0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640, 0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0,
    0X2080, 0XE041, 0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240, 0X6600, 0XA6C1, 0XA781, 0X6740,
    0XA501, 0X65C0, 0X6480, 0XA441, 0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41, 0XAA01, 0X6AC0,
    0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840, 0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40, 0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1,
    0XB681, 0X7640, 0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041, 0X5000, 0X90C1, 0X9181, 0X5140,
    0X9301, 0X53C0, 0X5280, 0X9241, 0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440, 0X9C01, 0X5CC0,
    0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40, 0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40, 0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0,
    0X4C80, 0X8C41, 0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641, 0X8201, 0X42C0, 0X4380, 0X8341,
    0X4100, 0X81C1, 0X8081, 0X4040};

uint16_t transport_modbus_crc16(const uint8_t *nData, uint16_t wLength) {
    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;
    while (wLength--) {
        nTemp = *nData++ ^ wCRCWord;
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }
    return wCRCWord;
}
device_class_t transport_modbus_class = {
    .type = TRANSPORT_MODBUS,
    .size = sizeof(transport_modbus_t),
    .phase_subindex = TRANSPORT_MODBUS_PHASE,
    .validate = (app_method_t) modbus_validate,
    .construct = (app_method_t)modbus_construct,
    .link = (app_method_t) modbus_link,
    .destruct = (app_method_t) modbus_destruct,
    .start = (app_method_t) modbus_start,
    .stop = (app_method_t) modbus_stop,
    .callback_signal = (device_callback_signal_t) modbus_signal,
    .callback_phase = (device_callback_phase_t)modbus_phase,
    .property_write = modbus_property_write,};
