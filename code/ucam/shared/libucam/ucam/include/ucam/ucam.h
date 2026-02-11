/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 17:41:31
 * @FilePath    : \lib_ucam\ucam\include\ucam\ucam.h
 * @Description : 
 */ 

#ifndef UCAM_H 
#define UCAM_H 

#include <sys/types.h>
#include <stdint.h>
#include <ucam/ucam_version.h>

#if (defined CHIP_SSC9381 || defined CHIP_SSC268 || \
defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211 || defined CHIP_SSC369)
#if !(defined SIGMASTAR_SDK)
#define SIGMASTAR_SDK
#endif
#elif (defined CHIP_HI3519V101 || defined CHIP_HI3559V100 || \
defined CHIP_HI3519A || defined CHIP_HI3516EV300)
#if !(defined HISI_SDK)
#define HISI_SDK
#endif
#elif (defined CHIP_RV1126)
#if !(defined ROCKCHIP_SDK)
#define ROCKCHIP_SDK
#endif
#elif (defined CHIP_GK7205V300)
#if !(defined RGK_SDK)
#define RGK_SDK
#endif
#elif (defined CHIP_NT98530)
#if !(defined NVT_SDK)
#define NVT_SDK
#endif
#elif (defined CHIP_CV5)
#if !(defined AMBARELLA_SDK)
#define AMBARELLA_SDK
#endif
//#else
//#error "SDK type no defined "
#endif

#if 0
//宏定义检查
#if !(defined CHIP_HI3519V101 || defined CHIP_HI3559V100 || \
defined CHIP_HI3519A || defined CHIP_HI3516EV300 || \
defined CHIP_SSC9381 || defined CHIP_SSC268 || \
defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211 \
|| defined CHIP_RV1126 || defined CHIP_GK7205V300 || defined CHIP_SSC369\
|| defined CHIP_NT98530 || defined CHIP_CV5 )
#error "You must defined CHIP_HI3519V101/CHIP_HI3559V100/CHIP_HI319A/CHIP_HI3516EV300/\
CHIP_SSC9381/CHIP_SSC268/CHIP_SSC333/CHIP_SSC337/CHIP_SSC9211/\
CHIP_RV1126 CHIP_GK7205V300"
#endif

#if !(defined HISI_SDK || defined SIGMASTAR_SDK || defined ROCKCHIP_SDK || defined RGK_SDK || defined NVT_SDK || defined AMBARELLA_SDK )
#error "You must defined HISI_SDK/SIGMASTAR_SDK/ROCKCHIP_SDK/RGK_SDK"
#endif
#endif

struct ucam
{ 
    uint32_t ucam_id;   /* ucam的id用于区分是哪个USB口,一个USB口对应一个ucam */
    struct uvc *uvc;    /* uvc句柄 */
    struct uac *uac;    /* uac句柄 */
    void *param;
};

#endif /*UCAM_H*/
