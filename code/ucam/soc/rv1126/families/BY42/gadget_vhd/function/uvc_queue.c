/*
 *	uvc_queue.c  --  USB Video Class driver - Buffers management
 *
 *	Copyright (C) 2005-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#ifdef __KERNEL__

#include <linux/version.h>

#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
#include "uvc_queue_v4.9.c"
#else
#include "uvc_queue_v3.18.c"
#endif

#endif /* __KERNEL__ */

