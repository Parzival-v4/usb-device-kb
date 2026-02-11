#ifndef _DFU_CONFIG_H
#define _DFU_CONFIG_H

#include <usb_types.h>
#include <usb/dfu.h>

typedef enum
{
	DFU_MODE_STATE_RUN_TIME = 0,
	DFU_MODE_STATE_DFU,
} DFU_ModeStateTypeDef;

#ifdef __LINUX_USB__

typedef enum
{
   DFU_USB_EP0_REQUEST_TYPE_CTRL = 0,
   DFU_USB_EP0_REQUEST_TYPE_DATA ,
} DFU_UsbRequestTypeDef;


struct dfu_usb_request {
	unsigned int			type;
	struct usb_ctrlrequest 	*ctrl;
	struct usb_request		*req;
} __attribute__((__packed__));

struct dfu_request_data
{
	__s32 length;
	__u8 data[DFU_USB_BUFSIZ];
};

#define DFU_IOC_SEND_EP0_DATA            		_IOW('D', 1, struct dfu_request_data)
#define DFU_IOC_GET_EP0_DATA            		_IOR('D', 2, struct dfu_request_data)
#define DFU_IOC_USB_CONNECT_STATE				_IOW('D', 3, int)
#define DFU_IOC_GET_USB_SOFT_CONNECT_STATE		_IOR('D', 4, uint32_t)
#define DFU_IOC_SET_USB_SOFT_CONNECT_CTRL		_IOW('D', 5, uint32_t)
#define DFU_IOC_GET_USB_SOFT_CONNECT_CTRL		_IOR('D', 6, uint32_t)
#endif /* #ifdef __LINUX_USB__ */

#endif /* _DFU_CONFIG_H */