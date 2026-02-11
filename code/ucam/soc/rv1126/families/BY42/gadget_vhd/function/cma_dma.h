/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2021-12-08 16:56:45
 * @FilePath    : \gadget_vhd\function\cma_dma.h
 * @Description : 
 */
#ifndef UVC_CMA_H 
#define UVC_CMA_H

#include <asm/cacheflush.h>
#include <asm/memory.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
#include <linux/dma-contiguous.h>
#endif
#include <linux/dma-mapping.h>
#include <asm/memory.h>
#include <asm/tlbflush.h>
#include <asm/pgtable.h>
#include <linux/vmalloc.h>

struct cma_dma_dev {
    struct device *pdev;
    int init;
    struct mutex		lock;
    struct list_head 	buf_list;
};

struct cma_dma_buf {
    unsigned long long phys_addr;
    void *kvirt;
    unsigned long long length;
    struct list_head list;
};

extern struct cma_dma_dev * cma_dma_init(struct device *pdev);
extern int cma_dma_release(struct cma_dma_dev *dev);
extern unsigned long long cma_dma_buf_alloc(struct cma_dma_dev *dev, size_t size, gfp_t flag);
extern void cma_dma_buf_free(struct cma_dma_dev *dev, unsigned long long phys_addr);
extern void *cma_dma_buf_kvirt_getby_phys(struct cma_dma_dev *dev, unsigned long long phys_addr);

#endif