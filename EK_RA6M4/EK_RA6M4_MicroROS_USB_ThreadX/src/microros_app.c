#include "./utils.h"

#include <time.h>

#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

#include <SEGGER_RTT.h>

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){bool l = true; while(1){set_led_status(LED_RED, l = !l); sleep_ms(100);}}}
#define APP_ERR_TRAP() {bool l = true; while(1){set_led_status(LED_RED, l = !l); sleep_ms(100);}};

void microros_app(void);
void subscription_callback(const void * msgin);

bool g_new_data    = false;

void microros_app(void)
{
    rcl_ret_t               rc          = RCL_RET_OK;
    rcl_allocator_t         allocator   = rcl_get_default_allocator();
    rcl_publisher_t         publisher   = { 0 };
    rcl_subscription_t      subscriber  = { 0 };
    rclc_executor_t         executor    = rclc_executor_get_zero_initialized_executor();

    std_msgs__msg__Int32    send_msg    = { 0 };
    std_msgs__msg__Int32    recv_msg    = { 0 };

    //create init_options
    rclc_support_t support;
    rclc_support_init(&support, 0, NULL, &allocator);

    // create nodes
	rcl_node_t node;
	rc = rclc_node_init_default(&node,
	                            "my_renesas_node",
	                            "",
	                            &support);
	if(rc != RCL_RET_OK)
	{
	    SEGGER_RTT_printf(0, "Failed microros_app::rclc_node_init_default, rc = %d!\r\n", rc);
	    APP_ERR_TRAP();
	}
	else
	{
	    SEGGER_RTT_printf(0, "rclc_node_init_default success!\r\n");
	}

    // create publisher
    rc = rclc_publisher_init_default(&publisher,
                                     &node,
                                     ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                                     "int_publisher");
    if(rc != RCL_RET_OK)
    {
        SEGGER_RTT_printf(0, "Failed microros_app::rclc_publisher_init_default, rc = %d!\r\n", rc);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "rclc_publisher_init_default success!\r\n");
    }

    // create subscriber
    rc = rclc_subscription_init_default(&subscriber,
                                        &node,
                                        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
                                        "int_subscriber");
    if(rc != RCL_RET_OK)
    {
        SEGGER_RTT_printf(0, "Failed microros_app::rclc_subscription_init_default, rc = %d!\r\n", rc);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "rclc_subscription_init_default success!\r\n");
    }

    rc = rclc_executor_init(&executor,
                            &support.context,
                            1,
                            &allocator);
    if(rc != RCL_RET_OK)
    {
        SEGGER_RTT_printf(0, "Failed microros_app::rclc_executor_init, rc = %d!\r\n", rc);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "rclc_executor_init success!\r\n");
    }

    rc = rclc_executor_add_subscription(&executor,
                                        &subscriber,
                                        &recv_msg,
                                        &subscription_callback,
                                        ON_NEW_DATA);
    if(rc != RCL_RET_OK)
    {
        SEGGER_RTT_printf(0, "Failed microros_app::rclc_executor_add_subscription, rc = %d!\r\n", rc);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "rclc_executor_add_subscription success!\r\n");
    }

    while(1)
    {
        rclc_executor_spin_some(&executor, 1000 * 1000);
        if(g_new_data)
        {
            g_new_data = false;
            send_msg.data = recv_msg.data;
        }

        rc = rcl_publish(&publisher, &send_msg, NULL);
        if(rc != RCL_RET_OK)
        {
            SEGGER_RTT_printf(0, "Failed microros_app::rcl_publish, rc = %d!\r\n", rc);
        }

        send_msg.data++;

        sleep_ms(100);
    }
}

// Implementation example:
void subscription_callback(const void * msgin)
{
    // Cast received message to used type
    const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;

    // Process message
    SEGGER_RTT_printf(0, "Received: %d\n", msg->data);

    g_new_data = true;
}
