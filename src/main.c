#include <libopencm3/stm32/adc.h>

/* semihosting Initializing */
extern void initialise_monitor_handles(void);

/* default values for CO_CANopenInit() */
#define NMT_CONTROL                                                                                                                        \
    CO_NMT_STARTUP_TO_OPERATIONAL                                                                                                          \
    | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION
#define FIRST_HB_TIME 501
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK true
#define OD_STATUS_BITS NULL

#define CO_GET_CNT(obj) OD_CNT_##obj
#define OD_GET(entry, index) OD_ENTRY_##entry

#include "main.h"
#include "semphr.h"
#include "storage/CO_storageAbstract.h"
#include "task.h"

#include "CO_application.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;      /* unused*/
    (void)pcTaskName; /* may be unused*/
    log_printf("System - Stack overflow! %s", pcTaskName);
    while (1) {
    }
}

void vApplicationMallocFailedHook(void) {
    log_printf("System - Malloc failed! %s");
    while (1) {
    }
}

typedef struct {
    uint8_t nodeId;   /* read from dip switches or nonvolatile memory, configurable
                         by LSS slave */
    uint16_t bitRate; /* read from dip switches or nonvolatile memory,
                         configurable by LSS slave */
} CO_config_communication_t;

/* Global variables and objects */
CO_t *CO = NULL;     /* CANopen object */
void *CANptr = NULL; /* CAN module address, user defined structure */

/* Initial values for*/
CO_config_communication_t CO_config_communication = {.nodeId = 4, .bitRate = 1000};

/* List of values to be stored in flash storage */
CO_storage_t CO_storage;
CO_storage_entry_t storageEntries[] = {
    /* Entirety of persistent section OD */
    {.addr = &OD_PERSIST_COMM, .len = sizeof(OD_PERSIST_COMM), .subIndexOD = 2, .attr = CO_storage_cmd | CO_storage_restore},

    /* Negotiated LSS settings */
    {.addr = &CO_config_communication,
     .len = sizeof(CO_config_communication_t),
     .subIndexOD = 0,
     .attr = CO_storage_cmd | CO_storage_restore | CO_storage_auto}};
uint8_t storageEntriesCount = sizeof(storageEntries) / sizeof(storageEntries[0]);

TaskHandle_t TaskCANOpenProcessingHandle;
TaskHandle_t TaskCANOpenMainlineHandle;

SemaphoreHandle_t TaskMainlineSemaphore = NULL;
SemaphoreHandle_t TaskProcessingSemaphore = NULL;

static CO_ReturnError_t initialize_memory(void) {
    uint32_t heapMemoryUsed = 0;
    /* Allocate memory */
    CO = CO_new(NULL, &heapMemoryUsed);
    CANptr = CO->CANmodule;
    if (CO == NULL) {
        log_printf("Error: Can't allocate memory\n");
        return CO_ERROR_OUT_OF_MEMORY;
    } else {
        if (heapMemoryUsed == 0) {
            log_printf("Config - Static memory\n");
        } else {
            log_printf("Config - On heap (%ubytes)\n", (unsigned int)heapMemoryUsed);
        }
    }
    return CO_ERROR_NO;
}

static CO_ReturnError_t initialize_storage(uint32_t *storageInitError) {
    log_printf("Config - Storage...\n");

    CO_ReturnError_t err;

    err = CO_storageAbstract_init(&CO_storage, CO->CANmodule, NULL, OD_ENTRY_H1010_storeParameters, OD_ENTRY_H1011_restoreDefaultParameters,
                                  storageEntries, storageEntriesCount, storageInitError);

    if (false && err != CO_ERROR_NO && err != CO_ERROR_DATA_CORRUPT) {
        error_printf("Error: Storage %d\n", (int)*storageInitError);
        return err;
    }

    return CO_ERROR_NO;
}

static bool_t LSSStoreConfigCallback(void *object, uint8_t id, uint16_t bitRate) {
    (void)object; /* unused*/
    log_printf("Config - Store LSS #%i @ %ikbps...\n", id, bitRate);
    CO_config_communication.bitRate = bitRate;
    CO_config_communication.nodeId = id;
    return CO_storageAbstract_store(&storageEntries[1], CO->CANmodule) == ODR_OK;
}

static CO_ReturnError_t initialize_communication(void) {
    CO_ReturnError_t err;

    log_printf("Config - Communication...\n");
    /* Enter CAN configuration. */
    CO->CANmodule->CANnormal = false;
    CO_CANsetConfigurationMode((void *)&CANptr);
    CO_CANmodule_disable(CO->CANmodule);

    /* Initialize CANopen driver */
    err = CO_CANinit(CO, CANptr, CO_config_communication.bitRate);
    if (false && err != CO_ERROR_NO) {
        error_printf("Error: CAN initialization failed: %d\n", err);
        return err;
    }

    /* Engage LSS configuration */
    CO_LSS_address_t lssAddress = {.identity = {.vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID,
                                                .productCode = OD_PERSIST_COMM.x1018_identity.productCode,
                                                .revisionNumber = OD_PERSIST_COMM.x1018_identity.revisionNumber,
                                                .serialNumber = OD_PERSIST_COMM.x1018_identity.serialNumber}};

    err = CO_LSSinit(CO, &lssAddress, &CO_config_communication.nodeId, &CO_config_communication.bitRate);
    CO_LSSslave_initCfgStoreCallback(CO->LSSslave, NULL, LSSStoreConfigCallback);

    if (err != CO_ERROR_NO) {
        error_printf("Error: LSS slave initialization failed: %d\n", err);
        return err;
    }

    return CO_ERROR_NO;
}

static void TaskCANOpenMainlineCallback(void *object) {
    (void)object; /* unused*/
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(TaskMainlineSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE || (CO_CONFIG_PDO)&CO_CONFIG_RPDO_ENABLE
static void TaskCANOpenProcessingCallback(void *object) {
    (void)object; /* unused*/
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(TaskProcessingSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif

static CO_ReturnError_t initialize_callbacks(CO_t *co) {
    /* Mainline tasks */
    if (CO_GET_CNT(EM) == 1) {
        CO_EM_initCallbackPre(co->em, NULL, TaskCANOpenMainlineCallback);
    }
    if (CO_GET_CNT(NMT) == 1) {
        CO_NMT_initCallbackPre(co->NMT, NULL, TaskCANOpenMainlineCallback);
    }
#if (CO_CONFIG_SRDO) & CO_CONFIG_SRDO_SRV_ENABLE
    for (int16_t i = 0; i < CO_GET_CNT(SRDO); i++) {
        CO_SRDO_initCallbackPre(co->SRDO[i], NULL, TaskCANOpenMainlineCallback);
    }
#endif
#if (CO_CONFIG_HB_CONS) & CO_CONFIG_HB_CONS_ENABLE
    if (CO_GET_CNT(HB_CONS) == 1) {
        CO_HBconsumer_initCallbackPre(co->HBCons, NULL, TaskCANOpenMainlineCallback);
    }
#endif
#if (CO_CONFIG_TIME) & CO_CONFIG_TIME_ENABLE
    if (CO_GET_CNT(TIME) == 1) {
        CO_TIME_initCallbackPre(co->TIME, NULL, TaskCANOpenMainlineCallback);
    }
#endif
#if (CO_CONFIG_SDO_CLI) & CO_CONFIG_SDO_CLI_ENABLE
    for (int16_t i = 0; i < CO_GET_CNT(SDO_CLI); i++) {
        CO_SDOclient_initCallbackPre(&co->SDOclient[i], NULL, TaskCANOpenMainlineCallback);
    }
#endif
    for (int16_t i = 0; i < CO_GET_CNT(SDO_SRV); i++) {
        CO_SDOserver_initCallbackPre(&co->SDOserver[i], NULL, TaskCANOpenMainlineCallback);
    }
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_MASTER
    CO_LSSmaster_initCallbackPre(co->LSSmaster, NULL, TaskCANOpenMainlineCallback);
#endif
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_SLAVE
    CO_LSSslave_initCallbackPre(co->LSSslave, NULL, TaskCANOpenMainlineCallback);
#endif

    /* Processing tasks */
#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
    if (CO_GET_CNT(SYNC) == 1) {
        CO_SYNC_initCallbackPre(co->SYNC, NULL, TaskCANOpenProcessingCallback);
    }
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
    for (int i = 0; i < CO_NO_RPDO; i++) {
        CO_RPDO_initCallbackPre(co->RPDO[i], (void *)0x01, TaskCANOpenProcessingCallback);
    }
#endif
    return CO_ERROR_NO;
}

static CO_ReturnError_t initialize_canopen(uint32_t *errInfo) {
    CO_ReturnError_t err;

    err = CO_CANopenInit(CO,                   /* CANopen object */
                         NULL,                 /* alternate NMT */
                         NULL,                 /* alternate em */
                         OD,                   /* Object dictionary */
                         OD_STATUS_BITS,       /* Optional OD_statusBit */
                         NMT_CONTROL,          /* CO_NMT_control_t */
                         FIRST_HB_TIME,        /* firstHBTime_ms */
                         SDO_SRV_TIMEOUT_TIME, /* SDOserverTimeoutTime_ms */
                         SDO_CLI_TIMEOUT_TIME, /* SDOclientTimeoutTime_ms */
                         SDO_CLI_BLOCK,        /* SDOclientBlockTransfer */
                         CO_config_communication.nodeId, errInfo);

    if (err == CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        return CO_ERROR_NO;
    }

    /* Execute optional external application code */
    app_communicationReset(CO);

    return CO_CANopenInitPDO(CO, CO->em, OD, CO_config_communication.nodeId, errInfo);
}

/* Task that configures devices and handles NMT commands */
static CO_NMT_reset_cmd_t CO_reset(CO_NMT_reset_cmd_t reset) {
    log_printf("System - Reset sequence %i...\n", CO_RESET_COMM);

    // Full device reset
    if (reset >= CO_RESET_COMM && CO != NULL) {
        log_printf("Config - Unloading...\n");
        CO_CANsetConfigurationMode((void *)&CANptr);
        CO_delete(CO);
    }

    // Reset communications: storage, canopen, communication
    if (reset == CO_RESET_COMM) {
        uint32_t initError = 0;
        uint32_t storageInitError = 0;
        CO_ReturnError_t err = initialize_memory();
#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
        if (err == CO_ERROR_NO) {
            err = initialize_storage(&storageInitError);
            if (storageInitError != 0) {
                initError = storageInitError;
            }
        }
        app_programConfigure(CO);
#endif
        if (err == CO_ERROR_NO)
            err = initialize_communication();
        if (err == CO_ERROR_NO)
            err = initialize_canopen(&initError);
        if (err == CO_ERROR_NO)
            err = app_programStart(&CO_config_communication.bitRate, &CO_config_communication.nodeId);
        if (err != CO_ERROR_NO) {
            if (err == CO_ERROR_OD_PARAMETERS) {
                log_printf("CANopen - Error in Object Dictionary entry 0x%X\n", initError);
            } else {
                log_printf("CANopen - Initialization failed: %d  0x%X\n", err, initError);
            }
            while (true) {
            };
        }

        initialize_callbacks(CO);

        log_printf("         #%i @ %ikbps\n", CO_config_communication.nodeId, CO_config_communication.bitRate);
        if (initError) {
            log_printf("CANopen - Error at entry:  0x%X\n", initError);
        }

        /* Emergency errors */
        if (!CO->nodeIdUnconfigured) {
            if (initError != 0) {
                CO_errorReport(CO->em, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_DATA_SET, initError);
            }
            if (storageInitError != 0) {
                CO_errorReport(CO->em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE, storageInitError);
            }
        }

        /* start CAN */
        CO_CANsetNormalMode(CO->CANmodule);
    }

    if (reset >= CO_RESET_APP) {
        /* Execute optional external application code */
        app_programEnd();
#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
        log_printf("CANopenNode - Autosaving...\n");
        CO_storageAbstract_auto_process(&CO_storage, true);
#endif
        log_printf("System - Resetting...\n");
        scb_reset_system();
    }

    log_printf("System - Running...\n");

    return CO_RESET_NOT;
}

/* Main task that does slower work */
static void TaskCANOpenMainline(void *pvParameters) {
    (void)pvParameters; /* unused */
    TaskMainlineSemaphore = xSemaphoreCreateBinary();
    TickType_t last_tick = xTaskGetTickCount();
    CO_NMT_reset_cmd_t reset = CO_reset(CO_RESET_COMM);

    while (true) {
        TickType_t current_tick = xTaskGetTickCount();
        uint32_t timeout = -1;
        uint32_t us_since_last_tick = TICKS_DIFF(current_tick, last_tick) * US_PER_TICK;

        reset = CO_process(CO, false, us_since_last_tick, &timeout);

        if (reset != CO_RESET_NOT) {
            reset = CO_reset(reset);
        }

        /* Execute optional external application code */
        app_programAsync(CO, us_since_last_tick, &timeout);

        last_tick = current_tick;
        xSemaphoreTake(TaskMainlineSemaphore, timeout / US_PER_TICK);
    }
}

/* Less frequent task that is invoked periodically */
static void TaskCANOpenProcessing(void *pvParameters) {
    (void)pvParameters; /* unused */
    TaskProcessingSemaphore = xSemaphoreCreateBinary();
    TickType_t last_tick = xTaskGetTickCount();
    TickType_t last_storage_tick = last_tick;

    while (true) {
        TickType_t current_tick = xTaskGetTickCount();
        uint32_t timeout = -1;
        uint32_t us_since_last_tick = TICKS_DIFF(current_tick, last_tick) * US_PER_TICK;

        /* Execute external application code */
        app_peripheralRead(CO, us_since_last_tick, &timeout);

        CO_LOCK_OD(co->CANmodule);
        if (!CO->nodeIdUnconfigured && CO->CANmodule->CANnormal) {
            bool_t syncWas = false;

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
            syncWas = CO_process_SYNC(CO, us_since_last_tick, &timeout);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
            CO_process_RPDO(CO, syncWas, us_since_last_tick, &timeout);
#endif
            /* Execute optional external application code */
            app_programRt(CO, us_since_last_tick, &timeout);
#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
            CO_process_TPDO(CO, syncWas, us_since_last_tick, &timeout);
#endif
            (void)syncWas;
        }
        CO_UNLOCK_OD(co->CANmodule);

#if (CO_CONFIG_STORAGE) & CO_CONFIG_STORAGE_ENABLE
        /* Thorttle autostorage to 1s */
        if ((current_tick - last_storage_tick) * US_PER_TICK > 1000000) {
            last_storage_tick = current_tick;
            log_printf("CANopenNode - Autosaving...\n");
            CO_storageAbstract_auto_process(&CO_storage, true);
        }
#endif

        /* Execute external application code */
        app_peripheralWrite(CO, us_since_last_tick, &timeout);

        last_tick = current_tick;
        xSemaphoreTake(TaskProcessingSemaphore, timeout / US_PER_TICK);
    }
}
/* main ***********************************************************************/
int main(void) {
#ifdef SEMIHOSTING
    initialise_monitor_handles(); /* This Function MUST come before the first log_printf() */
    log_printf("System - Starting...\n");
#endif
    xTaskCreate(TaskCANOpenMainline, "COMainline", 400, NULL, 2, &TaskCANOpenMainlineHandle);
    xTaskCreate(TaskCANOpenProcessing, "COProcessing", 200, NULL, 1, &TaskCANOpenProcessingHandle);

    log_printf("System - Starting tasks...\n");
    vTaskStartScheduler();
    for (;;)
        ;
    return 0;
}

#ifdef CO_CAN_INTERFACE
#ifdef STMF32F1
#if CO_CAN_RX_FIFO_INDEX == 0 || !defined(CO_CAN_RX_FIFO_INDEX)
void usb_hp_can_tx_isr(void) { CO_CANTxInterrupt(CO->CANmodule); }
void usb_lp_can_rx0_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#else
void usb_lp_can_rx1_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#endif
#else
#if CO_CAN_INTERFACE == CAN1
#if CO_CAN_RX_FIFO_INDEX == 0
void can1_tx_isr(void) { CO_CANTxInterrupt(CO->CANmodule); }
void can1_rx0_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#else
void can1_rx1_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#endif
#else
#if CO_CAN_RX_FIFO_INDEX == 0
void can2_tx_isr(void) { CO_CANTxInterrupt(CO->CANmodule); }
void can2_rx0_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#else
void can2_rx1_isr(void) { CO_CANRxInterrupt(CO->CANmodule); }
#endif
#endif
#endif
#endif
