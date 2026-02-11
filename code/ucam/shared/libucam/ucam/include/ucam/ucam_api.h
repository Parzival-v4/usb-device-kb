/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 17:41:31
 * @FilePath    : \lib_ucam\ucam\include\ucam\ucam_api.h
 * @Description : 
 */ 

#ifndef UCAM_API_H 
#define UCAM_API_H 


#ifdef __cplusplus
extern "C" {
#endif

uint32_t ucam_get_version_bcd(void);
uint32_t ucam_get_svn_version(void);

long long get_time_difference(struct timeval *tvStart, struct timeval *tvEnd);

/**
 * @description: 创建ucam内存
 * @param {NULL} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
struct ucam *create_ucam(void);

/**
 * @description: 关闭设备并释放内存
 * @param {ucam} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int ucam_free (struct ucam *ucam);

/**
 * @description: 打开usb连接
 * @param {ucam} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int ucam_usb_connet (struct ucam *ucam);

/**
 * @description: 断开usb连接
 * @param {ucam} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int ucam_usb_reconnet (struct ucam *ucam);

/**
 * @description: 断开USB后再重新连接
 * @param {ucam} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int ucam_usb_disconnet (struct ucam *ucam);

#ifdef __cplusplus
}
#endif


#endif /*UCAM_API_H*/