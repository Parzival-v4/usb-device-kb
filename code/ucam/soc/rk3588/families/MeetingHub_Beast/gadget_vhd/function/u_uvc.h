/*
 * u_uvc.h
 *
 * Utility definitions for the uvc function
 *
 * Copyright (c) 2013-2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef U_UVC_H
#define U_UVC_H

#include <linux/usb/composite.h>

#define to_f_uvc_opts(f)	container_of(f, struct f_uvc_opts, func_inst)

struct f_uvc_opts {
	struct usb_function_instance			func_inst;
	unsigned int					uvc_gadget_trace_param;
	unsigned int					streaming_interval;
	unsigned int					streaming_maxpacket;
	unsigned int					streaming_maxburst;
	unsigned int  					dfu_enable;
	unsigned int  					uac_enable;
	unsigned int  					uvc_s2_enable;
	unsigned int  					uvc_v150_enable;
	unsigned int  					win7_usb3_enable;

	unsigned int 					work_pump_enable;
	unsigned int  					zte_hid_enable;
	unsigned int					uvc_interrupt_ep_enable;
	unsigned int					auto_queue_data_enable;
	const struct uvc_descriptor_header * const	*fs_control;
	const struct uvc_descriptor_header * const	*ss_control;
	const struct uvc_descriptor_header * const	*fs_streaming;
	const struct uvc_descriptor_header * const	*hs_streaming;
	const struct uvc_descriptor_header * const	*ss_streaming;
	
	char *usb_sn_string;
	uint32_t usb_sn_string_length;

	char *usb_vendor_string;
	uint32_t usb_vendor_string_length;

	char *usb_product_string;
	uint32_t usb_product_string_length;
};

void uvc_set_trace_param(unsigned int trace);

#endif /* U_UVC_H */

