/* generated common source file - do not edit */
#include "common_data.h"
ioport_instance_ctrl_t g_ioport_ctrl;
const ioport_instance_t g_ioport =
{ .p_api = &g_ioport_on_ioport, .p_ctrl = &g_ioport_ctrl, .p_cfg = &g_bsp_pin_cfg, };
TX_EVENT_FLAGS_GROUP g_cdcacm_event_flags0;
void tx_startup_err_callback(void *p_instance, void *p_data);
void g_common_init(void)
{
    UINT err_g_cdcacm_event_flags0;
    err_g_cdcacm_event_flags0 = tx_event_flags_create (&g_cdcacm_event_flags0, (CHAR*) "CDCACM Activate Event Flags");
    if (TX_SUCCESS != err_g_cdcacm_event_flags0)
    {
        tx_startup_err_callback (&g_cdcacm_event_flags0, 0);
    }
}
