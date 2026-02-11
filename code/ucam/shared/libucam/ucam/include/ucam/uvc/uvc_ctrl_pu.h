/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_ctrl_pu.h
 * @Description : 
 */ 
#ifndef __UVC_CTRL_PU_H
#define __UVC_CTRL_PU_H

#include <ucam/uvc/uvc_ctrl.h>

struct uvc_pu_backlight_compensation {
	uint16_t wBacklightCompensation;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_backlight_compensation);

struct uvc_pu_brightness {
	int16_t wBrightness;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_brightness);

struct uvc_pu_contrast {
	uint16_t wContrast;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_contrast);

struct uvc_pu_contrast_auto {
	uint8_t bContrastAuto;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_contrast_auto);

struct uvc_pu_gain {
	uint16_t wGain;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_gain);

struct uvc_pu_power_line_frequency{
	uint8_t bPowerLineFrequency;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_power_line_frequency);

struct uvc_pu_hue{
	int16_t wHue;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_hue);

struct uvc_pu_hue_auto{
	uint8_t bHueAuto;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_hue_auto);

struct uvc_pu_saturation{
	uint16_t wSaturation;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_saturation);

struct uvc_pu_sharpness{
	uint16_t wSharpness;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_sharpness);

struct uvc_pu_gamma{
	uint16_t wGamma;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_gamma);

struct uvc_pu_white_balance_temperature{
	uint16_t wWhiteBalanceTemperature;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_white_balance_temperature);

struct uvc_pu_white_balance_temperature_auto{
	uint8_t bWhiteBalanceTemperatureAuto;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_white_balance_temperature_auto);

struct uvc_pu_white_balance_component{
	uint16_t wWhiteBalanceBlue;
    uint16_t wWhiteBalanceRed;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_white_balance_component);

struct uvc_pu_white_balance_component_auto{
	uint8_t bWhiteBalanceComponentAuto;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_white_balance_component_auto);

struct uvc_pu_digital_multiplier{
	uint16_t wMultiplierStep;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_digital_multiplier);

struct uvc_pu_digital_multiplier_limit{
	uint16_t wMultiplierLimit;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_digital_multiplier_limit);

struct uvc_pu_analog_video_standard{
	uint8_t bVideoStandard;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_analog_video_standard);

struct uvc_pu_analog_lock_status{
	uint8_t bStatus;
}__attribute__((__packed__));
DECLARE_UVC_CRTL_S(uvc_pu_analog_lock_status);

struct uvc_pu_config_t
{
	struct UVC_CRTL_S(uvc_pu_backlight_compensation) backlight_compensation;
    struct UVC_CRTL_S(uvc_pu_brightness) brightness;
    struct UVC_CRTL_S(uvc_pu_contrast) contrast;
    struct UVC_CRTL_S(uvc_pu_contrast_auto) contrast_auto;
    struct UVC_CRTL_S(uvc_pu_gain) gain;
    struct UVC_CRTL_S(uvc_pu_power_line_frequency) power_line_frequency;
    struct UVC_CRTL_S(uvc_pu_hue) hue;
    struct UVC_CRTL_S(uvc_pu_hue_auto) hue_auto;
    struct UVC_CRTL_S(uvc_pu_saturation) saturation;
    struct UVC_CRTL_S(uvc_pu_sharpness) sharpness;
    struct UVC_CRTL_S(uvc_pu_gamma) gamma;
    struct UVC_CRTL_S(uvc_pu_white_balance_temperature) white_balance_temperature;
    struct UVC_CRTL_S(uvc_pu_white_balance_temperature_auto) white_balance_temperature_auto;
    struct UVC_CRTL_S(uvc_pu_white_balance_component) white_balance_component;
    struct UVC_CRTL_S(uvc_pu_white_balance_component_auto) white_balance_component_auto;
    struct UVC_CRTL_S(uvc_pu_digital_multiplier) digital_multiplier;
    struct UVC_CRTL_S(uvc_pu_digital_multiplier_limit) digital_multiplier_limit;
    struct UVC_CRTL_S(uvc_pu_analog_video_standard) analog_video_standard;
    struct UVC_CRTL_S(uvc_pu_analog_lock_status) analog_lock_status;
};


#endif /* __UVC_CTRL_PU_H */