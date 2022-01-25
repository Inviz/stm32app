#include "enums.h"
char* get_actor_phase_name (uint32_t v) {
switch (v) {
case 0: return "ACTOR_ENABLED";
case 1: return "ACTOR_CONSTRUCTING";
case 2: return "ACTOR_LINKING";
case 3: return "ACTOR_STARTING";
case 4: return "ACTOR_CALIBRATING";
case 5: return "ACTOR_PREPARING";
case 6: return "ACTOR_RUNNING";
case 7: return "ACTOR_REQUESTING";
case 8: return "ACTOR_RESPONDING";
case 9: return "ACTOR_WORKING";
case 10: return "ACTOR_IDLE";
case 11: return "ACTOR_BUSY";
case 12: return "ACTOR_RESETTING";
case 13: return "ACTOR_PAUSING";
case 14: return "ACTOR_PAUSED";
case 15: return "ACTOR_RESUMING";
case 16: return "ACTOR_STOPPING";
case 17: return "ACTOR_STOPPED";
case 18: return "ACTOR_DISABLED";
case 19: return "ACTOR_DESTRUCTING";
default: return "Unknown";
}
};

char* get_actor_type_name (uint32_t v) {
switch (v) {
case 12288: return "APP";
case 16384: return "DEVICE_CIRCUIT";
case 24576: return "SYSTEM_MCU";
case 24608: return "SYSTEM_CANOPEN";
case 24832: return "MODULE_TIMER";
case 25088: return "TRANSPORT_CAN";
case 25120: return "TRANSPORT_SPI";
case 25152: return "TRANSPORT_USART";
case 25184: return "TRANSPORT_I2C";
case 25216: return "TRANSPORT_MODBUS";
case 25344: return "MODULE_ADC";
case 28928: return "STORAGE_W25";
case 32768: return "INPUT_SENSOR";
case 33024: return "CONTROL_TOUCHSCREEN";
case 36864: return "SCREEN_EPAPER";
case 38912: return "INDICATOR_LED";
default: return "Unknown";
}
};

char* get_app_signal_name (uint32_t v) {
switch (v) {
case 0: return "APP_SIGNAL_OK";
case 1: return "APP_SIGNAL_FAILURE";
case 2: return "APP_SIGNAL_TIMEOUT";
case 3: return "APP_SIGNAL_TIMER";
case 4: return "APP_SIGNAL_DMA_ERROR";
case 5: return "APP_SIGNAL_DMA_TRANSFERRING";
case 6: return "APP_SIGNAL_DMA_IDLE";
case 7: return "APP_SIGNAL_RX_COMPLETE";
case 8: return "APP_SIGNAL_TX_COMPLETE";
case 9: return "APP_SIGNAL_CATCHUP";
case 10: return "APP_SIGNAL_RESCHEDULE";
case 11: return "APP_SIGNAL_INCOMING";
case 12: return "APP_SIGNAL_BUSY";
case 13: return "APP_SIGNAL_NOT_FOUND";
case 14: return "APP_SIGNAL_UNCONFIGURED";
case 15: return "APP_SIGNAL_OUT_OF_MEMORY";
default: return "Unknown";
}
};

char* get_app_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "CORE_APP_TIMER_INDEX";
case 2: return "CORE_APP_STORAGE_INDEX";
case 3: return "CORE_APP_MCU_INDEX";
case 4: return "CORE_APP_CANOPEN_INDEX";
case 5: return "CORE_APP_PHASE";
default: return "Unknown";
}
};

char* get_app_event_type_name (uint32_t v) {
switch (v) {
case 32: return "APP_EVENT_THREAD_START";
case 33: return "APP_EVENT_THREAD_STOP";
case 34: return "APP_EVENT_THREAD_ALARM";
case 0: return "APP_EVENT_IDLE";
case 1: return "APP_EVENT_READ";
case 2: return "APP_EVENT_WRITE";
case 3: return "APP_EVENT_ERASE";
case 4: return "APP_EVENT_RESPONSE";
case 5: return "APP_EVENT_LOCK";
case 6: return "APP_EVENT_UNLOCK";
case 7: return "APP_EVENT_INTROSPECTION";
case 8: return "APP_EVENT_ENABLE";
case 9: return "APP_EVENT_DISABLE";
default: return "Unknown";
}
};

char* get_app_event_status_name (uint32_t v) {
switch (v) {
case 0: return "APP_EVENT_WAITING";
case 1: return "APP_EVENT_RECEIVED";
case 2: return "APP_EVENT_ADDRESSED";
case 3: return "APP_EVENT_HANDLED";
case 4: return "APP_EVENT_DEFERRED";
default: return "Unknown";
}
};

char* get_app_task_signal_name (uint32_t v) {
switch (v) {
case 0: return "APP_TASK_CONTINUE";
case 1: return "APP_TASK_COMPLETE";
case 2: return "APP_TASK_RETRY";
case 3: return "APP_TASK_HALT";
case 4: return "APP_TASK_STEP_RETRY";
case 5: return "APP_TASK_STEP_WAIT";
case 6: return "APP_TASK_STEP_COMPLETE";
case 7: return "APP_TASK_STEP_HALT";
default: return "Unknown";
}
};

char* get_actor_circuit_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "DEVICE_CIRCUIT_PORT";
case 2: return "DEVICE_CIRCUIT_PIN";
case 3: return "DEVICE_CIRCUIT_LIMIT_CURRENT";
case 4: return "DEVICE_CIRCUIT_LIMIT_VOLTAGE";
case 5: return "DEVICE_CIRCUIT_PSU_INDEX";
case 6: return "DEVICE_CIRCUIT_SENSOR_INDEX";
case 7: return "DEVICE_CIRCUIT_PHASE";
case 8: return "DEVICE_CIRCUIT_DUTY_CYCLE";
case 9: return "DEVICE_CIRCUIT_CURRENT";
case 10: return "DEVICE_CIRCUIT_VOLTAGE";
case 11: return "DEVICE_CIRCUIT_CONSUMERS";
default: return "Unknown";
}
};

char* get_indicator_led_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "INDICATOR_LED_PORT";
case 2: return "INDICATOR_LED_PIN";
case 3: return "INDICATOR_LED_PHASE";
case 4: return "INDICATOR_LED_DUTY_CYCLE";
default: return "Unknown";
}
};

char* get_input_sensor_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "INPUT_SENSOR_DISABLED";
case 2: return "INPUT_SENSOR_PORT";
case 3: return "INPUT_SENSOR_PIN";
case 4: return "INPUT_SENSOR_ADC_INDEX";
case 5: return "INPUT_SENSOR_ADC_CHANNEL";
case 6: return "INPUT_SENSOR_PHASE";
default: return "Unknown";
}
};

char* get_vpool_trunc_name (uint32_t v) {
switch (v) {
case 0: return "VPOOL_EXCLUDE";
case 1: return "VPOOL_INCLUDE";
default: return "Unknown";
}
};

char* get_module_adc_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "MODULE_ADC_INTERVAL";
case 2: return "MODULE_ADC_SAMPLE_COUNT_PER_CHANNEL";
case 3: return "MODULE_ADC_DMA_UNIT";
case 4: return "MODULE_ADC_DMA_STREAM";
case 5: return "MODULE_ADC_DMA_CHANNEL";
case 6: return "MODULE_ADC_PHASE";
default: return "Unknown";
}
};

char* get_module_timer_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "MODULE_TIMER_PRESCALER";
case 2: return "MODULE_TIMER_INITIAL_SUBSCRIPTIONS_COUNT";
case 3: return "MODULE_TIMER_PERIOD";
case 4: return "MODULE_TIMER_FREQUENCY";
case 5: return "MODULE_TIMER_PHASE";
default: return "Unknown";
}
};

char* get_screen_epaper_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "SCREEN_EPAPER_SPI_INDEX";
case 2: return "SCREEN_EPAPER_DC_PORT";
case 3: return "SCREEN_EPAPER_DC_PIN";
case 4: return "SCREEN_EPAPER_CS_PORT";
case 5: return "SCREEN_EPAPER_CS_PIN";
case 6: return "SCREEN_EPAPER_BUSY_PORT";
case 7: return "SCREEN_EPAPER_BUSY_PIN";
case 8: return "SCREEN_EPAPER_RESET_PORT";
case 9: return "SCREEN_EPAPER_RESET_PIN";
case 10: return "SCREEN_EPAPER_WIDTH";
case 11: return "SCREEN_EPAPER_HEIGHT";
case 12: return "SCREEN_EPAPER_MODE";
case 13: return "SCREEN_EPAPER_PHASE";
default: return "Unknown";
}
};

char* get_w25_commands_name (uint32_t v) {
switch (v) {
case 144: return "W25_CMD_MANUF_ACTOR";
case 159: return "W25_CMD_JEDEC_ID";
case 6: return "W25_CMD_UNLOCK";
case 4: return "W25_CMD_LOCK";
case 5: return "W25_CMD_READ_SR1";
case 53: return "W25_CMD_READ_SR2";
case 199: return "W25_CMD_CHIP_ERASE";
case 3: return "W25_CMD_READ_DATA";
case 11: return "W25_CMD_FAST_READ";
case 2: return "W25_CMD_WRITE_DATA";
case 75: return "W25_CMD_READ_UID";
case 171: return "W25_CMD_PWR_ON";
case 185: return "W25_CMD_PWR_OFF";
case 32: return "W25_CMD_ERA_SECTOR";
case 82: return "W25_CMD_ERA_32K";
case 216: return "W25_CMD_ERA_64K";
default: return "Unknown";
}
};

char* get_storage_w25_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "STORAGE_W25_SPI_INDEX";
case 2: return "STORAGE_W25_PAGE_SIZE";
case 3: return "STORAGE_W25_SIZE";
case 4: return "STORAGE_W25_PHASE";
default: return "Unknown";
}
};

char* get_system_canopen_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "SYSTEM_CANOPEN_CAN_INDEX";
case 2: return "SYSTEM_CANOPEN_CAN_FIFO_INDEX";
case 3: return "SYSTEM_CANOPEN_GREEN_LED_INDEX";
case 4: return "SYSTEM_CANOPEN_RED_LED_INDEX";
case 5: return "SYSTEM_CANOPEN_FIRST_HB_TIME";
case 6: return "SYSTEM_CANOPEN_SDO_SERVER_TIMEOUT";
case 7: return "SYSTEM_CANOPEN_SDO_CLIENT_TIMEOUT";
case 8: return "SYSTEM_CANOPEN_PHASE";
case 9: return "SYSTEM_CANOPEN_NODE_ID";
case 10: return "SYSTEM_CANOPEN_BITRATE";
default: return "Unknown";
}
};

char* get_system_mcu_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "SYSTEM_MCU_FAMILY";
case 2: return "SYSTEM_MCU_BOARD_TYPE";
case 3: return "SYSTEM_MCU_STORAGE_INDEX";
case 4: return "SYSTEM_MCU_PHASE";
case 5: return "SYSTEM_MCU_CPU_TEMPERATURE";
case 6: return "SYSTEM_MCU_STARTUP_TIME";
default: return "Unknown";
}
};

char* get_transport_can_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_CAN_TX_PORT";
case 2: return "TRANSPORT_CAN_TX_PIN";
case 3: return "TRANSPORT_CAN_RX_PORT";
case 4: return "TRANSPORT_CAN_RX_PIN";
case 5: return "TRANSPORT_CAN_BITRATE";
case 6: return "TRANSPORT_CAN_BRP";
case 7: return "TRANSPORT_CAN_SJW";
case 8: return "TRANSPORT_CAN_PROP";
case 9: return "TRANSPORT_CAN_PH_SEG1";
case 10: return "TRANSPORT_CAN_PH_SEG2";
case 11: return "TRANSPORT_CAN_PHASE";
default: return "Unknown";
}
};

char* get_transport_i2c_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_I2C_DMA_RX_UNIT";
case 2: return "TRANSPORT_I2C_DMA_RX_STREAM";
case 3: return "TRANSPORT_I2C_DMA_RX_CHANNEL";
case 4: return "TRANSPORT_I2C_DMA_RX_BUFFER_SIZE";
case 5: return "TRANSPORT_I2C_DMA_TX_UNIT";
case 6: return "TRANSPORT_I2C_DMA_TX_STREAM";
case 7: return "TRANSPORT_I2C_DMA_TX_CHANNEL";
case 8: return "TRANSPORT_I2C_BAUDRATE";
case 9: return "TRANSPORT_I2C_DATABITS";
case 10: return "TRANSPORT_I2C_PHASE";
default: return "Unknown";
}
};

char* get__name (uint32_t v) {
switch (v) {
case 1: return "MODBUS_READ_COIL_STATUS";
case 2: return "MODBUS_READ_DISCRETE_INPUTS";
case 3: return "MODBUS_READ_HOLDING_REGISTERS";
case 4: return "MODBUS_READ_INPUT_REGISTERS";
case 5: return "MODBUS_WRITE_SINGLE_COIL";
case 6: return "MODBUS_WRITE_SINGLE_REGISTER";
case 15: return "MODBUS_WRITE_MULTIPLE_COILS";
case 16: return "MODBUS_WRITE_MULTIPLE_REGISTERS";
default: return "Unknown";
}
};

char* get_transport_modbus_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_MODBUS_USART_INDEX";
case 2: return "TRANSPORT_MODBUS_RTS_PORT";
case 3: return "TRANSPORT_MODBUS_RTS_PIN";
case 4: return "TRANSPORT_MODBUS_SLAVE_ADDRESS";
case 5: return "TRANSPORT_MODBUS_RX_BUFFER_SIZE";
case 6: return "TRANSPORT_MODBUS_TIMEOUT";
case 7: return "TRANSPORT_MODBUS_PHASE";
default: return "Unknown";
}
};

char* get_transport_spi_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_SPI_IS_SLAVE";
case 2: return "TRANSPORT_SPI_SOFTWARE_SS_CONTROL";
case 3: return "TRANSPORT_SPI_MODE";
case 4: return "TRANSPORT_SPI_DMA_RX_UNIT";
case 5: return "TRANSPORT_SPI_DMA_RX_STREAM";
case 6: return "TRANSPORT_SPI_DMA_RX_CHANNEL";
case 7: return "TRANSPORT_SPI_DMA_RX_IDLE_TIMEOUT";
case 8: return "TRANSPORT_SPI_RX_BUFFER_SIZE";
case 9: return "TRANSPORT_SPI_RX_POOL_MAX_SIZE";
case 10: return "TRANSPORT_SPI_RX_POOL_INITIAL_SIZE";
case 11: return "TRANSPORT_SPI_RX_POOL_BLOCK_SIZE";
case 12: return "TRANSPORT_SPI_DMA_TX_UNIT";
case 13: return "TRANSPORT_SPI_DMA_TX_STREAM";
case 14: return "TRANSPORT_SPI_DMA_TX_CHANNEL";
case 15: return "TRANSPORT_SPI_AF_INDEX";
case 16: return "TRANSPORT_SPI_SS_PORT";
case 17: return "TRANSPORT_SPI_SS_PIN";
case 18: return "TRANSPORT_SPI_SCK_PORT";
case 19: return "TRANSPORT_SPI_SCK_PIN";
case 20: return "TRANSPORT_SPI_MISO_PORT";
case 21: return "TRANSPORT_SPI_MISO_PIN";
case 22: return "TRANSPORT_SPI_MOSI_PORT";
case 23: return "TRANSPORT_SPI_MOSI_PIN";
case 24: return "TRANSPORT_SPI_PHASE";
default: return "Unknown";
}
};

char* get_transport_usart_properties_properties_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_USART_DMA_RX_UNIT";
case 2: return "TRANSPORT_USART_DMA_RX_STREAM";
case 3: return "TRANSPORT_USART_DMA_RX_CHANNEL";
case 4: return "TRANSPORT_USART_DMA_RX_BUFFER_SIZE";
case 5: return "TRANSPORT_USART_DMA_TX_UNIT";
case 6: return "TRANSPORT_USART_DMA_TX_STREAM";
case 7: return "TRANSPORT_USART_DMA_TX_CHANNEL";
case 8: return "TRANSPORT_USART_BAUDRATE";
case 9: return "TRANSPORT_USART_DATABITS";
case 10: return "TRANSPORT_USART_PHASE";
default: return "Unknown";
}
};

