/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-29 16:20:05
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl_xu_h264.c
 * @Description : 
 */ 

#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/uvc_ctrl_xu_h264.h>
#include <ucam/uvc/uvc_api_stream.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_vs.h>

static int init_ctrls_xu_h264(struct uvc_dev *dev)
{
	ucam_strace("enter");
	
	/* UVCX_VIDEO_CONFIG_PROBE 	& UVCX_VIDEO_CONFIG_COMMIT */ 
	//h264_config default:
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwFrameInterval = UVC_FORMAT_30FPS; 		//帧率
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwBitRate = 4096*1024;			//码率
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bmHints = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wConfigurationIndex = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wWidth = 1920;					//帧宽
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wHeight = 1080;				//帧高
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wSliceUnits = 6;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wSliceMode = UVCX_H264_wSliceMode_PERSLICE;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wProfile = UVCX_H264_wProfile_HIGHPROFILE;	//编码等级
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wIFramePeriod = 1000;//UVC_KEY_FRAME_RATE_DEF;			//I帧时间
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wEstimatedVideoDelay = 0x1e;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wEstimatedMaxConfigDelay = 0x28;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bUsageType = 0x01;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bRateControlMode = UVCX_H264_bRateControlMode_VBR;	//码率模式
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bTemporalScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bSpatialScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bSNRScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bStreamMuxOption = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bStreamFormat = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bEntropyCABAC = 0;	
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bTimestamp = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bNumOfReorderFrames = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bPreviewFlipped = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bView =0 ;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bReserved1 = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bReserved2 = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bStreamID = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bSpatialLayerRatio = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wLeakyBucketSize = 0;

	//h264_config min:
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval = UVC_FORMAT_30FPS; 		//帧率 10000000 / 30
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwBitRate = 1*1024;			//码率
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bmHints = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wConfigurationIndex = 1;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wWidth = 176;					//帧宽
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wHeight = 144;					//帧高
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wSliceUnits = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wSliceMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wProfile = UVCX_H264_wProfile_BASELINEPROFILE;		//编码等级
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wIFramePeriod = 1;				//I帧间隔 2 * 1/30 * 1000
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wEstimatedVideoDelay = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wEstimatedMaxConfigDelay = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bUsageType = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bRateControlMode = UVCX_H264_bRateControlMode_CBR;	//码率模式
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bTemporalScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bSpatialScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bSNRScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bStreamMuxOption = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bStreamFormat = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bEntropyCABAC = 0;	
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bTimestamp = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bNumOfReorderFrames = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bPreviewFlipped = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bView =0 ;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bReserved1 = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bReserved2 = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bStreamID = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bSpatialLayerRatio = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wLeakyBucketSize = 0;


	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwFrameInterval = UVC_FORMAT_1FPS; 		//帧率 10000000 / 25
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwBitRate = 128*1024*1024;			//码率
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bmHints = UVCX_H264_bmHints_RESOLUTION | UVCX_H264_bmHints_PROFILE | UVCX_H264_bmHints_RATECONTROLMODE |  UVCX_H264_bmHints_USAGETYPE\
								| UVCX_H264_bmHints_SLICEMODE | UVCX_H264_bmHints_SLICEUNIT | UVCX_H264_bmHints_FRAMEINTERVAL | UVCX_H264_bmHints_BITRATE | UVCX_H264_bmHints_IFRAMEPERIOD;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wConfigurationIndex = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wWidth = dev->config.resolution_width_max;					//帧宽
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wHeight = dev->config.resolution_height_max;				//帧高
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wSliceUnits = 0x44;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wSliceMode = 0x03;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wProfile = UVCX_H264_wProfile_HIGHPROFILE;	//编码等级
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wIFramePeriod = 0xFFFF;//60 * 1000;			//I帧60S
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wEstimatedVideoDelay = 0x1e;//30ms
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wEstimatedMaxConfigDelay = 0x28;//40ms
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bUsageType = 0x01;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bRateControlMode = UVCX_H264_bRateControlMode_CONSTANTQP;	//码率模式
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bTemporalScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bSpatialScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bSNRScaleMode = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bStreamMuxOption = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bStreamFormat = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bEntropyCABAC = 1;	
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bTimestamp = 1;//0x00=picture timing SEI disabled ;0x01=picture timing SEI enabled
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bNumOfReorderFrames = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bPreviewFlipped = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bView =0 ;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bReserved1 = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bReserved2 = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bStreamID = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bSpatialLayerRatio = 0;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wLeakyBucketSize = 4000;	


	uvc_api_get_resolution_max (dev, dev->config.venc_chn, 
		&dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wWidth, &dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wHeight);

	uvc_api_get_resolution_min (dev, dev->config.venc_chn, 
		&dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wWidth, &dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wHeight);

	uvc_api_get_resolution_def (dev, dev->config.venc_chn, 
		&dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wWidth, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wHeight);


	uvc_api_get_frame_interval_max (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwFrameInterval);
	uvc_api_get_frame_interval_min (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval);
	uvc_api_get_frame_interval_def (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwFrameInterval);


	uvc_api_get_h264_bitrate_max (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwBitRate);
	uvc_api_get_h264_bitrate_min (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwBitRate);
	uvc_api_get_h264_bitrate_def (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwBitRate);

	uvc_api_get_h264_qp_steps_max (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_max);
	uvc_api_get_h264_qp_steps_min (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_min);
	uvc_api_get_h264_qp_steps_def (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_def);

	uvc_api_get_h264_i_frame_period_max (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wIFramePeriod);
	uvc_api_get_h264_i_frame_period_min (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wIFramePeriod);
	uvc_api_get_h264_i_frame_period_def (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wIFramePeriod);


	uvc_api_get_h264_slice_def (dev, dev->config.venc_chn, 
		&dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wSliceMode, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wSliceUnits);
	
	uvc_api_get_h264_rate_control_mode_def (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bRateControlMode);
	uvc_api_get_h264_profile_def (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wProfile);

	//memcpy(&uvcx_h264_config_cur, &uvcx_h264_config_def, sizeof ( struct uvcx_h264_config ));
	dev->config.uvc_xu_h264_config.uvcx_h264_config_cur = dev->config.uvc_xu_h264_config.uvcx_h264_config_def;


	
	/* UVCX_PICTURE_TYPE_CONTROL 	0x09 */
	/* <2>"3.3.9 UVCX_PICTURE_TYPE_CONTROL", page  31 */
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_def.wLayerID = 0x0001;
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_def.wPicType = 0x0000;
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_min.wLayerID = 0x0001;
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_min.wPicType = 0x0000;
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_max.wLayerID = 0x0001;
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_max.wPicType = 0x0002;
	
	dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_cur = dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_def;
	

	/* UVCX_VERSION 				0x0A */
	/* <2>"3.3.10 UVCX_VERSION", page 32 */
	dev->config.uvc_xu_h264_config.uvcx_version_def.wVersion = UVCX_H264_VERSION;
	dev->config.uvc_xu_h264_config.uvcx_version_cur = dev->config.uvc_xu_h264_config.uvcx_version_def;
	dev->config.uvc_xu_h264_config.uvcx_version_min = dev->config.uvc_xu_h264_config.uvcx_version_def;
	dev->config.uvc_xu_h264_config.uvcx_version_max = dev->config.uvc_xu_h264_config.uvcx_version_def;
	
	
	/* UVCX_RATE_CONTROL_MODE 		0x03 */ 
	/* <2>"3.3.3 UVCX_RATE_CONTROL_MODE", page 22 */
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_def.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_def.bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_h264_config_def.bRateControlMode;
	
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_min.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_min.bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bRateControlMode;
	
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_max.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_max.bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bRateControlMode;
	
	dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur = dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_def;
	
	/* UVCX_FRAMERATE_CONFIG 		0x0C */
	/* <2>"3.3.12 UVCX_FRAMERATE_CONFIG", page 33 */
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_def.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_def.dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwFrameInterval;
	
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_min.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_min.dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval;
	
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_max.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_max.dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwFrameInterval;
	
	dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur = dev->config.uvc_xu_h264_config.uvcx_framerate_config_def;
	
	/* UVCX_BITRATE_LAYERS 			0x0E */
	/* <2>"3.3.14 UVCX_BITRATE_LAYERS", page 35 */
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_def.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_def.dwPeakBitrate = dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwBitRate;
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_def.dwAverageBitrate = dev->config.uvc_xu_h264_config.uvcx_h264_config_def.dwBitRate;
	
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.dwPeakBitrate = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwBitRate;
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.dwAverageBitrate = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwBitRate;
	
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.wLayerID = 0;
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.dwPeakBitrate = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwBitRate;
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.dwAverageBitrate = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwBitRate;
	
	dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_def;
	
	dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_cur = dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_def;
	return 0;
}


static uvc_xu_h264_set_format_callback_fn_t g_uvc_xu_h264_set_format_callback_fn = NULL;

int register_uvc_xu_h264_set_format_callback(uvc_xu_h264_set_format_callback_fn_t callback_fn)
{
	g_uvc_xu_h264_set_format_callback_fn = callback_fn;
	return 0;
}

static int h264_video_set_format(struct uvc_dev *dev, struct uvcx_h264_config *h264_ctrl)
{
	ucam_trace("h264_video_set_format called \n");
	
	int ret;
	//uint32_t KeyFrameRate;
	unsigned char fps;

	fps = (unsigned char)(10000000.0f/h264_ctrl->dwFrameInterval);

	//dev->fcc = V4L2_PIX_FMT_H264;
	dev->width = h264_ctrl->wWidth;
	dev->height = h264_ctrl->wHeight;

	if((h264_ctrl->bmHints & UVCX_H264_bmHints_BITRATE) == 0)
	{
		//码率（kbps）设置	
		if(uvc_api_set_h264_bitrate (dev, dev->config.venc_chn, h264_ctrl->dwBitRate) != 0)
		{
			ucam_error("uvc_api_set_h264_bitrate fail");
		}
	}

		
	if((h264_ctrl->bmHints & UVCX_H264_bmHints_IFRAMEPERIOD) == 0)
	{
		//I 帧间隔设置 
		if(uvc_api_set_h264_i_frame_period (dev, dev->config.venc_chn, h264_ctrl->wIFramePeriod) != 0)
		{
			ucam_error("uvc_api_set_h264_i_frame_period fail");
		}
	}

	if((h264_ctrl->bmHints & UVCX_H264_bmHints_FRAMEINTERVAL) == 0)
	{
		//设置uvc的视频帧率
		if(uvc_api_set_frame_interval(dev, dev->config.venc_chn, h264_ctrl->dwFrameInterval) != 0)
		{
			ucam_error("uvc_api_set_frame_interval fail");
			//return -1;
		}
	}
	
	if((h264_ctrl->bmHints & UVCX_H264_bmHints_PROFILE) == 0)
	{
		//编码等级
		if(uvc_api_set_h264_profile(dev, dev->config.venc_chn, h264_ctrl->wProfile) != 0)
		{
			ucam_error("uvc_api_set_h264_profile fail");
		}
	}	

	if((h264_ctrl->bmHints & UVCX_H264_bmHints_RATECONTROLMODE) == 0)
	{
		//码率模式
		if(uvc_api_set_h264_rate_control_mode(dev, dev->config.venc_chn, h264_ctrl->bRateControlMode) != 0)
		{
			ucam_error("uvc_api_set_h264_rate_control_mode fail");
		}
	}
	
	if((h264_ctrl->bmHints & UVCX_H264_bmHints_SLICEMODE) == 0)
	{
		//设置slice模式
		if(uvc_api_set_h264_slice_mode (dev, dev->config.venc_chn, h264_ctrl->wSliceMode) != 0)
		{
			ucam_error("uvc_api_set_h264_slice_mode fail");
		}
	}

	if((h264_ctrl->bmHints & UVCX_H264_bmHints_SLICEUNIT) == 0)
	{
		//设置slice大小
		if(uvc_api_set_h264_slice_size (dev, dev->config.venc_chn, h264_ctrl->wSliceUnits) != 0)
		{
			ucam_error("uvc_api_set_h264_slice_mode fail");
		}
	}

	if((h264_ctrl->bmHints & UVCX_H264_bmHints_RESOLUTION) == 0)
	{	
		//设置uvc的视频分辨率
		if(uvc_api_set_resolution(dev, dev->config.venc_chn, dev->width, dev->height) != 0)
		{
			ucam_error("uvc_api_set_resolution fail");
			return -1;
		}
	}

	if(g_uvc_xu_h264_set_format_callback_fn)
	{
		g_uvc_xu_h264_set_format_callback_fn(dev, h264_ctrl);
	}

	if((h264_ctrl->bmHints & UVCX_H264_bmHints_RESOLUTION) == 0 || (h264_ctrl->bmHints & UVCX_H264_bmHints_PROFILE) == 0 
		|| (h264_ctrl->bmHints & UVCX_H264_bmHints_RATECONTROLMODE) == 0)
	{	
		if(uvc_api_set_format(dev, dev->config.venc_chn, dev->fcc) != 0)
		{
			ucam_error("uvc_api_set_format fail");
			return -1;
		}

		if((dev->fcc == V4L2_PIX_FMT_H264 || dev->fcc == V4L2_PIX_FMT_H265) && dev->uvc_stream_on)
		{
			
			if(uvc_api_set_stream_status (dev, dev->config.venc_chn, 0) != 0)
			{
				ucam_error("uvc_api_set_stream_status fail");
				return -1;
			}
			usleep(1000);
			pthread_mutex_lock(&dev->uvc_reqbufs_mutex);
			if(uvc_api_set_stream_status (dev, dev->config.venc_chn, 1) != 0)
			{
				ucam_error("uvc_api_set_stream_status fail");
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -1;
			}
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		}


		ucam_notice("Setting format to %s-%ux%ux%d",
			uvc_video_format_string(dev->fcc), dev->width, dev->height,fps);

		
		//printf("Setting format to 0x%08x %ux%up%u\n",
		//	dev->fcc, dev->width, dev->height,fps);

		struct v4l2_format fmt;
		memset(&fmt, 0, sizeof fmt);
		fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		fmt.fmt.pix.width = dev->width;
		fmt.fmt.pix.height = dev->height;
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
		fmt.fmt.pix.field = V4L2_FIELD_NONE;

		fmt.fmt.pix.sizeimage = uvc_get_v4l2_sizeimage(dev, dev->fcc, fmt.fmt.pix.width, fmt.fmt.pix.height);
		dev->sizeimage = fmt.fmt.pix.sizeimage;

		if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
			ucam_strace("Unable to set format: %s (%d).",strerror(errno), errno);
	}
				
	dev->uvc_h264_sei_en = h264_ctrl->bTimestamp;
	
	return ret;	
		
}

static void h264_config_value_limit(struct uvc_dev *dev, struct uvcx_h264_config *h264_config)
{
	//分辨率
	/*	
	if(!((h264_config->wWidth == 1920 && h264_config->wHeight == 1080) || (h264_config->wWidth == 1280 && h264_config->wHeight == 720)
	  ||(h264_config->wWidth == 960 && h264_config->wHeight == 540) || (h264_config->wWidth == 640 && h264_config->wHeight == 360)
	  || (h264_config->wWidth == 640 && h264_config->wHeight == 480) || (h264_config->wWidth == 176 && h264_config->wHeight == 144)
	  || (h264_config->wWidth == 320 && h264_config->wHeight == 180) || (h264_config->wWidth == 320 && h264_config->wHeight == 240)
	  || (h264_config->wWidth == 352 && h264_config->wHeight == 288) || (h264_config->wWidth == 720 && h264_config->wHeight == 576)
	  || (h264_config->wWidth == 1024 && h264_config->wHeight == 768) || (h264_config->wWidth == 3840 && h264_config->wHeight == 2160
	  || (h264_config->wWidth == 1024 && h264_config->wHeight == 576))
	  ))
	{
		//只支持 1080P、720P、540P、360P
		h264_config->wWidth = 1920;
		h264_config->wHeight = 1080;				
	}
	*/
	ucam_trace("enter");
	
	//帧率
	if(h264_config->dwFrameInterval < dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval)
		h264_config->dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval;	
	else if(h264_config->dwFrameInterval > dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwFrameInterval)
		h264_config->dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwFrameInterval;
	
	//if(h264_config->dwFrameInterval > dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval 
	//	&& h264_config->dwFrameInterval < dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwFrameInterval)
		//h264_config->dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwFrameInterval;	//只支持 25fps和30fps
	
	//码率
	if(h264_config->dwBitRate < dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwBitRate)
		h264_config->dwBitRate = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.dwBitRate;	
	else if(h264_config->dwBitRate > dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwBitRate)
		h264_config->dwBitRate = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.dwBitRate;	
	
	//I帧间隔
	if(h264_config->wIFramePeriod < dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wIFramePeriod)
		h264_config->wIFramePeriod = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.wIFramePeriod;	
	else if(h264_config->wIFramePeriod > dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wIFramePeriod)
		h264_config->wIFramePeriod = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wIFramePeriod;	

#if 0	
	//编码等级
	if(!(h264_config->wProfile == UVCX_H264_wProfile_BASELINEPROFILE || h264_config->wProfile == UVCX_H264_wProfile_MAINPROFILE 
		|| h264_config->wProfile == UVCX_H264_wProfile_HIGHPROFILE 
		|| h264_config->wProfile == UVCX_H264_wProfile_SCALABLEBASELINEPROFILE || h264_config->wProfile == UVCX_H264_wProfile_SCALABLEHIGHPROFILE))
		h264_config->wProfile = dev->config.uvc_xu_h264_config.uvcx_h264_config_def.wProfile;	
		
	
	//码率模式	
	if(h264_config->bRateControlMode < dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bRateControlMode)
		h264_config->bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_h264_config_min.bRateControlMode;	
	else if(h264_config->bRateControlMode > dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bRateControlMode)
		h264_config->bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_h264_config_max.bRateControlMode;
#endif	
}


static int h264_config_cpy(struct uvc_dev *dev, struct uvcx_h264_config *dest,struct uvcx_h264_config *src)
{
	struct uvcx_h264_config original;

	ucam_trace("enter");

	memcpy(&original, dest, sizeof(struct uvcx_h264_config));
	memcpy(dest, src, sizeof(struct uvcx_h264_config));
	
	//分辨率
	if((src->bmHints & UVCX_H264_bmHints_RESOLUTION) != 0) 
	{
		dest->wWidth = original.wWidth;
		dest->wHeight = original.wHeight;
	}
		
	//帧率
	if((src->bmHints & UVCX_H264_bmHints_FRAMEINTERVAL) != 0)
	{
		dest->dwFrameInterval = original.dwFrameInterval;
	}
	
	
	//码率
	if((src->bmHints & UVCX_H264_bmHints_BITRATE) != 0)
	{
		dest->dwBitRate = original.dwBitRate;
	}	
	
	//I帧间隔
	if((src->bmHints & UVCX_H264_bmHints_IFRAMEPERIOD) != 0)
	{
		dest->wIFramePeriod = original.wIFramePeriod;		
	}
	
	
	//编码等级
	if((src->bmHints & UVCX_H264_bmHints_PROFILE) != 0)
	{
		dest->wProfile = original.wProfile;		
	}
		
	
	//码率模式
	if((src->bmHints & UVCX_H264_bmHints_RATECONTROLMODE) != 0)
	{
		dest->bRateControlMode = original.bRateControlMode;		
	}
	
	//slice
	if((src->bmHints & UVCX_H264_bmHints_SLICEMODE) != 0)
	{
		dest->wSliceMode = original.wSliceMode;
	}

	//slice
	if((src->bmHints & UVCX_H264_bmHints_SLICEUNIT) != 0)
	{
		dest->wSliceUnits = original.wSliceUnits;
	}

	h264_config_value_limit(dev, dest);
	
	return 0;		
}

static int uvc_vs_set_h264_config(struct uvc_dev *dev, struct uvc_streaming_control *vs_ctrl)
{
	//float CompQuality;
	//unsigned int KeyFrameRate;
		
	if (dev->fcc != V4L2_PIX_FMT_H264)
		return -1;	

	
	//分辨率 
	dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wWidth = dev->width;
	dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wHeight = dev->height;

	//帧率
	dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwFrameInterval = vs_ctrl->dwFrameInterval;
	//码率
	//uvcx_h264_config_cur.dwBitRate = ((uint16_t)CompQuality) * 1024;
	
	//I帧间隔
	//if(vs_ctrl->wKeyFrameRate * 1000 > uvcx_h264_config_max.wIFramePeriod)
	//	KeyFrameRate = uvcx_h264_config_max.wIFramePeriod;
	//else
	//	KeyFrameRate = vs_ctrl->wKeyFrameRate * 1000;
	
	//uvcx_h264_config_cur.wIFramePeriod = KeyFrameRate;
				
	h264_config_value_limit(dev, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur);
	
	return 0;		
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_config_probe( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.1 UVCX_VIDEO_CONFIG_PROBE&UVCX_VIDEO_CONFIG_COMMIT", page 12 */
	uint8_t 	req = ctrl->bRequest; 

	struct uvcx_h264_config *h264_ctrl;

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_h264_config );
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				h264_ctrl = (struct uvcx_h264_config *)data->data;	
				h264_config_cpy(dev, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur, h264_ctrl);
			}
			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def, data->length);
			break;

		case UVC_GET_CUR:
			dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bTimestamp = dev->uvc_h264_sei_en;

			uvc_api_get_resolution (dev, dev->config.venc_chn, 
				&dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wWidth, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wHeight);

			uvc_api_get_frame_interval (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwFrameInterval);
			uvc_api_get_h264_bitrate (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwBitRate);
			uvc_api_get_h264_i_frame_period (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wIFramePeriod);

			uvc_api_get_h264_slice (dev, dev->config.venc_chn, 
				&dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wSliceMode, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wSliceUnits);
			
			uvc_api_get_h264_rate_control_mode (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bRateControlMode);
			uvc_api_get_h264_profile (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wProfile);

			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_h264_config );
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_config_commit( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.1 UVCX_VIDEO_CONFIG_PROBE&UVCX_VIDEO_CONFIG_COMMIT", page 12 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	struct uvcx_h264_config *h264_ctrl;

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_h264_config );
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				h264_ctrl = (struct uvcx_h264_config *)data->data;
				h264_config_cpy(dev, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur, h264_ctrl);	
				h264_video_set_format(dev, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur);
			}
			break;
			
		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_def, data->length);
			break;

		case UVC_GET_CUR:
			dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bTimestamp = dev->uvc_h264_sei_en;

			uvc_api_get_resolution (dev, dev->config.venc_chn, 
				&dev->config.uvc_xu_h264_config.uvcx_h264_config_max.wWidth, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wHeight);

			uvc_api_get_frame_interval (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwFrameInterval);
			uvc_api_get_h264_bitrate (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwBitRate);
			uvc_api_get_h264_i_frame_period (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wIFramePeriod);

			uvc_api_get_h264_slice (dev, dev->config.venc_chn, 
				&dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wSliceMode, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wSliceUnits);
			
			uvc_api_get_h264_rate_control_mode (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bRateControlMode);
			uvc_api_get_h264_profile (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.wProfile);

			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_h264_config_max, data->length);
			break;	
			
		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_h264_config );
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;	

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_rate_ctrl_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.3 UVCX_RATE_CONTROL_MODE", page 22 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 
	struct uvcx_rate_ctrl_mode *rate_ctrl_mode;

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_rate_ctrl_mode );
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				rate_ctrl_mode = (struct uvcx_rate_ctrl_mode *)data->data;
				memcpy( &dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur, rate_ctrl_mode, sizeof ( struct uvcx_rate_ctrl_mode ));
				
				if(dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode < dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_min.bRateControlMode)
					dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_min.bRateControlMode;
				else if(dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode > dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_max.bRateControlMode)
					dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_max.bRateControlMode;
					
				dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bRateControlMode = dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode;
				//码率模式
				if(dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode == UVCX_H264_bRateControlMode_CBR)
				{
	
				}
				else
				{
					dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode = UVCX_H264_bRateControlMode_VBR;
				}

				if(uvc_api_set_h264_rate_control_mode(dev, dev->config.venc_chn, dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode) != 0)
				{
					ucam_error("uvc_api_set_h264_rate_control_mode fail\n");
				}
			}

			break;
			
		case UVC_GET_RES:
			rate_ctrl_mode = (struct uvcx_rate_ctrl_mode*)data->data;
			rate_ctrl_mode->wLayerID = 1;
			rate_ctrl_mode->bRateControlMode = 1;
			break;
			
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_def, data->length);
			break;
			
		case UVC_GET_CUR:
			uvc_api_get_h264_rate_control_mode (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur.bRateControlMode);
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_rate_ctrl_mode_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_rate_ctrl_mode);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_temporal_scale_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	
	/* <2>"3.3.4 UVCX_TEMPORAL_SCALE_MODE", page 21 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_temporal_scale_mode );
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_temporal_scale_mode_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_temporal_scale_mode_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_temporal_scale_mode_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_temporal_scale_mode_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_temporal_scale_mode);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_spatial_scale_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.5 UVCX_SPATIAL_SCALE_MODE", page 22 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_spatial_scale_mode );
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_spatial_scale_mode_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_spatial_scale_mode_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_spatial_scale_mode_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_spatial_scale_mode_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_spatial_scale_mode);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_snr_scale_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.6 UVCX_SNR_SCALE_MODE", page 23 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_snr_scale_mode );
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:

			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_snr_scale_mode_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_snr_scale_mode_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_snr_scale_mode_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_snr_scale_mode_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_snr_scale_mode);
			break;

		case UVC_GET_INFO:
			ucam_strace("UVC_GET_INFO\n");
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_ltr_buffer_size_ctrl( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.7 UVCX_LTR_BUFFER_SIZE_CONTROL", page 27 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_ltr_buffer_size_ctrl );
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_buffer_size_ctrl_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_buffer_size_ctrl_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_buffer_size_ctrl_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_buffer_size_ctrl_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_ltr_buffer_size_ctrl);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_ltr_picture_ctrl( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.8 UVCX_LTR_PICTURE_CONTROL", page 28 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_ltr_picture_ctrl ); 
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_picture_ctrl_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_picture_ctrl_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_picture_ctrl_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_ltr_picture_ctrl_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_ltr_picture_ctrl);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_picture_type_ctrl( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.9 UVCX_PICTURE_TYPE_CONTROL", page  31 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 
	struct uvcx_picture_type_ctrl *picture_type;

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_picture_type_ctrl );
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				picture_type = (struct uvcx_picture_type_ctrl *)data->data;
				dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_cur.wLayerID = picture_type->wLayerID;
				if(picture_type->wPicType <= dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_max.wPicType){
					dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_cur.wPicType = picture_type->wPicType;
					if(uvc_api_set_h264_frame_requests(dev, dev->config.venc_chn, picture_type->wPicType) != 0)
					{
						ucam_error("uvc_api_set_h264_iframe_requests fail");
					}
				}
			}
			break;

		case UVC_GET_DEF:

			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_def, data->length);
			break;
		
		case UVC_GET_RES:
			picture_type = (struct uvcx_picture_type_ctrl*)data->data;
			picture_type->wLayerID = 1;
			picture_type->wPicType = 1;
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_picture_type_ctrl_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_picture_type_ctrl);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_version( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.10 UVCX_VERSION", page 32 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct device_version ); 
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_DEF:
			memcpy( data->data, &dev->uvc->config.DeviceVersion, data->length);
			break;
			
		case UVC_GET_RES:
			((uint32_t*)data->data)[0] = 1;
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->uvc->config.DeviceVersion, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->uvc->config.DeviceVersion, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->uvc->config.DeviceVersion, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			//((uint16_t*)data->data)[0] = sizeof ( struct uvcx_version);
			((uint16_t*)data->data)[0] = sizeof (struct device_version);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_encoder_reset( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.11.1 UVCX_ENCODER_RESET", page 32 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_encoder_reset ); 
	switch (req) {
		case UVC_SET_CUR:

			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_encoder_reset_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_encoder_reset_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_encoder_reset_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_encoder_reset_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_encoder_reset);
			break;

		case UVC_GET_INFO:
			ucam_strace("UVC_GET_INFO\n");
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_framerate_config( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.12 UVCX_FRAMERATE_CONFIG", page 33 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 
	struct uvcx_framerate_config *framerate_config;

	ucam_strace("enter");

	data->length = sizeof (struct uvcx_framerate_config ); 
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				framerate_config = (struct uvcx_framerate_config *)data->data;
				memcpy( &dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur, framerate_config, sizeof ( struct uvcx_framerate_config ));
				
				if(dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur.dwFrameInterval < dev->config.uvc_xu_h264_config.uvcx_framerate_config_min.dwFrameInterval)
					dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur.dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_framerate_config_min.dwFrameInterval;
				else if(dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur.dwFrameInterval > dev->config.uvc_xu_h264_config.uvcx_framerate_config_max.dwFrameInterval)
					dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur.dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_framerate_config_max.dwFrameInterval;
					
				dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwFrameInterval = dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur.dwFrameInterval;
				
				//设置uvc的视频帧率
				if(uvc_api_set_frame_interval (dev, dev->config.venc_chn, dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwFrameInterval) != 0)
				{
					ucam_error("uvc_api_set_frame_interval fail");
				}

			}
			break;

		case UVC_GET_RES:
			framerate_config = (struct uvcx_framerate_config*)data->data;
			framerate_config->wLayerID = 1;
			framerate_config->dwFrameInterval = 1;
			break;
			
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_framerate_config_def, data->length);
			break;

		case UVC_GET_CUR:
			uvc_api_get_frame_interval (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur.dwFrameInterval);
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_framerate_config_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_framerate_config_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_framerate_config_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_framerate_config);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_advance_config( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.13 UVCX_VIDEO_ADVANCE_CONFIG", page 34 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_video_advance_config );
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{

			}
			break;

		case UVC_GET_RES:
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_video_advance_config_def, data->length);
			break;

		case UVC_GET_CUR:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_video_advance_config_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_video_advance_config_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_video_advance_config_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_video_advance_config);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_bitrate_layer( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.14 UVCX_BITRATE_LAYERS", page 35 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 
	struct uvcx_bitrate_layers *bitrate_layers;

	ucam_strace("enter");

	data->length = sizeof ( struct uvcx_bitrate_layers ); 
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				bitrate_layers = (struct uvcx_bitrate_layers *)data->data;
				memcpy( &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur, bitrate_layers, sizeof ( struct uvcx_bitrate_layers ));
				
				if(dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwPeakBitrate < dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.dwPeakBitrate)
					dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwPeakBitrate = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.dwPeakBitrate;
				else if(dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwPeakBitrate > dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.dwPeakBitrate)
					dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwPeakBitrate = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.dwPeakBitrate;
				
				if(dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwAverageBitrate < dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.dwAverageBitrate)
					dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwAverageBitrate = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min.dwAverageBitrate;
				else if(dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwAverageBitrate > dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.dwAverageBitrate)
					dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwAverageBitrate = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max.dwAverageBitrate;
				
				if(dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bRateControlMode == UVCX_H264_bRateControlMode_CBR)
				{	
					dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwBitRate = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwAverageBitrate;
				}
				else
				{
					dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwBitRate = dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwPeakBitrate; 
				}
				
				//码率（kbps）设置	
				if(uvc_api_set_h264_bitrate (dev, dev->config.venc_chn, dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.dwBitRate) != 0)	
				{
					ucam_error("uvc_api_set_h264_bitrate fail\n");
				}	
				
			}
			break;
			
		case UVC_GET_RES:
			bitrate_layers = (struct uvcx_bitrate_layers*)data->data;
			bitrate_layers->wLayerID = 1;
			bitrate_layers->dwAverageBitrate = 1;
			bitrate_layers->dwPeakBitrate = 1;
			break;
			
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_def, data->length);
			break;

		case UVC_GET_CUR:
			if(dev->config.uvc_xu_h264_config.uvcx_h264_config_cur.bRateControlMode == UVCX_H264_bRateControlMode_CBR)
			{
				uvc_api_get_h264_bitrate (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwAverageBitrate);
			}
			else
			{
				uvc_api_get_h264_bitrate (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur.dwPeakBitrate);
			}	
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_bitrate_layers_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_bitrate_layers);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_xu_h264_qp_steps_layers( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	/* <2>"3.3.15 UVCX_QP_STEPS_LAYERS", page 36 */
	//uint8_t 	reqtype = ctrl->bRequestType & 80;
	uint8_t 	req = ctrl->bRequest; 
	struct uvcx_qp_steps_layers *qp_steps;

	ucam_strace("enter");
	
	data->length = sizeof ( struct uvcx_qp_steps_layers );
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				qp_steps = (struct uvcx_qp_steps_layers *)data->data;
				memcpy( &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_cur, qp_steps, sizeof ( struct uvcx_qp_steps_layers ));
					
				if(uvc_api_set_h264_qp_steps (dev, dev->config.venc_chn, dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_cur) != 0)	
				{
					ucam_error("uvc_api_set_h264_qp_steps fail\n");
				}	
				
			}
			break;

		case UVC_GET_RES:
			qp_steps = (struct uvcx_qp_steps_layers*)data->data;
			qp_steps->wLayerID = 1;
			qp_steps->bFrameType = 1;
			qp_steps->bMinQp = 1;
			qp_steps->bMaxQp = 1;
			break;
			
		case UVC_GET_DEF:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_def, data->length);
			break;

		case UVC_GET_CUR:
			uvc_api_get_h264_qp_steps (dev, dev->config.venc_chn, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_cur);
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_cur, data->length);
			break;

		case UVC_GET_MIN:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_min, data->length);
			break;

		case UVC_GET_MAX:
			memcpy( data->data, &dev->config.uvc_xu_h264_config.uvcx_qp_steps_layers_max, data->length);
			break;

		case UVC_GET_LEN:
			data->length = 2;
			((uint16_t*)data->data)[0] = sizeof ( struct uvcx_qp_steps_layers);
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_strace("unknown request type\n");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static uvc_requests_ctrl_fn_t uvc_requests_ctrl_xu_h264 [] = {
	NULL,
	uvc_requests_ctrl_xu_h264_config_probe,
	uvc_requests_ctrl_xu_h264_config_commit,
	uvc_requests_ctrl_xu_h264_rate_ctrl_mode,
	uvc_requests_ctrl_xu_h264_temporal_scale_mode,
	uvc_requests_ctrl_xu_h264_spatial_scale_mode,
	uvc_requests_ctrl_xu_h264_snr_scale_mode,
	uvc_requests_ctrl_xu_h264_ltr_buffer_size_ctrl,
	uvc_requests_ctrl_xu_h264_ltr_picture_ctrl,
	uvc_requests_ctrl_xu_h264_picture_type_ctrl,
	uvc_requests_ctrl_xu_h264_version,
	uvc_requests_ctrl_xu_h264_encoder_reset,
	uvc_requests_ctrl_xu_h264_framerate_config,
	uvc_requests_ctrl_xu_h264_advance_config,
	uvc_requests_ctrl_xu_h264_bitrate_layer,
	uvc_requests_ctrl_xu_h264_qp_steps_layers,
};


struct uvc_ctrl_attr_t uvc_ctrl_xu_h264= {
	.id = UVC_ENTITY_ID_XU_H264,
	.selectors_max_size = sizeof(uvc_requests_ctrl_xu_h264)/sizeof(uvc_requests_ctrl_fn_t),
	.init = init_ctrls_xu_h264,
	.requests_ctrl_fn = uvc_requests_ctrl_xu_h264,
};


struct uvc_ctrl_attr_t uvc_ctrl_xu_h264_s2= {
	.id = UVC_ENTITY_ID_XU_H264_S2,
	.selectors_max_size = sizeof(uvc_requests_ctrl_xu_h264)/sizeof(uvc_requests_ctrl_fn_t),
	.init = NULL,
	.requests_ctrl_fn = uvc_requests_ctrl_xu_h264,
};