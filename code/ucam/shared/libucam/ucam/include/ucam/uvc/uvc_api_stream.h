/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2022. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-09-14 10:58:08
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api_stream.h
 * @Description : 
 */
#ifndef UVC_API_STREAM_H 
#define UVC_API_STREAM_H 

#include <ucam/uvc/uvc_device.h>

/********************************************************************************************/
//extern int uvc_api_set_led_status (uint32_t status);
//extern int uvc_api_get_led_status (uint32_t *status);
/********************************************************************************************/
//extern int uvc_api_set_save_settings (void);
/********************************************************************************************/
extern int uvc_api_set_stream_status (struct uvc_dev *dev, int32_t venc_chn, uint32_t status);
extern int uvc_api_get_stream_status (struct uvc_dev *dev, int32_t venc_chn, uint32_t *status);
/********************************************************************************************/
extern int uvc_api_set_format (struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc_format);
extern int uvc_api_get_format (struct uvc_dev *dev, int32_t venc_chn, uint32_t *fcc_format);
extern int uvc_api_get_format_max (struct uvc_dev *dev, int32_t venc_chn, uint32_t *fcc_format);
extern int uvc_api_get_format_min (struct uvc_dev *dev, int32_t venc_chn, uint32_t *fcc_format);
extern int uvc_api_get_format_def (struct uvc_dev *dev, int32_t venc_chn, uint32_t *fcc_format);
/********************************************************************************************/
extern int uvc_api_set_resolution (struct uvc_dev *dev, int32_t venc_chn, uint16_t width, uint16_t height);
extern int uvc_api_get_resolution (struct uvc_dev *dev, int32_t venc_chn, uint16_t *width, uint16_t *height);
extern int uvc_api_get_resolution_max (struct uvc_dev *dev, int32_t venc_chn, uint16_t *width, uint16_t *height);
extern int uvc_api_get_resolution_min (struct uvc_dev *dev, int32_t venc_chn, uint16_t *width, uint16_t *height);
extern int uvc_api_get_resolution_def (struct uvc_dev *dev, int32_t venc_chn, uint16_t *width, uint16_t *height);
/********************************************************************************************/
extern int uvc_api_set_frame_interval (struct uvc_dev *dev, int32_t venc_chn, uint32_t frame_interval);
extern int uvc_api_get_frame_interval (struct uvc_dev *dev, int32_t venc_chn, uint32_t *frame_interval);
extern int uvc_api_get_frame_interval_max (struct uvc_dev *dev, int32_t venc_chn, uint32_t *frame_interval);
extern int uvc_api_get_frame_interval_min (struct uvc_dev *dev, int32_t venc_chn, uint32_t *frame_interval);
extern int uvc_api_get_frame_interval_def (struct uvc_dev *dev, int32_t venc_chn, uint32_t *frame_interval);
/********************************************************************************************/
extern int uvc_api_set_h264_bitrate (struct uvc_dev *dev, int32_t venc_chn, uint32_t bitrate);
extern int uvc_api_get_h264_bitrate (struct uvc_dev *dev, int32_t venc_chn, uint32_t *bitrate);
extern int uvc_api_get_h264_bitrate_max (struct uvc_dev *dev, int32_t venc_chn, uint32_t *bitrate);
extern int uvc_api_get_h264_bitrate_min (struct uvc_dev *dev, int32_t venc_chn, uint32_t *bitrate);
extern int uvc_api_get_h264_bitrate_def (struct uvc_dev *dev, int32_t venc_chn, uint32_t *bitrate);
/********************************************************************************************/
extern int uvc_api_set_h264_i_frame_period (struct uvc_dev *dev, int32_t venc_chn, uint16_t IFramePeriod);
extern int uvc_api_get_h264_i_frame_period (struct uvc_dev *dev, int32_t venc_chn,  uint16_t *IFramePeriod);
extern int uvc_api_get_h264_i_frame_period_max (struct uvc_dev *dev, int32_t venc_chn, uint16_t *IFramePeriod);
extern int uvc_api_get_h264_i_frame_period_min (struct uvc_dev *dev, int32_t venc_chn, uint16_t *IFramePeriod);
extern int uvc_api_get_h264_i_frame_period_def (struct uvc_dev *dev, int32_t venc_chn, uint16_t *IFramePeriod);
/********************************************************************************************/
extern int uvc_api_set_h264_slice(struct uvc_dev *dev, int32_t venc_chn, uint16_t mode, uint16_t size);
extern int uvc_api_get_h264_slice(struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode, uint16_t *size);
extern int uvc_api_get_h264_slice_max (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode, uint16_t *size);
extern int uvc_api_get_h264_slice_min (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode, uint16_t *size);
extern int uvc_api_get_h264_slice_def (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode, uint16_t *size);
extern int uvc_api_set_h264_slice_mode (struct uvc_dev *dev, int32_t venc_chn, uint16_t mode);
extern int uvc_api_get_h264_slice_mode (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode);
extern int uvc_api_get_h264_slice_mode_max (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode);
extern int uvc_api_get_h264_slice_mode_min (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode);
extern int uvc_api_get_h264_slice_mode_def (struct uvc_dev *dev, int32_t venc_chn, uint16_t *mode);
/********************************************************************************************/
extern int uvc_api_set_h264_slice_size (struct uvc_dev *dev, int32_t venc_chn, uint16_t size);
extern int uvc_api_get_h264_slice_size (struct uvc_dev *dev, int32_t venc_chn, uint16_t *size);
extern int uvc_api_get_h264_slice_size_max (struct uvc_dev *dev, int32_t venc_chn, uint16_t *size);
extern int uvc_api_get_h264_slice_size_min (struct uvc_dev *dev, int32_t venc_chn, uint16_t *size);
extern int uvc_api_get_h264_slice_size_def (struct uvc_dev *dev, int32_t venc_chn, uint16_t *size);
/********************************************************************************************/
extern int uvc_api_set_h264_frame_requests(struct uvc_dev *dev, int32_t venc_chn, uint16_t mode);
/********************************************************************************************/
extern int uvc_api_set_h264_rate_control_mode(struct uvc_dev *dev, int32_t venc_chn, uint8_t mode);
extern int uvc_api_get_h264_rate_control_mode (struct uvc_dev *dev, int32_t venc_chn, uint8_t *mode);
extern int uvc_api_get_h264_rate_control_mode_max (struct uvc_dev *dev, int32_t venc_chn, uint8_t *mode);
extern int uvc_api_get_h264_rate_control_mode_min (struct uvc_dev *dev, int32_t venc_chn, uint8_t *mode);
extern int uvc_api_get_h264_rate_control_mode_def (struct uvc_dev *dev, int32_t venc_chn, uint8_t *mode);
/********************************************************************************************/
extern int uvc_api_set_h264_profile(struct uvc_dev *dev, int32_t venc_chn, uint16_t profile);
extern int uvc_api_get_h264_profile (struct uvc_dev *dev, int32_t venc_chn, uint16_t *profile);
extern int uvc_api_get_h264_profile_max (struct uvc_dev *dev, int32_t venc_chn, uint16_t *profile);
extern int uvc_api_get_h264_profile_min (struct uvc_dev *dev, int32_t venc_chn, uint16_t *profile);
extern int uvc_api_get_h264_profile_def (struct uvc_dev *dev, int32_t venc_chn, uint16_t *profile);
/********************************************************************************************/
extern int uvc_api_set_h264_encoder_reset(struct uvc_dev *dev, int32_t venc_chn);
/********************************************************************************************/
extern int uvc_api_set_h264_qp_steps(struct uvc_dev *dev, int32_t venc_chn, struct uvcx_qp_steps_layers qp_steps);
extern int uvc_api_get_h264_qp_steps (struct uvc_dev *dev, int32_t venc_chn, struct uvcx_qp_steps_layers *p_qp_steps);
extern int uvc_api_get_h264_qp_steps_max (struct uvc_dev *dev, int32_t venc_chn, struct uvcx_qp_steps_layers *p_qp_steps);
extern int uvc_api_get_h264_qp_steps_min (struct uvc_dev *dev, int32_t venc_chn, struct uvcx_qp_steps_layers *p_qp_steps);
extern int uvc_api_get_h264_qp_steps_def (struct uvc_dev *dev, int32_t venc_chn, struct uvcx_qp_steps_layers *p_qp_steps);
/********************************************************************************************/

#endif /* UVC_API_STREAM_H */