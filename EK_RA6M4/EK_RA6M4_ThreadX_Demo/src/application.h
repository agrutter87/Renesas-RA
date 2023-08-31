#ifndef APPLICATION_H
#define APPLICATION_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <SEGGER/SEGGER_RTT.h>
#include "tx_api.h"

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/
#define APPLICATION_MEMORY_MAX                  (8192U)

#define APPLICATION_THREAD_NAME                 ("Application Thread")
#define APPLICATION_THREAD_PRIORITY             (1)
#define APPLICATION_THREAD_PREEMPT_THRESHOLD    (1)
#define APPLICATION_THREAD_PERIOD               (TX_TIMER_TICKS_PER_SECOND)
#define APPLICATION_THREAD_STACK_SIZE           (1024U)

#define THREAD_OBJECT_NAME_LENGTH_MAX           (32)
#define FEATURE_NAME_MAX_LENGTH                 (32)

/******************************************************************************
 * TYPES
 *****************************************************************************/
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

typedef struct st_application
{
    /* Thread Related */
    TX_THREAD           thread;
    CHAR                thread_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    VOID                (*thread_entry)(ULONG);
    ULONG               thread_input;
    VOID                *p_thread_stack;
    ULONG               thread_stack_size;
    UINT                thread_priority;
    UINT                thread_preempt_threshold;

    /* Queryable status */
    feature_status_t    status;

    /* Application Memory */
    TX_BYTE_POOL        memory_byte_pool;
    CHAR                memory_byte_pool_name[THREAD_OBJECT_NAME_LENGTH_MAX];
    ULONG               memory_byte_pool_size;

    /* Features */
    feature_t const     *p_features;
    ULONG               feature_count;
} application_t;

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
extern application_t g_application;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void application_define(TX_BYTE_POOL * p_memory_pool);
void application_get_status(feature_status_t * p_status);
void application_thread_entry(ULONG thread_input);

#endif // APPLICATION_H
