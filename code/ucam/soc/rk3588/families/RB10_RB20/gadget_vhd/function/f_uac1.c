/*
 * f_uac1.c -- USB Audio Class 1.0 Function (using u_audio API)
 *
 * Copyright (C) 2016 Ruslan Bilovol <ruslan.bilovol@gmail.com>
 *
 * This driver doesn't expect any real Audio codec to be present
 * on the device - the audio streams are simply sinked to and
 * sourced from a virtual ALSA sound card created.
 *
 * This file is based on f_uac1.c which is
 *   Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 *   Copyright (C) 2008 Analog Devices, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/usb/audio.h>
#include <linux/module.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <linux/version.h>

#include "./function/uac_ex.h"
#include "./function/uac_v4l2_ex.h"
#include "./function/u_uac1.h"
#include "uvc_config.h"

//#include "uvc.h"
	
static int audio_freq = 16000;
module_param(audio_freq, int, S_IRUGO);
MODULE_PARM_DESC(audio_freq, "Audio buffer size");

static int mic_volume_def = 0x2A00;//0x25be;
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

int get_uac_test_mode(void)
{
	return test_mode;
}


struct f_uac1 {
	struct list_head cs;
	struct uac_device uac_dev;
	u8 ac_intf, as_in_intf, as_out_intf;
	u8 ac_alt, as_in_alt, as_out_alt;	/* needed for get_alt() */
	u8 set_cmd;
	struct usb_audio_control *set_con;
	u32 event_type;
};
static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value);
static int generic_get_cmd(struct usb_audio_control *con, u8 cmd);

static inline struct f_uac1 *func_to_uac1(struct usb_function *f)
{
	return container_of(f, struct f_uac1, uac_dev.func);
}

/*
 * DESCRIPTORS ... most are static, but strings and full
 * configuration descriptors are built on demand.
 */

/*
 * We have three interfaces - one AudioControl and two AudioStreaming
 *
 * The driver implements a simple UAC_1 topology.
 * USB-OUT -> IT_1 -> OT_2 -> ALSA_Capture
 * ALSA_Playback -> IT_3 -> OT_4 -> USB-IN
 */
#define F_AUDIO_AC_INTERFACE		0
#define F_AUDIO_AS_OUT_INTERFACE	1
#define F_AUDIO_AS_IN_INTERFACE		2
/* Number of streaming interfaces */
#define F_AUDIO_NUM_INTERFACES		1

static struct usb_interface_assoc_descriptor uac_iad = {
	.bLength		= sizeof(uac_iad),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0,
	.bInterfaceCount	= 2,
	.bFunctionClass		= USB_CLASS_AUDIO,
	.bFunctionSubClass	= USB_SUBCLASS_AUDIOSTREAMING,
	.bFunctionProtocol	= UAC_VERSION_1,
	.iFunction			= 0,
};

/* B.3.1  Standard AC Interface Descriptor */
static struct usb_interface_descriptor ac_interface_desc = {
	.bLength 			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType 	= USB_DT_INTERFACE,
	.bAlternateSetting 	= 0,
	.bNumEndpoints 		= 0,
	.bInterfaceClass 	= USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
	.bInterfaceProtocol = UAC_VERSION_1,
	.iInterface			= 0
};

/*
 * The number of AudioStreaming and MIDIStreaming interfaces
 * in the Audio Interface Collection
 */
DECLARE_UAC_AC_HEADER_DESCRIPTOR(1);

#define UAC_DT_AC_HEADER_LENGTH	UAC_DT_AC_HEADER_SIZE(F_AUDIO_NUM_INTERFACES)


#define USB_OUT_IT_ID	1
static struct uac_input_terminal_descriptor usb_out_it_desc = {
	.bLength =		UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID =		USB_OUT_IT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	0,
	.bNrChannels 		= 1,
	.wChannelConfig =	cpu_to_le16(0x3),
	.iChannelNames 		= 0,
	.iTerminal 			= 0,
};

#if 0
#define IO_OUT_OT_ID	2
static struct uac1_output_terminal_descriptor io_out_ot_desc = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= IO_OUT_OT_ID,
	.wTerminalType		= cpu_to_le16(UAC_OUTPUT_TERMINAL_SPEAKER),
	.bAssocTerminal		= 0,
	.bSourceID		= USB_OUT_IT_ID,
	.iTerminal 			= 0,
};
#endif

#define IO_IN_IT_ID	UAC2_IO_IN_IT_ID//3
static struct uac_input_terminal_descriptor io_in_it_desc = {
	.bLength		= UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_INPUT_TERMINAL,
	.bTerminalID		= IO_IN_IT_ID,
	.wTerminalType		= cpu_to_le16(UAC_INPUT_TERMINAL_MICROPHONE),
	.bAssocTerminal		= 0,
	.bNrChannels 		= 1,
	.wChannelConfig		= cpu_to_le16(0x3),
	.iChannelNames 		= 0,
	.iTerminal 			= 0,
};

#define USB_IN_OT_ID	UAC2_USB_IN_OT_ID//4
#define UAC1_FEATURE_UNIT_ID	UAC2_FEATURE_UNIT_ID//5

static struct uac1_output_terminal_descriptor usb_in_ot_desc = {
	.bLength =		UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_OUTPUT_TERMINAL,
	.bTerminalID =		USB_IN_OT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	0,
	.bSourceID =UAC1_FEATURE_UNIT_ID,
	.iTerminal 			= 0,
	
};

#if 0
/*Audio Control Feature Unit Descriptor*/
DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);
static struct  uac_feature_unit_descriptor_0 uac_feature_unit_desc = {
        .bLength =      UAC_DT_FEATURE_UNIT_SIZE(0),     
		.bDescriptorType = USB_DT_CS_INTERFACE,      
        .bDescriptorSubtype     = UAC_FEATURE_UNIT, 
        .bUnitID = UAC1_FEATURE_UNIT_ID,      
        .bSourceID =IO_IN_IT_ID,   
        .bControlSize = 1,     
        .bmaControls[0] = (UAC_FU_MUTE | UAC_FU_VOLUME), 
        .iFeature = 0, 
};
#else
struct uac_feature_unit_descriptor_0 {			
	__u8  bLength;						
	__u8  bDescriptorType;					
	__u8  bDescriptorSubtype;				
	__u8  bUnitID;						
	__u8  bSourceID;					
	__u8  bControlSize;					
	__u8  bmaControls[1];				
	__u8  iFeature;						
} __attribute__ ((packed));

static struct  uac_feature_unit_descriptor_0 uac_feature_unit_desc = {
        .bLength =      sizeof(struct uac_feature_unit_descriptor_0),     
		.bDescriptorType = USB_DT_CS_INTERFACE,      
        .bDescriptorSubtype     = UAC_FEATURE_UNIT, 
        .bUnitID = UAC1_FEATURE_UNIT_ID,      
        .bSourceID =IO_IN_IT_ID,   
        .bControlSize = 1,     
        .bmaControls[0] = (UAC_FU_MUTE | UAC_FU_VOLUME), 
        .iFeature = 0, 
};
#endif


/* 2 input terminals and 2 output terminals */
//Ron_Lee 20180125
#if 0
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ 1*UAC_DT_INPUT_TERMINAL_SIZE + 1*UAC_DT_OUTPUT_TERMINAL_SIZE)
#else
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ 1*UAC_DT_INPUT_TERMINAL_SIZE + 1*UAC_DT_OUTPUT_TERMINAL_SIZE) \
	+ sizeof(uac_feature_unit_desc)
#endif

/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_1 ac_header_desc = {
	.bLength =		UAC_DT_AC_HEADER_LENGTH,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_HEADER,
	.bcdADC =		cpu_to_le16(0x0100),
	.wTotalLength =		cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bInCollection =	F_AUDIO_NUM_INTERFACES,
	.baInterfaceNr = {
	/* Interface number of the AudioStream interfaces */
		[0] =		1,
	}
};

#if 0
/* B.4.1  Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_out_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_1,
	.iInterface			= 0,
};


static struct usb_interface_descriptor as_out_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_1,
	.iInterface			= 0,
};
#endif

static struct usb_interface_descriptor as_in_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_1,
	.iInterface			= 0,
};

static struct usb_interface_descriptor as_in_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = UAC_VERSION_1,
	.iInterface			= 0,
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_in_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink =	USB_IN_OT_ID,
	.bDelay =		1,
	.wFormatTag =		cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};

DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(1);

static struct uac_format_type_i_discrete_descriptor_1 as_out_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bNrChannels 		= 2,/* Number of channels - 2 */
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
	.tSamFreq[0][0] = 0x80,
	.tSamFreq[0][1] = 0x3E,
	.tSamFreq[0][2] = 0x00,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_out_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,

	//.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.wMaxPacketSize	=	cpu_to_le16(0x40),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor as_out_ep_desc_comp = { 
        .bLength                = sizeof(struct usb_ss_ep_comp_descriptor),
        .bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,
        /* The following 3 values can be tweaked if necessary. */
        .bMaxBurst              = 0,
        .bmAttributes           = 0,
        .wBytesPerInterval      = cpu_to_le16(0x40),
};


static struct uac_format_type_i_discrete_descriptor_1 as_in_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
	.tSamFreq[0][0] = 0x80,
	.tSamFreq[0][1] = 0x3E,
	.tSamFreq[0][2] = 0x00,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_in_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,

	//.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.wMaxPacketSize	=	cpu_to_le16(0x40),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor as_in_ep_desc_comp = { 
        .bLength                = sizeof(struct usb_ss_ep_comp_descriptor),
        .bDescriptorType        = USB_DT_SS_ENDPOINT_COMP,
        /* The following 3 values can be tweaked if necessary. */
        .bMaxBurst              = 0,
        .bmAttributes           = 0,
        .wBytesPerInterval      = cpu_to_le16(0x40),
};


/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_in_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes =		1,
	.bLockDelayUnits =	0,
	.wLockDelay =		0,
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

static struct usb_audio_control_selector feature_unit_mic = {
	.list = LIST_HEAD_INIT(feature_unit_mic.list),
	//.id = FEATURE_UNIT_MIC_ID,
	.id = UAC1_FEATURE_UNIT_ID,
	.name = "Mute & Volume Control",
	.type = UAC_FEATURE_UNIT,
	.desc = (struct usb_descriptor_header *)&uac_feature_unit_desc,
};

static struct usb_descriptor_header *f_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,

	(struct usb_descriptor_header *)&io_in_it_desc,
#if 1
	(struct usb_descriptor_header *)&usb_in_ot_desc,
#endif
#if 1
//Ron_Lee 20180125
	(struct usb_descriptor_header *)&uac_feature_unit_desc,
#endif

	

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
#if 1
	(struct usb_descriptor_header *)&as_in_header_desc,
#endif
	(struct usb_descriptor_header *)&as_in_type_i_desc,
	(struct usb_descriptor_header *)&as_in_ep_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

static struct usb_descriptor_header *f_ss_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,

	(struct usb_descriptor_header *)&io_in_it_desc,
#if 1
	(struct usb_descriptor_header *)&usb_in_ot_desc,
#endif
#if 1
//Ron_Lee 20180125
	(struct usb_descriptor_header *)&uac_feature_unit_desc,
#endif

	

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
#if 1
	(struct usb_descriptor_header *)&as_in_header_desc,
#endif
	(struct usb_descriptor_header *)&as_in_type_i_desc,
	(struct usb_descriptor_header *)&as_in_ep_desc,

	(struct usb_descriptor_header *)&as_in_ep_desc_comp,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

enum {
	STR_AC_IF,
	STR_MIC_IF,
	STR_SPEAK_IF,

};

static struct usb_string strings_uac1[] = {
	[STR_AC_IF].s	= uac_name,
	[STR_MIC_IF].s	= mic_name,
	[STR_SPEAK_IF].s	= speak_name,
	{},			/* end of list */
};

static struct usb_gadget_strings str_uac1 = {
	.language = 0x0409,	/* en-us */
	.strings = strings_uac1,
};

static struct usb_gadget_strings *uac1_strings[] = {
	&str_uac1,
	NULL,
};

/*
 * This function is an ALSA sound card following USB Audio Class Spec 1.0.
 */

/*-------------------------------------------------------------------------*/
struct f_audio_buf {
        u8 *buf;
        int actual;
        struct list_head list;
};

#if 0
static struct f_audio_buf *f_audio_buffer_alloc(int buf_size)
{
        struct f_audio_buf *copy_buf;

        copy_buf = kzalloc(sizeof *copy_buf, GFP_ATOMIC);
        if (!copy_buf)
                return ERR_PTR(-ENOMEM);

        copy_buf->buf = kzalloc(buf_size, GFP_ATOMIC);
        if (!copy_buf->buf) {
                kfree(copy_buf);
                return ERR_PTR(-ENOMEM);
        }

        return copy_buf;
}
#endif

/*-------------------------------------------------------------------------*/

struct gaudio_snd_dev { 
        struct gaudio                   *card;
        struct file                     *filp;
        struct snd_pcm_substream        *substream;
        int                             access;
        int                             format;
        int                             channels;
        int                             rate;
};

struct gaudio {
        struct usb_function             func;
        struct usb_gadget               *gadget;

        /* ALSA sound device interfaces */
        struct gaudio_snd_dev           control;
        struct gaudio_snd_dev           playback;
        struct gaudio_snd_dev           capture;

        /* TODO */
};


struct f_audio {
        struct gaudio                   card;

        /* endpoints handle full and/or high speeds */
        struct usb_ep                   *out_ep;
        struct usb_endpoint_descriptor  *out_desc;

        spinlock_t                      lock;
        struct f_audio_buf *copy_buf;
        struct work_struct playback_work;
        struct list_head play_queue;

        /* Control Set command */
        struct list_head cs;
        u8 set_cmd;
        struct usb_audio_control *set_con;
};

static inline struct f_audio *func_to_audio(struct usb_function *f)
{
        return container_of(f, struct f_audio, card.func);
}

/*-------------------------------------------------------------------------*/

#if 0
static int f_audio_out_ep_complete(struct usb_ep *ep, struct usb_request *req)
{
        struct f_audio *audio = req->context;
        struct usb_composite_dev *cdev = audio->card.func.config->cdev;
        struct f_audio_buf *copy_buf = audio->copy_buf;
        int err;
		char *p;

        if (!copy_buf)
                return -EINVAL;

        /* Copy buffer is full, add it to the play_queue */
        if (audio_buf_size - copy_buf->actual < req->actual) {
                list_add_tail(&copy_buf->list, &audio->play_queue);
                schedule_work(&audio->playback_work);
                copy_buf = f_audio_buffer_alloc(audio_buf_size);
                if (IS_ERR(copy_buf))
                        return -ENOMEM;
        }

		p = (char *)req->buf;
		//printk(KERN_INFO "req->buf[0]:%x\n", *p);

        memcpy(copy_buf->buf + copy_buf->actual, req->buf, req->actual);
        copy_buf->actual += req->actual;
        audio->copy_buf = copy_buf;

        err = usb_ep_queue(ep, req, GFP_ATOMIC);
        if (err)
                ERROR(cdev, "%s queue req: %d\n", ep->name, err);

        return 0;

}
#endif


static void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_uac1 *uac1 = req->context;
	int status = req->status;
	struct usb_ep *in_ep;
	u32 data = 0;
	struct v4l2_event v4l2_event;
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	
	in_ep = uac1->uac_dev.in_ep;
	switch (status) {

	case 0: 			/* normal completion? */
		if (ep == in_ep){
			//f_audio_out_ep_complete(ep, req);
		}
		else if (uac1->set_con) {
			//printk("##########out_ep #######\n");
			memcpy(&data, req->buf, req->length);
			uac1->set_con->set(uac1->set_con, uac1->set_cmd,
					le16_to_cpu(data));
			uac1->set_con = NULL;
			
			v4l2_event.type = uac1->event_type;
			//printk("v4l2_event.type %d\n", v4l2_event.type);
			memcpy(&v4l2_event.u.data, req->buf, req->length);
			data = ((uint16_t *)&v4l2_event.u.data)[0];
			//printk("data = 0x%x,uac1 = 0x%p\n", data,uac1);
			v4l2_event_queue(uac1->uac_dev.vdev, &v4l2_event);
		}
		break;
	default:
		break;
	}
}


static int audio_set_intf_req(struct usb_function *f,
                const struct usb_ctrlrequest *ctrl)
{
	    struct f_uac1 *uac1 = func_to_uac1(f);
        struct usb_composite_dev *cdev = f->config->cdev;
        struct usb_request      *req = cdev->req;
        u8                      id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
        u16                     len = le16_to_cpu(ctrl->wLength);
        u16                     w_value = le16_to_cpu(ctrl->wValue);
        u8                      con_sel = (w_value >> 8) & 0xFF;
        u8                      cmd = (ctrl->bRequest & 0x0F);
        struct usb_audio_control_selector *cs;
        struct usb_audio_control *con;

        //printk(KERN_INFO "%s bRequest 0x%x, w_value 0x%04x, len %d, entity %d\n",__func__, ctrl->bRequest, w_value, len, id);

		if(id == UAC1_FEATURE_UNIT_ID){
		 	if(con_sel == 0x01){
				uac1->event_type = UAC_MIC_MUTE;
			}else{
				uac1->event_type = UAC_MIC_VOLUME;
			}
		}
#if 1
        list_for_each_entry(cs, &uac1->cs, list) {
                if (cs->id == id) {
                        list_for_each_entry(con, &cs->control, list) {
                                if (con->type == con_sel) {
                                        uac1->set_con = con;
                                        break;
                                }
                        }
                        break;
                }
        }


        uac1->set_cmd = cmd;
        req->context = uac1;
        req->complete = f_audio_complete;
#endif

        return len;
}

static int audio_get_intf_req(struct usb_function *f,
                const struct usb_ctrlrequest *ctrl)
{
	    struct f_uac1 *uac1 = func_to_uac1(f);
        struct usb_composite_dev *cdev = f->config->cdev;
        struct usb_request      *req = cdev->req;
        int                     value = -EOPNOTSUPP;
        u8                      id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
        u16                     len = le16_to_cpu(ctrl->wLength);
        u16                     w_value = le16_to_cpu(ctrl->wValue);
        u8                      con_sel = (w_value >> 8) & 0xFF;
        u8                      cmd = (ctrl->bRequest & 0x0F);
        struct usb_audio_control_selector *cs;
        struct usb_audio_control *con;

	printk("%s 2\n", __func__);
        printk("bRequest 0x%x, w_value 0x%04x, len %d, entity %d\n", ctrl->bRequest, w_value, len, id);

#if 1
        list_for_each_entry(cs, &uac1->cs, list) {
                if (cs->id == id) {
                        list_for_each_entry(con, &cs->control, list) {
                                if (con->type == con_sel && con->get) {
                                        value = con->get(con, cmd);
                                        break;
                                }
                        }
                        break;
                }
        }


        req->context = uac1;
        req->complete = f_audio_complete;
		len = min_t(size_t, sizeof(value), len);
        memcpy(req->buf, &value, len);
#endif

        //printk( "%s len:%d leave\n",__func__, len);
        return len;
}


/*
 * This function is an ALSA sound card following USB Audio Class Spec 1.0.
 */

static int audio_set_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	//struct usb_composite_dev *cdev = f->config->cdev;
	int			value = -EOPNOTSUPP;
	u16			ep = le16_to_cpu(ctrl->wIndex);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);

	printk("%s:bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n", 
			__func__, ctrl->bRequest, w_value, len, ep);

	switch (ctrl->bRequest) {
	case UAC_SET_CUR:
		value = len;
		break;

	case UAC_SET_MIN:
		break;

	case UAC_SET_MAX:
		break;

	case UAC_SET_RES:
		break;

	case UAC_SET_MEM:
		break;

	default:
		break;
	}

	return value;
}

static int audio_get_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	int value = -EOPNOTSUPP;
	u8 ep = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16 len = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	printk(  "%s:bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n", 
			__func__, ctrl->bRequest, w_value, len, ep);

	switch (ctrl->bRequest) {
	case UAC_GET_CUR:
	case UAC_GET_MIN:
	case UAC_GET_MAX:
	case UAC_GET_RES:
		value = len;
		break;
	case UAC_GET_MEM:
		break;
	default:
		break;
	}

	return value;
}

struct uac_event
{
	union {
		enum usb_device_speed speed;
		struct usb_ctrlrequest req;
		//struct uvc_request_data data;
	};
};


static int
f_audio_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);


	//printk(KERN_EMERG "%s:\n", __func__);

	/* composite driver infrastructure handles everything; interface
	 * activation uses set_alt().
	 */
	switch (ctrl->bRequestType) {
	    case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		//printk("%s: 1\n", __func__);
		value = audio_set_intf_req(f, ctrl);
		break;

	    case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		//printk("%s 2 ctrl->bRequestType:%x ctrl->bRequest:%x\n", __func__, ctrl->bRequestType, ctrl->bRequest);
		value = audio_get_intf_req(f, ctrl);
#if 0
//Ron_Lee 20180126
		{
		        struct f_uac1 *uac1 = func_to_uac1(f);
			struct uac_device *uac_dev = &uac1->uac_dev;
			struct v4l2_event v4l2_event;

			struct uac_event *p_uac_event = (void *)&v4l2_event.u.data;

			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_SETUP;
    
			//uac_dev->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
			//uac_dev->event_length = le16_to_cpu(ctrl->wLength);

			//printk(%s crtl->bRequestType;%x ctrl->bRequest:%x  w_length:%d\n", __func__, ctrl->bRequestType, ctrl->bRequest, w_length);
		//	memcpy(&uvc_event->req, ctrl, sizeof(uvc_event->req));
			memcpy(&p_uac_event->req, ctrl, sizeof(p_uac_event->req));
		//	printk("%s: %x %x  %d\n", __func__, uvc_event->req.bRequestType, uvc_event->req.bRequest, sizeof(uvc_event->req));

			
			v4l2_event_queue(uac_dev->vdev, &v4l2_event);


			//printk("%s: uvc_event->req.bRequestType:%x uvc_event->req.bRequest:%x trigger setup\n", __func__, uvc_event->req.bRequestType, uvc_event->req.bRequest);
			//printk("%s: uvc_event->req.bRequestType:%x  trigger setup\n", __func__, (struct uvc_event *)&v4l2_event.u.data->req.bRequestType);
		}
#endif
		
//
		break;

	    case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_set_endpoint_req(f, ctrl);
		break;

	    case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_get_endpoint_req(f, ctrl);
		break;

	    default:
		printk(   "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "audio response on err %d\n", value);
		//printk( "%s:value:%d leave\n", __func__, value);
	}

	/* device either stalls (value < 0) or reports success */
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
        //struct usb_request *req = uvc->control_req;
	struct usb_request	*req = cdev->req;
	int ret;

	//printk(KERN_INFO "%s:%d data->length:%d \n", __func__, __LINE__, data->length);
        if (data->length < 0)
                return usb_ep_set_halt(cdev->gadget->ep0);

#if 0
        req->length = min_t(unsigned int, uvc->event_length, data->length);
        req->zero = data->length < uvc->event_length;
#else
	req->zero = 0;
	req->length = data->length;
	
#endif
	//printk(KERN_INFO "%s:%d req->zero:%d req->length:%d \n", __func__, __LINE__, req->zero, req->length);

        memcpy(req->buf, data->data, req->length);

        //ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
        ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
        //ret = usb_ep_queue(uvc->in_ep, req, GFP_ATOMIC);
	//printk(KERN_INFO "%s:%d req->zero:%d req->length:%d ret:%d %p %p\n", __func__, __LINE__, req->zero, req->length, ret, cdev->gadget->ep0, req);

	return ret;
}
#endif

static int f_audio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_uac1 *uac1 = func_to_uac1(f);
	int ret = 0;

	//printk( "%s:intf:%d alt=%d\n", __func__, intf, alt);


	/* No i/f has more than 2 alt settings */
	if (alt > 1) {
		printk(  "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (intf == uac1->ac_intf) {
		/* Control I/f has only 1 AltSetting - 0 */
		if (alt) {
			printk(  "%s:%d Error!\n", __func__, __LINE__);
			return -EINVAL;
		}

		/*FIXME: notify userpace uac has been connected*/
		{
			struct uac_device *uac_dev = &uac1->uac_dev;
			struct v4l2_event v4l2_event;

			uac_device_stop_playback(&uac1->uac_dev);
			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_CONNECT;
			v4l2_event_queue(uac_dev->vdev, &v4l2_event);

			
			printk( "%s: trigger connect\n", __func__);
		    }

		return 0;
	}

	if (intf == uac1->as_out_intf) {
		uac1->as_out_alt = alt;

		if (alt) {
		} else {
		}
	} else if (intf == uac1->as_in_intf) {
		uac1->as_in_alt = alt;

			if (alt) {
				/*FIXME: notify userpace to start audio streaming*/
				{
					struct uac_device *uac_dev = &uac1->uac_dev;
					struct v4l2_event v4l2_event;

					
					ret = uac_device_start_playback(&uac1->uac_dev);

					memset(&v4l2_event, 0, sizeof(v4l2_event));
					v4l2_event.type = UAC_EVENT_STREAMON;


					v4l2_event_queue(uac_dev->vdev, &v4l2_event);

					//printk( "%s: trigger UAC_EVENT_STREAMON\n", __func__);
				}

				
			} else {	
				/*FIXME: notify userpace to start audio streaming*/
				{
					struct uac_device *uac_dev = &uac1->uac_dev;
					struct v4l2_event v4l2_event;
#if 0
					//Ron_Lee 20180126
					struct uvc_event *uvc_event = (void *)&v4l2_event.u.data;

					memset(&uvc_event->req, 0x01, sizeof(uvc_event->req));
#endif

					uac_device_stop_playback(&uac1->uac_dev);
					
					memset(&v4l2_event, 0, sizeof(v4l2_event));
					v4l2_event.type = UAC_EVENT_STREAMOFF;

					v4l2_event_queue(uac_dev->vdev, &v4l2_event);

					//printk( "%s: *#*#*#trigger UAC_EVENT_STREAMOFF\n", __func__);
				}

				
			}
	} else {
		printk("%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}

static int f_audio_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_uac1 *uac1 = func_to_uac1(f);

	printk("%s:\n", __func__);

	if (intf == uac1->ac_intf) {
		printk("%s:ac_intf = %d\n", __func__, uac1->ac_intf);
		return uac1->ac_alt;
	}
	else if (intf == uac1->as_out_intf) {
		printk("%s:as_out_intf = %d\n", __func__, uac1->as_out_intf);
		return uac1->as_out_alt;
	}
	else if (intf == uac1->as_in_intf) {
		printk("%s:as_in_intf = %d\n", __func__, uac1->as_in_intf);
		return uac1->as_in_alt;
	}
	else
		printk("%s:%d Invalid Interface %d!\n",
			__func__, __LINE__, intf);

	return -EINVAL;
}


static void f_audio_disable(struct usb_function *f)
{
	struct f_uac1 *uac1 = func_to_uac1(f);

	uac1->as_out_alt = 0;
	uac1->as_in_alt = 0;

	uac_device_stop_playback(&uac1->uac_dev);

	/*FIXME: notify userpace to disconnect*/
	{
		struct uac_device *uac_dev = &uac1->uac_dev;
		struct v4l2_event v4l2_event;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UAC_EVENT_DISCONNECT;
		v4l2_event_queue(uac_dev->vdev, &v4l2_event);
	}
	
	
}

/*-------------------------------------------------------------------------*/

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

static void __print_params(struct uac_params *params)
{
	printk( "pamrams: p_chmask=%d p_srate=%d(hz) p_ssize=%d \
			req_number=%d\n",
			params->p_chmask,
			params->p_srate,
			params->p_ssize,
			params->req_number
		);
}

/* audio function driver setup/binding */
static int f_audio_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev	*cdev = c->cdev;
	struct usb_gadget		*gadget = cdev->gadget;
	struct f_uac1			*uac1 = func_to_uac1(f);
	struct uac_device *uac_dev = &uac1->uac_dev;
	struct f_uac1_opts		*audio_opts;
	struct usb_ep			*ep = NULL;
	struct usb_string		*us;
	u8				*sam_freq;
	int				rate;
	int				status;

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


	audio_opts = container_of(f->fi, struct f_uac1_opts, func_inst);

	us = usb_gstrings_attach(cdev, uac1_strings, ARRAY_SIZE(strings_uac1));
	if (IS_ERR(us))
		return PTR_ERR(us);

	if(audio_freq != 16000)
	{
		audio_opts->p_srate = audio_freq;
	}
	
	as_out_type_i_desc.tSamFreq[0][0] = audio_opts->c_srate & 0xFF;
	as_out_type_i_desc.tSamFreq[0][1] = (audio_opts->c_srate  >> 8) & 0xFF;;
	as_out_type_i_desc.tSamFreq[0][2] = (audio_opts->c_srate  >> 16) & 0xFF;

	as_in_type_i_desc.tSamFreq[0][0] = audio_opts->p_srate & 0xFF;
	as_in_type_i_desc.tSamFreq[0][1] = (audio_opts->p_srate  >> 8) & 0xFF;;
	as_in_type_i_desc.tSamFreq[0][2] = (audio_opts->p_srate  >> 16) & 0xFF;

	if(uac_name[0] != 0)
	{
		uac_iad.iFunction = us[STR_AC_IF].id;
		ac_interface_desc.iInterface = us[STR_AC_IF].id;
	}

	if(mic_name[0] != 0)
	{
		uac_feature_unit_desc.iFeature = us[STR_MIC_IF].id;
		io_in_it_desc.iTerminal = us[STR_MIC_IF].id;
		io_in_it_desc.iChannelNames = us[STR_MIC_IF].id;
		usb_in_ot_desc.iTerminal = us[STR_MIC_IF].id;
		as_in_interface_alt_0_desc.iInterface = us[STR_MIC_IF].id;
		as_in_interface_alt_1_desc.iInterface = us[STR_MIC_IF].id;
	}
		
#if 0
	uac_iad.iFunction = us[STR_AC_IF].id;
	ac_interface_desc.iInterface = us[STR_AC_IF].id;
	usb_out_it_desc.iTerminal = us[STR_USB_OUT_IT].id;
	usb_out_it_desc.iChannelNames = us[STR_USB_OUT_IT_CH_NAMES].id;
	io_out_ot_desc.iTerminal = us[STR_IO_OUT_OT].id;
	as_out_interface_alt_0_desc.iInterface = us[STR_AS_OUT_IF_ALT0].id;
	as_out_interface_alt_1_desc.iInterface = us[STR_AS_OUT_IF_ALT1].id;
	io_in_it_desc.iTerminal = us[STR_IO_IN_IT].id;
	io_in_it_desc.iChannelNames = us[STR_IO_IN_IT_CH_NAMES].id;
	usb_in_ot_desc.iTerminal = us[STR_USB_IN_OT].id;
	//as_in_interface_alt_0_desc.iInterface = us[STR_AS_IN_IF_ALT0].id;
	as_in_interface_alt_1_desc.iInterface = us[STR_AS_IN_IF_ALT1].id;
#endif

	/* Set channel numbers */
	usb_out_it_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	usb_out_it_desc.wChannelConfig = cpu_to_le16(audio_opts->c_chmask);
	as_out_type_i_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	as_out_type_i_desc.bSubframeSize = audio_opts->c_ssize;
	as_out_type_i_desc.bBitResolution = audio_opts->c_ssize * 8;
	//io_in_it_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	io_in_it_desc.wChannelConfig = cpu_to_le16(audio_opts->p_chmask);
	as_in_type_i_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	as_in_type_i_desc.bSubframeSize = audio_opts->p_ssize;
	as_in_type_i_desc.bBitResolution = audio_opts->p_ssize * 8;

	//divce to host
	as_in_ep_desc.wMaxPacketSize = num_channels(audio_opts->p_chmask) * audio_opts->p_ssize * (audio_opts->p_srate/1000);
	as_in_ep_desc_comp.wBytesPerInterval = le16_to_cpu(as_in_ep_desc.wMaxPacketSize);
	//host to divce
	as_out_ep_desc.wMaxPacketSize = num_channels(audio_opts->c_chmask) * audio_opts->c_ssize * (audio_opts->c_srate/1000);
	as_out_ep_desc_comp.wBytesPerInterval = le16_to_cpu(as_out_ep_desc.wMaxPacketSize);
	

	/* Set sample rates */
	rate = audio_opts->c_srate;
	sam_freq = as_out_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	printk( "================sam_freq:%d rate:%d\n", *sam_freq, rate);
	rate = audio_opts->p_srate;
	sam_freq = as_in_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	printk( "================sam_freq:%d rate:%d\n", *sam_freq, rate);

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0) {
		printk( "%s:%d\n", __func__, __LINE__);
		goto fail;
	}

	uac_iad.bFirstInterface = status;
	ac_interface_desc.bInterfaceNumber = status;
	uac1->ac_intf = status;
	uac1->ac_alt = 0;

	printk( "ac_intf:%d\n", status);

	status = usb_interface_id(c, f);
	if (status < 0) {
		printk( "%s:%d\n", __func__, __LINE__);
		goto fail;
	}

	as_in_interface_alt_0_desc.bInterfaceNumber = status;
	as_in_interface_alt_1_desc.bInterfaceNumber = status;
	ac_header_desc.baInterfaceNr[0] = status;

	uac1->as_in_intf = status;
	uac1->as_in_alt = 0;

	printk( "as_in_intf:%d\n", status);

	uac_dev->gadget = gadget;

	status = -ENODEV;

#if 0
	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &as_out_ep_desc);
	if (!ep) {
		printk( "%s:%d\n", __func__,__LINE__);
		goto fail;
	}

	uac_dev->out_ep = ep;
	uac_dev->out_ep->desc = &as_out_ep_desc;
#endif

	ep = usb_ep_autoconfig(cdev->gadget, &as_in_ep_desc);
	if (!ep) {
		printk( "usb_ep_autoconfig fail %s:%d\n", __func__,__LINE__);
		goto fail;
	}
	uac_dev->in_ep = ep;
	uac_dev->in_ep->desc = &as_in_ep_desc;

	as_out_ep_desc_comp.wBytesPerInterval = le16_to_cpu(as_out_ep_desc.wMaxPacketSize);
	as_in_ep_desc_comp.wBytesPerInterval = le16_to_cpu(as_in_ep_desc.wMaxPacketSize);;
	/* copy descriptors, and track endpoint copies */
	status = usb_assign_descriptors(f, f_audio_desc, f_audio_desc, f_ss_audio_desc, NULL);
	if (status) {
		printk( "usb_assign_descriptors fail %s:%d\n", __func__,__LINE__);
		goto fail;
	}

	uac_dev->out_ep_maxpsize = le16_to_cpu(as_out_ep_desc.wMaxPacketSize);
	uac_dev->in_ep_maxpsize = le16_to_cpu(as_in_ep_desc.wMaxPacketSize);
	uac_dev->params.c_chmask = audio_opts->c_chmask;
	uac_dev->params.c_srate = audio_opts->c_srate;
	uac_dev->params.c_ssize = audio_opts->c_ssize;
	uac_dev->params.p_chmask = audio_opts->p_chmask;
	uac_dev->params.p_srate = audio_opts->p_srate;
	uac_dev->params.p_ssize = audio_opts->p_ssize;
	uac_dev->params.req_number = audio_opts->req_number;

	printk( "*******************pamrams: p_chmask=%d p_srate=%d(hz) p_ssize=%d \
			req_number=%d uac_dev->out_ep_maxpsize:%d\n",
			audio_opts->p_chmask,
			audio_opts->p_srate,
			audio_opts->p_ssize,
			audio_opts->req_number,
			uac_dev->out_ep_maxpsize
		);

	__print_params(&uac_dev->params);


	status = uac_device_setup(uac_dev);
	if (status)
		goto err_card_register;


	/*FIXME: add v4l2 interface*/
	{
		uac_dev = &uac1->uac_dev;

		if (uac_dev) {
			if (v4l2_device_register(&cdev->gadget->dev, &(uac_dev->v4l2_dev))) {
				printk(KERN_INFO "v4l2_device_register failed\n");
				goto error;
			}

			/* Initialise queue buffer. */
			status = uac_queue_init(&(uac_dev->queue));
			if (status < 0)
				goto error;

			/* Register a V4L2 device. */
			status = __uac_register_video(cdev, uac_dev);
			if (status < 0) {
				printk(KERN_INFO "Unable to register video device\n");
				goto error;
			}
		}
	}

	return 0;

error:
	if (uac_dev) {
		v4l2_device_unregister(&(uac1->uac_dev.v4l2_dev));
		if ((uac1->uac_dev.vdev))
			video_device_release((uac1->uac_dev.vdev));
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

/* Todo: add more control selecotor dynamically */
static int control_selector_init(struct f_uac1 *uac1)
{
	INIT_LIST_HEAD(&uac1->cs);
	list_add(&feature_unit_mic.list, &uac1->cs);
	INIT_LIST_HEAD(&feature_unit_mic.control);
	list_add(&mic_mute_control.list, &feature_unit_mic.control);
	list_add(&mic_volume_control.list, &feature_unit_mic.control);
	
	mic_volume_control.data[UAC__CUR] = mic_volume_def;
	mic_volume_control.data[UAC__MIN] = mic_volume_min;
	mic_volume_control.data[UAC__MAX] = mic_volume_max;
	mic_volume_control.data[UAC__RES] = mic_volume_res;
	mic_mute_control.data[UAC__CUR] = 0x0;


	uac1->event_type = 0;
	return 0;
}


/*-------------------------------------------------------------------------*/

static inline struct f_uac1_opts *to_f_uac1_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_uac1_opts,
			func_inst.group);
}

static void f_audio_free_inst(struct usb_function_instance *f)
{
	struct f_uac1_opts *opts;

	opts = container_of(f, struct f_uac1_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *f_audio_alloc_inst(void)
{
	struct f_uac1_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = f_audio_free_inst;

#if 0
    opts->c_srate = g_c_srate;
    opts->req_number = g_req_num;
#else
    opts->c_srate = UAC1_DEF_PSRATE;
    opts->req_number = UAC1_DEF_REQ_NUM; 
#endif

    opts->c_ssize = UAC1_DEF_CSSIZE;
	opts->c_chmask = UAC1_DEF_CCHMASK;
	opts->p_chmask = UAC1_DEF_PCHMASK;
	opts->p_srate = UAC1_DEF_PSRATE;
	opts->p_ssize = UAC1_DEF_PSSIZE;

	return &opts->func_inst;
}

static void f_audio_free(struct usb_function *f)
{
	struct f_uac1_opts *opts;
	struct f_uac1  *uac1 = func_to_uac1(f);

	opts = container_of(f->fi, struct f_uac1_opts, func_inst);
	mutex_lock(&opts->lock);
	--opts->refcnt;
	mutex_unlock(&opts->lock);

	kfree(uac1);
}

static void f_audio_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_uac1  *uac1 = func_to_uac1(f);
	struct uac_device *uac_dev = &uac1->uac_dev;

	uac_device_cleanup(uac_dev);
	usb_free_all_descriptors(f);

	uac_dev->gadget = NULL;

	if (uac_dev) { /* release v4l2 device*/
		video_unregister_device(uac_dev->vdev);
		v4l2_device_unregister(&(uac_dev->v4l2_dev));
	}
}

	
static void f_audio_suspend(struct usb_function *f)
{
	struct f_uac1 *uac1 = func_to_uac1(f);
	struct uac_device *uac_dev = &uac1->uac_dev;
	struct v4l2_event v4l2_event;
	
	uac1->as_out_alt = 0;
	uac1->as_in_alt = 0;
	uac_device_stop_playback(&uac1->uac_dev);
	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UAC_EVENT_SUSPEND;
	v4l2_event_queue(uac_dev->vdev, &v4l2_event);
}

static void f_audio_resume(struct usb_function *f)
{
	struct f_uac1 *uac1 = func_to_uac1(f);
	struct uac_device *uac_dev = &uac1->uac_dev;
	struct v4l2_event v4l2_event;

	
	memset(&v4l2_event, 0, sizeof(v4l2_event));
	v4l2_event.type = UAC_EVENT_RESUME;
	v4l2_event_queue(uac_dev->vdev, &v4l2_event);
}

static struct usb_function *f_audio_alloc(struct usb_function_instance *fi)
{
	struct f_uac1 *uac1;
	struct f_uac1_opts *opts;

	/* allocate and initialize one new instance */
	uac1 = kzalloc(sizeof(*uac1), GFP_KERNEL);
	if (!uac1)
		return ERR_PTR(-ENOMEM);

	opts = container_of(fi, struct f_uac1_opts, func_inst);
	mutex_lock(&opts->lock);
	++opts->refcnt;
	mutex_unlock(&opts->lock);

	uac1->uac_dev.func.name = "uac1";
	uac1->uac_dev.func.bind = f_audio_bind;
	uac1->uac_dev.func.unbind = f_audio_unbind;
	uac1->uac_dev.func.set_alt = f_audio_set_alt;
	uac1->uac_dev.func.get_alt = f_audio_get_alt;
	uac1->uac_dev.func.setup = f_audio_setup;
	uac1->uac_dev.func.disable = f_audio_disable;
	uac1->uac_dev.func.free_func = f_audio_free;
	uac1->uac_dev.func.suspend = f_audio_suspend;
	uac1->uac_dev.func.resume = f_audio_resume;
	uac1->uac_dev.alsa_buf_num = alsa_buf_num;
	
	control_selector_init(uac1);
	return &uac1->uac_dev.func;
}

DECLARE_USB_FUNCTION_INIT(uac1, f_audio_alloc_inst, f_audio_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ruslan Bilovol");
