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

#include "u_f.h"
#include "uvc.h"


//#define UVCIOC_USB_CONNECT_STATE	_IOW('U', 2, int)
//#define UVCIOC_GET_USB_SOFT_CONNECT_STATE	_IOR('U', 6, uint32_t)

#define HID_REPORT_LENGTH_MAX       256


static int major[DEV_HID_KEYBOARD_NUM_MAX], minors[DEV_HID_KEYBOARD_NUM_MAX];
static struct class *hidg_class;

#define KEYBOARD_REPORT_ID	0x03

static unsigned char  report_desc[] = {
	
	//keyboard
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
	0x85, KEYBOARD_REPORT_ID,	   //   REPORT_ID (3)
	0x05, 0x08,                    //   USAGE_PAGE (LEDs)
	0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
	0x29, 0x03,                    //   USAGE_MAXIMUM (Scroll Lock)		
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
	0x95, 0x03,					   //   REPORT_COUNT (3) 
	0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
	0x95, 0x05,                    //   REPORT_COUNT (5) 
	0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs) 
	0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x95, 0x08,                    //   REPORT_COUNT (8) 
	0x81, 0x02,                    //   INPUT (Data,Var,Abs) 
	0x75, 0x08,                    //   REPORT_SIZE (8) 
	0x95, 0x01,                    //   REPORT_COUNT (1) 
	0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
	0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated)) 
	0x29, 0x91, 				   //   USAGE_MAXIMUM (??)
	0x26, 0xff, 0x00,  			   //   LOGICAL_MAXIMUM (255)
	0x95, 0x06,                    //   REPORT_COUNT (6) 
	0x81, 0x00,                    //   INPUT (Data,Ary,Abs) 
	0xc0,                          // END_COLLECTION
};

static struct hidg_func_descriptor hid_des= {
	.subclass = 0,
	.protocol = 0,
	.report_length = 64,
	//.report_desc_length= 22,
	//.report_desc
};


/*-------------------------------------------------------------------------*/
/*                            HID gadget struct                            */

struct f_hidg_req_list {
	struct usb_request	*req;
	unsigned int		pos;
	struct list_head 	list;
};

struct f_hidg {
	/* configuration */
	unsigned char			bInterfaceSubClass;
	unsigned char			bInterfaceProtocol;
	unsigned short			report_desc_length;
	char					*report_desc;
	unsigned short			report_length;

	/* recv report */
	//struct list_head		completed_out_req;
	//spinlock_t			spinlock;
	//wait_queue_head_t		read_queue;
	unsigned int			qlen;

	/* send report */
	spinlock_t				write_spinlock;
	bool					write_pending;
	wait_queue_head_t		write_queue;
	struct usb_request		*req;

	//int					minor;
	struct cdev				cdev;
	struct usb_function		func;

	struct usb_ep			*in_ep;
	//struct usb_ep			*out_ep;
	
	unsigned char 			hid_idle;

};

void ghid_cleanup_keyboard(u8 bConfigurationValue);

static inline struct f_hidg *func_to_hidg(struct usb_function *f)
{
	return container_of(f, struct f_hidg, func);
}

/*-------------------------------------------------------------------------*/
/*                           Static descriptors                            */

static struct usb_interface_descriptor hidg_interface_desc = {
	.bLength		= sizeof hidg_interface_desc,
	.bDescriptorType	= USB_DT_INTERFACE,
	/* .bInterfaceNumber	= DYNAMIC */
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_HID,
	.iInterface			= 0, 
	/* .bInterfaceSubClass	= DYNAMIC */
	/* .bInterfaceProtocol	= DYNAMIC */
	/* .iInterface		= DYNAMIC */
};

static struct hid_descriptor hidg_desc = {
	.bLength			= sizeof hidg_desc,
	.bDescriptorType		= HID_DT_HID,
	.bcdHID				= 0x0111,
	.bCountryCode			= 0x00,
	.bNumDescriptors		= 0x1,
	/*.desc[0].bDescriptorType	= DYNAMIC */
	/*.desc[0].wDescriptorLenght	= DYNAMIC */
};

/* Super-Speed Support */

static struct usb_endpoint_descriptor hidg_ss_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor hidg_ss_in_ep_comp_desc = {
	.bLength		= sizeof(hidg_ss_in_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	.bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};


#if 0
static struct usb_endpoint_descriptor hidg_ss_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};
#endif

static struct usb_ss_ep_comp_descriptor hidg_ss_out_ep_comp_desc = {
	.bLength		= sizeof(hidg_ss_out_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	 .bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};

/* High-Speed Support */

static struct usb_endpoint_descriptor hidg_hs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

#if 0
static struct usb_endpoint_descriptor hidg_hs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};
#endif


/* Full-Speed Support */

static struct usb_endpoint_descriptor hidg_fs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};

#if 0
static struct usb_endpoint_descriptor hidg_fs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};
#endif

static struct usb_descriptor_header *hidg_ss_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,

	(struct usb_descriptor_header *)&hidg_ss_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_ss_in_ep_comp_desc,

	NULL,
};

static struct usb_descriptor_header *hidg_hs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,

	(struct usb_descriptor_header *)&hidg_hs_in_ep_desc,
	NULL,
};

static struct usb_descriptor_header *hidg_fs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,
	(struct usb_descriptor_header *)&hidg_fs_in_ep_desc,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                              Char Device                                */


static void f_hidg_req_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg *hidg = (struct f_hidg *)req->context;
	unsigned long flags;

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

static ssize_t f_hidg_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_hidg *hidg  = file->private_data;
	ssize_t status = -ENOMEM;
	unsigned long flags;

	ktime_t timeout;
	
#if (defined CHIP_RV1126) || (LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0))	
	timeout = 1*1000*1000*1000;
#else
	timeout.tv64 = 1*1000*1000*1000;
#endif

	if (hidg->in_ep == NULL)
		return -EFAULT;
	
	if (hidg->in_ep->enabled == false)
		return -EFAULT;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;
#endif

#if 1
	if(get_usb_link_state()!= 0)
	{
		if(hidg->in_ep)
		{
			usb_ep_disable(hidg->in_ep);
			//hidg->in_ep->driver_data = NULL;
		}
		status = -EFAULT;
		goto release_write_pending;
	}
#endif

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

	count  = min_t(unsigned, count, hidg->report_length);
	status = copy_from_user(hidg->req->buf, buffer, count);

	if (status != 0) {
		ERROR(hidg->func.config->cdev,
			"copy_from_user error\n");
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		return -EINVAL;
	}

	hidg->req->status   = 0;
	hidg->req->zero     = 0;
	hidg->req->length   = count;
	hidg->req->complete = f_hidg_req_complete;
	hidg->req->context  = hidg;
	hidg->write_pending = 1;

	status = usb_ep_queue(hidg->in_ep, hidg->req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		hidg->write_pending = 0;
		wake_up(&hidg->write_queue);
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

static unsigned int f_hidg_poll(struct file *file, poll_table *wait)
{
	struct f_hidg	*hidg  = file->private_data;
	unsigned int	ret = 0;

	poll_wait(file, &hidg->write_queue, wait);
	if (WRITE_COND)
		ret |= POLLOUT | POLLWRNORM;

	return ret;
}

#undef WRITE_COND

static int f_hidg_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static int f_hidg_open(struct inode *inode, struct file *fd)
{
	struct f_hidg *hidg =
		container_of(inode->i_cdev, struct f_hidg, cdev);

	fd->private_data = hidg;

	return 0;
}

/*-------------------------------------------------------------------------*/
/*                                usb_function                             */

static inline struct usb_request *hidg_alloc_ep_req(struct usb_ep *ep,
						    unsigned length)
{
	return alloc_ep_req(ep, length, length);
}



static int hidg_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_hidg			*hidg = func_to_hidg(f);
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
						   hidg_desc.bLength);
			memcpy(req->buf, &hidg_desc, length);
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

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_IDLE):
		VDBG(cdev, "get_idle\n");

		length = 1;
		((uint32_t *)req->buf)[0] = hidg->hid_idle;
		goto respond;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_IDLE):
		VDBG(cdev, "set_idle\n");

		hidg->hid_idle = (value >> 8) & 0xFF;
		
		length = 0;
		goto respond;
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

static void hidg_disable(struct usb_function *f)
{
	struct f_hidg *hidg = func_to_hidg(f);
	//struct f_hidg_req_list *list, *next;

	if(hidg->in_ep)
		usb_ep_disable(hidg->in_ep);

	//hidg->in_ep->driver_data = NULL;

	//list_for_each_entry_safe(list, next, &hidg->completed_out_req, list) {
	//	list_del(&list->list);
	//	kfree(list);
	//}
}

static int hidg_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev		*cdev = f->config->cdev;
	struct f_hidg				*hidg = func_to_hidg(f);
	int status = 0;

	VDBG(cdev, "hidg_set_alt intf:%d alt:%d\n", intf, alt);


	if (hidg->in_ep != NULL) {
		/* restart endpoint */
		if (hidg->in_ep->enabled)
			usb_ep_disable(hidg->in_ep);

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
		//hidg->in_ep->driver_data = hidg;
	}




fail:
	return status;
}



static const struct file_operations f_hidg_fops = {
	.owner		= THIS_MODULE,
	.open		= f_hidg_open,
	.release	= f_hidg_release,
	.write		= f_hidg_write,
	.read		= NULL,
	.poll		= f_hidg_poll,
	.llseek		= noop_llseek,
	.unlocked_ioctl = NULL,
};

static int __init hidg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	struct f_hidg		*hidg = func_to_hidg(f);
	int			status;
	dev_t			dev;
	
	int config_minor = DEV_HID_KEYBOARD_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_KEYBOARD_NUM_MAX)
		index = DEV_HID_KEYBOARD_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	hidg_interface_desc.bInterfaceNumber = status;
	hidg_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	hidg_desc.desc[0].wDescriptorLength =
		cpu_to_le16(hidg->report_desc_length);
		
	hidg_interface_desc.bInterfaceSubClass = hidg->bInterfaceSubClass;
	hidg_interface_desc.bInterfaceProtocol = hidg->bInterfaceProtocol;


	/* allocate instance-specific endpoints */
	status = -ENODEV;
	ep = usb_ep_autoconfig_ss_backwards(c->cdev->gadget, &hidg_hs_in_ep_desc, NULL);
	if (!ep)
		goto fail;
	ep->driver_data = hidg;	/* claim */
	hidg->in_ep = ep;


	/* preallocate request and buffer */
	status = -ENOMEM;
	hidg->req = usb_ep_alloc_request(hidg->in_ep, GFP_KERNEL);
	if (!hidg->req)
		goto fail;

	hidg->req->buf = kmalloc(hidg->report_length, GFP_KERNEL);
	if (!hidg->req->buf)
		goto fail;

	/* set descriptor dynamic values */
	
	hidg_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length);
	hidg_hs_in_ep_desc.bEndpointAddress = hidg->in_ep->address;

	hidg_fs_in_ep_desc.wMaxPacketSize = hidg_hs_in_ep_desc.wMaxPacketSize;
	hidg_fs_in_ep_desc.bEndpointAddress = hidg_hs_in_ep_desc.bEndpointAddress;

	hidg_ss_in_ep_desc.wMaxPacketSize = hidg_hs_in_ep_desc.wMaxPacketSize;
	hidg_ss_in_ep_comp_desc.wBytesPerInterval = hidg_ss_in_ep_desc.wMaxPacketSize;
	hidg_ss_in_ep_desc.bEndpointAddress = hidg_hs_in_ep_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, hidg_fs_descriptors,
			hidg_hs_descriptors, hidg_ss_descriptors, NULL);
	if (status)
		goto fail;
	
	spin_lock_init(&hidg->write_spinlock);
	//spin_lock_init(&hidg->spinlock);
	init_waitqueue_head(&hidg->write_queue);
	//init_waitqueue_head(&hidg->read_queue);
	//INIT_LIST_HEAD(&hidg->completed_out_req);

	/* create char device */
	cdev_init(&hidg->cdev, &f_hidg_fops);
	dev = MKDEV(major[index], config_minor);
	status = cdev_add(&hidg->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device_create(hidg_class, NULL, dev, NULL, "%s%d", "hidg_keyboard", config_minor - DEV_HID_KEYBOARD_FIRST_NUM);

	return 0;

fail_free_descs:
	usb_free_all_descriptors(f);
fail:
	ERROR(f->config->cdev, "hidg_bind FAILED\n");
	if (hidg->req != NULL) {
		kfree(hidg->req->buf);
		if (hidg->in_ep != NULL)
			usb_ep_free_request(hidg->in_ep, hidg->req);
	}

	return status;
}

static void hidg_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_hidg *hidg = func_to_hidg(f);

	int config_minor = DEV_HID_KEYBOARD_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_KEYBOARD_NUM_MAX)
		index = DEV_HID_KEYBOARD_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	device_destroy(hidg_class, MKDEV(major[index], config_minor));
	cdev_del(&hidg->cdev);


	/* disable/free request and end point */
	if(hidg->in_ep)
	{
		usb_ep_disable(hidg->in_ep);	
		if(hidg->req)
		{
			usb_ep_dequeue(hidg->in_ep, hidg->req);
			usb_ep_free_request(hidg->in_ep, hidg->req);
		}
		hidg->in_ep->driver_data = NULL;
	}

	if(hidg->req->buf)
		kfree(hidg->req->buf);

	
	ghid_cleanup_keyboard(c->bConfigurationValue);

	usb_free_all_descriptors(f);

	kfree(hidg->report_desc);
	kfree(hidg);
}

/*-------------------------------------------------------------------------*/
/*                                 Strings                                 */

#define CT_FUNC_HID_IDX	0

static struct usb_string ct_func_string_defs[] = {
	[CT_FUNC_HID_IDX].s	= "HID Interface",
	{},			/* end of list */
};

static struct usb_gadget_strings ct_func_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= ct_func_string_defs,
};

static struct usb_gadget_strings *ct_func_strings[] = {
	&ct_func_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                             usb_configuration                           */

int __init hidg_bind_config_keyboard(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc)
{
	struct f_hidg *hidg;
	int status;

	
#if 0
	/* maybe allocate device-global string IDs, and patch descriptors */
	if (ct_func_string_defs[CT_FUNC_HID_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ct_func_string_defs[CT_FUNC_HID_IDX].id = status;
		hidg_interface_desc.iInterface = status;
	}
#endif

	/* allocate and initialize one new instance */
	hidg = kzalloc(sizeof *hidg, GFP_KERNEL);
	if (!hidg){
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}
		

	//hidg->minor = index;


	if(fdesc == NULL){
		hid_des.report_desc_length = sizeof(report_desc);
		hidg->bInterfaceSubClass = hid_des.subclass;
		hidg->bInterfaceProtocol = hid_des.protocol;
		hidg->report_length = hid_des.report_length;
		hidg->report_desc_length = hid_des.report_desc_length;
		hidg->report_desc = kmemdup(report_desc,//hid_des.report_desc,
						hid_des.report_desc_length,
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

	hidg->func.name    = "hid_keyboard";
	hidg->func.strings = ct_func_strings;
	hidg->func.bind    = hidg_bind;
	hidg->func.unbind  = hidg_unbind;
	hidg->func.set_alt = hidg_set_alt;
	hidg->func.disable = hidg_disable;
	hidg->func.setup   = hidg_setup;

	/* this could me made configurable at some point */
	hidg->qlen	   = 4;

	status = usb_add_function(c, &hidg->func);
	if (status)
		kfree(hidg);

	printk("+%d %s: keyboard hidg_bind_config ok!\n",__LINE__,__func__);
	
	return status;
}
EXPORT_SYMBOL(hidg_bind_config_keyboard);

int __init ghid_setup_keyboard(struct usb_gadget *g, u8 bConfigurationValue)
{
	int status;
	dev_t dev;
	
	int count = DEV_HID_KEYBOARD_FIRST_NUM + bConfigurationValue;
	
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_KEYBOARD_NUM_MAX)
		index = DEV_HID_KEYBOARD_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	if(hidg_class == NULL)
		hidg_class = class_create(THIS_MODULE, "keyboard_hidg");
     
	status = alloc_chrdev_region(&dev, 0, count, "keyboard_hidg");
	if (!status) {
		major[index] = MAJOR(dev);
		minors[index] = count;
	}

	return status;
}
EXPORT_SYMBOL(ghid_setup_keyboard);

void ghid_cleanup_keyboard(u8 bConfigurationValue)
{
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_KEYBOARD_NUM_MAX)
		index = DEV_HID_KEYBOARD_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	if (major[index]) {
		unregister_chrdev_region(MKDEV(major[index], 0), minors[index]);
		major[index] = minors[index] = 0;
	}

	if(hidg_class != NULL && bConfigurationValue == 1)
	{
		class_destroy(hidg_class);
		hidg_class = NULL;
	}
}
EXPORT_SYMBOL(ghid_cleanup_keyboard);

