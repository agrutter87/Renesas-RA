/***********************************************************************************************************************
 * Copyright [2015-2021] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
 * 
 * This file is part of Renesas SynergyTM Software Package (SSP)
 *
 * The contents of this file (the "contents") are proprietary and confidential to Renesas Electronics Corporation
 * and/or its licensors ("Renesas") and subject to statutory and contractual protections.
 *
 * This file is subject to a Renesas SSP license agreement. Unless otherwise agreed in an SSP license agreement with
 * Renesas: 1) you may not use, copy, modify, distribute, display, or perform the contents; 2) you may not use any name
 * or mark of Renesas for advertising or publicity purposes or in connection with your use of the contents; 3) RENESAS
 * MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED
 * "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF
 * CONTRACT OR TORT, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents
 * included in this file may be subject to different terms.
 **********************************************************************************************************************/
/**********************************************************************************************************************
* File Name    : sf_console.h
* Description  : Console framework module instance
***********************************************************************************************************************/

#ifndef SF_CONSOLE_H
#define SF_CONSOLE_H

/*******************************************************************************************************************//**
 * @ingroup SF_Library
 * @defgroup Console Console Framework
 *
 * @brief RTOS-integrated Console Framework.
 *
 * This is a ThreadX aware console framework implemented using the SSP communications framework.
 *
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "sf_console_api.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Version of code that implements the API defined in this file */
#define SF_CONSOLE_CODE_VERSION_MAJOR  (2U)
#define SF_CONSOLE_CODE_VERSION_MINOR  (0U)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Console instance control block. DO NOT INITIALIZE.  Initialization occurs when sf_console_api_t::open is called */
typedef struct st_sf_console_instance_ctrl
{
    sf_console_menu_t   const * p_current_menu;   ///< Current menu is stored here.
    sf_comms_instance_t const * p_comms;          ///< Pointer to communications driver instance
    uint8_t                     new_line;         ///< Whether to echo input commands to transmitter
    bool                        echo;             ///< Whether to echo input commands to transmitter
    uint8_t                     input[SF_CONSOLE_MAX_INPUT_LENGTH]; ///< Input buffer used to store user input
} sf_console_instance_ctrl_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/
/** @cond INC_HEADER_DEFS_SEC */
/** Filled in Interface API structure for this Instance. */
extern const sf_console_api_t g_sf_console_on_sf_console;
/** @endcond */

/*******************************************************************************************************************//**
 * @} (end defgroup Console)
 **********************************************************************************************************************/
#endif /* SF_CONSOLE_H */
