/*
 * phy_sstar_utmi.c- Sigmastar
 *
 * Copyright (c) [2019~2021] SigmaStar Technology.
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

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/regmap.h>
//#include <registers.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <io.h>
#include <linux/bitops.h>
#include "phy_sstar_u3phy.h"
#include "phy_sstar_port.h"
#include "phy_sstar_utmi_reg.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include "phy_sstar_utmi_debugfs.h"

// 0x00: 550mv, 0x20: 575, 0x40: 600, 0x60: 625
#define UTMI_DISCON_LEVEL_2A              (0x62)
#define TX_RX_RESET_CLK_GATING_ECO_BITSET BIT(5)
#define LS_CROSS_POINT_ECO_BITSET         BIT(6)

#define UTMI_CALIBRATION_TIMEOUT 1000000000

static void sstar_utmi_sync_tx_swing_and_de_emphasis(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;
    int                    tx_swing[3];

    if (device_property_read_u32_array(u3phy_port->dev, "sstar,tx-swing-and-de-emphasis", tx_swing,
                                       ARRAY_SIZE(tx_swing)))
    {
        return;
    }
    CLRREG16(base + (0x16 << 2), BIT(4) | BIT(5) | BIT(6));
    SETREG16(base + (0x16 << 2), tx_swing[0] << 4);
    CLRREG16(base + (0x16 << 2), BIT(7) | BIT(8) | BIT(9));
    SETREG16(base + (0x16 << 2), tx_swing[1] << 7);
    CLRREG16(base + (0x17 << 2), BIT(3) | BIT(4) | BIT(5));
    SETREG16(base + (0x17 << 2), tx_swing[2] << 3);
}

static void sstar_utmi_show_trim_value(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    dev_info(&phy->dev, "%s rterm = 0x%04x\r\n", __func__, INREG16(base + (0x45 << 2)) & 0x003f);
    dev_info(&phy->dev, "%s hs_tx_current = 0x%04x\r\n", __func__, INREG16(base + (0x44 << 2)) & 0x03f0);
    dev_info(&phy->dev, "%s\r\n", __func__);
}

static void __maybe_unused sstar_utmi_avoid_floating(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    CLRREG16(base + (0x0 << 2), (BIT(3) | BIT(4) | BIT(5))); // DP_PUEN = 0 DM_PUEN = 0 R_PUMODE = 0

    /*
     * patch for DM always keep high issue
     * init overwrite register
     */
    SETREG16(base + (0x05 << 2), BIT(6)); // hs_txser_en_cb = 1
    CLRREG16(base + (0x05 << 2), BIT(7)); // hs_se0_cb = 0

    /* Turn on overwirte mode for D+/D- floating issue when UHC reset
     * Before UHC reset, R_DP_PDEN = 1, R_DM_PDEN = 1, tern_ov = 1 */
    SETREG16(base + (0x0 << 2), (BIT(7) | BIT(6) | BIT(1)));
    /* new HW term overwrite: on */
    // SETREG16(base + (0x29 << 2), (BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0));
}

static void sstar_utmi_disconnect_window_select(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    CLRREG16(base + (0x01 << 2), BIT(11) | BIT(12) | BIT(13)); // Disconnect window select
    SETREG16(base + (0x01 << 2), (0x05 << 11));
    dev_dbg(&phy->dev, "%s\r\n", __func__);
}

static void sstar_utmi_ISI_effect_improvement(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    SETREG16(base + (0x04 << 2), BIT(8)); // ISI effect improvement
    dev_dbg(&phy->dev, "%s\r\n", __func__);
}

static void sstar_utmi_RX_anti_dead_loc(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    SETREG16(base + (0x04 << 2), BIT(15)); // UTMI RX anti-dead-loc
    dev_dbg(&phy->dev, "%s\r\n", __func__);
}

static void sstar_utmi_chirp_signal_source_select(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    SETREG16(base + (0x0a << 2), BIT(13)); // Chirp signal source select
    dev_dbg(&phy->dev, "%s\r\n", __func__);
}

static void sstar_utmi_disable_improved_CDR(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    CLRREG16(base + (0x03 << 2), BIT(9)); // Disable improved CDR
    dev_dbg(&phy->dev, "%s\r\n", __func__);
}

static void sstar_utmi_RX_HS_CDR_stage_control(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    // RX HS CDR stage control
    CLRREG16(base + (0x03 << 2), (BIT(5) | BIT(6)));
    SETREG16(base + (0x03 << 2), BIT(6));
    dev_dbg(&phy->dev, "%s\r\n", __func__);
}

static void sstar_utmi_reset_assert(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    SETREG16(base + (0x03 << 2),
             BIT(0) | BIT(1) | BIT(8));    // bit0: RX sw reset; bit1: Tx sw reset; bit8: Tx FSM sw reset;
    SETREG16(base + (0x08 << 2), BIT(12)); // bit12: pwr good reset
} 

static void sstar_utmi_reset_deassert(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    CLRREG16(base + (0x03 << 2),
             BIT(0) | BIT(1) | BIT(8));    // bit0: RX sw reset; bit1: Tx sw reset; bit8: Tx FSM sw reset;
    CLRREG16(base + (0x08 << 2), BIT(12)); // bit12: pwr good reset
}

static int sstar_utmi_calibrate(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    u16 u16_ret;

    /* Init UTMI squelch level setting befor CA */
    OUTREG8(base + (0x15 << 2), (UTMI_DISCON_LEVEL_2A & (BIT(3) | BIT(2) | BIT(1) | BIT(0))));
    dev_dbg(&phy->dev, "%s, squelch level 0x%02x\n", __func__, INREG8(base + (0x15 << 2)));

    SETREG16(base + (0x1e << 2), BIT(0)); // set CA_START as 1
    mdelay(1);

    CLRREG16(base + (0x1e << 2), BIT(0)); // release CA_START

    if (readw_poll_timeout_atomic((base + (0x1e << 2)), u16_ret, u16_ret & BIT(1), 1,
                                  UTMI_CALIBRATION_TIMEOUT)) // polling bit <1> (CA_END)
    {
        dev_info(&phy->dev, "%s, calibration timeout!\n", __func__);
        return -ETIMEDOUT;
    }

    if (PHY_MODE_USB_HOST == phy_get_mode(phy))
    {
        OUTREG8(base + (0x15 << 2), UTMI_DISCON_LEVEL_2A);
    }

    dev_info(&phy->dev, "%s completed.\r\n", __func__);
    return 0;
}

static int sstar_utmi_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    if (PHY_MODE_USB_HOST == mode)
    {
        /* bit<3> for 240's phase as 120's clock set 1, bit<4> for 240Mhz in mac 0 for faraday 1 for etron */
        // SETREG16(base + (0x04 << 2), BIT(3));
        // dev_dbg(&phy->dev, "Init UTMI disconnect level setting\r\n");
        /* Init UTMI eye diagram parameter setting */
        // SETREG16(base + (0x16 << 2), 0x210);
        // ETREG16(base + (0x17 << 2), 0x8100);
        dev_dbg(&phy->dev, "Init UTMI eye diagram parameter setting\r\n");

        /* Enable hw auto deassert sw reset(tx/rx reset) */
        SETREG8(base + (0x02 << 2), TX_RX_RESET_CLK_GATING_ECO_BITSET);

        /* Change override to hs_txser_en.	Dm always keep high issue */
        SETREG16(base + (0x08 << 2), BIT(6));
    }
    else if (PHY_MODE_USB_DEVICE == mode)
    {
        // SETREG16(base + (0x16 << 2), 0x0290);
        // SETREG16(base + (0x17 << 2), 0110);

        SETREG16(base + (0x05 << 2), BIT(15)); // set reg_ck_inv_reserved[6] to solve timing problem
        dev_dbg(&phy->dev, "%s, set reg_ck_inv_reserved[6] to solve timing problem.\n", __func__);
        SETREG16(base + (0x02 << 2), BIT(7)); // avoid glitch

        OUTREG8(base + (0x1A << 2), 0x62); // Chirp k detection level: 0x80 => 400mv, 0x20 => 575mv
    }
    sstar_utmi_sync_tx_swing_and_de_emphasis(phy);

    CLRREG16(base + (0x04 << 2), BIT(3)); // for low vcore issue
    dev_info(&phy->dev, "%s mode = 0x02%x\r\n", __func__, mode);
    return 0;
}

static int sstar_utmi_exit(struct phy *phy)
{
    dev_info(&phy->dev, "%s completed.\n", __func__);
    return 0;
}

static int sstar_utmi_powe_off(struct phy *phy)
{
    u16 u16_val;

    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    u16_val = BIT(0) | BIT(1) | BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14);

    SETREG16(base + (0x0 << 2), u16_val);
    mdelay(5);

    dev_info(&phy->dev, "%s completed\n", __func__);
    return 0;
}

static int sstar_utmi_power_on(struct phy *phy)
{
    sstar_utmi_reset_deassert(phy);
    sstar_utmi_show_trim_value(phy);
    dev_dbg(&phy->dev, "%s\n", __func__);
    return 0;
}

static int sstar_utmi_init(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    /*
        [11] : enable PLL of usb2_atop without register
        [7] : enable bandgap current of usb2_atop
    */
    OUTREG16(base + (0x04 << 2), 0x0C27);
    SETREG16(base + (0x1f << 2), 0x0100); // for DesignWare USB3 DRD Controller

    sstar_utmi_powe_off(phy);
    sstar_utmi_reset_assert(phy);
    /*
        [15] : enable regulator of usb2_atop
        [12] : enable FS/LS transceiver of usb2_atop
        [10] : enable HS squelch circuit of usb2_atop
        [7] : enable FS/LS DM pull-down resistor
        [6] : enable FS/LS DP pull-down resistor
        [2] : enable reference block of usb2_atop
        [1] : enable FS/LS termination force mode
    */
    OUTREG16(base + (0x00 << 2), 0x6BC3);
    dev_dbg(&phy->dev, "%s 0x6BC3\n", __func__);
    mdelay(1);
    // OUTREG8(base + (0x00 << 2), 0x69); // Turn on UPLL, reg_pdn: bit<9>
    // dev_dbg(&phy->dev, "%s 0x69\n", __func__);
    // mdelay(2);

    /*
        [14] : enable HS current reference block of usb2_atop
        [13] : enable VBUS detector of usb2_atop
        [11] : enable HS pre-amplifier of usb2_atop
        [8] : enable HS de-serializer of usb2_atop
        [7] : disable FS/LS DM pull-down resistor
        [6] : disable FS/LS DP pull-down resistor
        [1] : disable FS/LS termination force mode
    */
    OUTREG16(base + (0x00 << 2), 0x0001); // Turn all (including hs_current) use override mode
    // SETREG16(base + (0x1f << 2), BIT(15));
    // SETREG16(base + (0x01 << 2), BIT(10));
    dev_dbg(&phy->dev, "%s 0x0001\n", __func__);
    // Turn on UPLL, reg_pdn: bit<9>
    mdelay(3);

    if (device_property_read_bool(&phy->dev, "utmi_dp_dm_swap"))
    {
        SETREG8(base + (0x5 << 2) + 1, BIT(5)); // dp dm swap
        dev_dbg(&phy->dev, "%s, dp dm swap\r\n", __func__);
    }

    sstar_utmi_RX_HS_CDR_stage_control(phy);
    sstar_utmi_disable_improved_CDR(phy);
    sstar_utmi_chirp_signal_source_select(phy);
    sstar_utmi_disconnect_window_select(phy);
    sstar_utmi_disable_improved_CDR(phy);
    sstar_utmi_RX_anti_dead_loc(phy);
    sstar_utmi_ISI_effect_improvement(phy);

    SETREG16(base + (0x4 << 2), BIT(6)); // RSIB_SERDES
    dev_info(&phy->dev, "%s completed.\r\n", __func__);
    return 0;
}

static int sstar_utmi_reset(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    sstar_utmi_reset_assert(phy);
    CLRREG16(base + (0x5 << 2), BIT(6)); // RSIB_SERDES
    SETREG16(base + (0x5 << 2), BIT(6));

    CLRREG16(base + (0x8 << 2), BIT(6));
    SETREG16(base + (0x8 << 2), BIT(6));
    sstar_utmi_reset_deassert(phy);
    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

void sstar_usb_phy_utmi_reset(void)
{
//未确定，后面再添加
}
EXPORT_SYMBOL(sstar_usb_phy_utmi_reset);

static const struct phy_ops sstar_htmi_ops = {
    .init      = sstar_utmi_init,
    .exit      = sstar_utmi_exit,
    .power_on  = sstar_utmi_power_on,
    .power_off = sstar_utmi_powe_off,
    .set_mode  = sstar_utmi_set_mode,
    .reset     = sstar_utmi_reset,
    .calibrate = sstar_utmi_calibrate,
    .owner     = THIS_MODULE,
};

int sstar_utmi_port_init(struct device *dev, struct sstar_phy_port *phy_port, struct device_node *np)
{
    int ret;

    phy_port->speed = USB_SPEED_HIGH;
    if (0 == (ret = sstar_port_init(dev, np, &sstar_htmi_ops, phy_port)))
    {
        sstar_phy_utmi_debugfs_init(phy_port);
    }

    return ret;
}
EXPORT_SYMBOL(sstar_utmi_port_init);

int sstar_utmi_port_remove(struct sstar_phy_port *phy_port)
{
    sstar_phy_utmi_debugfs_exit(phy_port);
    return sstar_port_deinit(phy_port);
}
EXPORT_SYMBOL(sstar_utmi_port_remove);

void sstar_utmi_disable_emphasis(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    CLRREG16(base + 0x16 * 4, 0x200);
}

EXPORT_SYMBOL_GPL(sstar_utmi_disable_emphasis);

void sstar_utmi_disable_slew_rate(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);
    void __iomem *         base       = (void __iomem *)u3phy_port->reg;

    SETREG16(base + 0x16 * 4, 0xc);
}

EXPORT_SYMBOL_GPL(sstar_utmi_disable_slew_rate);
#if 0
static const struct of_device_id sstar_utmi_dt_ids[] = {
	{ .compatible = "sstar, infinity7-utmi" }, 
	{}
};

static struct platform_driver sstar_utmi_driver = {
	.probe	= sstar_utmi_probe,
	.remove = sstar_utmi_remove,
	.driver =
		{
			.name = "sstar-utmi",
			.of_match_table = sstar_utmi_dt_ids,
		},
};

module_platform_driver(sstar_utmi_driver);

MODULE_DESCRIPTION("SSTAR PHY UTMI driver");
MODULE_LICENSE("GPL v2");
#endif
