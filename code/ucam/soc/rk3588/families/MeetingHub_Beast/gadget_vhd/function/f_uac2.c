// SPDX-License-Identifier: GPL-2.0+
/*
 * f_uac2.c -- USB Audio Class 2.0 Function
 *
 * Copyright (C) 2011
 *    Yadwinder Singh (yadi.brar01@gmail.com)
 *    Jaswinder Singh (jaswinder.singh@linaro.org)
 *
 * Copyright (C) 2020
 *    Ruslan Bilovol (ruslan.bilovol@gmail.com)
 */

#include <linux/usb/audio.h>
#include <linux/usb/audio-v2.h>
#include <linux/module.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>


#include "uac2_audio_ex.h"
#include "uac_v4l2_ex.h"
#include "u_uac2.h"

static int mic_volume_def = 0x3200; 
module_param(mic_volume_def, int, S_IRUGO);
MODULE_PARM_DESC(mic_volume_def, "Mic Volume Def");

static int mic_volume_min = 0x1400; 
module_param(mic_volume_min, int, S_IRUGO);
MODULE_PARM_DESC(mic_volume_min, "Mic Volume Min");

static int mic_volume_max= 0x3200;
module_param(mic_volume_max, int, S_IRUGO);
MODULE_PARM_DESC(mic_volume_max, "Mic Volume Max");

static int mic_volume_res= 0x0080;
module_param(mic_volume_res, int, S_IRUGO);
MODULE_PARM_DESC(mic_volume_res, "Mic Volume Res");

static int mic_srate_min = 16000;
module_param(mic_srate_min, int, S_IRUGO);
MODULE_PARM_DESC(mic_srate_min, "Mic Srate Min");

static int mic_srate_max = 48000;
module_param(mic_srate_max, int, S_IRUGO);
MODULE_PARM_DESC(mic_srate_max, "Mic Srate Max");

static int mic_srate_res = 16000;
module_param(mic_srate_res, int, S_IRUGO);
MODULE_PARM_DESC(mic_srate_res, "Mic Srate Res");


static int speak_volume_def = 0x0000;
module_param(speak_volume_def, int, S_IRUGO);
MODULE_PARM_DESC(speak_volume_def, "Speak Volume Def");

static int speak_volume_min = 0xC402;
module_param(speak_volume_min, int, S_IRUGO);
MODULE_PARM_DESC(speak_volume_min, "Speak Volume Min");

static int speak_volume_max= 0x0000;
module_param(speak_volume_max, int, S_IRUGO);
MODULE_PARM_DESC(speak_volume_max, "Speak Volume Max");

static int speak_volume_res= 0x0100;
module_param(speak_volume_res, int, S_IRUGO);
MODULE_PARM_DESC(speak_volume_res, "Speak Volume Res");

static int speak_srate_min = 16000;
module_param(speak_srate_min, int, S_IRUGO);
MODULE_PARM_DESC(speak_srate_min, "Speak Srate Min");

static int speak_srate_max = 48000;
module_param(speak_srate_max, int, S_IRUGO);
MODULE_PARM_DESC(speak_srate_max, "Speak Srate Max");

static int speak_srate_res = 16000;
module_param(speak_srate_res, int, S_IRUGO);
MODULE_PARM_DESC(speak_srate_res, "Speak Srate Res");


static unsigned int alsa_buf_num = 6;
module_param(alsa_buf_num, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(alsa_buf_num, "alsa_buf_num");

static char uac_name[128] =  {0};
static char *uac_name_label1 = NULL;
static char *uac_name_label2 = NULL;
static char *uac_name_label3 = NULL;
static char *uac_name_label4 = NULL;
static char *uac_name_label5 = NULL;

module_param(uac_name_label1, charp, 0644);
module_param(uac_name_label2, charp, 0644);
module_param(uac_name_label3, charp, 0644);
module_param(uac_name_label4, charp, 0644);
module_param(uac_name_label5, charp, 0644);
MODULE_PARM_DESC(uac_name_label1,"uac name string label");

static char mic_name[128] =  {0};
static char *mic_name_label1 = NULL;
static char *mic_name_label2 = NULL;
static char *mic_name_label3 = NULL;
static char *mic_name_label4 = NULL;
static char *mic_name_label5 = NULL;

module_param(mic_name_label1, charp, 0644);
module_param(mic_name_label2, charp, 0644);
module_param(mic_name_label3, charp, 0644);
module_param(mic_name_label4, charp, 0644);
module_param(mic_name_label5, charp, 0644);
MODULE_PARM_DESC(mic_name_label1,"mic name string label");

static char speak_name[128] =  {0};
static char *speak_name_label1 = NULL;
static char *speak_name_label2 = NULL;
static char *speak_name_label3 = NULL;
static char *speak_name_label4 = NULL;
static char *speak_name_label5 = NULL;

module_param(speak_name_label1, charp, 0644);
module_param(speak_name_label2, charp, 0644);
module_param(speak_name_label3, charp, 0644);
module_param(speak_name_label4, charp, 0644);
module_param(speak_name_label5, charp, 0644);
MODULE_PARM_DESC(speak_name_label1,"speak name string label");

static int test_mode = 0;
module_param(test_mode, int, 0644);

static unsigned int mic_uac_type = 0x0405;
module_param(mic_uac_type, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(mic_uac_type, "mic_uac_type");

static unsigned int speak_uac_type = 0x0405;
module_param(speak_uac_type, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(speak_uac_type, "speak_uac_type");

/* UAC2 spec: 4.1 Audio Channel Cluster Descriptor */
// #define UAC2_CHANNEL_MASK 0x07FFFFFF

/*
 * The driver implements a simple UAC_2 topology.
 * USB-OUT -> IT_1 -> FU -> OT_3 -> ALSA_Capture
 * ALSA_Playback -> IT_2 -> FU -> OT_4 -> USB-IN
 * Capture and Playback sampling rates are independently
 *  controlled by two clock sources :
 *    CLK_5 := c_srate, and CLK_6 := p_srate
 */

#define CONTROL_ABSENT	0
#define CONTROL_RDONLY	1
#define CONTROL_RDWR	3

#define CLK_FREQ_CTRL	0
#define CLK_VLD_CTRL	2
#define FU_MUTE_CTRL	0
#define FU_VOL_CTRL	2

#define UAC2_CS_CONTROL_CLOCK_VALID 0x02

#define COPY_CTRL	0
#define CONN_CTRL	2
#define OVRLD_CTRL	4
#define CLSTR_CTRL	6
#define UNFLW_CTRL	8
#define OVFLW_CTRL	10

#define UAC2_DT_CLK_SOURCE_SIZE		8
#define UAC2_DT_INPUT_TERMINAL_SIZE		17
#define UAC2_DT_OUTPUT_TERMINAL_SIZE		12

// #define UAC2_DT_FEATURE_UNIT_SIZE(ch)		(6 + ((ch) + 1) * 4)
#define UAC2_DT_FEATURE_UNIT_SIZE(ch)		(6 + ((ch) + 1) * 4)

/* 
 * Feature Unit Descriptor for ch=1 (Master + 1 logical channel)
 * UAC 2.0 spec: bLength = 6 + (ch+1)*4 = 6 + 8 = 14 bytes
 * Layout:
 *   - Header: 5 bytes (bLength, bDescriptorType, bDescriptorSubtype, bUnitID, bSourceID)
 *   - bmaControls: (ch+1)*4 = 8 bytes (Master + 1 channel)
 *   - iFeature: 1 byte
 */
struct uac2_feature_unit_descriptor_0 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bSourceID;
	__le32 bmaControls[3]; /* Master + n channel  */
	__u8 iFeature;
} __attribute__((packed));

int get_uac_test_mode(void)
{
	return test_mode;
}

struct f_uac2 {
	struct list_head cs;
	struct uac_device uac_dev;
	u8 ac_intf, as_in_intf, as_out_intf;
	u8 ac_alt, as_in_alt, as_out_alt;	/* needed for get_alt() */

	int clock_id;

	u8 set_cmd;
	struct usb_audio_control *set_con;
	u32 event_type;
};

static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value);
static int generic_get_cmd(struct usb_audio_control *con, u8 cmd);
static int sam_rate_set_cmd(struct usb_audio_control *con, u8 cmd, int value);
static int sam_rate_get_cmd(struct usb_audio_control *con, u8 cmd);
static int clk_valid_get_cmd(struct usb_audio_control *con, u8 cmd);

static inline struct f_uac2 *func_to_uac2(struct usb_function *f)
{
	return container_of(f, struct f_uac2, uac_dev.func);
}


static inline
struct f_uac2_opts *func_to_uac2_opts(struct usb_function *fn)
{
	return container_of(fn->fi, struct f_uac2_opts, func_inst);
}


/* --------- USB Function Interface ------------- */

#define F_AUDIO_AC_INTERFACE		0
#define F_AUDIO_AS_OUT_INTERFACE	1
#define F_AUDIO_AS_IN_INTERFACE		2
/* Number of streaming interfaces */
#define F_AUDIO_NUM_INTERFACES		2

static struct usb_interface_assoc_descriptor uac_iad = {
	.bLength		= sizeof uac_iad,
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0,
	.bInterfaceCount	= 3,
	.bFunctionClass		= USB_CLASS_AUDIO,
	.bFunctionSubClass	= UAC2_FUNCTION_SUBCLASS_UNDEFINED,
	.bFunctionProtocol	= UAC_VERSION_2,
	.iFunction		= 0,
};

/* Audio Control Interface */
static struct usb_interface_descriptor ac_interface_desc = {
	.bLength = sizeof ac_interface_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bNumEndpoints = 0, 
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.bInterfaceProtocol = UAC_VERSION_2,
};

#define UAC2_DT_AC_HEADER_SIZE		9

#define UAC_DT_TOTAL_LENGTH (UAC2_DT_AC_HEADER_SIZE \
	+ 2*UAC2_DT_INPUT_TERMINAL_SIZE + 2*UAC2_DT_OUTPUT_TERMINAL_SIZE \
	+ UAC2_DT_FEATURE_UNIT_SIZE(1) + UAC2_DT_FEATURE_UNIT_SIZE(1) \
	+ 2*UAC2_DT_CLK_SOURCE_SIZE)


static struct uac2_ac_header_descriptor ac_header_desc = {
	.bLength = UAC2_DT_AC_HEADER_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_MS_HEADER,
	.bcdADC = cpu_to_le16(0x200),
	.bCategory = UAC2_FUNCTION_IO_BOX,
	.wTotalLength = cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bmControls = 0,
};

#define USB_OUT_IT_ID	1
#define IO_OUT_OT_ID	2
#define IO_IN_IT_ID	3
#define USB_IN_OT_ID	4
#define FEATURE_UNIT_MIC_ID	5
#define FEATURE_UNIT_SPK_ID	6
#define CLOCK_SOURCE_MIC_ID	7
#define CLOCK_SOURCE_SPK_ID	8

/* Clock source for IN traffic */
static struct uac_clock_source_descriptor mic_clk_src_desc = {
	.bLength = UAC2_DT_CLK_SOURCE_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,

	.bDescriptorSubtype = UAC2_CLOCK_SOURCE,
	.bClockID = CLOCK_SOURCE_MIC_ID,
	.bmAttributes = UAC_CLOCK_SOURCE_TYPE_INT_PROG,
	.bmControls = (CONTROL_RDWR << CLK_FREQ_CTRL),
	.bAssocTerminal = 0,
};

/* Clock source for OUT traffic */
static struct uac_clock_source_descriptor spk_clk_src_desc = {
	.bLength = UAC2_DT_CLK_SOURCE_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC2_CLOCK_SOURCE,
	.bClockID = CLOCK_SOURCE_SPK_ID,
	.bmAttributes = UAC_CLOCK_SOURCE_TYPE_INT_PROG,
	.bmControls = (CONTROL_RDWR << CLK_FREQ_CTRL),
	.bAssocTerminal = 0,
};

/* Input Terminal for USB_OUT */
static struct uac2_input_terminal_descriptor usb_out_it_desc = {
	.bLength = UAC2_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_INPUT_TERMINAL,
	.bTerminalID = USB_OUT_IT_ID,
	.wTerminalType = cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal = 0,
	.bCSourceID = CLOCK_SOURCE_SPK_ID,
	.iChannelNames = 0,
	.bmControls = cpu_to_le16(CONTROL_RDWR << COPY_CTRL),
};

static struct uac2_feature_unit_descriptor_0 feature_unit_spk_desc = {
	.bLength = UAC2_DT_FEATURE_UNIT_SIZE(2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FEATURE_UNIT,
	.bUnitID = FEATURE_UNIT_SPK_ID,
	.bSourceID = USB_OUT_IT_ID,
	.bmaControls[0] = cpu_to_le32((CONTROL_RDWR << FU_MUTE_CTRL) | (CONTROL_RDWR << FU_VOL_CTRL)), 
	.bmaControls[1] = 0,
	.bmaControls[2] = 0,
	.iFeature = 0,
};

/* Ouput Terminal for I/O-Out */
static struct uac2_output_terminal_descriptor io_out_ot_desc = {
	.bLength = UAC2_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,

	.bDescriptorSubtype = UAC_OUTPUT_TERMINAL,
	.bTerminalID = IO_OUT_OT_ID,
	.wTerminalType = cpu_to_le16(UAC_OUTPUT_TERMINAL_SPEAKER),
	.bAssocTerminal = 0,
	.bSourceID = FEATURE_UNIT_SPK_ID,
	.bCSourceID = CLOCK_SOURCE_SPK_ID,
	.bmControls = cpu_to_le16(CONTROL_RDWR << COPY_CTRL),
};

/* Input Terminal for I/O-In */
static struct uac2_input_terminal_descriptor io_in_it_desc = {
	.bLength = UAC2_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,

	.bDescriptorSubtype = UAC_INPUT_TERMINAL,
	.bTerminalID = IO_IN_IT_ID,
	.wTerminalType = cpu_to_le16(UAC_INPUT_TERMINAL_MICROPHONE),
	.bAssocTerminal = 0,
	.bCSourceID = CLOCK_SOURCE_MIC_ID,
	.iChannelNames = 0,
	.bmControls = cpu_to_le16(CONTROL_RDWR << COPY_CTRL),
};

/*Audio Control Feature Unit Descriptor*/
static struct uac2_feature_unit_descriptor_0 feature_unit_mic_desc = {
	.bLength = UAC2_DT_FEATURE_UNIT_SIZE(2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FEATURE_UNIT,
	.bUnitID = FEATURE_UNIT_MIC_ID,
	.bSourceID = IO_IN_IT_ID,
	.bmaControls[0] = cpu_to_le32((CONTROL_RDWR << FU_MUTE_CTRL) | (CONTROL_RDWR << FU_VOL_CTRL)),
	.bmaControls[1] = 0,
	.bmaControls[2] = 0,
	.iFeature = 0,
};

/* Ouput Terminal for USB_IN */
static struct uac2_output_terminal_descriptor usb_in_ot_desc = {
	.bLength = UAC2_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_OUTPUT_TERMINAL,
	.bTerminalID = USB_IN_OT_ID,
	.wTerminalType = cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal = 0,
	.bSourceID = FEATURE_UNIT_MIC_ID,
	.bCSourceID = CLOCK_SOURCE_MIC_ID,
	.bmControls = cpu_to_le16(CONTROL_RDWR << COPY_CTRL),
};

/* AC IN Interrupt Endpoint */
#if 0
static struct usb_endpoint_descriptor fs_ep_int_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = cpu_to_le16(6),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_ep_int_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = cpu_to_le16(6),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_ep_int_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = cpu_to_le16(6),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_ep_int_desc_comp = {
	.bLength = sizeof(ss_ep_int_desc_comp),
	.bDescriptorType = USB_DT_SS_ENDPOINT_COMP,
	.wBytesPerInterval = cpu_to_le16(6),
};
#endif

/* Audio Streaming OUT Interface - Alt0 */
static struct usb_interface_descriptor as_out_interface_alt_0_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming OUT Interface - Alt1 */
static struct usb_interface_descriptor as_out_interface_alt_1_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 1,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming OUT Interface - Alt2 */
static struct usb_interface_descriptor as_out_interface_alt_2_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 2,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming OUT Interface - Alt3 */
static struct usb_interface_descriptor as_out_interface_alt_3_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 3,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming IN Interface - Alt0 */
static struct usb_interface_descriptor as_in_interface_alt_0_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming IN Interface - Alt1 */
static struct usb_interface_descriptor as_in_interface_alt_1_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 1,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming IN Interface - Alt2 */
static struct usb_interface_descriptor as_in_interface_alt_2_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 2,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Streaming IN Interface - Alt3 */
static struct usb_interface_descriptor as_in_interface_alt_3_desc = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,

	.bAlternateSetting = 3,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_2,
};

/* Audio Stream OUT Intface Desc */
static struct uac2_as_header_descriptor as_out_hdr_desc = {
	.bLength = sizeof as_out_hdr_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,

	.bDescriptorSubtype = UAC_AS_GENERAL,
	.bTerminalLink = USB_OUT_IT_ID,
	.bmControls = 0,
	.bFormatType = UAC_FORMAT_TYPE_I,
	.bmFormats = cpu_to_le32(UAC_FORMAT_TYPE_I_PCM),
	.iChannelNames = 0,
};

/* Audio Stream IN Intface Desc */
static struct uac2_as_header_descriptor as_in_hdr_desc = {
	.bLength = sizeof as_in_hdr_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,

	.bDescriptorSubtype = UAC_AS_GENERAL,
	.bTerminalLink = USB_IN_OT_ID,
	.bmControls = 0,
	.bFormatType = UAC_FORMAT_TYPE_I,
	.bmFormats = cpu_to_le32(UAC_FORMAT_TYPE_I_PCM),
	.iChannelNames = 0,
};

/* Audio USB_OUT Format */
static struct uac2_format_type_i_descriptor as_out_type_i_desc = {
	.bLength = sizeof as_out_type_i_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType = UAC_FORMAT_TYPE_I,
};

/* STD AS ISO OUT Endpoint */
static struct usb_endpoint_descriptor fs_epout_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_FS_EP_MAX_PACKET_SIZE),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epout_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_HS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epout_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epout_desc_comp = {
	.bLength		= sizeof(ss_epout_desc_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
};

/* Audio USB_OUT Format Alt 2*/
static struct uac2_format_type_i_descriptor as_out_type_i_desc_alt2 = {
	.bLength = sizeof as_out_type_i_desc_alt2,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType = UAC_FORMAT_TYPE_I,
};

/* STD AS ISO OUT Endpoint Alt 2*/
static struct usb_endpoint_descriptor fs_epout_desc_alt2 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_FS_EP_MAX_PACKET_SIZE),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epout_desc_alt2 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_HS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epout_desc_alt2 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epout_desc_comp_alt2 = {
	.bLength		= sizeof(ss_epout_desc_comp_alt2),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
};

/* Audio USB_OUT Format Alt 3*/
static struct uac2_format_type_i_descriptor as_out_type_i_desc_alt3 = {
	.bLength = sizeof as_out_type_i_desc_alt3,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType = UAC_FORMAT_TYPE_I,
};

/* STD AS ISO OUT Endpoint Alt 3*/
static struct usb_endpoint_descriptor fs_epout_desc_alt3 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_FS_EP_MAX_PACKET_SIZE),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epout_desc_alt3 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_HS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epout_desc_alt3 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epout_desc_comp_alt3 = {
	.bLength		= sizeof(ss_epout_desc_comp_alt3),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
};

#if 0
/* STD AS ISO IN Feedback Endpoint */
static struct usb_endpoint_descriptor fs_epin_fback_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_USAGE_FEEDBACK,
	.wMaxPacketSize = cpu_to_le16(3),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epin_fback_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_USAGE_FEEDBACK,
	.wMaxPacketSize = cpu_to_le16(4),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epin_fback_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_USAGE_FEEDBACK,
	.wMaxPacketSize = cpu_to_le16(4),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epin_fback_desc_comp = {
	.bLength		= sizeof(ss_epin_fback_desc_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval	= cpu_to_le16(4),
};
#endif

/* Audio USB_IN Format */
static struct uac2_format_type_i_descriptor as_in_type_i_desc = {
	.bLength = sizeof as_in_type_i_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType = UAC_FORMAT_TYPE_I,
};

/* STD AS ISO IN Endpoint */
static struct usb_endpoint_descriptor fs_epin_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_FS_EP_MAX_PACKET_SIZE),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epin_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_HS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epin_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epin_desc_comp = {
	.bLength		= sizeof(ss_epin_desc_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
};

/* Audio USB_IN Format Alt 2*/
static struct uac2_format_type_i_descriptor as_in_type_i_desc_alt2 = {
	.bLength = sizeof as_in_type_i_desc_alt2,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType = UAC_FORMAT_TYPE_I,
};

/* STD AS ISO IN Endpoint Alt 2*/
static struct usb_endpoint_descriptor fs_epin_desc_alt2 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_FS_EP_MAX_PACKET_SIZE),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epin_desc_alt2 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_HS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epin_desc_alt2 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epin_desc_comp_alt2 = {
	.bLength		= sizeof(ss_epin_desc_comp),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
};

/* Audio USB_IN Format Alt 3*/
static struct uac2_format_type_i_descriptor as_in_type_i_desc_alt3 = {
	.bLength = sizeof as_in_type_i_desc_alt3,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FORMAT_TYPE,
	.bFormatType = UAC_FORMAT_TYPE_I,
};

/* STD AS ISO IN Endpoint Alt 3*/
static struct usb_endpoint_descriptor fs_epin_desc_alt3 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_FS_EP_MAX_PACKET_SIZE),
	.bInterval = 1,
};

static struct usb_endpoint_descriptor hs_epin_desc_alt3 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_HS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_endpoint_descriptor ss_epin_desc_alt3 = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
	.wMaxPacketSize = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
	.bInterval = 4,
};

static struct usb_ss_ep_comp_descriptor ss_epin_desc_comp_alt3 = {
	.bLength		= sizeof(ss_epin_desc_comp_alt3),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst		= 0,
	.bmAttributes		= 0,
	.wBytesPerInterval = cpu_to_le16(UAC2_SS_EP_MAX_PACKET_SIZE),
};

/* CS AS ISO OUT Endpoint */
static struct uac2_iso_endpoint_descriptor as_iso_out_desc = {
	.bLength = sizeof as_iso_out_desc,
	.bDescriptorType = USB_DT_CS_ENDPOINT,

	.bDescriptorSubtype = UAC_EP_GENERAL,
	.bmAttributes = 0,
	.bmControls = 0,
	.bLockDelayUnits = 0,
	.wLockDelay = 0,
};

/* CS AS ISO IN Endpoint */
static struct uac2_iso_endpoint_descriptor as_iso_in_desc = {
	.bLength = sizeof as_iso_in_desc,
	.bDescriptorType = USB_DT_CS_ENDPOINT,

	.bDescriptorSubtype = UAC_EP_GENERAL,
	.bmAttributes = 0,
	.bmControls = 0,
	.bLockDelayUnits = 0,
	.wLockDelay = 0,
};

static struct usb_audio_control mic_mute_control = {
	.list = LIST_HEAD_INIT(mic_mute_control.list),
	.name = "Mute Control",
	.type = UAC_FU_MUTE,
	/* Todo: add real Mute control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control mic_volume_control = {
	.list = LIST_HEAD_INIT(mic_volume_control.list),
	.name = "Volume Control",
	.type = UAC_FU_VOLUME,
	/* Todo: add real Volume control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control spk_mute_control = {
	.list = LIST_HEAD_INIT(spk_mute_control.list),
	.name = "Mute Control",
	.type = UAC_FU_MUTE,
	/* Todo: add real Mute control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control spk_volume_control = {
	.list = LIST_HEAD_INIT(spk_volume_control.list),
	.name = "Volume Control",
	.type = UAC_FU_VOLUME,
	/* Todo: add real Volume control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

struct uac2_clock_control {
    struct usb_audio_control ctrl;
    struct f_uac2 *parent;
    u8 clock_id;
	u32 *srates;
    unsigned int srates_cnt;
};

static struct uac2_clock_control mic_clock_control = {
	.ctrl = {
		.list = LIST_HEAD_INIT(mic_clock_control.ctrl.list),
		.name = "Sample Rate Control",
		.type = UAC2_CS_CONTROL_SAM_FREQ,
		/* Todo: add real Sample Rate control code */
		.set = sam_rate_set_cmd,
		.get = sam_rate_get_cmd,
	},
	.clock_id = CLOCK_SOURCE_MIC_ID,
};

static struct uac2_clock_control spk_clock_control = {
	.ctrl = {
		.list = LIST_HEAD_INIT(spk_clock_control.ctrl.list),
		.name = "Sample Rate Control",
		.type = UAC2_CS_CONTROL_SAM_FREQ,
		/* Todo: add real Sample Rate control code */
		.set = sam_rate_set_cmd,
		.get = sam_rate_get_cmd,
	},
	.clock_id = CLOCK_SOURCE_SPK_ID,
};

static struct usb_audio_control_selector feature_unit_mic = {
	.list = LIST_HEAD_INIT(feature_unit_mic.list),
	.id = FEATURE_UNIT_MIC_ID,
	.name = "Mute & Volume Control",
	.type = UAC_FEATURE_UNIT,
	.desc = (struct usb_descriptor_header *)&feature_unit_mic_desc,
};

static struct usb_audio_control_selector feature_unit_spk = {
	.list = LIST_HEAD_INIT(feature_unit_spk.list),
	.id = FEATURE_UNIT_SPK_ID,
	.name = "Mute & Volume Control",
	.type = UAC_FEATURE_UNIT,
	.desc = (struct usb_descriptor_header *)&feature_unit_spk_desc,
};

static struct usb_audio_control_selector  mic_clk_src = {
	.list = LIST_HEAD_INIT(mic_clk_src.list),
	.id =  CLOCK_SOURCE_MIC_ID,
	.name = "Input Clock Source",
	.type = UAC2_CLOCK_SOURCE,
	.desc = (struct usb_descriptor_header *)&mic_clk_src_desc,
};

static struct usb_audio_control_selector  spk_clk_src = {
	.list = LIST_HEAD_INIT(spk_clk_src.list),
	.id =  CLOCK_SOURCE_SPK_ID,
	.name = "Output Clock Source",
	.type = UAC2_CLOCK_SOURCE,
	.desc = (struct usb_descriptor_header *)&spk_clk_src_desc,
};

static struct usb_descriptor_header *fs_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,

	(struct usb_descriptor_header *)&ac_header_desc,
	(struct usb_descriptor_header *)&mic_clk_src_desc,
	(struct usb_descriptor_header *)&spk_clk_src_desc,
	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&feature_unit_spk_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&feature_unit_mic_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc,
	(struct usb_descriptor_header *)&fs_epout_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_2_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc_alt2,
	(struct usb_descriptor_header *)&fs_epout_desc_alt2,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_3_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc_alt3,
	(struct usb_descriptor_header *)&fs_epout_desc_alt3,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc,
	(struct usb_descriptor_header *)&fs_epin_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_2_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc_alt2,
	(struct usb_descriptor_header *)&fs_epin_desc_alt2,
	(struct usb_descriptor_header *)&as_iso_in_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_3_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc_alt3,
	(struct usb_descriptor_header *)&fs_epin_desc_alt3,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

static struct usb_descriptor_header *hs_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,

	(struct usb_descriptor_header *)&ac_header_desc,
	(struct usb_descriptor_header *)&mic_clk_src_desc,
	(struct usb_descriptor_header *)&spk_clk_src_desc,
	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&feature_unit_spk_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&feature_unit_mic_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc,
	(struct usb_descriptor_header *)&hs_epout_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_2_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc_alt2,
	(struct usb_descriptor_header *)&hs_epout_desc_alt2,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_3_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc_alt3,
	(struct usb_descriptor_header *)&hs_epout_desc_alt3,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc,
	(struct usb_descriptor_header *)&hs_epin_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_2_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc_alt2,
	(struct usb_descriptor_header *)&hs_epin_desc_alt2,
	(struct usb_descriptor_header *)&as_iso_in_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_3_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc_alt3,
	(struct usb_descriptor_header *)&hs_epin_desc_alt3,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

static struct usb_descriptor_header *ss_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,

	(struct usb_descriptor_header *)&ac_header_desc,
	(struct usb_descriptor_header *)&mic_clk_src_desc,
	(struct usb_descriptor_header *)&spk_clk_src_desc,
	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&feature_unit_spk_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&feature_unit_mic_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc,
	(struct usb_descriptor_header *)&ss_epout_desc,
	(struct usb_descriptor_header *)&ss_epout_desc_comp,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_2_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc_alt2,
	(struct usb_descriptor_header *)&ss_epout_desc_alt2,
	(struct usb_descriptor_header *)&ss_epout_desc_comp_alt2,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_3_desc,
	(struct usb_descriptor_header *)&as_out_hdr_desc,
	(struct usb_descriptor_header *)&as_out_type_i_desc_alt3,
	(struct usb_descriptor_header *)&ss_epout_desc_alt3,
	(struct usb_descriptor_header *)&ss_epout_desc_comp_alt3,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc,
	(struct usb_descriptor_header *)&ss_epin_desc,
	(struct usb_descriptor_header *)&ss_epin_desc_comp,
	(struct usb_descriptor_header *)&as_iso_in_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_2_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc_alt2,
	(struct usb_descriptor_header *)&ss_epin_desc_alt2,
	(struct usb_descriptor_header *)&ss_epin_desc_comp_alt2,
	(struct usb_descriptor_header *)&as_iso_in_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_3_desc,
	(struct usb_descriptor_header *)&as_in_hdr_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc_alt3,
	(struct usb_descriptor_header *)&ss_epin_desc_alt3,
	(struct usb_descriptor_header *)&ss_epin_desc_comp_alt3,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

enum {
	STR_AC_IF,
	STR_MIC_IF,
	STR_SPEAK_IF,
};

static struct usb_string strings_fn[] = {
	[STR_AC_IF].s	= uac_name,
	[STR_MIC_IF].s	= mic_name,
	[STR_SPEAK_IF].s	= speak_name,
	{},			/* end of list */
};

static struct usb_gadget_strings str_fn = {
	.language = 0x0409,	/* en-us */
	.strings = strings_fn,
};

static struct usb_gadget_strings *fn_strings[] = {
	&str_fn,
	NULL,
};

struct cntrl_cur_lay2 {
	__le16	wCUR;
};

struct cntrl_range_lay2 {
	__le16	wNumSubRanges;
	__le16	wMIN;
	__le16	wMAX;
	__le16	wRES;
} __packed;

struct cntrl_cur_lay3 {
	__le32	dCUR;
};

struct cntrl_subrange_lay3 {
	__le32	dMIN;
	__le32	dMAX;
	__le32	dRES;
} __packed;

#define ranges_lay3_size(c) (sizeof(c.wNumSubRanges)	\
		+ le16_to_cpu(c.wNumSubRanges)		\
		* sizeof(struct cntrl_subrange_lay3))

#define DECLARE_UAC2_CNTRL_RANGES_LAY3(k, n)		\
	struct cntrl_ranges_lay3_##k {			\
	__le16	wNumSubRanges;				\
	struct cntrl_subrange_lay3 r[n];		\
} __packed

DECLARE_UAC2_CNTRL_RANGES_LAY3(srates, UAC_MAX_RATES);
/* Use macro to overcome line length limitation */


/*-------------------------------------------------------------------------*/

static void f_uac2_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_uac2 *uac2 = req->context;
	int status = req->status;
	struct usb_ep *in_ep;
	u32 data = 0;
	struct v4l2_event v4l2_event;
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	
	in_ep = uac2->uac_dev.in_ep;
	switch (status) {

	case 0: 			/* normal completion? */
		if (ep == in_ep){
			//f_audio_out_ep_complete(ep, req);
			//f_audio_capture_ep_complete(ep, req);
		}
		else if (uac2->set_con) {
			memcpy(&data, req->buf, req->length);
			if(req->length == 4) {
				uac2->set_con->set(uac2->set_con, uac2->set_cmd,
					le32_to_cpu(data));
			}
			else {
				uac2->set_con->set(uac2->set_con, uac2->set_cmd,
					le16_to_cpu(data));
			}
			
			uac2->set_con = NULL;
			
			if(uac2->event_type == -1)
				break;
			v4l2_event.type = uac2->event_type;
			memcpy(&v4l2_event.u.data, req->buf, req->length);
			data = ((uint16_t *)&v4l2_event.u.data)[0];
			v4l2_event_queue(uac2->uac_dev.vdev, &v4l2_event);
		}
		break;
	default:
		break;
	}
}

static int uac2_control_event(u8 id, u8 cs) {
	int type = -1;
	if(id == FEATURE_UNIT_MIC_ID){
	 	if(cs == 0x01){
			type = UAC_MIC_MUTE;
		}else{
			type = UAC_MIC_VOLUME;
		}
	}else if(id == FEATURE_UNIT_SPK_ID){
		if(cs == 0x01){
			type = UAC_SPEAK_MUTE;
		}else{
			type = UAC_SPEAK_VOLUME;
		}
	}
	else if(id == CLOCK_SOURCE_MIC_ID) {
		if(cs == UAC2_CS_CONTROL_SAM_FREQ){
			type = UAC_MIC_SAMPLERATE;
		}
	}
	else if(id == CLOCK_SOURCE_SPK_ID) {
		if(cs == UAC2_CS_CONTROL_SAM_FREQ){
			type = UAC_SPEAK_SAMPLERATE;
		}
	}
	return type;
}

static int
uac2_audio_set_intf_req(struct usb_function *fn, const struct usb_ctrlrequest *ctrl)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);
    struct usb_request *req = fn->config->cdev->req;
    u8 entity_id = (le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF;
    u8 con_sel   = (le16_to_cpu(ctrl->wValue) >> 8) & 0xFF;
    u8 cmd       = ctrl->bRequest & 0x0F;
    struct usb_audio_control_selector *cs;
    struct usb_audio_control *con = NULL;

    list_for_each_entry(cs, &uac2->cs, list) {
        if (cs->id != entity_id)
            continue;
        list_for_each_entry(con, &cs->control, list) {
            if (con->type == con_sel && con->set) {
                uac2->set_con = con;
                break;
            }
        }
        break;
    }

    if (!uac2->set_con)
        return -EOPNOTSUPP;

    uac2->set_cmd = cmd;
    uac2->event_type = uac2_control_event(entity_id, con_sel);
    req->context = uac2;

    req->complete = f_uac2_complete;

    return le16_to_cpu(ctrl->wLength);
}

static int
in_rq_range(struct usb_function *fn, const struct usb_ctrlrequest *cr)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);
	struct usb_request *req = fn->config->cdev->req;
	u16 w_length = le16_to_cpu(cr->wLength);
	u16 w_index = le16_to_cpu(cr->wIndex);
	u16 w_value = le16_to_cpu(cr->wValue);
	u8 entity_id = (w_index >> 8) & 0xff;
	u8 control_selector = w_value >> 8;
	int value = -EOPNOTSUPP;
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	printk(KERN_INFO "uac2 in_rq_range: entity=%u control=%u len=%u\n",
	       entity_id, control_selector, w_length);

	if ((entity_id == CLOCK_SOURCE_MIC_ID) || (entity_id == CLOCK_SOURCE_SPK_ID)) {
		if (control_selector == UAC2_CS_CONTROL_SAM_FREQ) {
			struct cntrl_ranges_lay3_srates rs;

        	struct uac2_clock_control *clk = NULL;
			int i;
			s32 max_sr = 0;
			s32 min_sr = 0;
			s32 res_sr = 0;
			list_for_each_entry(cs, &uac2->cs, list) {
				if (cs->id == entity_id){
					list_for_each_entry(con, &cs->control, list) {
						if (con->type == UAC2_CS_CONTROL_SAM_FREQ) {
							clk = container_of(con, struct uac2_clock_control, ctrl);
							min_sr = con->get(con, UAC__MIN);
							max_sr = con->get(con, UAC__MAX);
							res_sr = con->get(con, UAC__RES);
							break;
						}
					}
            		break;
				}
        	}
			if (!clk || !clk->srates_cnt) {
				printk(KERN_ERR "uac2: GET_RANGE SAM_FREQ failed: clk=%p srates_cnt=%u\n",
				       clk, clk ? clk->srates_cnt : 0);
				return -EOPNOTSUPP;
			}

			memset(&rs, 0, sizeof(rs));
			rs.wNumSubRanges = cpu_to_le16(clk->srates_cnt);
			
			for (i = 0; i < clk->srates_cnt; i++) {
				u32 rate = clk->srates[i];

				rs.r[i].dMIN = cpu_to_le32(rate);
				rs.r[i].dMAX = cpu_to_le32(rate);
				rs.r[i].dRES = 0;
				printk(KERN_INFO "uac2: GET_RANGE SAM_FREQ[%d] = %u\n", i, rate);
			}

			value = min_t(unsigned int, w_length, ranges_lay3_size(rs));
			printk(KERN_INFO "uac2: GET_RANGE SAM_FREQ returning %d bytes, wNumSubRanges=%u\n",
			       value, clk->srates_cnt);
			memcpy(req->buf, &rs, value);
		} else {
			printk(
				"%s:%d control_selector=%d TODO!\n",
				__func__, __LINE__, control_selector);
		}
	} else if (entity_id == FEATURE_UNIT_MIC_ID || entity_id == FEATURE_UNIT_SPK_ID) {
		if (control_selector == UAC_FU_VOLUME) {
			struct cntrl_range_lay2 r;
			s16 max_db = 0;
			s16 min_db = 0; 
			s16 res_db = 0;
			int found = 0;
			list_for_each_entry(cs, &uac2->cs, list) {
				if (cs->id == entity_id) {
					list_for_each_entry(con, &cs->control, list) {
						if(con->type == control_selector){
							max_db = con->get(con, UAC__MAX);
							min_db = con->get(con, UAC__MIN);
							res_db = con->get(con, UAC__RES);
							found = 1;
							break;
						}
					}
					break;
				}
			}
			if (!found) {
				printk(KERN_ERR "uac2: GET_RANGE VOLUME entity=%u not found!\n", entity_id);
			}
			r.wMAX = cpu_to_le16(max_db);
			r.wMIN = cpu_to_le16(min_db);
			r.wRES = cpu_to_le16(res_db);
			r.wNumSubRanges = cpu_to_le16(1);
			value = min_t(unsigned int, w_length, sizeof(r));
			printk(KERN_INFO "uac2: GET_RANGE VOLUME entity=%u min=%d max=%d res=%d returning %d bytes\n",
			       entity_id, min_db, max_db, res_db, value);
			memcpy(req->buf, &r, value);
		} else {
			printk(
				"%s:%d control_selector=%d TODO!\n",
				__func__, __LINE__, control_selector);
		}
	} else {
		printk(
			"%s:%d entity_id=%d control_selector=%d TODO!\n",
			__func__, __LINE__, entity_id, control_selector);
	}
	if(value > 0) {
		req->context = uac2;
		req->complete = f_uac2_complete;
	}
	return value;
}

static int
in_rq_cur(struct usb_function *fn, const struct usb_ctrlrequest *cr)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);
	struct usb_request *req = fn->config->cdev->req;
	u16 w_length = le16_to_cpu(cr->wLength);
	u16 w_index = le16_to_cpu(cr->wIndex);
	u16 w_value = le16_to_cpu(cr->wValue);
	u8 entity_id = (w_index >> 8) & 0xff;
	u8 control_selector = w_value >> 8;
	u16 len = 0;
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con = NULL;

	printk(KERN_INFO "uac2 in_rq_cur: entity=%u control=%u len=%u\n",
	       entity_id, control_selector, w_length);

	/* Search for the control selector in entity's control list */
	list_for_each_entry(cs, &uac2->cs, list) {
		if (cs->id == entity_id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == control_selector && con->get) {
					goto found;
				}
			}
			con = NULL;
			break;
		}
	}
	con = NULL;

found:
	if (!con || !con->get) {
		printk(KERN_ERR "uac2 in_rq_cur: no handler for entity=%u control=%u\n",
		       entity_id, control_selector);
		return -EOPNOTSUPP;
	}

	if (cs->type == UAC_FEATURE_UNIT && con->type == UAC_FU_MUTE) {
		u8 mute = con->get(con, UAC__CUR);
		len = min_t(u16, w_length, sizeof(mute));
		memcpy(req->buf, &mute, len);
	} else if (cs->type == UAC_FEATURE_UNIT && con->type == UAC_FU_VOLUME) {
		struct cntrl_cur_lay2 cur = {
			.wCUR = cpu_to_le16(con->get(con, UAC__CUR)),
		};
		len = min_t(u16, w_length, sizeof(cur));
		memcpy(req->buf, &cur, len);
	} else if (cs->type == UAC2_CLOCK_SOURCE && con->type == UAC2_CS_CONTROL_SAM_FREQ) {
		u32 rate = cpu_to_le32(con->get(con, UAC__CUR));
		len = min_t(u16, w_length, sizeof(rate));
		memcpy(req->buf, &rate, len);
	}
	else {
		return -EOPNOTSUPP;
	}

	if (len) {
		req->context = uac2;
		req->complete = f_uac2_complete;
	}

	return len;
}

static int
uac2_audio_get_intf_req(struct usb_function *fn, const struct usb_ctrlrequest *ctrl)
{
	/* GET_CUR */
	if(ctrl->bRequest ==  UAC2_CS_CUR) {
		return in_rq_cur(fn, ctrl);
	}
	/* GET_RANGE */
	else if (ctrl->bRequest == UAC2_CS_RANGE) {
		return in_rq_range(fn, ctrl);
	}
	else {
		return -EOPNOTSUPP;
	}
}

static int
setup_rq_inf(struct usb_function *fn, const struct usb_ctrlrequest *cr)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);
	u16 w_index = le16_to_cpu(cr->wIndex);
	u8 intf = w_index & 0xff;

	if (intf != uac2->ac_intf) {
		printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
		return -EOPNOTSUPP;
	}

	if (cr->bRequestType & USB_DIR_IN)
		return uac2_audio_get_intf_req(fn, cr);
	else if (cr->bRequest == UAC2_CS_CUR)
		return uac2_audio_set_intf_req(fn, cr);
	return -EOPNOTSUPP;
}

static int
afunc_setup(struct usb_function *fn, const struct usb_ctrlrequest *cr)
{
	struct usb_composite_dev *cdev = fn->config->cdev;
	struct usb_request *req = cdev->req;
	u16 w_length = le16_to_cpu(cr->wLength);
	int value = -EOPNOTSUPP;

	/* Only Class specific requests are supposed to reach here */
	if ((cr->bRequestType & USB_TYPE_MASK) != USB_TYPE_CLASS)
		return -EOPNOTSUPP;

	if ((cr->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE)
		value = setup_rq_inf(fn, cr);
	else
		printk(KERN_EMERG "audio response on err %d\n", value);

	if (value >= 0) {
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			ERROR(cdev, "audio response on err %d\n", value);
			req->status = 0;
		}
	}

	return value;
}

#if 1
struct uac_request_data
{
	__s32 length;
	__u8 data[60];
};

int uac_send_response(struct uac_device *uvc, struct uac_request_data *data)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request	*req = cdev->req;
	int ret;

	if (data->length < 0)
			return usb_ep_set_halt(cdev->gadget->ep0);

#if 0
        req->length = min_t(unsigned int, uvc->event_length, data->length);
        req->zero = data->length < uvc->event_length;
#else
	req->zero = 0;
	req->length = data->length;
	
#endif

    memcpy(req->buf, data->data, req->length);
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);

	return ret;
}
#endif

static struct usb_endpoint_descriptor *uac2_get_ep_desc(
	struct usb_gadget *gadget, bool is_in, int alt)
{
	if (is_in) {
		if (gadget->speed == USB_SPEED_SUPER) {
			switch (alt) {
			case 1: return (struct usb_endpoint_descriptor *)&ss_epin_desc;
			case 2: return (struct usb_endpoint_descriptor *)&ss_epin_desc_alt2;
			case 3: return (struct usb_endpoint_descriptor *)&ss_epin_desc_alt3;
			}
		} else if (gadget->speed == USB_SPEED_HIGH) {
			switch (alt) {
			case 1: return (struct usb_endpoint_descriptor *)&hs_epin_desc;
			case 2: return (struct usb_endpoint_descriptor *)&hs_epin_desc_alt2;
			case 3: return (struct usb_endpoint_descriptor *)&hs_epin_desc_alt3;
			}
		} else {
			switch (alt) {
			case 1: return (struct usb_endpoint_descriptor *)&fs_epin_desc;
			case 2: return (struct usb_endpoint_descriptor *)&fs_epin_desc_alt2;
			case 3: return (struct usb_endpoint_descriptor *)&fs_epin_desc_alt3;
			}
		}
	} else {
		if (gadget->speed == USB_SPEED_SUPER) {
			switch (alt) {
			case 1: return (struct usb_endpoint_descriptor *)&ss_epout_desc;
			case 2: return (struct usb_endpoint_descriptor *)&ss_epout_desc_alt2;
			case 3: return (struct usb_endpoint_descriptor *)&ss_epout_desc_alt3;
			}
		} else if (gadget->speed == USB_SPEED_HIGH) {
			switch (alt) {
			case 1: return (struct usb_endpoint_descriptor *)&hs_epout_desc;
			case 2: return (struct usb_endpoint_descriptor *)&hs_epout_desc_alt2;
			case 3: return (struct usb_endpoint_descriptor *)&hs_epout_desc_alt3;
			}
		} else {
			switch (alt) {
			case 1: return (struct usb_endpoint_descriptor *)&fs_epout_desc;
			case 2: return (struct usb_endpoint_descriptor *)&fs_epout_desc_alt2;
			case 3: return (struct usb_endpoint_descriptor *)&fs_epout_desc_alt3;
			}
		}
	}
	return NULL;
}

static int
afunc_set_alt(struct usb_function *fn, unsigned intf, unsigned alt)
{
	struct uac_device *uac_dev;
	struct v4l2_event v4l2_event;
	struct f_uac2 *uac2 = func_to_uac2(fn);
	int ret = 0;

	//printk(KERN_EMERG "%s:intf:%d alt=%d\n", __func__, intf, alt);

	if (intf == uac2->ac_intf) {
		/* Control I/f has only 1 AltSetting - 0 */
		if (alt) {
			printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
			return -EINVAL;
		}

		uac_device_stop_playback(&uac2->uac_dev);
		uac_device_stop_capture(&uac2->uac_dev);

		uac_dev = &uac2->uac_dev;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UAC_EVENT_CONNECT;
		v4l2_event_queue(uac_dev->vdev, &v4l2_event);

		printk(KERN_EMERG "%s: trigger connect\n", __func__);

		return 0;
	}

	if (intf == uac2->as_out_intf) {
		uac2->as_out_alt = alt;

		if (alt) {
			struct v4l2_event v4l2_event;
			struct usb_composite_dev *cdev = fn->config->cdev;
			uac2->uac_dev.out_ep->desc = uac2_get_ep_desc(cdev->gadget, false, alt);

			switch (alt) {
			case 1: uac2->uac_dev.params.c_ssize = 2; break;
			case 2: uac2->uac_dev.params.c_ssize = 3; break;
			case 3: uac2->uac_dev.params.c_ssize = 4; break;
			}

			uac_device_start_capture(&uac2->uac_dev);
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_STARTCAP;
			v4l2_event_queue(uac2->uac_dev.vdev, &v4l2_event);
		} else {
			struct v4l2_event v4l2_event;

			uac_device_stop_capture(&uac2->uac_dev);
			
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_STOPCAP;
			v4l2_event_queue(uac2->uac_dev.vdev, &v4l2_event);
		}
	} else if (intf == uac2->as_in_intf) {
		uac2->as_in_alt = alt;
		if (alt) {
			struct v4l2_event v4l2_event;
			struct usb_composite_dev *cdev = fn->config->cdev;
			uac2->uac_dev.in_ep->desc = uac2_get_ep_desc(cdev->gadget, true, alt);

			switch (alt) {
			case 1: uac2->uac_dev.params.p_ssize = 2; break;
			case 2: uac2->uac_dev.params.p_ssize = 3; break;
			case 3: uac2->uac_dev.params.p_ssize = 4; break;
			}
			
			ret = uac_device_start_playback(&uac2->uac_dev);
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_STREAMON;
			v4l2_event_queue(uac2->uac_dev.vdev, &v4l2_event);
		} else {
			struct v4l2_event v4l2_event;
			
			uac_device_stop_playback(&uac2->uac_dev);
			
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_STREAMOFF;
			v4l2_event_queue(uac2->uac_dev.vdev, &v4l2_event);
		}
	} else {
		printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}

static int
afunc_get_alt(struct usb_function *fn, unsigned intf)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);


	if (intf == uac2->ac_intf) {
		return uac2->ac_alt;
	}
	else if (intf == uac2->as_out_intf) {
		return uac2->as_out_alt;
	}
	else if (intf == uac2->as_in_intf) {
		return uac2->as_in_alt;
	}

	else
		printk(KERN_EMERG
			"%s:%d Invalid Interface %d!\n",
			__func__, __LINE__, intf);

	return -EINVAL;
}

static void
afunc_disable(struct usb_function *fn)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);

	uac2->as_in_alt = 0;
	uac2->as_out_alt = 0;
	uac_device_stop_playback(&uac2->uac_dev);
	uac_device_stop_capture(&uac2->uac_dev);

	/*FIXME: notify userpace to disconnect*/
	{
		struct uac_device *uac_dev = &uac2->uac_dev;
		struct v4l2_event v4l2_event;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UAC_EVENT_DISCONNECT;
		v4l2_event_queue(uac_dev->vdev, &v4l2_event);
	}
}

static int
__uac_register_video(struct usb_composite_dev *dev, struct uac_device *uac_dev)
{
	struct usb_composite_dev *cdev = dev;
	struct video_device *video;

	/* TODO reference counting. */
	video = video_device_alloc();
	if (video == NULL)
		return -ENOMEM;

	video->v4l2_dev = &uac_dev->v4l2_dev;
	video->fops = &uac_v4l2_fops;
	video->ioctl_ops = &uac_v4l2_ioctl_ops;
	video->release = video_device_release;
	video->vfl_dir = VFL_DIR_TX;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	video->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
#endif
	strlcpy(video->name, cdev->gadget->name, sizeof(video->name));

	uac_dev->vdev = video;
	video_set_drvdata(video, uac_dev);

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
	return video_register_device(video, VFL_TYPE_VIDEO, -1);
#else
	return video_register_device(video, VFL_TYPE_GRABBER, -1);
#endif
}


/* audio function driver setup/binding */
static int
afunc_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev	*cdev = c->cdev;
	struct usb_gadget		*gadget = cdev->gadget;
	struct f_uac2			*uac2 = func_to_uac2(f);
	struct uac_device *uac_dev = &uac2->uac_dev;
	struct f_uac2_opts		*audio_opts;
	struct usb_ep			*ep = NULL;
	struct usb_string		*us;
	int				status;

	io_out_ot_desc.wTerminalType = speak_uac_type;
	io_in_it_desc.wTerminalType = mic_uac_type;
	io_in_it_desc.bAssocTerminal = 0;
	io_out_ot_desc.bAssocTerminal = 0;

	memset(uac_name, 0, sizeof(uac_name));

	if (uac_name_label1 != NULL && uac_name_label1[0] != '\0')
	{
	    strcat(uac_name, uac_name_label1);
	}
	
	if (uac_name_label2 != NULL && uac_name_label2[0] != '\0')
	{
	    strcat(uac_name, " ");
	    strcat(uac_name, uac_name_label2);
	}

	if (uac_name_label3 != NULL && uac_name_label3[0] != '\0')
	{
	    strcat(uac_name, " ");
	    strcat(uac_name, uac_name_label3);
	}

	if (uac_name_label4 != NULL && uac_name_label4[0] != '\0')
	{
	    strcat(uac_name, " ");
	    strcat(uac_name, uac_name_label4);
	}

	if (uac_name_label5 != NULL && uac_name_label5[0] != '\0')
	{
	    strcat(uac_name, " ");
	    strcat(uac_name, uac_name_label5);
	}


	memset(mic_name, 0, sizeof(mic_name));
	if (mic_name_label1 != NULL && mic_name_label1[0] != '\0')
	{
	    strcat(mic_name, mic_name_label1);
	}
	
	if (mic_name_label2 != NULL && mic_name_label2[0] != '\0')
	{
	    strcat(mic_name, " ");
	    strcat(mic_name, mic_name_label2);
	}

	if (mic_name_label3 != NULL && mic_name_label3[0] != '\0')
	{
	    strcat(mic_name, " ");
	    strcat(mic_name, mic_name_label3);
	}

	if (mic_name_label4 != NULL && mic_name_label4[0] != '\0')
	{
	    strcat(mic_name, " ");
	    strcat(mic_name, mic_name_label4);
	}

	if (mic_name_label5 != NULL && mic_name_label5[0] != '\0')
	{
	    strcat(mic_name, " ");
	    strcat(mic_name, mic_name_label5);
	}

	memset(speak_name, 0, sizeof(speak_name));
	if (speak_name_label1 != NULL && speak_name_label1[0] != '\0')
	{
	    strcat(speak_name, speak_name_label1);
	}
	
	if (speak_name_label2 != NULL && speak_name_label2[0] != '\0')
	{
	    strcat(speak_name, " ");
	    strcat(speak_name, speak_name_label2);
	}

	if (speak_name_label3 != NULL && speak_name_label3[0] != '\0')
	{
	    strcat(speak_name, " ");
	    strcat(speak_name, speak_name_label3);
	}

	if (speak_name_label4 != NULL && speak_name_label4[0] != '\0')
	{
	    strcat(speak_name, " ");
	    strcat(speak_name, speak_name_label4);
	}

	if (speak_name_label5 != NULL && speak_name_label5[0] != '\0')
	{
	    strcat(speak_name, " ");
	    strcat(speak_name, speak_name_label5);
	}

	audio_opts = container_of(f->fi, struct f_uac2_opts, func_inst);

	us = usb_gstrings_attach(cdev, fn_strings, ARRAY_SIZE(strings_fn));
	if (IS_ERR(us))
		return PTR_ERR(us);

	if(uac_name[0] != 0)
	{
		uac_iad.iFunction = us[STR_AC_IF].id;
		ac_interface_desc.iInterface = us[STR_AC_IF].id;
	}

	if(mic_name[0] != 0)
	{
		// feature_unit_mic_desc.iFeature = us[STR_MIC_IF].id;
		io_in_it_desc.iTerminal = us[STR_MIC_IF].id;
		io_in_it_desc.iChannelNames = us[STR_MIC_IF].id;
		usb_in_ot_desc.iTerminal = us[STR_MIC_IF].id;
		as_in_interface_alt_0_desc.iInterface = us[STR_MIC_IF].id;
		as_in_interface_alt_1_desc.iInterface = us[STR_MIC_IF].id;
		as_in_interface_alt_2_desc.iInterface = us[STR_MIC_IF].id;
		as_in_interface_alt_3_desc.iInterface = us[STR_MIC_IF].id;
	}

	if(speak_name[0] != 0)
	{
		// feature_unit_spk_desc.iFeature = us[STR_SPEAK_IF].id;
		usb_out_it_desc.iTerminal = us[STR_SPEAK_IF].id;
		usb_out_it_desc.iChannelNames = us[STR_SPEAK_IF].id;
		io_out_ot_desc.iTerminal = us[STR_SPEAK_IF].id;
		as_out_interface_alt_0_desc.iInterface = us[STR_SPEAK_IF].id;
		as_out_interface_alt_1_desc.iInterface = us[STR_SPEAK_IF].id;
		as_out_interface_alt_2_desc.iInterface = us[STR_SPEAK_IF].id;
		as_out_interface_alt_3_desc.iInterface = us[STR_SPEAK_IF].id;
	}

	/* Initialize the configurable parameters */
	usb_out_it_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	usb_out_it_desc.bmChannelConfig = cpu_to_le32(audio_opts->c_chmask);
	io_in_it_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	io_in_it_desc.bmChannelConfig = cpu_to_le32(audio_opts->p_chmask);
	as_out_hdr_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	as_out_hdr_desc.bmChannelConfig = cpu_to_le32(audio_opts->c_chmask);
	as_in_hdr_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	as_in_hdr_desc.bmChannelConfig = cpu_to_le32(audio_opts->p_chmask);
	//as_out_type_i_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	as_out_type_i_desc.bSubslotSize = audio_opts->c_ssize;
	as_out_type_i_desc.bBitResolution = audio_opts->c_ssize * 8;
	as_out_type_i_desc_alt2.bSubslotSize = audio_opts->c_ssize + 1;
	as_out_type_i_desc_alt2.bBitResolution = (audio_opts->c_ssize + 1) * 8;
	as_out_type_i_desc_alt3.bSubslotSize = audio_opts->c_ssize + 2;
	as_out_type_i_desc_alt3.bBitResolution = (audio_opts->c_ssize + 2) * 8;
	//as_in_type_i_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	as_in_type_i_desc.bSubslotSize = audio_opts->p_ssize;
	as_in_type_i_desc.bBitResolution = audio_opts->p_ssize * 8;
	as_in_type_i_desc_alt2.bSubslotSize = audio_opts->p_ssize + 1;
	as_in_type_i_desc_alt2.bBitResolution = (audio_opts->p_ssize + 1) * 8;
	as_in_type_i_desc_alt3.bSubslotSize = audio_opts->p_ssize + 2;
	as_in_type_i_desc_alt3.bBitResolution = (audio_opts->p_ssize + 2) * 8;

	ac_header_desc.wTotalLength = cpu_to_le16(
		UAC2_DT_AC_HEADER_SIZE + 
		2*UAC2_DT_CLK_SOURCE_SIZE +
		2*UAC2_DT_INPUT_TERMINAL_SIZE +
		2*UAC2_DT_OUTPUT_TERMINAL_SIZE +
		feature_unit_mic_desc.bLength +
		feature_unit_spk_desc.bLength
	);

	//divce to host
	ss_epin_desc.wMaxPacketSize = cpu_to_le16(num_channels(audio_opts->p_chmask) * audio_opts->p_ssize * ((96000/1000) + 1));
	hs_epin_desc.wMaxPacketSize = ss_epin_desc.wMaxPacketSize;
	fs_epin_desc.wMaxPacketSize = ss_epin_desc.wMaxPacketSize;

	ss_epin_desc_alt2.wMaxPacketSize = cpu_to_le16(num_channels(audio_opts->p_chmask) * (audio_opts->p_ssize + 1) * ((96000/1000) + 1));
	hs_epin_desc_alt2.wMaxPacketSize = ss_epin_desc_alt2.wMaxPacketSize;
	fs_epin_desc_alt2.wMaxPacketSize = ss_epin_desc_alt2.wMaxPacketSize;

	ss_epin_desc_alt3.wMaxPacketSize = cpu_to_le16(num_channels(audio_opts->p_chmask) * (audio_opts->p_ssize + 2) * ((96000/1000) + 1));
	hs_epin_desc_alt3.wMaxPacketSize = ss_epin_desc_alt3.wMaxPacketSize;
	fs_epin_desc_alt3.wMaxPacketSize = ss_epin_desc_alt3.wMaxPacketSize;

	//host to divce
	ss_epout_desc.wMaxPacketSize = cpu_to_le16(num_channels(audio_opts->c_chmask) * audio_opts->c_ssize * ((96000/1000) + 1));
	hs_epout_desc.wMaxPacketSize = ss_epout_desc.wMaxPacketSize;
	fs_epout_desc.wMaxPacketSize = ss_epout_desc.wMaxPacketSize;

	ss_epout_desc_alt2.wMaxPacketSize = cpu_to_le16(num_channels(audio_opts->c_chmask) * (audio_opts->c_ssize + 1) * ((96000/1000) + 1));
	hs_epout_desc_alt2.wMaxPacketSize = ss_epout_desc_alt2.wMaxPacketSize;
	fs_epout_desc_alt2.wMaxPacketSize = ss_epout_desc_alt2.wMaxPacketSize;

	ss_epout_desc_alt3.wMaxPacketSize = cpu_to_le16(num_channels(audio_opts->c_chmask) * (audio_opts->c_ssize + 2) * ((96000/1000) + 1));
	hs_epout_desc_alt3.wMaxPacketSize = ss_epout_desc_alt3.wMaxPacketSize;
	fs_epout_desc_alt3.wMaxPacketSize = ss_epout_desc_alt3.wMaxPacketSize;

	ss_epin_desc_comp.wBytesPerInterval = ss_epin_desc.wMaxPacketSize;
	ss_epout_desc_comp.wBytesPerInterval = ss_epout_desc.wMaxPacketSize;

	ss_epin_desc_comp_alt2.wBytesPerInterval = ss_epin_desc_alt2.wMaxPacketSize;
	ss_epout_desc_comp_alt2.wBytesPerInterval = ss_epout_desc_alt2.wMaxPacketSize;

	ss_epin_desc_comp_alt3.wBytesPerInterval = ss_epin_desc_alt3.wMaxPacketSize;
	ss_epout_desc_comp_alt3.wBytesPerInterval = ss_epout_desc_alt3.wMaxPacketSize;
	
	/* Set sample rates */

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0) {
		printk(KERN_EMERG "%s:%d\n", __func__, __LINE__);
		goto fail;
	}

	uac_iad.bFirstInterface = status;
	ac_interface_desc.bInterfaceNumber = status;
	uac2->ac_intf = status;
	uac2->ac_alt = 0;

	printk(KERN_EMERG "ac_intf:%d\n", status);
	
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	as_out_interface_alt_0_desc.bInterfaceNumber = status;
	as_out_interface_alt_1_desc.bInterfaceNumber = status;
	as_out_interface_alt_2_desc.bInterfaceNumber = status;
	as_out_interface_alt_3_desc.bInterfaceNumber = status;
	uac2->as_out_intf = status;
	uac2->as_out_alt = 0;

	status = usb_interface_id(c, f);
	if (status < 0) {
		printk(KERN_EMERG "%s:%d\n", __func__, __LINE__);
		goto fail;
	}

	as_in_interface_alt_0_desc.bInterfaceNumber = status;
	as_in_interface_alt_1_desc.bInterfaceNumber = status;
	as_in_interface_alt_2_desc.bInterfaceNumber = status;
	as_in_interface_alt_3_desc.bInterfaceNumber = status;

	uac2->as_in_intf = status;
	uac2->as_in_alt = 0;

	printk(KERN_EMERG "as_in_intf:%d\n", status);

	uac_dev->gadget = gadget;
	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &ss_epout_desc_alt3);
	if (!ep) {
		printk(KERN_EMERG "%s:%d\n", __func__,__LINE__);
		goto fail;
	}

	uac_dev->out_ep = ep;
	uac_dev->out_ep->desc = &ss_epout_desc;
	ep->driver_data = uac_dev;
	hs_epout_desc.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;
	fs_epout_desc.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;
	ss_epout_desc.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;

	ss_epout_desc_alt2.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;
	hs_epout_desc_alt2.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;
	fs_epout_desc_alt2.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;

	hs_epout_desc_alt3.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;
	fs_epout_desc_alt3.bEndpointAddress = ss_epout_desc_alt3.bEndpointAddress;
	
	
	ep = usb_ep_autoconfig(cdev->gadget, &ss_epin_desc_alt3);
	if (!ep) {
		printk(KERN_EMERG "%s:%d\n", __func__,__LINE__);
		goto fail;
	}
	uac_dev->in_ep = ep;
	uac_dev->in_ep->desc = &ss_epin_desc;
	ep->driver_data = uac_dev;
	hs_epin_desc.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;
	fs_epin_desc.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;
	ss_epin_desc.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;

	ss_epin_desc_alt2.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;
	hs_epin_desc_alt2.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;
	fs_epin_desc_alt2.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;

	hs_epin_desc_alt3.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;
	fs_epin_desc_alt3.bEndpointAddress = ss_epin_desc_alt3.bEndpointAddress;
	
	ss_epout_desc_comp.wBytesPerInterval = le16_to_cpu(ss_epout_desc.wMaxPacketSize);
	ss_epin_desc_comp.wBytesPerInterval = le16_to_cpu(ss_epin_desc.wMaxPacketSize);

	ss_epout_desc_comp_alt2.wBytesPerInterval = le16_to_cpu(ss_epout_desc_alt2.wMaxPacketSize);
	ss_epin_desc_comp_alt2.wBytesPerInterval = le16_to_cpu(ss_epin_desc_alt2.wMaxPacketSize);

	ss_epout_desc_comp_alt3.wBytesPerInterval = le16_to_cpu(ss_epout_desc_alt3.wMaxPacketSize);
	ss_epin_desc_comp_alt3.wBytesPerInterval = le16_to_cpu(ss_epin_desc_alt3.wMaxPacketSize);

	/* copy descriptors, and track endpoint copies */
	status = usb_assign_descriptors(f, fs_audio_desc, hs_audio_desc, ss_audio_desc,
				     ss_audio_desc);
	if (status) {
		printk(KERN_EMERG "%s:%d\n", __func__,__LINE__);
		goto fail;
	}

	uac_dev->out_ep_maxpsize = le16_to_cpu(ss_epout_desc_alt3.wMaxPacketSize);
	uac_dev->in_ep_maxpsize = le16_to_cpu(ss_epin_desc_alt3.wMaxPacketSize);
	uac_dev->params.c_chmask = audio_opts->c_chmask;
	uac_dev->params.c_srates[0] = 48000; // Default to 48k
	uac_dev->params.c_ssize = audio_opts->c_ssize;
	uac_dev->params.p_chmask = audio_opts->p_chmask;
	uac_dev->params.p_srates[0] = 48000; // Default to 48k
	uac_dev->params.p_ssize = audio_opts->p_ssize;
	uac_dev->params.req_number = audio_opts->req_number;
	
	printk(KERN_EMERG "*******************pamrams: p_chmask=%d p_srate=%d(hz) p_ssize=%d \
			req_number=%d uac_dev->out_ep_maxpsize:%d\n",
			audio_opts->p_chmask,
			audio_opts->p_srates[0],
			audio_opts->p_ssize,
			audio_opts->req_number,
			uac_dev->out_ep_maxpsize
		);


	status = uac_device_setup(uac_dev);
	if (status){
		printk(KERN_EMERG "%s %d\n",__FUNCTION__,__LINE__);
		goto err_card_register;
	}
	/*FIXME: add v4l2 interface*/
	if (uac_dev) {
		if (v4l2_device_register(&cdev->gadget->dev, &(uac_dev->v4l2_dev))) {
			printk(KERN_EMERG "v4l2_device_register failed\n");
			goto error;
		}

		/* Initialise queue buffer. */
		status = uac_queue_init(&(uac_dev->queue));
		if (status < 0){
			printk(KERN_EMERG "%s %d\n",__FUNCTION__,__LINE__);
			goto error;
		}
		/* Register a V4L2 device. */
		status = __uac_register_video(cdev, uac_dev);
		if (status < 0) {
			printk(KERN_EMERG "Unable to register video device\n");
			goto error;
		}
	}

	printk(KERN_EMERG "f_audio_bind OK!!!\n");
	return 0;

error:
	if (uac_dev) {
		v4l2_device_unregister(&(uac2->uac_dev.v4l2_dev));
		if ((uac2->uac_dev.vdev))
			video_device_release((uac2->uac_dev.vdev));
	}

err_card_register:
	usb_free_all_descriptors(f);
fail:
	return status;
}

static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value)
{
	con->data[cmd] = value;

	return 0;
}

static int generic_get_cmd(struct usb_audio_control *con, u8 cmd)
{
	return con->data[cmd];
}

static int sam_rate_set_cmd(struct usb_audio_control *con, u8 cmd, int value)
{
    
	struct uac2_clock_control *clk_ctrl = container_of(con, struct uac2_clock_control, ctrl);
	struct f_uac2 *uac2 = clk_ctrl->parent;

    if (cmd != UAC2_CS_CONTROL_SAM_FREQ)
        return -EINVAL;
    if (clk_ctrl->clock_id == CLOCK_SOURCE_SPK_ID)
        uac2_set_capture_srate(&uac2->uac_dev, value);
    else
        uac2_set_playback_srate(&uac2->uac_dev, value);
	
    con->data[UAC__CUR] = value;
    return 0;
}

static int sam_rate_get_cmd(struct usb_audio_control *con, u8 cmd)
{
    return con->data[cmd];
}

static unsigned int count_valid_rates(const u32 *srates)
{
    unsigned int i;

    if (!srates)
        return 0;

    for (i = 0; i < UAC_MAX_RATES; i++) {
        if (!srates[i])
            break;
    }
    return i;
}


/* Todo: add more control selecotor dynamically */
static int control_selector_init(struct f_uac2 *uac2, struct usb_function_instance *fi)
{
	struct f_uac2_opts *opts = container_of(fi,
                        struct f_uac2_opts, func_inst);

	INIT_LIST_HEAD(&uac2->cs);
	list_add(&feature_unit_mic.list, &uac2->cs);
	INIT_LIST_HEAD(&feature_unit_mic.control);
	list_add(&mic_mute_control.list, &feature_unit_mic.control);
	list_add(&mic_volume_control.list, &feature_unit_mic.control);
	
	list_add(&feature_unit_spk.list, &uac2->cs);
	INIT_LIST_HEAD(&feature_unit_spk.control);
	list_add(&spk_mute_control.list, &feature_unit_spk.control);
	list_add(&spk_volume_control.list, &feature_unit_spk.control);

	/* add clock source control */
	list_add(&mic_clk_src.list, &uac2->cs);
	INIT_LIST_HEAD(&mic_clk_src.control);
	list_add(&mic_clock_control.ctrl.list, &mic_clk_src.control);

	list_add(&spk_clk_src.list, &uac2->cs);
	INIT_LIST_HEAD(&spk_clk_src.control);
	list_add(&spk_clock_control.ctrl.list, &spk_clk_src.control);

	mic_volume_control.data[UAC__CUR] = mic_volume_def;
	mic_volume_control.data[UAC__MIN] = mic_volume_min;
	mic_volume_control.data[UAC__MAX] = mic_volume_max;
	mic_volume_control.data[UAC__RES] = mic_volume_res;
	mic_mute_control.data[UAC__CUR] = 0x0;

	spk_volume_control.data[UAC__CUR] = speak_volume_def; 
	spk_volume_control.data[UAC__MIN] = speak_volume_min; 
	spk_volume_control.data[UAC__MAX] = speak_volume_max; 
	spk_volume_control.data[UAC__RES] = speak_volume_res;
	spk_mute_control.data[UAC__CUR] = 0x0;

	mic_clock_control.parent = uac2;
	mic_clock_control.srates = opts->p_srates;
	opts->p_srates[0] = 16000;
	opts->p_srates[1] = 32000;
	opts->p_srates[2] = 44100;
	opts->p_srates[3] = 48000;
	opts->p_srates[4] = 96000;
	opts->p_srates[5] = 0;

	mic_clock_control.srates_cnt = count_valid_rates(opts->p_srates);
	mic_clock_control.ctrl.data[UAC__CUR] = 48000;
	mic_clock_control.ctrl.data[UAC__MIN] = mic_srate_min;
	mic_clock_control.ctrl.data[UAC__MAX] = mic_srate_max;
	mic_clock_control.ctrl.data[UAC__RES] = mic_srate_res;

	spk_clock_control.parent = uac2;
	spk_clock_control.srates = opts->c_srates;
	opts->c_srates[0] = 16000;
	opts->c_srates[1] = 32000;
	opts->c_srates[2] = 44100;
	opts->c_srates[3] = 48000;
	opts->c_srates[4] = 96000;
	opts->c_srates[5] = 0;

	spk_clock_control.srates_cnt = count_valid_rates(opts->c_srates);
	spk_clock_control.ctrl.data[UAC__CUR] = 48000;
	spk_clock_control.ctrl.data[UAC__MIN] = speak_srate_min;
	spk_clock_control.ctrl.data[UAC__MAX] = speak_srate_max;
	spk_clock_control.ctrl.data[UAC__RES] = speak_srate_res;

	uac2->event_type = 0;
	return 0;
}

/*-------------------------------------------------------------------------*/

static inline struct f_uac2_opts *to_f_uac2_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_uac2_opts,
			    func_inst.group);
}

static void afunc_free_inst(struct usb_function_instance *f)
{
	struct f_uac2_opts *opts;

	opts = container_of(f, struct f_uac2_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *afunc_alloc_inst(void)
{
	struct f_uac2_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = afunc_free_inst;

	/*	config_group_init_type_name(&opts->func_inst.group, "",
				    &f_uac2_func_type); */

	opts->p_chmask = UAC2_DEF_PCHMASK;
	opts->p_srates[0] = UAC2_DEF_PSRATE;
	opts->p_ssize = UAC2_DEF_PSSIZE;
	opts->c_chmask = UAC2_DEF_CCHMASK;
	opts->c_srates[0] = UAC2_DEF_CSRATE;
	opts->c_ssize = UAC2_DEF_CSSIZE;
	opts->req_number = UAC2_DEF_REQ_NUM;

	return &opts->func_inst;
}

static void afunc_free(struct usb_function *f)
{
	struct f_uac2_opts *opts;
	struct f_uac2  *uac2 = func_to_uac2(f);

	opts = container_of(f->fi, struct f_uac2_opts, func_inst);
	mutex_lock(&opts->lock);
	--opts->refcnt;
	mutex_unlock(&opts->lock);

	kfree(uac2);
}

static void afunc_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_uac2  *uac2 = func_to_uac2(f);
	struct uac_device *uac_dev = &uac2->uac_dev;

	uac_device_cleanup(uac_dev);
	usb_free_all_descriptors(f);

	uac_dev->gadget = NULL;

	if (uac_dev) { /* release v4l2 device*/
		video_unregister_device(uac_dev->vdev);
		v4l2_device_unregister(&(uac_dev->v4l2_dev));
	}
}

static void
afunc_suspend(struct usb_function *fn)
{
	struct f_uac2 *uac2 = func_to_uac2(fn);

	// u_audio_suspend(&uac2->g_audio);

	struct uac_device *uac_dev = &uac2->uac_dev;
	struct v4l2_event v4l2_event;

	uac2->as_out_alt = 0;
	uac2->as_in_alt = 0;
	uac_device_stop_playback(&uac2->uac_dev);
	uac_device_stop_capture(&uac2->uac_dev);
	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UAC_EVENT_SUSPEND;
	v4l2_event_queue(uac_dev->vdev, &v4l2_event);
}

static void afunc_resume(struct usb_function *f)
{
	struct f_uac2 *uac2 = func_to_uac2(f);
	struct uac_device *uac_dev = &uac2->uac_dev;
	struct v4l2_event v4l2_event;

	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UAC_EVENT_RESUME;
	v4l2_event_queue(uac_dev->vdev, &v4l2_event);
}

static struct usb_function *afunc_alloc(struct usb_function_instance *fi)
{
	struct f_uac2	*uac2;
	struct f_uac2_opts *opts;

	uac2 = kzalloc(sizeof(*uac2), GFP_KERNEL);
	if (uac2 == NULL)
		return ERR_PTR(-ENOMEM);

	opts = container_of(fi, struct f_uac2_opts, func_inst);
	mutex_lock(&opts->lock);
	++opts->refcnt;
	mutex_unlock(&opts->lock);

	uac2->uac_dev.func.name = "uac2";
	uac2->uac_dev.func.bind = afunc_bind;
	uac2->uac_dev.func.unbind = afunc_unbind;
	uac2->uac_dev.func.set_alt = afunc_set_alt;
	uac2->uac_dev.func.get_alt = afunc_get_alt;
	uac2->uac_dev.func.setup = afunc_setup;
	uac2->uac_dev.func.disable = afunc_disable;
	uac2->uac_dev.func.free_func = afunc_free;
	uac2->uac_dev.func.suspend = afunc_suspend;
	uac2->uac_dev.func.resume = afunc_resume;
	uac2->uac_dev.alsa_buf_num = alsa_buf_num;
	control_selector_init(uac2, fi);
	return &uac2->uac_dev.func;
}

DECLARE_USB_FUNCTION_INIT(uac2, afunc_alloc_inst, afunc_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yadwinder Singh");
MODULE_AUTHOR("Jaswinder Singh");
MODULE_AUTHOR("Ruslan Bilovol");
