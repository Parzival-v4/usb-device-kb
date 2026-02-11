/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api_eu.h
 * @Description : 
 */ 
#ifndef UVC_API_EU_H 
#define UVC_API_EU_H 

#ifdef __cplusplus
extern "C" {
#endif

extern int uvc_set_simulcast_enable(struct uvc_dev * dev, uint32_t enable);
extern uint32_t simulcast_stream_id_to_venc_chn(struct uvc_dev * dev, uint32_t stream_id);
extern uint32_t simulcast_stream_id_to_venc_chn(struct uvc_dev * dev, uint32_t stream_id);
extern uint32_t venc_chn_to_simulcast_stream_id(struct uvc_dev * dev, uint32_t chn);
extern uint32_t venc_chn_to_simulcast_uvc_header_id(struct uvc_dev * dev, uint32_t chn);
extern uint16_t get_eu_stream_id(struct uvc_dev * dev);
extern uint32_t get_simulcast_start_or_stop_layer(struct uvc_dev * dev, uint32_t chn);
extern uint32_t get_current_simulcast_venc_chn(struct uvc_dev * dev);

#ifdef __cplusplus
}
#endif

#endif /*UVC_API_EU_H*/