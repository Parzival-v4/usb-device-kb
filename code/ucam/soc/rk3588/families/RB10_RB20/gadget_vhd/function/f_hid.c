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
#include <linux/version.h>

//#include "hi_osal.h"

#include "u_f.h"
#include "uvc.h"



//#define HID_EP_USB_CV_TEST

#define HID_REPORT_LENGTH_MAX       2048


#ifdef HID_EP_USB_CV_TEST
static unsigned int hid_ep_size = 16;
#else
static unsigned int hid_ep_size = 64;
#endif
module_param(hid_ep_size, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(hid_ep_size, "hid ep max packet size");


static int major[DEV_HID_NUM_MAX], minors[DEV_HID_NUM_MAX];

static struct class *hidg_class = NULL;

void ghid_cleanup(u8 bConfigurationValue);


struct hidg_func_descriptor hid_des_ep_interrupt= {
	.subclass = 0,
	.protocol = 0,
	.report_length = 256,
	.report_desc_length= 30,
	.report_desc= {
		0x06,0x00,0xFF,			//USAGE_PAGE (Vendor Defined Page 1)
		0x09,0x00,				//USAGE (Vendor Usage 1)
		0xA1,0x01,				//COLLECTION (Application)

		0x19,0x01,				//(Vendor Usage 1)
		0x29,0x08,				//(Vendor Usage 1)
		0x15,0x00,				//LOGICAL_MINIMUM (0)
		0x26,0xFF,0x00,			//LOGICAL_MAXIMUM (255)
		0x75,0x08,				//REPORT_SIZE (8)
		//0x95,0x40,				//REPORT_COUNT (64)
		0x96,0x00,0x01,				//REPORT_COUNT (256)
		0x81,0x02,				//INPUT (Data,Var,Abs)

		0x19,0x01,				//(Vendor Usage 1)
		0x29,0x08,				//(Vendor Usage 1)
		0x91,0x02,				//OUTPUT (Data,Var,Abs)

		0xC0					// END_COLLECTION
		}
};	

struct hidg_func_descriptor hid_des_ep0= {
	.subclass = 0,
	.protocol = 0,
	.report_length = 2048,
	.report_desc_length= 22,
	.report_desc= {
		0x06,0x00,0xFF,			//USAGE_PAGE (Vendor Defined Page 1)
		0x09,0x00,				//USAGE (Vendor Usage 1)
		0xA1,0x01,				//COLLECTION (Application)

		0x09,0x01,				//(Vendor Usage 1)
		0x15,0x00,				//LOGICAL_MINIMUM (0)
		0x26,0xFF,0x00,			//LOGICAL_MAXIMUM (255)
		//0x85, 0x01,             // Report ID (1)
		0x75,0x08,				//REPORT_SIZE (8)
		0x96, 0x00, 0x08,       // REPORT_COUNT (2048)
		0xB1,0x02,				//FEATURE (Data,Var,Abs)

		0xC0					// END_COLLECTION
		}
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
	uint32_t				report_desc_length;
	char					*report_desc;
	uint32_t				report_length;
	
	
	//int						minor;
	struct cdev				cdev;
	struct usb_function		func;
	
	struct list_head		completed_out_req;
	spinlock_t				read_spinlock;
	wait_queue_head_t		read_queue;
	
	spinlock_t				write_spinlock;
	bool					write_pending;
	wait_queue_head_t		write_queue;


	//usb control ep 
	unsigned int 			control_intf;
	struct usb_ep 			*control_ep;
	struct usb_request 		*control_req;
	void 					*control_buf;


	unsigned int 			hid_interrupt_ep_enable;
	unsigned int			qlen;
	struct usb_request		*req;

	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;

	
	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
	
	unsigned char 			hid_idle;
	
	
	ssize_t (*f_write)(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp);

	ssize_t (*f_read)(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr);

};

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
	.bNumEndpoints		= 0,
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
	//.wMaxPacketSize	= HID_EP_MAX_PACKET_SIZE,
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
	//.wBytesPerInterval = HID_EP_MAX_PACKET_SIZE, /*DYNAMIC*/
};



static struct usb_endpoint_descriptor hidg_ss_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	//.wMaxPacketSize	= HID_EP_MAX_PACKET_SIZE,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor hidg_ss_out_ep_comp_desc = {
	.bLength		= sizeof(hidg_ss_out_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	 .bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = HID_EP_MAX_PACKET_SIZE, /*DYNAMIC*/
};

/* High-Speed Support */

static struct usb_endpoint_descriptor hidg_hs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	//.wMaxPacketSize	= HID_EP_MAX_PACKET_SIZE,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_endpoint_descriptor hidg_hs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	//.wMaxPacketSize	= HID_EP_MAX_PACKET_SIZE,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};



/* Full-Speed Support */

static struct usb_endpoint_descriptor hidg_fs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	//.wMaxPacketSize	= HID_EP_MAX_PACKET_SIZE,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};

static struct usb_endpoint_descriptor hidg_fs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	//.wMaxPacketSize	= HID_EP_MAX_PACKET_SIZE,
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};

static struct usb_descriptor_header *hidg_ss_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,
	
	(struct usb_descriptor_header *)&hidg_ss_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_ss_in_ep_comp_desc,
	(struct usb_descriptor_header *)&hidg_ss_out_ep_desc,
	(struct usb_descriptor_header *)&hidg_ss_out_ep_comp_desc,

	NULL,
};

static struct usb_descriptor_header *hidg_hs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,	

	(struct usb_descriptor_header *)&hidg_hs_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_hs_out_ep_desc,

	NULL,
};

static struct usb_descriptor_header *hidg_fs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,

	(struct usb_descriptor_header *)&hidg_fs_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_fs_out_ep_desc,

	NULL,
};

	


/*-------------------------------------------------------------------------*/
/*                              Char Device                                */

static void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}

static void hidg_set_report_ep_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_composite_dev *cdev;
	struct f_hidg_req_list *req_list;
	unsigned long flags;
	struct f_hidg *hidg = (struct f_hidg *) req->context;

	if (!ep->enabled)
		return;
#if 1
	
	cdev = hidg->func.config->cdev;
	switch (req->status) {
	case 0:
		req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
		if (!req_list) {
			ERROR(cdev, "Unable to allocate mem for req_list\n");
			goto free_req;
		}

		req_list->req = req;

		spin_lock_irqsave(&hidg->read_spinlock, flags);
		list_add_tail(&req_list->list, &hidg->completed_out_req);
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);

		wake_up(&hidg->read_queue);
		break;
	default:
		ERROR(cdev, "Set report failed %d\n", req->status);
		fallthrough;
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
free_req:
		free_ep_req(ep, req);
		return;
	}
#else
	
	req->complete = hidg_set_report_complete;
	req->context  = hidg;
	req->length = hid_ep_size;
	int ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
	if (ret < 0) {
		free_ep_req(hidg->out_ep, req);
		return ret;
	}	
#endif
}


static void f_hidg_req_complete_ep_in(struct usb_ep *ep, struct usb_request *req)
{
	//struct f_hidg *hidg = (struct f_hidg *)ep->driver_data;
	struct f_hidg *hidg = (struct f_hidg *)req->context;
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

static ssize_t f_hidg_read_ep_out(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_hidg *hidg = file->private_data;
	struct f_hidg_req_list *list;
	struct usb_request *req;
	unsigned long flags;
	int ret;

	if (!hidg->out_ep->enabled)
		return -EFAULT;

	if (!count)
		return 0;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
			return -EFAULT;
#else
	if (!access_ok(VERIFY_WRITE, buffer, count))
			return -EFAULT;
#endif
	

	spin_lock_irqsave(&hidg->read_spinlock, flags);

#define READ_COND (!list_empty(&hidg->completed_out_req))

	/* wait for at least one buffer to complete */
	while (!READ_COND) {
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(hidg->read_queue, READ_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&hidg->read_spinlock, flags);
	}

	/* pick the first one */
	list = list_first_entry(&hidg->completed_out_req,
				struct f_hidg_req_list, list);

	/*
	 * Remove this from list to protect it from beign free()
	 * while host disables our function
	 */
	list_del(&list->list);

	req = list->req;
	count = min_t(unsigned int, count, req->actual - list->pos);
	spin_unlock_irqrestore(&hidg->read_spinlock, flags);

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
		kfree(list);

		req->length = hidg->report_length;
		ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
		if (ret < 0) {
			free_ep_req(hidg->out_ep, req);
			return ret;
		}
	} else {
		spin_lock_irqsave(&hidg->read_spinlock, flags);
		list_add(&list->list, &hidg->completed_out_req);
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);

		wake_up(&hidg->read_queue);
	}

	return count;
}


static ssize_t f_hidg_write_ep_in(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_hidg *hidg  = file->private_data;
	unsigned long flags;
	ssize_t status = -ENOMEM;
	
	
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

	spin_lock_irqsave(&hidg->write_spinlock, flags);

#define WRITE_COND (!hidg->write_pending)

	/* write queue */
	while (!WRITE_COND) {
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible_exclusive(
				hidg->write_queue, WRITE_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&hidg->write_spinlock, flags);
	}

	hidg->write_pending = 1;
	count  = min_t(unsigned, count, hidg->report_length);

	spin_unlock_irqrestore(&hidg->write_spinlock, flags);
	status = copy_from_user(hidg->req->buf, buffer, count);

	if (status != 0) {
		ERROR(hidg->func.config->cdev,
			"copy_from_user error\n");
		status = -EINVAL;
		goto release_write_pending;
	}

	hidg->req->status   = 0;
	hidg->req->zero     = 0;
	hidg->req->length   = count;
	hidg->req->complete = f_hidg_req_complete_ep_in;
	hidg->req->context  = hidg;

	status = usb_ep_queue(hidg->in_ep, hidg->req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		goto release_write_pending;
	} else {
		status = count;
	}

	return status;
release_write_pending:
	spin_lock_irqsave(&hidg->write_spinlock, flags);
	hidg->write_pending = 0;
	spin_unlock_irqrestore(&hidg->write_spinlock, flags);

	wake_up(&hidg->write_queue);

	return status;
}



static void hidg_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg *hidg = (struct f_hidg *) req->context;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct f_hidg_req_list *req_list;
	unsigned long flags;

	//if (!ep->enabled)
	//	return;
	
	if (hidg->event_setup_out) 
	{

		switch (req->status) {
		case 0:
			req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
			if (!req_list) {
				ERROR(cdev, "Unable to allocate mem for req_list\n");
				goto free_req;
			}

			req_list->req = req;

			spin_lock_irqsave(&hidg->read_spinlock, flags);
			list_add_tail(&req_list->list, &hidg->completed_out_req);
			spin_unlock_irqrestore(&hidg->read_spinlock, flags);

			wake_up(&hidg->read_queue);
			break;
		default:
			ERROR(cdev, "Set report failed %d\n", req->status);
			fallthrough;
		case -ECONNABORTED:		/* hardware forced ep reset */
		case -ECONNRESET:		/* request dequeued */
		case -ESHUTDOWN:		/* disconnect from host */
free_req:
			free_ep_req(ep, req);
			return;
		}

		hidg->event_setup_out = 0;
	}
	else
	{

	}
}

static int
hidg_usb_ep0_out_data(struct f_hidg	*hidg, const struct usb_ctrlrequest *ctrl)
{
	
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	//ssize_t status = -ENOMEM;
	__s32 length;
	int ret;
	
	//if (!cdev->gadget->ep0->enabled)
	//	return -1;

	hidg->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
	hidg->event_length = le16_to_cpu(ctrl->wLength);
	
	length = __le16_to_cpu(ctrl->wLength);
	length = min_t(unsigned int, hidg->report_length, length);
	req->complete = hidg_ep0_complete;
	req->length = length;


	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return ret;
	}
	
	return ret;
}

static ssize_t f_hidg_read_ep0(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_hidg *hidg = file->private_data;
	struct f_hidg_req_list *list;
	struct usb_request *req;
	unsigned long flags;
	//int ret;

	if (!count)
		return 0;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_WRITE, buffer, count))
		return -EFAULT;
#endif

	spin_lock_irqsave(&hidg->read_spinlock, flags);

//#define READ_COND (!list_empty(&hidg->completed_out_req))

	/* wait for at least one buffer to complete */
	while (!READ_COND) {
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(hidg->read_queue, READ_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&hidg->read_spinlock, flags);
	}

	/* pick the first one */
	list = list_first_entry(&hidg->completed_out_req,
				struct f_hidg_req_list, list);

	/*
	 * Remove this from list to protect it from beign free()
	 * while host disables our function
	 */
	list_del(&list->list);

	req = list->req;
	count = min_t(unsigned int, count, req->actual - list->pos);
	spin_unlock_irqrestore(&hidg->read_spinlock, flags);

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
		kfree(list);
#if 0
		req->length = hidg->report_length;
		ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
		if (ret < 0) {
			free_ep_req(hidg->out_ep, req);
			return ret;
		}
#endif
	} else {
		spin_lock_irqsave(&hidg->read_spinlock, flags);
		list_add(&list->list, &hidg->completed_out_req);
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);

		wake_up(&hidg->read_queue);
	}

	return count;
}


static ssize_t f_hidg_write_ep0(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_hidg *hidg  = file->private_data;
	unsigned long flags;
	ssize_t status = -ENOMEM;
	
	if (hidg->control_ep == NULL)
		return -EFAULT;
	
	if (hidg->control_ep->driver_data == NULL)
		return -EFAULT;
	
	//if (!hidg->control_ep->enabled)
	//	return -EFAULT;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;
#endif

	spin_lock_irqsave(&hidg->write_spinlock, flags);

//#define WRITE_COND (!hidg->write_pending)

	/* write queue */
	while (!WRITE_COND) {
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible_exclusive(
				hidg->write_queue, WRITE_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&hidg->write_spinlock, flags);
	}
	
	hidg->write_pending = 1;
	count  = min_t(unsigned, count, hidg->report_length);
	spin_unlock_irqrestore(&hidg->write_spinlock, flags);
	
	status = copy_from_user(hidg->control_req->buf, buffer, count);
	if (status != 0) {
		ERROR(hidg->func.config->cdev,
			"copy_from_user error\n");
		status = -EINVAL;
		goto release_write_pending;
	}

	hidg->control_req->status   = 0;
	hidg->control_req->zero     = 0;
	hidg->control_req->length   = count;
	hidg->control_req->complete = hidg_ep0_complete;
	hidg->control_req->context  = hidg;

	status = usb_ep_queue(hidg->control_ep, hidg->control_req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		goto release_write_pending;
	} else {
		status = count;
	}

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

static ssize_t f_hidg_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_hidg *hidg  = file->private_data;
	
	if(hidg->f_write)
		return hidg->f_write(file, buffer, count, offp);
	else
		return -ENOMEM;
}
static ssize_t f_hidg_read(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_hidg *hidg  = file->private_data;
	
	if(hidg->f_read)
		return hidg->f_read(file, buffer, count, ptr);
	else
		return -ENOMEM;
}

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

		if(hidg->hid_interrupt_ep_enable && hid_ep_size >= 48)
		{		
			/* send an empty report */
			length = min_t(unsigned, length, hidg->report_length);
			memset(req->buf, 0x0, length);
			goto respond;
		}
		else
		{
			//printk("get_report | wLenght=%d\n", ctrl->wLength);
			unsigned long flags;
			spin_lock_irqsave(&hidg->write_spinlock, flags);
			hidg->write_pending = 0;
			spin_unlock_irqrestore(&hidg->write_spinlock, flags);
			wake_up(&hidg->write_queue);		
			return 0;
		}
		break;

	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_PROTOCOL):
		VDBG(cdev, "get_protocol\n");
		goto stall;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_REPORT):
		VDBG(cdev, "set_report | wLenght=%d\n", ctrl->wLength);

		if(hidg->hid_interrupt_ep_enable && hid_ep_size >= 48)
			goto stall;
		else
			return hidg_usb_ep0_out_data(hidg, ctrl);

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
	struct f_hidg_req_list *list, *next;
	unsigned long flags;
	
	printk("hidg_disable\n");


	if(hidg->hid_interrupt_ep_enable)
	{
		if(hidg->in_ep)
		{
			usb_ep_disable(hidg->in_ep);
			//hidg->in_ep->driver_data = NULL;
		}
		if(hidg->out_ep)
		{
			usb_ep_disable(hidg->out_ep);
			//hidg->out_ep->driver_data = NULL;
		}
	}


	spin_lock_irqsave(&hidg->read_spinlock, flags);
	list_for_each_entry_safe(list, next, &hidg->completed_out_req, list) {
		//free_ep_req(hidg->out_ep, list->req);
		list_del(&list->list);
		kfree(list);
	}
	spin_unlock_irqrestore(&hidg->read_spinlock, flags);

}

static int hidg_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev		*cdev = f->config->cdev;
	struct f_hidg				*hidg = func_to_hidg(f);
	int i, status = 0;

	VDBG(cdev, "hidg_set_alt intf:%d alt:%d\n", intf, alt);
	
	if(intf == hidg->control_intf)
	{
		if(hidg->control_ep != NULL)
			hidg->control_ep->driver_data = hidg;
		
	}
	

	if(hidg->hid_interrupt_ep_enable)
	{
		if (hidg->in_ep != NULL) {
			/* restart endpoint */
			if(hidg->in_ep->enabled)
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


		if (hidg->out_ep != NULL) {
			/* restart endpoint */
			//if (hidg->out_ep->driver_data != NULL)
			if(hidg->out_ep->enabled)
				usb_ep_disable(hidg->out_ep);

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
			hidg->out_ep->driver_data = hidg;

			/*
			* allocate a bunch of read buffers and queue them all at once.
			*/
			for (i = 0; i < hidg->qlen && status == 0; i++) {
				struct usb_request *req =
						hidg_alloc_ep_req(hidg->out_ep,
								hid_ep_size);
				if (req) {
					req->complete = hidg_set_report_ep_out_complete;
					req->context  = hidg;
					status = usb_ep_queue(hidg->out_ep, req,
								GFP_ATOMIC);
					if (status)
						ERROR(cdev, "%s queue req --> %d\n",
							hidg->out_ep->name, status);
				} else {
					usb_ep_disable(hidg->out_ep);
					//hidg->out_ep->driver_data = NULL;
					status = -ENOMEM;
					goto fail;
				}
			}
		}
	}
	
fail:
	return status;
}

static long hid_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct f_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	int type;
	unsigned long cnt;

	switch (cmd) {
	
	case UVCIOC_USB_CONNECT_STATE:{
		
		cnt = copy_from_user((void *)&type, (const void *)arg ,sizeof(int));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		
		if (type == USB_STATE_CONNECT_RESET ){
			//printk("%s +%d %s:set usb connect reset\n", __FILE__,__LINE__,__func__);		
			usb_gadget_disconnect(cdev->gadget);
			usb_gadget_connect(cdev->gadget);	
			return 0;
		}else if(type == USB_STATE_DISCONNECT){
			//printk("%s +%d %s:set usb disconnect\n", __FILE__,__LINE__,__func__);
		
			usb_gadget_disconnect(cdev->gadget);
			return 0;
			
		}else if(type == USB_STATE_CONNECT){
			//printk("%s +%d %s:set usb connect\n", __FILE__,__LINE__,__func__);
			usb_gadget_connect(cdev->gadget);
			return 0;
		}
		else
			return -EINVAL;
	}

#ifdef USB_CHIP_DWC3	
	case UVCIOC_GET_USB_SOFT_CONNECT_STATE:{	
		uint32_t value = get_usb_soft_connect_state();
		cnt = copy_to_user((void *)arg, (const void *)&value, sizeof(uint32_t));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		
		return 0;
	}
	
	case UVCIOC_SET_USB_SOFT_CONNECT_CTRL:{
		uint32_t value;
		cnt = copy_from_user((void *)&value, (const void *)arg ,sizeof(uint32_t));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		set_usb_soft_connect_ctrl(value);			
		return 0;
	}
	
	case UVCIOC_GET_USB_SOFT_CONNECT_CTRL:{
		uint32_t value = get_usb_soft_connect_ctrl();
		cnt = copy_to_user((void *)arg, (const void*)&value, sizeof(uint32_t));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		return 0;
	}
#endif	
	case UVCIOC_GET_HID_REPORT_LEN:{
		//uint32_t *value = arg;
		struct f_hidg	*hidg  = file->private_data;
		printk("***************\n");
		printk("hidg->report_length:%d\n", hidg->report_length);
		
		//*value = hidg->report_length;
		//*((__user uint32_t *)arg) = hidg->report_length;
		cnt = copy_to_user((void __user *)arg, (const void *)&hidg->report_length, sizeof(uint32_t));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		
		return 0;	
	}
		
	default:
		printk("%s +%d %s: cmd = 0x%x\n", __FILE__,__LINE__,__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return ret;
}

const struct file_operations f_hidg_fops = {
	.owner		= THIS_MODULE,
	.open		= f_hidg_open,
	.release	= f_hidg_release,
	.write		= f_hidg_write,
	.read		= f_hidg_read,
	.poll		= f_hidg_poll,
	.llseek		= noop_llseek,
	.unlocked_ioctl = hid_ioctl,
};

void hid_ep_descriptors_enable(struct usb_function *f, int en)
{
	struct f_hidg *hidg = func_to_hidg(f);
	
	if(hidg->report_desc)
		kfree(hidg->report_desc);
		
		
	if(en)
	{
#ifdef HID_EP_USB_CV_TEST
		hidg->bInterfaceSubClass = hid_des_ep0.subclass;
		hidg->bInterfaceProtocol = hid_des_ep0.protocol;
		hidg->report_length = hid_des_ep0.report_length;
		hidg->report_desc_length = hid_des_ep0.report_desc_length;
		hidg->report_desc = kmemdup(hid_des_ep0.report_desc,
						hid_des_ep0.report_desc_length,
						GFP_KERNEL);
						
		hidg->f_write = f_hidg_write_ep0;
		hidg->f_read = f_hidg_read_ep0;	
		hidg->write_pending = 1;

#else
	
		hidg->bInterfaceSubClass = hid_des_ep_interrupt.subclass;
		hidg->bInterfaceProtocol = hid_des_ep_interrupt.protocol;
		hidg->report_length = hid_des_ep_interrupt.report_length;
		hidg->report_desc_length = hid_des_ep_interrupt.report_desc_length;
		hidg->report_desc = kmemdup(hid_des_ep_interrupt.report_desc,
						hid_des_ep_interrupt.report_desc_length,
						GFP_KERNEL);
						
		hidg->f_write = f_hidg_write_ep_in;
		hidg->f_read = f_hidg_read_ep_out;
		hidg->write_pending = 0;
#endif

		hidg_interface_desc.bNumEndpoints 	= 2;
		hidg_ss_in_ep_desc.bLength			= USB_DT_ENDPOINT_SIZE;
		hidg_ss_in_ep_comp_desc.bLength 	= sizeof(hidg_ss_in_ep_comp_desc);
		hidg_ss_out_ep_desc.bLength			= USB_DT_ENDPOINT_SIZE;
		hidg_ss_out_ep_comp_desc.bLength	= sizeof(hidg_ss_out_ep_comp_desc);	
		hidg_hs_in_ep_desc.bLength			= USB_DT_ENDPOINT_SIZE;
		hidg_hs_out_ep_desc.bLength			= USB_DT_ENDPOINT_SIZE;
		hidg_fs_in_ep_desc.bLength			= USB_DT_ENDPOINT_SIZE;
		hidg_fs_out_ep_desc.bLength			= USB_DT_ENDPOINT_SIZE;
	}
	else
	{
		hidg->bInterfaceSubClass = hid_des_ep0.subclass;
		hidg->bInterfaceProtocol = hid_des_ep0.protocol;
		hidg->report_length = hid_des_ep0.report_length;
		hidg->report_desc_length = hid_des_ep0.report_desc_length;
		hidg->report_desc = kmemdup(hid_des_ep0.report_desc,
						hid_des_ep0.report_desc_length,
						GFP_KERNEL);
						
		hidg_interface_desc.bNumEndpoints 	= 0;
		hidg_ss_in_ep_desc.bLength			= 0;
		hidg_ss_in_ep_comp_desc.bLength 	= 0;
		hidg_ss_out_ep_desc.bLength			= 0;
		hidg_ss_out_ep_comp_desc.bLength	= 0;	
		hidg_hs_in_ep_desc.bLength			= 0;
		hidg_hs_out_ep_desc.bLength			= 0;
		hidg_fs_in_ep_desc.bLength			= 0;
		hidg_fs_out_ep_desc.bLength			= 0;

		if(hidg->hid_interrupt_ep_enable)
		{
			if(hidg->in_ep && hidg->in_ep->driver_data)
			{

				usb_ep_disable(hidg->in_ep);
				hidg->in_ep->driver_data = NULL;	

				hidg->in_ep = NULL;
				
			}
			if(hidg->out_ep && hidg->out_ep->driver_data)
			{

				usb_ep_disable(hidg->out_ep);
				hidg->out_ep->driver_data = NULL;

				hidg->out_ep = NULL;				
			}
		}

		hidg->hid_interrupt_ep_enable = 0;
		
		hidg->f_write = f_hidg_write_ep0;
		hidg->f_read = f_hidg_read_ep0;
		hidg->write_pending = 1;
	}
	
}
EXPORT_SYMBOL(hid_ep_descriptors_enable);

static int __init hidg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	struct f_hidg		*hidg = func_to_hidg(f);
	int			status;
	dev_t			dev;
	
	int minor = DEV_HID_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_NUM_MAX)
		index = DEV_HID_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
			
	hidg_interface_desc.bInterfaceNumber = status;

hid_ep_init:
	/* set descriptor dynamic values */
	hidg_interface_desc.bInterfaceSubClass = hidg->bInterfaceSubClass;
	hidg_interface_desc.bInterfaceProtocol = hidg->bInterfaceProtocol;
	
	hidg_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	hidg_desc.desc[0].wDescriptorLength =
		cpu_to_le16(hidg->report_desc_length);

		
	hid_ep_descriptors_enable(f, hidg->hid_interrupt_ep_enable); 

	if(hidg->hid_interrupt_ep_enable)
	{	
		
		status = -ENODEV;
		ep = usb_ep_autoconfig(c->cdev->gadget, &hidg_fs_in_ep_desc);
		if (!ep)
			goto hid_ep_init_fail;
		ep->driver_data = hidg;	/* claim */
		ep->enabled = false;
		hidg->in_ep = ep;

		ep = usb_ep_autoconfig(c->cdev->gadget, &hidg_fs_out_ep_desc);
		if (!ep)
			goto hid_ep_init_fail;
		ep->driver_data = hidg;	/* claim */
		ep->enabled = false;
		hidg->out_ep = ep;

		/* preallocate request and buffer */
		status = -ENOMEM;
		hidg->req = usb_ep_alloc_request(hidg->in_ep, GFP_KERNEL);
		if (!hidg->req)
			goto hid_ep_init_fail;

		hidg->req->buf = kmalloc(hid_ep_size, GFP_KERNEL);
		if (!hidg->req->buf)
			goto hid_ep_init_fail;
		
		hidg_ss_in_ep_desc.wMaxPacketSize = cpu_to_le16(hid_ep_size);
		hidg_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hid_ep_size);
		hidg_fs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hid_ep_size);
		hidg_ss_out_ep_desc.wMaxPacketSize = cpu_to_le16(hid_ep_size);
		hidg_hs_out_ep_desc.wMaxPacketSize = cpu_to_le16(hid_ep_size);
		hidg_fs_out_ep_desc.wMaxPacketSize = cpu_to_le16(hid_ep_size);
		hidg_ss_in_ep_comp_desc.wBytesPerInterval = hidg_ss_in_ep_desc.wMaxPacketSize;
		hidg_ss_out_ep_comp_desc.wBytesPerInterval = hidg_ss_out_ep_desc.wMaxPacketSize;

		hidg_ss_in_ep_desc.bEndpointAddress =	
		hidg_hs_in_ep_desc.bEndpointAddress =
			hidg_fs_in_ep_desc.bEndpointAddress;
		
		hidg_ss_out_ep_desc.bEndpointAddress =	
		hidg_hs_out_ep_desc.bEndpointAddress =
			hidg_fs_out_ep_desc.bEndpointAddress;
	}

		
	hidg->control_intf = hidg_interface_desc.bInterfaceNumber;
	hidg->control_ep = c->cdev->gadget->ep0;
	hidg->control_ep->driver_data = hidg;
	
	/* Preallocate control endpoint request. */
	hidg->control_req = usb_ep_alloc_request(c->cdev->gadget->ep0, GFP_KERNEL);
	hidg->control_buf = kmalloc(HID_REPORT_LENGTH_MAX, GFP_KERNEL);
	if (hidg->control_req == NULL || hidg->control_buf == NULL) {
		status = -ENOMEM;
		printk("%s: kmalloc control_req or control_buf error \n", __func__);
		goto fail;
	}
	
	hidg->control_req->buf = hidg->control_buf;
	hidg->control_req->complete = hidg_ep0_complete;
	hidg->control_req->context = hidg;
	
	//hidg->write_pending = 1;

	status = usb_assign_descriptors(f, hidg_fs_descriptors,
			hidg_hs_descriptors, hidg_ss_descriptors, NULL);
	if (status)
		goto fail;

	spin_lock_init(&hidg->write_spinlock);
	spin_lock_init(&hidg->read_spinlock);
	init_waitqueue_head(&hidg->write_queue);
	init_waitqueue_head(&hidg->read_queue);
	INIT_LIST_HEAD(&hidg->completed_out_req);
	

	/* create char device */
	cdev_init(&hidg->cdev, &f_hidg_fops);

	dev = MKDEV(major[index], minor);

	status = cdev_add(&hidg->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device_create(hidg_class, NULL, dev, NULL, "%s%d", "hidg", minor - DEV_HID_FIRST_NUM);

	return 0;

fail_free_descs:
	usb_free_all_descriptors(f);
fail:

	ERROR(f->config->cdev, "hidg_bind FAILED\n");


hid_ep_init_fail: 	
	if (hidg->req != NULL) {
		kfree(hidg->req->buf);
		if (hidg->in_ep != NULL)
			usb_ep_free_request(hidg->in_ep, hidg->req);
	}
	if(hidg->hid_interrupt_ep_enable)
	{
		hid_ep_descriptors_enable(f, 0);
		goto hid_ep_init;
	}

	return status;
}

static void hidg_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_hidg *hidg = func_to_hidg(f);
	
	//printk("hidg_unbind bConfigurationValue : %d\n", c->bConfigurationValue);

	int minor = DEV_HID_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_NUM_MAX)
		index = DEV_HID_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	

	device_destroy(hidg_class, MKDEV(major[index], minor));

	cdev_del(&hidg->cdev);


	if(hidg->hid_interrupt_ep_enable)
	{
		if(hidg->in_ep)
		{
			usb_ep_disable(hidg->in_ep);
			if(hidg->req)
			{
				usb_ep_dequeue(hidg->in_ep, hidg->req);
				usb_ep_free_request(hidg->in_ep, hidg->req);
				if(hidg->req->buf)
					kfree(hidg->req->buf);
			}
			hidg->in_ep->driver_data = NULL;
		}
		if(hidg->out_ep)
		{
			usb_ep_disable(hidg->out_ep);
			hidg->out_ep->driver_data = NULL;
		}
			

	}


	hidg->control_ep->driver_data = NULL;
	
	if (hidg->control_req)
		usb_ep_free_request(c->cdev->gadget->ep0, hidg->control_req);
	
	if(hidg->control_buf)
		kfree(hidg->control_buf);

	ghid_cleanup(c->bConfigurationValue);
	
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

int __init hidg_bind_config(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc, unsigned int hid_interrupt_ep_enable)
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
		
		
	hid_des_ep_interrupt.report_length = hid_ep_size;
	if(hid_des_ep_interrupt.report_length > 255)
	{
		hid_des_ep_interrupt.report_desc_length = 30;
		
		hid_des_ep_interrupt.report_desc[18] = 0x96;
		hid_des_ep_interrupt.report_desc[19] = hid_des_ep_interrupt.report_length & 0xFF;
		hid_des_ep_interrupt.report_desc[20] = (hid_des_ep_interrupt.report_length >> 8) & 0xFF;
		
		hid_des_ep_interrupt.report_desc[21] = 0x81;//INPUT (Data,Var,Abs)
		hid_des_ep_interrupt.report_desc[22] = 0x02;
		
		hid_des_ep_interrupt.report_desc[23] = 0x19;//(Vendor Usage 1)
		hid_des_ep_interrupt.report_desc[24] = 0x01;
		
		hid_des_ep_interrupt.report_desc[25] = 0x29;//(Vendor Usage 1)
		hid_des_ep_interrupt.report_desc[26] = 0x08;
		
		hid_des_ep_interrupt.report_desc[27] = 0x91;//OUTPUT (Data,Var,Abs)
		hid_des_ep_interrupt.report_desc[28] = 0x02;
		
		hid_des_ep_interrupt.report_desc[29] = 0xc0;// END_COLLECTION
	}
	else
	{
		hid_des_ep_interrupt.report_desc_length = 29;
		
		hid_des_ep_interrupt.report_desc[18] = 0x95;
		hid_des_ep_interrupt.report_desc[19] = hid_des_ep_interrupt.report_length;
		
		hid_des_ep_interrupt.report_desc[20] = 0x81;//INPUT (Data,Var,Abs)
		hid_des_ep_interrupt.report_desc[21] = 0x02;
		
		hid_des_ep_interrupt.report_desc[22] = 0x19;//(Vendor Usage 1)
		hid_des_ep_interrupt.report_desc[23] = 0x01;
		
		hid_des_ep_interrupt.report_desc[24] = 0x29;//(Vendor Usage 1)
		hid_des_ep_interrupt.report_desc[25] = 0x08;
		
		hid_des_ep_interrupt.report_desc[26] = 0x91;//OUTPUT (Data,Var,Abs)
		hid_des_ep_interrupt.report_desc[27] = 0x02;
		
		hid_des_ep_interrupt.report_desc[28] = 0xc0;// END_COLLECTION
	}

	if(hid_interrupt_ep_enable)
	{
		
		hidg->bInterfaceSubClass = hid_des_ep_interrupt.subclass;
		hidg->bInterfaceProtocol = hid_des_ep_interrupt.protocol;
		hidg->report_length = hid_des_ep_interrupt.report_length;
		hidg->report_desc_length = hid_des_ep_interrupt.report_desc_length;
		hidg->report_desc = kmemdup(hid_des_ep_interrupt.report_desc,
						hid_des_ep_interrupt.report_desc_length,
						GFP_KERNEL);
	}
	else
	{
		hidg->bInterfaceSubClass = hid_des_ep0.subclass;
		hidg->bInterfaceProtocol = hid_des_ep0.protocol;
		hidg->report_length = hid_des_ep0.report_length;
		hidg->report_desc_length = hid_des_ep0.report_desc_length;
		hidg->report_desc = kmemdup(hid_des_ep0.report_desc,
						hid_des_ep0.report_desc_length,
						GFP_KERNEL);
	}
		
										
	if (!hidg->report_desc) {
		kfree(hidg);
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}
	
	hidg->hid_interrupt_ep_enable = hid_interrupt_ep_enable;
	//hidg->report_desc_length = sizeof(hidg->report_desc);
	
	hidg->func.name    = "hid";
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

	printk("+%d %s: hidg_bind_config ok!\n",__LINE__,__func__);
	
	return status;
}

int __init ghid_setup(struct usb_gadget *g, u8 bConfigurationValue)
{
	int status;
	dev_t dev;
	
	int count = DEV_HID_FIRST_NUM + bConfigurationValue;
	
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_NUM_MAX)
		index = DEV_HID_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	if(hidg_class == NULL)
		hidg_class = class_create(THIS_MODULE, "hidg");

	status = alloc_chrdev_region(&dev, 0, count, "hidg");
	if (!status) {
		major[index] = MAJOR(dev);
		minors[index] = count;
	}

	return status;
}

void ghid_cleanup(u8 bConfigurationValue)
{
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_NUM_MAX)
		index = DEV_HID_NUM_MAX - 1;
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
