/*
 * SEGGER_SYSVIEW_ThreadX.c
 *
 *  Created on: Aug 31, 2023
 *      Author: Alex
 */

#include "tx_api.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_ThreadX.h"

typedef struct SYSVIEW_THREADX_TASK_STATUS  SYSVIEW_THREADX_TASK_STATUS;

struct SYSVIEW_THREADX_TASK_STATUS {
  U32         thread_ptr;
  const char* tx_thread_name;
  unsigned    tx_thread_priority;
  U32         tx_thread_stack_start;
  unsigned    tx_thread_stack_highest_ptr;
};

static SYSVIEW_THREADX_TASK_STATUS  _aTasks[SYSVIEW_THREADX_MAX_NOF_TASKS];
static unsigned _NumTasks;

/*********************************************************************
*
*       _cbSendTaskList()
*
*  Function description
*    This function is part of the link between ThreadX and SYSVIEW.
*    Called from SystemView when asked by the host, it uses SYSVIEW
*    functions to send the entire task list to the host.
*/
static void _cbSendTaskList(void) {
    unsigned n;

    for (n = 0; n < _NumTasks; n++) {
  #if INCLUDE_uxTaskGetStackHighWaterMark // Report Task Stack High Watermark
      _aTasks[n].uStackHighWaterMark = uxTaskGetStackHighWaterMark((TaskHandle_t)_aTasks[n].xHandle);
  #endif
      SYSVIEW_SendThreadInfo((U32)_aTasks[n].thread_ptr, _aTasks[n].tx_thread_name, (unsigned)_aTasks[n].tx_thread_priority, (U32)_aTasks[n].tx_thread_stack_start, (unsigned)_aTasks[n].tx_thread_stack_highest_ptr);
    }
}

/*********************************************************************
*
*       _cbGetTime()
*
*  Function description
*    This function is part of the link between ThreadX and SYSVIEW.
*    Called from SystemView when asked by the host, returns the
*    current system time in micro seconds.
*/
static U64 _cbGetTime(void) {
  U64 Time;

  Time = tx_time_get();
  Time *= (TX_TIMER_TICKS_PER_SECOND / 1000);
  Time *= 1000;
  return Time;
}


/*********************************************************************
*
*       SYSVIEW_SendTaskInfo()
*
*  Function description
*    Record task information.
*/
void SYSVIEW_SendThreadInfo(U32 TaskID, const char* sName, unsigned Prio, U32 StackBase, unsigned StackSize) {
  SEGGER_SYSVIEW_TASKINFO TaskInfo;

  memset(&TaskInfo, 0, sizeof(TaskInfo)); // Fill all elements with 0 to allow extending the structure in future version without breaking the code
  TaskInfo.TaskID     = TaskID;
  TaskInfo.sName      = sName;
  TaskInfo.Prio       = Prio;
  TaskInfo.StackBase  = StackBase;
  TaskInfo.StackSize  = StackSize;
  SEGGER_SYSVIEW_SendTaskInfo(&TaskInfo);
}


/*********************************************************************
*
*       Public API structures
*
**********************************************************************
*/
// Callbacks provided to SYSTEMVIEW by FreeRTOS
const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
  _cbGetTime,
  _cbSendTaskList,
};
