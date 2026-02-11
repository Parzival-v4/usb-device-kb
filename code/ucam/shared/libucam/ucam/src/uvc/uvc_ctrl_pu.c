/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl_pu.c
 * @Description : 
 */ 
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_api_pu.h>

static int init_ctrls_pu(struct uvc_dev *dev);


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_backlight_compensation( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.1  Backlight Compensation Control",page 93 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_backlight_compensation backlight;
	
	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.backlight_compensation == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_backlight_compensation))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				backlight = ((struct uvc_pu_backlight_compensation*)data->data)[0];

				if(backlight.wBacklightCompensation > dev->config.uvc_pu_config.backlight_compensation.max.wBacklightCompensation || 
					backlight.wBacklightCompensation < dev->config.uvc_pu_config.backlight_compensation.min.wBacklightCompensation)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.backlight_compensation.cur = backlight;
				if(uvc_api_set_pu_backlight_compensation (dev, dev->config.uvc_pu_config.backlight_compensation.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_backlight_compensation fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
					
			}
			break;
		case UVC_GET_CUR:
			if(uvc_api_get_pu_backlight_compensation (dev, &dev->config.uvc_pu_config.backlight_compensation.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_backlight_compensation fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_backlight_compensation*)data->data)[0] = dev->config.uvc_pu_config.backlight_compensation.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_backlight_compensation*)data->data)[0] = dev->config.uvc_pu_config.backlight_compensation.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_backlight_compensation*)data->data)[0] = dev->config.uvc_pu_config.backlight_compensation.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_backlight_compensation*)data->data)[0] = dev->config.uvc_pu_config.backlight_compensation.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_backlight_compensation*)data->data)[0] = dev->config.uvc_pu_config.backlight_compensation.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_backlight_compensation);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_brightness( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* "4.2.2.3.2  Brightness Control", page 93. */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 

	struct uvc_pu_brightness brightness;
	
	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.brightness == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_brightness))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				brightness = ((struct uvc_pu_brightness*)data->data)[0];

				if(brightness.wBrightness > dev->config.uvc_pu_config.brightness.max.wBrightness ||
					 brightness.wBrightness < dev->config.uvc_pu_config.brightness.min.wBrightness)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.brightness.cur = brightness;
				if(uvc_api_set_pu_brightness (dev, dev->config.uvc_pu_config.brightness.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_brightness fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}			
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_brightness (dev, &dev->config.uvc_pu_config.brightness.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_brightness fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_brightness*)data->data)[0] = dev->config.uvc_pu_config.brightness.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_brightness*)data->data)[0] = dev->config.uvc_pu_config.brightness.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_brightness*)data->data)[0] = dev->config.uvc_pu_config.brightness.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_brightness*)data->data)[0] = dev->config.uvc_pu_config.brightness.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_brightness*)data->data)[0] = dev->config.uvc_pu_config.brightness.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_brightness);
			break;			

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_contrast( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.3  Contrast Control",page 93*/
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 

	struct uvc_pu_contrast contrast;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.contrast == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_contrast))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				contrast = ((struct uvc_pu_contrast*)data->data)[0];
				if(contrast.wContrast > dev->config.uvc_pu_config.contrast.max.wContrast ||
					 contrast.wContrast < dev->config.uvc_pu_config.contrast.min.wContrast)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.contrast.cur = contrast;
				if(uvc_api_set_pu_contrast(dev, dev->config.uvc_pu_config.contrast.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_contrast fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}

			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_contrast(dev, &dev->config.uvc_pu_config.contrast.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_contrast fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_contrast*)data->data)[0] = dev->config.uvc_pu_config.contrast.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_contrast*)data->data)[0] = dev->config.uvc_pu_config.contrast.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_contrast*)data->data)[0] = dev->config.uvc_pu_config.contrast.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_contrast*)data->data)[0] = dev->config.uvc_pu_config.contrast.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_contrast*)data->data)[0] = dev->config.uvc_pu_config.contrast.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_contrast);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_gain( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.4  Gain Control",page */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_gain gain;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.gain == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_gain))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				gain = ((struct uvc_pu_gain*)data->data)[0];
				if(gain.wGain > dev->config.uvc_pu_config.gain.max.wGain ||
					 gain.wGain < dev->config.uvc_pu_config.gain.min.wGain)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.gain.cur = gain;
				if(uvc_api_set_pu_gain(dev, dev->config.uvc_pu_config.gain.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_gain fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_gain(dev, &dev->config.uvc_pu_config.gain.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_gain fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_gain*)data->data)[0] = dev->config.uvc_pu_config.gain.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_gain*)data->data)[0] = dev->config.uvc_pu_config.gain.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_gain*)data->data)[0] = dev->config.uvc_pu_config.gain.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_gain*)data->data)[0] = dev->config.uvc_pu_config.gain.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_gain*)data->data)[0] = dev->config.uvc_pu_config.gain.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_gain);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_power_line_frequency( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.5  Power Line Frequency Control", page 94 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_power_line_frequency power_line_frequency;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.power_line_frequency == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_power_line_frequency))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				power_line_frequency = ((struct uvc_pu_power_line_frequency*)data->data)[0];
				if(power_line_frequency.bPowerLineFrequency > dev->config.uvc_pu_config.power_line_frequency.max.bPowerLineFrequency ||
					 power_line_frequency.bPowerLineFrequency < dev->config.uvc_pu_config.power_line_frequency.min.bPowerLineFrequency)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.power_line_frequency.cur = power_line_frequency;
				if(uvc_api_set_pu_power_line_frequency(dev, dev->config.uvc_pu_config.power_line_frequency.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_power_line_frequency fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:	
			if(uvc_api_get_pu_power_line_frequency(dev, &dev->config.uvc_pu_config.power_line_frequency.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_power_line_frequency fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_power_line_frequency*)data->data)[0] = dev->config.uvc_pu_config.power_line_frequency.cur;
			break;
		case UVC_GET_MIN:		
			((struct uvc_pu_power_line_frequency*)data->data)[0] = dev->config.uvc_pu_config.power_line_frequency.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_power_line_frequency*)data->data)[0] = dev->config.uvc_pu_config.power_line_frequency.max;
			break;
		case UVC_GET_DEF:
			((struct uvc_pu_power_line_frequency*)data->data)[0] = dev->config.uvc_pu_config.power_line_frequency.def;
			break;
		case UVC_GET_RES:
			((struct uvc_pu_power_line_frequency*)data->data)[0] = dev->config.uvc_pu_config.power_line_frequency.res;
			break;
		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_power_line_frequency);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_hue( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.6  Hue Control ", page 95 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest;
	struct uvc_pu_hue hue; 

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.hue == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_hue))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				hue = ((struct uvc_pu_hue*)data->data)[0];
				if(hue.wHue > dev->config.uvc_pu_config.hue.max.wHue ||
					hue.wHue < dev->config.uvc_pu_config.hue.min.wHue)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.hue.cur = hue;
				if(uvc_api_set_pu_hue(dev, dev->config.uvc_pu_config.hue.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_hue fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}				
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_hue(dev, &dev->config.uvc_pu_config.hue.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_hue fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_hue*)data->data)[0] = dev->config.uvc_pu_config.hue.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_hue*)data->data)[0] = dev->config.uvc_pu_config.hue.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_hue*)data->data)[0] = dev->config.uvc_pu_config.hue.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_hue*)data->data)[0] = dev->config.uvc_pu_config.hue.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_hue*)data->data)[0] = dev->config.uvc_pu_config.hue.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_hue);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_saturation( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.8   Saturation Control", page 96*/
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_saturation saturation;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.saturation == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_saturation))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				saturation = ((struct uvc_pu_saturation*)data->data)[0];
				if(saturation.wSaturation > dev->config.uvc_pu_config.saturation.max.wSaturation ||
					saturation.wSaturation < dev->config.uvc_pu_config.saturation.min.wSaturation)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.saturation.cur = saturation;
				if(uvc_api_set_pu_saturation(dev, dev->config.uvc_pu_config.saturation.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_saturation fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_saturation(dev, &dev->config.uvc_pu_config.saturation.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_saturation fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_saturation*)data->data)[0] = dev->config.uvc_pu_config.saturation.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_saturation*)data->data)[0] = dev->config.uvc_pu_config.saturation.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_saturation*)data->data)[0] = dev->config.uvc_pu_config.saturation.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_saturation*)data->data)[0] = dev->config.uvc_pu_config.saturation.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_saturation*)data->data)[0] = dev->config.uvc_pu_config.saturation.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_saturation);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_sharpness( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.9   Sharpness Control", page 96 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest;
	struct uvc_pu_sharpness sharpness; 

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.sharpness == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_sharpness))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				sharpness = ((struct uvc_pu_sharpness*)data->data)[0];
				if(sharpness.wSharpness > dev->config.uvc_pu_config.sharpness.max.wSharpness ||
					sharpness.wSharpness < dev->config.uvc_pu_config.sharpness.min.wSharpness)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.sharpness.cur = sharpness;
				if(uvc_api_set_pu_sharpness(dev, dev->config.uvc_pu_config.sharpness.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_sharpness fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_sharpness(dev, &dev->config.uvc_pu_config.sharpness.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_sharpness fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_sharpness*)data->data)[0] = dev->config.uvc_pu_config.sharpness.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_sharpness*)data->data)[0] = dev->config.uvc_pu_config.sharpness.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_sharpness*)data->data)[0] = dev->config.uvc_pu_config.sharpness.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_sharpness*)data->data)[0] = dev->config.uvc_pu_config.sharpness.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_sharpness*)data->data)[0] = dev->config.uvc_pu_config.sharpness.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_sharpness);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_gamma( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.10  Gamma Control ", page 97 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_gamma gamma;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.gamma == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_gamma))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				gamma = ((struct uvc_pu_gamma*)data->data)[0];
				if(gamma.wGamma > dev->config.uvc_pu_config.gamma.max.wGamma ||
					gamma.wGamma < dev->config.uvc_pu_config.gamma.min.wGamma)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.gamma.cur = gamma;
				if(uvc_api_set_pu_gamma(dev, dev->config.uvc_pu_config.gamma.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_gamma fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:	
			if(uvc_api_get_pu_gamma(dev, &dev->config.uvc_pu_config.gamma.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_gamma fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_gamma*)data->data)[0] = dev->config.uvc_pu_config.gamma.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_gamma*)data->data)[0] = dev->config.uvc_pu_config.gamma.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_gamma*)data->data)[0] = dev->config.uvc_pu_config.gamma.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_gamma*)data->data)[0] = dev->config.uvc_pu_config.gamma.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_gamma*)data->data)[0] = dev->config.uvc_pu_config.gamma.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_gamma);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_white_balance_temperature( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.11  White Balance Temperature Control", page 97 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_white_balance_temperature white_balance_temperature;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.white_balance_temperature == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:		
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_white_balance_temperature))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				white_balance_temperature = ((struct uvc_pu_white_balance_temperature*)data->data)[0];
				if(white_balance_temperature.wWhiteBalanceTemperature > dev->config.uvc_pu_config.white_balance_temperature.max.wWhiteBalanceTemperature ||
					white_balance_temperature.wWhiteBalanceTemperature < dev->config.uvc_pu_config.white_balance_temperature.min.wWhiteBalanceTemperature)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.white_balance_temperature.cur = white_balance_temperature;
				if(uvc_api_set_pu_white_balance_temperature(dev, dev->config.uvc_pu_config.white_balance_temperature.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_white_balance_temperature fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_white_balance_temperature(dev, &dev->config.uvc_pu_config.white_balance_temperature.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_white_balance_temperature fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_white_balance_temperature*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_white_balance_temperature*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_white_balance_temperature*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_white_balance_temperature*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_white_balance_temperature*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_white_balance_temperature);
			break;
			
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_white_balance_temperature_auto( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.12  White Balance Temperature, Auto Control ", page 98 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_white_balance_temperature_auto white_balance_temperature_auto;

	ucam_strace("enter");

	
	if(dev->config.processing_unit_bmControls.b.white_balance_temperature_auto == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_white_balance_temperature_auto))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				white_balance_temperature_auto = ((struct uvc_pu_white_balance_temperature_auto*)data->data)[0];
				if(white_balance_temperature_auto.bWhiteBalanceTemperatureAuto > dev->config.uvc_pu_config.white_balance_temperature_auto.max.bWhiteBalanceTemperatureAuto ||
					white_balance_temperature_auto.bWhiteBalanceTemperatureAuto < dev->config.uvc_pu_config.white_balance_temperature_auto.min.bWhiteBalanceTemperatureAuto)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.white_balance_temperature_auto.cur = white_balance_temperature_auto;
				if(uvc_api_set_pu_white_balance_temperature_auto(dev, dev->config.uvc_pu_config.white_balance_temperature_auto.cur) != 0)
				{
					ucam_error("uvc_api_get_pu_white_balance_temperature_auto fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_white_balance_temperature_auto(dev, &dev->config.uvc_pu_config.white_balance_temperature_auto.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_white_balance_temperature_auto fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_white_balance_temperature_auto*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature_auto.cur;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_white_balance_temperature_auto*)data->data)[0] = dev->config.uvc_pu_config.white_balance_temperature_auto.def;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_RES:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
			break;
		case UVC_GET_MIN:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
			break;
		case UVC_GET_MAX:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
			break;
		case UVC_GET_LEN:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_white_balance_component( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.13 White Balance Component Control ", page 98 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_white_balance_component white_balance_component;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.white_balance_component == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_white_balance_component))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				white_balance_component = ((struct uvc_pu_white_balance_component*)data->data)[0];
				if(white_balance_component.wWhiteBalanceBlue > dev->config.uvc_pu_config.white_balance_component.max.wWhiteBalanceBlue ||
					white_balance_component.wWhiteBalanceBlue < dev->config.uvc_pu_config.white_balance_component.min.wWhiteBalanceBlue)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				if(white_balance_component.wWhiteBalanceRed > dev->config.uvc_pu_config.white_balance_component.max.wWhiteBalanceRed ||
					white_balance_component.wWhiteBalanceRed < dev->config.uvc_pu_config.white_balance_component.min.wWhiteBalanceRed)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.white_balance_component.cur = white_balance_component;
				if(uvc_api_set_pu_white_balance_component(dev, dev->config.uvc_pu_config.white_balance_component.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_white_balance_component fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_white_balance_component(dev, &dev->config.uvc_pu_config.white_balance_component.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_white_balance_component fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_white_balance_component*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_white_balance_component*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_white_balance_component*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_white_balance_component*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_white_balance_component*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_white_balance_component);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_white_balance_component_auto( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.14  White Balance Component, Auto Control", page 99 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_white_balance_component_auto white_balance_component_auto;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.white_balance_component_auto == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_white_balance_component_auto))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				white_balance_component_auto = ((struct uvc_pu_white_balance_component_auto*)data->data)[0];
				if(white_balance_component_auto.bWhiteBalanceComponentAuto > dev->config.uvc_pu_config.white_balance_component_auto.max.bWhiteBalanceComponentAuto ||
					white_balance_component_auto.bWhiteBalanceComponentAuto < dev->config.uvc_pu_config.white_balance_component_auto.min.bWhiteBalanceComponentAuto)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.white_balance_component_auto.cur = white_balance_component_auto;
				if(uvc_api_set_pu_white_balance_component_auto(dev, dev->config.uvc_pu_config.white_balance_component_auto.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_white_balance_component_auto fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_white_balance_component_auto(dev, &dev->config.uvc_pu_config.white_balance_component_auto.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_white_balance_component_auto fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_white_balance_component_auto*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component_auto.cur;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_white_balance_component_auto*)data->data)[0] = dev->config.uvc_pu_config.white_balance_component_auto.def;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_white_balance_component_auto);
			break;
			
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_digital_multiplier( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.15  Digital Multiplier Control", page 99 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_digital_multiplier digital_multiplier;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.digital_multiplier == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_digital_multiplier))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				digital_multiplier = ((struct uvc_pu_digital_multiplier*)data->data)[0];
				if(digital_multiplier.wMultiplierStep > dev->config.uvc_pu_config.digital_multiplier.max.wMultiplierStep ||
					digital_multiplier.wMultiplierStep < dev->config.uvc_pu_config.digital_multiplier.min.wMultiplierStep)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.digital_multiplier.cur = digital_multiplier;
				if(uvc_api_set_pu_digital_multiplier(dev, dev->config.uvc_pu_config.digital_multiplier.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_digital_multiplier fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_digital_multiplier(dev, &dev->config.uvc_pu_config.digital_multiplier.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_digital_multiplier fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_digital_multiplier*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_digital_multiplier*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_digital_multiplier*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_digital_multiplier*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_digital_multiplier*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_digital_multiplier);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_digital_multiplier_limit( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.16 Digital Multiplier Limit Control", page 99 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_digital_multiplier_limit digital_multiplier_limit;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.digital_multiplier_limit == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_digital_multiplier_limit))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				digital_multiplier_limit = ((struct uvc_pu_digital_multiplier_limit*)data->data)[0];
				if(digital_multiplier_limit.wMultiplierLimit > dev->config.uvc_pu_config.digital_multiplier_limit.max.wMultiplierLimit ||
					digital_multiplier_limit.wMultiplierLimit < dev->config.uvc_pu_config.digital_multiplier_limit.min.wMultiplierLimit)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.digital_multiplier_limit.cur = digital_multiplier_limit;
				if(uvc_api_set_pu_digital_multiplier_limit(dev, dev->config.uvc_pu_config.digital_multiplier_limit.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_digital_multiplier_limit fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_digital_multiplier_limit(dev, &dev->config.uvc_pu_config.digital_multiplier_limit.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_digital_multiplier_limit fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_digital_multiplier_limit*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier_limit.cur;
			break;

		case UVC_GET_MIN:
			((struct uvc_pu_digital_multiplier_limit*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier_limit.min;
			break;

		case UVC_GET_MAX:
			((struct uvc_pu_digital_multiplier_limit*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier_limit.max;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_digital_multiplier_limit*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier_limit.def;
			break;

		case UVC_GET_RES:
			((struct uvc_pu_digital_multiplier_limit*)data->data)[0] = dev->config.uvc_pu_config.digital_multiplier_limit.res;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_digital_multiplier_limit);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}


static UvcRequestErrorCode_t uvc_requests_ctrl_pu_hue_auto( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.3.7   Hue, Auto Control", page 95 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_pu_hue_auto hue_auto;

	ucam_strace("enter");
	
	if(dev->config.processing_unit_bmControls.b.hue_auto == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_pu_hue_auto))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				
				hue_auto = ((struct uvc_pu_hue_auto*)data->data)[0];
				if(hue_auto.bHueAuto > dev->config.uvc_pu_config.hue_auto.max.bHueAuto ||
					hue_auto.bHueAuto < dev->config.uvc_pu_config.hue_auto.min.bHueAuto)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_pu_config.hue_auto.cur = hue_auto;
				if(uvc_api_set_pu_hue_auto(dev, dev->config.uvc_pu_config.hue_auto.cur) != 0)
				{
					ucam_error("uvc_api_set_pu_hue_auto fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_pu_hue_auto(dev, &dev->config.uvc_pu_config.hue_auto.cur) != 0)
			{
				ucam_error("uvc_api_get_pu_hue_auto fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			((struct uvc_pu_hue_auto*)data->data)[0] = dev->config.uvc_pu_config.hue_auto.cur;
			break;

		case UVC_GET_DEF:
			((struct uvc_pu_hue_auto*)data->data)[0] = dev->config.uvc_pu_config.hue_auto.def;
			break;

		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			((uint32_t*)data->data)[0] = sizeof(struct uvc_pu_hue_auto);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}

	return ret;
}

static uvc_requests_ctrl_fn_t uvc_requests_ctrl_pu [] = {
	NULL,
	uvc_requests_ctrl_pu_backlight_compensation,
	uvc_requests_ctrl_pu_brightness,
	uvc_requests_ctrl_pu_contrast,
	uvc_requests_ctrl_pu_gain,
	uvc_requests_ctrl_pu_power_line_frequency,
	uvc_requests_ctrl_pu_hue,
	uvc_requests_ctrl_pu_saturation,
	uvc_requests_ctrl_pu_sharpness,
	uvc_requests_ctrl_pu_gamma,
	uvc_requests_ctrl_pu_white_balance_temperature,
	uvc_requests_ctrl_pu_white_balance_temperature_auto,
	uvc_requests_ctrl_pu_white_balance_component,
	uvc_requests_ctrl_pu_white_balance_component_auto,
	uvc_requests_ctrl_pu_digital_multiplier,
	uvc_requests_ctrl_pu_digital_multiplier_limit,
	uvc_requests_ctrl_pu_hue_auto,
};

struct uvc_ctrl_attr_t uvc_ctrl_pu = {
	//.id = UVC_ENTITY_ID_PROCESSING_UNIT,
	.selectors_max_size = sizeof(uvc_requests_ctrl_pu)/sizeof(uvc_requests_ctrl_fn_t),
	.init = init_ctrls_pu,
	.requests_ctrl_fn = uvc_requests_ctrl_pu,
};

static int init_ctrls_pu(struct uvc_dev *dev)
{
	ucam_strace("enter");
	uvc_ctrl_pu.id = dev->config.processing_unit_id;
	dev->config.uvc_pu_config.backlight_compensation.def.wBacklightCompensation = 0;
	dev->config.uvc_pu_config.backlight_compensation.min.wBacklightCompensation = 0;
	dev->config.uvc_pu_config.backlight_compensation.max.wBacklightCompensation = 1;
	dev->config.uvc_pu_config.backlight_compensation.res.wBacklightCompensation = 1;
	if(dev->config.processing_unit_bmControls.b.backlight_compensation)
		uvc_api_get_attrs_pu_backlight_compensation(dev, &dev->config.uvc_pu_config.backlight_compensation);
	dev->config.uvc_pu_config.backlight_compensation.cur = dev->config.uvc_pu_config.backlight_compensation.def;

	dev->config.uvc_pu_config.brightness.def.wBrightness = 7;
	dev->config.uvc_pu_config.brightness.min.wBrightness = 0;
	dev->config.uvc_pu_config.brightness.max.wBrightness = 14;
	dev->config.uvc_pu_config.brightness.res.wBrightness = 1;
	if(dev->config.processing_unit_bmControls.b.brightness)
		uvc_api_get_attrs_pu_brightness(dev, &dev->config.uvc_pu_config.brightness);
	dev->config.uvc_pu_config.brightness.cur = dev->config.uvc_pu_config.brightness.def;

	dev->config.uvc_pu_config.contrast.def.wContrast = 8;
	dev->config.uvc_pu_config.contrast.min.wContrast = 0;
	dev->config.uvc_pu_config.contrast.max.wContrast = 14;
	dev->config.uvc_pu_config.contrast.res.wContrast = 1;
	if(dev->config.processing_unit_bmControls.b.contrast)
		uvc_api_get_attrs_pu_contrast(dev, &dev->config.uvc_pu_config.contrast);
	dev->config.uvc_pu_config.contrast.cur = dev->config.uvc_pu_config.contrast.def;

	dev->config.uvc_pu_config.gain.def.wGain = 6;
	dev->config.uvc_pu_config.gain.min.wGain = 0;
	dev->config.uvc_pu_config.gain.max.wGain = 64;
	dev->config.uvc_pu_config.gain.res.wGain = 1;
	if(dev->config.processing_unit_bmControls.b.gain)
		uvc_api_get_attrs_pu_gain(dev, &dev->config.uvc_pu_config.gain);
	dev->config.uvc_pu_config.gain.cur = dev->config.uvc_pu_config.gain.def;

	dev->config.uvc_pu_config.power_line_frequency.def.bPowerLineFrequency = 1;
	dev->config.uvc_pu_config.power_line_frequency.min.bPowerLineFrequency = 0;
	dev->config.uvc_pu_config.power_line_frequency.max.bPowerLineFrequency = 2;
	dev->config.uvc_pu_config.power_line_frequency.res.bPowerLineFrequency = 1;
	if(dev->config.processing_unit_bmControls.b.power_line_frequency)
		uvc_api_get_attrs_pu_power_line_frequency(dev, &dev->config.uvc_pu_config.power_line_frequency);
	dev->config.uvc_pu_config.power_line_frequency.cur = dev->config.uvc_pu_config.power_line_frequency.def;

	dev->config.uvc_pu_config.hue.def.wHue = 7;
	dev->config.uvc_pu_config.hue.min.wHue = 0;
	dev->config.uvc_pu_config.hue.max.wHue = 14;
	dev->config.uvc_pu_config.hue.res.wHue = 1;
	if(dev->config.processing_unit_bmControls.b.hue)
		uvc_api_get_attrs_pu_hue(dev, &dev->config.uvc_pu_config.hue);
	dev->config.uvc_pu_config.hue.cur = dev->config.uvc_pu_config.hue.def;

	dev->config.uvc_pu_config.saturation.def.wSaturation = 90;
	dev->config.uvc_pu_config.saturation.min.wSaturation = 60;
	dev->config.uvc_pu_config.saturation.max.wSaturation = 200;
	dev->config.uvc_pu_config.saturation.res.wSaturation = 10;
	if(dev->config.processing_unit_bmControls.b.saturation)
		uvc_api_get_attrs_pu_saturation(dev, &dev->config.uvc_pu_config.saturation);
	dev->config.uvc_pu_config.saturation.cur = dev->config.uvc_pu_config.saturation.def;

	dev->config.uvc_pu_config.sharpness.def.wSharpness = 4;
	dev->config.uvc_pu_config.sharpness.min.wSharpness = 0;
	dev->config.uvc_pu_config.sharpness.max.wSharpness = 14;
	dev->config.uvc_pu_config.sharpness.res.wSharpness = 1;
	if(dev->config.processing_unit_bmControls.b.sharpness)
		uvc_api_get_attrs_pu_sharpness(dev, &dev->config.uvc_pu_config.sharpness);
	dev->config.uvc_pu_config.sharpness.cur = dev->config.uvc_pu_config.sharpness.def;

	dev->config.uvc_pu_config.gamma.def.wGamma = 200;
	dev->config.uvc_pu_config.gamma.min.wGamma = 159;
	dev->config.uvc_pu_config.gamma.max.wGamma = 238;
	dev->config.uvc_pu_config.gamma.res.wGamma = 1;
	if(dev->config.processing_unit_bmControls.b.gamma)
		uvc_api_get_attrs_pu_gamma(dev, &dev->config.uvc_pu_config.gamma);
	dev->config.uvc_pu_config.gamma.cur = dev->config.uvc_pu_config.gamma.def;

	dev->config.uvc_pu_config.white_balance_temperature.def.wWhiteBalanceTemperature = 6500;
	dev->config.uvc_pu_config.white_balance_temperature.min.wWhiteBalanceTemperature = 2500;
	dev->config.uvc_pu_config.white_balance_temperature.max.wWhiteBalanceTemperature = 8000;
	dev->config.uvc_pu_config.white_balance_temperature.res.wWhiteBalanceTemperature = 100;
	if(dev->config.processing_unit_bmControls.b.white_balance_temperature)
		uvc_api_get_attrs_pu_white_balance_temperature(dev, &dev->config.uvc_pu_config.white_balance_temperature);
	dev->config.uvc_pu_config.white_balance_temperature.cur = dev->config.uvc_pu_config.white_balance_temperature.def;

	dev->config.uvc_pu_config.white_balance_temperature_auto.def.bWhiteBalanceTemperatureAuto = 1;
	dev->config.uvc_pu_config.white_balance_temperature_auto.min.bWhiteBalanceTemperatureAuto = 0;
	dev->config.uvc_pu_config.white_balance_temperature_auto.max.bWhiteBalanceTemperatureAuto = 1;
	dev->config.uvc_pu_config.white_balance_temperature_auto.res.bWhiteBalanceTemperatureAuto = 1;
	if(dev->config.processing_unit_bmControls.b.white_balance_temperature_auto)
		uvc_api_get_attrs_pu_white_balance_temperature_auto(dev, &dev->config.uvc_pu_config.white_balance_temperature_auto);
	dev->config.uvc_pu_config.white_balance_temperature_auto.cur = dev->config.uvc_pu_config.white_balance_temperature_auto.def;

	return 0;
}

