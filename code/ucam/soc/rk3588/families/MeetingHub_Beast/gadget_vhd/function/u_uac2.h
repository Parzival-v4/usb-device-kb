/*
 * u_uac2.h
 *
 * Utility definitions for UAC2 function
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef U_UAC2_H
#define U_UAC2_H

#include <linux/usb/composite.h>
#include "uac_common.h"

#define UAC2_DEF_PCHMASK 0x3
#define UAC2_DEF_PSRATE 48000
#define UAC2_DEF_PSSIZE 2
#define UAC2_DEF_CCHMASK 0x3
#define UAC2_DEF_CSRATE 48000
#define UAC2_DEF_CSSIZE 2
#define UAC2_DEF_REQ_NUM 20

#define UAC2_FS_EP_MAX_PACKET_SIZE 1023
#define UAC2_HS_EP_MAX_PACKET_SIZE 1024
#define UAC2_SS_EP_MAX_PACKET_SIZE 1024

struct f_uac2_opts {
	struct usb_function_instance	func_inst;
	int				p_chmask;
	int				p_srates[UAC_MAX_RATES];
	int				p_ssize;
	int				c_chmask;
	int				c_srates[UAC_MAX_RATES];
	int				c_ssize;
	int				req_number;
	bool				bound;

	struct mutex			lock;
	int				refcnt;
};

#endif
