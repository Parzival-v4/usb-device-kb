/*
 * USB Video Class definitions.
 *
 * Copyright (C) 2009 Laurent Pinchart <laurent.pinchart@skynet.be>
 *
 * This file holds USB constants and structures defined by the USB Device
 * Class Definition for Video Devices. Unless otherwise stated, comments
 * below reference relevant sections of the USB Video Class 1.1 specification
 * available at
 *
 * http://www.usb.org/developers/devclass_docs/USB_Video_Class_1_1.zip
 */

#ifndef __LINUX_USB_VIDEO_H
#define __LINUX_USB_VIDEO_H

#include <linux/types.h>

/* --------------------------------------------------------------------------
 * UVC constants
 */

/* A.2. Video Interface Subclass Codes */
#define UVC_SC_UNDEFINED						0x00
#define UVC_SC_VIDEOCONTROL						0x01
#define UVC_SC_VIDEOSTREAMING					0x02
#define UVC_SC_VIDEO_INTERFACE_COLLECTION		0x03

/* A.3. Video Interface Protocol Codes */
#define UVC_PC_PROTOCOL_UNDEFINED				0x00
#define UVC_PC_PROTOCOL_15						0x01

/* A.5. Video Class-Specific VC Interface Descriptor Subtypes */
#define UVC_VC_DESCRIPTOR_UNDEFINED			0x00
#define UVC_VC_HEADER						0x01
#define UVC_VC_INPUT_TERMINAL				0x02
#define UVC_VC_OUTPUT_TERMINAL				0x03
#define UVC_VC_SELECTOR_UNIT				0x04
#define UVC_VC_PROCESSING_UNIT				0x05
#define UVC_VC_EXTENSION_UNIT				0x06

/* A.6. Video Class-Specific VS Interface Descriptor Subtypes */
#define UVC_VS_UNDEFINED					0x00
#define UVC_VS_INPUT_HEADER					0x01
#define UVC_VS_OUTPUT_HEADER				0x02
#define UVC_VS_STILL_IMAGE_FRAME			0x03
#define UVC_VS_FORMAT_UNCOMPRESSED			0x04
#define UVC_VS_FRAME_UNCOMPRESSED			0x05
#define UVC_VS_FORMAT_MJPEG					0x06
#define UVC_VS_FRAME_MJPEG					0x07
#define UVC_VS_FORMAT_MPEG2TS				0x0a
#define UVC_VS_FORMAT_DV					0x0c
#define UVC_VS_COLORFORMAT					0x0d
#define UVC_VS_FORMAT_FRAME_BASED			0x10
#define UVC_VS_FRAME_FRAME_BASED			0x11
#define UVC_VS_FORMAT_STREAM_BASED			0x12
#define UVC_VS_FORMAT_H264					0x13
#define UVC_VS_FRAME_H264					0x14
#define UVC_VS_FORMAT_H264_V150				0x13
#define UVC_VS_FRAME_H264_V150				0x14
#define UVC_VS_FORMAT_H264_SIMULCAST		0x15

/* A.7. Video Class-Specific Endpoint Descriptor Subtypes */
#define UVC_EP_UNDEFINED				0x00
#define UVC_EP_GENERAL					0x01
#define UVC_EP_ENDPOINT					0x02
#define UVC_EP_INTERRUPT				0x03

/* A.8. Video Class-Specific Request Codes */
#define UVC_RC_UNDEFINED			0x00
#define UVC_SET_CUR					0x01
#define UVC_GET_CUR					0x81
#define UVC_GET_MIN					0x82
#define UVC_GET_MAX					0x83
#define UVC_GET_RES					0x84
#define UVC_GET_LEN					0x85
#define UVC_GET_INFO				0x86
#define UVC_GET_DEF					0x87

/* A.9.1. VideoControl Interface Control Selectors */
#define UVC_VC_CONTROL_UNDEFINED			0x00
#define UVC_VC_VIDEO_POWER_MODE_CONTROL		0x01
#define UVC_VC_REQUEST_ERROR_CODE_CONTROL	0x02

/* A.9.2. Terminal Control Selectors */
#define UVC_TE_CONTROL_UNDEFINED			0x00

/* A.9.3. Selector Unit Control Selectors */
#define UVC_SU_CONTROL_UNDEFINED			0x00
#define UVC_SU_INPUT_SELECT_CONTROL			0x01

/* A.9.4. Camera Terminal Control Selectors */
#define UVC_CT_CONTROL_UNDEFINED				0x00
#define UVC_CT_SCANNING_MODE_CONTROL			0x01
#define UVC_CT_AE_MODE_CONTROL					0x02
#define UVC_CT_AE_PRIORITY_CONTROL				0x03
#define UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL	0x04
#define UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL	0x05
#define UVC_CT_FOCUS_ABSOLUTE_CONTROL			0x06
#define UVC_CT_FOCUS_RELATIVE_CONTROL			0x07
#define UVC_CT_FOCUS_AUTO_CONTROL				0x08
#define UVC_CT_IRIS_ABSOLUTE_CONTROL			0x09
#define UVC_CT_IRIS_RELATIVE_CONTROL			0x0a
#define UVC_CT_ZOOM_ABSOLUTE_CONTROL			0x0b
#define UVC_CT_ZOOM_RELATIVE_CONTROL			0x0c
#define UVC_CT_PANTILT_ABSOLUTE_CONTROL			0x0d
#define UVC_CT_PANTILT_RELATIVE_CONTROL			0x0e
#define UVC_CT_ROLL_ABSOLUTE_CONTROL			0x0f
#define UVC_CT_ROLL_RELATIVE_CONTROL			0x10
#define UVC_CT_PRIVACY_CONTROL					0x11

/* A.9.5. Processing Unit Control Selectors */
#define UVC_PU_CONTROL_UNDEFINED						0x00
#define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL			0x01
#define UVC_PU_BRIGHTNESS_CONTROL						0x02
#define UVC_PU_CONTRAST_CONTROL							0x03
#define UVC_PU_GAIN_CONTROL								0x04
#define UVC_PU_POWER_LINE_FREQUENCY_CONTROL				0x05
#define UVC_PU_HUE_CONTROL								0x06
#define UVC_PU_SATURATION_CONTROL						0x07
#define UVC_PU_SHARPNESS_CONTROL						0x08
#define UVC_PU_GAMMA_CONTROL							0x09
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL		0x0a
#define UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL	0x0b
#define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL			0x0c
#define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL		0x0d
#define UVC_PU_DIGITAL_MULTIPLIER_CONTROL				0x0e
#define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL			0x0f
#define UVC_PU_HUE_AUTO_CONTROL							0x10
#define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL			0x11
#define UVC_PU_ANALOG_LOCK_STATUS_CONTROL				0x12

/* A.9.7. VideoStreaming Interface Control Selectors */
#define UVC_VS_CONTROL_UNDEFINED				0x00
#define UVC_VS_PROBE_CONTROL					0x01
#define UVC_VS_COMMIT_CONTROL					0x02
#define UVC_VS_STILL_PROBE_CONTROL				0x03
#define UVC_VS_STILL_COMMIT_CONTROL				0x04
#define UVC_VS_STILL_IMAGE_TRIGGER_CONTROL		0x05
#define UVC_VS_STREAM_ERROR_CODE_CONTROL		0x06
#define UVC_VS_GENERATE_KEY_FRAME_CONTROL		0x07
#define UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL		0x08
#define UVC_VS_SYNC_DELAY_CONTROL				0x09

/* B.1. USB Terminal Types */
#define UVC_TT_VENDOR_SPECIFIC			0x0100
#define UVC_TT_STREAMING				0x0101

/* B.2. Input Terminal Types */
#define UVC_ITT_VENDOR_SPECIFIC			0x0200
#define UVC_ITT_CAMERA					0x0201
#define UVC_ITT_MEDIA_TRANSPORT_INPUT	0x0202

/* B.3. Output Terminal Types */
#define UVC_OTT_VENDOR_SPECIFIC			0x0300
#define UVC_OTT_DISPLAY					0x0301
#define UVC_OTT_MEDIA_TRANSPORT_OUTPUT	0x0302

/* B.4. External Terminal Types */
#define UVC_EXTERNAL_VENDOR_SPECIFIC		0x0400
#define UVC_COMPOSITE_CONNECTOR				0x0401
#define UVC_SVIDEO_CONNECTOR				0x0402
#define UVC_COMPONENT_CONNECTOR				0x0403

/* 2.4.2.2. Status Packet Type */
#define UVC_STATUS_TYPE_CONTROL				1
#define UVC_STATUS_TYPE_STREAMING			2

#define UVC_STATUS_ATTRIBUTE_CONTROL_VALUE_CHANGE 				0
#define UVC_STATUS_ATTRIBUTE_CONTROL_INFO_CHANGE 				1
#define UVC_STATUS_ATTRIBUTE_CONTROL_FAILURE_CHANGE 			2
#define UVC_STATUS_ATTRIBUTE_CONTROL_MIN_CHANGE 				3
#define UVC_STATUS_ATTRIBUTE_CONTROL_MAX_CHANGE  				4

#define UVC_STATUS_MAX_PACKET_SIZE		64	
/*2.4.2.2  Status Interrupt Endpoint */
struct status_packet_format_video_streaming_s{
	__u8  bStatusType;
	__u8  bOriginator ;
	
	__u8  bEvent;
	__u8  bValue[UVC_STATUS_MAX_PACKET_SIZE - 3];
} __attribute__ ((packed));

struct status_packet_format_video_control_s{
	__u8  bStatusType;
	__u8  bOriginator ;
	
	__u8  bEvent;
	__u8  bSelector;
	__u8  bAttribute;
	__u8  bValue[UVC_STATUS_MAX_PACKET_SIZE - 5];
} __attribute__ ((packed));

/* 2.4.3.3. Payload Header Information */
#define UVC_STREAM_EOH					(1 << 7)
#define UVC_STREAM_ERR					(1 << 6)
#define UVC_STREAM_STI					(1 << 5)
#define UVC_STREAM_RES					(1 << 4)
#define UVC_STREAM_SCR					(1 << 3)
#define UVC_STREAM_PTS					(1 << 2)
#define UVC_STREAM_EOF					(1 << 1)
#define UVC_STREAM_FID					(1 << 0)

/* 4.1.2. Control Capabilities */
#define UVC_CONTROL_CAP_GET				(1 << 0)
#define UVC_CONTROL_CAP_SET				(1 << 1)
#define UVC_CONTROL_CAP_DISABLED		(1 << 2)
#define UVC_CONTROL_CAP_AUTOUPDATE		(1 << 3)
#define UVC_CONTROL_CAP_ASYNCHRONOUS	(1 << 4)
#define UVC_CONTROL_CAP_GET_SET			(UVC_CONTROL_CAP_GET | UVC_CONTROL_CAP_SET)


/*4.2.1.1  Power Mode Control */
#define DEVICE_POWER_MODE_FULL_POWER 					0
#define DEVICE_POWER_MODE_DEVICE_DEPENDENT_POWER		1

/*4.2.1.2  Request Error Code Control :
0x00: No error 
0x01: Not ready 
0x02: Wrong state 
0x03: Power 
0x04: Out of range 
0x05: Invalid unit 
0x06: Invalid control 
0x07: Invalid Request 
0x08: Invalid value within range 
0x09-0xFE: Reserved for future use 
0xFF: Unknown 
*/
typedef enum UvcRequestErrorCode_t
{
	UVC_REQUEST_ERROR_CODE_NO_ERROR = 0,
	UVC_REQUEST_ERROR_CODE_NOT_READY = 1,
	UVC_REQUEST_ERROR_CODE_WRONG_STATE = 2,
	UVC_REQUEST_ERROR_CODE_POWER = 3,
	UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE = 4,
	UVC_REQUEST_ERROR_CODE_INVALID_UNIT = 5,
	UVC_REQUEST_ERROR_CODE_INVALID_CONTROL = 6,
	UVC_REQUEST_ERROR_CODE_INVALID_REQUEST = 7,
	UVC_REQUEST_ERROR_CODE_INVALID_VALUE_WITHIN_RANGE = 8,
	UVC_REQUEST_ERROR_CODE_RESERVED_FOR_FUTURE_USE = 9, 
	
	UVC_REQUEST_ERROR_CODE_UNKNOWN = 0xFF,	
}UvcRequestErrorCode_t;

/* ------------------------------------------------------------------------
 * UVC structures
 */

/* All UVC descriptors have these 3 fields at the beginning */
struct uvc_descriptor_header {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
} __attribute__((packed));

/* 3.7.2. Video Control Interface Header Descriptor */
struct uvc_header_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u16 bcdUVC;
	__u16 wTotalLength;
	__u32 dwClockFrequency;
	__u8  bInCollection;
	__u8  baInterfaceNr[];
} __attribute__((__packed__));

#define UVC_DT_HEADER_SIZE(n)				(12+(n))

#define UVC_HEADER_DESCRIPTOR(n) \
	uvc_header_descriptor_##n

#define DECLARE_UVC_HEADER_DESCRIPTOR(n)		\
struct UVC_HEADER_DESCRIPTOR(n) {			\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u16 bcdUVC;					\
	__u16 wTotalLength;				\
	__u32 dwClockFrequency;				\
	__u8  bInCollection;				\
	__u8  baInterfaceNr[n];				\
} __attribute__ ((packed))

/* 3.7.2.1. Input Terminal Descriptor */
struct uvc_input_terminal_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bTerminalID;
	__u16 wTerminalType;
	__u8  bAssocTerminal;
	__u8  iTerminal;
} __attribute__((__packed__));

#define UVC_DT_INPUT_TERMINAL_SIZE			8

/* 3.7.2.2. Output Terminal Descriptor */
struct uvc_output_terminal_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bTerminalID;
	__u16 wTerminalType;
	__u8  bAssocTerminal;
	__u8  bSourceID;
	__u8  iTerminal;
} __attribute__((__packed__));

#define UVC_DT_OUTPUT_TERMINAL_SIZE			9

#if 0
/* 3.7.2.3. Camera Terminal Descriptor */
struct uvc_camera_terminal_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bTerminalID;
	__u16 wTerminalType;
	__u8  bAssocTerminal;
	__u8  iTerminal;
	__u16 wObjectiveFocalLengthMin;
	__u16 wObjectiveFocalLengthMax;
	__u16 wOcularFocalLength;
	__u8  bControlSize;
	__u8  bmControls[3];
} __attribute__((__packed__));

#define UVC_DT_CAMERA_TERMINAL_SIZE(n)			(15+(n))


/* 3.7.2.5. Processing Unit Descriptor */
struct uvc_processing_unit_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  bSourceID;
	__u16 wMaxMultiplier;
	__u8  bControlSize;
	__u8  bmControls[2];
	__u8  iProcessing;
	__u8  bmVideoStandards;
} __attribute__((__packed__));

#define UVC_DT_PROCESSING_UNIT_SIZE(n)			(10+(n))

#else

typedef union uvc_camera_terminal_bmControls {
		/** raw register data */
	__u8 d24[3];//24λ
		/** register bits */
	struct {//��λ��ǰ[0]...[32]
		unsigned scanning_mode:1;//����Ϊ1
		unsigned ae_mode:1;
		unsigned ae_priority:1;
		unsigned exposure_time_absolute:1;
		unsigned exposure_time_relative:1;
		unsigned focus_absolute:1;
		unsigned focus_relative:1;
		unsigned iris_absolute:1;
		unsigned iris_relative:1;
		unsigned zoom_absolute:1;
		unsigned zoom_relative:1;
		unsigned pantilt_absolute:1;
		unsigned pantilt_relative:1;
		unsigned roll_absolute:1;
		unsigned roll_relative:1;
		unsigned reserved1:1;//set to zero 
		unsigned reserved2:1;//set to zero 
		unsigned focus_auto:1;
		unsigned privacy:1;	
		unsigned focus_simple:1;
		unsigned window:1;	
		unsigned region_of_interest:1;		
		unsigned reserved3:2;//set to zero
	} b;
} uvc_camera_terminal_bmControls_t;

/* 3.7.2.3. Camera Terminal Descriptor */
struct uvc_camera_terminal_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bTerminalID;
	__u16 wTerminalType;
	__u8  bAssocTerminal;
	__u8  iTerminal;
	__u16 wObjectiveFocalLengthMin;
	__u16 wObjectiveFocalLengthMax;
	__u16 wOcularFocalLength;
	__u8  bControlSize;
	uvc_camera_terminal_bmControls_t bmControls;
} __attribute__((__packed__));

#define UVC_DT_CAMERA_TERMINAL_SIZE			(15+(3))

typedef union uvc_processing_unit_bmControls {
		/** raw register data */
	__u16 d16;//16λ
		/** register bits */
	struct {//��λ��ǰ[0]...[32]
		unsigned brightness:1;//����Ϊ1
		unsigned contrast:1;
		unsigned hue:1;
		unsigned saturation:1;
		unsigned sharpness:1;
		unsigned gamma:1;
		unsigned white_balance_temperature:1;
		unsigned white_balance_component:1;
		unsigned backlight_compensation:1;
		unsigned gain:1;
		unsigned power_line_frequency :1;
		unsigned hue_auto :1;
		unsigned white_balance_temperature_auto:1;
		unsigned white_balance_component_auto:1;
		unsigned digital_multiplier:1;
		unsigned digital_multiplier_limit:1;
		//unsigned analog_video_standard :1; 
		//unsigned analog_video_lock_status :1;
		//unsigned contrast_auto  :1; 
		//unsigned reserved:13;//set to zero 
	} b;
} uvc_processing_unit_bmControls_t;


/* 3.7.2.5. Processing Unit Descriptor */
struct uvc_processing_unit_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  bSourceID;
	__u16 wMaxMultiplier;
	__u8  bControlSize;
	uvc_processing_unit_bmControls_t bmControls;
	__u8  iProcessing;
	__u8  bmVideoStandards;//UVC 1.0 no support
} __attribute__((__packed__));

#define UVC_DT_PROCESSING_UNIT_SIZE				(10+(2))
#define UVC100_DT_PROCESSING_UNIT_SIZE			(9+(2))

#endif

/* 3.7.2.4. Selector Unit Descriptor */
struct uvc_selector_unit_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  bNrInPins;
	__u8  baSourceID[0];
	__u8  iSelector;
} __attribute__((__packed__));

#define UVC_DT_SELECTOR_UNIT_SIZE(n)			(6+(n))

#define UVC_SELECTOR_UNIT_DESCRIPTOR(n)	\
	uvc_selector_unit_descriptor_##n

#define DECLARE_UVC_SELECTOR_UNIT_DESCRIPTOR(n)	\
struct UVC_SELECTOR_UNIT_DESCRIPTOR(n) {		\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bUnitID;					\
	__u8  bNrInPins;				\
	__u8  baSourceID[n];				\
	__u8  iSelector;				\
} __attribute__ ((packed))

//UVC1.1 XU H264
typedef union xu_h264_bmControls {
		/** raw register data */
	__u16 d16;//16λ
		/** register bits */
	struct {//��λ��ǰ[0]...[16]
		unsigned uvcx_video_config_probe:1;//����Ϊ1
		unsigned uvcx_video_config_commit:1;
		unsigned uvcx_rate_control_mode:1;
		unsigned uvcx_temporal_scale_mode:1;
		unsigned uvcx_spatial_scale_mode:1;
		unsigned uvcx_snr_scale_mode:1;
		unsigned uvcx_ltr_buffer_size_control:1;
		unsigned uvcx_ltr_picture_control:1;
		unsigned uvcx_picture_type_control:1;
		unsigned uvcx_version:1;
		unsigned uvcx_encoder_reset:1;
		unsigned uvcx_framerate_config:1;
		unsigned uvcx_video_advance_config:1;
		unsigned uvcx_bitrate_layers:1;
		unsigned uvcx_qp_steps_layers:1;
		unsigned uvcx_video_undefined:1;
	} b;
} xu_h264_bmControls_t;


struct uvc_extension_unit_h264_descriptor_t {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  guidExtensionCode[16];
	__u8  bNumControls;
	__u8  bNrInPins;
	__u8  baSourceID;
	__u8  bControlSize;
	xu_h264_bmControls_t  bmControls;
	__u8  iExtension;
} __attribute__ ((packed));


typedef union uvc_xu_camera_terminal_bmControls {
		/** raw register data */
	__u8 d24[3];//24λ
		/** register bits */
	struct {//��λ��ǰ[0]...[32]
		unsigned scanning_mode:1;//����Ϊ1
		unsigned ae_mode:1;
		unsigned ae_priority:1;
		unsigned exposure_time_absolute:1;
		unsigned exposure_time_relative:1;
		unsigned focus_absolute:1;
		unsigned focus_relative:1;
		unsigned focus_auto:1;
		unsigned iris_absolute:1;
		unsigned iris_relative:1;
		unsigned zoom_absolute:1;
		unsigned zoom_relative:1;
		unsigned pantilt_absolute:1;
		unsigned pantilt_relative:1;
		unsigned roll_absolute:1;
		unsigned roll_relative:1;
		unsigned reserved1:1;//set to zero 
		unsigned reserved2:1;//set to zero 
		
		unsigned privacy:1;	
		unsigned focus_simple:1;
		unsigned window:1;	
		unsigned region_of_interest:1;		
		unsigned reserved3:2;//set to zero
	} b;
} uvc_xu_camera_terminal_bmControls_t;

struct uvc_extension_unit_camera_terminal_descriptor_t {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  guidExtensionCode[16];
	__u8  bNumControls;
	__u8  bNrInPins;
	__u8  baSourceID;
	__u8  bControlSize;
	uvc_xu_camera_terminal_bmControls_t  bmControls;
	__u8  iExtension;
} __attribute__ ((packed));

/* uvc1.5 3.7.2.6  Encoding Unit Descriptor  */
#define UVC_VC_ENCODING_UNIT				0x07
struct uvc_encoding_unit_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  bSourceID;
	__u8  iEncoding;
	__u8  bControlSize;
	__u8  bmControls[3];
	__u8  bmControlsRuntime[3];
} __attribute__((__packed__));

#define UVC_DT_ENCODING_UNIT_SIZE			(0x0D)

/* 3.7.2.6. Extension Unit Descriptor */
struct uvc_extension_unit_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bUnitID;
	__u8  guidExtensionCode[16];
	__u8  bNumControls;
	__u8  bNrInPins;
	__u8  baSourceID[0];
	__u8  bControlSize;
	__u8  bmControls[0];
	__u8  iExtension;
} __attribute__((__packed__));

#define UVC_DT_EXTENSION_UNIT_SIZE(p, n)		(24+(p)+(n))

#define UVC_EXTENSION_UNIT_DESCRIPTOR(p, n) \
	uvc_extension_unit_descriptor_##p_##n

#define DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(p, n)	\
struct UVC_EXTENSION_UNIT_DESCRIPTOR(p, n) {		\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bUnitID;					\
	__u8  guidExtensionCode[16];			\
	__u8  bNumControls;				\
	__u8  bNrInPins;				\
	__u8  baSourceID[p];				\
	__u8  bControlSize;				\
	__u8  bmControls[n];				\
	__u8  iExtension;				\
} __attribute__ ((packed))


/* 3.8.2.2. Video Control Interrupt Endpoint Descriptor */
struct uvc_control_endpoint_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u16 wMaxTransferSize;
} __attribute__((__packed__));

#define UVC_DT_CONTROL_ENDPOINT_SIZE			5

/* 3.9.2.1. Input Header Descriptor */
struct uvc_input_header_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bNumFormats;
	__u16 wTotalLength;
	__u8  bEndpointAddress;
	__u8  bmInfo;
	__u8  bTerminalLink;
	__u8  bStillCaptureMethod;
	__u8  bTriggerSupport;
	__u8  bTriggerUsage;
	__u8  bControlSize;
	__u8  bmaControls[];
} __attribute__((__packed__));

#define UVC_DT_INPUT_HEADER_SIZE(n, p)			(13+(n*p))

#define UVC_INPUT_HEADER_DESCRIPTOR(n, p) \
	uvc_input_header_descriptor_##n_##p

#define DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(n, p)	\
struct UVC_INPUT_HEADER_DESCRIPTOR(n, p) {		\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bNumFormats;				\
	__u16 wTotalLength;				\
	__u8  bEndpointAddress;				\
	__u8  bmInfo;					\
	__u8  bTerminalLink;				\
	__u8  bStillCaptureMethod;			\
	__u8  bTriggerSupport;				\
	__u8  bTriggerUsage;				\
	__u8  bControlSize;				\
	__u8  bmaControls[p][n];			\
} __attribute__ ((packed))

/* 3.9.2.2. Output Header Descriptor */
struct uvc_output_header_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bNumFormats;
	__u16 wTotalLength;
	__u8  bEndpointAddress;
	__u8  bTerminalLink;
	__u8  bControlSize;
	__u8  bmaControls[];
} __attribute__((__packed__));

#define UVC_DT_OUTPUT_HEADER_SIZE(n, p)			(9+(n*p))

#define UVC_OUTPUT_HEADER_DESCRIPTOR(n, p) \
	uvc_output_header_descriptor_##n_##p

#define DECLARE_UVC_OUTPUT_HEADER_DESCRIPTOR(n, p)	\
struct UVC_OUTPUT_HEADER_DESCRIPTOR(n, p) {		\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bNumFormats;				\
	__u16 wTotalLength;				\
	__u8  bEndpointAddress;				\
	__u8  bTerminalLink;				\
	__u8  bControlSize;				\
	__u8  bmaControls[p][n];			\
} __attribute__ ((packed))

/* 3.9.2.6. Color matching descriptor */
struct uvc_color_matching_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bColorPrimaries;
	__u8  bTransferCharacteristics;
	__u8  bMatrixCoefficients;
} __attribute__((__packed__));

#define UVC_DT_COLOR_MATCHING_SIZE			6

/* 4.3.1.1. Video Probe and Commit Controls */
struct uvc_streaming_control {
	__u16 bmHint;
	__u8  bFormatIndex;
	__u8  bFrameIndex;
	__u32 dwFrameInterval;
	__u16 wKeyFrameRate;
	__u16 wPFrameRate;
	__u16 wCompQuality;
	__u16 wCompWindowSize;
	__u16 wDelay;
	__u32 dwMaxVideoFrameSize;
	__u32 dwMaxPayloadTransferSize;
	__u32 dwClockFrequency;
	__u8  bmFramingInfo;
	__u8  bPreferedVersion;
	__u8  bMinVersion;
	__u8  bMaxVersion;
	//UVC1.5
        __u8    bUsage;
        __u8    bBitDepthLuma;
        __u8    bmSettings;
        __u8    bMaxNumberOfRefFramesPlus1;
        __u16   bmRateControlModes;
        __u32   bmLayoutPerStream[2];
} __attribute__((__packed__));

/* Uncompressed Payload - 3.1.1. Uncompressed Video Format Descriptor */
struct uvc_format_uncompressed {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
	__u8  guidFormat[16];
	__u8  bBitsPerPixel;
	__u8  bDefaultFrameIndex;
	__u8  bAspectRatioX;
	__u8  bAspectRatioY;
	__u8  bmInterfaceFlags;
	__u8  bCopyProtect;
} __attribute__((__packed__));

#define UVC_DT_FORMAT_UNCOMPRESSED_SIZE			27

/* Uncompressed Payload - 3.1.2. Uncompressed Video Frame Descriptor */
struct uvc_frame_uncompressed {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
	__u32 dwMinBitRate;
	__u32 dwMaxBitRate;
	__u32 dwMaxVideoFrameBufferSize;
	__u32 dwDefaultFrameInterval;
	__u8  bFrameIntervalType;
	__u32 dwFrameInterval[];
} __attribute__((__packed__));

#define UVC_DT_FRAME_UNCOMPRESSED_SIZE(n)		(26+4*(n))

#define UVC_FRAME_UNCOMPRESSED(n) \
	uvc_frame_uncompressed_##n

#define DECLARE_UVC_FRAME_UNCOMPRESSED(n)		\
struct UVC_FRAME_UNCOMPRESSED(n) {			\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bFrameIndex;				\
	__u8  bmCapabilities;				\
	__u16 wWidth;					\
	__u16 wHeight;					\
	__u32 dwMinBitRate;				\
	__u32 dwMaxBitRate;				\
	__u32 dwMaxVideoFrameBufferSize;		\
	__u32 dwDefaultFrameInterval;			\
	__u8  bFrameIntervalType;			\
	__u32 dwFrameInterval[n];			\
} __attribute__ ((packed))

/* MJPEG Payload - 3.1.1. MJPEG Video Format Descriptor */
struct uvc_format_mjpeg {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
	__u8  bmFlags;
	__u8  bDefaultFrameIndex;
	__u8  bAspectRatioX;
	__u8  bAspectRatioY;
	__u8  bmInterfaceFlags;
	__u8  bCopyProtect;
} __attribute__((__packed__));

#define UVC_DT_FORMAT_MJPEG_SIZE			11
#define UVC_DT_FORMAT_H264_SIZE			11

struct uvc_format_h264 {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
	__u8  bmFlags;
	__u8  bDefaultFrameIndex;
	__u8  bAspectRatioX;
	__u8  bAspectRatioY;
	__u8  bmInterfaceFlags;
	__u8  bCopyProtect;
} __attribute__((__packed__));



struct uvc_format_h264_base {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
	__u8  guidFormat[16];
	__u8  bBitsPerPixel;
	__u8  bDefaultFrameIndex;
	__u8  bAspectRatioX;
	__u8  bAspectRatioY;
	__u8  bmInterfaceFlags;
	__u8  bCopyProtect;
	__u8  bVariableSize;
} __attribute__((__packed__));

#define UVC_DT_FORMAT_H264_BASE_SIZE	28


/* MJPEG Payload - 3.1.2. MJPEG Video Frame Descriptor */
struct uvc_frame_mjpeg {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
	__u32 dwMinBitRate;
	__u32 dwMaxBitRate;
	__u32 dwMaxVideoFrameBufferSize;
	__u32 dwDefaultFrameInterval;
	__u8  bFrameIntervalType;
	__u32 dwFrameInterval[];
} __attribute__((__packed__));

#define UVC_DT_FRAME_MJPEG_SIZE(n)			(26+4*(n))

#define UVC_FRAME_MJPEG(n) \
	uvc_frame_mjpeg_##n

#define DECLARE_UVC_FRAME_MJPEG(n)			\
struct UVC_FRAME_MJPEG(n) {				\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bFrameIndex;				\
	__u8  bmCapabilities;				\
	__u16 wWidth;					\
	__u16 wHeight;					\
	__u32 dwMinBitRate;				\
	__u32 dwMaxBitRate;				\
	__u32 dwMaxVideoFrameBufferSize;		\
	__u32 dwDefaultFrameInterval;			\
	__u8  bFrameIntervalType;			\
	__u32 dwFrameInterval[n];			\
} __attribute__ ((packed))

#define UVC_DT_FRAME_H264_SIZE(n)			(26+4*(n))

#define UVC_FRAME_H264(n) \
	uvc_frame_h264_##n

#define UVC_FRAME_H264_BASE(n) \
	uvc_frame_h264_base##n

#define DECLARE_UVC_FRAME_H264(n)			\
struct UVC_FRAME_H264(n) {				\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bFrameIndex;				\
	__u8  bmCapabilities;				\
	__u16 wWidth;					\
	__u16 wHeight;					\
	__u32 dwMinBitRate;				\
	__u32 dwMaxBitRate;				\
	__u32 dwMaxVideoFrameBufferSize;		\
	__u32 dwDefaultFrameInterval;			\
	__u8  bFrameIntervalType;			\
	__u32 dwFrameInterval[n];			\
} __attribute__ ((packed))

#define DECLARE_UVC_FRAME_H264_BASE(n)			\
struct UVC_FRAME_H264_BASE(n) {				\
	__u8  bLength;					\
	__u8  bDescriptorType;				\
	__u8  bDescriptorSubType;			\
	__u8  bFrameIndex;				\
	__u8  bmCapabilities;				\
	__u16 wWidth;					\
	__u16 wHeight;					\
	__u32 dwMinBitRate;				\
	__u32 dwMaxBitRate;				\
	__u32 dwDefaultFrameInterval;			\
	__u8  bFrameIntervalType;			\
	__u32 dwBytesPerLine;			\
	__u32 dwFrameInterval[n];			\
} __attribute__ ((packed))

struct uvc_format_h264_v150 {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bFormatIndex;
        __u8  bNumFrameDescriptors;
        __u8  bDefaultFrameIndex;
        __u8  bMaxCodecConfigDelay ;
        __u8  bmSupportedSliceModes;
        __u8  bmSupportedSyncFrameTypes;
        __u8  bResolutionScaling;
        __u8  Reserved1;
        __u8  bmSupportedRateControlModes;
        __u16 wMaxMBperSecOneResolutionNoScalability;
        __u16 wMaxMBperSecTwoResolutionsNoScalability;
        __u16 wMaxMBperSecThreeResolutionsNoScalability;
        __u16 wMaxMBperSecFourResolutionsNoScalability;
        __u16 wMaxMBperSecOneResolutionTemporalScalabilit;
        __u16 wMaxMBperSecTwoResolutionsTemporalScalablility;
        __u16 wMaxMBperSecThreeResolutionsTemporalScalability;
        __u16 wMaxMBperSecFourResolutionsTemporalScalability;
        __u16 wMaxMBperSecOneResolutionTemporalQualityScalability;
        __u16 wMaxMBperSecTwoResolutionsTemporalQualityScalability;
        __u16 wMaxMBperSecThreeResolutionsTemporalQualityScalablity;
        __u16 wMaxMBperSecFourResolutionsTemporalQualityScalability;
        __u16 wMaxMBperSecOneResolutionsTemporalSpatialScalability;
        __u16 wMaxMBperSecTwoResolutionsTemporalSpatialScalability;
        __u16 wMaxMBperSecThreeResolutionsTemporalSpatialScalability;
        __u16 wMaxMBperSecFourResolutionsTemporalSpatialScalability;
        __u16 wMaxMBperSecOneResolutionFullScalability;
        __u16 wMaxMBperSecTwoResolutionsFullScalability;
        __u16 wMaxMBperSecThreeResolutionsFullScalability;
        __u16 wMaxMBperSecFourResolutionsFullScalability;
} __attribute__((__packed__));

/* H264_V150 Payload - 3.1.2. H264_V150 Video Frame Descriptor */
struct uvc_frame_h264_v150 {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bFrameIndex;
        __u16 wWidth;
        __u16 wHeight;
        __u16 wSARwidth;
        __u16 wSARheight;
        __u16 wProfile;
        __u8  bLevelIDC;
        __u16 wConstrainedToolset;
        __u32 bmSupportedUsages;
        __u16 bmCapabilities;
        __u32 bmSVCCapabilities;
        __u32 bmMVCCapabilities;
        __u32 dwMinBitRate;
        __u32 dwMaxBitRate;
        __u32 dwDefaultFrameInterval;
        __u8  bNumFrameIntervals;
        __u32 dwFrameInterval[];
} __attribute__((__packed__));


#define UVC_DT_FORMAT_H264_V150_SIZE                    52


#define UVC_DT_FRAME_H264_V150_SIZE(n)                  (44+4*(n))

#define UVC_FRAME_H264_V150(n) \
        uvc_frame_h264_v150_##n

#define DECLARE_UVC_FRAME_H264_V150(n)                  \
struct UVC_FRAME_H264_V150(n) {                         \
        __u8  bLength;                          \
        __u8  bDescriptorType;          \
        __u8  bDescriptorSubType;       \
        __u8  bFrameIndex;                      \
        __u16 wWidth;                           \
        __u16 wHeight;                          \
        __u16 wSARwidth;                        \
        __u16 wSARheight;                       \
        __u16 wProfile;                         \
        __u8  bLevelIDC;                        \
        __u16 wConstrainedToolset;      \
        __u32 bmSupportedUsages;        \
        __u16 bmCapabilities;           \
        __u32 bmSVCCapabilities;        \
        __u32 bmMVCCapabilities;        \
        __u32 dwMinBitRate;                     \
        __u32 dwMaxBitRate;                     \
        __u32 dwDefaultFrameInterval;   \
        __u8  bNumFrameIntervals;       \
        __u32 dwFrameInterval[n];       \
} __attribute__ ((packed))

#endif /* __LINUX_USB_VIDEO_H */

