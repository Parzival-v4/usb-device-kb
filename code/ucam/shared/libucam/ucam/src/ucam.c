/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-28 11:29:46
 * @FilePath    : \lib_ucam\ucam\src\ucam.c
 * @Description : 
 */ 
#include <ucam/ucam.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uac/uac_device.h>
#include <ucam/uvc/uvc_api.h>


uint32_t ucam_get_version_bcd(void)
{
    return LIB_UCAM_VERSION_BCD;
}

uint32_t ucam_get_svn_version(void)
{
    return 731;
}

long long get_time_difference(struct timeval *tvStart, struct timeval *tvEnd)
{
	long long  time_s,time_us,time;
	
	time_s = tvEnd->tv_sec - tvStart->tv_sec;
	time_us = tvEnd->tv_usec - tvStart->tv_usec;
	time = time_s * 1000000 + time_us;	
	return time;	
}


int ucam_usb_connet(struct ucam *ucam)
{
    if(ucam == NULL)
    {
        ucam_error("error ucam == NULL\n");
        return -ENOMEM;
    }
    
    if(ucam->uvc != NULL)
    {
        return uvc_usb_connet (ucam->uvc);
    }

    if(ucam->uac != NULL)
    {
        return uac_usb_connet (ucam->uac);
    }

    return 0;
}

int ucam_usb_reconnet(struct ucam *ucam)
{
    if(ucam == NULL)
    {
        ucam_error("error ucam == NULL\n");
        return -ENOMEM;
    }
       
    
    if(ucam->uvc != NULL)
    {
        return uvc_usb_reconnet (ucam->uvc);
    }

    if(ucam->uac != NULL)
    {
        return uac_usb_reconnet (ucam->uac);
    }

    return 0;
}

int ucam_usb_disconnet(struct ucam *ucam)
{
    if(ucam == NULL)
    {
        ucam_error("error ucam == NULL\n");
        return -ENOMEM;
    }
    
    if(ucam->uvc != NULL)
    {
        return uvc_usb_disconnet (ucam->uvc);
    }

    if(ucam->uac != NULL)
    {
        return uac_usb_disconnet (ucam->uac);
    }

    return 0;
}


int ucam_free(struct ucam *ucam)
{
    ucam_notice("enter\n");

    if(ucam == NULL)
    {
        ucam_error("error ucam == NULL\n");
        return -ENOMEM;
    }

    if(ucam->uac)
    {
       uac_free(ucam->uac);
       ucam->uac = NULL;
    }

    if(ucam->uvc)
    {
       uvc_free(ucam->uvc);
       ucam->uvc = NULL;
    }

    SAFE_FREE(ucam); 

    return 0;
}



struct ucam *create_ucam(void)
{
    struct ucam *ucam = NULL;

    ucam_error("VHD libucam Version:%s, SVN/Git:%s [Build:%s]",VHD_LIBUCAM_VERSION,LIB_UCAM_SVN_GIT_VERSION,LIB_UCAM_COMPILE_INFO);

    ucam = malloc(sizeof (*ucam));
	if (ucam == NULL) {
        ucam_error("Malloc Error!");
		return NULL;
	}

    memset(ucam, 0, sizeof (*ucam));

    return ucam;
}