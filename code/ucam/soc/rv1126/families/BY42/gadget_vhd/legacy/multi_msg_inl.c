/*
 * multi.c -- Multifunction Composite driver
 *
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 Nokia Corporation
 * Copyright (C) 2009 Samsung Electronics
 * Author: Michal Nazarewicz (mina86@mina86.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <linux/kernel.h>
#include <linux/usb/ch9.h>
#include <linux/module.h>

#include "f_mass_storage.h"

static struct usb_function_instance *fi_msg = NULL;
static struct usb_function *f_msg = NULL;


/****************************** Configurations ******************************/

static struct fsg_module_parameters mod_data = {
	.stall = 1
};
#ifdef CONFIG_USB_GADGET_DEBUG_FILES

static unsigned int fsg_num_buffers = CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS;

#else

/*
 * Number of buffers we will use.
 * 2 is usually enough for good buffering pipeline
 */
#define fsg_num_buffers	CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS

#endif /* CONFIG_USB_GADGET_DEBUG_FILES */

FSG_MODULE_PARAMETERS(/* no prefix */, mod_data);

static int multi_msg_do_config(struct usb_configuration *c)
{
	struct fsg_opts *opts;
	int ret;

	opts = fsg_opts_from_func_inst(fi_msg);

	f_msg = usb_get_function(fi_msg);
	if (IS_ERR(f_msg))
		return PTR_ERR(f_msg);

	ret = usb_add_function(c, f_msg);
	if (ret)
		goto put_func;

	return 0;

put_func:
	usb_put_function(f_msg);
	return ret;
}


/****************************** Gadget Bind ******************************/

static int multi_msg_bind(struct usb_composite_dev *cdev)
{
	struct fsg_opts *opts;
	struct fsg_config config;
	int status;

	fi_msg = usb_get_function_instance("mass_storage");
	if (IS_ERR(fi_msg))
		return PTR_ERR(fi_msg);

	fsg_config_from_params(&config, &mod_data, fsg_num_buffers);
	opts = fsg_opts_from_func_inst(fi_msg);

	opts->no_configfs = true;
	status = fsg_common_set_num_buffers(opts->common, fsg_num_buffers);
	if (status)
		goto fail;

	status = fsg_common_set_cdev(opts->common, cdev, config.can_stall);
	if (status)
		goto fail_set_cdev;

	fsg_common_set_sysfs(opts->common, true);
	status = fsg_common_create_luns(opts->common, &config);
	if (status)
		goto fail_set_cdev;

	fsg_common_set_inquiry_string(opts->common, config.vendor_name,
				      config.product_name);

	
	return 0;


fail_set_cdev:
	fsg_common_free_buffers(opts->common);
fail:
	usb_put_function_instance(fi_msg);
	return status;
}

static int multi_msg_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR(f_msg))
		usb_put_function(f_msg);

	if (!IS_ERR(fi_msg))
		usb_put_function_instance(fi_msg);

	return 0;
}