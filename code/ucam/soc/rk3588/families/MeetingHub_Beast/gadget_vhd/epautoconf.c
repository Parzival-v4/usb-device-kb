/*
 * epautoconf.c -- endpoint autoconfiguration for usb gadget drivers
 *
 * Copyright (C) 2004 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>

#include <linux/ctype.h>
#include <linux/string.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"

/*
 * This should work with endpoints from controller drivers sharing the
 * same endpoint naming convention.  By example:
 *
 *	- ep1, ep2, ... address is fixed, not direction or type
 *	- ep1in, ep2out, ... address and direction are fixed, not type
 *	- ep1-bulk, ep2-bulk, ... address and type are fixed, not direction
 *	- ep1in-bulk, ep2out-iso, ... all three are fixed
 *	- ep-* ... no functionality restrictions
 *
 * Type suffixes are "-bulk", "-iso", or "-int".  Numbers are decimal.
 * Less common restrictions are implied by gadget_is_*().
 *
 * NOTE:  each endpoint is unidirectional, as specified by its USB
 * descriptor; and isn't specific to a configuration or altsetting.
 */
static int
ep_matches (
	struct usb_gadget		*gadget,
	struct usb_ep			*ep,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp
)
{
	u8		type;
	u16		max;

	int		num_req_streams = 0;

	/* endpoint already claimed? */
	if (NULL != ep->driver_data || ep->claimed)
		return 0;

	/* only support ep0 for portable CONTROL traffic */
	type = usb_endpoint_type(desc);
	max = 0x7ff & usb_endpoint_maxp(desc);
	if (!max)
	{
		switch (type)
		{
			case USB_ENDPOINT_XFER_ISOC:
			case USB_ENDPOINT_XFER_BULK:
				max = 1024;
			break;
			default:
				break;
		}
	}

	if (usb_endpoint_dir_in(desc) && !ep->caps.dir_in)
		return 0;
	if (usb_endpoint_dir_out(desc) && !ep->caps.dir_out)
		return 0;

	if (max > ep->maxpacket_limit)
		return 0;

	/* "high bandwidth" works only at high speed */
	if (!gadget_is_dualspeed(gadget) && usb_endpoint_maxp_mult(desc) > 1)
		return 0;

	switch (type) {
	case USB_ENDPOINT_XFER_CONTROL:
		/* only support ep0 for portable CONTROL traffic */
		return 0;
	case USB_ENDPOINT_XFER_ISOC:
		if (!ep->caps.type_iso)
			return 0;
		/* ISO:  limit 1023 bytes full speed, 1024 high/super speed */
		if (!gadget_is_dualspeed(gadget) && max > 1023)
			return 0;
		break;
	case USB_ENDPOINT_XFER_BULK:
		if (!ep->caps.type_bulk)
			return 0;
		if (ep_comp && gadget_is_superspeed(gadget)) {
			/* Get the number of required streams from the
			 * EP companion descriptor and see if the EP
			 * matches it
			 */
			num_req_streams = ep_comp->bmAttributes & 0x1f;
			if (num_req_streams > ep->max_streams)
				return 0;
		}
		break;
	case USB_ENDPOINT_XFER_INT:
		/* Bulk endpoints handle interrupt transfers,
		 * except the toggle-quirky iso-synch kind
		 */
		if (!ep->caps.type_int && !ep->caps.type_bulk)
			return 0;
		/* INT:  limit 64 bytes full speed, 1024 high/super speed */
		if (!gadget_is_dualspeed(gadget) && max > 64)
			return 0;
		break;
	}

	
	return 1;
}


static struct usb_ep *found_ep_match(
	struct usb_gadget		*gadget,	
	struct usb_ep			*ep,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp)
{
	u8		type;

	type = usb_endpoint_type(desc);

	/* MATCH!! */

	/*
	 * If the protocol driver hasn't yet decided on wMaxPacketSize
	 * and wants to know the maximum possible, provide the info.
	 */
	if (desc->wMaxPacketSize == 0)
		desc->wMaxPacketSize = cpu_to_le16(ep->maxpacket_limit);


	/* report address */
	desc->bEndpointAddress &= USB_DIR_IN;
	if (isdigit (ep->name [2])) {
		u8	num = simple_strtoul (&ep->name [2], NULL, 10);
		desc->bEndpointAddress |= num;
	} else if (desc->bEndpointAddress & USB_DIR_IN) {
		if (++gadget->in_epnum > 15)
			return 0;
		desc->bEndpointAddress = USB_DIR_IN | gadget->in_epnum;
	} else {
		if (++gadget->out_epnum > 15)
			return 0;
		desc->bEndpointAddress |= gadget->out_epnum;
	}

	/* report (variable) full speed bulk maxpacket */
	if ((USB_ENDPOINT_XFER_BULK == type) && !ep_comp) {
		
		//BULK mode:usb2.0 max 512,usb3.0 max 1024 qhc 2016-07-21
		if(desc->wMaxPacketSize > 1024)
			desc->wMaxPacketSize = cpu_to_le16(1024);
	}

	ep->address = desc->bEndpointAddress;
	ep->claimed = true;
	ep->desc = NULL;
	ep->comp_desc = NULL;
	printk("usb_ep_autoconfig : Address:0x%02x MaxPacketSize:%d <--> %s maxpacket_limit:%d\n", 
		desc->bEndpointAddress, desc->wMaxPacketSize, ep->name, ep->maxpacket_limit);
	return ep;	
}


/**
 * usb_ep_autoconfig_ss() - choose an endpoint matching the ep
 * descriptor and ep companion descriptor
 * @gadget: The device to which the endpoint must belong.
 * @desc: Endpoint descriptor, with endpoint direction and transfer mode
 *    initialized.  For periodic transfers, the maximum packet
 *    size must also be initialized.  This is modified on
 *    success.
 * @ep_comp: Endpoint companion descriptor, with the required
 *    number of streams. Will be modified when the chosen EP
 *    supports a different number of streams.
 *
 * This routine replaces the usb_ep_autoconfig when needed
 * superspeed enhancments. If such enhancemnets are required,
 * the FD should call usb_ep_autoconfig_ss directly and provide
 * the additional ep_comp parameter.
 *
 * By choosing an endpoint to use with the specified descriptor,
 * this routine simplifies writing gadget drivers that work with
 * multiple USB device controllers.  The endpoint would be
 * passed later to usb_ep_enable(), along with some descriptor.
 *
 * That second descriptor won't always be the same as the first one.
 * For example, isochronous endpoints can be autoconfigured for high
 * bandwidth, and then used in several lower bandwidth altsettings.
 * Also, high and full speed descriptors will be different.
 *
 * Be sure to examine and test the results of autoconfiguration
 * on your hardware.  This code may not make the best choices
 * about how to use the USB controller, and it can't know all
 * the restrictions that may apply. Some combinations of driver
 * and hardware won't be able to autoconfigure.
 *
 * On success, this returns an un-claimed usb_ep, and modifies the endpoint
 * descriptor bEndpointAddress.  For bulk endpoints, the wMaxPacket value
 * is initialized as if the endpoint were used at full speed and
 * the bmAttribute field in the ep companion descriptor is
 * updated with the assigned number of streams if it is
 * different from the original value. To prevent the endpoint
 * from being returned by a later autoconfig call, claim it by
 * assigning ep->driver_data to some non-null value.
 *
 * On failure, this returns a null endpoint descriptor.
 */
struct usb_ep *usb_ep_autoconfig_ss(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp
)
{
	struct usb_ep	*ep;
	struct usb_ep	*minpacket_ep = NULL;
	unsigned minpacket = 0xFFFF;

	printk("EP DIR:%s MaxPacketSize:%d\n", 
			desc->bEndpointAddress & USB_DIR_IN ? "IN" : "OUT", 
			0x7ff & usb_endpoint_maxp(desc));

	/* Second, look at endpoints until an unclaimed one looks usable */
	list_for_each_entry (ep, &gadget->ep_list, ep_list) 
	{
		if (ep_matches(gadget, ep, desc, ep_comp))
		{
			if(minpacket > ep->maxpacket_limit)
			{
				minpacket = ep->maxpacket_limit;
				minpacket_ep = ep;
				printk("found:%s maxpacket_limit:%d\n", ep->name, ep->maxpacket_limit);
			}
		}	
	}

	if(minpacket_ep)
	{
		ep = minpacket_ep;
		goto found_ep;
	}

	printk("usb_ep_autoconfig fail\n");

	/* Fail */
	return NULL;
found_ep:
	return found_ep_match(gadget, ep, desc, ep_comp);
}
EXPORT_SYMBOL_GPL(usb_ep_autoconfig_ss);

/**
 * usb_ep_autoconfig() - choose an endpoint matching the
 * descriptor
 * @gadget: The device to which the endpoint must belong.
 * @desc: Endpoint descriptor, with endpoint direction and transfer mode
 *	initialized.  For periodic transfers, the maximum packet
 *	size must also be initialized.  This is modified on success.
 *
 * By choosing an endpoint to use with the specified descriptor, this
 * routine simplifies writing gadget drivers that work with multiple
 * USB device controllers.  The endpoint would be passed later to
 * usb_ep_enable(), along with some descriptor.
 *
 * That second descriptor won't always be the same as the first one.
 * For example, isochronous endpoints can be autoconfigured for high
 * bandwidth, and then used in several lower bandwidth altsettings.
 * Also, high and full speed descriptors will be different.
 *
 * Be sure to examine and test the results of autoconfiguration on your
 * hardware.  This code may not make the best choices about how to use the
 * USB controller, and it can't know all the restrictions that may apply.
 * Some combinations of driver and hardware won't be able to autoconfigure.
 *
 * On success, this returns an un-claimed usb_ep, and modifies the endpoint
 * descriptor bEndpointAddress.  For bulk endpoints, the wMaxPacket value
 * is initialized as if the endpoint were used at full speed.  To prevent
 * the endpoint from being returned by a later autoconfig call, claim it
 * by assigning ep->driver_data to some non-null value.
 *
 * On failure, this returns a null endpoint descriptor.
 */
struct usb_ep *usb_ep_autoconfig(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc
)
{
	return usb_ep_autoconfig_ss(gadget, desc, NULL);
}
EXPORT_SYMBOL_GPL(usb_ep_autoconfig);
/**
 * usb_ep_autoconfig_ss_backwards() - choose an endpoint matching the ep
 * descriptor and ep companion descriptor.
 *
 * unlike the usb_ep_autoconfig_ss(), this interface is auto configured
 * in reverse.
 */
struct usb_ep *usb_ep_autoconfig_ss_backwards(
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc,
	struct usb_ss_ep_comp_descriptor *ep_comp
)
{
	struct usb_ep	*ep;
	struct usb_ep	*minpacket_ep = NULL;
	unsigned minpacket = 0xFFFF;

	printk("EP DIR:%s MaxPacketSize:%d\n", 
			desc->bEndpointAddress & USB_DIR_IN ? "IN" : "OUT", 
			0x7ff & usb_endpoint_maxp(desc));

	/* Second, look at endpoints until an unclaimed one looks usable */
	list_for_each_entry_reverse (ep, &gadget->ep_list, ep_list) 
	{
		if (ep_matches(gadget, ep, desc, ep_comp))
		{
			if(minpacket > ep->maxpacket_limit)
			{
				minpacket = ep->maxpacket_limit;
				minpacket_ep = ep;
				printk("found:%s maxpacket_limit:%d\n", ep->name, ep->maxpacket_limit);
			}
		}	
	}

	if(minpacket_ep)
	{
		ep = minpacket_ep;
		goto found_ep;
	}

	printk("usb_ep_autoconfig fail\n");

	/* Fail */
	return NULL;
found_ep:
	return found_ep_match(gadget, ep, desc, ep_comp);
}
EXPORT_SYMBOL_GPL(usb_ep_autoconfig_ss_backwards);

/**
 * usb_ep_autoconfig_reset - reset endpoint autoconfig state
 * @gadget: device for which autoconfig state will be reset
 *
 * Use this for devices where one configuration may need to assign
 * endpoint resources very differently from the next one.  It clears
 * state such as ep->driver_data and the record of assigned endpoints
 * used by usb_ep_autoconfig().
 */
void usb_ep_autoconfig_reset (struct usb_gadget *gadget)
{
	struct usb_ep	*ep;

	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		ep->driver_data = NULL;
		ep->claimed = false;
		ep->enabled = 0;
	}
	gadget->in_epnum = 0;
	gadget->out_epnum = 0;
}
EXPORT_SYMBOL_GPL(usb_ep_autoconfig_reset);


void get_ep_max_num (struct usb_gadget *gadget, uint32_t *in_ep_max_num, uint32_t *out_ep_max_num)
{
	struct usb_ep	*ep;
	uint32_t in_num = 0;
	uint32_t out_num = 0;

	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		//epin1
		//if(strcmp(ep->name, "in") >= 0)

#if (defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211)
		if(ep->caps.dir_in)
			in_num++;
		if(ep->caps.dir_out)
			out_num++;
#else		

		if(ep->name[3] == 'i' && ep->name[4] == 'n')
			in_num++;
		else
			out_num++;
#endif
		printk("%s ep_list:%s maxpacket_limit:%d\n", gadget->name, ep->name, ep->maxpacket_limit);	
	}

	*in_ep_max_num = in_num;
	*out_ep_max_num = out_num;
}
EXPORT_SYMBOL_GPL(get_ep_max_num);
