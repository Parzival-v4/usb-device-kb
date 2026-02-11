/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-08-06 14:39:54
 * @FilePath    : \gadget_vhd\udc\dwc3_hisi\usb_ctrl.h
 * @Description : 
 */
#ifndef __USB_CTRL_H__
#define __USB_CTRL_H__

extern void dwc3_regs_soft_connect_init(void __iomem	*regs);

extern void set_usb_soft_connect_ctrl(uint32_t value);
extern uint32_t get_usb_soft_connect_ctrl(void);
extern int usb_soft_connect(void);
extern int vbus_usb_soft_connect(void);
extern int usb_soft_disconnect(void);
extern int vbus_usb_soft_disconnect(void);
extern int usb_soft_connect_reset(void);
extern uint32_t get_usb_soft_connect_status(void);
extern uint32_t get_usb_soft_connect_state(void);
#endif
