/*
* sstar_usb_phy.c- Sigmastar
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

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include "../drivers/sstar/include/ms_platform.h"
#include "usb_phy.h"

struct sstar_priv {
	void __iomem	*base;
};

enum phy_speed_mode {
	SPEED_MODE_GEN1 = 0,
	SPEED_MODE_GEN2 = 1,
	SPEED_MODE_GEN3 = 2,
};

struct tx_voltage_settings {
    u16 reg_biasi;
    u16 reg_drv;
    u16 reg_dem;
    char *descript;
};

#define MAX_TX_VOL_OPT  17

static struct tx_voltage_settings tx_voltage_array[MAX_TX_VOL_OPT] = {
    {0x05, 0x22, 0x0E, "Va0.80_Vb0.53_De_m3.50dB"}, /* Va 0.80, Vb 0.53, De-emphasis -3.50dB */
    {0x06, 0x26, 0x0F, "Va0.90_Vb0.60_De_m3.50dB"}, /* Va 0.90, Vb 0.60, De-emphasis -3.50dB */
    {0x07, 0x2A, 0x11, "Va1.00_Vb0.67_De_m3.50dB"}, /* Va 1.00, Vb 0.67, De-emphasis -3.50dB (recommended) */
    {0x07, 0x2E, 0x13, "Va1.10_Vb0.65_De_m3.50dB"}, /* Va 1.10, Vb 0.65, De-emphasis -3.50dB (recommended, default) */
    {0x07, 0x2B, 0x13, "Va1.05_Vb0.67_De_m3.90dB"}, /* Va 1.05, Vb 0.67, De-emphasis -3.90dB (recommended) */
    {0x07, 0x2C, 0x15, "Va1.09_Vb0.67_De_m4.20dB"}, /* Va 1.09, Vb 0.67, De-emphasis -4.20dB (recommended) */
    {0x07, 0x29, 0x13, "Va1.00_Vb0.63_De_m4.00dB"}, /* Va 1.00, Vb 0.63, De-emphasis -4.00dB */
    {0x07, 0x28, 0x15, "Va1.00_Vb0.59_De_m4.60dB"}, /* Va 1.00, Vb 0.59, De-emphasis -4.60dB */
    {0x07, 0x32, 0x14, "Va1.20_Vb0.60_De_m3.50dB"}, /* Va 1.20, Vb 0.60, De-emphasis -3.50dB (AVDDL_TX_USB3 needs 0.9V) */
    {0x07, 0x34, 0x11, "Va1.20_Vb0.85_De_m3.00dB"}, /* Va 1.20, Vb 0.85, De-emphasis -3.00dB */
    {0x07, 0x35, 0x0F, "Va1.20_Vb0.90_De_m2.50dB"}, /* Va 1.20, Vb 0.90, De-emphasis -2.50dB */
    {0x07, 0x2C, 0x16, "Va1.10_Vb0.67_De_m4.30dB"}, /* Va 1.10, Vb 0.67, De-emphasis -4.30dB */
    {0x07, 0x2B, 0x18, "Va1.10_Vb0.63_De_m4.84dB"}, /* Va 1.10, Vb 0.63, De-emphasis -4.84dB */
    {0x07, 0x2A, 0x1A, "Va1.10_Vb0.60_De_m5.26dB"}, /* Va 1.10, Vb 0.60, De-emphasis -5.26dB */
    {0x07, 0x31, 0x16, "Va1.20_Vb0.75_De_m4.08dB"}, /* Va 1.20, Vb 0.75, De-emphasis -4.08dB */
    {0x07, 0x30, 0x19, "Va1.20_Vb0.70_De_m4.68dB"}, /* Va 1.20, Vb 0.70, De-emphasis -4.68dB */
    {0x07, 0x2E, 0x1C, "Va1.20_Vb0.65_De_m5.33dB"}, /* Va 1.20, Vb 0.65, De-emphasis -5.33dB */
};

static uint tx_volt_idx = 3; /* Va 1.10, Vb 0.65, De-emphasis -3.50dB (recommended, default) */
module_param(tx_volt_idx, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(tx_volt_idx, "Index of Tx volatage setting, 0~16");

static uint drv_cur = 0x2E; /* driver current, default 6'b101110 as index 3 */
module_param(drv_cur, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(drv_cur, "driver current, in decimal format");

static uint dem_cur = 0x13; /* dem current, default 6'b010011 as index 3 */
module_param(dem_cur, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(dem_cur, "dem current, in decimal format");

static uint vcm_cur = 0x07; /* dem current, default 3'b111 as index 3 */
module_param(vcm_cur, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(vcm_cur, "VCM current, in decimal format");

static uint _get_tx_volt_idx(void)
{
    uint i;

    for (i = 0; i < MAX_TX_VOL_OPT; i++)
    {
        if ((drv_cur == tx_voltage_array[i].reg_drv) &&
            (dem_cur == tx_voltage_array[i].reg_dem) &&
            (vcm_cur == tx_voltage_array[i].reg_biasi)) {
            return i;
        }
    }
    return i; // no matched index
}

static void _init_tx_volt_settings(void)
{
    if ((tx_volt_idx != 3) && (tx_volt_idx < MAX_TX_VOL_OPT)) {
        vcm_cur = tx_voltage_array[tx_volt_idx].reg_biasi;
        drv_cur = tx_voltage_array[tx_volt_idx].reg_drv;
        dem_cur = tx_voltage_array[tx_volt_idx].reg_dem;
    }
    else {
        tx_volt_idx = _get_tx_volt_idx();
    }
}

static ssize_t tx_voltage_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    uint index = 0;

    index = simple_strtoul(buf, NULL, 10);
    if (index < MAX_TX_VOL_OPT) {
        tx_volt_idx = index;
        vcm_cur = tx_voltage_array[index].reg_biasi;
        drv_cur = tx_voltage_array[index].reg_drv;
        dem_cur = tx_voltage_array[index].reg_dem;
    } else {
        dev_err(dev, "invalid index for tx voltage %d\n", index);
    }

    return count;
}

static ssize_t tx_voltage_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    uint i;

    tx_volt_idx = _get_tx_volt_idx();
    for (i = 0; i < MAX_TX_VOL_OPT; i++)
    {
        if (i != tx_volt_idx) {
            str += scnprintf(str, end - str, "%2d:%s\n", i, tx_voltage_array[i].descript);
        } else {
            str += scnprintf(str, end - str, "%2d:%s <= current\n", i, tx_voltage_array[i].descript);
        }
    }

    return (str - buf);
}

DEVICE_ATTR(tx_voltage, 0644, tx_voltage_show, tx_voltage_store);

static int usb_phy_init(struct phy *phy)
{
    sstar_usb_phy_utmi_deinit(phy);
    sstar_usb_phy_switch_owner(1); //1: switch to USB3.0; 0: switch to USB2.0
    sstar_usb_phy_setting(phy, vcm_cur, drv_cur, dem_cur);

    return 0;
}

static int usb_phy_exit(struct phy *phy)
{
    // return back the UTMI ownership
    sstar_usb_phy_switch_owner(0); //1: switch to USB3.0; 0: switch to USB2.0
    sstar_usb_phy_utmi_deinit(phy);
    return 0;
}

static int usb_phy_power_on(struct phy *phy)
{
    sstar_usb_phy_power_on(phy);
    
    return 0;
}

static int usb_phy_reset(struct phy *phy)
{
    sstar_usb_phy_reset();

    return 0;
}

static const struct phy_ops usb_phy_ops = {
    .init       = usb_phy_init,
    .exit       = usb_phy_exit,
    .power_on   = usb_phy_power_on,
    .reset      = usb_phy_reset,
    .owner      = THIS_MODULE,
};

static int usb_phy_probe(struct platform_device *pdev)
{
	struct phy_provider *phy_provider;
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct phy *phy;
	struct sstar_priv *priv;


	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);


	printk("SStar USB3 PHY probe, base:%x\n", (unsigned int)priv->base);


	phy = devm_phy_create(dev, NULL, &usb_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(phy);
	}

	phy_set_drvdata(phy, priv);
	device_create_file(&pdev->dev, &dev_attr_tx_voltage);
	_init_tx_volt_settings();

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(phy_provider);
}

void usb_phy_disable_emphasis(void)
{
	sstar_usb_disable_emphasis();
}

EXPORT_SYMBOL_GPL(usb_phy_disable_emphasis);

void usb_phy_disable_slew_rate(void)
{
	sstar_usb_disable_slew_rate();
}

EXPORT_SYMBOL_GPL(usb_phy_disable_slew_rate);

static const struct of_device_id usb_phy_of_match[] = {
	{.compatible = "sstar,sstar-usb-phy",},
	{ },
};
MODULE_DEVICE_TABLE(of, usb_phy_of_match);

static struct platform_driver usb_phy_driver = {
	.probe	= usb_phy_probe,
	.driver = {
		.name	= "phy",
		.of_match_table	= usb_phy_of_match,
	}
};
module_platform_driver(usb_phy_driver);

MODULE_AUTHOR("Jiang Ann <jiang.ann@sigmastar.com>");
MODULE_DESCRIPTION("SStar USB3 PHY driver");
MODULE_ALIAS("platform:sstar-usb-phy");
MODULE_LICENSE("GPL v2");
