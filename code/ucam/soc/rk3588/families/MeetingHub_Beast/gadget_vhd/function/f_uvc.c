/*
 *	uvc_gadget.c  --  USB Video Class Gadget driver
 *
 * Copyright (C) ValueHD Corporation. All rights reserved
 *  
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>

#include "uvc.h"
#include "uvc_v4l2.h"
#include "uvc_video.h"
#include "u_uvc.h"

#include "uvc_config.h"


static unsigned long bulk_max_size = 1024;
module_param(bulk_max_size, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(bulk_max_size, "bulk max size");

static bool bulk_streaming_ep = 0;
module_param(bulk_streaming_ep, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(bulk_streaming_ep, "0 (Use ISOC video streaming ep) / "
					"1 (Use BULK video streaming ep)");
					
bool usb3_win7_mode = 0;
module_param(usb3_win7_mode, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(usb3_win7_mode, "1 (Use Win7 USB3.0 ISOC Mode) / "
					"1 (Use Win10 USB3.0 ISOC Mode)");
					
static int s2_entity_id_c1 = -1;
module_param(s2_entity_id_c1, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(s2_entity_id_c1, "to stream2 entity c1");

static int s2_entity_id_c2 = -1;
module_param(s2_entity_id_c2, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(s2_entity_id_c2, "to stream2 entity c2");

static int debug = 0;
module_param(debug, int, 0644);

unsigned int uvc_gadget_trace_param;

bool get_bulk_streaming_ep(void)
{
	return bulk_streaming_ep;
}
EXPORT_SYMBOL(get_bulk_streaming_ep);


int get_uvc_debug(void)
{
	return debug;
}
EXPORT_SYMBOL(get_uvc_debug);

/* --------------------------------------------------------------------------
 * Function descriptors
 */

/* string IDs are assigned dynamically */

#define UVC_STRING_CONTROL_IDX		0
#define UVC_STRING_STREAMING_IDX		1

static struct usb_string uvc_en_us_strings[] = {
	[UVC_STRING_CONTROL_IDX].s = "UVC Camera",
	[UVC_STRING_STREAMING_IDX].s = "Video Streaming",
	{  }
};

static struct usb_gadget_strings uvc_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = uvc_en_us_strings,
};

static struct usb_gadget_strings *uvc_function_strings[] = {
	&uvc_stringtab,
	NULL,
};

#define UVC_INTF_STREAMING_NUM					0
#define UVC_STREAMING_INTERFACE_PROTOCOL		0x00//UVC1.5 0x01,UVC1.1 0x00	

#define UVC_STATUS_MAX_PACKET_SIZE		64	/* 16 bytes status */


static struct usb_interface_assoc_descriptor uvc_iad = {
	.bLength		= sizeof(uvc_iad),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0,
	.bInterfaceCount	= 2,
	.bFunctionClass		= USB_CLASS_VIDEO,
	.bFunctionSubClass	= UVC_SC_VIDEO_INTERFACE_COLLECTION,
	.bFunctionProtocol	= 0x00,
	.iFunction		= 0,
};

static struct usb_interface_descriptor uvc_control_intf = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0,
	.bAlternateSetting	= 0,
#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	.bNumEndpoints		= 0,
#else
	.bNumEndpoints		= 0,
#endif
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOCONTROL,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_status_interrupt_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize		= cpu_to_le16(UVC_STATUS_MAX_PACKET_SIZE),
	.bInterval		= 8,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_status_interrupt_comp = {
	.bLength		= sizeof(struct usb_ss_ep_comp_descriptor),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The following 3 values can be tweaked if necessary. */
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval	= cpu_to_le16(UVC_STATUS_MAX_PACKET_SIZE),
};

static struct uvc_control_endpoint_descriptor uvc_status_interrupt_cs_ep = {
	.bLength		= UVC_DT_CONTROL_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_CS_ENDPOINT,
	.bDescriptorSubType	= UVC_EP_INTERRUPT,
	.wMaxTransferSize	= cpu_to_le16(UVC_STATUS_MAX_PACKET_SIZE),
};



static struct usb_interface_descriptor uvc_bulk_streaming_intf_alt0 = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= 0x00,
	.iInterface		= 0,
};

static struct usb_endpoint_descriptor uvc_fs_bulk_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	/* The wMaxPacketSize and bInterval values will be initialized from
	 * module parameters.
	 */
	.wMaxPacketSize		= 0,
	.bInterval		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt0 = {
	.bLength			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface			= 0,
};


static struct usb_interface_descriptor uvc_streaming_intf_alt1 = {
	.bLength			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 1,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface			= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt2 = {
	.bLength			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 2,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface			= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt3  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 3,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt4  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 4,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt5  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 5,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt6  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 6,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt7  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 7,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt8  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 8,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt9  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 9,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt10  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 10,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt11  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 11,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt12  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 12,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt13  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 13,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt14  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 14,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt15  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 15,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt16  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 16,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt17  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 17,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt18  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 18,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt19  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 19,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt20  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 20,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt21  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 21,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt22  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 22,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt23  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 23,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt24  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 24,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

static struct usb_interface_descriptor uvc_streaming_intf_alt25  = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= UVC_INTF_STREAMING_NUM,
	.bAlternateSetting	= 25,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_VIDEO,
	.bInterfaceSubClass	= UVC_SC_VIDEOSTREAMING,
	.bInterfaceProtocol	= UVC_STREAMING_INTERFACE_PROTOCOL,
	.iInterface		= 0,
};

//ISOC mode

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_1 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT1, USB2_EP_ISOC_PKT_SIZE_ALT1)),	
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_1 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT1, USB3_EP_ISOC_PKT_SIZE_ALT1)),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_1 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT1 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT1,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_2 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT2, USB2_EP_ISOC_PKT_SIZE_ALT2)),	
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_2 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT2, USB3_EP_ISOC_PKT_SIZE_ALT2)),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_2 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT2 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT2,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_3 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT3, USB2_EP_ISOC_PKT_SIZE_ALT3)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_3 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT3, USB3_EP_ISOC_PKT_SIZE_ALT3)),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_3 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT3 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT3,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_4 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT4, USB2_EP_ISOC_PKT_SIZE_ALT4)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_4 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT4, USB3_EP_ISOC_PKT_SIZE_ALT4)),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_4 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT4 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT4,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_5 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT5, USB2_EP_ISOC_PKT_SIZE_ALT5)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_5 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT5, USB3_EP_ISOC_PKT_SIZE_ALT5)),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_5 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT5 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT5,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_6 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT6, USB2_EP_ISOC_PKT_SIZE_ALT6)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_6 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT6, USB3_EP_ISOC_PKT_SIZE_ALT6)),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_6 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT6 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT6,
};


static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_7 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT7, USB2_EP_ISOC_PKT_SIZE_ALT7)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_7 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_7 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT7 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT7,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_8 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT8, USB2_EP_ISOC_PKT_SIZE_ALT8)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_8 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_8 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT8 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT8,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_9 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT9, USB2_EP_ISOC_PKT_SIZE_ALT9)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_9 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_9 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT9 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT9,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_10 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT10, USB2_EP_ISOC_PKT_SIZE_ALT10)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_10 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_10 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT10 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT10,
};

static struct usb_endpoint_descriptor uvc_hs_streaming_isoc_ep_11 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(USB_EP_ISO_PKT_SIZE(USB2_EP_ISOC_PKTS_COUNT_ALT11, USB2_EP_ISOC_PKT_SIZE_ALT11)),
	.bInterval			= 1,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_11 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_11 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT11 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT11,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_12 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_12 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT12 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT12,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_13 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_13 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT13 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT13,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_14 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_14 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT14 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT14,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_15 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_15 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT15 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT15,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_16 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_16 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT16 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT16,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_17 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_17 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= USB3_EP_ISOC_PKTS_COUNT_ALT17 - 1,
	.bmAttributes		= 0,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT17,
};


static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_18 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//20 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_18 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT18/2) - 1,
	.bmAttributes		= 1,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT18,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_19 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//24 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_19 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT19/2) - 1,
	.bmAttributes		= 1,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT19,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_20 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//28 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_20 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT20/2) - 1,
	.bmAttributes		= 1,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT20,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_21 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//32 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_21 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT21/2) - 1,
	.bmAttributes		= 1,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT21,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_22 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//36 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_22 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT22/3) - 1,
	.bmAttributes		= 2,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT22,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_23 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//39 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_23 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT23/3) - 1,
	.bmAttributes		= 2,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT23,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_24 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//45 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_24 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT24/3) - 1,
	.bmAttributes		= 2,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT24,
};

static struct usb_endpoint_descriptor uvc_ss_streaming_isoc_ep_25 = {
	.bLength			= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize 	= cpu_to_le16(1024),
	.bInterval			= 1,
};

//48 PKTS
static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_isoc_comp_25 = {
	.bLength			= USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst			= (USB3_EP_ISOC_PKTS_COUNT_ALT25/3) - 1,
	.bmAttributes		= 2,
	.wBytesPerInterval	= USB3_EP_ISOC_PACKET_SIZE_ALT25,
};




static struct usb_endpoint_descriptor uvc_fs_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,
	/* The wMaxPacketSize and bInterval values will be initialized from
	 * module parameters.
	 */
};

static struct usb_endpoint_descriptor uvc_hs_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,
	/* The wMaxPacketSize and bInterval values will be initialized from
	 * module parameters.
	 */
};

static struct usb_endpoint_descriptor uvc_ss_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,
	/* The wMaxPacketSize and bInterval values will be initialized from
	 * module parameters.
	 */
};

static struct usb_ss_ep_comp_descriptor uvc_ss_streaming_comp = {
	.bLength		= sizeof(uvc_ss_streaming_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
};



static const struct usb_descriptor_header * const uvc_fs_streaming[] = {
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt1,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_1,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt2,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_2,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt3,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_3,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt4,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_4,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt5,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_5,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt6,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_6,
#if 0 //USB 1.x wMaxPacketSize 0 ~ 1023
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt7,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_7,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt8,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_8,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt9,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_9,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt10,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_10,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt11,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_11,
#endif	
	NULL,
};

static const struct usb_descriptor_header * const uvc_hs_streaming[] = {
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt1,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_1,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt2,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_2,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt3,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_3,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt4,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_4,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt5,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_5,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt6,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_6,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt7,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_7,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt8,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_8,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt9,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_9,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt10,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_10,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt11,
	(const struct usb_descriptor_header *) &uvc_hs_streaming_isoc_ep_11,
	NULL,
};

static const struct usb_descriptor_header * const uvc_ss_streaming[] = {
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt1,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_1,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_1,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt2,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_2,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_2,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt3,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_3,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_3,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt4,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_4,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_4,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt5,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_5,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_5,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt6,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_6,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_6,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt7,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_7,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_7,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt8,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_8,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_8,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt9,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_9,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_9,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt10,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_10,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_10,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt11,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_11,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_11,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt12,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_12,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_12,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt13,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_13,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_13,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt14,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_14,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_14,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt15,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_15,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_15,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt16,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_16,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_16,

	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt17,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_17,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_17,

#if 1	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt18,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_18,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_18,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt19,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_19,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_19,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt20,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_20,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_20,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt21,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_21,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_21,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt22,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_22,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_22,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt23,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_23,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_23,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt24,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_24,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_24,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt25,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_25,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_25,
#endif	
	NULL,
};

static struct usb_endpoint_descriptor uvc_hs_bulk_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	/* The wMaxPacketSize and bInterval values will be initialized from
	 * module parameters.
	 */
	.wMaxPacketSize		= 0,
	.bInterval		= 0,
};


static struct usb_endpoint_descriptor uvc_ss_bulk_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	/* The wMaxPacketSize and bInterval values will be initialized from
	 * module parameters.
	 */
	.wMaxPacketSize		= 0,
	.bInterval		= 0,
};

static struct usb_ss_ep_comp_descriptor uvc_ss_bulk_streaming_comp = {
	.bLength		= sizeof(uvc_ss_bulk_streaming_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The following 3 values can be tweaked if necessary. */
	//.bMaxBurst		= 0x0F,
	.bMaxBurst		= 0x00,
	.bmAttributes		= 0,
	.wBytesPerInterval	= 0,//cpu_to_le16(1024),
};

static const struct usb_descriptor_header * const uvc_fs_bulk_streaming[] = {
	(struct usb_descriptor_header *) &uvc_fs_bulk_streaming_ep,
	NULL,
};

static const struct usb_descriptor_header * const uvc_hs_bulk_streaming[] = {
	(struct usb_descriptor_header *) &uvc_hs_bulk_streaming_ep,
	NULL,
};

static const struct usb_descriptor_header * const uvc_ss_bulk_streaming[] = {
	(struct usb_descriptor_header *) &uvc_ss_bulk_streaming_ep,
	(struct usb_descriptor_header *) &uvc_ss_bulk_streaming_comp,
	NULL,
};

void uvc_set_trace_param(unsigned int trace)
{
	uvc_gadget_trace_param = trace;
}
EXPORT_SYMBOL(uvc_set_trace_param);

//USB3.0 EPģʽ��0������ģʽ��WIN 10��,1��USB2.0ģʽ��WIN 7��
void usb_ss_endpoint_max_packet_size_switch(uint8_t mode)
{
	uvc_ss_streaming_isoc_ep_1.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT1, USB3_EP_ISOC_PKT_SIZE_ALT1);
	uvc_ss_streaming_isoc_ep_2.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT2, USB3_EP_ISOC_PKT_SIZE_ALT2);
	uvc_ss_streaming_isoc_ep_3.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT3, USB3_EP_ISOC_PKT_SIZE_ALT3);
	uvc_ss_streaming_isoc_ep_4.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT4, USB3_EP_ISOC_PKT_SIZE_ALT4);
	uvc_ss_streaming_isoc_ep_5.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT5, USB3_EP_ISOC_PKT_SIZE_ALT5);
	uvc_ss_streaming_isoc_ep_6.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT6, USB3_EP_ISOC_PKT_SIZE_ALT6);
	uvc_ss_streaming_isoc_ep_7.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT7, USB3_EP_ISOC_PKT_SIZE_ALT7);
	uvc_ss_streaming_isoc_ep_8.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT8, USB3_EP_ISOC_PKT_SIZE_ALT8);
	uvc_ss_streaming_isoc_ep_9.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT9, USB3_EP_ISOC_PKT_SIZE_ALT9);
	uvc_ss_streaming_isoc_ep_10.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT10, USB3_EP_ISOC_PKT_SIZE_ALT10);
	uvc_ss_streaming_isoc_ep_11.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT11, USB3_EP_ISOC_PKT_SIZE_ALT11);
	uvc_ss_streaming_isoc_ep_12.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT12, USB3_EP_ISOC_PKT_SIZE_ALT12);
	uvc_ss_streaming_isoc_ep_13.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT13, USB3_EP_ISOC_PKT_SIZE_ALT13);
	uvc_ss_streaming_isoc_ep_14.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT14, USB3_EP_ISOC_PKT_SIZE_ALT14);
	uvc_ss_streaming_isoc_ep_15.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT15, USB3_EP_ISOC_PKT_SIZE_ALT15);
	uvc_ss_streaming_isoc_ep_16.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT16, USB3_EP_ISOC_PKT_SIZE_ALT16);
	uvc_ss_streaming_isoc_ep_17.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT17, USB3_EP_ISOC_PKT_SIZE_ALT17);
	uvc_ss_streaming_isoc_ep_18.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT18, USB3_EP_ISOC_PKT_SIZE_ALT18);
	uvc_ss_streaming_isoc_ep_19.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT19, USB3_EP_ISOC_PKT_SIZE_ALT19);
	uvc_ss_streaming_isoc_ep_20.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT20, USB3_EP_ISOC_PKT_SIZE_ALT20);
	uvc_ss_streaming_isoc_ep_21.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT21, USB3_EP_ISOC_PKT_SIZE_ALT21);
	uvc_ss_streaming_isoc_ep_22.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT22, USB3_EP_ISOC_PKT_SIZE_ALT22);
	uvc_ss_streaming_isoc_ep_23.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT23, USB3_EP_ISOC_PKT_SIZE_ALT23);
	uvc_ss_streaming_isoc_ep_24.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT24, USB3_EP_ISOC_PKT_SIZE_ALT24);
	uvc_ss_streaming_isoc_ep_25.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT25, USB3_EP_ISOC_PKT_SIZE_ALT25);

	if(mode == 0)
	{
		printk("Windows 10 USB3.0 ISOC Mode\n");
		
		if(uvc_ss_streaming_isoc_ep_1.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_1.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_2.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_2.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_3.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_3.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_4.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_4.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_5.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_5.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_6.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_6.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_7.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_7.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_8.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_8.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_9.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_9.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_10.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_10.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_11.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_11.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_12.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_12.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_13.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_13.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_14.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_14.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_15.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_15.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_16.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_16.wMaxPacketSize = 1024;

		if(uvc_ss_streaming_isoc_ep_17.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_17.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_18.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_18.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_19.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_19.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_20.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_20.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_21.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_21.wMaxPacketSize = 1024;	
		
		if(uvc_ss_streaming_isoc_ep_22.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_22.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_23.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_23.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_24.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_24.wMaxPacketSize = 1024;
		
		if(uvc_ss_streaming_isoc_ep_25.wMaxPacketSize > 1024)
			uvc_ss_streaming_isoc_ep_25.wMaxPacketSize = 1024;
	
	}
	else
	{
		printk("Windows 7 USB3.0 ISOC Mode\n");
	}

}


#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT	
void uvc_interrupt_ep_enable(struct usb_function *f, int en)
{
	struct uvc_device *uvc = to_uvc(f);
	if(en)
	{
		uvc_control_intf.bNumEndpoints	= 1;
		uvc_status_interrupt_ep.bLength = USB_DT_ENDPOINT_SIZE;
		uvc_ss_status_interrupt_comp.bLength = sizeof(struct usb_ss_ep_comp_descriptor);
		uvc_status_interrupt_cs_ep.bLength = UVC_DT_CONTROL_ENDPOINT_SIZE;
	}
	else
	{
		if(uvc->uvc_interrupt_ep_enable)
		{
			unsigned long flags;
			spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);	
			if (uvc->uvc_status_interrupt_ep) {
				if (uvc->uvc_status_interrupt_req)
				{
					usb_ep_dequeue(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
					usb_ep_free_request(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
                    uvc->uvc_status_interrupt_req = NULL;
				}
				
                if(uvc->uvc_status_interrupt_ep->enabled)
					usb_ep_disable(uvc->uvc_status_interrupt_ep);
				uvc->uvc_status_interrupt_ep->driver_data = NULL;
				uvc->uvc_status_interrupt_ep = NULL;
			}
			uvc->uvc_status_write_pending = 0;
			spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);
		}
		uvc_control_intf.bNumEndpoints	= 0;
		uvc_status_interrupt_ep.bLength = 0;
		uvc_ss_status_interrupt_comp.bLength = 0;
		uvc_status_interrupt_cs_ep.bLength = 0;
		uvc->uvc_interrupt_ep_enable = 0;
		
	}
	
}
EXPORT_SYMBOL(uvc_interrupt_ep_enable);
#endif

/* --------------------------------------------------------------------------
 * Control requests
 */

static void
uvc_function_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_device *uvc = req->context;
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;

	//printk("ep0_complete\n");

    if (uvc->state == UVC_STATE_DISCONNECTED)
    {
        printk("ep0_complete: device disconnected\n");
        return;
    }
	
	if (uvc->event_setup_out) {
		uvc->event_setup_out = 0;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UVC_EVENT_DATA;
		

		if(uvc->auto_queue_data_enable)
		{
			uvc_event->auto_queue_data.length = req->actual;
			if(uvc_event->auto_queue_data.length > UVC_REQUEST_DATA_AUTO_QUEUE_LEN)
			{
				printk("data length error,data length:%d, max length:%d\n", 
					uvc_event->auto_queue_data.length, (__s32)UVC_REQUEST_DATA_AUTO_QUEUE_LEN);
				return;
			}
			memcpy(&uvc_event->auto_queue_data.data, req->buf, req->actual);
			memcpy(&uvc_event->auto_queue_data.usb_ctrl, &uvc->event_ctrl, sizeof(struct usb_ctrlrequest));
		}
		else
		{
			uvc_event->data.length = req->actual;
			memcpy(&uvc_event->data.data, req->buf, req->actual);
		}
		
#if 0
		printk("bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
				"wLength %04x\n", uvc_event->auto_queue_data.usb_ctrl.bRequestType, 
				uvc_event->auto_queue_data.usb_ctrl.bRequest,
				uvc_event->auto_queue_data.usb_ctrl.wValue, uvc_event->auto_queue_data.usb_ctrl.wIndex, 
				uvc_event->auto_queue_data.usb_ctrl.wLength);
#endif					
		v4l2_event_queue(uvc->vdev, &v4l2_event);
	}
}

static void
uvc_function_status_interrupt_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_device *uvc = req->context;
	unsigned long flags;
	
	//printk("status_interrupt_ep_complete\n");
			
	
	spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);
	uvc->uvc_status_write_pending = 0;
	spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);	

}

// SetConfiguration 
int usb_set_configuration_value(struct usb_function *f, int ConfigurationValue)
{
	struct uvc_device *uvc = to_uvc(f);
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	struct usb_ctrlrequest ctrl;
	
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.bRequestType = 0x00;
	ctrl.bRequest = 0x09;
	ctrl.wIndex = 0;
	ctrl.wValue = ConfigurationValue;
	ctrl.wLength = 0x00;
	
	//printk("usb_set_configuration_value : %d\n", ConfigurationValue);
	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_SETUP;
	memcpy(&uvc_event->req, &ctrl, sizeof(uvc_event->req));
	v4l2_event_queue(uvc->vdev, &v4l2_event);

	return 0;
}


static int
uvc_function_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct uvc_device *uvc = to_uvc(f);
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	
	uint16_t	w_index = le16_to_cpu(ctrl->wIndex);
	uint8_t		intf = w_index & 0xFF;
	uint8_t		id = w_index >> 8;
	int i;

    if (uvc->state == UVC_STATE_DISCONNECTED)
    {
        printk("setup: device disconnected\n");
        return 0;
    }

	#if 0
    printk(KERN_INFO "setup request %02x %02x value %04x index %04x %04x\n",
	 	ctrl->bRequestType, ctrl->bRequest, le16_to_cpu(ctrl->wValue),
	 	le16_to_cpu(ctrl->wIndex), le16_to_cpu(ctrl->wLength));
    #endif

	if((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD &&
		(ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_ENDPOINT && ctrl->bRequest == USB_REQ_CLEAR_FEATURE) 
	{
		if(uvc->video.bulk_streaming_ep && ctrl->wIndex == uvc->video.ep->address)
		{
#if 1
			//usb_ep_clear_halt(uvc->video.ep);
			if (uvc->state == UVC_STATE_STREAMING)
			{
				uvc->state = UVC_STATE_CONNECTED;

#if (defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211 || defined CHIP_CV5)
				usb_ep_disable(uvc->video.ep);
				config_ep_by_speed(f->config->cdev->gadget,
							&(uvc->func), uvc->video.ep);
				usb_ep_enable(uvc->video.ep);
#endif

#if 1
				memset(&v4l2_event, 0, sizeof(v4l2_event));
				v4l2_event.type = UVC_EVENT_STREAMOFF;
				v4l2_event_queue(uvc->vdev, &v4l2_event);	
				return 0;
#endif
			}
#endif		
		}
		
	}
	else if ((ctrl->bRequestType & USB_TYPE_MASK) != USB_TYPE_CLASS) 
	{
		INFO(f->config->cdev, "invalid request type\n");
		pr_notice("invalid request type\n");
		return -EINVAL;
	}
	

		
	if(f->config->bConfigurationValue == 2)
	{
		//UVC1.5 EU control to stream2 device setup
		if(intf == uvc->control_intf && (id == UVC_ENTITY_ID_ENCODING_UNIT || id == s2_entity_id_c2))
		{	
		
			struct usb_composite_dev	*cdev = f->config->cdev;
			if (cdev->config && f->list.next)
			{				
				//struct usb_function *f_uvc_s2 = cdev->config->interface[UVC_INTF_STREAMING_2_S2];
				struct usb_function *f_uvc_s2 = container_of(f->list.next, struct usb_function, list);
				if(f_uvc_s2)
				{					
					//printk("EU control to stream2 device setup\n");
					return f_uvc_s2->setup(f_uvc_s2, ctrl);//video1 -> video 2
				}
			}
		}	
	}
	else if(f->config->bConfigurationValue == 1 || f->config->bConfigurationValue == 3)
	{
		//UVC1.5 EU control to stream2 device setup
		if(intf == uvc->control_intf && (id == s2_entity_id_c1))
		{	
		
			struct usb_composite_dev	*cdev = f->config->cdev;
			if (cdev->config && f->list.next)
			{				
				//struct usb_function *f_uvc_s2 = cdev->config->interface[UVC_INTF_STREAMING_2_S2];
				struct usb_function *f_uvc_s2 = container_of(f->list.next, struct usb_function, list);
				if(f_uvc_s2)
				{					
					//printk("EU control to stream2 device setup\n");
					return f_uvc_s2->setup(f_uvc_s2, ctrl);//video1 -> video 2
				}
			}
		}	
	}

	/* Stall too big requests. */
	if (le16_to_cpu(ctrl->wLength) > UVC_MAX_REQUEST_SIZE)
		return -EINVAL;

	/* Tell the complete callback to generate an event for the next request
	 * that will be enqueued by UVCIOC_SEND_RESPONSE.
	 */
	uvc->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
	uvc->event_length = le16_to_cpu(ctrl->wLength);

#if 1	
	if(uvc->video.bulk_streaming_ep && (le16_to_cpu(ctrl->wValue) >> 8) == UVC_VS_COMMIT_CONTROL
			&& (le16_to_cpu(ctrl->wIndex) & 0xff) == uvc->streaming_intf
			&& ctrl->bRequest == UVC_SET_CUR)
	{
				
		if (uvc->state == UVC_STATE_STREAMING)
		{
			uvc->state = UVC_STATE_CONNECTED;

#if (defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211 || defined CHIP_CV5)
			usb_ep_disable(uvc->video.ep);
			config_ep_by_speed(f->config->cdev->gadget,
						&(uvc->func), uvc->video.ep);
			usb_ep_enable(uvc->video.ep);	
#endif

			for (i = 0; i < uvc->video.uvc_num_requests; ++i)
				if (uvc->video.ureq && uvc->video.ureq[i].req)
					usb_ep_dequeue(uvc->video.ep, uvc->video.ureq[i].req);

			uvcg_queue_enable(&uvc->video.queue, 0);
			

			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UVC_EVENT_STREAMOFF;
			v4l2_event_queue(uvc->vdev, &v4l2_event);	

		}
		
	}
#endif

	if(uvc->auto_queue_data_enable && uvc->event_length <= UVC_REQUEST_DATA_AUTO_QUEUE_LEN)
	{
		if(uvc->event_setup_out)
		{
			return uvc_setup_data(uvc, ctrl);
		}
	}

	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_SETUP;
	memcpy(&uvc_event->req, ctrl, sizeof(uvc_event->req));
	v4l2_event_queue(uvc->vdev, &v4l2_event);

	return 0;
}

void uvc_function_setup_continue(struct uvc_device *uvc)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;

	usb_composite_setup_continue(cdev);
}

static int
uvc_function_get_alt(struct usb_function *f, unsigned interface)
{
	struct uvc_device *uvc = to_uvc(f);

	INFO(f->config->cdev, "uvc_function_get_alt(%u)\n", interface);

	if (interface == uvc->control_intf)
		return 0;
	else if (interface != uvc->streaming_intf)
		return -EINVAL;
	else {
		/*
		 * Alt settings in an interface are supported only for
		 * ISOC endpoints as there are different alt-settings for
		 * zero-bandwidth and full-bandwidth cases, but the same
		 * is not true for BULK endpoints, as they have a single
		 * alt-setting.
		 */
		if (!uvc->video.bulk_streaming_ep)
		{
			return uvc->video.ep->enabled ? uvc->streaming_intf_alt : 0;
		}
		else
			return 0;
	}

	return -EINVAL;
}

extern int GetConfigurationValue(void);

static int
uvc_function_set_alt(struct usb_function *f, unsigned interface, unsigned alt)
{
	struct uvc_device *uvc = to_uvc(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;
	int ret;
    int i;
	

	printk("%d %s: uvc_function_set_alt(%u, %u) uvc->state: %d uvc->control_intf: %d : %d \n",__LINE__,__func__,
		interface, alt, uvc->state, uvc->control_intf, uvc->streaming_intf);

	if (interface == uvc->control_intf) {
		if (alt)
			return -EINVAL;

		//if (uvc->state == UVC_STATE_DISCONNECTED) 
		{
			printk("UVC_EVENT_CONNECT usb speed = %s, bConfigurationValue = %d\n",
				usb_speed_string(cdev->gadget->speed), f->config->bConfigurationValue);
				
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UVC_EVENT_CONNECT;
			uvc_event->speed = cdev->gadget->speed;
			//v4l2_event.reserved[0] = f->config->bConfigurationValue;
			//v4l2_event.reserved[1] = 0;

			v4l2_event_queue(uvc->vdev, &v4l2_event);
			
			
			if (uvc->video.bulk_streaming_ep)
				uvc->state = UVC_STATE_CONFIG;
			else
				uvc->state = UVC_STATE_CONNECTED;
		}
		
		usb_set_configuration_value(f, GetConfigurationValue()/*f->config->bConfigurationValue*/);
		
#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT	
		if(uvc->uvc_interrupt_ep_enable && uvc->uvc_status_interrupt_ep)
		{
			unsigned long flags;

			
			spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);
		
			if (uvc->uvc_status_interrupt_ep->enabled) {
				INFO(cdev, "reset UVC status_interrupt_ep\n");
				
				if (uvc->uvc_status_interrupt_req)
				{
					usb_ep_dequeue(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
					usb_ep_free_request(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
                    uvc->uvc_status_interrupt_req = NULL;
				}
						
                usb_ep_disable(uvc->uvc_status_interrupt_ep);
			}

			if (uvc->uvc_status_interrupt_ep->enabled == 0)
			{
				if (config_ep_by_speed(cdev->gadget, f, uvc->uvc_status_interrupt_ep))
				{
					spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);
					return -EINVAL;
				}
		
				usb_ep_enable(uvc->uvc_status_interrupt_ep);
				
				uvc->uvc_status_interrupt_req = usb_ep_alloc_request(uvc->uvc_status_interrupt_ep, GFP_KERNEL);
				uvc->uvc_status_interrupt_req->buf = uvc->uvc_status_interrupt_buf;
				uvc->uvc_status_interrupt_req->complete = uvc_function_status_interrupt_ep_complete;
				uvc->uvc_status_interrupt_req->context = uvc;	
			}
			
			uvc->uvc_status_write_pending = 0;
			spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);	
		}
		
#endif

		return 0;
	}

	if (interface != uvc->streaming_intf)
		return -EINVAL;

	/* TODO
	if (usb_endpoint_xfer_bulk(&uvc->desc.vs_ep))
		return alt ? -EINVAL : 0;
	*/
	
	
	if (uvc->video.bulk_streaming_ep)
	{
		switch (uvc->state) {
			case UVC_STATE_CONFIG:
				if (uvc->video.ep->enabled) {
				    usb_ep_disable(uvc->video.ep);
				}
				if (!uvc->video.ep->enabled) {
					/*
					 * Enable the video streaming endpoint,
					 * but don't change the 'uvc->state'.
					 */
					if (uvc->video.ep) {
						ret = config_ep_by_speed
							(f->config->cdev->gadget,
							&(uvc->func), uvc->video.ep);
						if (ret)
							return ret;
						uvc->video.ep->comp_desc = &uvc_ss_bulk_streaming_comp;
						ret = usb_ep_enable(uvc->video.ep);
						if (ret)
							return ret;

					}
				}
				
				//memset(&v4l2_event, 0, sizeof(v4l2_event));
				//v4l2_event.type = UVC_EVENT_STREAMON;
				//v4l2_event_queue(uvc->vdev, &v4l2_event);

				uvc->state = UVC_STATE_CONNECTED;
				return 0;

			case UVC_STATE_CONNECTED:

#if 0
				/*
				 * Enable the video streaming endpoint,
				 * but don't change the 'uvc->state'.
				 */
				if (uvc->video.ep) {
					ret = config_ep_by_speed
						(f->config->cdev->gadget,
						&(uvc->func), uvc->video.ep);
					if (ret)
						return ret;
					ret = usb_ep_enable(uvc->video.ep);
					if (ret)
						return ret;
				}


				memset(&v4l2_event, 0, sizeof(v4l2_event));
				v4l2_event.type = UVC_EVENT_STREAMON;
				v4l2_event_queue(uvc->vdev, &v4l2_event);
				uvc->state = UVC_STATE_STREAMING;
#endif
				return 0;

			case UVC_STATE_STREAMING:
#if 1
#if 0
				if (uvc->video.ep->enabled) {
					if (uvc->video.ep) {
						ret = usb_ep_disable(uvc->video.ep);
						if (ret)
							return ret;
					}
				}
#endif

				memset(&v4l2_event, 0, sizeof(v4l2_event));
				v4l2_event.type = UVC_EVENT_STREAMOFF;
				v4l2_event_queue(uvc->vdev, &v4l2_event);
				uvc->state = UVC_STATE_CONNECTED;
				INFO(cdev, "uvc_function_set_alt: state to streamoff\n");

#endif
				return 0;

			default:
				return -EINVAL;
			}			
	}
	
	
	uvc->streaming_intf_alt = alt;

	if(alt == 0){
		//if (uvc->state != UVC_STATE_STREAMING)
			//	return 0;
#if 1
		if (uvc->video.ep) {
			usb_ep_disable(uvc->video.ep);
		}


		printk("****stream off event****\n");
		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UVC_EVENT_STREAMOFF;
		v4l2_event_queue(uvc->vdev, &v4l2_event);
#endif
		//uvc->state = UVC_STATE_CONNECTED;
		return 0;
	} else {
		//if (uvc->state != UVC_STATE_CONNECTED)
		//{			
		//	return 0;
		//}

		if (!uvc->video.ep)
		{
			uvc->streaming_intf_alt = 0;
			return -EINVAL;
		}

		if (uvc->video.ep->enabled) {
			INFO(cdev, "reset UVC\n");
			usb_ep_disable(uvc->video.ep);
		}
		
		ret = config_ep_by_speed(f->config->cdev->gadget,
				&(uvc->func), uvc->video.ep);
		if (ret)
		{
			uvc->streaming_intf_alt = 0;
			return ret;
		}

		if(f->config->cdev->gadget->speed >= USB_SPEED_SUPER)
		{
			//uvc->video.ep->maxpacket = uvc->video.ep->maxpacket * (uvc->video.ep->mult + 1) * uvc->video.ep->maxburst;
			//pr_notice("uvc->video.ep->maxpacket: %d uvc->video.ep->mult: %d uvc->video.ep->maxburst: %d \n", uvc->video.ep->maxpacket, uvc->video.ep->mult, uvc->video.ep->maxburst);
			
			switch(alt){
				case 1:	
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_1;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_1;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT1, USB3_EP_ISOC_PKT_SIZE_ALT1);					
					break;
				case 2:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_2;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_2;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT2, USB3_EP_ISOC_PKT_SIZE_ALT2);					
					break;
				case 3:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_3;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_3;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT3, USB3_EP_ISOC_PKT_SIZE_ALT3);
					break;
				case 4:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_4;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_4;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT4, USB3_EP_ISOC_PKT_SIZE_ALT4);
					break;
				case 5:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_5;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_5;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT5, USB3_EP_ISOC_PKT_SIZE_ALT5);
					break;
				case 6:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_6;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_6;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT6, USB3_EP_ISOC_PKT_SIZE_ALT6);
					break;
				case 7:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_7;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_7;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT7, USB3_EP_ISOC_PKT_SIZE_ALT7);
					break;
				case 8:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_8;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_8;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT8, USB3_EP_ISOC_PKT_SIZE_ALT8);
					break;
				case 9:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_9;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_9;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT9, USB3_EP_ISOC_PKT_SIZE_ALT9);
					break;
				case 10:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_10;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_10;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT10, USB3_EP_ISOC_PKT_SIZE_ALT10);
					break;
				case 11:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_11;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_11;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT11, USB3_EP_ISOC_PKT_SIZE_ALT11);
					break;
				case 12:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_12;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_12;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT12, USB3_EP_ISOC_PKT_SIZE_ALT12);
					break;
				case 13:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_13;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_13;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT13, USB3_EP_ISOC_PKT_SIZE_ALT13);
					break;
				case 14:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_14;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_14;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT14, USB3_EP_ISOC_PKT_SIZE_ALT14);
					break;
				case 15:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_15;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_15;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT15, USB3_EP_ISOC_PKT_SIZE_ALT15);
					break;
				case 16:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_16;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_16;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT16, USB3_EP_ISOC_PKT_SIZE_ALT16);
					break;
				case 17:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_17;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_17;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT17, USB3_EP_ISOC_PKT_SIZE_ALT17);
					break;
				case 18:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_18;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_18;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT18, USB3_EP_ISOC_PKT_SIZE_ALT18);
					break;
				case 19:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_19;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_19;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT19, USB3_EP_ISOC_PKT_SIZE_ALT19);
					break;
				case 20:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_20;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_20;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT20, USB3_EP_ISOC_PKT_SIZE_ALT20);
					break;
				case 21:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_21;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_21;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT21, USB3_EP_ISOC_PKT_SIZE_ALT21);
					break;
				case 22:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_22;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_22;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT22, USB3_EP_ISOC_PKT_SIZE_ALT22);					
					break;
				case 23:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_23;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_23;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT23, USB3_EP_ISOC_PKT_SIZE_ALT23);	
					break;
				case 24:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_24;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_24;				
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT24, USB3_EP_ISOC_PKT_SIZE_ALT24);	
					break;
				case 25:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_25;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_25;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT25, USB3_EP_ISOC_PKT_SIZE_ALT25);	
					break;
					
				default:
					uvc->video.ep->desc = &uvc_ss_streaming_isoc_ep_17;
					uvc->video.ep->comp_desc = &uvc_ss_streaming_isoc_comp_17;
					uvc->video.ep->maxpacket = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT17, USB3_EP_ISOC_PKT_SIZE_ALT17);	
					uvc->streaming_intf_alt = 17;
					break;
			}
			
			//uvc->video.ep->maxpacket	= uvc->video.ep->desc->wMaxPacketSize;
			uvc->video.ep->maxburst 	= uvc->video.ep->comp_desc->bMaxBurst + 1;
			uvc->video.ep->mult 		= (uvc->video.ep->comp_desc->bmAttributes & 0x03) + 1;	
				
		} else {			
			switch(alt){
				case 1:					
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_1;
					break;
				case 2:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_2;
					break;
				case 3:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_3;
					break;
				case 4:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_4;
					break;
				case 5:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_5;
					break;
				case 6:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_6;
					break;
				case 7:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_7;
					break;
				case 8:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_8;
					break;
				case 9:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_9;
					break;
				case 10:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_10;
					break;
				case 11:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_11;
					break;
				default:
					uvc->video.ep->desc = &uvc_hs_streaming_isoc_ep_11;
					uvc->streaming_intf_alt = 11;
					break;
			}			
			uvc->video.ep->comp_desc = NULL;
			uvc->video.ep->maxburst = 1;
			
			uvc->video.ep->mult 		= (uvc->video.ep->desc->wMaxPacketSize >> 11 & 0x03) + 1;
			uvc->video.ep->maxpacket = uvc->video.ep->desc->wMaxPacketSize;							
		}
		
		uvc->video.max_payload_size = (uvc->video.ep->desc->wMaxPacketSize & 0x7FF) * (uvc->video.ep->mult) * (uvc->video.ep->maxburst);
		
		pr_notice("uvc->video.ep: %p uvc->video.ep->max_payload_size: %d \n", uvc->video.ep, uvc->video.max_payload_size);
	
		ret = usb_ep_enable(uvc->video.ep);
		if(ret)
		{
			printk("uvc usb_ep_enable error:%d\n", ret);
		}
		uvc->video.ep->driver_data = uvc;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UVC_EVENT_STREAMON;
		v4l2_event_queue(uvc->vdev, &v4l2_event);
#if (defined CHIP_CV5)
		return 0;
#else
		return USB_GADGET_DELAYED_STATUS;
#endif
	}
}

static void
uvc_function_disable(struct usb_function *f)
{
	struct uvc_device *uvc = to_uvc(f);
	struct v4l2_event v4l2_event;

	INFO(f->config->cdev, "uvc_function_disable\n");

    uvc->state = UVC_STATE_DISCONNECTED;

#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	if(uvc->uvc_interrupt_ep_enable)
	{
		unsigned long flags;
        spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);			
		if (uvc->uvc_status_interrupt_ep) {

            usb_ep_disable(uvc->uvc_status_interrupt_ep);
			if (uvc->uvc_status_interrupt_req)
			{
				usb_ep_dequeue(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
				usb_ep_free_request(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
                uvc->uvc_status_interrupt_req = NULL;
			}
		}
		uvc->uvc_status_write_pending = 0;
		spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);
	}
#endif


	if (uvc->video.ep) {
        usb_ep_disable(uvc->video.ep);
	}

	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_DISCONNECT;
	v4l2_event_queue(uvc->vdev, &v4l2_event);

    complete_all(&uvc->uvc_receive_ep0_completion);
}

/* --------------------------------------------------------------------------
 * Connection / disconnection
 */

void
uvc_function_connect(struct uvc_device *uvc)
{
	//struct usb_composite_dev *cdev = uvc->func.config->cdev;
	//int ret;

#if 0 //fixed me
	if ((ret = usb_function_activate(&uvc->func)) < 0)
		INFO(cdev, "UVC connect failed with %d\n", ret);
#endif
}

void
uvc_function_disconnect(struct uvc_device *uvc)
{
	//struct usb_composite_dev *cdev = uvc->func.config->cdev;
	//int ret;

#if 0 //fixed me
	if ((ret = usb_function_deactivate(&uvc->func)) < 0)
		INFO(cdev, "UVC disconnect failed with %d\n", ret);
#endif
}

/* --------------------------------------------------------------------------
 * USB probe and disconnect
 */

static int
uvc_register_video(struct uvc_device *uvc)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct video_device *video;

	/* TODO reference counting. */
	video = video_device_alloc();
	if (video == NULL)
		return -ENOMEM;

	video->v4l2_dev = &uvc->v4l2_dev;
	video->fops = &uvc_v4l2_fops;
	video->ioctl_ops = &uvc_v4l2_ioctl_ops;
	video->release = video_device_release;
	video->vfl_dir = VFL_DIR_TX;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	video->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
#endif
	strlcpy(video->name, cdev->gadget->name, sizeof(video->name));

	uvc->vdev = video;
	video_set_drvdata(video, uvc);

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
	return video_register_device(video, VFL_TYPE_VIDEO, -1);
#else
	return video_register_device(video, VFL_TYPE_GRABBER, -1);
#endif
}



static struct usb_descriptor_header **
uvc_copy_descriptors(struct uvc_device *uvc, enum usb_device_speed speed)
{
	struct uvc_input_header_descriptor *uvc_streaming_header;
	struct usb_interface_assoc_descriptor *iad;
	struct uvc_header_descriptor *uvc_control_header;
	const struct uvc_descriptor_header * const *uvc_control_desc;
	const struct uvc_descriptor_header * const *uvc_streaming_cls;
	const struct usb_descriptor_header * const *uvc_streaming_std;
	const struct usb_descriptor_header * const *src;
	struct usb_interface_descriptor *streaming_intf_alt0;
	struct usb_descriptor_header **dst;
	struct usb_descriptor_header **hdr;
	unsigned int control_size;
	unsigned int streaming_size;
	unsigned int n_desc;
	unsigned int bytes;
	void *mem;
	
	
	if (!uvc->video.bulk_streaming_ep)
		streaming_intf_alt0 = &uvc_streaming_intf_alt0;
	else
		streaming_intf_alt0 = &uvc_bulk_streaming_intf_alt0;

	switch (speed) {
	case USB_SPEED_SUPER_PLUS:
	case USB_SPEED_SUPER:
		uvc_control_desc = uvc->desc.ss_control;
		uvc_streaming_cls = uvc->desc.ss_streaming;
		uvc_streaming_std = uvc_ss_streaming;
		break;

	case USB_SPEED_HIGH:
		uvc_control_desc = uvc->desc.fs_control;
		uvc_streaming_cls = uvc->desc.hs_streaming;
		uvc_streaming_std = uvc_hs_streaming;
		break;

	case USB_SPEED_FULL:
	default:
		uvc_control_desc = uvc->desc.fs_control;
		uvc_streaming_cls = uvc->desc.fs_streaming;
		uvc_streaming_std = uvc_fs_streaming;
		break;
	}
	
	if (uvc->video.bulk_streaming_ep) {
		switch (speed) {
		case USB_SPEED_SUPER_PLUS:
		case USB_SPEED_SUPER:
			uvc_streaming_std = uvc_ss_bulk_streaming;
			break;

		case USB_SPEED_HIGH:
			uvc_streaming_std = uvc_hs_bulk_streaming;
			break;

		case USB_SPEED_FULL:
		default:
			uvc_streaming_std = uvc_fs_bulk_streaming;
			break;
		}
	}

	/* Descriptors layout
	 *
	 * uvc_iad
	 * uvc_control_intf
	 * Class-specific UVC control descriptors
	 * uvc_control_ep
	 * uvc_control_cs_ep
	 * uvc_ss_control_comp (for SS only)
	 * uvc_streaming_intf_alt0
	 * Class-specific UVC streaming descriptors
	 * uvc_{fs|hs}_streaming
	 */

	/* Count descriptors and compute their size. */
	control_size = 0;
	streaming_size = 0;
	bytes = uvc_iad.bLength + uvc_control_intf.bLength
	      //+ uvc_streaming_intf_alt0.bLength;
		  + streaming_intf_alt0->bLength;
		  
#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	if(uvc->uvc_interrupt_ep_enable)
	{
	    bytes += uvc_status_interrupt_ep.bLength + uvc_status_interrupt_cs_ep.bLength;

		if (speed >= USB_SPEED_SUPER) {
			bytes += uvc_ss_status_interrupt_comp.bLength;
			n_desc = 6;
		} else {
			n_desc = 5;
		}
	}
	else
	{
		if (speed >= USB_SPEED_SUPER) {
			n_desc = 3;
		} else {
			n_desc = 3;
		}
	}
#else
	if (speed >= USB_SPEED_SUPER) {
		n_desc = 3;
	} else {
		n_desc = 3;
	}
#endif

	for (src = (const struct usb_descriptor_header **)uvc_control_desc;
	     *src; ++src) {
		control_size += (*src)->bLength;
		bytes += (*src)->bLength;
		n_desc++;
	}
	for (src = (const struct usb_descriptor_header **)uvc_streaming_cls;
	     *src; ++src) {
		streaming_size += (*src)->bLength;
		bytes += (*src)->bLength;
		n_desc++;
	}
	for (src = uvc_streaming_std; *src; ++src) {
		bytes += (*src)->bLength;
		n_desc++;
	}

	mem = kmalloc((n_desc + 1) * sizeof(*src) + bytes, GFP_KERNEL);
	if (mem == NULL)
		return NULL;

	hdr = mem;
	dst = mem;
	mem += (n_desc + 1) * sizeof(*src);

	/* Copy the descriptors. */
	iad = mem;
	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_iad);
	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_control_intf);

	uvc_control_header = mem;
	UVC_COPY_DESCRIPTORS(mem, dst,
		(const struct usb_descriptor_header **)uvc_control_desc);
	uvc_control_header->wTotalLength = cpu_to_le16(control_size);
	
	printk("bcdUVC:0x%02x\n",uvc_control_header->bcdUVC);
	if(uvc_control_header->bcdUVC == 0x0150)
	{
		uvc_streaming_intf_alt0.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt1.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt2.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt3.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt4.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt5.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt6.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt7.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt8.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt9.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt10.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt11.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt12.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt13.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt14.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt15.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt16.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt17.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt18.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt19.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt20.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt21.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt22.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt23.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt24.bInterfaceProtocol = 1;
		uvc_streaming_intf_alt25.bInterfaceProtocol = 1;
	}
	else
	{	
		uvc_streaming_intf_alt0.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt1.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt2.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt3.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt4.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt5.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt6.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt7.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt8.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt9.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt10.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt11.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt12.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt13.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt14.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt15.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt16.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt17.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt18.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt19.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt20.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt21.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt22.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt23.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt24.bInterfaceProtocol = 0;
		uvc_streaming_intf_alt25.bInterfaceProtocol = 0;
		
	}
	
	uvc_control_header->baInterfaceNr[0] = uvc->streaming_intf;
	if(uvc_control_header->bLength	== UVC_DT_HEADER_SIZE(2))
	{
		iad->bInterfaceCount = 3;
		uvc_control_header->bInCollection = 2;
		uvc_control_header->baInterfaceNr[1] = uvc_control_header->baInterfaceNr[0] + 1;
	}
	else
	{
		iad->bInterfaceCount = 2;
		uvc_control_header->bInCollection = 1;
	}

	//if(uvc->video.bulk_streaming_ep)
	//	iad->bInterfaceCount -= 1;

#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	if(uvc->uvc_interrupt_ep_enable)
	{
		UVC_COPY_DESCRIPTOR(mem, dst, &uvc_status_interrupt_ep);

		if (speed >= USB_SPEED_SUPER)
			UVC_COPY_DESCRIPTOR(mem, dst, &uvc_ss_status_interrupt_comp);

		UVC_COPY_DESCRIPTOR(mem, dst, &uvc_status_interrupt_cs_ep);
	}
#endif
	
	//UVC_COPY_DESCRIPTOR(mem, dst, &uvc_streaming_intf_alt0);
	UVC_COPY_DESCRIPTOR(mem, dst, streaming_intf_alt0);

	uvc_streaming_header = mem;
	UVC_COPY_DESCRIPTORS(mem, dst,
		(const struct usb_descriptor_header**)uvc_streaming_cls);
	uvc_streaming_header->wTotalLength = cpu_to_le16(streaming_size);
	uvc_streaming_header->bEndpointAddress = uvc->video.ep->address;
	
	//hs ep
	uvc_hs_streaming_isoc_ep_1.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_2.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_3.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_4.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_5.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_6.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_7.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_8.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_9.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_10.bEndpointAddress = uvc->video.ep->address;
	uvc_hs_streaming_isoc_ep_11.bEndpointAddress = uvc->video.ep->address;
	
	//ss ep
	uvc_ss_streaming_isoc_ep_1.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_2.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_3.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_4.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_5.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_6.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_7.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_8.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_9.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_10.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_11.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_12.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_13.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_14.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_15.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_16.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_17.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_18.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_19.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_20.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_21.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_22.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_23.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_24.bEndpointAddress = uvc->video.ep->address;
	uvc_ss_streaming_isoc_ep_25.bEndpointAddress = uvc->video.ep->address;
	
	UVC_COPY_DESCRIPTORS(mem, dst, uvc_streaming_std);

	*dst = NULL;
	return hdr;
}

extern struct usb_ep *usb_ep_autoconfig_ss_backwards(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp
);

static int
uvc_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct uvc_device *uvc = to_uvc(f);
	struct usb_string *us;
	unsigned int max_packet_mult;
	unsigned int max_packet_size;
	struct usb_ep *ep;
	struct f_uvc_opts *opts;
	int ret = -EINVAL;

	INFO(cdev, "uvc_function_bind\n");
	
#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	if(uvc->uvc_interrupt_ep_enable)
		spin_lock_init(&uvc->status_interrupt_spinlock);
#endif

	if(c->bConfigurationValue == 2)
		uvc->video.bulk_streaming_ep = 0;
	else
		uvc->video.bulk_streaming_ep = bulk_streaming_ep;
	
	uvc->video.bulk_max_size = bulk_max_size;
	
	opts = to_f_uvc_opts(f->fi);
	/* Sanity check the streaming endpoint module parameters.
	 */
	if (!uvc->video.bulk_streaming_ep) {
		opts->streaming_interval = clamp(opts->streaming_interval, 1U, 16U);
		//opts->streaming_maxpacket = clamp(opts->streaming_maxpacket, 1U, 3072U);
		opts->streaming_maxpacket = clamp(opts->streaming_maxpacket, 1U, 49152U);//48K
		opts->streaming_maxburst = min(opts->streaming_maxburst, 15U);
	} else {
		opts->streaming_maxpacket = clamp(opts->streaming_maxpacket, 1U, 8192U);
		opts->streaming_maxburst = min(opts->streaming_maxburst, 15U);
	}

	/* Fill in the FS/HS/SS Video Streaming specific descriptors from the
	 * module parameters.
	 *
	 * NOTE: We assume that the user knows what they are doing and won't
	 * give parameters that their UDC doesn't support.
	 */
	if (opts->streaming_maxpacket <= 1024) {
		max_packet_mult = 1;
		max_packet_size = opts->streaming_maxpacket;
	} else if (opts->streaming_maxpacket <= 2048) {
		max_packet_mult = 2;
		max_packet_size = opts->streaming_maxpacket / 2;
	} else {
		max_packet_mult = 3;
		max_packet_size = opts->streaming_maxpacket / 3;
	}

	//pr_notice("opts->streaming_maxpacket: %d \n", opts->streaming_maxpacket);
	
	if (!uvc->video.bulk_streaming_ep) {
#if 0
		uvc_fs_streaming_ep.wMaxPacketSize =
			cpu_to_le16(min(opts->streaming_maxpacket, 1023U));
		uvc_fs_streaming_ep.bInterval = opts->streaming_interval;

		
		max_packet_size = min(max_packet_size, 3U);
		uvc_hs_streaming_ep.wMaxPacketSize =
			cpu_to_le16(max_packet_size | ((max_packet_mult - 1) << 11));
		uvc_hs_streaming_ep.bInterval = opts->streaming_interval;

		//USB3.0
		if(opts->streaming_maxpacket > 1024)
		{ 
			opts->streaming_maxpacket = 1024 * (opts->streaming_maxpacket / 1024);//������1024����
			max_packet_mult = opts->streaming_maxpacket / (1024) / (opts->streaming_maxburst + 1) - 1;
			uvc_ss_streaming_ep.wMaxPacketSize = ((opts->streaming_maxpacket / 1024 -1) << 11) | 0x400;
			uvc_ss_streaming_ep.bInterval = opts->streaming_interval;
			
			uvc_ss_streaming_comp.bMaxBurst = opts->streaming_maxburst;
			uvc_ss_streaming_comp.bmAttributes = max_packet_mult;
			uvc_ss_streaming_comp.wBytesPerInterval = (uvc_ss_streaming_comp.bMaxBurst + 1) * (uvc_ss_streaming_comp.bmAttributes + 1) * 1024;
			
		} else {
			uvc_ss_streaming_ep.wMaxPacketSize = cpu_to_le16(opts->streaming_maxpacket);
			uvc_ss_streaming_ep.bInterval = opts->streaming_interval;
			uvc_ss_streaming_comp.bmAttributes =0;
			uvc_ss_streaming_comp.bMaxBurst = 0;
			uvc_ss_streaming_comp.wBytesPerInterval = opts->streaming_maxpacket;
		}
#endif
	} else {
		uvc_fs_bulk_streaming_ep.wMaxPacketSize =
						min(opts->streaming_maxpacket, 64U);

		uvc_hs_bulk_streaming_ep.wMaxPacketSize = 512;
		uvc_ss_bulk_streaming_ep.wMaxPacketSize = max_packet_size;
		uvc_ss_streaming_comp.bMaxBurst = opts->streaming_maxburst;
		uvc_ss_streaming_comp.wBytesPerInterval =
			max_packet_size * opts->streaming_maxburst;
	}

	/* Allocate endpoints. */

	if (gadget_is_superspeed(c->cdev->gadget) ||
		gadget_is_superspeed_plus(c->cdev->gadget))
		if (!uvc->video.bulk_streaming_ep)
			ep = usb_ep_autoconfig_ss(cdev->gadget, &uvc_ss_streaming_ep, &uvc_ss_streaming_comp);
		else
			ep = usb_ep_autoconfig_ss(cdev->gadget, &uvc_ss_bulk_streaming_ep, &uvc_ss_bulk_streaming_comp);
	else if (gadget_is_dualspeed(cdev->gadget))
		if (!uvc->video.bulk_streaming_ep)
			ep = usb_ep_autoconfig(cdev->gadget, &uvc_hs_streaming_ep);
		else
			ep = usb_ep_autoconfig(cdev->gadget, &uvc_hs_bulk_streaming_ep);
	else
		if (!uvc->video.bulk_streaming_ep)
			ep = usb_ep_autoconfig(cdev->gadget, &uvc_fs_streaming_ep);
		else
			ep = usb_ep_autoconfig(cdev->gadget, &uvc_fs_bulk_streaming_ep);

	if (!ep) {
		INFO(cdev, "Unable to allocate streaming EP, usb max_speed : %s\n", usb_speed_string(c->cdev->gadget->max_speed));
		goto error;
	}
	
	if (gadget_is_superspeed(c->cdev->gadget) ||
		gadget_is_superspeed_plus(c->cdev->gadget))
		if (!uvc->video.bulk_streaming_ep)
		{
			ep->desc = (const struct usb_endpoint_descriptor    *)&uvc_ss_streaming_ep;
			ep->comp_desc = (const struct usb_ss_ep_comp_descriptor    *)&uvc_ss_streaming_comp;
		}
		else
		{
			ep->desc = (const struct usb_endpoint_descriptor    *)&uvc_ss_bulk_streaming_ep;
			ep->comp_desc = (const struct usb_ss_ep_comp_descriptor    *)&uvc_ss_bulk_streaming_comp;
		}
	else if (gadget_is_dualspeed(cdev->gadget))
		if (!uvc->video.bulk_streaming_ep)
			ep->desc = &uvc_hs_streaming_ep;
		else
			ep->desc = &uvc_hs_bulk_streaming_ep;
	else
		if (!uvc->video.bulk_streaming_ep)
			ep->desc = &uvc_fs_streaming_ep;
		else
			ep->desc = &uvc_fs_bulk_streaming_ep;
	
	uvc->video.ep = ep;
	ep->driver_data = uvc;
	uvc->video.uvc = uvc;
	
#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
uvc_interrupt_ep_init:
	if(uvc->uvc_interrupt_ep_enable)
	{
		uvc_interrupt_ep_enable(f, 1);

		ep = usb_ep_autoconfig_ss_backwards(cdev->gadget, &uvc_status_interrupt_ep, NULL);
		if (!ep) {
			INFO(cdev, "Unable to allocate control EP\n");
			goto uvc_interrupt_ep_error;
		}
		ep->driver_data = uvc;
		uvc->uvc_status_interrupt_ep = ep;
		
		/* Preallocate control endpoint request. */
		uvc->uvc_status_interrupt_req = usb_ep_alloc_request(uvc->uvc_status_interrupt_ep, GFP_KERNEL);
		uvc->uvc_status_interrupt_buf = kmalloc(UVC_STATUS_MAX_PACKET_SIZE, GFP_KERNEL);
		if (uvc->uvc_status_interrupt_req == NULL || uvc->uvc_status_interrupt_buf == NULL) {
			ret = -ENOMEM;
			printk("%s: kmalloc uvc_status_interrupt_req or uvc_status_interrupt_req error \n", __func__);
			goto uvc_interrupt_ep_error;
		}

		uvc->uvc_status_interrupt_req->buf = uvc->uvc_status_interrupt_buf;
		uvc->uvc_status_interrupt_req->complete = uvc_function_status_interrupt_ep_complete;
		uvc->uvc_status_interrupt_req->context = uvc;
	}
	else
	{
		uvc_interrupt_ep_enable(f, 0);
	}
#endif
	
	uvc->control_ep = cdev->gadget->ep0;

	if (!uvc->video.bulk_streaming_ep) {
		uvc_fs_streaming_ep.bEndpointAddress = uvc->video.ep->address;
		uvc_hs_streaming_ep.bEndpointAddress = uvc->video.ep->address;
		uvc_ss_streaming_ep.bEndpointAddress = uvc->video.ep->address;
	} else {
		uvc_fs_bulk_streaming_ep.bEndpointAddress = uvc->video.ep->address;
		uvc_hs_bulk_streaming_ep.bEndpointAddress = uvc->video.ep->address;
		uvc_ss_bulk_streaming_ep.bEndpointAddress = uvc->video.ep->address;
	}

	us = usb_gstrings_attach(cdev, uvc_function_strings,
				 ARRAY_SIZE(uvc_en_us_strings));
	if (IS_ERR(us)) {
		ret = PTR_ERR(us);
		INFO(cdev, "usb_gstrings_attach 1\n");
		goto error;
	}
	//uvc_iad.iFunction = us[UVC_STRING_CONTROL_IDX].id;
	//uvc_control_intf.iInterface = us[UVC_STRING_CONTROL_IDX].id;
	ret = us[UVC_STRING_STREAMING_IDX].id;
	//uvc_streaming_intf_alt0.iInterface = ret;
	//uvc_streaming_intf_alt1.iInterface = ret;
	//uvc_streaming_intf_alt2.iInterface = ret;
	//uvc_streaming_intf_alt3.iInterface = ret;
	//uvc_streaming_intf_alt4.iInterface = ret;

	/* Allocate interface IDs. */
	if ((ret = usb_interface_id(c, f)) < 0)
	{
		INFO(cdev, "usb_interface_id 1\n");
		goto error;
	}
	
	uvc_iad.bFirstInterface = ret;
	uvc_control_intf.bInterfaceNumber = ret;
	uvc->control_intf = ret;

	if ((ret = usb_interface_id(c, f)) < 0)
	{
		INFO(cdev, "usb_interface_id 2\n");
		goto error;
	}
	
	uvc_streaming_intf_alt0.bInterfaceNumber = ret;
	uvc_streaming_intf_alt1.bInterfaceNumber = ret;
	uvc_streaming_intf_alt2.bInterfaceNumber = ret;
	uvc_streaming_intf_alt3.bInterfaceNumber = ret;
	uvc_streaming_intf_alt4.bInterfaceNumber = ret;
	uvc_streaming_intf_alt5.bInterfaceNumber = ret;
	uvc_streaming_intf_alt6.bInterfaceNumber = ret;
	uvc_streaming_intf_alt7.bInterfaceNumber = ret;
	uvc_streaming_intf_alt8.bInterfaceNumber = ret;
	uvc_streaming_intf_alt9.bInterfaceNumber = ret;
	uvc_streaming_intf_alt10.bInterfaceNumber = ret;
	uvc_streaming_intf_alt11.bInterfaceNumber = ret;
	uvc_streaming_intf_alt12.bInterfaceNumber = ret;
	uvc_streaming_intf_alt13.bInterfaceNumber = ret;
	uvc_streaming_intf_alt14.bInterfaceNumber = ret;
	uvc_streaming_intf_alt15.bInterfaceNumber = ret;
	uvc_streaming_intf_alt16.bInterfaceNumber = ret;
	uvc_streaming_intf_alt17.bInterfaceNumber = ret;
	uvc_streaming_intf_alt18.bInterfaceNumber = ret;
	uvc_streaming_intf_alt19.bInterfaceNumber = ret;
	uvc_streaming_intf_alt20.bInterfaceNumber = ret;
	uvc_streaming_intf_alt21.bInterfaceNumber = ret;
	uvc_streaming_intf_alt22.bInterfaceNumber = ret;
	uvc_streaming_intf_alt23.bInterfaceNumber = ret;
	uvc_streaming_intf_alt24.bInterfaceNumber = ret;
	uvc_streaming_intf_alt25.bInterfaceNumber = ret;
	
	uvc_bulk_streaming_intf_alt0.bInterfaceNumber = ret;
	uvc->streaming_intf = ret;
	
	if(f->config->bConfigurationValue == 2)//uvc1.5 for win10
		usb_ss_endpoint_max_packet_size_switch(0);
	else if(f->config->bConfigurationValue == 3)//uvc1.1 for win7
		usb_ss_endpoint_max_packet_size_switch(1);
	else
		usb_ss_endpoint_max_packet_size_switch(usb3_win7_mode);

	/* Copy descriptors */
	f->fs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_FULL);
	if (gadget_is_dualspeed(cdev->gadget))
		f->hs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_HIGH);
	if (gadget_is_superspeed(c->cdev->gadget))
		f->ss_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_SUPER);
	if (gadget_is_superspeed_plus(c->cdev->gadget))
		f->ssp_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_SUPER_PLUS);

	/* Preallocate control endpoint request. */
	uvc->control_req = usb_ep_alloc_request(cdev->gadget->ep0, GFP_KERNEL);
	uvc->control_buf = kmalloc(UVC_MAX_REQUEST_SIZE, GFP_KERNEL);
	if (uvc->control_req == NULL || uvc->control_buf == NULL) {
		ret = -ENOMEM;
		printk("%s: kmalloc control_req or control_req error \n", __func__);
		goto error;
	}

	uvc->control_req->buf = uvc->control_buf;
	uvc->control_req->complete = uvc_function_ep0_complete;
	uvc->control_req->context = uvc;

	uvc->control_req_completion = uvc_function_ep0_complete;

	init_completion(&uvc->uvc_receive_ep0_completion);

	/* Avoid letting this gadget enumerate until the userspace server is
	 * active.
	 */
	
#if 0
	if ((ret = usb_function_deactivate(f)) < 0)
	{
		printk("%s: usb_function_deactivate error \n", __func__);
		goto error;
	}
#endif

	if (v4l2_device_register(&cdev->gadget->dev, &uvc->v4l2_dev)) {
		printk(KERN_INFO "v4l2_device_register failed\n");
		goto error;
	}

	/* Initialise video. */
	ret = uvcg_video_init(cdev, &uvc->video);
	if (ret < 0)
	{
		INFO(cdev, "uvcg_video_init error\n");
		goto error;
	}

	/* Register a V4L2 device. */
	ret = uvc_register_video(uvc);
	if (ret < 0) {
		printk(KERN_INFO "Unable to register video device\n");
		goto error;
	}

	return 0;

error:
	printk("%s err \n", __func__);
	v4l2_device_unregister(&uvc->v4l2_dev);
	if (uvc->vdev)
		video_device_release(uvc->vdev);

	if (uvc->control_ep)
		uvc->control_ep->driver_data = NULL;
	if (uvc->video.ep)
		uvc->video.ep->driver_data = NULL;

	if (uvc->control_req)
		usb_ep_free_request(cdev->gadget->ep0, uvc->control_req);
	kfree(uvc->control_buf);

#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
uvc_interrupt_ep_error:
	if(uvc->uvc_interrupt_ep_enable)
	{
		if (uvc->uvc_status_interrupt_ep)
			uvc->uvc_status_interrupt_ep->driver_data = NULL;

		if (uvc->uvc_status_interrupt_req)
			usb_ep_free_request(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
		
		if(uvc->uvc_status_interrupt_buf)
			kfree(uvc->uvc_status_interrupt_buf);

		uvc_interrupt_ep_enable(f, 0);
		goto uvc_interrupt_ep_init;
	}
#endif
	usb_free_all_descriptors(f);

    uvcg_video_release(&uvc->video);
	return ret;
}

/* --------------------------------------------------------------------------
 * USB gadget function
 */

static void uvc_free_inst(struct usb_function_instance *f)
{
	struct f_uvc_opts *opts = to_f_uvc_opts(f);

	kfree(opts);
}

static struct usb_function_instance *uvc_alloc_inst(void)
{
	struct f_uvc_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	opts->func_inst.free_func_inst = uvc_free_inst;

	return &opts->func_inst;
}

static void uvc_free(struct usb_function *f)
{
	struct uvc_device *uvc = to_uvc(f);

	kfree(uvc);
}

static void uvc_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct uvc_device *uvc = to_uvc(f);
	struct uvc_video *video = &uvc->video;

	INFO(cdev, "%s, bConfigurationValue:%d \n", __func__, c->bConfigurationValue);

	if (video->async_wq)
		destroy_workqueue(video->async_wq);

	if(uvc->video.ep)
	{
        usb_ep_disable(uvc->video.ep);
		uvc->video.ep->driver_data = NULL;
	}

	video_unregister_device(uvc->vdev);
	v4l2_device_unregister(&uvc->v4l2_dev);
	uvc->control_ep->driver_data = NULL;
	

	if(uvc->control_req)
		usb_ep_free_request(cdev->gadget->ep0, uvc->control_req);
	
	if(uvc->control_buf)
		kfree(uvc->control_buf);
	
#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	if(uvc->uvc_interrupt_ep_enable)
	{
		//unsigned long flags;	
		//spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);
		if (uvc->uvc_status_interrupt_ep)
		{
			if (uvc->uvc_status_interrupt_req)
			{
				usb_ep_dequeue(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
				usb_ep_free_request(uvc->uvc_status_interrupt_ep, uvc->uvc_status_interrupt_req);
                uvc->uvc_status_interrupt_req = NULL;
			}	

			usb_ep_disable(uvc->uvc_status_interrupt_ep);
			uvc->uvc_status_interrupt_ep->driver_data = NULL;
            uvc->uvc_status_interrupt_ep = NULL;
		}
		
		if(uvc->uvc_status_interrupt_buf)
			kfree(uvc->uvc_status_interrupt_buf);
		//spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);
	}
#endif

	usb_free_all_descriptors(f);

    uvcg_video_release(video);
}

static void uvc_function_suspend(struct usb_function *f)
{
	struct uvc_device *uvc = to_uvc(f);
	struct v4l2_event v4l2_event;

	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_SUSPEND;
	v4l2_event_queue(uvc->vdev, &v4l2_event);

	uvc->state = UVC_STATE_SUSPEND;	
}

static void uvc_function_resume(struct usb_function *f)
{
	struct uvc_device *uvc = to_uvc(f);
	struct v4l2_event v4l2_event;

	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UVC_EVENT_RESUME;
	v4l2_event_queue(uvc->vdev, &v4l2_event);

	uvc->state = UVC_STATE_RESUME;	
}

static struct usb_function *uvc_alloc(struct usb_function_instance *fi)
{
	struct uvc_device *uvc;
	struct f_uvc_opts *opts;

	uvc = kzalloc(sizeof(*uvc), GFP_KERNEL);
	if (uvc == NULL)
		return ERR_PTR(-ENOMEM);

	uvc->state = UVC_STATE_DISCONNECTED;
	opts = to_f_uvc_opts(fi);

	//uvc->debug = &debug;

	uvc->video.bulk_streaming_ep = bulk_streaming_ep;
	uvc->dfu_enable = opts->dfu_enable;
	uvc->uac_enable = opts->uac_enable;
	uvc->uvc_s2_enable = opts->uvc_s2_enable;
	uvc->uvc_v150_enable = opts->uvc_v150_enable;
	uvc->win7_usb3_enable = opts->win7_usb3_enable;
	uvc->work_pump_enable = opts->work_pump_enable;
	uvc->zte_hid_enable = opts->zte_hid_enable;
	uvc->uvc_interrupt_ep_enable = opts->uvc_interrupt_ep_enable;
	uvc->usb_sn_string = opts->usb_sn_string;
	uvc->usb_sn_string_length = opts->usb_sn_string_length;
	uvc->usb_vendor_string = opts->usb_vendor_string;
	uvc->usb_vendor_string_length = opts->usb_vendor_string_length;
	uvc->usb_product_string = opts->usb_product_string;
	uvc->usb_product_string_length = opts->usb_product_string_length;
	uvc->auto_queue_data_enable = opts->auto_queue_data_enable;

	uvc->desc.fs_control = opts->fs_control;
	uvc->desc.ss_control = opts->ss_control;
	uvc->desc.fs_streaming = opts->fs_streaming;
	uvc->desc.hs_streaming = opts->hs_streaming;
	uvc->desc.ss_streaming = opts->ss_streaming;

	/* Register the function. */
	uvc->func.name = "uvc";
	uvc->func.bind = uvc_function_bind;
	uvc->func.unbind = uvc_unbind;
	uvc->func.get_alt = uvc_function_get_alt;
	uvc->func.set_alt = uvc_function_set_alt;
	uvc->func.disable = uvc_function_disable;
	uvc->func.setup = uvc_function_setup;
	uvc->func.free_func = uvc_free;
	uvc->func.suspend = uvc_function_suspend;
	uvc->func.resume = uvc_function_resume;
	//uvc->func.switch_config = uvc_function_switch_config;

	return &uvc->func;
}

DECLARE_USB_FUNCTION_INIT(uvc, uvc_alloc_inst, uvc_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("VHD");
