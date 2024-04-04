/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "application.h"
#include "tx_api.h"
#include <stdio.h>
#include <time.h>

/* Features */
#include "console.h"
#include "uart_manager.h"
#include "pmodesp32_bt.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
static feature_t g_features[] =
{
    {
        .feature_name = "Application",
        .feature_define = application_define,
        .feature_get_status = application_get_status,
        .event_queue = { 0 },
        .p_event_queue_memory = NULL,
        .event_queue_name = "Application Event Queue",
        .event_queue_message_count = 10
    },
    {
        .feature_name = "Console",
        .feature_define = console_define,
        .feature_get_status = console_get_status,
        .event_queue = { 0 },
        .p_event_queue_memory = NULL,
        .event_queue_name = "Console Event Queue",
        .event_queue_message_count = 10
    },
    {
        .feature_name = "UART Manager",
        .feature_define = uart_manager_define,
        .feature_get_status = uart_manager_get_status,
        .event_queue = { 0 },
        .p_event_queue_memory = NULL,
        .event_queue_name = "UART Manager Event Queue",
        .event_queue_message_count = 10
    },
    {
        .feature_name = "PmodESP32 Bluetooth",
        .feature_define = pmodesp32_bt_define,
        .feature_get_status = pmodesp32_bt_get_status,
        .event_queue = { 0 },
        .p_event_queue_memory = NULL,
        .event_queue_name = "PmodESP32 Bluetooth Event Queue",
        .event_queue_message_count = 64
    },
#if 0
    {
        .feature_name = "GUI - GUIX",
        .feature_define = gui_define,
        .feature_get_status = gui_get_status
        .event_queue = { 0 },
    }
#endif
};

application_ctrl_t g_application_ctrl;
const application_cfg_t g_application_cfg =
{
 /* TX_THREAD creation arguments */
 .thread_name               = APPLICATION_THREAD_NAME,
 .thread_entry              = application_thread_entry,
 .thread_input              = 0,
 .thread_stack_size         = APPLICATION_THREAD_STACK_SIZE,
 .thread_priority           = APPLICATION_THREAD_PRIORITY,
 .thread_preempt_threshold  = APPLICATION_THREAD_PREEMPT_THRESHOLD,

 /* Thread monitor timer creation arguments */
 .thread_monitor_timer_name = APPLICATION_THREAD_MONITOR_TIMER_NAME,

 /* Application Memory */
 /* TX_BYTE_POOL creation arguments */
 .memory_byte_pool_name     = "Application Memory",

 /* Features */
 .p_features                = g_features,
 .feature_count             = sizeof(g_features) / sizeof(g_features[0]),
};

const application_t g_application =
{ .p_ctrl = &g_application_ctrl, .p_cfg  = &g_application_cfg };

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
VOID application_feature_monitor_timer_callback(ULONG id);

/******************************************************************************
 * FUNCTION: application_define
 *****************************************************************************/
void application_define(TX_BYTE_POOL * p_memory_pool, TX_QUEUE * p_event_queue)
{
    UINT        tx_err          = TX_SUCCESS;
    event_t     event_data      = { 0 };

    SEGGER_RTT_printf(0, "Initializing application...\r\n");

    g_application.p_ctrl->p_event_queue = p_event_queue;

    /* Allocate the stack */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_application.p_ctrl->p_thread_stack,
                              g_application.p_cfg->thread_stack_size,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed application_define::tx_byte_allocate (p_thread_stack), tx_err = %d\r\n", tx_err);
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

    /* Send the initialize event */
    event_data.event_type = APPLICATION_EVENT_INIT;

    tx_err = tx_queue_send(g_application.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed application_define::tx_queue_send, tx_err = %d\r\n", tx_err);
    }

    /* Create the thread monitor timer */
    tx_err = tx_timer_create(&g_application.p_ctrl->thread_monitor_event_timer,
                             (CHAR *)g_application.p_cfg->thread_monitor_timer_name,
                             application_feature_monitor_timer_callback,
                             0, APPLICATION_THREAD_MONITOR_TICKS, APPLICATION_THREAD_MONITOR_TICKS, TX_AUTO_ACTIVATE);
}

/******************************************************************************
 * FUNCTION: application_get_status
 *****************************************************************************/
void application_get_status(feature_status_t * p_status)
{
    UINT tx_err = TX_SUCCESS;

    if(g_application.p_ctrl->status.pp_thread == 0)
    {
        g_application.p_ctrl->status.thread_count = 1;

        tx_err = tx_byte_allocate(&g_application.p_ctrl->memory_byte_pool,
                                  (VOID **)&g_application.p_ctrl->status.pp_thread,
                                  sizeof(TX_THREAD *) * g_application.p_ctrl->status.thread_count,
                                  TX_NO_WAIT);
    }

    if(TX_SUCCESS == tx_err)
    {
        g_application.p_ctrl->status.pp_thread[0] = &g_application.p_ctrl->thread;
        *p_status = g_application.p_ctrl->status;
    }
}

/******************************************************************************
 * FUNCTION: application_thread_entry
 *****************************************************************************/
void application_thread_entry(ULONG thread_input)
{
    TX_PARAMETER_NOT_USED(thread_input);
    static ULONG    prev_ticks      = 0;
    static time_t   prev_time       = 0;
    UINT            tx_err          = TX_SUCCESS;
    event_t         event_data      = { 0 };

    while(1)
    {
        tx_queue_receive(g_application.p_ctrl->p_event_queue, &event_data, TX_WAIT_FOREVER);

        //SEGGER_RTT_printf(0, "APPLICATION EVENT %d\r\n", event_data.event_type);
        switch(event_data.event_type)
        {
            case APPLICATION_EVENT_INIT:
                prev_ticks = 0;
                prev_time = 0;
                break;

            case APPLICATION_EVENT_FEATURE_MONITOR_REQUEST:
                event_data.event_type = APPLICATION_EVENT_FEATURE_MONITOR_REPORT;
                event_data.event_payload.event_uint32data[0] = (ULONG)&application_get_status;
                tx_err = tx_queue_send(g_application.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
                if(TX_SUCCESS != tx_err)
                {
                    SEGGER_RTT_printf(0, "Failed application_thread_monitor_timer_callback::tx_queue_send, tx_err = %d\r\n", tx_err);
                }
                break;

            case APPLICATION_EVENT_FEATURE_MONITOR_REPORT:
                for(uint32_t feature_num = 0; feature_num < g_application.p_cfg->feature_count; feature_num++)
                {
                    if((event_data.event_payload.event_uint32data[0]) == ((ULONG)g_application.p_cfg->p_features[feature_num].feature_get_status))
                    {
#if 0
                        SEGGER_RTT_printf(0, "%s reported in @ t=%u\n", g_application.p_cfg->p_features[feature_num].feature_name, tx_time_get());
#endif
                    }
                }
                break;

            default:
                break;
        }

        /* Reset event data variable */
        memset(&event_data, 0, sizeof(event_data));

        time_t  current_time = 0;
        time(&current_time);
        time_t  elapsed_time = current_time - prev_time;

        ULONG   elapsed_ticks = tx_time_get() - prev_ticks;
        prev_ticks += elapsed_ticks;
        prev_time += elapsed_time;
#if 0
        SEGGER_RTT_printf(0, "APPLICATION: Ticks = %010d, Time = %010d(%03d)\r\n",
               (int)prev_ticks, (int)prev_time, (int)elapsed_time);
#endif
    }
}

/******************************************************************************
 * FUNCTION: application_thread_monitor_timer_callback
 *****************************************************************************/
VOID application_feature_monitor_timer_callback(ULONG id)
{
    FSP_PARAMETER_NOT_USED(id);

    UINT        tx_err          = TX_SUCCESS;
    event_t     event_data      = { 0 };

    for(ULONG feature_num = 0; feature_num < g_application.p_cfg->feature_count; feature_num++)
    {
        event_data.event_type = APPLICATION_EVENT_FEATURE_MONITOR_REQUEST;
        tx_err = tx_queue_send(&g_application.p_cfg->p_features[feature_num].event_queue, &event_data, TX_NO_WAIT);
        if(TX_SUCCESS != tx_err)
        {
            SEGGER_RTT_printf(0, "Failed application_thread_monitor_timer_callback::tx_queue_send, tx_err = %d\r\n", tx_err);
        }
    }
}
