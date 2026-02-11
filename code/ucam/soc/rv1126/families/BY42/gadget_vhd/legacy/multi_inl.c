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
#include <linux/module.h>
#include <linux/netdevice.h>

#ifndef CONFIG_USB_G_MULTI_RNDIS
#define CONFIG_USB_G_MULTI_RNDIS
#endif

//#define CONFIG_USB_G_MULTI_CDC

#define CONFIG_USB_F_ACM

#ifdef CONFIG_USB_F_ACM
#include "u_serial.h"
#endif

#if defined USB_ETH_RNDIS
#  undef USB_ETH_RNDIS
#endif
#ifdef CONFIG_USB_G_MULTI_RNDIS
#  define USB_ETH_RNDIS y
#endif


#include "f_mass_storage.h"

#include "u_ecm.h"
#ifdef USB_ETH_RNDIS
#  include "u_rndis.h"
#  include "rndis.h"
#endif
#include "u_ether.h"


int g_multi_rndis_enable = 0;
int g_multi_serial_enable = 0;


USB_ETHERNET_MODULE_PARAMETERS();


/****************************** Configurations ******************************/

#ifdef CONFIG_USB_F_ACM
static struct usb_function_instance *fi_acm = NULL;
#endif

/********** RNDIS **********/

#ifdef USB_ETH_RNDIS
static struct usb_function_instance *fi_rndis = NULL;

#ifdef CONFIG_USB_F_ACM
static struct usb_function *f_acm;
#endif

static struct usb_function *f_rndis;
//static struct usb_function *f_msg_rndis;

static int rndis_acm_do_config(struct usb_configuration *c)
{
	int ret;

	if(!IS_ERR_OR_NULL(fi_rndis))
	{
		f_rndis = usb_get_function(fi_rndis);
		if (IS_ERR_OR_NULL(f_rndis))
			return PTR_ERR(f_rndis);

		ret = usb_add_function(c, f_rndis);
		if (ret < 0)
			goto err_func_rndis;
	}

#ifdef CONFIG_USB_F_ACM
	if(!IS_ERR_OR_NULL(fi_acm))
	{
		f_acm = usb_get_function(fi_acm);
		if (IS_ERR_OR_NULL(f_acm)) {
			ret = PTR_ERR(f_acm);
			goto err_func_acm;
		}

		ret = usb_add_function(c, f_acm);
		if (ret)
			goto err_conf;
	}
#endif

	return 0;

#ifdef CONFIG_USB_F_ACM
err_conf:
	if(!IS_ERR_OR_NULL(fi_acm))
		usb_put_function(f_acm);
#endif

#ifdef CONFIG_USB_F_ACM
err_func_acm:
#endif
	if(!IS_ERR_OR_NULL(fi_rndis))
		usb_remove_function(c, f_rndis);
err_func_rndis:
	if(!IS_ERR_OR_NULL(fi_rndis))
		usb_put_function(f_rndis);
	return ret;
}

#endif


/********** CDC ECM **********/

#ifdef CONFIG_USB_G_MULTI_CDC
static struct usb_function_instance *fi_ecm;
static struct usb_function *f_acm_multi;
static struct usb_function *f_ecm;
static struct usb_function *f_msg_multi;

static int cdc_do_config(struct usb_configuration *c)
{
	int ret;

	f_ecm = usb_get_function(fi_ecm);
	if (IS_ERR_OR_NULL(f_ecm))
		return PTR_ERR(f_ecm);

	ret = usb_add_function(c, f_ecm);
	if (ret < 0)
		goto err_func_ecm;

	/* implicit port_num is zero */
	f_acm_multi = usb_get_function(fi_acm);
	if (IS_ERR_OR_NULL(f_acm_multi)) {
		ret = PTR_ERR(f_acm_multi);
		goto err_func_acm;
	}

	ret = usb_add_function(c, f_acm_multi);
	if (ret)
		goto err_conf;

	return 0;
err_run:
	usb_put_function(f_msg_multi);
err_fsg:
	usb_remove_function(c, f_acm_multi);
err_conf:
	usb_put_function(f_acm_multi);
err_func_acm:
	usb_remove_function(c, f_ecm);
err_func_ecm:
	usb_put_function(f_ecm);
	return ret;
}

#endif


static int multi_do_config(struct usb_configuration *c)
{
	int ret = 0;
#ifdef CONFIG_USB_G_MULTI_CDC
	ret = cdc_do_config(c);
	if(ret != 0)
	{
		printk("cdc_do_config fail\n");
		return ret;
	}
#endif

#ifdef USB_ETH_RNDIS
	ret = rndis_acm_do_config(c);
	if(ret != 0)
	{
		printk("rndis_do_config fail\n");
		return ret;
	}
#endif

	return ret;
}


/****************************** Gadget Bind ******************************/

static int  multi_bind(struct usb_composite_dev *cdev, int multi_enable)
{
	struct usb_gadget *gadget = cdev->gadget;
#ifdef CONFIG_USB_G_MULTI_CDC
	struct f_ecm_opts *ecm_opts;
#endif
#ifdef USB_ETH_RNDIS
	struct f_rndis_opts *rndis_opts;
#endif

	int status;

	if (!can_support_ecm(cdev->gadget)) {
		dev_err(&gadget->dev, "controller '%s' not usable\n",
			gadget->name);
		return -EINVAL;
	}
	
	g_multi_rndis_enable = multi_enable & 0x01 ? 1:0;
	g_multi_serial_enable = multi_enable & 0x02 ? 1:0;
	
	printk("rndis_enable:%d, serial_enable:%d\n",g_multi_rndis_enable,g_multi_serial_enable);

#ifdef CONFIG_USB_G_MULTI_CDC
	fi_ecm = usb_get_function_instance("ecm");
	if (IS_ERR_OR_NULL(fi_ecm))
		return PTR_ERR(fi_ecm);

	ecm_opts = container_of(fi_ecm, struct f_ecm_opts, func_inst);

	gether_set_qmult(ecm_opts->net, qmult);
	if (!gether_set_host_addr(ecm_opts->net, host_addr))
		pr_info("using host ethernet address: %s", host_addr);
	if (!gether_set_dev_addr(ecm_opts->net, dev_addr))
		pr_info("using self ethernet address: %s", dev_addr);
#endif

#ifdef USB_ETH_RNDIS
	if(g_multi_rndis_enable)
	{
		fi_rndis = usb_get_function_instance("rndis");
		if (IS_ERR_OR_NULL(fi_rndis)) {
			status = PTR_ERR(fi_rndis);
			goto fail;
		}

		rndis_opts = container_of(fi_rndis, struct f_rndis_opts, func_inst);

		gether_set_qmult(rndis_opts->net, qmult);
		if (!gether_set_host_addr(rndis_opts->net, host_addr))
			pr_info("using host ethernet address: %s", host_addr);
		if (!gether_set_dev_addr(rndis_opts->net, dev_addr))
			pr_info("using self ethernet address: %s", dev_addr);
	}
#endif

#if (defined CONFIG_USB_G_MULTI_CDC && defined USB_ETH_RNDIS)
	/*
	 * If both ecm and rndis are selected then:
	 *	1) rndis borrows the net interface from ecm
	 *	2) since the interface is shared it must not be bound
	 *	twice - in ecm's _and_ rndis' binds, so do it here.
	 */
	gether_set_gadget(ecm_opts->net, cdev->gadget);
	status = gether_register_netdev(ecm_opts->net);
	if (status)
		goto fail0;

	rndis_borrow_net(fi_rndis, ecm_opts->net);
	ecm_opts->bound = true;
#endif

#ifdef CONFIG_USB_F_ACM
	if(g_multi_serial_enable)
	{
		/* set up serial link layer */
		fi_acm = usb_get_function_instance("acm");
		if (IS_ERR_OR_NULL(fi_acm)) {
			status = PTR_ERR(fi_acm);
			goto fail0;
		}
	}
#endif

	//if(usb_gadget_connect(cdev->gadget) != 0)
	//	printk("usb_gadget_connect error!\n");

	return 0;


	/* error recovery */
#ifdef CONFIG_USB_F_ACM
//fail_string_ids:
fail0:
#endif
#ifdef USB_ETH_RNDIS
	if(!IS_ERR_OR_NULL(fi_rndis))
		usb_put_function_instance(fi_rndis);
fail:
#endif
#ifdef CONFIG_USB_G_MULTI_CDC
	usb_put_function_instance(fi_ecm);
#endif
	return status;
}

static int multi_unbind(struct usb_composite_dev *cdev)
{
#ifdef CONFIG_USB_G_MULTI_CDC
	usb_put_function(f_msg_multi);
#endif
#if 0//def USB_ETH_RNDIS
	if(!IS_ERR_OR_NULL(fi_rndis))
		usb_put_function(f_msg_rndis);
	
#endif

#ifdef CONFIG_USB_G_MULTI_CDC
	usb_put_function(f_acm_multi);
#endif

#ifdef CONFIG_USB_F_ACM
	if(!IS_ERR_OR_NULL(fi_acm))
	{
		usb_put_function(f_acm);
		usb_put_function_instance(fi_acm);
	}
#endif

#ifdef USB_ETH_RNDIS
	if(!IS_ERR_OR_NULL(fi_rndis))
	{
		usb_put_function(f_rndis);
		usb_put_function_instance(fi_rndis);
	}
#endif
#ifdef CONFIG_USB_G_MULTI_CDC
	usb_put_function(f_ecm);
	usb_put_function_instance(fi_ecm);
#endif

	return 0;
}