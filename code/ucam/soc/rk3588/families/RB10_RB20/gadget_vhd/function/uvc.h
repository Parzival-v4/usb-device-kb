/*
 *	uvc_gadget.h  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#ifndef _UVC_GADGET_H_
#define _UVC_GADGET_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>

#include "uvc_config.h"
#include "cma_dma.h"


#define UVC_EVENT_FIRST				(V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_CONNECT			(V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT		(V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON			(V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF			(V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP				(V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA				(V4L2_EVENT_PRIVATE_START + 5)
#define UVC_EVENT_AUDIO_STREAMON    (V4L2_EVENT_PRIVATE_START + 6)
#define UVC_EVENT_AUDIO_STREAMOFF   (V4L2_EVENT_PRIVATE_START + 7)
#define UVC_EVENT_SWITCH_CONFIGURATION   (V4L2_EVENT_PRIVATE_START + 8)
#define UVC_EVENT_SUSPEND   		(V4L2_EVENT_PRIVATE_START + 9)
#define UVC_EVENT_RESUME   			(V4L2_EVENT_PRIVATE_START + 10)
#define UVC_EVENT_LAST				(V4L2_EVENT_PRIVATE_START + 10)




#define UVC_REQUEST_DATA_AUTO_QUEUE_LEN (60 - sizeof(struct usb_ctrlrequest))	
struct uvc_request_data_auto_queue
{
	__s32 length;	
	__u8 data[UVC_REQUEST_DATA_AUTO_QUEUE_LEN];
	
	struct usb_ctrlrequest usb_ctrl;
};

#define UVC_REQUEST_DATA_LEN (60)	
struct uvc_request_data
{
	__s32 length;	
	__u8 data[UVC_REQUEST_DATA_LEN];

};



struct uvc_event
{
	union {
		enum usb_device_speed speed;
		struct usb_ctrlrequest req;
		struct uvc_request_data data;
		struct uvc_request_data_auto_queue auto_queue_data;
	};
};


/* ------------------------------------------------------------------------
 * Debugging, printing and logging
 */

#ifdef __KERNEL__

#include <linux/usb.h>	/* For usb_endpoint_* */
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-device.h>

#include "uvc_queue.h"

#define UVC_TRACE_PROBE				(1 << 0)
#define UVC_TRACE_DESCR				(1 << 1)
#define UVC_TRACE_CONTROL			(1 << 2)
#define UVC_TRACE_FORMAT			(1 << 3)
#define UVC_TRACE_CAPTURE			(1 << 4)
#define UVC_TRACE_CALLS				(1 << 5)
#define UVC_TRACE_IOCTL				(1 << 6)
#define UVC_TRACE_FRAME				(1 << 7)
#define UVC_TRACE_SUSPEND			(1 << 8)
#define UVC_TRACE_STATUS			(1 << 9)

#define UVC_WARN_MINMAX				0
#define UVC_WARN_PROBE_DEF			1

#if 0
extern unsigned int uvc_gadget_trace_param;


#define uvc_trace(flag, msg...) \
	do { \
		if (uvc_gadget_trace_param & flag) \
			printk(KERN_DEBUG "uvcvideo: " msg); \
	} while (0)

#define uvc_warn_once(dev, warn, msg...) \
	do { \
		if (!test_and_set_bit(warn, &dev->warnings)) \
			printk(KERN_INFO "uvcvideo: " msg); \
	} while (0)
		
#else

#define uvc_trace(flag, msg...) \
	do { \
		if (UVC_TRACE_STATUS & flag) \
			printk(KERN_DEBUG "uvcvideo: " msg); \
	} while (0)

#define uvc_warn_once(dev, warn, msg...) \
	do { \
		if (!test_and_set_bit(warn, &dev->warnings)) \
			printk(KERN_INFO "uvcvideo: " msg); \
	} while (0)	
#endif

#define uvc_printk(level, msg...) \
	printk(level "uvcvideo: " msg)

/* ------------------------------------------------------------------------
 * Driver specific constants
 */
#if defined(CONFIG_ARCH_HI3516EV200) || defined(CONFIG_ARCH_HI3516EV300) \
|| defined(CONFIG_ARCH_HI3518EV300) || defined(CONFIG_ARCH_HI3516DV200) || defined (CONFIG_HIUSB_DEVICE2_0)
#define UVC_NUM_REQUESTS		1
#else
#define UVC_NUM_REQUESTS		1//32
#endif
#define UVC_MAX_REQUEST_SIZE	4096
#define UVC_MAX_EVENTS			64


#define HEADER_SCR_PTS

#ifdef HEADER_SCR_PTS
#define UVC_VIDEO_HEADER_LEN	12 
#define UVC_VIDEO_SIMULCAST_HEADER_LEN	16
#else
#define UVC_VIDEO_HEADER_LEN	2 
#define UVC_VIDEO_SIMULCAST_HEADER_LEN	6
#endif

#define UVC_VIDEO_WIN_HELLO_HEADER_LEN	(UVC_VIDEO_HEADER_LEN + 16)

/* ------------------------------------------------------------------------
 * Structures
 */

struct reqbufs_addr_t {
	__u32 				count;
	unsigned long long	*addr;
	__u32 				*length;
	void 				**ppVirtualAddress;
} __attribute__ ((packed));

struct reqbufs_buf_used_t {
	__u32 				count;
	__u32 				*buf_used;
} __attribute__ ((packed));


/* 	stc: D42..D32: 1KHz SOF token counter 
*	D47..D43: Reserved, set to zero. 
*	USB SOF 125us
*/
#define USB_SOF_TO_UVC_STC(n)		(((n) >> 3) & 0x07FF)
struct uvc_payload_header_t {
	__u8	length;
	__u8	info;
	__u32	pts;
	__u32	scr;
	__u16 	stc;

#if 0
	union
	{
		__u32 simulcast_id;//UVC 1.5 simulcast stream id
		__u8 metadata[16];//UVC 1.5 Windows hello StandardFormatMetadata
	}
#endif
} __attribute__ ((packed));

struct uvc_payload_header_simulcast_t {
	__u8	length;
	__u8	info;
	__u32	pts;
	__u32	scr;
	__u16 	stc;


	__u32 simulcast_id;//UVC 1.5 simulcast stream id
} __attribute__ ((packed));

struct uvc_payload_header_win_hello_t {
	__u8	length;
	__u8	info;
	__u32	pts;
	__u32	scr;
	__u16 	stc;

	__u8 metadata[16];//UVC 1.5 Windows hello StandardFormatMetadata
} __attribute__ ((packed));

struct uvc_request {
	struct usb_request *req;
	u8 *req_buffer;
	struct uvc_video *video;
	struct sg_table sgt;
	u8 header[32];
	struct uvc_buffer *last_buf;
	unsigned int vb2_buf_index;
};

struct uvc_video
{
	struct usb_ep *ep;
	struct uvc_device *uvc;

	/* Frame parameters */
	u8 bpp;
	u32 fcc;
	unsigned int width;
	unsigned int height;
	unsigned int imagesize;
	struct mutex mutex;

	unsigned int uvc_num_requests;
	
	/* Requests */
	unsigned int req_size;
	struct uvc_request *ureq;
	struct list_head req_free;
	spinlock_t req_lock;

	//unsigned int req_int_count;

	
	int (*encode_header)(struct uvc_video *video, struct uvc_buffer *buf,
		u8 *data, int len);
		
	void (*encode) (struct usb_request *req, struct uvc_video *video,
			struct uvc_buffer *buf);

	/* Context data used by the completion handler */
	__u32 payload_size;
	__u32 max_payload_size;

	struct uvc_video_queue queue;
	unsigned int fid;
	bool bulk_streaming_ep;
	unsigned long bulk_max_size;
	
	
	//UVC header
	unsigned int uvc_video_header_len;
	uint32_t uvc_pts;//视频开始采集的时间戳
	uint32_t uvc_scr;//开始发出视频的时间戳
	uint32_t uvc_pts_copy;//从视频中拷贝pts时间戳,如果没有就使用系统的时间戳
	s64 uvc_pts_timestamp;
	
	//unsigned char temp_header[UVC_VIDEO_HEADER_LEN];
	//unsigned char simulcast_temp_header[UVC_VIDEO_SIMULCAST_HEADER_LEN];

	uint32_t simulcast_enable;
	uint32_t win_hello_enable;
	uint32_t cma_dma_buf_enable;

	struct uvc_ioctl_reqbufs_config_t reqbufs_config;
	struct reqbufs_addr_t reqbufs_addr;
	struct reqbufs_buf_used_t reqbufs_buf_used;
	struct mutex v4l2_ioctl_mutex;

	int work_pump_enable;
	struct work_struct pump;
	struct workqueue_struct *async_wq;

    struct cma_dma_dev *cma_dev;
};

enum uvc_state
{
	UVC_STATE_DISCONNECTED,
	UVC_STATE_CONNECTED,
	UVC_STATE_STREAMING,
	UVC_STATE_CONFIG,	//just for bulk-xfer
	UVC_STATE_SUSPEND,
	UVC_STATE_RESUME
};

struct uvc_device
{
	struct video_device *vdev;
	struct v4l2_device v4l2_dev;
	enum uvc_state state;
	struct usb_function func;
	struct uvc_video video;

	/* Descriptors */
	struct {
		const struct uvc_descriptor_header * const *fs_control;
		const struct uvc_descriptor_header * const *ss_control;
		const struct uvc_descriptor_header * const *fs_streaming;
		const struct uvc_descriptor_header * const *hs_streaming;
		const struct uvc_descriptor_header * const *ss_streaming;
	} desc;

	unsigned int control_intf;
	struct usb_ep *control_ep;
	struct usb_request *control_req;
	void *control_buf;
	void (*control_req_completion)(struct usb_ep *ep,
					struct usb_request *req);

	
	struct usb_ep *uvc_status_interrupt_ep;
	struct usb_request *uvc_status_interrupt_req;
	void *uvc_status_interrupt_buf;
	bool uvc_status_write_pending;
	
	spinlock_t		status_interrupt_spinlock;

	unsigned int streaming_intf;
	unsigned int streaming_intf_alt;

	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
	struct usb_ctrlrequest event_ctrl;
	

	unsigned int  dfu_enable;
	unsigned int  uac_enable;
	unsigned int  uvc_s2_enable;
	unsigned int  uvc_v150_enable;
	unsigned int  win7_usb3_enable;
	unsigned int  work_pump_enable;
	unsigned int  zte_hid_enable;
	unsigned int  uvc_interrupt_ep_enable;
	unsigned int  auto_queue_data_enable;
	
	char *usb_sn_string;
	uint32_t usb_sn_string_length;

	char *usb_vendor_string;
	uint32_t usb_vendor_string_length;

	char *usb_product_string;
	uint32_t usb_product_string_length;

	struct completion  uvc_receive_ep0_completion;

	//int *debug;
};

static inline struct uvc_device *to_uvc(struct usb_function *f)
{
	return container_of(f, struct uvc_device, func);
}

struct uvc_file_handle
{
	struct v4l2_fh vfh;
	struct uvc_video *device;
};

#define to_uvc_file_handle(handle) \
	container_of(handle, struct uvc_file_handle, vfh)

static inline int
uvc_setup_data(struct uvc_device *uvc, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = uvc->func.config->cdev;
	struct usb_request *req = uvc->control_req;

	if (ctrl->wLength < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	req->length = min_t(unsigned int, uvc->event_length, ctrl->wLength);
	req->zero = ctrl->wLength < uvc->event_length;
	req->complete = uvc->control_req_completion;

	memcpy(&uvc->event_ctrl, ctrl, sizeof(struct usb_ctrlrequest));
	return usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
}

static inline u64 uvc_get_timestamp_us(void)
{
	u64 result;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
	struct timespec64 ts;
	
	ktime_get_raw_ts64(&ts);
#else
	struct timespec ts;

	getrawmonotonic(&ts);
#endif
	
#if 1
	result = ((s64) ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec;
	do_div(result,NSEC_PER_USEC);
	return result;
#else
   	return (((s64) ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec)/NSEC_PER_USEC;
#endif
}

static inline u64 uvc_get_timestamp_ns(void)
{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
	struct timespec64 ts;
	ktime_get_raw_ts64(&ts);
#else
	struct timespec ts;
	getrawmonotonic(&ts);
#endif
    return (u64)(((s64) ts.tv_sec * NSEC_PER_SEC) + ts.tv_nsec);
}

/* ------------------------------------------------------------------------
 * Functions
 */

extern void uvc_function_setup_continue(struct uvc_device *uvc);
extern void uvc_endpoint_stream(struct uvc_device *dev);

extern void uvc_function_connect(struct uvc_device *uvc);
extern void uvc_function_disconnect(struct uvc_device *uvc);

//extern void * UvcCamOsMemMap(unsigned long long miu_phy_addr, u32 nSize, u8 bCache);
//extern void UvcCamOsMemUnmap(void* pVirtPtr, u32 nSize);

#endif /* __KERNEL__ */

#endif /* _UVC_GADGET_H_ */

