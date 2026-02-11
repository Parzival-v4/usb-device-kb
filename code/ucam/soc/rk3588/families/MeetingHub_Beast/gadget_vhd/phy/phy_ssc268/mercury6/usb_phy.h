/*
* usb_phy.h- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

#ifndef _USB_PHY_H_
#define _USB_PHY_H_

void sstar_usb_phy_utmi_deinit(struct phy *phy);
void sstar_usb_phy_switch_owner(int owner);
void sstar_usb_phy_power_on(struct phy *phy);
void sstar_usb_phy_setting(struct phy *phy, uint vcm_cur, uint drv_cur, uint dem_cur);
void sstar_usb_phy_reset(void);
void sstar_usb_disable_emphasis(void);
void sstar_usb_disable_slew_rate(void);

#endif