/*
 * ncm.inl -- helpers for embedding CDC-NCM into another composite gadget
 *
 * NOTE:
 * - This is NOT a standalone g_ncm gadget driver.
 * - Do NOT allocate strings/device descriptors here (webcam.c owns them).
 * - Only manage function instance/function lifecycle and optional gether opts.
 */

#include <linux/usb/composite.h>

#include "u_ether.h"
#include "u_ncm.h"

static struct usb_function_instance *fi_ncm;
static struct usb_function *f_ncm;

static int ncm_bind(struct usb_composite_dev *cdev)
{
	struct f_ncm_opts *ncm_opts;

	fi_ncm = usb_get_function_instance("ncm");
	if (IS_ERR(fi_ncm))
		return PTR_ERR(fi_ncm);

	ncm_opts = container_of(fi_ncm, struct f_ncm_opts, func_inst);

	/* Re-use gether module parameters when USB_MULTI_SUPPORT is enabled. */
#ifdef USB_MULTI_SUPPORT
	gether_set_qmult(ncm_opts->net, qmult);
	if (!gether_set_host_addr(ncm_opts->net, host_addr))
		pr_info("using host ethernet address: %s", host_addr);
	if (!gether_set_dev_addr(ncm_opts->net, dev_addr))
		pr_info("using self ethernet address: %s", dev_addr);
#endif

	return 0;
}

static int ncm_do_config(struct usb_configuration *c)
{
	int status;

	if (IS_ERR_OR_NULL(fi_ncm))
		return -EINVAL;

	f_ncm = usb_get_function(fi_ncm);
	if (IS_ERR(f_ncm))
		return PTR_ERR(f_ncm);

	status = usb_add_function(c, f_ncm);
	if (status < 0) {
		usb_put_function(f_ncm);
		f_ncm = NULL;
	}

	return status;
}

static void ncm_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR_OR_NULL(f_ncm)) {
		usb_put_function(f_ncm);
		f_ncm = NULL;
	}
	if (!IS_ERR_OR_NULL(fi_ncm)) {
		usb_put_function_instance(fi_ncm);
		fi_ncm = NULL;
	}
}
