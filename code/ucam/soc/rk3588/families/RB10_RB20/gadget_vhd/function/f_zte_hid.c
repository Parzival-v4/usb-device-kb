/*
 * f_hid.c -- USB HID function driver
 *
 * Copyright (C) 2010 Fabien Chouteau <fabien.chouteau@barco.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/usb/g_hid.h>
#include <linux/ktime.h>

#include "u_f.h"
#include "uvc.h"


static unsigned int zte_hid_ep_size = 4;
module_param(zte_hid_ep_size, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(zte_hid_ep_size, "hid ep max packet size");

//#define UVCIOC_USB_CONNECT_STATE	_IOW('U', 2, int)
//#define UVCIOC_GET_USB_SOFT_CONNECT_STATE	_IOR('U', 6, uint32_t)


static int zte_major[DEV_HID_ZTE_NUM_MAX], zte_minors[DEV_HID_ZTE_NUM_MAX];

static struct class *hidg_zte_class = NULL;

void ghid_zte_cleanup(u8 bConfigurationValue);
	
struct hidg_func_descriptor hid_zte_des= {
	.subclass = 1,
	.protocol = 1,
	.report_length = 4,
	.report_desc_length= 0x17,
	.report_desc= {
	    0x05, 0x01,                    	//USAGE_PAGE (Generic Desktop)
	    0x09, 0x06,                    	//USAGE (Keyboard)
	    0xa1, 0x01,                    	//COLLECTION (Application)
	    0x05, 0x07,                    	// USAGE_PAGE (Keyboard)
	    0x19, 0xe0,                    	//USAGE_MINIMUM (Keyboard LeftControl)
	    0x29, 0xe7,                    	//USAGE_MAXIMUM (Keyboard Right GUI)
	    0x15, 0x00,                    	//LOGICAL_MINIMUM (0)
	    0x25, 0x01,                    	//LOGICAL_MAXIMUM (1)
	    0x75,0x08,						//REPORT_SIZE (8)
	    0x95,0x04,						//REPORT_COUNT (4)
	    0x81,0x02,						//INPUT (Data,Var,Abs)

		0xC0							// END_COLLECTION

		}
};

struct zte_hid_custom_report_desc
{
	__u8 subclass;
	__u8 protocol;
	__u16 report_length;
    __u16 report_desc_length;
    __u8 report_desc[512];
}__attribute__ ((packed));

#define ZTEHIDIOC_SET_HID_REPORT_DESC   _IOW('Z', 1, struct zte_hid_custom_report_desc)

#define HID_ZTE_OUT_NUM_REQUESTS	4
/*-------------------------------------------------------------------------*/
/*                            HID gadget struct                            */

struct f_hidg_zte_req_list {
	struct usb_request	*req;
	unsigned int		 pos;
	struct list_head 	list;
};

struct f_hidg_zte {
	/* configuration */
	unsigned char			bInterfaceSubClass;
	unsigned char			bInterfaceProtocol;
	unsigned short			report_desc_length;
	char				     *report_desc;
	unsigned short			report_length;

	/* recv report */
	struct list_head		completed_out_req;
	spinlock_t				spinlock;
	wait_queue_head_t		read_queue;
	//unsigned int			qlen;

	/* send report */
	spinlock_t				write_spinlock;
	bool					write_pending;
	wait_queue_head_t		write_queue;
	struct usb_request		*req_in;
	struct usb_request		*req_out[HID_ZTE_OUT_NUM_REQUESTS];

	//int				minor;
	struct cdev				cdev;
	struct usb_function		func;

	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;
};

static inline struct f_hidg_zte *func_to_hidg_zte(struct usb_function *f)
{
	return container_of(f, struct f_hidg_zte, func);
}

/*-------------------------------------------------------------------------*/
/*                           Static descriptors                            */

static struct usb_interface_descriptor hidg_zte_interface_desc = {
	.bLength		= sizeof hidg_zte_interface_desc,
	.bDescriptorType	= USB_DT_INTERFACE,
	/* .bInterfaceNumber	= DYNAMIC */
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 2,
	.bInterfaceClass	= USB_CLASS_HID,
	.iInterface			= 0, 
	/* .bInterfaceSubClass	= DYNAMIC */
	/* .bInterfaceProtocol	= DYNAMIC */
	/* .iInterface		= DYNAMIC */
};

static struct hid_descriptor hidg_zte_desc = {
	.bLength			= sizeof hidg_zte_desc,
	.bDescriptorType		= HID_DT_HID,
	.bcdHID				= 0x0111,
	.bCountryCode			= 0x00,
	.bNumDescriptors		= 0x1,
	/*.desc[0].bDescriptorType	= DYNAMIC */
	/*.desc[0].wDescriptorLenght	= DYNAMIC */
};

/* Super-Speed Support */

static struct usb_endpoint_descriptor hidg_zte_ss_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= 16,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor hidg_zte_ss_in_ep_comp_desc = {
	.bLength		= sizeof(hidg_zte_ss_in_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	.bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};



static struct usb_endpoint_descriptor hidg_zte_ss_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= 16,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor hidg_zte_ss_out_ep_comp_desc = {
	.bLength		= sizeof(hidg_zte_ss_out_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	 .bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};

/* High-Speed Support */

static struct usb_endpoint_descriptor hidg_zte_hs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= 16,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_endpoint_descriptor hidg_zte_hs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= 16,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};



/* Full-Speed Support */

static struct usb_endpoint_descriptor hidg_zte_fs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= 16,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};

static struct usb_endpoint_descriptor hidg_zte_fs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize	= 16,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};

static struct usb_descriptor_header *hidg_zte_ss_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_zte_interface_desc,
	(struct usb_descriptor_header *)&hidg_zte_desc,
	(struct usb_descriptor_header *)&hidg_zte_ss_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_zte_ss_in_ep_comp_desc,
	(struct usb_descriptor_header *)&hidg_zte_ss_out_ep_desc,
	(struct usb_descriptor_header *)&hidg_zte_ss_out_ep_comp_desc,
	NULL,
};

static struct usb_descriptor_header *hidg_zte_hs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_zte_interface_desc,
	(struct usb_descriptor_header *)&hidg_zte_desc,
	(struct usb_descriptor_header *)&hidg_zte_hs_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_zte_hs_out_ep_desc,
	NULL,
};

static struct usb_descriptor_header *hidg_zte_fs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_zte_interface_desc,
	(struct usb_descriptor_header *)&hidg_zte_desc,
	(struct usb_descriptor_header *)&hidg_zte_fs_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_zte_fs_out_ep_desc,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                              Char Device                                */

static ssize_t f_hidg_zte_read(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_hidg_zte *hidg = file->private_data;
	struct f_hidg_zte_req_list *list;
	struct usb_request *req;
	unsigned long flags;
	int ret;

	if (!count)
		return 0;
	
	if (hidg->out_ep == NULL || !hidg->out_ep->enabled)
		return -EFAULT;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_WRITE, buffer, count))
		return -EFAULT;
#endif

	spin_lock_irqsave(&hidg->spinlock, flags);

#define READ_COND (!list_empty(&hidg->completed_out_req))

	/* wait for at least one buffer to complete */
	while (!READ_COND) {
		spin_unlock_irqrestore(&hidg->spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(hidg->read_queue, READ_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&hidg->spinlock, flags);
	}

	if (hidg->out_ep == NULL || !hidg->out_ep->enabled)
	{
		spin_unlock_irqrestore(&hidg->spinlock, flags);
		return -EFAULT;
	}
	
	/* pick the first one */
	list = list_first_entry(&hidg->completed_out_req,
				struct f_hidg_zte_req_list, list);
	req = list->req;
	count = min_t(unsigned int, count, req->actual - list->pos);
	//spin_unlock_irqrestore(&hidg->spinlock, flags);

	/* copy to user outside spinlock */
	count -= copy_to_user(buffer, req->buf + list->pos, count);
	list->pos += count;

	/*
	 * if this request is completely handled and transfered to
	 * userspace, remove its entry from the list and requeue it
	 * again. Otherwise, we will revisit it again upon the next
	 * call, taking into account its current read position.
	 */
	if (list->pos == req->actual) {
		//spin_lock_irqsave(&hidg->spinlock, flags);
		list_del(&list->list);
		kfree(list);
		//spin_unlock_irqrestore(&hidg->spinlock, flags);

		req->length = hidg->report_length;
		ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
		if (ret < 0)
		{
			spin_unlock_irqrestore(&hidg->spinlock, flags);
			return ret;
		}
	}
	spin_unlock_irqrestore(&hidg->spinlock, flags);
	return count;
}

static void f_hidg_zte_req_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg_zte *hidg = (struct f_hidg_zte *)req->context;
	unsigned long flags;

	if (!ep->enabled)
		return;

	if (req->status != 0) {
		ERROR(hidg->func.config->cdev,
			"End Point Request ERROR: %d\n", req->status);
	}

	spin_lock_irqsave(&hidg->write_spinlock, flags);
	hidg->write_pending = 0;
	spin_unlock_irqrestore(&hidg->write_spinlock, flags);
	wake_up(&hidg->write_queue);
}

extern int get_usb_link_state(void);

static ssize_t f_hidg_zte_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_hidg_zte *hidg  = file->private_data;
	unsigned long flags;
	ssize_t status = -ENOMEM;
	
	ktime_t timeout;

#if (defined CHIP_RV1126) || (LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0))	
	timeout = 1*1000*1000*1000;
#else
	timeout.tv64 = 1*1000*1000*1000;
#endif
	
	if (hidg->in_ep == NULL || hidg->in_ep->enabled == false || !hidg->req_in)
		return -EFAULT;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;
#endif

	if(get_usb_link_state() != 0)
	{
		#if 0
		if(hidg->in_ep)
		{
			usb_ep_disable(hidg->in_ep);
			//hidg->in_ep->driver_data = NULL;
		}
		#endif
		status = 0xffff;
		goto release_write_pending;
	}

	spin_lock_irqsave(&hidg->write_spinlock, flags);

#define WRITE_COND (!hidg->write_pending)

	/* write queue */
	while (!WRITE_COND) {
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

#if 0
		if (wait_event_interruptible_exclusive(
				hidg->write_queue, WRITE_COND))
			return -ERESTARTSYS;
#else
		if(wait_event_interruptible_hrtimeout(hidg->write_queue, WRITE_COND, timeout))
		{
			//return -ERESTARTSYS;
			status = -ERESTARTSYS;
			goto release_write_pending;
		}
#endif

		spin_lock_irqsave(&hidg->write_spinlock, flags);
	}
	
	
	if (hidg->in_ep == NULL || hidg->in_ep->enabled == false || !hidg->req_in)
	{
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		return -EFAULT;
	}


	count  = min_t(unsigned, count, hidg->report_length);
	status = copy_from_user(hidg->req_in->buf, buffer, count);

	if (status != 0) {
		ERROR(hidg->func.config->cdev,
			"copy_from_user error\n");
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		return -EINVAL;
	}

	hidg->req_in->status   = 0;
	hidg->req_in->zero     = 0;
	hidg->req_in->length   = count;
	hidg->req_in->complete = f_hidg_zte_req_complete;
	hidg->req_in->context  = hidg;
	hidg->write_pending = 1;

	status = usb_ep_queue(hidg->in_ep, hidg->req_in, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		goto release_write_pending;
	} else {
		status = count;
	}

	spin_unlock_irqrestore(&hidg->write_spinlock, flags);


	return status;
	
release_write_pending:
	spin_lock_irqsave(&hidg->write_spinlock, flags);
	hidg->write_pending = 0;
	spin_unlock_irqrestore(&hidg->write_spinlock, flags);

	wake_up(&hidg->write_queue);

	return status;
}

static unsigned int f_hidg_zte_poll(struct file *file, poll_table *wait)
{
	struct f_hidg_zte	*hidg  = file->private_data;
	unsigned int	ret = 0;

	poll_wait(file, &hidg->read_queue, wait);
	poll_wait(file, &hidg->write_queue, wait);

	if (WRITE_COND)
		ret |= POLLOUT | POLLWRNORM;

	if (READ_COND)
		ret |= POLLIN | POLLRDNORM;

	return ret;
}

#undef WRITE_COND
#undef READ_COND

static int f_hidg_zte_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static int f_hidg_zte_open(struct inode *inode, struct file *fd)
{
	struct f_hidg_zte *hidg =
		container_of(inode->i_cdev, struct f_hidg_zte, cdev);

	fd->private_data = hidg;

	return 0;
}

/*-------------------------------------------------------------------------*/
/*                                usb_function                             */

static inline struct usb_request *hidg_zte_alloc_ep_req(struct usb_ep *ep,
						    unsigned length)
{
	return alloc_ep_req(ep, length, length);
}

static void hidg_zte_set_report_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg_zte *hidg = (struct f_hidg_zte *) req->context;
	struct f_hidg_zte_req_list *req_list;
	unsigned long flags;

	req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
	if (!req_list)
		return;

	req_list->req = req;

	spin_lock_irqsave(&hidg->spinlock, flags);
	list_add_tail(&req_list->list, &hidg->completed_out_req);
	spin_unlock_irqrestore(&hidg->spinlock, flags);

	wake_up(&hidg->read_queue);
}

static int hidg_zte_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_hidg_zte			*hidg = func_to_hidg_zte(f);
	struct usb_composite_dev	*cdev = f->config->cdev;
	struct usb_request		*req  = cdev->req;
	int status = 0;
	__u16 value, length;

	value	= __le16_to_cpu(ctrl->wValue);
	length	= __le16_to_cpu(ctrl->wLength);

	VDBG(cdev,
	     "%s crtl_request : bRequestType:0x%x bRequest:0x%x Value:0x%x\n",
	     __func__, ctrl->bRequestType, ctrl->bRequest, value);

	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_REPORT):
		VDBG(cdev, "get_report\n");

		/* send an empty report */
		length = min_t(unsigned, length, hidg->report_length);
		memset(req->buf, 0x0, length);

		goto respond;
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_PROTOCOL):
		VDBG(cdev, "get_protocol\n");
		goto stall;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_REPORT):
		VDBG(cdev, "set_report | wLenght=%d\n", ctrl->wLength);
		goto stall;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_PROTOCOL):
		VDBG(cdev, "set_protocol\n");
		goto stall;
		break;

	case ((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8
		  | USB_REQ_GET_DESCRIPTOR):
		switch (value >> 8) {
		case HID_DT_HID:
			VDBG(cdev, "USB_REQ_GET_DESCRIPTOR: HID\n");
			length = min_t(unsigned short, length,
						   hidg_zte_desc.bLength);
			memcpy(req->buf, &hidg_zte_desc, length);
			goto respond;
			break;
		case HID_DT_REPORT:
			VDBG(cdev, "USB_REQ_GET_DESCRIPTOR: REPORT\n");
			length = min_t(unsigned short, length,
						   hidg->report_desc_length);
			memcpy(req->buf, hidg->report_desc, length);
			goto respond;
			break;

		default:
			VDBG(cdev, "Unknown descriptor request 0x%x\n",
				 value >> 8);
			goto stall;
			break;
		}
		break;

	default:
		VDBG(cdev, "Unknown request 0x%x\n",
			 ctrl->bRequest);
		goto stall;
		break;
	}

stall:
	return -EOPNOTSUPP;

respond:
	req->zero = 0;
	req->length = length;
	status = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
	if (status < 0)
		ERROR(cdev, "usb_ep_queue error on ep0 %d\n", value);
	return status;
}

static int hidg_epin_disable(struct f_hidg_zte *hidg, int free_ep)
{
	if(hidg->in_ep)
	{
		if (hidg->in_ep->enabled)
			usb_ep_disable(hidg->in_ep);
		
		if(hidg->req_in)
		{
			usb_ep_dequeue(hidg->in_ep, hidg->req_in);
			usb_ep_free_request(hidg->in_ep, hidg->req_in);
			if(hidg->req_in->buf)
			{
				kfree(hidg->req_in->buf);
				hidg->req_in->buf = NULL;
			}
			hidg->req_in = NULL;
		}

		if(free_ep)
		{
			hidg->in_ep->driver_data = NULL;
			hidg->in_ep = NULL;
		}
	}
	return 0;
}

static int hidg_epout_disable(struct f_hidg_zte *hidg, int free_ep)
{
	int i;

	if(hidg->out_ep)
	{
		if (hidg->out_ep->enabled)
			usb_ep_disable(hidg->out_ep);
		
		for (i = 0; i < HID_ZTE_OUT_NUM_REQUESTS; ++i)
		{
			if(hidg->req_out[i])
			{
				usb_ep_dequeue(hidg->out_ep, hidg->req_out[i]);
				usb_ep_free_request(hidg->out_ep, hidg->req_out[i]);
				if(hidg->req_out[i]->buf)
				{
					kfree(hidg->req_out[i]->buf);
					hidg->req_out[i]->buf = NULL;
				}
				hidg->req_out[i] = NULL;
			}
		}

		if(free_ep)
		{
			hidg->out_ep->driver_data = NULL;
			hidg->out_ep = NULL;
		}
	}

	return 0;
}

static int hidg_ep_enable(struct usb_function *f, int enable, int free_ep)
{
	struct usb_composite_dev		*cdev = f->config->cdev;
	struct f_hidg_zte				*hidg = func_to_hidg_zte(f);
	int i, status = 0;
	unsigned long flags_write;
	unsigned long flags;

	free_ep = enable == 0 ? free_ep : 0;

	printk("hidg_ep_enable:%d\n", enable);
	
	spin_lock_irqsave(&hidg->write_spinlock, flags_write);
	spin_lock_irqsave(&hidg->spinlock, flags);

	hidg_epin_disable(hidg, free_ep);
	hidg_epout_disable(hidg, free_ep);
	


	if (!enable) 
	{
		spin_unlock_irqrestore(&hidg->write_spinlock, flags_write);
		spin_unlock_irqrestore(&hidg->spinlock, flags);
		return 0;
	}

	if(hidg->in_ep)
	{
		status = config_ep_by_speed(f->config->cdev->gadget, f,
					hidg->in_ep);
		if (status) {
			ERROR(cdev, "config_ep_by_speed FAILED!\n");
			goto fail;
		}
		status = usb_ep_enable(hidg->in_ep);
		if (status < 0) {
			ERROR(cdev, "Enable IN endpoint FAILED!\n");
			goto fail;
		}

		hidg->req_in = hidg_zte_alloc_ep_req(hidg->in_ep, hidg->report_length);
		if (!hidg->req_in)
			goto fail;
	}

	if(hidg->out_ep)
	{
		status = config_ep_by_speed(f->config->cdev->gadget, f,
					    hidg->out_ep);
		if (status) {
			ERROR(cdev, "config_ep_by_speed FAILED!\n");
			goto fail;
		}
		status = usb_ep_enable(hidg->out_ep);
		if (status < 0) {
			ERROR(cdev, "Enable IN endpoint FAILED!\n");
			goto fail;
		}

		for (i = 0; i < HID_ZTE_OUT_NUM_REQUESTS && status == 0; i++) {
			struct usb_request *req =
					hidg_zte_alloc_ep_req(hidg->out_ep,
							  hidg->report_length);
			if (req) {
				hidg->req_out[i] = req;
				req->complete = hidg_zte_set_report_complete;
				req->context  = hidg;
				status = usb_ep_queue(hidg->out_ep, req,
						      GFP_ATOMIC);
				if (status)
					ERROR(cdev, "%s queue req --> %d\n",
						hidg->out_ep->name, status);
				
			} else {
				status = -ENOMEM;
				goto fail;
			}
		}
	}

	spin_unlock_irqrestore(&hidg->write_spinlock, flags_write);
	spin_unlock_irqrestore(&hidg->spinlock, flags);

	return 0;

fail:
	hidg_epin_disable(hidg, free_ep);
	hidg_epout_disable(hidg, free_ep);
	spin_unlock_irqrestore(&hidg->write_spinlock, flags_write);
	spin_unlock_irqrestore(&hidg->spinlock, flags);
	return status;
}

static void hidg_zte_disable(struct usb_function *f)
{
	struct f_hidg_zte *hidg = func_to_hidg_zte(f);
	struct f_hidg_zte_req_list *list, *next;

	hidg_ep_enable(f, 0, 0);

	list_for_each_entry_safe(list, next, &hidg->completed_out_req, list) {
		list_del(&list->list);
		kfree(list);
	}
}

static int hidg_zte_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	//struct usb_composite_dev		*cdev = f->config->cdev;
	//struct f_hidg_zte				*hidg = func_to_hidg_zte(f);

	//if(get_usb_link_state() == 0)
	{
		hidg_ep_enable(f, 1, 0);
	}	

	return 0;
}

static int hid_set_custom_report_desc(struct f_hidg_zte *hidg, unsigned long arg)
{
    int ret, cnt;
    struct zte_hid_custom_report_desc custom_report;
    struct usb_function *f = &hidg->func;
    
    cnt = copy_from_user((void *)&custom_report, (const void *)arg, sizeof(custom_report));
    if(cnt != 0)
    {
        printk("%s +%d %s:copy error\n", __FILE__, __LINE__, __func__);
        return -1;
    }

    if(hidg->report_desc)
		kfree(hidg->report_desc);

	
	hidg->bInterfaceSubClass = custom_report.subclass;
	hidg->bInterfaceProtocol = custom_report.protocol;
	hidg->report_length = custom_report.report_length;
	hidg->report_desc_length = custom_report.report_desc_length;
	hidg->report_desc = kmemdup(custom_report.report_desc,
						custom_report.report_desc_length,
						GFP_KERNEL);


	/* set descriptor dynamic values */
	hidg_zte_interface_desc.bInterfaceSubClass = hidg->bInterfaceSubClass;
	hidg_zte_interface_desc.bInterfaceProtocol = hidg->bInterfaceProtocol;
	
	#if 1
	hidg_zte_ss_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_fs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_ss_out_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_hs_out_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_fs_out_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	#endif
	hidg_zte_ss_in_ep_comp_desc.wBytesPerInterval = hidg_zte_ss_in_ep_desc.wMaxPacketSize;
	hidg_zte_ss_out_ep_comp_desc.wBytesPerInterval = hidg_zte_ss_out_ep_desc.wMaxPacketSize;
	hidg_zte_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	hidg_zte_desc.desc[0].wDescriptorLength =
		cpu_to_le16(hidg->report_desc_length);


    kfree(f->fs_descriptors);
    kfree(f->hs_descriptors);
    kfree(f->ss_descriptors);
    ret = usb_assign_descriptors(f, hidg_zte_fs_descriptors,
			hidg_zte_hs_descriptors, hidg_zte_ss_descriptors, NULL);
	if (ret)
	{
	    printk("%s +%d %s:hid report set error\n", __FILE__, __LINE__, __func__);
	}

    return ret;
}


static long hid_zte_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
#if 0	
	case UVCIOC_USB_CONNECT_STATE:{
		int *type = arg;
		
		if (*type == USB_STATE_CONNECT_RESET ){
			//printk("%s +%d %s:set usb connect reset\n", __FILE__,__LINE__,__func__);		
			usb_soft_connect_reset();	
			return 0;
		}else if(*type == USB_STATE_DISCONNECT){
			//printk("%s +%d %s:set usb disconnect\n", __FILE__,__LINE__,__func__);
		
			usb_soft_disconnect();	
			return 0;
			
		}else if(*type == USB_STATE_CONNECT){
			//printk("%s +%d %s:set usb connect\n", __FILE__,__LINE__,__func__);
			usb_soft_connect();
			return 0;
		}
		else
			return -EINVAL;
	}
	
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
	
	case UVCIOC_GET_HID_REPORT_LEN:{
		uint32_t *value = arg;
		struct f_hidg_zte	*hidg  = file->private_data;
		
		*value = hidg->report_length;
		return 0;	
	}
#endif	
	case ZTEHIDIOC_SET_HID_REPORT_DESC:
        return hid_set_custom_report_desc(file->private_data, arg);
	default:
		printk("%s +%d %s: cmd = 0x%x\n", __FILE__,__LINE__,__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return ret;
}



const struct file_operations f_hidg_zte_fops = {
	.owner		= THIS_MODULE,
	.open		= f_hidg_zte_open,
	.release	= f_hidg_zte_release,
	.write		= f_hidg_zte_write,
	.read		= f_hidg_zte_read,
	.poll		= f_hidg_zte_poll,
	.llseek		= noop_llseek,
	.unlocked_ioctl = hid_zte_ioctl,
};

static int __init hidg_zte_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	struct f_hidg_zte		*hidg = func_to_hidg_zte(f);
	int			status;
	dev_t			dev;
	
	int minor = DEV_HID_ZTE_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_ZTE_NUM_MAX)
		index = DEV_HID_ZTE_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	

	#if 1
	hidg_zte_ss_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_fs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_ss_out_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_hs_out_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_zte_fs_out_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	#endif
	hidg_zte_ss_in_ep_comp_desc.wBytesPerInterval = hidg_zte_ss_in_ep_desc.wMaxPacketSize;
	hidg_zte_ss_out_ep_comp_desc.wBytesPerInterval = hidg_zte_ss_out_ep_desc.wMaxPacketSize;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	hidg_zte_interface_desc.bInterfaceNumber = status;

	/* allocate instance-specific endpoints */
	status = -ENODEV;
	ep = usb_ep_autoconfig(c->cdev->gadget, &hidg_zte_fs_in_ep_desc);
	if (!ep)
		goto fail;
	ep->driver_data = hidg;	/* claim */
	hidg->in_ep = ep;

	ep = usb_ep_autoconfig(c->cdev->gadget, &hidg_zte_fs_out_ep_desc);
	if (!ep)
		goto fail;
	ep->driver_data = hidg;	/* claim */
	hidg->out_ep = ep;

	/* preallocate request and buffer */
	status = -ENOMEM;
	//hidg->req_in = hidg_zte_alloc_ep_req(hidg->in_ep, hidg->report_length);
	//if (!hidg->req_in)
	//	goto fail;

	/* set descriptor dynamic values */
	hidg_zte_interface_desc.bInterfaceSubClass = hidg->bInterfaceSubClass;
	hidg_zte_interface_desc.bInterfaceProtocol = hidg->bInterfaceProtocol;
	

	hidg_zte_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	hidg_zte_desc.desc[0].wDescriptorLength =
		cpu_to_le16(hidg->report_desc_length);

	hidg_zte_ss_in_ep_desc.bEndpointAddress =	
	hidg_zte_hs_in_ep_desc.bEndpointAddress =
		hidg_zte_fs_in_ep_desc.bEndpointAddress;
	
	hidg_zte_ss_out_ep_desc.bEndpointAddress =	
	hidg_zte_hs_out_ep_desc.bEndpointAddress =
		hidg_zte_fs_out_ep_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, hidg_zte_fs_descriptors,
			hidg_zte_hs_descriptors, hidg_zte_ss_descriptors, NULL);
	if (status)
		goto fail;

	spin_lock_init(&hidg->write_spinlock);
	spin_lock_init(&hidg->spinlock);
	init_waitqueue_head(&hidg->write_queue);
	init_waitqueue_head(&hidg->read_queue);
	INIT_LIST_HEAD(&hidg->completed_out_req);

	/* create char device */
	cdev_init(&hidg->cdev, &f_hidg_zte_fops);
	dev = MKDEV(zte_major[index], minor);
	
	status = cdev_add(&hidg->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device_create(hidg_zte_class, NULL, dev, NULL, "%s%d", "zte_hidg", minor - DEV_HID_ZTE_FIRST_NUM);

	return 0;

fail_free_descs:
	usb_free_all_descriptors(f);
fail:
	ERROR(f->config->cdev, "hidg_bind2 FAILED\n");
	hidg_ep_enable(f, 0, 1);

	return status;
}

static void hidg_zte_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_hidg_zte *hidg = func_to_hidg_zte(f);

	int minor = DEV_HID_ZTE_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_ZTE_NUM_MAX)
		index = DEV_HID_ZTE_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	

	device_destroy(hidg_zte_class, MKDEV(zte_major[index], minor));

	cdev_del(&hidg->cdev);

	/* disable/free request and end point */
	hidg_ep_enable(f, 0, 1);

	ghid_zte_cleanup(c->bConfigurationValue);

	usb_free_all_descriptors(f);

	kfree(hidg->report_desc);
	kfree(hidg);
}

/*-------------------------------------------------------------------------*/
/*                                 Strings                                 */

#define CT_FUNC_HID_IDX	0

static struct usb_string ct_func_string_defs_zte[] = {
	[CT_FUNC_HID_IDX].s	= "HID Interface",
	{},			/* end of list */
};

static struct usb_gadget_strings ct_func_string_table_zte = {
	.language	= 0x0409,	/* en-US */
	.strings	= ct_func_string_defs_zte,
};

static struct usb_gadget_strings *ct_func_strings_zte[] = {
	&ct_func_string_table_zte,
	NULL,
};


static void hidg_zte_suspend(struct usb_function *f)
{
	// if(get_usb_link_state() == 0)
	// {
	// 	hidg_ep_enable(f, 0, 0);
	// }
}

static void hidg_zte_resume(struct usb_function *f)
{
	// if(get_usb_link_state() == 0)
	// {
	// 	hidg_ep_enable(f, 1, 0);
	// }	
}


/*-------------------------------------------------------------------------*/
/*                             usb_configuration                           */

int __init hidg_zte_bind_config(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc)
{
	struct f_hidg_zte *hidg;
	int status;

	
#if 0
	/* maybe allocate device-global string IDs, and patch descriptors */
	if (ct_func_string_defs[CT_FUNC_HID_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ct_func_string_defs[CT_FUNC_HID_IDX].id = status;
		hidg_zte_interface_desc.iInterface = status;
	}
#endif

	/* allocate and initialize one new instance */
	hidg = kzalloc(sizeof *hidg, GFP_KERNEL);
	if (!hidg){
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}

	if(zte_hid_ep_size > 1024)
		zte_hid_ep_size = 1024;
	hid_zte_des.report_length = zte_hid_ep_size;
	hid_zte_des.report_desc[19] = zte_hid_ep_size;

	if(fdesc == NULL){
		hidg->bInterfaceSubClass = hid_zte_des.subclass;
		hidg->bInterfaceProtocol = hid_zte_des.protocol;
		hidg->report_length = hid_zte_des.report_length;
		hidg->report_desc_length = hid_zte_des.report_desc_length;
		hidg->report_desc = kmemdup(hid_zte_des.report_desc,
						hid_zte_des.report_desc_length,
						GFP_KERNEL);
		
	}else{
		hidg->bInterfaceSubClass = fdesc->subclass;
		hidg->bInterfaceProtocol = fdesc->protocol;
		hidg->report_length = fdesc->report_length;
		hidg->report_desc_length = fdesc->report_desc_length;
		hidg->report_desc = kmemdup(fdesc->report_desc,
				    fdesc->report_desc_length,
				    GFP_KERNEL);		
	}
										
	if (!hidg->report_desc) {
		kfree(hidg);
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}

	hidg->func.name    = "hid_zte";
	hidg->func.strings = ct_func_strings_zte;
	hidg->func.bind    = hidg_zte_bind;
	hidg->func.unbind  = hidg_zte_unbind;
	hidg->func.set_alt = hidg_zte_set_alt;
	hidg->func.disable = hidg_zte_disable;
	hidg->func.setup   = hidg_zte_setup;
	hidg->func.suspend = hidg_zte_suspend;
	hidg->func.resume = hidg_zte_resume;

	/* this could me made configurable at some point */
	//hidg->qlen	   = 4;

	status = usb_add_function(c, &hidg->func);
	if (status)
		kfree(hidg);

	printk("%d %s: hidg_bind_config ok!\n",__LINE__,__func__);
	
	return status;
}

int __init ghid_zte_setup(struct usb_gadget *g, u8 bConfigurationValue)
{
	int status;
	dev_t dev;
	
	int count = DEV_HID_ZTE_FIRST_NUM + bConfigurationValue;
	
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_ZTE_NUM_MAX)
		index = DEV_HID_ZTE_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	if(hidg_zte_class == NULL)
		hidg_zte_class = class_create(THIS_MODULE, "zte_hidg");
     
	status = alloc_chrdev_region(&dev, 0, count, "zte_hidg");
	if (!status) {
		zte_major[index] = MAJOR(dev);
		zte_minors[index] = count;
	}

	return status;
}

void ghid_zte_cleanup(u8 bConfigurationValue)
{
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_ZTE_NUM_MAX)
		index = DEV_HID_ZTE_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	if (zte_major[index]) {
		unregister_chrdev_region(MKDEV(zte_major[index], 0), zte_minors[index]);
		zte_major[index] = zte_minors[index] = 0;
	}

	if(hidg_zte_class != NULL && bConfigurationValue == 1)
	{
		class_destroy(hidg_zte_class);
		hidg_zte_class = NULL;
	}
}





