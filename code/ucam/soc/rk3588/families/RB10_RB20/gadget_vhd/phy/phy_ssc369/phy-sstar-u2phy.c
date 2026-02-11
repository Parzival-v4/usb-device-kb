/*
 * phy-sstar-u2phy.c - Sigmastar
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

#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include "phy-sstar-u2phy.h"
//#include <io.h>

// 0x00: 550mv, 0x20: 575, 0x40: 600, 0x60: 625
#define UTMI_DISCON_LEVEL_2A (0x62)

static void sstar_u2phy_utmi_avoid_floating(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* DP_PUEN = 0 DM_PUEN = 0 R_PUMODE = 0 */
    regmap_clear_bits(utmi, (0x0 << 2), (BIT(3) | BIT(4) | BIT(5)));

    /*
     * patch for DM always keep high issue
     * init overwrite register
     */
    regmap_set_bits(utmi, (0x05 << 2), BIT(6));   // hs_txser_en_cb = 1
    regmap_clear_bits(utmi, (0x05 << 2), BIT(7)); // hs_se0_cb = 0

    /* Turn on overwirte mode for D+/D- floating issue when UHC reset
     * Before UHC reset, R_DP_PDEN = 1, R_DM_PDEN = 1, tern_ov = 1 */
    regmap_set_bits(utmi, (0x0 << 2), (BIT(7) | BIT(6) | BIT(1)));
    /* new HW term overwrite: on */
    regmap_set_bits(utmi, (0x29 << 2), (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)));
}

static void sstar_u2phy_utmi_etron_enable(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;
    struct regmap *     usbc = priv->usbc;

    /* [7]: reg_etron_en, to enable utmi Preamble function */
    regmap_set_bits(utmi, (0x1f << 2), BIT(15));

    /* [11]: reg_preamble_en, to enable Faraday Preamble */
    regmap_set_bits(usbc, (0x07 << 2), BIT(11));

    /* [0]: reg_preamble_babble_fix, to patch Babble occurs in Preamble */
    /* [1]: reg_preamble_fs_within_pre_en, to patch FS crash problem */
    regmap_set_bits(usbc, (0x08 << 2), BIT(1) | BIT(0));
}

static void sstar_u2phy_utmi_disconnect_window_select(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* Disconnect window select */
    regmap_update_bits(utmi, (0x01 << 2), BIT(13) | BIT(12) | BIT(11), BIT(13) | BIT(11));
}

static void sstar_u2phy_utmi_ISI_effect_improvement(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* ISI effect improvement */
    regmap_set_bits(utmi, (0x04 << 2), BIT(8));
}

static void sstar_u2phy_utmi_RX_anti_dead_loc(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* UTMI RX anti-dead-loc */
    regmap_set_bits(utmi, (0x04 << 2), BIT(15));
}

static void sstar_u2phy_utmi_TX_timing_latch_select(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* TX timing select latch path */
    regmap_set_bits(utmi, (0x05 << 2), BIT(15));
}

static void sstar_u2phy_utmi_chirp_signal_source_select(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /*
     * [13]: Chirp signal source select
     * [14]: change to 55 interface
     */
    regmap_set_bits(utmi, (0x0a << 2), BIT(14) | BIT(13));
}

static void sstar_u2phy_utmi_CDR_control(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* RX HS CDR stage control */
    regmap_update_bits(utmi, (0x03 << 2), (BIT(6) | BIT(5)), BIT(6));
    /* Disable improved CDR */
    regmap_clear_bits(utmi, (0x03 << 2), BIT(9));
}

static void sstar_u2phy_utmi_new_hw_chirp(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* new HW chirp design, default overwrite to reg_2A */
    regmap_clear_bits(utmi, (0x20 << 2), BIT(4));

    /* Init UTMI disconnect level setting (UTMI_DISCON_LEVEL_2A: 0x62) */
    regmap_set_bits(utmi, (0x15 << 2), BIT(6) | BIT(5) | BIT(1));
    dev_info(&phy->dev, "Init UTMI disconnect level setting\r\n");
}

static int sstar_u2phy_utmi_calibrate(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    sstar_u2phy_utmi_disconnect_window_select(phy);
    sstar_u2phy_utmi_CDR_control(phy);

    sstar_u2phy_utmi_chirp_signal_source_select(phy);
    sstar_u2phy_utmi_RX_anti_dead_loc(phy);
    sstar_u2phy_utmi_ISI_effect_improvement(phy);
    sstar_u2phy_utmi_TX_timing_latch_select(phy);
    sstar_u2phy_utmi_new_hw_chirp(phy);

    /* Begin: ECO patch */
    /* [2]: reg_fl_sel_override, to override utmi to have FS drive strength */
    regmap_set_bits(utmi, (0x01 << 2), BIT(10));

    /* Enable deglitch SE0 (low-speed cross point) */
    regmap_set_bits(utmi, (0x02 << 2), BIT(6));

    /* Enable hw auto deassert sw reset(tx/rx reset) */
    regmap_set_bits(utmi, (0x02 << 2), BIT(5));

    sstar_u2phy_utmi_etron_enable(phy);
    /* End: ECO patch */
    return 0;
}

static int sstar_u2phy_utmi_set_mode(struct phy *phy, enum phy_mode mode, int submode)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;
    struct regmap *     usbc = priv->usbc;
    struct regmap *     ehc  = priv->ehc;

    switch (mode)
    {
        case PHY_MODE_USB_HOST:
            dev_info(&phy->dev, "Set phy mode -> host\n");
            /* UHC select enable */
            regmap_update_bits(usbc, (0x01 << 2), BIT(1) | BIT(0), BIT(0));

            /* [4]: 0:Vbus On, 1:Vbus off */
            /* [3]: Interrupt signal active high*/
            regmap_clear_bits(ehc, (0x20 << 2), BIT(4));
            udelay(1); // delay 1us
            regmap_set_bits(ehc, (0x20 << 2), BIT(3));

            /* improve the efficiency of USB access MIU when system is busy */
            regmap_set_bits(ehc, (0x40 << 2), BIT(15) | BIT(11) | BIT(10) | BIT(9) | BIT(8));

            /* Init UTMI eye diagram parameter setting */
            regmap_write(utmi, (0x16 << 2), 0x0210);
            regmap_write(utmi, (0x17 << 2), 0x8100);
            dev_info(&phy->dev, "Init UTMI eye diagram parameter setting\r\n");

            /* ENABLE_UHC_RUN_BIT_ALWAYS_ON_ECO, Don't close RUN bit when device disconnect */
            regmap_set_bits(ehc, (0x1A << 2), BIT(7));

            /* _USB_MIU_WRITE_WAIT_LAST_DONE_Z_PATCH, Enable PVCI i_miwcplt wait for mi2uh_last_done_z */
            regmap_set_bits(ehc, (0x41 << 2), BIT(12));

            /* ENABLE_UHC_EXTRA_HS_SOF_ECO, Extra HS SOF after bus reset */
            regmap_set_bits(ehc, (0x46 << 2), BIT(0));

            break;
        default:
            return -1;
    }
    return 0;
}

static int sstar_u2phy_utmi_reset(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* [0]: RX sw reset
     * [1]: Tx sw reset
     * [8]: Tx FSM sw reset
     */
    regmap_set_bits(utmi, (0x03 << 2), BIT(8) | BIT(1) | BIT(0));
    /* [12]: pwr good reset */
    regmap_set_bits(utmi, (0x08 << 2), BIT(12));
    mdelay(1);

    /* Clear reset */
    regmap_clear_bits(utmi, (0x03 << 2), BIT(8) | BIT(1) | BIT(0));
    regmap_clear_bits(utmi, (0x08 << 2), BIT(12));

    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

static int sstar_u2phy_utmi_power_off(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;

    /* [0]:  1: Enable USB_XCVR power-down control override
     * [8]:  1: Power down USB_XCVR HS de-serializer block
     * [9]:  1: Power down USB_XCVR pll block
     * [10]: 1: Power down USB_XCVR HS TED block
     * [11]: 1: Power down USB_XCVR HS pre-amplifier block
     * [12]: 1: Power down USB_XCVR FS/LS transceiver block
     * [13]: 1: Power down USB_XCVR VBUS detector block
     * [14]: 1: Power down USB_XCVR HS current reference block
     * [15]: 1: Power down USB_XCVR builtin regulator block
     */
    regmap_set_bits(utmi, (0x00 << 2), BIT(14) | BIT(13) | BIT(12) | BIT(11) | BIT(10) | BIT(9) | BIT(8) | BIT(0));
    mdelay(5);

    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

static int sstar_u2phy_utmi_power_on(struct phy *phy)
{
    struct sstar_u2phy *priv = phy_get_drvdata(phy);
    struct regmap *     utmi = priv->utmi;
    // struct regmap *bc = priv->bc;

    /* Turn on all use override mode*/
    regmap_write(utmi, (0x0 << 2), 0x0001);

    /*[6]:	reg_into_host_bc_sw_tri*/
    // regmap_clear_bits(bc, (0x06 << 2), BIT(6));

    /*[14]:  reg_host_bc_en*/
    // regmap_clear_bits(bc, (0x01 << 2), BIT(14));

    // regmap_clear_bits(utmi, (0x0 << 2), BIT(14));

    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

int sstar_u2phy_utmi_init(struct phy *phy)
{
    struct sstar_u2phy *priv    = phy_get_drvdata(phy);
    struct regmap *     utmi    = priv->utmi;
    struct regmap *     usbc    = priv->usbc;
    u32                 reg_val = 0;

    regmap_write(utmi, (0x0 << 2), 0x0001);

    if (priv->phy_data && priv->phy_data->has_utmi2_bank)
    {
        struct device_node *node;
        struct regmap *     utmi2;

        node = of_parse_phandle(phy->dev.of_node, "syscon-utmi2", 0);
        if (node)
        {
            utmi2 = syscon_node_to_regmap(node);
            if (IS_ERR(utmi2))
            {
                dev_info(&phy->dev, "No extra utmi setting\n");
            }
            else
            {
                regmap_write(utmi2, (0x0 << 2), 0x7F05);
                dev_info(&phy->dev, "Extra utmi2 setting\n");
            }
            of_node_put(node);
        }
    }
    regmap_write(utmi, (0x4 << 2), 0x0C27);
    sstar_u2phy_utmi_avoid_floating(phy);

    regmap_set_bits(usbc, (0x0 << 2), BIT(3) | BIT(1)); // Disable MAC initial suspend, Reset UHC
    regmap_clear_bits(usbc, (0x0 << 2), BIT(1));        // Release UHC reset
    regmap_set_bits(usbc, (0x0 << 2), BIT(5));          // enable UHC and OTG XIU function

    /* Init UTMI squelch level setting befor CA */
    regmap_update_bits(utmi, (0x15 << 2), 0xFF, UTMI_DISCON_LEVEL_2A & (BIT(3) | BIT(2) | BIT(1) | BIT(0)));
    regmap_read(utmi, (0x15 << 2), &reg_val);
    dev_info(&phy->dev, "squelch level 0x%08x\n", reg_val);

    /* set CA_START as 1 */
    regmap_set_bits(utmi, (0x1e << 2), BIT(0));
    mdelay(1);
    /* release CA_START */
    regmap_clear_bits(utmi, (0x1e << 2), BIT(0));

    /* polling bit <1> (CA_END) */
    do
    {
        regmap_read(utmi, (0x1e << 2), &reg_val);
    } while ((reg_val & BIT(1)) == 0);

    regmap_read(utmi, (0x1e << 2), &reg_val);
    if (0xFFF0 == reg_val || 0x0000 == (reg_val & 0xFFF0))
        dev_info(&phy->dev, "WARNING: CA Fail !! \n");

    /* Turn on overwirte mode for D+/D- floating issue when UHC reset
     * After UHC reset, disable overwrite bits
     */
    regmap_clear_bits(utmi, (0x0 << 2), (BIT(7) | BIT(6) | BIT(1)));
    /* new HW term overwrite: off */
    regmap_clear_bits(utmi, (0x29 << 2), (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0)));

    if (device_property_read_bool(&phy->dev, "utmi_dp_dm_swap"))
    {
        /* dp dm swap */
        regmap_set_bits(utmi, (0x5 << 2), BIT(13));
        dev_info(&phy->dev, "DP/DM swap\n");
    }
    else
    {
        dev_info(&phy->dev, "DP/DM no swap\n");
    }

    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

int sstar_u2phy_utmi_exit(struct phy *phy)
{
    return 0;
}

static int sstar_u2phy_on_disconnect(struct usb_phy *usb_phy, enum usb_device_speed speed)
{
    struct sstar_u2phy *u2phy = container_of(usb_phy, struct sstar_u2phy, usb_phy);
    struct phy *        phy   = u2phy->phy;
    const char *        s     = usb_speed_string(speed);

    dev_info(&phy->dev, "%s disconnect\n", s);
    if (phy)
    {
        sstar_u2phy_utmi_reset(phy);
        sstar_u2phy_utmi_power_off(phy);
        sstar_u2phy_utmi_power_on(phy);
    }
    return 0;
}

static int sstar_u2phy_on_connect(struct usb_phy *usb_phy, enum usb_device_speed speed)
{
    struct sstar_u2phy *u2phy = container_of(usb_phy, struct sstar_u2phy, usb_phy);
    struct phy *        phy   = u2phy->phy;
    const char *        s     = usb_speed_string(speed);

    dev_info(&phy->dev, "%s connect\n", s);
    return 0;
}

static const struct phy_ops sstar_u2phy_ops = {
    .init      = sstar_u2phy_utmi_init,
    .exit      = sstar_u2phy_utmi_exit,
    .power_on  = sstar_u2phy_utmi_power_on,
    .power_off = sstar_u2phy_utmi_power_off,
    .set_mode  = sstar_u2phy_utmi_set_mode,
    .reset     = sstar_u2phy_utmi_reset,
    .calibrate = sstar_u2phy_utmi_calibrate,
    .owner     = THIS_MODULE,
};

static struct usb2_phy_data m6p_phy_data = {
    .has_utmi2_bank = 1,
};

static const struct of_device_id sstar_u2phy_dt_ids[] = {{
                                                             .compatible = "sstar, u2phy",
                                                         },
                                                         {
                                                             .compatible = "sstar, mercury6p-u2phy",
                                                             .data       = &m6p_phy_data,
                                                         },
                                                         {}};
MODULE_DEVICE_TABLE(of, sstar_u2phy_dt_ids);

static int sstar_u2phy_probe(struct platform_device *pdev)
{
    // struct resource      res;
    struct device_node * np = pdev->dev.of_node;
    struct device_node * child_np;
    struct device_node * temp_np;
    struct phy *         phy;
    struct phy_provider *phy_provider;
    struct device *      dev = &pdev->dev;
    struct regmap *      syscon;
    /*void __iomem *       base;
    void __iomem *       ehc_base;
    void __iomem *       usbc_base;
    void __iomem *       bc_base;
    u64                  val;*/
    struct sstar_u2phy *       priv_phy;
    const struct of_device_id *of_match_id;

    of_match_id = of_match_device(sstar_u2phy_dt_ids, dev);
    if (!of_match_id)
    {
        dev_err(dev, "missing match\n");
        return -EINVAL;
    }

    if (!of_match_id->data)
    {
        dev_info(dev, "No extra phy data\n");
    }

    for_each_available_child_of_node(np, child_np)
    {
        priv_phy = devm_kzalloc(dev, sizeof(*priv_phy), GFP_KERNEL);
        if (!priv_phy)
        {
            dev_err(dev, "devm_kzalloc failed for phy private data.\n");
            return -ENOMEM;
        }

        temp_np = of_parse_phandle(child_np, "syscon-utmi", 0);
        if (temp_np)
        {
            syscon = syscon_node_to_regmap(temp_np);
            if (IS_ERR(syscon))
            {
                dev_err(dev, "failed to find utmi syscon regmap\n");
                return PTR_ERR(syscon);
            }
            priv_phy->utmi = syscon;
            of_node_put(temp_np);
        }

        temp_np = of_parse_phandle(child_np, "syscon-bc", 0);
        if (temp_np)
        {
            syscon = syscon_node_to_regmap(temp_np);
            if (IS_ERR(syscon))
            {
                dev_err(dev, "failed to find bc syscon regmap\n");
                return PTR_ERR(syscon);
            }
            priv_phy->bc = syscon;
            of_node_put(temp_np);
        }

        temp_np = of_parse_phandle(child_np, "syscon-usbc", 0);
        if (temp_np)
        {
            syscon = syscon_node_to_regmap(temp_np);
            if (IS_ERR(syscon))
            {
                dev_err(dev, "failed to find usbc syscon regmap\n");
                return PTR_ERR(syscon);
            }
            priv_phy->usbc = syscon;
            of_node_put(temp_np);
        }

        temp_np = of_parse_phandle(child_np, "syscon-uhc", 0);
        if (temp_np)
        {
            syscon = syscon_node_to_regmap(temp_np);
            if (IS_ERR(syscon))
            {
                dev_err(dev, "failed to find uhc syscon regmap\n");
                return PTR_ERR(syscon);
            }
            priv_phy->ehc = syscon;
            of_node_put(temp_np);
        }

        phy = devm_phy_create(dev, child_np, &sstar_u2phy_ops);
        if (IS_ERR(phy))
        {
            dev_err(dev, "failed to create PHY\n");
            return PTR_ERR(phy);
        }

        priv_phy->phy = phy;
        priv_phy->dev = dev;

        priv_phy->usb_phy.dev               = dev;
        priv_phy->usb_phy.notify_connect    = sstar_u2phy_on_connect;
        priv_phy->usb_phy.notify_disconnect = sstar_u2phy_on_disconnect;
        usb_add_phy_dev(&priv_phy->usb_phy);
        ATOMIC_INIT_NOTIFIER_HEAD(&priv_phy->usb_phy.notifier);
        priv_phy->phy_data = (struct usb2_phy_data *)of_match_id->data;
        phy_set_drvdata(phy, priv_phy);
    }

    phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
    return PTR_ERR_OR_ZERO(phy_provider);
}

static int sstar_u2phy_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver sstar_u2phy_driver = {
    .probe  = sstar_u2phy_probe,
    .remove = sstar_u2phy_remove,
    .driver =
        {
            .name           = "sstar-u2phy",
            .of_match_table = sstar_u2phy_dt_ids,
        },
};

module_platform_driver(sstar_u2phy_driver);

MODULE_ALIAS("platform:sstar-usb2-phy");
MODULE_AUTHOR("Zuhuang Zhang <zuhuang.zhang@sigmastar.com.cn>");
MODULE_DESCRIPTION("Sigmastar USB2 PHY driver");
MODULE_LICENSE("GPL v2");
