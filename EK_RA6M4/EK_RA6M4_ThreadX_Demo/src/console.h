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

/******************************************************************************
 * TYPES
 *****************************************************************************/
/* TODO: Define console interface functions according to SF COMMS API, use sf_uart as example */
#if 1
typedef void * sf_cmd_comms_instance_ctrl_t;
typedef void * sf_cmd_comms_cfg_t;
#endif

typedef struct st_console
{
    /* Thread Related */
    TX_THREAD                       thread;
    CHAR                            thread_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    VOID                            (*thread_entry)(ULONG);
    ULONG                           thread_input;
    VOID                            *p_thread_stack;
    ULONG                           thread_stack_size;
    UINT                            thread_priority;
    UINT                            thread_preempt_threshold;

    /* Queryable status */
    feature_status_t                status;

    /* CMD Interface Related */
    sf_cmd_comms_instance_ctrl_t    sf_comms_ctrl;          /* TODO: Define console interface functions according to SF COMMS API, use sf_uart as example */
    sf_cmd_comms_cfg_t              sf_comms_cfg_extend;    /* TODO: Define console interface functions according to SF COMMS API, use sf_uart as example */
    sf_comms_cfg_t                  sf_comms_cfg;
    sf_comms_api_t                  sf_comms_api;
    sf_comms_instance_t             sf_comms;

    /* Console Related */
    sf_console_command_t            *p_sf_console_commands;
    sf_console_menu_t               sf_console_menu;
    sf_console_instance_ctrl_t      sf_console_instance_ctrl;
    sf_console_cfg_t                sf_console_cfg;
    sf_console_instance_t           sf_console;
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
