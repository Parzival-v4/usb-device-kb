/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc.h
 * @Description : 
 */ 
#ifndef _UVC_H
#define _UVC_H


//(1/fps) * 10000000
#define	UVC_FORMAT_60FPS						  (uint32_t)(0x00028B0A)
#define	UVC_FORMAT_50FPS						  (uint32_t)(0x00030D40)
#define	UVC_FORMAT_30FPS						  (uint32_t)(0x00051615)
#define	UVC_FORMAT_25FPS						  (uint32_t)(0x00061A80)
#define	UVC_FORMAT_24FPS						  (uint32_t)(0x00065b9a)
#define	UVC_FORMAT_20FPS						  (uint32_t)(0x0007a120)
#define	UVC_FORMAT_15FPS						  (uint32_t)(0x000a2c2a)
#define	UVC_FORMAT_10FPS						  (uint32_t)(0x000F4240)
#define	UVC_FORMAT_7FPS						  	  (uint32_t)(0x00145855)
#define	UVC_FORMAT_5FPS						  	  (uint32_t)(0x001E8480)
#define	UVC_FORMAT_1FPS						  	  (uint32_t)(0x00989680)


#define	 UVC_FRAME_INTERVAL(fps) ((1/fps) * 10000000)


/*--- <1> A.9.4. Camera Terminal Control Selectors ---*/

/* UVC_CT_SCANNING_MODE_CONTROL 0x01 */
/* <1> "4.2.2.1.1  Scanning Mode Control", page 81 */
#define SCANNING_MODE_INTERLACED 	0x00
#define SCANNING_MODE_PROGRESSIVE 	0x01

/* UVC_CT_AE_MODE_CONTROL 0x02 */
/* <1> "4.2.2.1.2  Auto-Exposure Mode Control ", page 81 */
#define AE_MODE_MANUAL 				0x01
#define AE_MODE_AUTO   				0x02
#define AE_MODE_SHUTTER_PRIORITY 	0x04
#define AE_MODE_APERTURE_PRIORITY 	0x08
#define AE_MODE_ALL					0x0f

/* UVC_CT_AE_PRIORITY_CONTROL 0x03 */
/* <1> "4.2.2.1.3  Auto-Exposure Priority Control", page 82 */
#define AE_PRIORITY_FRAMERATE_CONSTANT 0x00
#define AE_PRIORITY_FRAMERATE_VARIED   0x01

/* UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04 */
/* <1> "4.2.2.1.4  Exposure Time (Absolute) Control", page 82 */
/* notes: This value is expressed in 100μs units */

/* UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL 0x05 */
/* <1> "4.2.2.1.5  Exposure Time (Relative) Control", page 83 */

/* UVC_CT_FOCUS_ABSOLUTE_CONTROL 0x06 */
/* <1> "4.2.2.1.6  Focus (Absolute) Control", page 84 */
/* notes: This value is expressed in millimeters.*/

/* UVC_CT_FOCUS_RELATIVE_CONTROL 0x07 */
/* <1> "4.2.2.1.7  Focus (Relative) Control", page 84 */
#define FOCUS_RELATIVE_STOP 	0
#define FOCUS_RELATIVE_NEAR 	1
#define FOCUS_RELATIVE_INFINITE 0xff 

/* UVC_CT_FOCUS_AUTO_CONTROL 0x08 */
/* <1>"4.2.2.1.8  Focus, Auto Control", page 85 */
#define FOCUS_AUTO_ENABLED 		1
#define FOCUS_AUTO_DISABLED 	0

/* UVC_CT_IRIS_ABSOLUTE_CONTROL 0x09 */
/* <1>"4.2.2.1.9  Iris (Absolute) Control ", page 85 */
/* This value is expressed in units of fstop * 100. ??? */

/* UVC_CT_IRIS_RELATIVE_CONTROL 0x0a */
/* <1>"4.2.2.1.10  Iris (Relative) Control ", page 86 */
#define IRIS_RELATIVE_DEFAULT 		0
#define IRIS_RELATIVE_OPEN_1STEP	1
#define IRIS_RELATIVE_CLOSE_1STEP	0xff 

/* UVC_CT_ZOOM_ABSOLUTE_CONTROL 0x0b */
/* <1>"4.2.2.1.11 Zoom (Absolute) Control ", page 86 */
/* ??? */

/* UVC_CT_ZOOM_RELATIVE_CONTROL 0x0c */
/* <1>"4.2.2.1.12 Zoom (Relative) Control ", page 87 */
#define ZOOM_RELATIVE_STOP 			0
#define ZOOM_RELATIVE_TELEPHOTO		1
#define ZOOM_RELATIVE_WIDEANGLE		0xff 
#define ZOOM_RELATIVE_DIGITALZOOM_OFF	0 
#define ZOOM_RELATIVE_DIGITALZOOM_ON	1 

/* UVC_CT_PANTILT_ABSOLUTE_CONTROL 0x0d */
/* <1>"4.2.2.1.13 PanTilt (Absolute) Control ", page 88 */
/* in arc second units. 1 arc second is 1/3600 of a degree. 
   Values range from –180*3600 arc second to +180*3600 arc second, 
   or a subset thereof, with the default set to zero. */

/* UVC_CT_PANTILT_RELATIVE_CONTROL 0x0e */
/* <1>"4.2.2.1.14 PanTilt (Relative) Control ",page 89 */
#define PANTILT_RELATIVE_PAN_STOP 			0
#define PANTILT_RELATIVE_PAN_CLOCKWISE		1	
#define PANTILT_RELATIVE_PAN_ANTICLOCKWISE  0xff	
#define PANTILT_RELATIVE_TILT_STOP 			0
#define PANTILT_RELATIVE_TILT_IMAGEUP	 	1	
#define PANTILT_RELATIVE_TILT_IMAGEDOWN	 	0xff	

/* UVC_CT_ROLL_ABSOLUTE_CONTROL 0x0f */
/* <1>"4.2.2.1.15 Roll (Absolute) Control ", page 90 */

/* UVC_CT_ROLL_RELATIVE_CONTROL 0x10 */
/* <1>"4.2.2.1.16 Roll (Relative) Control ",page 90 */
#define ROLL_RELATIVE_STOP 			0
#define ROLL_RELATIVE_CLOCKWISE		1
#define ROLL_RELATIVE_ANTICLOCKWISE 0xff 

/* UVC_CT_PRIVACY_CONTROL 0x11 */
/* <1>"4.2.2.1.17 Privacy Control ",page 91*/
#define PRIVACY_OPEN 	0
#define PRIVACY_CLOSE	1

/*--- <1> A.9.5. Processing Unit Control Selectors ---*/

/* UVC_PU_BACKLIGHT_COMPENSATION_CONTROL 0x01 */
/* <1>"4.2.2.3.1  Backlight Compensation Control",page 93 */

/* UVC_PU_BRIGHTNESS_CONTROL 0x02 */
/* <1>"4.2.2.3.2  Brightness Control", page 93 */

/* UVC_PU_CONTRAST_CONTROL 0x03 */
/* <1>"4.2.2.3.3  Contrast Control",page 93*/

/* UVC_PU_GAIN_CONTROL 0x04 */
/* <1>"4.2.2.3.4  Gain Control", page 94 */

/* UVC_PU_POWER_LINE_FREQUENCY_CONTROL 0x05 */
/* <1>"4.2.2.3.5  Power Line Frequency Control", page 94 */
#define POWERLINE_FREQUENCY_DISABLED 	0
#define POWERLINE_FREQUENCY_50HZ		1
#define POWERLINE_FREQUENCY_60HZ		2

/* UVC_PU_HUE_CONTROL 0x06 */
/* <1>"4.2.2.3.6  Hue Control ", page 95 */

/* UVC_PU_SATURATION_CONTROL 0x07 */
/* <1>"4.2.2.3.8   Saturation Control", page 96*/

/* UVC_PU_SHARPNESS_CONTROL 0x08 */
/* <1>"4.2.2.3.9   Sharpness Control", page 96 */

/* UVC_PU_GAMMA_CONTROL 0x09 */
/* <1>"4.2.2.3.10  Gamma Control ", page 97 */

/* UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL 0x0a */
/* <1>"4.2.2.3.11  White Balance Temperature Control", page 97 */

/* UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL 0x0b */
/* <1>"4.2.2.3.12  White Balance Temperature, Auto Control ", page 98 */
#define WHITE_BALANCE_TEMPERATURE_AUTO_DISABLED 0
#define WHITE_BALANCE_TEMPERATURE_AUTO_ENABLED 	1

/* UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL 0x0c */
/* <1>"4.2.2.3.13 White Balance Component Control ", page 98 */

/* UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0d */
/* <1>"4.2.2.3.14  White Balance Component, Auto Control", page 99 */
#define WHITE_BALANCE_COMPONENT_AUTO_DISABLED 0
#define WHITE_BALANCE_COMPONENT_AUTO_ENABLED 	1

/* UVC_PU_DIGITAL_MULTIPLIER_CONTROL 0x0e */
/* <1>"4.2.2.3.15  Digital Multiplier Control", page 99 */

/* UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL 0x0f */
/* <1>"4.2.2.3.16 Digital Multiplier Limit Control", page 99 */

/* UVC_PU_HUE_AUTO_CONTROL 0x10 */
/* <1>"4.2.2.3.7   Hue, Auto Control", page 95 */
#define HUE_AUTO_DISABLED 	0
#define HUE_AUTO_ENABLED 	1

/* UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL 0x11 */
/* <1>"4.2.2.3.17 Analog Video Standard Control", page 100 */
#define ANALOG_VIDEO_STANDARD_NONE  	0	// 0: None 
#define ANALOG_VIDEO_STANDARD_NTSC60	1	// 1: NTSC – 525/60 
#define ANALOG_VIDEO_STANDARD_PAL50		2	// 2: PAL – 625/50 
#define ANALOG_VIDEO_STANDARD_SECAM50	3	// 3: SECAM – 625/50 
#define ANALOG_VIDEO_STANDARD_NTSC50	4	// 4: NTSC – 625/50 
#define ANALOG_VIDEO_STANDARD_PAL60		5	// 5: PAL – 525/60 

/* UVC_PU_ANALOG_LOCK_STATUS_CONTROL 0x12 */
/* <1>"4.2.2.3.18 Analog Video Lock Status Control ", page 100 */
#define ANALOG_LOCK_STATUS_LOCKED 	0
#define ANALOG_LOCK_STATUS_UNLOCKED 1

/*--- <1> A.9.7. VideoStreaming Interface Control Selectors */
/* UVC_VS_CONTROL_UNDEFINED 			0x00 */ 

/* UVC_VS_PROBE_CONTROL 				0x01 */
/* UVC_VS_COMMIT_CONTROL 				0x02 */ 
/* <1>"4.3.1.1  Video Probe and Commit Controls ", page 103 */
struct uvc_vs_probe_commit_ctrl_s{
	uint16_t	bmHint;
	uint8_t 	bFormatIndex;
	uint8_t 	bFrameIndex;
	uint32_t	dwFrameInterval;
	uint16_t	wKeyFrameRate;
	uint16_t	wPFrameRate;
	uint16_t	wCompQuality;
	uint16_t	wCompWindowSize;
	uint16_t	wDelay;
	uint32_t	dwMaxVideoFrameSize;
	uint32_t	dwMaxPayloadTransferSize;
	uint32_t	dwClockFrequency;
	uint8_t		bmFramingInfo;
	uint8_t		bPreferedVersion;
	uint8_t		bMinVersion;
	uint8_t		bMaxVersion;

	//UVC1.5
	uint8_t    bUsage;
	uint8_t    bBitDepthLuma;
	uint8_t    bmSettings;
	uint8_t    bMaxNumberOfRefFramesPlus1;
	uint16_t   bmRateControlModes;
	uint32_t   bmLayoutPerStream[2];
}__attribute__((__packed__));

#define UVC_VS_bmHints_FIXED_dwFrameInterval  	0x01
#define UVC_VS_bmHints_FIXED_wKeyFrameRate 		0x02
#define UVC_VS_bmHints_FIXED_wPFrameRate  		0x04
#define UVC_VS_bmHints_FIXED_wCompQuality  		0x08
#define UVC_VS_bmHints_FIXED_wCompWindowSize  	0x10

#define UVC_VS_bmFramingInfo_FID	0x01
#define UVC_VS_bmFramingInfo_EOF	0x02

/* <1>"4.3.1.2  Video Still Probe Control and Still Commit Control", page 116 */
struct uvc_vs_still_probe_commit_ctrl_s{
	uint8_t 	bFormatIndex;
	uint8_t 	bFrameIndex;
	uint8_t 	bCompressionIndex;
	uint32_t	dwMaxVideoFrameSize;
	uint32_t	dwMaxPayloadTransferSize;
}__attribute__((__packed__));

/* VS_STILL_IMAGE_TRIGGER_CONTROL 	0x05 */ 
/* <1>"4.3.1.4  Still Image Trigger Control", page 118 */
struct uvc_vs_still_image_trigger_ctrl_s{
	uint8_t 	bTrigger;
}__attribute__((__packed__));

#define UVC_VS_STILL_IMAGE_TRIGGER_NORMAL				0x00
#define UVC_VS_STILL_IMAGE_TRIGGER_TRANSMIT				0x01
#define UVC_VS_STILL_IMAGE_TRIGGER_TRANSMIT_BULKPIPE	0x02
#define UVC_VS_STILL_IMAGE_TRIGGER_ABORT				0x03

/* VS_STREAM_ERROR_CODE_CONTROL 	0x06 */ 
/* <1>"4.3.1.7  Stream Error Code Control", page 120 */
struct uvc_vs_stream_error_code_ctrl_s{
	uint8_t 	bStreamErrorCode;
}__attribute__((__packed__));

#define UVC_VS_STREAM_ERROR_CODE_NOERROR					0x00
#define UVC_VS_STREAM_ERROR_CODE_PROTECTED_CONTENT			0x01
#define UVC_VS_STREAM_ERROR_CODE_INPUT_BUFFER_UNDERRUN		0x02
#define UVC_VS_STREAM_ERROR_CODE_DATA_DISCONTINUITY			0x03
#define UVC_VS_STREAM_ERROR_CODE_OUTPUT_BUFFER_UNDERRUN		0x04
#define UVC_VS_STREAM_ERROR_CODE_OUTPUT_BUFFER_OVERRUN		0x05
#define UVC_VS_STREAM_ERROR_CODE_FORMAT_CHANGE				0x06
#define UVC_VS_STREAM_ERROR_CODE_STILL_IMAGE_CAPTURE_ERROR	0x07
#define UVC_VS_STREAM_ERROR_CODE_UNKNOWN_ERROR				0x08

/* VS_GENERATE_KEY_FRAME_CONTROL 	0x07 */ 
/* <1>"4.3.1.5  Generate Key Frame Control", page 119 */
struct uvc_vs_generate_key_frame_ctrl_s{
	uint8_t 	bGenerateKeyFrame;
}__attribute__((__packed__));

/* VS_UPDATE_FRAME_SEGMENT_CONTROL 	0x08 */ 
/* <1>"4.3.1.6  Update Frame Segment Control", page 119 */
struct uvc_vs_update_frame_segment_ctrl_s{
	uint8_t 	bStartFrameSegment;
	uint8_t 	bEndFrameSegment;
}__attribute__((__packed__));

/* VS_SYNCH_DELAY_CONTROL 			0x09 */ 
/* <1>"4.3.1.3  Synch Delay Control", page 117 */
struct uvc_vs_synch_delay_ctrl_s{
	uint16_t 	wDelay;
}__attribute__((__packed__));

#endif /* _UVC_H */
