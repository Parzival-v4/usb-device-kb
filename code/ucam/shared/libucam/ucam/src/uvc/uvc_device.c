/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-17 18:57:21
 * @FilePath    : \release\lib_ucam\ucam\src\uvc\uvc_device.c
 * @Description : 
 */ 
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
#include <ucam/uac/uac_device.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_stream.h>
#include <ucam/uvc/uvc_api_config.h>
#include <ucam/uvc/uvc_api_eu.h>
#include <ucam/uvc/uvc_api_vs.h>

#include "uvc_stream.h"

static const uint16_t usb_id_list[][2] = {
	//VID bypass
    {0x2e7e,0xFFFF},//VHD
	{0x3242,0xFFFF},//HIS
	{0x146E,0xFFFF},//ClearOne
	{0x28DB,0xFFFF},//Konftel
	{0x145F,0xFFFF},//Trust
	{0x0543,0xFFFF},//ViewSonic
	{0x1395,0xFFFF},//EPOS EXPAND
	{0x095D,0xFFFF},//polycom
	{0x2757,0xFFFF},//鸿合
	{0x343F,0xFFFF},//好视通
	{0x24AE,0xFFFF},//雷柏
	{0x2D34,0xFFFF},//Nureva
	{0x2E03,0xFFFF},//M1000M DH-VCS-C5A0

	{0x109B,0xFFFF},//JX1700US haixin
	{0x33F1,0xFFFF},//JX1700USX haixin
	{0x2BDF,0xFFFF},//JX1700US 海康
	{0x291A,0xFFFF},//M1000 anker
	{0x4252,0xFFFF},//JX1701USV_QH
	{0x28E6,0xFFFF},//JX1700US Biamp Vidi150
	{0x17EF,0xFFFF},//J1700CL联想定制
	{0X1FF7,0xFFFF},//视联动力
	{0x25C1,0xFFFF},//Vaddio
	{0x4D53,0xFFFF},//MSolutions
	{0x3605,0xFFFF},//鸟狗
	{0x0d48,0xFFFF},//Promethean

	{0x046d,0x085e},//LOGITECH
};


int check_usb_id(uint16_t vid, uint16_t pid)
{
	int i;

	for(i = 0; i < (sizeof(usb_id_list))/4; i ++)
	{	
		if(usb_id_list[i][0] == vid)
		{
			if(usb_id_list[i][1] == pid || usb_id_list[i][1] == 0xffff)
			{
				return 1;
			}
		}	
	}

	return 0;
}


static int uvc_video_reqbufs(struct uvc_dev *dev, int nbufs);
void *uvc_ctrl_handle_thread(void *arg);
int32_t  uvc_events_process_streamon(struct uvc_dev *dev, 
	struct uvc_request_data *data);
int32_t uvc_events_process_streamoff(struct uvc_dev *dev, 
	struct uvc_request_data *data);

const char *usb_speed_string(enum ucam_usb_device_speed usb_speed)
{
	switch(usb_speed){
		case UCAM_USB_SPEED_UNKNOWN:
			return ("unknown-speed");
		case UCAM_USB_SPEED_LOW:
			return ("low-speed");	
		case UCAM_USB_SPEED_FULL:
			return ("full-speed");	
		case UCAM_USB_SPEED_HIGH:
			return ("high-speed");					
		case UCAM_USB_SPEED_WIRELESS:
			return ("wireless-speed");	
		case UCAM_USB_SPEED_SUPER:
			return ("super-speed");	
		case UCAM_USB_SPEED_SUPER_PLUS:
			return ("super-plus-speed");								
		default: 
			return("unknown");
	}	
}

const char *uvc_request_error_code_string(UvcRequestErrorCode_t ErrorCode)
{
	switch(ErrorCode){
		case UVC_REQUEST_ERROR_CODE_NO_ERROR:
			return ("No error");
		case UVC_REQUEST_ERROR_CODE_NOT_READY:
			return ("Not ready");	
		case UVC_REQUEST_ERROR_CODE_WRONG_STATE:
			return ("Wrong state");	
		case UVC_REQUEST_ERROR_CODE_POWER:
			return ("Power");					
		case UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE:
			return ("Out of range");	
		case UVC_REQUEST_ERROR_CODE_INVALID_UNIT:
			return ("Invalid unit");
		case UVC_REQUEST_ERROR_CODE_INVALID_CONTROL:
			return ("Invalid control");
		case UVC_REQUEST_ERROR_CODE_INVALID_REQUEST:
			return ("Invalid Request");
		case UVC_REQUEST_ERROR_CODE_INVALID_VALUE_WITHIN_RANGE:
			return ("Invalid value within range");			
		default: 
			return("unknown");
	}	
}

const char *uvc_video_format_string(uint32_t fcc)
{
	switch(fcc){
		case V4L2_PIX_FMT_H264:
			return ("H.264");
		//case V4L2_PIX_FMT_MPEG:
		//	return ("MPEG");
		case V4L2_PIX_FMT_MJPEG:
			return ("MJPEG");	
		case V4L2_PIX_FMT_YUYV:
			return ("YUYV");
		case V4L2_PIX_FMT_NV12:
			return ("NV12");
		case V4L2_PIX_FMT_NV21:
			return ("NV21");
		case V4L2_PIX_FMT_H265:
			return ("H.265");
		case V4L2_PIX_FMT_L8_IR:
			return ("L8_IR");
		case V4L2_PIX_FMT_L16_IR:
			return ("L16_IR");
		case V4L2_PIX_FMT_MJPEG_IR:
			return ("MJPEG_IR");
		case V4L2_PIX_FMT_YUV420:
			return ("YUV420");
		default: 
			return("unknown");
	}
}

const char *uvc_request_to_string(uint8_t bRequest)
{
	switch(bRequest){
		case UVC_SET_CUR:
			return ("SET CUR");
		case UVC_GET_CUR:
            return ("GET CUR");
        case UVC_GET_MIN:
            return ("GET MIN");
        case UVC_GET_MAX:
            return ("GET MAX");
        case UVC_GET_DEF:
            return ("GET DEF");
        case UVC_GET_RES:
            return ("GET RES");
        case UVC_GET_LEN:
            return ("GET LEN");
        case UVC_GET_INFO:
            return ("GET INFO");
		default: 
			return("unknown request");
	}
}

const char *v4l2_event_to_string(uint32_t type)
{
	switch(type){
		case UVC_EVENT_CONNECT:
			return ("CONNECT");
		case UVC_EVENT_DISCONNECT:
            return ("DISCONNECT");
        case UVC_EVENT_SETUP:
            return ("SETUP");
        case UVC_EVENT_DATA:
            return ("DATA");
        case UVC_EVENT_STREAMON:
            return ("STREAMON");
        case UVC_EVENT_STREAMOFF:
            return ("STREAMOFF");
		default: 
			return("unknown event");
	}
}

void uvc_stream_on_trigger(struct uvc_dev *dev)
{	
    clock_gettime(CLOCK_MONOTONIC, &dev->stream_on_trigger_time);
	dev->uvc_stream_on_trigger_flag = 1;
	//uvc_video_set_format(dev, &dev->commit);
	//uvc_events_process_streamon(dev, NULL);	
}

int stream_on_trigger_process(struct uvc_dev *dev)
{
    if(dev->uvc_stream_on_trigger_flag)
    {
        ucam_trace("stream start\n");
        dev->uvc_stream_on_trigger_flag = 0;
        //usleep(100*1000);
        if(dev->uvc_stream_on){
            uvc_events_process_streamoff(dev, NULL);
            usleep( 1*1000 );
        }		
        uvc_video_set_format(dev, &dev->commit);
        uvc_events_process_streamon(dev, NULL);
        if(dev->uvc->uvc_stream_event_callback)
        {
            dev->uvc->uvc_stream_event_callback(dev->uvc, dev->stream_num, 1);  
        }			
        		
    }
	return 0;	
}

static struct uvc_dev * malloc_dev(void)
{
	struct uvc_dev *dev =NULL;
	
	dev = malloc(sizeof *dev);
	if (dev == NULL) {
                ucam_trace("uvc_open:Malloc Error!");
		return NULL;
	}

	memset(dev, 0, sizeof *dev);
	return dev;
}

int uvc_open(const char *devname)
{
	int fd = -1;

	fd = open(devname, O_RDWR | O_NONBLOCK);
	if (fd == -1) {
		ucam_error("v4l2 open failed: %s (%d)", strerror(errno), errno);
		return -1;
	}

	ucam_strace("open succeeded, file descriptor = %d", fd);

	return fd;
}

void uvc_close(struct uvc_dev *dev)
{
	ucam_trace("enter");
	
	dev->init_flag = 0 ;
	dev->uvc_stream_on = 0;

#if 0	
	//断开连接USB
	int type = USB_STATE_DISCONNECT;
	if ((ioctl(dev->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
				ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
			strerror(errno), errno);	
	}
#endif


	uvc_events_process_streamoff(dev, NULL);
	
	if (!dev->uvc_ctrl_handle_thread_exit &&
        dev->uvc_ctrl_handle_thread_pid)
	{
        dev->uvc_ctrl_handle_thread_exit = 1;
		ucam_notice("thread exit, config: %d, dev: %d\n", dev->bConfigurationValue, dev->video_dev_index);
        pthread_join(dev->uvc_ctrl_handle_thread_pid, NULL);
        dev->uvc_ctrl_handle_thread_pid = 0;
	}


	if(dev->nbufs > 0)
		uvc_video_reqbufs(dev, 0);

	
	close(dev->fd);
    dev->fd = -1;


	SAFE_FREE(dev->ss_streaming);
	SAFE_FREE(dev->hs_streaming);
	SAFE_FREE(dev->hs_controls_desc);
	SAFE_FREE(dev->ss_controls_desc);
	SAFE_FREE(dev);
	
	//free(UvcStreamBuff);	
    ucam_trace("leave");
	ucam_trace("uvc_close!\n");
}

/* ---------------------------------------------------------------------------
 * Video streaming
 */

static int uvc_v4l2_init(struct uvc_dev *dev)
{
	int ret;	
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof fmt);
	
		
	fmt.fmt.pix.width = 1920;
	fmt.fmt.pix.height = 1080;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	fmt.fmt.pix.sizeimage = dev->config.uvc_max_format_bufsize;	
	dev->sizeimage = fmt.fmt.pix.sizeimage;
	
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if ((ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt)) < 0)
			ucam_error("VIDIOC_S_FMT: %s (%d) failed!", strerror(errno), errno);

	//uvc_video_reqbufs(dev, dev->video_fifo_num);

	return ret;
}

void uvc_video_reqbufs_flush_cache(struct uvc_dev *dev, uint32_t buf_index)
{
#if (REQBUFS_CACHE_ENABLE == 1)
	if(dev->reqbufs_config.cache_enable)
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, dev->video_reqbufs_addr[buf_index], 
			dev->mem[buf_index], dev->video_reqbufs_length[buf_index]);	
	}	
#endif
}

static int uvc_video_reqbufs(struct uvc_dev *dev, int nbufs)
{
	struct v4l2_requestbuffers rb;
	struct v4l2_buffer buf;
	unsigned int i;
	int ret;

	char mmz_name[32];
	struct uvc_ioctl_reqbufs_addr_t reqbufs_addr;


	if(dev->bConfigurationValue == 3)
	{
		//win7 config 3驱动已经转换到config 1，不需要申请buf
		return 0;
	}

	for (i = 0; i < dev->nbufs; ++i)
	{
		if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
		{
			if(dev->mem[i])
				dev->uvc->uvc_mmz_free_callback(dev, dev->video_reqbufs_addr[i], dev->mem[i], dev->video_reqbufs_length[i]);
		}
		else
		{
			munmap(dev->mem[i], dev->bufsize);
		}
	}

	if(dev->mem)
	{
		free(dev->mem);
		dev->mem = 0;
		dev->nbufs = 0;
	}

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		if(dev->video_reqbufs_addr)
		{
			free(dev->video_reqbufs_addr);
			dev->video_reqbufs_addr = 0;
		}

		if(dev->video_reqbufs_length)
		{
			free(dev->video_reqbufs_length);
			dev->video_reqbufs_length = 0;
		}
	}




	memset(&rb, 0, sizeof rb);
	rb.count = nbufs;
	rb.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	rb.memory = dev->config.reqbufs_memory_type;

	if(dev->uvc->config.chip_id ==  0x3518E300)
	{
		if(rb.count > 1 && dev->sizeimage > 2048*1024)
		{
			rb.count = 1;
		}
	}

	ret = ioctl(dev->fd, VIDIOC_REQBUFS, &rb);
	if (ret < 0) {
		ucam_error("Unable to allocate buffers: %s (%d).",
				strerror(errno), errno);
		return ret;
	}

	ucam_strace("%u buffers allocated.", rb.count);

	if(rb.count == 0)
	{
		return 0;
	}

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		dev->video_reqbufs_addr = malloc(rb.count * sizeof (unsigned long long));
		if (dev->video_reqbufs_addr == NULL) {
			ucam_trace("uvc_video_reqbufs:Malloc Error!");
			return -1;
		}

		memset(dev->video_reqbufs_addr, 0, (rb.count * sizeof (unsigned long long)));

		dev->video_reqbufs_length = malloc(rb.count * sizeof (uint32_t));
		if (dev->video_reqbufs_length == NULL) {
			ucam_trace("uvc_video_reqbufs:Malloc Error!");
			return -1;
		}
	}


	/* Map the buffers. */
	dev->mem = malloc(rb.count * sizeof dev->mem[0]);
	if (dev->mem == NULL) {
			ucam_trace("uvc_video_reqbufs:Malloc Error!");
			//return -1;
	}

	for (i = 0; i < rb.count; ++i) {
		memset(&buf, 0, sizeof buf);
		buf.index = i;
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		buf.memory = dev->config.reqbufs_memory_type;
		ret = ioctl(dev->fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			ucam_error("Unable to query buffer %u: %s (%d).", i,
					strerror(errno), errno);
			return -1;
		}			
		
		ucam_strace("length: %u offset: %u", buf.length, buf.m.offset);

		if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
		{
			memset(mmz_name, 0, sizeof(mmz_name));
			sprintf(mmz_name, "uvc reqbufs %d-%d", dev->stream_num, i);

#if REQBUFS_CACHE_ENABLE == 1	
			if(dev->reqbufs_config.cache_enable)
			{	
				ret = dev->uvc->uvc_mmz_alloc_cached_callback(dev, &dev->video_reqbufs_addr[i], ((void**)&dev->mem[i]), mmz_name, NULL, buf.length);	
				if (0 != ret || dev->mem[i] == NULL)
				{   
					ucam_error("uvc_mmz_alloc_cached_callback fail, ret = %d, p = %p",ret, dev->mem[i]);
					return -1;
				}
			}
			else
			{

				ret = dev->uvc->uvc_mmz_alloc_callback(dev, &dev->video_reqbufs_addr[i], ((void**)&dev->mem[i]), mmz_name, NULL, buf.length);
				if (0 != ret || dev->mem[i] == NULL)
				{   
					ucam_error("uvc_mmz_alloc_callback fail, ret = %d, p = %p",ret, dev->mem[i]);

					return -1;
				}
			}
		

#else	
			ret = dev->uvc->uvc_mmz_alloc_callback(dev, (MMZ_PHY_ADDR_TYPE *)&dev->video_reqbufs_addr[i], ((void**)&dev->mem[i]), mmz_name, NULL, buf.length);
			if (0 != ret || dev->mem[i] == NULL)
			{   
				ucam_error("uvc_mmz_alloc_callback fail, ret = %d, p = %p",ret, dev->mem[i]);
				return -1;
			}		
#endif	
			dev->video_reqbufs_length[i] = buf.length;		
			//ucam_strace("video_reqbufs_addr:0x%llx, length:%d", dev->video_reqbufs_addr[i], dev->video_reqbufs_length[i]);

			reqbufs_addr.index = i;
			reqbufs_addr.addr = dev->video_reqbufs_addr[i];
			reqbufs_addr.length = dev->video_reqbufs_length[i];
			if(uvc_ioctl_set_reqbufs_addr(dev, &reqbufs_addr) < 0)
			{
				ucam_error("uvc_ioctl_set_reqbufs_addr error");
				//return -1;
			}
		}
		else
		{

			//mmap分配缓存
			dev->mem[i] = mmap(0, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);
			if (dev->mem[i] == MAP_FAILED) {
				ucam_error("Unable to map buffer %u: %s (%d)", i,
						strerror(errno), errno);
				return -1;
			}
			memset(dev->mem[i], 0, buf.length);
			//ucam_strace("Buffer %u mapped at address %p.", i, dev->mem[i]);
		}
	}

	dev->bufsize = buf.length;
	dev->nbufs = rb.count;

	return 0;
}

int uvc_video_stream(struct uvc_dev *dev, int enable)
{
	struct v4l2_buffer buf;
	unsigned int i;
	int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	int ret;
	
	ucam_strace("enter, enable:%d, stream_num: %d \n ", enable, dev->stream_num);

	if (!enable) {
		
		ucam_trace("Stopping video stream\n");
		if(dev->uvc_stream_on == 0)
			return 0;

		dev->uvc_stream_on = 0;
        dev->uvc_stream_on_trigger_flag = 0;
		
		if(dev->config.uvc150_simulcast_en)
		{
			uvc_api_set_stream_status(dev, dev->config.simulcast_stream_0_venc_chn, 0);
			uvc_api_set_stream_status(dev, dev->config.simulcast_stream_1_venc_chn, 0);
			uvc_api_set_stream_status(dev, dev->config.simulcast_stream_2_venc_chn, 0);
		}
		else
		{
			uvc_api_set_stream_status (dev, dev->config.venc_chn, 0);
		}

		pthread_mutex_lock(&dev->uvc_reqbufs_mutex);
		
		dev->uvc_stream_on = 0;
        dev->uvc_stream_on_trigger_flag = 0;
		
		//先退出码流发送再关闭编码器，应先关闭后端再关闭前端，Stream-->V4L2-->USB-->Host
		
		
		dev->first_IDR_flag = 0;

		//usleep(100*1000);
		
		if (( ret = ioctl(dev->fd, VIDIOC_STREAMOFF, &type)) < 0) {
			ucam_error("ioctl VIDIOC_STREAMOFF: %s (%d).",
					strerror(errno), errno);
		}

		if(dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_DYNAMIC)
		{
			if(dev->config.uvc_ep_mode != UVC_USB_TRANSFER_MODE_BULK)
				uvc_video_reqbufs(dev, 0);	
		}

		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);	
		return 0;
	}

	
	if(enable){	
		pthread_mutex_lock(&dev->uvc_reqbufs_mutex);			
		ucam_trace("Starting video stream\n");
		if(dev->uvc_stream_on == 1)
		{
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			return 0;
		}
		

		if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
		{
			dev->reqbufs_config.enable = 1;
#ifdef SIGMASTAR_SDK
			dev->reqbufs_config.cache_enable = 0;
#else	
			if (dev->fcc == V4L2_PIX_FMT_YUYV || 
				dev->fcc == V4L2_PIX_FMT_NV12 ||
				dev->fcc == V4L2_PIX_FMT_YUV420)
			{
				dev->reqbufs_config.cache_enable = REQBUFS_CACHE_ENABLE;		
			}
			else
			{
				dev->reqbufs_config.cache_enable = 0;
			}
#endif

			if(dev->reqbufs_config.enable)
			{
				dev->reqbufs_config.data_offset = REQBUFS_USERPTR_DATA_OFFSET;
			}
			else
			{
				dev->reqbufs_config.data_offset = 0;
			}
#if 1			
			if(dev->stream_num && dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon)
			{
				if(dev->reqbufs_config.data_offset < 16)
					dev->reqbufs_config.data_offset = 16;
			}
#endif
			uvc_ioctl_set_reqbufs_config(dev, &dev->reqbufs_config);
		}


	
		if(dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_DYNAMIC)
		{
			if(dev->config.uvc_ep_mode != UVC_USB_TRANSFER_MODE_BULK)
			{
				if(uvc_video_reqbufs(dev, dev->config.uvc_max_queue_num) != 0)
				{
					pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
					return -1;
				}	
			}
		}

		//usleep(50 * 1000);

		if (( ret = ioctl(dev->fd, VIDIOC_STREAMON, &type)) < 0) {
 			ucam_error("ioctl VIDIOC_STREAMON: %s (%d).",
 			strerror(errno), errno);
 		}
		
		//usleep(50 * 1000);
		
		for (i = 0; i < dev->nbufs; ++i) {
			memset(&buf, 0, sizeof buf);

			buf.index = i;
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.memory = dev->config.reqbufs_memory_type;
			buf.bytesused = 0;


			if(dev->stream_num && dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon)
			{
				((uint32_t *)dev->mem[buf.index] + dev->reqbufs_config.data_offset)[0] = venc_chn_to_simulcast_uvc_header_id(dev, get_current_simulcast_venc_chn(dev));
			}
			
			//memset(dev->mem[buf.index], 0, 8);
			//ucam_strace("Queueing buffer %u.", i);

			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				buf.m.userptr = (unsigned long)dev->mem[buf.index];
				buf.length = dev->video_reqbufs_length[buf.index];
				uvc_video_reqbufs_flush_cache(dev, buf.index);
			}

			if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &buf)) < 0) {
				ucam_error("Unable to queue buffer: %s (%d).",
						strerror(errno), errno);
				ucam_trace("VIDIOC_QBUF error!, ret = 0x%x\n", ret);
				//exit(-1);
				
				break;
			}
		}
		
		//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			
		
		if(dev->stream_num && dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon && dev->fcc == V4L2_PIX_FMT_H264)
		{
			
			if(get_simulcast_start_or_stop_layer(dev, dev->config.simulcast_stream_0_venc_chn) == 0)
			{
				ucam_notice("simulcast stream %d is stop\n",dev->config.simulcast_stream_0_venc_chn);				
				dev->uvc_stream_on = 1;
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return ret;
			}	
		}


		if(uvc_api_set_format(dev, dev->config.venc_chn, dev->fcc) != 0)
		{
			ucam_error("uvc_api_set_format fail");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			return -1;
		}

		if(dev->fcc == V4L2_PIX_FMT_H264 || dev->fcc == V4L2_PIX_FMT_H265)
		{
			dev->first_IDR_flag = 1;
		}
		dev->PayloadTransferSize = dev->commit.dwMaxPayloadTransferSize;
		dev->uvc_stream_on = 1;
		uvc_api_set_stream_status (dev, dev->config.venc_chn, 1);
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	}

	return ret;
}

static int uvc_video_init( struct uvc_dev *dev )
{
	int ret;
	
	//创建处理uvc控制的线程
	ret=pthread_create(&dev->uvc_ctrl_handle_thread_pid, NULL, uvc_ctrl_handle_thread,(void *)dev);
	if(ret != 0){
		ucam_strace("Create uvc_ctrl_handle_thread erro!\n");
		uvc_close(dev);
		return ret;
	}
	ucam_strace("Create uvc_ctrl_handle_thread ok!\n");

	return 0;
}

/* -----------------------------------------------------------------------
 * Request processing
 */

static UvcRequestErrorCode_t uvc_events_process_standard ( struct uvc_dev *dev, 
	struct uvc_event *uvc_event,
	struct usb_ctrlrequest *ctrl, struct uvc_request_data *resp)
{
	//struct v4l2_event v4l2_event;
	//struct uvc_event *uvc_event = (void *)&v4l2_event->u.data;
	UvcRequestErrorCode_t ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
	//int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;	
			
	switch (ctrl->bRequestType) {
		case USB_TARGET_INTF:
			ucam_strace("USB_TARGET_INTF\n");
			if(ctrl->bRequest == USB_SC_SET_INTERFACE)
			{				
				if(ctrl->wIndex == dev->streaming_intf)
				{
					if(ctrl->wValue == 0)
					{
						if(dev->uvc_stream_on)
						{										
							ret = uvc_events_process_streamoff(dev, &uvc_event->data);
							//usleep( 50*1000 );
						}
						else	
						{
							//uvc_video_stream(dev, 0);
							
						}
						//ucam_trace("USB_TARGET_INTF:OFF\n");
					}
					else
					{
						ret = uvc_events_process_streamon(dev, &uvc_event->data);	
						//ucam_trace("USB_TARGET_INTF:ON\n");						
					}					
				}
				else if(ctrl->wIndex == 0 && ctrl->wValue == 0)
				{
					//USB重新连接	
					if(dev->uvc_stream_on)
					{										
						ret = uvc_events_process_streamoff(dev, &uvc_event->data);
					}
					else
					{
						//video_ep_reset(dev);						
						
					}
					dev->uvc->usb_speed = uvc_event->speed;
					//ucam_trace("SET CONFIG\n");
				}				
			}
			resp->length = 0;
			break;
			
		case USB_TARGET_ENDPT:
#if 1
			if(ctrl->bRequest == USB_SC_CLEAR_FEATURE && dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
			{
				ucam_strace("CLEAR_FEATURE\n");
				//ucam_trace("CLEAR_FEATURE\n");
				if (ctrl->wIndex == dev->streaming_ep_address)
				{
					if(dev->uvc_stream_on)
					{										
						uvc_events_process_streamoff(dev, &uvc_event->data);
						//ret = uvc_events_process_streamoff2(dev, &uvc_event->data);
						//usleep( 50*1000 );
					}
					else	
					{
						//uvc_video_stream(dev, 0);
					}

					//ucam_trace("CLEAR_FEATURE:OFF\n");					
				}					
			}
#endif
			resp->length = 0;
			break;
		default:
			//ucam_trace("ctrl->bRequestType = 0x%x\n",ctrl->bRequestType);	
			return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}
	
	return ret;
}

UvcRequestErrorCode_t uvc_events_process_class (struct uvc_dev *dev, 
    struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	ucam_strace("enter");
	//UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;

	if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
		return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

	
	return uvc_requests_ctrl(dev, ctrl, data, event_data);
}

static int usb_connect_reqbufs_max(struct uvc_dev *dev)
{
	struct v4l2_format fmt;
	int ret = 0;

	if(dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_DYNAMIC && dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
	{
		//ISOC 模式在打开码流时再申请
		return 0;
	}
	
	if(dev->nbufs > 0)
	{
		ucam_strace("uvc buf has already req, nbufs:%d", dev->nbufs);
		return 0;
	}
	
	pthread_mutex_lock(&dev->uvc_reqbufs_mutex);
	memset(&fmt, 0, sizeof fmt);
	
	fmt.fmt.pix.width = 1920;
	fmt.fmt.pix.height = 1080;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	fmt.fmt.pix.sizeimage = dev->config.uvc_max_format_bufsize; 
	dev->sizeimage = fmt.fmt.pix.sizeimage;
	
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(dev->fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0)
	{
		ucam_error("VIDIOC_S_FMT: %s (%d) failed!", strerror(errno), errno);
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}
	ret = uvc_video_reqbufs(dev, dev->config.uvc_max_queue_num);
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	

	return 0;
}

static UvcRequestErrorCode_t uvc_events_process_setup (struct uvc_dev *dev, 
	struct uvc_event *uvc_event,
    struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos;
            
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;

	if(ctrl->bRequestType == 0x00 && ctrl->bRequest == 0x09 && ctrl->wIndex == 0x00)
	{
		ucam_notice( "usb speed: %s , configure: %d", usb_speed_string(dev->uvc->usb_speed), ctrl->wValue);

		if(ctrl->wValue < 1 || ctrl->wValue > 3)
		{
			if(dev->bConfigurationValue != 0)
			{
				ctrl->wValue = dev->bConfigurationValue;
			}
			else
			{
				ctrl->wValue = 1;//使用默认配置
			}
		}
				
		if(ctrl->wValue == 3)//config3 for win7 usb3.0
		{
			ctrl->wValue = 1;
			dev->uvc->win7_os_configuration = 1;
		}
		else  if(dev->bConfigurationValue == 1 && dev->config.usb3_isoc_win7_mode == 1)
		{
			dev->uvc->win7_os_configuration = 1;
		}
		else
		{
			dev->uvc->win7_os_configuration = 0;
		}
		
		if(ctrl->wValue >= 1 /*&& ctrl->wValue <= dev->uvc->config.usb_config_max*/)
		{
			ucam_list_for_each(pos, &dev->uvc->uvc_dev_list)
			{
				uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);

				if(uvc_dev && uvc_dev->bConfigurationValue == ctrl->wValue)
				{
					if(uvc_dev->stream_num < uvc_dev->uvc->config.uvc_video_stream_max)
					{
						uvc_dev->uvc->uvc_dev[uvc_dev->stream_num] = uvc_dev;
					}
				}
			}
			dev->uvc->bConfigurationValue = ctrl->wValue;
            dev->uvc->usb_connet_status = 1;
			
			//
			if(dev->uvc->uac_enable && dev->uvc->ucam != NULL)
			{
				if(dev->uvc->ucam->uac != NULL)
				{
					uac_set_usb_configuration_value (dev->uvc->ucam->uac, dev->uvc->bConfigurationValue);
				}
			}

			usb_connect_reqbufs_max(dev);

            if(dev->uvc->usb_connet_event_callback)
            {
                dev->uvc->usb_connet_event_callback(dev->uvc, 
                    dev->uvc->usb_connet_status, dev->uvc->usb_speed, dev->uvc->bConfigurationValue); 
            }
			return UVC_REQUEST_ERROR_CODE_NO_ERROR;
		}
		else
		{
			return UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE;
		}
	}

	
	switch (ctrl->bRequestType & USB_TYPE_MASK) {
		case USB_TYPE_STANDARD:
			ret = uvc_events_process_standard(dev, uvc_event, ctrl, data);
			break;

		case USB_TYPE_CLASS:
			ret = uvc_events_process_class(dev, ctrl, data, event_data);
			break;

		default:
			ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
			break;
	}	
	
	return ret;
}

static UvcRequestErrorCode_t uvc_events_process_data(struct uvc_dev *dev, 
    struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
    return uvc_requests_ctrl(dev, ctrl, data, event_data);	
}


int32_t uvc_events_process_streamoff(struct uvc_dev *dev, 
	struct uvc_request_data *data)
{
	int32_t ret = 0;
	ucam_strace( "enter" );
	
	ucam_strace("dev->uvc_stream_on: %d\n",dev->uvc_stream_on);
	if(dev->uvc_stream_on)
	{
		uvc_video_stream(dev, 0);
		//uvc_video_reqbufs(dev, 0);
	}

	return ret;
	
}


int32_t  uvc_events_process_streamon(struct uvc_dev *dev, 
	struct uvc_request_data *data)
{
	int32_t ret = 0;

	if(check_usb_id(dev->uvc->config.DeviceVersion.idVendor, dev->uvc->config.DeviceVersion.idProduct) == 0)
	{
		ucam_notice("USB ID error\n");
		//system("reboot");
		return -1;
	}

	//ucam_strace( "enter, alt:%d", alt );
	if(dev->uvc_stream_on){	
		ucam_notice("Open the video repeatedly\n");
		ret = uvc_events_process_streamoff(dev,data); 
	}


	ret = uvc_video_stream(dev, 1);

	return ret;
}

int usb_ep0_set_halt(struct uvc_dev *dev)
{
	int ret;
	struct uvc_request_data resp;
	memset(&resp, 0, sizeof (resp));
	resp.length = -1;
	
	ret = ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
	if (ret < 0) {
		ucam_trace("UVCIOC_S_EVENT failed: %s (%d)", strerror(errno), errno);
		return ret;
	}

	return ret;	
}


int uvc_send_video_control_status(struct uvc_dev *dev, uint8_t *data, uint32_t len)
{
	int ret;
	struct uvc_request_data request;
	memset(&request, 0, sizeof (request));
	
	request.length = len;
	if(request.length > EP0_DATA_SIZE)
	{
		ucam_trace("length out of range! length = %d", len);
		return -1;
	}
	
	ucam_trace("send video control status, length = %d", len);
	
	memcpy(request.data, data, request.length);

	ret = ioctl(dev->fd, UVCIOC_SEND_STATUS, &request);
	if (ret < 0) {
		ucam_trace("UVCIOC_SEND_STATUS failed: %s (%d)", strerror(errno), errno);
		usleep(10000);
		ret = ioctl(dev->fd, UVCIOC_SEND_STATUS, &request);
	}	

	return ret;
}

int uvc_send_video_control_status_failure(struct uvc_dev *dev, struct usb_ctrlrequest *ctrl)
{
	struct status_packet_format_video_control_s control_status;
	uint32_t *value;
	uint32_t len;
	
	ucam_trace("enter");
	
	memset(&control_status, 0, sizeof (control_status));
	control_status.bStatusType = UVC_STATUS_TYPE_CONTROL;
	control_status.bOriginator = ctrl->wIndex >> 8;//ID of the Terminal, Unit or Interface that reports the interrupt 
	control_status.bEvent = 0;//Control Change 
	control_status.bSelector = ctrl->wValue >> 8;
	control_status.bAttribute = UVC_STATUS_ATTRIBUTE_CONTROL_FAILURE_CHANGE;
	value = (uint32_t *)control_status.bValue;
	*value = dev->vc_request_error_code;
	len = 1;
	
	return uvc_send_video_control_status(dev, (uint8_t *)&control_status, len + 5);
}


void uvc_video_control_status_handle_thread(struct uvc_dev *dev)
{
#if 1
	struct status_packet_format_video_control_s control_status;
	uint32_t *value;
	uint32_t len;
	
	memset(&control_status, 0, sizeof (control_status));
	control_status.bStatusType = UVC_STATUS_TYPE_CONTROL;
	control_status.bOriginator = UVC_ENTITY_ID_INPUT_TERMINAL;//ID of the Terminal, Unit or Interface that reports the interrupt 
	control_status.bEvent = 0;//Control Change 
	control_status.bSelector = UVC_CT_ZOOM_ABSOLUTE_CONTROL;
	control_status.bAttribute = UVC_STATUS_ATTRIBUTE_CONTROL_VALUE_CHANGE;
	value = (uint32_t *)control_status.bValue;
	*value = 0x666;
	len = 2;
	
	uvc_send_video_control_status(dev, (uint8_t *)&control_status, len + 5);
#endif
}

int usb_ep0_set_zlp(struct uvc_dev *dev)
{
	int ret;
	struct uvc_request_data resp;
	memset(&resp, 0, sizeof resp);
	resp.length = 0;
	
	ret = ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
	if (ret < 0) {
		ucam_trace("UVCIOC_S_EVENT failed: %s (%d)", strerror(errno), errno);
		return ret;	
	}

	return ret;	
}

//在断开或者连接USB的时候关闭未关闭的流
int32_t uvc_streamoff_all(struct uvc_dev *dev)
{
	int32_t ret = -1;

	struct uvc_dev *uvc_dev;
	struct ucam_list_head *pos;

	if(dev == NULL)
		return -ENOMEM;

	ucam_list_for_each(pos, &dev->uvc->uvc_dev_list)
	{
		uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
		if(uvc_dev)
		{
			if(uvc_dev->uvc_stream_on)
				ret = uvc_events_process_streamoff(uvc_dev, NULL);
		}
	}

	return ret;	
}

int32_t uvc_free_all_reqbufs(struct uvc_dev *dev)
{
	int32_t ret = -1;

	struct uvc_dev *uvc_dev;
	struct ucam_list_head *pos;

	if(dev == NULL)
		return -ENOMEM;

	ucam_list_for_each(pos, &dev->uvc->uvc_dev_list)
	{
		uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
		if(uvc_dev)
		{
			if(uvc_dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_DYNAMIC)
			{
				if(uvc_dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
				{
					if(uvc_dev->nbufs > 0)
					{
						pthread_mutex_lock(&uvc_dev->uvc_reqbufs_mutex);
						uvc_video_reqbufs(uvc_dev, 0);
						pthread_mutex_unlock(&uvc_dev->uvc_reqbufs_mutex);
					}
				}
			}
			else
			{
				if(uvc_dev->nbufs > 0)
				{
					pthread_mutex_lock(&uvc_dev->uvc_reqbufs_mutex);
					uvc_video_reqbufs(uvc_dev, 0);
					pthread_mutex_unlock(&uvc_dev->uvc_reqbufs_mutex);
				}		
			}
		}
	}

	return ret;
}

static int32_t uvc_events_process(struct uvc_dev *dev)
{
	//struct v4l2_event v4l2_event;
	struct uvc_event *uvc_event = (void *)&dev->v4l2_event.u.data;
	struct uvc_request_data resp;
	int32_t ret = 0;
	uint32_t uvc_stream_on;
	struct usb_ctrlrequest *ctrl = &uvc_event->req;

	ret = ioctl(dev->fd, VIDIOC_DQEVENT, &dev->v4l2_event);
	if (ret < 0) {
		ucam_error("VIDIOC_DQEVENT failed: %s (%d)", strerror(errno),
				errno);
		return ret;
	}

	memset(&resp, 0, sizeof resp);
	resp.length = -EL2HLT;
	//ucam_traceout("\n\n****************UVC_COMMAND*******************\n");

	//ucam_strace( "v4l2_event type:%#x", v4l2_event.type );
	ucam_debug( "event :%s, stream num:%d", v4l2_event_to_string(dev->v4l2_event.type), dev->stream_num);
	//ucam_notice("dev:%p", dev);
	switch (dev->v4l2_event.type) {
		case UVC_EVENT_CONNECT:
			ret = 0;
			ucam_notice( "UVC_EVENT_CONNECT, usb speed:%s", usb_speed_string(uvc_event->speed));
			
			uvc_streamoff_all(dev);

            dev->uvc->usb_connet_status = 1;
			dev->uvc->usb_speed = uvc_event->speed;
			
			if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
			{
				dev->MaxPayloadTransferSize = dev->MaxPayloadTransferSize_bulk;
			}
			else
			{
				if(dev->uvc->usb_speed >= UCAM_USB_SPEED_SUPER){
					dev->MaxPayloadTransferSize = dev->config.MaxPayloadTransferSize_usb3;		
				}else{
					dev->MaxPayloadTransferSize = dev->config.MaxPayloadTransferSize_usb2;
				}
			}
			break;
			
		case UVC_EVENT_DISCONNECT:
			ucam_notice("UVC_EVENT_DISCONNECT\n");
			
			dev->uvc->usb_connet_status = 0;
			
			uvc_streamoff_all(dev);
			if(dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_DYNAMIC || dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_DYNAMIC_USB_CONNECT)
			{
				uvc_free_all_reqbufs(dev);
			}
            
			dev->vc_request_error_code  = UVC_REQUEST_ERROR_CODE_NO_ERROR;
            if(dev->uvc->usb_connet_event_callback)
            {
                dev->uvc->usb_connet_event_callback(dev->uvc, 
                    dev->uvc->usb_connet_status, dev->uvc->usb_speed, dev->uvc->bConfigurationValue); 
            }
			break;
		
		case UVC_EVENT_SUSPEND:
			ucam_notice("UVC_EVENT_SUSPEND\n");
			
			dev->uvc->usb_suspend = 1;

			if(dev->uvc_stream_on)
			{
				uvc_events_process_streamoff(dev, &uvc_event->data);
				if(dev->uvc->uvc_stream_event_callback)
				{
					dev->uvc->uvc_stream_event_callback(dev->uvc, dev->stream_num, 0); 
				}
			}
			
            if(dev->uvc->usb_suspend_event_callback)
            {
                dev->uvc->usb_suspend_event_callback(dev->uvc); 
            }
			break;
			
		case UVC_EVENT_RESUME:
			ucam_notice("UVC_EVENT_RESUME\n");
			
			dev->uvc->usb_suspend = 0;
            if(dev->uvc->usb_resume_event_callback)
            {
                dev->uvc->usb_resume_event_callback(dev->uvc); 
            }
			break;
			
		case UVC_EVENT_SETUP:
			ucam_debug("usb_ctrl_req: bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
				"wLength %04x", 
				uvc_event->req.bRequestType, uvc_event->req.bRequest,
				uvc_event->req.wValue, uvc_event->req.wIndex, uvc_event->req.wLength);
			
			dev->uvc->usb_suspend = 0;
			if(dev->uvc->usb_connet_status == 0)
			{
				dev->uvc->usb_connet_status = 1;
			}
			dev->vc_request_error_code = uvc_events_process_setup(dev, uvc_event, &uvc_event->req, &resp, 0);
			if(dev->vc_request_error_code != UVC_REQUEST_ERROR_CODE_NO_ERROR)
			{
				ret = -1;
				ucam_error( "uvc_request_error: %s", uvc_request_error_code_string(dev->vc_request_error_code));
				ucam_error("usb_ctrl_req: bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
				"wLength %04x", 
				uvc_event->req.bRequestType, uvc_event->req.bRequest,
				uvc_event->req.wValue, uvc_event->req.wIndex, uvc_event->req.wLength);
				//if(uvc_event->req.wLength)
				{
					//uvc_send_video_control_status_failure(dev, ctrl);
					usb_ep0_set_halt(dev);
					ucam_error("uvc ctrl process error EP0 SET STALL !!!");
				}

				goto DONE;				
			}
			if(dev->uvc->uvc_requests_ctrl_event_callback)
            {
                dev->uvc->uvc_requests_ctrl_event_callback(dev, &uvc_event->req, &resp, 0);
            }
			break;

		case UVC_EVENT_DATA:
			ucam_debug("usb_ctrl_req: bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
				"wLength %04x", 
				uvc_event->auto_queue_data.usb_ctrl.bRequestType, 
				uvc_event->auto_queue_data.usb_ctrl.bRequest,
				uvc_event->auto_queue_data.usb_ctrl.wValue, 
				uvc_event->auto_queue_data.usb_ctrl.wIndex, 
				uvc_event->auto_queue_data.usb_ctrl.wLength);


			if(uvc_event->data.length <= 4)
			{			
				ucam_debug( "receive length: %d, data: 0x%08x(%d)", 
					uvc_event->data.length , ((uint32_t*)&uvc_event->data.data)[0], ((uint32_t*)&uvc_event->data.data)[0]);
			}
			else if(uvc_event->data.length <= EP0_DATA_SIZE)
			{
				char string[256] = {0};
				int i;
				for(i = 0; i < uvc_event->data.length; i++)
				{
					char temp[4] = {0};
					sprintf(temp, "%02x ", uvc_event->data.data[i]);
					strcat(string, temp);
				}
				//接收
				ucam_debug( "receive length: %d, data: %s", 
					uvc_event->data.length, string);
				
			}
			
			//ucam_trace( "got UVC_EVENT_DATA. length:%d\n", uvc_event->data.length );
			if(uvc_event->data.length >= 0){

				dev->uvc_set_ctrl = uvc_event->auto_queue_data.usb_ctrl;
				dev->vc_request_error_code = uvc_events_process_data(dev, &dev->uvc_set_ctrl, &uvc_event->data, 1);
				if(dev->uvc->uvc_requests_ctrl_event_callback)
                {
                    dev->uvc->uvc_requests_ctrl_event_callback(dev, &dev->uvc_set_ctrl, &uvc_event->data, 1);
                }
			}
			else{
				//ucam_trace( "got UVC_EVENT_DATA. length:%d\n", uvc_event->data.length );
				dev->vc_request_error_code = UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
			}
			
			if(dev->vc_request_error_code != UVC_REQUEST_ERROR_CODE_NO_ERROR)
			{
				ret = -1;
				ucam_error( "uvc_request_error: %s", uvc_request_error_code_string(dev->vc_request_error_code));
				ucam_error("usb_ctrl_req: bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
				"wLength %04x", 
				uvc_event->auto_queue_data.usb_ctrl.bRequestType, 
				uvc_event->auto_queue_data.usb_ctrl.bRequest,
				uvc_event->auto_queue_data.usb_ctrl.wValue, 
				uvc_event->auto_queue_data.usb_ctrl.wIndex, 
				uvc_event->auto_queue_data.usb_ctrl.wLength);
				//uvc_send_video_control_status_failure(dev, &dev->uvc_set_ctrl);
			}

			//set cur 返回 UVC_REQUEST_ERROR_CODE mac os 不兼容
			return 0;
#if 0			
			if(dev->vc_request_error_code == UVC_REQUEST_ERROR_CODE_OUT_OF_RANGE)
			{
				return 0;
			}
			
			if(ret < 0)
			{
				//set cur 返回 UVC_REQUEST_ERROR_CODE mac os 不兼容
				usb_ep0_set_halt(dev);
				ucam_error("uvc ctrl process error EP0 SET STALL !!!");
			}			
			return ret;
#endif
		case UVC_EVENT_STREAMON:
			ucam_notice( "UVC_EVENT_STREAMON, stream num:%d", dev->stream_num);
			dev->uvc->usb_suspend = 0;
			uvc_stream_on = dev->uvc_stream_on;
			if(dev->uvc->usb_connet_status == 0)
			{
				dev->uvc->usb_connet_status = 1;
			}
			ret = uvc_events_process_streamon(dev, &uvc_event->data);
			if(ret == 0)
                dev->vc_request_error_code = UVC_REQUEST_ERROR_CODE_NO_ERROR;	
			else
				dev->vc_request_error_code = UVC_REQUEST_ERROR_CODE_WRONG_STATE;

			if(dev->uvc->uvc_stream_event_callback && uvc_stream_on == 0)
			{
				return dev->uvc->uvc_stream_event_callback(dev->uvc, dev->stream_num, 1);  
			}
			break;

		case UVC_EVENT_STREAMOFF:
			ucam_notice( "UVC_EVENT_STREAMOFF, stream num:%d", dev->stream_num);
			uvc_stream_on = dev->uvc_stream_on;
			ret = uvc_events_process_streamoff(dev, &uvc_event->data);
			if(ret == 0)
				dev->vc_request_error_code = UVC_REQUEST_ERROR_CODE_NO_ERROR;
			else
				dev->vc_request_error_code = UVC_REQUEST_ERROR_CODE_WRONG_STATE;

			if(dev->uvc->uvc_stream_event_callback && uvc_stream_on == 1)
            {
                dev->uvc->uvc_stream_event_callback(dev->uvc, dev->stream_num, 0); 
            }
			break;

		default:
			ucam_error( "invalid event, length:%d", uvc_event->data.length );
			return -1;
			break;
	}

	if ( resp.length > 0 && (ctrl->bRequestType & USB_DIR_IN)){
		
		if(resp.length <= 4)
		{
			ucam_debug("response length: %d, data: 0x%08x(%d)", 
				resp.length, ((uint32_t*)&resp.data)[0], ((uint32_t*)&resp.data)[0]);
		}
		else if(resp.length <= EP0_DATA_SIZE)
		{
			char string[256] = {0};
			int i;
			for(i = 0; i < resp.length; i++)
			{
				char temp[4] = {0};
				sprintf(temp, "%02x ", resp.data[i]);
				strcat(string, temp);
			}
			
			ucam_debug("response length: %d, data: %s", 
				resp.length, string);
		}
		

		ioctl(dev->fd, UVCIOC_SEND_RESPONSE, &resp);
		if (ret < 0) {
			ucam_error("UVCIOC_S_EVENT failed: %s (%d)", strerror(errno), errno);
			return ret;
		}
	}
	else{
		ucam_trace("no response");
	}

DONE:	
	return ret;
}

static void uvc_events_init( struct uvc_dev *dev )
{
	int i;
	struct v4l2_event_subscription sub;

	uvc_fill_streaming_ctrl(dev, &dev->probe, 0, 0, 0);
	uvc_fill_streaming_ctrl(dev, &dev->commit, 0, 0, 0);
	
	if (dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK) {
		/* FIXME Crude hack, must be negotiated with the driver. */
		dev->probe.dwMaxPayloadTransferSize = dev->MaxPayloadTransferSize_bulk;
		dev->commit.dwMaxPayloadTransferSize = dev->MaxPayloadTransferSize_bulk;
	}

	memset(&sub, 0, sizeof sub);

	for ( i = UVC_EVENT_FIRST; i <= UVC_EVENT_LAST; i++ ){
		sub.type = i;
		ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	}
}

static long long uvc_timestamp_diff_ns(struct timespec tstamp)
{
    struct timespec now, diff;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (tstamp.tv_nsec > now.tv_nsec)
    {
        diff.tv_sec = now.tv_sec - tstamp.tv_sec - 1;
        diff.tv_nsec = (now.tv_nsec + 1000000000L) - tstamp.tv_nsec;
    }
    else
    {
        diff.tv_sec = now.tv_sec - tstamp.tv_sec;
        diff.tv_nsec = now.tv_nsec - tstamp.tv_nsec;
    }
    return (diff.tv_sec * 1000000000) + diff.tv_nsec;
}

void *uvc_ctrl_handle_thread(void *arg)
{
	fd_set fds;
	int ret;
	struct timeval tv;
	struct v4l2_capability cap;
	struct uvc_dev *dev = (struct uvc_dev *)arg;
	int usb_link_state = 0;
	int usb_link_state_cnt = 0;
    long long timestamp_diff;

	ucam_strace("enter");
	
	FD_ZERO(&fds);
	FD_SET(dev->fd, &fds);
	
	prctl(PR_SET_NAME, "uvc_ctrl_handle_thread");
	
    tv.tv_sec=0;
    tv.tv_usec=50;
	//usleep(50 * 1000);
	ret = ioctl(dev->fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		ucam_error("unable to query device: %s (%d)", strerror(errno),
				errno);
		close(dev->fd);
		return NULL;
	}
	
    dev->uvc_ctrl_handle_thread_exit = 0;
	while (!dev->uvc_ctrl_handle_thread_exit) {
		fd_set efds = fds;

		tv.tv_sec=0;
		tv.tv_usec=10000;
		ret = select(dev->fd + 1, NULL, NULL, &efds, &tv);//必须得用超时，否则会由于不同步导致select不到命令问题
		if ( ret == - 1 ){
			ucam_error("select fail!");
			continue;
		}
		else if( ret == 0 )
		{
			//ucam_trace("select timeout!");	
#if 1
			//有时候连接USB后没有打开码流也会进入待机状态，所以只能判断打开码流的时候
			//不然重新打开码流后usb_connet_status状态会不对
			if(dev->uvc->usb_connet_status && dev->uvc_stream_on && dev->uvc->usb_suspend == 0)
			{	
				pthread_mutex_lock(&dev->uvc->uvc_event_mutex);
				ret = uvc_ioctl_get_usb_link_state(dev , &usb_link_state);
				if(ret == 0 && usb_link_state != 0)
				{
					usb_link_state_cnt ++;
					if(usb_link_state_cnt > 50)//500ms
					{
						ucam_error("usb_link_state:%d\n",usb_link_state);
						usb_link_state_cnt = 0;
						dev->uvc->usb_connet_status = 0;
						uvc_streamoff_all(dev);
						if(dev->uvc->usb_connet_event_callback)
						{
							dev->uvc->usb_connet_event_callback(dev->uvc, 
								dev->uvc->usb_connet_status, dev->uvc->usb_speed, dev->uvc->bConfigurationValue); 
						}
					}					
				}
				else
				{
					usb_link_state_cnt = 0;  
				}
				pthread_mutex_unlock(&dev->uvc->uvc_event_mutex);
			}
#endif
            pthread_mutex_lock(&dev->uvc->uvc_event_mutex);
            if (dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK &&
                dev->uvc_stream_on_trigger_flag)
            {
                
                timestamp_diff = uvc_timestamp_diff_ns(dev->stream_on_trigger_time);
                if(timestamp_diff > 100000000 || timestamp_diff < -100000000)
                {
                    stream_on_trigger_process(dev);
                }
            }
            pthread_mutex_unlock(&dev->uvc->uvc_event_mutex);
			continue;
		}

		if (FD_ISSET(dev->fd, &efds))
		{
			pthread_mutex_lock(&dev->uvc->uvc_event_mutex);
			uvc_events_process(dev);
			pthread_mutex_unlock(&dev->uvc->uvc_event_mutex);
		}
	}
    ucam_trace("leave");

    dev->uvc_ctrl_handle_thread_exit = 1;
    ucam_debug("thread exit");
    ret = 0;
    pthread_exit(&ret); 
	return NULL;
}


static int init_entity_ctrls(struct uvc_dev *dev)
{
    return uvc_ctrl_init(dev, dev->uvc->uvc_ctrl_attrs);
}

int uvc_set_dev_config_default(struct uvc_dev *dev)
{
	ucam_trace("uvc_set_dev_config_default");

	if(dev == NULL)
	{
		return -ENOMEM;
	}

	dev->bConfigurationValue = 0;
	dev->config.uvc_ep_mode = UVC_USB_TRANSFER_MODE_ISOC;
	dev->config.MaxPayloadTransferSize_usb2 = USB2_EP_ISOC_PACKET_SIZE_MAX;
	dev->config.MaxPayloadTransferSize_usb3 = USB3_EP_ISOC_PACKET_SIZE_ALT23;
	dev->config.MaxPayloadTransferSize_usb3_win7 = USB3_EP_ISOC_PACKET_SIZE_ALT21;
	dev->config.usb3_isoc_speed_fast_mode = 1;
	dev->config.usb3_isoc_speed_multiple = 1.5;
	dev->config.win7_usb3_isoc_speed_multiple = 1.2;
	dev->config.usb3_isoc_win7_mode = 0;
    dev->config.uvc_protocol = UVC_PROTOCOL_V110;
    dev->config.uvc_max_queue_num = 1;
	dev->reqbufs_config.data_offset = 0;

	dev->config.resolution_width_min = 160;
	dev->config.resolution_height_min = 120;
	dev->config.resolution_width_max = 1920;
	dev->config.resolution_height_max = 1080;

	dev->config.yuv_resolution_width_min = 0;
	dev->config.yuv_resolution_height_min = 0;
	dev->config.yuv_resolution_width_max = 0;
	dev->config.yuv_resolution_height_max = 0;


	dev->config.fps_max = 30;
	dev->config.yuv_fps_max = 0;

	dev->config.fps_min = 5;
	dev->config.yuv_fps_min = 0;


	dev->config.reqbufs_memory_type = V4L2_REQBUFS_MEMORY_TYPE_DEF;
	dev->config.uvc_max_format_bufsize = 4*1024*1024;
   	dev->config.venc_chn = 0;
	dev->config.windows_hello_enable = 0;

	//星网锐捷 ID 3已经使用为XU 控制UNIT ID
	dev->config.processing_unit_id = UVC_ENTITY_ID_PROCESSING_UNIT_DEF;

    dev->config.camera_terminal_bmControls.b.scanning_mode = UVC_CT_SCANNING_MODE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.ae_mode = UVC_CT_AE_MODE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.ae_priority = UVC_CT_AE_PRIORITY_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.exposure_time_absolute = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.exposure_time_relative = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.focus_absolute = UVC_CT_FOCUS_ABSOLUTE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.focus_relative = UVC_CT_FOCUS_RELATIVE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.iris_absolute = UVC_CT_IRIS_ABSOLUTE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.iris_relative = UVC_CT_IRIS_RELATIVE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.zoom_absolute = UVC_CT_ZOOM_ABSOLUTE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.zoom_relative = UVC_CT_ZOOM_RELATIVE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.pantilt_absolute = UVC_CT_PANTILT_ABSOLUTE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.pantilt_relative = UVC_CT_PANTILT_RELATIVE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.roll_absolute = UVC_CT_ROLL_ABSOLUTE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.roll_relative = UVC_CT_ROLL_RELATIVE_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.reserved1 = 0;//set to zero 
	dev->config.camera_terminal_bmControls.b.reserved2 = 0;//set to zero 
	dev->config.camera_terminal_bmControls.b.focus_auto = UVC_CT_FOCUS_AUTO_CONTROL_SUPPORTED;
	dev->config.camera_terminal_bmControls.b.privacy = UVC_CT_PRIVACY_CONTROL_SUPPORTED;	
	dev->config.camera_terminal_bmControls.b.focus_simple = 0;
	dev->config.camera_terminal_bmControls.b.window = 0;	
	dev->config.camera_terminal_bmControls.b.region_of_interest = 0;		
	dev->config.camera_terminal_bmControls.b.reserved3 = 0;//set to zero

    dev->config.processing_unit_bmControls.b.brightness = UVC_PU_BRIGHTNESS_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.contrast = UVC_PU_CONTRAST_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.hue = UVC_PU_HUE_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.saturation = UVC_PU_SATURATION_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.sharpness = UVC_PU_SHARPNESS_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.gamma = UVC_PU_GAMMA_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.white_balance_temperature = UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.white_balance_component = UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.backlight_compensation = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.gain = UVC_PU_GAIN_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.power_line_frequency = UVC_PU_POWER_LINE_FREQUENCY_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.hue_auto = UVC_PU_HUE_AUTO_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.white_balance_temperature_auto = UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.white_balance_component_auto = UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL_SUPPORTED;
	dev->config.processing_unit_bmControls.b.digital_multiplier = 0;
	dev->config.processing_unit_bmControls.b.digital_multiplier_limit = 0;

#if !(defined SIGMASTAR_SDK) || (defined CHIP_SSC333) || (defined CHIP_SSC9211)	
	dev->config.uvc_frame_buf_mode = UVC_FRAME_BUF_MODE_DYNAMIC;
#elif (defined CHIP_CV5)
	dev->config.uvc_frame_buf_mode = UVC_FRAME_BUF_MODE_STATIC;	
#else
	dev->config.uvc_frame_buf_mode = UVC_FRAME_BUF_MODE_DYNAMIC_USB_CONNECT;	
#endif

    dev->config.mjpeg_compression_ratio = 4;
    dev->config.h264_compression_ratio = 12;

    return 0;
}

int uvc_get_dev_config(struct uvc_dev *dev, struct uvc_dev_config *dev_config)
{
	if(dev == NULL || dev_config == NULL)
	{
		return -ENOMEM;
	}

	memcpy(dev_config, &dev->config, sizeof(struct uvc_dev_config));

	return 0;
}

int uvc_set_dev_config(struct uvc_dev *dev, struct uvc_dev_config *dev_config)
{
	if(dev == NULL || dev_config == NULL)
	{
		return -ENOMEM;
	}

	memcpy(&dev->config, dev_config, sizeof(struct uvc_dev_config));

	return 0;
}

#if 0
void handleSig(int signo,struct uvc_dev *dev)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
		ucam_trace("================SIGINT or SIGTERM==============\n");
		uvc_close(dev);
		//exit(0);
    }	
}
#endif

struct uvc_dev *uvc_add_dev(struct uvc *uvc, uint8_t video_dev_index, uint32_t stream_num)
{
	struct uvc_dev *dev = NULL;
	char device[16];
	struct uvc_ioctl_uvc_config_t uvc_config;
	struct ucam_list_head *pos;

    if(uvc == NULL || stream_num >= uvc->config.uvc_video_stream_max)
    {
        ucam_error("config out of range");
        return NULL;
    }

	ucam_list_for_each(pos, &uvc->uvc_dev_list)
	{
		dev = ucam_list_entry(pos, struct uvc_dev, list);

        //判断是否已经注册过了
        if(dev && dev->video_dev_index == video_dev_index)
        {
            return NULL;
        }
    }

	sprintf(device, "/dev/video%d", video_dev_index);

	dev = malloc_dev();
	ucam_notice("dev:%p, %s", dev, device);
	if (NULL == dev)
	{
		ucam_error("malloc dev fail");
		return NULL;
	}

    dev->uvc_ctrl_handle_thread_pid = 0;
    dev->fd = -1;

	dev->video_dev_index = video_dev_index;
	dev->stream_num = stream_num;
	dev->uvc = uvc;	
	
	dev->fd = uvc_open(device);
	if (dev->fd == -1)
	{
		ucam_error("uvc open fail");
		goto error;
	}
	uvc_set_dev_config_default(dev);


	if(uvc_ioctl_get_uvc_config(dev, &uvc_config) == 0)
    {
		if(uvc->uvc_dev[0] == NULL)
		{
			dev->uvc->dfu_enable = uvc_config.dfu_enable;
			dev->uvc->uac_enable = uvc_config.uac_enable;
			dev->uvc->uvc_s2_enable = uvc_config.uvc_s2_enable;
			dev->uvc->uvc_v150_enable = uvc_config.uvc_v150_enable;
			dev->uvc->win7_usb3_enable = uvc_config.win7_usb3_enable;
			dev->uvc->zte_hid_enable = uvc_config.zte_hid_enable;

			ucam_notice("dfu_enable: %d", dev->uvc->dfu_enable);
			ucam_notice("uac_enable: %d", dev->uvc->uac_enable);
			ucam_notice("uvc_s2_enable: %d", dev->uvc->uvc_s2_enable);
			ucam_notice("uvc_v150_enable: %d", dev->uvc->uvc_v150_enable);
			ucam_notice("win7_usb3_enable: %d", dev->uvc->win7_usb3_enable);
			ucam_notice("zte_hid_enable: %d\n", dev->uvc->zte_hid_enable);	
		}

		if(!(dev->uvc->uvc_s2_enable & 0x7F))
		{
			dev->uvc->uvc_s2_enable = 0;
		}
		dev->bConfigurationValue = uvc_config.bConfigurationValue;
		dev->control_intf = uvc_config.control_intf;
		dev->streaming_intf = uvc_config.streaming_intf;
		dev->status_interrupt_ep_address = uvc_config.status_interrupt_ep_address;
		dev->streaming_ep_address = uvc_config.streaming_ep_address;
		ucam_notice("bulk_streaming_ep: %d", uvc_config.bulk_streaming_ep);

		//UVC1.5和第二路不支持BULK模式
		if(uvc_config.bulk_streaming_ep && dev->stream_num == 0 && dev->bConfigurationValue != 2)
		{
			dev->config.uvc_ep_mode = UVC_USB_TRANSFER_MODE_BULK; 
			dev->MaxPayloadTransferSize_bulk = uvc_config.bulk_max_size;
			dev->MaxPayloadTransferSize = dev->MaxPayloadTransferSize_bulk;
			//dev->config.MaxPayloadTransferSize_usb2 = dev->MaxPayloadTransferSize;
			//dev->config.MaxPayloadTransferSize_usb3 = dev->MaxPayloadTransferSize;//32768;
			ucam_notice("uvc BULK EP mode, max_packet_size : %d", dev->MaxPayloadTransferSize);
		}
		else
		{
			dev->config.uvc_ep_mode = UVC_USB_TRANSFER_MODE_ISOC;
			//dev->config.MaxPayloadTransferSize_usb2 = 3*1024;
			//dev->config.MaxPayloadTransferSize_usb3 = 48*1024;
			dev->MaxPayloadTransferSize = dev->config.MaxPayloadTransferSize_usb3;
			ucam_notice("uvc ISOC EP mode");
		}
		
		ucam_notice("ConfigurationValue: %d", dev->bConfigurationValue);
		ucam_notice("video_index: %d", dev->video_dev_index);
		ucam_notice("control_intf: %d", dev->control_intf);
		ucam_notice("streaming_intf: %d", dev->streaming_intf);
		ucam_notice("status_interrupt_ep_address: 0x%x", dev->status_interrupt_ep_address);
		ucam_notice("streaming_ep_address: 0x%x", dev->streaming_ep_address);
		
                
    }
	else
	{	
		ucam_error("uvc_ioctl_get_uvc_config fail");
	}
	
	if(dev->bConfigurationValue > USB_CONFIG_MAX)
	{
		ucam_error("bConfigurationValue error, max=%d", USB_CONFIG_MAX);
		goto error;
	}

	if(uvc->uvc_dev[stream_num] == NULL || dev->bConfigurationValue == 1)
    {
       uvc->uvc_dev[stream_num] = dev;
    } 
    ucam_list_add_tail(&dev->list, &uvc->uvc_dev_list);

	return dev;

error:

    if(dev)
    {
        if (dev->fd != -1)
            uvc_close(dev);

        if(dev)
            SAFE_FREE(dev);
    }

    return NULL;
}

int uvc_dev_init(struct uvc_dev *dev)
{

	ucam_strace("uvc dev(%p) video_dev_index: %d init ...",dev, dev->video_dev_index);

    if(dev == NULL)
    {
        ucam_error("dev == NULL");
        return -1;
    }

	if(dev->config.fps_max > 60)
	{
		ucam_error("fps_max <= 60");
		return -1;
	}
	
	if(dev->config.fps_min < 5)
	{
		ucam_error("fps_min >= 5");
		return -1;
	}

	if(dev->config.yuv_fps_max <= 0)
		dev->config.yuv_fps_max = dev->config.fps_max;

	if(dev->config.yuv_fps_min <= 0)
		dev->config.yuv_fps_min = dev->config.fps_min;

	
	if(dev->config.yuv_fps_min > dev->config.yuv_fps_max)
		dev->config.yuv_fps_min = dev->config.yuv_fps_max;

	if(dev->config.fps_min > dev->config.fps_max)
		dev->config.fps_min = dev->config.fps_max;


	if(dev->config.yuv_resolution_width_min == 0 || dev->config.yuv_resolution_height_min == 0)
	{
		dev->config.yuv_resolution_width_min = dev->config.resolution_width_min;
		dev->config.yuv_resolution_height_min = dev->config.resolution_height_min;
	}
	
	if(dev->config.yuv_resolution_width_max == 0 || dev->config.yuv_resolution_height_max == 0)
	{
		dev->config.yuv_resolution_width_max = dev->config.resolution_width_max;
		dev->config.yuv_resolution_height_max = dev->config.resolution_height_max;
	}
	

	dev->MaxPayloadTransferSize = dev->config.MaxPayloadTransferSize_usb2;
	if(dev->MaxPayloadTransferSize <= 0)
		dev->MaxPayloadTransferSize = 1024;


	ucam_notice("ConfigurationValue: %d, stream_num: %d", dev->bConfigurationValue, dev->stream_num);
	ucam_notice("uvc_protocol = 0x%04x", dev->config.uvc_protocol);
	ucam_notice("uvc_ep_mode = %s", dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK ? "BULK":"ISOC");	
	ucam_notice("MaxPayloadTransferSize_usb2 = %d", dev->config.MaxPayloadTransferSize_usb2);
	ucam_notice("MaxPayloadTransferSize_usb3 = %d", dev->config.MaxPayloadTransferSize_usb3);
	ucam_notice("MaxPayloadTransferSize_usb3_win7 = %d", dev->config.MaxPayloadTransferSize_usb3_win7);
	ucam_notice("usb3_isoc_speed_fast_mode = %d", dev->config.usb3_isoc_speed_fast_mode);
	ucam_notice("usb3_isoc_speed_multiple = %f", dev->config.usb3_isoc_speed_multiple);
	ucam_notice("win7_usb3_isoc_speed_multiple = %f", dev->config.win7_usb3_isoc_speed_multiple);
	ucam_notice("MaxPayloadTransferSize = %d", dev->MaxPayloadTransferSize);
	ucam_notice("max_format_bufsize = %d", dev->config.uvc_max_format_bufsize);
	ucam_notice("video_fifo_num = %d\n", dev->config.uvc_max_queue_num);	

	ucam_notice("venc_chn = %d", dev->config.venc_chn);
	ucam_notice("resolution_width_min = %d", dev->config.resolution_width_min);
	ucam_notice("resolution_height_min = %d", dev->config.resolution_height_min);
	ucam_notice("resolution_width_max = %d", dev->config.resolution_width_max);
	ucam_notice("resolution_height_max = %d", dev->config.resolution_height_max);
	ucam_notice("fps_min = %d, yuv_fps_min:%d", dev->config.fps_min, dev->config.yuv_fps_min);
	ucam_notice("fps_max = %d, yuv_fps_max:%d", dev->config.fps_max, dev->config.yuv_fps_max);
	ucam_notice("uvc_yuy2_enable = %d", dev->config.uvc_yuy2_enable);
	ucam_notice("uvc_nv12_enable = %d", dev->config.uvc_nv12_enable);
	ucam_notice("uvc_mjpeg_enable = %d", dev->config.uvc_mjpeg_enable);
	ucam_notice("uvc110_h264_enable = %d", dev->config.uvc110_h264_enable);
	ucam_notice("uvc110_h265_enable = %d", dev->config.uvc110_h265_enable);
	ucam_notice("uvc150_simulcast_en = %d", dev->config.uvc150_simulcast_en);
	if(dev->config.uvc150_simulcast_en)
	{
		ucam_notice("simulcast_stream_0_venc_chn = %d", dev->config.simulcast_stream_0_venc_chn);
		ucam_notice("simulcast_stream_1_venc_chn = %d", dev->config.simulcast_stream_1_venc_chn);
		ucam_notice("simulcast_stream_2_venc_chn = %d", dev->config.simulcast_stream_2_venc_chn);
	}


	pthread_mutex_init(&dev->uvc_reqbufs_mutex, 0);

	if(init_entity_ctrls(dev) != 0)
    {
        ucam_error("init_entity_ctrls fail");
		goto error;
    }

	uvc_events_init(dev);
	if(uvc_v4l2_init(dev) != 0)
    {
        ucam_error("uvc_v4l2_init fail");
		goto error;  
    }
	
	if(uvc_video_init(dev) != 0)
    {
       	ucam_error("uvc_video_init fail");
		goto error; 
    }

	if(uvc_send_stream_ops_int(dev) != 0)
	{
       	ucam_error("uvc_send_stream_ops_int fail");
		goto error; 
    }

	if(uvc_send_stream_common_ops_int(dev) != 0)
	{
       	ucam_error("uvc_send_stream_common_ops_int fail");
		goto error; 
    }
	
	if(reset_uvc_descriptor(dev, 1, 1) != 0)
	{
		ucam_error("reset_uvc_descriptor fail!");
	}


	if(dev->stream_num == 0)
	{
		if(reset_usb_sn(dev) != 0)
		{
			ucam_error("reset_usb_sn fail!");
		}
	}
	
	
	dev->init_flag = 1;
	
	ucam_notice("uvc dev init ok");
	return 0;

error:

    return -1;
}

int uvc_dev_init_all(struct uvc *uvc)
{
	struct ucam_list_head *posdev, *_posdev;
	struct uvc_dev *dev;

	ucam_notice("ChipId:%#x\n", uvc->config.chip_id);
	ucam_notice("memory page size: %ld\n", sysconf(_SC_PAGE_SIZE));

	uvc->config.DeviceVersion.SocVersion.mode = uvc->config.DeviceVersion.UsbVersion.mode;
	uvc->config.DeviceVersion.SocVersion.bigVersion = uvc->config.DeviceVersion.UsbVersion.bigVersion;
	uvc->config.DeviceVersion.SocVersion.littleVersion = uvc->config.DeviceVersion.UsbVersion.littleVersion;


	if(check_usb_id(uvc->config.DeviceVersion.idVendor, uvc->config.DeviceVersion.idProduct) == 0)
	{
		ucam_notice("USB ID error\n");
		//return -1;
	}
	

	ucam_list_for_each_safe(posdev, _posdev,  &uvc->uvc_dev_list)
	{
		dev = ucam_list_entry(posdev, struct uvc_dev, list);
		if(dev)
		{
			if(uvc_dev_init(dev) != 0)
			{
				ucam_notice("uvc_dev_init fail, video_dev_index:%d\n", dev->video_dev_index);
				return -1;
			}

			if(uvc->win7_usb3_enable)
			{
				if(dev->bConfigurationValue == 3)
				{
					usleep(100000);
					ucam_list_del(&dev->list);
					uvc_close(dev);	
				}			
			}
		}
	}
	uvc->init_flag = 1;
    if(UVC_CMA_Enable_Check(uvc) != 0)
    {
        return -1;
    }
	

	ucam_list_for_each_safe(posdev, _posdev,  &uvc->uvc_dev_list)
	{
		dev = ucam_list_entry(posdev, struct uvc_dev, list);
		if(dev)
		{
			if (dev->config.uvc_frame_buf_mode == UVC_FRAME_BUF_MODE_STATIC &&
				dev->bConfigurationValue != 3)
			{
				if(usb_connect_reqbufs_max(dev) != 0)
				{
					ucam_error("usb_connect_reqbufs_max error");
				}						
			}
		}
	}
		
	ucam_notice("uvc init OK!");
	return 0;
}



int uvc_usb_connet (struct uvc *uvc)
{
	int ret;
	int type;

	if(uvc == NULL || uvc->uvc_dev[0] == NULL)
	{
		ucam_error("error uvc == NULL\n");
		return -ENOMEM;
	}

	ucam_notice("connec usb\n");
	//重新连接USB
	type = USB_STATE_CONNECT;	
	if ((ret = ioctl(uvc->uvc_dev[0]->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
		ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
		strerror(errno), errno);	
		return -1;
	}
	return 0;
}

int uvc_usb_reconnet (struct uvc *uvc)
{
	int ret;
	int type;

	if(uvc == NULL || uvc->uvc_dev[0] == NULL)
	{
		ucam_error("error uvc == NULL\n");
		return -ENOMEM;
	}

	ucam_notice("reconnet usb\n");
	//重新连接USB
	type = USB_STATE_CONNECT_RESET;	
	if ((ret = ioctl(uvc->uvc_dev[0]->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
		ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
		strerror(errno), errno);	
		return -1;
	}
	return 0;
}

int uvc_usb_disconnet (struct uvc *uvc)
{
	int ret;
	int type;

	if(uvc == NULL || uvc->uvc_dev[0] == NULL)
	{
		ucam_error("error uvc == NULL\n");
		return -ENOMEM;
	}

	ucam_notice("connec usb\n");
	//重新连接USB
	type = USB_STATE_DISCONNECT;	
	if ((ret = ioctl(uvc->uvc_dev[0]->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
		ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
		strerror(errno), errno);	
		return -1;
	}
	return 0;
}



int uvc_get_usb_speed(struct uvc *uvc, uint32_t *usb_speed)
{
	if(uvc == NULL)
	{
		return -ENOMEM;
	}
	*usb_speed = uvc->usb_speed; 
	return 0;
}

int uvc_get_usb_connet_status(struct uvc *uvc, uint32_t *connet_status)
{
	if(uvc == NULL)
	{
		return -ENOMEM;
	}
	*connet_status = uvc->usb_connet_status;
	return 0;
}

int uvc_get_usb_config_index(struct uvc *uvc, uint32_t *config_index)
{
	if(uvc == NULL)
	{
		return -ENOMEM;
	}
	*config_index = uvc->bConfigurationValue;
	return 0;
}

int uvc_get_protocol(struct uvc *uvc, uint32_t *uvc_protocol)
{
	if(uvc == NULL || uvc->uvc_dev[0] == NULL)
	{
		return -ENOMEM;
	}
	*uvc_protocol = uvc->uvc_dev[0]->config.uvc_protocol;
	return 0;
}


int uvc_set_protocol(struct uvc *uvc, uint32_t uvc_protocol)
{
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos;
	int set_flag = 0;

	if(uvc == NULL)
	{
		return -ENOMEM;
	}

	if(uvc->init_flag == 0)
	{
		ucam_error("error uvc init_flag:%d\n", uvc->init_flag);
		return -1;
	}

	if(uvc_protocol != 0x100 && uvc_protocol != 0x110 && uvc_protocol != 0x150)
	{
		ucam_error("error uvc_protocol:0x%x", uvc_protocol);
		return -1;
	}


	ucam_list_for_each(pos, &uvc->uvc_dev_list)
	{
		uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
        if(uvc_dev)
		{	
			if(uvc_dev->bConfigurationValue == 1)
			{
				if(uvc_dev->config.uvc_protocol != uvc_protocol)
				{
					uvc_dev->config.uvc_protocol = uvc_protocol;
					if(reset_uvc_descriptor(uvc_dev, 1, 0) != 0)
					{
						ucam_error("reset_uvc_descriptor fail!");
						return -1;
					}
					set_flag = 1;
				}	
			}		
		}
    }

	if(set_flag)
	{
		//uvc_streamoff_all(uvc->uvc_dev[0]);
		uvc_usb_reconnet(uvc);
	}
	return 0;
}


int set_usb3_isoc_speed_fast_mode(struct uvc *uvc, int mode)
{
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos;

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
			if(uvc_dev->config.uvc_ep_mode != UVC_USB_TRANSFER_MODE_BULK)
			{
				//双流时留出带宽给第二路
				if(uvc_dev->uvc->uvc_s2_enable == 0 && mode)
				{
					//USB3.0 ISOC带宽限制，win7最大带宽不能超过256M/s(ALT21)
					uvc_dev->config.MaxPayloadTransferSize_usb3 = USB3_EP_ISOC_PACKET_SIZE_ALT23;
					uvc_dev->config.MaxPayloadTransferSize_usb3_win7 = USB3_EP_ISOC_PACKET_SIZE_ALT21;
				}

				//USB3.0 大于或者等于1080P时，以高带宽发送来降低延时
				uvc_dev->config.usb3_isoc_speed_fast_mode = mode;
				//USB3.0 带宽计算倍数，一般至少预留20%以上的带宽
				if(uvc_dev->config.usb3_isoc_speed_fast_mode)
					uvc_dev->config.usb3_isoc_speed_multiple = 1.5;
				else
					uvc_dev->config.usb3_isoc_speed_multiple = 1.2;

				uvc_dev->config.win7_usb3_isoc_speed_multiple = 1.2;
			}		
		}
    }

	return 0;
}

int uvc_set_usb3_isoc_win7_mode(struct uvc *uvc, uint8_t usb3_isoc_win7_mode)
{
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos;
	int set_flag = 0;

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
			if(uvc_dev->config.uvc_ep_mode != UVC_USB_TRANSFER_MODE_BULK &&
				uvc_dev->bConfigurationValue == 1)
			{
				if(uvc_dev->config.usb3_isoc_win7_mode != usb3_isoc_win7_mode)
				{
					uvc_dev->config.usb3_isoc_win7_mode = usb3_isoc_win7_mode;
					if(reset_uvc_descriptor(uvc_dev, 0, 0) != 0)
					{
						ucam_error("reset_uvc_descriptor fail!");
						return -1;
					}
					set_flag = 1;
				}	
			}		
		}
    }

	if(set_flag)
	{
		//uvc_streamoff_all(uvc->uvc_dev[0]);
		uvc_usb_reconnet(uvc);
	}

	return 0;
}



int uvc_register_usb_connet_event_callback(struct uvc *uvc, usb_connet_event_callback_t callback)
{
    if(uvc == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uvc->usb_connet_event_callback = callback;

    return 0;
}


int uvc_register_stream_event_callback(struct uvc *uvc, uvc_stream_event_callback_t callback)
{
    if(uvc == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uvc->uvc_stream_event_callback = callback;

    return 0;
}


int uvc_register_uvc_requests_ctrl_event_callback(struct uvc *uvc, uvc_requests_ctrl_fn_t callback)
{
    if(uvc == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uvc->uvc_requests_ctrl_event_callback = callback;

    return 0;
}


int uvc_register_usb_suspend_event_callback(struct uvc *uvc, uvc_usb_suspend_event_callback_t callback)
{
    if(uvc == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uvc->usb_suspend_event_callback = callback;

    return 0;
}

int uvc_register_usb_resume_event_callback(struct uvc *uvc, uvc_usb_resume_event_callback_t callback)
{
    if(uvc == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uvc->usb_resume_event_callback = callback;

    return 0;
}

int uvc_register_dam_memcpy_callback(struct uvc *uvc, uvc_dam_memcpy_callback_t callback, uint32_t align)
{
    if(uvc == NULL || callback == NULL)
    {
        return -ENOMEM;
    }

	if(align < 1)
		align = 1;

    uvc->uvc_dam_memcpy_callback = callback;
	uvc->uvc_dam_align = align;
    return 0;
}

struct uvc * create_uvc(struct ucam *ucam, uint32_t uvc_video_stream_max)
{
    struct uvc *uvc = NULL;

	ucam_error("VHD libucam Version:%s, SVN/Git:%s [Build:%s]",VHD_LIBUCAM_VERSION,LIB_UCAM_SVN_GIT_VERSION,LIB_UCAM_COMPILE_INFO);

	if(uvc_video_stream_max > UVC_VIDEO_STREAM_MAX)
	{
		ucam_error("uvc_video_stream_max Error!");
		return NULL;
	}

    uvc = malloc(sizeof (*uvc));
	if (uvc == NULL) {
        ucam_error("uvc Malloc Error!");
		return NULL;
	}

    memset(uvc, 0, sizeof (*uvc));
	UCAM_INIT_LIST_HEAD(&uvc->uvc_dev_list);
    UCAM_INIT_LIST_HEAD(&uvc->uvc_xu_ctrl_list);
	pthread_mutex_init(&uvc->uvc_event_mutex, 0);
	uvc->ucam = ucam;
	uvc->config.uvc_video_stream_max = uvc_video_stream_max;
	uvc->config.reset_usb_sn_enable = 1;
	uvc->config.usb_speed_max = UCAM_USB_SPEED_SUPER;
	uvc->config.wCompQualityMax = 61;
	uvc->config.uvc_xu_h264_ctrl_enable = 1;

	uvc->bConfigurationValue = 1;
	uvc->usb_speed = UCAM_USB_SPEED_SUPER;
	uvc->dfu_enable = 1;
	uvc->uac_enable = 1;
	uvc->uvc_s2_enable = 0;
	uvc->uvc_v150_enable = 1;
	uvc->win7_usb3_enable = 1;
	uvc->zte_hid_enable = 0;
	uvc->uvc_dam_align = 1;

	uvc->uvc_dev = malloc(uvc_video_stream_max * sizeof(struct uvc_dev *));
	if (uvc->uvc_dev == NULL) {
		ucam_error("uvc->uvc_dev Malloc Error!");
		goto error;
	}
	memset(uvc->uvc_dev, 0, uvc_video_stream_max * sizeof(struct uvc_dev *));

    return uvc;

error:
	SAFE_FREE(uvc->uvc_dev);
    SAFE_FREE(uvc);
	return NULL;
}

int uvc_free(struct uvc *uvc)
{
    int save_flag = 0;
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos, *npos;
	int stream_num;

    if(uvc == NULL)
    {
        ucam_error("uvc == NULL\n");
        return -ENOMEM;
    }

	for(stream_num = 0; stream_num < UVC_VIDEO_STREAM_MAX; stream_num++)
	{
		SAFE_FREE(uvc->h264_svc_buf[stream_num]);
	}

	ucam_list_for_each_safe(pos, npos,  &uvc->uvc_dev_list)
	{
		uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
        if(uvc_dev)
		{	
			if(save_flag == 0)
			{
				save_flag = 1;
				//uvc_api_set_save_settings ();
			}	
			ucam_list_del(&uvc_dev->list);
			uvc_close(uvc_dev);						
		}
    }

	SAFE_FREE(uvc->uvc_dev);
    SAFE_FREE(uvc);

    return 0;
}
