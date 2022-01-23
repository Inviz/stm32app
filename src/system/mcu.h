#ifndef INC_SYSTEM_MCU
#define INC_SYSTEM_MCU

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/rcc.h>
#include "core/app.h"

/* Start of autogenerated OD types */
/* 0x6000: System MCUnull */
typedef struct system_mcu_properties {
    uint8_t parameter_count;
    char family[8];
    char board_type[10];
    uint32_t storage_index;
    uint8_t phase;
    int16_t cpu_temperature;
    uint32_t startup_time;
} system_mcu_properties_t;
/* End of autogenerated OD types */

struct system_mcu {
    device_t *device;
    system_mcu_properties_t *properties;
    device_t *storage;
    const struct rcc_clock_scale *clock;
};

extern device_methods_t system_mcu_methods;

/* Start of autogenerated OD accessors */
#define SUBIDX_MCU_FAMILY 0x1
#define SUBIDX_MCU_BOARD_TYPE 0x2
#define SUBIDX_MCU_STORAGE_INDEX 0x3
#define SUBIDX_MCU_PHASE 0x4
#define SUBIDX_MCU_CPU_TEMPERATURE 0x5
#define SUBIDX_MCU_STARTUP_TIME 0x6

ODR_t system_mcu_set_family(system_mcu_t *mcu, char value); // 0x60XX01: mcu properties family
char system_mcu_get_family(system_mcu_t *mcu); // 0x60XX01: mcu properties family
ODR_t system_mcu_set_board_type(system_mcu_t *mcu, char value); // 0x60XX02: mcu properties board_type
char system_mcu_get_board_type(system_mcu_t *mcu); // 0x60XX02: mcu properties board_type
ODR_t system_mcu_set_storage_index(system_mcu_t *mcu, uint32_t value); // 0x60XX03: mcu properties storage_index
uint32_t system_mcu_get_storage_index(system_mcu_t *mcu); // 0x60XX03: mcu properties storage_index
ODR_t system_mcu_set_phase(system_mcu_t *mcu, uint8_t value); // 0x60XX04: mcu properties phase
uint8_t system_mcu_get_phase(system_mcu_t *mcu); // 0x60XX04: mcu properties phase
ODR_t system_mcu_set_cpu_temperature(system_mcu_t *mcu, int16_t value); // 0x60XX05: mcu properties cpu_temperature
int16_t system_mcu_get_cpu_temperature(system_mcu_t *mcu); // 0x60XX05: mcu properties cpu_temperature
ODR_t system_mcu_set_startup_time(system_mcu_t *mcu, uint32_t value); // 0x60XX06: mcu properties startup_time
uint32_t system_mcu_get_startup_time(system_mcu_t *mcu); // 0x60XX06: mcu properties startup_time
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
