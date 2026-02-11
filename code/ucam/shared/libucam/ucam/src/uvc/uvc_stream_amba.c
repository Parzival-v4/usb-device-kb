/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 13:45:47
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream_amba.c
 * @Description : 
 */ 
#ifdef AMBARELLA_SDK

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <signal.h>

#include <linux/usb/ch9.h>
//#include <ucam/videodev2.h>

#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_ctrl_vs.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_stream.h>
#include <ucam/uvc/uvc_api_config.h>
#include <ucam/uvc/uvc_api_eu.h>
#include "uvc_stream.h"
#include "uvc_stream_yuv.h"


/**
 * @description: 将编码视频发给USB主机，必须在stream on状态下去才允许发送
 * @param {venc_chn：venc通道号；fcc：视频格式；pVStreamData：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int amba_send_stream_encode(struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc, void* pStreamData)
{	
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	

	//uint16_t uvc_h264_profile;
	uint8_t* naluData = NULL;
	uint8_t nalu_type = 0;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	struct uvc_encode_frame_handle *pVStreamData = (struct uvc_encode_frame_handle *)pStreamData;
	
	if(dev == NULL || pVStreamData == NULL)
	{
		return -ENOMEM;
	}
	
    if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	ret = uvc_api_get_stream_status(dev, dev->config.venc_chn, &stream_status);
	if(ret != 0 || stream_status == 0)
	{
		return -EALREADY;
	}

	if((dev->fcc != V4L2_PIX_FMT_MJPEG && dev->fcc != V4L2_PIX_FMT_H264 && dev->fcc != V4L2_PIX_FMT_H265)
		 || fcc != dev->fcc)
	{
		ucam_error("uvc format error, uvc format:%s, send format:%s",
			uvc_video_format_string(dev->fcc), uvc_video_format_string(fcc));
		return -1;	
	}

	switch (fcc)
	{
		case V4L2_PIX_FMT_MJPEG:
			break;
		case V4L2_PIX_FMT_H264:
			if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && 
				dev->bConfigurationValue == 2 && dev->uvc->uvc_v150_enable)
			{
				if(dev->stream_num > 0 && 
					dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon)
				{
					if(get_simulcast_start_or_stop_layer(dev, venc_chn) == 0)
					{
						ucam_strace("### chn = [%d], simulcast is stop####\n",venc_chn);	
						return 0;
					}
				}
			}
			if(dev->first_IDR_flag)
			{
				naluData = (uint8_t *)(pVStreamData->buf);
				nalu_type = naluData[4] & 0x1f;
				if(nalu_type != 0x07)
				{
					uvc_api_set_h264_frame_requests(dev, venc_chn, UVCX_H264_PICTURE_TYPE_IDRFRAME);
					return 0;
				}
				dev->first_IDR_flag = 0;
			}
			break;
		case V4L2_PIX_FMT_H265:
			if(dev->first_IDR_flag)
			{
				naluData = (uint8_t *)(pVStreamData->buf);			
				nalu_type = (naluData[4]>>1) & 0x3f;
				if(nalu_type != 32)
				{
					uvc_api_set_h264_frame_requests(dev, venc_chn, UVCX_H264_PICTURE_TYPE_IDRFRAME);
					return 0;
				}
				dev->first_IDR_flag = 0;
			}
			break;
		default:
            ucam_error("unknown format %d", fcc);
			return -1;
            //break;
	}


	ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}
	
	v4l2_buf.bytesused = pVStreamData->size;
	if(v4l2_buf.bytesused  > v4l2_buf.length)
	{
		ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length); 
		ret = -1; 
		goto error_release;
	}
	uvc_us_to_timeval(pVStreamData->u64PTS, &v4l2_buf.timestamp);

	uvc_memcpy(uvc_mem, pVStreamData->buf, v4l2_buf.bytesused);
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		reqbufs_buf_used.index = v4l2_buf.index;
		reqbufs_buf_used.buf_used = v4l2_buf.bytesused;
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_release;
		}
	}

	ret = uvc_video_qbuf_lock(dev, venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		return -1;
	}	
	return ret;

error_release:
	if(uvc_mem)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		}	
	}
	else
	{
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	}
	return ret;
}

/**
 * @description: 将YUY2发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int amba_send_stream_yuy2_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	uint16_t uvc_width;
	uint16_t uvc_height;

	const uint8_t* src_y;
	int src_stride_y;
    const uint8_t* src_uv;
	int src_stride_uv;
    uint8_t* dst_yuy2;
    int dst_stride_yuy2;
    uint16_t width_offset = 0;
	uint16_t height_offset = 0;
	
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;

	struct uvc_yuv_frame_handle *pVBuf = (struct uvc_yuv_frame_handle*)pBuf;
	
	if(dev == NULL || pVBuf == NULL)
	{
		return -ENOMEM;
	}
	
    if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	ret = uvc_api_get_stream_status(dev, dev->config.venc_chn, &stream_status);
	if(ret != 0 || stream_status == 0)
	{
		return -EALREADY;
	}

	if(dev->fcc != V4L2_PIX_FMT_YUYV)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->format != 0x01)
	{
		ucam_error("format error, format:%d",pVBuf->format);
		return -1;
	}

	uvc_width = ALIGN_UP(dev->width, 16);
	uvc_height = dev->height;//ALIGN_UP(dev->height, 16);
	if(pVBuf->width != uvc_width || pVBuf->height != uvc_height)
	{
		ucam_strace("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->width, pVBuf->height, dev->width, dev->height);
		//return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		//return ret;
	} 
    
    //w*h*2
    dst_stride_yuy2 = dev->width << 1;
    dst_yuy2 = (uint8_t*)uvc_mem;
    v4l2_buf.bytesused = (dev->width * dev->height)  << 1;
   

    //w*h*3/2
    src_stride_y = pVBuf->width;
	src_stride_uv = pVBuf->width;
    src_y = pVBuf->buf;
    src_uv = src_y + pVBuf->width * pVBuf->height;

	//扣取中间的画面
	if(pVBuf->width > dev->width)
    {
        width_offset = (pVBuf->width - dev->width) >> 1;
    }

	if(pVBuf->height > dev->height)
	{
		height_offset = (pVBuf->height - dev->height) >> 1;
	}
	
	if(width_offset || height_offset)
	{
		src_y += width_offset + height_offset * src_stride_y;
		src_uv += width_offset + (height_offset * src_stride_uv)/2;
	}
	
	ret = UVC_NV12ToYUY2(src_y,
               src_stride_y,
               src_uv,
               src_stride_uv,
               dst_yuy2,
               dst_stride_yuy2,
               dev->width,
               dev->height);
	if(ret != 0)
	{
		ucam_error("UVC_NV12ToYUY2 error, ret=%d", ret);
		goto error_release;
	}

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		reqbufs_buf_used.index = v4l2_buf.index;
		reqbufs_buf_used.buf_used = v4l2_buf.bytesused;
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_release;
		}
	}

	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}

	return ret;


error_release:
	if(uvc_mem)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		}	
	}
	else
	{
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	}
	return ret;
}

/**
 * @description: 将NV12发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int amba_send_stream_nv12_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	uint16_t uvc_width;
	uint16_t uvc_height;

	const uint8_t* src_y;
	int src_stride_y;
    const uint8_t* src_uv;
	int src_stride_uv;

	uint8_t* dst_y;
	int dst_stride_y;
    uint8_t* dst_uv;
	int dst_stride_uv;

    uint16_t width_offset = 0;
	uint16_t height_offset = 0;

	int y;
	
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;

	struct uvc_yuv_frame_handle *pVBuf = (struct uvc_yuv_frame_handle*)pBuf;
	
	if(dev == NULL || pVBuf == NULL)
	{
		return -ENOMEM;
	}
	
    if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	ret = uvc_api_get_stream_status(dev, dev->config.venc_chn, &stream_status);
	if(ret != 0 || stream_status == 0)
	{
		return -EALREADY;
	}

	if(dev->fcc != V4L2_PIX_FMT_NV12)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->format != 0x01)
	{
		ucam_error("format error, format:%d",pVBuf->format);
		return -1;
	}

	uvc_width = ALIGN_UP(dev->width, 16);
	uvc_height = dev->height;//ALIGN_UP(dev->height, 16);
	if(pVBuf->width != uvc_width || pVBuf->height != uvc_height)
	{
		ucam_strace("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->width, pVBuf->height, dev->width, dev->height);
		//return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	} 
    
    //w*h*2 
    dst_y = (uint8_t*)uvc_mem;
	dst_stride_y = dev->width;

	dst_uv = dst_y + (dev->width * dev->height);
	dst_stride_uv = dev->width;
    v4l2_buf.bytesused = (dev->width * dev->height) * 3/2;
   

    //w*h*3/2
    src_stride_y = pVBuf->width;
	src_stride_uv = pVBuf->width;
    src_y = pVBuf->buf;
    src_uv = src_y + pVBuf->width * pVBuf->height;

	//扣取中间的画面
	if(pVBuf->width > dev->width)
    {
        width_offset = (pVBuf->width - dev->width) >> 1;
    }

	if(pVBuf->height > dev->height)
	{
		height_offset = (pVBuf->height - dev->height) >> 1;
	}
	
	if(width_offset || height_offset)
	{
		src_y += width_offset + height_offset * src_stride_y;
		src_uv += width_offset + (height_offset * src_stride_uv)/2;
	}
	
	for (y = 0; y < dev->height - 1; y += 2) {
		memcpy(dst_y, src_y, dev->width);
		memcpy(dst_uv, src_uv, dev->width);
		memcpy(dst_y + dst_stride_y, src_y + src_stride_y, dev->width);
		
		src_y += src_stride_y << 1;
		src_uv += src_stride_uv;
		dst_y += dst_stride_y << 1;
		dst_uv += dst_stride_uv;
	}

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		reqbufs_buf_used.index = v4l2_buf.index;
		reqbufs_buf_used.buf_used = v4l2_buf.bytesused;
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_release;
		}
	}
	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}

	return ret;

#if 1

error_release:
	if(uvc_mem)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		}	
	}
	else
	{
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	}
	return ret;
#endif
}

int uvc_send_stream_ops_int(struct uvc_dev *dev)
{
	switch (dev->config.reqbufs_memory_type)
	{
	case V4L2_REQBUFS_MEMORY_TYPE_USERPTR:
	case V4L2_REQBUFS_MEMORY_TYPE_MMAP:
		dev->send_stream_ops.send_encode = amba_send_stream_encode;
		dev->send_stream_ops.send_yuy2 = amba_send_stream_yuy2_mmap;
		dev->send_stream_ops.send_nv12 = amba_send_stream_nv12_mmap;
		//dev->send_stream_ops.send_l8_ir = NULL;
		break;	
	default:
		//不支持其他的类型
		return -1;
	}
	
	return 0;	
}

#endif /* #ifdef AMBARELLA_SDK */