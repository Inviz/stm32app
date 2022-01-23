#include "enums.h"

char* get_app_signal_name (uint32_t v) {
switch (v) {
case 0: return "APP_SIGNAL_OK";
case 1: return "APP_SIGNAL_TIMEOUT";
case 2: return "APP_SIGNAL_TIMER";
case 3: return "APP_SIGNAL_DMA_ERROR";
case 4: return "APP_SIGNAL_DMA_TRANSFERRING";
case 5: return "APP_SIGNAL_DMA_IDLE";
case 6: return "APP_SIGNAL_RX_COMPLETE";
case 7: return "APP_SIGNAL_TX_COMPLETE";
case 8: return "APP_SIGNAL_CATCHUP";
case 9: return "APP_SIGNAL_RESCHEDULE";
case 10: return "APP_SIGNAL_INCOMING";
case 11: return "APP_SIGNAL_BUSY";
case 12: return "APP_SIGNAL_NOT_FOUND";
default: return "Unknown";
}
};

char* get_device_phase_name (uint32_t v) {
switch (v) {
case 0: return "DEVICE_ENABLED";
case 1: return "DEVICE_CONSTRUCTING";
case 2: return "DEVICE_LINKING";
case 3: return "DEVICE_STARTING";
case 4: return "DEVICE_CALIBRATING";
case 5: return "DEVICE_PREPARING";
case 6: return "DEVICE_RUNNING";
case 7: return "DEVICE_REQUESTING";
case 8: return "DEVICE_RESPONDING";
case 9: return "DEVICE_WORKING";
case 10: return "DEVICE_IDLE";
case 11: return "DEVICE_BUSY";
case 12: return "DEVICE_RESETTING";
case 13: return "DEVICE_PAUSING";
case 14: return "DEVICE_PAUSED";
case 15: return "DEVICE_RESUMING";
case 16: return "DEVICE_STOPPING";
case 17: return "DEVICE_STOPPED";
case 18: return "DEVICE_DISABLED";
case 19: return "DEVICE_DESTRUCTING";
default: return "Unknown";
}
};

char* get_device_type_name (uint32_t v) {
switch (v) {
case 12288: return "APP";
case 14336: return "DEVICE_CIRCUIT";
case 24576: return "SYSTEM_MCU";
case 24592: return "SYSTEM_CANOPEN";
case 24832: return "MODULE_TIMER";
case 24864: return "MODULE_ADC";
case 25088: return "TRANSPORT_CAN";
case 25120: return "TRANSPORT_SPI";
case 25152: return "TRANSPORT_USART";
case 25184: return "TRANSPORT_I2C";
case 25216: return "TRANSPORT_MODBUS";
case 25376: return "STORAGE_W25";
case 26624: return "INPUT_SENSOR";
case 26880: return "CONTROL_TOUCHSCREEN";
case 28672: return "SCREEN_EPAPER";
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

char* get_vpool_trunc_name (uint32_t v) {
switch (v) {
case 0: return "VPOOL_EXCLUDE";
case 1: return "VPOOL_INCLUDE";
default: return "Unknown";
}
};

char* get_w25_commands_name (uint32_t v) {
switch (v) {
case 144: return "W25_CMD_MANUF_DEVICE";
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

