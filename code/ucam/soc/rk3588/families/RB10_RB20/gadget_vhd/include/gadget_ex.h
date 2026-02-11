/*
 * <linux/usb/gadget.h>
 *
 * We call the USB code inside a Linux-based peripheral device a "gadget"
 * driver, except for the hardware-specific bus glue.  One USB host can
 * master many USB gadgets, but the gadgets are only slaved to one host.
 *
 *
 * (C) Copyright 2002-2004 by David Brownell
 * All Rights Reserved.
 *
 * This software is licensed under the GNU GPL version 2.
 */

#ifndef __GADGET_EX_H
#define __GADGET_EX_H

#include <linux/version.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>


#if 0
static inline int usb_ep_enable_ex(struct usb_ep *ep)
{
	int ret = 0;

	if(ep == NULL)
		return -1;
	
	//if(ep->driver_data == NULL)
	//	return -1;

	if (ep->enabled)
		goto out;

	ret = ep->ops->enable(ep, ep->desc);
	if (ret)
		goto out;

	ep->enabled = true;
	printk("%s enable\n",ep->name);

out:
	//trace_usb_ep_enable(ep, ret);

	return ret;
}


static inline int usb_ep_disable_ex(struct usb_ep *ep)
{
	int ret = 0;

	if(ep == NULL)
		return -1;
	
	//if(ep->driver_data == NULL)
	//	return -1;

	if (!ep->enabled)
		goto out;

	ep->enabled = false;
	ret = ep->ops->disable(ep);
	if (ret)
	{
		ep->enabled = true;
		goto out;
	}
		

	ep->enabled = false;
	printk("%s disable\n",ep->name);

out:
	//trace_usb_ep_disable(ep, ret);

	return ret;
}


static inline int usb_ep_queue_ex(struct usb_ep *ep,
			       struct usb_request *req, gfp_t gfp_flags)
{
	int ret = 0;

	if(ep == NULL || req == NULL){
		ret = -ESHUTDOWN;
		goto out;
	}

	if (WARN_ON_ONCE(!ep->enabled && ep->address)) {
		ret = -ESHUTDOWN;
		goto out;
	}

	ret = ep->ops->queue(ep, req, gfp_flags);

out:
	//trace_usb_ep_queue(ep, req, ret);

	return ret;
}

#define usb_ep_enable usb_ep_enable_ex
#define usb_ep_disable usb_ep_disable_ex
#define usb_ep_queue usb_ep_queue_ex


extern int usb_assign_descriptors(struct usb_function *f,
		struct usb_descriptor_header **fs,
		struct usb_descriptor_header **hs,
		struct usb_descriptor_header **ss);
#endif		

#ifdef USB_CHIP_DWC3
extern void set_usb_soft_connect_ctrl(uint32_t value);
extern uint32_t get_usb_soft_connect_ctrl(void);
extern uint32_t get_usb_soft_connect_state(void);
#endif

extern struct usb_ep *usb_ep_autoconfig_ss_backwards(struct usb_gadget *,
			struct usb_endpoint_descriptor *,
			struct usb_ss_ep_comp_descriptor *);


#endif /* __GADGET_EX_H */
