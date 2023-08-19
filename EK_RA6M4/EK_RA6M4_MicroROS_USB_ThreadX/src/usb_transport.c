#include <microros_transports.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <microros_thread.h>
#include <r_usb_pcdc_api.h>

#include <uxr/client/transport.h>
#include <uxr/client/util/time.h>
#include <rmw_microxrcedds_c/config.h>

#define CDCACM_FLAG                         ((ULONG) 0x0001)
#define CDCACM_ACTIVATE_FLAG                ((ULONG) 0x0004)
#define CDCACM_DEACTIVATE_FLAG              ((ULONG) 0x0008)
#define DEVICE_FRAME_LENGTH_HIGH_SPEED              (103U)
#define DEVICE_FRAME_LENGTH_FULL_SPEED              (93U)
#define STRING_FRAMEWORK_LENGTH                     (105U)
#define LANGUAGE_ID_FRAME_WORK_LENGTH               (2U)
#define CONFIG_NUMB                                 (1U)
#define INTERFACE_NUMB0                             (0x00)

#define MEMPOOL_SIZE                                (18432U)
#define BYTE_SIZE                                   (4U)

static uint32_t g_ux_pool_memory[MEMPOOL_SIZE / BYTE_SIZE];
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER g_ux_device_class_cdc_acm0_parameter;
static UX_SLAVE_CLASS_CDC_ACM * g_cdc;

extern uint8_t g_device_framework_full_speed[];
extern uint8_t g_device_framework_hi_speed[];
extern uint8_t g_language_id_framework[];
extern uint8_t g_string_framework[];

#ifdef RMW_UXRCE_TRANSPORT_CUSTOM

// Renesas FSP USB-PCDC handling

#define NUM_STRING_DESCRIPTOR   (7U)             /* String descriptor */
#define READ_BUF_SIZE           (8U)
#define LINE_CODING_LENGTH      (0x07U)

usb_class_t g_usb_class_type = 0x00;
usb_setup_t usb_setup;
usb_pcdc_linecoding_t g_line_coding;
// uint8_t g_buf[READ_BUF_SIZE] = {0};

extern uint8_t g_apl_device[];
extern uint8_t g_apl_configuration[];
extern uint8_t g_apl_hs_configuration[];
extern uint8_t g_apl_qualifier_descriptor[];
extern uint8_t * gp_apl_string_table[];

const usb_descriptor_t g_usb_descriptor =
{
    g_apl_device,                   /* Pointer to the device descriptor */
    g_apl_configuration,            /* Pointer to the configuration descriptor for Full-speed */
    g_apl_hs_configuration,         /* Pointer to the configuration descriptor for Hi-speed */
    g_apl_qualifier_descriptor,     /* Pointer to the qualifier descriptor */
    gp_apl_string_table,             /* Pointer to the string descriptor table */
    NUM_STRING_DESCRIPTOR
};

#define USB_NO_TIMEOUT -1

typedef enum handle_usb_operation_t {
    USB_NOOP,
    USB_WRITE,
    USB_READ,
} handle_usb_operation_t;

size_t handle_usb(handle_usb_operation_t op, uint8_t * buf, size_t len, int timeout);

uint8_t reading_buffer[1000];
size_t reading_buffer_size = 0;
size_t reading_buffer_ptr;

size_t handle_usb(handle_usb_operation_t op, uint8_t * buf, size_t len, int timeout)
{
#if 1
    UINT status = UX_SUCCESS;
    size_t actual_length = 0;

    if(op == USB_WRITE){
        status = ux_device_class_cdc_acm_write (g_cdc, buf, len, (ULONG *)&actual_length);
    } else if (op == USB_READ) {
        status = ux_device_class_cdc_acm_read (g_cdc, buf, len, (ULONG *)&actual_length);
    }

#else
    usb_status_t event = {0};
    fsp_err_t err = FSP_SUCCESS;

    if(op == USB_WRITE){
        err = R_USB_Write(&g_basic0_ctrl, buf, len, (uint8_t)g_usb_class_type);
    } else if (op == USB_READ) {
        err = R_USB_Read(&g_basic0_ctrl, reading_buffer, sizeof(reading_buffer), (uint8_t)g_usb_class_type);
    }

    int64_t start = uxr_millis();
    while (timeout == USB_NO_TIMEOUT || (uxr_millis() -  start < timeout))
    {
        err = R_USB_EventGet(&g_basic0_ctrl, &event);
        if (FSP_SUCCESS != err){ return 0;}

        switch (event)
        {
            case USB_STATUS_CONFIGURED:
            case USB_STATUS_WRITE_COMPLETE:
                if (op == USB_WRITE) {
                    return len;
                }
                break;
            case USB_STATUS_READ_COMPLETE:
                if (op == USB_READ) {
                    return g_basic0_ctrl.data_size;
                }
                break;
            case USB_STATUS_REQUEST:   /* Receive Class Request */
                R_USB_SetupGet(&g_basic0_ctrl, &usb_setup);

                /* Check for the specific CDC class request IDs */
                if (USB_PCDC_SET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataGet(&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_GET_LINE_CODING == (usb_setup.request_type & USB_BREQUEST))
                {
                    err =  R_USB_PeriControlDataSet(&g_basic0_ctrl, (uint8_t *) &g_line_coding, LINE_CODING_LENGTH );
                    if (FSP_SUCCESS != err){ return 0;}
                }
                else if (USB_PCDC_SET_CONTROL_LINE_STATE == (usb_setup.request_type & USB_BREQUEST))
                {
                    err = R_USB_PeriControlStatusSet(&g_basic0_ctrl, USB_SETUP_STATUS_ACK);
                    if (FSP_SUCCESS != err){ return 0;}
                }
                break;
            case USB_STATUS_SUSPEND:
            case USB_STATUS_DETACH:
            default:
                break;
        }
    }
#endif
    return 0;
}

// micro-ROS USB-PCDC transports

bool renesas_e2_transport_open(struct uxrCustomTransport * transport){
    (void) transport;

    /* To check ux api return status */
    UINT status = UX_SUCCESS;

    /* ux_system_initialization */
    status = ux_system_initialize(g_ux_pool_memory, MEMPOOL_SIZE, UX_NULL, 0);
    if (UX_SUCCESS != status)
    {
        SEGGER_RTT_printf(0, "microros_thread_entry::ux_system_initialize failed, status = %d", status);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "ux_system_initialize success!\r\n");
    }

    /* ux_device stack initialization */
    status = ux_device_stack_initialize(g_device_framework_hi_speed,
                                        DEVICE_FRAME_LENGTH_HIGH_SPEED,
                                        g_device_framework_full_speed,
                                        DEVICE_FRAME_LENGTH_FULL_SPEED,
                                        g_string_framework,
                                        STRING_FRAMEWORK_LENGTH,
                                        g_language_id_framework,
                                        LANGUAGE_ID_FRAME_WORK_LENGTH,
                                        &usbx_status_callback);
    if (UX_SUCCESS != status)
    {
        SEGGER_RTT_printf(0, "microros_thread_entry::ux_device_stack_initialize failed, status = %d", status);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "ux_device_stack_initialize success!\r\n");
    }

    /* The activate command is used when the host has sent a SET_CONFIGURATION command
     and this interface has to be mounted. Both Bulk endpoints have to be mounted
     and the cdc_acm thread needs to be activated.  */
    g_ux_device_class_cdc_acm0_parameter.ux_slave_class_cdc_acm_instance_activate = ux_cdc_device0_instance_activate;

    /* The deactivate command is used when the device has been extracted.
     The device endpoints have to be dismounted and the cdc_acm thread canceled.  */
    g_ux_device_class_cdc_acm0_parameter.ux_slave_class_cdc_acm_instance_deactivate = ux_cdc_device0_instance_deactivate;

    /* ux_device stack class registration */
    status = ux_device_stack_class_register (_ux_system_slave_class_cdc_acm_name, _ux_device_class_cdc_acm_entry, CONFIG_NUMB,
                                             INTERFACE_NUMB0, (void*) &g_ux_device_class_cdc_acm0_parameter);
    if (UX_SUCCESS != status)
    {
        SEGGER_RTT_printf(0, "microros_thread_entry::ux_device_stack_class_register failed, status = %d", status);
        APP_ERR_TRAP();
    }
    else
    {
        SEGGER_RTT_printf(0, "ux_device_stack_class_register success!\r\n");
    }

    /* Initialize the peripheral mode according to the USB speed selection */
    if (USB_SPEED_FS == g_basic0_cfg.usb_speed)
    {
        /* Peri mode initialization with Full Speed */
        status = usb_peri_usbx_initialize (R_USB_FS0_BASE);
    }
    else if (USB_SPEED_HS == g_basic0_cfg.usb_speed)
    {
        /* Peri mode initialization with HIGH Speed */
        status = usb_peri_usbx_initialize (R_USB_HS0_BASE);
    }
    else
    {
        /* do nothing */
    }

    R_USB_Open(&g_basic0_ctrl, &g_basic0_cfg);
    return true;
}

bool renesas_e2_transport_close(struct uxrCustomTransport * transport){
    (void) transport;

    R_USB_Close(&g_basic0_ctrl);
    return true;
}

size_t renesas_e2_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * error){
    (void) transport;
    (void) error;

    return handle_usb(USB_WRITE, (uint8_t *) buf, len, WRITE_TIMEOUT);
}

size_t renesas_e2_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* error){
    (void) transport;
    (void) error;

    size_t readed = 0;

    if (reading_buffer_size == 0) {
        reading_buffer_size = handle_usb(USB_READ, NULL, 0, timeout);
        reading_buffer_ptr = 0;
    }

    while (reading_buffer_ptr < reading_buffer_size && readed < len) {
        buf[readed] = reading_buffer[reading_buffer_ptr];
        reading_buffer_ptr++;
        readed++;
    }

    if (reading_buffer_ptr == reading_buffer_size)
    {
        reading_buffer_size = 0;
    }

    return readed;
}

/*******************************************************************************************************************//**
 * @brief     In this function, it activates the cdc instance.
 * @param[IN] cdc_instance    Pointer to the area store the instance pointer
 * @retval    none
 **********************************************************************************************************************/
static void ux_cdc_device0_instance_activate (void * cdc_instance)
{
    /* Save the CDC instance.  */
    g_cdc = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
    tx_event_flags_set(&g_cdcacm_event_flags0, CDCACM_ACTIVATE_FLAG, TX_OR);
}

/*******************************************************************************************************************//**
 * @brief     In this function, it deactivates the cdc instance.
 * @param[IN] cdc_instance    Pointer to area store the instance pointer
 * @retval    none
 **********************************************************************************************************************/
static void ux_cdc_device0_instance_deactivate (void * cdc_instance)
{
    FSP_PARAMETER_NOT_USED(cdc_instance);
    g_cdc = UX_NULL;
    tx_event_flags_set(&g_cdcacm_event_flags0, CDCACM_DEACTIVATE_FLAG, TX_OR);
}

/*******************************************************************************************************************//**
 * @brief     In this function, usb callback events will be captured into one variable.
 * @param[IN] status    Whenever any event occurred, status gets update
 * @retval    zero
 **********************************************************************************************************************/
UINT usbx_status_callback (ULONG status)
{
    switch (status)
    {
        case UX_DEVICE_ATTACHED:
        {
            tx_event_flags_set(&g_cdcacm_event_flags0, CDCACM_FLAG, TX_OR);
        }
        break;

        case UX_DEVICE_REMOVED:
        {
            tx_event_flags_set(&g_cdcacm_event_flags0, ~CDCACM_FLAG, TX_AND);
        }
        break;

        default:
        {
            /* do nothing */
        }
        break;
    }
    return 0;
}



#endif //RMW_UXRCE_TRANSPORT_CUSTOM
