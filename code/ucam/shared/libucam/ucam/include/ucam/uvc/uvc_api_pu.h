/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api_pu.h
 * @Description : 
 */ 
#ifndef UVC_API_PU_H 
#define UVC_API_PU_H 

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************************************/
int uvc_api_set_pu_backlight_compensation (struct uvc_dev *dev, struct uvc_pu_backlight_compensation backlight);
int uvc_api_get_pu_backlight_compensation (struct uvc_dev *dev, struct uvc_pu_backlight_compensation *backlight);
int uvc_api_get_attrs_pu_backlight_compensation (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_backlight_compensation) *backlight);

/********************************************************************************************/
int uvc_api_set_pu_brightness (struct uvc_dev *dev, struct uvc_pu_brightness brightness);
int uvc_api_get_pu_brightness (struct uvc_dev *dev, struct uvc_pu_brightness *brightness);
int uvc_api_get_attrs_pu_brightness (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_brightness) *brightness);

/********************************************************************************************/
int uvc_api_set_pu_contrast (struct uvc_dev *dev, struct uvc_pu_contrast contrast);
int uvc_api_get_pu_contrast (struct uvc_dev *dev, struct uvc_pu_contrast *contrast);
int uvc_api_get_attrs_pu_contrast (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_contrast) *contrast);

/********************************************************************************************/
int uvc_api_set_pu_gain (struct uvc_dev *dev, struct uvc_pu_gain gain);
int uvc_api_get_pu_gain (struct uvc_dev *dev, struct uvc_pu_gain *gain);
int uvc_api_get_attrs_pu_gain (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_gain) *gain);

/********************************************************************************************/
int uvc_api_set_pu_power_line_frequency (struct uvc_dev *dev, struct uvc_pu_power_line_frequency power_line_frequency);
int uvc_api_get_pu_power_line_frequency (struct uvc_dev *dev, struct uvc_pu_power_line_frequency *power_line_frequency);
int uvc_api_get_attrs_pu_power_line_frequency (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_power_line_frequency) *power_line_frequency);

/********************************************************************************************/
int uvc_api_set_pu_hue (struct uvc_dev *dev, struct uvc_pu_hue hue);
int uvc_api_get_pu_hue (struct uvc_dev *dev, struct uvc_pu_hue *hue);
int uvc_api_get_attrs_pu_hue (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_hue) *hue);

/********************************************************************************************/
int uvc_api_set_pu_saturation (struct uvc_dev *dev, struct uvc_pu_saturation saturation);
int uvc_api_get_pu_saturation (struct uvc_dev *dev, struct uvc_pu_saturation *saturation);
int uvc_api_get_attrs_pu_saturation (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_saturation) *saturation);

/********************************************************************************************/
int uvc_api_set_pu_sharpness (struct uvc_dev *dev, struct uvc_pu_sharpness sharpness);
int uvc_api_get_pu_sharpness (struct uvc_dev *dev, struct uvc_pu_sharpness *sharpness);
int uvc_api_get_attrs_pu_sharpness (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_sharpness) *sharpness);

/********************************************************************************************/
int uvc_api_set_pu_gamma (struct uvc_dev *dev, struct uvc_pu_gamma gamma);
int uvc_api_get_pu_gamma (struct uvc_dev *dev, struct uvc_pu_gamma *gamma);
int uvc_api_get_attrs_pu_gamma (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_gamma) *gamma);

/********************************************************************************************/
int uvc_api_set_pu_white_balance_temperature (struct uvc_dev *dev, struct uvc_pu_white_balance_temperature white_balance_temperature);
int uvc_api_get_pu_white_balance_temperature (struct uvc_dev *dev, struct uvc_pu_white_balance_temperature *white_balance_temperature);
int uvc_api_get_attrs_pu_white_balance_temperature (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_white_balance_temperature) *white_balance_temperature);

/********************************************************************************************/
int uvc_api_set_pu_white_balance_temperature_auto (struct uvc_dev *dev, struct uvc_pu_white_balance_temperature_auto white_balance_temperature_auto);
int uvc_api_get_pu_white_balance_temperature_auto (struct uvc_dev *dev, struct uvc_pu_white_balance_temperature_auto *white_balance_temperature_auto);
int uvc_api_get_attrs_pu_white_balance_temperature_auto (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_white_balance_temperature_auto) *white_balance_temperature_auto);

/********************************************************************************************/
int uvc_api_set_pu_white_balance_component (struct uvc_dev *dev, struct uvc_pu_white_balance_component white_balance_component);
int uvc_api_get_pu_white_balance_component (struct uvc_dev *dev, struct uvc_pu_white_balance_component *white_balance_component);
int uvc_api_get_attrs_pu_white_balance_component (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_white_balance_component) *white_balance_component);

/********************************************************************************************/
int uvc_api_set_pu_white_balance_component_auto (struct uvc_dev *dev, struct uvc_pu_white_balance_component_auto white_balance_component_auto);
int uvc_api_get_pu_white_balance_component_auto (struct uvc_dev *dev, struct uvc_pu_white_balance_component_auto *white_balance_component_auto);
int uvc_api_get_attrs_pu_white_balance_component_auto (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_white_balance_component_auto) *white_balance_component_auto);

/********************************************************************************************/
int uvc_api_set_pu_digital_multiplier (struct uvc_dev *dev, struct uvc_pu_digital_multiplier digital_multiplier);
int uvc_api_get_pu_digital_multiplier(struct uvc_dev *dev, struct uvc_pu_digital_multiplier *digital_multiplier);
int uvc_api_get_attrs_pu_digital_multiplier (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_digital_multiplier) *digital_multiplier);

/********************************************************************************************/
int uvc_api_set_pu_digital_multiplier_limit (struct uvc_dev *dev, struct uvc_pu_digital_multiplier_limit digital_multiplier_limit);
int uvc_api_get_pu_digital_multiplier_limit(struct uvc_dev *dev, struct uvc_pu_digital_multiplier_limit *digital_multiplier_limit);
int uvc_api_get_attrs_pu_digital_multiplier_limit (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_digital_multiplier_limit) *digital_multiplier_limit);

/********************************************************************************************/
int uvc_api_set_pu_hue_auto (struct uvc_dev *dev, struct uvc_pu_hue_auto hue_auto);
int uvc_api_get_pu_hue_auto(struct uvc_dev *dev, struct uvc_pu_hue_auto *hue_auto);
int uvc_api_get_attrs_pu_hue_auto (struct uvc_dev *dev, struct UVC_CRTL_S(uvc_pu_hue_auto) *hue_auto);

#ifdef __cplusplus
}
#endif

#endif /*UVC_API_PU_H*/