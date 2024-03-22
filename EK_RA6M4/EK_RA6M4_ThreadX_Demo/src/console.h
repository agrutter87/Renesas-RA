#ifndef CONSOLE_H
#define CONSOLE_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "application.h"
#include "sf_console.h"
#include "sf_console_api.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/
#define CONSOLE_THREAD_NAME                 ("Console Thread")
#define CONSOLE_THREAD_PRIORITY             (TX_MAX_PRIORITIES - 1)
#define CONSOLE_THREAD_PREEMPT_THRESHOLD    (TX_MAX_PRIORITIES - 1)
#define CONSOLE_THREAD_PERIOD               (TX_TIMER_TICKS_PER_SECOND)
#define CONSOLE_THREAD_STACK_SIZE           (APPLICATION_THREAD_STACK_SIZE)
#define CONSOLE_EVENT_QUEUE_NAME            ("Console Event Queue")

/******************************************************************************
 * TYPES
 *****************************************************************************/
/* TODO: Define console interface functions according to SF COMMS API, use sf_uart as example */
#if 0
typedef void * sf_cmd_comms_instance_ctrl_t;
typedef void * sf_cmd_comms_cfg_t;
#endif

typedef struct st_console_ctrl
{
    /* Thread Related */
    TX_THREAD                       thread;
    VOID                            *p_thread_stack;

    /* Event Queue Related */
    TX_QUEUE                        event_queue;
    VOID                            *p_event_queue_memory;

    /* Queryable status */
    feature_status_t                status;
} console_ctrl_t;

typedef struct st_console_cfg
{
    /* Thread Related */
    CHAR                            thread_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    VOID                            (*thread_entry)(ULONG);
    ULONG                           thread_input;
    ULONG                           thread_stack_size;
    UINT                            thread_priority;
    UINT                            thread_preempt_threshold;

    /* Event Queue Related */
    CHAR                            event_queue_name[THREAD_OBJECT_NAME_LENGTH_MAX];

    /* Console Related */
    sf_console_instance_t const     *p_console;
} console_cfg_t;

typedef struct st_console
{
    console_ctrl_t      *p_ctrl;
    console_cfg_t const *p_cfg;
} console_t;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void console_define(TX_BYTE_POOL * p_memory_pool);
void console_get_status(feature_status_t * p_status);
void console_thread_entry(ULONG thread_input);

/******************************************************************************
 * CALLBACK FUNCTIONS
 *****************************************************************************/
void feature_start_callback(sf_console_callback_args_t * p_args);
void feature_stop_callback(sf_console_callback_args_t * p_args);
void feature_status_callback(sf_console_callback_args_t * p_args);
void custom_code_callback(sf_console_callback_args_t * p_args);

#endif // CONSOLE_H
