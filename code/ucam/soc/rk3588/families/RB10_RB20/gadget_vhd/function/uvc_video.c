/*
 *	uvc_video.c  --  USB Video Class Gadget driver
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
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>

#include <media/v4l2-dev.h>

#include <linux/version.h>
#include <asm/cacheflush.h>

#include <linux/timekeeping.h>

#include "uvc.h"
#include "uvc_queue.h"
#include "vhd_usb_id.h"
#include "uvc_v4l2.h"

//#define PACKAGE_TEST

extern uint16_t get_vhd_usb_vid(void);
extern uint16_t get_vhd_usb_pid(void);
extern int get_usb_link_state(void);
int usb_id_check_fail = 0;
uint32_t test_cnt;

static int check_usb_id(uint16_t vid, uint16_t pid)
{
	int i;

	for(i = 0; i < (sizeof(usb_id_list))/4; i ++)
	{	
		if(usb_id_list[i][0] == vid)
		{
			if(usb_id_list[i][1] == pid || usb_id_list[i][1] == 0xffff)
			{
				return 1;
			}
		}	
	}

	printk("vid:0x%x, pid:0x%x\n", vid,pid);
	return 0;
}

/* --------------------------------------------------------------------------
 * Video codecs 祖传代码请勿乱动！
 */

#ifdef PACKAGE_TEST
static uint32_t package_index = 0;
static uint32_t frame_index = 0;
#endif

static void update_uvc_header_timestamp(
	struct uvc_video *video,
	struct uvc_buffer *buf)
{
	u64 result;
	if(video->queue.buf_used == 0)
	{
		//一帧的开始,UVC时间戳的单位为us(时钟:1M)
		if(video->uvc_pts_copy == 0)
		{
			//ns-->us
			//video->uvc_pts = (uint32_t)((video->uvc_pts_timestamp) / NSEC_PER_USEC);
			result = video->uvc_pts_timestamp;
			do_div(result,NSEC_PER_USEC);
			video->uvc_pts = (uint32_t)result;
		}
		else
		{
			//ns-->us
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
			//video->uvc_pts = (uint32_t)((buf->buf.vb2_buf.timestamp) / NSEC_PER_USEC);
			result = buf->buf.vb2_buf.timestamp;
			do_div(result,NSEC_PER_USEC);
			video->uvc_pts = (uint32_t)result;
#else     
			video->uvc_pts = (uint32_t)((s64)buf->buf.v4l2_buf.timestamp.tv_sec * USEC_PER_SEC + buf->buf.v4l2_buf.timestamp.tv_usec);
#endif
		}
		video->uvc_scr = (uint32_t)uvc_get_timestamp_us();

		//printk("uvc pts:%d, src:%d, diff:%d\n",video->uvc_pts,video->uvc_scr,video->uvc_scr - video->uvc_pts);
	}
}

static int
uvc_video_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct usb_gadget *gadget = video->uvc->func.config->cdev->gadget;
	struct uvc_payload_header_t *uvc_header = (struct uvc_payload_header_t *)data;

	uvc_header->length = UVC_VIDEO_HEADER_LEN;
	
#ifdef HEADER_SCR_PTS	//去掉包头的其他部分，否则盒子收不到数据而且会死机	
#ifndef PACKAGE_TEST
	update_uvc_header_timestamp(video, buf);
	uvc_header->info = UVC_STREAM_SCR | UVC_STREAM_PTS | video->fid;
	uvc_header->pts = video->uvc_pts;
	uvc_header->scr = video->uvc_scr;
	uvc_header->stc = USB_SOF_TO_UVC_STC(gadget->ops->get_frame(gadget));

#else
	uvc_header->info = UVC_STREAM_EOH | video->fid;
	uvc_header->pts = frame_index;
	uvc_header->scr = package_index;
	uvc_header->stc = 0;
	package_index++;
#endif
#else
	uvc_header->info = UVC_STREAM_EOH | video->fid;
#endif /* #ifdef HEADER_SCR_PTS */

	if (buf->bytesused != 0 && (buf->bytesused - video->queue.buf_used <= len - UVC_VIDEO_HEADER_LEN))
	{
		//memcpy(video->temp_header, data, UVC_VIDEO_HEADER_LEN);
		uvc_header->info |= UVC_STREAM_EOF;

#if defined HEADER_SCR_PTS && defined PACKAGE_TEST
		frame_index ++;
		package_index = 0;
#endif
	}

	return UVC_VIDEO_HEADER_LEN;
}

static int
uvc_video_simulcast_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct usb_gadget *gadget = video->uvc->func.config->cdev->gadget;
	struct uvc_payload_header_simulcast_t *uvc_header = (struct uvc_payload_header_simulcast_t *)data;

	uvc_header->length = UVC_VIDEO_SIMULCAST_HEADER_LEN;

#ifdef HEADER_SCR_PTS	//去掉包头的其他部分，否则盒子收不到数据而且会死机
	update_uvc_header_timestamp(video, buf);
	uvc_header->info = UVC_STREAM_SCR | UVC_STREAM_PTS | video->fid;
	uvc_header->pts = video->uvc_pts;
	uvc_header->scr = video->uvc_scr;
	uvc_header->stc = USB_SOF_TO_UVC_STC(gadget->ops->get_frame(gadget));

	memcpy(&uvc_header->simulcast_id, buf->mem + buf->bytesused + video->reqbufs_config.data_offset, 4);
#else
	uvc_header->info = UVC_STREAM_EOH | video->fid;
	memcpy(data + 3, buf->mem + buf->bytesused + video->reqbufs_config.data_offset, 4);
#endif
	
	
	if (buf->bytesused != 0 && (buf->bytesused - video->queue.buf_used <= len - UVC_VIDEO_SIMULCAST_HEADER_LEN))
	{
		//memcpy(video->simulcast_temp_header, data, UVC_VIDEO_SIMULCAST_HEADER_LEN);
		uvc_header->info |= UVC_STREAM_EOF;
	}
	return UVC_VIDEO_SIMULCAST_HEADER_LEN;
}

static int
uvc_video_win_hello_encode_header(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct usb_gadget *gadget = video->uvc->func.config->cdev->gadget;
	struct uvc_payload_header_win_hello_t *uvc_header = (struct uvc_payload_header_win_hello_t *)data;

	uvc_header->length = UVC_VIDEO_WIN_HELLO_HEADER_LEN;
	
#ifdef HEADER_SCR_PTS	//去掉包头的其他部分，否则盒子收不到数据而且会死机
	update_uvc_header_timestamp(video, buf);
	uvc_header->info = UVC_STREAM_SCR | UVC_STREAM_PTS | video->fid;
	uvc_header->pts = video->uvc_pts;
	uvc_header->scr = video->uvc_scr;
	uvc_header->stc = USB_SOF_TO_UVC_STC(gadget->ops->get_frame(gadget));

	memcpy(uvc_header->metadata, buf->mem + buf->bytesused + video->reqbufs_config.data_offset, 16);
#else
	uvc_header->info = UVC_STREAM_EOH | video->fid;
	memcpy(data + 3, buf->mem + buf->bytesused + video->reqbufs_config.data_offset, 16);
#endif

	if (buf->bytesused != 0 && (buf->bytesused - video->queue.buf_used <= len - UVC_VIDEO_WIN_HELLO_HEADER_LEN))
	{
		//memcpy(video->simulcast_temp_header, data, UVC_VIDEO_WIN_HELLO_HEADER_LEN);
		uvc_header->info |= UVC_STREAM_EOF;
	}
	return UVC_VIDEO_WIN_HELLO_HEADER_LEN;
}

static int
uvc_video_encode_data(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct uvc_video_queue *queue = &video->queue;
	unsigned int nbytes;
	void *mem;

	if (buf->bytesused == 0)
	{
		video->queue.buf_used = 0;
		return 0;
	}

	/* Copy video data to the USB buffer. */
	mem = buf->mem + queue->buf_used;
	nbytes = min((unsigned int)len, buf->bytesused - queue->buf_used);

	memcpy(data, mem, nbytes);
	queue->buf_used += nbytes;

	return nbytes;
}

static void
uvc_video_encode_bulk(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;

	/* Add a header at the beginning of the payload. */
	if (video->payload_size == 0) {
		ret = video->encode_header(video, buf, mem, len);
		video->payload_size += ret;
		mem += ret;
		len -= ret;
	}

	/* Process video data. */
	len = min((int)(video->max_payload_size - video->payload_size), len);
	ret = uvc_video_encode_data(video, buf, mem, len);

	video->payload_size += ret;
	len -= ret;

	req->length = video->req_size - len;
	//req->zero = video->payload_size == video->max_payload_size;
	//一帧的最后一包小于UVC包的大小时发短数据包作为结束
	req->zero = req->length < video->max_payload_size;

#if 0		
	if (buf->bytesused == video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		//if(buf->bytesused != 0)
		video->fid ^= UVC_STREAM_FID;
		if(video->uvc_pts_copy == 0)
		{
			video->uvc_pts_timestamp = uvc_get_timestamp_ns();
		}


		video->payload_size = 0;
	}
#endif

	if (video->payload_size == video->max_payload_size ||
	    buf->bytesused == video->queue.buf_used)
		video->payload_size = 0;
}

static void
uvc_video_encode_isoc(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;

	/* Add the header. */
	ret = video->encode_header(video, buf, mem, len);
	mem += ret;
	len -= ret;

	/* Process video data. */
	ret = uvc_video_encode_data(video, buf, mem, len);
	len -= ret;

	req->length = video->req_size - len;

#if 0
	if (buf->bytesused == video->queue.buf_used) {
		video->queue.buf_used = 0;
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		//if(buf->bytesused != 0)
		video->fid ^= UVC_STREAM_FID;
		if(video->uvc_pts_copy == 0)
		{
			video->uvc_pts_timestamp = uvc_get_timestamp_ns();
		}

	}
#endif
}

/* --------------------------------------------------------------------------
 * DDR to USB encode
 */
static int
uvc_video_encode_data_ddr_to_usb(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len)
{
	struct uvc_video_queue *queue = &video->queue;
	unsigned int nbytes;
	void *mem;

	if (buf->bytesused == 0)
	{
		video->queue.buf_used = 0;
		return 0;
	}
	/* Copy video data to the USB buffer. */
	mem = buf->mem + queue->buf_used + video->reqbufs_config.data_offset;
	nbytes = min((unsigned int)len, buf->bytesused - queue->buf_used);

	memcpy(data, mem, nbytes);
	queue->buf_used += nbytes;

	return nbytes;
}

int check_reqbufs_buf_used(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	if (buf->bytesused == 0)
	{
		video->queue.buf_used = 0;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index] = 0;
#else
		video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index] = 0;
#endif
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		if(buf->bytesused != 0)
		{
			video->fid ^= UVC_STREAM_FID;
			if(video->uvc_pts_copy == 0)
			{
				video->uvc_pts_timestamp = uvc_get_timestamp_ns();
			}
		}
		return -1;
	}	
		
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
	if(video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index] >= buf->bytesused)
	{
		return 0;
	}

	if(video->queue.buf_used + video->req_size - video->uvc_video_header_len > 
		video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index])
	{
		return -1;
	}
#else
	if(video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index] >= buf->bytesused)
	{
		return 0;
	}

	if(video->queue.buf_used + video->req_size - video->uvc_video_header_len > 
		video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index])
	{
		return -1;
	}
#endif

	return 0;
}

static void
uvc_video_encode_bulk_ddr_to_usb(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;
	uint32_t offset = 0;
	struct uvc_request *ureq = req->context;

	if(check_reqbufs_buf_used(req, video, buf) != 0)
	{
		req->length = 0;
		req->zero = 0;
		return;
	}

	//第一个包没有偏移12给UVC头或者最后一个包就使用USB req的BUF
	if((video->queue.buf_used + video->reqbufs_config.data_offset) < video->uvc_video_header_len /* ||
		(buf->bytesused - video->queue.buf_used <= len - video->uvc_video_header_len)*/)
	{
		req->buf = ureq->req_buffer;
		mem = req->buf;

		/* Add a header at the beginning of the payload. */
		if (video->payload_size == 0) {
			ret = video->encode_header(video, buf, mem, len);
			video->payload_size += ret;
			mem += ret;
			len -= ret;
		}

		/* Process video data. */
		len = min((int)(video->max_payload_size - video->payload_size), len);
		ret = uvc_video_encode_data_ddr_to_usb(video, buf, mem, len);

		video->payload_size += ret;
		len -= ret;

		req->length = video->req_size - len;
	}
	else
	{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		if(buf->buf.vb2_buf.index >= video->reqbufs_addr.count)
		{
			printk("buf->buf.index : %d >= %d\n", buf->buf.vb2_buf.index, video->reqbufs_addr.count);
			return;
		}
#else
		if(buf->buf.v4l2_buf.index >= video->reqbufs_addr.count)
		{
			printk("buf->buf.index : %d >= %d\n", buf->buf.v4l2_buf.index, video->reqbufs_addr.count);
			return;
		}
#endif
		req->buf = NULL;
		offset = video->queue.buf_used  + video->reqbufs_config.data_offset - video->uvc_video_header_len;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		req->dma = video->reqbufs_addr.addr[buf->buf.vb2_buf.index] + offset;
		ureq->vb2_buf_index = buf->buf.vb2_buf.index;
#else
		req->dma = video->reqbufs_addr.addr[buf->buf.v4l2_buf.index] + offset;
		ureq->vb2_buf_index = buf->buf.v4l2_buf.index;
#endif
		mem = buf->mem + offset;

		//将UVC头拷贝到DDR
		/* Add a header at the beginning of the payload. */
		if (video->payload_size == 0) 
		{
			ret = video->encode_header(video, buf, mem, len);
			video->payload_size += ret;
			len -= ret;
		}


		/* Process video data. */
		len = min((int)(video->max_payload_size - video->payload_size), len);
		ret = min((unsigned int)len, buf->bytesused - video->queue.buf_used);
		video->queue.buf_used += ret;
		
		video->payload_size += ret;
		len -= ret;

		req->length = video->req_size - len;

		if(video->reqbufs_config.cache_enable)
		{
#ifdef CONFIG_64BIT
			//__flush_dcache_area(mem, req->length);
#else
			__cpuc_flush_dcache_area(mem, req->length);
#endif
		}
		// if(req->dma & 0x0F)
		// {
		// 	printk("DMA transfers must be aligned to a 16-byte boundary, dma addr:0x%08x\n", req->dma);
		// }
	}



	//req->zero = video->payload_size == video->max_payload_size;
	//一帧的最后一包小于UVC包的大小时发短数据包作为结束
	req->zero = req->length < video->max_payload_size;


	if (video->payload_size == video->max_payload_size ||
	    buf->bytesused == video->queue.buf_used)
		video->payload_size = 0;
}

static void
uvc_video_encode_isoc_ddr_to_usb(struct usb_request *req, struct uvc_video *video,
		struct uvc_buffer *buf)
{
	void *mem = req->buf;
	int len = video->req_size;
	int ret;
	uint32_t offset = 0;
	struct uvc_request *ureq = req->context;


	if(check_reqbufs_buf_used(req, video, buf) != 0)
	{
		req->length = 0;
		return;
	}

	//第一个包没有偏移12给UVC头或者最后一个包就使用USB req的BUF
	if((video->queue.buf_used + video->reqbufs_config.data_offset) < video->uvc_video_header_len /* ||
		(buf->bytesused - video->queue.buf_used <= len - video->uvc_video_header_len)*/)
	{
		req->buf = ureq->req_buffer;
		mem = req->buf;

		/* Add the header. */
		ret = video->encode_header(video, buf, mem, len);
		mem += ret;
		len -= ret;

		/* Process video data. */
		ret = uvc_video_encode_data_ddr_to_usb(video, buf, mem, len);
		len -= ret;

		req->length = video->req_size - len;
	}
	else
	{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		if(buf->buf.vb2_buf.index >= video->reqbufs_addr.count)
		{
			printk("buf->buf.index : %d >= %d\n", buf->buf.vb2_buf.index, video->reqbufs_addr.count);
			return;
		}
#else
		if(buf->buf.v4l2_buf.index >= video->reqbufs_addr.count)
		{
			printk("buf->buf.index : %d >= %d\n", buf->buf.v4l2_buf.index, video->reqbufs_addr.count);
			return;
		}
#endif
		req->buf = NULL;
		offset = video->queue.buf_used  + video->reqbufs_config.data_offset - video->uvc_video_header_len;
	
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		req->dma = (dma_addr_t)(video->reqbufs_addr.addr[buf->buf.vb2_buf.index] + offset);
		ureq->vb2_buf_index = buf->buf.vb2_buf.index;
#else
		req->dma = video->reqbufs_addr.addr[buf->buf.v4l2_buf.index] + offset;
		ureq->vb2_buf_index = buf->buf.v4l2_buf.index;
#endif
		mem = buf->mem + offset;
		

		//将UVC头拷贝到DDR
		/* Add the header. */
		ret = video->encode_header(video, buf, mem, len);
		len -= ret;

		/* Process video data. */
		ret = min((unsigned int)len, buf->bytesused - video->queue.buf_used);
		video->queue.buf_used += ret;
		len -= ret;

		req->length = video->req_size - len;

		if(video->reqbufs_config.cache_enable)
		{
#ifdef CONFIG_64BIT
			//__flush_dcache_area(mem, req->length);
#else
			__cpuc_flush_dcache_area(mem, req->length);
#endif
		}

#if 0	
		if(req->dma & 0x0F)
		{
			printk("DMA transfers must be aligned to a 16-byte boundary, dma addr:0x%08x\n", req->dma);
		}	
#endif
	}

}

/* --------------------------------------------------------------------------
 * Request handling
 */

/*
 * I somehow feel that synchronisation won't be easy to achieve here. We have
 * three events that control USB requests submission:
 *
 * - USB request completion: the completion handler will resubmit the request
 *   if a video buffer is available.
 *
 * - USB interface setting selection: in response to a SET_INTERFACE request,
 *   the handler will start streaming if a video buffer is available and if
 *   video is not currently streaming.
 *
 * - V4L2 buffer queueing: the driver will start streaming if video is not
 *   currently streaming.
 *
 * Race conditions between those 3 events might lead to deadlocks or other
 * nasty side effects.
 *
 * The "video currently streaming" condition can't be detected by the irqqueue
 * being empty, as a request can still be in flight. A separate "queue paused"
 * flag is thus needed.
 *
 * The paused flag will be set when we try to retrieve the irqqueue head if the
 * queue is empty, and cleared when we queue a buffer.
 *
 * The USB request completion handler will get the buffer at the irqqueue head
 * under protection of the queue spinlock. If the queue is empty, the streaming
 * paused flag will be set. Right after releasing the spinlock a userspace
 * application can queue a buffer. The flag will then cleared, and the ioctl
 * handler will restart the video stream.
 */
extern int get_uvc_debug(void);

static void
uvc_video_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_request *ureq = req->context;
	struct uvc_video *video = ureq->video;
	struct uvc_video_queue *queue = &video->queue;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;

	struct uvc_device *uvc = video->uvc;
	int ep_queue_error_cnt = 0;
	uint32_t offset;
	int debug = get_uvc_debug();

	if(debug >= 2)
	{
		printk("video complete : status:%d, actual:%d, length:%d\n",
				req->status, req->actual, req->length);
	}
	else if(debug == 1)
	{
		if (req->actual < req->length || req->status != 0)
		{
			printk("video complete : status:%d, actual:%d, length:%d\n",
				req->status, req->actual, req->length);
		}
	}

	if(usb_id_check_fail)
	{		
		if(test_cnt > 200)
		{
			uvcg_queue_cancel(queue, 0);
			goto requeue;
		}
	}

	switch (req->status) {
	case -EXDEV:
		if (req->actual == req->length || req->length == 0)
		{
			goto queue_head;
		}
		break;
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		//printk(KERN_DEBUG "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		goto requeue;

	default:
		printk(KERN_INFO "VS request completed with status %d.\n",
			req->status);
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}
	
	if (video->ep->enabled == 0  || uvc->state != UVC_STATE_STREAMING)
	{
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}


	if (req->actual < req->length)
	{
#if UVC_NUM_REQUESTS == 1
		spin_lock_irqsave(&video->queue.irqlock, flags);
		if(req->actual == 0)
		{
			goto tran_zero;
		}

		if(!req->buf)
		{
			req->length = req->length - req->actual;
			offset = video->queue.buf_used  + video->reqbufs_config.data_offset - video->uvc_video_header_len;
			req->dma = video->reqbufs_addr.addr[ureq->vb2_buf_index] + offset;
			goto tran_zero;
		}
		else
		{
			req->length = req->length - req->actual;
			req->buf += req->actual;
			goto tran_zero;
		}
#else
		printk("video complete error: actual:%d, length:%d\n",
			req->actual, req->length);
#endif
	}

queue_head:
	spin_lock_irqsave(&video->queue.irqlock, flags);
	buf = uvcg_queue_head(&video->queue);
	if (buf == NULL ) {
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
		goto requeue;
	}


	if (buf->bytesused == video->queue.buf_used || buf->bytesused == 0) {
		video->queue.buf_used = 0;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index] = 0;
#else
		video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index] = 0;
#endif
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		if(buf->bytesused != 0)
		{
			video->fid ^= UVC_STREAM_FID;
			if(video->uvc_pts_copy == 0)
			{
				video->uvc_pts_timestamp = uvc_get_timestamp_ns();
			}
		}

		//下一帧
		buf = uvcg_queue_head(&video->queue);
		if (buf == NULL )
		{
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			goto requeue;
		}
	}

	video->encode(req, video, buf);


tran_zero:
	if(debug >= 3)
	{
		printk("uvc_ep_queue : length:%d\n",req->length);
	}

	if ((ret = usb_ep_queue(ep, req, GFP_ATOMIC)) < 0 && video->ep->enabled) 
	{
		printk(KERN_INFO "uvc_video_complete Failed to queue request (%d).\n", ret);
		if(ret == -ESHUTDOWN)
		{
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			/* disconnect from host. */
			uvcg_queue_cancel(queue, 1);
			goto requeue;
		}
		ep_queue_error_cnt ++;
		if(ep_queue_error_cnt > 3)
		{
			printk(KERN_INFO "Failed to queue request (%d), cancel uvc queue!\n", ret);
			usb_ep_set_halt(ep);
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			uvcg_queue_cancel(queue, 0);
			goto requeue;
		}
		else
		{
			goto tran_zero;
		}
	}
	spin_unlock_irqrestore(&video->queue.irqlock, flags);

	return;

requeue:
	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
}

static void
uvc_video_complete_work(struct usb_ep *ep, struct usb_request *req)
{
	struct uvc_request *ureq = req->context;
	struct uvc_video *video = ureq->video;
	struct uvc_video_queue *queue = &video->queue;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;

	struct uvc_device *uvc = video->uvc;
	int ep_queue_error_cnt = 0;
	uint32_t offset;
	int debug = get_uvc_debug();

	if(debug >= 2)
	{
		printk("video complete : status:%d, actual:%d, length:%d\n",
				req->status, req->actual, req->length);
	}
	else if(debug == 1)
	{
		if (req->actual < req->length || req->status != 0)
		{
			printk("video complete : status:%d, actual:%d, length:%d\n",
				req->status, req->actual, req->length);
		}
	}

	if(usb_id_check_fail)
	{		
		if(test_cnt > 200)
		{
			uvcg_queue_cancel(queue, 0);
			goto requeue;
		}
	}

	switch (req->status) {
	case -EXDEV:
		if (req->actual == req->length || req->length == 0)
		{
			goto queue_head;
		}
		break;
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		//printk(KERN_DEBUG "VS request cancelled.\n");
		uvcg_queue_cancel(queue, 1);
		goto requeue;

	default:
		printk(KERN_INFO "VS request completed with status %d.\n",
			req->status);
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}
	
	if (video->ep->enabled == 0  || uvc->state != UVC_STATE_STREAMING)
	{
		uvcg_queue_cancel(queue, 0);
		goto requeue;
	}


	if (req->actual < req->length)
	{
#if UVC_NUM_REQUESTS == 1
		spin_lock_irqsave(&video->queue.irqlock, flags);
		if(req->actual == 0)
		{
			goto tran_zero;
		}

		if(!req->buf)
		{
			req->length = req->length - req->actual;
			offset = video->queue.buf_used  + video->reqbufs_config.data_offset - video->uvc_video_header_len;
			req->dma = video->reqbufs_addr.addr[ureq->vb2_buf_index] + offset;
			goto tran_zero;
		}
		else
		{
			req->length = req->length - req->actual;
			req->buf += req->actual;
			goto tran_zero;
		}
#else
		printk("video complete error: actual:%d, length:%d\n",
			req->actual, req->length);
#endif
	}

queue_head:
	spin_lock_irqsave(&video->queue.irqlock, flags);
	buf = uvcg_queue_head(&video->queue);
	if (buf == NULL ) {
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
		goto requeue;
	}


	if (buf->bytesused == video->queue.buf_used || buf->bytesused == 0) {
		video->queue.buf_used = 0;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
		video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index] = 0;
#else
		video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index] = 0;
#endif
		buf->state = UVC_BUF_STATE_DONE;
		uvcg_queue_next_buffer(&video->queue, buf);
		if(buf->bytesused != 0)
		{
			video->fid ^= UVC_STREAM_FID;
			if(video->uvc_pts_copy == 0)
			{
				video->uvc_pts_timestamp = uvc_get_timestamp_ns();
			}
		}

		//下一帧
		buf = uvcg_queue_head(&video->queue);
		if (buf == NULL )
		{
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			goto requeue;
		}
	}

	if(video->work_pump_enable)
	{
		spin_unlock_irqrestore(&video->queue.irqlock, flags);
		goto requeue;
	}

	video->encode(req, video, buf);


tran_zero:
	if(debug >= 3)
	{
		printk("uvc_ep_queue : length:%d\n",req->length);
	}

	if ((ret = usb_ep_queue(ep, req, GFP_ATOMIC)) < 0 && video->ep->enabled) 
	{
		printk(KERN_INFO "uvc_video_complete Failed to queue request (%d).\n", ret);
		if(ret == -ESHUTDOWN)
		{
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			/* disconnect from host. */
			uvcg_queue_cancel(queue, 1);
			goto requeue;
		}
		ep_queue_error_cnt ++;
		if(ep_queue_error_cnt > 3)
		{
			printk(KERN_INFO "Failed to queue request (%d), cancel uvc queue!\n", ret);
			usb_ep_set_halt(ep);
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			uvcg_queue_cancel(queue, 0);
			goto requeue;
		}
		else
		{
			goto tran_zero;
		}
	}
	spin_unlock_irqrestore(&video->queue.irqlock, flags);

	return;

requeue:
	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);

	if (uvc->state == UVC_STATE_STREAMING && video->ep->enabled)
		queue_work(video->async_wq, &video->pump);
}

static int
uvc_video_free_requests(struct uvc_video *video)
{
	unsigned int i;

	if (video->ureq) {
		for (i = 0; i < video->uvc_num_requests; ++i) {

			if (video->ureq[i].req) {
				usb_ep_free_request(video->ep, video->ureq[i].req);
				video->ureq[i].req = NULL;
			}

			if (video->ureq[i].req_buffer) {
				kfree(video->ureq[i].req_buffer);
				video->ureq[i].req_buffer = NULL;
			}
		}

		kfree(video->ureq);
		video->ureq = NULL;
	}

	INIT_LIST_HEAD(&video->req_free);
	video->req_size = 0;
	return 0;
}

static int
uvc_video_alloc_requests(struct uvc_video *video)
{
	unsigned int req_size;
	unsigned int req_buf_size;
	unsigned int i;
	int ret = -ENOMEM;

	//BUG_ON(video->req_size);

	//req_size = video->ep->maxpacket * max_t(unsigned int, video->ep->maxburst, 1) * (video->ep->mult + 1);
	//req_size = video->ep->maxpacket;
	
	req_size = video->max_payload_size;
	req_buf_size = video->max_payload_size;

	if(video->reqbufs_config.enable)
	{
		if(req_buf_size > 65536)
		{
			req_buf_size = 65536;
		}
	}

	printk("video->ep: %p req_size: %d \n", video->ep, req_size);

	video->ureq = kcalloc(video->uvc_num_requests, sizeof(struct uvc_request), GFP_KERNEL);
	if (video->ureq == NULL)
		return -ENOMEM;

	for (i = 0; i < video->uvc_num_requests; ++i) {
		video->ureq[i].req_buffer = kmalloc(req_buf_size, GFP_KERNEL);
		if (video->ureq[i].req_buffer == NULL)
			goto error;

		video->ureq[i].req = usb_ep_alloc_request(video->ep, GFP_KERNEL);
		if (video->ureq[i].req == NULL)
			goto error;

		video->ureq[i].req->buf = video->ureq[i].req_buffer;
		video->ureq[i].req->length = 0;
		video->ureq[i].req->complete = uvc_video_complete;
		video->ureq[i].req->context = &video->ureq[i];
		video->ureq[i].video = video;
		video->ureq[i].last_buf = NULL;
		video->ureq[i].vb2_buf_index = i;
		if(video->work_pump_enable)
		{
			video->ureq[i].req->complete = uvc_video_complete_work;
		}

		list_add_tail(&video->ureq[i].req->list, &video->req_free);


	}

	video->req_size = req_size;

	return 0;

error:
	uvc_video_free_requests(video);
	return ret;
}

/* --------------------------------------------------------------------------
 * Video streaming
 */

/*
 * uvcg_video_pump - Pump video data into the USB requests
 *
 * This function fills the available USB requests (listed in req_free) with
 * video data from the queued buffers.
 */

int uvcg_video_pump(struct uvc_video *video)
{
	struct uvc_video_queue *queue = &video->queue;
	struct usb_request *req;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;
	struct uvc_request *ureq;

	struct uvc_device *uvc = video->uvc;
	
	int ep_queue_error_cnt = 0;

	
	if (video->ep->enabled == 0  || (uvc->state != UVC_STATE_STREAMING))
	{
		return 0;
	}
	
	/* FIXME TODO Race between uvcg_video_pump and requests completion
	 * handler ???
	 */

	while (1) {
		
		if (video->ep->enabled == 0  || uvc->state != UVC_STATE_STREAMING)
		{
			return 0;
		}
	
		/* Retrieve the first available USB request, protected by the
		 * request lock.
		 */
		spin_lock_irqsave(&video->req_lock, flags);
		if (list_empty(&video->req_free)) {
			spin_unlock_irqrestore(&video->req_lock, flags);
			return 0;
		}
		req = list_first_entry(&video->req_free, struct usb_request,
					list);
		list_del(&req->list);
		spin_unlock_irqrestore(&video->req_lock, flags);

		/* Retrieve the first available video buffer and fill the
		 * request, protected by the video queue irqlock.
		 */
		spin_lock_irqsave(&queue->irqlock, flags);
		buf = uvcg_queue_head(queue);
		if (buf == NULL) {				
			spin_unlock_irqrestore(&queue->irqlock, flags);
			break;
		}

		//如果是0长度则跳过这一帧
		if (buf->bytesused == 0) 
		{
#if 0
			video->queue.buf_used = 0;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
			video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index] = 0;
#else
			video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index] = 0;
#endif
			buf->state = UVC_BUF_STATE_DONE;
			uvcg_queue_next_buffer(&video->queue, buf);
			if(buf->bytesused != 0)
			{
				video->fid ^= UVC_STREAM_FID;
				if(video->uvc_pts_copy == 0)
				{
					video->uvc_pts_timestamp = uvc_get_timestamp_ns();
				}
			}
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			break;
#else
			req->length = 0;
			ureq = req->context;
			req->buf = ureq->req_buffer;
#if (defined CHIP_SSC333 || defined CHIP_SSC337)

#else
			//goto tran_zero;
#endif
#endif
		}
		else
		{
			//最后一包未发送完成，在complet后进入下一帧
			if (buf->bytesused == video->queue.buf_used)
			{
				spin_unlock_irqrestore(&queue->irqlock, flags);
				break;
			}
		}
		
		video->encode(req, video, buf);

tran_zero:
		/* Queue the USB request */
		ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
		if (ret < 0 && video->ep->enabled) {
			printk(KERN_INFO "uvcg_video_pump Failed to queue request (%d)\n", ret);
			if(ret == -ESHUTDOWN)
			{
				spin_unlock_irqrestore(&video->queue.irqlock, flags);
				/* disconnect from host. */
				uvcg_queue_cancel(queue, 1);
				break;
			}
			ep_queue_error_cnt ++;
			if(ep_queue_error_cnt > 3)
			{
				printk(KERN_INFO "Failed to queue request (%d), cancel uvc queue!\n", ret);
				usb_ep_set_halt(video->ep);
				spin_unlock_irqrestore(&queue->irqlock, flags);
				uvcg_queue_cancel(queue, 0);
				break;
			}
			else
			{
				goto tran_zero;
			}
		}
		spin_unlock_irqrestore(&queue->irqlock, flags);
	}
	
//requeue:
	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
	return 0;
}

static void uvcg_video_pump_work(struct work_struct *work)
{
	struct uvc_video *video = container_of(work, struct uvc_video, pump);
	struct uvc_video_queue *queue = &video->queue;
	struct usb_request *req;
	struct uvc_buffer *buf;
	unsigned long flags;
	int ret;
	struct uvc_request *ureq;
	struct uvc_device *uvc = video->uvc;
	
	int ep_queue_error_cnt = 0;

	
	if (video->ep->enabled == 0  || (uvc->state != UVC_STATE_STREAMING))
	{
		return;
	}
	
	/* FIXME TODO Race between uvcg_video_pump and requests completion
	 * handler ???
	 */

	while (1) {
		
		if (video->ep->enabled == 0  || uvc->state != UVC_STATE_STREAMING)
		{
			return;
		}
	
		/* Retrieve the first available USB request, protected by the
		 * request lock.
		 */
		spin_lock_irqsave(&video->req_lock, flags);
		if (list_empty(&video->req_free)) {
			spin_unlock_irqrestore(&video->req_lock, flags);
			return;
		}
		req = list_first_entry(&video->req_free, struct usb_request,
					list);
		list_del(&req->list);
		spin_unlock_irqrestore(&video->req_lock, flags);

		/* Retrieve the first available video buffer and fill the
		 * request, protected by the video queue irqlock.
		 */
		spin_lock_irqsave(&queue->irqlock, flags);
		buf = uvcg_queue_head(queue);
		if (buf == NULL) {				
			spin_unlock_irqrestore(&queue->irqlock, flags);
			break;
		}

		//如果是0长度则跳过这一帧
		if (buf->bytesused == 0) 
		{
#if 0
			video->queue.buf_used = 0;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
			video->reqbufs_buf_used.buf_used[buf->buf.vb2_buf.index] = 0;
#else
			video->reqbufs_buf_used.buf_used[buf->buf.v4l2_buf.index] = 0;
#endif
			buf->state = UVC_BUF_STATE_DONE;
			uvcg_queue_next_buffer(&video->queue, buf);
			if(buf->bytesused != 0)
			{
				video->fid ^= UVC_STREAM_FID;
				if(video->uvc_pts_copy == 0)
				{
					video->uvc_pts_timestamp = uvc_get_timestamp_ns();
				}
			}
			spin_unlock_irqrestore(&video->queue.irqlock, flags);
			break;
#else
			req->length = 0;
			ureq = req->context;
			req->buf = ureq->req_buffer;
#if (defined CHIP_SSC333 || defined CHIP_SSC337)

#else
			//goto tran_zero;
#endif
#endif
		}
		else
		{
			//最后一包未发送完成，在complet后进入下一帧
			if (buf->bytesused == video->queue.buf_used)
			{
				spin_unlock_irqrestore(&queue->irqlock, flags);
				break;
			}
		}
		
		video->encode(req, video, buf);

tran_zero:
		/* Queue the USB request */
		ret = usb_ep_queue(video->ep, req, GFP_ATOMIC);
		if (ret < 0 && video->ep->enabled) {
			printk(KERN_INFO "uvcg_video_pump Failed to queue request (%d)\n", ret);
			if(ret == -ESHUTDOWN)
			{
				spin_unlock_irqrestore(&video->queue.irqlock, flags);
				/* disconnect from host. */
				uvcg_queue_cancel(queue, 1);
				break;
			}
			ep_queue_error_cnt ++;
			if(ep_queue_error_cnt > 3)
			{
				printk(KERN_INFO "Failed to queue request (%d), cancel uvc queue!\n", ret);
				usb_ep_set_halt(video->ep);
				spin_unlock_irqrestore(&queue->irqlock, flags);
				uvcg_queue_cancel(queue, 0);
				break;
			}
			else
			{
				goto tran_zero;
			}
		}
		spin_unlock_irqrestore(&queue->irqlock, flags);
	}
	
//requeue:
	spin_lock_irqsave(&video->req_lock, flags);
	list_add_tail(&req->list, &video->req_free);
	spin_unlock_irqrestore(&video->req_lock, flags);
	return;
}



/*
 * Enable or disable the video stream.
 */
int uvcg_video_enable(struct uvc_video *video, int enable)
{
	unsigned int i;
	int ret;
	struct uvc_device *uvc = video->uvc;
	struct usb_gadget *gadget = video->uvc->func.config->cdev->gadget;
	if(gadget->ops->get_frame == NULL)
	{
		printk("gadget->ops->get_frame error!\n");
		return -ENODEV;
	}

	printk("uvcg_video_enable called, enable: %d \n", enable);
	if (video->ep == NULL) {
		printk(KERN_INFO "Video enable failed, device is "
			"uninitialized.\n");
		return -ENODEV;
	}

	uvcg_queue_cancel(&video->queue, 0);

	if (!enable) 
	{
		if(video->work_pump_enable)
		{
			cancel_work_sync(&video->pump);
		}
		if (video->ep->enabled && video->bulk_streaming_ep == 0)
		{
			if(get_usb_link_state() != 0)
			{
				usb_ep_disable(video->ep);
			}
		}

		for (i = 0; i < video->uvc_num_requests; ++i)
			if (video->ureq && video->ureq[i].req)
				usb_ep_dequeue(video->ep, video->ureq[i].req);

		uvc_video_free_requests(video);
		uvcg_queue_enable(&video->queue, 0);
		
		return 0;
	}

	if(gadget->speed == USB_SPEED_UNKNOWN)
	{
		return -1;
	}

	if(video->uvc_pts_copy == 0)
	{
		video->uvc_pts_timestamp = uvc_get_timestamp_ns();
	}

	if (video->ep->enabled == 0) {
		ret = config_ep_by_speed(gadget, &(uvc->func), uvc->video.ep);
		if(ret)
			return ret;
		usb_ep_enable(video->ep);
		video->ep->driver_data = uvc;
	}

	
	if(video->simulcast_enable)
	{
		video->uvc_video_header_len = UVC_VIDEO_SIMULCAST_HEADER_LEN;
		video->encode_header = uvc_video_simulcast_encode_header;
	}
	else if(video->win_hello_enable)
	{
		video->uvc_video_header_len = UVC_VIDEO_WIN_HELLO_HEADER_LEN;
		video->encode_header = uvc_video_win_hello_encode_header;
	}
	else
	{
		video->uvc_video_header_len = UVC_VIDEO_HEADER_LEN;
		video->encode_header = uvc_video_encode_header;
	}

	if(video->bulk_streaming_ep){
		if(video->reqbufs_config.enable){
			video->encode = uvc_video_encode_bulk_ddr_to_usb;
		}
		else{
			video->encode = uvc_video_encode_bulk;
		}
		video->payload_size = 0;
	} else{
		if(video->reqbufs_config.enable){
			video->encode = uvc_video_encode_isoc_ddr_to_usb;
		}
		else{
			video->encode = uvc_video_encode_isoc;
		}
		video->payload_size = 0;
	}

	if ((ret = uvcg_queue_enable(&video->queue, 1)) < 0)
		return ret;

	if ((ret = uvc_video_alloc_requests(video)) < 0)
		return ret;
	
	video->fid = 0;

#ifdef PACKAGE_TEST
	package_index = 0;
	frame_index = 0;
#endif


	if(check_usb_id(get_vhd_usb_vid(), get_vhd_usb_pid()) == 0 || 
		check_usb_id(video->uvc->func.config->cdev->desc.idVendor,video->uvc->func.config->cdev->desc.idProduct) == 0)
	{
		usb_id_check_fail = 0;
	}
	else
	{
		usb_id_check_fail = 1;
		if(test_cnt < 200)
			test_cnt ++;
	}


	if(video->bulk_streaming_ep)
	{
		return 0;
	}

	if(video->work_pump_enable)
	{
		queue_work(video->async_wq, &video->pump);
		return 0;
	}
	
	//msleep(10);
	return uvcg_video_pump(video);
}

/*
 * Initialize the UVC video stream.
 */
int uvcg_video_init(struct usb_composite_dev *cdev, struct uvc_video *video)
{
	INIT_LIST_HEAD(&video->req_free);
	spin_lock_init(&video->req_lock);
	
	INIT_WORK(&video->pump, uvcg_video_pump_work);
	/* Allocate a work queue for asynchronous video pump handler. */
	video->async_wq = alloc_workqueue("uvcgadget", WQ_UNBOUND | WQ_HIGHPRI, 0);
	if (!video->async_wq)
		return -EINVAL;
	video->work_pump_enable = 1;

	video->fcc = V4L2_PIX_FMT_YUYV;
	video->bpp = 16;
	video->width = 320;
	video->height = 240;
	video->imagesize = 320 * 240 * 2;
	
	video->simulcast_enable = 0;
	video->uvc_pts = uvc_get_timestamp_us();
	video->uvc_scr = 0;
	video->uvc_pts_copy = 0;
	video->uvc_pts_timestamp = uvc_get_timestamp_ns();
	video->uvc_video_header_len = UVC_VIDEO_HEADER_LEN;

	video->reqbufs_config.enable = 0;
	video->reqbufs_config.cache_enable = 0;
	video->reqbufs_config.data_offset = 0;


#if 0
	memset(video->temp_header, 0, UVC_VIDEO_HEADER_LEN);
	memset(video->simulcast_temp_header, 0, UVC_VIDEO_SIMULCAST_HEADER_LEN);
	
	video->temp_header[0] = UVC_VIDEO_HEADER_LEN;
	video->simulcast_temp_header[0] = UVC_VIDEO_SIMULCAST_HEADER_LEN;

#ifdef HEADER_SCR_PTS
	video->temp_header[1] = UVC_STREAM_SCR | UVC_STREAM_PTS;
	memcpy(video->temp_header + 2, &video->current_pts, 4);
	memcpy(video->temp_header + 6, &video->current_scr, 4);
	memcpy(video->temp_header + 10, &video->current_sof, 2);

	video->simulcast_temp_header[1] = UVC_STREAM_SCR | UVC_STREAM_PTS;
	memcpy(video->simulcast_temp_header + 2, &video->current_pts, 4);
	memcpy(video->simulcast_temp_header + 6, &video->current_scr, 4);
	memcpy(video->simulcast_temp_header + 10, &video->current_sof, 2);
#endif
#endif

	if (video->bulk_streaming_ep)
		video->max_payload_size = video->bulk_max_size;

	
	mutex_init(&video->v4l2_ioctl_mutex);

	/* Initialize the video buffers queue. */
#if LINUX_VERSION_CODE>= KERNEL_VERSION(4,9,0)
	uvcg_queue_init(&video->queue, V4L2_BUF_TYPE_VIDEO_OUTPUT, &video->mutex);
#else
	uvcg_queue_init(&video->queue, V4L2_BUF_TYPE_VIDEO_OUTPUT);
#endif


#if 1 //(defined CHIP_X3 || defined CHIP_CV5)
    video->cma_dev = cma_dma_init(cdev->gadget->dev.parent);//udc->dev
#else
	video->cma_dev = cma_dma_init(&cdev->gadget->dev);
#endif
    if(!video->cma_dev)
    {
        printk("cma_dma_init error\n");
        return -1;
    }

	return 0;
}

int uvcg_video_release(struct uvc_video *video)
{
    if(!video)
    {
        return -1;
    }


    if (video->ep->enabled == 1  && video->uvc && video->uvc->state == UVC_STATE_STREAMING)
    {
        uvcg_video_enable(video, 0);
    }

    uvcg_free_buffers(&video->queue);

    v4l2_ioctl_mutex_lock(video);

    if(video->reqbufs_addr.ppVirtualAddress)
    {
#ifdef CONFIG_ARCH_SSTAR
        for(i= 0; i < video->reqbufs_addr.count; i++)
        {
            if(video->reqbufs_addr.ppVirtualAddress[i])
                UvcCamOsMemUnmap(video->reqbufs_addr.ppVirtualAddress[i], video->reqbufs_addr.length[i]);
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

    v4l2_ioctl_mutex_unlock(video);
    

    if(video->cma_dev)
    {
        cma_dma_release(video->cma_dev);
        video->cma_dev = NULL;
    }
    return 0;
}

