#ifndef INC_DEBUG
#define INC_DEBUG

#ifdef DEBUG

#undef configASSERT
#define configASSERT(x) if (!x) { log_printf("Assert failed!");  __asm("BKPT #0\n"); }


#include <libopencm3/cm3/scb.h>
#include "FreeRTOS.h"
#include <task.h>
#include "core/types.h"

void vApplicationMallocFailedHook(void);

__attribute__((used)) void hardfault_discovery(struct scb_exception_stack_frame *frame);
__attribute__((naked)) void hardfault_handler(void);
#endif

#endif