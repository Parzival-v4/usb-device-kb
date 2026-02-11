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

#include "vbus_gpio.h"
//#include "hiusb3_hi3519.h"
//#include "usb_chips.h"


#define DATA_OFFSET		(0x3FC)
#define DIR_OFFSET		(0x400)
#define IS_OFFSET		(0x404)
#define IBE_OFFSET		(0x408)
#define IEV_OFFSET		(0x40c)
#define IE_OFFSET		(0x410)
#define RIS_OFFSET		(0x414)
#define MIS_OFFSET		(0x418)
#define IC_OFFSET		(0x41c)


#define SYS_WRITEL(Addr, Value) ((*(volatile unsigned int *)(Addr)) = (Value))
#define SYS_READ(Addr)          (*((volatile int *)(Addr)))

static void  *reg_mutex_base=0;
static void  *reg_gpio_base=0;
	
static struct task_struct *vbus_task;


#ifdef HARDWARE_VX61U
#define GPIO_REG_MUTEX_BASE		(0x12040000+0x009C)
#define GPIO_REG_GPIO_BASE		0x12145000
#define GPIO_DATA_VBUS			0x20//GPIO5_5
#else
#define GPIO_REG_MUTEX_BASE		0
#define GPIO_REG_GPIO_BASE		0
#define GPIO_DATA_VBUS			0
#endif

static unsigned long gpio_mutex_base = 0x1204009C;
module_param(gpio_mutex_base, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_mutex_base, "gpio_reg_mutex_base");

static unsigned long gpio_mutex_value = 0x0;
module_param(gpio_mutex_value, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_mutex_value, "gpio_mutex_value");

static unsigned long gpio_base = 0x12145000;
module_param(gpio_base, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_base, "gpio_reg_base");


static unsigned long gpio_data = 0x20;
module_param(gpio_data, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_data, "vbus_gpio_data");

static unsigned long delay_ms = 800;
module_param(delay_ms, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(delay_ms, "delay time ms");

extern void vbus_usb_soft_connect(void);
extern void vbus_usb_soft_disconnect(void);
extern void dwc3_gadget_disconnect_vbus(void);
extern uint32_t get_usb_soft_connect_status(void);
extern void diable_disconn_irq(void);


static int  vbus_reg_init(void)
{
	if(gpio_mutex_base == 0 || gpio_base == 0)
		return -1;

    reg_mutex_base = (void*)ioremap(gpio_mutex_base, 0x10000);
    if (NULL == reg_mutex_base)
    {   
        goto out;
    }
    reg_gpio_base = (void*)ioremap(gpio_base, 0x1000);//GPIO5
    if (NULL == reg_gpio_base)
    {   
        goto out;
    }

    return 0;
out:
    printk("vbus_reg_init error");
    return -1;
}

static int vbus_thread(void *__unused)
{
	unsigned char data,data2;
    unsigned char t;

	data2 = 0;
    t = SYS_READ((unsigned long)reg_gpio_base+DIR_OFFSET);
    SYS_WRITEL((unsigned long)reg_gpio_base+DIR_OFFSET, t&(~gpio_data));

	diable_disconn_irq();

	
	printk("vbus_thread running\n");
	while(!kthread_should_stop())
	{
		
		data = SYS_READ((unsigned long)reg_gpio_base+DATA_OFFSET) & gpio_data;
		if(data2 != data  && get_usb_soft_connect_status())
		{			
			msleep(20);
            if(data == (SYS_READ((unsigned long)reg_gpio_base+DATA_OFFSET) & gpio_data))
            {	
				if(data)
				{
					printk("usb connect\n");
					vbus_usb_soft_disconnect();
					msleep(delay_ms);
					vbus_usb_soft_connect();
				}
				else
				{
					printk("usb disconnect\n");
					vbus_usb_soft_disconnect();
					dwc3_gadget_disconnect_vbus();
				}
            }
		}
		else
		{
#if 0			
#endif
		}
		data2 = data;
		
		msleep(100);
	}
	printk("vbus_thread leave\n");
	return 0;
}

int vbus_gpio_init(void) 
{
	//int reg;
	
	printk("vbus_init\n");
	
    if(vbus_reg_init() != 0)
		return -1;
	
    SYS_WRITEL(reg_mutex_base, gpio_mutex_value);

	vbus_task = kthread_run(vbus_thread, NULL, "pcd_vbus_task");
	if (IS_ERR(vbus_task))
		return -1;


	return 0;	
}  

void vbus_gpio_exit(void)
{

	if (!IS_ERR(vbus_task))
		kthread_stop(vbus_task);
	
	if(reg_mutex_base)
		iounmap(reg_mutex_base);
	
	if(reg_gpio_base)
		iounmap(reg_gpio_base);

	//return 0;
}



module_init(vbus_gpio_init);
module_exit(vbus_gpio_exit);

MODULE_DESCRIPTION("VBUS GPIO controlled driver");
MODULE_AUTHOR("QHC");
MODULE_LICENSE("GPL");

