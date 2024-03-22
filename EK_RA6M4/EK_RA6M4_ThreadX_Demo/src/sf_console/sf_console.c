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
/*************************************************************************//*******************************************
* File Name    : sf_console.c
* Description  : Console Framwork (SF_CONSOLE) Module driver file.
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include <ctype.h>
#include "sf_console.h"
#include "sf_console_cfg.h"
#include "sf_console_private_api.h"
#include "tx_api.h"

#define FSP_SUCCESS (0)
#define FSP_ERROR_RETURN(a, err)                        \
    {                                                   \
        if ((a))                                        \
        {                                               \
            (void) 0;                  /* Do nothing */ \
        }                                               \
        else                                            \
        {                                               \
            FSP_ERROR_LOG(err);                         \
            return err;                                 \
        }                                               \
    }
#define FSP_ERROR_LOG(err)

/** Macro for error logger. */
#ifndef SF_CONSOLE_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define SF_CONSOLE_ERROR_RETURN(a, err)  FSP_ERROR_RETURN((a), (err))
#endif

#define CR_CODE                 ((uint8_t) '\r')
#define LF_CODE                 ((uint8_t) '\n')
#define NULL_CODE               ((uint8_t) '\0')
#define SPACE_CODE              ((uint8_t) ' ')
#define HELP_CODE               ((uint8_t) '?')
#define BACKSPACE_CODE          ((uint8_t) 0x8)
#define DELETE_CODE             ((uint8_t) 0x7f)

/** Begins escape code sequence, which is used for arrow key input.  Example: Up arrow key input = "\e[A" */
#ifdef __ICCARM__
/* IAR generates a warning because the first character of the escape code is not valid by itself.  To parse
 * the escape code one character at a time, we suppress this warning. */
#pragma diag_suppress=Pe192
#endif
#define ESC_CODE_1              ((uint8_t) 0x1B)
#define ESC_CODE_2              ((uint8_t) '[')    /** Second character in escape code sequence. */
#define UP_ARROW_CODE           ((uint8_t) 'A')    /* Valid after ESC codes */
#define DOWN_ARROW_CODE         ((uint8_t) 'B')    /* Valid after ESC codes */
#define RIGHT_ARROW_CODE        ((uint8_t) 'C')    /* Valid after ESC codes */
#define LEFT_ARROW_CODE         ((uint8_t) 'D')    /* Valid after ESC codes */


/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/
typedef enum e_sf_console_cursor_dir
{
    SF_CONSOLE_CURSOR_DIR_LEFT,
    SF_CONSOLE_CURSOR_DIR_RIGHT
} sf_console_cursor_dir_t;

/***********************************************************************************************************************
Private function prototypes
***********************************************************************************************************************/
static int32_t check_for_match(uint8_t const * const p_test, uint8_t const * const p_ref);
static uint32_t check_for_overflow(sf_console_instance_ctrl_t * const p_ctrl, uint32_t * p_index, uint32_t const bytes);
static uint32_t insert_char(sf_console_instance_ctrl_t * const p_ctrl,
                             uint8_t                    * const p_input,
                             uint8_t                            ch,
                             uint32_t                           index,
                             uint32_t                           last_index);
static uint32_t delete_char(sf_console_instance_ctrl_t * p_ctrl, uint8_t * p_input, uint32_t index, uint32_t * p_last_index);
static void move_cursor(sf_console_instance_ctrl_t * p_ctrl, sf_console_cursor_dir_t dir, uint32_t num_spaces);
static uint32_t print_help_menu(sf_console_instance_ctrl_t * const p_ctrl,
                                 sf_console_menu_t    const * const p_menu,
                                 UINT                               timeout);
static void sf_console_continue_parsing(sf_console_instance_ctrl_t * const p_ctrl,
                                        uint8_t              const * const p_input,
                                        uint32_t                     const bytes);
static void sf_console_call_callback(sf_console_instance_ctrl_t * const p_ctrl,
                                     void                      (*p_callback)(sf_console_callback_args_t * p_args),
                                     void                 const *       p_context,
                                     uint8_t              const * const p_input,
                                     uint32_t                     const bytes,
                                     int32_t                            length);
static uint32_t sf_console_read_process_up_arrow(sf_console_instance_ctrl_t * const p_ctrl,
                                             uint8_t                    * const p_dest,
                                             uint32_t                   *       p_index,
                                             uint32_t                   *       p_length);
static uint32_t sf_console_read_process_escape(sf_console_instance_ctrl_t * const p_ctrl,
                                                uint8_t                    * const p_dest,
                                                uint32_t                   *       p_index,
                                                uint32_t                   *       p_length);
static uint32_t sf_console_read_process_backspace(sf_console_instance_ctrl_t * const p_ctrl,
                                                   uint8_t                    * const p_dest,
                                                   uint32_t                   *       p_index,
                                                   uint32_t                   *       p_length);
static bool sf_console_read_process_new_line(sf_console_instance_ctrl_t * const p_ctrl,
                                             uint8_t                    * const p_dest,
                                             uint32_t                           length,
                                             uint8_t                      const rx);
static uint32_t sf_console_read_main(sf_console_instance_ctrl_t * const p_ctrl,
                                      uint8_t                    * const p_dest,
                                      uint32_t                     const bytes,
                                      uint32_t                     const timeout);

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
static uint32_t sf_console_parse_param_check(sf_console_instance_ctrl_t * const p_ctrl,
                                              sf_console_menu_t const * const p_menu,
                                              uint8_t           const * const p_input);
#endif

/***********************************************************************************************************************
Private global variables
***********************************************************************************************************************/
/* String used for white space. */
static const uint8_t g_white_space[] =
{
    SPACE_CODE,
    SPACE_CODE,
    SPACE_CODE,
    SPACE_CODE,
};

#if defined(__GNUC__)
/* This structure is affected by warnings from a GCC compiler bug. This pragma suppresses the warnings in this
 * structure only.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

/***********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/
/*LDRA_NOANALYSIS LDRA_INSPECTED below not working. */
/*LDRA_INSPECTED 27 D This structure must be accessible in user code. It cannot be static. */
const sf_console_api_t g_sf_console_on_sf_console = 
{
    .open         = SF_CONSOLE_Open,
    .close        = SF_CONSOLE_Close,
    .prompt       = SF_CONSOLE_Prompt,
    .parse        = SF_CONSOLE_Parse,
    .read         = SF_CONSOLE_Read,
    .write        = SF_CONSOLE_Write,
    .argumentFind = SF_CONSOLE_ArgumentFind,
};
/*LDRA_ANALYSIS */

/*******************************************************************************************************************//**
 * @addtogroup Console
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
Functions
***********************************************************************************************************************/

/******************************************************************************************************************//**
 * @brief Initialize console framework and open low-level communication driver.
 *
 * @retval  FSP_SUCCESS          Console channel is successfully opened
 * @retval  FSP_ERR_ASSERTION    Parameter check failed for one of the following :
 * @retval                       -Pointer to the control block is NULL
 * @retval                       -Pointer to the config block is NULL
 * @retval                       -Pointer p_cfg->p_initial_menu is NULL
 * @retval                       -Pointer p_cfg->p_comms is NULL
 * @retval                       -Pointer p_cfg->p_comms->p_api is NULL
 * @retval                       -Pointer p_cfg->p_comms->p_api->open is NULL
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_Open (sf_console_ctrl_t * const p_api_ctrl, sf_console_cfg_t const * const p_cfg)
{
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_api_ctrl;

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_cfg);
    FSP_ASSERT(NULL != p_cfg->p_initial_menu);
    FSP_ASSERT(NULL != p_cfg->p_comms);
    FSP_ASSERT(NULL != p_cfg->p_comms->p_api);
    FSP_ASSERT(NULL != p_cfg->p_comms->p_api->open);
#endif

    /** Open UART driver */
    uint32_t err;
    err = p_cfg->p_comms->p_api->open(p_cfg->p_comms->p_ctrl, p_cfg->p_comms->p_cfg);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    /** Store echo configuration and initial menu in control block */
    p_ctrl->echo = p_cfg->echo;
    p_ctrl->p_current_menu = p_cfg->p_initial_menu;
    p_ctrl->p_comms = p_cfg->p_comms;

    /** Prompt for input autostart is true */
    if (p_cfg->autostart)
    {
        err = SF_CONSOLE_Prompt(p_ctrl, p_cfg->p_initial_menu, SF_CONSOLE_PRV_TIMEOUT, true);
        SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    return FSP_SUCCESS;
}  /* End of function SF_CONSOLE_Open() */

/******************************************************************************************************************//**
 * @brief Close the communications driver.
 *
 * @retval FSP_SUCCESS           Console successfully closed
 * @retval FSP_ERR_ASSERTION     Pointer to control block is NULL
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_Close (sf_console_ctrl_t * const p_api_ctrl)
{
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_api_ctrl;

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
#endif

    /** Close UART driver */
    uint32_t err;
    err = p_ctrl->p_comms->p_api->close(p_ctrl->p_comms->p_ctrl);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    return FSP_SUCCESS;
}  /* End of function SF_CONSOLE_Close() */

/******************************************************************************************************************//**
 * @brief Looks for input string in menu, and calls callback function if found.
 *
 * @retval FSP_SUCCESS           Data parsed successfully, command found and callback called.
 * @retval FSP_ERR_UNSUPPORTED   Command not found in the current menu.
 * @retval FSP_ERR_ASSERTION     One or more input parameter pointers are invalid.
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note   This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_Parse(sf_console_ctrl_t       * const p_api_ctrl,
                           sf_console_menu_t const * const p_menu,
                           uint8_t           const * const p_input,
                           uint32_t                  const bytes)
{
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_api_ctrl;

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    /* Parameter check.*/
    if(FSP_SUCCESS != sf_console_parse_param_check(p_ctrl, p_menu, p_input))
    {
        return FSP_ERR_ASSERTION;
    }
#endif

    /** Print help menu if help command is entered */
    if (check_for_match(p_input, (uint8_t *) SF_CONSOLE_HELP_COMMAND))
    {
        uint32_t err;
        err = print_help_menu(p_ctrl, p_menu, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
        return FSP_SUCCESS;
    }

    /** Go back one menu if previous menu command is entered. */
    if (check_for_match(p_input, (uint8_t *) SF_CONSOLE_MENU_PREVIOUS_COMMAND))
    {
        if (NULL != p_ctrl->p_current_menu->menu_prev)
        {
            p_ctrl->p_current_menu = p_ctrl->p_current_menu->menu_prev;

            sf_console_continue_parsing(p_ctrl, p_input, bytes);
        }
        return FSP_SUCCESS;
    }

    /** Go back to root menu if root menu command is entered. */
    if (check_for_match(p_input, (uint8_t *) SF_CONSOLE_ROOT_MENU_COMMAND))
    {
        while (NULL != p_ctrl->p_current_menu->menu_prev)
        {
            p_ctrl->p_current_menu = p_ctrl->p_current_menu->menu_prev;
        }

        sf_console_continue_parsing(p_ctrl, p_input, bytes);
        return FSP_SUCCESS;
    }

    /** Look for matching commands, call callback if command found. */
    for (uint32_t i = 0U; i < p_menu->num_commands; i++)
    {
        int32_t length = check_for_match(p_input, p_menu->command_list[i].command);
        if (length > 0)
        {
            /* Match found, call callback if its not null, then return success. */
            sf_console_call_callback(p_ctrl, p_menu->command_list[i].callback, p_menu->command_list[i].context,
                    p_input, bytes, length);
            return FSP_SUCCESS;
        }
    }

    /* No valid command found, return error. */
    FSP_ERROR_LOG(FSP_ERR_UNSUPPORTED);
    return (FSP_ERR_UNSUPPORTED);

}  /* End of function SF_CONSOLE_Parse() */

/******************************************************************************************************************//**
 * @brief Prompt the user to input a command.
 *
 * @retval FSP_SUCCESS           Received valid command and called callback
 * @retval FSP_ERR_ASSERTION     p_ctrl is NULL
 * @retval FSP_ERR_UNSUPPORTED   Command not found in the current menu.
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_Prompt(   sf_console_ctrl_t * const         p_api_ctrl,
                                sf_console_menu_t const * const p_menu,
                                UINT const                      timeout,
                                bool                            print_menu)
{
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_api_ctrl;

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
#endif

    /** Update stored current menu pointer if a new pointer is specified. */
    if (NULL != p_menu)
    {
        p_ctrl->p_current_menu = p_menu;
    }

    if(print_menu)
    {
        /** Print menu name followed by ">" to prompt for user input. */
        SF_CONSOLE_Write(p_ctrl, p_ctrl->p_current_menu->menu_name, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_Write(p_ctrl, (uint8_t *) ">", SF_CONSOLE_PRV_TIMEOUT);
    }

    /** Lock the console UART framework to reserve exclusive access until the command completes.
     *  @note Transmission is only locked while the menu name is printed and while the input command is non-zero in length.
     *  This allow debug messages to print from other threads while echo is off or no input command has been entered. */
    uint32_t err;
    err = p_ctrl->p_comms->p_api->lock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_RX, timeout);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    /** Wait for input */
    err = SF_CONSOLE_Read(p_ctrl, &(p_ctrl->input[0]), SF_CONSOLE_MAX_INPUT_LENGTH, timeout);
    if (FSP_SUCCESS != err)
    {
        /* Error. Unlock comms reception. */
        p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_RX);
    }
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    /** Parse input and call associated user callback. */
    if (0U != p_ctrl->input[0])
    {
        err = SF_CONSOLE_Parse(p_ctrl, p_ctrl->p_current_menu, &p_ctrl->input[0], SF_CONSOLE_MAX_INPUT_LENGTH);
        if (FSP_ERR_UNSUPPORTED == err)
        {
            SF_CONSOLE_Write(p_ctrl, (uint8_t *) "Unsupported ", SF_CONSOLE_PRV_TIMEOUT);
            SF_CONSOLE_Write(p_ctrl, p_ctrl->p_current_menu->menu_name, SF_CONSOLE_PRV_TIMEOUT);
            SF_CONSOLE_Write(p_ctrl, (uint8_t *) " Command\r\n", SF_CONSOLE_PRV_TIMEOUT);
        }
    }

    /** Command is complete, so unlock comms reception. */
    p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_RX);

    return err;
}  /* End of function SF_CONSOLE_Prompt() */

/******************************************************************************************************************//**
 * @brief Reads data into the destination byte by byte and echos input to the console.
 *
 * @retval FSP_SUCCESS           Data read completed successfully
 * @retval FSP_ERR_ASSERTION     Parameter check failed for one of the following :
 * @retval                       -Pointer p_dest is NULL
 * @retval                       -Pointer to the control block is NULL
 * @retval                       -Pointer p_ctrl->p_comms is NULL
 * @retval                       -Pointer p_ctrl->p_comms->p_api is NULL
 * @retval                       -Pointer p_ctrl->p_comms->p_api->lock is NULL
 * @retval                       -Pointer p_ctrl->p_comms->p_api->unlock is NULL
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_Read ( sf_console_ctrl_t * const p_api_ctrl,
                            uint8_t           * const p_dest,
                            uint32_t            const bytes,
                            uint32_t            const timeout)
{
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_api_ctrl;

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_dest);
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_ctrl->p_comms);
    FSP_ASSERT(NULL != p_ctrl->p_comms->p_api);
    FSP_ASSERT(NULL != p_ctrl->p_comms->p_api->lock);
    FSP_ASSERT(NULL != p_ctrl->p_comms->p_api->unlock);
#endif

    /** Lock the communications framework reception until carriage return is received. */
    uint32_t err;
    err = p_ctrl->p_comms->p_api->lock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_RX, timeout);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    /** Read one byte at a time, checking for carriage returns, backspace, delete, and escape codes. */
    err = sf_console_read_main(p_ctrl, p_dest, bytes, timeout);

    /** Unlock the communications framework reception */
    p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_RX);

    return err;
}  /* End of function SF_CONSOLE_Read() */

/******************************************************************************************************************//**
 * @brief Write a NULL terminated string to the console.
 *
 * @retval FSP_SUCCESS           Data write completed successfully
 * @retval FSP_ERR_ASSERTION     Pointer to the control block is NULL
 * @retval FSP_ERR_INVALID_SIZE  If length passed is zero or length passed is greater than 128U (max value).
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_Write (  sf_console_ctrl_t * const p_api_ctrl,
                            uint8_t const *  const p_src,
                            uint32_t         const timeout)
{
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_api_ctrl;

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_ctrl);
#endif

    /** Write null terminated string.  Calculate the length.  If it isn't longer than the maximum, write the entire
     *  string to the console. */
    uint32_t length = strlen((char *) p_src);
    SF_CONSOLE_ERROR_RETURN(0U != length, FSP_ERR_INVALID_SIZE);
    SF_CONSOLE_ERROR_RETURN(length < SF_CONSOLE_MAX_WRITE_LENGTH, FSP_ERR_INVALID_SIZE);
    uint32_t err = p_ctrl->p_comms->p_api->write(p_ctrl->p_comms->p_ctrl, p_src, length, timeout);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    return FSP_SUCCESS;
}  /* End of function SF_CONSOLE_Write() */

/******************************************************************************************************************//**
 * @brief Finds a command line argument in an input string and returns the index of the character immediately
 *        following the argument and any string numbers converted to integers.
 *
 * @retval FSP_SUCCESS           Argument found successfully
 * @retval FSP_ERR_ASSERTION     p_arg or p_str is NULL
 * @retval FSP_ERR_INTERNAL      String passed after parsing command is NULL_CODE.
 * @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
 * @note This function is reentrant for any channel.
***********************************************************************************************************************/
uint32_t SF_CONSOLE_ArgumentFind (uint8_t const *        const p_arg,
                            uint8_t const *        const p_str,
                            int32_t *       const p_index,
                            int32_t *       const p_data)
{
#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(NULL != p_arg);
    FSP_ASSERT(NULL != p_str);
#endif

    int32_t i = 0;

    while (p_str[i] != NULL_CODE)
    {
        /** Search for first letter match at beginning of word. */
        if ((0 == i) || (SPACE_CODE == p_str[i - 1]))
        {
            if (toupper(p_str[i]) == toupper(p_arg[0]))
            {
                /** If the input string matches the input argument, store the index of the character following the
                 *  argument in p_index and the data at that index in p_data.  Then return. */
                int32_t length = check_for_match(&p_str[i], &p_arg[0]);
                if (length)
                {
                    int32_t index = i + length;
                    if (NULL != p_index)
                    {
                        *p_index = index;
                    }
                    if (NULL != p_data)
                    {
                        *p_data = atoi((char *) &p_str[index]);
                    }
                    return FSP_SUCCESS;
                }
            }
        }
        i++;
    }

    *p_index = -1;
    *p_data = -1;

    return FSP_ERR_INTERNAL;
}  /* End of function SF_CONSOLE_ArgumentFind() */

/******************************************************************************************************************//**
 * @brief Callback provided to continue parsing the next menu down.
 *
 * @param[in] p_args  Pointer to callback arguments to use in the next menu.
***********************************************************************************************************************/
void SF_CONSOLE_CallbackNextMenu(sf_console_callback_args_t * p_args)
{
    /** Next level menu is passed in as the user context parameter */
    sf_console_menu_t * p_menu = (sf_console_menu_t *) p_args->context;

    /** Update current menu. */
    sf_console_instance_ctrl_t * p_ctrl = (sf_console_instance_ctrl_t *) p_args->p_ctrl;
    p_ctrl->p_current_menu = p_menu;

    /** Check to see if the next menu command was input in the remaining string. */
    if (NULL_CODE != p_args->p_remaining_string[0])
    {
        SF_CONSOLE_Parse(p_args->p_ctrl, p_menu, p_args->p_remaining_string, p_args->bytes);
    }
}

/** @} (end addtogroup Console) */

/***********************************************************************************************************************
Private Functions
***********************************************************************************************************************/
/******************************************************************************************************************//**
* @brief  Checks to see if a test string matches a reference string.
* @note   This function is insensitive to case.
* @param[in]  p_test    String to look for
* @param[in]  p_ref     Reference string
* @return  Length of reference string if there is a match
***********************************************************************************************************************/
static int32_t check_for_match(uint8_t const * const p_test, uint8_t const * const p_ref)
{
    int32_t i = 0;

    /** Compare characters until they don't match or end of the reference string is reached. */
    while (NULL_CODE != (p_ref[i]))
    {
        /** Convert to upper case.  This console is not case sensitive. */
        int32_t temp1 = toupper((int32_t) p_test[i]);
        int32_t temp2 = toupper((int32_t) p_ref[i]);
        if (temp1 != temp2)
        {
            return 0;
        }

        i++;
    }

    /** Check for the end of input string and a non-space character */
    if((NULL_CODE != p_test[i]) && (SPACE_CODE != p_test[i]))
    {
        return 0;
    }

    return i;
}  /* End of function check_for_match */

/******************************************************************************************************************//**
* @brief  Deletes character at input index and shifts the rest of the string left.
* @pre    Lock the UART framework before calling this function.
* @param[in]      p_ctrl        Console control block, used to print to the console if echo mode is on.
* @param[in,out]  p_input       Input string to remove character from.
* @param[in]      index         Index of character to remove
* @param[in,out]  p_last_index  Index after the last valid character in the input string.
* @retval         FSP_SUCCESS   Deleting characters from console is processed successfully.
* @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes
***********************************************************************************************************************/
static uint32_t delete_char(sf_console_instance_ctrl_t * p_ctrl, uint8_t * p_input, uint32_t index, uint32_t * p_last_index)
{
    /** Character can only be deleted if it is not the last character (index < last_index) */
    uint32_t last_index = *p_last_index;
    if (index < last_index)
    {
        /* Save start pointer to print it later. */
        uint8_t * p_start = &p_input[index];

        /** Shift characters left in input string */
        while (index < (last_index - 1))
        {
            p_input[index] = p_input[index + 1];
            index++;
        }

        /** Store a space at the end to erase the last character on the terminal */
        p_input[index] = SPACE_CODE;
        p_input[last_index] = NULL_CODE;
        if (p_ctrl->echo)
        {
            /* Print shifted string, then move cursor back to start index. */
            uint32_t err;
            err = SF_CONSOLE_Write(p_ctrl, p_start, SF_CONSOLE_PRV_TIMEOUT);
            if (FSP_SUCCESS == err)
            {
                move_cursor(p_ctrl, SF_CONSOLE_CURSOR_DIR_LEFT, strlen((char *) p_start));
            }
        }
        p_input[index] = NULL_CODE;
        last_index--;
        *p_last_index = last_index;
        if ((0U == last_index) && (p_ctrl->echo))
        {
            /** Unlock transmission to allow debug messages if there is no input.  Only needed when echo is on. */
            uint32_t err = p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX);
            SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
        }
    }

    return FSP_SUCCESS;
} /* End of function delete_char */

/******************************************************************************************************************//**
* @brief  Inserts character at input index and shifts the rest of the string right.
* @pre    Lock the UART framework before calling this function.
* @param[in]  p_ctrl       Console control block, used to print to the console if echo mode is on.
* @param[in]  p_input      Input string to insert character into.
* @param[in]  ch           Character to insert
* @param[in]  index        Index to insert character
* @param[in]  last_index   Index after the last valid character in the input string.  This will be the index of the last
*                          valid character in the string after this function is complete.
* @retval     FSP_SUCCESS  Character has been inserted or processed successfully.
* @return                  See @ref Common_Error_Codes or lower level drivers for other possible return codes
***********************************************************************************************************************/
static uint32_t insert_char(sf_console_instance_ctrl_t * const p_ctrl,
                             uint8_t                    * const p_input,
                             uint8_t                            ch,
                             uint32_t                           index,
                             uint32_t                           last_index)
{
    /* Save start pointer to print it later. */
    uint8_t * p_start = &p_input[index];

    /** Shift characters right in input string */
    uint32_t temp = last_index;
    while (temp > index)
    {
        p_input[temp] = p_input[temp - 1];
        temp--;
    }

    /** Insert character */
    p_input[index] = ch;

    /** Add a null character to terminate the string. */
    p_input[last_index + 1] = NULL_CODE;
    if (p_ctrl->echo)
    {
        /* Print shifted string, then move cursor back to index after inserted character. */
        uint32_t err;
        if (0U == last_index)
        {
            /** When the first byte is received, lock transmission if echo is on. */
            err = p_ctrl->p_comms->p_api->lock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX, SF_CONSOLE_PRV_TIMEOUT);
            SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
        }
        err = SF_CONSOLE_Write(p_ctrl, p_start, SF_CONSOLE_PRV_TIMEOUT);
        if (FSP_SUCCESS == err)
        {
            move_cursor(p_ctrl, SF_CONSOLE_CURSOR_DIR_LEFT, (strlen((char *) p_start) - 1));
        }
    }

    return FSP_SUCCESS;
} /* End of function insert_char */

/******************************************************************************************************************//**
* @brief  Moves cursor in the direction specified by the number of spaces specified.
* @pre    Lock the UART framework before calling this function.
* @param[in]  p_ctrl       Console control block, used to print to the console if echo mode is on.
* @param[in]  dir         Select to move cursor right or left.
* @param[in]  num_spaces  Number of spaces to move cursor.
***********************************************************************************************************************/
static void move_cursor(sf_console_instance_ctrl_t * p_ctrl, sf_console_cursor_dir_t dir, uint32_t num_spaces)
{
    if (p_ctrl->echo)
    {
        /** Prepare escape code to send */
        uint8_t buf[4];
        buf[0] = ESC_CODE_1;
        buf[1] = ESC_CODE_2;
        if (SF_CONSOLE_CURSOR_DIR_LEFT == dir)
        {
            buf[2] = LEFT_ARROW_CODE;
        }
        else
        {
            /* SF_CONSOLE_CURSOR_DIR_RIGHT == dir */
            buf[2] = RIGHT_ARROW_CODE;
        }
        buf[3] = NULL_CODE;

        /** Send the escape code num_spaces times */
        for (uint32_t i = 0U; i < num_spaces; i++)
        {
            SF_CONSOLE_Write(p_ctrl, &buf[0], SF_CONSOLE_PRV_TIMEOUT);
        }
    }
} /* End of function move_cursor */

/******************************************************************************************************************//**
* @brief  Prints help menu to console.
* @param[in]  p_ctrl      Console control block, used to print to the console if echo mode is on.
* @param[in]  p_menu      Menu to print help for.
* @param[in]  timeout     Timeout value
* @retval FSP_SUCCESS     Prints help menu to console successfully.
* @return                 See @ref Common_Error_Codes or lower level drivers for other possible return codes
***********************************************************************************************************************/
static uint32_t print_help_menu(sf_console_instance_ctrl_t * const p_ctrl, sf_console_menu_t const * const p_menu,
        UINT timeout)
{
    /** Lock console UART channel to reserve exclusive access for multiple writes */
    uint32_t err;
    err = p_ctrl->p_comms->p_api->lock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX, timeout);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    /** Print "<MENU_NAME> Help Menu" indented by 2 spaces. */
    p_ctrl->p_comms->p_api->write(p_ctrl->p_comms->p_ctrl, &g_white_space[0], 2, SF_CONSOLE_PRV_TIMEOUT);
    SF_CONSOLE_Write(p_ctrl, p_menu->menu_name, SF_CONSOLE_PRV_TIMEOUT);
    SF_CONSOLE_Write(p_ctrl, (uint8_t *) " Help Menu\r\n", SF_CONSOLE_PRV_TIMEOUT);

    /** If it is possible to go back one menu, print this option and the home option. */
    if (NULL != p_ctrl->p_current_menu->menu_prev)
    {
    	p_ctrl->p_comms->p_api->write(p_ctrl->p_comms->p_ctrl, &g_white_space[0], 4, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_Write(p_ctrl, SF_CONSOLE_ROOT_MENU_COMMAND, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_Write(p_ctrl, (uint8_t *) " : Back to root menu\r\n", SF_CONSOLE_PRV_TIMEOUT);
        p_ctrl->p_comms->p_api->write(p_ctrl->p_comms->p_ctrl, &g_white_space[0], 4, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_Write(p_ctrl, SF_CONSOLE_MENU_PREVIOUS_COMMAND, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_Write(p_ctrl, (uint8_t *) " : Up one menu level\r\n", SF_CONSOLE_PRV_TIMEOUT);
    }

    /** Print each command followed by the associated help string if one is provided. Commands are
     *  indented by 4 spaces and followed by carriage return and newline characters. */
    for (uint32_t i = 0U; i < p_menu->num_commands; i++)
    {
    	p_ctrl->p_comms->p_api->write(p_ctrl->p_comms->p_ctrl, &g_white_space[0], 4, SF_CONSOLE_PRV_TIMEOUT);
        SF_CONSOLE_Write(p_ctrl, p_menu->command_list[i].command, SF_CONSOLE_PRV_TIMEOUT);
        if (NULL != p_menu->command_list[i].help)
        {
            SF_CONSOLE_Write(p_ctrl, (uint8_t *) " : ", SF_CONSOLE_PRV_TIMEOUT);
            SF_CONSOLE_Write(p_ctrl, p_menu->command_list[i].help, SF_CONSOLE_PRV_TIMEOUT);
        }
        SF_CONSOLE_Write(p_ctrl, (uint8_t *) "\r\n", SF_CONSOLE_PRV_TIMEOUT);
    }
    SF_CONSOLE_Write(p_ctrl, (uint8_t *) "\r\n", SF_CONSOLE_PRV_TIMEOUT);

    /** Unlock console UART channel after all writes are complete. */
    p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX);

    return FSP_SUCCESS;
} /* End of function print_help_menu */


/******************************************************************************************************************//**
* @brief  Finds the end of the current string, then parses the remaining string.
* @param[in]  p_ctrl      Console control block, passed to the parse command to parse the remaining string.
* @param[in]  p_input     Used to search for the end of the current command, which marks the beginning of the remaining
*                         string.
* @param[in]  bytes       The total number of bytes in the input string p_input
***********************************************************************************************************************/
static void sf_console_continue_parsing(sf_console_instance_ctrl_t * const p_ctrl,
                                        uint8_t              const * const p_input,
                                        uint32_t                     const bytes)
{

    if ((bytes > 0U) && (NULL_CODE != p_input[0]))
    {
        uint32_t length = 1U;
        /* Find the end of the string or the beginning of the next word to pass to the callback. */
        while (((NULL_CODE != p_input[length]) && (SPACE_CODE != p_input[length - 1])) && (length < bytes))
        {
            length++;
        }
        if (NULL_CODE != p_input[length])
        {
            SF_CONSOLE_Parse(p_ctrl, p_ctrl->p_current_menu, &p_input[length], bytes - length);
        }
    }
}

/******************************************************************************************************************//**
* @brief  Finds the end of the current string, then passes the remaining string to the callback.
* @param[in]  p_ctrl      Console control block, passed in the callback arguments
* @param[in]  p_callback  The callback function to call
* @param[in]  p_context   The user context to pass in the callback arguments
* @param[in]  p_input     Used to search for the end of the current command, which marks the beginning of the remaining
*                         string.
* @param[in]  bytes       The total number of bytes in the input string p_input
* @param[in]  length      The starting index to search for the end of the current argument
***********************************************************************************************************************/
static void sf_console_call_callback(sf_console_instance_ctrl_t * const p_ctrl,
                                     void                      (*       p_callback)(sf_console_callback_args_t * p_args),
                                     void                 const *       p_context,
                                     uint8_t              const * const p_input,
                                     uint32_t                     const bytes,
                                     int32_t                            length)
{
    if (NULL != p_callback)
    {
        /* Find the end of the string or the beginning of the next word to pass to the callback. */
        while (((NULL_CODE != p_input[length]) && (SPACE_CODE != p_input[length - 1])) && ((uint32_t) length < SF_CONSOLE_MAX_INPUT_LENGTH))
        {
            length++;
        }

        /* Call user provided callback */
        sf_console_cb_args_t args;
        args.p_ctrl = p_ctrl;
        args.context = p_context;
        args.p_remaining_string = &p_input[length];
        args.bytes = bytes - (uint32_t) length;

        if (SF_CONSOLE_CALLBACK_NEXT_FUNCTION == p_callback)
        {
            SF_CONSOLE_CallbackNextMenu(&args);
        }
        else
        {
            p_callback(&args);
        }
    }
}

/******************************************************************************************************************//**
* @brief  Processes up arrow key input to the console.
* @param[in,out]  p_ctrl       Console control block
* @param[in,out]  p_dest       The destination buffer where input data is stored
* @param[in,out]  p_index      The cursor index in the destination buffer
* @param[in,out]  p_length     The length of the destination buffer (in bytes)
* @retval         FSP_SUCCESS  Up arrow processed successfully.
* @return                      See @ref Common_Error_Codes or lower level drivers for other possible return codes.
***********************************************************************************************************************/
static uint32_t sf_console_read_process_up_arrow(sf_console_instance_ctrl_t * const p_ctrl,
                                             uint8_t                    * const p_dest,
                                             uint32_t                   *       p_index,
                                             uint32_t                   *       p_length)
{
    /* Restore persistent value from last input.  This only works if no input has been entered. */
    if ((&p_ctrl->input[0]) == (&p_dest[*p_index]))
    {
        uint32_t len = strlen((char *) p_dest);
        uint32_t err;
        if ((len > 0U) && (p_ctrl->echo))
        {
            /** Lock transmission if echo is on. */
            err = p_ctrl->p_comms->p_api->lock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX, SF_CONSOLE_PRV_TIMEOUT);
            SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
            SF_CONSOLE_Write(p_ctrl, p_dest, SF_CONSOLE_PRV_TIMEOUT);
        }
        *p_index += len;
        *p_length += len;
    }
    return FSP_SUCCESS;
}

/******************************************************************************************************************//**
* @brief  Processes escape code (arrow key) inputs to the console.
* @param[in,out]  p_ctrl            Console control block
* @param[in,out]  p_dest            The destination buffer where input data is stored
* @param[in,out]  p_index           The cursor index in the destination buffer
* @param[in,out]  p_length          The length of the destination buffer (in bytes)
* @retval         FSP_SUCCESS       Received Escape code processed successfully.
* @retval         FSP_ERR_INTERNAL  Incorrect Escape code received.
***********************************************************************************************************************/
static uint32_t sf_console_read_process_escape(sf_console_instance_ctrl_t * const p_ctrl,
                                                uint8_t                    * const p_dest,
                                                uint32_t                   *       p_index,
                                                uint32_t                   *       p_length)
{
    /* Received ESC code. Expect 2 more characters '[', then differentiating character. */
    uint8_t rxbuf[2] = {0, 0};
    uint32_t err;
    err = p_ctrl->p_comms->p_api->read(p_ctrl->p_comms->p_ctrl, (uint8_t *)&rxbuf, 2, SF_CONSOLE_PRV_TIMEOUT);
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
    SF_CONSOLE_ERROR_RETURN(ESC_CODE_2 == rxbuf[0], FSP_ERR_INTERNAL);

    switch (rxbuf[1])
    {
    case LEFT_ARROW_CODE :
    {
        /* Move cursor left if cursor isn't already at the start index. */
        if ((0U != *p_length) && (*p_index > 0U))
        {
            move_cursor(p_ctrl, SF_CONSOLE_CURSOR_DIR_LEFT, 1U);
            (*p_index)--;
        }
        break;
    }
    case RIGHT_ARROW_CODE:
    {
        /* Move cursor right if cursor isn't already at the last index. */
        if (*p_index < *p_length)
        {
            move_cursor(p_ctrl, SF_CONSOLE_CURSOR_DIR_RIGHT, 1U);
            (*p_index)++;
        }
        break;
    }
    case UP_ARROW_CODE:
    {
        /* Restore persistent value from last input.  This only works if no input has been entered. */
        err = sf_console_read_process_up_arrow(p_ctrl, p_dest, p_index, p_length);
        SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
    }
    break;
    default:
    break;
    }

    return FSP_SUCCESS;
}

/******************************************************************************************************************//**
* @brief  Processes backspace inputs to the console.
* @param[in,out]  p_ctrl      Console control block
* @param[in,out]  p_dest      The destination buffer where input data is stored
* @param[in,out]  p_index     The current cursor index in the destination buffer
* @param[in,out]  p_length    The length of the destination buffer (in bytes)
* @retval         FSP_SUCCESS Backspace input to the console is processed successfully.
***********************************************************************************************************************/
static uint32_t sf_console_read_process_backspace(sf_console_instance_ctrl_t * const p_ctrl,
                                              uint8_t                    * const p_dest,
                                              uint32_t                   *       p_index,
                                              uint32_t                   *       p_length)
{
    /* Remove character before the current index if cursor isn't already at the start index. */
    if(0U != *p_index)
    {
        if (p_ctrl->echo)
        {
            /* This moves the cursor backwards. */
            SF_CONSOLE_Write(p_ctrl, (uint8_t *) "\b", SF_CONSOLE_PRV_TIMEOUT);
        }
        (*p_index)--;

        /* This actually removes the character from the input buffer and updates the terminal view. */
        uint32_t err = delete_char(p_ctrl, p_dest, *p_index, p_length);
        SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    return FSP_SUCCESS;
}

/******************************************************************************************************************//**
* @brief  Processes new line inputs (CR, LF, and NULL) to the console.
* @param[in]  p_ctrl      Console control block
* @param[in]  p_dest      The destination buffer where input data is stored
* @param[in]  length      The length of the destination buffer (in bytes)
* @param[in]  rx          The new line character received (CR, LF, or NULL)
* @retval     true        New line input processed successfully.
* @retval     false       Incorrect input for new line.
***********************************************************************************************************************/
static bool sf_console_read_process_new_line(sf_console_instance_ctrl_t * const p_ctrl,
                                             uint8_t                    * const p_dest,
                                             uint32_t                           length,
                                             uint8_t                      const rx)
{
    if ((CR_CODE == p_ctrl->new_line) && ((LF_CODE == rx) || (NULL_CODE == rx)))
    {
        /* Ignore LF or NULL if CR is the last new line character. */
        p_ctrl->new_line = rx;
        return false;
    }

    p_ctrl->new_line = rx;
    if (p_ctrl->echo)
    {
        /* Carriage return indicates the end of the string.  Print carriage return and newline. */
        SF_CONSOLE_Write(p_ctrl, (uint8_t *) "\r\n", SF_CONSOLE_PRV_TIMEOUT);
    }

    /* Terminate the string with a null character. */
    p_dest[length] = NULL_CODE;

    return true;
}

#if SF_CONSOLE_CFG_PARAM_CHECKING_ENABLE
/******************************************************************************************************************//**
* @brief  Validate input parameters.
* @param[in]  p_ctrl            Console control block.
* @param[in]  p_menu            Pointer to a menu of valid input commands for this prompt.
* @param[in]  p_input           Pointer to a null terminated string to search for in the command list.
*
* @retval     FSP_SUCCESS       All the input parameter pointers are valid.
* @retval     FSP_ERR_ASSERTION One or more input parameter pointers are invalid.
***********************************************************************************************************************/
static uint32_t sf_console_parse_param_check(sf_console_instance_ctrl_t * const p_ctrl,
                                              sf_console_menu_t const * const p_menu,
                                              uint8_t           const * const p_input)
{
    FSP_ASSERT(NULL != p_ctrl);
    FSP_ASSERT(NULL != p_menu);
    FSP_ASSERT(NULL != p_input);

    return FSP_SUCCESS;
}
#endif

/******************************************************************************************************************//**
* @brief  Processes one input byte.
* @param[in,out]  p_ctrl             Console control block
* @param[in,out]  p_dest             The destination buffer where input data is stored
* @param[in]      rx                 The received byte
* @param[in,out]  p_index            The cursor index in the destination buffer
* @param[in,out]  p_length           The length of the destination buffer (in bytes)
* @param[out]     p_read_complete    Whether or not the read is complete
* @retval         FSP_SUCCESS        One input byte is processed successfully.
***********************************************************************************************************************/
static uint32_t sf_console_read_process_byte(sf_console_instance_ctrl_t * const p_ctrl,
        uint8_t                    * const p_dest,
        uint8_t                      const rx,
        uint32_t                   *       p_index,
        uint32_t                   *       p_length,
        bool                       *       p_read_complete)
{
    uint32_t err = FSP_SUCCESS;
    *p_read_complete = false;

    /* Interpret input byte */
    switch (rx)
    {
    case ESC_CODE_1:
    {
        err = sf_console_read_process_escape(p_ctrl, p_dest, p_index, p_length);
        break;
    }
    case BACKSPACE_CODE:
    {
        err = sf_console_read_process_backspace(p_ctrl, p_dest, p_index, p_length);
        break;
    }
    case DELETE_CODE:
    {
        /* Remove character at the current index if cursor isn't already at the last index. */
        err = delete_char(p_ctrl, p_dest, *p_index, p_length);
        break;
    }
    case LF_CODE:
    case NULL_CODE:
    case CR_CODE:
    {
        *p_read_complete = sf_console_read_process_new_line(p_ctrl, p_dest, *p_length, rx);
        break;
    }
    default:
    {
        /* Insert character */
        err = insert_char(p_ctrl, p_dest, rx, *p_index, *p_length);

        /* Increment the index and length. */
        (*p_index)++;
        (*p_length)++;
        break;
    }
    }

    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
    return FSP_SUCCESS;
}

/******************************************************************************************************************//**
* @brief  Processes all input to the console.
* @param[in]  p_ctrl            Console control block
* @param[in]  p_dest            The destination buffer where input data is stored
* @param[in]  bytes             The length of the destination buffer (in bytes)
* @param[in]  timeout           The timeout accepted to read a single byte (in ThreadX ticks)
* @retval     FSP_SUCCESS       One byte of data is read successfully.
* @retval     FSP_ERR_OVERFLOW  Input buffer is overflowed.
* @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
***********************************************************************************************************************/
static uint32_t sf_console_read_main(sf_console_instance_ctrl_t * const p_ctrl,
                                      uint8_t                    * const p_dest,
                                      uint32_t                     const bytes,
                                      uint32_t                     const timeout)
{
    /** Read one byte at a time, checking for carriage returns, backspace, delete, and escape codes. */
    uint32_t err = FSP_SUCCESS;
    uint8_t rx;
    uint32_t index = 0;
    uint32_t length = 0;
    while (true)
    {
        /* Read a single byte. Return error if read fails. */
        err = p_ctrl->p_comms->p_api->read(p_ctrl->p_comms->p_ctrl, &rx, 1, timeout);
        SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

        bool read_complete = false;
        err = sf_console_read_process_byte(p_ctrl, p_dest, rx, &index, &length, &read_complete);

        if (read_complete)
        {
            /** Break out of the loop when read is finished. */
            break;
        }

        err = check_for_overflow (p_ctrl, &index, bytes);
        if (FSP_SUCCESS != err)
        {
            break;
        }
    }
    SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);

    if ((p_ctrl->echo) && (index > 0U))
    {
        /** Unlock transmission to allow debug messages.  Only needed when echo is on. */
        err = p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX);
        SF_CONSOLE_ERROR_RETURN(FSP_SUCCESS == err, err);
    }

    return FSP_SUCCESS;
}

/******************************************************************************************************************//**
* @brief  Checks to see overflow error condition.
* @note   This function is insensitive to case.
* @param[in]  p_ctrl    Console control block
* @param[in]  p_index   The cursor index in the destination buffer
* @param[in]  bytes     The length of the destination buffer (in bytes)
* @retval     FSP_SUCCESS       One byte of data is read successfully.
* @retval     FSP_ERR_OVERFLOW  Input buffer is overflowed.
* @return                       See @ref Common_Error_Codes or lower level drivers for other possible return codes.
***********************************************************************************************************************/
static uint32_t check_for_overflow(sf_console_instance_ctrl_t * const p_ctrl,uint32_t *p_index,uint32_t const bytes)
{
    uint32_t err = FSP_SUCCESS;
    if (*p_index >= (bytes - 1U))
    {
        /* Input buffer overflowed, return error. */
        /* One byte is reserved for a NULL terminating character. */
        if (p_ctrl->echo)
        {
            /** Unlock transmission to allow debug messages.  Only needed when echo is on. */
            p_ctrl->p_comms->p_api->unlock(p_ctrl->p_comms->p_ctrl, SF_COMMS_LOCK_TX);
        }
        err = FSP_ERR_OVERFLOW;
    }
    return err;
}

/* End of file */
