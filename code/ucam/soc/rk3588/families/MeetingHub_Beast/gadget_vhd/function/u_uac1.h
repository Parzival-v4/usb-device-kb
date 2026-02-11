/*
 * u_uac1.h - Utility definitions for UAC1 function
 *
 * Copyright (C) 2016 Ruslan Bilovol <ruslan.bilovol@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __U_UAC1_H
#define __U_UAC1_H

#include <linux/usb/composite.h>



#define UAC1_DEF_CCHMASK	0x3//ChannelConfig
#define UAC1_DEF_CSRATE		16000
#define UAC1_DEF_CSSIZE		2
#define UAC1_DEF_PCHMASK	0x3//0x3

#define UAC1_DEF_PSRATE		16000//音频采样率
#define UAC1_DEF_PSSIZE		2 //SubframeSize

#define UAC1_DEF_REQ_NUM	20//usb req队列的个数

#define UAC1_OUT_EP_MAX_PACKET_SIZE	0x40

struct f_uac1_opts {
	struct usb_function_instance	func_inst;
	int				c_chmask;
	int				c_srate;
	int				c_ssize;
	int				p_chmask;
	int				p_srate;
	int				p_ssize;
	int				req_number;
	int             req_buf_size;
	unsigned			bound:1;

	struct mutex			lock;
	int				refcnt;
};

#if 0
struct uvc_request_data
{
	__s32 length;
	__u8 data[60];
};

struct uvc_event
{
	union {
		enum usb_device_speed speed;
		struct usb_ctrlrequest req;
		struct uvc_request_data data;
	};
};
#endif

#endif /* __U_UAC1_H */
