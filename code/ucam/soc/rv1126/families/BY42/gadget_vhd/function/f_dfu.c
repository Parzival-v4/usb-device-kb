/*
 * f_dfu.c -- USB DFU function driver
 *
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
#include <linux/completion.h>

//#include "hi_osal.h"

#include "u_f.h"
#include "uvc.h"
#include "usb/dfu.h"
#include "dfu_config.h"
#include "u_os_desc.h"

#ifndef WRITE_SUPPORT
//#define WRITE_SUPPORT
#endif

struct dfu_dev_name_string
{
	char string[256];
};
#define DFU_IOC_SET_DEV_NAME_STRING            	_IOW('D', 7, struct dfu_dev_name_string)

static char g_dfu_string[256] = "DFU Run-Time Interface"; 
int g_set_dfu_string_flag = 0;

static int dfu_major[DEV_DFU_NUM_MAX], dfu_minors[DEV_DFU_NUM_MAX];

static struct class *dfug_class = NULL;

void gdfu_cleanup(u8 bConfigurationValue);

/*-------------------------------------------------------------------------*/
/*                            DFU gadget struct                            */

struct f_dfug_req_list {
	const struct usb_ctrlrequest 	*ctrl;
	unsigned int		pos;
	struct list_head 	list;
};


struct f_dfug {

	/* recv report */
	struct list_head		completed_out_req;
	spinlock_t				spinlock;
	wait_queue_head_t		read_queue;

#ifdef WRITE_SUPPORT	
	/* send report */
	struct mutex			lock;
	bool					write_pending;
	wait_queue_head_t		write_queue;
#endif

	//int						minor;
	struct cdev				cdev;
	struct usb_function		func;
	
	//usb control ep 
	unsigned int 			control_intf;
	struct usb_ep 			*control_ep;
	struct usb_request 		*control_req;
	void 					*control_buf;
	
	unsigned int			dfu_xfer_size;
	
	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
	
	struct completion  get_ep0_data_done_completion;
};

static inline struct f_dfug *func_to_dfug(struct usb_function *f)
{
	return container_of(f, struct f_dfug, func);
}

/*-------------------------------------------------------------------------*/
/*                           Static descriptors                            */

static struct usb_interface_descriptor dfu_interface_alt0_desc = {
	.bLength			= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0,
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 0,
	.bInterfaceClass	= USB_CLASS_APP_SPEC,
	.bInterfaceSubClass	= 1,
	.bInterfaceProtocol	= 1,/* dynamic */ //Run-Time mode 1, DFU mode 2
	.iInterface			= 0,/* dynamic */
};


const struct dfu_functional_descriptor dfu_functional_desc = {
	.bLength 			= USB_DFU_FUNC_DESC_SIZE,
	.bDescriptorType 	= USB_DT_WIRE_ADAPTER,
	.bmAttributes 		= 0x0D,
	.wDetachTimeOut 	= 0xFF,
	.wTransferSize 		= DFU_USB_BUFSIZ,
	.bcdDFUVersion 		= 0x0110,
};

static struct usb_descriptor_header *dfug_ss_descriptors[] = {
	(struct usb_descriptor_header *)&dfu_interface_alt0_desc,
	(struct usb_descriptor_header *)&dfu_functional_desc,
	NULL,
};

static struct usb_descriptor_header *dfug_hs_descriptors[] = {
	(struct usb_descriptor_header *)&dfu_interface_alt0_desc,
	(struct usb_descriptor_header *)&dfu_functional_desc,
	NULL,
};

static struct usb_descriptor_header *dfug_fs_descriptors[] = {
	(struct usb_descriptor_header *)&dfu_interface_alt0_desc,
	(struct usb_descriptor_header *)&dfu_functional_desc,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                              Char Device                                */

static void dfug_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_dfug *dfug = (struct f_dfug *) req->context;
	
	if (dfug->event_setup_out) 
	{
		//printk("dfug_ep0_complete: length = %d\n", req->length);
		dfug->event_setup_out = 0;
		complete_all(&dfug->get_ep0_data_done_completion);
		
	}
	else
	{
#ifdef WRITE_SUPPORT
		if (req->status != 0) {
			ERROR(dfug->func.config->cdev,
			"End Point Request ERROR: %d\n", req->status);
		}

		dfug->write_pending = 0;
		wake_up(&dfug->write_queue);
#endif
	}
}


static ssize_t f_dfug_read(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_dfug *dfug = file->private_data;
	struct f_dfug_req_list *list;
	const struct usb_ctrlrequest 	*ctrl;
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

	spin_lock_irqsave(&dfug->spinlock, flags);

#define READ_COND (!list_empty(&dfug->completed_out_req))

	/* wait for at least one buffer to complete */
	while (!READ_COND) {
		spin_unlock_irqrestore(&dfug->spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(dfug->read_queue, READ_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&dfug->spinlock, flags);
	}

	
	/* pick the first one */
	list = list_first_entry(&dfug->completed_out_req,
				struct f_dfug_req_list, list);

	ctrl = list->ctrl;
	count = min_t(unsigned int, count, sizeof(struct usb_ctrlrequest));
	spin_unlock_irqrestore(&dfug->spinlock, flags);
	
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
		spin_lock_irqsave(&dfug->spinlock, flags);
		list_del(&list->list);
		kfree(list);
		spin_unlock_irqrestore(&dfug->spinlock, flags);
	}


	return count;
}


static ssize_t f_dfug_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	ssize_t status = -ENOMEM;

#ifdef WRITE_SUPPORT
	struct f_dfug *dfug = (struct f_dfug *)file->private_data;
	
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;
#endif

	mutex_lock(&dfug->lock);

#define WRITE_COND (!dfug->write_pending)

	/* write queue */
	while (!WRITE_COND) {
		mutex_unlock(&dfug->lock);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible_exclusive(
				dfug->write_queue, WRITE_COND))
			return -ERESTARTSYS;

		mutex_lock(&dfug->lock);
	}

	count  = min_t(unsigned, count, dfug->dfu_xfer_size);
	status = copy_from_user(dfug->control_req->buf, buffer, count);

	if (status != 0) {
		ERROR(dfug->func.config->cdev,
			"copy_from_user error\n");
		mutex_unlock(&dfug->lock);
		return -EINVAL;
	}

	dfug->control_req->status   = 0;
	dfug->control_req->zero     = 0;
	dfug->control_req->length   = count;
	dfug->control_req->complete = dfug_ep0_complete;
	dfug->control_req->context  = dfug;
	dfug->write_pending = 1;

	status = usb_ep_queue(dfug->control_ep, dfug->control_req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(dfug->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		dfug->write_pending = 0;
		wake_up(&dfug->write_queue);
	} else {
		status = count;
	}


	mutex_unlock(&dfug->lock);
	
#endif
	
	return status;
}

static unsigned int f_dfug_poll(struct file *file, poll_table *wait)
{
	struct f_dfug	*dfug  = file->private_data;
	unsigned int	ret = 0;


	poll_wait(file, &dfug->read_queue, wait);

#ifdef WRITE_SUPPORT	
	poll_wait(file, &dfug->write_queue, wait);

	if (WRITE_COND)
		ret |= POLLOUT | POLLWRNORM;
#endif	

	if (READ_COND)
		ret |= POLLIN | POLLRDNORM;

	return ret;
}

#undef WRITE_COND
#undef READ_COND

static int f_dfug_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static int f_dfug_open(struct inode *inode, struct file *fd)
{
	struct f_dfug *dfug =
		container_of(inode->i_cdev, struct f_dfug, cdev);

	fd->private_data = dfug;

	return 0;
}

/*-------------------------------------------------------------------------*/
/*                                usb_function                             */

static int
dfug_usb_send_ep0_data(struct file* file, struct dfu_request_data *data)
{
	struct f_dfug	*dfug  = file->private_data;
	struct usb_composite_dev *cdev = dfug->func.config->cdev;
	struct usb_request *req = dfug->control_req;
	ssize_t status = -ENOMEM;
	__s32 length;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		return usb_ep_set_halt(cdev->gadget->ep0);
	}	
	
	if (length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	
	req->length = min_t(unsigned int, dfug->dfu_xfer_size, length);
	req->zero = length < dfug->event_length;
	
	if(length > 0)
	{
		status = copy_from_user(req->buf, data->data, req->length);
		if (status != 0) {
			ERROR(cdev,"copy_from_user error\n");
			return usb_ep_set_halt(cdev->gadget->ep0);
		}
	}
	//printk("dfug_usb_send_ep0_data : %d\n",req->length);
	
	return usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
}

static int
dfug_usb_get_ep0_data(struct file* file, struct dfu_request_data *data)
{
	struct f_dfug	*dfug  = file->private_data;
	struct usb_composite_dev *cdev = dfug->func.config->cdev;
	struct usb_request *req = dfug->control_req;
	ssize_t status = -ENOMEM;
	__s32 length;
	int ret;

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
	
	req->length = min_t(unsigned int, dfug->dfu_xfer_size, length);
	req->zero = length < dfug->event_length;

	//printk("dfug_usb_get_ep0_data : %d\n",req->length);
	
	dfug->get_ep0_data_done_completion.done = 0;
	
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		complete_all(&dfug->get_ep0_data_done_completion);
		return ret;
	}
	wait_for_completion_interruptible(&dfug->get_ep0_data_done_completion);
	
	length = min_t(unsigned int, req->length, length);
	status = copy_to_user(data->data, req->buf, length);
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");	
		
		return -1;
	}	
	
	return length;
}


static void dfug_setup_add_list(struct f_dfug *dfug, const struct usb_ctrlrequest *ctrl)
{
	struct f_dfug_req_list *ctrl_list;
	unsigned long flags;

	ctrl_list = kzalloc(sizeof(*ctrl_list), GFP_ATOMIC);
	if (!ctrl_list)
	{
		return;
	}

	ctrl_list->ctrl = ctrl;

	spin_lock_irqsave(&dfug->spinlock, flags);
	list_add_tail(&ctrl_list->list, &dfug->completed_out_req);
	spin_unlock_irqrestore(&dfug->spinlock, flags);

	
	wake_up(&dfug->read_queue);
}


static int dfug_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_dfug			*dfug = func_to_dfug(f);
	//struct usb_composite_dev	*cdev = f->config->cdev;
	//struct usb_request		*req  = cdev->req;
	//int status = 0;

#if 0
	printk("DFU setup request %02x %02x value %04x index %04x %04x\n",
		ctrl->bRequestType, ctrl->bRequest, le16_to_cpu(ctrl->wValue),
	 	le16_to_cpu(ctrl->wIndex), le16_to_cpu(ctrl->wLength));
#endif
		 
	if(ctrl->wIndex != dfug->control_intf)
	{
		printk("dfu intf error\n");
		return -EOPNOTSUPP;
	}

		
	/* Stall too big requests. */
	if (le16_to_cpu(ctrl->wLength) > dfug->dfu_xfer_size)
		return -EINVAL;
		
	/* Tell the complete callback to generate an event for the next request
	 * that will be enqueued by UVCIOC_SEND_RESPONSE.
	 */
	dfug->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
	dfug->event_length = le16_to_cpu(ctrl->wLength);
	
	//printk("dfug_setup_add_list\n");	
	dfug_setup_add_list(dfug, ctrl);
	
	//if(ctrl->bRequestType == 0x21 && ctrl->bRequest == 0x00 && ctrl->wValue == 0x03E8 && ctrl->wLength == 0x00)
	if(dfug->event_setup_out && ctrl->wLength == 0x00)
	{
		//printk("DFU detach\n");
		return	USB_GADGET_DELAYED_STATUS;
	}

	return 0;
}

static void dfug_disable(struct usb_function *f)
{
	struct f_dfug *dfug = func_to_dfug(f);
	struct f_dfug_req_list *list, *next;


	list_for_each_entry_safe(list, next, &dfug->completed_out_req, list) {
		list_del(&list->list);
		kfree(list);
	}
}


static int dfug_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	int status = 0;

	struct f_dfug *dfug = func_to_dfug(f);
	
	if(intf == dfug->control_intf)
	{
		printk("DFU_USB_CONNECT usb speed = %s, bConfigurationValue = %d\n",
				usb_speed_string(f->config->cdev->gadget->speed), f->config->bConfigurationValue);
				
		if(dfug->control_ep != NULL)
			dfug->control_ep->driver_data = dfug;
	}
	

	return status;
}


static long dfu_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct f_dfug	*dfug  = file->private_data;
	struct usb_composite_dev *cdev = dfug->func.config->cdev;
	int type;
	unsigned long cnt;

	switch (cmd) {
		
	case DFU_IOC_SEND_EP0_DATA:	
		return dfug_usb_send_ep0_data(file, (struct dfu_request_data *)arg);
	case DFU_IOC_GET_EP0_DATA:
		return dfug_usb_get_ep0_data(file, (struct dfu_request_data *)arg);		
	case DFU_IOC_USB_CONNECT_STATE:{	
		
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
	
	case DFU_IOC_SET_DEV_NAME_STRING:
	{
		memset(g_dfu_string, 0, sizeof(g_dfu_string));
		printk("Set dfu name:%s\n",((struct dfu_dev_name_string *)arg)->string);
		strcat(g_dfu_string, ((struct dfu_dev_name_string *)arg)->string);
		g_set_dfu_string_flag = 1;
		return 0;
	}
#ifdef USB_CHIP_DWC3	
	case DFU_IOC_GET_USB_SOFT_CONNECT_STATE:{	
		uint32_t value = get_usb_soft_connect_state();
		cnt = copy_to_user((void *)arg, (const void*)&value, sizeof(uint32_t));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		
		return 0;
	}
	
	case DFU_IOC_SET_USB_SOFT_CONNECT_CTRL:{
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
	
	case DFU_IOC_GET_USB_SOFT_CONNECT_CTRL:{
		uint32_t value = get_usb_soft_connect_ctrl();
		cnt = copy_to_user((void *)arg, (const void *)&value, sizeof(uint32_t));
		if(cnt != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
			return -1;
		}
		return 0;
	}
#endif		
	default:
		printk("%s: cmd = 0x%x\n", __func__,cmd);
		return -ENOIOCTLCMD;
	}

	return ret;
}

const struct file_operations f_dfug_fops = {
	.owner		= THIS_MODULE,
	.open		= f_dfug_open,
	.release	= f_dfug_release,
	.write		= f_dfug_write,
	.read		= f_dfug_read,
	.poll		= f_dfug_poll,
	.llseek		= noop_llseek,
	.unlocked_ioctl = dfu_ioctl,
};


char	dfu_ext_compat_id[16] = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

char	dfu_ext_name[] = "DeviceInterfaceGUIDs";
///* Add a '\0' for USB_EXT_PROP_UNICODE_MULTI type data */
char	dfu_ext_data[] = "{AD87D424-938D-4A61-B06B-2371363EFC31}\0";

struct usb_os_desc_ext_prop dfu_desc_ext_prop = {
	.type = USB_EXT_PROP_UNICODE_MULTI,
	.name_len = sizeof(dfu_ext_name) * 2,
	.name = dfu_ext_name,
	.data_len = sizeof(dfu_ext_data) * 2,
	.data = dfu_ext_data,	
};
struct usb_os_desc	dfu_os_desc;
struct usb_os_desc_table dfu_os_desc_table = {
	.if_id = 0,
	.os_desc = &dfu_os_desc,
};

#ifdef MS_OS_20_WINUSB_ENABLE
extern void SetMsOsWinUsbInterface(__u8  bFirstInterface);
#endif
static int __init dfug_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_dfug		*dfug = func_to_dfug(f);
	int			status;
	dev_t			dev;
	//7字Unicode字符串: MSFT100
	const char qw_sign[OS_STRING_QW_SIGN_LEN] = {'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00, '1', 0x00, '0', 0x00, '0', 0x00};
	
	int minor = DEV_DFU_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_DFU_NUM_MAX)
		index = DEV_DFU_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	dfu_interface_alt0_desc.bInterfaceNumber = status;
	dfug->control_intf = dfu_interface_alt0_desc.bInterfaceNumber;

	dfug->dfu_xfer_size = DFU_USB_BUFSIZ;
	dfug->control_ep = cdev->gadget->ep0;
	dfug->control_ep->driver_data = dfug;
	

	/* Preallocate control endpoint request. */
	dfug->control_req = usb_ep_alloc_request(cdev->gadget->ep0, GFP_KERNEL);
	dfug->control_buf = kmalloc(dfug->dfu_xfer_size, GFP_KERNEL);
	if (dfug->control_req == NULL || dfug->control_buf == NULL) {
		status = -ENOMEM;
		printk("%s: kmalloc control_req or control_buf error \n", __func__);
		goto fail;
	}
	
	dfug->control_req->buf = dfug->control_buf;
	dfug->control_req->complete = dfug_ep0_complete;
	dfug->control_req->context = dfug;
	
	status = usb_assign_descriptors(f, dfug_fs_descriptors,
			dfug_hs_descriptors, dfug_ss_descriptors, NULL);
	if (status)
		goto fail;

	spin_lock_init(&dfug->spinlock);
	init_waitqueue_head(&dfug->read_queue);
	INIT_LIST_HEAD(&dfug->completed_out_req);
	
#ifdef WRITE_SUPPORT
	mutex_init(&dfug->lock);
	init_waitqueue_head(&dfug->write_queue);
#endif

	init_completion(&dfug->get_ep0_data_done_completion);

	/* create char device */
	cdev_init(&dfug->cdev, &f_dfug_fops);
	dev = MKDEV(dfu_major[index], minor);

	status = cdev_add(&dfug->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device_create(dfug_class, NULL, dev, NULL, "%s%d", "dfug", minor - DEV_DFU_FIRST_NUM);

	if(c->bConfigurationValue == 1)
	{
		cdev->use_os_string = true;
		cdev->b_vendor_code = 0x01;
		cdev->os_desc_config = c;
		memcpy(cdev->qw_sign, qw_sign, OS_STRING_QW_SIGN_LEN);
		f->os_desc_table = &dfu_os_desc_table;
		f->os_desc_n = 1;
		f->os_desc_table->if_id = dfu_interface_alt0_desc.bInterfaceNumber;
		f->os_desc_table->os_desc->ext_compat_id = dfu_ext_compat_id;
		INIT_LIST_HEAD(&f->os_desc_table->os_desc->ext_prop);

		f->os_desc_table->os_desc->ext_prop_len =
			dfu_desc_ext_prop.name_len  + dfu_desc_ext_prop.data_len  + 14;
		++f->os_desc_table->os_desc->ext_prop_count;
		list_add_tail(&dfu_desc_ext_prop.entry, &f->os_desc_table->os_desc->ext_prop);

#ifdef MS_OS_20_WINUSB_ENABLE
		SetMsOsWinUsbInterface(dfu_interface_alt0_desc.bInterfaceNumber);
#endif
	}

	return 0;

fail_free_descs:
	usb_free_all_descriptors(f);
fail:
	ERROR(f->config->cdev, "dfug_bind FAILED\n");

	if (dfug->control_ep)
		dfug->control_ep->driver_data = NULL;

	if (dfug->control_req)
		usb_ep_free_request(cdev->gadget->ep0, dfug->control_req);
	
	if(dfug->control_buf != NULL)
		kfree(dfug->control_buf);

	return status;
}

static void dfug_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_dfug *dfug = func_to_dfug(f);
	struct usb_composite_dev *cdev = c->cdev;
	
	int minor = DEV_DFU_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_DFU_NUM_MAX)
		index = DEV_DFU_NUM_MAX - 1;
	else if(index < 0)
		index = 0;


	device_destroy(dfug_class, MKDEV(dfu_major[index], minor));

	cdev_del(&dfug->cdev);

	/* disable/free request and end point */
	dfug->control_ep->driver_data = NULL;
	
	if (dfug->control_req)
		usb_ep_free_request(cdev->gadget->ep0, dfug->control_req);
	
	if(dfug->control_buf)
		kfree(dfug->control_buf);

	gdfu_cleanup(c->bConfigurationValue);
	
	usb_free_all_descriptors(f);
	
	complete_all(&dfug->get_ep0_data_done_completion);
	
	kfree(dfug);
	
}

/*-------------------------------------------------------------------------*/
/*                                 Strings                                 */

#define DFU_CT_FUNC_IDX	0

static struct usb_string dfu_ct_func_string_defs[] = {
	[DFU_CT_FUNC_IDX].s	= g_dfu_string,
	{},			/* end of list */
};

static struct usb_gadget_strings dfu_ct_func_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= dfu_ct_func_string_defs,
};

static struct usb_gadget_strings *dfu_ct_func_strings[] = {
	&dfu_ct_func_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                             usb_configuration                           */

int __init dfug_bind_config(struct usb_configuration *c,
			    int dfu_mode, int iInterface)
{
	struct f_dfug *dfug;
	int status;
	
#if 1
	if(dfu_mode == 0)
	{
		memset(g_dfu_string, 0, sizeof(g_dfu_string));
		strcat(g_dfu_string, "DFU Run-Time Interface");
	}
	else
	{
		memset(g_dfu_string, 0, sizeof(g_dfu_string));
		strcat(g_dfu_string, "DFU Interface");
	}
	
	//if(g_set_dfu_string_flag == 1 || dfu_mode == 0)
	{
		/* maybe allocate device-global string IDs, and patch descriptors */
		if (dfu_ct_func_string_defs[DFU_CT_FUNC_IDX].id == 0)
		{
			status = usb_string_id(c->cdev);
			if (status < 0)
				return status;
			dfu_ct_func_string_defs[DFU_CT_FUNC_IDX].id = status;
			dfu_interface_alt0_desc.iInterface = status;
		}
	}
#endif

	/* allocate and initialize one new instance */
	dfug = kzalloc(sizeof *dfug, GFP_KERNEL);
	if (!dfug){
		printk("%s +%d %s: dfug_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}


	if(dfu_mode == 0){
		//runtime mode
		dfu_interface_alt0_desc.bInterfaceProtocol	= 1;//Run-Time mode 1, DFU mode 2
		//dfu_interface_alt0_desc.iInterface			= 0;
	}else{
		//dfu mode
		dfu_interface_alt0_desc.bInterfaceProtocol	= 2;//Run-Time mode 1, DFU mode 2
		// if(g_set_dfu_string_flag == 0)
		// {
		// 	dfu_interface_alt0_desc.iInterface			= iInterface;
		// }
	}
										

	dfug->func.name    = "dfu";
	dfug->func.strings = dfu_ct_func_strings;
	dfug->func.bind    = dfug_bind;
	dfug->func.unbind  = dfug_unbind;
	dfug->func.set_alt = dfug_set_alt;
	dfug->func.disable = dfug_disable;
	dfug->func.setup   = dfug_setup;

	status = usb_add_function(c, &dfug->func);
	if (status)
		kfree(dfug);

	printk("+%d %s: dfug_bind_config ok!\n",__LINE__,__func__);
	
	return status;
}

int __init gdfu_setup(struct usb_gadget *g, u8 bConfigurationValue)
{
	int status;
	dev_t dev;
	
	int count = DEV_DFU_FIRST_NUM + bConfigurationValue;
	
	int index = bConfigurationValue - 1;
	
	if(index > DEV_DFU_NUM_MAX)
		index = DEV_DFU_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	if(dfug_class == NULL)
		dfug_class = class_create(THIS_MODULE, "dfug");

	status = alloc_chrdev_region(&dev, 0, count, "dfug");
	if (!status) {
		dfu_major[index] = MAJOR(dev);
		dfu_minors[index] = count;
	}

	return status;
}

void gdfu_cleanup(u8 bConfigurationValue)
{
	int index = bConfigurationValue - 1;
	
	if(index > DEV_DFU_NUM_MAX)
		index = DEV_DFU_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	if (dfu_major[index]) {
		unregister_chrdev_region(MKDEV(dfu_major[index], 0), dfu_minors[index]);
		dfu_major[index] = dfu_minors[index] = 0;
	}

	if(dfug_class != NULL && bConfigurationValue == 1)
	{
		class_destroy(dfug_class);
		dfug_class = NULL;
	}
}
