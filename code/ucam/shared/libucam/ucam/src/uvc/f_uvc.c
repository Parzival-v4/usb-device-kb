/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\f_uvc.c
 * @Description : 
 */ 
#include <linux/usb/ch9.h>
//#include <linux/usb/gadget.h>

#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/video.h>
#include <ucam/ucam.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_vs.h>


#define UVC_INTF_STREAMING_NUM					0
#define UVC_STREAMING_INTERFACE_PROTOCOL		UVC_PC_PROTOCOL_UNDEFINED//UVC1.5 0x01,UVC1.1 0x00	
#define UVC_STATUS_MAX_PACKET_SIZE				64	/* 16 bytes status */



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
	.bNumEndpoints		= 0,
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
	.wMaxPacketSize		= 64,
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

#if 0
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
#endif

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

#if 0

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
#endif


static const struct usb_descriptor_header * uvc_fs_streaming[] = {
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
	NULL,
};

static const struct usb_descriptor_header * uvc_hs_streaming[] = {
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

static const struct usb_descriptor_header * uvc_ss_streaming[] = {
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

#if 0	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt24,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_24,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_24,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt25,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_25,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_25,
#endif

#endif	
	NULL,
};

//todo
static const struct usb_descriptor_header * uvc_ssp_streaming[] = {
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

#if 0	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt24,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_24,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_24,
	
	(const struct usb_descriptor_header *) &uvc_streaming_intf_alt25,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_ep_25,
	(const struct usb_descriptor_header *) &uvc_ss_streaming_isoc_comp_25,
#endif

#endif	
	NULL,
};

static struct usb_endpoint_descriptor uvc_hs_bulk_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= 512,
	.bInterval		= 0,
};


static struct usb_endpoint_descriptor uvc_ss_bulk_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= 1024,
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

static struct usb_endpoint_descriptor uvc_ssp_bulk_streaming_ep = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,

	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= 1024,
	.bInterval		= 0,
};

static struct usb_ss_ep_comp_descriptor uvc_ssp_bulk_streaming_comp = {
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

static const struct usb_descriptor_header * const uvc_ssp_bulk_streaming[] = {
	(struct usb_descriptor_header *) &uvc_ssp_bulk_streaming_ep,
	(struct usb_descriptor_header *) &uvc_ssp_bulk_streaming_comp,
	NULL,
};



//USB3.0 EP模式，0：正常模式（WIN 10）,1：USB2.0模式（WIN 7）
static void usb_ss_endpoint_max_packet_size_switch(struct uvc_dev *dev, uint8_t mode)
{
	const struct usb_descriptor_header * const *src;
	struct usb_endpoint_descriptor *uvc_streaming_isoc_ep;

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
	//uvc_ss_streaming_isoc_ep_24.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT24, USB3_EP_ISOC_PKT_SIZE_ALT24);
	//uvc_ss_streaming_isoc_ep_25.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(USB3_EP_ISOC_PKTS_COUNT_ALT25, USB3_EP_ISOC_PKT_SIZE_ALT25);
	

	if(mode == 0)
	{
		ucam_strace("Windows 10 USB3.0 ISOC Mode");
		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
		{
			for (src = uvc_ss_streaming; *src; ++src) 
			{
				if((*src)->bDescriptorType == USB_DT_ENDPOINT)
				{
					uvc_streaming_isoc_ep = (struct usb_endpoint_descriptor *)*src;
					if(uvc_streaming_isoc_ep->wMaxPacketSize > 1024)
						uvc_streaming_isoc_ep->wMaxPacketSize = 1024;
				}
			}

			if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
			{
				for (src = uvc_ssp_streaming; *src; ++src) 
				{
					if((*src)->bDescriptorType == USB_DT_ENDPOINT)
					{
						uvc_streaming_isoc_ep = (struct usb_endpoint_descriptor *)*src;
						if(uvc_streaming_isoc_ep->wMaxPacketSize > 1024)
							uvc_streaming_isoc_ep->wMaxPacketSize = 1024;
					}
				}	
			}
		}
	}
	else
	{
		ucam_strace("Windows 7 USB3.0 ISOC Mode");
	}
}


static void set_uvc_streaming(struct uvc_dev *dev, __u8 bInterfaceNumber, __u8  bInterfaceProtocol, __u8 bEndpointAddress)
{
	const struct usb_descriptor_header * const *src;
	struct usb_interface_descriptor *uvc_streaming_intf_alt;
	struct usb_endpoint_descriptor *uvc_streaming_isoc_ep;

	ucam_strace("InterfaceNumber:%d, EndpointAddress:0x%02x", bInterfaceNumber, bEndpointAddress);

	uvc_bulk_streaming_intf_alt0.bInterfaceNumber = bInterfaceNumber;
	uvc_bulk_streaming_intf_alt0.bInterfaceProtocol = bInterfaceProtocol;

	uvc_streaming_intf_alt0.bInterfaceNumber = bInterfaceNumber;
	uvc_streaming_intf_alt0.bInterfaceProtocol = bInterfaceProtocol;

	uvc_fs_bulk_streaming_ep.bEndpointAddress = bEndpointAddress;
	uvc_hs_bulk_streaming_ep.bEndpointAddress = bEndpointAddress;
	uvc_ss_bulk_streaming_ep.bEndpointAddress = bEndpointAddress;
	uvc_ssp_bulk_streaming_ep.bEndpointAddress = bEndpointAddress;
	
	for (src = uvc_fs_streaming; *src; ++src) 
	{
		if((*src)->bDescriptorType == USB_DT_INTERFACE)
		{
			uvc_streaming_intf_alt = (struct usb_interface_descriptor *)*src;
			uvc_streaming_intf_alt->bInterfaceNumber = bInterfaceNumber;
			uvc_streaming_intf_alt->bInterfaceProtocol = bInterfaceProtocol;

		}
		if((*src)->bDescriptorType == USB_DT_ENDPOINT)
		{
			uvc_streaming_isoc_ep = (struct usb_endpoint_descriptor *)*src;
			uvc_streaming_isoc_ep->bEndpointAddress = bEndpointAddress;
		}
	}


	for (src = uvc_hs_streaming; *src; ++src) 
	{
		if((*src)->bDescriptorType == USB_DT_INTERFACE)
		{
			uvc_streaming_intf_alt = (struct usb_interface_descriptor *)*src;
			uvc_streaming_intf_alt->bInterfaceNumber = bInterfaceNumber;
			uvc_streaming_intf_alt->bInterfaceProtocol = bInterfaceProtocol;

		}
		if((*src)->bDescriptorType == USB_DT_ENDPOINT)
		{
			uvc_streaming_isoc_ep = (struct usb_endpoint_descriptor *)*src;
			uvc_streaming_isoc_ep->bEndpointAddress = bEndpointAddress;
		}
	}

	if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
	{
		for (src = uvc_ss_streaming; *src; ++src) 
		{
			if((*src)->bDescriptorType == USB_DT_INTERFACE)
			{
				uvc_streaming_intf_alt = (struct usb_interface_descriptor *)*src;
				uvc_streaming_intf_alt->bInterfaceNumber = bInterfaceNumber;
				uvc_streaming_intf_alt->bInterfaceProtocol = bInterfaceProtocol;

			}
			if((*src)->bDescriptorType == USB_DT_ENDPOINT)
			{
				uvc_streaming_isoc_ep = (struct usb_endpoint_descriptor *)*src;
				uvc_streaming_isoc_ep->bEndpointAddress = bEndpointAddress;
			}
		}

		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
		{
			for (src = uvc_ssp_streaming; *src; ++src) 
			{
				if((*src)->bDescriptorType == USB_DT_INTERFACE)
				{
					uvc_streaming_intf_alt = (struct usb_interface_descriptor *)*src;
					uvc_streaming_intf_alt->bInterfaceNumber = bInterfaceNumber;
					uvc_streaming_intf_alt->bInterfaceProtocol = bInterfaceProtocol;

				}
				if((*src)->bDescriptorType == USB_DT_ENDPOINT)
				{
					uvc_streaming_isoc_ep = (struct usb_endpoint_descriptor *)*src;
					uvc_streaming_isoc_ep->bEndpointAddress = bEndpointAddress;
				}
			}
		}
	}
}


static void set_uvc_control_intf(__u8 bInterfaceNumber, __u8  bInterfaceProtocol)
{
	uvc_iad.bFirstInterface = bInterfaceNumber;
	
	uvc_control_intf.bInterfaceNumber = bInterfaceNumber;
	uvc_control_intf.bInterfaceProtocol = bInterfaceProtocol;
}

static void set_uvc_status_interrupt_ep(__u8 bEndpointAddress)
{
	uvc_status_interrupt_ep.bEndpointAddress = bEndpointAddress;
}

extern struct uvc_output_terminal_descriptor uvc_output_terminal;
extern struct uvc_output_terminal_descriptor uvc_output_terminal_s2;
extern struct uvc_output_terminal_descriptor uvc150_output_terminal_s2;

int uvc_copy_descriptors(struct uvc_dev *dev, enum ucam_usb_device_speed speed, struct uvc_ioctl_desc_t *descriptor)
{
	struct uvc_input_header_descriptor *uvc_streaming_header;
	struct usb_interface_assoc_descriptor *iad;
	struct uvc_header_descriptor *uvc_control_header;
	const struct uvc_descriptor_header * const *uvc_control_desc;
	const struct uvc_descriptor_header * const *uvc_streaming_cls;
	const struct usb_descriptor_header * const *uvc_streaming_std;
	const struct usb_descriptor_header * const *src;
	struct usb_interface_descriptor *streaming_intf_alt0;
	//struct usb_descriptor_header **dst;
	//struct usb_descriptor_header **hdr;
	unsigned int control_size;
	unsigned int streaming_size;
	unsigned int n_desc;
	unsigned int bytes;
	void *mem;
	int uvc_s2_dev_show = 0;


	struct ucam_list_head *pos;
    const struct uvc_xu_ctrl_attr_t *uvc_xu_ctrl_attr;

	unsigned int bulk_streaming_ep;
	__u8  bInterfaceProtocol;


	ucam_strace("usb:%s", usb_speed_string(speed));

	
	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK && dev->bConfigurationValue != 2)
		bulk_streaming_ep = 1;
	else
		bulk_streaming_ep = 0;

	if(bulk_streaming_ep)
	{
		ucam_notice( "bulk_streaming_ep:%d", bulk_streaming_ep);
	}
		
	if(speed >= UCAM_USB_SPEED_SUPER)
	{
		if(dev->bConfigurationValue == 3)
			usb_ss_endpoint_max_packet_size_switch(dev, 1);
		else if(dev->bConfigurationValue == 1)
			usb_ss_endpoint_max_packet_size_switch(dev, dev->config.usb3_isoc_win7_mode);
		else
			usb_ss_endpoint_max_packet_size_switch(dev, 0);
	}	
	
	if((dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable) || 
		dev->config.uvc_protocol == UVC_PROTOCOL_V150)
		bInterfaceProtocol = UVC_PC_PROTOCOL_15;
	else
		bInterfaceProtocol = UVC_PC_PROTOCOL_UNDEFINED;
	
	
	set_uvc_control_intf(dev->control_intf, bInterfaceProtocol);	
	set_uvc_streaming(dev, dev->streaming_intf, bInterfaceProtocol, dev->streaming_ep_address);
	set_uvc_status_interrupt_ep(dev->status_interrupt_ep_address);


	uvc_control_desc = (const struct uvc_descriptor_header * const *)
		get_uvc_control_descriptors(dev, speed);
	uvc_streaming_cls = (const struct uvc_descriptor_header * const *)
		get_uvc_streaming_cls_descriptor(dev, speed);

	
	if (!bulk_streaming_ep)
		streaming_intf_alt0 = &uvc_streaming_intf_alt0;
	else
		streaming_intf_alt0 = &uvc_bulk_streaming_intf_alt0;

	switch (speed) {
		case UCAM_USB_SPEED_SUPER_PLUS:
			if (bulk_streaming_ep)
				uvc_streaming_std = uvc_ssp_bulk_streaming;
			else
				uvc_streaming_std = uvc_ssp_streaming;
			break;
		case UCAM_USB_SPEED_SUPER:
			if (bulk_streaming_ep)
				uvc_streaming_std = uvc_ss_bulk_streaming;
			else
				uvc_streaming_std = uvc_ss_streaming;
			break;
		case UCAM_USB_SPEED_HIGH:
			if (bulk_streaming_ep)
				uvc_streaming_std = uvc_hs_bulk_streaming;
			else
				uvc_streaming_std = uvc_hs_streaming;
			break;
		case UCAM_USB_SPEED_FULL:
		default:
			if (bulk_streaming_ep)
				uvc_streaming_std = uvc_fs_bulk_streaming;
			else
				uvc_streaming_std = uvc_fs_streaming;
			break;
	}

	if (dev->stream_num != 0 && dev->bConfigurationValue != 2)
	{
		uvc_s2_dev_show = dev->uvc->uvc_s2_enable & 0x80;
		if(uvc_s2_dev_show)
		{
			ucam_notice("uvc_s2_dev_show intf:0x%02x,0x%02x",dev->control_intf,dev->streaming_intf);
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
	bytes = streaming_intf_alt0->bLength;
	n_desc = 1;
	if(dev->stream_num == 0)
	{
		bytes += uvc_iad.bLength + uvc_control_intf.bLength;
		n_desc += 2;

		if((dev->status_interrupt_ep_address & 0x0F) != 0)
		{
			uvc_control_intf.bNumEndpoints	= 1;
			bytes += uvc_status_interrupt_ep.bLength + uvc_status_interrupt_cs_ep.bLength;

			n_desc += 2;

			if (speed >= UCAM_USB_SPEED_SUPER) {
				bytes += uvc_ss_status_interrupt_comp.bLength;
				n_desc ++;
			} 
		}
		else
		{
			uvc_control_intf.bNumEndpoints	= 0;

		}


		for (src = (const struct usb_descriptor_header **)uvc_control_desc;
			*src; ++src) {
			control_size += (*src)->bLength;
			bytes += (*src)->bLength;
			n_desc++;
		}

        ucam_list_for_each(pos, &dev->uvc->uvc_xu_ctrl_list)
        {
            uvc_xu_ctrl_attr = ucam_list_entry(pos, struct uvc_xu_ctrl_attr_t, list);
            if (uvc_xu_ctrl_attr)
            {
                if (!uvc_xu_ctrl_attr->disable && uvc_xu_ctrl_attr->desc)
                {
                    control_size += uvc_xu_ctrl_attr->desc->bLength;
					bytes += uvc_xu_ctrl_attr->desc->bLength;
					n_desc++;
                }
            }
        }


		control_size += uvc_output_terminal.bLength;
		bytes += uvc_output_terminal.bLength;
		n_desc++;

		if(dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable)
		{
			control_size += uvc150_output_terminal_s2.bLength;
			bytes += uvc150_output_terminal_s2.bLength;
			n_desc++;
		}		
		else if(dev->uvc->uvc_s2_enable && !(dev->uvc->uvc_s2_enable & 0x80))
		{
			control_size += uvc_output_terminal_s2.bLength;
			bytes += uvc_output_terminal_s2.bLength;
			n_desc++;	
		}
	} 
	else if(uvc_s2_dev_show)
	{
		bytes += uvc_iad.bLength + uvc_control_intf.bLength;
		n_desc += 2;

		uvc_control_intf.bNumEndpoints	= 0;
		

		for (src = (const struct usb_descriptor_header **)uvc_control_desc;
			*src; ++src) {
			control_size += (*src)->bLength;
			bytes += (*src)->bLength;
			n_desc++;
		}
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

	ucam_notice("Copy the %s descriptors, bytes:%d", usb_speed_string(speed), bytes);

	//mem = malloc(bytes);
	if(bytes > descriptor->desc_buf_phy_length)
	{
		ucam_error("descriptor length out of range, %d > %d", bytes, descriptor->desc_buf_phy_length);
		return -1;
	}

	mem = descriptor->desc_buf;


	if(mem == NULL)
		return -1;

	//descriptor->n_desc = (n_desc + 1) * sizeof(*src);

	descriptor->n_desc = n_desc;
	descriptor->desc_buf_length = bytes;
	descriptor->speed = speed;
	//descriptor->desc_buf = mem;

	
	/* Copy the descriptors. */

	if(dev->stream_num == 0)
	{
		iad = mem;

		UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_iad);
		UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_control_intf);

		uvc_control_header = mem;
		UVC_COPY_DESCRIPTORS_TO_BUF(mem,
			(const struct usb_descriptor_header **)uvc_control_desc);

        ucam_list_for_each(pos, &dev->uvc->uvc_xu_ctrl_list)
        {
            uvc_xu_ctrl_attr = ucam_list_entry(pos, struct uvc_xu_ctrl_attr_t, list);
            if (uvc_xu_ctrl_attr)
            {
                if (!uvc_xu_ctrl_attr->disable && uvc_xu_ctrl_attr->desc)
                {
                    UVC_COPY_DESCRIPTOR_TO_BUF(mem, uvc_xu_ctrl_attr->desc);
                }
            }
        }


		UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_output_terminal);
		if(dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable)
		{
			UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc150_output_terminal_s2);
		}
		else if(dev->uvc->uvc_s2_enable && !(dev->uvc->uvc_s2_enable & 0x80))
		{		
			UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_output_terminal_s2);
		}

		uvc_control_header->wTotalLength = cpu_to_le16(control_size);
		
		//printk("bcdUVC:0x%02x\n",uvc_control_header->bcdUVC);
		if(uvc_control_header->bcdUVC == 0x0150)
		{
			bInterfaceProtocol = UVC_PC_PROTOCOL_15;
		}
		else
		{	
			bInterfaceProtocol = UVC_PC_PROTOCOL_UNDEFINED;
		}
		
		set_uvc_streaming(dev, dev->streaming_intf, bInterfaceProtocol, dev->streaming_ep_address);
		
		
		uvc_control_header->baInterfaceNr[0] = dev->streaming_intf;
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

		//if(bulk_streaming_ep)
		//	iad->bInterfaceCount -= 1;


		if((dev->status_interrupt_ep_address & 0x0F) != 0)
		{
			UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_status_interrupt_ep);

			if (speed >= UCAM_USB_SPEED_SUPER)
				UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_ss_status_interrupt_comp);

			UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_status_interrupt_cs_ep);
		
		}
	}
	else if(uvc_s2_dev_show)
	{
		iad = mem;

		UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_iad);
		UVC_COPY_DESCRIPTOR_TO_BUF(mem, &uvc_control_intf);


		uvc_control_header = mem;
		UVC_COPY_DESCRIPTORS_TO_BUF(mem,
			(const struct usb_descriptor_header **)uvc_control_desc);
		

		uvc_control_header->wTotalLength = cpu_to_le16(control_size);
		
		//printk("bcdUVC:0x%02x\n",uvc_control_header->bcdUVC);
		if(uvc_control_header->bcdUVC == 0x0150)
		{
			bInterfaceProtocol = UVC_PC_PROTOCOL_15;
		}
		else
		{	
			bInterfaceProtocol = UVC_PC_PROTOCOL_UNDEFINED;
		}
		
		set_uvc_streaming(dev, dev->streaming_intf, bInterfaceProtocol, dev->streaming_ep_address);
		
		
		uvc_control_header->baInterfaceNr[0] = dev->streaming_intf;
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
	}


	UVC_COPY_DESCRIPTOR_TO_BUF(mem, streaming_intf_alt0);

	uvc_streaming_header = mem;
	UVC_COPY_DESCRIPTORS_TO_BUF(mem,
		(const struct usb_descriptor_header**)uvc_streaming_cls);
	uvc_streaming_header->wTotalLength = cpu_to_le16(streaming_size);
	uvc_streaming_header->bEndpointAddress = dev->streaming_ep_address;

	UVC_COPY_DESCRIPTORS_TO_BUF(mem, uvc_streaming_std);
	
	return 0;
}
