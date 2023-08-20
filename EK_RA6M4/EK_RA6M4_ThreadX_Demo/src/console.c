/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "sf_rtt_comms.h"
#include "console.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
console_t * gp_console = 0;

/* Assigns the callback functions to each command */
const sf_console_command_t            g_console_commands[] =
{
    {
        .command    = (uint8_t *) "feature start",
        .help       = (uint8_t *) "Starts a feature. USAGE: feature start <feature_name> (NOT IMPLEMENTED)",
        .callback   = feature_start_callback,
        .context    = NULL
    },
    {
        .command    = (uint8_t *) "feature stop",
        .help       = (uint8_t *) "Stops a feature. USAGE: feature stop <feature_name> (NOT IMPLEMENTED)",
        .callback   = feature_start_callback,
        .context    = NULL
    },
    {
        .command    = (uint8_t *) "feature status",
        .help       = (uint8_t *) "Starts a feature.",
        .callback   = feature_status_callback,
        .context    = NULL
    },
    {
        .command    = (uint8_t *) "custom",
        .help       = (uint8_t *) "Does some custom code.",
        .callback   = custom_code_callback,
        .context    = NULL
    },
};
const sf_console_menu_t g_sf_console_menu =
{
 .menu_prev       = NULL,
 .menu_name       = (uint8_t *)"",
 .num_commands    = sizeof(g_console_commands) / sizeof(g_console_commands[0]),
 .command_list    = g_console_commands
};

/* SF Comms definitions */
sf_rtt_comms_instance_ctrl_t g_sf_comms_ctrl;
const sf_rtt_comms_cfg_t g_sf_comms_extended_cfg = { };
const sf_comms_cfg_t g_sf_comms_cfg =
{
 .p_extend = &g_sf_comms_extended_cfg
};
const sf_comms_instance_t g_sf_comms =
{
 .p_ctrl = &g_sf_comms_ctrl,
 .p_cfg  = &g_sf_comms_cfg,
 .p_api  = &g_sf_comms_on_sf_rtt_comms
};

/* SF Console definitions */
sf_console_instance_ctrl_t g_sf_console_ctrl;
const sf_console_cfg_t g_sf_console_cfg =
{
 .p_comms           = &g_sf_comms,
 .p_initial_menu    = &g_sf_console_menu,
 .echo              = true,
 .autostart         = false
};
const sf_console_instance_t g_sf_console =
{
 .p_ctrl = &g_sf_console_ctrl,
 .p_cfg  = &g_sf_console_cfg,
 .p_api  = &g_sf_console_on_sf_console
};

/* Console Object definitions */
console_ctrl_t  g_console_ctrl;
const console_cfg_t   g_console_cfg =
{
 /* Thread creation arguments */
 .thread_name               = CONSOLE_THREAD_NAME,
 .thread_entry              = console_thread_entry,
 .thread_input              = 0, // TODO: Use for something useful
 .thread_stack_size         = CONSOLE_THREAD_STACK_SIZE,
 .thread_priority           = CONSOLE_THREAD_PRIORITY,
 .thread_preempt_threshold  = CONSOLE_THREAD_PREEMPT_THRESHOLD,

 /* Event queue creation arguments */
 .event_queue_name          = CONSOLE_EVENT_QUEUE_NAME,

 /* SF Console */
 .p_console                 = &g_sf_console

};
const console_t g_console =
{
 .p_ctrl = &g_console_ctrl,
 .p_cfg  = &g_console_cfg
};

/******************************************************************************
 * FUNCTION: console_define
 *****************************************************************************/
void console_define(TX_BYTE_POOL * p_memory_pool)
{
    UINT        tx_err          = TX_SUCCESS;
    event_t     event_data      = { .event_type = APPLICATION_EVENT_INIT, .event_payload = { 0 } };

    SEGGER_RTT_printf(0, "Initializing console...\r\n");

    /* Allocate the memory for the event queue */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_console.p_ctrl->p_event_queue_memory,
                              EVENT_QUEUE_MEMORY_MAX,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed console_define::tx_byte_allocate (p_event_queue_memory), tx_err = %d\r\n", tx_err);
    }

    /* Create the event queue */
    tx_err = tx_queue_create(&g_console.p_ctrl->event_queue,
                             (CHAR *)g_console.p_cfg->event_queue_name,
                             EVENT_QUEUE_MESSAGE_SIZE,
                             g_console.p_ctrl->p_event_queue_memory,
                             EVENT_QUEUE_MEMORY_MAX);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed console_define::tx_queue_create, tx_err = %d\r\n", tx_err);
    }

    /* Allocate the stack for the thread */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_console.p_ctrl->p_thread_stack,
                              g_console.p_cfg->thread_stack_size,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed console_define::tx_byte_allocate, tx_err = %d\r\n", tx_err);
    }

    /* Create the thread.  */
    tx_err = tx_thread_create(&g_console.p_ctrl->thread,
                              (CHAR *)g_console.p_cfg->thread_name,
                              g_console.p_cfg->thread_entry,
                              g_console.p_cfg->thread_input,
                              g_console.p_ctrl->p_thread_stack,
                              g_console.p_cfg->thread_stack_size,
                              g_console.p_cfg->thread_priority,
                              g_console.p_cfg->thread_preempt_threshold,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed console_define::tx_thread_create, tx_err = %d\r\n", tx_err);
    }

    /* Send the initialize event */
    tx_err = tx_queue_send(&g_console.p_ctrl->event_queue, &event_data, TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed console_define::tx_queue_send, tx_err = %d\r\n", tx_err);
    }
}

/******************************************************************************
 * FUNCTION: console_get_status
 *****************************************************************************/
void console_get_status(feature_status_t * p_status)
{
    *p_status = g_console.p_ctrl->status;
}

/******************************************************************************
 * FUNCTION: console_thread_entry
 *****************************************************************************/
void console_thread_entry(ULONG thread_input)
{
    fsp_err_t                   fsp_err     = FSP_SUCCESS;
    sf_console_instance_t const *p_console  = g_console.p_cfg->p_console;
    UINT                        tx_err      = TX_SUCCESS;
    event_t                     event_data  = { 0 };

    FSP_PARAMETER_NOT_USED(thread_input);

    while(1)
    {
        tx_queue_receive(&g_console.p_ctrl->event_queue, &event_data, TX_WAIT_FOREVER);

        SEGGER_RTT_printf(0, "CONSOLE EVENT %d\r\n", event_data.event_type);
        switch(event_data.event_type)
        {
            case APPLICATION_EVENT_INIT:
                fsp_err = p_console->p_api->open(p_console->p_ctrl, p_console->p_cfg);
                if(FSP_SUCCESS != fsp_err)
                {
                    SEGGER_RTT_printf(0, "Failed console_thread_entry::g_sf_console0.p_api->open, fsp_err = %d\r\n", fsp_err);
                }

                fsp_err = p_console->p_api->write(p_console->p_ctrl, (uint8_t *)"\r\nWelcome to Grutter's example ThreadX System Developer\r\nEnter '?' for a list of commands...\r\n", 100);
                if(FSP_SUCCESS != fsp_err)
                {
                    SEGGER_RTT_printf(0, "Failed console_thread_entry::g_sf_console0.p_api->write, fsp_err = %d\r\n", fsp_err);
                }

#if 0
                /* This version blocks, so its unclear if it is sleeping or not */
                fsp_err = p_console->p_api->prompt(p_console->p_ctrl, p_console->p_cfg->p_initial_menu, TX_NO_WAIT);
                if((FSP_SUCCESS != fsp_err) && (FSP_ERR_TIMEOUT != fsp_err))
                {
                    SEGGER_RTT_printf(0, "Failed console_thread_entry::g_sf_console0.p_api->prompt, fsp_err = %d\r\n", fsp_err);
                }
#endif
                break;

            case APPLICATION_EVENT_FEATURE_MONITOR_REQUEST:
                event_data.event_type = APPLICATION_EVENT_FEATURE_MONITOR_REPORT;
                event_data.event_payload.event_ulongdata = (ULONG)&console_get_status;
                tx_err = tx_queue_send(&g_application.p_ctrl->event_queue, &event_data, TX_NO_WAIT);
                if(TX_SUCCESS != tx_err)
                {
                    SEGGER_RTT_printf(0, "Failed console_thread_entry::tx_queue_send, tx_err = %d\r\n", tx_err);
                }
                break;

            default:
                break;
        }
    }
}
