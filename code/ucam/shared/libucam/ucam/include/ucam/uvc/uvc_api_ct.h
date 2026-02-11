/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api_ct.h
 * @Description : 
 */ 
#ifndef UVC_API_CT_H 
#define UVC_API_CT_H 

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************************************/
int uvc_api_set_ct_scanning_mode (struct uvc_dev *dev, struct uvc_ct_scanning_mode scanning_mode);
int uvc_api_get_ct_scanning_mode (struct uvc_dev *dev, struct uvc_ct_scanning_mode *scanning_mode);
int uvc_api_get_attrs_ct_scanning_mode (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_scanning_mode) *scanning_mode);

/********************************************************************************************/
int uvc_api_set_ct_ae_mode (struct uvc_dev *dev, struct uvc_ct_ae_mode ae_mode);
int uvc_api_get_ct_ae_mode (struct uvc_dev *dev, struct uvc_ct_ae_mode *ae_mode);
int uvc_api_get_attrs_ct_ae_mode (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_ae_mode) *ae_mode);

/********************************************************************************************/
int uvc_api_set_ct_ae_priority (struct uvc_dev *dev, struct uvc_ct_ae_priority ae_priority);
int uvc_api_get_ct_ae_priority (struct uvc_dev *dev, struct uvc_ct_ae_priority *ae_priority);
int uvc_api_get_attrs_ct_ae_priority (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_ae_priority) *ae_priority);

/********************************************************************************************/
int uvc_api_set_ct_exposure_time_absolute (struct uvc_dev *dev, struct uvc_ct_exposure_time_absolute exposure_time_absolute);
int uvc_api_get_ct_exposure_time_absolute (struct uvc_dev *dev, struct uvc_ct_exposure_time_absolute *exposure_time_absolute);
int uvc_api_get_attrs_ct_exposure_time_absolute (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_exposure_time_absolute) *exposure_time_absolute);

/********************************************************************************************/
int uvc_api_set_ct_exposure_time_relative (struct uvc_dev *dev, struct uvc_ct_exposure_time_relative exposure_time_relative);
int uvc_api_get_ct_exposure_time_relative (struct uvc_dev *dev, struct uvc_ct_exposure_time_relative *exposure_time_relative);
int uvc_api_get_attrs_ct_exposure_time_relative (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_exposure_time_relative) *exposure_time_relative);

/********************************************************************************************/
int uvc_api_set_ct_focus_absolute (struct uvc_dev *dev, struct uvc_ct_focus_absolute focus_absolute);
int uvc_api_get_ct_focus_absolute (struct uvc_dev *dev, struct uvc_ct_focus_absolute *focus_absolute);
int uvc_api_get_attrs_ct_focus_absolute (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_focus_absolute) *focus_absolute);

/********************************************************************************************/
int uvc_api_set_ct_focus_relative (struct uvc_dev *dev, struct uvc_ct_focus_relative focus_relative);
int uvc_api_get_ct_focus_relative (struct uvc_dev *dev, struct uvc_ct_focus_relative *focus_relative);
int uvc_api_get_attrs_ct_focus_relative (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_focus_relative) *focus_relative);

/********************************************************************************************/
int uvc_api_set_ct_focus_auto (struct uvc_dev *dev, struct uvc_ct_focus_auto focus_auto);
int uvc_api_get_ct_focus_auto (struct uvc_dev *dev, struct uvc_ct_focus_auto *focus_auto);
int uvc_api_get_attrs_ct_focus_auto (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_focus_auto) *focus_auto);

/********************************************************************************************/
int uvc_api_set_ct_iris_absolute (struct uvc_dev *dev, struct uvc_ct_iris_absolute iris_absolute);
int uvc_api_get_ct_iris_absolute (struct uvc_dev *dev, struct uvc_ct_iris_absolute *iris_absolute);
int uvc_api_get_attrs_ct_iris_absolute (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_iris_absolute) *iris_absolute);

/********************************************************************************************/
int uvc_api_set_ct_iris_relative (struct uvc_dev *dev, struct uvc_ct_iris_relative iris_relative);
int uvc_api_get_ct_iris_relative(struct uvc_dev *dev, struct uvc_ct_iris_relative *iris_relative);
int uvc_api_get_attrs_ct_iris_relative (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_iris_relative) *iris_relative);

/********************************************************************************************/
int uvc_api_set_ct_zoom_absolute (struct uvc_dev *dev, struct uvc_ct_zoom_absolute zoom_absolute);
int uvc_api_get_ct_zoom_absolute (struct uvc_dev *dev, struct uvc_ct_zoom_absolute *zoom_absolute);
int uvc_api_get_attrs_ct_zoom_absolute (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_zoom_absolute) *zoom_absolute);

/********************************************************************************************/
int uvc_api_set_ct_zoom_relative (struct uvc_dev *dev, struct uvc_ct_zoom_relative zoom_relative);
int uvc_api_get_ct_zoom_relative (struct uvc_dev *dev, struct uvc_ct_zoom_relative *zoom_relative);
int uvc_api_get_attrs_ct_zoom_relative (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_zoom_relative) *zoom_relative);

/********************************************************************************************/
int uvc_api_set_ct_pantilt_absolute (struct uvc_dev *dev, struct uvc_ct_pantilt_absolute pantilt_absolute);
int uvc_api_get_ct_pantilt_absolute (struct uvc_dev *dev, struct uvc_ct_pantilt_absolute *pantilt_absolute);
int uvc_api_get_attrs_ct_pantilt_absolute (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_pantilt_absolute) *pantilt_absolute);

/********************************************************************************************/
int uvc_api_set_ct_pantilt_relative (struct uvc_dev *dev, struct uvc_ct_pantilt_relative pantilt_relative);
int uvc_api_get_ct_pantilt_relative (struct uvc_dev *dev, struct uvc_ct_pantilt_relative *pantilt_relative);
int uvc_api_get_attrs_ct_pantilt_relative (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_pantilt_relative) *pantilt_relative);

/********************************************************************************************/
int uvc_api_set_ct_roll_absolute (struct uvc_dev *dev, struct uvc_ct_roll_absolute roll_absolute);
int uvc_api_get_ct_roll_absolute (struct uvc_dev *dev, struct uvc_ct_roll_absolute *roll_absolute);
int uvc_api_get_attrs_ct_roll_absolute (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_roll_absolute) *roll_absolute);

/********************************************************************************************/
int uvc_api_set_ct_roll_relative (struct uvc_dev *dev, struct uvc_ct_roll_relative roll_relative);
int uvc_api_get_ct_roll_relative (struct uvc_dev *dev, struct uvc_ct_roll_relative *roll_relative);
int uvc_api_get_attrs_ct_roll_relative (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_roll_relative) *roll_relative);

/********************************************************************************************/
int uvc_api_set_ct_privacy (struct uvc_dev *dev, struct uvc_ct_privacy privacy);
int uvc_api_get_ct_privacy (struct uvc_dev *dev, struct uvc_ct_privacy *privacy);
int uvc_api_get_attrs_ct_privacy (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_ct_privacy) *privacy);

#ifdef __cplusplus
}
#endif

#endif /*UVC_API_CT_H*/