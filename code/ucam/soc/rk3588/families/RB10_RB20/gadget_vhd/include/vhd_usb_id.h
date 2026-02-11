/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2021-09-08 19:32:28
 * @FilePath    : \release\gadget_vhd\include\vhd_usb_id.h
 * @Description : 
 */
#ifndef _VHD_USB_ID_H
#define _VHD_USB_ID_H

const uint16_t usb_id_list[][2] = {
	//VID bypass
    {0x2e7e,0xFFFF},//VHD
	{0x3242,0xFFFF},//HIS
	{0x146E,0xFFFF},//ClearOne
	{0x28DB,0xFFFF},//Konftel
	{0x145F,0xFFFF},//Trust
	{0x0543,0xFFFF},//ViewSonic
	{0x1395,0xFFFF},//EPOS EXPAND
	{0x095D,0xFFFF},//polycom
	{0x2757,0xFFFF},//鸿合
	{0x343F,0xFFFF},//好视通
	{0x24AE,0xFFFF},//雷柏
	{0x2D34,0xFFFF},//Nureva
	{0x2E03,0xFFFF},//M1000M DH-VCS-C5A0

	{0x109B,0xFFFF},//JX1700US haixin
	{0x33F1,0xFFFF},//JX1700USX haixin
	{0x2BDF,0xFFFF},//JX1700US 海康
	{0x291A,0xFFFF},//M1000 anker
	{0x4252,0xFFFF},//JX1701USV_QH
	{0x28E6,0xFFFF},//JX1700US Biamp Vidi150
	{0x17EF,0xFFFF},//J1700CL联想定制
	{0x1FF7,0xFFFF},//视联动力
	{0x25C1,0xFFFF},//Vaddio

	{0x046d,0x085e},//LOGITECH
};

#endif /* _VHD_USB_ID_H */
