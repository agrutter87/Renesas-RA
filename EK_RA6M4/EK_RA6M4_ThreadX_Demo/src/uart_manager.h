#ifndef UART_MANAGER_H
#define UART_MANAGER_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "application.h"
#include "hal_data.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/
#define UART_MANAGER_THREAD_NAME                ("UART Manager Thread")
#define UART_MANAGER_THREAD_PRIORITY            (TX_MAX_PRIORITIES - 1)
#define UART_MANAGER_THREAD_PREEMPT_THRESHOLD   (TX_MAX_PRIORITIES - 1)
#define UART_MANAGER_THREAD_PERIOD              (TX_TIMER_TICKS_PER_SECOND)
#define UART_MANAGER_THREAD_STACK_SIZE          (APPLICATION_THREAD_STACK_SIZE)
#define UART_MANAGER_RX_TICKS                   (100)
#define UART_MANAGER_CHANNELS_MAX               (10)
#define UART_MANAGER_TX_QUEUE_MESSAGE_SIZE      (1)
#define UART_MANAGER_RX_QUEUE_MESSAGE_SIZE      (1)
#define UART_MANAGER_TX_QUEUE_MEMORY_MAX        ((64 * UART_MANAGER_TX_QUEUE_MESSAGE_SIZE) + sizeof(void *))
#define UART_MANAGER_RX_QUEUE_MEMORY_MAX        ((64 * UART_MANAGER_RX_QUEUE_MESSAGE_SIZE) + sizeof(void *))

#define TESTING_uart_callback                   (0)
#define TESTING_uart_manager_request_tx         (1)
#define TESTING_uart_manager_register_rx        (1)

/******************************************************************************
 * TYPES
 *****************************************************************************/
typedef enum st_uart_manager_event
{
    UART_MANAGER_EVENT_TX = APPLICATION_EVENT_END,
#if TESTING_uart_manager_register_rx
    UART_MANAGER_EVENT_RX
#endif
} uart_manager_event_t;

typedef struct st_uart_manager_ctrl
{
    /* Thread Related */
    TX_THREAD                       thread;
    VOID                            *p_thread_stack;

    /* Event Queue Related */
    TX_QUEUE                        *p_event_queue;

    /* Memory Related */
    TX_BYTE_POOL                    *p_memory_byte_pool;

    /* Send/receive Queue Related */
    TX_QUEUE                        *p_registered_rx_queue[UART_MANAGER_CHANNELS_MAX];
    ULONG                           registered_rx_event[UART_MANAGER_CHANNELS_MAX];

    /* Queryable status */
    feature_status_t                status;
} uart_manager_ctrl_t;

typedef struct st_uart_manager_cfg
{
    /* Thread Related */
    CHAR                            thread_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    VOID                            (*thread_entry)(ULONG);
    ULONG                           thread_input;
    ULONG                           thread_stack_size;
    UINT                            thread_priority;
    UINT                            thread_preempt_threshold;

    /* UART Related */
    uart_instance_t const           *p_uart[UART_MANAGER_CHANNELS_MAX];
} uart_manager_cfg_t;

typedef struct st_uart_manager
{
    uart_manager_ctrl_t      *p_ctrl;
    uart_manager_cfg_t const *p_cfg;
} uart_manager_t;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void uart_manager_define(TX_BYTE_POOL * p_memory_pool, TX_QUEUE * p_event_queue);
void uart_manager_get_status(feature_status_t * p_status);
void uart_manager_thread_entry(ULONG thread_input);
UINT uart_manager_request_tx(uint8_t channel, uint8_t * p_data, uint16_t length);
UINT uart_manager_register_rx(uint8_t channel, TX_QUEUE * p_event_queue, ULONG event_id);

#endif // UART_MANAGER_H
