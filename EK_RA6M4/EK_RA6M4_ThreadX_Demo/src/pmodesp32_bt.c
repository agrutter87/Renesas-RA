/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "pmodesp32_bt.h"
#include "uart_manager.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
/* PmodESP32 Bluetooth Object definitions */
pmodesp32_bt_ctrl_t g_pmodesp32_bt_ctrl;
const pmodesp32_bt_cfg_t   g_pmodesp32_bt_cfg =
{
 /* Thread creation arguments */
 .thread_name               = PMODESP32_BT_THREAD_NAME,
 .thread_entry              = pmodesp32_bt_thread_entry,
 .thread_input              = 0, // TODO: Use for something useful
 .thread_stack_size         = PMODESP32_BT_THREAD_STACK_SIZE,
 .thread_priority           = PMODESP32_BT_THREAD_PRIORITY,
 .thread_preempt_threshold  = PMODESP32_BT_THREAD_PREEMPT_THRESHOLD,

 /* UART */
 .uart_manager_channel      = 0,
 .reset_pin                 = BSP_IO_PORT_07_PIN_08,
};

const pmodesp32_bt_t g_pmodesp32_bt =
{
 .p_ctrl = &g_pmodesp32_bt_ctrl,
 .p_cfg  = &g_pmodesp32_bt_cfg
};

/******************************************************************************
 * FUNCTION: pmodesp32_bt_define
 *****************************************************************************/
void pmodesp32_bt_define(TX_BYTE_POOL * p_memory_pool, TX_QUEUE * p_event_queue)
{
    UINT                    tx_err      = TX_SUCCESS;
    event_t                 event_data  = { 0 };

    SEGGER_RTT_printf(0, "Initializing PmodESP32 Bluetooth...\r\n");

    g_pmodesp32_bt.p_ctrl->p_memory_byte_pool = p_memory_pool;
    g_pmodesp32_bt.p_ctrl->p_event_queue = p_event_queue;

    /* Allocate the stack for the thread */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_pmodesp32_bt.p_ctrl->p_thread_stack,
                              g_pmodesp32_bt.p_cfg->thread_stack_size,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed pmodesp32_bt_define::tx_byte_allocate, tx_err = %d\r\n", tx_err);
    }

    /* Create the thread.  */
    tx_err = tx_thread_create(&g_pmodesp32_bt.p_ctrl->thread,
                              (CHAR *)g_pmodesp32_bt.p_cfg->thread_name,
                              g_pmodesp32_bt.p_cfg->thread_entry,
                              g_pmodesp32_bt.p_cfg->thread_input,
                              g_pmodesp32_bt.p_ctrl->p_thread_stack,
                              g_pmodesp32_bt.p_cfg->thread_stack_size,
                              g_pmodesp32_bt.p_cfg->thread_priority,
                              g_pmodesp32_bt.p_cfg->thread_preempt_threshold,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed pmodesp32_bt_define::tx_thread_create, tx_err = %d\r\n", tx_err);
    }

    /* Send the initialize event */
    event_data.event_type = APPLICATION_EVENT_INIT;

    tx_err = tx_queue_send(g_pmodesp32_bt.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed pmodesp32_bt_define::tx_queue_send, tx_err = %d\r\n", tx_err);
    }
}

/******************************************************************************
 * FUNCTION: pmodesp32_bt_get_status
 *****************************************************************************/
void pmodesp32_bt_get_status(feature_status_t * p_status)
{
    UINT tx_err = TX_SUCCESS;

    if(g_pmodesp32_bt.p_ctrl->status.pp_thread == 0)
    {
        g_pmodesp32_bt.p_ctrl->status.thread_count = 1;

        tx_err = tx_byte_allocate(g_pmodesp32_bt.p_ctrl->p_memory_byte_pool,
                                  (VOID **)&g_pmodesp32_bt.p_ctrl->status.pp_thread,
                                  sizeof(TX_THREAD *) * g_pmodesp32_bt.p_ctrl->status.thread_count,
                                  TX_NO_WAIT);
    }

    if(TX_SUCCESS == tx_err)
    {
        g_pmodesp32_bt.p_ctrl->status.pp_thread[0] = &g_pmodesp32_bt.p_ctrl->thread;
        *p_status = g_pmodesp32_bt.p_ctrl->status;
    }
}

/******************************************************************************
 * FUNCTION: pmodesp32_bt_thread_entry
 *****************************************************************************/
void pmodesp32_bt_thread_entry(ULONG thread_input)
{
    UINT                        tx_err      = TX_SUCCESS;
    event_t                     event_data  = { 0 };

    char message[] = "AT\r\n";

    FSP_PARAMETER_NOT_USED(thread_input);

    while(1)
    {
        tx_queue_receive(g_pmodesp32_bt.p_ctrl->p_event_queue, &event_data, TX_WAIT_FOREVER);

        //SEGGER_RTT_printf(0, "PMODESP32_BT EVENT %d\r\n", event_data.event_type);
        switch(event_data.event_type)
        {
            case APPLICATION_EVENT_INIT:
                g_ioport.p_api->pinCfg(g_ioport.p_ctrl, g_pmodesp32_bt.p_cfg->reset_pin,
                                       ((uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT
                                               | (uint32_t) IOPORT_CFG_PORT_OUTPUT_HIGH));

                tx_err = uart_manager_register_rx(g_pmodesp32_bt.p_cfg->uart_manager_channel,
                                                  g_pmodesp32_bt.p_ctrl->p_event_queue, PMODESP32_BT_EVENT_RX);
                break;

            case APPLICATION_EVENT_FEATURE_MONITOR_REQUEST:
                event_data.event_type = APPLICATION_EVENT_FEATURE_MONITOR_REPORT;
                event_data.event_payload.event_uint32data[0] = (ULONG)&pmodesp32_bt_get_status;
                tx_err = tx_queue_send(g_application.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
                if(TX_SUCCESS != tx_err)
                {
                    SEGGER_RTT_printf(0, "Failed pmodesp32_bt_thread_entry::tx_queue_send, tx_err = %d\r\n", tx_err);
                }

                SEGGER_RTT_printf(0, "%s", message);
                uart_manager_request_tx(g_pmodesp32_bt.p_cfg->uart_manager_channel,
                                        (uint8_t *)message, (uint16_t)strlen(message));
                break;

            case PMODESP32_BT_EVENT_RX:
                SEGGER_RTT_PutChar(0, event_data.event_payload.event_uint8data[0]);
                break;

            default:
                break;
        }
    }
}
