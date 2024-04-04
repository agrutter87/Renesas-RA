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
#define CONSOLE_RX_TICKS                    (100)

/******************************************************************************
 * TYPES
 *****************************************************************************/
typedef enum st_console_event
{
    CONSOLE_EVENT_CHECK_RX = APPLICATION_EVENT_END,
    CONSOLE_EVENT_CHANGE_MENU
} console_event_t;

typedef struct st_console_ctrl
{
    /* Thread Related */
    TX_THREAD                       thread;
    VOID                            *p_thread_stack;

    /* Event Queue Related */
    TX_QUEUE                        *p_event_queue;

    /* Memory Related */
    TX_BYTE_POOL                    *p_memory_byte_pool;

    /* Timer Related */
    TX_TIMER                        rx_timer;

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

    /* Timer Related */
    CHAR                            rx_timer_name[THREAD_OBJECT_NAME_LENGTH_MAX];

    /* Console Related */
    sf_console_instance_t const     *p_console;
} console_cfg_t;

typedef struct st_console
{
    console_ctrl_t      *p_ctrl;
    console_cfg_t const *p_cfg;
} console_t;

extern const sf_console_menu_t g_sf_console_menu;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void console_define(TX_BYTE_POOL * p_memory_pool, TX_QUEUE * p_event_queue);
void console_get_status(feature_status_t * p_status);
void console_thread_entry(ULONG thread_input);
UINT console_request_menu_change(sf_console_menu_t const * p_new_menu);

/******************************************************************************
 * CALLBACK FUNCTIONS
 *****************************************************************************/
void feature_start_callback(sf_console_callback_args_t * p_args);
void feature_stop_callback(sf_console_callback_args_t * p_args);
void feature_status_callback(sf_console_callback_args_t * p_args);
void custom_code_callback(sf_console_callback_args_t * p_args);


#endif // CONSOLE_H
