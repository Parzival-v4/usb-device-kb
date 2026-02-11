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
#include "gadget.h"
#include "io.h"

enum usb_connect_state
{
	USB_STATE_DISCONNECT,
	USB_STATE_CONNECT,
	USB_STATE_CONNECT_RESET,
};


void __iomem		*dwc3_regs = NULL;


typedef union dctl_data {
		/** raw register data */
	uint32_t d32;//32位
		/** register bits */
	struct {//低位在前[0]...[32]
		unsigned reserved:1;//长度为1
		unsigned test_control:4;
		unsigned ulstchngraq:4;
		unsigned accept_u1_enable:1;
		unsigned initiate_u1_enable:1;
		unsigned accept_u2_enable:1;
		unsigned initiate_u2_enable:1;
		unsigned reserved2:3;
		unsigned controller_save_state :1;
		unsigned controller_restore_state:1;
		unsigned l1_hibernation_en:1;
		unsigned keepconnect:1;
		unsigned reserved3:3;
		unsigned appl1res:1;
		unsigned hird_threshold:5;
		unsigned reserved4:1;
		unsigned soft_core_reset:1;
		unsigned run_stop:1;
	} b;
} dctl_data_t;

volatile static uint32_t soft_connect_disable_ctrl	= 0;
volatile static uint32_t usb_soft_connect_status	= 0;

void dwc3_regs_soft_connect_init(void __iomem	*regs)
{
	dwc3_regs = regs;
}

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
	unsigned long flags;
	dctl_data_t dctl_reg;
	
	if(soft_connect_disable_ctrl)
		return -1;

	if(dwc3_regs == NULL)
	{
		printk("dwc3_regs == NULL!\n");
		return -1;
	}
		
	
	local_irq_save(flags);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	
	//2.如果软件要在软断开或者检测到断开事件后重新启动连接，
	//则需要往此比特写 1 之前将 DCTL[8：5]设置成 5； 
	dctl_reg.b.ulstchngraq = 5;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	dctl_reg.b.run_stop = 1;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	usb_soft_connect_status = 1;
	dwc3_mdelay(5);	
	//printk("%s +%d %s:usb connect OK\n", __FILE__,__LINE__,__func__);

	local_irq_restore(flags);	

	return 0;	
}
EXPORT_SYMBOL(usb_soft_connect);

int vbus_usb_soft_connect(void)
{
	unsigned long flags;
	dctl_data_t dctl_reg;
	
	if(soft_connect_disable_ctrl)
		return -1;

	if(dwc3_regs == NULL)
		return -1;
	
	local_irq_save(flags);

	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	
	//2.如果软件要在软断开或者检测到断开事件后重新启动连接，
	//则需要往此比特写 1 之前将 DCTL[8：5]设置成 5； 
	dctl_reg.b.ulstchngraq = 5;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	dctl_reg.b.run_stop = 1;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	dwc3_mdelay(5);	
	//printk("%s +%d %s:usb connect OK\n", __FILE__,__LINE__,__func__);

	local_irq_restore(flags);	

	return 0;	
}
EXPORT_SYMBOL(vbus_usb_soft_connect);

int usb_soft_disconnect(void)
{
	unsigned long flags;
	dctl_data_t dctl_reg;
	
	if(soft_connect_disable_ctrl)
		return -1;
	
	if(dwc3_regs == NULL)
		return -1;
	
	local_irq_save(flags);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	dctl_reg.b.run_stop = 0;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	usb_soft_connect_status = 0;
	dwc3_mdelay(5);
		
	//printk("%s +%d %s:usb disconnect OK\n", __FILE__,__LINE__,__func__);

	local_irq_restore(flags);

	return 0;
}
EXPORT_SYMBOL(usb_soft_disconnect);

int vbus_usb_soft_disconnect(void)
{
	unsigned long flags;
	dctl_data_t dctl_reg;
	
	if(soft_connect_disable_ctrl)
		return -1;
	
	if(dwc3_regs == NULL)
		return -1;
	
	local_irq_save(flags);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	dctl_reg.b.run_stop = 0;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	dwc3_mdelay(5);
		
	//printk("%s +%d %s:usb disconnect OK\n", __FILE__,__LINE__,__func__);

	local_irq_restore(flags);

	return 0;
}
EXPORT_SYMBOL(vbus_usb_soft_disconnect);

//USB软断开重连
int usb_soft_connect_reset(void)
{
	unsigned long flags;
	dctl_data_t dctl_reg;
	
	if(soft_connect_disable_ctrl)
		return -1;

	if(dwc3_regs == NULL)
		return -1;
	
	local_irq_save(flags);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	dctl_reg.b.run_stop = 0;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	dwc3_mdelay(10);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	
	//2.如果软件要在软断开或者检测到断开事件后重新启动连接，
	//则需要往此比特写 1 之前将 DCTL[8：5]设置成 5； 
	dctl_reg.b.ulstchngraq = 5;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	dctl_reg.b.run_stop = 1;
	dwc3_writel(dwc3_regs, DWC3_DCTL, dctl_reg.d32);
	
	usb_soft_connect_status = 1;
		
	//printk("%s +%d %s:usb connect reset OK\n", __FILE__,__LINE__,__func__);

	local_irq_restore(flags);

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
	unsigned long flags;
	dctl_data_t dctl_reg;

	if(dwc3_regs == NULL)
		return 0;
	
	local_irq_save(flags);

	dctl_reg.d32 = dwc3_readl(dwc3_regs, DWC3_DCTL);
	if(dctl_reg.b.run_stop)
	{
		return USB_STATE_CONNECT;
	}
	else
	{
		return USB_STATE_DISCONNECT;
	}			
}
EXPORT_SYMBOL(get_usb_soft_connect_state);
