/*
 *	uvc_v4l2.c  --  USB Video Class Gadget driver
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
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <linux/version.h>

#include "f_uvc.h"
#include "uvc.h"
#include "uvc_queue.h"
#include "uvc_video.h"
#include "uvc_v4l2.h"

#include "uvc_config.h"
#include "cma_dma.h"


#ifdef CONFIG_ARCH_SSTAR
#include "ms_platform.h"
#include "cam_os_wrapper.h"


void * UvcCamOsMemMap(unsigned long long miu_phy_addr, u32 nSize, u8 bCache)
{
	u64 phy_addr = 0;
	void* pVirtPtr = NULL;
#if (defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9381)
	void* pPhyPtr = NULL;
#endif

#if 0//(defined CHIP_SSC369)
	phy_addr = CamOsMemMiuToPhys(miu_phy_addr);
	if(phy_addr == 0)
	{
		printk("Chip_MIU_to_Phys error\n");
		return NULL;
	}
#else
	phy_addr = Chip_MIU_to_Phys(miu_phy_addr);
	if(phy_addr == 0)
	{
		printk("Chip_MIU_to_Phys error\n");
		return NULL;
	}
#endif

#if (defined CHIP_SSC333 || defined CHIP_SSC337 \
|| defined CHIP_SSC9381 || defined CHIP_SSC9351)
	memcpy(&pPhyPtr, &phy_addr, sizeof(void *));
	//pVirtPtr = CamOsMemMap(phy_addr, nSize, bCache);
	pVirtPtr = CamOsMemMap((void *)pPhyPtr, nSize, bCache);
	if(pVirtPtr == NULL)
	{
		printk("CamOsMemMap error, phy_addr:0x%llx\n",phy_addr);
		return NULL;
	}
#elif (defined CHIP_SSC369)
	pVirtPtr = CamOsMemMap((ss_phys_addr_t)phy_addr, nSize, bCache);
#else
	pVirtPtr = CamOsMemMap((CamOsPhysAddr_t)phy_addr, nSize, bCache);
#endif

	return pVirtPtr;
}
//EXPORT_SYMBOL(UvcCamOsMemMap);

void UvcCamOsMemUnmap(void *pVirtPtr, u32 nSize)
{
	if(pVirtPtr)
		CamOsMemUnmap(pVirtPtr, nSize);
}
//EXPORT_SYMBOL(UvcCamOsMemUnmap);
#endif


void v4l2_ioctl_mutex_lock(struct uvc_video *video)
{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)			
	mutex_lock(&video->v4l2_ioctl_mutex);
#endif
}

void v4l2_ioctl_mutex_unlock(struct uvc_video *video)
{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)		
	mutex_unlock(&video->v4l2_ioctl_mutex);
#endif
}

/* --------------------------------------------------------------------------
 * Requests handling
 */

static int
uvc_send_response(struct uvc_device *uvc, struct uvc_request_data *data)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request *req = uvc->control_req;

    if (uvc->state == UVC_STATE_DISCONNECTED)
    {
        printk("device disconnected\n");
        return -1;
    }

	if (data->length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	req->length = min_t(unsigned int, uvc->event_length, data->length);
	req->zero = data->length < uvc->event_length;
	req->complete = uvc->control_req_completion;

	memcpy(req->buf, data->data, req->length);

	return usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
}

static int
uvc_send_status(struct uvc_device *uvc, struct uvc_request_data *data)
{
	int ret = 0;

#ifdef UVC_INTERRUPT_ENDPOINT_SUPPORT
	unsigned long flags;
	struct usb_composite_dev *cdev;
	struct usb_request *req;

    if (uvc->state == UVC_STATE_DISCONNECTED)
    {
        printk("device disconnected\n");
        return -1;
    }

	if(uvc->uvc_interrupt_ep_enable)
	{
		cdev = uvc->func.config->cdev;
		req = uvc->uvc_status_interrupt_req;
		
		if(!uvc->uvc_status_interrupt_ep)
			return -1;
		
		if(uvc->uvc_status_interrupt_ep->enabled == 0 || req == NULL || uvc->uvc_status_write_pending)
			return -1;
		
		//printk("uvc_send_status, length = %d\n", data->length);
		
		spin_lock_irqsave(&uvc->status_interrupt_spinlock, flags);
		
		if (data->length < 0)
		{
			spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);
			ret = usb_ep_set_halt(uvc->uvc_status_interrupt_ep);
			return ret;
		}

		req->length = min_t(unsigned int, 1024, data->length);
		req->zero = 0;
		req->status   = 0;
		uvc->uvc_status_write_pending = 1;

		memcpy(req->buf, data->data, req->length);
		spin_unlock_irqrestore(&uvc->status_interrupt_spinlock, flags);
		
		ret = usb_ep_queue(uvc->uvc_status_interrupt_ep, req, GFP_KERNEL);
	}
	
	return ret;
#else
	return 0;	
#endif
}

static void uvc_ep0_data_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_device *uvc = req->context;
	
	if (uvc->event_setup_out) 
	{
		//printk("ep0_complete: length = %d\n", req->length);
		uvc->event_setup_out = 0;
		complete_all(&uvc->uvc_receive_ep0_completion);
	}
}

static int
uvc_send_ep0_data(struct uvc_device *uvc, struct uvc_ep0_data_t *data)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request *req = uvc->control_req;

    if (uvc->state == UVC_STATE_DISCONNECTED)
    {
        printk("device disconnected\n");
        return -1;
    }

	if (data->length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	req->length = min_t(unsigned int, uvc->event_length, data->length);
	req->zero = data->length < uvc->event_length;

	memcpy(req->buf, data->data, req->length);

	req->complete = uvc_ep0_data_complete;
	return usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
}

static int
uvc_receive_ep0_data(struct uvc_device *uvc, struct uvc_ep0_data_t *data)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request *req = uvc->control_req;
	//ssize_t status = -ENOMEM;
	__s32 length;
	int ret;

    if (uvc->state == UVC_STATE_DISCONNECTED)
    {
        printk("device disconnected\n");
        return -1;
    }

	length = data->length;	
	if (length < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return 0;
	}
	
	req->length = min_t(unsigned int, uvc->event_length, length);
	req->zero = length < uvc->event_length;
	
	uvc->uvc_receive_ep0_completion.done = 0;
	req->complete = uvc_ep0_data_complete;

	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		complete_all(&uvc->uvc_receive_ep0_completion);
		return 0;
	}
	wait_for_completion_interruptible(&uvc->uvc_receive_ep0_completion);
	
	length = min_t(unsigned int, req->length, length);
	memcpy(data->data, req->buf, length);
	
	return length;
}

/* --------------------------------------------------------------------------
 * V4L2 ioctls
 */

struct uvc_format
{
	u8 bpp;
	u32 fcc;
};

static struct uvc_format uvc_formats[] = {
	//{ 16, V4L2_PIX_FMT_YUYV  },
	{ 0, V4L2_PIX_FMT_YUYV  },//由应用程序设置
	{ 0,  V4L2_PIX_FMT_MJPEG },
	{ 0,  V4L2_PIX_FMT_H264 },
	//{ 12, V4L2_PIX_FMT_NV12  },
	{ 0, V4L2_PIX_FMT_NV12  },//由应用程序设置
//Ron_Lee 20180322

	//{ 0, V4L2_PIX_FMT_H265  },
//
};

static int
uvc_v4l2_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct usb_composite_dev *cdev = uvc->func.config->cdev;

	strlcpy(cap->driver, "g_uvc", sizeof(cap->driver));
	strlcpy(cap->card, cdev->gadget->name, sizeof(cap->card));
	strlcpy(cap->bus_info, dev_name(&cdev->gadget->dev),
		sizeof(cap->bus_info));

#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)	
	cap->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
#else
	cap->capabilities = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
#endif

	return 0;
}

static int
uvc_v4l2_get_format(struct file *file, void *fh, struct v4l2_format *fmt)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	fmt->fmt.pix.pixelformat = video->fcc;
	fmt->fmt.pix.width = video->width;
	fmt->fmt.pix.height = video->height;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = video->bpp * video->width / 8;
	fmt->fmt.pix.sizeimage = video->imagesize;
	fmt->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	fmt->fmt.pix.priv = 0;

	return 0;
}

static int
uvc_v4l2_set_format(struct file *file, void *fh, struct v4l2_format *fmt)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	struct uvc_format *format;
	unsigned int imagesize;
	unsigned int bpl;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(uvc_formats); ++i) {
		format = &uvc_formats[i];
		if (format->fcc == fmt->fmt.pix.pixelformat)
			break;
	}

	if (i == ARRAY_SIZE(uvc_formats)) {
		printk(KERN_INFO "Unsupported format 0x%08x.\n",
			fmt->fmt.pix.pixelformat);
		return -EINVAL;
	}

	bpl = format->bpp * fmt->fmt.pix.width / 8;
	imagesize = bpl ? bpl * fmt->fmt.pix.height : fmt->fmt.pix.sizeimage;

	video->fcc = format->fcc;
	video->bpp = format->bpp;
	video->width = fmt->fmt.pix.width;
	video->height = fmt->fmt.pix.height;
	video->imagesize = imagesize;

	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = bpl;
	fmt->fmt.pix.sizeimage = imagesize;
	fmt->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	fmt->fmt.pix.priv = 0;

	return 0;
}

static int
uvc_v4l2_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;
#ifdef CONFIG_ARCH_SSTAR
	int i;
#endif

	if (b->type != video->queue.queue.type)
		return -EINVAL;

	v4l2_ioctl_mutex_lock(video);
	if(b->count == 0)
	{
		if(video->reqbufs_addr.ppVirtualAddress)
		{
#ifdef CONFIG_ARCH_SSTAR
			for(i= 0; i < video->reqbufs_addr.count; i++)
			{
				if(video->reqbufs_addr.ppVirtualAddress[i])
				{
					UvcCamOsMemUnmap(video->reqbufs_addr.ppVirtualAddress[i], video->reqbufs_addr.length[i]);
				}
			}	
#endif
			kfree(video->reqbufs_addr.ppVirtualAddress);
			video->reqbufs_addr.ppVirtualAddress = 0;
		}

		if(video->reqbufs_addr.addr)
		{
			kfree(video->reqbufs_addr.addr);
			video->reqbufs_addr.addr = 0;
		}
		if(video->reqbufs_addr.length)
		{
			kfree(video->reqbufs_addr.length);
			video->reqbufs_addr.length = 0;
		}

		video->reqbufs_addr.count = 0;

		if(video->reqbufs_buf_used.buf_used)
		{
			kfree(video->reqbufs_buf_used.buf_used);
            video->reqbufs_buf_used.buf_used = NULL;
		}
		video->reqbufs_buf_used.count = 0;
	}
	else
	{
		video->reqbufs_addr.count = b->count;
		
		video->reqbufs_addr.addr = kzalloc(b->count * sizeof(unsigned long long), GFP_KERNEL);
		if(video->reqbufs_addr.addr == NULL)
		{
			printk("uvc_v4l2_reqbufs kzalloc error\n");
			v4l2_ioctl_mutex_unlock(video);
			return -EINVAL;

		}
		video->reqbufs_addr.length = kzalloc(b->count * sizeof(__u32), GFP_KERNEL);
		if(video->reqbufs_addr.length == NULL)
		{
			printk("uvc_v4l2_reqbufs kzalloc error\n");
			v4l2_ioctl_mutex_unlock(video);
			return -EINVAL;
		}

		video->reqbufs_addr.ppVirtualAddress = kzalloc(b->count * sizeof(void *), GFP_KERNEL);
		if(video->reqbufs_addr.ppVirtualAddress == NULL)
		{
			printk("uvc_v4l2_reqbufs kzalloc error\n");
			v4l2_ioctl_mutex_unlock(video);
			return -EINVAL;
		}

		video->reqbufs_buf_used.count = b->count;
		video->reqbufs_buf_used.buf_used = kzalloc(b->count * sizeof(__u32), GFP_KERNEL);
		if(video->reqbufs_buf_used.buf_used == NULL)
		{
			printk("uvc_v4l2_reqbufs kzalloc error\n");
			v4l2_ioctl_mutex_unlock(video);
			return -EINVAL;
		}
	}	

	ret = uvcg_alloc_buffers(&video->queue, b);
	v4l2_ioctl_mutex_unlock(video);

	return ret;
}

static int
uvc_v4l2_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;

	v4l2_ioctl_mutex_lock(video);
	ret =  uvcg_query_buffer(&video->queue, b);
	v4l2_ioctl_mutex_unlock(video);

	return ret;
}

static int
uvc_v4l2_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;

	if (video->ep->enabled == 0 || uvc->state != UVC_STATE_STREAMING)
	{
		return -1;
	}

	v4l2_ioctl_mutex_lock(video);
	
	ret = uvcg_queue_buffer(&video->queue, b);
	if (ret < 0)
	{
		//WARN_ON(1);
		printk("uvcg_queue_buffer error !!!, ret = %d\n", ret);
		v4l2_ioctl_mutex_unlock(video);
		return ret;
	}

	if(uvc->state != UVC_STATE_STREAMING)
	{
		v4l2_ioctl_mutex_unlock(video);
		return ret;
	}

	if(video->work_pump_enable)
	{
		queue_work(video->async_wq, &video->pump);
	}
	else
	{
		ret = uvcg_video_pump(video);
	}
	
	v4l2_ioctl_mutex_unlock(video);

	return ret;
}

static int
uvc_v4l2_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;

	if (video->ep->enabled == 0 || uvc->state != UVC_STATE_STREAMING)
	{
		return -1;
	}
	
	v4l2_ioctl_mutex_lock(video);
	ret = uvcg_dequeue_buffer(&video->queue, b, file->f_flags & O_NONBLOCK);
	v4l2_ioctl_mutex_unlock(video);

	return ret;
}

static int
uvc_v4l2_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	int ret;

	if (type != video->queue.queue.type)
	{
		ret = -EINVAL;
		goto error;
	}

	if (video->ep->enabled == 0 && !video->bulk_streaming_ep)
	{
		printk("video ep not enabled\n");
		ret = -EINVAL;
		goto error;
	}

	v4l2_ioctl_mutex_lock(video);

    if(video->uvc->work_pump_enable == 2)
    {
        video->work_pump_enable = 1;//强制设置
    }
    else if (video->reqbufs_config.enable)
		video->work_pump_enable = 0;
	else 
		video->work_pump_enable = video->uvc->work_pump_enable;
	
	/* Enable UVC video. */
	ret = uvcg_video_enable(video, 1);
	if (ret < 0)
	{
		v4l2_ioctl_mutex_unlock(video);
		goto error;
	}

	/*
	 * Complete the alternate setting selection setup phase now that
	 * userspace is ready to provide video frames.
	 */
#if (!defined CHIP_CV5) && (!defined CHIP_CV72)
	if (!video->bulk_streaming_ep) {
		uvc_function_setup_continue(uvc);
	}
#endif

	uvc->state = UVC_STATE_STREAMING;

	v4l2_ioctl_mutex_unlock(video);
	return 0;

error:
#if (!defined CHIP_CV5) && (!defined CHIP_CV72)
	if (!video->bulk_streaming_ep)
	{
		uvc_function_setup_continue(uvc);
	}
#endif
	return ret;
}

static int
uvc_v4l2_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	int code = 0;
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;

	if (type != video->queue.queue.type)
		return -EINVAL;

	v4l2_ioctl_mutex_lock(video);
	
	code = uvcg_video_enable(video, 0);

	//if (video->bulk_streaming_ep)
		uvc->state = UVC_STATE_CONNECTED;

	v4l2_ioctl_mutex_unlock(video);
	
	return code;
}

static int
uvc_v4l2_subscribe_event(struct v4l2_fh *fh,
			 const struct v4l2_event_subscription *sub)
{
	if (sub->type < UVC_EVENT_FIRST || sub->type > UVC_EVENT_LAST)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, UVC_MAX_EVENTS, NULL);
}

static int
uvc_v4l2_unsubscribe_event(struct v4l2_fh *fh,
			   const struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}


extern int get_usb_link_state(void);

static long
uvc_v4l2_ioctl_default(struct file *file, void *fh, bool valid_prio,
		       unsigned int cmd, void *arg)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_video *video = &uvc->video;
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct uvc_ioctl_usb_sn_string_t *usb_sn_string;
	struct uvc_ioctl_usb_vendor_string_t *usb_vendor_string;
	struct uvc_ioctl_usb_product_string_t *usb_product_string;
	struct uvc_ioctl_reqbufs_config_t *reqbufs_config;
	struct uvc_ioctl_reqbufs_addr_t *reqbufs_addr;
	struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used;

	switch (cmd) {
	case UVCIOC_SEND_RESPONSE:
		return uvc_send_response(uvc, arg);
	case UVCIOC_SEND_EP0_DATA:
		return uvc_send_ep0_data(uvc, arg);
	case UVCIOC_RECEIVE_EP0_DATA:
		return uvc_receive_ep0_data(uvc, arg);	
	case UVCIOC_SEND_STATUS:
		return uvc_send_status(uvc, arg);
		
	case UVCIOC_USB_CONNECT_STATE:{
		int *type = arg;
		
		if (*type == USB_STATE_CONNECT_RESET ){
			//printk("%s +%d %s:set usb connect reset\n", __FILE__,__LINE__,__func__);

			usb_gadget_disconnect(cdev->gadget);
			if(usb_gadget_connect(cdev->gadget) != 0)
				printk("usb_gadget_connect error!\n");		
			return 0;
		}else if(*type == USB_STATE_DISCONNECT){
			//printk("%s +%d %s:set usb disconnect\n", __FILE__,__LINE__,__func__);

			usb_gadget_disconnect(cdev->gadget);
			
			return 0;
			
		}else if(*type == USB_STATE_CONNECT){
			//printk("%s +%d %s:set usb connect\n", __FILE__,__LINE__,__func__);

			if(usb_gadget_connect(cdev->gadget) != 0)
				printk("usb_gadget_connect error!\n");
			return 0;
		}
		else
			return -EINVAL;
	}
#ifdef USB_CHIP_DWC3	
	case UVCIOC_GET_USB_SOFT_CONNECT_STATE:{	
		uint32_t *usb_soft_connect_state = arg;

		*usb_soft_connect_state = get_usb_soft_connect_state();	
		return 0;
	}
	
	case UVCIOC_SET_USB_SOFT_CONNECT_CTRL:{
		uint32_t *value = arg;
		
		set_usb_soft_connect_ctrl((uint32_t)*value);		
		return 0;
	}
	
	case UVCIOC_GET_USB_SOFT_CONNECT_CTRL:{
		uint32_t *value = arg;
		
		*value = get_usb_soft_connect_ctrl();
		return 0;
	}
#endif

	case UVCIOC_SET_SIMULCAST_ENABLE:{
		uint32_t *value = arg;
		
		uvc->video.simulcast_enable = (uint32_t)*value;
		printk("%s +%d %s: set simulcast enable:%d\n", __FILE__,__LINE__,__func__,uvc->video.simulcast_enable);
		return 0;
	}

	case UVCIOC_SET_WINDOWS_HELLO_ENABLE:{
		uint32_t *value = arg;
		
		uvc->video.win_hello_enable = (uint32_t)*value;
		printk("%s +%d %s: set win_hello enable:%d\n", __FILE__,__LINE__,__func__,uvc->video.win_hello_enable);
		return 0;
	}

	case UVCIOC_SET_UVC_PTS_COPY:{
		uint32_t *value = arg;
		
		uvc->video.uvc_pts_copy = (uint32_t)*value;
		return 0;
	}
	
	case UVCIOC_GET_UVC_CONFIG:
	{
		struct uvc_ioctl_uvc_config_t *uvc_config = (struct uvc_ioctl_uvc_config_t *)arg;

		uvc_config->bConfigurationValue = uvc->func.config->bConfigurationValue;
		uvc_config->bulk_streaming_ep = uvc->video.bulk_streaming_ep;
		uvc_config->control_intf = uvc->control_intf;
		uvc_config->streaming_intf = uvc->streaming_intf;

		if(uvc->video.ep)
		{
			uvc_config->streaming_ep_address = uvc->video.ep->address;
		}		
		else
		{
			uvc_config->streaming_ep_address = 0;
		}
			
		if(uvc->uvc_status_interrupt_ep)
		{
			uvc_config->status_interrupt_ep_address = uvc->uvc_status_interrupt_ep->address;
		}	
		else
		{
			uvc_config->status_interrupt_ep_address = 0;
		}

		uvc_config->dfu_enable = uvc->dfu_enable;
		uvc_config->uac_enable = uvc->uac_enable;
		uvc_config->uvc_s2_enable = uvc->uvc_s2_enable;
		uvc_config->uvc_v150_enable = uvc->uvc_v150_enable;
		uvc_config->win7_usb3_enable = uvc->win7_usb3_enable;
		uvc_config->zte_hid_enable = uvc->zte_hid_enable;
		uvc_config->bulk_max_size = uvc->video.bulk_max_size;
		
		return 0;
	}


	case UVCIOC_SET_UVC_DESCRIPTORS:
	{
		struct uvc_ioctl_desc_t 	*desc = (struct uvc_ioctl_desc_t *)arg;
		struct usb_descriptor_header 	**dst;
		void *mem;
		unsigned int n_desc_length;
		unsigned int  descriptors_length;

		if(desc->desc_buf_length > 0 && desc->desc_buf != NULL)
		{
			n_desc_length = (desc->n_desc + 1) * sizeof(const struct usb_descriptor_header *);
			descriptors_length = n_desc_length + desc->desc_buf_length; 

			//printk("desc_length = %d\n", desc->desc_length);


			switch (desc->speed) {
			case USB_SPEED_SUPER_PLUS:
	
				if(uvc->func.ssp_descriptors)
				{
					usb_free_descriptors(uvc->func.ssp_descriptors);
				}
				
				mem = kzalloc(descriptors_length, GFP_KERNEL);
				if(mem == NULL)
					return -1;
					
				dst = mem;
				uvc->func.ssp_descriptors = mem;
				mem += n_desc_length;
				break;
			case USB_SPEED_SUPER:
	
				if(uvc->func.ss_descriptors)
				{
					usb_free_descriptors(uvc->func.ss_descriptors);
				}
				
				mem = kzalloc(descriptors_length, GFP_KERNEL);
				if(mem == NULL)
					return -1;
					
				dst = mem;
				uvc->func.ss_descriptors = mem;
				mem += n_desc_length;
				break;
			case USB_SPEED_HIGH:
				if(uvc->func.hs_descriptors)
				{
					usb_free_descriptors(uvc->func.hs_descriptors);
				}
				
				mem = kzalloc(descriptors_length, GFP_KERNEL);
				if(mem == NULL)
					return -1;

				dst = mem;
				uvc->func.hs_descriptors = mem;
				mem += n_desc_length;
				break;
			case USB_SPEED_FULL:
			default:
				if(uvc->func.fs_descriptors)
				{
					usb_free_descriptors(uvc->func.fs_descriptors);
				}
				
				mem = kzalloc(descriptors_length, GFP_KERNEL);
				if(mem == NULL)
					return -1;
				
				dst = mem;
				uvc->func.fs_descriptors = mem;
				mem += n_desc_length;
				break;
			}	

#ifdef CONFIG_ARCH_SSTAR
			if(desc->desc_buf_phy_addr && desc->desc_buf_phy_length > 0)
			{
				void* pVirtPtr;
				pVirtPtr = UvcCamOsMemMap(desc->desc_buf_phy_addr, desc->desc_buf_phy_length, 0);
				if(pVirtPtr == NULL)
				{
					printk("CamOsMemMap Fail, phy_addr:0x%llx\n",desc->desc_buf_phy_addr);
					return -1;
				}
				UVC_COPY_DESCRIPTORS_FROM_BUF(mem, dst, pVirtPtr, desc->desc_buf_length);
				UvcCamOsMemUnmap(pVirtPtr, desc->desc_buf_phy_length);
			}
			else
			{
				UVC_COPY_DESCRIPTORS_FROM_BUF(mem, dst, desc->desc_buf, desc->desc_buf_length);
			}		
#else
			if(desc->desc_buf_phy_addr && desc->desc_buf_phy_length > 0)
			{
				unsigned char *temp_buf =  ioremap(desc->desc_buf_phy_addr, desc->desc_buf_phy_length);
				UVC_COPY_DESCRIPTORS_FROM_BUF(mem, dst, temp_buf, desc->desc_buf_length);
				iounmap(temp_buf);
			}
			else
			{
				UVC_COPY_DESCRIPTORS_FROM_BUF(mem, dst, desc->desc_buf, desc->desc_buf_length);
			}
#endif

		}

		return 0;
	}

	case UVCIOC_SET_UVC_DESC_BUF_MODE:
	{
		struct uvc_ioctl_desc_buf_t 	*desc = (struct uvc_ioctl_desc_buf_t *)arg;

		if(desc->buf_length > UVC_DESC_BUF_LEN || desc->buf_length > desc->total_length)
		{
			return -1;
		}

		switch (desc->speed) {
		case USB_SPEED_SUPER_PLUS:
			if(desc->mem == NULL)
			{
				if(uvc->func.ssp_descriptors)
				{
					usb_free_descriptors(uvc->func.ssp_descriptors);
					uvc->func.ssp_descriptors = NULL;
				}
				
				uvc->func.ssp_descriptors = kzalloc(desc->total_length, GFP_KERNEL);
				if(uvc->func.ssp_descriptors == NULL)
					return -1;

				desc->mem = uvc->func.ssp_descriptors;
				return 0;		
			}
			break;
		case USB_SPEED_SUPER:
			if(desc->mem == NULL)
			{
				if(uvc->func.ss_descriptors)
				{
					usb_free_descriptors(uvc->func.ss_descriptors);
					uvc->func.ss_descriptors = NULL;
				}
				
				uvc->func.ss_descriptors = kzalloc(desc->total_length, GFP_KERNEL);
				if(uvc->func.ss_descriptors == NULL)
					return -1;

				desc->mem = uvc->func.ss_descriptors;
				return 0;		
			}
			break;
		case USB_SPEED_HIGH:
			if(desc->mem == NULL)
			{
				if(uvc->func.hs_descriptors)
				{
					usb_free_descriptors(uvc->func.hs_descriptors);
					uvc->func.hs_descriptors = NULL;
				}
				
				uvc->func.hs_descriptors = kzalloc(desc->total_length, GFP_KERNEL);
				if(uvc->func.hs_descriptors == NULL)
					return -1;
				
				desc->mem = uvc->func.hs_descriptors;
				return 0;
			}
			break;
		case USB_SPEED_FULL:
		default:
			if(desc->mem == NULL)
			{
				if(uvc->func.fs_descriptors)
				{
					usb_free_descriptors(uvc->func.fs_descriptors);
					uvc->func.fs_descriptors = NULL;
				}
				
				uvc->func.fs_descriptors = kzalloc(desc->total_length, GFP_KERNEL);
				if(uvc->func.fs_descriptors == NULL)
					return -1;
				
				desc->mem = uvc->func.fs_descriptors;
				return 0;
			}
			break;
		}
		memcpy(desc->mem + desc->mem_offset, desc->buf, desc->buf_length);
		return 0;
	}
	
	case UVCIOC_SET_USB_SN:
		usb_sn_string = (struct uvc_ioctl_usb_sn_string_t *)arg;
		if(usb_sn_string)
		{
			
			if(uvc->usb_sn_string && usb_sn_string->string_buff_length > 0 \
				&& usb_sn_string->string_buff_length <= uvc->usb_sn_string_length)
			{

				printk("+%d , usb_sn_string : %s\n",__LINE__,usb_sn_string->string);
				memset(uvc->usb_sn_string, 0, uvc->usb_sn_string_length);	
				memcpy(uvc->usb_sn_string, usb_sn_string->string, usb_sn_string->string_buff_length);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}			
		return 0;
	case UVCIOC_SET_USB_VENDOR_STR:
		usb_vendor_string = (struct uvc_ioctl_usb_vendor_string_t *)arg;
		if(usb_vendor_string)
		{
			
			if(uvc->usb_vendor_string && usb_vendor_string->string_buff_length > 0 \
				&& usb_vendor_string->string_buff_length <= uvc->usb_vendor_string_length)
			{

				printk("+%d , usb_vendor_string : %s\n",__LINE__,usb_vendor_string->string);
				memset(uvc->usb_vendor_string, 0, uvc->usb_vendor_string_length);	
				memcpy(uvc->usb_vendor_string, usb_vendor_string->string, usb_vendor_string->string_buff_length);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}			
		return 0;
	case UVCIOC_SET_USB_PRODUCT_STR:
		usb_product_string = (struct uvc_ioctl_usb_product_string_t *)arg;
		if(usb_product_string)
		{
			
			if(uvc->usb_product_string && usb_product_string->string_buff_length > 0 \
				&& usb_product_string->string_buff_length <= uvc->usb_product_string_length)
			{

				printk("+%d , usb_product_string : %s\n",__LINE__,usb_product_string->string);
				memset(uvc->usb_product_string, 0, uvc->usb_product_string_length);	
				memcpy(uvc->usb_product_string, usb_product_string->string, usb_product_string->string_buff_length);
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}			
		return 0;
	case UVCIOC_GET_USB_LINK_STATE:
		*((int *)arg) = get_usb_link_state(); 
		return 0;
	case UVCIOC_SET_REQBUFS_CONFIG:
		reqbufs_config = (struct uvc_ioctl_reqbufs_config_t *)arg;
		if(reqbufs_config)
		{
			memcpy(&uvc->video.reqbufs_config, reqbufs_config, sizeof(struct uvc_ioctl_reqbufs_config_t));
			//printk("+%d , enable:%d, cache_enable:%d, offset:%d\n",__LINE__,
			//	uvc->video.reqbufs_config.enable, uvc->video.reqbufs_config.cache_enable,
			//	uvc->video.reqbufs_config.data_offset);
		}
		else
		{
			return -1;
		}			
		return 0;
	case UVCIOC_SET_REQBUFS_ADDR:
		reqbufs_addr = (struct uvc_ioctl_reqbufs_addr_t *)arg;
		if(reqbufs_addr)
		{
			v4l2_ioctl_mutex_lock(video);
			if(uvc->video.reqbufs_addr.count == 0)
			{
				printk("+%d %s: count = 0\n", __LINE__,__func__);
				v4l2_ioctl_mutex_unlock(video);
				return -1;
			}

			if(reqbufs_addr->index >= uvc->video.reqbufs_addr.count)
			{
				printk("+%d %s: index %d > count %d\n", __LINE__,__func__, reqbufs_addr->index, uvc->video.reqbufs_addr.count);
				v4l2_ioctl_mutex_unlock(video);
				return -1;
			}

			if(uvc->video.reqbufs_addr.addr == NULL ||
				uvc->video.reqbufs_addr.length == NULL)
			{
				printk("+%d %s: buf is NULL\n", __LINE__,__func__);
				v4l2_ioctl_mutex_unlock(video);
				return -1;
			}
			
			
			uvc->video.reqbufs_addr.length[reqbufs_addr->index] = reqbufs_addr->length;
            if(uvc->video.cma_dma_buf_enable)
            {
                uvc->video.reqbufs_addr.addr[reqbufs_addr->index] = reqbufs_addr->addr;               
                uvc->video.reqbufs_addr.ppVirtualAddress[reqbufs_addr->index] = cma_dma_buf_kvirt_getby_phys(video->cma_dev, reqbufs_addr->addr);
            }
            else
            {
#ifdef CONFIG_ARCH_SSTAR
                uvc->video.reqbufs_addr.addr[reqbufs_addr->index] = Chip_MIU_to_Phys(reqbufs_addr->addr);
                uvc->video.reqbufs_addr.ppVirtualAddress[reqbufs_addr->index] = UvcCamOsMemMap(reqbufs_addr->addr, reqbufs_addr->length, 0);
#else
                uvc->video.reqbufs_addr.addr[reqbufs_addr->index] = reqbufs_addr->addr;
#endif 
            }
			// printk("index:%d, addr:0x%llx, length:%d, p:0x%p\n",
			// 	reqbufs_addr->index,uvc->video.reqbufs_addr.addr[reqbufs_addr->index],
			// 	uvc->video.reqbufs_addr.length[reqbufs_addr->index],
			// 	uvc->video.reqbufs_addr.ppVirtualAddress[reqbufs_addr->index]);	

			v4l2_ioctl_mutex_unlock(video);
			//printk("+%d , index:%d, addr:0x%llx, length:%d\n",__LINE__,
			//	reqbufs_addr->index, reqbufs_addr->addr,
			//	reqbufs_addr->length);


		}
		else
		{
			return -1;
		}			
		return 0;

	case UVCIOC_SET_REQBUFS_BUF_USED:
		if (video->ep->enabled == 0  || (uvc->state != UVC_STATE_STREAMING))
		{
			//printk("+%d %s: stream off\n", __LINE__,__func__);
			return 0;
		}

		reqbufs_buf_used = (struct uvc_ioctl_reqbufs_buf_used_t *)arg;
		if(reqbufs_buf_used)
		{
			v4l2_ioctl_mutex_lock(video);
			if(uvc->video.reqbufs_buf_used.count == 0)
			{
				printk("+%d %s: count = 0\n", __LINE__,__func__);
				v4l2_ioctl_mutex_unlock(video);
				return -1;
			}

			if(reqbufs_buf_used->index >= uvc->video.reqbufs_buf_used.count)
			{
				printk("+%d %s: index %d > count %d\n", __LINE__,__func__, reqbufs_buf_used->index, uvc->video.reqbufs_buf_used.count);
				v4l2_ioctl_mutex_unlock(video);
				return -1;
			}

			if(uvc->video.reqbufs_buf_used.buf_used == NULL)
			{
				printk("+%d %s: buf is NULL\n", __LINE__,__func__);
				v4l2_ioctl_mutex_unlock(video);
				return -1;
			}
			
			uvc->video.reqbufs_buf_used.buf_used[reqbufs_buf_used->index] = reqbufs_buf_used->buf_used;
			
			//printk("+%d , index:%d, buf_used:%d\n",__LINE__,
			//	reqbufs_buf_used->index, reqbufs_buf_used->buf_used);

			v4l2_ioctl_mutex_unlock(video);
		}
		else
		{
			return -1;
		}			
		return 0;	
    case UVCIOC_CMA_DMA_BUF_ENABLE:
    {
        uint32_t *value = arg;
        uvc->video.cma_dma_buf_enable = (uint32_t)*value;
        return 0;
    }
    case UVCIOC_CMA_DMA_BUF_ALLOC:
    {
        unsigned long long phys_addr = 0;
        struct uvc_ioctl_cma_dma_buf *cma_dma_buf = (struct uvc_ioctl_cma_dma_buf *)arg;

        phys_addr = cma_dma_buf_alloc(video->cma_dev, cma_dma_buf->length, GFP_KERNEL);
        if(phys_addr == 0)
        {
            pr_err("uvc dma_alloc_coherent failed.\n");
            return -1;
        }
        cma_dma_buf->phys_addr = phys_addr;
       
        return 0;
    }
    case UVCIOC_CMA_DMA_BUF_FREE:
    {
        struct uvc_ioctl_cma_dma_buf *cma_dma_buf = (struct uvc_ioctl_cma_dma_buf *)arg;
        if(cma_dma_buf->phys_addr == 0)
        {
            return -1;
        }

        cma_dma_buf_free(video->cma_dev, cma_dma_buf->phys_addr);
        return 0;
    }
	default:
		printk("%s +%d %s: cmd = 0x%x\n", __FILE__,__LINE__,__func__,cmd);
		return -ENOIOCTLCMD;
	}
}

const struct v4l2_ioctl_ops uvc_v4l2_ioctl_ops = {
	.vidioc_querycap = uvc_v4l2_querycap,
	.vidioc_g_fmt_vid_out = uvc_v4l2_get_format,
	.vidioc_s_fmt_vid_out = uvc_v4l2_set_format,
	.vidioc_reqbufs = uvc_v4l2_reqbufs,
	.vidioc_querybuf = uvc_v4l2_querybuf,
	.vidioc_qbuf = uvc_v4l2_qbuf,
	.vidioc_dqbuf = uvc_v4l2_dqbuf,
	.vidioc_streamon = uvc_v4l2_streamon,
	.vidioc_streamoff = uvc_v4l2_streamoff,
	.vidioc_subscribe_event = uvc_v4l2_subscribe_event,
	.vidioc_unsubscribe_event = uvc_v4l2_unsubscribe_event,
	.vidioc_default = uvc_v4l2_ioctl_default,
};

/* --------------------------------------------------------------------------
 * V4L2
 */

static int
uvc_v4l2_open(struct file *file)
{
    struct video_device *vdev = video_devdata(file);
    struct uvc_device *uvc = video_get_drvdata(vdev);
    struct uvc_file_handle *handle;
    //struct usb_composite_dev *cdev = uvc->func.config->cdev;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
		return -ENOMEM;

	v4l2_fh_init(&handle->vfh, vdev);
	v4l2_fh_add(&handle->vfh);

	handle->device = &uvc->video;
	file->private_data = &handle->vfh;

	//uvc_function_connect(uvc);
	return 0;
}

static int
uvc_v4l2_release(struct file *file)
{
	//struct video_device *vdev = video_devdata(file);
	//struct uvc_device *uvc = video_get_drvdata(vdev);
	struct uvc_file_handle *handle = to_uvc_file_handle(file->private_data);
	//struct uvc_video *video = handle->device;
#ifdef CONFIG_ARCH_SSTAR	
	//int i;
#endif

	if (IS_ERR_OR_NULL(handle))
	{
		dump_stack();
		return PTR_ERR(handle);
	}
	

    file->private_data = NULL;
    v4l2_fh_del(&handle->vfh);
    v4l2_fh_exit(&handle->vfh);
    kfree(handle);

	return 0;
}

static int
uvc_v4l2_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	return uvcg_queue_mmap(&uvc->video.queue, vma);
}

static unsigned int
uvc_v4l2_poll(struct file *file, poll_table *wait)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	return uvcg_queue_poll(&uvc->video.queue, file, wait);
}

#ifndef CONFIG_MMU
static unsigned long uvcg_v4l2_get_unmapped_area(struct file *file,
		unsigned long addr, unsigned long len, unsigned long pgoff,
		unsigned long flags)
{
	struct video_device *vdev = video_devdata(file);
	struct uvc_device *uvc = video_get_drvdata(vdev);

	return uvcg_queue_get_unmapped_area(&uvc->video.queue, pgoff);
}
#endif

struct v4l2_file_operations uvc_v4l2_fops = {
	.owner		= THIS_MODULE,
	.open		= uvc_v4l2_open,
	.release	= uvc_v4l2_release,
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
	.unlocked_ioctl		= video_ioctl2,
#else
	.ioctl		= video_ioctl2,
#endif
	.mmap		= uvc_v4l2_mmap,
	.poll		= uvc_v4l2_poll,
#ifndef CONFIG_MMU
	.get_unmapped_area = uvcg_v4l2_get_unmapped_area,
#endif
};

