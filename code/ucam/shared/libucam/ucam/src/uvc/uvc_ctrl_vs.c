/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-15 19:34:55
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl_vs.c
 * @Description : 
 */
#include "assert.h" 
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/uvc.h>
#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_vs.h>
#include <ucam/uvc/uvc_api_stream.h>
#include <ucam/uvc/uvc_api_eu.h>
#include <ucam/uvc/uvc_api_config.h>

int32_t uvc_events_process_streamoff(struct uvc_dev *dev, 
	struct uvc_request_data *data);

static int uvc_vs_ctrl_analysis(struct uvc_dev *dev, struct uvc_streaming_control *vs_ctrl, 
	uint32_t *fcc, 	uint16_t *width, uint16_t *height)
{
	return get_uvc_format(dev, vs_ctrl, fcc, width, height);
}

size_t uvc_page_size(void)
{
	long s = sysconf(_SC_PAGE_SIZE);
	assert(s > 0);
	return s;
}

size_t uvc_page_align(size_t size)
{
	size_t r;
	long psz = uvc_page_size();
	r = size % psz;
	if (r)
		return size + psz - r;
	return size;
}

uint32_t uvc_get_v4l2_sizeimage(struct uvc_dev *dev, uint32_t fcc, uint16_t width, uint16_t height)
{
	uint32_t sizeimage = dev->config.uvc_max_format_bufsize;

	switch (fcc)
	{
	case V4L2_PIX_FMT_H265:
	case V4L2_PIX_FMT_H264:
		//H264有可能会动态设置分辨率，需要使用最大分辨率进行设置
		if(dev->uvc->config.chip_id ==  0x3518E300)
		{
			sizeimage = (dev->config.resolution_width_max * dev->config.resolution_height_max)/2 + REQBUFS_USERPTR_DATA_OFFSET;
		}
		else
		{
			sizeimage = dev->config.resolution_width_max * dev->config.resolution_height_max + REQBUFS_USERPTR_DATA_OFFSET;
		}
		break;
	case V4L2_PIX_FMT_MJPEG:
	case V4L2_PIX_FMT_MJPEG_IR:
		if(dev->uvc->config.chip_id ==  0x3518E300)
		{
			sizeimage = (width * height)/2 + REQBUFS_USERPTR_DATA_OFFSET;
		}
		else
		{
			sizeimage = width * height + REQBUFS_USERPTR_DATA_OFFSET;
		}
		break;
	case V4L2_PIX_FMT_YUYV:
		sizeimage = width * height * 2 + REQBUFS_USERPTR_DATA_OFFSET;
		break;
	case V4L2_PIX_FMT_YUV420:
	case V4L2_PIX_FMT_NV12:
		sizeimage = width * height * 1.5 + REQBUFS_USERPTR_DATA_OFFSET;
		break;
	case V4L2_PIX_FMT_L8_IR:
		sizeimage = width * height + REQBUFS_USERPTR_DATA_OFFSET + 64;
		break;
	case V4L2_PIX_FMT_L16_IR:
		sizeimage = width * height * 2 + REQBUFS_USERPTR_DATA_OFFSET + 64;
		break;
	default:
		ucam_error("UVC fcc = 0x%x",dev->fcc);
		break;
	}

	if(sizeimage > dev->config.uvc_max_format_bufsize)
	{
		ucam_error("sizeimage:%d > uvc_max_format_bufsize:%d",
			sizeimage, dev->config.uvc_max_format_bufsize);

		sizeimage = dev->config.uvc_max_format_bufsize;
	}

	/* align in page_size(4K) */
	sizeimage = uvc_page_align(sizeimage);

	sizeimage += uvc_page_size();

	ucam_notice("sizeimage:%d",sizeimage);

	return sizeimage;
}


int uvc_video_set_format(struct uvc_dev *dev, struct uvc_streaming_control *vs_ctrl)
{
	int ret;

	unsigned char fps;
    fps = (unsigned char)(10000000.0f/vs_ctrl->dwFrameInterval);
	struct uvcx_h264_config *h264_config = &dev->config.uvc_xu_h264_config.uvcx_h264_config_cur;
	

	
	if(dev->fcc == V4L2_PIX_FMT_H264 || dev->fcc == V4L2_PIX_FMT_H265)
	{			
		//设置PROFILE
		#if 1
		if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 
			&& dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable)
		{
			if(vs_ctrl->bUsage < 2)
			{
				if(dev->stream_num)
				{
					dev->H264SVCEnable = 0;
					if(vs_ctrl->bFrameIndex < UVC_H264_FORMAT2_INDEX_640x480)
					{
						uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_0_venc_chn, UVCX_H264_wProfile_BASELINEPROFILE);
						if(dev->uvc150_simulcast_streamon)
						{
							uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_1_venc_chn, UVCX_H264_wProfile_BASELINEPROFILE);
							uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_2_venc_chn, UVCX_H264_wProfile_BASELINEPROFILE);
						}
					}
					else
					{
						uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_0_venc_chn, UVCX_H264_wProfile_HIGHPROFILE);
						if(dev->uvc150_simulcast_streamon)
						{
							uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_1_venc_chn, UVCX_H264_wProfile_HIGHPROFILE);
							uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_2_venc_chn, UVCX_H264_wProfile_HIGHPROFILE);
						}
					}
				}
				else
				{
					uvc_api_set_h264_profile(dev, dev->config.venc_chn, UVCX_H264_wProfile_BASELINEPROFILE);
				}		
			}
			else
			{
				if(dev->stream_num)
				{
					dev->H264SVCEnable = 1;
					uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_0_venc_chn, UVCX_H264_wProfile_SCALABLEHIGHPROFILE);
					if(dev->uvc150_simulcast_streamon)
					{
						uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_1_venc_chn, UVCX_H264_wProfile_SCALABLEHIGHPROFILE);
						uvc_api_set_h264_profile(dev, dev->config.simulcast_stream_2_venc_chn, UVCX_H264_wProfile_SCALABLEHIGHPROFILE);
					}
				}
				else
				{
					uvc_api_set_h264_profile(dev, dev->config.venc_chn, UVCX_H264_wProfile_SCALABLEHIGHPROFILE);
				}		
			}
		}
		else
		{
			uvc_api_set_h264_profile(dev, dev->config.venc_chn, h264_config->wProfile);
		}
		#endif
	}
	else if(dev->fcc == V4L2_PIX_FMT_MJPEG)
	{
		//ucam_strace("Setting MJPEG Qfactor to %d",(uint8_t)CompQuality);	
		//printf("Setting MJPEG Qfactor to %d\n",(uint8_t)CompQuality);	
	}	
	//设置uvc的视频帧率
	if(uvc_api_set_frame_interval(dev, dev->config.venc_chn, vs_ctrl->dwFrameInterval) != 0)
	{
		ucam_error("uvc_api_set_frame_interval fail");
		//return -1;
	}
	
	//设置uvc的视频分辨率
	if(uvc_api_set_resolution(dev, dev->config.venc_chn, dev->width, dev->height) != 0)
	{
		ucam_error("uvc_api_set_resolution fail");
		return -1;
	}

	if(uvc_api_set_format(dev, dev->config.venc_chn, dev->fcc) != 0)
	{
		ucam_error("uvc_api_set_format fail");
		return -1;
	}
		
	ucam_notice("Setting format to %s-%ux%up%u",
		uvc_video_format_string(dev->fcc), dev->width, dev->height,fps);

	
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = dev->width;
	fmt.fmt.pix.height = dev->height;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	//fmt.fmt.pix.sizeimage = dev->config.uvc_max_format_bufsize;
	
	if(dev->fcc == V4L2_PIX_FMT_H265 
		|| dev->fcc == V4L2_PIX_FMT_L8_IR
	  	||dev->fcc == V4L2_PIX_FMT_L16_IR
		||dev->fcc == V4L2_PIX_FMT_MJPEG_IR
		||dev->fcc == V4L2_PIX_FMT_YUV420)
	{
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
	}
	else
	{
		fmt.fmt.pix.pixelformat = dev->fcc;
	}

	fmt.fmt.pix.sizeimage = uvc_get_v4l2_sizeimage(dev, dev->fcc, fmt.fmt.pix.width, fmt.fmt.pix.height);
	dev->sizeimage = fmt.fmt.pix.sizeimage;
		
	if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
		ucam_error("Unable to set format: %s (%d).",
				strerror(errno), errno);
	
#if 0
	if(dev->config.windows_hello_enable)
	{
		if( dev->fcc == V4L2_PIX_FMT_L8_IR
	  		||dev->fcc == V4L2_PIX_FMT_L16_IR
			||dev->fcc == V4L2_PIX_FMT_MJPEG_IR)
		{
			uvc_set_windows_hello_metadata_enable(dev, 1);
		}
		else
		{
			uvc_set_windows_hello_metadata_enable(dev, 0);
		}
	}
#endif
	return ret;
}

void uvc_fill_streaming_ctrl(struct uvc_dev *dev,
		struct uvc_streaming_control *ctrl,
		int iformat, int iframe, int intervel)
{
	memset(ctrl, 0, sizeof *ctrl);

	ctrl->bmHint = 1;
	ctrl->bFormatIndex = 1;//iformat + 1;
	ctrl->bFrameIndex = 1;//iframe + 1;
	ctrl->dwFrameInterval = 333333;//frame->intervals[intervel];
	/*
	switch (format->fcc) {
		case V4L2_PIX_FMT_H264:
		case V4L2_PIX_FMT_MJPEG:
			//ctrl->dwMaxVideoFrameSize = MAX_SIZE_ONE_H264_IMAGE;
			ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
			break;
	}
	*/
	//ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
	//ctrl->dwMaxPayloadTransferSize = dev->MaxPayloadTransferSize;	/* TODO this should be filled by the driver. */
	//ctrl->bmFramingInfo = 3;
	ctrl->dwMaxVideoFrameSize = 0;
	ctrl->dwMaxPayloadTransferSize = 0;
	ctrl->bmFramingInfo = 0;
	ctrl->dwClockFrequency = UVC_CLOCK_FREQUENCY; /* 1us period pts */
	
	ctrl->bPreferedVersion = 0;
	ctrl->bMaxVersion = 0;
	ctrl->wCompQuality = dev->uvc->config.wCompQualityMax;
	ctrl->wKeyFrameRate = 0;	
	
	if(dev->config.uvc_protocol == UVC_PROTOCOL_V150)
	{
		ctrl->bUsage = 2;
		ctrl->bmSettings = 1;
		ctrl->bMaxNumberOfRefFramesPlus1 = 0x01;
		ctrl->bmRateControlModes = 0x01;
		ctrl->bmLayoutPerStream[0] = 0x00000003;
		ctrl->bmLayoutPerStream[1] = 0x00;
	}
}

static uint32_t UvcMaxPayloadTransferSize(struct uvc_dev *dev, uint32_t VideoFormat, uint16_t width, uint16_t height, uint32_t FrameInterval)
{
	uint32_t MaxPayloadTransferSize = 0;
	uint32_t MaxVideoFrameBufferSize = 0;
	uint8_t fps = 0;

	fps = (uint8_t)(10000000.0f/FrameInterval);

	ucam_strace("stream_num: %d, %s-%ux%up%u\n",
		dev->stream_num, uvc_video_format_string(VideoFormat), width, height, fps);
	
	//640 * 360 * 2 * 30 = 13824000 / 1024 = 13500 / 8 = 1687.5;
	//MaxVideoFrameBufferSize	= width*height*2*fps*1.1;//Width x Height x 2 x fps x 1.1
	//MaxVideoFrameBufferSize= MaxVideoFrameBufferSize / 1024 / 8;

	//USB3.2 todo
	if (dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER_PLUS)
	{
		if(dev->uvc->win7_os_configuration)
			MaxVideoFrameBufferSize	= width*height*2.0f*fps*dev->config.win7_usb3_isoc_speed_multiple;//Width x Height x 2 x fps x 1.1
		else
			MaxVideoFrameBufferSize	= width*height*2.0f*fps*dev->config.usb3_isoc_speed_multiple;//Width x Height x 2 x fps x 1.1

		if(VideoFormat == V4L2_PIX_FMT_MJPEG || VideoFormat == V4L2_PIX_FMT_MJPEG_IR)
			MaxVideoFrameBufferSize = MaxVideoFrameBufferSize / dev->config.mjpeg_compression_ratio;
		else if(VideoFormat == V4L2_PIX_FMT_H264 || VideoFormat == V4L2_PIX_FMT_H265)
			MaxVideoFrameBufferSize = MaxVideoFrameBufferSize / dev->config.h264_compression_ratio;
		else if(VideoFormat == V4L2_PIX_FMT_NV12 || VideoFormat == V4L2_PIX_FMT_YUV420)
			MaxVideoFrameBufferSize = (MaxVideoFrameBufferSize * 3) / 4;
		else if(VideoFormat == V4L2_PIX_FMT_L8_IR)
			MaxVideoFrameBufferSize = (MaxVideoFrameBufferSize ) / 2;

		MaxVideoFrameBufferSize= (MaxVideoFrameBufferSize / 1024 / 8) + 12;

		if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT1)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT1;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT2)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT2;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT3)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT3;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT4)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT4;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT5)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT5;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT6)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT6;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT7)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT7;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT8)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT8;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT9)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT9;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT10)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT10;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT11)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT11;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT12)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT12;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT13)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT13;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT14)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT14;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT15)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT15;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT16)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT16;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT17)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT17;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT18)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT18;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT19)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT19;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT20)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT20;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT21)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT21;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT22)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT22;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT23)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT23;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT24)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT24;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT25)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT25;
		else
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT25;
	}
	else if (dev->uvc->usb_speed == UCAM_USB_SPEED_SUPER)
	{
		if(dev->uvc->win7_os_configuration)
			MaxVideoFrameBufferSize	= width*height*2.0f*fps*dev->config.win7_usb3_isoc_speed_multiple;//Width x Height x 2 x fps x 1.1
		else
			MaxVideoFrameBufferSize	= width*height*2.0f*fps*dev->config.usb3_isoc_speed_multiple;//Width x Height x 2 x fps x 1.1

		if(VideoFormat == V4L2_PIX_FMT_MJPEG || VideoFormat == V4L2_PIX_FMT_MJPEG_IR)
			MaxVideoFrameBufferSize = MaxVideoFrameBufferSize / dev->config.mjpeg_compression_ratio;
		else if(VideoFormat == V4L2_PIX_FMT_H264  || VideoFormat == V4L2_PIX_FMT_H265)
			MaxVideoFrameBufferSize = MaxVideoFrameBufferSize / dev->config.h264_compression_ratio;
		else if(VideoFormat == V4L2_PIX_FMT_NV12 || VideoFormat == V4L2_PIX_FMT_YUV420)
			MaxVideoFrameBufferSize = (MaxVideoFrameBufferSize * 3) / 4;
		else if(VideoFormat == V4L2_PIX_FMT_L8_IR)
			MaxVideoFrameBufferSize = (MaxVideoFrameBufferSize ) / 2;

		MaxVideoFrameBufferSize= (MaxVideoFrameBufferSize / 1024 / 8) + 12;

		if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT1)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT1;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT2)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT2;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT3)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT3;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT4)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT4;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT5)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT5;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT6)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT6;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT7)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT7;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT8)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT8;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT9)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT9;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT10)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT10;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT11)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT11;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT12)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT12;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT13)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT13;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT14)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT14;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT15)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT15;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT16)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT16;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT17)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT17;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT18)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT18;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT19)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT19;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT20)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT20;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT21)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT21;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT22)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT22;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT23)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT23;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT24)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT24;
		else if(MaxVideoFrameBufferSize <= USB3_EP_ISOC_PACKET_SIZE_ALT25)
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT25;
		else
			MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT25;
	}
	else
	{
		MaxVideoFrameBufferSize	= width*height*2*fps*1.2;//Width x Height x 2 x fps x 1.1

		if(VideoFormat == V4L2_PIX_FMT_MJPEG || VideoFormat == V4L2_PIX_FMT_MJPEG_IR)
			MaxVideoFrameBufferSize = MaxVideoFrameBufferSize / dev->config.mjpeg_compression_ratio;
		else if(VideoFormat == V4L2_PIX_FMT_H264  || VideoFormat == V4L2_PIX_FMT_H265)
			MaxVideoFrameBufferSize = MaxVideoFrameBufferSize / dev->config.h264_compression_ratio;
		else if(VideoFormat == V4L2_PIX_FMT_NV12 || VideoFormat == V4L2_PIX_FMT_YUV420)
			MaxVideoFrameBufferSize = (MaxVideoFrameBufferSize * 3) / 4;
		else if(VideoFormat == V4L2_PIX_FMT_L8_IR)
			MaxVideoFrameBufferSize = (MaxVideoFrameBufferSize ) / 2;

		MaxVideoFrameBufferSize= (MaxVideoFrameBufferSize / 1024 / 8) + 12;

		if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT1)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT1;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT2)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT2;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT3)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT3;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT4)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT4;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT5)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT5;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT6)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT6;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT7)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT7;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT8)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT8;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT9)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT9;
		else if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT10)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT10;
		else //if(MaxVideoFrameBufferSize <= USB2_EP_ISOC_PACKET_SIZE_ALT11)
			MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT11;
	}

	ucam_strace("usb speed:%s ISOC MaxPayloadTransferSize: %d\n",usb_speed_string(dev->uvc->usb_speed),MaxPayloadTransferSize);
	return MaxPayloadTransferSize;
}


static uint32_t isoc_usb_packet_size_limit(struct uvc_dev *dev, struct uvc_streaming_control *probe_commit_ctrl)
{
	uint32_t MaxPayloadTransferSize = 0;

	uint32_t fcc;
	uint16_t width, height;


	uvc_vs_ctrl_analysis(dev, probe_commit_ctrl, &fcc, &width, &height);


	MaxPayloadTransferSize = UvcMaxPayloadTransferSize(dev, fcc, width, height, probe_commit_ctrl->dwFrameInterval);

	//USB3.2 todo
	
	//最小带宽限制
	if(fcc == V4L2_PIX_FMT_H264 || fcc == V4L2_PIX_FMT_H265)
	{
		//H264有可能会动态设置分辨率，需要使用最大分辨率进行设置

		//制式		最大码流	 USB带宽（4倍码流）
		//1080P30	8Mb			----4MB
		//1080P60	16Mb		----8MB
		//4KP30		32Mb		----16MB
		//4KP60		64Mb		----32MB
		if(dev->uvc->usb_speed < UCAM_USB_SPEED_SUPER)
		{
			//USB2.0
			if(dev->config.resolution_width_max > 1920 && dev->config.resolution_height_max > 1080)
			{
				//4K
				if(dev->stream_num == 0)//第一路
					MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT11;
				else//第二路
					MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT9;
			}
			else
			{
				//1080P
				if(dev->stream_num == 0)
					MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT9;
				else
					MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT6;
			}
		}
		else
		{
			//USB3.0
			if(dev->config.resolution_width_max > 1920 && dev->config.resolution_height_max > 1080)
			{
				//4K
				if(dev->config.fps_max > 30)
				{
					//P60
					MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT13;
				}
				else
				{
					//P30
					MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT9;
				}			
			}
			else
			{
				//1080P
				MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT9;
			}		
		}
	}

	if(dev->uvc->usb_speed == UCAM_USB_SPEED_HIGH)
	{
		if((fcc == V4L2_PIX_FMT_MJPEG && width >= 960 && height >= 540) || 
			((fcc == V4L2_PIX_FMT_YUYV || fcc == V4L2_PIX_FMT_NV12 || fcc == V4L2_PIX_FMT_YUV420) 
			&& width >= 640 && height >= 360))
		{
			if(MaxPayloadTransferSize < USB2_EP_ISOC_PACKET_SIZE_ALT11)
				MaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_ALT11;
		}
	}
	
	if(dev->config.usb3_isoc_speed_fast_mode && dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER && width >= 1920 && height >= 1080)
	{
		if(fcc == V4L2_PIX_FMT_MJPEG)
		{
			if (dev->uvc->win7_os_configuration == 0)
			{
				if(MaxPayloadTransferSize < USB3_EP_ISOC_PACKET_SIZE_ALT19)
					MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT19;
			}
			else
			{
				if(width > 1920 && height > 1080)
				{
					if(MaxPayloadTransferSize < USB3_EP_ISOC_PACKET_SIZE_ALT17)
						MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT17;
				}
			}
		}
		else if(fcc == V4L2_PIX_FMT_YUYV || fcc == V4L2_PIX_FMT_NV12 || fcc == V4L2_PIX_FMT_YUV420)
		{
			if (dev->uvc->win7_os_configuration == 0)
			{
				if(MaxPayloadTransferSize < USB3_EP_ISOC_PACKET_SIZE_ALT23)
					MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT23;
			}
			else
			{
				if(width > 1920 && height > 1080)
				{
					if(MaxPayloadTransferSize < USB3_EP_ISOC_PACKET_SIZE_ALT21)
						MaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_ALT21;
				}
			}
		}
	}

	
	//双流最大带宽限制
#if 0	
	if(dev->uvc->uvc_s2_enable)
	{
		if(dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER)
		{
			if(dev->config.yuv_resolution_width_max <= 1920 && dev->config.yuv_resolution_height_max <= 1080)
			{
				if(MaxPayloadTransferSize > USB3_EP_ISOC_PACKET_SIZE_ALT21)
					MaxPayloadTransferSize =  USB3_EP_ISOC_PACKET_SIZE_ALT21;
			}
		}
	}
#endif




	//最大带宽限制
	if(dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER)
	{
		if (dev->uvc->win7_os_configuration)
		{
			if(MaxPayloadTransferSize > dev->config.MaxPayloadTransferSize_usb3_win7)
				MaxPayloadTransferSize =  dev->config.MaxPayloadTransferSize_usb3_win7;
		}
		else
		{
			if(MaxPayloadTransferSize > dev->config.MaxPayloadTransferSize_usb3)
				MaxPayloadTransferSize = dev->config.MaxPayloadTransferSize_usb3;
		}
	}
	else
	{
		if(MaxPayloadTransferSize > dev->config.MaxPayloadTransferSize_usb2)
			MaxPayloadTransferSize = dev->config.MaxPayloadTransferSize_usb2;
	}
	
	return MaxPayloadTransferSize;
}


static void probe_commit_ctrl_config_value_limit(struct uvc_dev *dev,
		struct uvc_streaming_control *probe_commit_ctrl)
{
	//unsigned int fps = 0;

	uint32_t fcc = 0;
	uint16_t width, height;

	
	probe_commit_ctrl->wCompWindowSize = 0;
	probe_commit_ctrl->wDelay = 0;
	//if(probe_commit_ctrl->dwMaxPayloadTransferSize > dev->MaxPayloadTransferSize 
	//	|| probe_commit_ctrl->dwMaxPayloadTransferSize == 0){
	//	probe_commit_ctrl->dwMaxPayloadTransferSize = dev->MaxPayloadTransferSize;
	//}
	
	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
		probe_commit_ctrl->dwMaxPayloadTransferSize = dev->MaxPayloadTransferSize;

	ucam_strace("probe_commit_ctrl->dwMaxPayloadTransferSize: %d dev->uvc_ep_mode: %d \n", probe_commit_ctrl->dwMaxPayloadTransferSize, dev->config.uvc_ep_mode);	
	probe_commit_ctrl->bmHint = 0x0001;

	//视频格式索引
	if(probe_commit_ctrl->bFormatIndex < 1)
		probe_commit_ctrl->bFormatIndex = 1;

	uvc_vs_ctrl_analysis(dev, probe_commit_ctrl, &fcc, &width, &height);
	

	//fps:1~60	
	if(probe_commit_ctrl->dwFrameInterval <= UVC_FORMAT_60FPS)
		probe_commit_ctrl->dwFrameInterval = UVC_FORMAT_60FPS;
	else if(probe_commit_ctrl->dwFrameInterval >= UVC_FORMAT_1FPS)
		probe_commit_ctrl->dwFrameInterval = UVC_FORMAT_1FPS;

	//fps = (unsigned int)(10000000.0f/probe_commit_ctrl->dwFrameInterval);

	if(fcc == V4L2_PIX_FMT_NV12 || fcc == V4L2_PIX_FMT_YUV420)
	{
		probe_commit_ctrl->dwMaxVideoFrameSize = width* height * 3 / 2;

	}
	else if(fcc == V4L2_PIX_FMT_YUYV)
	{
		probe_commit_ctrl->dwMaxVideoFrameSize = width* height * 2;
	}
	else if(fcc == V4L2_PIX_FMT_H264 || fcc == V4L2_PIX_FMT_H265 || fcc == V4L2_PIX_FMT_MJPEG)
	{
		probe_commit_ctrl->dwMaxVideoFrameSize = uvc_get_v4l2_sizeimage(dev, fcc, width, height);
	}
	else
	{
		probe_commit_ctrl->dwMaxVideoFrameSize = width* height * 2;
	}

	ucam_strace("probe_commit_ctrl->dwMaxPayloadTransferSize: %d \n", probe_commit_ctrl->dwMaxPayloadTransferSize);	
	//probe_commit_ctrl->wCompQuality = UVC_COMP_QUALITY_MAX;
	//probe_commit_ctrl->wKeyFrameRate = UVC_KEY_FRAME_RATE_MAX;
	
	//压缩质量
	if(probe_commit_ctrl->wCompQuality < 0)
		probe_commit_ctrl->wCompQuality = 0;
	else if(probe_commit_ctrl->wCompQuality > dev->uvc->config.wCompQualityMax)
		probe_commit_ctrl->wCompQuality = dev->uvc->config.wCompQualityMax;
	//H264 I 帧间隔
	if(probe_commit_ctrl->wKeyFrameRate < 0)
		probe_commit_ctrl->wKeyFrameRate = 0;
	else if(probe_commit_ctrl->wKeyFrameRate > 65535)
		probe_commit_ctrl->wKeyFrameRate = 65535;
			

	
	//UVC1.5部分参数
	if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && probe_commit_ctrl->bFormatIndex  == UVC_FORMAT_INDEX_H264)
	{
		if(probe_commit_ctrl->bUsage == 0)
		{
			probe_commit_ctrl->bUsage = 1;
			probe_commit_ctrl->bmRateControlModes = 0x0004;
			probe_commit_ctrl->bmLayoutPerStream[0] = 0x00000001;
			probe_commit_ctrl->bmLayoutPerStream[1] = 0x00000000;
		}
		else if(probe_commit_ctrl->bUsage == 2)
		{
			probe_commit_ctrl->bmRateControlModes = 0x0001;
			probe_commit_ctrl->bmLayoutPerStream[0] = 0x00000003;
			probe_commit_ctrl->bmLayoutPerStream[1] = 0x00000000;
			
			//强制UCCONFIG 0
			//probe_commit_ctrl->bUsage = 1;
			//probe_commit_ctrl->bmLayoutPerStream[0] = 0x00000001;
			
			//强制2层
			//probe_commit_ctrl->bmLayoutPerStream[0] = 0x00000002;
		}
		if(probe_commit_ctrl->bBitDepthLuma < 1)
			probe_commit_ctrl->bBitDepthLuma = 8;
		if(probe_commit_ctrl->bmSettings < 1)
			probe_commit_ctrl->bmSettings = 0x21;
		if(probe_commit_ctrl->bMaxNumberOfRefFramesPlus1 < 1)
			probe_commit_ctrl->bMaxNumberOfRefFramesPlus1 = 0x01;
		if(probe_commit_ctrl->bmRateControlModes  < 1)
			probe_commit_ctrl->bmRateControlModes  = 0x0004;
		if(probe_commit_ctrl->bmLayoutPerStream[0]  < 1)
			probe_commit_ctrl->bmLayoutPerStream[0]  = 0x00000001;
	}

	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
	{
		probe_commit_ctrl->dwMaxPayloadTransferSize = isoc_usb_packet_size_limit(dev, probe_commit_ctrl);

		ucam_notice("probe_commit_ctrl->dwMaxPayloadTransferSize: %d",probe_commit_ctrl->dwMaxPayloadTransferSize);
	}
}

static UvcRequestErrorCode_t uvc_requests_ctrl_vs_probe ( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	/* <1>"4.3.1.1  Video Probe and Commit Controls ", page 103 */
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	uint8_t 	req = ctrl->bRequest; 

	struct uvc_streaming_control *vs_ctrl;
	struct uvc_streaming_control *target;

	vs_ctrl = (struct uvc_streaming_control *)data->data;

	ucam_strace("enter");


	if(data->length > sizeof ( struct uvc_streaming_control))
		data->length = sizeof ( struct uvc_streaming_control);

	switch ( req ) {
		case UVC_SET_CUR:
			if(event_data)
			{
				uint8_t length = ctrl->wLength;
		
				length = ctrl->wLength;
				if(length > sizeof (struct uvc_streaming_control ))
					length = sizeof (struct uvc_streaming_control );


				target = &dev->probe;
				memcpy( target, vs_ctrl, length);
				//ucam_trace("bFormatIndex: %d bFrameIndex: %d \n",target->bFormatIndex, target->bFrameIndex);
				
				//target->wCompQuality = UVC_COMP_QUALITY_MAX;
				
				probe_commit_ctrl_config_value_limit(dev, target);
				//target = &dev->commit;
				//memcpy(target, vs_ctrl, length);
				ucam_strace("bmLayoutPerStream[0]: %x bmLayoutPerStream[1]: %x target->bFormatIndex: %x target->bFrameIndex: %x \n",
					target->bmLayoutPerStream[0], target->bmLayoutPerStream[1], target->bFormatIndex, target->bFrameIndex);	

#if 0
				if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK){
					if(dev->uvc_stream_on){
						uvc_events_process_streamoff(dev, NULL);

					}
				}
#endif
			}
			break;

		case UVC_GET_CUR:
			memcpy(vs_ctrl, &dev->probe, sizeof ( struct uvc_streaming_control));
			ucam_strace("vs_ctrl->bmLayoutPerStream[0]: %x \n", vs_ctrl->bmLayoutPerStream[0]);
			break;

		case UVC_GET_MIN:
			//uvc_fill_streaming_ctrl(dev, vs_ctrl, UVC_FORMAT_INDEX_MJPEG - 1, UVC_MJPEG_FORMAT_INDEX_176x144 - 1, 0);//mjpeg 360P30
			memcpy(vs_ctrl, &dev->probe, sizeof ( struct uvc_streaming_control));
		
			//vs_ctrl->bFormatIndex = 1;
			//vs_ctrl->bFrameIndex = 1;
			
			
			vs_ctrl->bmHint = 0x0001;		
			vs_ctrl->wCompQuality = 0x0002;
			vs_ctrl->wKeyFrameRate = 0;
			vs_ctrl->bBitDepthLuma = 0x08;
			vs_ctrl->bmSettings = 0x21;
			vs_ctrl->bMaxNumberOfRefFramesPlus1 = 1;
			if (dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER)
			{
				vs_ctrl->dwMaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_MIN;
			}
			else
			{
				vs_ctrl->dwMaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_MIN;
			}
			if(vs_ctrl->bUsage == 0)
			{
				vs_ctrl->bUsage = 1;
				vs_ctrl->bmRateControlModes = 0x0004;
				vs_ctrl->bmLayoutPerStream[0] = 0x00000001;
				vs_ctrl->bmLayoutPerStream[1] = 0x00000000;
			}
			
			break;
		case UVC_GET_MAX:
			memcpy(vs_ctrl, &dev->probe, sizeof ( struct uvc_streaming_control));
		
			vs_ctrl->bmHint = 0x0001;
			vs_ctrl->wCompQuality = dev->uvc->config.wCompQualityMax;
			vs_ctrl->wKeyFrameRate = 0;
			vs_ctrl->bBitDepthLuma = 0x08;
			vs_ctrl->bmSettings = 0x21;
			vs_ctrl->bMaxNumberOfRefFramesPlus1 = 1;
			if (dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER)
			{
				vs_ctrl->dwMaxPayloadTransferSize = USB3_EP_ISOC_PACKET_SIZE_MAX;
			}
			else
			{
				vs_ctrl->dwMaxPayloadTransferSize = USB2_EP_ISOC_PACKET_SIZE_MAX;
			}
			if(vs_ctrl->bUsage == 0)
			{
				vs_ctrl->bUsage = 1;
				vs_ctrl->bmRateControlModes = 0x0004;
				vs_ctrl->bmLayoutPerStream[0] = 0x00000001;
				vs_ctrl->bmLayoutPerStream[1] = 0x00000000;
			}
			break;
		case UVC_GET_DEF:
			uvc_fill_streaming_ctrl(dev, vs_ctrl, 0, 0, 0);
			//memcpy(vs_ctrl, &dev->probe, sizeof ( struct uvc_streaming_control));
			vs_ctrl->bmHint = 0x0001;
			vs_ctrl->wCompQuality = dev->uvc->config.wCompQualityMax;
			vs_ctrl->wKeyFrameRate = 0;
			
			vs_ctrl->bFormatIndex = 1;
            vs_ctrl->bFrameIndex = 1;
            //vs_ctrl->dwMaxVideoFrameSize = 1920 * 1080 *2;
            vs_ctrl->dwFrameInterval = UVC_FORMAT_30FPS;
			vs_ctrl->bUsage = 2;
			//vs_ctrl->bUsage = 1;
			vs_ctrl->bmSettings = 1;
			vs_ctrl->bMaxNumberOfRefFramesPlus1 = 1;
			vs_ctrl->bmLayoutPerStream[0] = 0x00000003;
			vs_ctrl->bmLayoutPerStream[1] = 0x00;
			break;

		case UVC_GET_RES:
			memset(vs_ctrl, 0, sizeof ( struct uvc_streaming_control));
			break;

		case UVC_GET_LEN:
			ucam_trace("UVC_GET_LEN dev->uvc_protocol: %d \n", dev->config.uvc_protocol);
			
			if(dev->config.uvc_protocol <= UVC_PROTOCOL_V100)
				data->data[0] = 26;
			else if(dev->config.uvc_protocol <= UVC_PROTOCOL_V110)
				data->data[0] = 34;
			else 
				data->data[0] = 48;
			
			data->length = 1;
			break;

		case UVC_GET_INFO:
			data->data[0] = 0x03;
			data->length = 1;
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	
	return ret;
}

extern int32_t  uvc_events_process_streamon(struct uvc_dev *dev, 
	struct uvc_request_data *data);

static UvcRequestErrorCode_t uvc_requests_ctrl_vs_commit ( struct uvc_dev *dev, 
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	uint8_t 	req = ctrl->bRequest; 
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	struct uvc_streaming_control *vs_ctrl;
	struct uvc_streaming_control *target;
	uint32_t fcc;
	uint16_t width, height;
	uint8_t length;

	vs_ctrl = (struct uvc_streaming_control *)data->data;


	if(data->length > sizeof ( struct uvc_streaming_control))
		data->length = sizeof ( struct uvc_streaming_control);

	ucam_strace("enter");
	switch (req) {
		case UVC_SET_CUR:
			if(event_data)
			{
				length = ctrl->wLength;
	
				if(length > sizeof (struct uvc_streaming_control ))
					length = sizeof (struct uvc_streaming_control );
				
				target = &dev->commit;
			
				memcpy( target, vs_ctrl, length);

				if(uvc_vs_ctrl_analysis(dev, target, &fcc, &width, &height) != 0)
				{
					ucam_error("uvc_vs_ctrl_analysis fail");
					return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
				}
				dev->fcc = fcc;
				dev->width = width;
				dev->height = height;
				probe_commit_ctrl_config_value_limit(dev, target);	
				if(dev->config.uvc150_simulcast_en)
				{
					if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && dev->stream_num)
					{
						//if( vs_ctrl->bFormatIndex == UVC_FORMAT_INDEX_H264_SIMULCAST_S2)
						if(vs_ctrl->bFormatIndex > UVC_FORMAT_INDEX_H264_S2)
						{
							uvc_set_simulcast_enable(dev, 1);	
						}
						else 
						{
							uvc_set_simulcast_enable(dev, 0);
						}
					}
				}					
				if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK){
					dev->uvc->usb_suspend = 0;
#if 1
					if(dev->uvc_stream_on){
						uvc_events_process_streamoff(dev, NULL);

					}
					uvc_stream_on_trigger(dev);
#else
					//usleep(100*1000);
					if(dev->uvc_stream_on){
						uvc_events_process_streamoff(dev, data);
						usleep( 1*1000 );
					}		
					uvc_video_set_format(dev, &dev->commit);
					uvc_events_process_streamon(dev, data);
					if(dev->uvc->uvc_stream_event_callback)
					{
						dev->uvc->uvc_stream_event_callback(dev->uvc, dev->stream_num, 1);  
					}	
#endif
				}else{
					uvc_video_set_format(dev,target);	
				}

			}		
			break;

		case UVC_GET_CUR:
			memcpy(vs_ctrl, &dev->commit, sizeof ( struct uvc_streaming_control));
			//data->length = 26;
			//data->length = ctrl->wLength;	
			vs_ctrl->bmHint = 0x0001;
			//vs_ctrl->dwMaxPayloadTransferSize = dev->MaxPayloadTransferSize;
			vs_ctrl->wCompQuality = 0;
			vs_ctrl->wKeyFrameRate = 0;	
			break;

		case UVC_GET_LEN:

			if(dev->config.uvc_protocol <= UVC_PROTOCOL_V100)
				data->data[0] = 26;
			else if(dev->config.uvc_protocol <= UVC_PROTOCOL_V110)
				data->data[0] = 34;
			else 
				data->data[0] = 48;

			data->length = 1;
			break;

		case UVC_GET_INFO:
			data->data[0] = 0x03;
			data->length = 1;
			break;
		default:
			ucam_error("invalid request");
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}



static int init_ctrls_vs(struct uvc_dev *dev)
{
	//return uvc_streaming_desc_init(dev);
	return 0;
}

static uvc_requests_ctrl_fn_t uvc_requests_ctrl_vs [] = {
	NULL,
	uvc_requests_ctrl_vs_probe,
	uvc_requests_ctrl_vs_commit,
};

static struct uvc_ctrl_attr_t uvc_ctrl_vs = {
	.id = 0,
	.selectors_max_size = sizeof(uvc_requests_ctrl_vs)/sizeof(uvc_requests_ctrl_fn_t),
	.init = init_ctrls_vs,
	.requests_ctrl_fn = uvc_requests_ctrl_vs,
};


struct uvc_ctrl_attr_t *get_uvc_ctrl_vs(void)
{
    return &uvc_ctrl_vs;
}
