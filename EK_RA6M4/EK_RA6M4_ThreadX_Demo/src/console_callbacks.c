/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "console.h"
#include "application.h"

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
extern const console_t g_console;

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

    UINT tx_err = TX_SUCCESS;
    char *p_buffer = NULL;

    SEGGER_RTT_printf(0, "Getting feature status...\n");

    ULONG               feature_count   = g_application.p_cfg->feature_count;
    feature_status_t    status          = { 0 };

    SEGGER_RTT_printf(0, "|                          Feature |   Status   |                Threads(Priority) | Stack Utilization |\n");
    SEGGER_RTT_printf(0, "|----------------------------------|------------|----------------------------------|-------------------|\n");

    tx_err = tx_byte_allocate(g_console.p_ctrl->p_memory_byte_pool, (VOID **)&p_buffer, 128, TX_NO_WAIT);
    if(TX_SUCCESS == tx_err)
    {
        snprintf(p_buffer, 128, "| %32s |            |                                  | %7lu / %7lu |\n",
                "Application Memory Pool",
                (g_application.p_ctrl->memory_byte_pool.tx_byte_pool_size
                        - g_application.p_ctrl->memory_byte_pool.tx_byte_pool_available),
                g_application.p_ctrl->memory_byte_pool.tx_byte_pool_size,
                status.pp_thread[0]->tx_thread_stack_size);
        SEGGER_RTT_Write(0, p_buffer, strlen(p_buffer));
        tx_byte_release(p_buffer);
    }

    for(uint32_t feature_num = 0; feature_num < feature_count; feature_num++)
    {
        tx_err = tx_byte_allocate(g_console.p_ctrl->p_memory_byte_pool, (VOID **)&p_buffer, 128, TX_NO_WAIT);
        if(TX_SUCCESS == tx_err)
        {
            /* Do something */
            g_application.p_cfg->p_features[feature_num].feature_get_status(&status);
            if(status.pp_thread == 0)
            {
                snprintf(p_buffer, 128, "| %32s | %10lu | %32s |                   |\n",
                        g_application.p_cfg->p_features[feature_num].feature_name,
                        status.return_code,
                        "No Threads");
                SEGGER_RTT_Write(0, p_buffer, strlen(p_buffer));
            }
            else
            {
                snprintf(p_buffer, 128, "| %32s | %10lu | %28s(%2u) | %7lu / %7lu |\n",
                        g_application.p_cfg->p_features[feature_num].feature_name,
                        status.return_code,
                        status.pp_thread[0]->tx_thread_name,
                        status.pp_thread[0]->tx_thread_priority,
                        ((ULONG)status.pp_thread[0]->tx_thread_stack_end
                                - (ULONG)status.pp_thread[0]->tx_thread_stack_highest_ptr),
                        status.pp_thread[0]->tx_thread_stack_size);
                SEGGER_RTT_Write(0, p_buffer, strlen(p_buffer));
            }

            if(status.thread_count > 1)
            {
                for(uint8_t thread_index = 1; thread_index < status.thread_count; thread_index++)
                {
                    tx_err = tx_byte_allocate(g_console.p_ctrl->p_memory_byte_pool, (VOID **)&p_buffer, 128, TX_NO_WAIT);
                    if(TX_SUCCESS == tx_err)
                    {
                        snprintf(p_buffer, 128, "|                                  |            | %28s(%2u) | %7lu / %7lu |\n",
                                status.pp_thread[thread_index]->tx_thread_name,
                                status.pp_thread[thread_index]->tx_thread_priority,
                                ((ULONG)status.pp_thread[thread_index]->tx_thread_stack_end
                                        - (ULONG)status.pp_thread[thread_index]->tx_thread_stack_highest_ptr),
                                status.pp_thread[thread_index]->tx_thread_stack_size);
                        SEGGER_RTT_Write(0, p_buffer, strlen(p_buffer));
                    }
                }
            }

            tx_byte_release(p_buffer);
        }
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

