/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2025. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2024-02-26 15:20:00
 * @FilePath    : ucam/uvc/uvc_stream_common.h
 * @Description : 
 */
#ifndef UVC_STREAM_COMMON_H 
#define UVC_STREAM_COMMON_H 

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    E_UVC_SEND_STREAM_COMMON_MODE_NORMAL = 0,   //use pVirAddr
    E_UVC_SEND_STREAM_COMMON_MODE_CACHE,        //mmap cache from phyAddr
    E_UVC_SEND_STREAM_COMMON_MODE_DMA,          //dam memcpy from phyAddr, 需要userptr模式才能够支持
    E_UVC_SEND_STREAM_COMMON_MODE_MAX
} UVC_Send_Stream_Common_Mode_e;

typedef struct UVC_Raw_FrameData_s
{
    uint32_t            u32Width;
    uint32_t            u32Height;
    void *              pVirAddr[3];
    unsigned long long  phyAddr[3]; 
    uint32_t            u32Stride[3];
} UVC_Raw_FrameData_t;

typedef struct UVC_Raw_Stream_s
{
    uint32_t            u32Format;//采用V4L2的定义
    uint64_t            u64Pts;
    UVC_Raw_FrameData_t stFrameData;
} UVC_Raw_Stream_t;

typedef struct UVC_VENC_Pack_s
{
    unsigned long long   phyAddr;
    uint8_t *            pu8Addr;
    uint32_t             u32Len;
    uint32_t             u32Offset;
    uint64_t             u64PTS;
} UVC_VENC_Pack_t;

typedef struct UVC_VENC_Stream_s
{
    uint32_t            u32Format;//采用V4L2的定义
    
    UVC_VENC_Pack_t *   pstPack;
    uint32_t            u32PackCount;
} UVC_VENC_Stream_t;

typedef int (*uvc_send_encode_fun_t)(struct uvc_dev *dev, int32_t chn, uint32_t fcc, UVC_VENC_Stream_t* data);
typedef int (*uvc_send_raw_fun_t)(struct uvc_dev *dev, int32_t chn, uint32_t fcc, UVC_Raw_Stream_t* data);

struct uvc_send_raw_info
{
	uint32_t	format_in;
	uint32_t    format_out;
	uvc_send_raw_fun_t send_raw;
};

extern int uvc_send_stream_common_encode(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_VENC_Stream_t *data);

extern int uvc_send_stream_common_raw(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data);


extern int uvc_send_stream_common_ops_int(struct uvc_dev *dev);

extern int uvc_set_stream_common_mode(
	struct uvc_dev *dev, 
	UVC_Send_Stream_Common_Mode_e mode);

#ifdef __cplusplus
}
#endif

#endif /*UVC_STREAM_COMMON_H*/