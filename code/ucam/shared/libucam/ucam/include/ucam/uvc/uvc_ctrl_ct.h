/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_ctrl_ct.h
 * @Description : 
 */ 
#ifndef __UVC_CTRL_CT_H
#define __UVC_CTRL_CT_H

#include <ucam/uvc/uvc_ctrl.h>

struct uvc_ct_scanning_mode {
	uint8_t bScanningMode;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_scanning_mode);


struct uvc_ct_ae_mode {
	uint8_t bAutoExposureMode;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_ae_mode);

struct uvc_ct_ae_priority {
	uint8_t bAutoExposurePriority;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_ae_priority);


struct uvc_ct_exposure_time_absolute {
	uint32_t dwExposureTimeAbsolute;//100µs units
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_exposure_time_absolute);

struct uvc_ct_exposure_time_relative {
	int8_t bExposureTimeRelative;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_exposure_time_relative);

struct uvc_ct_focus_absolute {
	uint16_t wFocusAbsolute;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_focus_absolute);

struct uvc_ct_focus_relative {
	int8_t bFocusRelative;
	//uint8_t bSpeed; 
	int8_t bSpeed; 
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_focus_relative);

struct uvc_ct_focus_auto {
	uint8_t bFocusAuto;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_focus_auto);

struct uvc_ct_iris_absolute {
	uint16_t wIrisAbsolute;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_iris_absolute);


struct uvc_ct_iris_relative {
	uint8_t bIrisRelative;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_iris_relative);


struct uvc_ct_zoom_absolute {
	uint16_t wObjectiveFocalLength;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_zoom_absolute);

struct uvc_ct_zoom_relative {
	int8_t bZoom;
	uint8_t bDigitalZoom;
	//uint8_t bSpeed;
	int8_t bSpeed;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_zoom_relative);

struct uvc_ct_pantilt_absolute {
	int32_t dwPanAbsolute;
	int32_t dwTiltAbsolute;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_pantilt_absolute);

struct uvc_ct_pantilt_relative {
	int8_t bPanRelative;
	//uint8_t bPanSpeed;
	int8_t bPanSpeed;
	int8_t bTiltRelative;
	//uint8_t bTiltSpeed;
	int8_t bTiltSpeed;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_pantilt_relative);


struct uvc_ct_roll_absolute {
	int16_t wRollAbsolute;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_roll_absolute);

struct uvc_ct_roll_relative {
	int8_t bRollRelative;
	uint8_t bSpeed;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_roll_relative);

struct uvc_ct_privacy {
	uint8_t bPrivacy;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_ct_privacy);


struct uvc_ct_config_t
{
	struct UVC_CRTL_S(uvc_ct_scanning_mode) scanning_mode;
	struct UVC_CRTL_S(uvc_ct_ae_mode) ae_mode;
	struct UVC_CRTL_S(uvc_ct_ae_priority) ae_priority;
	struct UVC_CRTL_S(uvc_ct_exposure_time_absolute) exposure_time_absolute;
	struct UVC_CRTL_S(uvc_ct_exposure_time_relative) exposure_time_relative;
	struct UVC_CRTL_S(uvc_ct_focus_absolute) focus_absolute;
	struct UVC_CRTL_S(uvc_ct_focus_relative) focus_relative;
	struct UVC_CRTL_S(uvc_ct_focus_auto) focus_auto;
	struct UVC_CRTL_S(uvc_ct_iris_absolute) iris_absolute;
	struct UVC_CRTL_S(uvc_ct_iris_relative) iris_relative;
	struct UVC_CRTL_S(uvc_ct_zoom_absolute) zoom_absolute;
	struct UVC_CRTL_S(uvc_ct_zoom_relative) zoom_relative;
	struct UVC_CRTL_S(uvc_ct_pantilt_absolute) pantilt_absolute;
	struct UVC_CRTL_S(uvc_ct_pantilt_relative) pantilt_relative;
	struct UVC_CRTL_S(uvc_ct_roll_absolute) roll_absolute;
	struct UVC_CRTL_S(uvc_ct_roll_relative) roll_relative;
	struct UVC_CRTL_S(uvc_ct_privacy) privacy;

	uint32_t exposure_time_absolute_info;
};





#endif /* __UVC_CTRL_CT_H */