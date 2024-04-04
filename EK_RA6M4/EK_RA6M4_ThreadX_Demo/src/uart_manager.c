/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "uart_manager.h"
#include "hal_data.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
/* UART Manager Object definitions */
uart_manager_ctrl_t g_uart_manager_ctrl;
const uart_manager_cfg_t   g_uart_manager_cfg =
{
 /* Thread creation arguments */
 .thread_name               = UART_MANAGER_THREAD_NAME,
 .thread_entry              = uart_manager_thread_entry,
 .thread_input              = 0, // TODO: Use for something useful
 .thread_stack_size         = UART_MANAGER_THREAD_STACK_SIZE,
 .thread_priority           = UART_MANAGER_THREAD_PRIORITY,
 .thread_preempt_threshold  = UART_MANAGER_THREAD_PREEMPT_THRESHOLD,

 /* UART */
 .p_uart                    = {&g_uart0, 0, 0, 0, 0, 0, 0, &g_uart7, 0, &g_uart9},
};

const uart_manager_t g_uart_manager =
{
 .p_ctrl = &g_uart_manager_ctrl,
 .p_cfg  = &g_uart_manager_cfg
};

/******************************************************************************
 * FUNCTION: uart_manager_define
 *****************************************************************************/
void uart_manager_define(TX_BYTE_POOL * p_memory_pool, TX_QUEUE * p_event_queue)
{
    UINT                    tx_err      = TX_SUCCESS;
    event_t                 event_data  = { 0 };

    SEGGER_RTT_printf(0, "Initializing UART Manager...\r\n");

    g_uart_manager.p_ctrl->p_memory_byte_pool = p_memory_pool;
    g_uart_manager.p_ctrl->p_event_queue = p_event_queue;

    /* Allocate the stack for the thread */
    tx_err = tx_byte_allocate(p_memory_pool,
                              (VOID **) &g_uart_manager.p_ctrl->p_thread_stack,
                              g_uart_manager.p_cfg->thread_stack_size,
                              TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed uart_manager_define::tx_byte_allocate, tx_err = %d\r\n", tx_err);
    }

    /* Create the thread.  */
    tx_err = tx_thread_create(&g_uart_manager.p_ctrl->thread,
                              (CHAR *)g_uart_manager.p_cfg->thread_name,
                              g_uart_manager.p_cfg->thread_entry,
                              g_uart_manager.p_cfg->thread_input,
                              g_uart_manager.p_ctrl->p_thread_stack,
                              g_uart_manager.p_cfg->thread_stack_size,
                              g_uart_manager.p_cfg->thread_priority,
                              g_uart_manager.p_cfg->thread_preempt_threshold,
                              TX_NO_TIME_SLICE,
                              TX_AUTO_START);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed uart_manager_define::tx_thread_create, tx_err = %d\r\n", tx_err);
    }

    /* Send the initialize event */
    event_data.event_type = APPLICATION_EVENT_INIT;

    tx_err = tx_queue_send(g_uart_manager.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
    if(TX_SUCCESS != tx_err)
    {
        SEGGER_RTT_printf(0, "Failed uart_manager_define::tx_queue_send, tx_err = %d\r\n", tx_err);
    }
}

/******************************************************************************
 * FUNCTION: uart_manager_get_status
 *****************************************************************************/
void uart_manager_get_status(feature_status_t * p_status)
{
    UINT tx_err = TX_SUCCESS;

    if(g_uart_manager.p_ctrl->status.pp_thread == 0)
    {
        g_uart_manager.p_ctrl->status.thread_count = 1;

        tx_err = tx_byte_allocate(g_uart_manager.p_ctrl->p_memory_byte_pool,
                                  (VOID **)&g_uart_manager.p_ctrl->status.pp_thread,
                                  sizeof(TX_THREAD *) * g_uart_manager.p_ctrl->status.thread_count,
                                  TX_NO_WAIT);
    }

    if(TX_SUCCESS == tx_err)
    {
        g_uart_manager.p_ctrl->status.pp_thread[0] = &g_uart_manager.p_ctrl->thread;
        *p_status = g_uart_manager.p_ctrl->status;
    }
}

/******************************************************************************
 * FUNCTION: uart_manager_thread_entry
 *****************************************************************************/
void uart_manager_thread_entry(ULONG thread_input)
{
    fsp_err_t                   fsp_err     = FSP_SUCCESS;
    uart_instance_t const       *p_uart     = NULL;
    UINT                        tx_err      = TX_SUCCESS;
    event_t                     event_data  = { 0 };
#if TESTING_uart_manager_request_tx
    char message[] = "Hello World\r\n";
#endif
    FSP_PARAMETER_NOT_USED(thread_input);

    while(1)
    {
        tx_queue_receive(g_uart_manager.p_ctrl->p_event_queue, &event_data, TX_WAIT_FOREVER);

        //SEGGER_RTT_printf(0, "UART_MANAGER EVENT %d\r\n", event_data.event_type);
        switch(event_data.event_type)
        {
            case APPLICATION_EVENT_INIT:
                for(uint8_t uart_index = 0; uart_index < UART_MANAGER_CHANNELS_MAX; uart_index++)
                {
                    p_uart = g_uart_manager.p_cfg->p_uart[uart_index];

                    if(0 != p_uart)
                    {
                        fsp_err = p_uart->p_api->open(p_uart->p_ctrl, p_uart->p_cfg);
                        if(FSP_SUCCESS != fsp_err)
                        {
                            SEGGER_RTT_printf(0, "Failed uart_manager_thread_entry::p_uart->p_api->open, uart_index = %d, fsp_err = %d\r\n", uart_index, fsp_err);
                        }
                    }
                }

#if TESTING_uart_manager_register_rx
                tx_err = uart_manager_register_rx(7, g_uart_manager.p_ctrl->p_event_queue, UART_MANAGER_EVENT_RX);
#endif
                break;

            case APPLICATION_EVENT_FEATURE_MONITOR_REQUEST:
                event_data.event_type = APPLICATION_EVENT_FEATURE_MONITOR_REPORT;
                event_data.event_payload.event_uint32data[0] = (ULONG)&uart_manager_get_status;
                tx_err = tx_queue_send(g_application.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
                if(TX_SUCCESS != tx_err)
                {
                    SEGGER_RTT_printf(0, "Failed uart_manager_thread_entry::tx_queue_send, tx_err = %d\r\n", tx_err);
                }
#if TESTING_uart_manager_register_rx
                SEGGER_RTT_printf(0, "Sending \"%s\"...\r\n", message);
#endif
#if TESTING_uart_manager_request_tx
                uart_manager_request_tx(7, (uint8_t *)message, (uint16_t)strlen(message));
#endif
                break;

            case UART_MANAGER_EVENT_TX:
                p_uart = g_uart_manager.p_cfg->p_uart[event_data.event_payload.event_uint32data[0]];
                fsp_err = p_uart->p_api->write(p_uart->p_ctrl,
                                               (uint8_t *)event_data.event_payload.event_p_data[2],
                                               event_data.event_payload.event_uint32data[1]);
                if(FSP_SUCCESS != fsp_err)
                {
                    SEGGER_RTT_printf(0, "Failed uart_manager_thread_entry::p_uart->p_api->write, fsp_err = %d\r\n", fsp_err);
                }

                tx_byte_release(event_data.event_payload.event_p_data[2]);

                break;

#if TESTING_uart_manager_register_rx
            case UART_MANAGER_EVENT_RX:
                SEGGER_RTT_printf(0, "RX: 0x%08x (%c)\r\n",
                                  event_data.event_payload.event_uint8data[0],
                                  event_data.event_payload.event_uint8data[0]);
                break;
#endif

            default:
                break;
        }
    }
}

/******************************************************************************
 * FUNCTION: uart_callback
 *****************************************************************************/
void uart_callback(uart_callback_args_t *p_args)
{
    UINT                    tx_err      = TX_SUCCESS;

#if TESTING_uart_callback
    SEGGER_RTT_printf(0, "uart_callback:, channel=%lu, event=%lu, data=%lu, context=0x%08x\r\n",
                      p_args->channel,
                      p_args->event,
                      p_args->data,
                      p_args->p_context);
#endif

    if(p_args->channel < UART_MANAGER_CHANNELS_MAX)
    {
        if(UART_EVENT_RX_COMPLETE & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_RX_COMPLETE\r\n");
        }
        if(UART_EVENT_TX_COMPLETE & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_TX_COMPLETE\r\n");
        }
        if(UART_EVENT_RX_CHAR & p_args->event)
        {
            if(NULL != g_uart_manager.p_ctrl->p_registered_rx_queue[p_args->channel])
            {
                event_t event_data  = { 0 };
                event_data.event_type = g_uart_manager.p_ctrl->registered_rx_event[p_args->channel];
                event_data.event_payload.event_uint8data[0] = (UCHAR)p_args->data;

                tx_err = tx_queue_send(g_uart_manager.p_ctrl->p_registered_rx_queue[p_args->channel], &event_data, TX_NO_WAIT);
                if(TX_SUCCESS != tx_err)
                {
                    SEGGER_RTT_printf(0, "Failed uart_callback::tx_queue_send, tx_err = %d\r\n", tx_err);
                }
            }
        }
        if(UART_EVENT_ERR_PARITY & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_ERR_PARITY\r\n");
        }
        if(UART_EVENT_ERR_FRAMING & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_ERR_FRAMING\r\n");
        }
        if(UART_EVENT_ERR_OVERFLOW & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_ERR_OVERFLOW\r\n");
        }
        if(UART_EVENT_BREAK_DETECT & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_BREAK_DETECT\r\n");
        }
        if(UART_EVENT_TX_DATA_EMPTY & p_args->event)
        {
            //SEGGER_RTT_printf(0, "UART_EVENT_TX_DATA_EMPTY\r\n");
        }
    }
}

UINT uart_manager_request_tx(uint8_t channel, uint8_t * p_data, uint32_t length)
{
    UINT    tx_err      = TX_SUCCESS;
    event_t event_data  = { 0 };

    if(!(channel < UART_MANAGER_CHANNELS_MAX))
    {
        tx_err = TX_OPTION_ERROR;
    }
    else if(p_data == NULL)
    {
        tx_err = TX_PTR_ERROR;
    }
    else if(length == 0)
    {
        tx_err = TX_SIZE_ERROR;
    }

    if(TX_SUCCESS == tx_err)
    {
        tx_err = tx_byte_allocate(g_uart_manager.p_ctrl->p_memory_byte_pool,
                                  (VOID **) &event_data.event_payload.event_p_data[2],
                                  length,
                                  TX_NO_WAIT);
        if(TX_SUCCESS != tx_err)
        {
            SEGGER_RTT_printf(0, "Failed uart_manager_request_tx::tx_byte_allocate, tx_err = %d\r\n", tx_err);
        }
    }

    if(TX_SUCCESS == tx_err)
    {
        event_data.event_type = UART_MANAGER_EVENT_TX;
        event_data.event_payload.event_uint32data[0] = (UCHAR)channel;
        event_data.event_payload.event_uint32data[1] = length;
        memcpy(event_data.event_payload.event_p_data[2], p_data, length);

        tx_err = tx_queue_send(g_uart_manager.p_ctrl->p_event_queue, &event_data, TX_NO_WAIT);
        if(TX_SUCCESS != tx_err)
        {
            SEGGER_RTT_printf(0, "Failed uart_manager_request_tx::tx_queue_send, tx_err = %d\r\n", tx_err);
        }
    }

    return tx_err;
}

UINT uart_manager_register_rx(uint8_t channel, TX_QUEUE * p_event_queue, ULONG event_id)
{
    UINT    tx_err      = TX_SUCCESS;

    if(!(channel < UART_MANAGER_CHANNELS_MAX))
    {
        tx_err = TX_OPTION_ERROR;
    }
    else if(p_event_queue == NULL)
    {
        tx_err = TX_PTR_ERROR;
    }
    else if(event_id < APPLICATION_EVENT_END)
    {
        tx_err = TX_NOT_AVAILABLE;
    }

    if(TX_SUCCESS == tx_err)
    {
        g_uart_manager.p_ctrl->p_registered_rx_queue[channel] = p_event_queue;
        g_uart_manager.p_ctrl->registered_rx_event[channel] = event_id;
    }

    return tx_err;
}
