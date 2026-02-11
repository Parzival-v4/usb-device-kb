#ifndef __LINUX_USB_HID_H
#define __LINUX_USB_HID_H


#include <usb_types.h>

/*
 * HID class requests
 */

#define HID_REQ_GET_REPORT		0x01
#define HID_REQ_GET_IDLE		0x02
#define HID_REQ_GET_PROTOCOL	0x03
#define HID_REQ_SET_REPORT		0x09
#define HID_REQ_SET_IDLE		0x0A
#define HID_REQ_SET_PROTOCOL	0x0B

/*
 * HID class descriptor types
 */

#define HID_DT_HID			(USB_TYPE_CLASS | 0x01)
#define HID_DT_REPORT		(USB_TYPE_CLASS | 0x02)
#define HID_DT_PHYSICAL		(USB_TYPE_CLASS | 0x03)

struct hid_descriptor{
	__u8 bLength;
	__u8 bDescriptorType;
	__u16 bcdHID;
	__u8 bCountryCode;
	__u8 bNumDescriptors;
	__u8 bDesType;
	__u16 wDescriptorLenght;
} __attribute__((__packed__));

#define USB_HID_DESC_SIZE		9

#endif /* __LINUX_USB_HID_H */
