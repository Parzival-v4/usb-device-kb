#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
//#include <mach/io.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>

#include "core.h"
#include "cdnsp-gadget.h"
#include "drd.h"
#include "usb_ctrl.h"

extern int cdnsp_gadget_resume(struct cdns *cdns, bool hibernated);
extern int cdnsp_gadget_suspend(struct cdns *cdns, bool do_wakeup);
extern int cdnsp_port_connect(int is_on);
extern void cdnsp_stop(struct cdnsp_device *pdev);


struct cdns *g_cdns = NULL;

volatile static uint32_t soft_connect_disable_ctrl	= 0;
volatile static uint32_t usb_soft_connect_status	= 0;

void cdnsp_regs_soft_connect_init(struct cdns *cdns)
{
	if(g_cdns)
		printk("%s error, repeat setting dwc3 regs\n", __func__);
	g_cdns = cdns;
}
EXPORT_SYMBOL(cdnsp_regs_soft_connect_init);

void dwc3_mdelay(uint32_t msecs)
{
	if (in_interrupt())
		mdelay(msecs);
	else
		msleep(msecs);
}

void set_usb_soft_connect_ctrl(uint32_t value)
{
	soft_connect_disable_ctrl = value; 	
}
EXPORT_SYMBOL(set_usb_soft_connect_ctrl);

uint32_t get_usb_soft_connect_ctrl(void)
{
	return soft_connect_disable_ctrl; 	
}
EXPORT_SYMBOL(get_usb_soft_connect_ctrl);


int usb_soft_connect(void)
{
	int ret;
	ret = cdnsp_gadget_resume(g_cdns, false);
	usb_soft_connect_status = 1;
	return ret;
}
EXPORT_SYMBOL(usb_soft_connect);

int vbus_usb_soft_connect(void)
{
	cdnsp_port_connect(1);
	return 0;
}
EXPORT_SYMBOL(vbus_usb_soft_connect);

int usb_soft_disconnect(void)
{
	usb_soft_connect_status = 0;
	return cdnsp_gadget_suspend(g_cdns, false);
}
EXPORT_SYMBOL(usb_soft_disconnect);

int vbus_usb_soft_disconnect(void)
{
	cdnsp_port_connect(0);
	return 0;
}
EXPORT_SYMBOL(vbus_usb_soft_disconnect);

//USB软断开重连
int usb_soft_connect_reset(void)
{
	usb_soft_disconnect();
	dwc3_mdelay(10);
	usb_soft_connect();

	return 0;
}
EXPORT_SYMBOL(usb_soft_connect_reset);

uint32_t get_usb_soft_connect_status(void)
{
	return usb_soft_connect_status;
}
EXPORT_SYMBOL(get_usb_soft_connect_status);

uint32_t get_usb_soft_connect_state(void)
{
	return ((struct cdnsp_device*)g_cdns->gadget_dev)->gadget.state==0;
}
EXPORT_SYMBOL(get_usb_soft_connect_state);

void diable_disconn_irq(void)
{
	cdnsp_stop((struct cdnsp_device*)g_cdns->gadget_dev);
}
EXPORT_SYMBOL(diable_disconn_irq);

MODULE_LICENSE("GPL v2");
