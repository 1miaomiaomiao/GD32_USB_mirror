/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-11-16     ZYH          first version
 */
#include <rthw.h>
#include <rtdevice.h>
#include <drivers/usb_device.h>
#include "winusb.h"
/* SWD-readable data-path counters (bulk stream / touch debugging). */
volatile uint32_t g_dbg_scr_arm = 0;    /* win_usb_read arms */
volatile uint32_t g_dbg_touch_arm = 0;  /* win_usb_write arms */
volatile uint32_t g_dbg_touch_drop = 0; /* win_usb_write dropped (in_busy) */
/* Called from _function_enable (bus configured / re-configured after a
 * reset). The USB2SCR control thread uses it to re-announce the current
 * host mode after a host (re)connect, which lets the steady-state be
 * completely silent: no bulk-IN traffic while streaming, so the IN and
 * OUT arming paths never overlap and the DWC2 core stays stable. */
void (*g_winusb_enable_cb)(void) = RT_NULL;
struct winusb_device
{
    struct rt_device parent;
    void (*cmd_handler)(rt_uint8_t *buffer,rt_size_t size);
    rt_uint8_t cmd_buff[256];
    uep_t ep_out;
    uep_t ep_in;
    /* Set while a bulk-IN transfer is armed; cleared on completion.
     * rt_usbd_io_request() has no in-flight guard, so arming a second IN
     * transfer before the host polled the first one re-programs an already
     * enabled DWC2 IN endpoint (DIEPTLEN/DIEPCTL while EPEN) which wedges
     * the USB core and the host sees the device vanish. Callers must drop
     * the packet when this is set. */
    volatile rt_uint8_t in_busy;
    /* Serializes the two arming threads (disp thread -> bulk-OUT read,
     * ctl thread -> bulk-IN write). Both end up doing read-modify-write
     * cycles on DWC2 endpoint registers (DOEPCTL / DIEPCTL + masks) from
     * THREAD context; two concurrent arms can lose an update and wedge the
     * core -- observed as a Windows "pipe error" seconds into a mirror
     * stream, followed by the device failing to re-enumerate. The ISRs
     * only flip software flags (remain_size / in_busy), never these
     * registers, so a thread-level mutex is sufficient. */
    rt_mutex_t xfer_lock;
};
#define WINUSB_INTF_STR_INDEX 13
typedef struct winusb_device * winusb_device_t;

ALIGN(4)
static struct udevice_descriptor dev_desc =
{
    USB_DESC_LENGTH_DEVICE,     //bLength;
    USB_DESC_TYPE_DEVICE,       //type;
    USB_BCD_VERSION,            //bcdUSB;
    0x00,                       //bDeviceClass;
    0x00,                       //bDeviceSubClass;
    0x00,                       //bDeviceProtocol;
    0x40,                       //bMaxPacketSize0;
    _VENDOR_ID,                 //idVendor;
    _PRODUCT_ID,                //idProduct;
    USB_BCD_DEVICE,             //bcdDevice;
    USB_STRING_MANU_INDEX,      //iManufacturer;
    USB_STRING_PRODUCT_INDEX,   //iProduct;
    USB_STRING_SERIAL_INDEX,    //iSerialNumber;
    USB_DYNAMIC,                //bNumConfigurations;
};

//FS and HS needed
ALIGN(4)
static struct usb_qualifier_descriptor dev_qualifier =
{
    sizeof(dev_qualifier),          //bLength
    USB_DESC_TYPE_DEVICEQUALIFIER,  //bDescriptorType
    0x0200,                         //bcdUSB
    0xFF,                           //bDeviceClass
    0x00,                           //bDeviceSubClass
    0x00,                           //bDeviceProtocol
    64,                             //bMaxPacketSize0
    0x01,                           //bNumConfigurations
    0,
};

ALIGN(4)
struct winusb_descriptor _winusb_desc =
{
#ifdef RT_USB_DEVICE_COMPOSITE
    /* Interface Association Descriptor */
    {
        USB_DESC_LENGTH_IAD,
        USB_DESC_TYPE_IAD,
        USB_DYNAMIC,
        0x01,
        0xFF,
        0x00,
        0x00,
        0x00,
    },
#endif
    /*interface descriptor*/
    {
        USB_DESC_LENGTH_INTERFACE,  //bLength;
        USB_DESC_TYPE_INTERFACE,    //type;
        USB_DYNAMIC,                //bInterfaceNumber;
        0x00,                       //bAlternateSetting;
        0x02,                       //bNumEndpoints
        0xFF,                       //bInterfaceClass;
        0x00,                       //bInterfaceSubClass;
        0x00,                       //bInterfaceProtocol;
#ifdef RT_USB_DEVICE_COMPOSITE
        WINUSB_INTF_STR_INDEX,
#else
        0x00,                       //iInterface;
#endif
    },
    /*endpoint descriptor*/
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_OUT,
        USB_EP_ATTR_BULK,
        USB_DYNAMIC,
        0x00,
    },
    /*endpoint descriptor*/
    {
        USB_DESC_LENGTH_ENDPOINT,
        USB_DESC_TYPE_ENDPOINT,
        USB_DYNAMIC | USB_DIR_IN,
        USB_EP_ATTR_BULK,
        USB_DYNAMIC,
        0x00,
    },
};

ALIGN(4)
const static char* _ustring[] =
{
    "Language",
    "RT-Thread Team.",
    "RTT Win USB",
    "32021919830108",
    "Configuration",
    "Interface",
    USB_STRING_OS//must be
};

ALIGN(4)
struct usb_os_proerty winusb_proerty[] =
{
    USB_OS_PROPERTY_DESC(USB_OS_PROPERTY_TYPE_REG_SZ,"DeviceInterfaceGUID",RT_WINUSB_GUID),
};

ALIGN(4)
struct usb_os_function_comp_id_descriptor winusb_func_comp_id_desc =
{
    .bFirstInterfaceNumber = USB_DYNAMIC,
    .reserved1          = 0x01,
    .compatibleID       = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00},
    .subCompatibleID    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .reserved2          = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static rt_err_t _ep_out_handler(ufunction_t func, rt_size_t size)
{
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    if(winusb_device->parent.rx_indicate != RT_NULL)
    {
        winusb_device->parent.rx_indicate(&winusb_device->parent, size);
    }
    return RT_EOK;
}

static rt_err_t _ep_in_handler(ufunction_t func, rt_size_t size)
{
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    winusb_device->in_busy = 0;
    if(winusb_device->parent.tx_complete != RT_NULL)
    {
        winusb_device->parent.tx_complete(&winusb_device->parent, winusb_device->ep_in->buffer);
    }
    return RT_EOK;
}
static ufunction_t cmd_func = RT_NULL;
static rt_err_t _ep0_cmd_handler(udevice_t device, rt_size_t size)
{
    winusb_device_t winusb_device;

    if(cmd_func != RT_NULL)
    {
        winusb_device = (winusb_device_t)cmd_func->user_data;
        cmd_func = RT_NULL;
        if(winusb_device->cmd_handler != RT_NULL)
        {
            winusb_device->cmd_handler(winusb_device->cmd_buff,size);
        }
    }
    dcd_ep0_send_status(device->dcd);
    return RT_EOK;
}
static rt_err_t _ep0_cmd_read(ufunction_t func, ureq_t setup)
{
    winusb_device_t winusb_device = (winusb_device_t)func->user_data;
    cmd_func = func;
    rt_usbd_ep0_read(func->device,winusb_device->cmd_buff,setup->wLength,_ep0_cmd_handler);
    return RT_EOK;
}
static rt_err_t _interface_handler(ufunction_t func, ureq_t setup)
{
    switch(setup->bRequest)
    {
    case 'A':
        switch(setup->wIndex)
        {
        case 0x05:
            usbd_os_proerty_descriptor_send(func,setup,winusb_proerty,sizeof(winusb_proerty)/sizeof(winusb_proerty[0]));
            break;
        }
        break;
    case 0x0A://customer
        _ep0_cmd_read(func, setup);
        break;
    }

    return RT_EOK;
}
static rt_err_t _function_enable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);
    if(g_winusb_enable_cb != RT_NULL)
    {
        g_winusb_enable_cb();
    }
    return RT_EOK;
}
static rt_err_t _function_disable(ufunction_t func)
{
    RT_ASSERT(func != RT_NULL);
    /* Bus reset / unconfigure: any armed IN transfer is gone, make sure
     * the busy guard cannot stick and block future touch packets. */
    ((winusb_device_t)func->user_data)->in_busy = 0;
    return RT_EOK;
}

static struct ufunction_ops ops =
{
    _function_enable,
    _function_disable,
    RT_NULL,
};

static rt_err_t _winusb_descriptor_config(winusb_desc_t winusb, rt_uint8_t cintf_nr, rt_uint8_t device_is_hs)
{
#ifdef RT_USB_DEVICE_COMPOSITE
    winusb->iad_desc.bFirstInterface = cintf_nr;
#endif
    winusb->ep_out_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    winusb->ep_in_desc.wMaxPacketSize = device_is_hs ? 512 : 64;
    winusb_func_comp_id_desc.bFirstInterfaceNumber = cintf_nr;
    return RT_EOK;
}

static rt_size_t win_usb_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    if(((ufunction_t)dev->user_data)->device->state != USB_STATE_CONFIGURED)
    {
        return 0;
    }
    winusb_device_t winusb_device = (winusb_device_t)dev;
    udevice_t device = ((ufunction_t)dev->user_data)->device;

    if(winusb_device->xfer_lock == RT_NULL)
    {
        return 0;
    }
    rt_mutex_take(winusb_device->xfer_lock, RT_WAITING_FOREVER);

    /* Guard against double-arm: if the previous OUT transfer has not
     * completed (remain_size != 0), the DWC2 DOEPnCTL EPEN bit is still set.
     * Re-writing DOEPLEN/DOEPCTL while EPEN=1 wedges the OUT endpoint (no
     * more transfer-complete interrupts) and eventually forces a bus reset.
     * Return 0 so the caller can retry later. */
    if(winusb_device->ep_out->request.remain_size != 0)
    {
        rt_mutex_release(winusb_device->xfer_lock);
        return 0;
    }

    winusb_device->ep_out->buffer = buffer;
    winusb_device->ep_out->request.buffer = buffer;
    winusb_device->ep_out->request.size = size;
    winusb_device->ep_out->request.remain_size = size;
    winusb_device->ep_out->request.req_type = UIO_REQUEST_READ_FULL;

    /* Arm the whole bulk-OUT transfer in one DCD call. The generic
     * rt_usbd_io_request() path clamps every arm to a single wMaxPacketSize
     * (512 B at HS) and re-arms from the USB device thread only after the
     * packet interrupt -> message -> thread round-trip, which throttles HS
     * bulk to well under 1 MB/s. The GD32 OTG FIFO path drains packets in
     * the RXFIFO ISR into xfer_buf and signals transfer-complete once, so a
     * multi-packet arm streams at wire speed with one completion per URB. */
    dcd_ep_read_prepare(device->dcd,
                        winusb_device->ep_out->ep_desc->bEndpointAddress,
                        buffer, size);
    g_dbg_scr_arm++;
    rt_mutex_release(winusb_device->xfer_lock);
    return size;
}
static rt_size_t win_usb_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    if(((ufunction_t)dev->user_data)->device->state != USB_STATE_CONFIGURED)
    {
        return 0;
    }
    winusb_device_t winusb_device = (winusb_device_t)dev;
    /* Drop the packet if the previous IN transfer is still pending (host
     * not polling). Re-arming an enabled IN endpoint wedges the DWC2 core. */
    if(winusb_device->xfer_lock == RT_NULL || winusb_device->in_busy)
    {
        g_dbg_touch_drop++;
        return 0;
    }
    rt_mutex_take(winusb_device->xfer_lock, RT_WAITING_FOREVER);
    if(winusb_device->in_busy)
    {
        rt_mutex_release(winusb_device->xfer_lock);
        g_dbg_touch_drop++;
        return 0;
    }
    winusb_device->in_busy = 1;
    g_dbg_touch_arm++;
    winusb_device->ep_in->buffer = (void *)buffer;
    winusb_device->ep_in->request.buffer = winusb_device->ep_in->buffer;
    winusb_device->ep_in->request.size = size;
    winusb_device->ep_in->request.req_type = UIO_REQUEST_WRITE;
    rt_usbd_io_request(((ufunction_t)dev->user_data)->device,winusb_device->ep_in,&winusb_device->ep_in->request);
    rt_mutex_release(winusb_device->xfer_lock);
    return size;
}
static rt_err_t  win_usb_control(rt_device_t dev, int cmd, void *args)
{
    winusb_device_t winusb_device = (winusb_device_t)dev;
    if(RT_DEVICE_CTRL_CONFIG == cmd)
    {
        winusb_device->cmd_handler = (void(*)(rt_uint8_t*,rt_size_t))args;
    }
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops winusb_device_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    win_usb_read,
    win_usb_write,
    win_usb_control,
};
#endif

static rt_err_t rt_usb_winusb_init(ufunction_t func)
{
    winusb_device_t winusb_device   = (winusb_device_t)func->user_data;
    winusb_device->parent.type      = RT_Device_Class_Miscellaneous;
    winusb_device->xfer_lock        = rt_mutex_create("winusb_x", RT_IPC_FLAG_PRIO);

#ifdef RT_USING_DEVICE_OPS
    winusb_device->parent.ops       = &winusb_device_ops;
#else
    winusb_device->parent.init      = RT_NULL;
    winusb_device->parent.open      = RT_NULL;
    winusb_device->parent.close     = RT_NULL;
    winusb_device->parent.read      = win_usb_read;
    winusb_device->parent.write     = win_usb_write;
    winusb_device->parent.control   = win_usb_control;
#endif

    winusb_device->parent.user_data = func;


    return rt_device_register(&winusb_device->parent, "winusb", RT_DEVICE_FLAG_RDWR);
}

ufunction_t rt_usbd_function_winusb_create(udevice_t device)
{
    ufunction_t         func;
    winusb_device_t     winusb_device;

    uintf_t             winusb_intf;
    ualtsetting_t       winusb_setting;
    winusb_desc_t       winusb_desc;

    /* parameter check */
    RT_ASSERT(device != RT_NULL);

    /* set usb device string description */
#ifdef RT_USB_DEVICE_COMPOSITE
    rt_usbd_device_set_interface_string(device, WINUSB_INTF_STR_INDEX, _ustring[2]);
#else
    rt_usbd_device_set_string(device, _ustring);
#endif

    /* create a cdc function */
    func = rt_usbd_function_new(device, &dev_desc, &ops);
    rt_usbd_device_set_qualifier(device, &dev_qualifier);

    /* allocate memory for cdc vcom data */
    winusb_device = (winusb_device_t)rt_malloc(sizeof(struct winusb_device));
    rt_memset((void *)winusb_device, 0, sizeof(struct winusb_device));
    func->user_data = (void*)winusb_device;
    /* create an interface object */
    winusb_intf = rt_usbd_interface_new(device, _interface_handler);

    /* create an alternate setting object */
    winusb_setting = rt_usbd_altsetting_new(sizeof(struct winusb_descriptor));

    /* config desc in alternate setting */
    rt_usbd_altsetting_config_descriptor(winusb_setting, &_winusb_desc, (rt_off_t)&((winusb_desc_t)0)->intf_desc);

    /* configure the hid interface descriptor */
    _winusb_descriptor_config(winusb_setting->desc, winusb_intf->intf_num, device->dcd->device_is_hs);

    /* create endpoint */
    winusb_desc = (winusb_desc_t)winusb_setting->desc;
    winusb_device->ep_out = rt_usbd_endpoint_new(&winusb_desc->ep_out_desc, _ep_out_handler);
    winusb_device->ep_in  = rt_usbd_endpoint_new(&winusb_desc->ep_in_desc, _ep_in_handler);

    /* add the int out and int in endpoint to the alternate setting */
    rt_usbd_altsetting_add_endpoint(winusb_setting, winusb_device->ep_out);
    rt_usbd_altsetting_add_endpoint(winusb_setting, winusb_device->ep_in);

    /* add the alternate setting to the interface, then set default setting */
    rt_usbd_interface_add_altsetting(winusb_intf, winusb_setting);
    rt_usbd_set_altsetting(winusb_intf, 0);

    /* add the interface to the mass storage function */
    rt_usbd_function_add_interface(func, winusb_intf);

    rt_usbd_os_comp_id_desc_add_os_func_comp_id_desc(device->os_comp_id_desc, &winusb_func_comp_id_desc);
    /* initilize winusb */
    rt_usb_winusb_init(func);
    return func;
}

struct udclass winusb_class =
{
    .rt_usbd_function_create = rt_usbd_function_winusb_create
};

int rt_usbd_winusb_class_register(void)
{
    rt_usbd_class_register(&winusb_class);
    return 0;
}
INIT_PREV_EXPORT(rt_usbd_winusb_class_register);
