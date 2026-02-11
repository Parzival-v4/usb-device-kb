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

#define POLYCOM_HID_REPORT_LENGTH_MAX       4096

#define POLYCOM_HID_EP_MAX_PACKET_SIZE		4

struct hid_request_data
{
	__s32 length;
	__u8 data[POLYCOM_HID_REPORT_LENGTH_MAX];
};

#define POLYCOM_HID_IOC_SEND_EP0_DATA            		_IOW('H', 1, struct hid_request_data)
#define POLYCOM_HID_IOC_GET_EP0_DATA            		_IOR('H', 2, struct hid_request_data)
#define POLYCOM_HID_IOC_SEND_EP_DATA            		_IOW('H', 3, struct hid_request_data)
#define POLYCOM_HID_IOC_GET_EP_DATA            			_IOR('H', 4, struct hid_request_data)
#define POLYCOM_HID_IOC_USB_CONNECT_STATE               _IOW('H', 5, int)
#define POLYCOM_HID_IOC_GET_HID_REPORT_LEN				_IOR('H', 6, uint32_t)

static int polycom_major[DEV_HID_POLYCOM_NUM_MAX], polycom_minors[DEV_HID_POLYCOM_NUM_MAX];
static struct class *polycom_hidg_class;

static unsigned char	report_desc[] = {

    0x05, 0x0B,         /*  Usage Page (Telephony),                 */
    0x09, 0x01,         /*  Usage (01h),                            */
    0xA1, 0x01,         /*  Collection (Application),               */
	0x05, 0x08,         /*      Usage Page (LED),                   */
    0x85, 0x03,         /*      Report ID (03),                     */
    0x75, 0x01,         /*      Report Size (1),                    */

    0x09, 0x18,         /*      Usage (18h),    Ring                */
    0x95, 0x01,         /*      Report Count (1),                   */
    0x91, 0x22,         /*      Output (Variable, No Preferred),    */

    0x09, 0x09,         /*      Usage (09h),    Mute                */
    0x95, 0x01,         /*      Report Count (1),                   */
    0x91, 0x22,         /*      Output (Variable, No Preferred),    */

    0x09, 0x17,         /*      Usage (17h),    Off_Hook            */
    0x95, 0x01,         /*      Report Count (1),                   */
    0x91, 0x22,         /*      Output (Variable, No Preferred),    */

    0x09, 0x20,         /*      Usage (20h),   Hold                 */
    0x95, 0x01,         /*      Report Count (1),                   */
    0x91, 0x22,         /*      Output (Variable, No Preferred),    */

    0x75, 0x01,         /*      Report Size (1),                    */
    0x95, 0x04,         /*      Report Count (4),                   */
    0x91, 0x01,         /*      Output (Constant),                  */

    0xC0,               /*  End Collection,                         */


	
	// Feature Report for UA
	0x06, 0xA0, 0xFF,              // USAGE_PAGE (Vendor Defined)
	0x09, 0x01,                    // USAGE (Vendor Defined)
	0xA1, 0x01,                    // COLLECTION (Application)

	//--------- Vendor Defined Feature Report ------------------------
	// FEATURE REPORT (2048)
	0x09, 0x11,                    // USAGE (Vendor Defined)
	0x15, 0x00,                    // Logical Minimum (0)
	0x26, 0xFF, 0x00,              // Logical Maximum (255)
	0x85, 0x05,                    // Report ID (5)
	0x96, 0xFF, 0x07,              // REPORT_COUNT (2047)
	0x75, 0x08,                    // REPORT_SIZE (8)
	0xB1, 0x02,                    // FEATURE (Data, Variable, Absolute)

	//--------- Vendor Defined Feature Report ------------------------
	// FEATURE REPORT (256) non-block
	0x09, 0x10,                    // USAGE (Vendor Defined)
	0x15, 0x00,                    // Logical Minimum (0)
	0x26, 0xFF, 0x00,              // Logical Maximum (255)
	0x85, 0x06,                    // Report ID (6)
	0x96, 0x00, 0x01,              // REPORT_COUNT (256)
	0x75, 0x08,                    // REPORT_SIZE (8)
	0xB1, 0x02,                    // FEATURE (Data, Variable, Absolute)

	//--------- Vendor Defined Feature Report ------------------------
	// FEATURE REPORT (4) ACK
	0x09, 0x12,                    // USAGE (Vendor Defined)
	0x15, 0x00,                    // Logical Minimum (0)
	0x26, 0xFF, 0x00,              // Logical Maximum (255)
	0x85, 0x08,                    // Report ID (8)
	0x95, 0x04,                    // REPORT_COUNT (4)
	0x75, 0x08,                    // REPORT_SIZE (8)
	0xB1, 0x02,                    // FEATURE (Data, Variable, Absolute)

	//--------- Vendor Defined Input Report ------------------------
	// INPUT REPORT (3) Notification
	0x09, 0x13,                    // USAGE (Vendor Defined)
	0x15, 0x00,                    // Logical Minimum (0)
	0x26, 0xFF, 0x00,              // Logical Maximum (255)
	0x85, 0x09,                    // Report ID (9)
	0x95, 0x03,                    // REPORT_COUNT (3)
	0x75, 0x08,                    // REPORT_SIZE (8)
	0x81, 0x02,                    // INPUT (Data, Variable, Absolute)

	//--------- Vendor Defined Feature Report ------------------------
	// FEATURE REPORT (512) non-block
	0x09, 0x14,                    // USAGE (Vendor Defined)
	0x15, 0x00,                    // Logical Minimum (0)
	0x26, 0xFF, 0x00,              // Logical Maximum (255)
	0x85, 0x0A,                    // Report ID (10)
	0x96, 0x00, 0x02,              // REPORT_COUNT (512)
	0x75, 0x08,                    // REPORT_SIZE (8)
	0xB1, 0x02,                    // FEATURE (Data, Variable, Absolute)

	// FEATURE REPORT (64) non-block
	0x09, 0x15,                    // USAGE (Vendor Defined)
	0x15, 0x00,                    // Logical Minimum (0)
	0x26, 0xFF, 0x00,              // Logical Maximum (255)
	0x85, 0x0B,                    // Report ID (11)
	0x95, 0x40,                    // REPORT_COUNT (64)
	0x75, 0x08,                    // REPORT_SIZE (8)
	0xB1, 0x02,                    // FEATURE (Data, Variable, Absolute)

	0xC0,                          // END_COLLECTION	
};

static struct hidg_func_descriptor polycom_hid_des= {
	.subclass = 0,
	.protocol = 0,
	.report_length = 2048,//POLYCOM_HID_REPORT_LENGTH_MAX,
	.report_desc_length= 0,
};	


/*-------------------------------------------------------------------------*/
/*                            HID gadget struct                            */

typedef enum 
{
  HID_LIST_TYPE_REQ = 1,
  HID_LIST_TYPE_CTRL = 2,
  HID_LIST_TYPE_EP0_OUT_DATA = 3,
}hidg_list_type;

struct f_polycom_hidg_list {
	void					*p;
	unsigned int			pos;
	unsigned int			type;
	struct list_head 		list;
};


struct f_polycom_hidg {
	/* configuration */
	unsigned char			bInterfaceSubClass;
	unsigned char			bInterfaceProtocol;
	unsigned short			report_desc_length;
	char					*report_desc;
	unsigned short			report_length;

	/* recv report */
	struct list_head		completed_out_req;
	spinlock_t			read_spinlock;
	wait_queue_head_t		read_queue;
	//unsigned int			qlen;

	/* send report */
	spinlock_t			write_spinlock;
	bool				write_pending;
	wait_queue_head_t		write_queue;
	struct usb_request		*req;

	int						minor;
	struct cdev				cdev;
	struct usb_function		func;

	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;
	
	
	//usb control ep 
	unsigned int 			control_intf;
	struct usb_ep 			*control_ep;
	struct usb_request 		*control_req;
	void 					*control_buf;
	
	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
	
	struct completion  get_ep0_data_done_completion;
	struct completion  send_ep0_data_done_completion;
	
	struct completion  ep_in_data_done_completion;
	struct completion  ep_out_data_done_completion;
	
	unsigned char 			hid_idle;
};

static inline struct f_polycom_hidg *func_to_polycom_hidg(struct usb_function *f)
{
	return container_of(f, struct f_polycom_hidg, func);
}

/*-------------------------------------------------------------------------*/
/*                           Static descriptors                            */

static struct usb_interface_descriptor polycom_hidg_interface_desc = {
	.bLength		= sizeof polycom_hidg_interface_desc,
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

static struct hid_descriptor polycom_hidg_desc = {
	.bLength			= sizeof polycom_hidg_desc,
	.bDescriptorType		= HID_DT_HID,
	.bcdHID				= 0x0111,
	.bCountryCode			= 0x00,
	.bNumDescriptors		= 0x1,
	/*.desc[0].bDescriptorType	= DYNAMIC */
	/*.desc[0].wDescriptorLenght	= DYNAMIC */
};

/* Super-Speed Support */

static struct usb_endpoint_descriptor polycom_hidg_ss_in_ep_desc = {
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

static struct usb_ss_ep_comp_descriptor polycom_hidg_ss_in_ep_comp_desc = {
	.bLength		= sizeof(polycom_hidg_ss_in_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	.bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};



static struct usb_endpoint_descriptor polycom_hidg_ss_out_ep_desc = {
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

static struct usb_ss_ep_comp_descriptor polycom_hidg_ss_out_ep_comp_desc = {
	.bLength		= sizeof(polycom_hidg_ss_out_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	 .bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};

/* High-Speed Support */

static struct usb_endpoint_descriptor polycom_hidg_hs_in_ep_desc = {
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

static struct usb_endpoint_descriptor polycom_hidg_hs_out_ep_desc = {
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



/* Full-Speed Support */

static struct usb_endpoint_descriptor polycom_hidg_fs_in_ep_desc = {
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

static struct usb_endpoint_descriptor polycom_hidg_fs_out_ep_desc = {
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

static struct usb_descriptor_header *polycom_hidg_ss_descriptors[] = {
	(struct usb_descriptor_header *)&polycom_hidg_interface_desc,
	(struct usb_descriptor_header *)&polycom_hidg_desc,
	(struct usb_descriptor_header *)&polycom_hidg_ss_in_ep_desc,
	(struct usb_descriptor_header *)&polycom_hidg_ss_in_ep_comp_desc,
	//(struct usb_descriptor_header *)&polycom_hidg_ss_out_ep_desc,
	//(struct usb_descriptor_header *)&polycom_hidg_ss_out_ep_comp_desc,
	NULL,
};

static struct usb_descriptor_header *polycom_hidg_hs_descriptors[] = {
	(struct usb_descriptor_header *)&polycom_hidg_interface_desc,
	(struct usb_descriptor_header *)&polycom_hidg_desc,
	(struct usb_descriptor_header *)&polycom_hidg_hs_in_ep_desc,
	//(struct usb_descriptor_header *)&polycom_hidg_hs_out_ep_desc,
	NULL,
};

static struct usb_descriptor_header *polycom_hidg_fs_descriptors[] = {
	(struct usb_descriptor_header *)&polycom_hidg_interface_desc,
	(struct usb_descriptor_header *)&polycom_hidg_desc,
	(struct usb_descriptor_header *)&polycom_hidg_fs_in_ep_desc,
	//(struct usb_descriptor_header *)&polycom_hidg_fs_out_ep_desc,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                              Char Device                                */

//static int hid_return_status(struct f_polycom_hidg *hidg);

#if 0
static void polycom_hidg_ep0_data_out_complete_add_list(struct usb_ep *ep, struct usb_request *req)
{
	struct f_polycom_hidg *hidg = (struct f_polycom_hidg *) req->context;
	struct f_polycom_hidg_list *req_list;
	unsigned long flags;

	req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
	if (!req_list)
		return;

	req_list->type = HID_LIST_TYPE_EP0_OUT_DATA;
	req_list->p = req;

	spin_lock_irqsave(&hidg->read_spinlock, flags);
	list_add_tail(&req_list->list, &hidg->completed_out_req);
	spin_unlock_irqrestore(&hidg->read_spinlock, flags);

	wake_up(&hidg->read_queue);
}
#endif
static void polycom_hidg_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_polycom_hidg *hidg = (struct f_polycom_hidg *) req->context;
	

	if (hidg->event_setup_out) 
	{
		
		//printk("dfug_ep0_complete out: length = %d\n", req->length);
		//polycom_hidg_ep0_data_out_complete_add_list(ep, req);
		hidg->event_setup_out = 0;
		//hid_return_status(hidg);
		complete_all(&hidg->get_ep0_data_done_completion);
		
	}
	else
	{
		//printk("dfug_ep0_complete in: length = %d\n", req->length);
		//hid_return_status(hidg);
		complete_all(&hidg->send_ep0_data_done_completion);	
	}
}

static void polycom_hidg_ep_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_polycom_hidg *hidg = (struct f_polycom_hidg *) req->context;
	

	complete_all(&hidg->ep_in_data_done_completion);	
}

#if 0
static void polycom_hidg_ep_out_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_polycom_hidg *hidg = (struct f_polycom_hidg *) req->context;
	
	complete_all(&hidg->ep_out_data_done_completion);	
}
#endif 

static ssize_t f_polycom_hidg_read(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_polycom_hidg *hidg = file->private_data;
	struct f_polycom_hidg_list *list;
	struct usb_ctrlrequest 	*ctrl;
	struct usb_request *req;
	unsigned long flags;
	int ret;

	if (!count)
		return 0;
	
	if(get_usb_link_state()!= 0)
		return -EFAULT;

	if (!access_ok(VERIFY_WRITE, buffer, count))
		return -EFAULT;

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
				struct f_polycom_hidg_list, list);
	
	if(list->type == HID_LIST_TYPE_CTRL)
	{
		ctrl = (struct usb_ctrlrequest 	*)list->p;
		count = min_t(unsigned int, count, sizeof(struct usb_ctrlrequest));
#if 0
		printk("HID setup request %02x %02x value %04x index %04x %04x\n",
			ctrl->bRequestType, ctrl->bRequest, le16_to_cpu(ctrl->wValue),
			le16_to_cpu(ctrl->wIndex), le16_to_cpu(ctrl->wLength));
#endif
		
		
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);
		
		/* copy to user outside spinlock */
		count -= copy_to_user(buffer, (uint8_t *)ctrl + list->pos, count);
		list->pos += count;
		
		/*
		 * if this request is completely handled and transfered to
		 * userspace, remove its entry from the list and requeue it
		 * again. Otherwise, we will revisit it again upon the next
		 * call, taking into account its current read position.
		 */
		if (list->pos == sizeof(struct usb_ctrlrequest)) 
		{
			spin_lock_irqsave(&hidg->read_spinlock, flags);
			list_del(&list->list);
			kfree(list);
			spin_unlock_irqrestore(&hidg->read_spinlock, flags);
		}
	}
	else if(list->type == HID_LIST_TYPE_EP0_OUT_DATA)
	{
		req = (struct usb_request *)list->p;
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
			spin_lock_irqsave(&hidg->read_spinlock, flags);
			list_del(&list->list);
			kfree(list);
			spin_unlock_irqrestore(&hidg->read_spinlock, flags);
		}	
	}
	else if(list->type == HID_LIST_TYPE_REQ)
	{
		req = (struct usb_request *)list->p;
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
			spin_lock_irqsave(&hidg->read_spinlock, flags);
			list_del(&list->list);
			kfree(list);
			spin_unlock_irqrestore(&hidg->read_spinlock, flags);

			req->length = POLYCOM_HID_EP_MAX_PACKET_SIZE;
			ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
			if (ret < 0)
				return ret;
		}	
	}
	else
	{
		spin_lock_irqsave(&hidg->read_spinlock, flags);
		list_del(&list->list);
		kfree(list);
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);
	}


	return count;
}


static void f_polycom_hidg_req_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_polycom_hidg *hidg = (struct f_polycom_hidg *)ep->driver_data;
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

static ssize_t f_polycom_hidg_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_polycom_hidg *hidg  = file->private_data;
	unsigned long flags;
	ssize_t status = -ENOMEM;
	ktime_t timeout;
	
	timeout.tv64 = 1*1000*1000*1000;
	
	if (hidg->in_ep == NULL)
		return -EFAULT;
	
	if (hidg->in_ep->enabled == false)
		return -EFAULT;

	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;

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


	spin_lock_irqsave(&hidg->write_spinlock, flags);

#define WRITE_COND (!hidg->write_pending)

	/* write queue */
	while (!WRITE_COND) {
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if(wait_event_interruptible_hrtimeout(hidg->write_queue, WRITE_COND, timeout))
		{
			//return -ERESTARTSYS;
			status = -ERESTARTSYS;
			goto release_write_pending;
		}

		spin_lock_irqsave(&hidg->write_spinlock, flags);
	}

	hidg->write_pending = 1;
	count  = min_t(unsigned, count, POLYCOM_HID_EP_MAX_PACKET_SIZE);
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
	hidg->req->complete = f_polycom_hidg_req_complete;
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


static unsigned int f_polycom_hidg_poll(struct file *file, poll_table *wait)
{
	struct f_polycom_hidg	*hidg  = file->private_data;
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

static int f_polycom_hidg_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static int f_polycom_hidg_open(struct inode *inode, struct file *fd)
{
	struct f_polycom_hidg *hidg =
		container_of(inode->i_cdev, struct f_polycom_hidg, cdev);

	fd->private_data = hidg;

	return 0;
}

/*-------------------------------------------------------------------------*/
/*                                usb_function                             */

#if 0
static void polycom_hidg_free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}
#endif 

static int
polycom_hidg_usb_send_ep0_data(struct file* file, struct hid_request_data *data)
{
	struct f_polycom_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	ssize_t status = -ENOMEM;
	__s32 length;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		return usb_ep_set_halt(cdev->gadget->ep0);
	}	
	
	if (length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	
	length = min_t(unsigned int, POLYCOM_HID_REPORT_LENGTH_MAX, length);
	req->zero = length < hidg->event_length;
	req->complete = polycom_hidg_ep0_complete;
	req->length = length;
	if(length > 0)
	{
		status = copy_from_user(req->buf, data->data, req->length);
		if (status != 0) {
			ERROR(cdev,"copy_from_user error\n");
			return usb_ep_set_halt(cdev->gadget->ep0);
		}
	}
	else 
		return usb_ep_set_halt(cdev->gadget->ep0);
	
	//printk("dfug_usb_send_ep0_data : %d\n",req->length);
	
	hidg->send_ep0_data_done_completion.done = 0;
	
	status = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		usb_ep_set_halt(cdev->gadget->ep0);
		complete_all(&hidg->send_ep0_data_done_completion);	
	} else {
		status = length;
	}
	

	wait_for_completion_interruptible(&hidg->send_ep0_data_done_completion);
	
	return status;
}

static int
polycom_hidg_usb_get_ep0_data(struct file* file, struct hid_request_data *data)
{
	struct f_polycom_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	ssize_t status = -ENOMEM;
	__s32 length;
	int ret;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		usb_ep_set_halt(cdev->gadget->ep0);
		return -1;
	}	
	
	if (length < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return -1;
	}
	
	length = min_t(unsigned int, POLYCOM_HID_REPORT_LENGTH_MAX, length);
	req->complete = polycom_hidg_ep0_complete;
	req->length = length;
	req->zero = length < hidg->event_length;

	//printk("dfug_usb_get_ep0_data : %d\n",req->length);
	
	hidg->get_ep0_data_done_completion.done = 0;
	
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		complete_all(&hidg->get_ep0_data_done_completion);
		return ret;
	}
	wait_for_completion_interruptible(&hidg->get_ep0_data_done_completion);
	
	length = min_t(unsigned int, req->length, length);
	status = copy_to_user(data->data, req->buf, length);
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");	
		
		return -1;
	}	
	
	return length;
}

static int
polycom_hidg_usb_ep_in_data(struct file* file, struct hid_request_data *data)
{
	struct f_polycom_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->req;
	ssize_t status = -ENOMEM;
	__s32 length;
	int ret;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		return usb_ep_set_halt(hidg->in_ep);
	}	
	
	if (length < 0)
		return usb_ep_set_halt(hidg->in_ep);
	
	length = min_t(unsigned int, POLYCOM_HID_REPORT_LENGTH_MAX, length);

	req->status   = 0;
	req->zero     = 0;
	req->length   = length;
	req->complete = polycom_hidg_ep_in_complete;
	req->context  = hidg;
	//hidg->write_pending = 1;
	
	if(length > 0)
	{
		status = copy_from_user(hidg->req->buf, data->data, hidg->req->length);
		if (status != 0) {
			ERROR(cdev,"copy_from_user error\n");
			return usb_ep_set_halt(hidg->in_ep);
		}
	}
	
	hidg->ep_in_data_done_completion.done = 0;
	
	status = usb_ep_queue(hidg->in_ep, hidg->req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		usb_ep_set_halt(hidg->in_ep);
		complete_all(&hidg->send_ep0_data_done_completion);	
	} else {
		status = length;
	}

	wait_for_completion_interruptible(&hidg->ep_in_data_done_completion);
	
	return ret;
}

#if 0
static int
polycom_hidg_usb_ep_out_data(struct file* file, struct hid_request_data *data)
{
	struct f_polycom_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->req;
	ssize_t status = -ENOMEM;
	__s32 length;
	int ret;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		usb_ep_set_halt(hidg->out_ep);
		return -1;
	}	
	
	if (length < 0)
	{
		usb_ep_set_halt(hidg->out_ep);
		return -1;
	}
	
	length = min_t(unsigned int, POLYCOM_HID_REPORT_LENGTH_MAX, length);
	req->complete = polycom_hidg_ep_out_complete;
	req->length = length;
	req->context  = hidg;
	req->status   = 0;
	req->zero     = 0;

	//printk("dfug_usb_get_ep0_data : %d\n",req->length);
	
	hidg->ep_out_data_done_completion.done = 0;
	
	ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(hidg->out_ep);
		complete_all(&hidg->ep_out_data_done_completion);
		return ret;
	}
	wait_for_completion_interruptible(&hidg->ep_out_data_done_completion);
	
	length = min_t(unsigned int, req->length, length);
	status = copy_to_user(data->data, req->buf, length);
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");	
		
		return -1;
	}	
	
	return length;
}


static int hid_return_status(struct f_polycom_hidg *hidg)
{
	ssize_t status = -ENOMEM;
	int count = 4;
	unsigned long flags;
	
	spin_lock_irqsave(&hidg->write_spinlock, flags);
	
	hidg->req->status   = 0;
	hidg->req->zero     = 0;
	hidg->req->length   = count;
	hidg->req->complete = f_polycom_hidg_req_complete;
	hidg->req->context  = hidg;
	
	((uint8_t *)hidg->req->buf)[0] = 0x09;
	((uint8_t *)hidg->req->buf)[1] = 0x01;
	((uint8_t *)hidg->req->buf)[2] = 0x00;
	((uint8_t *)hidg->req->buf)[3] = 0x00;
	
	
	status = usb_ep_queue(hidg->in_ep, hidg->req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
	} else {
		status = count;
	}	

	spin_unlock_irqrestore(&hidg->write_spinlock, flags);

	return status;
}
#endif

static inline struct usb_request *polycom_hidg_alloc_ep_req(struct usb_ep *ep,
						    unsigned length)
{
	return alloc_ep_req(ep, length, length);
}

#if 0
static void polycom_hidg_set_report_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_polycom_hidg *hidg = (struct f_polycom_hidg *) req->context;
	struct f_polycom_hidg_list *req_list;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	unsigned long flags;
	
	switch (req->status) {
	case 0:
		req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
		if (!req_list) {
			ERROR(cdev, "Unable to allocate mem for req_list\n");
			goto free_req;
		}

		req_list->type = HID_LIST_TYPE_REQ;
		req_list->p = req;

		spin_lock_irqsave(&hidg->read_spinlock, flags);
		list_add_tail(&req_list->list, &hidg->completed_out_req);
		spin_unlock_irqrestore(&hidg->read_spinlock, flags);

		wake_up(&hidg->read_queue);
		break;
	default:
		ERROR(cdev, "Set report failed %d\n", req->status);
		/* FALLTHROUGH */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
free_req:
		polycom_hidg_free_ep_req(ep, req);
		return;
	}
}
#endif

#if 0
static int
polycom_hidg_setup_get_ep0_data_to_list(struct f_polycom_hidg *hidg, struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	__s32 length = le16_to_cpu(ctrl->wLength);
	int ret;
	
	if (length < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return -EOPNOTSUPP;
	}
	
	req->length = min_t(unsigned int, POLYCOM_HID_REPORT_LENGTH_MAX, length);
	req->complete = polycom_hidg_set_report_complete;

	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return -EOPNOTSUPP;
	}
	
	return 0;
}
#endif

static int polycom_hidg_setup_add_list(struct f_polycom_hidg *hidg, const struct usb_ctrlrequest *ctrl)
{
	struct f_polycom_hidg_list *ctrl_list;
	unsigned long flags;
	
	if(ctrl->wIndex != hidg->control_intf)
	{
		printk("dfu intf error\n");
		return -EOPNOTSUPP;
	}

	ctrl_list = kzalloc(sizeof(*ctrl_list), GFP_ATOMIC);
	if (!ctrl_list)
	{
		printk("kzalloc error\n");
		return -EOPNOTSUPP;
	}

	ctrl_list->type = HID_LIST_TYPE_CTRL;
	ctrl_list->p = (struct usb_ctrlrequest *)ctrl;
	/* Tell the complete callback to generate an event for the next request
	 * that will be enqueued by UVCIOC_SEND_RESPONSE.
	 */
	hidg->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
	hidg->event_length = le16_to_cpu(ctrl->wLength);

	//printk("HID setup_add_list\n");
	
	spin_lock_irqsave(&hidg->read_spinlock, flags);
	list_add_tail(&ctrl_list->list, &hidg->completed_out_req);
	spin_unlock_irqrestore(&hidg->read_spinlock, flags);

	
	wake_up(&hidg->read_queue);
	
	return 0;
}

#if 0
static int
polycom_hidg_usb_ep0_out_data(struct f_polycom_hidg	*hidg, const struct usb_ctrlrequest *ctrl)
{
	
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	__s32 length;
	int ret;
	
	hidg->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
	hidg->event_length = le16_to_cpu(ctrl->wLength);
	
	length = __le16_to_cpu(ctrl->wLength);
	length = min_t(unsigned int, POLYCOM_HID_REPORT_LENGTH_MAX, length);
	req->complete = polycom_hidg_ep0_complete;
	req->length = length;


	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return ret;
	}
	
	return ret;
}
#endif

static int polycom_hidg_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_polycom_hidg			*hidg = func_to_polycom_hidg(f);
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
		//printk("get_report\n");
		return polycom_hidg_setup_add_list(hidg, ctrl);
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
		VDBG(cdev, "set_report\n");
		//printk("set_report\n");
#if 1
		return polycom_hidg_setup_add_list(hidg, ctrl);
		//return polycom_hidg_setup_get_ep0_data_to_list(hidg, ctrl);
#else
		return polycom_hidg_usb_ep0_out_data(hidg, ctrl);
#endif
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
						   polycom_hidg_desc.bLength);
			memcpy(req->buf, &polycom_hidg_desc, length);
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

static void polycom_hidg_disable(struct usb_function *f)
{
	struct f_polycom_hidg *hidg = func_to_polycom_hidg(f);
	struct f_polycom_hidg_list *list, *next;
	unsigned long flags;

	usb_ep_disable(hidg->in_ep);
	hidg->in_ep->driver_data = NULL;

	//usb_ep_disable(hidg->out_ep);
	//hidg->out_ep->driver_data = NULL;

	spin_lock_irqsave(&hidg->read_spinlock, flags);
	list_for_each_entry_safe(list, next, &hidg->completed_out_req, list) {
		//free_ep_req(hidg->out_ep, list->req);
		list_del(&list->list);
		kfree(list);
	}
	spin_unlock_irqrestore(&hidg->read_spinlock, flags);
}

static int polycom_hidg_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev		*cdev = f->config->cdev;
	struct f_polycom_hidg				*hidg = func_to_polycom_hidg(f);
	int status = 0;

	VDBG(cdev, "hidg_set_alt intf:%d alt:%d\n", intf, alt);
	
	if(intf == hidg->control_intf)
	{
		if(hidg->control_ep != NULL)
			hidg->control_ep->driver_data = hidg;
	}

	if (hidg->in_ep != NULL) {
		/* restart endpoint */
		if (hidg->in_ep->driver_data != NULL)
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
		hidg->in_ep->driver_data = hidg;
	}

#if 0
	if (hidg->out_ep != NULL) {
		/* restart endpoint */
		if (hidg->out_ep->driver_data != NULL)
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
					polycom_hidg_alloc_ep_req(hidg->out_ep,
							  POLYCOM_HID_EP_MAX_PACKET_SIZE);
			if (req) {
				req->complete = polycom_hidg_set_report_complete;
				req->context  = hidg;
				status = usb_ep_queue(hidg->out_ep, req,
						      GFP_ATOMIC);
				if (status)
					ERROR(cdev, "%s queue req --> %d\n",
						hidg->out_ep->name, status);
			} else {
				usb_ep_disable(hidg->out_ep);
				hidg->out_ep->driver_data = NULL;
				status = -ENOMEM;
				goto fail;
			}
		}
	}
#endif
fail:
	return status;
}

extern int usb_soft_connect_reset(void);
extern int usb_soft_disconnect(void);
extern int usb_soft_connect(void);

static long polycom_hid_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
		
	case POLYCOM_HID_IOC_SEND_EP0_DATA:	
		return polycom_hidg_usb_send_ep0_data(file, (struct hid_request_data *)arg);
	case POLYCOM_HID_IOC_GET_EP0_DATA:
		return polycom_hidg_usb_get_ep0_data(file, (struct hid_request_data *)arg);	
		
	case POLYCOM_HID_IOC_SEND_EP_DATA:	
		return polycom_hidg_usb_ep_in_data(file, (struct hid_request_data *)arg);
	case POLYCOM_HID_IOC_GET_EP_DATA:
		//return polycom_hidg_usb_ep_out_data(file, (struct hid_request_data *)arg);	
		return -EINVAL;
	
	case POLYCOM_HID_IOC_USB_CONNECT_STATE:{
		int type;
		ret = copy_from_user(&type, (int *)arg ,sizeof(int));
		
		if (type == USB_STATE_CONNECT_RESET ){
			//printk("%s +%d %s:set usb connect reset\n", __FILE__,__LINE__,__func__);		
			usb_soft_connect_reset();	
			return 0;
		}else if(type == USB_STATE_DISCONNECT){
			//printk("%s +%d %s:set usb disconnect\n", __FILE__,__LINE__,__func__);
		
			usb_soft_disconnect();	
			return 0;
			
		}else if(type == USB_STATE_CONNECT){
			//printk("%s +%d %s:set usb connect\n", __FILE__,__LINE__,__func__);
			usb_soft_connect();
			return 0;
		}
		else
			return -EINVAL;
	}
	
	case POLYCOM_HID_IOC_GET_HID_REPORT_LEN:{
		//uint32_t *value = arg;
		struct f_polycom_hidg	*hidg  = file->private_data;
		//printk("***************\n");
		//printk("hidg->report_length:%d\n", hidg->report_length);
		
		//*value = hidg->report_length;
		//*((__user uint32_t *)arg) = hidg->report_length;
		ret = copy_to_user((uint32_t *)arg, &hidg->report_length, sizeof(uint32_t));
		
		return 0;	
	}
		
	default:
		printk("%s +%d %s: cmd = 0x%x\n", __FILE__,__LINE__,__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return ret;
}

const struct file_operations f_polycom_hidg_fops = {
	.owner		= THIS_MODULE,
	.open		= f_polycom_hidg_open,
	.release	= f_polycom_hidg_release,
	.write		= f_polycom_hidg_write,
	.read		= f_polycom_hidg_read,
	.poll		= f_polycom_hidg_poll,
	.llseek		= noop_llseek,
	.unlocked_ioctl = polycom_hid_ioctl,
};

static int __init polycom_hidg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	struct f_polycom_hidg		*hidg = func_to_polycom_hidg(f);
	int			status;
	dev_t			dev;

	int config_minor = DEV_HID_POLYCOM_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_POLYCOM_NUM_MAX)
		index = DEV_HID_POLYCOM_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	polycom_hidg_interface_desc.bInterfaceNumber = status;

	/* allocate instance-specific endpoints */
	status = -ENODEV;
	ep = usb_ep_autoconfig(c->cdev->gadget, &polycom_hidg_fs_in_ep_desc);
	if (!ep)
		goto fail;
	ep->driver_data = c->cdev;	/* claim */
	hidg->in_ep = ep;
#if 0
	ep = usb_ep_autoconfig(c->cdev->gadget, &polycom_hidg_fs_out_ep_desc);
	if (!ep)
		goto fail;
	ep->driver_data = c->cdev;	/* claim */
	hidg->out_ep = ep;
#endif
	/* preallocate request and buffer */
	status = -ENOMEM;
	hidg->req = usb_ep_alloc_request(hidg->in_ep, GFP_KERNEL);
	if (!hidg->req)
		goto fail;

	hidg->req->buf = kmalloc(POLYCOM_HID_EP_MAX_PACKET_SIZE, GFP_KERNEL);
	if (!hidg->req->buf)
		goto fail;

	/* set descriptor dynamic values */
	polycom_hidg_interface_desc.bInterfaceSubClass = hidg->bInterfaceSubClass;
	polycom_hidg_interface_desc.bInterfaceProtocol = hidg->bInterfaceProtocol;

#if 0
	polycom_hidg_ss_in_ep_desc.wMaxPacketSize = cpu_to_le16(1024);
	polycom_hidg_ss_out_ep_desc.wMaxPacketSize = cpu_to_le16(1024);
	polycom_hidg_ss_in_ep_comp_desc.wBytesPerInterval = 2048;
	polycom_hidg_ss_out_ep_comp_desc.wBytesPerInterval = 2048;
	polycom_hidg_ss_in_ep_comp_desc.bMaxBurst = 1;
	polycom_hidg_ss_out_ep_comp_desc.bMaxBurst = 1;

	polycom_hidg_hs_in_ep_desc.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(2, 1024);
	polycom_hidg_hs_out_ep_desc.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(2, 1024);


	polycom_hidg_fs_in_ep_desc.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(2, 1024);
	polycom_hidg_fs_out_ep_desc.wMaxPacketSize = USB_EP_ISO_PKT_SIZE(2, 1024);
#else
	polycom_hidg_ss_in_ep_desc.wMaxPacketSize = cpu_to_le16(POLYCOM_HID_EP_MAX_PACKET_SIZE);
	polycom_hidg_ss_out_ep_desc.wMaxPacketSize = cpu_to_le16(POLYCOM_HID_EP_MAX_PACKET_SIZE);
	polycom_hidg_ss_in_ep_comp_desc.wBytesPerInterval = POLYCOM_HID_EP_MAX_PACKET_SIZE;
	polycom_hidg_ss_out_ep_comp_desc.wBytesPerInterval = POLYCOM_HID_EP_MAX_PACKET_SIZE;
	polycom_hidg_ss_in_ep_comp_desc.bMaxBurst = 0;
	polycom_hidg_ss_out_ep_comp_desc.bMaxBurst = 0;

	polycom_hidg_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(POLYCOM_HID_EP_MAX_PACKET_SIZE);
	polycom_hidg_hs_out_ep_desc.wMaxPacketSize = cpu_to_le16(POLYCOM_HID_EP_MAX_PACKET_SIZE);


	polycom_hidg_fs_in_ep_desc.wMaxPacketSize = cpu_to_le16(POLYCOM_HID_EP_MAX_PACKET_SIZE);
	polycom_hidg_fs_out_ep_desc.wMaxPacketSize = cpu_to_le16(POLYCOM_HID_EP_MAX_PACKET_SIZE);
#endif


	
	polycom_hidg_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	polycom_hidg_desc.desc[0].wDescriptorLength =
		cpu_to_le16(hidg->report_desc_length);

	polycom_hidg_ss_in_ep_desc.bEndpointAddress =	
	polycom_hidg_hs_in_ep_desc.bEndpointAddress =
		polycom_hidg_fs_in_ep_desc.bEndpointAddress;
	
	polycom_hidg_ss_out_ep_desc.bEndpointAddress =	
	polycom_hidg_hs_out_ep_desc.bEndpointAddress =
		polycom_hidg_fs_out_ep_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, polycom_hidg_fs_descriptors,
			polycom_hidg_hs_descriptors, polycom_hidg_ss_descriptors, NULL);
	if (status)
		goto fail;

	spin_lock_init(&hidg->write_spinlock);
	spin_lock_init(&hidg->read_spinlock);
	init_waitqueue_head(&hidg->write_queue);
	init_waitqueue_head(&hidg->read_queue);
	INIT_LIST_HEAD(&hidg->completed_out_req);
	
	
	
	
	hidg->control_intf = polycom_hidg_interface_desc.bInterfaceNumber;
	hidg->control_ep = c->cdev->gadget->ep0;
	hidg->control_ep->driver_data = hidg;
	
	/* Preallocate control endpoint request. */
	hidg->control_req = usb_ep_alloc_request(c->cdev->gadget->ep0, GFP_KERNEL);
	hidg->control_buf = kmalloc(POLYCOM_HID_REPORT_LENGTH_MAX, GFP_KERNEL);
	if (hidg->control_req == NULL || hidg->control_buf == NULL) {
		status = -ENOMEM;
		printk("%s: kmalloc control_req or control_buf error \n", __func__);
		goto fail;
	}
	
	hidg->control_req->buf = hidg->control_buf;
	hidg->control_req->complete = polycom_hidg_ep0_complete;
	hidg->control_req->context = hidg;
	
	init_completion(&hidg->get_ep0_data_done_completion);
	init_completion(&hidg->send_ep0_data_done_completion);
	init_completion(&hidg->ep_in_data_done_completion);
	init_completion(&hidg->ep_out_data_done_completion);

	/* create char device */
	cdev_init(&hidg->cdev, &f_polycom_hidg_fops);
	dev = MKDEV(polycom_major[index], config_minor);
	status = cdev_add(&hidg->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device_create(polycom_hidg_class, NULL, dev, NULL, "%s%d", "hidg-polycom", config_minor - DEV_HID_POLYCOM_FIRST_NUM);
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
	
	if (hidg->control_ep)
		hidg->control_ep->driver_data = NULL;

	if (hidg->control_req)
		usb_ep_free_request(c->cdev->gadget->ep0, hidg->control_req);
	
	if(hidg->control_buf != NULL)
		kfree(hidg->control_buf);

	return status;
}

static void polycom_hidg_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_polycom_hidg *hidg = func_to_polycom_hidg(f);
	struct usb_composite_dev *cdev = c->cdev;

	int config_minor = DEV_HID_POLYCOM_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_POLYCOM_NUM_MAX)
		index = DEV_HID_POLYCOM_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	device_destroy(polycom_hidg_class, MKDEV(polycom_major[index], config_minor));
	cdev_del(&hidg->cdev);

	/* disable/free request and end point */
	usb_ep_disable(hidg->in_ep);
	//usb_ep_disable(hidg->out_ep);
	usb_ep_dequeue(hidg->in_ep, hidg->req);
	kfree(hidg->req->buf);
	usb_ep_free_request(hidg->in_ep, hidg->req);
	
	hidg->control_ep->driver_data = NULL;
	
	if (hidg->control_req)
		usb_ep_free_request(cdev->gadget->ep0, hidg->control_req);
	
	if(hidg->control_buf)
		kfree(hidg->control_buf);
	
	complete_all(&hidg->get_ep0_data_done_completion);
	complete_all(&hidg->send_ep0_data_done_completion);
	complete_all(&hidg->ep_in_data_done_completion);
	complete_all(&hidg->ep_out_data_done_completion);

	usb_free_all_descriptors(f);

	kfree(hidg->report_desc);
	kfree(hidg);
}

/*-------------------------------------------------------------------------*/
/*                                 Strings                                 */

#define POLYCOM_CT_FUNC_HID_IDX	0

static struct usb_string polycom_ct_func_string_defs[] = {
	[POLYCOM_CT_FUNC_HID_IDX].s	= "HID Interface",
	{},			/* end of list */
};

static struct usb_gadget_strings polycom_ct_func_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= polycom_ct_func_string_defs,
};

static struct usb_gadget_strings *polycom_ct_func_strings[] = {
	&polycom_ct_func_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                             usb_configuration                           */

int __init polycom_hidg_bind_config(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc)
{
	struct f_polycom_hidg *hidg;
	int status;
	
#if 0
	/* maybe allocate device-global string IDs, and patch descriptors */
	if (ct_func_string_defs[POLYCOM_CT_FUNC_HID_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		ct_func_string_defs[POLYCOM_CT_FUNC_HID_IDX].id = status;
		hidg_interface_desc.iInterface = status;
	}
#endif

	/* allocate and initialize one new instance */
	hidg = kzalloc(sizeof *hidg, GFP_KERNEL);
	if (!hidg){
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}


	if(fdesc == NULL){
		hidg->bInterfaceSubClass = polycom_hid_des.subclass;
		hidg->bInterfaceProtocol = polycom_hid_des.protocol;
		hidg->report_length = polycom_hid_des.report_length;
		hidg->report_desc_length = sizeof(report_desc);//polycom_hid_des.report_desc_length;
		hidg->report_desc = kmemdup(report_desc,
						hidg->report_desc_length,
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

	hidg->func.name    = "polycom_hid";
	hidg->func.strings = polycom_ct_func_strings;
	hidg->func.bind    = polycom_hidg_bind;
	hidg->func.unbind  = polycom_hidg_unbind;
	hidg->func.set_alt = polycom_hidg_set_alt;
	hidg->func.disable = polycom_hidg_disable;
	hidg->func.setup   = polycom_hidg_setup;

	/* this could me made configurable at some point */
	//hidg->qlen	   = 4;

	status = usb_add_function(c, &hidg->func);
	if (status)
		kfree(hidg);

	printk("%s +%d %s: hidg_bind_config ok!\n", __FILE__,__LINE__,__func__);
	
	return status;
}

int __init polycom_ghid_setup(struct usb_gadget *g, u8 bConfigurationValue)
{
	int status;
	dev_t dev;
	
	int count = DEV_HID_POLYCOM_FIRST_NUM + bConfigurationValue;
	
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_POLYCOM_NUM_MAX)
		index = DEV_HID_POLYCOM_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	if(polycom_hidg_class == NULL)
		polycom_hidg_class = class_create(THIS_MODULE, "hidg-polycom");
     
	status = alloc_chrdev_region(&dev, 0, count, "hidg-polycom");
	if (!status) {
		polycom_major[index] = MAJOR(dev);
		polycom_minors[index] = count;
	}

	return status;
}

void polycom_ghid_cleanup(u8 bConfigurationValue)
{
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_POLYCOM_NUM_MAX)
		index = DEV_HID_POLYCOM_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	if (polycom_major[index]) {
		unregister_chrdev_region(MKDEV(polycom_major[index], 0), polycom_minors[index]);
		polycom_major[index] = polycom_minors[index] = 0;
	}

	if(polycom_hidg_class != NULL && bConfigurationValue == 1)
	{
		class_destroy(polycom_hidg_class);
		polycom_hidg_class = NULL;
	}
}

