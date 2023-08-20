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
 .thread_name               = CONSOLE_THREAD_NAME,
 .thread_entry              = console_thread_entry,
 .thread_input              = 0, // TODO: Use for something useful
 .thread_stack_size         = CONSOLE_THREAD_STACK_SIZE,
 .thread_priority           = CONSOLE_THREAD_PRIORITY,
 .thread_preempt_threshold  = CONSOLE_THREAD_PREEMPT_THRESHOLD,
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
    UINT tx_err = TX_SUCCESS;

    SEGGER_RTT_printf(0, "Initializing console...\r\n");

    /* Allocate the stack for the thread */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_console.p_ctrl->p_thread_stack,
                              g_console.p_cfg->thread_stack_size,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed console_tx_define::tx_byte_allocate, tx_err = %d\r\n", tx_err);
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
        SEGGER_RTT_printf(0, "Failed console_tx_define::tx_thread_create, tx_err = %d\r\n", tx_err);
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
    fsp_err_t               fsp_err     = FSP_SUCCESS;
    sf_console_instance_t const *p_console  = g_console.p_cfg->p_console;

    FSP_PARAMETER_NOT_USED(thread_input);

    SEGGER_RTT_printf(0, "Started console\r\n");

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

    while(1)
    {
        /* This version blocks, so its unclear if it is sleeping or not */
        fsp_err = p_console->p_api->prompt(p_console->p_ctrl, p_console->p_cfg->p_initial_menu, TX_WAIT_FOREVER);
        if((FSP_SUCCESS != fsp_err) && (FSP_ERR_TIMEOUT != fsp_err))
        {
            SEGGER_RTT_printf(0, "Failed console_thread_entry::g_sf_console0.p_api->prompt, fsp_err = %d\r\n", fsp_err);
        }
    }
}
