/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2021-04-14 10:25:49
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream_yuv.h
 * @Description : 
 */
#ifndef UVC_STREAM_YUV_H 
#define UVC_STREAM_YUV_H 

#ifdef __cplusplus
extern "C" {
#endif

void UVC_UVSwapRow_NEON_16(const uint8_t* src_uv, uint8_t* dst_vu, int width);
void UVC_UVSwapRow_Any_NEON(const uint8_t* src_uv, uint8_t* dst_vu, int width);
int UVC_NV21ToNV12(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_vu,
               int src_stride_vu,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_uv,
               int dst_stride_uv,
               int width,
               int height);
void UVC_UVSplitRow_NEON_16(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width);
void UVC_UVSplitRow_Any_NEON(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width);

void UVC_NV21ToYUY2Row_NEON_16(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width);
void UVC_NV21ToYUY2Row_Any_NEON(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width);
int UVC_NV21ToYUY2(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_vu,
               int src_stride_vu,
               uint8_t* dst_yuy2,
               int dst_stride_yuy2,
               int width,
               int height);

void UVC_NV12ToYUY2Row_NEON_16(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width);
void UVC_NV12ToYUY2Row_Any_NEON(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width);
int UVC_NV12ToYUY2(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_uv,
               int src_stride_uv,
               uint8_t* dst_yuy2,
               int dst_stride_yuy2,
               int width,
               int height);

#ifdef __cplusplus
}
#endif

#endif /*UVC_STREAM_YUV_H*/