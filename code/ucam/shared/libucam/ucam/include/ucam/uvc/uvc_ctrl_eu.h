/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_ctrl_eu.h
 * @Description : 
 */ 
#ifndef __UVC_CTRL_EU_H
#define __UVC_CTRL_EU_H

#include <sys/types.h>
#include <stdint.h>

/*A.9.6. Encoding Unit Control Selectors */
#define EU_SELECT_LAYER_CONTROL			(0x1)
#define EU_PROFILE_TOOLSET_CONTROL		(0x2)
#define EU_VIDEO_RESOLUTION_CONTROL 	(0x3)
#define EU_MIN_FRAME_INTERVAL_CONTROL 	(0x4)
#define EU_SLICE_MODE_CONTROL			(0x5)
#define EU_RATE_CONTROL_MODE_CONTROL 	(0x6)
#define EU_AVERAGE_BITRATE_CONTROL		(0x7)
#define EU_CPB_SIZE_CONTROL				(0x8)
#define EU_PEAK_BIT_RATE_CONTROL		(0x9)
#define EU_QUANTIZATION_PARAMS_CONTROL 	(0xa)
#define EU_SYNC_REF_FRAME_CONTROL 		(0xb)
#define EU_LTR_BUFFER_CONTROL			(0xc)
#define EU_LTR_PICTURE_CONTROL			(0xd)
#define EU_LTR_VALIDATION_CONTROL		(0xe)
#define EU_LEVEL_IDC_LIMIT_CONTROL 		(0xf)
#define EU_SEI_PAYLOADTYPE_CONTROL 		(0x10)
#define EU_QP_RANGE_CONTROL				(0x11)
#define EU_PRIORITY_CONTROL				(0x12)
#define EU_START_OR_STOP_LAYER_CONTROL	(0x13)
#define EU_ERROR_RESILIENCY_CONTROL		(0x14)



typedef union LayerOrViewIDBits {
		/** raw register data */
	uint16_t d16;//16位
		/** register bits */
	struct {//低位在前[0]...[16]
		unsigned dependency_id:3;//长度为3
		unsigned quality_id:4;
		unsigned temporal_id:3;
		unsigned stream_id:3;
		unsigned reserved:3;
	} b;
} LayerOrViewIDBits_t;

struct uvc_encode_unit_attr{
	//EU_SELECT_LAYER_CONTROL
	#define EU_SELECT_LAYER_CONTROL_LEN 2
	LayerOrViewIDBits_t wLayerOrViewID;

	/*
	For multi-layer streams, a combination of dependency_id, quality_id, temporal_id and stream_id. Bits:
	0-2: dependency_id
	3-6: quality_id
	7-9: temporal_id
	10-12: stream_id
	13-15: Reserved, set to 0

	For multi-view streams, this value contains a combination of view_id, temporal_id, stream_id, and interface number. Bits:
	0-6: view_id
	7-9: temporal_id
	10-12: stream_id
	13-15: Reserved, set to 0

	When the stream does not support layering or multiples view, bits 0-9 are zero.
	 */

	//EU_PROFILE_TOOLSET_CONTROL
	#define EU_PROFILE_TOOLSET_CONTROL_LEN 5
	uint16_t wProfile;
	/*
		0x4240: Constrained Baseline Profile
		0x4200: Baseline Profile
		0x4D00: Main Profile
		0x640C: Constrained High Profile
		0x6400: High Profile
		0x5304: Scalable Constrained Baseline Profile
		0x5300: Scalable Baseline Profile
		0x5604: Scalable Constrained High Profile
		0x5600: Scalable High Profile
		0x7600: Multiview High Profile
		0x8000: Stereo High Profile
	 */
	uint16_t wConstrainedToolset;//Reserved. Set to 0.
	uint8_t	bmSettings;

	//EU_VIDEO_RESOLUTION_CONTROL
	#define EU_VIDEO_RESOLUTION_CONTROL_LEN	4
	uint16_t wWidth;
	uint16_t wHeight;

	//EU_MIN_FRAME_INTERVAL_CONTROL
	#define EU_MIN_FRAME_INTERVAL_CONTROL_LEN	4
	uint32_t dwFrameInterval;

	//EU_SLICE_MODE_CONTROL
	#define EU_SLICE_MODE_CONTROL_LEN	4
	uint16_t wSliceMode;
	/*
		0: Maximum number of MBs per slice mode
		1: Target compressed size per slice mode
		2: Number of slices per frame mode
		3: Number of Macroblock rows per slice mode
		4-255: Reserved
	*/
	uint16_t wSliceConfigSetting;
	/*
		Mode 0: Maximum number of MBs per slice.
		Mode 1: Target size for each slice NALU in bytes.
		Mode 2: Number of slices per frame.
		Mode 3: Number of macroblock rows per slice.
	 */

	//EU_RATE_CONTROL_MODE_CONTROL
	#define EU_RATE_CONTROL_MODE_CONTROL_LEN	1
	uint8_t bRateControlMode;
	/*
	    0: Reserved
		1: Variable Bit Rate low delay (VBR)
		2: Constant bit rate (CBR)
		3: Constant QP
		4: Global VBR low delay (GVBR)
		5: Variable bit rate non-low delay (VBRN)
		6: Global VBR non-low delay (GVBRN)
		7-255: Reserved
	 */

	//EU_AVERAGE_BITRATE_CONTROL
	#define EU_AVERAGE_BITRATE_CONTROL_LEN	4
	uint32_t dwAverageBitRate;

	//EU_CPB_SIZE_CONTROL
	#define EU_CPB_SIZE_CONTROL_LEN			4
	uint32_t dwCPBsize;

	//EU_PEAK_BIT_RATE_CONTROL
	#define EU_PEAK_BIT_RATE_CONTROL_LEN	4
	uint32_t dwPeakBitRate ;

	//EU_QUANTIZATION_PARAMS_CONTROL
	#define EU_QUANTIZATION_PARAMS_CONTROL_LEN	6
	uint16_t wQpPrime_I;
	uint16_t wQpPrime_P;
	uint16_t wQpPrime_B;

	//EU_QP_RANGE_CONTROL
	#define EU_QP_RANGE_CONTROL_LEN		2
	uint8_t bMinQp;
	uint8_t bMaxQp;

	//EU_SYNC_REF_FRAME_CONTROL
#define EU_SYNC_REF_FRAME_CONTROL_LEN	4
	uint8_t	bSyncFrameType;
	uint16_t wSyncFrameInterval;
	uint8_t bGradualDecoderRefresh;

	//EU_LTR_BUFFER_CONTROL
	#define EU_LTR_BUFFER_CONTROL_LEN	2
	uint8_t bNumHostControlLTRBuffers;
	/*
		Number of Long-Term Reference Frames the host can control.
	*/
	uint8_t bTrustMode;
	/*
		Trust mode for the LTR feature.
		0 – For each inserted LTR, device sets associated bit in bmValidLTRs to 0 (Don’t Trust Until)
		1 – For each inserted LTR, device sets associated bit in bmValidLTRs to 1 (Trust Until)
	 */

	//EU_LTR_PICTURE_CONTROL
	#define EU_LTR_PICTURE_CONTROL_LEN		2
	uint8_t bPutAtPositionInLTRBuffer;
	uint8_t bLTRMode ;

	//EU_LTR_VALIDATION_CONTROL
	#define EU_LTR_VALIDATION_CONTROL_LEN	2
	uint16_t bmValidLTRs ;

	//EU_SEI_PAYLOADTYPE_CONTROL
	#define EU_SEI_PAYLOADTYPE_CONTROL_LEN	8
	uint64_t bmSEIMessages;

	//EU_PRIORITY_CONTROL
	#define EU_PRIORITY_CONTROL_LEN		1
	uint8_t bPriority;

	//EU_START_OR_STOP_LAYER_CONTROL
	#define EU_START_OR_STOP_LAYER_CONTROL_LEN		1
	uint8_t bUpdate;

	//EU_LEVEL_IDC_LIMIT_CONTROL
	#define EU_LEVEL_IDC_LIMIT_CONTROL_LEN		1
	uint8_t bLevelIDC;

	//EU_ERROR_RESILIENCY_CONTROL
	#define EU_ERROR_RESILIENCY_CONTROL_LEN		2
	uint16_t bmErrorResiliencyFeatures;

};


#endif /* __UVC_CTRL_EU_H */