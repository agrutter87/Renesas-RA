#ifndef APPLICATION_H
#define APPLICATION_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "tx_api.h"
#include "SEGGER_RTT.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/
#define APPLICATION_MEMORY_MAX                  (8192U)

#define APPLICATION_THREAD_NAME                 ("Application Thread")
#define APPLICATION_THREAD_PRIORITY             (1)
#define APPLICATION_THREAD_PREEMPT_THRESHOLD    (1)
#define APPLICATION_THREAD_PERIOD               (TX_TIMER_TICKS_PER_SECOND)
#define APPLICATION_THREAD_STACK_SIZE           (1024U)
#define APPLICATION_EVENT_QUEUE_NAME            ("Application Event Queue")

#define APPLICATION_THREAD_MONITOR_TIMER_NAME   ("Application Thread Monitor Timer")
#define APPLICATION_THREAD_MONITOR_TICKS        (1000)

#define THREAD_OBJECT_NAME_LENGTH_MAX           (32)
#define FEATURE_NAME_MAX_LENGTH                 (32)
#define EVENT_QUEUE_MESSAGE_SIZE                (2)
#define EVENT_QUEUE_MEMORY_MAX                  ((10 * EVENT_QUEUE_MESSAGE_SIZE) + sizeof(void *))

/******************************************************************************
 * TYPES
 *****************************************************************************/
typedef enum st_application_event
{
    APPLICATION_EVENT_INIT                      = 0,
    APPLICATION_EVENT_FEATURE_MONITOR_REQUEST   = 1,
    APPLICATION_EVENT_FEATURE_MONITOR_REPORT    = 2,
    APPLICATION_EVENT_END                       = 3,
} application_event_t;

typedef struct st_event
{
    ULONG  event_type;
    union
    {
        UINT     event_timer_id;
        UCHAR    event_uchardata[4];
        USHORT   event_ushortdata[2];
        ULONG    event_ulongdata;
        CHAR     event_chardata[4];
        SHORT    event_shortdata[2];
        INT      event_intdata[2];
        LONG     event_longdata;
    } event_payload;
} event_t;

typedef struct st_feature_status
{
    ULONG return_code;
} feature_status_t;

typedef struct st_feature
{
    /* Readable name */
    CHAR feature_name[FEATURE_NAME_MAX_LENGTH];

    /* Function to be called with tx_application_define.
     * - Allocates memory from p_memory_pool for everything */
    void (*feature_define)(TX_BYTE_POOL * p_memory_pool);

    /* Function to be use by console (or other features in general?) to get the status
     * of the feature */
    void (*feature_get_status)(feature_status_t * p_status);
} feature_t;

typedef struct st_application_ctrl
{
    TX_BYTE_POOL        memory_byte_pool;
    TX_TIMER            thread_monitor_event_timer;

    /* Thread Related */
    TX_THREAD           thread;
    VOID                *p_thread_stack;

    /* Event Queue Related */
    TX_QUEUE            event_queue;
    VOID                *p_event_queue_memory;

    /* Queryable status */
    feature_status_t    status;
} application_ctrl_t;

typedef struct st_application_cfg
{
    /* Thread Related */
    CHAR                thread_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    VOID                (*thread_entry)(ULONG);
    ULONG               thread_input;
    ULONG               thread_stack_size;
    UINT                thread_priority;
    UINT                thread_preempt_threshold;

    /* Event Queue Related */
    CHAR                event_queue_name[THREAD_OBJECT_NAME_LENGTH_MAX];

    /* Thread Monitor Timer Related */
    CHAR                thread_monitor_timer_name[THREAD_OBJECT_NAME_LENGTH_MAX];

    /* Application Memory */
    CHAR                memory_byte_pool_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    ULONG               memory_byte_pool_size;

    /* Features */
    feature_t const     *p_features;
    ULONG               feature_count;
} application_cfg_t;

typedef struct st_application
{
    application_ctrl_t      *p_ctrl;
    application_cfg_t const *p_cfg;
} application_t;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
extern const application_t g_application;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void application_define(TX_BYTE_POOL * p_memory_pool);
void application_get_status(feature_status_t * p_status);
void application_thread_entry(ULONG thread_input);

#endif // APPLICATION_H
