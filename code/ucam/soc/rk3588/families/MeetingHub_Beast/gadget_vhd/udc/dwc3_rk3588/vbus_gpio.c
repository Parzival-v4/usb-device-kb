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
#include "core.h"
	
static struct task_struct *vbus_task;

static unsigned gpio_num = 0xFFFF;
module_param(gpio_num, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_num, "vbus_gpio_num");

static unsigned long delay_ms = 800;
module_param(delay_ms, ulong, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(delay_ms, "delay time ms");

static bool gpio_connect_val = 1;
module_param(gpio_connect_val, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpio_connect_val, "0 (when connected gpio value is 0) / 1 (when connected gpio value is 1)");

extern void vbus_usb_soft_connect(void);
extern void vbus_usb_soft_disconnect(void);
extern void dwc3_gadget_disconnect_vbus(void);
extern uint32_t get_usb_soft_connect_status(void);
extern void diable_disconn_irq(void);
extern int get_usb_link_state(void);

#define DEBOUNCE_COUNT 3    // 需要3次连续相同的采样才认为状态稳定
#define SAMPLE_DELAY_MS 20  // 采样间隔20ms，足够过滤抖动
#define RECONNECT_MIN_INTERVAL_SEC 5

static inline int is_gpio_connected(void)
{
    return gpio_connect_val ? __gpio_get_value(gpio_num) : !__gpio_get_value(gpio_num);
}

static int vbus_thread(void *__unused)
{
    int data, data2;
    int stable_count;
    int new_state;
    int i;
    static unsigned long last_reconnect_jiffies;

    data = 0;
    data2 = 0;
    diable_disconn_irq();

    printk("vbus_thread running\n");
    while(!kthread_should_stop())
    {
        data = is_gpio_connected();
        if(data2 != data && get_usb_soft_connect_status())
        {
            // 进行连续3次采样确认，每次间隔20ms
            new_state = data;
            stable_count = 1;

            for(i = 0; i < DEBOUNCE_COUNT - 1; i++) {
                msleep(SAMPLE_DELAY_MS);
                if(new_state == is_gpio_connected()) {
                    stable_count++;
                } else {
                    break;  // 如果出现不同的状态，直接退出
                }
            }

            // 只有连续3次采样都相同，才认为是真实的状态变化
            if(stable_count >= DEBOUNCE_COUNT) {
                if(new_state == 1)
                {
                    printk("usb connect\n");
                    vbus_usb_soft_disconnect();
                    msleep(delay_ms);
                    vbus_usb_soft_connect();
                }
                else if(new_state == 0)
                {
                    printk("usb disconnect\n");
                    vbus_usb_soft_disconnect();
                    dwc3_gadget_disconnect_vbus();
                }
                data2 = new_state;
            }
        } else {
            /* 如果保持 connect 状态，周期性检测 link state 是否为 CMPLY，满足间隔后软重连 */
            if (data == 1 && time_after(jiffies, last_reconnect_jiffies + RECONNECT_MIN_INTERVAL_SEC * HZ)) {
                int link = get_usb_link_state();
                if (link == DWC3_LINK_STATE_CMPLY) {
                    printk("usb link CMPLY detected, perform soft reconnect\n");
                    /* 记录尝试时间，保证两次尝试间隔 > RECONNECT_MIN_INTERVAL_SEC */
                    last_reconnect_jiffies = jiffies;

                    vbus_usb_soft_disconnect();
                    dwc3_gadget_disconnect_vbus();
                    msleep(delay_ms);
                    vbus_usb_soft_connect();
                }
            }
        }

        msleep(50);  // 保持50ms的主循环周期
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

