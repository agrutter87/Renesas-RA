/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "console.h"
#include "application.h"

/******************************************************************************
 * FUNCTION: feature_start_callback
 *****************************************************************************/
void feature_start_callback(sf_console_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    SEGGER_RTT_printf(0, "Starting feature...");

    tx_thread_sleep(100);

    SEGGER_RTT_printf(0, "done\r\n");
}

/******************************************************************************
 * FUNCTION: feature_stop_callback
 *****************************************************************************/
void feature_stop_callback(sf_console_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    SEGGER_RTT_printf(0, "Stopping feature...");

    tx_thread_sleep(100);

    SEGGER_RTT_printf(0, "done\r\n");
}

/******************************************************************************
 * FUNCTION: feature_get_status_callback
 *****************************************************************************/
void feature_status_callback(sf_console_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    SEGGER_RTT_printf(0, "Getting feature status...\n");

    ULONG               feature_count   = g_application.p_cfg->feature_count;
    feature_status_t    status          = { 0 };

    SEGGER_RTT_printf(0, "|                          Feature |   Status   |\n");
    SEGGER_RTT_printf(0, "|----------------------------------|------------|\n");

    for(uint32_t feature_num = 0; feature_num < feature_count; feature_num++)
    {
        /* Do something */
        g_application.p_cfg->p_features[feature_num].feature_get_status(&status);
        SEGGER_RTT_printf(0, "| %32s | %10d |\n", g_application.p_cfg->p_features[feature_num].feature_name, status.return_code);
    }

    SEGGER_RTT_printf(0, "done\r\n");
}

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif /* M_PI */
/******************************************************************************
 * FUNCTION: custom_code_callback
 *****************************************************************************/
void custom_code_callback(sf_console_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    double vacuum_permeability = 4 * M_PI * 0.0000001; // T m/A
    double max_current = 2.0; // A
    double current_inc = max_current / 10.0;
    double distance_from_wire = 0.005; // m

    SEGGER_RTT_printf(0, "Starting custom code...\n");
    SEGGER_RTT_printf(0, "|   B(uT)   | Dist(mm) | Current(A) |\n");
    SEGGER_RTT_printf(0, "|-----------|----------|------------|\n");

    for(double current = current_inc; current < max_current; current += current_inc)
    {
        for(distance_from_wire = 0.0001; distance_from_wire < 0.0010; distance_from_wire +=0.0001)
        {
            double magnetic_field = (vacuum_permeability * current) / (2 * M_PI * distance_from_wire) * 1000000;
            SEGGER_RTT_printf(0, "| %8.3f | %8.1f | %10.1f |\n", magnetic_field, distance_from_wire * 1000, current);
        }
    }

    SEGGER_RTT_printf(0, "done\r\n");
}

