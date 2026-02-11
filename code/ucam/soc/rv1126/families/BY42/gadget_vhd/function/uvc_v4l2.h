/*
 *	uvc_v4l2.h  --  USB Video Class Gadget driver
 *
 * Copyright (C) 2009-2010
 *		Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *		Author: Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __UVC_V4L2_H__
#define __UVC_V4L2_H__

extern const struct v4l2_ioctl_ops uvc_v4l2_ioctl_ops;
extern struct v4l2_file_operations uvc_v4l2_fops;

extern void v4l2_ioctl_mutex_lock(struct uvc_video *video);
extern void v4l2_ioctl_mutex_unlock(struct uvc_video *video);

#ifdef CONFIG_ARCH_SSTAR
extern void * UvcCamOsMemMap(unsigned long long miu_phy_addr, u32 nSize, u8 bCache);
extern void UvcCamOsMemUnmap(void *pVirtPtr, u32 nSize);
#endif

#endif /* __UVC_V4L2_H__ */
