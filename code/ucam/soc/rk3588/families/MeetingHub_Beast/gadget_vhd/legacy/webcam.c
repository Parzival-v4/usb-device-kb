/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) ValueHD Corporation. All rights reserved
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
//#include <linux/usb/video.h>

#include <linux/ioport.h>
#include <linux/io.h>
//#include <mach/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "usb/video.h"
#include "u_uvc.h"
#include "uvc_config.h"




#ifdef USB_MULTI_HID_SUPPORT 
#include "../function/f_hid.c"
#endif


#ifdef UAC_AUDIO_SUPPORT
//Ron_Lee 20180124
#include "audio.inl"
//
#endif

#ifdef DFU_SUPPORT 
#include "../function/f_dfu.c"
#endif


#ifdef USB_ZTE_HID_SUPPORT
#include "../function/f_zte_hid.c"
#include "../function/f_zte_upgrade_hid.c"
#endif

#ifdef USB_MULTI_SUPPORT
#include "multi_inl.c"
#endif

#ifdef USB_MULTI_MSG_SUPPORT
#include "multi_msg_inl.c"
#endif

#ifdef USB_POLYCOM_HID_SUPPORT
#include "../function/f_hid_polycom.c"

static unsigned int g_polycom_hid_enable = 0;
module_param(g_polycom_hid_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_polycom_hid_enable, "Polycom HID Enable");
#endif


//Ron_Lee 20180627
static int g_vendor_id = 0x2e7e;
static int g_customer_id = 0x0703;
static int g_device_bcd = 0x0001;
static char g_usb_vendor_label_arr[64] = {0}; 
static char *g_usb_vendor_label1 = "HD Camera";
static char *g_usb_vendor_label2 = NULL;
static char *g_usb_vendor_label3 = NULL;
static char *g_usb_vendor_label4 = NULL;

static char g_usb_product_label_arr[64] = {0}; 
static char *g_usb_product_label1 = "HD Camera";
static char *g_usb_product_label2 = NULL;
static char *g_usb_product_label3 = NULL;
static char *g_usb_product_label4 = NULL;
static char *g_usb_product_label5 = NULL;

static char g_serial_number_arr[64] = {0}; 
static char *g_serial_number = "123";


//static int g_audio_enable = 1;
//static int g_usb_uvc_mode = 0; 

module_param(g_vendor_id, int , 0644);
MODULE_PARM_DESC(g_vendor_id, "Vendor ID");

module_param(g_customer_id, int , 0644);
MODULE_PARM_DESC(g_customer_id, "Customer ID");

module_param(g_device_bcd, int , 0644);
MODULE_PARM_DESC(g_device_bcd, "Device BCD");

//module_param(g_firmware_version, charp , 0644);
//MODULE_PARM_DESC(g_firmware_version, "Firmware Version");

//module_param(g_device_name, charp , 0644);
//MODULE_PARM_DESC(g_device_name, "HD Camera");

#if 1
module_param(g_usb_vendor_label1, charp, 0644);
module_param(g_usb_vendor_label2, charp, 0644);
module_param(g_usb_vendor_label3, charp, 0644);
module_param(g_usb_vendor_label4, charp, 0644);
MODULE_PARM_DESC(g_usb_vendor_label1,"Vendor Label");

module_param(g_usb_product_label1, charp, 0644);
module_param(g_usb_product_label2, charp, 0644);
module_param(g_usb_product_label3, charp, 0644);
module_param(g_usb_product_label4, charp, 0644);
module_param(g_usb_product_label5, charp, 0644);
MODULE_PARM_DESC(g_usb_product_label1,"Product Label");
#else
staitc int n_para;
module_param_array(g_usb_vendor_label, char, &n_para, S_IRUGO);
module_param_array(g_usb_product_label, char, &n_para, S_IRUGO);
#endif

//static int flag_3519v101 = 1;
//const struct uvc_descriptor_header *p_h264_4k_descriptor_header = NULL;
//

USB_GADGET_COMPOSITE_OPTIONS();

extern void get_ep_max_num (struct usb_gadget *gadget, uint32_t *in_epnum, uint32_t *out_epnum);


#ifdef SFB_HID_SUPPORT
extern int __init hidg_bind_config_consumer(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc, int mode);
extern int __init ghid_setup_consumer(struct usb_gadget *g, u8 bConfigurationValue);


extern int __init hidg_bind_config_keyboard(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc);
extern int __init ghid_setup_keyboard(struct usb_gadget *g, u8 bConfigurationValue);
#endif

/*-------------------------------------------------------------------------*/
//serial number       linshiheng 20180913
module_param(g_serial_number, charp, 0644);
MODULE_PARM_DESC(g_serial_number,"Serial Number");
//audio_enable
static unsigned int g_audio_enable = 1;
module_param(g_audio_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_audio_enable, "Audio Enable flag");

//uvc_enable
static unsigned int g_uvc_enable = 1;
module_param(g_uvc_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_uvc_enable, "UVC Enable");

//usb_gadget_connect
static unsigned int usb_connect = 0;
module_param(usb_connect, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(usb_connect, "USB connect");

//dfu_enable
static unsigned int g_dfu_enable = 1;
module_param(g_dfu_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_dfu_enable, "DFU Enable");

//dfu_last
static unsigned int g_dfu_last = 0;
module_param(g_dfu_last, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_dfu_last, "DFU interface last");

//uvc_s2_enable
static unsigned int g_uvc_s2_enable = 0;
module_param(g_uvc_s2_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_uvc_s2_enable, "UVC Stream 2 Enable");

//g_uvc_v150_enable
static unsigned int g_uvc_v150_enable = 1;
module_param(g_uvc_v150_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_uvc_v150_enable, "UVC 1.5 Enable");

//g_win7_usb3_enable
static unsigned int g_win7_usb3_enable = 1;
module_param(g_win7_usb3_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_win7_usb3_enable, "Win7 USB3.0 Enable");
static unsigned int work_pump_enable = 1;
module_param(work_pump_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(work_pump_enable, "work_pump_enable");

//zte_hid_enable
static unsigned int g_zte_hid_enable = 0;
module_param(g_zte_hid_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_zte_hid_enable, "ZTE HID Enable");

static unsigned int g_zte_upgrade_hid_enable = 0;
module_param(g_zte_upgrade_hid_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_zte_upgrade_hid_enable, "ZTE HID Upgrade Enable");

//uvc_interrupt_ep_enable
static unsigned int g_uvc_interrupt_ep_enable = 1;
module_param(g_uvc_interrupt_ep_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_uvc_interrupt_ep_enable, "UVC Interrupt EP Enable");

//hidu_enable
static unsigned int g_hid_enable = 1;
module_param(g_hid_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_hid_enable, "HID Enable");

//hid_interrupt_ep_enable
static unsigned int g_hid_interrupt_ep_enable = 0;
module_param(g_hid_interrupt_ep_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_hid_interrupt_ep_enable, "HID Interrupt EP Enable");

//uvc_request_auto_enable
static unsigned int g_uvc_request_auto_enable = 1;
module_param(g_uvc_request_auto_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_uvc_request_auto_enable, "uvc request Auto Enable");

//sfb_hid_enable
static unsigned int g_sfb_hid_enable = 0;
module_param(g_sfb_hid_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_sfb_hid_enable, "SFB HID Enable");

//sfb_hid_enable
static unsigned int g_sfb_keyboard_enable = 0;
module_param(g_sfb_keyboard_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_sfb_keyboard_enable, "SFB Keyboard Enable");

//rndis_enable
static unsigned int g_rndis_enable = 0;
module_param(g_rndis_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_rndis_enable, "RNDIS Enable");

//msg_enable
static unsigned int g_msg_enable = 0;
module_param(g_msg_enable, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_msg_enable, "MSG Enable");

static unsigned int self_power = 0;
module_param(self_power, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(self_power, "usb self power");


//iManufacturer string
static unsigned int ManufacturerStrings = 0;
module_param(ManufacturerStrings, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(ManufacturerStrings, "iManufacturer String Enable");


//frame_limit
static int g_frame_limit = 15;
module_param(g_frame_limit, int, 0644);
MODULE_PARM_DESC(g_frame_limit, "Frame limit parameter");

/* module parameters specific to the Video streaming endpoint */
static unsigned int streaming_interval = 1;
module_param(streaming_interval, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_interval, "1 - 16");

static unsigned int streaming_maxpacket = 3072;
module_param(streaming_maxpacket, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxpacket, "1 - 1023 (FS), 1 - 3072 (hs/ss)");

static unsigned int streaming_maxburst = 14;
module_param(streaming_maxburst, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxburst, "0 - 15 (ss only)");

static unsigned int trace;
module_param(trace, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(trace, "Trace level bitmask");
/* --------------------------------------------------------------------------
 * Device descriptor
 */

#ifdef PLESCO
#define WEBCAM_VENDOR_ID		0x2e7e	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0800	/* Webcam A/V gadget */
#else
#define WEBCAM_VENDOR_ID		0x2e7e	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0703	/* Webcam A/V gadget */
#endif

#define WEBCAM_DEVICE_BCD		0x0012	/* 0.11 */


static char webcam_config_label[] = "Video";

//static char

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string webcam_strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = g_usb_vendor_label_arr,
	[USB_GADGET_PRODUCT_IDX].s = g_usb_product_label_arr,
	[USB_GADGET_SERIAL_IDX].s = g_serial_number_arr,
	[STRING_DESCRIPTION_IDX].s = webcam_config_label,
	{  }
};

static struct usb_gadget_strings webcam_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = webcam_strings,
};

static struct usb_gadget_strings *webcam_device_strings[] = {
	&webcam_stringtab,
	NULL,
};

#ifdef USB_UVC_SUPPORT
static struct usb_function_instance *fi_uvc;
static struct usb_function *f_uvc;

static struct usb_function_instance *fi_uvc_s2;
static struct usb_function *f_uvc_s2;
#endif


//UVC_HEADER
DECLARE_UVC_HEADER_DESCRIPTOR(1);
DECLARE_UVC_HEADER_DESCRIPTOR(2);

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 2);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 3);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 4);


//YUY2
DECLARE_UVC_FRAME_UNCOMPRESSED(1);
DECLARE_UVC_FRAME_UNCOMPRESSED(2);
DECLARE_UVC_FRAME_UNCOMPRESSED(3);
DECLARE_UVC_FRAME_UNCOMPRESSED(4);
DECLARE_UVC_FRAME_UNCOMPRESSED(5);
DECLARE_UVC_FRAME_UNCOMPRESSED(6);
DECLARE_UVC_FRAME_UNCOMPRESSED(7);
DECLARE_UVC_FRAME_UNCOMPRESSED(8);
DECLARE_UVC_FRAME_UNCOMPRESSED(9);
DECLARE_UVC_FRAME_UNCOMPRESSED(10);

//MJPEG
DECLARE_UVC_FRAME_MJPEG(2);
DECLARE_UVC_FRAME_MJPEG(3);
DECLARE_UVC_FRAME_MJPEG(4);
DECLARE_UVC_FRAME_MJPEG(8);

//H264 UVC1.1
DECLARE_UVC_FRAME_H264_BASE(2);
DECLARE_UVC_FRAME_H264_BASE(3);
DECLARE_UVC_FRAME_H264_BASE(4);
DECLARE_UVC_FRAME_H264_BASE(8);

//H264 UVC1.5
DECLARE_UVC_FRAME_H264_V150(2);
DECLARE_UVC_FRAME_H264_V150(3);
DECLARE_UVC_FRAME_H264_V150(4);
DECLARE_UVC_FRAME_H264_V150(8);

DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1,2);
DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1,3);



static struct usb_device_descriptor webcam_device_descriptor = {
	.bLength			= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB				= cpu_to_le16(0x0200),
	.bDeviceClass		= USB_CLASS_MISC,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
	.bMaxPacketSize0	= 0, /* dynamic */
	.idVendor			= cpu_to_le16(WEBCAM_VENDOR_ID),
	.idProduct			= cpu_to_le16(WEBCAM_PRODUCT_ID),
	.bcdDevice			= cpu_to_le16(WEBCAM_DEVICE_BCD),
	.iManufacturer		= 0, /* dynamic */
	.iProduct			= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};


static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};


/* --------------------------------------------------------------------------
 * UVC1.1 configuration
 */
 
static struct UVC_HEADER_DESCRIPTOR(1) uvc_control_header = {
	.bLength			= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};


static const struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength			= UVC_DT_CAMERA_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_INPUT_TERMINAL,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal			= 0,
	
#ifdef UVC_PTZ_CTRL_SUPPORT
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(16384),
	.wOcularFocalLength		= cpu_to_le16(20),
	.bControlSize		= 3,
	
	.bmControls.b.scanning_mode = UVC_CT_SCANNING_MODE_CONTROL_SUPPORTED,
	.bmControls.b.ae_mode = UVC_CT_AE_MODE_CONTROL_SUPPORTED,
	.bmControls.b.ae_priority = UVC_CT_AE_PRIORITY_CONTROL_SUPPORTED,
	.bmControls.b.exposure_time_absolute = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.exposure_time_relative = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.focus_absolute = UVC_CT_FOCUS_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.focus_relative = UVC_CT_FOCUS_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.iris_absolute = UVC_CT_IRIS_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.iris_relative = UVC_CT_IRIS_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.zoom_absolute = UVC_CT_ZOOM_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.zoom_relative = UVC_CT_ZOOM_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.pantilt_absolute = UVC_CT_PANTILT_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.pantilt_relative = UVC_CT_PANTILT_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.roll_absolute = UVC_CT_ROLL_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.roll_relative = UVC_CT_ROLL_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.reserved1 = 0,//set to zero 
	.bmControls.b.reserved2 = 0,//set to zero 
	.bmControls.b.focus_auto = UVC_CT_FOCUS_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.privacy = UVC_CT_PRIVACY_CONTROL_SUPPORTED,	
	.bmControls.b.focus_simple = 0,
	.bmControls.b.window = 0,	
	.bmControls.b.region_of_interest = 0,		
	.bmControls.b.reserved3 = 0,//set to zero
#else
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	
	.bmControls.d24[0] = 0,
	.bmControls.d24[1] = 0,
	.bmControls.d24[2] = 0,
#endif
};

static const struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	.bUnitID		= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bSourceID		= UVC_ENTITY_ID_INPUT_TERMINAL,
	.wMaxMultiplier		= cpu_to_le16(0x4000),
	.bControlSize		= 2,
	
#ifdef UVC_PROCESS_CTRL_SUPPORT	
	.bmControls.b.brightness = UVC_PU_BRIGHTNESS_CONTROL_SUPPORTED,
	.bmControls.b.contrast = UVC_PU_CONTRAST_CONTROL_SUPPORTED,
	.bmControls.b.hue = UVC_PU_HUE_CONTROL_SUPPORTED,
	.bmControls.b.saturation = UVC_PU_SATURATION_CONTROL_SUPPORTED,
	.bmControls.b.sharpness = UVC_PU_SHARPNESS_CONTROL_SUPPORTED,
	.bmControls.b.gamma = UVC_PU_GAMMA_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_temperature = UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_component = UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL_SUPPORTED,
	.bmControls.b.backlight_compensation = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL_SUPPORTED,
	.bmControls.b.gain = UVC_PU_GAIN_CONTROL_SUPPORTED,
	.bmControls.b.power_line_frequency = UVC_PU_POWER_LINE_FREQUENCY_CONTROL_SUPPORTED,
	.bmControls.b.hue_auto = UVC_PU_HUE_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_temperature_auto = UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_component_auto = UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.digital_multiplier = 0,
	.bmControls.b.digital_multiplier_limit = 0,
#else
	.bmControls.d16 = 0,
#endif

	.iProcessing		= 0,
	.bmVideoStandards   = 0 //UVC 1.0 no support
};


static const struct uvc_extension_unit_h264_descriptor_t uvc_xu_h264 = {
	.bLength				= UVC_DT_EXTENSION_UNIT_SIZE(1,2),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VC_EXTENSION_UNIT,
	.bUnitID				= UVC_ENTITY_ID_XU_H264,
	.guidExtensionCode		= UVC_GUID_XUCONTROL_H264,
	.bNumControls			= 0x07,
	.bNrInPins				= 1,
	.baSourceID				= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bControlSize			= 2,

	.bmControls.b.uvcx_video_config_probe 		= 1,
	.bmControls.b.uvcx_video_config_commit 		= 1,
	.bmControls.b.uvcx_rate_control_mode 		= 1,
	.bmControls.b.uvcx_temporal_scale_mode 		= 0,
	.bmControls.b.uvcx_spatial_scale_mode 		= 0,
	.bmControls.b.uvcx_snr_scale_mode 			= 0,
	.bmControls.b.uvcx_ltr_buffer_size_control 	= 0,
	.bmControls.b.uvcx_ltr_picture_control 		= 0,
	.bmControls.b.uvcx_picture_type_control 	= 1,
	.bmControls.b.uvcx_version 					= 1,
	.bmControls.b.uvcx_encoder_reset 			= 0,
	.bmControls.b.uvcx_framerate_config 		= 1,
	.bmControls.b.uvcx_video_advance_config 	= 0,
	.bmControls.b.uvcx_bitrate_layers 			= 1,
	.bmControls.b.uvcx_qp_steps_layers 			= 0,
	.bmControls.b.uvcx_video_undefined 			= 0,

	.iExtension				= 0,
};


static const struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 3) uvc_xu_custom_commands = {
        .bLength                        = UVC_DT_EXTENSION_UNIT_SIZE(1,3),
        .bDescriptorType                = USB_DT_CS_INTERFACE,
        .bDescriptorSubType             = UVC_VC_EXTENSION_UNIT,
        .bUnitID                        = UVC_ENTITY_ID_XU_CUSTOM_COMMANDS,
        .guidExtensionCode              = UVC_GUID_XU_CUSTOM_COMMANDS,
        //.bNumControls                   = 8,
        .bNumControls                   = 1,
        .bNrInPins                      = 1, 
        .baSourceID[0]                  = UVC_ENTITY_ID_PROCESSING_UNIT,
        .bControlSize                   = 3, 
        //.bmControls[0]                  = 0x07, 
        .bmControls[0]                  = 0x7f, 
        //.bmControls[1]                  = 0x1f,
        .bmControls[1]                  = 0x00,
        .bmControls[2]                  = 0x00,
        .iExtension                     = 0, 
};


static const struct uvc_output_terminal_descriptor uvc_output_terminal = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID			= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iTerminal			= 0,
};
#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 4) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x04,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
	.bmaControls[2][0]	= 0,
	.bmaControls[3][0]	= 0,
};
#else
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 3) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x03,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
	.bmaControls[2][0]	= 0,
};
#endif
#else
#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 3) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x03,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
	.bmaControls[2][0]	= 0,
};
#else
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 2) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x02,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
};
#endif
#endif

//YUY2
static const struct uvc_format_uncompressed uvc_format_yuv_hs = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= UVC_FORMAT_INDEX_YUV2,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_640x480_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_640x480,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 480,
	.dwMinBitRate		= 640*480*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 640*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*480*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_160x120_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_160x120,
	.bmCapabilities		= 0,
	.wWidth				= 160,
	.wHeight			= 120,
	.dwMinBitRate		= 160*120*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 160*120*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 160*120*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_176x144_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_176x144,
	.bmCapabilities		= 0,
	.wWidth				= 176,
	.wHeight			= 144,
	.dwMinBitRate		= 176*144*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 176*144*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 176*144*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_320x180_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_320x180,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 180,
	.dwMinBitRate		= 320*180*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 320*180*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*180*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_320x240_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_320x240,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 240,
	.dwMinBitRate		= 320*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 320*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*240*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_352x288_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_352x288,
	.bmCapabilities		= 0,
	.wWidth				= 352,
	.wHeight			= 288,
	.dwMinBitRate		= 352*288*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 352*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 352*288*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_424x240_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_424x240,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 240,
	.dwMinBitRate		= 424*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*240*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_424x320_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_424x320,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 320,
	.dwMinBitRate		= 424*320*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*320*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*320*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_480x270_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_480x270,
	.bmCapabilities		= 0,
	.wWidth				= 480,
	.wHeight			= 272,
	.dwMinBitRate		= 480*272*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 480*272*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 480*272*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_512x288_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_512x288,
	.bmCapabilities		= 0,
	.wWidth				= 512,
	.wHeight			= 288,
	.dwMinBitRate		= 512*288*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 512*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 512*288*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_640x360_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_640x360,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 360,
	.dwMinBitRate		= 640*360*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 640*360*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*360*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_720x480_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_720x480,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 480,
	.dwMinBitRate		= 720*480*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 720*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*480*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(7) uvc_frame_yuv_720x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(7),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_720x576,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 576,
	.dwMinBitRate		= 720*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 720*576*2*25*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*576*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_25FPS),
	.bFrameIntervalType	= 7,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_800x448_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_800x448,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 448,
	.dwMinBitRate		= 800*448*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 800*448*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*448*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(6) uvc_frame_yuv_800x600_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(6),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_800x600,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 600,
	.dwMinBitRate		= 800*600*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 800*600*2*24*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*600*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_24FPS),
	.bFrameIntervalType	= 6,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_848x480_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_848x480,
	.bmCapabilities		= 0,
	.wWidth				= 848,
	.wHeight			= 480,
	.dwMinBitRate		= 848*480*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 848*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 848*480*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_960x540_hs = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static const struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_1024x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*576*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#if 1 
static const struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_1024x768_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*768*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuv_1280x720_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuv_1600x896_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1600x896,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (896),
	.dwMinBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*896*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*896*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#else
static const struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuv_1600x900_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1600x900,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_1920x1080_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

//MJPEG
static const struct uvc_format_mjpeg uvc_format_mjpg_hs = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= UVC_FORMAT_INDEX_MJPEG,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};	

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_640x480 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_640x480,
	.bmCapabilities		= 0,
	.wWidth				= (640),
	.wHeight			= (480),
	.dwMinBitRate		= (640*480*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (640*480*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (640*480*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_160x120 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_160x120,
	.bmCapabilities		= 0,
	.wWidth				= (160),
	.wHeight			= (120),
	.dwMinBitRate		= (160*120*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (160*120*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (160*120*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_176x144 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_176x144,
	.bmCapabilities		= 0,
	.wWidth				= (176),
	.wHeight			= (144),
	.dwMinBitRate		= (176*144*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (176*144*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (176*144*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_320x180 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_320x180,
	.bmCapabilities		= 0,
	.wWidth				= (320),
	.wHeight			= (180),
	.dwMinBitRate		= (320*180*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (320*180*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (320*180*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_320x240 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_320x240,
	.bmCapabilities		= 0,
	.wWidth				= (320),
	.wHeight			= (240),
	.dwMinBitRate		= (320*240*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (320*240*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (320*240*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_352x288 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_352x288,
	.bmCapabilities		= 0,
	.wWidth				= (352),
	.wHeight			= (288),
	.dwMinBitRate		= (352*288*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (352*288*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (352*288*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_424x240 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_424x240,
	.bmCapabilities		= 0,
	.wWidth				= (424),
	.wHeight			= (240),
	.dwMinBitRate		= (424*240*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (424*240*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (424*240*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_424x320 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_424x320,
	.bmCapabilities 	= 0,
	.wWidth 			= (424),
	.wHeight			= (320),
	.dwMinBitRate		= (424*320*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (424*320*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (424*320*2),//Width x Height x 2
	.dwDefaultFrameInterval = (UVC_FORMAT_30FPS),
	.bFrameIntervalType = 8,
	.dwFrameInterval[0] = (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7] = (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_480x270 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_480x270,
	.bmCapabilities		= 0,
	.wWidth				= (480),
	.wHeight			= (272),
	.dwMinBitRate		= (480*272*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (480*272*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (480*272*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_512x288 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_512x288,
	.bmCapabilities		= 0,
	.wWidth				= (512),
	.wHeight			= (288),
	.dwMinBitRate		= (512*288*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (512*288*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (512*288*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_640x360 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_640x360,
	.bmCapabilities		= 0,
	.wWidth				= (640),
	.wHeight			= (360),
	.dwMinBitRate		= (640*360*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (640*360*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (640*360*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_720x480 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_720x480,
	.bmCapabilities 	= 0,
	.wWidth 			= (720),
	.wHeight			= (480),
	.dwMinBitRate		= (720*480*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (720*480*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (720*480*2),//Width x Height x 2
	.dwDefaultFrameInterval = (UVC_FORMAT_30FPS),
	.bFrameIntervalType = 8,
	.dwFrameInterval[0] = (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7] = (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_720x576 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_720x576,
	.bmCapabilities		= 0,
	.wWidth				= (720),
	.wHeight			= (576),
	.dwMinBitRate		= (720*576*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (720*576*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (720*576*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_800x448 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_800x448,
	.bmCapabilities		= 0,
	.wWidth				= (800),
	.wHeight			= (448),
	.dwMinBitRate		= (800*448*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (800*448*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (800*448*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_800x600 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_800x600,
	.bmCapabilities		= 0,
	.wWidth				= (800),
	.wHeight			= (600),
	.dwMinBitRate		= (800*600*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (800*600*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (800*600*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_848x480 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_848x480,
	.bmCapabilities		= 0,
	.wWidth				= (848),
	.wHeight			= (480),
	.dwMinBitRate		= (848*480*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (848*480*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (848*480*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_960x540 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_1024x576 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= (1024),
	.wHeight			= (576),
	.dwMinBitRate		= (1024*576*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1024*576*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1024*576*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_1024x768 = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= (1024),
	.wHeight			= (768),
	.dwMinBitRate		= (1024*768*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1024*768*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1024*768*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_1280x720 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_1600x896 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_1600x896,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (896),
	.dwMinBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*896*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*896*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#else
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_1600x900 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_1600x900,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_1920x1080 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_2560x1440 = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_2560x1440,
	.bmCapabilities 	= 0,
	.wWidth 			= (2560),
	.wHeight			= (1440),
	.dwMinBitRate		= (2560*1440*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*2),//Width x Height x 2
	.dwDefaultFrameInterval = (UVC_FORMAT_30FPS),
	.bFrameIntervalType = 8,
	.dwFrameInterval[0] = (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7] = (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_MJPEG(4) uvc_frame_mjpg_3840x2160_hs = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_3840x2160,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
static const struct uvc_format_uncompressed uvc_format_nv12_hs = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= UVC_FORMAT_INDEX_NV12,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1280x720,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= UVC_NV12_FORMAT_INDEX_640x480,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_640x480_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_640x480,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 480,
	.dwMinBitRate		= 640*480*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 640*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*480*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_160x120_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_160x120,
	.bmCapabilities		= 0,
	.wWidth				= 160,
	.wHeight			= 120,
	.dwMinBitRate		= 160*120*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 160*120*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 160*120*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_176x144_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_176x144,
	.bmCapabilities		= 0,
	.wWidth				= 176,
	.wHeight			= 144,
	.dwMinBitRate		= 176*144*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 176*144*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 176*144*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_320x180_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_320x180,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 180,
	.dwMinBitRate		= 320*180*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 320*180*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*180*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_320x240_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_320x240,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 240,
	.dwMinBitRate		= 320*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 320*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*240*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_352x288_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_352x288,
	.bmCapabilities		= 0,
	.wWidth				= 352,
	.wHeight			= 288,
	.dwMinBitRate		= 352*288*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 352*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 352*288*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_424x240_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_424x240,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 240,
	.dwMinBitRate		= 424*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*240*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_424x320_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_424x320,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 320,
	.dwMinBitRate		= 424*320*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*320*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*320*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_480x270_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_480x270,
	.bmCapabilities		= 0,
	.wWidth				= 480,
	.wHeight			= 270,
	.dwMinBitRate		= 480*270*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 480*270*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 480*270*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_512x288_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_512x288,
	.bmCapabilities		= 0,
	.wWidth				= 512,
	.wHeight			= 288,
	.dwMinBitRate		= 512*288*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 512*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 512*288*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_640x360_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_640x360,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 360,
	.dwMinBitRate		= 640*360*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 640*360*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*360*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_720x480_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_720x480,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 480,
	.dwMinBitRate		= 720*480*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 720*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*480*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_720x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_720x576,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 576,
	.dwMinBitRate		= 720*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 720*576*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*576*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(7) uvc_frame_nv12_800x448_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(7),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_800x448,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 448,
	.dwMinBitRate		= 800*448*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 800*448*2*25*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*448*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_20FPS),
	.bFrameIntervalType	= 7,
	.dwFrameInterval[0]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_800x600_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_800x600,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 600,
	.dwMinBitRate		= 800*600*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 800*600*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*600*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0] = (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0 
static const struct UVC_FRAME_UNCOMPRESSED(7) uvc_frame_nv12_848x480_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(7),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_848x480,
	.bmCapabilities		= 0,
	.wWidth				= 848,
	.wHeight			= 480,
	.dwMinBitRate		= 848*480*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 848*480*2*25*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 848*480*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_20FPS),
	.bFrameIntervalType	= 7,
	.dwFrameInterval[0]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_960x540_hs = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static const struct UVC_FRAME_UNCOMPRESSED(6) uvc_frame_nv12_1024x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(6),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*576*2*24*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_24FPS),
	.bFrameIntervalType	= 6,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_20FPS),//(1/20)x10^7	
	.dwFrameInterval[2] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[4] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#if 0 
static const struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_1024x768_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*768*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_1280x720_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0 
static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1600x900_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1600x900,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1920x1080_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

#endif
/************************ss*********************************************/

static const struct uvc_format_uncompressed uvc_format_yuv_ss = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= UVC_FORMAT_INDEX_YUV2,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_640x480_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_640x480,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 480,
	.dwMinBitRate		= 640*480*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 640*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*480*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_160x120_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_160x120,
	.bmCapabilities		= 0,
	.wWidth				= 160,
	.wHeight			= 120,
	.dwMinBitRate		= 160*120*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 160*120*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 160*120*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_176x144_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_176x144,
	.bmCapabilities		= 0,
	.wWidth				= 176,
	.wHeight			= 144,
	.dwMinBitRate		= 176*144*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 176*144*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 176*144*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_320x180_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_320x180,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 180,
	.dwMinBitRate		= 320*180*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 320*180*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*180*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_320x240_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_320x240,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 240,
	.dwMinBitRate		= 320*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 320*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*240*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_352x288_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_352x288,
	.bmCapabilities		= 0,
	.wWidth				= 352,
	.wHeight			= 288,
	.dwMinBitRate		= 352*288*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 352*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 352*288*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_424x240_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_424x240,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 240,
	.dwMinBitRate		= 424*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*240*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_424x320_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_424x320,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 320,
	.dwMinBitRate		= 424*320*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*320*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*320*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_480x270_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_480x270,
	.bmCapabilities		= 0,
	.wWidth				= 480,
	.wHeight			= 272,
	.dwMinBitRate		= 480*272*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 480*272*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 480*272*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_512x288_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_512x288,
	.bmCapabilities		= 0,
	.wWidth				= 512,
	.wHeight			= 288,
	.dwMinBitRate		= 512*288*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 512*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 512*288*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_640x360_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_640x360,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 360,
	.dwMinBitRate		= 640*360*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 640*360*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*360*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_720x480_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_720x480,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 480,
	.dwMinBitRate		= 720*480*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 720*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*480*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_720x576_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_720x576,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 576,
	.dwMinBitRate		= 720*576*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 720*576*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*576*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_800x448_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_800x448,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 448,
	.dwMinBitRate		= 800*448*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 800*448*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*448*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_800x600_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_800x600,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 600,
	.dwMinBitRate		= 800*600*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 800*600*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*600*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_848x480_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_848x480,
	.bmCapabilities		= 0,
	.wWidth				= 848,
	.wHeight			= 480,
	.dwMinBitRate		= 848*480*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 848*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 848*480*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_960x540_ss = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_1024x576_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 1024*576*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_1024x768_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 1024*768*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_1280x720_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_1600x896_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1600x896,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (896),
	.dwMinBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*896*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*896*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#else

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_1600x900_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1600x900,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_1920x1080_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_2560x1440_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_2560x1440,
	.bmCapabilities		= 0,
	.wWidth				= (2560),
	.wHeight			= (1440),
	.dwMinBitRate		= (2560*1440*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
static const struct uvc_format_uncompressed uvc_format_nv12_ss = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= UVC_FORMAT_INDEX_NV12,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_640x480_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_640x480,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 480,
	.dwMinBitRate		= 640*480*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 640*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*480*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_160x120_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_160x120,
	.bmCapabilities		= 0,
	.wWidth				= 160,
	.wHeight			= 120,
	.dwMinBitRate		= 160*120*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 160*120*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 160*120*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_176x144_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_176x144,
	.bmCapabilities		= 0,
	.wWidth				= 176,
	.wHeight			= 144,
	.dwMinBitRate		= 176*144*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 176*144*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 176*144*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_320x180_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_320x180,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 180,
	.dwMinBitRate		= 320*180*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 320*180*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*180*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_320x240_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_320x240,
	.bmCapabilities		= 0,
	.wWidth				= 320,
	.wHeight			= 240,
	.dwMinBitRate		= 320*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 320*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 320*240*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_352x288_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_352x288,
	.bmCapabilities		= 0,
	.wWidth				= 352,
	.wHeight			= 288,
	.dwMinBitRate		= 352*288*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 352*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 352*288*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_424x240_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_424x240,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 240,
	.dwMinBitRate		= 424*240*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*240*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*240*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_424x320_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_424x320,
	.bmCapabilities		= 0,
	.wWidth				= 424,
	.wHeight			= 320,
	.dwMinBitRate		= 424*320*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 424*320*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 424*320*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_480x270_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_480x270,
	.bmCapabilities		= 0,
	.wWidth				= 480,
	.wHeight			= 270,
	.dwMinBitRate		= 480*270*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 480*270*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 480*270*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_512x288_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_512x288,
	.bmCapabilities		= 0,
	.wWidth				= 512,
	.wHeight			= 288,
	.dwMinBitRate		= 512*288*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 512*288*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 512*288*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_640x360_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_640x360,
	.bmCapabilities		= 0,
	.wWidth				= 640,
	.wHeight			= 360,
	.dwMinBitRate		= 640*360*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 640*360*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 640*360*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_720x480_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_720x480,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 480,
	.dwMinBitRate		= 720*480*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 720*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*480*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_720x576_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_720x576,
	.bmCapabilities		= 0,
	.wWidth				= 720,
	.wHeight			= 576,
	.dwMinBitRate		= 720*576*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 720*576*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 720*576*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0 
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_800x448_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_800x448,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 448,
	.dwMinBitRate		= 800*448*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 800*448*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*448*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_800x600_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_800x600,
	.bmCapabilities		= 0,
	.wWidth				= 800,
	.wHeight			= 600,
	.dwMinBitRate		= 800*600*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 800*600*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 800*600*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0 
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_848x480_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_848x480,
	.bmCapabilities		= 0,
	.wWidth				= 848,
	.wHeight			= 480,
	.dwMinBitRate		= 848*480*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 848*480*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 848*480*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_960x540_ss = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_1024x576_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 1024*576*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0 
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_1024x768_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= 1024*768*2*30*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_1280x720_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_1600x900_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1600x900,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_1920x1080_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_2560x1440_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_2560x1440,
	.bmCapabilities		= 0,
	.wWidth				= (2560),
	.wHeight			= (1440),
	.dwMinBitRate		= (2560*1440*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
#endif

//MJPEG
static const struct uvc_format_mjpeg uvc_format_mjpg_ss = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= UVC_FORMAT_INDEX_MJPEG,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	= UVC_MJPEG_FORMAT_INDEX_1920x1080,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};	

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static const struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_3840x2160_ss = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_3840x2160,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*30*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType	= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT
//H264 UVC1.1
static const struct uvc_format_h264_base uvc110_format_h264 = {
	.bLength				= sizeof(uvc110_format_h264),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= UVC_FORMAT_INDEX_H264,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
#endif

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_640x480 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_640x480,
	.bmCapabilities			= 0,
	.wWidth					= (640),
	.wHeight				= (480),
	.dwMinBitRate			= (640*480*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (640*480*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 0
static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_320x180 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_320x180,
	.bmCapabilities			= 0,
	.wWidth					= (320),
	.wHeight				= (180),
	.dwMinBitRate			= (320*180*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (320*180*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_320x240 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_320x240,
	.bmCapabilities			= 0,
	.wWidth					= (320),
	.wHeight				= (240),
	.dwMinBitRate			= (320*240*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (320*240*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_352x288 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_352x288,
	.bmCapabilities			= 0,
	.wWidth					= (352),
	.wHeight				= (288),
	.dwMinBitRate			= (352*288*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (352*288*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_424x240 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_424x240,
	.bmCapabilities			= 0,
	.wWidth					= (424),
	.wHeight				= (240),
	.dwMinBitRate			= (424*240*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (424*240*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_424x320 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_424x320,
	.bmCapabilities			= 0,
	.wWidth					= (424),
	.wHeight				= (320),
	.dwMinBitRate			= (424*320*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (424*320*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_480x270 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_480x270,
	.bmCapabilities			= 0,
	.wWidth					= (480),
	.wHeight				= (270),
	.dwMinBitRate			= (480*270*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (480*270*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_512x288 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_512x288,
	.bmCapabilities			= 0,
	.wWidth					= (512),
	.wHeight				= (288),
	.dwMinBitRate			= (512*288*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (512*288*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_640x360 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_640x360,
	.bmCapabilities			= 0,
	.wWidth					= (640),
	.wHeight				= (360),
	.dwMinBitRate			= (640*360*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (640*360*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_720x480 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_720x480,
	.bmCapabilities			= 0,
	.wWidth					= (720),
	.wHeight				= (480),
	.dwMinBitRate			= (720*480*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (720*480*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_720x576 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_720x576,
	.bmCapabilities			= 0,
	.wWidth					= (720),
	.wHeight				= (576),
	.dwMinBitRate			= (720*576*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (720*576*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_800x448 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_800x448,
	.bmCapabilities			= 0,
	.wWidth					= (800),
	.wHeight				= (448),
	.dwMinBitRate			= (800*448*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (800*448*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_800x600 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_800x600,
	.bmCapabilities			= 0,
	.wWidth					= (800),
	.wHeight				= (600),
	.dwMinBitRate			= (800*600*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (800*600*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_848x480 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_848x480,
	.bmCapabilities			= 0,
	.wWidth					= (848),
	.wHeight				= (480),
	.dwMinBitRate			= (848*480*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (848*480*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_960x540 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_960x540,
	.bmCapabilities			= 0,
	.wWidth					= (960),
	.wHeight				= (540),
	.dwMinBitRate			= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (960*540*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};



static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_1024x576 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_1024x576,
	.bmCapabilities			= 0,
	.wWidth					= (1024),
	.wHeight				= (576),
	.dwMinBitRate			= (1024*576*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (1024*576*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_1024x768 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_1024x768,
	.bmCapabilities			= 0,
	.wWidth					= (1024),
	.wHeight				= (768),
	.dwMinBitRate			= (1024*768*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (1024*768*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_1280x720 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_1280x720,
	.bmCapabilities			= 0,
	.wWidth					= (1280),
	.wHeight				= (720),
	.dwMinBitRate			= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (1280*720*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_1600x896 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_1600x896,
	.bmCapabilities			= 0,
	.wWidth					= (1600),
	.wHeight				= (896),
	.dwMinBitRate			= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (1600*896*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#else
static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_1600x900 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_1600x900,
	.bmCapabilities			= 0,
	.wWidth					= (1600),
	.wHeight				= (900),
	.dwMinBitRate			= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (1600*900*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_1920x1080 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC110_H264_FORMAT_INDEX_1920x1080,
	.bmCapabilities			= 0,
	.wWidth					= (1920),
	.wHeight				= (1080),
	.dwMinBitRate			= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (1920*1080*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};



#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_2560x1440 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC_MJPEG_FORMAT_INDEX_2560x1440,
	.bmCapabilities			= 0,
	.wWidth					= (2560),
	.wHeight				= (1440),
	.dwMinBitRate			= (2560*1440*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (2560*1440*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_3840x2160 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= UVC_MJPEG_FORMAT_INDEX_3840x2160,
	.bmCapabilities			= 0,
	.wWidth					= (3840),
	.wHeight				= (2160),
	.dwMinBitRate			= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (3840*2160*2*30*8),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= 8,
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct uvc_descriptor_header * const uvc_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

/***************************************************************************/
/**********************************fs***************************************/
/***************************************************************************/
static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {

	(const struct uvc_descriptor_header *) &uvc_input_header,
	
	(const struct uvc_descriptor_header *) &uvc_format_yuv_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_640x480_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_320x180_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_320x240_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_352x288_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_424x240_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_424x320_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_480x270_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_512x288_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_640x360_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720x576_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_800x448_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_800x600_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_848x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_960x540_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1024x576_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1024x768_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1280x720_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1600x896_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_1600x900_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1920x1080_hs,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
	(const struct uvc_descriptor_header *) &uvc_format_mjpg_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_640x480,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_320x180,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_320x240,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_352x288,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_424x240,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_424x320,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_480x270,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_512x288,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_640x360,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720x480,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720x576,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_800x448,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_800x600,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_848x480,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_960x540,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x576,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x768,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1280x720,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x896,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x900,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_2560x1440,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_3840x2160_hs,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_format_nv12_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_640x480_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_320x180_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_320x240_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_352x288_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_424x240_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_424x320_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_480x270_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_512x288_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_640x360_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_720x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_720x576_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_800x448_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_800x600_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_848x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_960x540_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1024x576_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1024x768_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1280x720_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1600x900_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1920x1080_hs,
	(const struct uvc_descriptor_header *) &uvc_color_matching, 
#endif

#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_format_h264,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x480,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x180,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_352x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x320,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_480x270,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_512x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x360,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x448,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x600,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_848x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_960x540,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160,
#endif

	(const struct uvc_descriptor_header *) &uvc_color_matching,
#endif /*#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT*/
	
	NULL,
};


/***************************************************************************/
/**********************************hs***************************************/
/***************************************************************************/
static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {

	(const struct uvc_descriptor_header *) &uvc_input_header,
	
	(const struct uvc_descriptor_header *) &uvc_format_yuv_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_640x480_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_320x180_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_320x240_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_352x288_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_424x240_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_424x320_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_480x270_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_512x288_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_640x360_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720x576_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_800x448_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_800x600_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_848x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_960x540_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1024x576_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1024x768_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1280x720_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_1600x900_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1600x896_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1920x1080_hs,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
	(const struct uvc_descriptor_header *) &uvc_format_mjpg_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_640x480,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_320x180,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_320x240,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_352x288,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_424x240,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_424x320,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_480x270,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_512x288,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_640x360,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720x480,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720x576,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_800x448,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_800x600,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_848x480,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_960x540,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x576,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x768,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1280x720,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x896,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x900,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_2560x1440,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_3840x2160_hs,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_format_nv12_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_640x480_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_320x180_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_320x240_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_352x288_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_424x240_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_424x320_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_480x270_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_512x288_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_640x360_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_720x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_720x576_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_800x448_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_800x600_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_848x480_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_960x540_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1024x576_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1024x768_hs,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1280x720_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1600x900_hs,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1920x1080_hs,
	(const struct uvc_descriptor_header *) &uvc_color_matching, 
#endif

#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_format_h264,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x480,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x180,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_352x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x320,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_480x270,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_512x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x360,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x448,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x600,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_848x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_960x540,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160,
#endif

	(const struct uvc_descriptor_header *) &uvc_color_matching,
#endif /*#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT*/

	NULL,
};


/***************************************************************************/
/**********************************ss***************************************/
/***************************************************************************/
static const struct uvc_descriptor_header * const uvc_ss_streaming_cls[] = {

	(const struct uvc_descriptor_header *) &uvc_input_header,
	
	(const struct uvc_descriptor_header *) &uvc_format_yuv_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_640x480_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_320x180_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_320x240_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_352x288_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_424x240_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_424x320_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_480x270_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_512x288_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_640x360_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720x480_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_720x576_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_800x448_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_800x600_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_848x480_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_960x540_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1024x576_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1024x768_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1280x720_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1600x896_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_1600x900_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_1920x1080_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_yuv_2560x1440_ss,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
	(const struct uvc_descriptor_header *) &uvc_format_mjpg_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_640x480,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_320x180,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_320x240,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_352x288,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_424x240,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_424x320,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_480x270,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_512x288,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_640x360,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720x480,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720x576,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_800x448,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_800x600,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_848x480,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_960x540,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x576,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x768,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1280x720,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x896,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x900,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_2560x1440,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_3840x2160_ss,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_format_nv12_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_640x480_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_320x180_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_320x240_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_352x288_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_424x240_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_424x320_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_480x270_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_512x288_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_640x360_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_720x480_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_720x576_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_800x448_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_800x600_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_848x480_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_960x540_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1024x576_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1024x768_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1280x720_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1600x900_ss,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_1920x1080_ss,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_2560x1440_ss,
	(const struct uvc_descriptor_header *) &uvc_color_matching, 
#endif

#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_format_h264,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x480,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x180,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_352x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x320,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_480x270,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_512x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x360,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x448,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x600,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_848x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_960x540,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160,
#endif

	(const struct uvc_descriptor_header *) &uvc_color_matching,
#endif /*#ifdef UVC_VIDEO_FORMAT_H264_SUPPORT*/
	
	NULL,
};


/* --------------------------------------------------------------------------
 * UVC1.1 S2 configuration
 */
 
//uvc_control

static struct UVC_HEADER_DESCRIPTOR(2) uvc_control_header_s2 = {
	.bLength			= UVC_DT_HEADER_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 1, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
	.baInterfaceNr[1]	= 0, /* dynamic */
};

static const struct uvc_output_terminal_descriptor uvc_output_terminal_s2 = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID			= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iTerminal			= 0,
};

static const struct uvc_descriptor_header * const uvc_fs_control_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header_s2,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
#ifdef	UVC_XU_CTRL_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
#endif
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	(const struct uvc_descriptor_header *) &uvc_output_terminal_s2,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header_s2,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
#ifdef	UVC_XU_CTRL_SUPPORT
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
#endif
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	(const struct uvc_descriptor_header *) &uvc_output_terminal_s2,
	NULL,
};

//UVC_STREAM

static struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc110_input_header_s2 = {
	.bLength			= UVC_DT_INPUT_HEADER_SIZE(1, 1),/* dynamic */
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x01,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.bStillCaptureMethod= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
};

//H264 UVC1.1
static const struct uvc_format_h264_base uvc110_format_h264_s2 = {
	.bLength				= sizeof(uvc110_format_h264_s2),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= UVC_FORMAT_INDEX_H264_S2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};


static const struct uvc_descriptor_header * const uvc_fs_streaming_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc110_input_header_s2,
	
	(const struct uvc_descriptor_header *) &uvc110_format_h264_s2,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x480,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x180,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_352x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x320,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_480x270,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_512x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x360,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x448,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x600,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_848x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_960x540,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160,
#endif

	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
	NULL,
};

static const struct uvc_descriptor_header * const uvc_hs_streaming_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc110_input_header_s2,
	
	(const struct uvc_descriptor_header *) &uvc110_format_h264_s2,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x480,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x180,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_352x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x320,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_480x270,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_512x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x360,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x448,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x600,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_848x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_960x540,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160,
#endif

	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_streaming_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc110_input_header_s2,
	
	(const struct uvc_descriptor_header *) &uvc110_format_h264_s2,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x480,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x180,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_320x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_352x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x240,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_424x320,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_480x270,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_512x288,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_640x360,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_720x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x448,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_800x600,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_848x480,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_960x540,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896,
	//(const struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440,
	(const struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160,
#endif

	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
	NULL,
};



/* --------------------------------------------------------------------------
 * UVC1.5 configuration
 */
 
static struct UVC_HEADER_DESCRIPTOR(2) uvc150_control_header = {
	.bLength			= UVC_DT_HEADER_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0150),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 1, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
	.baInterfaceNr[1]	= 0, /* dynamic */
};

//uvc1.5 for h264
static const struct uvc_encoding_unit_descriptor uvc150_encoding= {
	.bLength				= UVC_DT_ENCODING_UNIT_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VC_ENCODING_UNIT,
	.bUnitID				= UVC_ENTITY_ID_ENCODING_UNIT,
	.bSourceID				= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iEncoding				= 0,
	.bControlSize			= 0x03,
#if 1
	.bmControls[0]			= 0xFF,
	.bmControls[1]			= 0xC6,
	.bmControls[2]			= 0x07,
	.bmControlsRuntime[0]	= 0xFD,
	.bmControlsRuntime[1]	= 0x06,
	.bmControlsRuntime[2]	= 0x07,
#else
	.bmControls[0]			= 0,//0xFF,
	.bmControls[1]			= 0,//0xC6,
	.bmControls[2]			= 0,//0x07,
	.bmControlsRuntime[0]	= 0,//0xFD,
	.bmControlsRuntime[1]	= 0,//0x06,
	.bmControlsRuntime[2]	= 0,//0x07,
#endif
};



static const struct uvc_output_terminal_descriptor uvc150_output_terminal_s2 = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID			= UVC_ENTITY_ID_ENCODING_UNIT,
	.iTerminal			= 0,
};

#ifdef SIMULCAST_SUPPORT
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 2) uvc150_input_header_s2 = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x02,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0x04,
};
#else
static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc150_input_header_s2 = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x01,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
};
#endif


//H264 V1.5
static const struct uvc_format_h264_v150 uvc150_format_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_V150,
	.bFormatIndex			= UVC_FORMAT_INDEX_H264_S2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 0x00F5,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 0x00F5,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};

//DECLARE_UVC_FRAME_H264_V150(2);

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_640x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_640x480,
	.wWidth					= (640),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,

	.bmCapabilities			= 0x0021,
	/*
     D00 = 1  yes -  CAVLC only
     D01 = 0   no -  CABAC only
     D02 = 0   no -  Constant frame rate
     D03 = 1  yes -  Separate QP for luma/chroma
     D04 = 0   no -  Separate QP for Cb/Cr
     D05 = 1  yes -  No picture reordering
     D06 = 0   no -  Long-term reference frame
     D07 = 0   no -  Reserved
     D08 = 0   no -  Reserved
     D09 = 0   no -  Reserved
     D10 = 0   no -  Reserved
     D11 = 0   no -  Reserved
     D12 = 0   no -  Reserved
     D13 = 0   no -  Reserved
     D14 = 0   no -  Reserved
     D15 = 0   no -  Reserved
	 */
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	/*
	 D2..D0   = 1  Maximum number of temporal layers = 3
     D3       = 0  no - Rewrite Support
     D6..D4   = 0  Maximum number of CGS layers = 1
     D9..D7   = 0  Number of MGS sublayers
     D10      = 0  no - Additional SNR scalability support in spatial enhancement layers
     D13..D11 = 0  Maximum number of spatial layers = 1
     D14      = 0  no - Reserved
     D15      = 0  no - Reserved
     D16      = 0  no - Reserved
     D17      = 0  no - Reserved
     D18      = 0  no - Reserved
     D19      = 0  no - Reserved
     D20      = 0  no - Reserved
     D21      = 0  no - Reserved
     D22      = 0  no - Reserved
     D23      = 0  no - Reserved
     D24      = 0  no - Reserved
     D25      = 0  no - Reserved
     D26      = 0  no - Reserved
     D27      = 0  no - Reserved
     D28      = 0  no - Reserved
     D29      = 0  no - Reserved
     D30      = 0  no - Reserved
     D31      = 0  no - Reserved
	 */
	.bmMVCCapabilities		= 0x00000000,
	/*
	 D2..D0   = 0  Maximum number of temporal layers = 1
     D10..D3  = 0  Maximum number of view components = 1
     D11      = 0  no - Reserved
     D12      = 0  no - Reserved
     D13      = 0  no - Reserved
     D14      = 0  no - Reserved
     D15      = 0  no - Reserved
     D16      = 0  no - Reserved
     D17      = 0  no - Reserved
     D18      = 0  no - Reserved
     D19      = 0  no - Reserved
     D20      = 0  no - Reserved
     D21      = 0  no - Reserved
     D22      = 0  no - Reserved
     D23      = 0  no - Reserved
     D24      = 0  no - Reserved
     D25      = 0  no - Reserved
     D26      = 0  no - Reserved
     D27      = 0  no - Reserved
     D28      = 0  no - Reserved
     D29      = 0  no - Reserved
     D30      = 0  no - Reserved
     D31      = 0  no - Reserved
	 */
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 0
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_160x120_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_160x120,
	.wWidth					= (160),
	.wHeight				= (120),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_176x144_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_176x144,
	.wWidth					= (176),
	.wHeight				= (144),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_320x180_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_320x180,
	.wWidth					= (320),
	.wHeight				= (180),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_320x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_320x240,
	.wWidth					= (320),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_352x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_352x288,
	.wWidth					= (352),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_424x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_424x240,
	.wWidth					= (424),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_424x320_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_424x320,
	.wWidth					= (424),
	.wHeight				= (320),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_480x270_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_480x270,
	.wWidth					= (480),
	.wHeight				= (270),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_512x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_512x288,
	.wWidth					= (512),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_640x360_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_640x360,
	.wWidth					= (640),
	.wHeight				= (360),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_720x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_720x480,
	.wWidth					= (720),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_720x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_720x576,
	.wWidth					= (720),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_800x448_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_800x448,
	.wWidth					= (800),
	.wHeight				= (448),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_800x600_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_800x600,
	.wWidth					= (800),
	.wHeight				= (600),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_848x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_848x480,
	.wWidth					= (848),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_960x540_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_960x540,
	.wWidth					= (960),
	.wHeight				= (540),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_1024x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1024x576,
	.wWidth					= (1024),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_1024x768_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1024x768,
	.wWidth					= (1024),
	.wHeight				= (768),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_1280x720_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1280x720,
	.wWidth					= (1280),
	.wHeight				= (720),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_1600x896_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1600x896,
	.wWidth					= (1600),
	.wHeight				= (896),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#else
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_1600x900_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1600x900,
	.wWidth					= (1600),
	.wHeight				= (900),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_1920x1080_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1920x1080,
	.wWidth					= (1920),
	.wHeight				= (1080),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_2560x1440_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_2560x1440,
	.wWidth					= (2560),
	.wHeight				= (1440),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*2),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*2),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};



static const struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_3840x2160_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_3840x2160,
	.wWidth					= (3840),
	.wHeight				= (2160),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*4),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*4),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

//H264

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_640x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_640x480,
	.wWidth					= (640),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 0
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_160x120_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_160x120,
	.wWidth					= (160),
	.wHeight				= (120),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_176x144_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_176x144,
	.wWidth					= (176),
	.wHeight				= (144),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_320x180_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_320x180,
	.wWidth					= (320),
	.wHeight				= (180),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_320x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_320x240,
	.wWidth					= (320),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_352x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_352x288,
	.wWidth					= (352),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_424x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_424x240,
	.wWidth					= (424),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_424x320_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_424x320,
	.wWidth					= (424),
	.wHeight				= (320),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_480x270_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_480x270,
	.wWidth					= (480),
	.wHeight				= (270),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_512x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_512x288,
	.wWidth					= (512),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_640x360_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_640x360,
	.wWidth					= (640),
	.wHeight				= (360),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_720x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_720x480,
	.wWidth					= (720),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_720x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_720x576,
	.wWidth					= (720),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_800x448_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_800x448,
	.wWidth					= (800),
	.wHeight				= (448),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_800x600_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_800x600,
	.wWidth					= (800),
	.wHeight				= (600),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_848x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_848x480,
	.wWidth					= (848),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_960x540_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_960x540,
	.wWidth					= (960),
	.wHeight				= (540),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_1024x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1024x576,
	.wWidth					= (1024),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_1024x768_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1024x768,
	.wWidth					= (1024),
	.wHeight				= (768),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_1280x720_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1280x720,
	.wWidth					= (1280),
	.wHeight				= (720),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_1600x896_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1600x896,
	.wWidth					= (1600),
	.wHeight				= (896),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 0
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_1600x900_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1600x900,
	.wWidth					= (1600),
	.wHeight				= (900),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_1920x1080_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1920x1080,
	.wWidth					= (1920),
	.wHeight				= (1080),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_2560x1440_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_2560x1440,
	.wWidth					= (2560),
	.wHeight				= (1440),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*2),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*2),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static const struct UVC_FRAME_H264_V150(8) uvc150_frame2_h264_3840x2160_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_3840x2160,
	.wWidth					= (3840),
	.wHeight				= (2160),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*4),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*4),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

#ifdef SIMULCAST_SUPPORT

static const struct uvc_format_h264_v150 uvc150_format_h264_simulcast_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_SIMULCAST,
	.bFormatIndex			= UVC_FORMAT_INDEX_H264_SIMULCAST_S2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 0x006C,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 0x00F5,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x006C,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 0x00F5,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};
#endif

 
static const struct uvc_descriptor_header * const uvc150_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc150_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
#ifdef	UVC_XU_CTRL_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_encoding,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
#endif
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	(const struct uvc_descriptor_header *) &uvc150_output_terminal_s2,
	NULL,
};

static const struct uvc_descriptor_header * const uvc150_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc150_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
#ifdef	UVC_XU_CTRL_SUPPORT	
	(const struct uvc_descriptor_header *) &uvc150_encoding,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
#endif
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	(const struct uvc_descriptor_header *) &uvc150_output_terminal_s2,
	NULL,
};


//the second stream

static const struct uvc_descriptor_header * const uvc150_fs_streaming_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc150_input_header_s2,
	
	(const struct uvc_descriptor_header *) &uvc150_format_h264_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2,

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2,
#endif

	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1920x1080_s2,

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_3840x2160_s2,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
#ifdef SIMULCAST_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_format_h264_simulcast_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
#endif
	NULL,
};

static const struct uvc_descriptor_header * const uvc150_hs_streaming_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc150_input_header_s2,
	
	(const struct uvc_descriptor_header *) &uvc150_format_h264_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT	
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2,
#endif

	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT	
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_3840x2160_s2,
#endif	
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
#ifdef SIMULCAST_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_format_h264_simulcast_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
#endif
	
	NULL,
};

static const struct uvc_descriptor_header * const uvc150_ss_streaming_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc150_input_header_s2,
	
	(const struct uvc_descriptor_header *) &uvc150_format_h264_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2,
#endif

	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame2_h264_3840x2160_s2,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	
#ifdef SIMULCAST_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_format_h264_simulcast_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_480x270_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2,
	//(const struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2,
	(const struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2,
#endif
	(const struct uvc_descriptor_header *) &uvc_color_matching,
#endif
	
	NULL,
};




/* --------------------------------------------------------------------------
 * USB configuration
 */

static int __init
webcam_config_bind(struct usb_configuration *c)
{
	int status = 0;

#ifdef USB_MULTI_SUPPORT
	//需要放在第一，否则Windows驱动加载失败
	if(g_rndis_enable && !(g_rndis_enable & 0x02))
	{
		status = multi_do_config(c);
		if(status != 0)
		{
			printk("multi_do_config error\n");
		}
	}
#endif

#ifdef UAC_AUDIO_SUPPORT	
	//Ron_Lee 20180124
	/* FIXME: add audio device*/
	if(g_audio_enable == 2)
	{
	    audio_do_config(c);
	}
	//
#endif

#ifdef DFU_SUPPORT
	if(g_dfu_enable && g_dfu_last == 0)
		status = dfug_bind_config(c, g_dfu_enable == 2 ? 1:0, 0);
#endif

#ifdef USB_UVC_SUPPORT
	if(g_uvc_enable)
	{
		printk("fi_uvc:%p\n", fi_uvc);
		f_uvc = usb_get_function(fi_uvc);
		if (IS_ERR_OR_NULL(f_uvc))
			return PTR_ERR(f_uvc);

		status = usb_add_function(c, f_uvc);
		if (status < 0)
			usb_put_function(f_uvc);

		
		if(c->bConfigurationValue == 2 && g_uvc_v150_enable)//uvc1.5
		{
			if (!IS_ERR_OR_NULL(fi_uvc_s2))
			{
				printk("fi_uvc_s2:%p\n", fi_uvc_s2);
				f_uvc_s2 = usb_get_function(fi_uvc_s2);
				if (IS_ERR_OR_NULL(f_uvc_s2))
					return PTR_ERR(f_uvc_s2);

				status = usb_add_function(c, f_uvc_s2);
				if (status < 0)
					usb_put_function(f_uvc_s2);		
			}	
		}
		else
		{
#ifdef UVC_S2_SUPPORT
			if (!IS_ERR_OR_NULL(fi_uvc_s2) && g_uvc_s2_enable)
			{
				printk("fi_uvc_s2:%p\n", fi_uvc_s2);
				f_uvc_s2 = usb_get_function(fi_uvc_s2);
				if (IS_ERR_OR_NULL(f_uvc_s2))
					return PTR_ERR(f_uvc_s2);

				status = usb_add_function(c, f_uvc_s2);
				if (status < 0)
					usb_put_function(f_uvc_s2);		
			}
#endif
		}
	}
#endif /* #ifdef USB_UVC_SUPPORT */

#ifdef USB_MULTI_HID_SUPPORT
	if(g_hid_enable)
		status = hidg_bind_config(c, NULL, g_hid_interrupt_ep_enable);
#endif

#ifdef SFB_HID_SUPPORT
	if(g_sfb_hid_enable)
	{
		status = hidg_bind_config_consumer(c, NULL, g_sfb_hid_enable);
		
	}

	if(g_sfb_keyboard_enable)
	{
		status = hidg_bind_config_keyboard(c, NULL);
	}
#endif

#ifdef DFU_SUPPORT
	if(g_dfu_enable && g_dfu_last == 1)
		status = dfug_bind_config(c, g_dfu_enable == 2 ? 1:0, 0);
#endif

#ifdef USB_MULTI_MSG_SUPPORT
	if(g_msg_enable)
	{
		status = multi_msg_do_config(c);
		if(status != 0)
		{
			printk("multi_msg_do_config error\n");
		}
	}
#endif

#ifdef USB_MULTI_SUPPORT

	if(g_rndis_enable == 2)
	{
		status = multi_do_config(c);
		if(status != 0)
		{
			printk("multi_do_config error\n");
		}
	}
#endif

#ifdef UAC_AUDIO_SUPPORT	
	//Ron_Lee 20180124
	/* FIXME: add audio device*/
	if(g_audio_enable == 1)
	{
	    audio_do_config(c);
	}
	//
#endif

#ifdef USB_ZTE_HID_SUPPORT
	if(g_zte_hid_enable)
		status = hidg_zte_bind_config(c, NULL);
	
	if(g_zte_upgrade_hid_enable)
		status = hidg_zte_upgrade_bind_config(c, NULL);
#endif

#ifdef USB_POLYCOM_HID_SUPPORT
	if(g_polycom_hid_enable)
		status = polycom_hidg_bind_config(c, NULL);
#endif

	return status;
}


static struct usb_configuration webcam_config_driver = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_ONE,//USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= 896,//CONFIG_USB_GADGET_VBUS_DRAW,
};

#ifdef USB_UVC_SUPPORT

static struct usb_configuration webcam_config_driver_uvc150 = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 2,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_ONE,//USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= 896,//CONFIG_USB_GADGET_VBUS_DRAW,
};

static struct usb_configuration webcam_config_driver_win7 = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 3,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_ONE,//USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= 896,//CONFIG_USB_GADGET_VBUS_DRAW,
};

#endif

static int /* __init_or_exit */
webcam_unbind(struct usb_composite_dev *cdev)
{
#ifdef USB_UVC_SUPPORT
	if(g_uvc_enable)
	{
		if (!IS_ERR_OR_NULL(f_uvc))
			usb_put_function(f_uvc);
		if (!IS_ERR_OR_NULL(fi_uvc))
			usb_put_function_instance(fi_uvc);
		
	
#if (defined UVC_S2_SUPPORT || defined UVC_V150_SUPPORT)
		if (!IS_ERR_OR_NULL(f_uvc_s2))
			usb_put_function(f_uvc_s2);
		if (!IS_ERR_OR_NULL(fi_uvc_s2))
			usb_put_function_instance(fi_uvc_s2);
#endif
	}

#endif /* #ifdef USB_UVC_SUPPORT */

#ifdef UAC_AUDIO_SUPPORT
	//Ron_Lee 20180124
	/* FIXME: add audio device*/
	if(g_audio_enable)
	{
	    audio_unbind(cdev);
	}
#endif

#ifdef USB_MULTI_SUPPORT
	if(g_rndis_enable)
		return multi_unbind(cdev);
#endif

#ifdef USB_MULTI_MSG_SUPPORT
	if(g_msg_enable)
		return multi_msg_unbind(cdev);
#endif

	return 0;
}

#ifdef USB_UVC_SUPPORT
extern int get_usb_link_state(void);
bool get_bulk_streaming_ep(void);
int GetConfigurationValue(void);
enum usb_device_speed GetUsbSpeedCurrent(void);
static int camera_proc_show(struct seq_file *m, void *v)
{
	//struct usb_composite_dev *cdev = v;
	seq_printf(m, "\n");
	seq_printf(m, "-----ValueHD Corporation USB Camera Version:%s [Build:%s] -----\n", WEBCAM_VERSION,COMPILE_INFO);
	seq_printf(m, "-----svn/git:%s -----\n", SVN_GIT_VERSION);
	seq_printf(m, "\n");
	seq_printf(m, "-----USB Config---------------------------------------------------------------------------------------\n");
	seq_printf(m, "     usb_vid		usb_pid		usb_bcd		device_name        serial_number\n");
	seq_printf(m, "     0x%04x		0x%04x		0x%04x		%s        %s\n", g_vendor_id, g_customer_id, g_device_bcd, g_usb_product_label_arr, g_serial_number_arr);

	seq_printf(m, "\n");
	seq_printf(m, "-----Function Config-----------------------------------------------------------------------------------\n");
	seq_printf(m, "     dfu_enable  hid_enable  uac_enable  uvc_double_stream_enable  uvc_v150_enable  win7_usb3_enable  ir_hid_enable  bulk_streaming_ep\n");
	seq_printf(m, "     %d           %d           %d           %d                         %d                %d                 %d              %d\n", 
						g_dfu_enable, g_hid_enable, g_audio_enable, g_uvc_s2_enable, g_uvc_v150_enable, g_win7_usb3_enable, g_zte_hid_enable, get_bulk_streaming_ep());
	
	seq_printf(m, "\n");
	seq_printf(m, "-----USB Status----------------------------------------------------------------------------------------\n");
	seq_printf(m, "     USB_Speed  USB_ConfigurationValue USB_Link_State\n");
	seq_printf(m, "     %s         %d                  %d\n", usb_speed_string(GetUsbSpeedCurrent()), GetConfigurationValue(), get_usb_link_state());

	return 0;
}

static int camera_proc_open(struct inode *inode, struct  file *file) 
{
  	return single_open(file, camera_proc_show, NULL);
}
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
static const struct proc_ops camera_proc_fops = {
  //.owner = THIS_MODULE,
  .proc_open = camera_proc_open,
  .proc_read = seq_read,
  .proc_lseek = seq_lseek,
  .proc_release = single_release,
};
#else
static const struct file_operations camera_proc_fops = {
  .owner = THIS_MODULE,
  .open = camera_proc_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};
#endif

static int camera_proc_init(void) 
{
	struct proc_dir_entry *proc_file_entry;
	proc_file_entry = proc_create("usb_camera", 0, NULL, &camera_proc_fops);
	if(proc_file_entry == NULL)
	{
		printk("camera proc_create error!\n");
		return -ENOMEM;
	}
	return 0;
}

#endif


static int __init
webcam_bind(struct usb_composite_dev *cdev)
{
	int ret;
	
#ifdef USB_UVC_SUPPORT
	struct f_uvc_opts *uvc_opts = NULL;
	struct f_uvc_opts *uvc_s2_opts = NULL;

	uint32_t in_ep_max_num = 0;
	uint32_t out_ep_max_num = 0;
	uint32_t in_epnum = 0;
	uint32_t out_epnum = 0;
#endif

	//Ron_Lee 20180626
#if 0
static int g_vendor_id = 0x2e7e;
static int g_customer_id = 0x0703;
static char g_usb_vendor_label[64] = "Linux Foundation";
static char g_usb_product_label[64] = "HD Camera";

static int g_audio_enable = 1;
#endif
	printk("g_vendor_id:%x\n", g_vendor_id);
	printk("g_customer_id:%x\n", g_customer_id);
	printk("g_device_bcd:%x\n",g_device_bcd);

	printk("g_usb_vendor_label1:%s\n", g_usb_vendor_label1);
	printk("g_usb_vendor_label2:%s\n", g_usb_vendor_label2);
	printk("g_usb_vendor_label3:%s\n", g_usb_vendor_label3);
	printk("g_usb_vendor_label4:%s\n", g_usb_vendor_label4);

	printk("g_usb_product_label1:%s\n", g_usb_product_label1);
	printk("g_usb_product_label2:%s\n", g_usb_product_label2);
	printk("g_usb_product_label3:%s\n", g_usb_product_label3);
	printk("g_usb_product_label4:%s\n", g_usb_product_label4);
	printk("g_usb_product_label5:%s\n", g_usb_product_label5);

	printk("g_audio_enable:%d\n", g_audio_enable);
	
	strcat(g_usb_vendor_label_arr, g_usb_vendor_label1);
	if (g_usb_vendor_label2 != NULL && g_usb_vendor_label2[0] != ' ' && g_usb_vendor_label2[0] != '\0')
	{
	    strcat(g_usb_vendor_label_arr, " ");
	    strcat(g_usb_vendor_label_arr, g_usb_vendor_label2);
	}
	if (g_usb_vendor_label3 != NULL && g_usb_vendor_label3[0] != ' ' && g_usb_vendor_label3[0] != '\0')
	{
	    strcat(g_usb_vendor_label_arr, " ");
	    strcat(g_usb_vendor_label_arr, g_usb_vendor_label3);
	}
	if (g_usb_vendor_label4 != NULL && g_usb_vendor_label4[0] != ' ' && g_usb_vendor_label4[0] != '\0')
	{
	    strcat(g_usb_vendor_label_arr, " ");
	    strcat(g_usb_vendor_label_arr, g_usb_vendor_label4);
	}
	printk("g_usb_vendor_label_arr:%s\n", g_usb_vendor_label_arr);

	strcat(g_usb_product_label_arr, g_usb_product_label1);
	if (g_usb_product_label2 != NULL && g_usb_product_label2[0] != ' ' && g_usb_product_label2[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label2);
	}
	if (g_usb_product_label3 != NULL && g_usb_product_label3[0] != ' ' && g_usb_product_label3[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label3);
	}
	if (g_usb_product_label4 != NULL && g_usb_product_label4[0] != ' ' && g_usb_product_label4[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label4);
	}
	if (g_usb_product_label5 != NULL && g_usb_product_label5[0] != ' ' && g_usb_product_label5[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label5);
	}
	printk("g_usb_prodcut_label_arr:%s\n", g_usb_product_label_arr);
	
	strcat(g_serial_number_arr, g_serial_number);
	printk("g_serial_number_arr:%s\n", g_serial_number_arr);
	//

	if(self_power)
	{
		webcam_config_driver.bmAttributes = USB_CONFIG_ATT_SELFPOWER;
		webcam_config_driver.MaxPower = 0;

		webcam_config_driver_uvc150.bmAttributes = USB_CONFIG_ATT_SELFPOWER;
		webcam_config_driver_uvc150.MaxPower = 0;

		webcam_config_driver_win7.bmAttributes = USB_CONFIG_ATT_SELFPOWER;
		webcam_config_driver_win7.MaxPower = 0;
	}
#if 0
	//Ron_Lee 20180507
	unsigned int reg = 0;
	reg = readl(SCSYSID);
	

	if (reg == 0x1000001)
	{
	    flag_3519v101 = 1;
	}
	printk("reg:%x flag_3519v101:%d\n", reg, flag_3519v101);
#endif

	//printk("flag_3519v101:%d\n", flag_3519v101);

#ifdef UAC_AUDIO_SUPPORT	
	//
	//Ron_Lee 20180124	
	/* FIXME: add audio device*/
	if(g_audio_enable)
	{
	    audio_bind(cdev);
	}
	//
#endif

#ifdef USB_MULTI_SUPPORT
	if(g_rndis_enable)
	{
		if(multi_bind(cdev, g_rndis_enable) != 0)
		{
			printk("multi_bind fail\n");
		}
	}
#endif

#ifdef USB_MULTI_MSG_SUPPORT
	if(g_msg_enable)
	{
		if(multi_msg_bind(cdev) != 0)
		{
			printk("multi_msg_bind fail\n");
		}
	}
#endif

#ifdef USB_UVC_SUPPORT
	if(g_uvc_enable)
	{
		fi_uvc = usb_get_function_instance("uvc");
		if (IS_ERR_OR_NULL(fi_uvc))
			return PTR_ERR(fi_uvc);

		uvc_opts = container_of(fi_uvc, struct f_uvc_opts, func_inst);

		if(uvc_opts == NULL)
		{
			return -1;
		}

		
		uvc_opts->streaming_interval = streaming_interval;
		uvc_opts->streaming_maxpacket = streaming_maxpacket;
		uvc_opts->streaming_maxburst = streaming_maxburst;
		uvc_set_trace_param(trace);

		uvc_opts->dfu_enable = g_dfu_enable;
		uvc_opts->uac_enable = g_audio_enable;
		uvc_opts->uvc_s2_enable = g_uvc_s2_enable;
		uvc_opts->uvc_v150_enable = g_uvc_v150_enable;
		uvc_opts->win7_usb3_enable = g_win7_usb3_enable;
		uvc_opts->work_pump_enable = work_pump_enable;
		uvc_opts->zte_hid_enable = g_zte_hid_enable;
		uvc_opts->uvc_interrupt_ep_enable = g_uvc_interrupt_ep_enable;
		uvc_opts->auto_queue_data_enable = g_uvc_request_auto_enable;
		
		uvc_opts->usb_sn_string = g_serial_number_arr;
		uvc_opts->usb_sn_string_length = sizeof(g_serial_number_arr);

		uvc_opts->usb_vendor_string = g_usb_vendor_label_arr;
		uvc_opts->usb_vendor_string_length = sizeof(g_usb_vendor_label_arr);

		uvc_opts->usb_product_string = g_usb_product_label_arr;
		uvc_opts->usb_product_string_length = sizeof(g_usb_product_label_arr);

		get_ep_max_num (cdev->gadget, &in_ep_max_num, &out_ep_max_num);
		printk("%s IN EP max: %d, OUT EP max: %d\n", cdev->gadget->name, in_ep_max_num, out_ep_max_num);
		in_epnum = 1;//uvc stream 1 ep
		out_epnum = 0;
		if(g_audio_enable) in_epnum ++;	
		if(g_uvc_s2_enable || g_uvc_v150_enable) in_epnum ++;
		if(g_uvc_interrupt_ep_enable) in_epnum ++;
		if(g_zte_hid_enable)
		{
			in_epnum ++;
			out_epnum ++;
		}
		if(g_hid_enable && g_hid_interrupt_ep_enable)
		{
			in_epnum ++;
			out_epnum ++;
		}


		if(in_epnum > in_ep_max_num)
		{
			g_uvc_interrupt_ep_enable = 0;
			uvc_opts->uvc_interrupt_ep_enable = 0;
			in_epnum --;
		}

		if(in_epnum > in_ep_max_num || out_epnum > out_ep_max_num)
		{
			if(g_hid_enable && g_hid_interrupt_ep_enable)
			{
				in_epnum --;
				out_epnum --;
				g_hid_interrupt_ep_enable = 0;
			}
		}

		if(in_epnum > in_ep_max_num || out_epnum > out_ep_max_num)
		{
			printk("USB EP config error! in_epnum use:%d out_epnum use:%d, IN EP max: %d, OUT EP max: %d\n", 
				in_epnum, out_epnum, in_ep_max_num, out_ep_max_num);	
		}
		

		uvc_opts->fs_control = uvc_fs_control_cls;
		uvc_opts->ss_control = uvc_ss_control_cls;

		uvc_opts->fs_streaming = uvc_fs_streaming_cls;
		uvc_opts->hs_streaming = uvc_hs_streaming_cls;
		uvc_opts->ss_streaming = uvc_ss_streaming_cls;
		
		fi_uvc_s2 = usb_get_function_instance("uvc_s2");
		if (!IS_ERR_OR_NULL(fi_uvc_s2))
		{	
			uvc_s2_opts = container_of(fi_uvc_s2, struct f_uvc_opts, func_inst);
		}
	
#ifdef UVC_S2_SUPPORT
		if (!IS_ERR_OR_NULL(fi_uvc_s2) && g_uvc_s2_enable && uvc_s2_opts)
		{	
			uvc_s2_opts->work_pump_enable = work_pump_enable;
			uvc_s2_opts->uvc_s2_enable = g_uvc_s2_enable;

			uvc_s2_opts->fs_control = uvc_fs_control_cls_s2;
			uvc_s2_opts->ss_control = uvc_ss_control_cls_s2;
			
			uvc_opts->fs_control = uvc_fs_control_cls_s2;
			uvc_opts->ss_control = uvc_ss_control_cls_s2;

			uvc_s2_opts->fs_streaming = uvc_fs_streaming_cls_s2;
			uvc_s2_opts->hs_streaming = uvc_hs_streaming_cls_s2;
			uvc_s2_opts->ss_streaming = uvc_ss_streaming_cls_s2;
			uvc_s2_opts->auto_queue_data_enable = g_uvc_request_auto_enable;
		}
		else
		{
			//return PTR_ERR(fi_uvc_s2);
		}
#endif
	}
#endif /* #ifdef USB_UVC_SUPPORT */

	/* Allocate string descriptor numbers ... note that string contents
	 * can be overridden by the composite_dev glue.
	 */
	ret = usb_string_ids_tab(cdev, webcam_strings);
	if (ret < 0)
		goto error;

	if(g_zte_hid_enable || ManufacturerStrings)
	{
		webcam_device_descriptor.iManufacturer = webcam_strings[USB_GADGET_MANUFACTURER_IDX].id;
	}
	webcam_device_descriptor.iProduct = webcam_strings[USB_GADGET_PRODUCT_IDX].id;
	webcam_device_descriptor.iSerialNumber = webcam_strings[USB_GADGET_SERIAL_IDX].id;
	webcam_device_descriptor.idVendor = cpu_to_le16(g_vendor_id);
	webcam_device_descriptor.idProduct = cpu_to_le16(g_customer_id);
	webcam_device_descriptor.bcdDevice = cpu_to_le16(g_device_bcd);
	//webcam_config_driver.iConfiguration = webcam_strings[STRING_DESCRIPTION_IDX].id;

#ifdef DFU_SUPPORT 
	if(g_dfu_enable == 2 && g_audio_enable == 0 \
		&& g_hid_enable == 0 && g_uvc_enable == 0)
	{
		webcam_device_descriptor.bDeviceClass = USB_CLASS_PER_INTERFACE;
		webcam_device_descriptor.bDeviceSubClass = 0;
		webcam_device_descriptor.bDeviceProtocol = 0;
	}
	else
	{
		webcam_device_descriptor.bDeviceClass = USB_CLASS_MISC;
		webcam_device_descriptor.bDeviceSubClass = 0x02;
		webcam_device_descriptor.bDeviceProtocol = 0x01;
	}
#endif

#ifdef USB_MULTI_HID_SUPPORT 
	if(g_hid_enable)
	{
		/* set up HID */
		ret = ghid_setup(cdev->gadget, webcam_config_driver.bConfigurationValue);
		if (ret < 0){
			printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
			goto error;
		}
	}
#endif

#ifdef SFB_HID_SUPPORT
	if(g_sfb_hid_enable)
	{
		/* set up HID */
		ret = ghid_setup_consumer(cdev->gadget, webcam_config_driver.bConfigurationValue);
		if (ret < 0){
			printk("%s +%d %s: ghid_setup_consumer fail!\n", __FILE__,__LINE__,__func__);
			goto error;
		}	
	}

	if(g_sfb_keyboard_enable)
	{
		/* set up HID */
		ret = ghid_setup_keyboard(cdev->gadget, webcam_config_driver.bConfigurationValue);
		if (ret < 0){
			printk("%s +%d %s: ghid_setup_keyboard fail!\n", __FILE__,__LINE__,__func__);
			goto error;
		}
	}

#endif


#ifdef DFU_SUPPORT 
	if(g_dfu_enable)
	{
		/* set up DFU */
		ret = gdfu_setup(cdev->gadget, webcam_config_driver.bConfigurationValue);
		if (ret < 0){
			printk("%s +%d %s: gdfu_setup fail!\n", __FILE__,__LINE__,__func__);
			goto error;
		}
	}
#endif

#ifdef USB_ZTE_HID_SUPPORT
		if(g_zte_hid_enable)
		{
			/* set up HID */
			ret = ghid_zte_setup(cdev->gadget, webcam_config_driver.bConfigurationValue);
			if (ret < 0){
				printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
				goto error;
			}
		}
		if(g_zte_upgrade_hid_enable)
		{
			/* set up HID */
			ret = ghid_zte_upgrade_setup(cdev->gadget, webcam_config_driver.bConfigurationValue);
			if (ret < 0){
				printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
				goto error;
			}
		}
#endif

#ifdef USB_POLYCOM_HID_SUPPORT
		if(g_polycom_hid_enable)
		{
			/* set up HID */
			ret = polycom_ghid_setup(cdev->gadget, webcam_config_driver.bConfigurationValue);
			if (ret < 0){
				printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
				goto error;
			}
		}
#endif

	/* Register our configuration. */
	if ((ret = usb_add_config(cdev, &webcam_config_driver,
					webcam_config_bind)) < 0)
		goto error;

#ifdef USB_UVC_SUPPORT	
	if(g_uvc_enable)
	{
#ifdef UVC_V150_SUPPORT
		if(g_uvc_v150_enable)
		{
			uvc_opts->work_pump_enable = work_pump_enable;
			uvc_opts->fs_control = uvc150_fs_control_cls;
			uvc_opts->ss_control = uvc150_ss_control_cls;

			uvc_opts->fs_streaming = uvc_fs_streaming_cls;
			uvc_opts->hs_streaming = uvc_hs_streaming_cls;
			uvc_opts->ss_streaming = uvc_ss_streaming_cls;

			uvc_opts->auto_queue_data_enable = g_uvc_request_auto_enable;

			
			if (!IS_ERR_OR_NULL(fi_uvc_s2) && uvc_s2_opts)
			{			
				uvc_s2_opts->work_pump_enable = work_pump_enable;
				uvc_s2_opts->fs_control = uvc150_fs_control_cls;
				uvc_s2_opts->ss_control = uvc150_ss_control_cls;

				uvc_s2_opts->fs_streaming = uvc150_fs_streaming_cls_s2;
				uvc_s2_opts->hs_streaming = uvc150_hs_streaming_cls_s2;
				uvc_s2_opts->ss_streaming = uvc150_ss_streaming_cls_s2;
				uvc_s2_opts->auto_queue_data_enable = g_uvc_request_auto_enable;
			}
			else
			{
				//return PTR_ERR(fi_uvc_s2);
			}
			
#ifdef USB_MULTI_HID_SUPPORT 
			if(g_hid_enable)
			{
				/* set up HID */
				ret = ghid_setup(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: ghid_setup_config2 fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef SFB_HID_SUPPORT
			if(g_sfb_hid_enable)
			{
				/* set up HID */
				ret = ghid_setup_consumer(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: ghid_setup_consumer fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}

			if(g_sfb_keyboard_enable)
			{
				/* set up HID */
				ret = ghid_setup_keyboard(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: ghid_setup_keyboard fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef DFU_SUPPORT 
			if(g_dfu_enable)
			{
				/* set up DFU */
				ret = gdfu_setup(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: gdfu_setup_config2 fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef USB_ZTE_HID_SUPPORT
			if(g_zte_hid_enable)
			{
				/* set up HID */
				ret = ghid_zte_setup(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}

			if(g_zte_upgrade_hid_enable)
			{
				/* set up HID */
				ret = ghid_zte_upgrade_setup(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef USB_POLYCOM_HID_SUPPORT
			if(g_polycom_hid_enable)
			{
				/* set up HID */
				ret = polycom_ghid_setup(cdev->gadget, webcam_config_driver_uvc150.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif					
			/* Register our configuration. */
			if ((ret = usb_add_config(cdev, &webcam_config_driver_uvc150,
							webcam_config_bind)) < 0)
				goto error;
		}
#endif /*#ifdef UVC_V150_SUPPORT*/
	}
#endif /* #ifdef USB_UVC_SUPPORT */

#ifdef USB_UVC_SUPPORT
	if(g_uvc_enable)
	{
#ifdef USB3_WIN7_SUPPORT
		if(g_win7_usb3_enable)
		{
			uvc_opts->work_pump_enable = work_pump_enable;
			uvc_opts->fs_control = uvc_fs_control_cls;
			uvc_opts->ss_control = uvc_ss_control_cls;

			uvc_opts->fs_streaming = uvc_fs_streaming_cls;
			uvc_opts->hs_streaming = uvc_hs_streaming_cls;
			uvc_opts->ss_streaming = uvc_ss_streaming_cls;
			uvc_opts->auto_queue_data_enable = g_uvc_request_auto_enable;

		
#ifdef UVC_S2_SUPPORT
			if (!IS_ERR_OR_NULL(fi_uvc_s2) && g_uvc_s2_enable && uvc_s2_opts)
			{	
				uvc_s2_opts->work_pump_enable = work_pump_enable;
				uvc_s2_opts->fs_control = uvc_fs_control_cls_s2;
				uvc_s2_opts->ss_control = uvc_ss_control_cls_s2;
				
				uvc_opts->fs_control = uvc_fs_control_cls_s2;
				uvc_opts->ss_control = uvc_ss_control_cls_s2;

				uvc_s2_opts->fs_streaming = uvc_fs_streaming_cls_s2;
				uvc_s2_opts->hs_streaming = uvc_hs_streaming_cls_s2;
				uvc_s2_opts->ss_streaming = uvc_ss_streaming_cls_s2;
				uvc_s2_opts->auto_queue_data_enable = g_uvc_request_auto_enable;
			}
			else
			{
				//return PTR_ERR(fi_uvc_s2);
			}
#endif
		
#ifdef USB_MULTI_HID_SUPPORT 
			if(g_hid_enable)
			{
				/* set up HID */
				ret = ghid_setup(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: ghid_setup_config2 fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef SFB_HID_SUPPORT
			if(g_sfb_hid_enable)
			{
				/* set up HID */
				ret = ghid_setup_consumer(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: ghid_setup_consumer fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}

			if(g_sfb_keyboard_enable)
			{
				/* set up HID */
				ret = ghid_setup_keyboard(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: ghid_setup_keyboard fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef DFU_SUPPORT 
			if(g_dfu_enable)
			{
				/* set up DFU */
				ret = gdfu_setup(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: gdfu_setup_config2 fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef USB_ZTE_HID_SUPPORT
			if(g_zte_hid_enable)
			{
				/* set up HID */
				ret = ghid_zte_setup(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}

			if(g_zte_upgrade_hid_enable)
			{
				/* set up HID */
				ret = ghid_zte_upgrade_setup(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif

#ifdef USB_POLYCOM_HID_SUPPORT
			if(g_polycom_hid_enable)
			{
				/* set up HID */
				ret = polycom_ghid_setup(cdev->gadget, webcam_config_driver_win7.bConfigurationValue);
				if (ret < 0){
					printk("%s +%d %s: hidg_init fail!\n", __FILE__,__LINE__,__func__);
					goto error;
				}
			}
#endif				
		/* Register our configuration. */
		if ((ret = usb_add_config(cdev, &webcam_config_driver_win7,
						webcam_config_bind)) < 0)
			goto error;
		}
#endif /*#ifdef USB3_WIN7_SUPPORT*/	
	}

	camera_proc_init();
#endif /* #ifdef USB_UVC_SUPPORT */

	//usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "Webcam Video Gadget\n");

	if(usb_connect)
	{
		printk("usb_gadget_connect\n");
		if(usb_gadget_connect(cdev->gadget) != 0)
			printk("usb_gadget_connect error!\n");
	}

	return 0;

error:
#ifdef USB_UVC_SUPPORT
	if(g_uvc_enable)
	{
		usb_put_function_instance(fi_uvc);
	}
#endif
	return ret;
}


/* --------------------------------------------------------------------------
 * Driver
 */

static __refdata struct usb_composite_driver webcam_driver = {
	.name		= "g_webcam",
	.dev		= &webcam_device_descriptor,
	.strings	= webcam_device_strings,
	.max_speed	= USB_SPEED_SUPER_PLUS,
	.bind		= webcam_bind,
	.unbind		= webcam_unbind,
};

module_usb_composite_driver(webcam_driver);

MODULE_AUTHOR("VHD");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.3");

//EXPORT_SYMBOL(g_vendor_id);

