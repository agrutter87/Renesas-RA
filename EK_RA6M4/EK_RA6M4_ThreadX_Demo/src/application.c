/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "application.h"
#include "tx_api.h"
#include <stdio.h>
#include <time.h>

/* Features */
#include "console.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
const feature_t g_features[] =
{
    {
        .feature_name = "Application",
        .feature_define = application_define,
        .feature_get_status = application_get_status
    },
    {
        .feature_name = "Console",
        .feature_define = console_define,
        .feature_get_status = console_get_status
    },
#if 0
    {
        .feature_name = "GUI - GUIX",
        .feature_define = gui_define,
        .feature_get_status = gui_get_status
    }
#endif
};

application_ctrl_t g_application_ctrl;
const application_cfg_t g_application_cfg =
{
 /* TX_THREAD creation arguments */
 .thread_name                = APPLICATION_THREAD_NAME,
 .thread_entry               = application_thread_entry,
 .thread_input               = 0,
 .thread_stack_size          = APPLICATION_THREAD_STACK_SIZE,
 .thread_priority            = APPLICATION_THREAD_PRIORITY,
 .thread_preempt_threshold   = APPLICATION_THREAD_PREEMPT_THRESHOLD,

 /* Application Memory */
 /* TX_BYTE_POOL creation arguments */
 .memory_byte_pool_name      = "Application Memory",
 .memory_byte_pool_size      = APPLICATION_MEMORY_MAX,

 /* Features */
 .p_features                 = g_features,
 .feature_count              = sizeof(g_features) / sizeof(g_features[0]),
};

const application_t g_application =
{ .p_ctrl = &g_application_ctrl, .p_cfg  = &g_application_cfg };

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/

/******************************************************************************
 * FUNCTION: application_define
 *****************************************************************************/
void application_define(TX_BYTE_POOL * p_memory_pool)
{
    UINT tx_err = TX_SUCCESS;

    SEGGER_RTT_printf(0, "Initializing application...\r\n");

    /* FOR MAIN THREAD: */
    /* Allocate the stack */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_application.p_ctrl->p_thread_stack,
                              g_application.p_cfg->thread_stack_size,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed application_define::tx_byte_allocate, tx_err = %d\r\n", tx_err);
    }

    /* Create the thread.  */
    tx_err = tx_thread_create(&g_application.p_ctrl->thread,
                              (CHAR *)g_application.p_cfg->thread_name,
                              g_application.p_cfg->thread_entry,
                              g_application.p_cfg->thread_input,
                              g_application.p_ctrl->p_thread_stack,
                              g_application.p_cfg->thread_stack_size,
                              g_application.p_cfg->thread_priority,
                              g_application.p_cfg->thread_preempt_threshold,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed application_define::tx_thread_create, tx_err = %d\r\n", tx_err);
    }
}

/******************************************************************************
 * FUNCTION: application_get_status
 *****************************************************************************/
void application_get_status(feature_status_t * p_status)
{
    *p_status = g_application.p_ctrl->status;
}

/******************************************************************************
 * FUNCTION: application_thread_entry
 *****************************************************************************/
void application_thread_entry(ULONG thread_input)
{
    TX_PARAMETER_NOT_USED(thread_input);
    static ULONG    prev_ticks  = 0;
    static time_t   prev_time   = 0;

    SEGGER_RTT_printf(0, "Started application\r\n");

    while(1)
    {
        time_t  current_time = 0;
        time(&current_time);
        time_t  elapsed_time = current_time - prev_time;

        ULONG   elapsed_ticks = tx_time_get() - prev_ticks;
        if(elapsed_ticks >= APPLICATION_THREAD_PERIOD)
        {
            prev_ticks += elapsed_ticks;
            prev_time += elapsed_time;
#if 0
            SEGGER_RTT_printf(0, "APPLICATION: Ticks = %010d, Time = %010d(%03d)\r\n",
                   (int)prev_ticks, (int)prev_time, (int)elapsed_time);
#endif
        }

        tx_thread_sleep(APPLICATION_THREAD_PERIOD - elapsed_ticks);
    }
}
