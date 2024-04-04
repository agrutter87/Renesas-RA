#ifndef PMODESP32_BT_H
#define PMODESP32_BT_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "application.h"
#include "hal_data.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/
#define PMODESP32_BT_THREAD_NAME                ("PmodESP32 Bluetooth Thread")
#define PMODESP32_BT_THREAD_PRIORITY            (TX_MAX_PRIORITIES - 1)
#define PMODESP32_BT_THREAD_PREEMPT_THRESHOLD   (TX_MAX_PRIORITIES - 1)
#define PMODESP32_BT_THREAD_PERIOD              (TX_TIMER_TICKS_PER_SECOND)
#define PMODESP32_BT_THREAD_STACK_SIZE          (APPLICATION_THREAD_STACK_SIZE)

/******************************************************************************
 * TYPES
 *****************************************************************************/
typedef enum st_pmodesp32_bt_event
{
    PMODESP32_BT_EVENT_RX = APPLICATION_EVENT_END,
} pmodesp32_bt_event_t;

typedef struct st_pmodesp32_bt_ctrl
{
    /* Thread Related */
    TX_THREAD                       thread;
    VOID                            *p_thread_stack;

    /* Event Queue Related */
    TX_QUEUE                        *p_event_queue;

    /* Memory Related */
    TX_BYTE_POOL                    *p_memory_byte_pool;

    /* Queryable status */
    feature_status_t                status;
} pmodesp32_bt_ctrl_t;

typedef struct st_pmodesp32_bt_cfg
{
    /* Thread Related */
    CHAR                            thread_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    VOID                            (*thread_entry)(ULONG);
    ULONG                           thread_input;
    ULONG                           thread_stack_size;
    UINT                            thread_priority;
    UINT                            thread_preempt_threshold;

    /* UART Related */
    uint8_t                         uart_manager_channel;
    bsp_io_port_pin_t               reset_pin;
} pmodesp32_bt_cfg_t;

typedef struct st_pmodesp32_bt
{
    pmodesp32_bt_ctrl_t      *p_ctrl;
    pmodesp32_bt_cfg_t const *p_cfg;
} pmodesp32_bt_t;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void pmodesp32_bt_define(TX_BYTE_POOL * p_memory_pool, TX_QUEUE * p_event_queue);
void pmodesp32_bt_get_status(feature_status_t * p_status);
void pmodesp32_bt_thread_entry(ULONG thread_input);

#endif // PMODESP32_BT_H
