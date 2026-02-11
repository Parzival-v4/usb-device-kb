/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-15 19:32:45
 * @FilePath    : \lib_ucam\ucam\src\uvc\webcam.c
 * @Description : 
 */ 
#include <linux/usb/ch9.h>
//#include <linux/usb/gadget.h>

#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_ctrl_vs.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_config.h>

static uint16_t	g_limit_fps_width = 0;
static uint16_t	g_limit_fps_height = 0;
static uint32_t	g_limit_fps_max = 0;

static uint16_t g_def_width[3] = {0, 0, 0};//hs ss ssp
static uint16_t g_def_height[3] = {0, 0, 0};//hs ss ssp
static uint32_t g_def_fps[3] = {0, 0, 0};//hs ss ssp

struct uvc_fps_list_t {
	uint8_t bypass;
	uint32_t fps;
	uint32_t interval;
};

#define UVC_FPS_LIST_MAX 10

//fps一定要从到小排列，目前最大个数是10，超出了要同步修改很多
static struct uvc_fps_list_t g_uvc_fps_list[UVC_FPS_LIST_MAX] = {
	{0, 60, UVC_FORMAT_60FPS}, 
	{0, 50, UVC_FORMAT_50FPS}, 
	{0, 30, UVC_FORMAT_30FPS}, 
	{0, 25, UVC_FORMAT_25FPS},
	{0, 24, UVC_FORMAT_24FPS}, 
	{0, 20, UVC_FORMAT_20FPS},
	{0, 15, UVC_FORMAT_15FPS},  
	{0, 10, UVC_FORMAT_10FPS},
	{0, 7, UVC_FORMAT_7FPS},
	{0, 5, UVC_FORMAT_5FPS}
};

//UVC_HEADER
DECLARE_UVC_HEADER_DESCRIPTOR(1);
DECLARE_UVC_HEADER_DESCRIPTOR(2);

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 2);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 3);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 4);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 5);


//YUY2
DECLARE_UVC_FRAME_UNCOMPRESSED(1);
DECLARE_UVC_FRAME_UNCOMPRESSED(2);
DECLARE_UVC_FRAME_UNCOMPRESSED(3);
DECLARE_UVC_FRAME_UNCOMPRESSED(4);
DECLARE_UVC_FRAME_UNCOMPRESSED(5);
DECLARE_UVC_FRAME_UNCOMPRESSED(6);
DECLARE_UVC_FRAME_UNCOMPRESSED(7);
DECLARE_UVC_FRAME_UNCOMPRESSED(8);
DECLARE_UVC_FRAME_UNCOMPRESSED(9);
DECLARE_UVC_FRAME_UNCOMPRESSED(10);

//MJPEG
DECLARE_UVC_FRAME_MJPEG(1);
DECLARE_UVC_FRAME_MJPEG(2);
DECLARE_UVC_FRAME_MJPEG(3);
DECLARE_UVC_FRAME_MJPEG(4);
DECLARE_UVC_FRAME_MJPEG(8);
DECLARE_UVC_FRAME_MJPEG(9);
DECLARE_UVC_FRAME_MJPEG(10);

//H264 UVC1.1
DECLARE_UVC_FRAME_H264_BASE(1);
DECLARE_UVC_FRAME_H264_BASE(2);
DECLARE_UVC_FRAME_H264_BASE(3);
DECLARE_UVC_FRAME_H264_BASE(4);
DECLARE_UVC_FRAME_H264_BASE(8);
DECLARE_UVC_FRAME_H264_BASE(9);
DECLARE_UVC_FRAME_H264_BASE(10);

//H264 UVC1.5
DECLARE_UVC_FRAME_H264_V150(1);
DECLARE_UVC_FRAME_H264_V150(2);
DECLARE_UVC_FRAME_H264_V150(3);
DECLARE_UVC_FRAME_H264_V150(4);
DECLARE_UVC_FRAME_H264_V150(8);
DECLARE_UVC_FRAME_H264_V150(9);
DECLARE_UVC_FRAME_H264_V150(10);


#define DECLARE_UVC_FRAME_YUY2_60FPS(w,h,s) \
static struct UVC_FRAME_UNCOMPRESSED(10) uvc_frame_yuv_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(10), \
	.bDescriptorType	= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED, \
	/*.bFrameIndex		= UVC_YUV_FORMAT_INDEX_##w##x##h, */\
	.bmCapabilities		= 0,\
	.wWidth				= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/ \
	.dwMaxBitRate		= (w*h*2*60*8U),/*Width x Height x 2 x 30 x 8*/ \
	.dwMaxVideoFrameBufferSize	= (w*h*2),/*Width x Height x 2*/ \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType	= (10),\
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_NV12_60FPS(w,h,s) \
static struct UVC_FRAME_UNCOMPRESSED(10) uvc_frame_nv12_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(10), \
	.bDescriptorType	= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED, \
	/*.bFrameIndex		= UVC_NV12_FORMAT_INDEX_##w##x##h, */\
	.bmCapabilities		= 0, \
	.wWidth				= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/\
	.dwMaxBitRate		= (w*h*2*60*8U),/*Width x Height x 2 x 30 x 8*/ \
	.dwMaxVideoFrameBufferSize	= (w*h*3/2),/*Width x Height x 3/2 */ \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType	= (10),\
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_I420_60FPS(w,h,s) \
static struct UVC_FRAME_UNCOMPRESSED(10) uvc_frame_i420_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(10), \
	.bDescriptorType	= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED, \
	/*.bFrameIndex		= UVC_I420_FORMAT_INDEX_##w##x##h, */\
	.bmCapabilities		= 0, \
	.wWidth				= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/\
	.dwMaxBitRate		= (w*h*2*60*8U),/*Width x Height x 2 x 30 x 8*/ \
	.dwMaxVideoFrameBufferSize	= (w*h*3/2),/*Width x Height x 3/2 */ \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType	= (10),\
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_MJPEG_60FPS(w,h,s) \
static struct UVC_FRAME_MJPEG(10) uvc_frame_mjpg_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(10),\
	.bDescriptorType	= USB_DT_CS_INTERFACE,\
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,\
	/*.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_##w##x##h,*/\
	.bmCapabilities 	= 0,\
	.wWidth 			= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/ \
	.dwMaxBitRate		= (w*h*2*60*8U),/*Width x Height x 2 x 30 x 8*/\
	.dwMaxVideoFrameBufferSize	= (w*h*2),/*Width x Height x 2*/\
	.dwDefaultFrameInterval = (UVC_FORMAT_30FPS),\
	.bFrameIntervalType = (10),\
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_H264_BASE_60FPS(w,h,s) \
static struct UVC_FRAME_H264_BASE(10) uvc110_frame_h264_##w##x##h##s = {\
	.bLength				= UVC_DT_FRAME_H264_SIZE(10),\
	.bDescriptorType		= USB_DT_CS_INTERFACE,\
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,\
	/*.bFrameIndex			= UVC110_H264_FORMAT_INDEX_##w##x##h,*/\
	.bmCapabilities			= 0,\
	.wWidth					= (w),\
	.wHeight				= (h),\
	.dwMinBitRate			= (w*h*2*5*8U),\
	.dwMaxBitRate			= (w*h*2*60*8U),\
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType		= (10),\
	.dwBytesPerLine         = 0,\
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC150_FRAME_H264_60FPS(w,h,s) \
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_##w##x##h##s = { \
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10), \
	.bDescriptorType		= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150, \
	.bFrameIndex			= 0, \
	.wWidth					= (w), \
	.wHeight				= (h), \
	.wSARwidth 				= 0x0001, \
	.wSARheight 			= 0x0001, \
	.wProfile				= 0x4240, \
	.bLevelIDC				= 0x28, \
	.wConstrainedToolset	= 0x0000, \
	.bmSupportedUsages		= 0x00000003, \
	.bmCapabilities			= 0x0021, \
	.bmSVCCapabilities		= SVC_MAX_LAYERS, \
	.bmMVCCapabilities		= 0x00000000, \
	.dwMinBitRate			= (256000), \
	.dwMaxBitRate			= (12000000), \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS), \
	.bNumFrameIntervals		= (10), \
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC150_FRAME2_H264_60FPS(w,h,s) \
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_##w##x##h##s = { \
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10), \
	.bDescriptorType		= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150, \
	.bFrameIndex			= 0, \
	.wWidth					= (w), \
	.wHeight				= (h), \
	.wSARheight 			= 0x0001, \
	.wProfile				= 0x640C, \
	.bLevelIDC				= 0x28, \
	.wConstrainedToolset	= 0x0000, \
	.bmSupportedUsages		= 0x00000003, \
	.bmCapabilities			= 0x0020, \
	.bmSVCCapabilities		= SVC_MAX_LAYERS, \
	.bmMVCCapabilities		= 0x00000000, \
	.dwMinBitRate			= (256000), \
	.dwMaxBitRate			= (12000000), \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS), \
	.bNumFrameIntervals		= (10), \
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}




#define DECLARE_UVC_FRAME_YUY2_30FPS(w,h,s) \
static struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_yuv_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8), \
	.bDescriptorType	= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED, \
	/*.bFrameIndex		= UVC_YUV_FORMAT_INDEX_##w##x##h, */\
	.bmCapabilities		= 0,\
	.wWidth				= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/ \
	.dwMaxBitRate		= (w*h*2*30*8U),/*Width x Height x 2 x 30 x 8*/ \
	.dwMaxVideoFrameBufferSize	= (w*h*2),/*Width x Height x 2*/ \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType	= (8),\
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_NV12_30FPS(w,h,s) \
static struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_nv12_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8), \
	.bDescriptorType	= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED, \
	/*.bFrameIndex		= UVC_NV12_FORMAT_INDEX_##w##x##h, */\
	.bmCapabilities		= 0, \
	.wWidth				= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/\
	.dwMaxBitRate		= (w*h*2*30*8U),/*Width x Height x 2 x 30 x 8*/ \
	.dwMaxVideoFrameBufferSize	= (w*h*3/2),/*Width x Height x 3/2 */ \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType	= (8),\
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_I420_30FPS(w,h,s) \
static struct UVC_FRAME_UNCOMPRESSED(8) uvc_frame_i420_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(8), \
	.bDescriptorType	= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED, \
	/*.bFrameIndex		= UVC_I420_FORMAT_INDEX_##w##x##h, */\
	.bmCapabilities		= 0, \
	.wWidth				= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/\
	.dwMaxBitRate		= (w*h*2*30*8U),/*Width x Height x 2 x 30 x 8*/ \
	.dwMaxVideoFrameBufferSize	= (w*h*3/2),/*Width x Height x 3/2 */ \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType	= (8),\
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_MJPEG_30FPS(w,h,s) \
static struct UVC_FRAME_MJPEG(8) uvc_frame_mjpg_##w##x##h##s = { \
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(8),\
	.bDescriptorType	= USB_DT_CS_INTERFACE,\
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,\
	/*.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_##w##x##h,*/\
	.bmCapabilities 	= 0,\
	.wWidth 			= (w),\
	.wHeight			= (h),\
	.dwMinBitRate		= (w*h*2*5*8U),/*Width x Height x 2 x 25 x 8*/ \
	.dwMaxBitRate		= (w*h*2*30*8U),/*Width x Height x 2 x 30 x 8*/\
	.dwMaxVideoFrameBufferSize	= (w*h*2),/*Width x Height x 2*/\
	.dwDefaultFrameInterval = (UVC_FORMAT_30FPS),\
	.bFrameIntervalType = (8),\
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC_FRAME_H264_BASE_30FPS(w,h,s) \
static struct UVC_FRAME_H264_BASE(8) uvc110_frame_h264_##w##x##h##s = {\
	.bLength				= UVC_DT_FRAME_H264_SIZE(8),\
	.bDescriptorType		= USB_DT_CS_INTERFACE,\
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,\
	/*.bFrameIndex			= UVC110_H264_FORMAT_INDEX_##w##x##h,*/\
	.bmCapabilities			= 0,\
	.wWidth					= (w),\
	.wHeight				= (h),\
	.dwMinBitRate			= (w*h*2*5*8U),\
	.dwMaxBitRate			= (w*h*2*30*8U),\
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),\
	.bFrameIntervalType		= (8),\
	.dwBytesPerLine         = 0,\
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC150_FRAME_H264_30FPS(w,h,s) \
static struct UVC_FRAME_H264_V150(8) uvc150_frame_h264_##w##x##h##s = { \
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(8), \
	.bDescriptorType		= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150, \
	.bFrameIndex			= 0, \
	.wWidth					= (w), \
	.wHeight				= (h), \
	.wSARwidth 				= 0x0001, \
	.wSARheight 			= 0x0001, \
	.wProfile				= 0x4240, \
	.bLevelIDC				= 0x28, \
	.wConstrainedToolset	= 0x0000, \
	.bmSupportedUsages		= 0x00000003, \
	.bmCapabilities			= 0x0021, \
	.bmSVCCapabilities		= SVC_MAX_LAYERS, \
	.bmMVCCapabilities		= 0x00000000, \
	.dwMinBitRate			= (256000), \
	.dwMaxBitRate			= (12000000), \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS), \
	.bNumFrameIntervals		= (8), \
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),\
}

#define DECLARE_UVC150_FRAME2_H264_60FPS(w,h,s) \
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_##w##x##h##s = { \
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10), \
	.bDescriptorType		= USB_DT_CS_INTERFACE, \
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150, \
	.bFrameIndex			= 0, \
	.wWidth					= (w), \
	.wHeight				= (h), \
	.wSARheight 			= 0x0001, \
	.wProfile				= 0x640C, \
	.bLevelIDC				= 0x28, \
	.wConstrainedToolset	= 0x0000, \
	.bmSupportedUsages		= 0x00000003, \
	.bmCapabilities			= 0x0020, \
	.bmSVCCapabilities		= SVC_MAX_LAYERS, \
	.bmMVCCapabilities		= 0x00000000, \
	.dwMinBitRate			= (256000), \
	.dwMaxBitRate			= (12000000), \
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS), \
	.bNumFrameIntervals		= (10), \
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),\
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),\
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),\
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),\
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),\
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),\
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),\
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),\
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),\
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),\
}


struct uvc_format_header {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
} __attribute__ ((packed));

struct uvc_frame_header {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
} __attribute__ ((packed));

struct uvc_frame_uncompressed_header {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
} __attribute__ ((packed));

struct uvc_frame_h264_v150_header {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u16 wWidth;
	__u16 wHeight;
} __attribute__ ((packed));




static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};


/* --------------------------------------------------------------------------
 * UVC1.1 configuration
 */

static struct UVC_HEADER_DESCRIPTOR(1) uvc_control_header = {
	.bLength			= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(UVC_CLOCK_FREQUENCY),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};


static struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength			= UVC_DT_CAMERA_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_INPUT_TERMINAL,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal			= 0,
	
#ifdef UVC_PTZ_CTRL_SUPPORT
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(16384),
	.wOcularFocalLength		= cpu_to_le16(20),
	.bControlSize		= 3,
	
	.bmControls.b.scanning_mode = UVC_CT_SCANNING_MODE_CONTROL_SUPPORTED,
	.bmControls.b.ae_mode = UVC_CT_AE_MODE_CONTROL_SUPPORTED,
	.bmControls.b.ae_priority = UVC_CT_AE_PRIORITY_CONTROL_SUPPORTED,
	.bmControls.b.exposure_time_absolute = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.exposure_time_relative = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.focus_absolute = UVC_CT_FOCUS_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.focus_relative = UVC_CT_FOCUS_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.iris_absolute = UVC_CT_IRIS_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.iris_relative = UVC_CT_IRIS_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.zoom_absolute = UVC_CT_ZOOM_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.zoom_relative = UVC_CT_ZOOM_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.pantilt_absolute = UVC_CT_PANTILT_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.pantilt_relative = UVC_CT_PANTILT_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.roll_absolute = UVC_CT_ROLL_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.roll_relative = UVC_CT_ROLL_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.reserved1 = 0,//set to zero 
	.bmControls.b.reserved2 = 0,//set to zero 
	.bmControls.b.focus_auto = UVC_CT_FOCUS_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.privacy = UVC_CT_PRIVACY_CONTROL_SUPPORTED,	
	.bmControls.b.focus_simple = 0,
	.bmControls.b.window = 0,	
	.bmControls.b.region_of_interest = 0,		
	.bmControls.b.reserved3 = 0,//set to zero
#else
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	
	.bmControls.d24[0] = 0,
	.bmControls.d24[1] = 0,
	.bmControls.d24[2] = 0,
#endif
};

static struct uvc_extension_unit_camera_terminal_descriptor_t uvc_xu_camera_terminal = {
	.bLength				= UVC_DT_EXTENSION_UNIT_SIZE(1,3),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VC_EXTENSION_UNIT,
	.bUnitID				= UVC_ENTITY_ID_XU_PTZ,
	.guidExtensionCode		= UVC_GUID_XUCONTROL_PTZ,
	.bNumControls			= 9,
	.bNrInPins				= 1,
	//.baSourceID				= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bControlSize			= 3,
	.iExtension				= 0,

#ifdef UVC_PTZ_CTRL_SUPPORT
	.bmControls.b.scanning_mode = UVC_CT_SCANNING_MODE_CONTROL_SUPPORTED,
	.bmControls.b.ae_mode = UVC_CT_AE_MODE_CONTROL_SUPPORTED,
	.bmControls.b.ae_priority = UVC_CT_AE_PRIORITY_CONTROL_SUPPORTED,
	.bmControls.b.exposure_time_absolute = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.exposure_time_relative = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.focus_absolute = UVC_CT_FOCUS_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.focus_relative = UVC_CT_FOCUS_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.iris_absolute = UVC_CT_IRIS_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.iris_relative = UVC_CT_IRIS_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.zoom_absolute = UVC_CT_ZOOM_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.zoom_relative = UVC_CT_ZOOM_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.pantilt_absolute = UVC_CT_PANTILT_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.pantilt_relative = UVC_CT_PANTILT_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.roll_absolute = UVC_CT_ROLL_ABSOLUTE_CONTROL_SUPPORTED,
	.bmControls.b.roll_relative = UVC_CT_ROLL_RELATIVE_CONTROL_SUPPORTED,
	.bmControls.b.reserved1 = 0,//set to zero 
	.bmControls.b.reserved2 = 0,//set to zero 
	.bmControls.b.focus_auto = UVC_CT_FOCUS_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.privacy = UVC_CT_PRIVACY_CONTROL_SUPPORTED,	
	.bmControls.b.focus_simple = 0,
	.bmControls.b.window = 0,	
	.bmControls.b.region_of_interest = 0,		
	.bmControls.b.reserved3 = 0,//set to zero
#else
	.bmControls.d24[0] = 0,
	.bmControls.d24[1] = 0,
	.bmControls.d24[2] = 0,
#endif
};

static struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	//.bUnitID		= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bSourceID		= UVC_ENTITY_ID_INPUT_TERMINAL,
	.wMaxMultiplier		= cpu_to_le16(0x4000),
	.bControlSize		= 2,
	
#ifdef UVC_PROCESS_CTRL_SUPPORT	
	.bmControls.b.brightness = UVC_PU_BRIGHTNESS_CONTROL_SUPPORTED,
	.bmControls.b.contrast = UVC_PU_CONTRAST_CONTROL_SUPPORTED,
	.bmControls.b.hue = UVC_PU_HUE_CONTROL_SUPPORTED,
	.bmControls.b.saturation = UVC_PU_SATURATION_CONTROL_SUPPORTED,
	.bmControls.b.sharpness = UVC_PU_SHARPNESS_CONTROL_SUPPORTED,
	.bmControls.b.gamma = UVC_PU_GAMMA_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_temperature = UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_component = UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL_SUPPORTED,
	.bmControls.b.backlight_compensation = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL_SUPPORTED,
	.bmControls.b.gain = UVC_PU_GAIN_CONTROL_SUPPORTED,
	.bmControls.b.power_line_frequency = UVC_PU_POWER_LINE_FREQUENCY_CONTROL_SUPPORTED,
	.bmControls.b.hue_auto = UVC_PU_HUE_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_temperature_auto = UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.white_balance_component_auto = UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL_SUPPORTED,
	.bmControls.b.digital_multiplier = 0,
	.bmControls.b.digital_multiplier_limit = 0,
#else
	.bmControls.d16 = 0,
#endif

	.iProcessing		= 0,
	.bmVideoStandards   = 0 //UVC 1.0 no support
};


static struct uvc_extension_unit_h264_descriptor_t uvc_xu_h264 = {
	.bLength				= UVC_DT_EXTENSION_UNIT_SIZE(1,2),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VC_EXTENSION_UNIT,
	.bUnitID				= UVC_ENTITY_ID_XU_H264,
	.guidExtensionCode		= UVC_GUID_XUCONTROL_H264,
	.bNumControls			= 0x07,
	.bNrInPins				= 1,
	//.baSourceID				= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bControlSize			= 2,

	.bmControls.b.uvcx_video_config_probe 		= 1,
	.bmControls.b.uvcx_video_config_commit 		= 1,
	.bmControls.b.uvcx_rate_control_mode 		= 1,
	.bmControls.b.uvcx_temporal_scale_mode 		= 0,
	.bmControls.b.uvcx_spatial_scale_mode 		= 0,
	.bmControls.b.uvcx_snr_scale_mode 			= 0,
	.bmControls.b.uvcx_ltr_buffer_size_control 	= 0,
	.bmControls.b.uvcx_ltr_picture_control 		= 0,
	.bmControls.b.uvcx_picture_type_control 	= 1,
	.bmControls.b.uvcx_version 					= 1,
	.bmControls.b.uvcx_encoder_reset 			= 1,
	.bmControls.b.uvcx_framerate_config 		= 1,
	.bmControls.b.uvcx_video_advance_config 	= 1,
	.bmControls.b.uvcx_bitrate_layers 			= 1,
	.bmControls.b.uvcx_qp_steps_layers 			= 1,
	.bmControls.b.uvcx_video_undefined 			= 0,

	.iExtension				= 0,
};

static  struct uvc_extension_unit_h264_descriptor_t uvc_xu_h264_s2 = {
	.bLength				= UVC_DT_EXTENSION_UNIT_SIZE(1,2),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VC_EXTENSION_UNIT,
	.bUnitID				= UVC_ENTITY_ID_XU_H264_S2,
	.guidExtensionCode		= UVC_GUID_XUCONTROL_H264,
	.bNumControls			= 0x07,
	.bNrInPins				= 1,
	//.baSourceID				= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bControlSize			= 2,

	.bmControls.b.uvcx_video_config_probe 		= 1,
	.bmControls.b.uvcx_video_config_commit 		= 1,
	.bmControls.b.uvcx_rate_control_mode 		= 1,
	.bmControls.b.uvcx_temporal_scale_mode 		= 0,
	.bmControls.b.uvcx_spatial_scale_mode 		= 0,
	.bmControls.b.uvcx_snr_scale_mode 			= 0,
	.bmControls.b.uvcx_ltr_buffer_size_control 	= 0,
	.bmControls.b.uvcx_ltr_picture_control 		= 0,
	.bmControls.b.uvcx_picture_type_control 	= 1,
	.bmControls.b.uvcx_version 					= 1,
	.bmControls.b.uvcx_encoder_reset 			= 1,
	.bmControls.b.uvcx_framerate_config 		= 1,
	.bmControls.b.uvcx_video_advance_config 	= 1,
	.bmControls.b.uvcx_bitrate_layers 			= 1,
	.bmControls.b.uvcx_qp_steps_layers 			= 1,
	.bmControls.b.uvcx_video_undefined 			= 0,

	.iExtension				= 0,
};




struct uvc_output_terminal_descriptor uvc_output_terminal = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	//.bSourceID			= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iTerminal			= 0,
};


/* YUY2 HS */

DECLARE_UVC_FRAME_YUY2_30FPS(640,480,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(160,120,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(176,144,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(320,180,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(320,240,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(352,288,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(424,240,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(424,320,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(480,272,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(512,288,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(640,360,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(720,480,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(720,576,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(800,448,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(800,600,_hs);
DECLARE_UVC_FRAME_YUY2_30FPS(848,480,_hs);


static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_960x540_hs = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_1024x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*576*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
 
static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_1024x768_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*768*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuv_1280x720_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_yuv_1600x896_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1600x896,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (896),
	.dwMinBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*896*2*7.5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*896*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_7FPS),
	.bFrameIntervalType	= 2,
	//.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
//#else
static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_yuv_1600x900_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*7.5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_7FPS),
	.bFrameIntervalType	= 2,
	//.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_1920x1080_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_2560x1440_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2560),
	.wHeight			= (1440),
	.dwMinBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_3840x2160_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_8000x6000_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_yuv_8000x6000_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*2*8),//Width x Height x 2 x 30 x 8 ,x3 integer overflow in expression
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_3FPS),
	.bFrameIntervalType	= 2,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),//(1/5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

//竖屏分辨率
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuv_720x1280_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (720),
	.wHeight			= (1280),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_1080x1920_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1080),
	.wHeight			= (1920),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_1440x2560_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1440),
	.wHeight			= (2560),
	.dwMinBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_2160x3840_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2160),
	.wHeight			= (3840),
	.dwMinBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_6000x8000_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_yuv_6000x8000_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*2*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_3FPS),
	.bFrameIntervalType	= 2,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),//(1/5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

/* I420 HS */

DECLARE_UVC_FRAME_I420_30FPS(640,480,_hs);
DECLARE_UVC_FRAME_I420_30FPS(160,120,_hs);
DECLARE_UVC_FRAME_I420_30FPS(176,144,_hs);
DECLARE_UVC_FRAME_I420_30FPS(320,180,_hs);
DECLARE_UVC_FRAME_I420_30FPS(320,240,_hs);
DECLARE_UVC_FRAME_I420_30FPS(352,288,_hs);
DECLARE_UVC_FRAME_I420_30FPS(424,240,_hs);
DECLARE_UVC_FRAME_I420_30FPS(424,320,_hs);
DECLARE_UVC_FRAME_I420_30FPS(480,272,_hs);
DECLARE_UVC_FRAME_I420_30FPS(512,288,_hs);
DECLARE_UVC_FRAME_I420_30FPS(640,360,_hs);
DECLARE_UVC_FRAME_I420_30FPS(720,480,_hs);
DECLARE_UVC_FRAME_I420_30FPS(720,576,_hs);
DECLARE_UVC_FRAME_I420_30FPS(800,448,_hs);
DECLARE_UVC_FRAME_I420_30FPS(800,600,_hs);
DECLARE_UVC_FRAME_I420_30FPS(848,480,_hs);


static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_i420_960x540_hs = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_960x540,
	.bmCapabilities		= 0,
	.wWidth				= (960),
	.wHeight			= (540),
	.dwMinBitRate		= (960*540*5*8*2),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (960*540*15*8*2),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (960*540*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_i420_1024x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*576*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*1.5,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
 
static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_i420_1024x768_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1024x768,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*768*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*1.5,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_i420_1280x720_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_i420_1600x896_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1600x896,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (896),
	.dwMinBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*896*2*7.5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*896*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_7FPS),
	.bFrameIntervalType	= 2,
	//.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
//#else
static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_i420_1600x900_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*7.5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_7FPS),
	.bFrameIntervalType	= 2,
	//.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_1920x1080_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_2560x1440_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2560),
	.wHeight			= (1440),
	.dwMinBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_3840x2160_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_8000x6000_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_i420_8000x6000_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*2*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_3FPS),
	.bFrameIntervalType	= 2,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),//(1/5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

//竖屏分辨率
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_i420_720x1280_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (720),
	.wHeight			= (1280),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*10*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_10FPS),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_1080x1920_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_YUV_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1080),
	.wHeight			= (1920),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_1440x2560_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1440),
	.wHeight			= (2560),
	.dwMinBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_2160x3840_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2160),
	.wHeight			= (3840),
	.dwMinBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_i420_6000x8000_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_i420_6000x8000_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*2*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_3FPS),
	.bFrameIntervalType	= 2,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),//(1/5)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};
	
/* MJPEG */
DECLARE_UVC_FRAME_MJPEG_60FPS(640,480,);
DECLARE_UVC_FRAME_MJPEG_60FPS(160,120,);
DECLARE_UVC_FRAME_MJPEG_60FPS(176,144,);
DECLARE_UVC_FRAME_MJPEG_60FPS(320,180,);
DECLARE_UVC_FRAME_MJPEG_60FPS(320,240,);
DECLARE_UVC_FRAME_MJPEG_60FPS(352,288,);
DECLARE_UVC_FRAME_MJPEG_60FPS(424,240,);
DECLARE_UVC_FRAME_MJPEG_60FPS(424,320,);
DECLARE_UVC_FRAME_MJPEG_60FPS(480,272,);
DECLARE_UVC_FRAME_MJPEG_60FPS(512,288,);
DECLARE_UVC_FRAME_MJPEG_60FPS(640,360,);
DECLARE_UVC_FRAME_MJPEG_60FPS(720,480,);
DECLARE_UVC_FRAME_MJPEG_60FPS(720,576,);
DECLARE_UVC_FRAME_MJPEG_60FPS(800,448,);
DECLARE_UVC_FRAME_MJPEG_60FPS(800,600,);
DECLARE_UVC_FRAME_MJPEG_60FPS(848,480,);
DECLARE_UVC_FRAME_MJPEG_60FPS(960,540,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1024,576,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1024,768,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1280,720,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1600,896,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1600,900,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1920,1080,);


#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
DECLARE_UVC_FRAME_MJPEG_30FPS(2560,1440,_hs);
DECLARE_UVC_FRAME_MJPEG_30FPS(3840,2160,_hs);

DECLARE_UVC_FRAME_MJPEG_60FPS(2560,1440,_ss);
DECLARE_UVC_FRAME_MJPEG_60FPS(3840,2160,_ss);


static struct UVC_FRAME_MJPEG(2) uvc_frame_mjpg_8000x6000_hs = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	/*.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_##w##x##h,*/
	.bmCapabilities 	= 0,
	.wWidth 			= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8U),/*Width x Height x 2 x 25 x 8*/ 
	.dwMaxBitRate		= (8000*6000*2*3*8U),/*Width x Height x 2 x 30 x 8*/
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),/*Width x Height x 2*/
	.dwDefaultFrameInterval = (UVC_FORMAT_1FPS),
	.bFrameIntervalType = (2),
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),
};

static struct UVC_FRAME_MJPEG(2) uvc_frame_mjpg_8000x6000_ss = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	/*.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_##w##x##h,*/
	.bmCapabilities 	= 0,
	.wWidth 			= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8U),/*Width x Height x 2 x 25 x 8*/ 
	.dwMaxBitRate		= (8000*6000*2*3*8U),/*Width x Height x 2 x 30 x 8*/
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),/*Width x Height x 2*/
	.dwDefaultFrameInterval = (UVC_FORMAT_3FPS),
	.bFrameIntervalType = (2),
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),
};

//竖屏分辨率
DECLARE_UVC_FRAME_MJPEG_60FPS(720,1280,);
DECLARE_UVC_FRAME_MJPEG_60FPS(1080,1920,);
DECLARE_UVC_FRAME_MJPEG_30FPS(1440,2560,_hs);
DECLARE_UVC_FRAME_MJPEG_30FPS(2160,3840,_hs);

DECLARE_UVC_FRAME_MJPEG_30FPS(1440,2560,_ss);
DECLARE_UVC_FRAME_MJPEG_30FPS(2160,3840,_ss);

static struct UVC_FRAME_MJPEG(2) uvc_frame_mjpg_6000x8000_hs = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	/*.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_##w##x##h,*/
	.bmCapabilities 	= 0,
	.wWidth 			= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8U),/*Width x Height x 2 x 25 x 8*/ 
	.dwMaxBitRate		= (8000*6000*2*3*8U),/*Width x Height x 2 x 30 x 8*/
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),/*Width x Height x 2*/
	.dwDefaultFrameInterval = (UVC_FORMAT_1FPS),
	.bFrameIntervalType = (2),
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),
};

static struct UVC_FRAME_MJPEG(2) uvc_frame_mjpg_6000x8000_ss = {
	.bLength			= UVC_DT_FRAME_MJPEG_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	/*.bFrameIndex		= UVC_MJPEG_FORMAT_INDEX_##w##x##h,*/
	.bmCapabilities 	= 0,
	.wWidth 			= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8U),/*Width x Height x 2 x 25 x 8*/ 
	.dwMaxBitRate		= (8000*6000*2*3*8U),/*Width x Height x 2 x 30 x 8*/
	.dwMaxVideoFrameBufferSize	= (8000*6000*2),/*Width x Height x 2*/
	.dwDefaultFrameInterval = (UVC_FORMAT_3FPS),
	.bFrameIntervalType = (2),
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),
};
#endif


/* UVC1.1 H264/H265 */
DECLARE_UVC_FRAME_H264_BASE_60FPS(640,480,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(160,120,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(176,144,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(320,180,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(320,240,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(352,288,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(424,240,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(424,320,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(480,272,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(512,288,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(640,360,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(720,480,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(720,576,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(800,448,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(800,600,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(848,480,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(960,540,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1024,576,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1024,768,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1280,720,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1600,896,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1600,900,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1920,1080,);


#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
DECLARE_UVC_FRAME_H264_BASE_60FPS(2560,1440,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(3840,2160,);

static struct UVC_FRAME_H264_BASE(2) uvc110_frame_h264_8000x6000 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(2),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	/*.bFrameIndex			= UVC110_H264_FORMAT_INDEX_##w##x##h,*/
	.bmCapabilities			= 0,
	.wWidth					= (8000),
	.wHeight				= (6000),
	.dwMinBitRate			= (8000*6000*2*1*8U),
	.dwMaxBitRate			= (8000*6000*2*3*8U),
	.dwDefaultFrameInterval	= (UVC_FORMAT_3FPS),
	.bFrameIntervalType		= (2),
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),
};
#endif


//竖屏分辨率
DECLARE_UVC_FRAME_H264_BASE_60FPS(720,1280,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(1080,1920,);
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
DECLARE_UVC_FRAME_H264_BASE_60FPS(1440,2560,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(2160, 3840,);

static struct UVC_FRAME_H264_BASE(2) uvc110_frame_h264_6000x8000 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(2),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	/*.bFrameIndex			= UVC110_H264_FORMAT_INDEX_##w##x##h,*/
	.bmCapabilities			= 0,
	.wWidth					= (6000),
	.wHeight				= (8000),
	.dwMinBitRate			= (8000*6000*2*1*8U),
	.dwMaxBitRate			= (8000*6000*2*3*8U),
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType		= (2),
	.dwBytesPerLine         = 0,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),
};
#endif



#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
/* NV12 HS */
DECLARE_UVC_FRAME_NV12_30FPS(640,480,_hs);
#if 1
DECLARE_UVC_FRAME_NV12_30FPS(160,120,_hs);
DECLARE_UVC_FRAME_NV12_30FPS(176,144,_hs);
DECLARE_UVC_FRAME_NV12_30FPS(320,180,_hs);
DECLARE_UVC_FRAME_NV12_30FPS(320,240,_hs);
#endif
DECLARE_UVC_FRAME_NV12_30FPS(352,288,_hs);
#if 1
DECLARE_UVC_FRAME_NV12_30FPS(424,240,_hs);
DECLARE_UVC_FRAME_NV12_30FPS(424,320,_hs);
#endif
DECLARE_UVC_FRAME_NV12_30FPS(480,272,_hs);
#if 1
DECLARE_UVC_FRAME_NV12_30FPS(512,288,_hs);
#endif

DECLARE_UVC_FRAME_NV12_30FPS(640,360,_hs);
DECLARE_UVC_FRAME_NV12_30FPS(720,480,_hs);
DECLARE_UVC_FRAME_NV12_30FPS(720,576,_hs);
#if 1
DECLARE_UVC_FRAME_NV12_30FPS(800,448,_hs);
#endif
DECLARE_UVC_FRAME_NV12_30FPS(800,600,_hs);
#if 1 
DECLARE_UVC_FRAME_NV12_30FPS(848,480,_hs);
#endif
DECLARE_UVC_FRAME_NV12_30FPS(960,540,_hs);

static struct UVC_FRAME_UNCOMPRESSED(6) uvc_frame_nv12_1024x576_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(6),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1024x576,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 576,
	.dwMinBitRate		= 1024*576*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*576*2*24*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*576*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_24FPS),
	.bFrameIntervalType	= 6,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_20FPS),//(1/20)x10^7	
	.dwFrameInterval[2] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[4] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[5] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#if 1 
static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_1024x768_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= 1024,
	.wHeight			= 768,
	.dwMinBitRate		= 1024*768*2*5*8,//Width x Height x 2 x 5 x 8
	.dwMaxBitRate		= 1024*768*2*15*8,//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= 1024*768*3/2,//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0] = (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[1] = (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[2] = (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0] = (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1] = (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2] = (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3] = (UVC_FORMAT_5FPS),//(1/5)x10^7

};
#endif
static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_1280x720_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (1280),
	.wHeight			= (720),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1600x896_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (896),
	.dwMinBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*896*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*896*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1600x900_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1600),
	.wHeight			= (900),
	.dwMinBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1600*900*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1600*900*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1920x1080_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1920),
	.wHeight			= (1080),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_2560x1440_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2560),
	.wHeight			= (1440),
	.dwMinBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_3840x2160_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_8000x6000_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(2) uvc_frame_nv12_8000x6000_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (8000),
	.wHeight			= (6000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*2*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_3FPS),
	.bFrameIntervalType	= 2,
	.dwFrameInterval[0]	= (UVC_FORMAT_3FPS),//(1/3)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};
#endif


//竖屏分辨率
static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_720x1280_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1280x720,
	.bmCapabilities		= 0,
	.wWidth				= (720),
	.wHeight			= (1280),
	.dwMinBitRate		= (1280*720*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1280*720*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1280*720*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1080x1920_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= UVC_NV12_FORMAT_INDEX_1920x1080,
	.bmCapabilities		= 0,
	.wWidth				= (1080),
	.wHeight			= (1920),
	.dwMinBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (1920*1080*2*5*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (1920*1080*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_5FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_1440x2560_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (1440),
	.wHeight			= (2560),
	.dwMinBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (2560*1440*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (2560*1440*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_2160x3840_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2160),
	.wHeight			= (3840),
	.dwMinBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_6000x8000_hs = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};

static struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_6000x8000_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (6000),
	.wHeight			= (8000),
	.dwMinBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (8000*6000*2*1*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (8000*6000*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_1FPS),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= (UVC_FORMAT_1FPS),//(1/5)x10^7
};
#endif
/************************ss*********************************************/

DECLARE_UVC_FRAME_YUY2_60FPS(640,480,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(160,120,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(176,144,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(320,180,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(320,240,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(352,288,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(424,240,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(424,320,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(480,272,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(512,288,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(640,360,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(720,480,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(720,576,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(800,448,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(800,600,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(848,480,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(960,540,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1024,576,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1024,768,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1280,720,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1600,896,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1600,900,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1920,1080,_ss);
DECLARE_UVC_FRAME_YUY2_30FPS(2560,1440,_ss);

//竖屏分辨率
DECLARE_UVC_FRAME_YUY2_60FPS(720,1280,_ss);
DECLARE_UVC_FRAME_YUY2_60FPS(1080,1920,_ss);
DECLARE_UVC_FRAME_YUY2_30FPS(1440,2560,_ss);

DECLARE_UVC_FRAME_I420_60FPS(640,480,_ss);
DECLARE_UVC_FRAME_I420_60FPS(160,120,_ss);
DECLARE_UVC_FRAME_I420_60FPS(176,144,_ss);
DECLARE_UVC_FRAME_I420_60FPS(320,180,_ss);
DECLARE_UVC_FRAME_I420_60FPS(320,240,_ss);
DECLARE_UVC_FRAME_I420_60FPS(352,288,_ss);
DECLARE_UVC_FRAME_I420_60FPS(424,240,_ss);
DECLARE_UVC_FRAME_I420_60FPS(424,320,_ss);
DECLARE_UVC_FRAME_I420_60FPS(480,272,_ss);
DECLARE_UVC_FRAME_I420_60FPS(512,288,_ss);
DECLARE_UVC_FRAME_I420_60FPS(640,360,_ss);
DECLARE_UVC_FRAME_I420_60FPS(720,480,_ss);
DECLARE_UVC_FRAME_I420_60FPS(720,576,_ss);
DECLARE_UVC_FRAME_I420_60FPS(800,448,_ss);
DECLARE_UVC_FRAME_I420_60FPS(800,600,_ss);
DECLARE_UVC_FRAME_I420_60FPS(848,480,_ss);
DECLARE_UVC_FRAME_I420_60FPS(960,540,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1024,576,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1024,768,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1280,720,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1600,896,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1600,900,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1920,1080,_ss);
DECLARE_UVC_FRAME_I420_30FPS(2560,1440,_ss);

//竖屏分辨率
DECLARE_UVC_FRAME_I420_60FPS(720,1280,_ss);
DECLARE_UVC_FRAME_I420_60FPS(1080,1920,_ss);
DECLARE_UVC_FRAME_I420_30FPS(1440,2560,_ss);


#ifdef UVC_VIDEO_FORMAT_NV12_SUPPORT
DECLARE_UVC_FRAME_NV12_60FPS(640,480,_ss);
#if 1
DECLARE_UVC_FRAME_NV12_60FPS(160,120,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(176,144,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(320,180,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(320,240,_ss);
#endif
DECLARE_UVC_FRAME_NV12_60FPS(352,288,_ss);
#if 1
DECLARE_UVC_FRAME_NV12_60FPS(424,240,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(424,320,_ss);
#endif
DECLARE_UVC_FRAME_NV12_60FPS(480,272,_ss);
#if 1
DECLARE_UVC_FRAME_NV12_60FPS(512,288,_ss);
#endif
DECLARE_UVC_FRAME_NV12_60FPS(640,360,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(720,480,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(720,576,_ss);

#if 1 
DECLARE_UVC_FRAME_NV12_60FPS(800,448,_ss);
#endif
DECLARE_UVC_FRAME_NV12_60FPS(800,600,_ss);
#if 1 
DECLARE_UVC_FRAME_NV12_60FPS(848,480,_ss);
#endif
DECLARE_UVC_FRAME_NV12_60FPS(960,540,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(1024,576,_ss);
#if 1 
DECLARE_UVC_FRAME_NV12_60FPS(1024,768,_ss);
#endif
DECLARE_UVC_FRAME_NV12_60FPS(1280,720,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(1600,896,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(1600,900,_ss);

DECLARE_UVC_FRAME_NV12_60FPS(1920,1080,_ss);
DECLARE_UVC_FRAME_NV12_30FPS(2560,1440,_ss);

//竖屏分辨率
DECLARE_UVC_FRAME_NV12_60FPS(720,1280,_ss);
DECLARE_UVC_FRAME_NV12_60FPS(1080,1920,_ss);
DECLARE_UVC_FRAME_NV12_30FPS(1440,2560,_ss);
#endif


static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_3840x2160_ss = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_i420_3840x2160_ss = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_3840x2160_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (3840),
	.wHeight			= (2160),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};



//竖屏分辨率
static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_yuv_2160x3840_ss = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2160),
	.wHeight			= (3840),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_i420_2160x3840_ss = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2160),
	.wHeight			= (3840),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*1.5),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7

};

static struct UVC_FRAME_UNCOMPRESSED(4) uvc_frame_nv12_2160x3840_ss = {
	.bLength			= UVC_DT_FRAME_UNCOMPRESSED_SIZE(4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 0,
	.bmCapabilities		= 0,
	.wWidth				= (2160),
	.wHeight			= (3840),
	.dwMinBitRate		= (3840*2160*2*5*8),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate		= (3840*2160*2*15*8),//Width x Height x 2 x 30 x 8
	.dwMaxVideoFrameBufferSize	= (3840*2160*3/2),//Width x Height x 2
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType	= 4,
	//.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	//.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	//.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	//.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};




static const struct uvc_descriptor_header * const uvc_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_xu_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	//(const struct uvc_descriptor_header *) &uvc_xu_menu,
	
	//(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_xu_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	//(const struct uvc_descriptor_header *) &uvc_xu_menu,
	
	//(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};



/* --------------------------------------------------------------------------
 * UVC1.1 S2 configuration
 */
 
//uvc_control

static struct UVC_HEADER_DESCRIPTOR(2) uvc_control_header_s2 = {
	.bLength			= UVC_DT_HEADER_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(UVC_CLOCK_FREQUENCY),
	.bInCollection		= 1, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
	.baInterfaceNr[1]	= 0, /* dynamic */
};

struct uvc_output_terminal_descriptor uvc_output_terminal_s2 = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	//.bSourceID			= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iTerminal			= 0,
};

static const struct uvc_descriptor_header * const uvc_fs_control_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header_s2,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_xu_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	(const struct uvc_descriptor_header *) &uvc_xu_h264_s2,
	// (const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	// (const struct uvc_descriptor_header *) &uvc_xu_menu,
	
	// (const struct uvc_descriptor_header *) &uvc_output_terminal,
	// (const struct uvc_descriptor_header *) &uvc_output_terminal_s2,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls_s2[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header_s2,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_xu_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	(const struct uvc_descriptor_header *) &uvc_xu_h264_s2,
	// (const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	// (const struct uvc_descriptor_header *) &uvc_xu_menu,
	
	// (const struct uvc_descriptor_header *) &uvc_output_terminal,
	// (const struct uvc_descriptor_header *) &uvc_output_terminal_s2,
	NULL,
};


static struct UVC_HEADER_DESCRIPTOR(1) uvc_control_header_s2_show = {
	.bLength			= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(UVC_CLOCK_FREQUENCY),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};


static struct uvc_camera_terminal_descriptor uvc_camera_terminal_s2_show = {
	.bLength			= UVC_DT_CAMERA_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_INPUT_TERMINAL | 0x80,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal			= 0,

	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	
	.bmControls.d24[0] = 0,
	.bmControls.d24[1] = 0,
	.bmControls.d24[2] = 0,
};

static struct uvc_processing_unit_descriptor uvc_processing_s2_show = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	//.bUnitID		= UVC_ENTITY_ID_PROCESSING_UNIT,
	.bSourceID		= UVC_ENTITY_ID_INPUT_TERMINAL | 0x80,
	.wMaxMultiplier		= cpu_to_le16(0x4000),
	.bControlSize		= 2,
	.bmControls.d16 = 0,
	.iProcessing		= 0,
	.bmVideoStandards   = 0 //UVC 1.0 no support
};

struct uvc_output_terminal_descriptor uvc_output_terminal_s2_show = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_TERMINAL | 0x80,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	//.bSourceID			= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iTerminal			= 0,
};

static const struct uvc_descriptor_header * const uvc_control_cls_s2_show[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header_s2_show,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal_s2_show,
	(const struct uvc_descriptor_header *) &uvc_processing_s2_show,
	(const struct uvc_descriptor_header *) &uvc_output_terminal_s2_show,
	NULL,
};


/* --------------------------------------------------------------------------
 * UVC1.5 configuration
 */
 
static struct UVC_HEADER_DESCRIPTOR(2) uvc150_control_header = {
	.bLength			= UVC_DT_HEADER_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC				= cpu_to_le16(0x0150),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(UVC_CLOCK_FREQUENCY),
	.bInCollection		= 1, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
	.baInterfaceNr[1]	= 0, /* dynamic */
};

//uvc1.5 for h264
static struct uvc_encoding_unit_descriptor uvc150_encoding= {
	.bLength				= UVC_DT_ENCODING_UNIT_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VC_ENCODING_UNIT,
	.bUnitID				= UVC_ENTITY_ID_ENCODING_UNIT,
	//.bSourceID				= UVC_ENTITY_ID_PROCESSING_UNIT,
	.iEncoding				= 0,
	.bControlSize			= 0x03,
#if 1
	.bmControls[0]			= 0xFF,
	.bmControls[1]			= 0xC6,
	.bmControls[2]			= 0x07,
	.bmControlsRuntime[0]	= 0xFD,
	.bmControlsRuntime[1]	= 0x06,
	.bmControlsRuntime[2]	= 0x07,
#else
	.bmControls[0]			= 0,//0xFF,
	.bmControls[1]			= 0,//0xC6,
	.bmControls[2]			= 0,//0x07,
	.bmControlsRuntime[0]	= 0,//0xFD,
	.bmControlsRuntime[1]	= 0,//0x06,
	.bmControlsRuntime[2]	= 0,//0x07,
#endif
};



const struct uvc_output_terminal_descriptor uvc150_output_terminal_s2 = {
	.bLength			= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_ENTITY_ID_OUTPUT_ENCODING,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID			= UVC_ENTITY_ID_ENCODING_UNIT,
	.iTerminal			= 0,
};


//DECLARE_UVC_FRAME_H264_V150(2);

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_640x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_640x480,
	.wWidth					= (640),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,

	.bmCapabilities			= 0x0021,
	/*
     D00 = 1  yes -  CAVLC only
     D01 = 0   no -  CABAC only
     D02 = 0   no -  Constant frame rate
     D03 = 1  yes -  Separate QP for luma/chroma
     D04 = 0   no -  Separate QP for Cb/Cr
     D05 = 1  yes -  No picture reordering
     D06 = 0   no -  Long-term reference frame
     D07 = 0   no -  Reserved
     D08 = 0   no -  Reserved
     D09 = 0   no -  Reserved
     D10 = 0   no -  Reserved
     D11 = 0   no -  Reserved
     D12 = 0   no -  Reserved
     D13 = 0   no -  Reserved
     D14 = 0   no -  Reserved
     D15 = 0   no -  Reserved
	 */
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	/*
	 D2..D0   = 1  Maximum number of temporal layers = 3
     D3       = 0  no - Rewrite Support
     D6..D4   = 0  Maximum number of CGS layers = 1
     D9..D7   = 0  Number of MGS sublayers
     D10      = 0  no - Additional SNR scalability support in spatial enhancement layers
     D13..D11 = 0  Maximum number of spatial layers = 1
     D14      = 0  no - Reserved
     D15      = 0  no - Reserved
     D16      = 0  no - Reserved
     D17      = 0  no - Reserved
     D18      = 0  no - Reserved
     D19      = 0  no - Reserved
     D20      = 0  no - Reserved
     D21      = 0  no - Reserved
     D22      = 0  no - Reserved
     D23      = 0  no - Reserved
     D24      = 0  no - Reserved
     D25      = 0  no - Reserved
     D26      = 0  no - Reserved
     D27      = 0  no - Reserved
     D28      = 0  no - Reserved
     D29      = 0  no - Reserved
     D30      = 0  no - Reserved
     D31      = 0  no - Reserved
	 */
	.bmMVCCapabilities		= 0x00000000,
	/*
	 D2..D0   = 0  Maximum number of temporal layers = 1
     D10..D3  = 0  Maximum number of view components = 1
     D11      = 0  no - Reserved
     D12      = 0  no - Reserved
     D13      = 0  no - Reserved
     D14      = 0  no - Reserved
     D15      = 0  no - Reserved
     D16      = 0  no - Reserved
     D17      = 0  no - Reserved
     D18      = 0  no - Reserved
     D19      = 0  no - Reserved
     D20      = 0  no - Reserved
     D21      = 0  no - Reserved
     D22      = 0  no - Reserved
     D23      = 0  no - Reserved
     D24      = 0  no - Reserved
     D25      = 0  no - Reserved
     D26      = 0  no - Reserved
     D27      = 0  no - Reserved
     D28      = 0  no - Reserved
     D29      = 0  no - Reserved
     D30      = 0  no - Reserved
     D31      = 0  no - Reserved
	 */
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_160x120_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_160x120,
	.wWidth					= (160),
	.wHeight				= (120),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_176x144_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_176x144,
	.wWidth					= (176),
	.wHeight				= (144),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_320x180_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_320x180,
	.wWidth					= (320),
	.wHeight				= (180),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_320x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_320x240,
	.wWidth					= (320),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_352x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_352x288,
	.wWidth					= (352),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_424x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_424x240,
	.wWidth					= (424),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_424x320_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_424x320,
	.wWidth					= (424),
	.wHeight				= (320),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_480x272_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_480x272,
	.wWidth					= (480),
	.wHeight				= (272),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_512x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_512x288,
	.wWidth					= (512),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_640x360_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_640x360,
	.wWidth					= (640),
	.wHeight				= (360),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_720x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_720x480,
	.wWidth					= (720),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_720x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_720x576,
	.wWidth					= (720),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_800x448_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_800x448,
	.wWidth					= (800),
	.wHeight				= (448),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_800x600_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_800x600,
	.wWidth					= (800),
	.wHeight				= (600),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_848x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_848x480,
	.wWidth					= (848),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_960x540_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_960x540,
	.wWidth					= (960),
	.wHeight				= (540),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_1024x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1024x576,
	.wWidth					= (1024),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1 
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_1024x768_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1024x768,
	.wWidth					= (1024),
	.wHeight				= (768),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_1280x720_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1280x720,
	.wWidth					= (1280),
	.wHeight				= (720),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_1600x896_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1600x896,
	.wWidth					= (1600),
	.wHeight				= (896),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
//#else
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_1600x900_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1600x896,
	.wWidth					= (1600),
	.wHeight				= (900),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_1920x1080_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_1920x1080,
	.wWidth					= (1920),
	.wHeight				= (1080),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_2560x1440_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_2560x1440,
	.wWidth					= (2560),
	.wHeight				= (1440),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*2),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*2),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};



static struct UVC_FRAME_H264_V150(10) uvc150_frame_h264_3840x2160_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT_INDEX_3840x2160,
	.wWidth					= (3840),
	.wHeight				= (2160),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x4240,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0021,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*4),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*4),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

//H264

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_640x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_640x480,
	.wWidth					= (640),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_160x120_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_160x120,
	.wWidth					= (160),
	.wHeight				= (120),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_176x144_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_176x144,
	.wWidth					= (176),
	.wHeight				= (144),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_320x180_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_320x180,
	.wWidth					= (320),
	.wHeight				= (180),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (7000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_320x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_320x240,
	.wWidth					= (320),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_352x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_352x288,
	.wWidth					= (352),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_424x240_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_424x240,
	.wWidth					= (424),
	.wHeight				= (240),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_424x320_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_424x320,
	.wWidth					= (424),
	.wHeight				= (320),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_480x272_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_480x272,
	.wWidth					= (480),
	.wHeight				= (272),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_512x288_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_512x288,
	.wWidth					= (512),
	.wHeight				= (288),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_640x360_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_640x360,
	.wWidth					= (640),
	.wHeight				= (360),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_720x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_720x480,
	.wWidth					= (720),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_720x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_720x576,
	.wWidth					= (720),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (64000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_800x448_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_800x448,
	.wWidth					= (800),
	.wHeight				= (448),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_800x600_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_800x600,
	.wWidth					= (800),
	.wHeight				= (600),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_848x480_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_848x480,
	.wWidth					= (848),
	.wHeight				= (480),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_960x540_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_960x540,
	.wWidth					= (960),
	.wHeight				= (540),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_1024x576_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1024x576,
	.wWidth					= (1024),
	.wHeight				= (576),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_1024x768_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1024x768,
	.wWidth					= (1024),
	.wHeight				= (768),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_1280x720_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1280x720,
	.wWidth					= (1280),
	.wHeight				= (720),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_1600x896_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1600x896,
	.wWidth					= (1600),
	.wHeight				= (896),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#if 1
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_1600x900_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= 0,
	.wWidth					= (1600),
	.wHeight				= (900),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 8,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_1920x1080_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_1920x1080,
	.wWidth					= (1920),
	.wHeight				= (1080),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};

#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_2560x1440_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_2560x1440,
	.wWidth					= (2560),
	.wHeight				= (1440),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*2),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*2),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};


static struct UVC_FRAME_H264_V150(10) uvc150_frame2_h264_3840x2160_s2 = {
	.bLength				= UVC_DT_FRAME_H264_V150_SIZE(10),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_H264_V150,
	.bFrameIndex			= UVC_H264_FORMAT2_INDEX_3840x2160,
	.wWidth					= (3840),
	.wHeight				= (2160),

	.wSARwidth 				= 0x0001,
	.wSARheight 			= 0x0001,
	.wProfile				= 0x640C,
	.bLevelIDC				= 0x28,
	.wConstrainedToolset	= 0x0000,
	.bmSupportedUsages		= 0x00000003,
	.bmCapabilities			= 0x0020,
	.bmSVCCapabilities		= SVC_MAX_LAYERS,
	.bmMVCCapabilities		= 0x00000000,
	.dwMinBitRate			= (256000*4),//Width x Height x 2 x 25 x 8
	.dwMaxBitRate			= (12000000*4),//Width x Height x 2 x 30 x 8
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bNumFrameIntervals		= 10,
	.dwFrameInterval[0]	= (UVC_FORMAT_60FPS),//(1/60)x10^7
	.dwFrameInterval[1]	= (UVC_FORMAT_50FPS),//(1/50)x10^7
	.dwFrameInterval[2]	= (UVC_FORMAT_30FPS),//(1/30)x10^7
	.dwFrameInterval[3]	= (UVC_FORMAT_25FPS),//(1/25)x10^7
	.dwFrameInterval[4]	= (UVC_FORMAT_24FPS),//(1/24)x10^7
	.dwFrameInterval[5]	= (UVC_FORMAT_20FPS),//(1/20)x10^7
	.dwFrameInterval[6]	= (UVC_FORMAT_15FPS),//(1/15)x10^7
	.dwFrameInterval[7]	= (UVC_FORMAT_10FPS),//(1/10)x10^7
	.dwFrameInterval[8]	= (UVC_FORMAT_7FPS),//(1/7.5)x10^7
	.dwFrameInterval[9]	= (UVC_FORMAT_5FPS),//(1/5)x10^7
};
#endif



/************************************************4CIF*****************************************************/

DECLARE_UVC_FRAME_YUY2_60FPS(704,576,_ss);
DECLARE_UVC_FRAME_YUY2_30FPS(704,576,_hs);
DECLARE_UVC_FRAME_I420_60FPS(704,576,_ss);
DECLARE_UVC_FRAME_I420_30FPS(704,576,_hs);
DECLARE_UVC_FRAME_NV12_60FPS(704,576,_ss);
DECLARE_UVC_FRAME_NV12_30FPS(704,576,_hs);
DECLARE_UVC_FRAME_MJPEG_60FPS(704,576,);
DECLARE_UVC_FRAME_H264_BASE_60FPS(704,576,);
DECLARE_UVC150_FRAME_H264_60FPS(704,576,_s2);
DECLARE_UVC150_FRAME2_H264_60FPS(704,576,_s2);


/*********************************************************************************************************/
 
static const struct uvc_descriptor_header * const uvc150_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc150_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc150_encoding,
	(const struct uvc_descriptor_header *) &uvc_xu_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	//(const struct uvc_descriptor_header *) &uvc_xu_menu,
	
	//(const struct uvc_descriptor_header *) &uvc_output_terminal,
	//(const struct uvc_descriptor_header *) &uvc150_output_terminal_s2,
	NULL,
};

static const struct uvc_descriptor_header * const uvc150_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc150_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc150_encoding,
	(const struct uvc_descriptor_header *) &uvc_xu_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_xu_h264,
	//(const struct uvc_descriptor_header *) &uvc_xu_custom_commands,
	//(const struct uvc_descriptor_header *) &uvc_xu_menu,
	
	//(const struct uvc_descriptor_header *) &uvc_output_terminal,
	//(const struct uvc_descriptor_header *) &uvc150_output_terminal_s2,
	NULL,
};

static struct UVC_INPUT_HEADER_DESCRIPTOR(1, 5) uvc_streaming_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 0x02,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo				= 0,
	.bTerminalLink		= UVC_ENTITY_ID_OUTPUT_TERMINAL,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
	.bmaControls[2][0]	= 0,
	.bmaControls[3][0]	= 0,
	.bmaControls[4][0]	= 0,
};

/**************************************************HS**********************************************************/
//YUY2
static struct uvc_format_uncompressed uvc_format_hs_yuy2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_hs_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1280x720,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_hs_i420 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1280x720,
	.guidFormat		=
		{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

//MJPEG
static struct uvc_format_mjpeg uvc_format_hs_mjpeg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	=  0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

//H264 UVC1.1
static struct uvc_format_h264_base uvc110_format_hs_h264 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};


static struct uvc_format_h264_base uvc110_format_hs_h265 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H265,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};


//H264 V1.5
static struct uvc_format_h264_v150 uvc150_format_hs_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_V150,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 243,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 243,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};

static struct uvc_format_h264_v150 uvc150_format_hs_h264_simulcast_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_SIMULCAST,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 243,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 486,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 729,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 243,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 486,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 729,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};

static struct uvc_format_uncompressed uvc_format_hs_yuy2_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,	
};

static struct uvc_format_uncompressed uvc_format_hs_nv12_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1280x720,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,	
};

static struct uvc_format_uncompressed uvc_format_hs_i420_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1280x720,
	.guidFormat		=
		{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,	
};

static struct uvc_format_mjpeg uvc_format_hs_mjpeg_s2 = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	=  0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,	
};
static struct uvc_format_h264_base uvc110_format_hs_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,	
};

static struct uvc_format_h264_base uvc110_format_hs_h265_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H265,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};

/**************************************************SS**********************************************************/
static struct uvc_format_uncompressed uvc_format_ss_yuy2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_ss_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_ss_i420 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

//MJPEG
static struct uvc_format_mjpeg uvc_format_ss_mjpeg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_h264_base uvc110_format_ss_h264 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
static struct uvc_format_h264_base uvc110_format_ss_h265 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H265,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};

static struct uvc_format_uncompressed uvc_format_ss_yuy2_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_ss_nv12_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_ss_i420_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_mjpeg uvc_format_ss_mjpeg_s2 = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
static struct uvc_format_h264_base uvc110_format_ss_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
static struct uvc_format_h264_base uvc110_format_ss_h265_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H265,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
static struct uvc_format_h264_v150 uvc150_format_ss_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_V150,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 243,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 243,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};
static struct uvc_format_h264_v150 uvc150_format_ss_h264_simulcast_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_SIMULCAST,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 243,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 486,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 729,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 243,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 486,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 729,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};

/**************************************************SSP**********************************************************/
static struct uvc_format_uncompressed uvc_format_ssp_yuy2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_ssp_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_uncompressed uvc_format_ssp_i420 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

//MJPEG
static struct uvc_format_mjpeg uvc_format_ssp_mjpeg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static struct uvc_format_h264_base uvc110_format_ssp_h264 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
static struct uvc_format_h264_base uvc110_format_ssp_h265 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H265,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};

static struct uvc_format_uncompressed uvc_format_ssp_yuy2_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_YUV_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
static struct uvc_format_uncompressed uvc_format_ssp_nv12_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
static struct uvc_format_uncompressed uvc_format_ssp_i420_s2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 0,
	.bNumFrameDescriptors	= UVC_NV12_FORMAT_INDEX_1920x1080,
	.guidFormat		=
		{ 'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
static struct uvc_format_mjpeg uvc_format_ssp_mjpeg_s2 = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	=  UVC_MJPEG_FORMAT_INDEX_1920x1080,
#endif
	.bmFlags		= 1,
	.bDefaultFrameIndex	= 0x01,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
static struct uvc_format_h264_base uvc110_format_ssp_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H264,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
static struct uvc_format_h264_base uvc110_format_ssp_h265_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_3840x2160,
#else
	.bNumFrameDescriptors	= UVC110_H264_FORMAT_INDEX_1920x1080,
#endif
	.guidFormat				= UVC_GUID_FORMAT_H265,
	.bBitsPerPixel			= 16,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 1,
};
static struct uvc_format_h264_v150 uvc150_format_ssp_h264_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_V150,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 243,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 243,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};
static struct uvc_format_h264_v150 uvc150_format_ssp_h264_simulcast_s2 = {
	.bLength				= UVC_DT_FORMAT_H264_V150_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_H264_SIMULCAST,
	.bFormatIndex			= 0,
#ifdef	UVC_VIDEO_FORMAT_4K_SUPPORT
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_3840x2160, /* NOTES: when more frames supported, inc this */
#else
	.bNumFrameDescriptors	= UVC_H264_FORMAT2_INDEX_1920x1080,
#endif
	.bDefaultFrameIndex		= 0x01,

	.bMaxCodecConfigDelay 			= 0x01,
	.bmSupportedSliceModes			= 0x0E,
	.bmSupportedSyncFrameTypes		= 0x0A,
	.bResolutionScaling				= 0x03,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x03,//0x0F,
	/*
	 * bmSupportedRateControlModes
	 *
	 D00 -  Variable Bit Rate (VBR) with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D01 -  Constant Bit Rate (CBR) (H.264 low_delay_hrd_flag = 0)
     D02 -  Constant QP
     D03 -  Global VBR with underflow allowed (H.264 low_delay_hrd_flag = 1)
     D04 -  VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D05 -  Global VBR without underflow (H.264 low_delay_hrd_flag = 0)
     D06 -  Reserved
     D07 -  Reserved
	 */

	.wMaxMBperSecOneResolutionNoScalability 				= 243,
	.wMaxMBperSecTwoResolutionsNoScalability 				= 486,
	.wMaxMBperSecThreeResolutionsNoScalability 				= 729,
	.wMaxMBperSecFourResolutionsNoScalability 				= 0x0000,
	.wMaxMBperSecOneResolutionTemporalScalabilit 			= 243,
	.wMaxMBperSecTwoResolutionsTemporalScalablility 		= 486,
	.wMaxMBperSecThreeResolutionsTemporalScalability 		= 729,
	.wMaxMBperSecFourResolutionsTemporalScalability 		= 0x0000,
	.wMaxMBperSecOneResolutionTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalQualityScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalQualityScalablity 	= 0x0000,
	.wMaxMBperSecFourResolutionsTemporalQualityScalability	= 0x0000,
	.wMaxMBperSecOneResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecTwoResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecThreeResolutionsTemporalSpatialScalability = 0x0000,
	.wMaxMBperSecFourResolutionsTemporalSpatialScalability 	= 0x0000,
	.wMaxMBperSecOneResolutionFullScalability 				= 0x0000,
	.wMaxMBperSecTwoResolutionsFullScalability 				= 0x0000,
	.wMaxMBperSecThreeResolutionsFullScalability 			= 0x0000,
	.wMaxMBperSecFourResolutionsFullScalability 			= 0x0000,
};

/**************************************************HS**********************************************************/

static struct uvc_frame_info_t uvc_streaming_cls_hs_yuy2[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_640x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_160x120_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_176x144_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_320x180_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_320x240_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_352x288_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_424x240_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_424x320_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_480x272_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_512x288_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_640x360_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_704x576_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_800x448_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_800x600_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_848x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_960x540_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1024x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1024x768_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1280x720_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1600x896_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1600x900_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1920x1080_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_2560x1440_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_3840x2160_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_8000x6000_hs, 1, 1 },

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x1280_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1080x1920_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1440x2560_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_2160x3840_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_6000x8000_hs, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};

static struct uvc_frame_info_t uvc_streaming_cls_hs_i420[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_i420_640x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_160x120_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_176x144_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_320x180_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_320x240_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_352x288_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_424x240_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_424x320_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_480x272_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_512x288_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_640x360_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_704x576_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_800x448_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_800x600_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_848x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_960x540_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1024x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1024x768_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1280x720_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1600x896_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1600x900_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1920x1080_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_2560x1440_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_3840x2160_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_8000x6000_hs, 1, 1 },

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x1280_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1080x1920_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1440x2560_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_2160x3840_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_6000x8000_hs, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_hs_nv12[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_640x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_160x120_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_176x144_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_320x180_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_320x240_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_352x288_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_424x240_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_424x320_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_480x272_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_512x288_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_640x360_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_704x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x480_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_800x448_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_800x600_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_848x480_hs, 1, 1},
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_960x540_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1024x576_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1024x768_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1280x720_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1600x896_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1600x900_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1920x1080_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_2560x1440_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_3840x2160_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_8000x6000_hs, 1, 1 },

	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x1280_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1080x1920_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1440x2560_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_2160x3840_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_6000x8000_hs, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_hs_mjpeg[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_640x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_160x120, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_176x144, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_320x180, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_320x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_352x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_424x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_424x320, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_480x272, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_512x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_640x360, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_704x576, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_800x448, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_800x600, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_848x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_960x540, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x768, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1280x720, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x896, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x900, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1920x1080, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_2560x1440_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_3840x2160_hs, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_8000x6000_hs, 1, 1 },
#endif

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x1280, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1080x1920, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1440x2560_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_2160x3840_hs, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_6000x8000_hs, 1, 1 },

	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_hs_h264[] = {
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_640x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_160x120, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_176x144, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_320x180, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_320x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_352x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_424x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_424x320, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_480x272, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_512x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_640x360, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_704x576, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_720x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_720x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_800x448, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_800x600, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_848x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_960x540, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_8000x6000, 1, 1 },
#endif

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_720x1280, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1080x1920, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1440x2560, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_2160x3840, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_6000x8000, 1, 1 },

	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};



static struct uvc_frame_info_t uvc_streaming_cls_hs_h265[] = {
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_640x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_160x120, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_176x144, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_320x180, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_320x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_352x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_424x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_424x320, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_480x272, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_512x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_640x360, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_704x576, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_720x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_720x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_800x448, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_800x600, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_848x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_960x540, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1024x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1024x768, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1280x720, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1600x896, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1600x900, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1920x1080, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT	
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_2560x1440, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_3840x2160, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_8000x6000, 1, 1 },
#endif
	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_720x1280, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1080x1920, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_1440x2560, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_2160x3840, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc110_frame_h264_6000x8000, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};




static struct uvc_frame_info_t uvc150_streaming_cls_hs_h264_s2[] = {
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_160x120_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_176x144_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_480x272_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_704x576_s2, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2, 0, 0 },
	//{(struct uvc_descriptor_header *) &uvc150_frame_h264_8000x6000_s2, 1, 1 },
#endif

	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_640x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_160x120_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_176x144_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_320x180_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_320x240_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_352x288_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_424x240_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_424x320_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_480x272_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_512x288_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_640x360_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_704x576_s2, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_720x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_720x576_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_800x448_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_800x600_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_848x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_960x540_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x576_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_1024x768_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_1280x720_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x896_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_1600x900_s2, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_1920x1080_s2, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_2560x1440_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame2_h264_3840x2160_s2, 0, 0 },
	//{(struct uvc_descriptor_header *) &uvc150_frame2_h264_8000x6000_s2, 1, 1 },
#endif

	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};

static struct uvc_frame_info_t uvc150_streaming_cls_hs_h264_simulcast_s2[] = {

	{(struct uvc_descriptor_header *) &uvc150_frame_h264_640x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_160x120_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_176x144_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_320x180_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_320x240_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_352x288_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_424x240_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_424x320_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_480x272_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_512x288_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_640x360_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_704x576_s2, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_720x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_720x576_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_800x448_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_800x600_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_848x480_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_960x540_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1024x576_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1024x768_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1280x720_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1600x896_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1600x900_s2, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_1920x1080_s2, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_2560x1440_s2, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc150_frame_h264_3840x2160_s2, 0, 0 },
	//{(struct uvc_descriptor_header *) &uvc150_frame_h264_8000x6000_s2, 1, 1 },
#endif

	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};



/**************************************************SS**********************************************************/

static struct uvc_frame_info_t uvc_streaming_cls_ss_yuy2[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_640x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_160x120_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_176x144_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_320x180_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_320x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_352x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_424x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_424x320_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_480x272_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_512x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_640x360_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_704x576_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_800x448_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_800x600_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_848x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_960x540_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1024x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1024x768_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1280x720_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1600x896_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1600x900_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1920x1080_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_3840x2160_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_8000x6000_ss, 1, 1 },
	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x1280_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1080x1920_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};

static struct uvc_frame_info_t uvc_streaming_cls_ss_i420[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_i420_640x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_160x120_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_176x144_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_320x180_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_320x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_352x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_424x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_424x320_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_480x272_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_512x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_640x360_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_704x576_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_800x448_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_800x600_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_848x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_960x540_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1024x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1024x768_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1280x720_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1600x896_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1600x900_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1920x1080_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_3840x2160_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_8000x6000_ss, 1, 1 },
	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x1280_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1080x1920_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_ss_nv12[] = {
	
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_640x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_160x120_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_176x144_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_320x180_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_320x240_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_352x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_424x240_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_424x320_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_480x272_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_512x288_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_640x360_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_704x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_800x448_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_800x600_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_848x480_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_960x540_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1024x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1024x768_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1280x720_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1600x896_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1600x900_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1920x1080_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_3840x2160_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_8000x6000_ss, 1, 1 },

	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x1280_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1080x1920_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_ss_mjpeg[] = {

	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_640x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_160x120, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_176x144, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_320x180, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_320x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_352x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_424x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_424x320, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_480x272, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_512x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_640x360, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_704x576, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_800x448, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_800x600, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_848x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_960x540, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x768, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1280x720, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x896, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x900, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1920x1080, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_3840x2160_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_8000x6000_ss, 1, 1 },
#endif

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x1280, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1080x1920, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};

/**************************************************SSP**********************************************************/

static struct uvc_frame_info_t uvc_streaming_cls_ssp_yuy2[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_640x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_160x120_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_176x144_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_320x180_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_320x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_352x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_424x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_424x320_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_480x272_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_512x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_640x360_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_704x576_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_800x448_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_800x600_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_848x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_960x540_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1024x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1024x768_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1280x720_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1600x896_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1600x900_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1920x1080_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_3840x2160_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_8000x6000_ss, 1, 1 },
	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_720x1280_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1080x1920_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_yuv_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};

static struct uvc_frame_info_t uvc_streaming_cls_ssp_i420[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_i420_640x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_160x120_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_176x144_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_320x180_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_320x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_352x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_424x240_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_424x320_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_480x272_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_512x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_640x360_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_704x576_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_800x448_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_800x600_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_848x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_960x540_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1024x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1024x768_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1280x720_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1600x896_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1600x900_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1920x1080_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_3840x2160_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_8000x6000_ss, 1, 1 },
	
	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_i420_720x1280_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1080x1920_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_i420_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_ssp_nv12[] = {
	
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_640x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_160x120_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_176x144_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_320x180_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_320x240_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_352x288_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_424x240_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_424x320_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_480x272_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_512x288_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_640x360_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_704x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x480_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_800x448_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_800x600_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_848x480_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_960x540_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1024x576_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1024x768_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1280x720_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1600x896_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1600x900_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1920x1080_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_3840x2160_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_8000x6000_ss, 1, 1 },

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_720x1280_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1080x1920_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_nv12_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0 },
};


static struct uvc_frame_info_t uvc_streaming_cls_ssp_mjpeg[] = {

	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_640x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_160x120, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_176x144, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_320x180, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_320x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_352x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_424x240, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_424x320, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_480x272, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_512x288, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_640x360, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_704x576, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_800x448, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_800x600, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_848x480, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_960x540, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x576, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1024x768, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1280x720, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x896, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1600x900, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1920x1080, 0, 0 },

#ifdef UVC_VIDEO_FORMAT_4K_SUPPORT
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_2560x1440_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_3840x2160_ss, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_8000x6000_ss, 1, 1 },
#endif

	//竖屏分辨率
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_720x1280, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1080x1920, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_1440x2560_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_2160x3840_ss, 1, 1 },
	{(struct uvc_descriptor_header *) &uvc_frame_mjpg_6000x8000_ss, 1, 1 },
	
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },

	{NULL, 0, 0 },
};

/************************************************************************************************************/

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_yuy2= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_yuy2,
	.frame_info = uvc_streaming_cls_hs_yuy2,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_nv12= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_nv12,
	.frame_info = uvc_streaming_cls_hs_nv12,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_i420= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_i420,
	.frame_info = uvc_streaming_cls_hs_i420,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_mjpeg= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_mjpeg,
	.frame_info = uvc_streaming_cls_hs_mjpeg,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_h264= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_hs_h264,
	.frame_info = uvc_streaming_cls_hs_h264,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_h265= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_hs_h265,
	.frame_info = uvc_streaming_cls_hs_h265,
};



//USB 3.0
static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_yuy2= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_yuy2,
	.frame_info = uvc_streaming_cls_ss_yuy2,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_nv12= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_nv12,
	.frame_info = uvc_streaming_cls_ss_nv12,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_i420= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_i420,
	.frame_info = uvc_streaming_cls_ss_i420,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_mjpeg= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_mjpeg,
	.frame_info = uvc_streaming_cls_ss_mjpeg,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_h264= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ss_h264,
	.frame_info = uvc_streaming_cls_hs_h264,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_h265= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ss_h265,
	.frame_info = uvc_streaming_cls_hs_h265,
};

//USB 3.2
static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_yuy2= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_yuy2,
	.frame_info = uvc_streaming_cls_ssp_yuy2,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_i420= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_i420,
	.frame_info = uvc_streaming_cls_ssp_i420,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_nv12= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_nv12,
	.frame_info = uvc_streaming_cls_ssp_nv12,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_mjpeg= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_mjpeg,
	.frame_info = uvc_streaming_cls_ssp_mjpeg,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_h264= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ssp_h264,
	.frame_info = uvc_streaming_cls_hs_h264,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_h265= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ssp_h265,
	.frame_info = uvc_streaming_cls_hs_h265,
};


/************************************************************************************************************/

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_yuy2_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_yuy2_s2,
	.frame_info = uvc_streaming_cls_hs_yuy2,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_nv12_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_nv12_s2,
	.frame_info = uvc_streaming_cls_hs_nv12,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_i420_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_i420_s2,
	.frame_info = uvc_streaming_cls_hs_i420,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_mjpeg_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_hs_mjpeg_s2,
	.frame_info = uvc_streaming_cls_hs_mjpeg,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_h264_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_hs_h264_s2,
	.frame_info = uvc_streaming_cls_hs_h264,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_hs_h265_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_hs_h265_s2,
	.frame_info = uvc_streaming_cls_hs_h265,
};

static struct uvc_streaming_desc_attr_t uvc150_streaming_desc_hs_h264_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc150_format_hs_h264_s2,
	.frame_info = uvc150_streaming_cls_hs_h264_s2,
};

static struct uvc_streaming_desc_attr_t uvc150_streaming_desc_hs_h264_simulcast_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc150_format_hs_h264_simulcast_s2,
	.frame_info = uvc150_streaming_cls_hs_h264_simulcast_s2,
};


//USB 3.0
static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_yuy2_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_yuy2_s2,
	.frame_info = uvc_streaming_cls_ss_yuy2,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_nv12_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_nv12_s2,
	.frame_info = uvc_streaming_cls_ss_nv12,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_i420_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_i420_s2,
	.frame_info = uvc_streaming_cls_ss_i420,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_mjpeg_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ss_mjpeg_s2,
	.frame_info = uvc_streaming_cls_ss_mjpeg,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_h264_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ss_h264_s2,
	.frame_info = uvc_streaming_cls_hs_h264,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ss_h265_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ss_h265_s2,
	.frame_info = uvc_streaming_cls_hs_h265,
};

static struct uvc_streaming_desc_attr_t uvc150_streaming_desc_ss_h264_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc150_format_ss_h264_s2,
	.frame_info = uvc150_streaming_cls_hs_h264_s2,
};

static struct uvc_streaming_desc_attr_t uvc150_streaming_desc_ss_h264_simulcast_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc150_format_ss_h264_simulcast_s2,
	.frame_info = uvc150_streaming_cls_hs_h264_simulcast_s2,
};

//USB 3.2
static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_yuy2_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_yuy2_s2,
	.frame_info = uvc_streaming_cls_ssp_yuy2,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_i420_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_i420_s2,
	.frame_info = uvc_streaming_cls_ssp_i420,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_nv12_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_nv12_s2,
	.frame_info = uvc_streaming_cls_ssp_nv12,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_mjpeg_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_ssp_mjpeg_s2,
	.frame_info = uvc_streaming_cls_ssp_mjpeg,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_h264_s2 = {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ssp_h264_s2,
	.frame_info = uvc_streaming_cls_hs_h264,
};

static struct uvc_streaming_desc_attr_t uvc_streaming_desc_ssp_h265_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc110_format_ssp_h265_s2,
	.frame_info = uvc_streaming_cls_hs_h265,
};

static struct uvc_streaming_desc_attr_t uvc150_streaming_desc_ssp_h264_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc150_format_ssp_h264_s2,
	.frame_info = uvc150_streaming_cls_hs_h264_s2,
};

static struct uvc_streaming_desc_attr_t uvc150_streaming_desc_ssp_h264_simulcast_s2= {
	.format_header = (struct uvc_descriptor_header *)&uvc150_format_ssp_h264_simulcast_s2,
	.frame_info = uvc150_streaming_cls_hs_h264_simulcast_s2,
};
/************************************************for windows hello*******************************************/

static struct uvc_format_h264_base uvc_format_windows_hello_l8_ir = {
	.bLength				= UVC_DT_FORMAT_H264_BASE_SIZE,
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex			= 0,
	.bNumFrameDescriptors	= 2,
	.guidFormat				= UVC_GUID_FORMAT_L8_IR,
	.bBitsPerPixel			= 8,
	.bDefaultFrameIndex		= 0x01,
	.bAspectRatioX			= 0,
	.bAspectRatioY			= 0,
	.bmInterfaceFlags		= 0,
	.bCopyProtect			= 0,
	.bVariableSize			= 0,
};

static struct UVC_FRAME_H264_BASE(1) uvc_frame_windows_hello_l8_ir_320x320 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(1),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= 1,
	.bmCapabilities			= 0,
	.wWidth					= (320),
	.wHeight				= (320),
	.dwMinBitRate			= (320*320*30*8U),
	.dwMaxBitRate			= (320*320*30*8U),
	.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS),
	.bFrameIntervalType		= (1),
	.dwBytesPerLine         = 0x00000154,
	.dwFrameInterval[0]	= (UVC_FORMAT_30FPS),
};

static struct UVC_FRAME_H264_BASE(1) uvc_frame_windows_hello_l8_ir_640x360 = {
	.bLength				= UVC_DT_FRAME_H264_SIZE(1),
	.bDescriptorType		= USB_DT_CS_INTERFACE,
	.bDescriptorSubType		= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex			= 2,
	.bmCapabilities			= 0,
	.wWidth					= (640),
	.wHeight				= (360),
	.dwMinBitRate			= (640*360*15*8U),
	.dwMaxBitRate			= (640*360*15*8U),
	.dwDefaultFrameInterval	= (UVC_FORMAT_15FPS),
	.bFrameIntervalType		= (1),
	.dwBytesPerLine         = 0x00000154,
	.dwFrameInterval[0]	= (UVC_FORMAT_15FPS),
};

static struct uvc_frame_info_t uvc_streaming_cls_windows_hello_l8_ir[] = {
	{(struct uvc_descriptor_header *) &uvc_frame_windows_hello_l8_ir_320x320, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_frame_windows_hello_l8_ir_640x360, 0, 0 },
	{(struct uvc_descriptor_header *) &uvc_color_matching, 0, 0 },
	{NULL, 0, 0}
};


static struct uvc_streaming_desc_attr_t uvc_streaming_desc_windows_hello = {
	.format_header = (struct uvc_descriptor_header *)&uvc_format_windows_hello_l8_ir,
	.frame_info = uvc_streaming_cls_windows_hello_l8_ir,
};

/************************************************************************************************************/


/************************************************************************************************************/

static struct uvc_streaming_desc_attr_t *hs_uvc_streaming_desc_attrs_c1_s1[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_mjpeg,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_yuy2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_nv12,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_i420,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_h264,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_h265,
	NULL,
};


static struct uvc_streaming_desc_attr_t *ss_uvc_streaming_desc_attrs_c1_s1[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_yuy2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_nv12,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_i420,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_mjpeg,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_h264,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_h265,
	NULL,
};

static struct uvc_streaming_desc_attr_t *ssp_uvc_streaming_desc_attrs_c1_s1[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_yuy2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_nv12,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_i420,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_mjpeg,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_h264,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_h265,
	NULL,
};

static struct uvc_streaming_desc_attr_t *hs_uvc_streaming_desc_attrs_c1_s2[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_mjpeg_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_yuy2_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_nv12_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_i420_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_h264_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_h265_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_windows_hello,
	NULL,
};

static struct uvc_streaming_desc_attr_t *ss_uvc_streaming_desc_attrs_c1_s2[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_yuy2_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_nv12_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_i420_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_mjpeg_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_h264_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_h265_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_windows_hello,
	NULL,
};

static struct uvc_streaming_desc_attr_t *ssp_uvc_streaming_desc_attrs_c1_s2[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_yuy2_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_nv12_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_i420_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_mjpeg_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_h264_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_h265_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_windows_hello,
	NULL,
};

static struct uvc_streaming_desc_attr_t *hs_uvc_streaming_desc_attrs_c2_s1[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_mjpeg,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_yuy2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_nv12,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_hs_i420,
	NULL,
};

static struct uvc_streaming_desc_attr_t *ss_uvc_streaming_desc_attrs_c2_s1[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_yuy2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_nv12,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_i420,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ss_mjpeg,
	NULL,
};

static struct uvc_streaming_desc_attr_t *ssp_uvc_streaming_desc_attrs_c2_s1[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_yuy2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_nv12,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_i420,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_ssp_mjpeg,
	NULL,
};

static struct uvc_streaming_desc_attr_t *hs_uvc_streaming_desc_attrs_c2_s2[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc150_streaming_desc_hs_h264_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc150_streaming_desc_hs_h264_simulcast_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_windows_hello,
	NULL,
};


static struct uvc_streaming_desc_attr_t *ss_uvc_streaming_desc_attrs_c2_s2[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc150_streaming_desc_ss_h264_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc150_streaming_desc_ss_h264_simulcast_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_windows_hello,
	NULL,
};

static struct uvc_streaming_desc_attr_t *ssp_uvc_streaming_desc_attrs_c2_s2[] = {
	(struct uvc_streaming_desc_attr_t *)&uvc150_streaming_desc_ssp_h264_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc150_streaming_desc_ssp_h264_simulcast_s2,
	(struct uvc_streaming_desc_attr_t *)&uvc_streaming_desc_windows_hello,
	NULL,
};

/************************************************************************************************************/


static inline int uvc_format_to_v4l2_fcc(const struct uvc_format_header *format_header, uint32_t *fcc)
{
	const struct uvc_format_uncompressed *format_uncompressed;
	const struct uvc_format_h264_base *uvc_format_base;

	if(format_header->bDescriptorSubType == UVC_VS_FORMAT_UNCOMPRESSED)
	{
		format_uncompressed = (const struct uvc_format_uncompressed *)format_header;
		
		if (format_uncompressed->guidFormat[0] == 'N' &&
			format_uncompressed->guidFormat[1] == 'V' &&
			format_uncompressed->guidFormat[2] == '1' &&
			format_uncompressed->guidFormat[3] == '2')
			*fcc = V4L2_PIX_FMT_NV12;
		else if (format_uncompressed->guidFormat[0] == 'I' &&
			format_uncompressed->guidFormat[1] == '4' &&
			format_uncompressed->guidFormat[2] == '2' &&
			format_uncompressed->guidFormat[3] == '0')
			*fcc = V4L2_PIX_FMT_YUV420;
		else
			*fcc = V4L2_PIX_FMT_YUYV;
	}
	else if(format_header->bDescriptorSubType == UVC_VS_FORMAT_MJPEG)
	{
		*fcc = V4L2_PIX_FMT_MJPEG;
	}
	else if(format_header->bDescriptorSubType == UVC_VS_FORMAT_FRAME_BASED)
	{
		uvc_format_base = (const struct uvc_format_h264_base *)format_header;

		if(uvc_format_base->guidFormat[0] == 'H' && uvc_format_base->guidFormat[1] == '2' && 
			uvc_format_base->guidFormat[2] == '6' && uvc_format_base->guidFormat[3] == '5')
		{
			*fcc = V4L2_PIX_FMT_H265;
		}
		else if(uvc_format_base->guidFormat[0] == 'H' && uvc_format_base->guidFormat[1] == 'E' && 
			uvc_format_base->guidFormat[2] == 'V' && uvc_format_base->guidFormat[3] == 'C')
		{
			*fcc = V4L2_PIX_FMT_H265;
		}
		else if(uvc_format_base->guidFormat[0] == 0x32 && uvc_format_base->guidFormat[1] == 0x00 && 
			uvc_format_base->guidFormat[2] == 0x00 && uvc_format_base->guidFormat[3] == 0x00)
		{
			*fcc = V4L2_PIX_FMT_L8_IR;
		}
		else if(uvc_format_base->guidFormat[0] == 0x51 && uvc_format_base->guidFormat[1] == 0x00 && 
			uvc_format_base->guidFormat[2] == 0x00 && uvc_format_base->guidFormat[3] == 0x00)
		{
			*fcc = V4L2_PIX_FMT_L16_IR;
		}
		else if(uvc_format_base->guidFormat[0] == 0x4d && uvc_format_base->guidFormat[1] == 0x4a && 
			uvc_format_base->guidFormat[2] == 0x50 && uvc_format_base->guidFormat[3] == 0x47)
		{
			*fcc = V4L2_PIX_FMT_MJPEG_IR;
		}
		else
		{
			*fcc = V4L2_PIX_FMT_H264;
		}
	}
	else if(format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_V150)
	{
		//ucam_strace("UVC1.5 H264");
		*fcc = V4L2_PIX_FMT_H264;
	}
	else if(format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_SIMULCAST)
	{
		//ucam_strace("UVC1.5 H264 SIMULCAST");
		*fcc = V4L2_PIX_FMT_H264;
	}
	else
	{
		ucam_notice("format error! bDescriptorSubType : 0x%x\n",format_header->bDescriptorSubType);
		return -1;
	}

	//ucam_strace("UVC bFormatIndex:%d ----> V4L2 %s", format_header->bFormatIndex, uvc_video_format_string(*fcc));

	return 0;
}

static inline int get_uvc_frame_width_height(const struct uvc_frame_header *frame_header, uint16_t *width, uint16_t *height)
{
	const struct uvc_frame_uncompressed_header *frame_uncompressed_header;
	const struct uvc_frame_h264_v150_header *frame_h264_v150_header;

	if(frame_header == NULL)
	{
		return -1;
	}

	if(frame_header->bDescriptorSubType == UVC_VS_FRAME_H264_V150)
	{
		frame_h264_v150_header = (const struct uvc_frame_h264_v150_header *)frame_header;
		*width = frame_h264_v150_header->wWidth;
		*height = frame_h264_v150_header->wHeight;	
	}
	else
	{
		frame_uncompressed_header = (const struct uvc_frame_uncompressed_header *)frame_header;
		*width = frame_uncompressed_header->wWidth;
		*height = frame_uncompressed_header->wHeight;
	}

	return 0;
}

int get_uvc_format_desc_is_enable(struct uvc_dev *dev, const struct uvc_format_header *format_header, int *enable)
{
	uint32_t fcc;

	if(uvc_format_to_v4l2_fcc(format_header, &fcc) != 0)
	{
		return -1;
	}

	switch(fcc){
		case V4L2_PIX_FMT_H264:
			if(dev->bConfigurationValue == 2)
			{	
				if(format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_SIMULCAST)
				{
					*enable = dev->config.uvc150_simulcast_en;
				}
				else
				{
					*enable = 1;
				}		
			}
			else
			{
				*enable = dev->config.uvc110_h264_enable;
			}
			break;
		case V4L2_PIX_FMT_MJPEG:
			*enable = dev->config.uvc_mjpeg_enable;
			break;	
		case V4L2_PIX_FMT_YUYV:
			*enable = dev->config.uvc_yuy2_enable;
			break;
		case V4L2_PIX_FMT_NV12:
			*enable = dev->config.uvc_nv12_enable;
			break;
		case V4L2_PIX_FMT_YUV420:
			*enable = dev->config.uvc_i420_enable;
			break;
		case V4L2_PIX_FMT_H265:
			*enable = dev->config.uvc110_h265_enable;
			break;
		case V4L2_PIX_FMT_L8_IR:
		case V4L2_PIX_FMT_L16_IR:
		case V4L2_PIX_FMT_MJPEG_IR:
			*enable = dev->config.windows_hello_enable;
			break;
		default: 
			return -1;
	}

	return 0;
}

#define SET_UVC_FRAME_MIN_BITRATE(FRAME,FPS) (FRAME->dwMinBitRate = FRAME->wWidth * FRAME->wHeight * 2 * 8 * FPS)
#define SET_UVC_FRAME_MAX_BITRATE(FRAME,FPS) (FRAME->dwMaxBitRate = FRAME->wWidth * FRAME->wHeight * 2 * 8 * FPS)


#define GET_UVC_FRAME_INTERVAL_MIN_MAX(FRAME) \
{\
	fps = (uint32_t)(10000000.0f/FRAME->dwFrameInterval[0]);\
	if(fps_max > fps)\
	{\
		fps_max = fps;\
	}\
	if(fps_min > fps_max)\
	{\
		fps_min = fps_max;\
	}\
	if (fps_min <= 0 || fps_max <= 0)\
	{\
		ucam_error("fps_min(%d) or fps_max(%d) error!",fps_min, fps_max);\
		return -1;\
	}\
	for(i = 0; i < uvc_fps_list_num; i ++)\
	{\
		if(fps_max <= uvc_fps_list[i][0])\
		{\
			format_index_min = i;\
		}\
		if(fps_min <= uvc_fps_list[i][0])\
		{\
			format_index_max = i;\
		}\
	}\
	if (format_index_min < 0 || format_index_max < 0 || format_index_min > format_index_max)\
	{\
		ucam_error("fps_min(%d) or fps_max(%d) error!",fps_min, fps_max);\
		return -1;\
	}\
}

#define SET_UVC_FRAME_INTERVAL(FRAME)\
{\
	GET_UVC_FRAME_INTERVAL_MIN_MAX(FRAME);\
	if(uvc_fps_list[format_index_min][0] >= 30)\
	{FRAME->dwDefaultFrameInterval = UVC_FORMAT_30FPS;}\
	else\
	{FRAME->dwDefaultFrameInterval = uvc_fps_list[format_index_min][1];}\
	j=0;\
	for(i = format_index_min; i <= format_index_max; i++)\
	{\
		FRAME->dwFrameInterval[j++] = uvc_fps_list[i][1];\
		if(def_fps != 0 && def_fps == uvc_fps_list[i][0]) {\
				FRAME->dwDefaultFrameInterval = uvc_fps_list[i][1];\
		}\
	};\
	SET_UVC_FRAME_MIN_BITRATE(FRAME, uvc_fps_list[format_index_max][0]);\
	SET_UVC_FRAME_MAX_BITRATE(FRAME, uvc_fps_list[format_index_min][0]);\
}



int set_uvc_frame_fps(struct uvc_dev *dev, enum ucam_usb_device_speed speed, 
	struct uvc_frame_header *frame_header, uint32_t fps_min, uint32_t fps_max)
{
	struct UVC_FRAME_UNCOMPRESSED(10) *uvc_frame_uncompressed; //YUY2 NV12 
	struct UVC_FRAME_MJPEG(10) *uvc_frame_mjpeg;//MJPEG
	struct UVC_FRAME_H264_BASE(10) *uvc_frame_h264_base;//UVC1.1 H264
	struct UVC_FRAME_H264_V150(10) *uvc_frame_h264_v150;//UVC1.5 H264

	int i,j;
	int format_index_min = -1;
	int format_index_max = -1;
	uint32_t fps;
	uint32_t def_fps;
	uint32_t uvc_fps_list[UVC_FPS_LIST_MAX][2];
	int uvc_fps_list_num;

	if(speed == UCAM_USB_SPEED_SUPER_PLUS)
	{
		def_fps = g_def_fps[2];
	}
	else if(speed == UCAM_USB_SPEED_SUPER)
	{
		def_fps = g_def_fps[1];
	}
	else
	{
		def_fps = g_def_fps[0];
	}

	uvc_fps_list_num = 0;
	for(i = 0; i < (sizeof(g_uvc_fps_list)/sizeof(struct uvc_fps_list_t)); i++)
	{
		if(!g_uvc_fps_list[i].bypass)
		{
			uvc_fps_list[uvc_fps_list_num][0] = g_uvc_fps_list[i].fps;
			uvc_fps_list[uvc_fps_list_num][1] = g_uvc_fps_list[i].interval;
			uvc_fps_list_num ++;
			if(uvc_fps_list_num > UVC_FPS_LIST_MAX)
			{
				return -1;
			}
		}	
	}

	if(uvc_fps_list_num == 0)
	{
		return -1;
	}

	switch (frame_header->bDescriptorSubType)
	{
	case UVC_VS_FRAME_UNCOMPRESSED://YUY2 NV12
		uvc_frame_uncompressed = (struct UVC_FRAME_UNCOMPRESSED(10) *)(frame_header);
		if(uvc_frame_uncompressed->dwFrameInterval[0] > uvc_fps_list[uvc_fps_list_num - 1][1])
		{
			//个别帧率低于最低限制帧率，不处理了
			break;
		}
		SET_UVC_FRAME_INTERVAL(uvc_frame_uncompressed);	
		uvc_frame_uncompressed->bFrameIntervalType = format_index_max - format_index_min + 1;
		uvc_frame_uncompressed->bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(uvc_frame_uncompressed->bFrameIntervalType);
		break;
	case UVC_VS_FRAME_MJPEG://MJPEG
		uvc_frame_mjpeg = (struct UVC_FRAME_MJPEG(10) *)(frame_header);
		if(uvc_frame_mjpeg->dwFrameInterval[0] > uvc_fps_list[uvc_fps_list_num - 1][1])
		{
			//个别帧率低于最低限制帧率，不处理了
			break;
		}
		SET_UVC_FRAME_INTERVAL(uvc_frame_mjpeg);
		uvc_frame_mjpeg->bFrameIntervalType = format_index_max - format_index_min + 1;
		uvc_frame_mjpeg->bLength = UVC_DT_FRAME_MJPEG_SIZE(uvc_frame_mjpeg->bFrameIntervalType);
		break;
	case UVC_VS_FRAME_FRAME_BASED://UVC1.1 H264
		uvc_frame_h264_base = (struct UVC_FRAME_H264_BASE(10) *)(frame_header);
		if(uvc_frame_h264_base->dwFrameInterval[0] > uvc_fps_list[uvc_fps_list_num - 1][1])
		{
			//个别帧率低于最低限制帧率，不处理了
			break;
		}
		SET_UVC_FRAME_INTERVAL(uvc_frame_h264_base);
		uvc_frame_h264_base->bFrameIntervalType = format_index_max - format_index_min + 1;
		uvc_frame_h264_base->bLength = UVC_DT_FRAME_H264_SIZE(uvc_frame_h264_base->bFrameIntervalType);
		break;
	case UVC_VS_FRAME_H264_V150://UVC1.5 H264
		uvc_frame_h264_v150 = (struct UVC_FRAME_H264_V150(10) *)(frame_header);
		if(uvc_frame_h264_v150->dwFrameInterval[0] > uvc_fps_list[uvc_fps_list_num - 1][1])
		{
			//个别帧率低于最低限制帧率，不处理了
			break;
		}
		SET_UVC_FRAME_INTERVAL(uvc_frame_h264_v150);
		uvc_frame_h264_v150->bNumFrameIntervals = format_index_max - format_index_min + 1;
		uvc_frame_h264_v150->bLength = UVC_DT_FRAME_H264_V150_SIZE(uvc_frame_h264_v150->bNumFrameIntervals);
		break;	
	default:
		ucam_error("error, bDescriptorSubType: 0x%x", frame_header->bDescriptorSubType);
		return -1;
		break;
	}

	return 0;
}

/**
 * @description: 设置第一个视频格式的默认分辨率
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_uvc_default_frame_index(
	struct uvc_dev *dev, 
	const struct uvc_descriptor_header * const *uvc_streaming_cls, 
	enum ucam_usb_device_speed speed,
	uint16_t def_width,
	uint16_t def_height)
{
	const struct usb_descriptor_header * const *src = NULL;
	const struct uvc_format_header * format_header;
	const struct uvc_frame_header * frame_header;

	uint32_t fcc;
	uint16_t Width = 0;
	uint16_t Height = 0;
	int FrameIndex = -1;

	struct uvc_format_uncompressed *format_uncompressed;
	struct uvc_format_mjpeg *format_mjpeg;
	struct uvc_format_h264_base *format_h264;
	struct uvc_format_h264_v150 *format_h264_v150;


	ucam_strace("enter");

	if(!dev || !uvc_streaming_cls)
	{
		return -1;
	}

	for (src = (const struct usb_descriptor_header **)uvc_streaming_cls;
	     *src; ++src) 
	{
		format_header = (const struct uvc_format_header *)*src;	
		if(format_header->bDescriptorSubType == UVC_VS_FORMAT_UNCOMPRESSED ||
			format_header->bDescriptorSubType == UVC_VS_FORMAT_MJPEG || 
			format_header->bDescriptorSubType == UVC_VS_FORMAT_FRAME_BASED || 
			format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_V150 ||
			format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_SIMULCAST
		)
		{
			if(format_header->bFormatIndex == 1)
			{
				//先找到视频格式索引，再找分辨率索引
				++src;
				break;		
			}
		}	
	}

	if(*src == NULL)
	{
		ucam_error("error!\n");
		return -1;
	}
	
	if(uvc_format_to_v4l2_fcc(format_header, &fcc) != 0)
	{
		ucam_error("uvc_format_to_v4l2_fcc  error!\n");
		return -1;
	}
	


	FrameIndex = format_header->bNumFrameDescriptors;
	//找分辨率索引
	for (; *src; ++src)
	{
		frame_header = (const struct uvc_frame_header *)*src;
		if(frame_header->bDescriptorSubType != UVC_VS_COLORFORMAT)
		{
			if(get_uvc_frame_width_height(frame_header, &Width, &Height) != 0)
			{
				return -1;
			}
			FrameIndex = frame_header->bFrameIndex;

			if(def_width > 0 && def_height > 0)
			{
				//默认设置，没有就使用最后一个分辨率
				if(Width == def_width && Height == def_height)
				{			
					break;
				}
			}
			else
			{
			
				if(speed < UCAM_USB_SPEED_SUPER && format_header->bDescriptorSubType == UVC_VS_FORMAT_UNCOMPRESSED)
				{
					//USB 2.0 YUY2默认为640x480P，没有就使用最后一个分辨率
					if(Width == 640 && Height == 480)
					{					
						break;
					}
				}
				else
				{
					//默认为1080P，没有就使用最后一个分辨率
					if(Width == 1920 && Height == 1080)
					{			
						break;
					}
				}
			}		
		}
		else if(frame_header->bDescriptorSubType == UVC_VS_COLORFORMAT)
		{
			//已到分辨率描述符末尾
			break;
		}
	}

	if(Width == 0 || Height == 0)
	{
		ucam_error("uvc_vs_ctrl_analysis  error!\n");
		return -1;
	}

	switch (format_header->bDescriptorSubType)
	{
	case UVC_VS_FORMAT_UNCOMPRESSED:
		format_uncompressed = (struct uvc_format_uncompressed *)format_header;
		format_uncompressed->bDefaultFrameIndex = FrameIndex;
		break;
	case UVC_VS_FORMAT_MJPEG:
		format_mjpeg = (struct uvc_format_mjpeg *)format_header;
		format_mjpeg->bDefaultFrameIndex = FrameIndex;
		break;
	case UVC_VS_FORMAT_FRAME_BASED:
		format_h264 = (struct uvc_format_h264_base *)format_header;
		format_h264->bDefaultFrameIndex = FrameIndex;
		break;
	case UVC_VS_FORMAT_H264_V150:
	case UVC_VS_FORMAT_H264_SIMULCAST:
		format_h264_v150 = (struct uvc_format_h264_v150 *)format_header;
		format_h264_v150->bDefaultFrameIndex = FrameIndex;
		break;														
	default:
		return -1;
		break;
	}	


	ucam_notice("set %s uvc default frame: %s-%ux%u\n",
		usb_speed_string(speed), uvc_video_format_string(fcc), Width, Height);


	return 0;
}

static inline struct uvc_descriptor_header **
uvc_copy_controls_descriptors(struct uvc_dev *dev, enum ucam_usb_device_speed speed)
{
	const struct uvc_descriptor_header * const *src;
	const struct uvc_descriptor_header ** uvc_controls_desc_config;

	unsigned int n_desc = 0;
	unsigned int bytes = 0;

	struct uvc_descriptor_header **dst;
	struct uvc_descriptor_header **hdr;
	void *mem;
	uint32_t mem_size;

	switch (speed) {
		case UCAM_USB_SPEED_SUPER_PLUS:
			uvc_controls_desc_config = dev->ssp_controls_desc_config;
			break;
		case UCAM_USB_SPEED_SUPER:
			uvc_controls_desc_config = dev->ss_controls_desc_config;
			break;
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			uvc_controls_desc_config = dev->hs_controls_desc_config;
			break;
	}

	if(uvc_controls_desc_config == NULL)
	{
		ucam_error("uvc_controls_desc_config == NULL error");
		return NULL;
	}

	for (src = (const struct uvc_descriptor_header **)uvc_controls_desc_config;
			*src; ++src) 
	{
		bytes += (*src)->bLength;
		n_desc++;
	}

	mem_size = (n_desc + 1) * sizeof(struct uvc_descriptor_header *) + bytes;
	mem = malloc(mem_size);
	if (mem == NULL)
	{
		ucam_error("malloc error");
		return NULL;
	}

	//二级指针以NULL为结束
	memset(mem, 0, mem_size);	

	hdr = mem;
	dst = mem;
	mem += (n_desc + 1) * sizeof(struct uvc_descriptor_header *);
	
	UVC_COPY_DESCRIPTORS(mem, dst, 
		(const struct usb_descriptor_header * const *)uvc_controls_desc_config);

	return hdr;
}

static inline struct uvc_descriptor_header **
uvc_copy_streaming_descriptors(struct uvc_dev *dev, enum ucam_usb_device_speed speed)
{
	const struct uvc_descriptor_header * const *src;
	const struct uvc_frame_info_t  *src2;
	const struct uvc_streaming_desc_attr_t ** uvc_streaming_desc_attrs;
	struct uvc_streaming_desc_attr_t *uvc_streaming_desc_attr;
	int uvc_format_desc_enable;
	struct uvc_format_header *format_header;
	struct uvc_frame_header *frame_header;
	//const struct uvc_frame_uncompressed_header *frame_uncompressed_header;
	//const struct uvc_frame_h264_v150_header *frame_h264_v150_header;
	uint32_t fcc;
	unsigned int n_format = 0;
	unsigned int n_frame = 0;
	uint16_t width, height;
	unsigned int n_desc = 0;
	unsigned int bytes = 0;


	struct uvc_descriptor_header **dst;
	struct uvc_descriptor_header **hdr;
	void *mem;
	void *mem_temp;
	uint32_t mem_size;

	ucam_strace("usb:%s", usb_speed_string(speed));

	switch (speed) {
		case UCAM_USB_SPEED_SUPER_PLUS:
			uvc_streaming_desc_attrs = dev->ssp_uvc_streaming_desc_attrs;
			break;
		case UCAM_USB_SPEED_SUPER:
			uvc_streaming_desc_attrs = dev->ss_uvc_streaming_desc_attrs;
			break;
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			uvc_streaming_desc_attrs = dev->hs_uvc_streaming_desc_attrs;
			break;
	}

	if(uvc_streaming_desc_attrs == NULL)
	{
		ucam_error("uvc_streaming_desc_attrs == NULL error");
		return NULL;
	}

	for (src = (const struct uvc_descriptor_header **)uvc_streaming_desc_attrs; *src; ++src)
    {
		uvc_streaming_desc_attr = (struct uvc_streaming_desc_attr_t *)*src;
        if(uvc_streaming_desc_attr->format_header)
        {
			format_header = (struct uvc_format_header *)uvc_streaming_desc_attr->format_header;	
			if(uvc_format_to_v4l2_fcc(format_header, &fcc) != 0)
			{
				ucam_error("uvc_format_to_v4l2_fcc error");
				return NULL;
			}
			if(get_uvc_format_desc_is_enable(dev, format_header, &uvc_format_desc_enable) != 0)
			{
				ucam_error("get_uvc_format_desc_is_enable error");
				return NULL;
			}
			if(uvc_format_desc_enable)
			{
				n_format ++;
				format_header->bFormatIndex = n_format;
				bytes += format_header->bLength;
				n_desc ++;
				n_frame = 0;


				ucam_strace("add %s FormatIndex:%d", uvc_video_format_string(fcc), format_header->bFormatIndex);

				for (src2 = uvc_streaming_desc_attr->frame_info; src2->frame_header; src2++)
				{
					frame_header = (struct uvc_frame_header *)src2->frame_header;
					
					if(src2->bypass)
					{
						continue;
					}

					//使能分辨率设置
					if(frame_header->bDescriptorSubType != UVC_VS_COLORFORMAT)
					{
						if(get_uvc_frame_width_height(frame_header, &width, &height) != 0)
						{
							ucam_error("get_uvc_frame_width_height error");
							return NULL;
						}


						if (fcc == V4L2_PIX_FMT_YUYV || 
							fcc == V4L2_PIX_FMT_NV12 || 
							fcc == V4L2_PIX_FMT_YUV420) 
						{
							if(dev->uvc->config.chip_id ==  0x3518E300 && dev->uvc->uvc_s2_enable)
							{
								//hi3518ev300双流时限制YUY2的分辨率
								if(width <= 640 && height <= 480 &&
									width >= dev->config.yuv_resolution_width_min &&
									height >= dev->config.yuv_resolution_height_min)

								{
									n_frame++;
									frame_header->bFrameIndex = n_frame;

									bytes += frame_header->bLength;
									n_desc ++;
								}
							}
							else if(width <= dev->config.yuv_resolution_width_max && 
								height <= dev->config.yuv_resolution_height_max &&
								width >= dev->config.yuv_resolution_width_min &&
								height >= dev->config.yuv_resolution_height_min)
							{
								n_frame++;
								frame_header->bFrameIndex = n_frame;

								bytes += frame_header->bLength;
								n_desc ++;
							}
						}
						else if(width <= dev->config.resolution_width_max && 
							height <= dev->config.resolution_height_max &&
							width >= dev->config.resolution_width_min &&
							height >= dev->config.resolution_height_min)
						{
							n_frame++;
							frame_header->bFrameIndex = n_frame;

							bytes += frame_header->bLength;
							n_desc ++;
						}
					}
					else
					{
						//color_matching
						bytes += (src2->frame_header)->bLength;
						n_desc ++;
					}
				}
				format_header->bNumFrameDescriptors = n_frame;
			}
        }
    }

	uvc_streaming_header.bNumFormats = n_format;
	uvc_streaming_header.bLength	 = UVC_DT_INPUT_HEADER_SIZE(1, uvc_streaming_header.bNumFormats);
	if((dev->uvc->uvc_s2_enable & 0x80) && 
		dev->stream_num != 0 && dev->bConfigurationValue != 2)
	{
		uvc_streaming_header.bTerminalLink 	= UVC_ENTITY_ID_OUTPUT_TERMINAL | 0x80;
	}
	else
	{
		uvc_streaming_header.bTerminalLink 	= dev->stream_num ? UVC_ENTITY_ID_OUTPUT_ENCODING:UVC_ENTITY_ID_OUTPUT_TERMINAL;
	}
	

	bytes += uvc_streaming_header.bLength;
	n_desc ++;


	ucam_strace("Copy the %s descriptors, bytes:%d", usb_speed_string(speed), bytes);

	mem_size = (n_desc + 1) * sizeof(struct uvc_descriptor_header *) + bytes;
	mem = malloc(mem_size);
	if (mem == NULL)
	{
		ucam_error("malloc error");
		return NULL;
	}

	//二级指针以NULL为结束
	memset(mem, 0, mem_size);	

	hdr = mem;
	dst = mem;
	mem += (n_desc + 1) * sizeof(struct uvc_descriptor_header *);

	

	UVC_COPY_DESCRIPTOR(mem, dst, &uvc_streaming_header);

	for (src = (const struct uvc_descriptor_header **)uvc_streaming_desc_attrs; *src; ++src)
	{
		uvc_streaming_desc_attr = (struct uvc_streaming_desc_attr_t *)*src;
		if(uvc_streaming_desc_attr->format_header)
        {
			format_header = (struct uvc_format_header *)uvc_streaming_desc_attr->format_header;	
			if(uvc_format_to_v4l2_fcc(format_header, &fcc) != 0)
			{
				ucam_error("uvc_format_to_v4l2_fcc error");
				return NULL;
			}
			if(get_uvc_format_desc_is_enable(dev, format_header, &uvc_format_desc_enable) != 0)
			{
				ucam_error("get_uvc_format_desc_is_enable error");
				goto copy_error;
			}

			if(uvc_format_desc_enable)
			{
				UVC_COPY_DESCRIPTOR(mem, dst, (const struct usb_descriptor_header*)uvc_streaming_desc_attr->format_header);
				//UVC_COPY_DESCRIPTORS(mem, dst,
				//	(const struct usb_descriptor_header**)uvc_streaming_desc_attr->frame_header);

				for (src2 = uvc_streaming_desc_attr->frame_info; src2->frame_header; src2++)
				{
					frame_header = (struct uvc_frame_header *)src2->frame_header;
					
					if(src2->bypass)
					{
						continue;
					}

					//使能分辨率设置
					if(frame_header->bDescriptorSubType != UVC_VS_COLORFORMAT)
					{
						if(get_uvc_frame_width_height(frame_header, &width, &height) != 0)
						{
							ucam_error("get_uvc_frame_width_height error");
							goto copy_error;
						}

						if (fcc == V4L2_PIX_FMT_YUYV || 
							fcc == V4L2_PIX_FMT_NV12 ||
							fcc == V4L2_PIX_FMT_YUV420)
						{
							if(dev->uvc->config.chip_id ==  0x3518E300 && dev->uvc->uvc_s2_enable)
							{
								//hi3518ev300双流时限制YUY2的分辨率
								if(width <= 640 && height <= 480 &&
									width >= dev->config.yuv_resolution_width_min &&
									height >= dev->config.yuv_resolution_height_min)
								{
									mem_temp = mem;
									UVC_COPY_DESCRIPTOR(mem, dst, (const struct usb_descriptor_header*)frame_header);
									if(g_limit_fps_width != 0 && g_limit_fps_height != 0 && g_limit_fps_max != 0
										&& (width * height > g_limit_fps_width * g_limit_fps_height))
									{
										if(set_uvc_frame_fps(dev, speed, (struct uvc_frame_header *)mem_temp, dev->config.yuv_fps_min, g_limit_fps_max) != 0)
										{
											ucam_error("set_uvc_frame_fps error");
											goto copy_error;
										}
									}
									else
									{
										if(set_uvc_frame_fps(dev, speed, (struct uvc_frame_header *)mem_temp, dev->config.yuv_fps_min, dev->config.yuv_fps_max) != 0)
										{
											ucam_error("set_uvc_frame_fps error");
											goto copy_error;
										}
									}
								}
							}
							else if(width <= dev->config.yuv_resolution_width_max && 
								height <= dev->config.yuv_resolution_height_max &&
								width >= dev->config.yuv_resolution_width_min &&
								height >= dev->config.yuv_resolution_height_min)
							{
								mem_temp = mem;
								UVC_COPY_DESCRIPTOR(mem, dst, (const struct usb_descriptor_header*)frame_header);
								if(g_limit_fps_width != 0 && g_limit_fps_height != 0 && g_limit_fps_max != 0
										&& (width * height > g_limit_fps_width * g_limit_fps_height))
								{
									if(set_uvc_frame_fps(dev, speed, (struct uvc_frame_header *)mem_temp, dev->config.yuv_fps_min, g_limit_fps_max) != 0)
									{
										ucam_error("set_uvc_frame_fps error");
										goto copy_error;
									}
								}
								else
								{
									if(set_uvc_frame_fps(dev, speed, (struct uvc_frame_header *)mem_temp, dev->config.yuv_fps_min, dev->config.yuv_fps_max) != 0)
									{
										ucam_error("set_uvc_frame_fps error");
										goto copy_error;
									}
								}
							}
						}
						else if(width <= dev->config.resolution_width_max && 
							height <= dev->config.resolution_height_max &&
							width >= dev->config.resolution_width_min &&
							height >= dev->config.resolution_height_min)
						{
							mem_temp = mem;
							UVC_COPY_DESCRIPTOR(mem, dst, (const struct usb_descriptor_header*)frame_header);
							if(fcc != V4L2_PIX_FMT_L8_IR 
								&& fcc != V4L2_PIX_FMT_L16_IR
								&& fcc != V4L2_PIX_FMT_MJPEG_IR)
							{
								if(g_limit_fps_width != 0 && g_limit_fps_height != 0 && g_limit_fps_max != 0
										&& (width * height > g_limit_fps_width * g_limit_fps_height))
								{
									if(set_uvc_frame_fps(dev, speed, (struct uvc_frame_header *)mem_temp, dev->config.fps_min, g_limit_fps_max) != 0)
									{
										ucam_error("set_uvc_frame_fps error");
										goto copy_error;
									}
								}
								else
								{
									if(set_uvc_frame_fps(dev, speed, (struct uvc_frame_header *)mem_temp, dev->config.fps_min, dev->config.fps_max) != 0)
									{
										ucam_error("set_uvc_frame_fps error");
										goto copy_error;
									}
								}
							}
						}
					}
					else
					{
						UVC_COPY_DESCRIPTOR(mem, dst, (const struct usb_descriptor_header*)(src2->frame_header));
					}
				}
			}
		}
	}
	
	if(speed == UCAM_USB_SPEED_SUPER_PLUS)
	{
		if(set_uvc_default_frame_index(dev, 
		 	(const struct uvc_descriptor_header * const *)hdr, 
		 	speed, g_def_width[2], g_def_height[2]) != 0)
		{
			ucam_error("set_uvc_default_frame_index  error!\n");
			return NULL;
		}
	}
	else if(speed == UCAM_USB_SPEED_SUPER)
	{
		if(set_uvc_default_frame_index(dev, 
		 	(const struct uvc_descriptor_header * const *)hdr, 
		 	speed, g_def_width[1], g_def_height[1]) != 0)
		{
			ucam_error("set_uvc_default_frame_index  error!\n");
			return NULL;
		}
	}
	else
	{
		if(set_uvc_default_frame_index(dev, 
		 	(const struct uvc_descriptor_header * const *)hdr, 
			speed, g_def_width[0], g_def_height[0]) != 0)
		{
			ucam_error("set_uvc_default_frame_index  error!\n");
			return NULL;
		}
	}

	return hdr;

copy_error:
	if(hdr)
		free(hdr);
	
	return NULL;
}

const struct uvc_descriptor_header **
get_uvc_control_descriptors_config(struct uvc_dev *dev, enum ucam_usb_device_speed speed)
{

	int uvc_s2_dev_show = dev->uvc->uvc_s2_enable & 0x80;
	
	if (uvc_s2_dev_show && dev->stream_num != 0 && dev->bConfigurationValue != 2)
	{
		return (const struct uvc_descriptor_header **)uvc_control_cls_s2_show;
	}

	switch (speed) {
	case UCAM_USB_SPEED_SUPER_PLUS:
	case UCAM_USB_SPEED_SUPER:
		if(dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable)
		{
			return (const struct uvc_descriptor_header **)uvc150_ss_control_cls;
		}
		else if(dev->uvc->uvc_s2_enable && !uvc_s2_dev_show)
		{
			return (const struct uvc_descriptor_header **)uvc_ss_control_cls_s2;
		}
		else
		{
			return (const struct uvc_descriptor_header **)uvc_ss_control_cls;
		}

	case UCAM_USB_SPEED_HIGH:
	case UCAM_USB_SPEED_FULL:
	default:
		if(dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable)
		{
			return (const struct uvc_descriptor_header **)uvc150_fs_control_cls;
		}
		else if(dev->uvc->uvc_s2_enable && !uvc_s2_dev_show)
		{	
			return (const struct uvc_descriptor_header **)uvc_fs_control_cls_s2;
		}
		else
		{
			return (const struct uvc_descriptor_header **)uvc_fs_control_cls;
		}
	}
}

int uvc_controls_desc_copy(struct uvc_dev *dev)
{
	uvc_processing.bUnitID = dev->config.processing_unit_id;
	uvc_xu_camera_terminal.baSourceID = dev->config.processing_unit_id;
	uvc_xu_h264.baSourceID = dev->config.processing_unit_id;
	uvc_xu_h264_s2.baSourceID = dev->config.processing_unit_id;
	uvc_output_terminal.bSourceID = dev->config.processing_unit_id;
	uvc_output_terminal_s2.bSourceID = dev->config.processing_unit_id;
	uvc150_encoding.bSourceID = dev->config.processing_unit_id;

	uvc_processing_s2_show.bUnitID = dev->config.processing_unit_id | 0x80;
	uvc_output_terminal_s2_show.bSourceID = dev->config.processing_unit_id | 0x80;
	
	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
	{
		uvc_frame_yuv_800x600_hs.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(8);
		uvc_frame_yuv_800x600_hs.dwMaxBitRate = 800*600*2*30*8;
		uvc_frame_yuv_800x600_hs.dwDefaultFrameInterval	= (UVC_FORMAT_30FPS);
		uvc_frame_yuv_800x600_hs.bFrameIntervalType	= 8;
		uvc_frame_yuv_800x600_hs.dwFrameInterval[0]	= (UVC_FORMAT_30FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[1]	= (UVC_FORMAT_25FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[2]	= (UVC_FORMAT_24FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[3]	= (UVC_FORMAT_20FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[4]	= (UVC_FORMAT_15FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[5]	= (UVC_FORMAT_10FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[6]	= (UVC_FORMAT_7FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[7]	= (UVC_FORMAT_5FPS);	
	}
	else
	{
		uvc_frame_yuv_800x600_hs.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(7);
		uvc_frame_yuv_800x600_hs.dwMaxBitRate = 800*600*2*25*8;
		uvc_frame_yuv_800x600_hs.dwDefaultFrameInterval	= (UVC_FORMAT_25FPS);
		uvc_frame_yuv_800x600_hs.bFrameIntervalType	= 7;
		uvc_frame_yuv_800x600_hs.dwFrameInterval[0]	= (UVC_FORMAT_25FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[1]	= (UVC_FORMAT_24FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[2]	= (UVC_FORMAT_20FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[3]	= (UVC_FORMAT_15FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[4]	= (UVC_FORMAT_10FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[5]	= (UVC_FORMAT_7FPS);
		uvc_frame_yuv_800x600_hs.dwFrameInterval[6]	= (UVC_FORMAT_5FPS);
	}
	

	memcpy(&uvc_camera_terminal.bmControls.d24, &dev->config.camera_terminal_bmControls.d24, 
		sizeof(uvc_camera_terminal_bmControls_t));
	uvc_processing.bmControls.d16 = dev->config.processing_unit_bmControls.d16;
	
#if 0
	//第二路UVC控制
	memcpy(&uvc_camera_terminal_s2_show.bmControls.d24, &dev->config.camera_terminal_bmControls.d24, 
		sizeof(uvc_camera_terminal_bmControls_t));
	uvc_processing_s2_show.bmControls.d16 = dev->config.processing_unit_bmControls.d16;
#endif

	uvc_control_header.bcdUVC	= cpu_to_le16(dev->config.uvc_protocol);
	uvc_control_header_s2.bcdUVC	= cpu_to_le16(dev->config.uvc_protocol);
	uvc_control_header_s2_show.bcdUVC	= cpu_to_le16(dev->config.uvc_protocol);

	if(dev->config.uvc_protocol == UVC_PROTOCOL_V100)
	{
		uvc_processing.bLength = UVC100_DT_PROCESSING_UNIT_SIZE;
		uvc_processing_s2_show.bLength = UVC100_DT_PROCESSING_UNIT_SIZE;	
	}
	else
	{
		uvc_processing.bLength = UVC_DT_PROCESSING_UNIT_SIZE;
		uvc_processing_s2_show.bLength = UVC_DT_PROCESSING_UNIT_SIZE;
	}
	
	if((dev->bConfigurationValue != 2 && dev->config.uvc110_s2_h264_enable == 0 && dev->config.uvc110_h264_enable == 0
		&& dev->config.uvc110_s2_h265_enable == 0 && dev->config.uvc110_h265_enable == 0)
		|| dev->uvc->config.uvc_xu_h264_ctrl_enable == 0
	)
	{
		uvc_xu_h264.bLength = 0;
	}
	else
	{
		uvc_xu_h264.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1,2);
	}

	if((dev->uvc->uvc_s2_enable == 0 || (dev->config.uvc110_h264_enable == 0 && dev->config.uvc110_s2_h264_enable == 0 
		&& dev->config.uvc110_h265_enable == 0 && dev->config.uvc110_s2_h265_enable == 0))
		|| dev->uvc->config.uvc_xu_h264_ctrl_enable == 0
	)
	{
		uvc_xu_h264_s2.bLength = 0;
	}
	else
	{
		uvc_xu_h264_s2.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1,2);
	}
	
	if(dev->config.uvc110_h265_enable == 1)
	{
		memcpy(uvc110_format_hs_h265.guidFormat, "H265", 4);
		memcpy(uvc110_format_hs_h265_s2.guidFormat, "H265", 4);
		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
		{
			memcpy(uvc110_format_ss_h265.guidFormat, "H265", 4);
			memcpy(uvc110_format_ss_h265_s2.guidFormat, "H265", 4);

			if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
			{
				memcpy(uvc110_format_ssp_h265.guidFormat, "H265", 4);
				memcpy(uvc110_format_ssp_h265_s2.guidFormat, "H265", 4);
			}
		}
	}
	else if(dev->config.uvc110_h265_enable == 2)
	{
		memcpy(uvc110_format_hs_h265.guidFormat, "HEVC", 4);
		memcpy(uvc110_format_hs_h265_s2.guidFormat, "HEVC", 4);
		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
		{
			memcpy(uvc110_format_ss_h265.guidFormat, "HEVC", 4);
			memcpy(uvc110_format_ss_h265_s2.guidFormat, "HEVC", 4);

			if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
			{
				memcpy(uvc110_format_ssp_h265.guidFormat, "HEVC", 4);
				memcpy(uvc110_format_ssp_h265_s2.guidFormat, "HEVC", 4);
			}
		}
	}

	uvc_xu_camera_terminal.bmControls.b.scanning_mode= uvc_camera_terminal.bmControls.b.scanning_mode;
	uvc_xu_camera_terminal.bmControls.b.ae_mode = uvc_camera_terminal.bmControls.b.ae_mode;
	uvc_xu_camera_terminal.bmControls.b.ae_priority = uvc_camera_terminal.bmControls.b.ae_priority;
	uvc_xu_camera_terminal.bmControls.b.exposure_time_absolute = uvc_camera_terminal.bmControls.b.exposure_time_absolute;
	uvc_xu_camera_terminal.bmControls.b.exposure_time_relative = uvc_camera_terminal.bmControls.b.exposure_time_relative;
	uvc_xu_camera_terminal.bmControls.b.focus_absolute = uvc_camera_terminal.bmControls.b.focus_absolute;
	uvc_xu_camera_terminal.bmControls.b.focus_relative = uvc_camera_terminal.bmControls.b.focus_relative;
	uvc_xu_camera_terminal.bmControls.b.iris_absolute = uvc_camera_terminal.bmControls.b.iris_absolute;
	uvc_xu_camera_terminal.bmControls.b.iris_relative = uvc_camera_terminal.bmControls.b.iris_relative;
	uvc_xu_camera_terminal.bmControls.b.zoom_absolute = uvc_camera_terminal.bmControls.b.zoom_absolute;
	uvc_xu_camera_terminal.bmControls.b.zoom_relative = uvc_camera_terminal.bmControls.b.zoom_relative;
	uvc_xu_camera_terminal.bmControls.b.pantilt_absolute = uvc_camera_terminal.bmControls.b.pantilt_absolute;
	uvc_xu_camera_terminal.bmControls.b.pantilt_relative = uvc_camera_terminal.bmControls.b.pantilt_relative;
	uvc_xu_camera_terminal.bmControls.b.roll_absolute = uvc_camera_terminal.bmControls.b.roll_absolute;
	uvc_xu_camera_terminal.bmControls.b.roll_relative = uvc_camera_terminal.bmControls.b.roll_relative;
	uvc_xu_camera_terminal.bmControls.b.reserved1 = uvc_camera_terminal.bmControls.b.reserved1;
	uvc_xu_camera_terminal.bmControls.b.reserved2 = uvc_camera_terminal.bmControls.b.reserved2;
	uvc_xu_camera_terminal.bmControls.b.focus_auto = uvc_camera_terminal.bmControls.b.focus_auto;
	uvc_xu_camera_terminal.bmControls.b.privacy = uvc_camera_terminal.bmControls.b.privacy;
	uvc_xu_camera_terminal.bmControls.b.focus_simple = uvc_camera_terminal.bmControls.b.focus_simple;
	uvc_xu_camera_terminal.bmControls.b.window = uvc_camera_terminal.bmControls.b.window;
	uvc_xu_camera_terminal.bmControls.b.region_of_interest = uvc_camera_terminal.bmControls.b.region_of_interest;
	uvc_xu_camera_terminal.bmControls.b.reserved3 = uvc_camera_terminal.bmControls.b.reserved3;

	if(dev->hs_controls_desc_config == NULL)
	{
		dev->hs_controls_desc_config = get_uvc_control_descriptors_config(dev, UCAM_USB_SPEED_HIGH);
	}

	SAFE_FREE(dev->hs_controls_desc);
	dev->hs_controls_desc = uvc_copy_controls_descriptors(dev, UCAM_USB_SPEED_HIGH);
	if(dev->hs_controls_desc == NULL)
	{
		ucam_error("uvc_copy_controls_descriptors  error!\n");
		return -1;
	}

	if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
	{	
		if(dev->ss_controls_desc_config == NULL)
		{
			dev->ss_controls_desc_config = get_uvc_control_descriptors_config(dev, UCAM_USB_SPEED_SUPER);
		}
		SAFE_FREE(dev->ss_controls_desc);
		dev->ss_controls_desc = uvc_copy_controls_descriptors(dev, UCAM_USB_SPEED_SUPER);
		if(dev->ss_controls_desc == NULL)
		{
			ucam_error("uvc_copy_controls_descriptors  error!\n");
			return -1;
		}

		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
		{
			if(dev->ssp_controls_desc_config == NULL)
			{
				dev->ssp_controls_desc_config = get_uvc_control_descriptors_config(dev, UCAM_USB_SPEED_SUPER_PLUS);
			}
			SAFE_FREE(dev->ssp_controls_desc);
			dev->ssp_controls_desc = uvc_copy_controls_descriptors(dev, UCAM_USB_SPEED_SUPER_PLUS);
			if(dev->ssp_controls_desc == NULL)
			{
				ucam_error("uvc_copy_controls_descriptors  error!\n");
				return -1;
			}
		}
	}

	return 0;
}

int uvc_streaming_desc_copy(struct uvc_dev *dev)
{
	if(dev->hs_uvc_streaming_desc_attrs == NULL)
	{
		ucam_notice("hs_uvc_streaming_desc_attrs is NULL");
		if(dev->bConfigurationValue == 2)//UVC1.5 
		{
			if(dev->stream_num == 0)
			{
				dev->hs_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)hs_uvc_streaming_desc_attrs_c2_s1;
			}
			else
			{
				dev->hs_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)hs_uvc_streaming_desc_attrs_c2_s2;
			}
		}
		else
		{
			if(dev->stream_num == 0)
			{
				dev->hs_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)hs_uvc_streaming_desc_attrs_c1_s1;
			}
			else
			{
				dev->hs_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)hs_uvc_streaming_desc_attrs_c1_s2;
			}
		}
	}

	if(dev->ss_uvc_streaming_desc_attrs == NULL)
	{
		ucam_notice("ss_uvc_streaming_desc_attrs is NULL");

		if(dev->bConfigurationValue == 2)//UVC1.5 
		{
			if(dev->stream_num == 0)
			{
				dev->ss_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ss_uvc_streaming_desc_attrs_c2_s1;
			}
			else
			{
				dev->ss_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ss_uvc_streaming_desc_attrs_c2_s2;
			}
		}
		else
		{
			if(dev->stream_num == 0)
			{
				dev->ss_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ss_uvc_streaming_desc_attrs_c1_s1;
			}
			else
			{
				dev->ss_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ss_uvc_streaming_desc_attrs_c1_s2;
			}
		}		
	}

	if(dev->ssp_uvc_streaming_desc_attrs == NULL)
	{
		ucam_notice("ssp_uvc_streaming_desc_attrs is NULL");

		if(dev->bConfigurationValue == 2)//UVC1.5 
		{
			if(dev->stream_num == 0)
			{
				dev->ssp_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ssp_uvc_streaming_desc_attrs_c2_s1;
			}
			else
			{
				dev->ssp_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ssp_uvc_streaming_desc_attrs_c2_s2;
			}
		}
		else
		{
			if(dev->stream_num == 0)
			{
				dev->ssp_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ssp_uvc_streaming_desc_attrs_c1_s1;
			}
			else
			{
				dev->ssp_uvc_streaming_desc_attrs = (const struct uvc_streaming_desc_attr_t **)ssp_uvc_streaming_desc_attrs_c1_s2;
			}
		}		
	}

	SAFE_FREE(dev->hs_streaming);
	dev->hs_streaming = uvc_copy_streaming_descriptors(dev, UCAM_USB_SPEED_HIGH);
	if(dev->hs_streaming == NULL)
	{
		ucam_error("uvc_copy_streaming_descriptors  error!\n");
		return -1;
	}

	if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
	{
		SAFE_FREE(dev->ss_streaming);
		dev->ss_streaming = uvc_copy_streaming_descriptors(dev, UCAM_USB_SPEED_SUPER);
		if(dev->ss_streaming == NULL)
		{
			ucam_error("uvc_copy_streaming_descriptors  error!\n");
			return -1;
		}

		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
		{
			SAFE_FREE(dev->ssp_streaming);
			dev->ssp_streaming = uvc_copy_streaming_descriptors(dev, UCAM_USB_SPEED_SUPER_PLUS);
			if(dev->ssp_streaming == NULL)
			{
				ucam_error("uvc_copy_streaming_descriptors  error!\n");
				return -1;
			}
		}
	}

	return 0;
}

int uvc_streaming_desc_init(struct uvc_dev *dev, int controls_desc_copy, int streaming_desc_copy)
{
	int ret = 0;
	ucam_strace("enter");

	if(controls_desc_copy)
	{
		ret = uvc_controls_desc_copy(dev);
		if(ret != 0)
		{
			ucam_error("uvc_controls_desc_copy error!\n");
			return ret;
		}
	}

	if(streaming_desc_copy)
	{
		ret = uvc_streaming_desc_copy(dev);
		if(ret != 0)
		{
			ucam_error("uvc_streaming_desc_copy error!\n");
			return ret;
		}
	}

	return ret;
}

struct uvc_descriptor_header **
get_uvc_control_descriptors(struct uvc_dev *dev, enum ucam_usb_device_speed speed)
{
	switch (speed) {
		case UCAM_USB_SPEED_SUPER_PLUS:
			return dev->ssp_controls_desc;
		case UCAM_USB_SPEED_SUPER:
			return dev->ss_controls_desc;
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			return dev->hs_controls_desc;
	}	

	return NULL;
}


struct uvc_descriptor_header **
get_uvc_streaming_cls_descriptor(struct uvc_dev *dev, enum ucam_usb_device_speed speed)
{
	switch (speed) {
		case UCAM_USB_SPEED_SUPER_PLUS:
			return dev->ssp_streaming;
		case UCAM_USB_SPEED_SUPER:
			return dev->ss_streaming;
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			return dev->hs_streaming;
	}	

	return NULL;
}

int get_uvc_format(struct uvc_dev *dev, struct uvc_streaming_control *vs_ctrl, 
	uint32_t *fcc, 	uint16_t *width, uint16_t *height)
{
	struct uvc_descriptor_header **uvc_streaming_cls;
	const struct usb_descriptor_header * const *src = NULL;
	
	const struct uvc_format_header * format_header;
	const struct uvc_frame_header * frame_header;
	//const struct uvc_frame_uncompressed_header *frame_uncompressed_header;
	//const struct uvc_frame_h264_v150_header *frame_h264_v150_header;

	uint16_t wWidth = 0;
	uint16_t wHeight = 0;

	uvc_streaming_cls = get_uvc_streaming_cls_descriptor(dev, dev->uvc->usb_speed);
	if(uvc_streaming_cls == NULL)
	{
		ucam_error("get_uvc_streaming_cls_descriptor  error!\n");
		return -1;
	}

	for (src = (const struct usb_descriptor_header **)uvc_streaming_cls;
	     *src; ++src) 
	{
		format_header = (const struct uvc_format_header *)*src;	
		if(format_header->bDescriptorSubType == UVC_VS_FORMAT_UNCOMPRESSED ||
			format_header->bDescriptorSubType == UVC_VS_FORMAT_MJPEG || 
			format_header->bDescriptorSubType == UVC_VS_FORMAT_FRAME_BASED || 
			format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_V150 ||
			format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_SIMULCAST
		)
		{
			if(vs_ctrl->bFormatIndex == format_header->bFormatIndex)
			{
				//先找到视频格式索引，再找分辨率索引
				++src;
				break;		
			}
		}	
	}

	
	if(*src == NULL)
	{
		ucam_error("uvc_vs_ctrl_analysis  error!\n");
		return -1;
	}
	
	if(uvc_format_to_v4l2_fcc(format_header, fcc) != 0)
	{
		ucam_error("uvc_format_to_v4l2_fcc  error!\n");
		return -1;
	}


	//找分辨率索引
	for (; *src; ++src)
	{
		frame_header = (const struct uvc_frame_header *)*src;
		if(frame_header->bLength == 0)
		{
			continue;
		}
		if(vs_ctrl->bFrameIndex == frame_header->bFrameIndex)
		{
			if(get_uvc_frame_width_height(frame_header, &wWidth, &wHeight) != 0)
			{
				return -1;
			}	
			break;
		}
		else if(frame_header->bDescriptorSubType == UVC_VS_COLORFORMAT)
		{
			//已到分辨率描述符末尾
			break;
		}
	}

	if(wWidth == 0 || wHeight == 0)
	{
		ucam_error("uvc_vs_ctrl_analysis  error!\n");
		return -1;
	}
		

	*width  = wWidth;
	*height = wHeight;
	

	ucam_notice("uvc_vs_ctrl_analysis: %s-%ux%u\n",
		uvc_video_format_string(*fcc), *width, *height);


	return 0;	
	
}

/********************************************************************************************************
* 配置接口
********************************************************************************************************/

/**
 * @description: 获取视频配置描述符
 * @param 	speed:USB的速率
 * 			config：第几个USB配置, 1第一个配置，2第二个配置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流
 * @return: 0:success; other:error
 */
static struct uvc_streaming_desc_attr_t ** get_uvc_streaming_desc_attrs(
	enum ucam_usb_device_speed speed, 
	uint32_t config, uint32_t stream_num)
{
	struct uvc_streaming_desc_attr_t ** uvc_streaming_desc_attrs = NULL;

	switch (speed) 
	{
		case UCAM_USB_SPEED_SUPER_PLUS:
			if(config == 2)
			{
				if(stream_num == 1)
				{
					uvc_streaming_desc_attrs = ssp_uvc_streaming_desc_attrs_c2_s2;
				}
				else
				{
					uvc_streaming_desc_attrs = ssp_uvc_streaming_desc_attrs_c2_s1;
				}
			}
			else
			{
				if(stream_num == 1)
				{
					uvc_streaming_desc_attrs = ssp_uvc_streaming_desc_attrs_c1_s2;
				}
				else
				{
					uvc_streaming_desc_attrs = ssp_uvc_streaming_desc_attrs_c1_s1;
				}
			}	
			break;
		case UCAM_USB_SPEED_SUPER:
			if(config == 2)
			{
				if(stream_num == 1)
				{
					uvc_streaming_desc_attrs = ss_uvc_streaming_desc_attrs_c2_s2;
				}
				else
				{
					uvc_streaming_desc_attrs = ss_uvc_streaming_desc_attrs_c2_s1;
				}
			}
			else
			{
				if(stream_num == 1)
				{
					uvc_streaming_desc_attrs = ss_uvc_streaming_desc_attrs_c1_s2;
				}
				else
				{
					uvc_streaming_desc_attrs = ss_uvc_streaming_desc_attrs_c1_s1;
				}
			}	
			break;		
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			if(config == 2)
			{
				if(stream_num == 1)
				{
					uvc_streaming_desc_attrs = hs_uvc_streaming_desc_attrs_c2_s2;
				}
				else
				{
					uvc_streaming_desc_attrs = hs_uvc_streaming_desc_attrs_c2_s1;
				}
			}
			else
			{
				if(stream_num == 1)
				{
					uvc_streaming_desc_attrs = hs_uvc_streaming_desc_attrs_c1_s2;
				}
				else
				{
					uvc_streaming_desc_attrs = hs_uvc_streaming_desc_attrs_c1_s1;
				}
			}
			break;
	}

	return 	uvc_streaming_desc_attrs;
}

//fcc为0时表示不判断格式
//width和height都为0时表示不判断分辨率
//bypass 0表示不设置bypass, >0表示设置bypass, <0表示设置bypass为默认值
static int set_format_resolution_bypass(
    const struct uvc_streaming_desc_attr_t ** uvc_streaming_desc_attrs,
	uint32_t fcc, uint16_t width, uint16_t height, 
    int bypass)
{
	const struct uvc_descriptor_header * const *src;
	struct uvc_frame_info_t *src2;
	struct uvc_streaming_desc_attr_t *uvc_streaming_desc_attr;
	struct uvc_frame_header *frame_header;
	uint16_t frame_width, frame_height;
	uint32_t format_fcc;
    uint8_t set_bypass = !!bypass;

	for (src = (const struct uvc_descriptor_header **)uvc_streaming_desc_attrs; *src; ++src)
	{
		uvc_streaming_desc_attr = (struct uvc_streaming_desc_attr_t *)*src;
		if(uvc_streaming_desc_attr->format_header)
        {
            if(fcc)
            {
                if(uvc_format_to_v4l2_fcc((const struct uvc_format_header *)
                    uvc_streaming_desc_attr->format_header, 
                    &format_fcc) != 0)
                {
                    return -1;
                }

                if(format_fcc != fcc)
                {
                    continue;
                }
            }
	
			for (src2 = uvc_streaming_desc_attr->frame_info; src2->frame_header; src2++)
			{
				frame_header = (struct uvc_frame_header *)src2->frame_header;
				
                if(bypass < 0)
                {
                    set_bypass = src2->def;
                }

				if(src2->bypass == set_bypass)
				{
					continue;
				}

				if(frame_header->bDescriptorSubType != UVC_VS_COLORFORMAT)
				{
                    if(width == 0 && height == 0)
                    {
                        src2->bypass = set_bypass;
                    }
                    else 
                    {
                        if(get_uvc_frame_width_height(frame_header, &frame_width, &frame_height) != 0)
                        {
                            ucam_error("get_uvc_frame_width_height error");
                            return -1;
                        }

                        if(width == frame_width && height == frame_height)
                        {
                            src2->bypass = set_bypass;
                            continue;
                        }
                    }
				}
			}
		}
	}

	return 0;
}

static int set_format_resolution_bypass_all(
    uint32_t fcc, uint16_t width, uint16_t height, int bypass)
{
	int ret;

	ret = set_format_resolution_bypass((const struct uvc_streaming_desc_attr_t **)
		hs_uvc_streaming_desc_attrs_c1_s1, fcc, width, height, bypass);
	if(ret != 0)
	{
		ucam_error("hs_c1_s1 set_format_resolution_bypass error");
		return ret;
	}


	ret = set_format_resolution_bypass((const struct uvc_streaming_desc_attr_t **)
		ss_uvc_streaming_desc_attrs_c1_s1, fcc, width, height, bypass);
	if(ret != 0)
	{
		ucam_error("ss_c1_s1 set_format_resolution_bypass error");
		return ret;
	}

	ret = set_format_resolution_bypass((const struct uvc_streaming_desc_attr_t **)
		hs_uvc_streaming_desc_attrs_c1_s2, fcc, width, height, bypass);
	if(ret != 0)
	{
		ucam_error("hs_c1_s2 set_format_resolution_bypass error");
		return ret;
	}

	ret = set_format_resolution_bypass((const struct uvc_streaming_desc_attr_t **)
		ss_uvc_streaming_desc_attrs_c1_s2, fcc, width, height, bypass);
	if(ret != 0)
	{
		ucam_error("ss_c1_s2 set_format_resolution_bypass error");
		return ret;
	}

	ret = set_format_resolution_bypass((const struct uvc_streaming_desc_attr_t **)
		hs_uvc_streaming_desc_attrs_c2_s2, fcc, width, height, bypass);
	if(ret != 0)
	{
		ucam_error("hs_c2_s2 set_format_resolution_bypass error");
		return ret;
	}

	ret = set_format_resolution_bypass((const struct uvc_streaming_desc_attr_t **)
		ss_uvc_streaming_desc_attrs_c2_s2, fcc, width, height, bypass);
	if(ret != 0)
	{
		ucam_error("ss_c2_s2 set_format_resolution_bypass error");
		return ret;
	}
	
	return 0;
}

/**
 * @description: 屏蔽UVC的分辨率
 * @param 	width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_bypass_resolution(uint16_t width, uint16_t height)
{
	return set_format_resolution_bypass_all(0, width, height, 1);
}

/**
 * @description: 清除掉被屏蔽的UVC分辨率
 * @param 	width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_clear_bypass_resolution(uint16_t width, uint16_t height)
{
	return set_format_resolution_bypass_all(0, width, height, 0);
}

/**
 * @description: uvc描述符分辨率恢复默认设置
 * @param 	width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_restore_bypass_resolution(uint16_t width, uint16_t height)
{
	return set_format_resolution_bypass_all(0, width, height, -1);
}

/**
 * @description: 使能所有的视频分辨率
 * @return: 0:success; other:error
 */
int uvc_clear_all_bypass_resolution(void)
{
	return set_format_resolution_bypass_all(0, 0, 0, 0);
}

/**
 * @description: 恢复默认设置所有的视频分辨率使能
 * @return: 0:success; other:error
 */
int uvc_restore_all_bypass_resolution(void)
{
	return set_format_resolution_bypass_all(0, 0, 0, -1);
}

/**
 * @description: 屏蔽视频分辨率
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * 			width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_bypass_specific_resolution(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height)
{	
	int ret;

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("hs_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("hs_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("hs_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, 1);
		if(ret != 0)
		{
			ucam_error("hs_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	return 0;
}

/**
 * @description: 使能视频分辨率
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * 			width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_clear_bypass_specific_resolution(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height)
{
	int ret;

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("hs_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("hs_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("hs_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, 0);
		if(ret != 0)
		{
			ucam_error("hs_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	return 0;	
}

/**
 * @description: 视频分辨率使能恢复默认设置
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * 			width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_restore_bypass_specific_resolution(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height)
{
	int ret;

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c1_s1,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("hs_c1_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c1_s2,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("hs_c1_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c2_s1,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("hs_c2_s1 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ss_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			ssp_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_format_resolution_bypass(
			(const struct uvc_streaming_desc_attr_t **)
			hs_uvc_streaming_desc_attrs_c2_s2,
			fcc, width, height, -1);
		if(ret != 0)
		{
			ucam_error("hs_c2_s2 set_format_resolution_bypass error");
			return ret;
		}
	}

	return 0;
}

static int set_default_format(
	struct uvc_streaming_desc_attr_t ** uvc_streaming_desc_attrs, 
	uint32_t fcc)
{
	struct uvc_streaming_desc_attr_t *uvc_streaming_desc_attr;
	const struct uvc_descriptor_header * const *src;
	const struct uvc_format_header * format_header;
	uint32_t format_fcc;
	uint32_t index = 0;

	for (src = (const struct uvc_descriptor_header **)uvc_streaming_desc_attrs; *src; ++src)
	{
		uvc_streaming_desc_attr = (struct uvc_streaming_desc_attr_t *)*src;
		if(uvc_streaming_desc_attr->format_header)
        {
			format_header = (struct uvc_format_header *)uvc_streaming_desc_attr->format_header;	
			if(uvc_format_to_v4l2_fcc(format_header, &format_fcc) != 0)
			{
				ucam_error("uvc_format_to_v4l2_fcc error");
				return -1;
			}

			if(format_fcc == fcc)
			{
				break;
			}
		}
		index ++;
	}

	if(index > 0)
	{
		uvc_streaming_desc_attr = uvc_streaming_desc_attrs[0]; 
		uvc_streaming_desc_attrs[0] = uvc_streaming_desc_attrs[index];
		uvc_streaming_desc_attrs[index] = uvc_streaming_desc_attr;
	}

	return 0;
}
/**
 * @description: 设置UVC默认的视频格式
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_uvc_default_format(enum ucam_usb_device_speed speed, uint32_t fcc)
{
	struct uvc_streaming_desc_attr_t ** uvc_streaming_desc_attrs;

	switch (speed) 
	{
		case UCAM_USB_SPEED_SUPER_PLUS:
			uvc_streaming_desc_attrs = ssp_uvc_streaming_desc_attrs_c1_s1;
			break;
		case UCAM_USB_SPEED_SUPER:
			uvc_streaming_desc_attrs = ss_uvc_streaming_desc_attrs_c1_s1;
			break;		
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			uvc_streaming_desc_attrs = hs_uvc_streaming_desc_attrs_c1_s1;
			break;
	}

	return set_default_format(uvc_streaming_desc_attrs, fcc);
}

/**
 * @description: 设置UVC默认的视频格式
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * @return: 0:success; other:error
 */
int set_uvc_specific_default_format(int speed, 
	int config, int stream_num, uint32_t fcc)
{
	int ret;

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_default_format(
			ss_uvc_streaming_desc_attrs_c1_s1,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_default_format(
			ssp_uvc_streaming_desc_attrs_c1_s1,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c1_s1 set_default_format error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_default_format(
			hs_uvc_streaming_desc_attrs_c1_s1,
			fcc);
		if(ret != 0)
		{
			ucam_error("hs_c1_s1 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_default_format(
			ss_uvc_streaming_desc_attrs_c1_s2,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_default_format(
			ssp_uvc_streaming_desc_attrs_c1_s2,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c1_s2 set_default_format error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 1 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_default_format(
			hs_uvc_streaming_desc_attrs_c1_s2,
			fcc);
		if(ret != 0)
		{
			ucam_error("hs_c1_s2 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_default_format(
			ss_uvc_streaming_desc_attrs_c2_s1,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_default_format(
			ssp_uvc_streaming_desc_attrs_c2_s1,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c2_s1 set_default_format error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 1 || stream_num == -1))
	{
		ret = set_default_format(
			hs_uvc_streaming_desc_attrs_c2_s1,
			fcc);
		if(ret != 0)
		{
			ucam_error("hs_c2_s1 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_default_format(
			ss_uvc_streaming_desc_attrs_c2_s2,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_default_format error");
			return ret;
		}
	}

	if((speed == UCAM_USB_SPEED_SUPER_PLUS || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_default_format(
			ssp_uvc_streaming_desc_attrs_c2_s2,
			fcc);
		if(ret != 0)
		{
			ucam_error("ss_c2_s2 set_default_format error");
			return ret;
		}
	}

	if((speed < UCAM_USB_SPEED_SUPER || speed == -1) &&
		(config == 2 || config == -1) && (stream_num == 2 || stream_num == -1))
	{
		ret = set_default_format(
			hs_uvc_streaming_desc_attrs_c2_s2,
			fcc);
		if(ret != 0)
		{
			ucam_error("hs_c2_s2 set_default_format error");
			return ret;
		}
	}

	return 0;
}

int replacement_uvc_frame_width_height(struct uvc_descriptor_header **uvc_streaming_cls)
{
	const struct usb_descriptor_header * const *src = NULL;
	const struct uvc_format_header * format_header;
	struct uvc_frame_header * frame_header;
	struct uvc_frame_uncompressed_header *frame_uncompressed_header;
	struct uvc_frame_h264_v150_header *frame_h264_v150_header;
	uint16_t temp;
	int format_flag = 0;
	
	if(uvc_streaming_cls == NULL)
	{
		return -1;
	}

	for (src = (const struct usb_descriptor_header **)uvc_streaming_cls;
	     *src; ++src) 
	{
		format_header = (const struct uvc_format_header *)*src;
		if(format_header->bDescriptorType == USB_DT_CS_INTERFACE)
		{
			if(format_header->bDescriptorSubType == UVC_VS_FORMAT_UNCOMPRESSED ||
				format_header->bDescriptorSubType == UVC_VS_FORMAT_MJPEG || 
				format_header->bDescriptorSubType == UVC_VS_FORMAT_FRAME_BASED || 
				format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_V150 ||
				format_header->bDescriptorSubType == UVC_VS_FORMAT_H264_SIMULCAST
			)
			{
				//先找到视频格式索引，再找分辨率索引
				format_flag = 1;
				
			}

			if(format_flag)
			{
				frame_header = (struct uvc_frame_header *)*src;
				if(frame_header->bDescriptorSubType == UVC_VS_FRAME_H264_V150)
				{
					frame_h264_v150_header = (struct uvc_frame_h264_v150_header *)frame_header;
					temp = frame_h264_v150_header->wWidth;
					frame_h264_v150_header->wWidth = frame_h264_v150_header->wHeight;
					frame_h264_v150_header->wHeight = temp;	
				}
				else if(
					frame_header->bDescriptorSubType == UVC_VS_FRAME_UNCOMPRESSED
					|| frame_header->bDescriptorSubType == UVC_VS_FRAME_MJPEG
					|| frame_header->bDescriptorSubType == UVC_VS_FRAME_FRAME_BASED
					)
				{
					frame_uncompressed_header = (struct uvc_frame_uncompressed_header *)frame_header;
					temp = frame_uncompressed_header->wWidth;
					frame_uncompressed_header->wWidth = frame_uncompressed_header->wHeight;
					frame_uncompressed_header->wHeight = temp;
				}

				if(frame_header->bDescriptorSubType == UVC_VS_COLORFORMAT)
				{
					//已到分辨率描述符末尾
					format_flag = 0;
				}
			}
		}
	}

	return 0;
}

/**
 * @description: 翻转UVC分辨率的宽高
 * @param {flip} 0-正常，1-翻转
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int flip_uvc_frame_width_height(struct uvc *uvc, uint32_t flip)
{
	static uint32_t g_flip = 0;
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos;

	if(!!g_flip == !!flip)
	{
		return 0;
	}

    if(uvc == NULL)
    {
        ucam_error("uvc == NULL\n");
        return -ENOMEM;
    }

	if(uvc->init_flag == 0)
	{
		ucam_error("error uvc init_flag:%d\n", uvc->init_flag);
		return -1;
	}
	
	ucam_list_for_each(pos, &uvc->uvc_dev_list)
	{
		uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
        if(uvc_dev)
		{	
            if(uvc_dev->ss_streaming)
            {
                if(replacement_uvc_frame_width_height(uvc_dev->ss_streaming) != 0)
                {
                    ucam_error("replacement_uvc_format_width_height  error!\n");
                    return -1;
                }
            }

            if(uvc_dev->hs_streaming)
            {
                if(replacement_uvc_frame_width_height(uvc_dev->hs_streaming) != 0)
                {
                    ucam_error("replacement_uvc_format_width_height  error!\n");
                    return -1;
                }
            }
            
			if(reset_uvc_descriptor(uvc_dev, 0, 0) != 0)
			{
				ucam_error("reset_uvc_descriptor fail!");
				return -1;
			}
		}
    }

	g_flip = !!flip;
	
	return 0;
}


/**
 * @description: 限制帧率的最大值，当分辨率超过设置的分辨率时按照设置的帧率进行限制，
 * 				 必须在初始化之前调用才生效
 * @param {width * height} 超过这个分辨率时才被限制  {fps_max} 能达到的最大帧率
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int limit_uvc_frame_width_height_fps(uint16_t width, uint16_t height, uint32_t fps_max)
{
	g_limit_fps_width = width;
	g_limit_fps_height = height;
	g_limit_fps_max = fps_max;
	return 0;
}

int set_uvc_default_width_height_fps(int speed, 
	uint16_t width, uint16_t height, uint32_t fps)
{
	
	switch (speed) 
	{
		case -1:
			g_def_width[0] = width;
			g_def_height[0] = height;
			g_def_fps[0] = fps;
			g_def_width[1] = width;
			g_def_height[1] = height;
			g_def_fps[1] = fps;
			g_def_width[2] = width;
			g_def_height[2] = height;
			g_def_fps[2] = fps;
			break;
		case UCAM_USB_SPEED_SUPER_PLUS:
			g_def_width[2] = width;
			g_def_height[2] = height;
			g_def_fps[2] = fps;
			break;
		case UCAM_USB_SPEED_SUPER:
			g_def_width[1] = width;
			g_def_height[1] = height;
			g_def_fps[1] = fps;
			break;	
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			g_def_width[0] = width;
			g_def_height[0] = height;
			g_def_fps[0] = fps;
			break;
	}

	return 0;
}

int set_uvc_default_format_v2(int speed, 
	uint32_t fcc, uint16_t width, uint16_t height, uint32_t fps)
{
	
	switch (speed) 
	{
		case -1:
			g_def_width[0] = width;
			g_def_height[0] = height;
			g_def_fps[0] = fps;
			g_def_width[1] = width;
			g_def_height[1] = height;
			g_def_fps[1] = fps;
			g_def_width[2] = width;
			g_def_height[2] = height;
			g_def_fps[2] = fps;
			set_uvc_default_format(UCAM_USB_SPEED_SUPER_PLUS, fcc);
			set_uvc_default_format(UCAM_USB_SPEED_SUPER, fcc);
			set_uvc_default_format(UCAM_USB_SPEED_HIGH, fcc);
			break;
		case UCAM_USB_SPEED_SUPER_PLUS:
			g_def_width[2] = width;
			g_def_height[2] = height;
			g_def_fps[2] = fps;
			set_uvc_default_format(speed, fcc);
			break;
		case UCAM_USB_SPEED_SUPER:
			g_def_width[1] = width;
			g_def_height[1] = height;
			g_def_fps[1] = fps;
			set_uvc_default_format(speed, fcc);
			break;	
		case UCAM_USB_SPEED_HIGH:
		case UCAM_USB_SPEED_FULL:
		default:
			g_def_width[0] = width;
			g_def_height[0] = height;
			g_def_fps[0] = fps;
			set_uvc_default_format(speed, fcc);
			break;
	}

	return 0;
}

int set_uvc_specific_default_format_v2(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height, uint32_t fps)
{
	int ret;
	
	ret = set_uvc_specific_default_format(speed, 
		config, stream_num, fcc);
	if(ret)
	{
		return ret;
	}
	
	return set_uvc_default_width_height_fps(speed, 
		width, height, fps);
	
}

int uvc_bypass_fps(uint32_t fps, uint8_t bypass)
{
	int i;

	for(i = 0; i < (sizeof(g_uvc_fps_list)/sizeof(struct uvc_fps_list_t)); i++)
	{
		if(g_uvc_fps_list[i].fps == fps)
		{
			g_uvc_fps_list[i].bypass = bypass;
			return 0;
		}
	}
	return 1;
}



