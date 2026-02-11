/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl_vc.c
 * @Description : 
 */ 
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/uvc_api.h>

typedef union vc_video_power_mode_bmControls {
		/** raw register data */
	uint16_t d8;//8位
		/** register bits */
	struct {//低位在前[0]...[8]
		unsigned power_mode_setting:4;//RW
		/*
		0000B:Full power mode  
		0001B:device dependent 
		power mode (opt.) 
		All other bits are reserved. 
		*/
		unsigned device_dependent_power_mode_supported:1;//R
		unsigned device_uses_power_supplied_by_usb:1;//R
		unsigned device_uses_power_supplied_by_battery:1;//R
		unsigned device_uses_power_supplied_by_ac:1;//R

	} b;
} vc_video_power_mode_bmControls_t;



#define UVC_VC_VIDEO_POWER_MODE_DEF 	0x30		
#define UVC_VC_VIDEO_POWER_MODE_MIN 	0  	
#define UVC_VC_VIDEO_POWER_MODE_MAX 	0xFF
#define UVC_VC_VIDEO_POWER_MODE_RES 	1 
#define UVC_VC_VIDEO_POWER_MODE_INFO 	UVC_CONTROL_CAP_GET_SET 
#define UVC_VC_VIDEO_POWER_MODE_LEN 	1	
struct vc_video_power_mode_ctrl_s {
	vc_video_power_mode_bmControls_t cur;
}__attribute__((__packed__));

static struct vc_video_power_mode_ctrl_s vc_video_power_mode = {
	.cur.b.power_mode_setting = 0,
	.cur.b.device_dependent_power_mode_supported = 1,
	.cur.b.device_uses_power_supplied_by_usb = 1,
	.cur.b.device_uses_power_supplied_by_battery = 0,
	.cur.b.device_uses_power_supplied_by_ac = 0,
};



#define UVC_VC_REQUEST_ERROR_CODE_DEF 	UVC_REQUEST_ERROR_CODE_NO_ERROR		
#define UVC_VC_REQUEST_ERROR_CODE_MIN 	0  	
#define UVC_VC_REQUEST_ERROR_CODE_MAX 	0xFF
#define UVC_VC_REQUEST_ERROR_CODE_RES 	1 
#define UVC_VC_REQUEST_ERROR_CODE_INFO 	UVC_CONTROL_CAP_GET 
#define UVC_VC_REQUEST_ERROR_CODE_LEN 	1


static UvcRequestErrorCode_t uvc_requests_ctrl_vc_video_power_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	switch (req) {
		case UVC_SET_CUR:
#if 0			
			if(event_data)
			{
				vc_video_power_mode_bmControls_t cur = ((vc_video_power_mode_bmControls_t *)data->data)[0];
				if(cur.b.power_mode_setting > DEVICE_POWER_MODE_DEVICE_DEPENDENT_POWER)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;
				}
				vc_video_power_mode.cur.b.power_mode_setting = cur.b.power_mode_setting;
			}
#else			
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
#endif				
			
			break;
			
		case UVC_GET_CUR:
			((uint32_t*)data->data)[0] = vc_video_power_mode.cur.d8;
			break;
			
		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_VC_VIDEO_POWER_MODE_INFO;
			break;
			
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = UVC_VC_VIDEO_POWER_MODE_LEN;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;

		case UVC_GET_DEF:
			((uint32_t*)data->data)[0] = UVC_VC_VIDEO_POWER_MODE_DEF;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;
			
		case UVC_GET_MIN:
			((uint32_t*)data->data)[0] = UVC_VC_VIDEO_POWER_MODE_MIN;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;

		case UVC_GET_MAX:
			((uint32_t*)data->data)[0] = UVC_VC_VIDEO_POWER_MODE_MAX;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;

		case UVC_GET_RES:
			((uint32_t*)data->data)[0] = UVC_VC_VIDEO_POWER_MODE_RES;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}

	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_vc_request_error_code( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 

	ucam_strace("enter");

	switch (req) {
		case UVC_SET_CUR:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;
			
		case UVC_GET_CUR:
			((uint32_t*)data->data)[0] = dev->vc_request_error_code;
			ucam_strace( "uvc_get_request_error: %s(code:%d)", uvc_request_error_code_string(dev->vc_request_error_code), dev->vc_request_error_code);
			break;
			
		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_VC_REQUEST_ERROR_CODE_INFO;
			break;
			
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = UVC_VC_REQUEST_ERROR_CODE_DEF;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;

		case UVC_GET_DEF:
			((uint32_t*)data->data)[0] = UVC_VC_REQUEST_ERROR_CODE_DEF;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;
			
		case UVC_GET_MIN:
			((uint32_t*)data->data)[0] = UVC_VC_REQUEST_ERROR_CODE_MIN;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;

		case UVC_GET_MAX:
			((uint32_t*)data->data)[0] = UVC_VC_REQUEST_ERROR_CODE_MAX;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;

		case UVC_GET_RES:
			((uint32_t*)data->data)[0] = UVC_VC_REQUEST_ERROR_CODE_RES;
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;//no supported
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}

	return ret;
}

static int init_ctrls_vc(struct uvc_dev *dev)
{
	ucam_strace("enter");
	return 0;
}


static uvc_requests_ctrl_fn_t uvc_requests_ctrl_vc [] = {
	NULL,
	uvc_requests_ctrl_vc_video_power_mode,
	uvc_requests_ctrl_vc_request_error_code,
};

struct uvc_ctrl_attr_t uvc_ctrl_vc = {
	.id = 0,
	.selectors_max_size = sizeof(uvc_requests_ctrl_vc)/sizeof(uvc_requests_ctrl_fn_t),
	.init = init_ctrls_vc,
	.requests_ctrl_fn = uvc_requests_ctrl_vc,
};
