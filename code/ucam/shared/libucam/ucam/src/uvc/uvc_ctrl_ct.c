/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl_ct.c
 * @Description : 
 */ 
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/uvc.h>
#include <ucam/uvc/uvc_api_ct.h>



static UvcRequestErrorCode_t uvc_requests_ctrl_ct_scanning_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.1.1  Scanning Mode Control", page 81 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest;
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_scanning_mode scanning_mode;


	ucam_strace("enter");

	if(dev->config.camera_terminal_bmControls.b.scanning_mode == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_scanning_mode))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				scanning_mode = *((struct uvc_ct_scanning_mode*)data->data);

				if(scanning_mode.bScanningMode > dev->config.uvc_ct_config.scanning_mode.max.bScanningMode || 
					scanning_mode.bScanningMode < dev->config.uvc_ct_config.scanning_mode.min.bScanningMode)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.scanning_mode.cur = scanning_mode;
				if(uvc_api_set_ct_scanning_mode (dev, dev->config.uvc_ct_config.scanning_mode.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_scanning_mode fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}		
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_scanning_mode (dev, &dev->config.uvc_ct_config.scanning_mode.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_scanning_mode fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_scanning_mode*)data->data) = dev->config.uvc_ct_config.scanning_mode.cur;
			break;
		
		case UVC_GET_DEF:
			*((struct uvc_ct_scanning_mode*)data->data) = dev->config.uvc_ct_config.scanning_mode.def;
			break;
		
		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;

		
		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_scanning_mode*)data->data) = dev->config.uvc_ct_config.scanning_mode.min;
			break;

		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_scanning_mode*)data->data) = dev->config.uvc_ct_config.scanning_mode.max;
			break;

		case UVC_GET_RES:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_scanning_mode*)data->data) = dev->config.uvc_ct_config.scanning_mode.res;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_scanning_mode);
			break;
		default:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}

	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_ae_mode( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* "4.2.2.1.2  Auto-Exposure Mode Control ", page 81 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_ae_mode ae_mode;

	uint8_t info_change = 0;

	ucam_strace("enter");


	if(dev->config.camera_terminal_bmControls.b.ae_mode == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_ae_mode))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				ae_mode = *((struct uvc_ct_ae_mode*)data->data);

				if(ae_mode.bAutoExposureMode > dev->config.uvc_ct_config.ae_mode.max.bAutoExposureMode || 
					ae_mode.bAutoExposureMode < dev->config.uvc_ct_config.ae_mode.min.bAutoExposureMode)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				if(dev->config.uvc_ct_config.ae_mode.cur.bAutoExposureMode != ae_mode.bAutoExposureMode)
				{    
					info_change = 1; 
				}
				dev->config.uvc_ct_config.ae_mode.cur = ae_mode;
				if(uvc_api_set_ct_ae_mode (dev, dev->config.uvc_ct_config.ae_mode.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_ae_mode fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}

				if(info_change)
                {

					if(dev->config.uvc_ct_config.ae_mode.cur.bAutoExposureMode == 0x08)
					{
						dev->config.uvc_ct_config.exposure_time_absolute_info = 0x0F;

					}
					else if(dev->config.uvc_ct_config.ae_mode.cur.bAutoExposureMode == 0x01)
					{
						dev->config.uvc_ct_config.exposure_time_absolute_info = 0x0B;
					}
	
                }
			}

			break;

		
		case UVC_GET_CUR:
			if(uvc_api_get_ct_ae_mode (dev, &dev->config.uvc_ct_config.ae_mode.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_ae_mode fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_ae_mode*)data->data) = dev->config.uvc_ct_config.ae_mode.cur;
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_ae_mode*)data->data) = dev->config.uvc_ct_config.ae_mode.def;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_ae_mode*)data->data) = dev->config.uvc_ct_config.ae_mode.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
			
		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_ae_mode*)data->data) = dev->config.uvc_ct_config.ae_mode.min;
			break;
		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_ae_mode *)data->data) = dev->config.uvc_ct_config.ae_mode.max;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_ae_mode);
			break;
		default:

			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_ae_priority( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1> "4.2.2.1.3  Auto-Exposure Priority Control", page 82 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_ae_priority ae_priority;

	ucam_strace("enter");

	if(dev->config.camera_terminal_bmControls.b.ae_priority == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
	
	switch (req) {
		case UVC_SET_CUR:

			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_ae_priority))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				ae_priority = *((struct uvc_ct_ae_priority*)data->data);

				if(ae_priority.bAutoExposurePriority > dev->config.uvc_ct_config.ae_priority.max.bAutoExposurePriority || 
					ae_priority.bAutoExposurePriority < dev->config.uvc_ct_config.ae_priority.min.bAutoExposurePriority)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.ae_priority.cur = ae_priority;
				if(uvc_api_set_ct_ae_priority (dev, dev->config.uvc_ct_config.ae_priority.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_ae_priority fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_ae_priority (dev, &dev->config.uvc_ct_config.ae_priority.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_ae_priority fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_ae_priority*)data->data) = dev->config.uvc_ct_config.ae_priority.cur;
			break;

		case UVC_GET_DEF:

			*((struct uvc_ct_ae_priority*)data->data) = dev->config.uvc_ct_config.ae_priority.def;
			break;	

		case UVC_GET_INFO:

			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_ae_priority*)data->data) = dev->config.uvc_ct_config.ae_priority.min;
			break;

		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_ae_priority*)data->data) = dev->config.uvc_ct_config.ae_priority.max;
			break;
			
		case UVC_GET_RES:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_ae_priority*)data->data) = dev->config.uvc_ct_config.ae_priority.res;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_ae_priority);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_exposure_time_absolute( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1> "4.2.2.1.4  Exposure Time (Absolute) Control", page 82 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_exposure_time_absolute exposure_time_absolute;

	ucam_strace("enter");

	if(dev->config.camera_terminal_bmControls.b.exposure_time_absolute == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:

			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_exposure_time_absolute))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				exposure_time_absolute = *((struct uvc_ct_exposure_time_absolute*)data->data);

				if(exposure_time_absolute.dwExposureTimeAbsolute > dev->config.uvc_ct_config.exposure_time_absolute.max.dwExposureTimeAbsolute || 
					exposure_time_absolute.dwExposureTimeAbsolute < dev->config.uvc_ct_config.exposure_time_absolute.min.dwExposureTimeAbsolute)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.exposure_time_absolute.cur = exposure_time_absolute;
				if(uvc_api_set_ct_exposure_time_absolute (dev, dev->config.uvc_ct_config.exposure_time_absolute.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_exposure_time_absolute fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_exposure_time_absolute (dev, &dev->config.uvc_ct_config.exposure_time_absolute.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_exposure_time_absolute fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_exposure_time_absolute*)data->data) = dev->config.uvc_ct_config.exposure_time_absolute.cur;
			break;

		case UVC_GET_DEF:

			*((struct uvc_ct_exposure_time_absolute*)data->data) = dev->config.uvc_ct_config.exposure_time_absolute.def;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_exposure_time_absolute*)data->data) = dev->config.uvc_ct_config.exposure_time_absolute.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_exposure_time_absolute*)data->data) = dev->config.uvc_ct_config.exposure_time_absolute.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_exposure_time_absolute*)data->data) = dev->config.uvc_ct_config.exposure_time_absolute.res;
			break;

		case UVC_GET_INFO:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				*((uint32_t*)data->data) = dev->config.uvc_ct_config.exposure_time_absolute_info;
			}
			else
				*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_exposure_time_absolute);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_exposure_time_relative( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1> "4.2.2.1.5  Exposure Time (Relative) Control", page 83 */
	/* notes: If both Relative and Absolute Controls are supported, 
	   a SET_CUR to the Relative Control with a value other than 0x00 
	   shall result in a Control Change interrupt for the Absolute 
	   Control (section 2.4.2.2, “Status Interrupt Endpoint”). */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_exposure_time_relative exposure_time_relative;

	ucam_strace("enter");

	
	if(dev->config.camera_terminal_bmControls.b.exposure_time_relative == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;	

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_exposure_time_relative))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				exposure_time_relative = *((struct uvc_ct_exposure_time_relative*)data->data);

				if(exposure_time_relative.bExposureTimeRelative > dev->config.uvc_ct_config.exposure_time_relative.max.bExposureTimeRelative || 
					exposure_time_relative.bExposureTimeRelative < dev->config.uvc_ct_config.exposure_time_relative.min.bExposureTimeRelative)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.exposure_time_relative.cur = exposure_time_relative;
				if(uvc_api_set_ct_exposure_time_relative (dev, dev->config.uvc_ct_config.exposure_time_relative.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_exposure_time_relative fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_exposure_time_relative (dev, &dev->config.uvc_ct_config.exposure_time_relative.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_exposure_time_relative fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_exposure_time_relative*)data->data) = dev->config.uvc_ct_config.exposure_time_relative.cur;
			break;
		case UVC_GET_DEF:
			*((struct uvc_ct_exposure_time_relative*)data->data) = dev->config.uvc_ct_config.exposure_time_relative.def;
			break;
		case UVC_GET_INFO:
			((uint32_t*)data->data)[0] = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_exposure_time_relative*)data->data) = dev->config.uvc_ct_config.exposure_time_relative.min;
			break;

		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_exposure_time_relative*)data->data) = dev->config.uvc_ct_config.exposure_time_relative.max;
			break;
			
		case UVC_GET_RES:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_exposure_time_relative*)data->data) = dev->config.uvc_ct_config.exposure_time_relative.res;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_exposure_time_relative);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_focus_absolute( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1> "4.2.2.1.6  Focus (Absolute) Control", page 84 */
	/* notes: This value is expressed in millimeters.*/
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_focus_absolute focus_absolute;

	ucam_strace("enter");
	

	if(dev->config.camera_terminal_bmControls.b.focus_absolute == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_focus_absolute))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				focus_absolute = *((struct uvc_ct_focus_absolute*)data->data);

				if(focus_absolute.wFocusAbsolute > dev->config.uvc_ct_config.focus_absolute.max.wFocusAbsolute || 
					focus_absolute.wFocusAbsolute < dev->config.uvc_ct_config.focus_absolute.min.wFocusAbsolute)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.focus_absolute.cur = focus_absolute;
				if(uvc_api_set_ct_focus_absolute (dev, dev->config.uvc_ct_config.focus_absolute.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_focus_absolute fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_focus_absolute*)data->data) = dev->config.uvc_ct_config.focus_absolute.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_focus_absolute (dev, &dev->config.uvc_ct_config.focus_absolute.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_focus_absolute fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_focus_absolute*)data->data) = dev->config.uvc_ct_config.focus_absolute.cur;
			break;

		case UVC_GET_MIN:

			*((struct uvc_ct_focus_absolute*)data->data) = dev->config.uvc_ct_config.focus_absolute.min;
			break;

		case UVC_GET_MAX:

			*((struct uvc_ct_focus_absolute*)data->data) = dev->config.uvc_ct_config.focus_absolute.max;
			break;

		case UVC_GET_RES:

			*((struct uvc_ct_focus_absolute*)data->data) = dev->config.uvc_ct_config.focus_absolute.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_focus_absolute);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_focus_relative( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1> "4.2.2.1.7  Focus (Relative) Control", page 84 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_focus_relative focus_relative;

	ucam_strace("enter");

	if(dev->config.camera_terminal_bmControls.b.focus_relative == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_focus_relative))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				focus_relative = *((struct uvc_ct_focus_relative*)data->data);

				if(focus_relative.bSpeed > dev->config.uvc_ct_config.focus_relative.max.bSpeed || 
					focus_relative.bSpeed < dev->config.uvc_ct_config.focus_relative.min.bSpeed)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.focus_relative.cur = focus_relative;
				if(uvc_api_set_ct_focus_relative (dev, dev->config.uvc_ct_config.focus_relative.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_focus_relative fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_focus_relative*)data->data) = dev->config.uvc_ct_config.focus_relative.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_focus_relative (dev, &dev->config.uvc_ct_config.focus_relative.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_focus_relative fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}			
			*((struct uvc_ct_focus_relative*)data->data) = dev->config.uvc_ct_config.focus_relative.cur;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_focus_relative*)data->data) = dev->config.uvc_ct_config.focus_relative.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_focus_relative*)data->data) = dev->config.uvc_ct_config.focus_relative.max;
			break;

		case UVC_GET_RES:			
			*((struct uvc_ct_focus_relative*)data->data) = dev->config.uvc_ct_config.focus_relative.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_focus_relative);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_focus_auto( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.8  Focus, Auto Control" */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_focus_auto focus_auto;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.focus_auto == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_focus_auto))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				focus_auto = *((struct uvc_ct_focus_auto*)data->data);

				if(focus_auto.bFocusAuto > dev->config.uvc_ct_config.focus_auto.max.bFocusAuto || 
					focus_auto.bFocusAuto < dev->config.uvc_ct_config.focus_auto.min.bFocusAuto)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.focus_auto.cur = focus_auto;
				if(uvc_api_set_ct_focus_auto (dev, dev->config.uvc_ct_config.focus_auto.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_focus_auto fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_focus_auto (dev, &dev->config.uvc_ct_config.focus_auto.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_focus_auto fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_focus_auto*)data->data) = dev->config.uvc_ct_config.focus_auto.cur;
			break;
		
		case UVC_GET_DEF:
			*((struct uvc_ct_focus_auto*)data->data) = dev->config.uvc_ct_config.focus_auto.def;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		

		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_focus_auto*)data->data) = dev->config.uvc_ct_config.focus_auto.min;
			break;

		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_focus_auto*)data->data) = dev->config.uvc_ct_config.focus_auto.max;
			break;
			
		case UVC_GET_RES:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_focus_auto*)data->data) = dev->config.uvc_ct_config.focus_auto.res;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_focus_auto);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_iris_absolute( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.9  Iris (Absolute) Control ", page 85 */
	/* This value is expressed in units of fstop * 100. ??? */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_iris_absolute iris_absolute;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.iris_absolute == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_iris_absolute))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				iris_absolute = *((struct uvc_ct_iris_absolute*)data->data);

				if(iris_absolute.wIrisAbsolute > dev->config.uvc_ct_config.iris_absolute.max.wIrisAbsolute || 
					iris_absolute.wIrisAbsolute < dev->config.uvc_ct_config.iris_absolute.min.wIrisAbsolute)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.iris_absolute.cur = iris_absolute;
				if(uvc_api_set_ct_iris_absolute (dev, dev->config.uvc_ct_config.iris_absolute.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_iris_absolute fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_iris_absolute*)data->data) = dev->config.uvc_ct_config.iris_absolute.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_iris_absolute (dev, &dev->config.uvc_ct_config.iris_absolute.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_iris_absolute fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_iris_absolute*)data->data) = dev->config.uvc_ct_config.iris_absolute.cur;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_iris_absolute*)data->data) = dev->config.uvc_ct_config.iris_absolute.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_iris_absolute*)data->data) = dev->config.uvc_ct_config.iris_absolute.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_iris_absolute*)data->data) = dev->config.uvc_ct_config.iris_absolute.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_iris_absolute);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_iris_relative( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.10  Iris (Relative) Control ", page 86 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_iris_relative iris_relative;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.iris_relative == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:	
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_iris_relative))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				iris_relative = *((struct uvc_ct_iris_relative*)data->data);

				if(iris_relative.bIrisRelative > dev->config.uvc_ct_config.iris_relative.max.bIrisRelative || 
					iris_relative.bIrisRelative < dev->config.uvc_ct_config.iris_relative.min.bIrisRelative)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.iris_relative.cur = iris_relative;
				if(uvc_api_set_ct_iris_relative (dev, dev->config.uvc_ct_config.iris_relative.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_iris_relative fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}		
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_iris_relative (dev, &dev->config.uvc_ct_config.iris_relative.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_iris_relative fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_iris_relative*)data->data) = dev->config.uvc_ct_config.iris_relative.cur;
			break;
		case UVC_GET_DEF:
			*((struct uvc_ct_iris_relative*)data->data) = dev->config.uvc_ct_config.iris_relative.def;
			break;
		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_iris_relative*)data->data) = dev->config.uvc_ct_config.iris_relative.min;
			break;

		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_iris_relative*)data->data) = dev->config.uvc_ct_config.iris_relative.max;
			break;
			
		case UVC_GET_RES:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_iris_relative*)data->data) = dev->config.uvc_ct_config.iris_relative.res;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_iris_relative);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_zoom_absolute( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.11 Zoom (Absolute) Control ", page 86 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_zoom_absolute zoom_absolute;

	ucam_strace("enter");
	
	
	if(dev->config.camera_terminal_bmControls.b.zoom_absolute == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_zoom_absolute))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				zoom_absolute = *((struct uvc_ct_zoom_absolute*)data->data);

				if(zoom_absolute.wObjectiveFocalLength > dev->config.uvc_ct_config.zoom_absolute.max.wObjectiveFocalLength || 
					zoom_absolute.wObjectiveFocalLength < dev->config.uvc_ct_config.zoom_absolute.min.wObjectiveFocalLength)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.zoom_absolute.cur = zoom_absolute;
				if(uvc_api_set_ct_zoom_absolute (dev, dev->config.uvc_ct_config.zoom_absolute.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_zoom_absolute fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_zoom_absolute*)data->data) = dev->config.uvc_ct_config.zoom_absolute.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_zoom_absolute (dev, &dev->config.uvc_ct_config.zoom_absolute.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_zoom_absolute fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_zoom_absolute*)data->data) = dev->config.uvc_ct_config.zoom_absolute.cur;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_zoom_absolute*)data->data) = dev->config.uvc_ct_config.zoom_absolute.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_zoom_absolute*)data->data) = dev->config.uvc_ct_config.zoom_absolute.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_zoom_absolute*)data->data) = dev->config.uvc_ct_config.zoom_absolute.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_zoom_absolute);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_zoom_relative( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.12 Zoom (Relative) Control ", page 87 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_zoom_relative zoom_relative;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.zoom_relative == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_zoom_relative))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				zoom_relative = *((struct uvc_ct_zoom_relative*)data->data);

				//0: Stop 
				//1:moving to telephoto direction 
				//0xFF:moving to wide-angle direction
				if(!(zoom_relative.bZoom == 0 || zoom_relative.bZoom == 1 || (uint8_t)zoom_relative.bZoom == 0xFF))
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}
				
				//0: Digital Zoom OFF 
				//1: Digital Zoom On 
				if(!(zoom_relative.bDigitalZoom == 0 || zoom_relative.bDigitalZoom == 1))
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				if(zoom_relative.bSpeed > dev->config.uvc_ct_config.zoom_relative.max.bSpeed ||
					zoom_relative.bSpeed < dev->config.uvc_ct_config.zoom_relative.min.bSpeed)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.zoom_relative.cur = zoom_relative;
				if(uvc_api_set_ct_zoom_relative (dev, dev->config.uvc_ct_config.zoom_relative.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_zoom_relative fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_zoom_relative *)data->data) = dev->config.uvc_ct_config.zoom_relative.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_zoom_relative (dev, &dev->config.uvc_ct_config.zoom_relative.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_zoom_relative fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_zoom_relative *)data->data) = dev->config.uvc_ct_config.zoom_relative.cur;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_zoom_relative *)data->data) = dev->config.uvc_ct_config.zoom_relative.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_zoom_relative *)data->data) = dev->config.uvc_ct_config.zoom_relative.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_zoom_relative *)data->data) = dev->config.uvc_ct_config.zoom_relative.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_zoom_relative);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_pantilt_absolute( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.13 PanTilt (Absolute) Control ", page 88 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_pantilt_absolute pantilt_absolute;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.pantilt_absolute == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_pantilt_absolute))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				pantilt_absolute = *((struct uvc_ct_pantilt_absolute*)data->data);

				if(pantilt_absolute.dwPanAbsolute > dev->config.uvc_ct_config.pantilt_absolute.max.dwPanAbsolute || 
					pantilt_absolute.dwPanAbsolute < dev->config.uvc_ct_config.pantilt_absolute.min.dwPanAbsolute)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				if(pantilt_absolute.dwTiltAbsolute > dev->config.uvc_ct_config.pantilt_absolute.max.dwTiltAbsolute || 
					pantilt_absolute.dwTiltAbsolute < dev->config.uvc_ct_config.pantilt_absolute.min.dwTiltAbsolute)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_ct_config.pantilt_absolute.cur = pantilt_absolute;
				if(uvc_api_set_ct_pantilt_absolute (dev, dev->config.uvc_ct_config.pantilt_absolute.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_pantilt_absolute fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_pantilt_absolute*)data->data) = dev->config.uvc_ct_config.pantilt_absolute.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_pantilt_absolute (dev, &dev->config.uvc_ct_config.pantilt_absolute.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_pantilt_absolute fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
            *((struct uvc_ct_pantilt_absolute*)data->data) = dev->config.uvc_ct_config.pantilt_absolute.cur;

			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_pantilt_absolute*)data->data) = dev->config.uvc_ct_config.pantilt_absolute.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_pantilt_absolute*)data->data) = dev->config.uvc_ct_config.pantilt_absolute.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_pantilt_absolute*)data->data) = dev->config.uvc_ct_config.pantilt_absolute.res;

			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_pantilt_absolute);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_pantilt_relative( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.14 PanTilt (Relative) Control ",page 89 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_pantilt_relative pantilt_relative;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.pantilt_relative == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_pantilt_relative))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				pantilt_relative = *((struct uvc_ct_pantilt_relative*)data->data);

				if(pantilt_relative.bPanSpeed > dev->config.uvc_ct_config.pantilt_relative.max.bPanSpeed || 
					pantilt_relative.bPanSpeed < dev->config.uvc_ct_config.pantilt_relative.min.bPanSpeed)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				if(pantilt_relative.bTiltSpeed > dev->config.uvc_ct_config.pantilt_relative.max.bTiltSpeed || 
					pantilt_relative.bTiltSpeed < dev->config.uvc_ct_config.pantilt_relative.min.bTiltSpeed)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}

				dev->config.uvc_ct_config.pantilt_relative.cur = pantilt_relative;
				if(uvc_api_set_ct_pantilt_relative (dev, dev->config.uvc_ct_config.pantilt_relative.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_pantilt_relative fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_pantilt_relative *)data->data) = dev->config.uvc_ct_config.pantilt_relative.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_pantilt_relative (dev, &dev->config.uvc_ct_config.pantilt_relative.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_pantilt_relative fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_pantilt_relative *)data->data) = dev->config.uvc_ct_config.pantilt_relative.cur;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_pantilt_relative *)data->data) = dev->config.uvc_ct_config.pantilt_relative.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_pantilt_relative *)data->data) = dev->config.uvc_ct_config.pantilt_relative.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_pantilt_relative *)data->data) = dev->config.uvc_ct_config.pantilt_relative.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_pantilt_relative);
			break;	

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_roll_absolute( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.1.11 Zoom (Absolute) Control ", page 86 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	//uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_roll_absolute roll_absolute;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.roll_absolute == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_roll_absolute))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				roll_absolute = *((struct uvc_ct_roll_absolute*)data->data);

				if(roll_absolute.wRollAbsolute > dev->config.uvc_ct_config.roll_absolute.max.wRollAbsolute || 
					roll_absolute.wRollAbsolute < dev->config.uvc_ct_config.roll_absolute.min.wRollAbsolute)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.roll_absolute.cur = roll_absolute;
				if(uvc_api_set_ct_roll_absolute (dev, dev->config.uvc_ct_config.roll_absolute.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_roll_absolute fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_roll_absolute*)data->data) = dev->config.uvc_ct_config.roll_absolute.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_roll_absolute (dev, &dev->config.uvc_ct_config.roll_absolute.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_roll_absolute fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_roll_absolute*)data->data) = dev->config.uvc_ct_config.roll_absolute.cur;
			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_roll_absolute*)data->data) = dev->config.uvc_ct_config.roll_absolute.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_roll_absolute*)data->data) = dev->config.uvc_ct_config.roll_absolute.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_roll_absolute*)data->data) = dev->config.uvc_ct_config.roll_absolute.res;
			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		
		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_roll_absolute);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_roll_relative( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	/* <1>"4.2.2.1.16 Roll (Relative) Control ",page 90 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	struct uvc_ct_roll_relative roll_relative;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.roll_relative == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_roll_relative))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				roll_relative = *((struct uvc_ct_roll_relative*)data->data);

				if(roll_relative.bSpeed > dev->config.uvc_ct_config.roll_relative.max.bSpeed || 
					roll_relative.bSpeed < dev->config.uvc_ct_config.roll_relative.min.bSpeed)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.roll_relative.cur = roll_relative;
				if(uvc_api_set_ct_roll_relative (dev, dev->config.uvc_ct_config.roll_relative.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_roll_relative fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_DEF:
			*((struct uvc_ct_roll_relative*)data->data) = dev->config.uvc_ct_config.roll_relative.def;
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_roll_relative (dev, &dev->config.uvc_ct_config.roll_relative.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_roll_relative fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_roll_relative*)data->data) = dev->config.uvc_ct_config.roll_relative.cur;

			break;

		case UVC_GET_MIN:
			*((struct uvc_ct_roll_relative*)data->data) = dev->config.uvc_ct_config.roll_relative.min;
			break;

		case UVC_GET_MAX:
			*((struct uvc_ct_roll_relative*)data->data) = dev->config.uvc_ct_config.roll_relative.max;
			break;

		case UVC_GET_RES:
			*((struct uvc_ct_roll_relative*)data->data) = dev->config.uvc_ct_config.roll_relative.res;

			break;

		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;

		case UVC_GET_LEN:
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_roll_relative);
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static UvcRequestErrorCode_t uvc_requests_ctrl_ct_privacy( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.2.2.1.17 Privacy Control ",page 91*/
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 
	uint8_t		id = ctrl->wIndex >> 8;
	struct uvc_ct_privacy privacy;

	ucam_strace("enter");
	
	if(dev->config.camera_terminal_bmControls.b.privacy == 0)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				if(ctrl->wLength > sizeof(struct uvc_ct_privacy))
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;

				privacy = *((struct uvc_ct_privacy*)data->data);

				if(privacy.bPrivacy > dev->config.uvc_ct_config.privacy.max.bPrivacy || 
					privacy.bPrivacy < dev->config.uvc_ct_config.privacy.min.bPrivacy)
				{
					return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;	
				}	

				dev->config.uvc_ct_config.privacy.cur = privacy;
				if(uvc_api_set_ct_privacy (dev, dev->config.uvc_ct_config.privacy.cur) != 0)
				{
					ucam_error("uvc_api_set_ct_privacy fail");
					return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
				}
			}
			break;

		case UVC_GET_CUR:
			if(uvc_api_get_ct_privacy (dev, &dev->config.uvc_ct_config.privacy.cur) != 0)
			{
				ucam_error("uvc_api_get_ct_privacy fail");
				return UVC_REQUEST_ERROR_CODE_WRONG_STATE;
			}
			*((struct uvc_ct_privacy*)data->data) = dev->config.uvc_ct_config.privacy.cur;
			break;
		case UVC_GET_DEF:
			*((struct uvc_ct_privacy*)data->data) = dev->config.uvc_ct_config.privacy.def;
			break;
		case UVC_GET_INFO:
			*((uint32_t*)data->data) = UVC_CONTROL_CAP_GET_SET;
			break;
		case UVC_GET_MIN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_privacy*)data->data) = dev->config.uvc_ct_config.privacy.min;
			break;

		case UVC_GET_MAX:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_privacy*)data->data) = dev->config.uvc_ct_config.privacy.max;
			break;
			
		case UVC_GET_RES:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((struct uvc_ct_privacy*)data->data) = dev->config.uvc_ct_config.privacy.res;
			break;
		case UVC_GET_LEN:
			if(id == UVC_ENTITY_ID_INPUT_TERMINAL)
			{
				return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			}
			*((uint32_t*)data->data) = sizeof(struct uvc_ct_privacy);
			break;

		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	return ret;
}

static int init_ctrls_ct(struct uvc_dev *dev)
{
	ucam_strace("enter");

	dev->config.uvc_ct_config.ae_mode.def.bAutoExposureMode = AE_MODE_AUTO;
	dev->config.uvc_ct_config.ae_mode.min.bAutoExposureMode = AE_MODE_MANUAL;
	dev->config.uvc_ct_config.ae_mode.max.bAutoExposureMode = AE_MODE_ALL;
	dev->config.uvc_ct_config.ae_mode.res.bAutoExposureMode = AE_MODE_ALL;
	if(dev->config.camera_terminal_bmControls.b.ae_mode)
	{
		uvc_api_get_attrs_ct_ae_mode (dev, &dev->config.uvc_ct_config.ae_mode);
		if(dev->config.uvc_ct_config.ae_mode.res.bAutoExposureMode == 1 &&
			dev->config.uvc_ct_config.ae_mode.max.bAutoExposureMode != 1)
		{
			ucam_error("ae_mode res error");
			dev->config.uvc_ct_config.ae_mode.res.bAutoExposureMode = AE_MODE_ALL;
		}
	}
		
	dev->config.uvc_ct_config.ae_mode.cur = dev->config.uvc_ct_config.ae_mode.def; 

	dev->config.uvc_ct_config.exposure_time_absolute_info = 0x0F;
	dev->config.uvc_ct_config.exposure_time_absolute.def.dwExposureTimeAbsolute = 80;
	dev->config.uvc_ct_config.exposure_time_absolute.min.dwExposureTimeAbsolute = 1;
	dev->config.uvc_ct_config.exposure_time_absolute.max.dwExposureTimeAbsolute = 333;
	dev->config.uvc_ct_config.exposure_time_absolute.res.dwExposureTimeAbsolute = 1;
	if(dev->config.camera_terminal_bmControls.b.exposure_time_absolute)
		uvc_api_get_attrs_ct_exposure_time_absolute (dev, &dev->config.uvc_ct_config.exposure_time_absolute);
	dev->config.uvc_ct_config.exposure_time_absolute.cur = dev->config.uvc_ct_config.exposure_time_absolute.def;

	dev->config.uvc_ct_config.focus_absolute.def.wFocusAbsolute = 50;
	dev->config.uvc_ct_config.focus_absolute.min.wFocusAbsolute = 0;
	dev->config.uvc_ct_config.focus_absolute.max.wFocusAbsolute = 101;
	dev->config.uvc_ct_config.focus_absolute.res.wFocusAbsolute = 1;
	if(dev->config.camera_terminal_bmControls.b.focus_absolute)
		uvc_api_get_attrs_ct_focus_absolute (dev, &dev->config.uvc_ct_config.focus_absolute);
	dev->config.uvc_ct_config.focus_absolute.cur = dev->config.uvc_ct_config.focus_absolute.def;

	dev->config.uvc_ct_config.focus_relative.def.bFocusRelative = 0;
	dev->config.uvc_ct_config.focus_relative.def.bSpeed = 0x01;
	dev->config.uvc_ct_config.focus_relative.min.bFocusRelative = 0;
	dev->config.uvc_ct_config.focus_relative.min.bSpeed = 0;
	dev->config.uvc_ct_config.focus_relative.max.bFocusRelative = 0;
	dev->config.uvc_ct_config.focus_relative.max.bSpeed = 0x07;
	dev->config.uvc_ct_config.focus_relative.res.bFocusRelative = 0;
	dev->config.uvc_ct_config.focus_relative.res.bSpeed = 0x01;
	if(dev->config.camera_terminal_bmControls.b.focus_relative)
		uvc_api_get_attrs_ct_focus_relative (dev, &dev->config.uvc_ct_config.focus_relative);
	dev->config.uvc_ct_config.focus_relative.cur = dev->config.uvc_ct_config.focus_relative.def;

	dev->config.uvc_ct_config.focus_auto.def.bFocusAuto = 1;
	dev->config.uvc_ct_config.focus_auto.min.bFocusAuto = 0;
	dev->config.uvc_ct_config.focus_auto.max.bFocusAuto = 1;
	dev->config.uvc_ct_config.focus_auto.res.bFocusAuto = 1;
	if(dev->config.camera_terminal_bmControls.b.focus_auto)
		uvc_api_get_attrs_ct_focus_auto (dev, &dev->config.uvc_ct_config.focus_auto);
	dev->config.uvc_ct_config.focus_auto.cur = dev->config.uvc_ct_config.focus_auto.def;

	dev->config.uvc_ct_config.iris_absolute.def.wIrisAbsolute = 50;
	dev->config.uvc_ct_config.iris_absolute.min.wIrisAbsolute = 10;
	dev->config.uvc_ct_config.iris_absolute.max.wIrisAbsolute = 100;
	dev->config.uvc_ct_config.iris_absolute.res.wIrisAbsolute = 1;
	if(dev->config.camera_terminal_bmControls.b.iris_absolute)
		uvc_api_get_attrs_ct_iris_absolute (dev, &dev->config.uvc_ct_config.iris_absolute);
	dev->config.uvc_ct_config.iris_absolute.cur = dev->config.uvc_ct_config.iris_absolute.def;

	dev->config.uvc_ct_config.zoom_absolute.def.wObjectiveFocalLength = 0;
	dev->config.uvc_ct_config.zoom_absolute.min.wObjectiveFocalLength = 0;
	dev->config.uvc_ct_config.zoom_absolute.max.wObjectiveFocalLength = 0x4000;
	dev->config.uvc_ct_config.zoom_absolute.res.wObjectiveFocalLength = 1;
	if(dev->config.camera_terminal_bmControls.b.zoom_absolute)
		uvc_api_get_attrs_ct_zoom_absolute (dev, &dev->config.uvc_ct_config.zoom_absolute);
	dev->config.uvc_ct_config.zoom_absolute.cur = dev->config.uvc_ct_config.zoom_absolute.def;

	dev->config.uvc_ct_config.zoom_relative.def.bSpeed = 6;
	dev->config.uvc_ct_config.zoom_relative.min.bSpeed = 0;
	dev->config.uvc_ct_config.zoom_relative.max.bSpeed = 7;
	dev->config.uvc_ct_config.zoom_relative.res.bSpeed = 1;
	if(dev->config.camera_terminal_bmControls.b.zoom_relative)
		uvc_api_get_attrs_ct_zoom_relative (dev, &dev->config.uvc_ct_config.zoom_relative);
	dev->config.uvc_ct_config.zoom_relative.cur = dev->config.uvc_ct_config.zoom_relative.def;

	dev->config.uvc_ct_config.pantilt_absolute.def.dwPanAbsolute = 0;
	dev->config.uvc_ct_config.pantilt_absolute.def.dwTiltAbsolute = 0;
	dev->config.uvc_ct_config.pantilt_absolute.min.dwPanAbsolute = -170 * 3600;
	dev->config.uvc_ct_config.pantilt_absolute.min.dwTiltAbsolute = -30 * 3600;
	dev->config.uvc_ct_config.pantilt_absolute.max.dwPanAbsolute = 170 * 3600;
	dev->config.uvc_ct_config.pantilt_absolute.max.dwTiltAbsolute = 90 * 3600;
	dev->config.uvc_ct_config.pantilt_absolute.res.dwPanAbsolute = 1;
	dev->config.uvc_ct_config.pantilt_absolute.res.dwTiltAbsolute = 1;
	if(dev->config.camera_terminal_bmControls.b.pantilt_absolute)
		uvc_api_get_attrs_ct_pantilt_absolute (dev, &dev->config.uvc_ct_config.pantilt_absolute);
	dev->config.uvc_ct_config.pantilt_absolute.cur = dev->config.uvc_ct_config.pantilt_absolute.def;

	dev->config.uvc_ct_config.pantilt_relative.def.bPanRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.def.bPanSpeed = 16;
	dev->config.uvc_ct_config.pantilt_relative.def.bTiltRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.def.bTiltSpeed = 16;

	dev->config.uvc_ct_config.pantilt_relative.min.bPanRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.min.bPanSpeed = -24;
	dev->config.uvc_ct_config.pantilt_relative.min.bTiltRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.min.bTiltSpeed = -20;

	dev->config.uvc_ct_config.pantilt_relative.max.bPanRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.max.bPanSpeed = 24;
	dev->config.uvc_ct_config.pantilt_relative.max.bTiltRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.max.bTiltSpeed = 20;

	dev->config.uvc_ct_config.pantilt_relative.res.bPanRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.res.bPanSpeed = 1;
	dev->config.uvc_ct_config.pantilt_relative.res.bTiltRelative = 0;
	dev->config.uvc_ct_config.pantilt_relative.res.bTiltSpeed = 1;

	if(dev->config.camera_terminal_bmControls.b.pantilt_relative)
		uvc_api_get_attrs_ct_pantilt_relative (dev, &dev->config.uvc_ct_config.pantilt_relative);
	dev->config.uvc_ct_config.pantilt_relative.cur = dev->config.uvc_ct_config.pantilt_relative.def;

	return 0;
}


static uvc_requests_ctrl_fn_t uvc_requests_ctrl_ct [] = {
	NULL,
	uvc_requests_ctrl_ct_scanning_mode,
	uvc_requests_ctrl_ct_ae_mode,
	uvc_requests_ctrl_ct_ae_priority,
	uvc_requests_ctrl_ct_exposure_time_absolute,
	uvc_requests_ctrl_ct_exposure_time_relative,
	uvc_requests_ctrl_ct_focus_absolute,
	uvc_requests_ctrl_ct_focus_relative,
	uvc_requests_ctrl_ct_focus_auto,
	uvc_requests_ctrl_ct_iris_absolute,
	uvc_requests_ctrl_ct_iris_relative,
	uvc_requests_ctrl_ct_zoom_absolute,
	uvc_requests_ctrl_ct_zoom_relative,
	uvc_requests_ctrl_ct_pantilt_absolute,
	uvc_requests_ctrl_ct_pantilt_relative,
	uvc_requests_ctrl_ct_roll_absolute,
	uvc_requests_ctrl_ct_roll_relative,
	uvc_requests_ctrl_ct_privacy,
};

struct uvc_ctrl_attr_t uvc_ctrl_ct = {
	.id = UVC_ENTITY_ID_INPUT_TERMINAL,
	.selectors_max_size = sizeof(uvc_requests_ctrl_ct)/sizeof(uvc_requests_ctrl_fn_t),
	.init = init_ctrls_ct,
	.requests_ctrl_fn = uvc_requests_ctrl_ct,
};


struct uvc_ctrl_attr_t uvc_ctrl_xu_ct = {
	.id = UVC_ENTITY_ID_XU_PTZ,
	.selectors_max_size = sizeof(uvc_requests_ctrl_ct)/sizeof(uvc_requests_ctrl_fn_t),
	.init = NULL,
	.requests_ctrl_fn = uvc_requests_ctrl_ct,
};
