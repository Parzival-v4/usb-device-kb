/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_ctrl_xu_h264.h
 * @Description : 
 */ 
#ifndef __UVC_CTRL_XU_H264_H
#define __UVC_CTRL_XU_H264_H

/*--- <2> 3.3 H.264UVCExtensionsUnits(XUs), page 11  ---*/
/*
	    ControlSelector 				Value 	Comments
*/
#define UVCX_VIDEO_UNDEFINED 			0x00 // Reserved
#define UVCX_VIDEO_CONFIG_PROBE 		0x01 // Negotiateencoding parameters without altering current streaming state
#define UVCX_VIDEO_CONFIG_COMMIT 		0x02 // Sets the current configuration of the encoder
#define UVCX_RATE_CONTROL_MODE 			0x03 // Configuration of the encoder in bitrate/quality mode.
#define UVCX_TEMPORAL_SCALE_MODE 		0x04 // Number of layers
#define UVCX_SPATIAL_SCALE_MODE 		0x05 // Setting the spatial mode
#define UVCX_SNR_SCALE_MODE 			0x06 // Setting the quality mode
#define UVCX_LTR_BUFFER_SIZE_CONTROL 	0x07 // LTR Bufferusage
#define UVCX_LTR_PICTURE_CONTROL 		0x08 // LTR Control
#define UVCX_PICTURE_TYPE_CONTROL 		0x09 // I,IDR frame requests 
#define UVCX_VERSION 					0x0A // Spec. version supported from the device
#define UVCX_ENCODER_RESET 				0x0B // Encoder Reset
#define UVCX_FRAMERATE_CONFIG 			0x0C // Dynamic frame rate configuration
#define UVCX_VIDEO_ADVANCE_CONFIG 		0x0D // Configuration for level_idc
#define UVCX_BITRATE_LAYERS 			0x0E // Bitrate per layer
#define UVCX_QP_STEPS_LAYERS 			0x0F // Minimum/Maximum QP Configuration per layers


/* UVCX_VIDEO_UNDEFINED 		0x00 */
/* UVCX_VIDEO_CONFIG_PROBE 		0x01 & UVCX_VIDEO_CONFIG_COMMIT 	0x02 */ 
/* <2>"3.3.1 UVCX_VIDEO_CONFIG_PROBE&UVCX_VIDEO_CONFIG_COMMIT", page 12 */
/* <2>"5 Appendix-B", page 44 */
struct uvcx_h264_config{
	/* In 100ns frame interval Note:This shall not be lower than the UVC_PROBE/COMMITd wFrameInterval.*/
	uint32_t dwFrameInterval; 

	/* Average bits per second */
	uint32_t dwBitRate;

	uint16_t bmHints;
	uint16_t wConfigurationIndex;
	uint16_t wWidth;
	uint16_t wHeight;
	uint16_t wSliceUnits;
	uint16_t wSliceMode;
	uint16_t wProfile;
	uint16_t wIFramePeriod;
	uint16_t wEstimatedVideoDelay;
	uint16_t wEstimatedMaxConfigDelay;
	uint8_t bUsageType;
	uint8_t bRateControlMode;
	uint8_t bTemporalScaleMode;
	uint8_t bSpatialScaleMode;
	uint8_t bSNRScaleMode;
	uint8_t bStreamMuxOption;
	uint8_t bStreamFormat;
	uint8_t bEntropyCABAC;
	uint8_t bTimestamp;
	uint8_t bNumOfReorderFrames;
	uint8_t bPreviewFlipped;
	uint8_t bView;
	uint8_t bReserved1;
	uint8_t bReserved2;
	uint8_t bStreamID;
	uint8_t bSpatialLayerRatio;
	uint16_t wLeakyBucketSize;
}__attribute__((__packed__));


/* UVCX H264 video configuration parameters bitmap for bmHints */
#define UVCX_H264_bmHints_RESOLUTION 		0x0001 /* 0x0001:Resolution(wHeightandwWidth) */
#define UVCX_H264_bmHints_PROFILE			0x0002 /* 0x0002:Profile(wProfile) */
#define UVCX_H264_bmHints_RATECONTROLMODE	0x0004 /* 0x0004:RateControlMode(bRateControlMode) */
#define UVCX_H264_bmHints_USAGETYPE			0x0008 /* 0x0008:UsageType(bUsageType) */
#define UVCX_H264_bmHints_SLICEMODE        	0x0010 /* 0x0010:SliceMode(wSliceMode) */
#define UVCX_H264_bmHints_SLICEUNIT        	0x0020 /* 0x0020:SliceUnit(wSliceUnits) */
#define UVCX_H264_bmHints_MVCVIEW          	0x0040 /* 0x0040:MVCView(bView) */
#define UVCX_H264_bmHints_TEMPORAL         	0x0080 /* 0x0080:Temporal(bTemporalScaleMode) */
#define UVCX_H264_bmHints_SNR              	0x0100 /* 0x0100:SNR(bSNRScaleMode) */
#define UVCX_H264_bmHints_SPATIAL          	0x0200 /* 0x0200:Spatial(bSpatialScaleMode) */
#define UVCX_H264_bmHints_SPATIALLAYERRATIO	0x0400 /* 0x0400:SpatialLayerRatio(bSpatialLayerRatio) */
#define UVCX_H264_bmHints_FRAMEINTERVAL    	0x0800 /* 0x0800:Frameinterval(dwFrameInterval) */
#define UVCX_H264_bmHints_LEAKYBUCKETSIZE  	0x1000 /* 0x1000:LeakyBucketSize(wLeakyBucketSize) */
#define UVCX_H264_bmHints_BITRATE          	0x2000 /* 0x2000:BitRate(dwBitRate) */
#define UVCX_H264_bmHints_ENTROPYCABAC     	0x4000 /* 0x4000:EntropyCABAC(bEntropyCABAC) */
#define UVCX_H264_bmHints_IFRAMEPERIOD     	0x8000 /* 0x8000:IFramePeriod(wIFramePeriod) */

/* UVCX H264 video "profile_idc as defined in H.264 specification" and "Constrained flags" for wProfile */
#define UVCX_H264_wProfile_CONSTRAINED_BASELINEPROFILE  0x4240	/* 0x4240 ->Constrained BaselineProfile */
#define UVCX_H264_wProfile_BASELINEPROFILE   		0x4200	/* 0x4200 ->BaselineProfile */
#define UVCX_H264_wProfile_MAINPROFILE   			0x4d00 	/* 0x4D00 ->MainProfile */
#define UVCX_H264_wProfile_CONSTRAINED_HIGHPROFILE  0x640c	/* 0x640c ->Constrained HighProfile */
#define UVCX_H264_wProfile_HIGHPROFILE   			0x6400	/* 0x6400 ->HighProfile */
#define UVCX_H264_wProfile_CONSTRAINED_SCALABLEBASELINEPROFILE  0x5304	/* 0x5304 ->Constrained ScalableBaselineProfile */
#define UVCX_H264_wProfile_SCALABLEBASELINEPROFILE  	0x5300	/* 0x5300 ->ScalableBaselineProfile */
#define UVCX_H264_wProfile_CONSTRAINED_SCALABLEHIGHPROFILE   	0x5604	/* 0x5604 ->Constrained ScalableHighProfile */
#define UVCX_H264_wProfile_SCALABLEHIGHPROFILE   	0x5600	/* 0x5600 ->ScalableHighProfile */
#define UVCX_H264_wProfile_MULTIVIEWHIGHPROFILE   	0x7600	/* 0x7600 ->MultiviewHighProfile */
#define UVCX_H264_wProfile_STEREOHIGHPROFILE  		0x8000	/* 0x8000 ->StereoHighProfile */
#define UVCX_H264_wProfile_CONSTRAINT_SET0_FLAG 		0x0080	/* 0x0080->constraint_set0_flag */
#define UVCX_H264_wProfile_CONSTRAINT_SET1_FLAG 		0x0040	/* 0x0040->constraint_set1_flag */
#define UVCX_H264_wProfile_CONSTRAINT_SET2_FLAG 		0x0020	/* 0x0020->constraint_set2_flag */
#define UVCX_H264_wProfile_CONSTRAINT_SET3_FLAG 		0x0010	/* 0x0010->constraint_set3_flag */
#define UVCX_H264_wProfile_CONSTRAINT_SET4_FLAG 		0x0008	/* 0x0008->constraint_set4_flag */
#define UVCX_H264_wProfile_CONSTRAINT_SET5_FLAG 		0x0004	/* 0x0004->constraint_set5_flag */

/* UVCX H264 video "The time between IDR frames in milliseconds." for wIFramePeriod */
#define UVCX_H264_wIFramePeriod_NOREQ	0x0000	/* 0x0000=No periodicity requirements for IDR frames. */

/* UVCX H264 video "Encoder Configuration based on the host configured usage type" for bUsageType */
#define UVCX_H264_bUsageType_REALTIME	0x01	/* 0x01:Real-time(videoconf) */
#define UVCX_H264_bUsageType_BROADCAST	0x02	/* 0x02:Broadcast */
#define UVCX_H264_bUsageType_STORAGE		0x03	/* 0x03:Storage */

/* UVCX H264 video for bRateControlMode */
#define UVCX_H264_bRateControlMode_CBR					0x01	/* 0x01:CBR */
#define UVCX_H264_bRateControlMode_VBR					0x02	/* 0x02:VBR */
#define UVCX_H264_bRateControlMode_CONSTANTQP			0x03	/* 0x03:ConstantQP */
#define UVCX_H264_bRateControlMode_FIXEDFRAMERATEFLAG	0x10	/* 0x10:fixed_frame_rate_flag */

/* UVCX H264 video for bSNRScaleMode */
#define UVCX_H264_bSNRScaleMode_NOLAYER 0x00	/* 0x00:No SNR Enhancement Layer */
#define UVCX_H264_bSNRScaleMode_CGS_NONREWRITE_TWOLAYER  	0x02	/* 0x02:CGS_NonRewrite_TwoLayer */
#define UVCX_H264_bSNRScaleMode_CGS_NONREWRITE_THREELAYER	0x03	/* 0x03:CGS_NonRewrite_ThreeLayer */
#define UVCX_H264_bSNRScaleMode_CGS_REWRITE_TWOLAYER  		0x04	/* 0x04:CGS_Rewrite_TwoLayer */
#define UVCX_H264_bSNRScaleMode_CGS_REWRITE_THREELAYER  		0x05	/* 0x05:CGS_Rewrite_ThreeLayer */
#define UVCX_H264_bSNRScaleMode_MGS_TWOLAYER  				0x06	/* 0x06:MGS_TwoLayer */

/* UVCX H264 video "Auxiliary stream ctrl " for bStreamMuxOption */
#define UVCX_H264_bStreamMuxOption_ENABLE 			0x01	/* Bit0:Enable/Disable auxiliary stream */
#define UVCX_H264_bStreamMuxOption_H264				0x02	/* Bit1:Embed H.264 auxiliary stream. */
#define UVCX_H264_bStreamMuxOption_YUY2				0x04	/* Bit2:Embed YUY2 auxiliary stream. */
#define UVCX_H264_bStreamMuxOption_NV12				0x08	/* Bit3:Embed NV12 auxiliary stream. */
#define UVCX_H264_bStreamMuxOption_MJPED_CONTAINER 	0x40		/* Bit6:MJPEG payload used as a container. */

/* UVCX H264 video "" for bStreamFormat */
#define UVCX_H264_bStreamFormat_BYTESTREAM		0x00	/* 0x00–Output data in Byte stream format (H.264 Annex-B) */
#define UVCX_H264_bStreamFormat_NALSTREAM		0x01	/* 0x01–Output data in NAL stream format */

/* UVCX H264 video "" for bEntropyCABAC */
#define UVCX_H264_bEntropyCABAC_CAVLC	0x00	/* 0x00=CAVLC */
#define UVCX_H264_bEntropyCABAC_CABAC	0x01	/* 0x01=CABAC  */

/* UVCX H264 video "" for bTimestamp */
#define UVCX_H264_bTimestamp_DISABLED	0x00	/* 0x00=picture timing SEI disabled */
#define UVCX_H264_bTimestamp_ENABLED		0x01	/* 0x01=picture timing SEI enabled */

/* UVCX H264 video "" for bPreviewFlipped */
#define UVCX_H264_bPreviewFlipped_NOCHANGE 	0x00	/* 0x00=NoChange */
#define UVCX_H264_bPreviewFlipped_HORIZONTAL	0x01	/* 0x01=Horizontal Flipped Image for non H.264 streams */

/* UVCX_RATE_CONTROL_MODE 		0x03 */ 
/* <2>"3.3.3 UVCX_RATE_CONTROL_MODE", page 22 */
struct uvcx_rate_ctrl_mode {
	uint16_t 	wLayerID;
	uint8_t 	bRateControlMode;
}__attribute__((__packed__));


/* UVCX_TEMPORAL_SCALE_MODE 	0x04 */
/* <2>"3.3.4 UVCX_TEMPORAL_SCALE_MODE", page 21 */
struct uvcx_temporal_scale_mode {
	uint16_t	wLayerID;
	uint8_t		bTemporalScaleMode;
}__attribute__((__packed__));


/* UVCX_SPATIAL_SCALE_MODE 		0x05 */
/* <2>"3.3.5 UVCX_SPATIAL_SCALE_MODE", page 22 */
struct uvcx_spatial_scale_mode {
	uint16_t	wLayerID;
	uint8_t		bSpatialScaleMode;
}__attribute__((__packed__));


/* UVCX_SNR_SCALE_MODE 			0x06 */
/* <2>"3.3.6 UVCX_SNR_SCALE_MODE", page 23 */
struct uvcx_snr_scale_mode {
	uint16_t	wLayerID;
	uint8_t		bSNRScaleMode;
	uint8_t		bMGSSublayerMode;
}__attribute__((__packed__));


/* UVCX_LTR_BUFFER_SIZE_CONTROL 0x07 */
/* <2>"3.3.7 UVCX_LTR_BUFFER_SIZE_CONTROL", page 27 */
struct uvcx_ltr_buffer_size_ctrl {
	uint16_t	wLayerID;
	uint8_t		bLTRBufferSize;
	uint8_t		bLTREncoderControl;
}__attribute__((__packed__));


/* UVCX_LTR_PICTURE_CONTROL 	0x08 */
/* <2>"3.3.8 UVCX_LTR_PICTURE_CONTROL", page 28 */
struct uvcx_ltr_picture_ctrl { 
	uint16_t	wLayerID;
	uint8_t		bPutAtPositionInLTRBuffer;
	uint8_t		bEncodeUsingLTR;
}__attribute__((__packed__));


/* UVCX_PICTURE_TYPE_CONTROL 	0x09 */
/* <2>"3.3.9 UVCX_PICTURE_TYPE_CONTROL", page  31 */
struct uvcx_picture_type_ctrl { 	
	uint16_t	wLayerID;
	uint16_t	wPicType;
}__attribute__((__packed__));


#define UVCX_H264_PICTURE_TYPE_IFRAME 		0x0000	/* 0x0000:I-Frame */
#define UVCX_H264_PICTURE_TYPE_IDRFRAME 	0x0001	/* 0x0001:Generate an IDR frame */
#define UVCX_H264_PICTURE_TYPE_IDRSPSPPS 	0x0002	/* 0x0002:Generate an IDR frame with new SPS and PPS */

/* UVCX_VERSION 				0x0A */
/* <2>"3.3.10 UVCX_VERSION", page 32 */
struct uvcx_version { 
	uint16_t	wVersion;
}__attribute__((__packed__));


#define UVCX_H264_VERSION	0x0100	/* Version 1.00, 0x0100 for this version, BCDformat */

/* UVCX_ENCODER_RESET 			0x0B */
/* <2>"3.3.11.1 UVCX_ENCODER_RESET", page 32 */
struct uvcx_encoder_reset { 
	uint16_t	wLayerID;
}__attribute__((__packed__));


/* UVCX_FRAMERATE_CONFIG 		0x0C */
/* <2>"3.3.12 UVCX_FRAMERATE_CONFIG", page 33 */
/* Notes: In 100ns frame interval */
struct uvcx_framerate_config { 
	uint16_t	wLayerID;
	uint32_t	dwFrameInterval;
}__attribute__((__packed__));


/* UVCX_VIDEO_ADVANCE_CONFIG 	0x0D */
/* <2>"3.3.13 UVCX_VIDEO_ADVANCE_CONFIG", page 34 */
struct uvcx_video_advance_config { 
	uint16_t	wLayerID;
	uint32_t	dwMb_max;
	uint8_t		blevel_idc;
	uint8_t		bReserved;
}__attribute__((__packed__));


/* UVCX_BITRATE_LAYERS 			0x0E */
/* <2>"3.3.14 UVCX_BITRATE_LAYERS", page 35 */
struct uvcx_bitrate_layers { 
	uint16_t	wLayerID;
	uint32_t	dwPeakBitrate;
	uint32_t	dwAverageBitrate;
}__attribute__((__packed__));


/* UVCX_QP_STEPS_LAYERS 		0x0F */
/* <2>"3.3.15 UVCX_QP_STEPS_LAYERS", page 36 */
struct uvcx_qp_steps_layers {
	uint16_t	wLayerID;
	uint8_t		bFrameType;
	uint8_t		bMinQp;
	uint8_t		bMaxQp;
}__attribute__((__packed__));


#define UVCX_H264_FRAMETYPE_IFRAME	0x01	/* 0x01=Iframe */
#define UVCX_H264_FRAMETYPE_PFRAME	0x02	/* 0x02=Pframe */
#define UVCX_H264_FRAMETYPE_BFRAME	0x04	/* 0x04=Bframe */
#define UVCX_H264_FRAMETYPE_ALLTYPE	0x04	/* 0x07=alltypes */

#define UVCX_H264_wSliceMode_NOSLICE	0x00
#define UVCX_H264_wSliceMode_BITSLICE	0x01
#define UVCX_H264_wSliceMode_MBITSLICE	0x02
#define UVCX_H264_wSliceMode_PERSLICE	0x03


struct uvc_h264_config_t
{
	struct uvcx_h264_config uvcx_h264_config_def;
	struct uvcx_h264_config uvcx_h264_config_cur;
	struct uvcx_h264_config uvcx_h264_config_min;
	struct uvcx_h264_config uvcx_h264_config_max;

	struct uvcx_rate_ctrl_mode uvcx_rate_ctrl_mode_def;
	struct uvcx_rate_ctrl_mode uvcx_rate_ctrl_mode_cur;
	struct uvcx_rate_ctrl_mode uvcx_rate_ctrl_mode_min;
	struct uvcx_rate_ctrl_mode uvcx_rate_ctrl_mode_max;

	struct uvcx_temporal_scale_mode uvcx_temporal_scale_mode_def;
	struct uvcx_temporal_scale_mode uvcx_temporal_scale_mode_cur;
	struct uvcx_temporal_scale_mode uvcx_temporal_scale_mode_min;
	struct uvcx_temporal_scale_mode uvcx_temporal_scale_mode_max;

	struct uvcx_spatial_scale_mode uvcx_spatial_scale_mode_def;
	struct uvcx_spatial_scale_mode uvcx_spatial_scale_mode_cur;
	struct uvcx_spatial_scale_mode uvcx_spatial_scale_mode_min;
	struct uvcx_spatial_scale_mode uvcx_spatial_scale_mode_max;

	struct uvcx_snr_scale_mode uvcx_snr_scale_mode_def;
	struct uvcx_snr_scale_mode uvcx_snr_scale_mode_cur;
	struct uvcx_snr_scale_mode uvcx_snr_scale_mode_min;
	struct uvcx_snr_scale_mode uvcx_snr_scale_mode_max;

	struct uvcx_ltr_buffer_size_ctrl uvcx_ltr_buffer_size_ctrl_def;
	struct uvcx_ltr_buffer_size_ctrl uvcx_ltr_buffer_size_ctrl_cur;
	struct uvcx_ltr_buffer_size_ctrl uvcx_ltr_buffer_size_ctrl_min;
	struct uvcx_ltr_buffer_size_ctrl uvcx_ltr_buffer_size_ctrl_max;

	struct uvcx_ltr_picture_ctrl uvcx_ltr_picture_ctrl_def;
	struct uvcx_ltr_picture_ctrl uvcx_ltr_picture_ctrl_cur;
	struct uvcx_ltr_picture_ctrl uvcx_ltr_picture_ctrl_min;
	struct uvcx_ltr_picture_ctrl uvcx_ltr_picture_ctrl_max;

	struct uvcx_picture_type_ctrl uvcx_picture_type_ctrl_def;
	struct uvcx_picture_type_ctrl uvcx_picture_type_ctrl_cur;
	struct uvcx_picture_type_ctrl uvcx_picture_type_ctrl_min;
	struct uvcx_picture_type_ctrl uvcx_picture_type_ctrl_max;

	struct uvcx_version uvcx_version_def;
	struct uvcx_version uvcx_version_cur;
	struct uvcx_version uvcx_version_min;
	struct uvcx_version uvcx_version_max;

	struct uvcx_encoder_reset uvcx_encoder_reset_def;
	struct uvcx_encoder_reset uvcx_encoder_reset_cur;
	struct uvcx_encoder_reset uvcx_encoder_reset_min;
	struct uvcx_encoder_reset uvcx_encoder_reset_max;

	struct uvcx_framerate_config uvcx_framerate_config_def;
	struct uvcx_framerate_config uvcx_framerate_config_cur;
	struct uvcx_framerate_config uvcx_framerate_config_min;
	struct uvcx_framerate_config uvcx_framerate_config_max;

	struct uvcx_video_advance_config uvcx_video_advance_config_def;
	struct uvcx_video_advance_config uvcx_video_advance_config_cur;
	struct uvcx_video_advance_config uvcx_video_advance_config_min;
	struct uvcx_video_advance_config uvcx_video_advance_config_max;

	struct uvcx_bitrate_layers uvcx_bitrate_layers_def;
	struct uvcx_bitrate_layers uvcx_bitrate_layers_cur;
	struct uvcx_bitrate_layers uvcx_bitrate_layers_min;
	struct uvcx_bitrate_layers uvcx_bitrate_layers_max;

	struct uvcx_qp_steps_layers uvcx_qp_steps_layers_def;
	struct uvcx_qp_steps_layers uvcx_qp_steps_layers_cur;
	struct uvcx_qp_steps_layers uvcx_qp_steps_layers_min;
	struct uvcx_qp_steps_layers uvcx_qp_steps_layers_max;
};

typedef int (*uvc_xu_h264_set_format_callback_fn_t)(struct uvc_dev *dev, struct uvcx_h264_config *h264_ctrl);

extern int register_uvc_xu_h264_set_format_callback(uvc_xu_h264_set_format_callback_fn_t callback_fn);

#endif /* __UVC_CTRL_XU_H264_H */