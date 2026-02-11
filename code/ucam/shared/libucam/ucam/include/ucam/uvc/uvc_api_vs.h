/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api_vs.h
 * @Description : 
 */ 
#ifndef UVC_API_VS_H 
#define UVC_API_VS_H 

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t uvc_get_v4l2_sizeimage(struct uvc_dev *dev, uint32_t fcc, uint16_t width, uint16_t height);
extern void uvc_fill_streaming_ctrl(struct uvc_dev *dev,
		struct uvc_streaming_control *ctrl,
		int iformat, int iframe, int intervel);

extern int uvc_video_set_format(struct uvc_dev *dev, struct uvc_streaming_control *vs_ctrl);

extern struct uvc_ctrl_attr_t *get_uvc_ctrl_vs(void);

extern void uvc_stream_on_trigger(struct uvc_dev *dev);

extern int uvc_streaming_desc_init(struct uvc_dev *dev, int controls_desc_copy, int streaming_desc_copy);
extern int get_uvc_format(struct uvc_dev *dev, struct uvc_streaming_control *vs_ctrl, 
	uint32_t *fcc, 	uint16_t *width, uint16_t *height);


extern struct uvc_descriptor_header **
get_uvc_control_descriptors(struct uvc_dev *dev, enum ucam_usb_device_speed speed);

extern struct uvc_descriptor_header **
get_uvc_streaming_cls_descriptor(struct uvc_dev *dev, enum ucam_usb_device_speed speed);

#ifdef __cplusplus
}
#endif

#endif /*UVC_API_VS_H*/