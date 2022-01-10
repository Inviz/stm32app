/*
 * CAN module object for generic microcontroller.
 *
 * This file is a template for other microcontrollers.
 *
 * @file        CO_driver.c
 * @ingroup     CO_driver
 * @author      Yaroslaff Fedin
 * @copyright   2004 - 2020 Yaroslaff Fedin
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "301/CO_driver.h"
//#include <libopencm3/stm32/flash.h>

/******************************************************************************/
void CO_CANsetConfigurationMode(void *CANptr) {
    can_reset(((CO_CANmodule_t *) CANptr)->port);
    /* Put CAN module in configuration mode */
}

/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t *CANmodule) {
    /* Put CAN module in normal mode */

    CANmodule->CANnormal = true;
}

/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t *CANmodule, void *CANptr, CO_CANrx_t rxArray[], uint16_t rxSize, CO_CANtx_t txArray[],
                                   uint16_t txSize, uint16_t CANbitRate) {
    uint16_t i;

    /* verify arguments */
    if (CANmodule == NULL || rxArray == NULL || txArray == NULL) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    /* Configure object variables */
    CANmodule->CANptr = CANptr;
    CANmodule->rxArray = rxArray;
    CANmodule->rxSize = rxSize;
    CANmodule->txArray = txArray;
    CANmodule->txSize = txSize;
    CANmodule->CANerrorStatus = 0;
    CANmodule->CANnormal = false;
    CANmodule->useCANrxFilters = false; //(rxSize <= 32U) ? true : false;/* microcontroller dependent */
    CANmodule->bufferInhibitFlag = false;
    CANmodule->firstCANtxMessage = true;
    CANmodule->CANtxCount = 0U;
    CANmodule->errOld = 0U;

    for (i = 0U; i < rxSize; i++) {
        rxArray[i].ident = 0U;
        rxArray[i].mask = 0xFFFFU;
        rxArray[i].object = NULL;
        rxArray[i].CANrx_callback = NULL;
    }
    for (i = 0U; i < txSize; i++) {
        txArray[i].bufferFull = false;
    }


    /* Configure CAN module registers */
#ifdef CO_CAN_INTERFACE
    CANmodule->port = CO_CAN_INTERFACE;
    CANmodule->rxFifoIndex = CO_CAN_RX_FIFO_INDEX;
    CANmodule->sjw = CO_CAN_SJW;
    CANmodule->ph_seg1 = CO_CAN_PH_SEG1;
    CANmodule->ph_seg2 = CO_CAN_PH_SEG2;
    CANmodule->prop = CO_CAN_PROP;
    CANmodule->brp = CO_CAN_PH_BRP;
#endif

    if (can_init(CANmodule->port, /* Which port to use? */
                 false,              /* TTCM: Time triggered comm mode? */
                 true,               /* ABOM: Automatic bus-off management? */
                 true,               /* AWUM: Automatic wakeup mode? */
                 false,              /* NART: No automatic retransmission? */
                 false,              /* RFLM: Receive FIFO locked mode? */
                 false,              /* TXFP: Transmit FIFO priority? */
                 CANmodule->sjw << CAN_BTR_SJW_SHIFT, (CANmodule->ph_seg1 + CANmodule->prop - 1) << CAN_BTR_TS1_SHIFT,
                 (CANmodule->ph_seg2 - 1) << CAN_BTR_TS2_SHIFT, CANmodule->brp, false, false) != 0) {
        return CO_ERROR_INVALID_STATE;
    }

    /* CAN filter 0 init. */
    can_filter_id_mask_32bit_init(0,     /* Filter ID */
                                  0,     /* CAN ID */
                                  0,     /* CAN ID mask */
                                  0,     /* FIFO assignment (here: FIFO0) */
                                  true); /* Enable the filter. */

    nvic_enable_irq(CANmodule->port == CAN2 ? CANmodule->rxFifoIndex == 0 ? 64 : 65 : CANmodule->rxFifoIndex == 0 ? 20 : 21);
    can_enable_irq(CANmodule->port, CANmodule->rxFifoIndex == 0 ? CAN_IER_FMPIE0 : CAN_IER_FMPIE1);
    /* Configure CAN module hardware filters */
    if (CANmodule->useCANrxFilters) {
        /* CAN module filters are used, they will be configured with */
        /* CO_CANrxBufferInit() functions, called by separate CANopen */
        /* init functions. */
        /* Configure all masks so, that received message must match filter */
    } else {
        /* CAN module filters are not used, all messages with standard 11-bit */
        /* identifier will be received */
        /* Configure mask 0 so, that all messages with standard identifier are
         * accepted */
    }

    /* configure CAN interrupt registers */

    return CO_ERROR_NO;
}

/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t *CANmodule) {
    if (CANmodule != NULL) {
        CO_CANsetConfigurationMode(CANmodule->CANptr);
    }
}

/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index, uint16_t ident, uint16_t mask, bool_t rtr, void *object,
                                    void (*CANrx_callback)(void *object, void *message)) {
    CO_ReturnError_t ret = CO_ERROR_NO;

    if ((CANmodule != NULL) && (object != NULL) && (CANrx_callback != NULL) && (index < CANmodule->rxSize)) {
        /* buffer, which will be configured */
        CO_CANrx_t *buffer = &CANmodule->rxArray[index];

        /* Configure object variables */
        buffer->object = object;
        buffer->CANrx_callback = CANrx_callback;

        /* CAN identifier and CAN mask, bit aligned with CAN module. Different on
         * different microcontrollers. */
        buffer->ident = ident & 0x07FFU;
        if (rtr) {
            buffer->ident |= 0x0800U;
        }
        buffer->mask = (mask & 0x07FFU) | 0x0800U;

        /* Set CAN hardware module filter and mask. */
        if (CANmodule->useCANrxFilters) {
            //    can_filter_id_mask_32bit_init(
            //        index,     /* Filter ID */
            //        CANmodule->port,     /* CAN ID */
            //        buffer->ident,     /* CAN ID mask */
            //        buffer->mask,     /* FIFO assignment (here: FIFO0) */
            //        true); /* Enable the filter. */
        }
    } else {
        ret = CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return ret;
}

/******************************************************************************/
CO_CANtx_t *CO_CANtxBufferInit(CO_CANmodule_t *CANmodule, uint16_t index, uint16_t ident, bool_t rtr, uint8_t noOfBytes, bool_t syncFlag) {
    CO_CANtx_t *buffer = NULL;

    if ((CANmodule != NULL) && (index < CANmodule->txSize)) {
        /* get specific buffer */
        buffer = &CANmodule->txArray[index];

        /* CAN identifier, DLC and rtr, bit aligned with CAN module transmit buffer.
         * Microcontroller specific. */
        buffer->ident = ((uint32_t)ident & 0x07FFU) | ((uint32_t)(((uint32_t)noOfBytes & 0xFU) << 12U)) | ((uint32_t)(rtr ? 0x8000U : 0U));

        buffer->DLC = noOfBytes;
        buffer->rtr = rtr;
        buffer->bufferFull = false;
        buffer->syncFlag = syncFlag;
    }

    return buffer;
}

/******************************************************************************/
CO_ReturnError_t CO_CANsend(CO_CANmodule_t *CANmodule, CO_CANtx_t *buffer) {
    CO_ReturnError_t err = CO_ERROR_NO;

    /* Verify overflow */
    if (buffer->bufferFull) {
        if (!CANmodule->firstCANtxMessage) {
            /* don't set error, if bootup message is still on buffers */
            CANmodule->CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW;
        }
        err = CO_ERROR_TX_OVERFLOW;
    }

    CO_LOCK_CAN_SEND(CANmodule);
    /* Try sending message right away, if any of mailboxes are available */
    if (can_transmit(CANmodule->port, buffer->ident, false, buffer->rtr, buffer->DLC, buffer->data) != -1) {
        CANmodule->bufferInhibitFlag = buffer->syncFlag;
        // CANmodule->firstCANtxMessage = false;
    }
    /* if no buffer is free, message will be sent by interrupt */
    else {
        buffer->bufferFull = true;
        CANmodule->CANtxCount++;
        nvic_enable_irq(CANmodule->port == CAN2 ? 63 : 19);
        can_enable_irq(CANmodule->port, CAN_IER_TMEIE);
    }

    CO_UNLOCK_CAN_SEND(CANmodule);

    return err;
}

/******************************************************************************/
void CO_CANclearPendingSyncPDOs(CO_CANmodule_t *CANmodule) {
    uint32_t tpdoDeleted = 0U;

    CO_LOCK_CAN_SEND(CANmodule);
    /* Abort message from CAN module, if there is synchronous TPDO.
     * Take special care with this functionality. */
    if (/*messageIsOnCanBuffer && */ CANmodule->bufferInhibitFlag) {
        /* clear TXREQ */
        CANmodule->bufferInhibitFlag = false;
        tpdoDeleted = 1U;
    }
    /* delete also pending synchronous TPDOs in TX buffers */
    if (CANmodule->CANtxCount != 0U) {
        uint16_t i;
        CO_CANtx_t *buffer = &CANmodule->txArray[0];
        for (i = CANmodule->txSize; i > 0U; i--) {
            if (buffer->bufferFull) {
                if (buffer->syncFlag) {
                    buffer->bufferFull = false;
                    CANmodule->CANtxCount--;
                    tpdoDeleted = 2U;
                }
            }
            buffer++;
        }
    }
    CO_UNLOCK_CAN_SEND(CANmodule);

    if (tpdoDeleted != 0U) {
        CANmodule->CANerrorStatus |= CO_CAN_ERRTX_PDO_LATE;
    }
}

/******************************************************************************/
/* Get error counters from the module. If necessary, function may use
 * different way to determine errors. */
static uint16_t rxErrors = 0, txErrors = 0, overflow = 0;

void CO_CANmodule_process(CO_CANmodule_t *CANmodule) {
    uint32_t err;

    err = ((uint32_t)txErrors << 16) | ((uint32_t)rxErrors << 8) | overflow;

    if (CANmodule->errOld != err) {
        uint16_t status = CANmodule->CANerrorStatus;

        CANmodule->errOld = err;

        if (txErrors >= 256U) {
            /* bus off */
            status |= CO_CAN_ERRTX_BUS_OFF;
        } else {
            /* recalculate CANerrorStatus, first clear some flags */
            status &=
                0xFFFF ^ (CO_CAN_ERRTX_BUS_OFF | CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE | CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE);

            /* rx bus warning or passive */
            if (rxErrors >= 128) {
                status |= CO_CAN_ERRRX_WARNING | CO_CAN_ERRRX_PASSIVE;
            } else if (rxErrors >= 96) {
                status |= CO_CAN_ERRRX_WARNING;
            }

            /* tx bus warning or passive */
            if (txErrors >= 128) {
                status |= CO_CAN_ERRTX_WARNING | CO_CAN_ERRTX_PASSIVE;
            } else if (rxErrors >= 96) {
                status |= CO_CAN_ERRTX_WARNING;
            }

            /* if not tx passive clear also overflow */
            if ((status & CO_CAN_ERRTX_PASSIVE) == 0) {
                status &= 0xFFFF ^ CO_CAN_ERRTX_OVERFLOW;
            }
        }

        if (overflow != 0) {
            /* CAN RX bus overflow */
            status |= CO_CAN_ERRRX_OVERFLOW;
        }

        CANmodule->CANerrorStatus = status;
    }
}

/* Match can message against buffers and invoke callback */
void CO_CANRxInterrupt(CO_CANmodule_t *CANmodule) {
    bool ext, rtr;
    uint8_t fmi;
    CO_CANrxMsg_t rcvMsg;      /* pointer to received message in CAN module */
    uint16_t index;            /* index of received message */
    CO_CANrx_t *buffer = NULL; /* receive message buffer from CO_CANmodule_t object. */
    bool_t msgMatched = false;
    uint16_t rcvMsgIdent; /* identifier of the received message */

    can_receive(CANmodule->port, CANmodule->rxFifoIndex, true, &rcvMsg.ident, &ext, &rtr, &fmi, &rcvMsg.DLC, rcvMsg.data, NULL);

    rcvMsgIdent = rcvMsg.ident;
    if (rtr)
        rcvMsgIdent |= 0x0800;

    if (CANmodule->useCANrxFilters) {
        /* CAN module filters are used. Message with known 11-bit identifier has */
        /* been received */
        index = 0; /* get index of the received message here. Or something similar */
        if (index < CANmodule->rxSize) {
            buffer = &CANmodule->rxArray[index];
            /* verify also RTR */
            if (((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U) {
                msgMatched = true;
            }
        }
    } else {
        /* CAN module filters are not used, message with any standard 11-bit
         * identifier */
        /* has been received. Search rxArray form CANmodule for the same CAN-ID. */
        buffer = &CANmodule->rxArray[0];
        for (index = CANmodule->rxSize; index > 0U; index--) {
            if (((rcvMsgIdent ^ buffer->ident) & buffer->mask) == 0U) {
                msgMatched = true;
                break;
            }
            buffer++;
        }
    }

    /* Call specific function, which will process the message */
    if (msgMatched && (buffer != NULL) && (buffer->CANrx_callback != NULL)) {
        /*printf("can_receive * ident: 0x%03X, DLC: %d 0x[%02X %02X %02X %02X %02X %02X %02X %02X] flags: 0x%08X idx: %d, cb: %d",
                 rcvMsg.ident, rcvMsg.DLC, rcvMsg.data[0], rcvMsg.data[1], rcvMsg.data[2], rcvMsg.data[3],
                 rcvMsg.data[4], rcvMsg.data[5], rcvMsg.data[6], rcvMsg.data[7], 0, CANmodule->rxSize - index,
                 (int)(void *)buffer->CANrx_callback);*/

        buffer->CANrx_callback(buffer->object, &rcvMsg);
    }
}
/* Interrupt called when message was not sent immediately due to
    full TX mailboxes, which now became free.
    CanOpenNode allows for messages to be superseeded by updated
    values by buffering them by type */
void CO_CANTxInterrupt(CO_CANmodule_t *CANmodule) {
    /* First CAN message (bootup) was sent successfully */
    CANmodule->firstCANtxMessage = false;
    /* clear flag from previous message */
    CANmodule->bufferInhibitFlag = false;
    /* Are there any new messages waiting to be send */
    if (CANmodule->CANtxCount != 0U) {
        /* search through whole array of pointers to transmit message buffers. */
        for (uint16_t i = 0; i < CANmodule->txSize; i++) {
            /* find unset messages */
            CO_CANtx_t *buffer = &CANmodule->txArray[i];
            if (!buffer->bufferFull)
                continue;

            /* if all transmit mailboxes are full, leave message unsent until next
             * interrupt */
            if (can_transmit(CANmodule->port, buffer->ident, false, false, buffer->DLC, buffer->data) == -1)
                return;

            buffer->bufferFull = false;
            CANmodule->CANtxCount--;
            CANmodule->bufferInhibitFlag = buffer->syncFlag;
        }
    };

    can_disable_irq(CANmodule->port, CAN_IER_TMEIE);
}