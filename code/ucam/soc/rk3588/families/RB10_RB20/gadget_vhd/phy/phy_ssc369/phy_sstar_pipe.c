/*
 * phy_sstar_pipe.c- Sigmastar
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
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/module.h>
//#include <linux/resource.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
//#include <registers.h>
#include <io.h>
#include "phy_sstar_u3phy.h"
#include "phy_sstar_port.h"
#include "phy_sstar_pipe_reg.h"
#include "phy_sstar_pipe_debugfs.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

extern int Get_Testbus_Count(int regEE_test_bus24b_sel, int regEA_clk_out_sel, int regEC_single_clk_out_sel);
static int tx_r50, rx_r50, tx_ibias;

static void sstar_pipe_show_trim_value(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;

    tx_r50   = INREG16(pipe_phyd + (0x2a << 2)) & 0x0f80;
    rx_r50   = INREG16(pipe_phyd + (0x3c << 2)) & 0x01f0;
    tx_ibias = INREG16(pipe_phyd + (0x50 << 2)) & 0xfc00;
    dev_info(port->dev, "%s tx_r50 = 0x%04x\n", __func__, tx_r50);
    dev_info(port->dev, "%s rx_r50 = 0x%04x\n", __func__, rx_r50);
    dev_info(port->dev, "%s tx_ibias = 0x%04x\n", __func__, tx_ibias);
}

static void sstar_pipe_writeback_trim_value(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;

    SETREG16(pipe_phyd + (0x0b << 2), BIT(4));
    SETREG16(pipe_phyd + (0x0e << 2), BIT(2));
    SETREG16(pipe_phyd + (0x46 << 2), BIT(13));
    CLRSETREG16(pipe_phyd + (0x2a << 2), 0x0f80, tx_r50);
    CLRSETREG16(pipe_phyd + (0x3c << 2), 0x01f0, rx_r50);
    CLRSETREG16(pipe_phyd + (0x50 << 2), 0xfc00, tx_ibias);
    dev_info(port->dev, "%s tx_r50 = 0x%04x\n", __func__, tx_r50);
    dev_info(port->dev, "%s rx_r50 = 0x%04x\n", __func__, rx_r50);
    dev_info(port->dev, "%s tx_ibias = 0x%04x\n", __func__, tx_ibias);
}

void sstar_pipe_tx_polarity_inv_disabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    // CLRREG16(base + PHYD_REG_RG_TX_POLARTY_INV, BIT_TX_POLARTY_INV_EN);
    OUTREG16(pipe_phyd + (0x12 << 2), 0x420f);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

void sstar_pipe_LPFS_det_hw_mode(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd  = port->reg;
    void __iomem *pipe_phya0 = pipe_phyd + (0x200 * 1);
    CLRREG16(pipe_phyd + (0xc << 2), BIT(15));
    OUTREG16(pipe_phya0 + (0x3 << 2), 0x89ac);
    dev_dbg(port->dev, "%s completed.\n", __func__);
}

void sstar_pipe_LPFS_det_power_on(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd  = port->reg;
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    SETREG16(pipe_phyd + (0xc << 2), BIT(15));
    CLRREG16(pipe_phyd + (0x34 << 2), BIT(14));                    // RG_SSUSB_LFPS_PWD[14] = 0 // temp add here
    CLRSETREG16(pipe_phyd + (0x65 << 2), BIT(8) | BIT(9), 0x0300); // low pass filter strength
    SETREG16(pipe_phya1 + (0x53 << 2), BIT(5));
    dev_dbg(port->dev, "%s completed.\n", __func__);
}

void sstar_pipe_hw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    CLRREG16(pipe_phya1 + (0x00 << 2), BIT(0));
    udelay(10);
    SETREG16(pipe_phya1 + (0x00 << 2), BIT(0));
    dev_dbg(port->dev, "%s completed.\n", __func__);
}

void sstar_pipe_assert_sw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    CLRREG16(pipe_phya1 + (0x00 << 2), BIT(4));
    dev_dbg(port->dev, "%s, completed.\n", __func__);
}

void sstar_pipe_deassert_sw_reset(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);
    SETREG16(pipe_phya1 + (0x00 << 2), BIT(4));
    dev_dbg(port->dev, "%s completed.\n", __func__);
}

void sstar_pipe_sw_reset(struct sstar_phy_port *port)
{
    sstar_pipe_assert_sw_reset(port);
    udelay(10);
    sstar_pipe_deassert_sw_reset(port);
    dev_dbg(port->dev, "%s completed.\n", __func__);
}

void sstar_pipe_synthesis_enabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);

    CLRREG16(pipe_phya0 + (0x44 << 2), 0x0001); // synth clk enabled
    CLRREG16(pipe_phya0 + (0x44 << 2), 0x0001);
    OUTREG16(pipe_phya0 + (0x41 << 2), 0x002c); // synth clk
    OUTREG16(pipe_phya0 + (0x40 << 2), 0x3c9e); // synth clk
    SETREG16(pipe_phya0 + (0x44 << 2), 0x0001); // synth clk enabled
    // CLRREG16(pipe_phya0 + (0x44 << 2), 0x0001);
    dev_dbg(port->dev, "%s syn clk =  0x002c3c9e\n", __func__);
}

void sstar_pipe_tx_loop_div(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    OUTREG16(pipe_phya0 + (0x21 << 2), 0x0010); // txpll loop div second
    dev_dbg(port->dev, "%s, pipe_phya0\n", __func__);
}

void sstar_pipe_sata_test(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    OUTREG16(pipe_phya0 + (0x11 << 2), 0x2a00); // gcr_sata_test
    OUTREG16(pipe_phya0 + (0x10 << 2), 0x0000); // gcr_sata_test
    dev_dbg(port->dev, "%s, pipe_phya0\n", __func__);
}

void sstar_pipe_EQ_rstep(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    OUTREG16(pipe_phyd + (0x42 << 2), 0x5582);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

void sstar_pipe_DFE(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x0d << 2), 0x0);
    OUTREG16(pipe_phya2 + (0x0e << 2), 0x0);
    OUTREG16(pipe_phya2 + (0x0f << 2), 0x0171);
    dev_dbg(port->dev, "%s, sstar_pipe_DFE\n", __func__);
}

void sstar_sata_rx_tx_pll_frq_det_enabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    CLRREG16(pipe_phya0 + 0x54 * 4, 0xf); // The continue lock number to judge rxpll frequency is lock or not
    CLRREG16(pipe_phya0 + 0x73 * 4,
             0xff); // PLL State :
                    // The lock threshold distance between predict counter number and real counter number of rxpll
    CLRREG16(pipe_phya0 + 0x77 * 4,
             0xff); // CDR State :
                    // The lock threshold distance between predict counter number and real counter number of rxpll
    CLRREG16(pipe_phya0 + 0x56 * 4, 0xffff);
    SETREG16(pipe_phya0 + 0x56 * 4, 0x5dc0); // the time out reset reserve for PLL unlock for cdr_pd fsm PLL_MODE state
                                             // waiting time default : 20us  (24MHz base : 480 * 41.667ns)

    CLRREG16(pipe_phya0 + 0x57 * 4, 0xffff);
    SETREG16(pipe_phya0 + 0x57 * 4, 0x1e0); // the time out reset reserve for CDR unlock

    CLRREG16(pipe_phya0 + 0x70 * 4, 0x04); // reg_sata_phy_rxpll_det_hw_mode_always[2] = 0
    // Enable RXPLL frequency lock detection
    SETREG16(pipe_phya0 + 0x70 * 4, 0x02); // reg_sata_phy_rxpll_det_sw_enable_always[1] = 1
    // Enable TXPLL frequency lock detection
    SETREG16(pipe_phya0 + 0x60 * 4, 0x02);   // reg_sata_phy_txpll_det_sw_enable_always[1] = 1
    SETREG16(pipe_phya0 + 0x50 * 4, 0x2000); // cdr state change to freq_unlokc_det[13] = 1
    dev_dbg(port->dev, "%s, pipe_phya0\n", __func__);
}

void sstar_pipe_txpll_vco_ldo(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x17 << 2), 0x0180);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_txpll_en_d2s(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x1c << 2), 0x000f);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_txpll_en_vco(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x1b << 2), 0x0100);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_txpll_ictrl_new(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x15 << 2), 0x0020);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_rxpll_en_d2s(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x21 << 2), 0x000f);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_rxpll_ctrk_r2(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x08 << 2), 0x0000);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_rxpll_d2s_ldo_ref(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x06 << 2), 0x6060);
    dev_dbg(port->dev, "%s, pipe_phya2\n", __func__);
}

void sstar_pipe_rxpll_vote_sel(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x05 << 2), 0x0200);
    dev_dbg(port->dev, "%s\n", __func__);
}
void sstar_pipe_rx_ictrl(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    OUTREG16(pipe_phya0 + (0x09 << 2), 0x0002);
    dev_dbg(port->dev, "%s, pipe_phya0\n", __func__);
}

void sstar_pipe_rx_ictrl_pll(void __iomem *base)
{
    OUTREG16(base + (0x30 << 2), 0x100f);
}

void sstar_pipe_rx_div_setting(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    // OUTREG16(base + PHYA0_REG_SATA_RXPLL, 0x100e); // rxoll_lop_div_first
    OUTREG16(pipe_phya0 + (0x30 << 2), 0x1009);
    OUTREG16(pipe_phya0 + (0x31 << 2), 0x0005); // rxpll_loop_div_second
    OUTREG16(pipe_phya0 + (0x33 << 2), 0x0505);
    dev_dbg(port->dev, "%s, pipe_phya0\n", __func__);
}

void sstar_pipe_rxpll_ictrl_CDR(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x09 << 2), 0x0001);
    dev_dbg(port->dev, "%s\n", __func__);
}

void sstar_pipe_rxpll_vco_ldo_voltage(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG16(pipe_phya2 + (0x06 << 2), 0x6060);
    dev_dbg(port->dev, "%s\n", __func__);
}

__maybe_unused static void sstar_pipe_force_IDRV_6DB(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    SETREG16(pipe_phyd + 0xa * 4, 0x0020); // IDRV_6DB register force setting
    CLRREG16(pipe_phyd + (0x41 << 2), 0x00fc);
    SETREG16(pipe_phyd + (0x41 << 2), 0x00C8); // IDRV_6DB register setting
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

static void sstar_phy_pipe_tx_swing_and_de_emphasis(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd, *pipe_phya2;
    int           tx_swing[3];

    pipe_phyd  = port->reg;
    pipe_phya2 = port->reg + (0x200 * 3);

    if (device_property_read_u32_array(port->dev, "sstar,tx-swing-and-de-emphasis", tx_swing, ARRAY_SIZE(tx_swing)))
    {
        return;
    }

    SETREG16(pipe_phyd + (0x0a << 2), BIT(5) | BIT(6));
    CLRSETREG16(pipe_phyd + (0x41 << 2), 0x00fc, 0x00fc & (tx_swing[0] << 2));
    CLRSETREG16(pipe_phyd + (0x41 << 2), 0x3f00, 0x3f00 & (tx_swing[1] << 8));

    SETREG16(pipe_phyd + (0x0a << 2), BIT(0) | BIT(1));
    SETREG16(pipe_phya2 + (0x0f << 2), BIT(12));
    CLRSETREG16(pipe_phya2 + (0x11 << 2), 0xf000, 0xf000 & (tx_swing[2] << 12));

    // SETREG16(pipe_phya2 + (0x2f << 2), BIT(4));
    dev_dbg(port->dev, "%s\n", __func__);
}

void sstar_pipe_tx_swing_setting(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    // sstar_pipe_force_IDRV_6DB(port);

    OUTREG16(pipe_phyd + (0x2f << 2), 0x241d);

    // SETREG16(pipe_phyd + (0x0b << 2), 0x0050);

    // CLRREG16(pipe_phyd + (0x2a << 2), BITS_TX_RTERM(0x1F));
    // SETREG16(pipe_phyd + (0x2a << 2), BITS_TX_RTERM(0x10));
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}
/*
void sstar_pipe_tx_ibiasi(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya2 = port->reg + (0x200 * 3);
    OUTREG8(pipe_phya2 + ((0x11 << 2) + 1), 0x70);
    dev_dbg(port->dev, "%s\n", __func__);
}
*/
void sstar_pipe_ibias_trim(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    OUTREG16(pipe_phyd + (0x46 << 2), 0x2802);
    OUTREG16(pipe_phyd + (0x50 << 2), 0x0300);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

void sstar_pipe_rx_imp_sel(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    OUTREG16(pipe_phyd + (0x0e << 2), 0x8904);
    OUTREG16(pipe_phyd + (0x3c << 2), 0x0100);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

void sstar_pipe_recal(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd = port->reg;
    SETREG16(pipe_phyd + (0x10 << 2), 0x2000);
    mdelay(1);
    CLRREG16(pipe_phyd + (0x10 << 2), 0x2000);
    dev_dbg(port->dev, "%s, pipe_phyd\n", __func__);
}

void sstar_pipe_set_phya1(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya1 = port->reg + (0x200 * 2);

    SETREG16(pipe_phya1 + 0x20 * 4, 0x04); // guyo
    CLRREG16(pipe_phya1 + 0x25 * 4, 0xffff);

    CLRREG16(pipe_phya1 + 0x49 * 4, 0x200);
    SETREG16(pipe_phya1 + 0x49 * 4, 0xc4e); // reg_rx_lfps_t_burst_gap = 3150
    dev_dbg(port->dev, "%s, pipe_phya1\n", __func__);
}

void sstar_pipe_eco_enabled(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);

    // Enable ECO
    CLRREG16(pipe_phya0 + 0x03 * 4, 0x0f);
    SETREG16(pipe_phya0 + 0x03 * 4, 0x0d);
    dev_dbg(port->dev, "%s, pipe_phya0\n", __func__);
}

void sstar_pipe_ssc(struct sstar_phy_port *port)
{
    void __iomem *pipe_phya0 = port->reg + (0x200 * 1);
    // U3 S(spread)S(spectrum)C(clock) setting
    dev_dbg(port->dev, "%s SSC\n", __func__);
    OUTREG16(pipe_phya0 + (0x40 << 2), 0x55ff);
    OUTREG16(pipe_phya0 + (0x41 << 2), 0x002c);
    OUTREG16(pipe_phya0 + (0x42 << 2), 0x000a);
    OUTREG16(pipe_phya0 + (0x43 << 2), 0x0271);
    OUTREG16(pipe_phya0 + (0x44 << 2), 0x0001);
    dev_dbg(port->dev, "%s, U3 SSC\n", __func__);
}

void sstar_pipe_tx_current(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd, *pipe_phya0, *pipe_phya2;

    pipe_phyd  = port->reg;
    pipe_phya0 = port->reg + (0x200 * 1);
    pipe_phya2 = port->reg + (0x200 * 3);

    SETREG16(pipe_phyd + (0x0a << 2), BIT(6));

    CLRREG16(pipe_phyd + (0x41 << 2), 0x00fc);
    SETREG16(pipe_phyd + (0x41 << 2), 0x00c0);
    CLRREG16(pipe_phyd + (0x41 << 2), 0x3f00);
    SETREG16(pipe_phyd + (0x41 << 2), 0x2a00);

    CLRREG16(pipe_phya2 + (0x11 << 2), 0xf000);
    SETREG16(pipe_phya2 + (0x11 << 2), 0xc000);

    SETREG16(pipe_phya2 + (0x2f << 2), BIT(4));
    dev_dbg(port->dev, "%s enhance transmitter swing\n", __func__);
}

static void sstar_pipe_param_init(struct sstar_phy_port *port)
{
    void __iomem *pipe_phyd, *pipe_phya0, *pipe_phya1, *pipe_phya2;

    pipe_phyd  = port->reg;
    pipe_phya0 = pipe_phyd + (0x200 * 1);
    pipe_phya1 = pipe_phyd + (0x200 * 2);
    pipe_phya2 = pipe_phyd + (0x200 * 3);

    /* EQ default */
    CLRREG16(pipe_phya1 + (0x51 << 2), (BIT(5) - 1));
    SETREG16(pipe_phya1 + (0x51 << 2), BIT(4));
    sstar_pipe_synthesis_enabled(port);
    sstar_pipe_eco_enabled(port);
    sstar_pipe_tx_polarity_inv_disabled(port);
    sstar_pipe_tx_loop_div(port);
    sstar_pipe_sata_test(port);
    sstar_pipe_rx_div_setting(port);
    sstar_pipe_rxpll_ictrl_CDR(port);
    sstar_pipe_rxpll_vco_ldo_voltage(port);
    sstar_pipe_EQ_rstep(port);
    sstar_pipe_DFE(port);
    sstar_pipe_rxpll_d2s_ldo_ref(port);
    sstar_pipe_rxpll_vote_sel(port);
    sstar_pipe_rx_ictrl(port);
    sstar_pipe_txpll_vco_ldo(port);
    sstar_pipe_txpll_en_d2s(port);
    // sstar_pipe_txpll_en_vco(port);
    sstar_pipe_rxpll_en_d2s(port);
    sstar_pipe_txpll_ictrl_new(port);
    sstar_pipe_rxpll_ctrk_r2(port);
    sstar_pipe_tx_swing_setting(port);
    // sstar_pipe_tx_ibiasi(port);
    sstar_sata_rx_tx_pll_frq_det_enabled(port);
    sstar_pipe_set_phya1(port);
    SETREG16(pipe_phya0 + (0x58 * 4), 0x0003);
    sstar_phy_pipe_tx_swing_and_de_emphasis(port);

    /* for I7U02 LFPS setting*/
    CLRSETREG16(pipe_phya2 + (0x2f << 2), BIT(8) | BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3), 0x28 << 3);
    // sstar_pipe_deassert_sw_reset(port);
    // sstar_pipe_LPFS_det_hw_mode(port);
    sstar_pipe_LPFS_det_hw_mode(port);
    dev_dbg(port->dev, "%s completed.\n", __func__);
}

static int sstar_pipe_init(struct phy *phy)
{
    struct sstar_phy_port *port = phy_get_drvdata(phy);

    sstar_pipe_show_trim_value(port);
    sstar_pipe_hw_reset(port);
    sstar_pipe_param_init(port);
    sstar_pipe_synthesis_enabled(port);
    // udelay(200);
    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

static int sstar_pipe_exit(struct phy *phy)
{
    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

static int sstar_pipe_reset(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);

    void __iomem *pipe_phya0 = u3phy_port->reg + (0x200 * 1);
    void __iomem *pipe_phya2 = u3phy_port->reg + (0x200 * 3);

    unsigned long flags;

    CLRREG16(pipe_phya2 + (0x16 << 2), BIT(14)); // txpll disabeld
    SETREG16(pipe_phya0 + (0x20 << 2), BIT(0));
    CLRREG16(pipe_phya2 + (0x1b << 2), BIT(8));

    CLRREG16(pipe_phya0 + (0x15 << 2), BIT(5)); // rxpll disabeld
    SETREG16(pipe_phya0 + (0x30 << 2), BIT(0));

    sstar_pipe_assert_sw_reset(u3phy_port);

    SETREG16(pipe_phya2 + (0x16 << 2), BIT(14)); // txpll en
    OUTREG16(pipe_phya0 + (0x20 << 2), 0x0100);
    spin_lock_irqsave(&u3phy_port->lock, flags);
    sstar_pipe_deassert_sw_reset(u3phy_port);
    udelay(10);
    SETREG16(pipe_phya2 + (0x1b << 2), BIT(8));
    SETREG16(pipe_phya0 + (0x15 << 2), BIT(5)); // rxpll en
    OUTREG16(pipe_phya0 + (0x30 << 2), 0x100e);
    SETREG16(pipe_phya0 + (0x14 << 2), BIT(15));
    udelay(10);
    CLRREG16(pipe_phya0 + (0x14 << 2), BIT(15));
    spin_unlock_irqrestore(&u3phy_port->lock, flags);
    udelay(200);

    dev_info(&phy->dev, "%s completed.\n", __func__);

    return 0;
}

__maybe_unused static int sstar_pipe_check_pll(struct sstar_phy_port *u3phy_port)
{
    void __iomem *pipe_phya0 = u3phy_port->reg + (0x200 * 1);
    int           freq;

    // Select USB30 SATA_TX500M_CK to debug bus test clock
    OUTREG16(pipe_phya0 + (0x0d << 2), 0x0800); // Select debug signal : test clock
    // Select test clock source :
    // 0x02 : CLKO_SATA_TX250M_CK
    // 0x03 : CLKO_SATA_LN0_RX500M_CK
    CLRREG16(pipe_phya0 + (0x0c << 2), 0x07);
    SETREG16(pipe_phya0 + (0x0c << 2), 0x03); // Select test clock source

    freq = Get_Testbus_Count(0x1b, 0x67, 0x01);

    // Drive CLKO_SATA_LN0_RX500M_CK clock to pad.
    // CLKO_SATA_LN0_RX500M_CK = 500 MHz  / 64
    //  reg_calc_cnt_report = (500M/64) / 12M x 1000 x0.95 ~ (500M/64) / 12M x 1000 x 1.05 = 618~684
    if (618 > freq || 684 < freq)
    {
        dev_err(u3phy_port->dev, "error! clac cnt report = %d.\n", freq);
        return -1;
    }
    else
        dev_info(u3phy_port->dev, "clac cnt report = %d.\n", freq);
    return 0;
}

static int sstar_pipe_power_on(struct phy *phy)
{
    int                    retry = 0;
    unsigned long          flags;
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);

    void __iomem *pipe_phya0 = u3phy_port->reg + (0x200 * 1);
    void __iomem *pipe_phya2 = u3phy_port->reg + (0x200 * 3);

retry:
    retry++;
    sstar_pipe_writeback_trim_value(u3phy_port);
    // sstar_pipe_LPFS_det_power_on(u3phy_port);
    SETREG16(pipe_phya2 + (0x16 << 2), BIT(14)); // txpll en
    OUTREG16(pipe_phya0 + (0x20 << 2), 0x0100);
    spin_lock_irqsave(&u3phy_port->lock, flags);
    sstar_pipe_deassert_sw_reset(u3phy_port);
    udelay(10);
    SETREG16(pipe_phya2 + (0x1b << 2), BIT(8));
    SETREG16(pipe_phya0 + (0x15 << 2), BIT(5)); // rxpll en
    OUTREG16(pipe_phya0 + (0x30 << 2), 0x100e);
    SETREG16(pipe_phya0 + (0x14 << 2), BIT(15));
    udelay(10);
    CLRREG16(pipe_phya0 + (0x14 << 2), BIT(15));
    spin_unlock_irqrestore(&u3phy_port->lock, flags);
    udelay(200);
    dev_info(&phy->dev, "%s\n", __func__);

    if (0 == sstar_pipe_check_pll(u3phy_port))
    {
        return 0;
    }

    if (20 != retry)
    {
        sstar_pipe_hw_reset(u3phy_port);
        sstar_pipe_param_init(u3phy_port);
        sstar_pipe_synthesis_enabled(u3phy_port);
        goto retry;
    }

    return -1;
}

static int sstar_pipe_power_off(struct phy *phy)
{
    struct sstar_phy_port *u3phy_port = phy_get_drvdata(phy);

    void __iomem *pipe_phya0, *pipe_phya2;
    pipe_phya0 = u3phy_port->reg + (0x200 * 1);
    pipe_phya2 = u3phy_port->reg + (0x200 * 3);

    CLRREG16(pipe_phya2 + (0x16 << 2), BIT(14)); // txpll en
    SETREG16(pipe_phya0 + (0x20 << 2), BIT(0));
    CLRREG16(pipe_phya2 + (0x1b << 2), BIT(8));

    CLRREG16(pipe_phya0 + (0x15 << 2), BIT(5)); // rxpll en
    SETREG16(pipe_phya0 + (0x30 << 2), BIT(0));

    // CLRREG16(pipe_phya2 + (0x1C << 2), 0x0F);
    // CLRREG16(pipe_phya2 + (0x1B << 2), BIT(8));
    // CLRREG16(pipe_phya2 + (0x21 << 2), 0x0F);

    dev_info(&phy->dev, "%s\n", __func__);
    return 0;
}

const struct phy_ops sstar_pipe_ops = {
    .init      = sstar_pipe_init,
    .exit      = sstar_pipe_exit,
    .power_on  = sstar_pipe_power_on,
    .power_off = sstar_pipe_power_off,
    .reset     = sstar_pipe_reset,
    .owner     = THIS_MODULE,
};

static void sstar_pipe_event(struct work_struct *work)
{
    struct sstar_phy_port *phy_port   = container_of(work, struct sstar_phy_port, events);
    void __iomem *         pipe_phya1 = phy_port->reg + (0x200 * 2);

    dev_info(phy_port->dev, "rx_symblk_rx_valid = 0x%02x\r\n", INREG16(pipe_phya1 + (0x61 << 2)) & 0x1000);
    dev_info(phy_port->dev, "rx_symblk_fail_cnt = 0x%02x\r\n", INREG16(pipe_phya1 + (0x62 << 2)) & 0x00f);
    dev_info(phy_port->dev, "rx_symblk_lock_cnt = 0x%02x\r\n", INREG16(pipe_phya1 + (0x62 << 2)) & 0x03f0);
    dev_info(phy_port->dev, "rx_symblk_8b_10b_cerr = 0x%02x\r\n", INREG16(pipe_phya1 + (0x62 << 2)) & 0x2000);
    dev_info(phy_port->dev, "rx_symblk_cerr_cnt = 0x%02x\r\n", INREG16(pipe_phya1 + (0x63 << 2)) & 0x001f);
}

int sstar_pipe_port_init(struct device *dev, struct sstar_phy_port *phy_port, struct device_node *np)
{
    int ret;

    phy_port->speed = USB_SPEED_SUPER;

    if (0 == (ret = sstar_port_init(dev, np, &sstar_pipe_ops, phy_port)))
    {
        INIT_WORK(&phy_port->events, sstar_pipe_event);
        sstar_phy_pipe_debugfs_init(phy_port);
    }

    return ret;
}
EXPORT_SYMBOL(sstar_pipe_port_init);

int sstar_pipe_port_remove(struct sstar_phy_port *phy_port)
{
    sstar_phy_pipe_debugfs_exit(phy_port);
    return sstar_port_deinit(phy_port);
}
EXPORT_SYMBOL(sstar_pipe_port_remove);
