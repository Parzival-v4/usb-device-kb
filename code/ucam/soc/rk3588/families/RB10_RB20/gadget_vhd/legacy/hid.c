/*
 * hid.c -- HID Composite driver
 *
 * Based on multi.c
 *
 * Copyright (C) 2010 Fabien Chouteau <fabien.chouteau@barco.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb/composite.h>

//#include "gadget_chips.h"

/*-------------------------------------------------------------------------*/
#define HIDG_VENDOR_ID		0x2E7E
#define HIDG_PRODUCT_ID		0x0000
#define HIDG_DEVICE_BCD		0x0712	/* 0.11 */

static char hid_product_label[] = "Camera Upgrade";
//USB�汾�ű�ʶ
static char hidg_software_version[] = "V2.02";

/* string IDs are assigned dynamically */
#define STRING_PRODUCT_IDX			0
#define STRING_SERIAL_NUMBER_IDX	1

/*-------------------------------------------------------------------------*/

/*
 * kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "../function/f_hid.c"

static ushort usb_vid = HIDG_VENDOR_ID;
module_param(usb_vid, ushort, S_IRUGO);
MODULE_PARM_DESC(usb_vid, "USB Vendor ID");

static ushort usb_pid = HIDG_PRODUCT_ID;
module_param(usb_pid, ushort, S_IRUGO);
MODULE_PARM_DESC(usb_pid, "USB Product ID");

static ushort usb_bcd = HIDG_DEVICE_BCD;
module_param(usb_bcd, ushort, S_IRUGO);
MODULE_PARM_DESC(usb_bcd, "USB Device version (BCD)");

static char *usb_product_string = "Camera Upgrade";
module_param(usb_product_string, charp, S_IRUGO);
MODULE_PARM_DESC(usb_product_string, "USB Product string");

static char *usb_serial_string = "V1.01";
module_param(usb_serial_string, charp, S_IRUGO);
MODULE_PARM_DESC(usb_serial_string, "USB Serial string");




struct hidg_func_node {
	struct list_head node;
	struct hidg_func_descriptor *func;
};

static LIST_HEAD(hidg_func_list);

/*-------------------------------------------------------------------------*/
USB_GADGET_COMPOSITE_OPTIONS();

static struct usb_device_descriptor device_desc = {
	.bLength			= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB				= cpu_to_le16(0x0200),
	.bDeviceClass		= USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass	= 0x00,
	.bDeviceProtocol	= 0x00,
	//.bMaxPacketSize0	= 0, /* dynamic */
	.idVendor			= cpu_to_le16(HIDG_VENDOR_ID),
	.idProduct			= cpu_to_le16(HIDG_PRODUCT_ID),
	.bcdDevice			= cpu_to_le16(HIDG_DEVICE_BCD),
	.iManufacturer		= 0, /* dynamic */
	.iProduct			= 1, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,

	/* REVISIT SRP-only hardware is possible, although
	 * it would not be called "OTG" ...
	 */
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};


/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
	[STRING_PRODUCT_IDX].s = hid_product_label,
	[STRING_SERIAL_NUMBER_IDX].s = hidg_software_version,
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};



/****************************** Configurations ******************************/

static int __init do_config(struct usb_configuration *c)
{
	struct hidg_func_node *e;
	int status = 0;

	if (gadget_is_otg(c->cdev->gadget)) {
		c->descriptors = otg_desc;
		c->bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	}

	list_for_each_entry(e, &hidg_func_list, node) {
		status = hidg_bind_config(c, e->func,  0);
		if (status)
			break;
	}

	return status;
}

static struct usb_configuration config_driver = {
	.label			= "HID Gadget",
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_ONE,//Bus Powered ,
	.MaxPower		= 500,
};

/****************************** Gadget Bind ******************************/

static int __init hid_bind(struct usb_composite_dev *cdev)
{
	//struct usb_gadget *gadget = cdev->gadget;
	struct list_head *tmp;
	int status, funcs = 0;
	uint32_t i;
	

	for(i = 0; i < 255; i++){
		if(usb_product_string[i] == '#')
			usb_product_string[i] = ' ';	
		if(usb_product_string[i] == 0)
			break;
	}
	
	for(i = 0; i < 255; i++){
		if(usb_serial_string[i] == '#')
			usb_serial_string[i] = ' ';	
		if(usb_serial_string[i] == 0)
			break;
	}

	device_desc.idProduct = usb_pid;	
	device_desc.idVendor = usb_vid;
	device_desc.bcdDevice = usb_bcd; 
	strings_dev[STRING_PRODUCT_IDX].s = usb_product_string;
	strings_dev[STRING_SERIAL_NUMBER_IDX].s = usb_serial_string;
	
	
	list_for_each(tmp, &hidg_func_list)
		funcs++;

	if (!funcs)
		return -ENODEV;

	/* set up HID */
	status = ghid_setup(cdev->gadget, funcs);
	if (status < 0)
		return status;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		return status;
	
	strings_dev[STRING_SERIAL_NUMBER_IDX].id = status;
	device_desc.iSerialNumber = status;

	/* register our configuration */
	status = usb_add_config(cdev, &config_driver, do_config);
	if (status < 0)
		return status;

	usb_composite_overwrite_options(cdev, &coverwrite);
	//dev_info(&gadget->dev, hid_product_label ", version: " hidg_software_version "\n");

	//if(usb_gadget_connect(cdev->gadget) != 0)
	//	printk("usb_gadget_connect error!\n");

	return 0;
}

static int __exit hid_unbind(struct usb_composite_dev *cdev)
{
	//ghid_cleanup();
	return 0;
}

static int __init hidg_plat_driver_probe(struct platform_device *pdev)
{
	struct hidg_func_descriptor *func = dev_get_platdata(&pdev->dev);
	struct hidg_func_node *entry;

	if (!func) {
		dev_err(&pdev->dev, "Platform data missing\n");
		return -ENODEV;
	}

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	entry->func = func;
	list_add_tail(&entry->node, &hidg_func_list);

	return 0;
}

static int hidg_plat_driver_remove(struct platform_device *pdev)
{
	struct hidg_func_node *e, *n;

	list_for_each_entry_safe(e, n, &hidg_func_list, node) {
		list_del(&e->node);
		kfree(e);
	}

	return 0;
}


/****************************** Some noise ******************************/


static __refdata struct usb_composite_driver hidg_driver = {
	.name		= "g_hid",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= hid_bind,
	.unbind		= __exit_p(hid_unbind),
};

static struct platform_driver hidg_plat_driver = {
	.remove		= hidg_plat_driver_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "hidg",
	},
};

static struct platform_device my_hid = {
		.name = "hidg",
		.id = 1,
		.num_resources = 0,
		.resource = 0,
		.dev = {
				.platform_data = &hid_des_ep0,
		}
};




static int __init hidg_init(void)
{
	int status;
	
	status = platform_device_register(&my_hid);
	if (status < 0) {
		printk("____ reg failed\n");
		platform_device_unregister(&my_hid);
		return status;
	}

	status = platform_driver_probe(&hidg_plat_driver,
				hidg_plat_driver_probe);
	if (status < 0)
		return status;

	status = usb_composite_probe(&hidg_driver);
	if (status < 0)
		platform_driver_unregister(&hidg_plat_driver);

	return status;
}
module_init(hidg_init);

static void __exit hidg_cleanup(void)
{
	platform_driver_unregister(&hidg_plat_driver);
	platform_device_unregister(&my_hid);
	usb_composite_unregister(&hidg_driver);
}
module_exit(hidg_cleanup);


MODULE_DESCRIPTION("Camera Upgrade");
MODULE_AUTHOR("Fabien Chouteau, Peter Korsgaard");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");
