/*
 * dfu.c -- DFU Composite driver
 *
 * Based on multi.c
 *
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

#include "usb/dfu.h"

/*-------------------------------------------------------------------------*/
#define DFU_VENDOR_ID		0x095D
#define DFU_PRODUCT_ID		0x9213
#define DFU_DEVICE_BCD		0x0001	


static char dfu_flash_label[] = FLASH_DESC_STR;


/* string IDs are assigned dynamically */
#define STRING_VENDOR_IDX			0
#define STRING_PRODUCT_IDX			1
#define STRING_SERIAL_NUMBER_IDX	2
#define STRING_DFU_FLASH_IDX		3

/*-------------------------------------------------------------------------*/

/*
 * kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "../function/f_dfu.c"

static int g_vendor_id = DFU_VENDOR_ID;
static int g_customer_id = DFU_PRODUCT_ID;
static int g_device_bcd = DFU_DEVICE_BCD;

static char g_usb_vendor_label_arr[64] = {0}; 
static char *g_usb_vendor_label1 = "Linux Foundation";
static char *g_usb_vendor_label2 = NULL;
static char *g_usb_vendor_label3 = NULL;
static char *g_usb_vendor_label4 = NULL;

static char g_usb_product_label_arr[64] = {0}; 
static char *g_usb_product_label1 = "HD Camera";
static char *g_usb_product_label2 = NULL;
static char *g_usb_product_label3 = NULL;
static char *g_usb_product_label4 = NULL;
static char *g_usb_product_label5 = NULL;

static char g_serial_number_arr[64] = {0}; 
static char *g_serial_number = "123";

//static int g_audio_enable = 1;
//static int g_usb_uvc_mode = 0; 

module_param(g_vendor_id, int , 0644);
MODULE_PARM_DESC(g_vendor_id, "Vendor ID");

module_param(g_customer_id, int , 0644);
MODULE_PARM_DESC(g_customer_id, "Customer ID");

module_param(g_device_bcd, int , 0644);
MODULE_PARM_DESC(g_device_bcd, "Device BCD");

//serial number       linshiheng 20180913
module_param(g_serial_number, charp, 0644);
MODULE_PARM_DESC(g_serial_number,"Serial Number");

#if 1
module_param(g_usb_vendor_label1, charp, 0644);
module_param(g_usb_vendor_label2, charp, 0644);
module_param(g_usb_vendor_label3, charp, 0644);
module_param(g_usb_vendor_label4, charp, 0644);
MODULE_PARM_DESC(g_usb_vendor_label1,"Vendor Label");

module_param(g_usb_product_label1, charp, 0644);
module_param(g_usb_product_label2, charp, 0644);
module_param(g_usb_product_label3, charp, 0644);
module_param(g_usb_product_label4, charp, 0644);
module_param(g_usb_product_label5, charp, 0644);
MODULE_PARM_DESC(g_usb_product_label1,"Product Label");
#else
staitc int n_para;
module_param_array(g_usb_vendor_label, char, &n_para, S_IRUGO);
module_param_array(g_usb_product_label, char, &n_para, S_IRUGO);
#endif



static char *dfu_flash_string = FLASH_DESC_STR;
module_param(dfu_flash_string, charp, S_IRUGO);
MODULE_PARM_DESC(dfu_flash_string, "DFU Flash string");

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
	.idVendor			= cpu_to_le16(DFU_VENDOR_ID),
	.idProduct			= cpu_to_le16(DFU_PRODUCT_ID),
	.bcdDevice			= cpu_to_le16(DFU_DEVICE_BCD),
	.iManufacturer		= 0, /* dynamic */
	.iProduct			= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};


/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
	[STRING_VENDOR_IDX].s			= g_usb_vendor_label_arr,
	[STRING_PRODUCT_IDX].s 			= g_usb_product_label_arr,
	[STRING_SERIAL_NUMBER_IDX].s 	= g_serial_number_arr,
	[STRING_DFU_FLASH_IDX].s 		= dfu_flash_label,
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
	return dfug_bind_config(c, 1, strings_dev[STRING_DFU_FLASH_IDX].id);
}

static struct usb_configuration config_driver = {
	.label			= "DFU Gadget",
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_ONE,//Bus Powered ,
	.MaxPower		= 500,
};

/****************************** Gadget Bind ******************************/

static int __init dfu_bind(struct usb_composite_dev *cdev)
{
	int status;
	uint32_t i;
	
	printk("g_vendor_id:%x\n", g_vendor_id);
	printk("g_customer_id:%x\n", g_customer_id);
	printk("g_device_bcd:%x\n",g_device_bcd);

	printk("g_usb_vendor_label1:%s\n", g_usb_vendor_label1);
	printk("g_usb_vendor_label2:%s\n", g_usb_vendor_label2);
	printk("g_usb_vendor_label3:%s\n", g_usb_vendor_label3);
	printk("g_usb_vendor_label4:%s\n", g_usb_vendor_label4);

	printk("g_usb_product_label1:%s\n", g_usb_product_label1);
	printk("g_usb_product_label2:%s\n", g_usb_product_label2);
	printk("g_usb_product_label3:%s\n", g_usb_product_label3);
	printk("g_usb_product_label4:%s\n", g_usb_product_label4);
	printk("g_usb_product_label5:%s\n", g_usb_product_label5);

	for(i = 0; i < 255; i++){
		if(dfu_flash_string[i] == '#')
			dfu_flash_string[i] = ' ';	
		if(dfu_flash_string[i] == 0)
			break;
	}
	
	strcat(g_usb_vendor_label_arr, g_usb_vendor_label1);
	if (g_usb_vendor_label2 != NULL && g_usb_vendor_label2[0] != ' ' && g_usb_vendor_label2[0] != '\0')
	{
	    strcat(g_usb_vendor_label_arr, " ");
	    strcat(g_usb_vendor_label_arr, g_usb_vendor_label2);
	}
	if (g_usb_vendor_label3 != NULL && g_usb_vendor_label3[0] != ' ' && g_usb_vendor_label3[0] != '\0')
	{
	    strcat(g_usb_vendor_label_arr, " ");
	    strcat(g_usb_vendor_label_arr, g_usb_vendor_label3);
	}
	printk("g_usb_vendor_label_arr:%s\n", g_usb_vendor_label_arr);

	strcat(g_usb_product_label_arr, g_usb_product_label1);
	if (g_usb_product_label2 != NULL && g_usb_product_label2[0] != ' ' && g_usb_product_label2[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label2);
	}
	if (g_usb_product_label3 != NULL && g_usb_product_label3[0] != ' ' && g_usb_product_label3[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label3);
	}
	if (g_usb_product_label4 != NULL && g_usb_product_label4[0] != ' ' && g_usb_product_label4[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label4);
	}
	if (g_usb_product_label5 != NULL && g_usb_product_label5[0] != ' ' && g_usb_product_label5[0] != '\0')
	{
	    strcat(g_usb_product_label_arr, " ");
	    strcat(g_usb_product_label_arr, g_usb_product_label5);
	}
	printk("g_usb_prodcut_label_arr:%s\n", g_usb_product_label_arr);
	
	strcat(g_serial_number_arr, g_serial_number);
	printk("g_serial_number_arr:%s\n", g_serial_number_arr);

	device_desc.idVendor = g_vendor_id;
	device_desc.idProduct = g_customer_id;	
	device_desc.bcdDevice = g_device_bcd; 
	

	strings_dev[STRING_DFU_FLASH_IDX].s = dfu_flash_string;

	/* set up DFU */
	status = gdfu_setup(cdev->gadget, config_driver.bConfigurationValue);
	if (status < 0)
		return status;

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */

	status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		return status;
	
	//strings_dev[STRING_SERIAL_NUMBER_IDX].id = status;
	device_desc.iManufacturer = strings_dev[STRING_VENDOR_IDX].id;
	device_desc.iProduct = strings_dev[STRING_PRODUCT_IDX].id;
	device_desc.iSerialNumber = strings_dev[STRING_SERIAL_NUMBER_IDX].id;

	/* register our configuration */
	status = usb_add_config(cdev, &config_driver, do_config);
	if (status < 0)
		return status;

	usb_composite_overwrite_options(cdev, &coverwrite);

	return 0;
}

static int dfu_unbind(struct usb_composite_dev *cdev)
{

	return 0;
}



/* --------------------------------------------------------------------------
 * Driver
 */

static __refdata struct usb_composite_driver dfug_driver = {
	.name		= "g_dfu",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= dfu_bind,
	.unbind		= dfu_unbind,
};

module_usb_composite_driver(dfug_driver);


MODULE_AUTHOR("QHC");
MODULE_DESCRIPTION("DFU Gadget");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");
