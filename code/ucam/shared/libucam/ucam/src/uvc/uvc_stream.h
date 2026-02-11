/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2021-04-14 10:25:49
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream.h
 * @Description : 
 */
#ifndef UVC_STREAM_H 
#define UVC_STREAM_H 

#ifdef __cplusplus
extern "C" {
#endif

#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a) - 1)))
extern int uvc_video_stream(struct uvc_dev *dev, int enable);
extern void uvc_video_reqbufs_flush_cache(struct uvc_dev *dev, uint32_t buf_index);

void uvc_memcpy_neon(uint8_t* dst, const uint8_t* src, int size);
void uvc_memcpy(uint8_t* dst, const uint8_t* src, int size);

int uvc_nalu_recode_avc(const uint8_t *src, int src_count, uint8_t *dst);
int uvc_nalu_recode_svc(const uint8_t *src, int src_count, uint8_t *dst);
int uvc_nalu_recode_svc_layers_3_to_2(const uint8_t *src, int src_count, uint8_t *dst);

int uvc_video_dqbuf_lock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf, void **mem);
int uvc_video_dqbuf_nolock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf, void **mem);

int uvc_video_qbuf_lock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf);
int uvc_video_qbuf_nolock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf);

int uvc_memcpy_and_qbuf(struct uvc_dev *dev, int32_t venc_chn, 
    struct v4l2_buffer *v4l2_buf, void *uvc_mem, 
    const void *send_data, uint32_t send_size, int qbuf_en, 
	struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used);

int uvc_dma_memcpy(
	struct uvc *uvc,
	unsigned long long phyDst, 
	unsigned long long phySrc,
	int size);
    
int uvc_dma_memcpy_and_qbuf(struct uvc_dev *dev, int32_t venc_chn, 
    struct v4l2_buffer *v4l2_buf,  unsigned long long uvc_phyAddr, void *uvc_pVirAddr,
    unsigned long long send_data_phyAddr, void *send_data_pVirAddr, 
	uint32_t send_size, int qbuf_en, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used);

extern int uvc_send_stream_ops_int(struct uvc_dev *dev);
extern int uvc_us_to_timeval(const uint64_t usec, struct timeval *timestamp);

extern int UVC_CMA_Enable_Check(struct uvc *uvc);

#ifdef __cplusplus
}
#endif

#endif /*UVC_STREAM_H*/