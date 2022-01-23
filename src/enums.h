#ifndef INC_ENUMS
#define INC_ENUMS
#include "core/types.h"
// defined in ./src/core/app.h
char* get_app_signal_name (uint32_t v);
// defined in ./src/core/device.h
char* get_device_phase_name (uint32_t v);
// defined in ./src/core/device.h
char* get_device_type_name (uint32_t v);
// defined in ./src/core/event.h
char* get_app_event_type_name (uint32_t v);
// defined in ./src/core/event.h
char* get_app_event_status_name (uint32_t v);
// defined in ./src/core/task.h
char* get_app_task_signal_name (uint32_t v);
// defined in ./src/lib/vpool.h
char* get_vpool_trunc_name (uint32_t v);
// defined in ./src/storage/w25.h
char* get_w25_commands_name (uint32_t v);
// defined in ./src/transport/modbus.h
char* get__name (uint32_t v);
#endif