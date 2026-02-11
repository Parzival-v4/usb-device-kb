/*
 * f_hid.c -- USB HID function driver
 *
 * Copyright (C) 2010 Fabien Chouteau <fabien.chouteau@barco.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hid.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/usb/g_hid.h>
#include <linux/time.h>

#include "u_f.h"
#include "uvc.h"


//#define UVCIOC_USB_CONNECT_STATE	_IOW('U', 2, int)
//#define UVCIOC_GET_USB_SOFT_CONNECT_STATE	_IOR('U', 6, uint32_t)

#define HID_REPORT_LENGTH_MAX       256

static unsigned int g_hid_consumer_report_length_max = HID_REPORT_LENGTH_MAX;
module_param(g_hid_consumer_report_length_max, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_hid_consumer_report_length_max, "HID consumer ctrl data buffer");

static unsigned int g_hid_consumer_ep_in_report_len = 64;
module_param(g_hid_consumer_ep_in_report_len, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(g_hid_consumer_ep_in_report_len, "HID consumer report EP IN length");

static char g_hid_ucq_string_arr[32] ={0};
static char *g_hid_ucq_string = "UCQ01001000001000";
module_param(g_hid_ucq_string, charp, 0644);
MODULE_PARM_DESC(g_hid_ucq_string,"UCQ");

static int major[DEV_HID_CONSUMER_NUM_MAX], minors[DEV_HID_CONSUMER_NUM_MAX];

typedef struct skype_hid_report{
	struct usb_ctrlrequest ctrl;
 	unsigned char data[32];
}skype_hid_report_s;

typedef enum 
{
  HID_LIST_TYPE_REQ = 1,
  HID_LIST_TYPE_CTRL = 2,
}hidg_list_type;



static struct class *hidg_class;
struct usb_ctrlrequest hid_ctrl = {0};
//static u8 hid_data[8] = {0};

//skype_hid_report_s skype_report = {0};


typedef struct phone_status{
	int call_reminder_status;   //�����??D
	int calling_status;		  //o??D?D
	int is_on_the_phone;		  //����?��?D
	int hold_the_phone;		  //����?�����3
	int press_call_key_flag;    //??���??2|o??��
	int pc_call_flag;            //PC??o??D?��???����y
	int pc_hang_flag;            //PC??1��??
	int suspend_call_flag;      //����?��?D1��?e���䨬?
	int resume_call_flag;       //����?��???��
	int mute_on_off; 			 //PC?2��?���䨬?
	int mute_flag; 				  //PC?2��?���䨬???��?����??
}phone_status_s;

phone_status_s phone_st = {0};
#define UVCIOC_GET_HID_REPORT			_IOR('U', 7, phone_status_s)
#define UVCIOC_SET_HID_REPORT			_IOW('U', 7, phone_status_s)

struct hid_request_data
{
	__s32 length;
    //为了兼容V1.0.98之前的hid_consumer，实际data最大size为g_hid_consumer_report_length_max
	__u8 data[HID_REPORT_LENGTH_MAX];   
};

#define HID_IOC_SEND_EP0_DATA            		_IOW('H', 1, struct hid_request_data)
#define HID_IOC_GET_EP0_DATA            		_IOR('H', 2, struct hid_request_data)
#define HID_IOC_SEND_EP_DATA            		_IOW('H', 3, struct hid_request_data)

//static int answer_call_flag = 0;
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
struct timespec64 set_Start;
#else
struct timeval set_Start;
#endif


#define TELEPHONY_REPORT_ID 0x02
#define KEYBOARD_REPORT_ID	0x03
#define CONSUMER_REPORT_ID	0x01


static unsigned char  report_desc[] = {
#if 1

#if 0
    0x06, 0x99, 0xFF,              // USAGE_PAGE (Vendor Defined Page 1)
	0x09, 0x00,                    // USAGE (Vendor Defined)
	0xA1, 0x01,                    // COLLECTION (Application)
	
    0x09, 0x03,                    // USAGE (Usage ID 3)   
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
	0x85, 0x9a,                    // REPORT_ID (154)   
	0x75, 0x08,                    // REPORT_SIZE (8)
	0x95, 0x40,                    // REPORT_COUNT (64)
    0xB1, 0x02,                    // FEATURE (Data, Variable, Absolute)

    0x09, 0x04,                    // USAGE (Usage ID 4)
    0x15, 0x00,                    // LOGICAL_MINIMUM (0)
    0x25, 0x01,                    // LOGICAL_MAXIMUM (1)
	0x85, 0x9b,                    // REPORT_ID (155)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x95, 0x40,                    // REPORT_COUNT (64)
    0x81, 0x02,                    // INPUT (Data, Variable, Absolute)

    0xc0,                          //   END_COLLECTION
#endif


#if 1
	//Telephony
    0x05, 0x0b,                    // USAGE_PAGE (Telephony Devices)
    0x09, 0x05,                    // USAGE (Headset)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, TELEPHONY_REPORT_ID,     //   REPORT_ID (2)
    0x05, 0x0b,                    //   USAGE_PAGE (Telephony Devices)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x09, 0x20,                    //   USAGE (Hook Switch)
    0x09, 0x97,                    //   USAGE (Line Busy Tone)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x22,                    //   INPUT (Data,Var,Abs,NPrf) 
	0x09, 0x2f,                    //   USAGE (Phone Mute)
	0x09, 0x21,                    //   USAGE (Flash) 
	0x09, 0x70,                    //   USAGE (Voice Mail)
	0x09, 0x50,                    //   USAGE (Speed Dial)
	0x75, 0x01,                    //     REPORT_SIZE (1) 
	0x95, 0x04,                    //   REPORT_COUNT (4) 
	0x81, 0x06,                    //   INPUT (Data,Var,Rel) 
	0x09, 0x06,                    //   USAGE (Telephony Key Pad) 
	0xa1, 0x02,                    //   COLLECTION (Logical) 
	0x19, 0xb0,                    //     USAGE_MINIMUM (Button Phone Key 0)
	0x29, 0xbb,					   //     USAGE_MAXIMUM (Button Phone Key Pound)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0c,                    //     LOGICAL_MAXIMUM (12)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x40,                    //     INPUT (Data,Ary,Abs,Null)
    0xc0,                          // END_COLLECTION
		
    0x09, 0x07,                    //   USAGE (Programmable Button)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x05, 0x09,                    //   USAGE_PAGE (Button)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x09, 0x17,                    //   USAGE (Off Hook)
    0x09, 0x09,                    //   USAGE (Mute)
    0x09, 0x18,                    //   USAGE (Ring)
	0x09, 0x20,                    //   USAGE (Hook Switch)
    0x09, 0x21,                    //   USAGE (Microphone)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x91, 0x22,                    //   OUTPUT (Data,Var,Abs,NPrf)
    0x05, 0x0b,                    //   USAGE_PAGE (Telephony Devices)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x09, 0x9e,                    //   USAGE (Ringer)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x91, 0x22,                    //   OUTPUT (Data,Var,Abs,NPrf)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x0a,                    //   REPORT_COUNT (10)
    0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs)
    0xc0,                          // END_COLLECTION
#endif

#if 0
	//keyboard
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
	0x85, KEYBOARD_REPORT_ID,	   //   REPORT_ID (3)
	//0x05, 0x08,                    //   USAGE_PAGE (LEDs)
	//0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
	//0x29, 0x03,                    //   USAGE_MAXIMUM (Scroll Lock)		
	//0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    //0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    //0x75, 0x01,                    //   REPORT_SIZE (1)
	//0x95, 0x03,					   //   REPORT_COUNT (3) 
	//0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
	//0x95, 0x05,                    //   REPORT_COUNT (5) 
	//0x91, 0x01,                    //   OUTPUT (Cnst,Ary,Abs) 
	0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x95, 0x08,                    //   REPORT_COUNT (8) 
	0x81, 0x02,                    //   INPUT (Data,Var,Abs) 
	0x75, 0x08,                    //   REPORT_SIZE (8) 
	0x95, 0x01,                    //   REPORT_COUNT (1) 
	0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
	0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated)) 
	0x29, 0x91, 				   //   USAGE_MAXIMUM (??)
	0x26, 0xff, 0x00,  			   //   LOGICAL_MAXIMUM (255)
	0x95, 0x06,                    //   REPORT_COUNT (6) 
	0x81, 0x00,                    //   INPUT (Data,Ary,Abs) 
	0xc0,                          // END_COLLECTION
#endif

#if 1
	//Consumer
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, CONSUMER_REPORT_ID,      //   REPORT_ID (1)
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x09, 0xea,                    //   USAGE (Volume Down)
    0x09, 0xe9,                    //   USAGE (Volume Up)
    0x09, 0xe2,                    //   USAGE (Mute)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x0d,                    //   REPORT_COUNT (13)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0xc0,                           //   END_COLLECTION	
#endif


#endif	
};


static unsigned char  report_desc_volume[] = {
	//Consumer
    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,                    // USAGE (Consumer Control)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, CONSUMER_REPORT_ID,      //   REPORT_ID (1)
    0x05, 0x0c,                    //   USAGE_PAGE (Consumer Devices)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x09, 0xea,                    //   USAGE (Volume Down)
    0x09, 0xe9,                    //   USAGE (Volume Up)
    0x09, 0xe2,                    //   USAGE (Mute)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    // REPORT_SIZE (1)
    0x95, 0x0d,                    //   REPORT_COUNT (13)
    0x81, 0x01,                    //   INPUT (Cnst,Ary,Abs)
    0xc0,                           //   END_COLLECTION		
};

static struct hidg_func_descriptor hid_des= {
	.subclass = 0,
	.protocol = 0,
	.report_length = 64,
	//.report_desc_length= 0xD1,
	.report_desc_length= 0,
	//.report_desc_length= 0x14a,
#if 0
	.report_desc= {
	#if 0
		0x05,
		0x0c, 0x09, 0x01, 0xa1, 0x01, 0x85, 0x01, 0x15, 0x00, 0x25, 0x01, 0x09, 0xe9, 0x09, 0xea, 0x75,
		0x01, 0x95, 0x02, 0x81, 0x06, 0x95, 0x06, 0x81, 0x01, 0x85, 0x02, 0x05, 0x0c, 0x09, 0x00, 0x95,
		0x10, 0x81, 0x02, 0x85, 0x03, 0x09, 0x00, 0x95, 0x08, 0x91, 0x02, 0x85, 0x04, 0x09, 0x00, 0x75,
		0x08, 0x95, 0x24, 0x91, 0x02, 0x85, 0x05, 0x09, 0x00, 0x95, 0x20, 0x81, 0x02, 0x85, 0x06, 0x09,
		0x00, 0x95, 0x24, 0x91, 0x02, 0x85, 0x07, 0x09, 0x00, 0x95, 0x20, 0x81, 0x02, 0xc0, 0x05, 0x0b,
		0x09, 0x05, 0xa1, 0x01, 0x85, 0x08, 0x15, 0x00, 0x25, 0x01, 0x09, 0x20, 0x75, 0x01, 0x95, 0x01,
		0x81, 0x22, 0x09, 0x97, 0x75, 0x01, 0x95, 0x01, 0x81, 0x22, 0x09, 0x2b, 0x75, 0x01, 0x95, 0x01,
		0x81, 0x22, 0x09, 0x2a, 0x75, 0x01, 0x95, 0x01, 0x81, 0x22, 0x09, 0x2f, 0x75, 0x01, 0x95, 0x01,
		0x81, 0x06, 0x09, 0x21, 0x75, 0x01, 0x95, 0x01, 0x81, 0x06, 0x09, 0x24, 0x75, 0x01, 0x95, 0x01,
		0x81, 0x06, 0x09, 0x07, 0x05, 0x09, 0x09, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02, 0x05, 0x08,
		0x85, 0x09, 0x09, 0x09, 0x95, 0x01, 0x91, 0x22, 0x95, 0x07, 0x91, 0x01, 0x85, 0x17, 0x09, 0x17,
		0x95, 0x01, 0x91, 0x22, 0x95, 0x07, 0x91, 0x01, 0x85, 0x18, 0x09, 0x18, 0x95, 0x01, 0x91, 0x22,
		0x95, 0x07, 0x91, 0x01, 0x85, 0x20, 0x09, 0x20, 0x95, 0x01, 0x91, 0x22, 0x95, 0x07, 0x91, 0x01,
		0xc0,
	#else
	0x05, 0x0b, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x02, 0x05, 0x0b, 0x15, 0x00, 0x25, 0x01, 0x09, 0x20,
	0x09, 0x97, 0x75, 0x01, 0x95, 0x02, 0x81, 0x22, 0x09, 0x2f, 0x09, 0x21, 0x09, 0x70, 0x09, 0x50,
	0x75, 0x01, 0x95, 0x04, 0x81, 0x06, 0x09, 0x06, 0xa1, 0x02, 0x19, 0xb0, 0x29, 0xbb, 0x15, 0x00,
	0x25, 0x0c, 0x75, 0x04, 0x95, 0x01, 0x81, 0x40, 0xc0, 0x09, 0x07, 0x15, 0x00, 0x25, 0x01, 0x05,
	0x09, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02, 0x75, 0x01, 0x95, 0x05, 0x81, 0x01, 0x05, 0x08, 0x15,
	0x00, 0x25, 0x01, 0x09, 0x17, 0x09, 0x09, 0x09, 0x18, 0x09, 0x20, 0x09, 0x21, 0x75, 0x01, 0x95,
	0x05, 0x91, 0x22, 0x05, 0x0b, 0x15, 0x00, 0x25, 0x01, 0x09, 0x9e, 0x75, 0x01, 0x95, 0x01, 0x91,
	0x22, 0x75, 0x01, 0x95, 0x0a, 0x91, 0x01, 0xc0, 0x05, 0x0c, 0x09, 0x01, 0xa1, 0x01, 0x85, 0x01, 
	0x05, 0x0c, 0x15, 0x00, 0x25, 0x01, 0x09, 0xea, 0x09, 0xe9, 0x09, 0xe2, 0x75, 0x01, 0x95, 0x03, 
	0x81, 0x02, 0x75, 0x01, 0x95, 0x0d, 0x81, 0x01, 0xc0,

	0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x03, 0x05, 0x08, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 
	0x25, 0x01, 0x75, 0x01, 0x95, 0x03, 0x91, 0x02, 0x95, 0x05, 0x91, 0x01, 0x05, 0x07, 0x19, 0xe0, 
	0x29, 0xe7, 0x95, 0x08, 0x81, 0x02, 0x75, 0x08, 0x95, 0x01, 0x81, 0x01, 0x19, 0x00, 0x29, 0x91, 
	0x26, 0xff, 0x00, 0x95, 0x06, 0x81, 0x00, 0xc0,

	#endif
	}
#endif	
};

struct f_hidg_req_list {
	void					*p;
	unsigned int			pos;
	unsigned int			type;
	struct list_head 		list;
};

#define HID_REPORT_DESC_CAN_BE_SET      0
#define HID_REPORT_DESC_CANNOT_BE_SET   1
#define HID_REPORT_DESC_IS_SETTING      2

struct f_hidg {
	/* configuration */
	unsigned char			bInterfaceSubClass;
	unsigned char			bInterfaceProtocol;
	unsigned short			report_desc_length;
	char					*report_desc;
	unsigned short			report_length;
    unsigned char           report_desc_set_status;

	/* recv report */
	struct list_head		completed_out_req;
	spinlock_t				spinlock;
	wait_queue_head_t		read_queue;
	unsigned int			qlen;

	/* send report */
	spinlock_t				write_spinlock;
	bool					write_pending;
	wait_queue_head_t		write_queue;
	struct usb_request		*req;

	//int					minor;
	struct cdev				cdev;
	struct usb_function		func;

	struct usb_ep			*in_ep;
	struct usb_ep			*out_ep;

	//usb control ep 
	unsigned int 			control_intf;
	struct usb_ep 			*control_ep;
	struct usb_request 		*control_req;
	void 					*control_buf;
	
	unsigned char 			hid_idle;
	
	/* Events */
	unsigned int event_length;
	unsigned int event_setup_out : 1;
	
	struct completion  get_ep0_data_done_completion;
	struct completion  send_ep0_data_done_completion;
	
	struct completion  ep_in_data_done_completion;
	struct completion  ep_out_data_done_completion;
};

void ghid_cleanup_consumer(u8 bConfigurationValue);

static inline struct f_hidg *func_to_hidg(struct usb_function *f)
{
	return container_of(f, struct f_hidg, func);
}

/*-------------------------------------------------------------------------*/
/*                           Static descriptors                            */

static struct usb_interface_descriptor hidg_interface_desc = {
	.bLength		= sizeof hidg_interface_desc,
	.bDescriptorType	= USB_DT_INTERFACE,
	
/* .bInterfaceNumber	= DYNAMIC */
	.bAlternateSetting	= 0,
	.bNumEndpoints		= 1,
	.bInterfaceClass	= USB_CLASS_HID,
	.iInterface			= 0, 
	
	/* .bInterfaceSubClass	= DYNAMIC */
	/* .bInterfaceProtocol	= DYNAMIC */
	/* .iInterface		= DYNAMIC */
};

static struct hid_descriptor hidg_desc = {
	.bLength			= sizeof hidg_desc,
	.bDescriptorType		= HID_DT_HID,
	.bcdHID				= 0x0111,
	.bCountryCode			= 0x00,
	.bNumDescriptors		= 0x1,
	/*.desc[0].bDescriptorType	= DYNAMIC */
	/*.desc[0].wDescriptorLenght	= DYNAMIC */
};


/* Super-Speed Support */

static struct usb_endpoint_descriptor hidg_ss_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x04, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

static struct usb_ss_ep_comp_descriptor hidg_ss_in_ep_comp_desc = {
	.bLength		= sizeof(hidg_ss_in_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	.bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};

#if 0
static struct usb_endpoint_descriptor hidg_ss_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};
#endif

static struct usb_ss_ep_comp_descriptor hidg_ss_out_ep_comp_desc = {
	.bLength		= sizeof(hidg_ss_out_ep_comp_desc),
	.bDescriptorType	= USB_DT_SS_ENDPOINT_COMP,
	/* The bMaxBurst, bmAttributes and wBytesPerInterval values will be
	 * initialized from module parameters.
	 */
	 .bMaxBurst = 0,
	.bmAttributes = 0,
	//.wBytesPerInterval = 0; /*DYNAMIC*/
};

/* High-Speed Support */

static struct usb_endpoint_descriptor hidg_hs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x04, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};

#if 0
static struct usb_endpoint_descriptor hidg_hs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				      * HID gadget configuration?
				      * (struct hidg_func_descriptor)
				      */
};
#endif

/* Full-Speed Support */

static struct usb_endpoint_descriptor hidg_fs_in_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x02, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};

#if 0
static struct usb_endpoint_descriptor hidg_fs_out_ep_desc = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_INT,
	/*.wMaxPacketSize	= DYNAMIC */
	.bInterval		= 0x01, /* FIXME: Add this field in the
				       * HID gadget configuration?
				       * (struct hidg_func_descriptor)
				       */
};
#endif

static struct usb_descriptor_header *hidg_ss_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,

	(struct usb_descriptor_header *)&hidg_ss_in_ep_desc,
	(struct usb_descriptor_header *)&hidg_ss_in_ep_comp_desc,

	NULL,
};

static struct usb_descriptor_header *hidg_hs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,

	(struct usb_descriptor_header *)&hidg_hs_in_ep_desc,
	NULL,
};

static struct usb_descriptor_header *hidg_fs_descriptors[] = {
	(struct usb_descriptor_header *)&hidg_interface_desc,
	(struct usb_descriptor_header *)&hidg_desc,
	(struct usb_descriptor_header *)&hidg_fs_in_ep_desc,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                              Char Device                                */

extern int get_usb_link_state(void);

static void hidg_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg *hidg = (struct f_hidg *) req->context;
	
	if (hidg->event_setup_out) 
	{
		//printk("dfug_ep0_complete out: length = %d\n", req->length);
		hidg->event_setup_out = 0;
		//hid_return_status(hidg);
		complete_all(&hidg->get_ep0_data_done_completion);	
	}
	else
	{
		//printk("dfug_ep0_complete in: length = %d\n", req->length);
		//hid_return_status(hidg);
		complete_all(&hidg->send_ep0_data_done_completion);	
	}
}

static void hidg_ep_in_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg *hidg = (struct f_hidg *) req->context;
	

	complete_all(&hidg->ep_in_data_done_completion);	
}

static ssize_t f_hidg_read(struct file *file, char __user *buffer,
			size_t count, loff_t *ptr)
{
	struct f_hidg *hidg = file->private_data;
	struct f_hidg_req_list *list;
	struct usb_ctrlrequest	*ctrl;
	struct usb_request *req;
	unsigned long flags;
	int ret;

	if (!count)
		return 0;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	//if (!hidg->out_ep->enabled)
	//	return -EFAULT;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_WRITE, buffer, count))
		return -EFAULT;
#endif

	spin_lock_irqsave(&hidg->spinlock, flags);

#define READ_COND (!list_empty(&hidg->completed_out_req))

	/* wait for at least one buffer to complete */
	while (!READ_COND) {
		spin_unlock_irqrestore(&hidg->spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(hidg->read_queue, READ_COND))
			return -ERESTARTSYS;

		spin_lock_irqsave(&hidg->spinlock, flags);
	}

	
	/* pick the first one */
	list = list_first_entry(&hidg->completed_out_req,
				struct f_hidg_req_list, list);
	
	if(list->type == HID_LIST_TYPE_CTRL)
	{
		ctrl = (struct usb_ctrlrequest	*)list->p;
		count = min_t(unsigned int, count, sizeof(struct usb_ctrlrequest));
#if 0
		printk("HID setup request %02x %02x value %04x index %04x %04x\n",
			ctrl->bRequestType, ctrl->bRequest, le16_to_cpu(ctrl->wValue),
			le16_to_cpu(ctrl->wIndex), le16_to_cpu(ctrl->wLength));
#endif
		
		
		spin_unlock_irqrestore(&hidg->spinlock, flags);
		
		/* copy to user outside spinlock */
		count -= copy_to_user(buffer, (uint8_t *)ctrl + list->pos, count);
		list->pos += count;
		
		/*
		 * if this request is completely handled and transfered to
		 * userspace, remove its entry from the list and requeue it
		 * again. Otherwise, we will revisit it again upon the next
		 * call, taking into account its current read position.
		 */
		if (list->pos == sizeof(struct usb_ctrlrequest)) 
		{
			spin_lock_irqsave(&hidg->spinlock, flags);
			list_del(&list->list);
			kfree(list);
			spin_unlock_irqrestore(&hidg->spinlock, flags);
		}
	}
	else if(list->type == HID_LIST_TYPE_REQ)
	{
		req = (struct usb_request *)list->p;
		count = min_t(unsigned int, count, req->actual - list->pos);
		spin_unlock_irqrestore(&hidg->spinlock, flags);

		/* copy to user outside spinlock */
		count -= copy_to_user(buffer, req->buf + list->pos, count);
		list->pos += count;

		/*
		 * if this request is completely handled and transfered to
		 * userspace, remove its entry from the list and requeue it
		 * again. Otherwise, we will revisit it again upon the next
		 * call, taking into account its current read position.
		 */
		if (list->pos == req->actual) {
			spin_lock_irqsave(&hidg->spinlock, flags);
			list_del(&list->list);
			kfree(list);
			spin_unlock_irqrestore(&hidg->spinlock, flags);

			req->length = hidg->report_length;
			ret = usb_ep_queue(hidg->out_ep, req, GFP_KERNEL);
			if (ret < 0)
				return ret;
		}	
	}else
	{
		spin_lock_irqsave(&hidg->spinlock, flags);
		list_del(&list->list);
		kfree(list);
		spin_unlock_irqrestore(&hidg->spinlock, flags);
	}


	return count;
}


static void f_hidg_req_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg *hidg = (struct f_hidg *)req->context;
	unsigned long flags;

	if (req->status != 0) {
		ERROR(hidg->func.config->cdev,
			"End Point Request ERROR: %d\n", req->status);
	}

	spin_lock_irqsave(&hidg->write_spinlock, flags);
	hidg->write_pending = 0;
	spin_unlock_irqrestore(&hidg->write_spinlock, flags);
	wake_up(&hidg->write_queue);
}

static ssize_t f_hidg_write(struct file *file, const char __user *buffer,
			    size_t count, loff_t *offp)
{
	struct f_hidg *hidg  = file->private_data;
	ssize_t status = -ENOMEM;
	unsigned long flags;
	
	ktime_t timeout;
	
#if (defined CHIP_RV1126) || (LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0))	
	timeout = 1*1000*1000*1000;
#else
	timeout.tv64 = 1*1000*1000*1000;
#endif

	if (hidg->in_ep == NULL)
		return -EFAULT;
	
	if (hidg->in_ep->enabled == false)
		return -EFAULT;

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,4,0)
	if (!access_ok(buffer, count))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_READ, buffer, count))
		return -EFAULT;
#endif

#if 1
	if(get_usb_link_state()!= 0)
	{
		if(hidg->in_ep)
		{
			usb_ep_disable(hidg->in_ep);
			//hidg->in_ep->driver_data = NULL;
		}
		status = -EFAULT;
		goto release_write_pending;
	}
#endif

	spin_lock_irqsave(&hidg->write_spinlock, flags);

#define WRITE_COND (!hidg->write_pending)

	/* write queue */
	while (!WRITE_COND) {
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

#if 0
		if (wait_event_interruptible_exclusive(
				hidg->write_queue, WRITE_COND))
			return -ERESTARTSYS;
#else
		if(wait_event_interruptible_hrtimeout(hidg->write_queue, WRITE_COND, timeout))
		{
			wake_up(&hidg->write_queue);
			return -ERESTARTSYS;
			// status = -ERESTARTSYS;
			// goto release_write_pending;
		}
#endif

		spin_lock_irqsave(&hidg->write_spinlock, flags);
	}

	count  = min_t(unsigned, count, hidg->report_length);
	status = copy_from_user(hidg->req->buf, buffer, count);

	if (status != 0) {
		ERROR(hidg->func.config->cdev,
			"copy_from_user error\n");
		spin_unlock_irqrestore(&hidg->write_spinlock, flags);
		return -EINVAL;
	}

	hidg->req->status	= 0;
	hidg->req->zero 	= 0;
	hidg->req->length	= count;
	hidg->req->complete = f_hidg_req_complete;
	hidg->req->context	= hidg;
	hidg->write_pending = 1;

	status = usb_ep_queue(hidg->in_ep, hidg->req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		hidg->write_pending = 0;
		wake_up(&hidg->write_queue);
	} else {
		status = count;
	}

	spin_unlock_irqrestore(&hidg->write_spinlock, flags);

	return status;

release_write_pending:
	spin_lock_irqsave(&hidg->write_spinlock, flags);
	hidg->write_pending = 0;
	spin_unlock_irqrestore(&hidg->write_spinlock, flags);

	wake_up(&hidg->write_queue);

	return status;
}


static unsigned int f_hidg_poll(struct file *file, poll_table *wait)
{
	struct f_hidg	*hidg  = file->private_data;
	unsigned int	ret = 0;

	poll_wait(file, &hidg->read_queue, wait);
	poll_wait(file, &hidg->write_queue, wait);

	if (WRITE_COND)
		ret |= POLLOUT | POLLWRNORM;

	if (READ_COND)
		ret |= POLLIN | POLLRDNORM;

	return ret;
}

#undef WRITE_COND
#undef READ_COND

static int f_hidg_release(struct inode *inode, struct file *fd)
{
	fd->private_data = NULL;
	return 0;
}

static int f_hidg_open(struct inode *inode, struct file *fd)
{
	struct f_hidg *hidg =
		container_of(inode->i_cdev, struct f_hidg, cdev);

	fd->private_data = hidg;

	return 0;
}

/*-------------------------------------------------------------------------*/
/*                                usb_function                             */

static int
hidg_usb_send_ep0_data(struct file* file, struct hid_request_data *data)
{
	struct f_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	ssize_t status = -ENOMEM;
	__s32 length;
	//int ret;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		return usb_ep_set_halt(cdev->gadget->ep0);
	}	
	
	if (length < 0)
		return usb_ep_set_halt(cdev->gadget->ep0);

	
	length = min_t(unsigned int, g_hid_consumer_report_length_max, length);
	req->zero = length < hidg->event_length;
	req->complete = hidg_ep0_complete;
	req->length = length;
	if(length > 0)
	{
		status = copy_from_user(req->buf, data->data, req->length);
		if (status != 0) {
			ERROR(cdev,"copy_from_user error\n");
			return usb_ep_set_halt(cdev->gadget->ep0);
		}
	}
	else 
		return usb_ep_set_halt(cdev->gadget->ep0);
	
	//printk("dfug_usb_send_ep0_data : %d\n",req->length);
	
	hidg->send_ep0_data_done_completion.done = 0;
	
	status = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		usb_ep_set_halt(cdev->gadget->ep0);
		complete_all(&hidg->send_ep0_data_done_completion);	
	} else {
		status = length;
	}
	

	wait_for_completion_interruptible(&hidg->send_ep0_data_done_completion);
	
	return status;
}

static int
hidg_usb_get_ep0_data(struct file* file, struct hid_request_data *data)
{
	struct f_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->control_req;
	ssize_t status = -ENOMEM;
	__s32 length;
	int ret;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		usb_ep_set_halt(cdev->gadget->ep0);
		return -1;
	}	
	
	if (length < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		return -1;
	}
	
	length = min_t(unsigned int, g_hid_consumer_report_length_max, length);
	req->complete = hidg_ep0_complete;
	req->length = length;

	//printk("dfug_usb_get_ep0_data : %d\n",req->length);
	
	hidg->get_ep0_data_done_completion.done = 0;
	
	ret = usb_ep_queue(cdev->gadget->ep0, req, GFP_KERNEL);
	if (ret < 0)
	{
		usb_ep_set_halt(cdev->gadget->ep0);
		complete_all(&hidg->get_ep0_data_done_completion);
		return ret;
	}
	wait_for_completion_interruptible(&hidg->get_ep0_data_done_completion);
	
	length = min_t(unsigned int, req->length, length);
	status = copy_to_user(data->data, req->buf, length);
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");	
		
		return -1;
	}	
	
	return length;
}

static int
hidg_usb_ep_in_data(struct file* file, struct hid_request_data *data)
{
	struct f_hidg	*hidg  = file->private_data;
	struct usb_composite_dev *cdev = hidg->func.config->cdev;
	struct usb_request *req = hidg->req;
	ssize_t status = -ENOMEM;
	__s32 length;
	int ret = 0;

	if (hidg->in_ep == NULL)
		return -EFAULT;
	
	if (hidg->in_ep->enabled == false)
		return -EFAULT;

	if(get_usb_link_state()!= 0)
		return -EFAULT;

	status = copy_from_user(&length, &data->length,sizeof(__s32));
	if (status != 0) {
		ERROR(cdev,"copy_from_user error\n");
		return usb_ep_set_halt(hidg->in_ep);
	}	
	
	if (length < 0)
		return usb_ep_set_halt(hidg->in_ep);
	
	length = min_t(unsigned int, hidg->report_length, length);

	req->status   = 0;
	req->zero     = 0;
	req->length   = length;
	req->complete = hidg_ep_in_complete;
	req->context  = hidg;
	//hidg->write_pending = 1;
	
	if(length > 0)
	{
		status = copy_from_user(hidg->req->buf, data->data, hidg->req->length);
		if (status != 0) {
			ERROR(cdev,"copy_from_user error\n");
			return usb_ep_set_halt(hidg->in_ep);
		}
	}
	
	hidg->ep_in_data_done_completion.done = 0;
	
	status = usb_ep_queue(hidg->in_ep, hidg->req, GFP_ATOMIC);
	if (status < 0) {
		ERROR(hidg->func.config->cdev,
			"usb_ep_queue error on int endpoint %zd\n", status);
		usb_ep_set_halt(hidg->in_ep);
		complete_all(&hidg->send_ep0_data_done_completion);	
	} else {
		status = length;
	}

	wait_for_completion_interruptible(&hidg->ep_in_data_done_completion);
	
	return ret;
}


static inline struct usb_request *hidg_alloc_ep_req(struct usb_ep *ep,
						    unsigned length)
{
	return alloc_ep_req(ep, length, length);
}

#if 0
static void hidg_set_report_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_hidg *hidg = (struct f_hidg *) req->context;
	struct f_hidg_req_list *req_list;
	unsigned long flags;

	req_list = kzalloc(sizeof(*req_list), GFP_ATOMIC);
	if (!req_list)
		return;

	req_list->type = HID_LIST_TYPE_REQ;
	req_list->p = req;

	spin_lock_irqsave(&hidg->spinlock, flags);
	list_add_tail(&req_list->list, &hidg->completed_out_req);
	spin_unlock_irqrestore(&hidg->spinlock, flags);

	wake_up(&hidg->read_queue);
}
#endif

#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
int is_time_out(struct timespec64 tvStart, struct timespec64 tvEnd)
{
	uint64_t  time_s,time_ns,time;
	
	if(tvEnd.tv_sec >= tvStart.tv_sec){
		time_s = tvEnd.tv_sec - tvStart.tv_sec;
		time_ns = tvEnd.tv_nsec - tvStart.tv_nsec;
		//time = time_s * 1000000 + time_ns/1000;
		do_div(time_ns, 1000);
		time = time_s * 1000000 + time_ns;
		
		//time = time/1000;
		do_div(time, 1000);
		if(time >= 600){
			return true;
		}
		else
			return false;
	}else{
		return false;
	}
}
#else
int is_time_out(struct timeval tvStart, struct timeval tvEnd)
{
	int64_t  time_s,time_us,time;
	
	if(tvEnd.tv_sec >= tvStart.tv_sec){
		time_s = tvEnd.tv_sec - tvStart.tv_sec;
		time_us = tvEnd.tv_usec - tvStart.tv_usec;
		time = time_s * 1000000 + time_us;
		
		time = time/1000;
		if(time >= 600){
			return true;
		}
		else
			return false;
	}else{
		return false;
	}
}
#endif

#if 0
static void f_hid_complete(struct usb_ep *ep, struct usb_request *req)
{
	//struct f_hidg	*hidg = req->context;
	int status = req->status;
	//u32 data = 0;
	static struct timeval  tvStart,tvEnd;
	//char call_buff[2] = {0x08,0x03};
	//char hang_buff[2] = {0x08,0x00};
	switch (status) {
		case 0: 			/* normal completion? */
			memcpy(skype_report.data, req->buf, req->length);
					
			if(skype_report.ctrl.wValue == 0x0218 && skype_report.ctrl.wIndex == 0x0003){
				if(phone_st.hold_the_phone == 1){
					break;
				}
				if(skype_report.data[0] == 0x18 && skype_report.data[1] == 0x01){
					do_gettimeofday(&tvStart);
					if(phone_st.call_reminder_status == 1){
						phone_st.call_reminder_status = 0;
						answer_call_flag = 1;
						phone_st.is_on_the_phone = 1;
						phone_st.hold_the_phone = 0; 
					}else if(phone_st.is_on_the_phone == 0){
						phone_st.call_reminder_status = 1;
						phone_st.is_on_the_phone = 0;
						phone_st.hold_the_phone = 0; 
					}
				}
				if(skype_report.data[0] == 0x18 && skype_report.data[1] == 0x00){
					//if(call_come){
					//	phone_st.call_reminder_status = 0;
					//	call_come = 0;
					//}
					do_gettimeofday(&tvEnd);
					if(is_time_out(tvStart, tvEnd)){
						if(phone_st.calling_status == 1){
							phone_st.calling_status = 0;
							phone_st.is_on_the_phone = 1;
							phone_st.hold_the_phone = 0; 
						}else if(phone_st.press_call_key_flag == 1 && (is_time_out(set_Start, tvEnd))){//���º��м���ʹ��SFB����һ������
							phone_st.call_reminder_status = 0;
							phone_st.is_on_the_phone = 0;
							phone_st.calling_status = 1; 
							phone_st.hold_the_phone = 0; 
							phone_st.press_call_key_flag = 0;
							//printk("press_call_key_flag\n");
						}else{
							phone_st.call_reminder_status = 0;
							if(answer_call_flag == 1){
								phone_st.is_on_the_phone = 1;
								answer_call_flag = 0;
							}else{
								phone_st.is_on_the_phone = 0;
							}
							phone_st.calling_status = 0; 
							phone_st.hold_the_phone = 0; 
							//phone_st.press_call_key_flag = 0;
							//printk("hang up phone 11111111111111\n");
						}
					}
					//printk("phone_st.press_call_key_flag  = %d\n",phone_st.press_call_key_flag );

				}
				
			}else if(skype_report.ctrl.wValue == 0x0217 && skype_report.ctrl.wIndex == 0x0003){
				if(skype_report.data[0] == 0x17 && skype_report.data[1] == 0x01){
					do_gettimeofday(&tvStart);
					if(phone_st.call_reminder_status == 1){ //�����绰
						phone_st.call_reminder_status = 0;
						phone_st.is_on_the_phone = 1;
						phone_st.hold_the_phone = 0; 
					}else if(phone_st.hold_the_phone == 0){
						phone_st.calling_status = 1;  //���е绰
						phone_st.call_reminder_status = 0;
						phone_st.is_on_the_phone = 0;
						phone_st.hold_the_phone = 0; 
						//printk("calling !!! \n");
					}
					phone_st.pc_call_flag = 1;
					phone_st.pc_hang_flag = 0;
				}else if(skype_report.data[0] == 0x17 && skype_report.data[1] == 0x00){
					do_gettimeofday(&tvEnd);
					if(is_time_out(tvStart, tvEnd)){
						if(phone_st.calling_status == 1 || phone_st.is_on_the_phone == 1 || phone_st.hold_the_phone == 1){
							phone_st.pc_hang_flag = 1;
						}
						phone_st.call_reminder_status = 0;
						phone_st.is_on_the_phone = 0;
						phone_st.calling_status = 0; 
						phone_st.hold_the_phone = 0; 
						phone_st.pc_call_flag = 0;
						//printk("hang up phone\n");
					}
				}
				
			}else if(skype_report.ctrl.wValue == 0x0220 && skype_report.ctrl.wIndex == 0x0003){
				do_gettimeofday(&tvStart);
				if(skype_report.data[0] == 0x20 && skype_report.data[1] == 0x01){	
					if(phone_st.is_on_the_phone == 1 || phone_st.resume_call_flag == 1){
						phone_st.suspend_call_flag = 1;
						phone_st.resume_call_flag = 0;
						phone_st.hold_the_phone = 1;
						phone_st.is_on_the_phone = 0;
						//printk("hold_the_phone \n");
					}else{
						phone_st.hold_the_phone = 1;
					}
				}else if(skype_report.data[0] == 0x20 && skype_report.data[1] == 0x00){
					if(phone_st.hold_the_phone == 1 || phone_st.suspend_call_flag == 1){
						phone_st.resume_call_flag = 1;
						phone_st.suspend_call_flag = 0;
						phone_st.is_on_the_phone = 1;
						phone_st.hold_the_phone = 0;
						//printk("on_the_phone \n");
					}else{
						//phone_st.is_on_the_phone = 1;
					}
				}
			}else if(skype_report.ctrl.wValue == 0x0209 && skype_report.ctrl.wIndex == 0x0003){
				phone_st.mute_flag = 1;
				if(skype_report.data[0] == 0x09 && skype_report.data[1] == 0x01){
					phone_st.mute_on_off = 1;
				}else{
					phone_st.mute_on_off = 0;
				}
			}
			break;
		default:
			break;
	}
}

#endif


#if 0
static int hid_set_intf_req(struct usb_function *f,
                const struct usb_ctrlrequest *ctrl)
{
	struct f_hidg			*hidg = func_to_hidg(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int len = le16_to_cpu(ctrl->wLength);
	DBG(cdev, "bRequest 0x%x\n",
			ctrl->bRequest);
	req->context = hidg;
	req->complete = f_hid_complete;

	return len;
}
#endif

static int hidg_setup_add_list(struct f_hidg *hidg, const struct usb_ctrlrequest *ctrl)
{
	struct f_hidg_req_list *ctrl_list;
	unsigned long flags;
	
	if(ctrl->wIndex != hidg->control_intf)
	{
		printk("dfu intf error\n");
		return -EOPNOTSUPP;
	}

	ctrl_list = kzalloc(sizeof(*ctrl_list), GFP_ATOMIC);
	if (!ctrl_list)
	{
		printk("kzalloc error\n");
		return -EOPNOTSUPP;
	}

	ctrl_list->type = HID_LIST_TYPE_CTRL;
	ctrl_list->p = (void *)ctrl;
	/* Tell the complete callback to generate an event for the next request
	 * that will be enqueued by UVCIOC_SEND_RESPONSE.
	 */
	hidg->event_setup_out = !(ctrl->bRequestType & USB_DIR_IN);
	hidg->event_length = le16_to_cpu(ctrl->wLength);

	//printk("HID setup_add_list\n");
	
	spin_lock_irqsave(&hidg->spinlock, flags);
	list_add_tail(&ctrl_list->list, &hidg->completed_out_req);
	spin_unlock_irqrestore(&hidg->spinlock, flags);

	
	wake_up(&hidg->read_queue);
	
	return 0;
}

static int hidg_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_hidg			*hidg = func_to_hidg(f);
	struct usb_composite_dev	*cdev = f->config->cdev;
	struct usb_request		*req  = cdev->req;
	int status = 0;
	__u16 value, length;
    unsigned int zero_flag = 0;
	//int val = -EOPNOTSUPP;
	value	= __le16_to_cpu(ctrl->wValue);
	length	= __le16_to_cpu(ctrl->wLength);
	
	DBG(cdev,
	     "%s crtl_request : bRequestType:0x%x bRequest:0x%x Value:0x%x\n",
	     __func__, ctrl->bRequestType, ctrl->bRequest, value);

	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_REPORT):
		DBG(cdev, "get_report\n");
		return hidg_setup_add_list(hidg, ctrl);
	#if 0
		/* send an empty report */
		length = min_t(unsigned, length, hidg->report_length);
		memset(req->buf, 0x0, length);

		goto respond;
		break;
	#endif
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_PROTOCOL):
		DBG(cdev,"get_protocol\n");
		goto stall;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_REPORT):
		DBG(cdev," set_report | wLenght=%d\n", ctrl->wLength);
		return hidg_setup_add_list(hidg, ctrl);
	#if 0
		//memcpy(&hid_ctrl,ctrl,sizeof(struct usb_ctrlrequest));
		skype_report.ctrl.bRequestType = ctrl->bRequestType;
		skype_report.ctrl.bRequest = ctrl->bRequest;
		skype_report.ctrl.wValue = ctrl->wValue;
		skype_report.ctrl.wIndex = ctrl->wIndex;
		skype_report.ctrl.wLength = ctrl->wLength;
		
		//goto stall;
		val = hid_set_intf_req(f, ctrl);
		if (val >= 0) {
		
			req->zero = 0;
			req->length = val;
			val = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
			if (val < 0)
				ERROR(cdev, "audio response on err %d\n", val);
			//printk(KERN_EMERG "%s:value:%d leave\n", __func__, value);
		}
		return val;
		//break;
	#endif
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_PROTOCOL):
		DBG(cdev,"set_protocol\n");
		goto stall;
		break;

	case ((USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_INTERFACE) << 8
		  | USB_REQ_GET_DESCRIPTOR):
		switch (value >> 8) {
		case HID_DT_HID:
			DBG(cdev, "USB_REQ_GET_DESCRIPTOR: HID\n");
			length = min_t(unsigned short, length,
						   hidg_desc.bLength);
			memcpy(req->buf, &hidg_desc, length);
			goto respond;
			break;
		case HID_DT_REPORT:
			DBG(cdev,"USB_REQ_GET_DESCRIPTOR: REPORT\n");
            if(hidg->report_desc_set_status == HID_REPORT_DESC_IS_SETTING) //正在设置报告描述符
                goto stall;
            hidg->report_desc_set_status = HID_REPORT_DESC_CANNOT_BE_SET;
            if((hidg->report_desc_length % cdev->gadget->ep0->maxpacket == 0) && 
                (length > hidg->report_desc_length))
            {
                zero_flag = 1;
            }
			length = min_t(unsigned short, length,
						   hidg->report_desc_length);
			memcpy(req->buf, hidg->report_desc, length);
			goto respond;
			break;

		default:
			DBG(cdev,"Unknown descriptor request 0x%x\n",
				 value >> 8);
			goto stall;
			break;
		}
		break;
		
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_GET_IDLE):
		VDBG(cdev, "get_idle\n");

		length = 1;
		((uint32_t *)req->buf)[0] = hidg->hid_idle;
		goto respond;
		break;

	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8
		  | HID_REQ_SET_IDLE):
		VDBG(cdev, "set_idle\n");

		hidg->hid_idle = (value >> 8) & 0xFF;
		
		length = 0;
		goto respond;
		break;
	default:
		DBG(cdev,"Unknown request 0x%x\n",
			 ctrl->bRequest);
		goto stall;
		break;
	}

stall:
	return -EOPNOTSUPP;

	

respond:
	req->zero = zero_flag;
	req->length = length;
	status = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
	if (status < 0)
		ERROR(cdev, "usb_ep_queue error on ep0 %d\n", value);
	return status;
}

static void hidg_disable(struct usb_function *f)
{
	struct f_hidg *hidg = func_to_hidg(f);
	struct f_hidg_req_list *list, *next;
	unsigned long flags;

	if(hidg->in_ep != NULL){
		usb_ep_disable(hidg->in_ep);
		//hidg->in_ep->driver_data = NULL;
	}

	spin_lock_irqsave(&hidg->spinlock, flags);
	list_for_each_entry_safe(list, next, &hidg->completed_out_req, list) {
		list_del(&list->list);
		kfree(list);
	}
	spin_unlock_irqrestore(&hidg->spinlock, flags);
}

static int hidg_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev		*cdev = f->config->cdev;
	struct f_hidg				*hidg = func_to_hidg(f);
	int status = 0;

	VDBG(cdev, "hidg_set_alt intf:%d alt:%d\n", intf, alt);

	if (hidg->in_ep != NULL) {
		/* restart endpoint */
		if (hidg->in_ep->enabled)
			usb_ep_disable(hidg->in_ep);

		status = config_ep_by_speed(f->config->cdev->gadget, f,
					    hidg->in_ep);
		if (status) {
			ERROR(cdev, "config_ep_by_speed FAILED!\n");
			goto fail;
		}
		status = usb_ep_enable(hidg->in_ep);
		if (status < 0) {
			ERROR(cdev, "Enable IN endpoint FAILED!\n");
			goto fail;
		}
		//hidg->in_ep->driver_data = hidg;
	}

fail:
	return status;
}

static int hid_set_custom_report_desc(struct f_hidg *hidg, unsigned long arg)
{
    int ret, cnt;
    struct hid_custom_report_desc *p_report = NULL;
    int report_len, data_len;
    struct usb_function *f = &hidg->func;
    
    cnt = copy_from_user((void *)&report_len, (const void *)arg, sizeof(int));
    if(cnt != 0)
    {
        printk("%s +%d %s:copy error\n", __FILE__, __LINE__, __func__);
        return -1;
    }
    
    data_len = report_len + sizeof(struct hid_custom_report_desc);
    p_report = kmalloc(data_len, GFP_KERNEL);
    if (p_report == NULL)
    {
        printk("%s +%d %s:malloc error\n", __FILE__, __LINE__, __func__);
        return -ENOMEM;
    }
    cnt = copy_from_user((void *)p_report, (const void *)arg, data_len);
    if(cnt != 0)
    {
        printk("%s +%d %s:copy error\n", __FILE__, __LINE__, __func__);
        kfree(p_report);
        return -2;
    }
    
    if(p_report->report_desc_length > g_hid_consumer_report_length_max)
    {
        printk("%s +%d %s:report length error, max is %d\n",__FILE__, __LINE__,
            __func__, g_hid_consumer_report_length_max);
        kfree(p_report);
        return -EACCES;
    }

    if(hidg->report_desc_set_status == HID_REPORT_DESC_CANNOT_BE_SET)
    {
        printk("%s +%d %s:hid report set error\n", __FILE__, __LINE__, __func__);
        kfree(p_report);
        ret = -EACCES;
    }
    hidg->report_desc_set_status = HID_REPORT_DESC_IS_SETTING;

    if(hidg->report_desc)
		kfree(hidg->report_desc);

    //hidg->report_length = p_report->report_length;
    hidg->report_desc_length = p_report->report_desc_length;
    hidg->report_desc = kmemdup(p_report->report_desc,
						p_report->report_desc_length,
						GFP_KERNEL);
    hidg_desc.desc[0].wDescriptorLength = cpu_to_le16(hidg->report_desc_length);
    kfree(f->fs_descriptors);
    kfree(f->hs_descriptors);
    kfree(f->ss_descriptors);
    ret = usb_assign_descriptors(f, hidg_fs_descriptors,
			hidg_hs_descriptors, hidg_ss_descriptors, NULL);
	if (ret)
	{
	    printk("%s +%d %s:hid report set error\n", __FILE__, __LINE__, __func__);
	}
    hidg->report_desc_set_status = HID_REPORT_DESC_CAN_BE_SET;

    kfree(p_report);
    return ret;
}

static long hid_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct f_hidg	*hidg  = file->private_data;
	size_t count;

	switch (cmd) {
#if 0	
	case UVCIOC_USB_CONNECT_STATE:{
		int *type = arg;
		
		if (*type == USB_STATE_CONNECT_RESET ){
			//printk("%s +%d %s:set usb connect reset\n", __FILE__,__LINE__,__func__);		
			usb_soft_connect_reset();	
			return 0;
		}else if(*type == USB_STATE_DISCONNECT){
			//printk("%s +%d %s:set usb disconnect\n", __FILE__,__LINE__,__func__);
		
			usb_soft_disconnect();	
			return 0;
			
		}else if(*type == USB_STATE_CONNECT){
			//printk("%s +%d %s:set usb connect\n", __FILE__,__LINE__,__func__);
			usb_soft_connect();
			return 0;
		}
		else
			return -EINVAL;
	}
	
	case UVCIOC_GET_USB_SOFT_CONNECT_STATE:{	
		uint32_t *usb_soft_connect_state = arg;

		*usb_soft_connect_state = get_usb_soft_connect_state();	
		
		return 0;
	}
	
	case UVCIOC_SET_USB_SOFT_CONNECT_CTRL:{
		uint32_t *value = arg;
		
		set_usb_soft_connect_ctrl((uint32_t)*value);		
		return 0;
	}
	
	case UVCIOC_GET_USB_SOFT_CONNECT_CTRL:{
		uint32_t *value = arg;
		
		*value = get_usb_soft_connect_ctrl();
		return 0;
	}
#endif	
	case UVCIOC_GET_HID_REPORT_LEN:{
		count = copy_to_user((void *)arg, (const void *)&hidg->report_length, sizeof(uint32_t));
		if(count != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
		}
		return 0;	
	}
	case UVCIOC_GET_HID_REPORT:{
		count = copy_to_user((void *)arg, (const void *)&phone_st, sizeof(phone_status_s));
		if(count != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
		}
		return 0;
	}
	case UVCIOC_SET_HID_REPORT:{
	
	
		count = copy_from_user((void *)&phone_st, (const void *)arg,sizeof(phone_status_s));
		if(count != 0)
		{
			printk("%s +%d %s:copy error\n",__FILE__,__LINE__,__func__);
		}
#if LINUX_VERSION_CODE>= KERNEL_VERSION(5,10,0)
		ktime_get_raw_ts64(&set_Start);
#else
		do_gettimeofday(&set_Start);
#endif
		return 0;
	}
	case HID_IOC_SEND_EP0_DATA:	
		return hidg_usb_send_ep0_data(file, (struct hid_request_data *)arg);
	case HID_IOC_GET_EP0_DATA:
		return hidg_usb_get_ep0_data(file, (struct hid_request_data *)arg);	
		
	case HID_IOC_SEND_EP_DATA:	
		return hidg_usb_ep_in_data(file, (struct hid_request_data *)arg);
    case UVCIOC_SET_HID_REPORT_DESC:
        printk("%s +%d %s:UVCIOC_SET_HID_REPORT_DESC\n", __FILE__,__LINE__,__func__);
        return hid_set_custom_report_desc(hidg, arg);  
	default:
		//printk("%s +%d %s: cmd = 0x%x\n", __FILE__,__LINE__,__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return ret;
}

static const struct file_operations f_hidg_fops = {
	.owner		= THIS_MODULE,
	.open		= f_hidg_open,
	.release	= f_hidg_release,
	.write		= f_hidg_write,
	.read		= f_hidg_read,
	.poll		= f_hidg_poll,
	.llseek		= noop_llseek,
	.unlocked_ioctl = hid_ioctl,
};

static int __init hidg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_ep		*ep;
	struct f_hidg		*hidg = func_to_hidg(f);
	int			status;
	dev_t			dev;


	int config_minor = DEV_HID_CONSUMER_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_CONSUMER_NUM_MAX)
		index = DEV_HID_CONSUMER_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	hidg_interface_desc.bInterfaceNumber = status;

	/* allocate instance-specific endpoints */
	status = -ENODEV;
    hidg->report_length = g_hid_consumer_ep_in_report_len;
    hidg_hs_in_ep_desc.wMaxPacketSize = cpu_to_le16(hidg->report_length > 1024 ? 1024 : hidg->report_length);
	ep = usb_ep_autoconfig_ss_backwards(c->cdev->gadget, &hidg_hs_in_ep_desc, NULL);
	if (!ep)
		goto fail;
	ep->driver_data = hidg;	/* claim */
	hidg->in_ep = ep;
	printk(KERN_EMERG  "%s:%d hidg->in_ep = 0x%p\n", __func__, __LINE__,hidg->in_ep);
	/* preallocate request and buffer */
	status = -ENOMEM;
	hidg->req = usb_ep_alloc_request(hidg->in_ep, GFP_KERNEL);
	if (!hidg->req)
		goto fail;

	hidg->req->buf = kmalloc(hidg->report_length, GFP_KERNEL);
	if (!hidg->req->buf)
		goto fail;

	/* set descriptor dynamic values */
	hidg_interface_desc.bInterfaceSubClass = hidg->bInterfaceSubClass;
	hidg_interface_desc.bInterfaceProtocol = hidg->bInterfaceProtocol;
	
	hidg_desc.desc[0].bDescriptorType = HID_DT_REPORT;
	hidg_desc.desc[0].wDescriptorLength =
		cpu_to_le16(hidg->report_desc_length);

	hidg_hs_in_ep_desc.bEndpointAddress = hidg->in_ep->address;

	hidg_fs_in_ep_desc.wMaxPacketSize = hidg_hs_in_ep_desc.wMaxPacketSize > 64 ? cpu_to_le16(64) : hidg_hs_in_ep_desc.wMaxPacketSize;
	hidg_fs_in_ep_desc.bEndpointAddress = hidg_hs_in_ep_desc.bEndpointAddress;

	hidg_ss_in_ep_desc.wMaxPacketSize = hidg_hs_in_ep_desc.wMaxPacketSize;
	hidg_ss_in_ep_comp_desc.wBytesPerInterval = hidg_ss_in_ep_desc.wMaxPacketSize;
	hidg_ss_in_ep_desc.bEndpointAddress = hidg_hs_in_ep_desc.bEndpointAddress;
		
	status = usb_assign_descriptors(f, hidg_fs_descriptors,
			hidg_hs_descriptors, hidg_ss_descriptors, NULL);
	if (status)
		goto fail;

	spin_lock_init(&hidg->write_spinlock);
	spin_lock_init(&hidg->spinlock);
	init_waitqueue_head(&hidg->write_queue);
	init_waitqueue_head(&hidg->read_queue);
	INIT_LIST_HEAD(&hidg->completed_out_req);

	hidg->control_intf = hidg_interface_desc.bInterfaceNumber;
	hidg->control_ep = c->cdev->gadget->ep0;
	hidg->control_ep->driver_data = hidg;
	
	/* Preallocate control endpoint request. */
	hidg->control_req = usb_ep_alloc_request(c->cdev->gadget->ep0, GFP_KERNEL);
	hidg->control_buf = kmalloc(g_hid_consumer_report_length_max, GFP_KERNEL);
	if (hidg->control_req == NULL || hidg->control_buf == NULL) {
		status = -ENOMEM;
		printk("%s: kmalloc control_req or control_buf error \n", __func__);
		goto fail;
	}
	
	hidg->control_req->buf = hidg->control_buf;
	hidg->control_req->complete = hidg_ep0_complete;
	hidg->control_req->context = hidg;
	
	init_completion(&hidg->get_ep0_data_done_completion);
	init_completion(&hidg->send_ep0_data_done_completion);
	init_completion(&hidg->ep_in_data_done_completion);
	init_completion(&hidg->ep_out_data_done_completion);
	
	/* create char device */
	cdev_init(&hidg->cdev, &f_hidg_fops);
	dev = MKDEV(major[index], config_minor);
	status = cdev_add(&hidg->cdev, dev, 1);
	if (status)
		goto fail_free_descs;

	device_create(hidg_class, NULL, dev, NULL, "%s%d", "hidg_consumer", config_minor - DEV_HID_CONSUMER_FIRST_NUM);

	return 0;

fail_free_descs:
	usb_free_all_descriptors(f);
fail:
	ERROR(f->config->cdev, "hidg_bind FAILED\n");
	if (hidg->req != NULL) {
		kfree(hidg->req->buf);
		if (hidg->in_ep != NULL)
			usb_ep_free_request(hidg->in_ep, hidg->req);
	}

	return status;
}

static void hidg_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_hidg *hidg = func_to_hidg(f);
	struct usb_composite_dev *cdev = c->cdev;

	int config_minor = DEV_HID_CONSUMER_FIRST_NUM + c->bConfigurationValue - 1;
	
	int index = c->bConfigurationValue - 1;
	
	if(index > DEV_HID_CONSUMER_NUM_MAX)
		index = DEV_HID_CONSUMER_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	device_destroy(hidg_class, MKDEV(major[index], config_minor));
	cdev_del(&hidg->cdev);

	/* disable/free request and end point */
	if(hidg->in_ep)
	{
		usb_ep_disable(hidg->in_ep);
		if(hidg->req)
		{
			usb_ep_dequeue(hidg->in_ep, hidg->req);
			usb_ep_free_request(hidg->in_ep, hidg->req);
		}

		hidg->in_ep->driver_data = NULL;
	}
	
	if(hidg->req->buf)
		kfree(hidg->req->buf);
	
	hidg->control_ep->driver_data = NULL;
	
	if (hidg->control_req)
		usb_ep_free_request(cdev->gadget->ep0, hidg->control_req);
	
	if(hidg->control_buf)
		kfree(hidg->control_buf);
	
	complete_all(&hidg->get_ep0_data_done_completion);
	complete_all(&hidg->send_ep0_data_done_completion);
	complete_all(&hidg->ep_in_data_done_completion);
	complete_all(&hidg->ep_out_data_done_completion);
	
	ghid_cleanup_consumer(c->bConfigurationValue);

	usb_free_all_descriptors(f);

	kfree(hidg->report_desc);
	kfree(hidg);
	printk(KERN_EMERG  "%s:%d \n", __func__, __LINE__);
}

/*-------------------------------------------------------------------------*/
/*                                 Strings                                 */

#define CT_FUNC_HID_UCQ_IDX	0

static struct usb_string ct_func_string_defs[] = {
	[CT_FUNC_HID_UCQ_IDX].s	= g_hid_ucq_string_arr,
	{},			/* end of list */
};

static struct usb_gadget_strings ct_func_string_table = {
	.language	= 0x0409,	/* en-US */
	.strings	= ct_func_string_defs,
};

static struct usb_gadget_strings *ct_func_strings[] = {
	&ct_func_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/
/*                             usb_configuration                           */

int __init hidg_bind_config_consumer(struct usb_configuration *c,
			    struct hidg_func_descriptor *fdesc, int mode)
{
	struct f_hidg *hidg;
	int status;
    unsigned char  *p_report_desc;
	

	status = usb_string_id(c->cdev);
	if (status < 0)
		return status;
	ct_func_string_defs[CT_FUNC_HID_UCQ_IDX].id = 0x21;
    strncpy(g_hid_ucq_string_arr, g_hid_ucq_string, sizeof(g_hid_ucq_string_arr)-1);

	/* allocate and initialize one new instance */
	hidg = kzalloc(sizeof *hidg, GFP_KERNEL);
	if (!hidg){
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}
    hidg->report_desc_set_status = HID_REPORT_DESC_CAN_BE_SET;
		
	//hidg->minor = index;

	if(fdesc == NULL){
		if(mode == 2)
		{
			hid_des.report_desc_length = sizeof(report_desc_volume);
            p_report_desc = report_desc_volume;
		}
		else
		{			
			hid_des.report_desc_length = sizeof(report_desc);
            p_report_desc = report_desc;
		}
        hidg->bInterfaceSubClass = hid_des.subclass;
		hidg->bInterfaceProtocol = hid_des.protocol;
		hidg->report_length = hid_des.report_length;
		hidg->report_desc_length = hid_des.report_desc_length;
		hidg->report_desc = kmemdup(p_report_desc,
					hid_des.report_desc_length,
					GFP_KERNEL);
	}else{
		hidg->bInterfaceSubClass = fdesc->subclass;
		hidg->bInterfaceProtocol = fdesc->protocol;
		hidg->report_length = fdesc->report_length;
		hidg->report_desc_length = fdesc->report_desc_length;
		hidg->report_desc = kmemdup(fdesc->report_desc,
				    fdesc->report_desc_length,
				    GFP_KERNEL);		
	}
										
	if (!hidg->report_desc) {
		kfree(hidg);
		printk("%s +%d %s: hidg_bind_config fail\n", __FILE__,__LINE__,__func__);
		return -ENOMEM;
	}

	hidg->func.name    = "hid1";
	hidg->func.strings = ct_func_strings;
	hidg->func.bind    = hidg_bind;
	hidg->func.unbind  = hidg_unbind;
	hidg->func.set_alt = hidg_set_alt;
	hidg->func.disable = hidg_disable;
	hidg->func.setup   = hidg_setup;

	/* this could me made configurable at some point */
	hidg->qlen	   = 4;

	status = usb_add_function(c, &hidg->func);
	if (status)
		kfree(hidg);

	printk("%d %s: consumer hidg_bind_config ok!\n",__LINE__,__func__);
	
	return status;
}
EXPORT_SYMBOL(hidg_bind_config_consumer);

int __init ghid_setup_consumer(struct usb_gadget *g, u8 bConfigurationValue)
{
	int status;
	dev_t dev;
	
	int count = DEV_HID_CONSUMER_FIRST_NUM + bConfigurationValue;
	
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_CONSUMER_NUM_MAX)
		index = DEV_HID_CONSUMER_NUM_MAX - 1;
	else if(index < 0)
		index = 0;

	if(hidg_class == NULL)
		hidg_class = class_create(THIS_MODULE, "consumer_hidg");
     
	status = alloc_chrdev_region(&dev, 0, count, "consumer_hidg");
	if (!status) {
		major[index] = MAJOR(dev);
		minors[index] = count;
	}

	return status;
}
EXPORT_SYMBOL(ghid_setup_consumer);

void ghid_cleanup_consumer(u8 bConfigurationValue)
{
	int index = bConfigurationValue - 1;
	
	if(index > DEV_HID_CONSUMER_NUM_MAX)
		index = DEV_HID_CONSUMER_NUM_MAX - 1;
	else if(index < 0)
		index = 0;
	
	if (major[index]) {
		unregister_chrdev_region(MKDEV(major[index], 0), minors[index]);
		major[index] = minors[index] = 0;
	}

	if(hidg_class != NULL && bConfigurationValue == 1)
	{
		class_destroy(hidg_class);
		hidg_class = NULL;
	}
}
EXPORT_SYMBOL(ghid_cleanup_consumer);

