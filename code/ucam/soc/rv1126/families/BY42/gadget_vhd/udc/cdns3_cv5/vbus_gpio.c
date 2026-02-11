#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/byteorder.h>
#include <linux/io.h>
//#include <asm/system.h>
#include <asm/unaligned.h>
#include <linux/kthread.h>

#include <asm-generic/gpio.h>

#include "vbus_gpio.h"
#include "usb_ctrl.h"
	
static struct task_struct *vbus_task;

static unsigned gpio_num = 0xFFFF;
module_param(gpio_num, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_num, "vbus_gpio_num");

static unsigned long delay_ms = 800;
module_param(delay_ms, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(delay_ms, "delay time ms");

extern int vbus_usb_soft_connect(void);
// extern void vbus_usb_soft_disconnect(void);
// extern void dwc3_gadget_disconnect_vbus(void);
extern uint32_t get_usb_soft_connect_status(void);
extern void diable_disconn_irq(void);

static int vbus_thread(void *__unused)
{
	int data,data2;

	data = 0;
	data2 = 0;
	diable_disconn_irq();

	
	printk("vbus_thread running\n");
	while(!kthread_should_stop())
	{
		data = __gpio_get_value(gpio_num);
		if(data2 != data && get_usb_soft_connect_status())
		{			
			msleep(20);
			if(data == __gpio_get_value(gpio_num))
			{	
				if(data == 1)
				{
					printk("usb connect\n");
					vbus_usb_soft_disconnect();
					msleep(delay_ms);
					vbus_usb_soft_connect();
				}
				else if(data == 0)
				{
					printk("usb disconnect\n");
					vbus_usb_soft_disconnect();
					// dwc3_gadget_disconnect_vbus();
				}
			}
		}
		data2 = data;	
		msleep(100);
	}
	printk("vbus_thread leave\n");
	return 0;
}

int vbus_gpio_init(void) 
{
	int ret;
	
	if(gpio_num == 0xFFFF)
	{
		printk("please set gpio_num\n");
		return -1;
	}	

	ret = gpio_request(gpio_num, "vbus_gpio");
	if(ret != 0)
	{
		printk("gpio_request error! ret:%d\n",ret);
		return ret;
	}

	ret = gpio_direction_input(gpio_num);
	if(ret != 0)
	{
		printk("gpio_direction_input error! ret:%d\n",ret);
		return ret;
	}
	

	vbus_task = kthread_run(vbus_thread, NULL, "vbus_task");
	if (IS_ERR(vbus_task))
		return -1;

	return 0;	
}  

void vbus_gpio_exit(void)
{
	if (!IS_ERR(vbus_task))
		kthread_stop(vbus_task);
}

module_init(vbus_gpio_init);
module_exit(vbus_gpio_exit);

MODULE_DESCRIPTION("VBUS GPIO controlled driver");
MODULE_AUTHOR("QHC");
MODULE_LICENSE("GPL");

