#ifndef SF_CMD_COMMS_H
#define SF_CMD_COMMS_H

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "sf_comms_api.h"

typedef struct st_sf_rtt_comms_instance_ctrl
{

} sf_rtt_comms_instance_ctrl_t;

typedef struct st_sf_rtt_comms_cfg
{

} sf_rtt_comms_cfg_t;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
fsp_err_t SF_RTT_COMMS_Open(sf_comms_ctrl_t * const p_ctrl, sf_comms_cfg_t const * const p_cfg);
fsp_err_t SF_RTT_COMMS_Close(sf_comms_ctrl_t * const p_ctrl);
fsp_err_t SF_RTT_COMMS_Read(sf_comms_ctrl_t * const p_ctrl,
                       uint8_t * const p_dest,
                       uint32_t const bytes,
                       UINT const timeout);
fsp_err_t SF_RTT_COMMS_Write(sf_comms_ctrl_t * const p_ctrl,
                        uint8_t const * const p_src,
                        uint32_t const bytes,
                        UINT const timeout);
fsp_err_t SF_RTT_COMMS_Lock(sf_comms_ctrl_t * const p_ctrl, sf_comms_lock_t lock_type, UINT timeout);
fsp_err_t SF_RTT_COMMS_Unlock(sf_comms_ctrl_t * const p_ctrl, sf_comms_lock_t lock_type);

/******************************************************************************
 * EXPORTED GLOBALS
 *****************************************************************************/
extern const sf_comms_api_t g_sf_comms_on_sf_rtt_comms;

#endif // SF_CMD_COMMS_H
