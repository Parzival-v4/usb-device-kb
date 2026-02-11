/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2021-12-10 11:37:26
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_cma_dma_buf.c
 * @Description : 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <signal.h>
#include <math.h>
#include <poll.h>

#include <ucam/ucam_dbgout.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_api_config.h>

#ifndef CMA_ALIGN_UP
#define CMA_ALIGN_UP(val, alignment) ((((val)+(alignment)-1)/(alignment))*(alignment))
#endif


static void* UVC_CMA_Mmap(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE phyAddr, uint32_t u32Size)
{
    void *pVirtualAddress;
    int fd = -1;
    u32Size = CMA_ALIGN_UP(u32Size,4096);

    fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0)
    {
        ucam_error("Open dev/mem error");
        return NULL;
    }

    pVirtualAddress = mmap ((void *)0, u32Size, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, fd, phyAddr);
    if (MAP_FAILED == pVirtualAddress )
    {
        ucam_error("mmap error");
        close(fd);
        return NULL;
    }
    close(fd);
    return pVirtualAddress;
}

static void* UVC_CMA_MmapCache(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE phyAddr, uint32_t u32Size)
{
    void *pVirtualAddress;
    int fd = -1;
    u32Size = CMA_ALIGN_UP(u32Size,4096);

    fd = open("/dev/mem", O_RDWR);
    if (fd < 0)
    {
        ucam_error("Open dev/mem error");
        return NULL;
    }

    pVirtualAddress = mmap ((void *)0, u32Size, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, fd, phyAddr);
    if (MAP_FAILED  == pVirtualAddress )
    {
        ucam_error("mmap error");
        close(fd);
        return NULL;
    }
    close(fd);

    return pVirtualAddress;
}

static int32_t UVC_CMA_Munmap(struct uvc_dev *dev, void *pVirAddr, uint32_t u32Size)
{
    u32Size = CMA_ALIGN_UP(u32Size,4096);
    
    return munmap(pVirAddr, u32Size);
}

static int32_t UVC_CMA_MflushCache(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE phyAddr, void *pVirAddr, uint32_t u32Size)
{
    //未支持
    return 0;
}

static int32_t UVC_CMA_MmzAlloc(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE *phyAddr, void **ppVirAddr,
							const char* strMmb, const char* strZone, uint32_t u32Len)
{
    int ret;
    struct uvc_ioctl_cma_dma_buf cma_dma_buf;
    void *mem;

    if(dev == NULL)
    {
        return -1;
    }

    u32Len = ALIGN_UP(u32Len,4096);
    cma_dma_buf.length = u32Len;

    ret = uvc_cma_dma_buf_alloc(dev, &cma_dma_buf);
    if(ret != 0)
    {
        ucam_error("dma_buf_alloc error");
        return -1;
    }
    *phyAddr = cma_dma_buf.phys_addr;

    mem = UVC_CMA_Mmap(dev, *phyAddr, u32Len);
    if (NULL == mem )
    {
        ucam_error("UVC_CMA_Mmap error");
        uvc_cma_dma_buf_free(dev, &cma_dma_buf);
        return -1;
    }
    *ppVirAddr = mem;
    return 0;
}

static int32_t UVC_CMA_MmzAlloc_Cached(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE *phyAddr, void **ppVirAddr,
							const char* strMmb, const char* strZone, uint32_t u32Len)
{
    int ret;
    struct uvc_ioctl_cma_dma_buf cma_dma_buf;
    void *mem;

    if(dev == NULL)
    {
        return -1;
    }

    u32Len = ALIGN_UP(u32Len,4096);
    cma_dma_buf.length = u32Len;

    ret = uvc_cma_dma_buf_alloc(dev, &cma_dma_buf);
    if(ret != 0)
    {
        ucam_error("dma_buf_alloc error");
        return -1;
    }
    *phyAddr = cma_dma_buf.phys_addr;

    mem = UVC_CMA_MmapCache(dev, *phyAddr, u32Len);
    if (NULL == mem )
    {
        ucam_error("UVC_CMA_MmapCache error");
        uvc_cma_dma_buf_free(dev, &cma_dma_buf);
        return -1;
    }
    *ppVirAddr = mem;
    return 0;
}

static int32_t UVC_CMA_MmzFlushCache(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE phyAddr, void *pVirAddr, uint32_t u32Size)
{
    //未支持
    return 0;
}

static int32_t UVC_CMA_MmzFree(struct uvc_dev *dev, MMZ_PHY_ADDR_TYPE phyAddr, void *pVirAddr, uint32_t u32Size)
{
    int ret;
    struct uvc_ioctl_cma_dma_buf cma_dma_buf;

    if(dev == NULL)
    {
        return -1;
    }

    u32Size = CMA_ALIGN_UP(u32Size,4096);
    cma_dma_buf.phys_addr = phyAddr;
    cma_dma_buf.length = u32Size;
    ret = UVC_CMA_Munmap(dev, pVirAddr, u32Size);
    if(ret != 0)
    {
        ucam_error("UVC_CMA_Munmap error");
        return ret;
    }
    return uvc_cma_dma_buf_free(dev, &cma_dma_buf);
}


int UVC_CMA_Enable_Check(struct uvc *uvc)
{
    struct uvc_dev *uvc_dev = NULL;
    struct ucam_list_head *pos;
    int set_flag = 0;
    uint32_t enable = 1;

    if(uvc == NULL)
    {
        ucam_error("uvc == NULL\n");
        return -ENOMEM;
    }

    if(uvc->init_flag == 0)
    {
        ucam_error("error uvc init_flag:%d\n", uvc->init_flag);
        return -1;
    }
    
    if(uvc->uvc_dev[0]->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_MMAP 
        || uvc->uvc_mmz_alloc_callback)
    {
        return 0;
    }

    ucam_list_for_each(pos, &uvc->uvc_dev_list)
    {
        uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
        if(uvc_dev)
        {   
            if(uvc_set_cma_dma_buf_enable(uvc_dev, enable) != 0)
            {
                ucam_error("uvc_set_cma_dma_buf_enable error\n");
                return -1;
            }

            uvc_dev->config.reqbufs_memory_type = V4L2_REQBUFS_MEMORY_TYPE_USERPTR;
            set_flag = 1;
        }
    }

    if(set_flag)
    {
        uvc->uvc_mmap_callback = UVC_CMA_Mmap;
        uvc->uvc_mmap_cache_callback = UVC_CMA_MmapCache;
        uvc->uvc_munmap_callback = UVC_CMA_Munmap;
        uvc->uvc_mflush_cache_callback = UVC_CMA_MflushCache;
        uvc->uvc_mmz_alloc_callback = UVC_CMA_MmzAlloc;
        uvc->uvc_mmz_alloc_cached_callback = UVC_CMA_MmzAlloc_Cached;
        uvc->uvc_mmz_flush_cache_callback = UVC_CMA_MmzFlushCache;
        uvc->uvc_mmz_free_callback = UVC_CMA_MmzFree;
    }

    return 0;
}
