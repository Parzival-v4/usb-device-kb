/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl_eu.c
 * @Description : 
 */
#include <ucam/uvc/uvc_ctrl_eu.h> 
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_api_eu.h>
#include <ucam/uvc/uvc_api_stream.h>

int uvc_set_simulcast_enable(struct uvc_dev * dev, uint32_t enable)
{
	uint32_t simulcast_enable = enable;	
	
	if (ioctl(dev->fd, UVCIOC_SET_SIMULCAST_ENABLE, &simulcast_enable) < 0) {
		ucam_error("UVCIOC_SET_SIMULCAST_ENABLE: %s (%d) failed", strerror(errno), errno);	
		return -1;
	}

	dev->uvc150_simulcast_streamon = enable;

	ucam_notice("simulcast_en:%#x", dev->uvc150_simulcast_streamon);
	return 0;
}

uint32_t simulcast_stream_id_to_venc_chn(struct uvc_dev * dev, uint32_t stream_id)
{
	uint32_t chn = 0;
	
	
	if(dev->uvc150_simulcast_streamon)
	{
		if(stream_id == 1)
			chn = dev->config.simulcast_stream_1_venc_chn;
		else if(stream_id == 2)
			chn = dev->config.simulcast_stream_2_venc_chn;
		else
			chn = dev->config.simulcast_stream_0_venc_chn;
	}
	else
	{
		chn = dev->config.venc_chn;
	}

	return chn;
}

uint32_t venc_chn_to_simulcast_stream_id(struct uvc_dev * dev, uint32_t chn)
{
	uint32_t stream_id = 0;
		
	if(dev->uvc150_simulcast_streamon)
	{
		if(chn == dev->config.simulcast_stream_1_venc_chn)
			stream_id = 1;
		else if(chn == dev->config.simulcast_stream_2_venc_chn)
			stream_id = 2;
		else
			stream_id = 0;
	}
	else
	{
		stream_id = 0;	
	}

	return stream_id;
}

uint32_t venc_chn_to_simulcast_uvc_header_id(struct uvc_dev * dev, uint32_t chn)
{
	uint32_t uvc_header = 0x55550000;
	LayerOrViewIDBits_t layer_id;
	
	layer_id.d16 = 0;
		
	if(dev->uvc150_simulcast_streamon)
	{
		if(chn == dev->config.simulcast_stream_1_venc_chn)
		{
			layer_id.b.stream_id = 1;
		}
		else if(chn == dev->config.simulcast_stream_2_venc_chn)
		{
			layer_id.b.stream_id = 2;
		}
		else
		{
			layer_id.b.stream_id = 0;
		}
		
		uvc_header |= layer_id.d16;
	}
	else
	{
		uvc_header = 0;	
	}

	return uvc_header;
}

uint16_t get_eu_stream_id(struct uvc_dev * dev)
{
	return  dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.stream_id;
}

uint32_t get_simulcast_start_or_stop_layer(struct uvc_dev * dev, uint32_t chn)
{		
	return dev->config.uvc_encode_unit_attr_cur[venc_chn_to_simulcast_stream_id(dev, chn)].bUpdate;
}

uint32_t get_current_simulcast_venc_chn(struct uvc_dev * dev)
{
	return simulcast_stream_id_to_venc_chn(dev, get_eu_stream_id(dev));
}



UvcRequestErrorCode_t uvc_encode_unit_select_layer( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	
	uint8_t req = ctrl->bRequest;
	//uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_SELECT_LAYER_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.d16 = ((uint16_t*)data->data)[0];
				
				if(dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.stream_id >= SIMULCAST_MAX_LAYERS)
					dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.stream_id = SIMULCAST_MAX_LAYERS - 1;
						
				
				ucam_notice("set wLayerOrViewID = 0x%x , stream_id = %d, temporal_id = %d, quality_id = %d, dependency_id = %d", dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.d16, 
					dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.stream_id, dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.temporal_id, 
					dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.quality_id, dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.b.dependency_id);
			}
			break;

		case UVC_GET_CUR:
			((uint16_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.d16;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_SELECT_LAYER_CONTROL_LEN;
			data->length = 2;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

#define UVC_ENCODE_UNIT_ATTR_DEFAULT_PROFILE	0x4240
UvcRequestErrorCode_t uvc_encode_unit_profile ( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);
	ucam_strace("enter, req:%#x", req);
	data->length = EU_PROFILE_TOOLSET_CONTROL_LEN;

	
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile = ((uint16_t*)data->data)[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].wConstrainedToolset = ((uint16_t*)data->data)[1];
				dev->config.uvc_encode_unit_attr_cur[stream_id].bmSettings = ((uint8_t*)data->data)[4];
				ucam_trace("uvc eu ctrls set profile: %x \n", dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile);
				if(dev->H264SVCEnable == 0)
				{
					if(dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_BASELINEPROFILE || dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_CONSTRAINED_BASELINEPROFILE)
					{
						ucam_trace("Setting H.264 Encode Level to BASELINEPROFILE");
						if(uvc_api_set_h264_profile(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_wProfile_BASELINEPROFILE) != 0)
						{
							ucam_error("uvc_api_set_h264_profile fail");
						}
					}
					else if(dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_MAINPROFILE)
					{		
						ucam_trace("Setting H.264 Encode Level to MAINPROFILE");
						if(uvc_api_set_h264_profile(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_wProfile_MAINPROFILE) != 0)
						{
							ucam_error("uvc_api_set_h264_profile fail");
						}
					}
					else if(dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_HIGHPROFILE || dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_CONSTRAINED_HIGHPROFILE)
					{
						ucam_trace("Setting H.264 Encode Level to HIGHROFILE");
						if(uvc_api_set_h264_profile(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_wProfile_HIGHPROFILE) != 0)
						{
							ucam_error("uvc_api_set_h264_profile fail");
						}
					}
					else if(dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_SCALABLEBASELINEPROFILE || dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_CONSTRAINED_SCALABLEBASELINEPROFILE)
					{
						ucam_trace("Setting H.264 Encode Level to SCALABLEBASELINEPROFILE");
						if(uvc_api_set_h264_profile(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_wProfile_SCALABLEBASELINEPROFILE) != 0)
						{
							ucam_error("uvc_api_set_h264_profile fail");
						}
					}
					else if(dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_SCALABLEHIGHPROFILE || dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile == UVCX_H264_wProfile_CONSTRAINED_SCALABLEHIGHPROFILE)
					{
						ucam_trace("Setting H.264 Encode Level to SCALABLEHIGHPROFILE");
						if(uvc_api_set_h264_profile(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_wProfile_SCALABLEHIGHPROFILE) != 0)
						{
							ucam_error("uvc_api_set_h264_profile fail");
						}
					}
					else
					{
						ucam_trace("Defalt Setting H.264 Encode Level to MAINPROFILE");
					}
					
					if(dev->uvc_stream_on && get_simulcast_start_or_stop_layer(dev, get_current_simulcast_venc_chn(dev)))
					{
						ucam_strace("simulcast restart: chn: %d",get_current_simulcast_venc_chn(dev));
						if(uvc_api_set_stream_status (dev, get_current_simulcast_venc_chn(dev), 0) != 0)
						{
							ucam_error("uvc_api_set_stream_status fail");
							return -1;
						}

						if(uvc_api_set_stream_status (dev, get_current_simulcast_venc_chn(dev), 1) != 0)
						{
							ucam_error("uvc_api_set_stream_status fail");
							return -1;
						}

					}
				}
			}
			break;

		case UVC_GET_DEF:
			((uint16_t*)data->data)[0] = UVC_ENCODE_UNIT_ATTR_DEFAULT_PROFILE;
			((uint16_t*)data->data)[1] = 0;
			break;

		case UVC_GET_CUR:
			uvc_api_get_h264_profile (dev, get_current_simulcast_venc_chn(dev), &dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile);
			((uint16_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].wProfile;
			((uint16_t*)data->data)[1] = dev->config.uvc_encode_unit_attr_cur[stream_id].wConstrainedToolset;
			((uint8_t*)data->data)[4] = dev->config.uvc_encode_unit_attr_cur[stream_id].bmSettings;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_PROFILE_TOOLSET_CONTROL_LEN;
			data->length = 2;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_video_resolution_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_VIDEO_RESOLUTION_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].wWidth = ((uint16_t*)data->data)[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].wHeight = ((uint16_t*)data->data)[1];
				
				
				ucam_notice("uvc eu ctrls set Width : %d, Height : %d, venc_chn : %d\n",dev->config.uvc_encode_unit_attr_cur[stream_id].wWidth, 
						dev->config.uvc_encode_unit_attr_cur[stream_id].wHeight, 
				get_current_simulcast_venc_chn(dev));

				//设置uvc的视频分辨率
				if(uvc_api_set_resolution(dev, get_current_simulcast_venc_chn(dev), dev->config.uvc_encode_unit_attr_cur[stream_id].wWidth, 
					dev->config.uvc_encode_unit_attr_cur[stream_id].wHeight) != 0)
				{
					ucam_error("uvc_api_set_resolution fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			uvc_api_get_resolution (dev, get_current_simulcast_venc_chn(dev),
				&dev->config.uvc_encode_unit_attr_cur[stream_id].wWidth, &dev->config.uvc_encode_unit_attr_cur[stream_id].wHeight);
			((uint16_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].wWidth;
			((uint16_t*)data->data)[1] = dev->config.uvc_encode_unit_attr_cur[stream_id].wHeight;
			break;

		case UVC_GET_MIN:
			((uint16_t*)data->data)[0] = 640;
			((uint16_t*)data->data)[1] = 360;
			break;
		case UVC_GET_MAX:
			((uint16_t*)data->data)[0] = 3280;
			((uint16_t*)data->data)[1] = 2160;
			break;
		case UVC_GET_DEF:
			((uint16_t*)data->data)[0] = 1920;
			((uint16_t*)data->data)[1] = 1080;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_VIDEO_RESOLUTION_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_min_frame_interval_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_MIN_FRAME_INTERVAL_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].dwFrameInterval = ((uint32_t*)data->data)[0];
				//设置uvc的视频帧率
				ucam_notice("uvc eu ctrls set dwFrameInterval : %d, venc_chn : %d\n",dev->config.uvc_encode_unit_attr_cur[stream_id].dwFrameInterval, get_current_simulcast_venc_chn(dev));
				if(uvc_api_set_frame_interval(dev, get_current_simulcast_venc_chn(dev), dev->config.uvc_encode_unit_attr_cur[stream_id].dwFrameInterval) != 0)
				{
					ucam_error("uvc_api_set_frame_interval fail");
					//return -1;
				}
			}
			break;

		case UVC_GET_CUR:
			uvc_api_get_frame_interval(dev, get_current_simulcast_venc_chn(dev), &dev->config.uvc_encode_unit_attr_cur[stream_id].dwFrameInterval);
			((uint32_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].dwFrameInterval;
			break;

		case UVC_GET_MIN:
			((uint32_t*)data->data)[0] = 400000;
			break;
		case UVC_GET_MAX:
		case UVC_GET_DEF:
			((uint32_t*)data->data)[0] = 333333;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_MIN_FRAME_INTERVAL_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_slice_mode_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_SLICE_MODE_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceMode = ((uint16_t*)data->data)[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceConfigSetting = ((uint16_t*)data->data)[1];
				ucam_strace("set wSliceMode = 0x%x, wSliceConfigSetting = %d",((uint16_t*)data->data)[0],((uint16_t*)data->data)[1]);

				if(uvc_api_set_h264_slice (dev, get_current_simulcast_venc_chn(dev), dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceMode,
					dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceConfigSetting) != 0)
				{
					ucam_error("uvc_api_set_h264_slice fail");
				}
			}
			break;

		case UVC_GET_CUR:
			uvc_api_get_h264_slice (dev, get_current_simulcast_venc_chn(dev), &dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceMode,
					&dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceConfigSetting);
			((uint16_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceMode;
			((uint16_t*)data->data)[1] = dev->config.uvc_encode_unit_attr_cur[stream_id].wSliceConfigSetting;
			break;

		case UVC_GET_MIN:
		case UVC_GET_MAX:
		case UVC_GET_DEF:
			((uint16_t*)data->data)[0] = 2;
			((uint16_t*)data->data)[1] = 4;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_SLICE_MODE_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}

#define H264_ENCODE_VBR 1
#define H264_ENCODE_CBR 2
UvcRequestErrorCode_t uvc_encode_unit_rate_control_mode_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);
	uint8_t RateControlMode;

	ucam_strace("enter, req:%#x", req);
	data->length = EU_RATE_CONTROL_MODE_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode = ((uint8_t*)data->data)[0];
				ucam_trace("uvc_encode_unit_attr_cur.bRateControlMode: %d \n", dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode);
	
				if(dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode == H264_ENCODE_CBR)
				{
					//码率模式
					if(uvc_api_set_h264_rate_control_mode(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_bRateControlMode_CBR) != 0)
					{
						ucam_error("uvc_api_set_h264_rate_control_mode fail");
					}
				}
				else if(dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode == H264_ENCODE_VBR)
				{
					//码率模式
					if(uvc_api_set_h264_rate_control_mode(dev, get_current_simulcast_venc_chn(dev), UVCX_H264_bRateControlMode_VBR) != 0)
					{
						ucam_error("uvc_api_set_h264_rate_control_mode fail");
					}
				}
				else
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;
				}
				
				if(dev->uvc_stream_on && get_simulcast_start_or_stop_layer(dev, get_current_simulcast_venc_chn(dev)))
				{
					ucam_strace("simulcast restart: chn: %d",get_current_simulcast_venc_chn(dev));
					if(uvc_api_set_stream_status (dev, get_current_simulcast_venc_chn(dev), 0) != 0)
					{
						ucam_error("uvc_api_set_stream_status fail");
						return -1;
					}

					if(uvc_api_set_stream_status (dev, get_current_simulcast_venc_chn(dev), 1) != 0)
					{
						ucam_error("uvc_api_set_stream_status fail");
						return -1;
					}

				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_h264_rate_control_mode(dev, get_current_simulcast_venc_chn(dev), &RateControlMode) != 0)
			{
				ucam_error("uvc_api_get_h264_rate_control_mode fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}

			if(RateControlMode == UVCX_H264_bRateControlMode_CBR)
				dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode = H264_ENCODE_CBR;
			else if(RateControlMode == UVCX_H264_bRateControlMode_VBR)
				dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode = H264_ENCODE_VBR;
			else
				return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

			((uint8_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode;
			ucam_trace("uvc_encode_unit_attr_cur.bRateControlMode: %d \n", dev->config.uvc_encode_unit_attr_cur[stream_id].bRateControlMode);
			break;

		case UVC_GET_DEF:
			((uint8_t*)data->data)[0] = H264_ENCODE_VBR;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_RATE_CONTROL_MODE_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}


UvcRequestErrorCode_t uvc_encode_unit_average_bitrate_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_AVERAGE_BITRATE_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].dwAverageBitRate = ((uint32_t*)data->data)[0];
				ucam_trace("uvc_encode_unit_attr_cur.dwAverageBitRate: %d \n", dev->config.uvc_encode_unit_attr_cur[stream_id].dwAverageBitRate);
				if(uvc_api_set_h264_bitrate (dev, get_current_simulcast_venc_chn(dev), dev->config.uvc_encode_unit_attr_cur[stream_id].dwAverageBitRate) !=0)
				{
					ucam_error("uvc_api_set_h264_bitrate fail");
					//return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			uvc_api_get_h264_bitrate (dev, get_current_simulcast_venc_chn(dev), &dev->config.uvc_encode_unit_attr_cur[stream_id].dwAverageBitRate);
			((uint32_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].dwAverageBitRate;
			break;

		case UVC_GET_MIN:
			((uint32_t*)data->data)[0] = 512* 1024;
			break;
		case UVC_GET_MAX:
			((uint32_t*)data->data)[0] = 48000000;
			break;
		case UVC_GET_DEF:
			((uint32_t*)data->data)[0] = 3000000;
			break;
		case UVC_GET_RES:
			((uint32_t*)data->data)[0] = 1;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_AVERAGE_BITRATE_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_cpb_size_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_CPB_SIZE_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].dwCPBsize = ((uint32_t*)data->data)[0];
			}
			break;

		case UVC_GET_CUR:
			((uint32_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].dwCPBsize;
			break;

		case UVC_GET_MIN:
			((uint32_t*)data->data)[0] = 8000;
			break;
		case UVC_GET_MAX:
			((uint32_t*)data->data)[0] = 3000000;
			break;
		case UVC_GET_DEF:
			((uint32_t*)data->data)[0] = 187500;
			break;
		case UVC_GET_RES:
			((uint32_t*)data->data)[0] = 100;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_CPB_SIZE_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_quantization_params_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_QUANTIZATION_PARAMS_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].wQpPrime_I = ((uint16_t*)data->data)[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].wQpPrime_P = ((uint16_t*)data->data)[1] ;
				dev->config.uvc_encode_unit_attr_cur[stream_id].wQpPrime_B = ((uint16_t*)data->data)[2];
			}
			break;

		case UVC_GET_CUR:
			((uint16_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].wQpPrime_I;
			((uint16_t*)data->data)[1] = dev->config.uvc_encode_unit_attr_cur[stream_id].wQpPrime_P;
			((uint16_t*)data->data)[2] = dev->config.uvc_encode_unit_attr_cur[stream_id].wQpPrime_B;
			break;
		//最大最小值参考海思的 HiMPP IPC V2.0 媒体处理软件开发参考  P571
		case UVC_GET_MIN:
			((uint16_t*)data->data)[0] = 0;
			((uint16_t*)data->data)[1] = 0;
			((uint16_t*)data->data)[2] = 0;
			break;
		case UVC_GET_MAX:
			((uint16_t*)data->data)[0] = 51;
			((uint16_t*)data->data)[1] = 51;
			((uint16_t*)data->data)[2] = 51;
			break;
		case UVC_GET_DEF:
		case UVC_GET_RES:
			((uint16_t*)data->data)[0] = 25;
			((uint16_t*)data->data)[1] = 25;
			((uint16_t*)data->data)[2] = 25;
			break;
			break;

		case UVC_GET_INFO:
			data->length = 1;
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_QUANTIZATION_PARAMS_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}


UvcRequestErrorCode_t uvc_encode_unit_quantization_params_range_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_QP_RANGE_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].bMinQp = ((uint8_t*)data->data)[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].bMaxQp = ((uint8_t*)data->data)[1];
			}
			break;

		case UVC_GET_CUR:
			((uint8_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].bMinQp;
			((uint8_t*)data->data)[1] = dev->config.uvc_encode_unit_attr_cur[stream_id].bMaxQp;
			break;
		case UVC_GET_MIN:
		case UVC_GET_MAX:
		case UVC_GET_DEF:
			((uint8_t*)data->data)[0] = 10;
			((uint8_t*)data->data)[1] = 51;
			break;
		case UVC_GET_RES:
			((uint8_t*)data->data)[0] = 2;
			((uint8_t*)data->data)[1] = 0;
			break;

		case UVC_GET_INFO:
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_QUANTIZATION_PARAMS_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
		
	return ret;
}


UvcRequestErrorCode_t uvc_encode_unit_sync_ref_frame_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_SYNC_REF_FRAME_CONTROL_LEN;
	//uint8_t fps = 0;
	//uint16_t Gop = 0;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].bSyncFrameType = ((uint8_t*)data->data)[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].wSyncFrameInterval = ((uint16_t*)&data->data[1])[0];
				dev->config.uvc_encode_unit_attr_cur[stream_id].bGradualDecoderRefresh = ((uint8_t*)data->data)[3];

				if(uvc_api_set_h264_i_frame_period(dev, get_current_simulcast_venc_chn(dev), 
					dev->config.uvc_encode_unit_attr_cur[stream_id].wSyncFrameInterval) != 0)
				{
					ucam_error("uvc_api_set_h264_i_frame_period fail");
				}

				if(dev->config.uvc_encode_unit_attr_cur[stream_id].bGradualDecoderRefresh == 1)
				{
					if(get_simulcast_start_or_stop_layer(dev, get_current_simulcast_venc_chn(dev)))
						uvc_api_set_h264_frame_requests(dev, get_current_simulcast_venc_chn(dev), 1);
				}
			}
			break;

		case UVC_GET_CUR:
			uvc_api_get_h264_i_frame_period(dev, get_current_simulcast_venc_chn(dev), 
					&dev->config.uvc_encode_unit_attr_cur[stream_id].wSyncFrameInterval);
			((uint8_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].bSyncFrameType;
			((uint16_t*)&data->data[1])[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].wSyncFrameInterval;
			((uint8_t*)data->data)[3] = dev->config.uvc_encode_unit_attr_cur[stream_id].bGradualDecoderRefresh;
			break;

		case UVC_GET_MIN:
			 ((uint8_t*)data->data)[0] = 1;
			 ((uint16_t*)&data->data[1])[0] = 0;
			 ((uint8_t*)data->data)[3] = 0;
			break;

		case UVC_GET_MAX:
			 ((uint8_t*)data->data)[0] = 1;
			 ((uint16_t*)&data->data[1])[0] = 50000;
			 ((uint8_t*)data->data)[3] = 0;
			break;


		case UVC_GET_RES:
			 ((uint8_t*)data->data)[0] = 1;
			 ((uint16_t*)&data->data[1])[0] = 1;
			 ((uint8_t*)data->data)[3] = 1;
			break;

		case UVC_GET_DEF:
			 ((uint8_t*)data->data)[0] = 1;
			 ((uint16_t*)&data->data[1])[0] = 50000;
			 ((uint8_t*)data->data)[3] = 0;
			break;

		case UVC_GET_INFO:
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_SYNC_REF_FRAME_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_level_id_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_LEVEL_IDC_LIMIT_CONTROL_LEN;
	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].bLevelIDC = ((uint8_t*)data->data)[0];	
			}
			break;

		case UVC_GET_CUR:
			 ((uint8_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].bLevelIDC;
			break;

		case UVC_GET_MIN:
			 ((uint8_t*)data->data)[0] = 0x65;
			break;

		case UVC_GET_MAX:
			 ((uint8_t*)data->data)[0] = 0x28;//0x1E;
			break;


		case UVC_GET_RES:
			 ((uint8_t*)data->data)[0] = 1;
			break;

		case UVC_GET_DEF:
			 ((uint8_t*)data->data)[0] = 0x28;
			break;

		case UVC_GET_INFO:
			((uint8_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_LEVEL_IDC_LIMIT_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_sei_messages_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_SEI_PAYLOADTYPE_CONTROL_LEN;

	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].bmSEIMessages = ((uint64_t*)data->data)[0];
			}
			break;

		case UVC_GET_CUR:
			((uint64_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].bmSEIMessages;
			break;

		case UVC_GET_MIN:
			((uint64_t*)data->data)[0] = 0;
			break;

		case UVC_GET_MAX:
			((uint64_t*)data->data)[0] = 0;
			break;


		case UVC_GET_RES:
			((uint64_t*)data->data)[0] = 1;
			break;

		case UVC_GET_DEF:
			((uint64_t*)data->data)[0] = 0;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_SEI_PAYLOADTYPE_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

UvcRequestErrorCode_t uvc_encode_unit_start_or_stop_layer_control( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t req = ctrl->bRequest;
	uint16_t stream_id = get_eu_stream_id(dev);

	ucam_strace("enter, req:%#x", req);
	data->length = EU_START_OR_STOP_LAYER_CONTROL_LEN;

	switch(req)
	{
		case UVC_SET_CUR:
			if(event_data)
			{
				dev->config.uvc_encode_unit_attr_cur[stream_id].bUpdate = ((uint8_t*)data->data)[0];	
				if(dev->uvc_stream_on)
				{
					if(get_simulcast_start_or_stop_layer(dev, get_current_simulcast_venc_chn(dev)) == 0)
					{
						ucam_notice("simulcast stop: chn: %d",get_current_simulcast_venc_chn(dev));
						uvc_api_set_stream_status (dev, get_current_simulcast_venc_chn(dev), 0);
					}
					else 
					{
						ucam_notice("simulcast start: chn: %d",get_current_simulcast_venc_chn(dev));
						uvc_api_set_stream_status (dev, get_current_simulcast_venc_chn(dev), 1);				
					}
				}
			}
			break;

		case UVC_GET_CUR:
			((uint8_t*)data->data)[0] = dev->config.uvc_encode_unit_attr_cur[stream_id].bUpdate;
			break;

		case UVC_GET_MIN:
			((uint8_t*)data->data)[0] = 0;
			break;

		case UVC_GET_MAX:
			((uint8_t*)data->data)[0] = 1;
			break;


		case UVC_GET_RES:
			((uint8_t*)data->data)[0] = 1;
			break;

		case UVC_GET_DEF:
			((uint64_t*)data->data)[0] = 0;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = EU_SEI_PAYLOADTYPE_CONTROL_LEN;
			data->length = 2;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}





static int init_ctrls_eu(struct uvc_dev *dev)
{
	int i;

	ucam_strace("enter");

	dev->config.uvc_encode_unit_attr_cur[0].wLayerOrViewID.d16 = 0;//第一个stream

	dev->config.uvc_encode_unit_attr_cur[0].wProfile = UVC_ENCODE_UNIT_ATTR_DEFAULT_PROFILE;
	dev->config.uvc_encode_unit_attr_cur[0].wConstrainedToolset = 0;
	dev->config.uvc_encode_unit_attr_cur[0].bmSettings =0;

	dev->config.uvc_encode_unit_attr_cur[0].wWidth = 640;
	dev->config.uvc_encode_unit_attr_cur[0].wHeight = 360;


	dev->config.uvc_encode_unit_attr_cur[0].dwFrameInterval = 333333;

	dev->config.uvc_encode_unit_attr_cur[0].wSliceMode = 2;
	dev->config.uvc_encode_unit_attr_cur[0].wSliceConfigSetting = 1;//VHD_VENC_SUB_SLICE_SIZE;


	dev->config.uvc_encode_unit_attr_cur[0].bRateControlMode = H264_ENCODE_VBR;


	dev->config.uvc_encode_unit_attr_cur[0].dwAverageBitRate = 4096* 1024 * 1024U;

	dev->config.uvc_encode_unit_attr_cur[0].dwCPBsize  = 0;

	dev->config.uvc_encode_unit_attr_cur[0].dwPeakBitRate = 7281;


	dev->config.uvc_encode_unit_attr_cur[0].wQpPrime_I = 80;
	dev->config.uvc_encode_unit_attr_cur[0].wQpPrime_P = 80;
	dev->config.uvc_encode_unit_attr_cur[0].wQpPrime_B = 80;

	dev->config.uvc_encode_unit_attr_cur[0].bMinQp  = 1;
	dev->config.uvc_encode_unit_attr_cur[0].bMaxQp  = 99;

	dev->config.uvc_encode_unit_attr_cur[0].bSyncFrameType = 5;
	dev->config.uvc_encode_unit_attr_cur[0].wSyncFrameInterval = 5000;
	dev->config.uvc_encode_unit_attr_cur[0].bGradualDecoderRefresh = 1;

	dev->config.uvc_encode_unit_attr_cur[0].bNumHostControlLTRBuffers = 1;
	dev->config.uvc_encode_unit_attr_cur[0].bTrustMode = 0;

	dev->config.uvc_encode_unit_attr_cur[0].bPutAtPositionInLTRBuffer = 0;
	dev->config.uvc_encode_unit_attr_cur[0].bLTRMode = 1;

	dev->config.uvc_encode_unit_attr_cur[0].bmValidLTRs  = 1;

	dev->config.uvc_encode_unit_attr_cur[0].bmSEIMessages = 0;

	dev->config.uvc_encode_unit_attr_cur[0].bPriority  = 5;

	dev->config.uvc_encode_unit_attr_cur[0].bUpdate  = 0;

	dev->config.uvc_encode_unit_attr_cur[0].bLevelIDC  = 0x28;

	dev->config.uvc_encode_unit_attr_cur[0].bmErrorResiliencyFeatures = 0;

	
	for(i = 1; i < SIMULCAST_MAX_LAYERS;i++)
	{
		dev->config.uvc_encode_unit_attr_cur[i] = dev->config.uvc_encode_unit_attr_cur[0];
	}

	return 0;
}


static uvc_requests_ctrl_fn_t uvc_requests_ctrl_eu [] = {
	NULL,										//UNDEFINED
	uvc_encode_unit_select_layer,				//EU_SELECT_LAYER_CONTROL
	uvc_encode_unit_profile,					//EU_PROFILE_TOOLSET_CONTROL
	uvc_encode_unit_video_resolution_control,	//EU_VIDEO_RESOLUTION_CONTROL
	uvc_encode_unit_min_frame_interval_control,	//EU_MIN_FRAME_INTERVAL_CONTROL
	uvc_encode_unit_slice_mode_control,			//EU_SLICE_MODE_CONTROL
	uvc_encode_unit_rate_control_mode_control,	//EU_RATE_CONTROL_MODE_CONTROL
	uvc_encode_unit_average_bitrate_control,	//EU_AVERAGE_BITRATE_CONTROL
	uvc_encode_unit_cpb_size_control,			//EU_CPB_SIZE_CONTROL
	NULL,										//EU_PEAK_BIT_RATE_CONTROL
	uvc_encode_unit_quantization_params_control,//EU_QUANTIZATION_PARAMS_CONTROL
	uvc_encode_unit_sync_ref_frame_control,		//EU_SYNC_REF_FRAME_CONTROL
	NULL,										//EU_LTR_BUFFER_CONTROL
	NULL,										//EU_LTR_PICTURE_CONTROL
	NULL,										//EU_LTR_VALIDATION_CONTROL
	uvc_encode_unit_level_id_control,			//EU_LEVEL_IDC_LIMIT_CONTROL
	uvc_encode_unit_sei_messages_control,		//EU_SEI_PAYLOADTYPE_CONTROL
	uvc_encode_unit_quantization_params_range_control,//EU_QP_RANGE_CONTROL
	NULL,										//EU_PRIORITY_CONTROL
	uvc_encode_unit_start_or_stop_layer_control,//EU_START_OR_STOP_LAYER_CONTROL
	NULL,										//EU_ERROR_RESILIENCY_CONTROL
};

struct uvc_ctrl_attr_t uvc_ctrl_eu= {
	.id = UVC_ENTITY_ID_ENCODING_UNIT,
	.selectors_max_size = sizeof(uvc_requests_ctrl_eu)/sizeof(uvc_requests_ctrl_fn_t),
	.init = init_ctrls_eu,
	.requests_ctrl_fn = uvc_requests_ctrl_eu,
};