/***********************************************************************************************************************
 * Copyright [2020-2022] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics America Inc. and may only be used with products
 * of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.  Renesas products are
 * sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for the selection and use
 * of Renesas products and Renesas assumes no liability.  No license, express or implied, to any intellectual property
 * right is granted by Renesas. This software is protected under all applicable laws, including copyright laws. Renesas
 * reserves the right to change or discontinue this software and/or this documentation. THE SOFTWARE AND DOCUMENTATION
 * IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT
 * PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE OR
 * DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.  TO THE MAXIMUM
 * EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR DOCUMENTATION
 * (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING,
 * WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY LOST PROFITS,
 * OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

#include "hal_data.h"
#include "application.h"

void R_BSP_WarmStart(bsp_warm_start_event_t event);
void tx_application_define_user(void *first_unused_memory);

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart (bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open(&g_ioport_ctrl, g_ioport.p_cfg);
    }
}

/******************************************************************************
 * FUNCTION: tx_application_define_user()
 *****************************************************************************/
void tx_application_define_user(void *first_unused_memory)
{
    UINT tx_err = TX_SUCCESS;

    /* Create a byte memory pool from which to allocate the thread stacks. */
    tx_err = tx_byte_pool_create(&g_application.p_ctrl->memory_byte_pool,
                                 (CHAR *)g_application.p_cfg->memory_byte_pool_name,
                                 first_unused_memory,
                                 (APPLICATION_MEMORY_MAX - (ULONG)first_unused_memory));

    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed tx_application_define_user::tx_byte_pool_create, tx_err = %d\r\n", tx_err);
    }
    else
    {
        for(ULONG feature_num = 0; feature_num < g_application.p_cfg->feature_count; feature_num++)
        {
            /* Allocate the memory for the event queue for this feature */
            tx_err = tx_byte_allocate(&g_application.p_ctrl->memory_byte_pool,
                                      (VOID **) &g_application.p_cfg->p_features[feature_num].p_event_queue_memory,
                                      EVENT_QUEUE_MEMORY_MAX(g_application.p_cfg->p_features[feature_num].event_queue_message_count),
                                      TX_NO_WAIT);
            if(TX_SUCCESS != tx_err)
            {
                SEGGER_RTT_printf(0, "Failed tx_application_define_user::tx_byte_allocate (p_event_queue_memory), tx_err = %d\r\n", tx_err);
            }

            /* Create the event queue for this feature */
            tx_err = tx_queue_create(&g_application.p_cfg->p_features[feature_num].event_queue,
                                     (CHAR *)g_application.p_cfg->p_features[feature_num].event_queue_name,
                                     EVENT_QUEUE_MESSAGE_SIZE,
                                     g_application.p_cfg->p_features[feature_num].p_event_queue_memory,
                                     EVENT_QUEUE_MEMORY_MAX(g_application.p_cfg->p_features[feature_num].event_queue_message_count));

            g_application.p_cfg->p_features[feature_num].feature_define(&g_application.p_ctrl->memory_byte_pool, &g_application.p_cfg->p_features[feature_num].event_queue);
        }
    }
}
