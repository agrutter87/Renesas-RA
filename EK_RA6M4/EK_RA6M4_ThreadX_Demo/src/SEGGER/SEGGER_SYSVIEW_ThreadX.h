/*
 * SEGGER_SYSVIEW_ThreadX.h
 *
 *  Created on: Aug 31, 2023
 *      Author: Alex
 */

#ifndef SEGGER_SEGGER_SYSVIEW_THREADX_H_
#define SEGGER_SEGGER_SYSVIEW_THREADX_H_

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define SYSVIEW_THREADX_MAX_NOF_TASKS   (8)

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/
void SYSVIEW_SendThreadInfo (U32 TaskID, const char* sName, unsigned Prio, U32 StackBase, unsigned StackSize);

#endif /* SEGGER_SEGGER_SYSVIEW_THREADX_H_ */
