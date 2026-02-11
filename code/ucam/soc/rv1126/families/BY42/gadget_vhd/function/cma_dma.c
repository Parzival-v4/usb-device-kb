/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2021-12-08 16:56:00
 * @FilePath    : \gadget_vhd\function\cma_dma.c
 * @Description : 
 */
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "cma_dma.h"


unsigned long long cma_dma_buf_alloc(struct cma_dma_dev *dev, size_t size, gfp_t flag)
{
    unsigned long long phys_addr = 0;
    void *kvirt = NULL;
    struct cma_dma_buf *buf = NULL;

    if(!dev)
    {
        return -1;
    }

    mutex_lock(&dev->lock);
    if(dev->init == 0)
    {
        mutex_unlock(&dev->lock);
        return 0;
    }

    kvirt = dma_alloc_coherent(dev->pdev, size,
                (dma_addr_t *)&phys_addr, flag);
    if(kvirt == NULL)
    {
        pr_err("dma_alloc_coherent failed.\n");
        mutex_unlock(&dev->lock);
        return 0;
    }

    buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (buf == NULL)
    {
        dma_free_coherent(dev->pdev, size,
            kvirt, (dma_addr_t)phys_addr);
        mutex_unlock(&dev->lock);
        return 0;
    }
	
    	
    buf->phys_addr = phys_addr;
    buf->kvirt = kvirt;
    buf->length = size;
    INIT_LIST_HEAD(&buf->list);
    
    
    list_add_tail(&buf->list, &dev->buf_list);
    mutex_unlock(&dev->lock);

    return phys_addr;
}
EXPORT_SYMBOL(cma_dma_buf_alloc);

void cma_dma_buf_free(struct cma_dma_dev *dev, unsigned long long phys_addr)
{
    struct cma_dma_buf *buf = NULL;
    struct cma_dma_buf *find_buf = NULL;

    if(!dev)
    {
        return;
    }

    mutex_lock(&dev->lock);
    if(dev->init == 0)
    {
        mutex_unlock(&dev->lock);
        return;
    }

    
    list_for_each_entry(buf, &dev->buf_list, list)
    {
        if(buf)
        { 
            if(buf->phys_addr == phys_addr)
            {
                find_buf = buf;
                break;
            }
        }
    }
    

    if(find_buf == NULL)
    {
        mutex_unlock(&dev->lock);
        return;
    }

    if(find_buf->phys_addr == 0 || find_buf->kvirt == NULL)
    {
        mutex_unlock(&dev->lock);
        return;
    }

    dma_free_coherent(dev->pdev, find_buf->length,
        find_buf->kvirt, (dma_addr_t)find_buf->phys_addr);
        
    find_buf->phys_addr = 0;
    find_buf->kvirt = NULL;
    list_del(&find_buf->list);
    kfree(find_buf);
    mutex_unlock(&dev->lock);
    find_buf = NULL;
}
EXPORT_SYMBOL(cma_dma_buf_free);

void *cma_dma_buf_kvirt_getby_phys(struct cma_dma_dev *dev, unsigned long long phys_addr)
{
    struct cma_dma_buf *buf = NULL;
    struct cma_dma_buf *find_buf = NULL;

    if(!dev)
    {
        return NULL;
    }

    mutex_lock(&dev->lock);

    if(dev->init == 0)
    {
        mutex_unlock(&dev->lock);
        return NULL;
    }

    
    list_for_each_entry(buf, &dev->buf_list, list)
    {
        if(buf)
        { 
            if(buf->phys_addr == phys_addr)
            {
                find_buf = buf;
                break;
            }
        }
    }
    mutex_unlock(&dev->lock);

    if(find_buf == NULL)
        return NULL;
    
    return find_buf->kvirt;
}
EXPORT_SYMBOL(cma_dma_buf_kvirt_getby_phys);

struct cma_dma_dev * cma_dma_init(struct device *pdev)
{
    struct cma_dma_dev *dev;

    if(pdev == NULL)
    {
        return NULL;
    }

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL)
    {
        return NULL;
    }

    dev->pdev = pdev;
    mutex_init(&dev->lock);
    INIT_LIST_HEAD(&dev->buf_list);
    dev->init = 1;

    return dev;
}
EXPORT_SYMBOL(cma_dma_init);

int cma_dma_release(struct cma_dma_dev *dev)
{
    struct cma_dma_buf *buf, *next;

    if(!dev)
    {
        return -1;
    }
    
    mutex_lock(&dev->lock);
    dev->init = 0;
	list_for_each_entry_safe(buf, next, &dev->buf_list, list) {
        if(buf)
        {
            if(buf->phys_addr != 0 && buf->kvirt != NULL)
            {
                dma_free_coherent(dev->pdev, buf->length,
                    buf->kvirt, (dma_addr_t)buf->phys_addr);
            }
            list_del(&buf->list);
            kfree(buf);
        }
	}
	mutex_unlock(&dev->lock);
    kfree(dev);

    return 0;
}
EXPORT_SYMBOL(cma_dma_release);