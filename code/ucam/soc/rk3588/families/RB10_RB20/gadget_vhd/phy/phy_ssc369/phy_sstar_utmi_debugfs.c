/*
 * phy_sstar_utmi_debugfs.c- Sigmastar
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ptrace.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include "phy_sstar_u3phy.h"
#include <io.h>

static int sstar_phy_utmi_cm_cur_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    cm_cur;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    cm_cur = INREG16(base + (0x17 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "cm_current = 0x%04x\r\n", cm_cur);
    bit_masks = BIT(3) | BIT(4) | BIT(5);
    cm_cur    = (bit_masks & cm_cur) >> 3;
    switch (cm_cur)
    {
        case 0x03:
            cm_cur = 105;
            break;
        case 0x02:
            cm_cur = 110;
            break;
        case 0x01:
        case 0x07:
            cm_cur = 115;
            break;
        case 0x00:
        case 0x06:
            cm_cur = 120;
            break;
        case 0x05:
            cm_cur = 125;
            break;
        case 0x04:
            cm_cur = 130;
            break;
        default:
            cm_cur = 105;
    }
    seq_printf(s, "cm_current: %d%%\r\n", cm_cur);
    return 0;
}

static int sstar_phy_utmi_cm_cur_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_cm_cur_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_cm_cur_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    cm_cur;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &cm_cur);
    if (ret)
    {
        return ret;
    }

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = BIT(3) | BIT(4) | BIT(5);
    CLRREG16(base + (0x17 << 2), bit_masks);

    switch (cm_cur)
    {
        case 105:
            cm_cur = 0x03;
            break;
        case 110:
            cm_cur = 0x02;
            break;
        case 115:
            cm_cur = 0x01;
            break;
        case 120:
            cm_cur = 0x00;
            break;
        case 125:
            cm_cur = 0x05;
            break;
        case 130:
            cm_cur = 0x04;
            break;
        default:
            cm_cur = 0x03;
    }

    SETREG16(base + (0x17 << 2), bit_masks & (cm_cur << 3));
    seq_printf(s, "cm_current = 0x%04x\r\n", bit_masks & (cm_cur << 3));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_cm_cur_fops = {
    .open    = sstar_phy_utmi_cm_cur_open,
    .write   = sstar_phy_utmi_cm_cur_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_dem_cur_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    dem_cur;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    dem_cur = INREG16(base + (0x16 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "de_emphasis_current = 0x%04x\r\n", dem_cur);
    bit_masks = (BIT(7) | BIT(8) | BIT(9));
    dem_cur   = (bit_masks & dem_cur) >> 7;
    switch (dem_cur)
    {
        case 0x04:
            dem_cur = 105;
            break;
        case 0x05:
            dem_cur = 110;
            break;
        case 0x06:
            dem_cur = 115;
            break;
        case 0x07:
            dem_cur = 120;
            break;
        default:
            dem_cur = 100;
    }
    seq_printf(s, "de_emphasis_current: %d%%\r\n", dem_cur);
    return 0;
}

static int sstar_phy_utmi_dem_cur_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_dem_cur_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_dem_cur_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    dem_cur;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &dem_cur);
    if (ret)
    {
        return ret;
    }

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = (BIT(7) | BIT(8) | BIT(9));
    CLRREG16(base + (0x16 << 2), bit_masks);
    switch (dem_cur)
    {
        case 100:
            dem_cur = 0x00;
            break;
        case 105:
            dem_cur = 0x04;
            break;
        case 110:
            dem_cur = 0x05;
            break;
        case 115:
            dem_cur = 0x06;
            break;
        case 120:
            dem_cur = 0x07;
            break;
        default:
            dem_cur = 0x11;
    }
    SETREG16(base + (0x16 << 2), bit_masks & (dem_cur << 7));
    seq_printf(s, "de_emphasis_current = 0x%04x\r\n", bit_masks & (dem_cur << 7));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_dem_cur_fops = {
    .open    = sstar_phy_utmi_dem_cur_open,
    .write   = sstar_phy_utmi_dem_cur_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int sstar_phy_utmi_tx_swing_show(struct seq_file *s, void *unused)
{
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    u32                    tx_swing;
    int                    bit_masks;

    spin_lock_irqsave(&port->lock, flags);
    tx_swing = INREG16(base + (0x16 << 2));
    spin_unlock_irqrestore(&port->lock, flags);
    seq_printf(s, "tx_swing = 0x%04x\r\n", tx_swing);
    bit_masks = (BIT(4) | BIT(5) | BIT(6));
    tx_swing  = (bit_masks & tx_swing) >> 4;

    switch (tx_swing)
    {
        case 0x04:
            tx_swing = 80;
            break;
        case 0x05:
            tx_swing = 85;
            break;
        case 0x06:
            tx_swing = 90;
            break;
        case 0x07:
            tx_swing = 95;
            break;
        case 0x00:
            tx_swing = 100;
            break;
        case 0x01:
            tx_swing = 105;
            break;
        case 0x02:
            tx_swing = 110;
            break;
        case 0x03:
            tx_swing = 115;
            break;
        default:
            tx_swing = 110;
    }

    seq_printf(s, "tx_swing: %d%%\r\n", tx_swing);
    return 0;
}

static int sstar_phy_utmi_tx_swing_open(struct inode *inode, struct file *file)
{
    return single_open(file, sstar_phy_utmi_tx_swing_show, inode->i_private);
}

static ssize_t sstar_phy_utmi_tx_swing_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    struct seq_file *      s    = file->private_data;
    struct sstar_phy_port *port = s->private;
    void __iomem *         base = port->reg;
    unsigned long          flags;
    char                   buf[32] = {0};
    u32                    tx_swing;
    int                    ret;
    int                    bit_masks;

    if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
        return -EFAULT;

    ret = kstrtouint(buf, 0, &tx_swing);
    if (ret)
    {
        return ret;
    }

    switch (tx_swing)
    {
        case 80:
            tx_swing = 0x04;
            break;
        case 85:
            tx_swing = 0x05;
            break;
        case 90:
            tx_swing = 0x06;
            break;
        case 95:
            tx_swing = 0x07;
            break;
        case 100:
            tx_swing = 0x00;
            break;
        case 105:
            tx_swing = 0x01;
            break;
        case 110:
            tx_swing = 0x02;
            break;
        case 115:
            tx_swing = 0x03;
            break;
        default:
            tx_swing = 0x02;
    }

    spin_lock_irqsave(&port->lock, flags);
    bit_masks = (BIT(4) | BIT(5) | BIT(6));
    CLRREG16(base + (0x16 << 2), bit_masks);
    SETREG16(base + (0x16 << 2), bit_masks & (tx_swing << 4));
    seq_printf(s, "tx_swing = 0x%04x\r\n", bit_masks & (tx_swing << 4));
    spin_unlock_irqrestore(&port->lock, flags);

    return count;
}

static const struct file_operations sstar_tx_swing_fops = {
    .open    = sstar_phy_utmi_tx_swing_open,
    .write   = sstar_phy_utmi_tx_swing_write,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};

void sstar_phy_utmi_debugfs_init(struct sstar_phy_port *port)
{
    spin_lock_init(&port->lock);
    port->root = debugfs_create_dir(dev_name(port->dev), usb_debug_root);

    debugfs_create_file("tx_swing", 0644, port->root, port, &sstar_tx_swing_fops);
    debugfs_create_file("de_emphasis_current", 0644, port->root, port, &sstar_dem_cur_fops);
    debugfs_create_file("cm_current", 0644, port->root, port, &sstar_cm_cur_fops);
}

void sstar_phy_utmi_debugfs_exit(struct sstar_phy_port *port)
{
    debugfs_remove_recursive(port->root);
}
