/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <sf_rtt_comms.h>
#include <SEGGER/SEGGER_RTT.h>

/******************************************************************************
 * CONSTANTS
 *****************************************************************************/

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
const sf_comms_api_t g_sf_comms_on_sf_rtt_comms =
{
    .open          = SF_RTT_COMMS_Open,
    .close         = SF_RTT_COMMS_Close,
    .read          = SF_RTT_COMMS_Read,
    .write         = SF_RTT_COMMS_Write,
    .lock          = SF_RTT_COMMS_Lock,
    .unlock        = SF_RTT_COMMS_Unlock,
};

/******************************************************************************
 * FUNCTION: SF_RTT_COMMS_Open
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Open(sf_comms_ctrl_t * const p_ctrl, sf_comms_cfg_t const * const p_cfg)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    /* TODO: IMPLEMENT? */
    TX_PARAMETER_NOT_USED(p_ctrl);
    TX_PARAMETER_NOT_USED(p_cfg);

    return fsp_err;
}

/******************************************************************************
 * FUNCTION: SF_RTT_COMMS_Close
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Close(sf_comms_ctrl_t * const p_ctrl)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    /* TODO: IMPLEMENT? */
    TX_PARAMETER_NOT_USED(p_ctrl);

    return fsp_err;
}

/******************************************************************************
 * FUNCTION: SF_RTT_COMMS_Read
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Read(sf_comms_ctrl_t * const p_ctrl,
                       uint8_t * const p_dest,
                       uint32_t const bytes,
                       UINT const timeout)
{
    /* TODO: IMPLEMENT? */
    TX_PARAMETER_NOT_USED(p_ctrl);
    TX_PARAMETER_NOT_USED(timeout);

    fsp_err_t fsp_err = FSP_SUCCESS;
    int c;
    uint8_t * p_buffer = p_dest;
    uint32_t bytes_read = 0;

    while(1)
    {
        c = SEGGER_RTT_GetKey();
        if(c != -1)
        {
            *p_buffer++ = (uint8_t) c;
            bytes_read++;
        }

        if(bytes_read == bytes)
        {
            break;
        }
        else if(c == -1)
        {
            fsp_err = FSP_ERR_TIMEOUT;
            break;
        }
    }

    return fsp_err;
}

/******************************************************************************
 * FUNCTION: SF_RTT_COMMS_Write
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Write(sf_comms_ctrl_t * const p_ctrl,
                        uint8_t const * const p_src,
                        uint32_t const bytes,
                        UINT const timeout)
{
    /* TODO: IMPLEMENT? */
    TX_PARAMETER_NOT_USED(p_ctrl);
    TX_PARAMETER_NOT_USED(timeout);

    fsp_err_t fsp_err = FSP_SUCCESS;
    uint8_t const *p_buffer = p_src;
    uint32_t bytes_written = 0;

    while(1)
    {
        /* putchar does not support timeout */
        SEGGER_RTT_PutChar(0, (char)(*p_buffer++));
        bytes_written++;

        if(bytes_written == bytes)
        {
            break;
        }
    }

    return fsp_err;
}

/******************************************************************************
 * FUNCTION: SF_RTT_COMMS_Lock
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Lock(sf_comms_ctrl_t * const p_ctrl, sf_comms_lock_t lock_type, UINT timeout)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    /* TODO: IMPLEMENT? */
    TX_PARAMETER_NOT_USED(p_ctrl);
    TX_PARAMETER_NOT_USED(lock_type);
    TX_PARAMETER_NOT_USED(timeout);

    return fsp_err;
}

/******************************************************************************
 * FUNCTION: SF_RTT_COMMS_Unlock
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Unlock(sf_comms_ctrl_t * const p_ctrl, sf_comms_lock_t lock_type)
{
    fsp_err_t fsp_err = FSP_SUCCESS;

    /* TODO: IMPLEMENT? */
    TX_PARAMETER_NOT_USED(p_ctrl);
    TX_PARAMETER_NOT_USED(lock_type);

    return fsp_err;
}

