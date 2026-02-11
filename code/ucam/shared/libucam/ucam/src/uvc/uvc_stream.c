/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 13:45:47
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream.c
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
#include <ucam/uvc/uvc_ctrl_vs.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_stream.h>
#include <ucam/uvc/uvc_api_config.h>
#include <ucam/uvc/uvc_api_eu.h>
#include "uvc_stream.h"

#define UVC_SEND_SELECT_TIMEOUT (10*1000)

// This module is for GCC Neon armv8 64 bit.
#if defined(__aarch64__)
// Copy multiple of 32.
void UVC_CopyRow_NEON(const uint8_t* src, uint8_t* dst, int count) {
  asm volatile(
      "1:                                        \n"
      "ldp        q0, q1, [%0], #32              \n"
      "subs       %w2, %w2, #32                  \n"  // 32 processed per loop
      "stp        q0, q1, [%1], #32              \n"
      "b.gt       1b                             \n"
      : "+r"(src),                  // %0
        "+r"(dst),                  // %1
        "+r"(count)                 // %2  // Output registers
      :                             // Input registers
      : "cc", "memory", "v0", "v1"  // Clobber List
  );
}
#elif !defined(__x86_64__)
// Copy multiple of 32.  vld4.8  allow unaligned and is fastest on a15.
static void UVC_CopyRow_NEON(const uint8_t* src, uint8_t* dst, int count) {
  asm volatile (
  "1:                                          \n"
    //MEMACCESS(0)
    "vld1.8     {d0, d1, d2, d3}, [%0]!        \n"  // load 32
    "subs       %2, %2, #32                    \n"  // 32 processed per loop
    //MEMACCESS(1)
    "vst1.8     {d0, d1, d2, d3}, [%1]!        \n"  // store 32
    "bgt        1b                             \n"
  : "+r"(src),   // %0
    "+r"(dst),   // %1
    "+r"(count)  // %2  // Output registers
  :                     // Input registers
  : "cc", "memory", "q0", "q1"  // Clobber List
  );
}

#endif

void uvc_memcpy_neon(uint8_t* dst, const uint8_t* src, int size)
{	
	if(dst == NULL || src == 0 || size <= 0)
	{
		return;
	}
#if !defined(__x86_64__)
	int r = size & 0x1F;                                                    
    int n = size & ~0x1F;
	if(n > 0)
	{
		UVC_CopyRow_NEON(src, dst, n);
	}

	if(r > 0)
	{
		memcpy(dst + n, src + n, r);
	}
#else
	memcpy(dst, src, size);
#endif
}

void uvc_memcpy(uint8_t* dst, const uint8_t* src, int size)
{
	if(dst == NULL || src == 0 || size <= 0)
	{
		return;
	}	
#if 0
	int r = size & 0x1F;                                                    
    int n = size & ~0x1F;
	if(n > 0)
	{
		UVC_CopyRow_NEON(src, dst, n);
	}

	if(r > 0)
	{
		memcpy(dst + n, src + n, r);
	}
#else
	memcpy(dst, src, size);
#endif
}

static const uint8_t nalu_header[4] = {0x00, 0x00, 0x00, 0x01};

int uvc_nalu_recode_avc(const uint8_t *src, int src_count, uint8_t *dst)  
{
    int dst_count = 0;
    int slice_index[20] = {0};//假设最多20个slice  由于设置每个slice最大的大小是2K，相当于每个包最大400K
    int i;
	const uint8_t *p_src = src;
	uint8_t *p_dst = dst;
	unsigned int count = 0;
	unsigned int slice_size = 0;
	
	
    if((src[4] & 0x1f) == 0x01)//type 20
    {
		for(i = 0; i < src_count; i++)
		{
			if(p_src[i] == 0 && p_src[i+1] == 0 && p_src[i+2] == 0 && p_src[i+3] == 1 && (p_src[i+4] & 0x1f) == 0x01)// find SEI and remove it
			{
				slice_index[count] = i;
				count++;
				i = i + 4;
			}
		}
		//printf("slice count: %d \n", count);
		if(count == 0)
			return 0;
		
		for(i = 0; i < count - 1; i++)
		{
			slice_size = slice_index[i+1] - slice_index[i];
			uvc_memcpy(p_dst, nalu_header, 4);
			p_dst[4] = 0x6e;
			p_dst[5] = 0x80;
			p_dst[6] = 0x80;
			p_dst[7] = 0x0f;
			uvc_memcpy(p_dst + 8, p_src, slice_size);
			p_dst += (slice_size + 8);
			p_src += slice_size;
		}
		//拷贝最后一个slice
		slice_size = src_count - slice_index[count - 1];
		uvc_memcpy(p_dst, nalu_header, 4);
		p_dst[4] = 0x6e;
		p_dst[5] = 0x80;
		p_dst[6] = 0x80;
		p_dst[7] = 0x0f;
		uvc_memcpy(p_dst + 8, p_src, slice_size);
		dst_count = src_count + 8 * count;
    } else if((src[4] & 0x1f) == 0x7){
		//sps   I帧在这里
		for(i = 0; i < src_count; i++)
		{
			if(p_src[i] == 0 && p_src[i+1] == 0 && p_src[i+2] == 0 && p_src[i+3] == 1 && (p_src[i+4] & 0x1f) == 0x05)// find SEI and remove it
			{
				slice_index[count] = i;
				count++;
				i = i + 4;
			}
		}
		//printf("slice count: %d \n", count);
		if(count == 0)
			return 0;
		
		
		uvc_memcpy(p_dst, p_src, slice_index[0]);
		p_dst += slice_index[0];
		p_src += slice_index[0];
		//拷贝第一个到第count-1个slice
		for(i = 0; i < count - 1; i++)
		{
			slice_size = slice_index[i+1] - slice_index[i];
			uvc_memcpy(p_dst, nalu_header, 4);
			p_dst[4] = 0x6e;
			p_dst[5] = 0xc0;
			p_dst[6] = 0x80;
			p_dst[7] = 0x0f;
			uvc_memcpy(p_dst + 8, p_src, slice_size);
			p_dst += (slice_size + 8);
			p_src += slice_size;
		}
		//拷贝最后一个slice
		slice_size = src_count - slice_index[count - 1];
		uvc_memcpy(p_dst, nalu_header, 4);
		p_dst[4] = 0x6e;
		p_dst[5] = 0xc0;
		p_dst[6] = 0x80;
		p_dst[7] = 0x0f;
		uvc_memcpy(p_dst + 8, p_src, slice_size);
		dst_count = src_count + 8 * count;
    }
    return dst_count;
}

int uvc_nalu_recode_svc(const uint8_t *src, int src_count, uint8_t *dst)  
{
    int dst_count = 0;
    int slice_index[20] = {0};//假设最多20个slice  由于设置每个slice最大的大小是2K，相当于每个包最大400K
	unsigned int count = 0;
    int i;
    int temporal_id;
	const uint8_t *p_src = src;
	uint8_t *p_dst = dst;
	unsigned int slice_size = 0;
    if((src[4] & 0x1f) == 0x14)//type 20
    {
		for(i = 0; i < src_count; i++)
		{
			if(p_src[i] == 0 && p_src[i+1] == 0 && p_src[i+2] == 0 && p_src[i+3] == 1 && (p_src[i+4] & 0x1f) == 0x14)
			{
				slice_index[count] = i;
				count++;
				i = i + 4;
			}
		}
		//strace("slice count: %d \n", count);
		if(count == 0)
			return 0;
		
		for(i = 0; i < count - 1; i++)
		{
			slice_size = slice_index[i+1] - slice_index[i];
			uvc_memcpy(p_dst, p_src, 8);
			p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
			temporal_id = (p_dst[7] >> 5) & 0x7;
			//temporal_id = temporal_id - 1;//采用标准的，060 SDK不标准
			p_dst[5] = p_dst[5] | temporal_id;
			p_dst[6] = 0x80;
			p_dst[7] = (temporal_id << 5) | (p_dst[7] & 0x1f);
			uvc_memcpy(p_dst+8, nalu_header, 4);
			/*
			if(temporal_id == 1)
				p_dst[12] = 0x41;
			else if(temporal_id == 2)
				p_dst[12] = 0x21;
			else if(temporal_id == 3)
				p_dst[12] = 0x01;
			*/
			if(temporal_id == 0)
				p_dst[12] = 0x41;
			else if(temporal_id == 1)
				p_dst[12] = 0x21;
			else if(temporal_id == 2)
				p_dst[12] = 0x01;

			uvc_memcpy(p_dst+13, p_src+8, slice_size -8);
			
			p_dst += (slice_size + 5);
			p_src += slice_size;
		}
		
		slice_size = src_count - slice_index[count - 1];
		uvc_memcpy(p_dst, p_src, 8);
		p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
		temporal_id = (p_dst[7] >> 5) & 0x7;
		//temporal_id = temporal_id - 1;//采用标准的，060 SDK不标准
		p_dst[5] = p_dst[5] | temporal_id;
		p_dst[6] = 0x80;
		p_dst[7] = (temporal_id << 5) | (p_dst[7] & 0x1f);
		uvc_memcpy(p_dst+8, nalu_header, 4);
/*
		if(temporal_id == 1)
			p_dst[12] = 0x41;
		else if(temporal_id == 2)
			p_dst[12] = 0x21;
		else if(temporal_id == 3)
			p_dst[12] = 0x01;
*/
		if(temporal_id == 0)
		    p_dst[12] = 0x41;
		else if(temporal_id == 1)
		    p_dst[12] = 0x21;
		else if(temporal_id == 2)
		    p_dst[12] = 0x01;
		uvc_memcpy(p_dst+13, p_src+8, slice_size - 8);
		
		dst_count = src_count + 5 * count;
		
		
    } else if((src[4] & 0x1f) == 0xF){
		//sps   I帧在这里
		for(i = 0; i < src_count; i++)
		{
			if(src[i] == 0 && src[i+1] == 0 && src[i+2] == 0 && src[i+3] == 1 && (src[i+4] & 0x1f) == 0x14)
			{
				slice_index[count] = i;
				count++;
				i = i + 4;
			}
		}
		//strace("slice count: %d \n", count);
		if(count == 0)
			return 0;
		
		uvc_memcpy(p_dst, p_src, slice_index[0]);
		dst[4] = 0x67;
		dst[5] = 0x64;
		p_dst += slice_index[0];
		p_src += slice_index[0];
		
		
		for(i = 0; i < count - 1; i++)
		{
			slice_size = slice_index[i+1] - slice_index[i];
			uvc_memcpy(p_dst, p_src, 8);
			p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
			temporal_id = (p_dst[7] >> 5) & 0x7;
			p_dst[5] = p_dst[5] | temporal_id;
			p_dst[6] = 0x80;
			uvc_memcpy(p_dst+8, nalu_header, 4);
			p_dst[12] = 0x65;
			uvc_memcpy(p_dst+13, p_src+8, slice_size -8);
			
			p_dst += (slice_size + 5);
			p_src += slice_size;
		}
		
		slice_size = src_count - slice_index[count - 1];
		uvc_memcpy(p_dst, p_src, 8);
		p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
		temporal_id = (p_dst[7] >> 5) & 0x7;
		p_dst[5] = p_dst[5] | temporal_id;
		p_dst[6] = 0x80;
		uvc_memcpy(p_dst+8, nalu_header, 4);
		p_dst[12] = 0x65;
		uvc_memcpy(p_dst+13, p_src+8, slice_size - 8);
		
		dst_count = src_count + 5 * count;
		
    }

    return dst_count;
}

int uvc_nalu_recode_svc_layers_3_to_2(const uint8_t *src, int src_count, uint8_t *dst)  
{
    int dst_count = 0;
    int slice_index[20] = {0};//假设最多20个slice  由于设置每个slice最大的大小是2K，相当于每个包最大400K
	unsigned int count = 0;
    int i;
    int temporal_id;
	const uint8_t *p_src = src;
	uint8_t *p_dst = dst;
	unsigned int slice_size = 0;
    if((src[4] & 0x1f) == 0x14)//type 20
    {
		for(i = 0; i < src_count; i++)
		{
			if(p_src[i] == 0 && p_src[i+1] == 0 && p_src[i+2] == 0 && p_src[i+3] == 1 && (p_src[i+4] & 0x1f) == 0x14)
			{
				slice_index[count] = i;
				count++;
				i = i + 4;
			}
		}
		//strace("slice count: %d \n", count);
		if(count == 0)
			return 0;
		
		for(i = 0; i < count - 1; i++)
		{
			slice_size = slice_index[i+1] - slice_index[i];
			uvc_memcpy(p_dst, p_src, 8);
			p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
			temporal_id = (p_dst[7] >> 5) & 0x7;
			//temporal_id = temporal_id - 1;//采用标准的，060 SDK不标准
			
			if(temporal_id == 0)
			{
				temporal_id = 0;
			}
			else if(temporal_id == 1)
			{
				temporal_id = 0;
			}
			else if(temporal_id == 2)
			{
				temporal_id = 1;
			}
			
			p_dst[5] = p_dst[5] | temporal_id;
			p_dst[6] = 0x80;
			p_dst[7] = (temporal_id << 5) | (p_dst[7] & 0x1f);
			uvc_memcpy(p_dst+8, nalu_header, 4);
			/*
			if(temporal_id == 1)
				p_dst[12] = 0x41;
			else if(temporal_id == 2)
				p_dst[12] = 0x21;
			else if(temporal_id == 3)
				p_dst[12] = 0x01;
			*/
			if(temporal_id == 0)
				p_dst[12] = 0x21;
			else if(temporal_id == 1)
				p_dst[12] = 0x01;
			else if(temporal_id == 2)
				p_dst[12] = 0x01;

			uvc_memcpy(p_dst+13, p_src+8, slice_size -8);
			
			p_dst += (slice_size + 5);
			p_src += slice_size;
		}
		
		slice_size = src_count - slice_index[count - 1];
		uvc_memcpy(p_dst, p_src, 8);
		p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
		temporal_id = (p_dst[7] >> 5) & 0x7;
		//temporal_id = temporal_id - 1;//采用标准的，060 SDK不标准
		if(temporal_id == 0)
		{
			temporal_id = 0;
		}
		else if(temporal_id == 1)
		{
			temporal_id = 0;
		}
		else if(temporal_id == 2)
		{
			temporal_id = 1;
		}
			
		p_dst[5] = p_dst[5] | temporal_id;
		p_dst[6] = 0x80;
		p_dst[7] = (temporal_id << 5) | (p_dst[7] & 0x1f);
		uvc_memcpy(p_dst+8, nalu_header, 4);
/*
		if(temporal_id == 1)
			p_dst[12] = 0x41;
		else if(temporal_id == 2)
			p_dst[12] = 0x21;
		else if(temporal_id == 3)
			p_dst[12] = 0x01;
*/
		if(temporal_id == 0)
		    p_dst[12] = 0x21;
		else if(temporal_id == 1)
		    p_dst[12] = 0x01;
		else if(temporal_id == 2)
		    p_dst[12] = 0x01;
		uvc_memcpy(p_dst+13, p_src+8, slice_size - 8);
		
		dst_count = src_count + 5 * count;
		
		
    } else if((src[4] & 0x1f) == 0xF){
		//sps   I帧在这里
		for(i = 0; i < src_count; i++)
		{
			if(src[i] == 0 && src[i+1] == 0 && src[i+2] == 0 && src[i+3] == 1 && (src[i+4] & 0x1f) == 0x14)
			{
				slice_index[count] = i;
				count++;
				i = i + 4;
			}
		}
		//strace("slice count: %d \n", count);
		if(count == 0)
			return 0;
		
		uvc_memcpy(p_dst, p_src, slice_index[0]);
		dst[4] = 0x67;
		dst[5] = 0x64;
		p_dst += slice_index[0];
		p_src += slice_index[0];
		
		
		for(i = 0; i < count - 1; i++)
		{
			slice_size = slice_index[i+1] - slice_index[i];
			uvc_memcpy(p_dst, p_src, 8);
			p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
			temporal_id = (p_dst[7] >> 5) & 0x7;
			
			if(temporal_id == 0)
			{
				temporal_id = 0;
			}
			else if(temporal_id == 1)
			{
				temporal_id = 0;
			}
			else if(temporal_id == 2)
			{
				temporal_id = 1;
			}
			p_dst[5] = p_dst[5] | temporal_id;
			p_dst[6] = 0x80;
			uvc_memcpy(p_dst+8, nalu_header, 4);
			p_dst[12] = 0x65;
			uvc_memcpy(p_dst+13, p_src+8, slice_size -8);
			
			p_dst += (slice_size + 5);
			p_src += slice_size;
		}
		
		slice_size = src_count - slice_index[count - 1];
		uvc_memcpy(p_dst, p_src, 8);
		p_dst[4] = (p_dst[4] & 0x60) | 0x0E;
		temporal_id = (p_dst[7] >> 5) & 0x7;
		if(temporal_id == 0)
		{
			temporal_id = 0;
		}
		else if(temporal_id == 1)
		{
			temporal_id = 0;
		}
		else if(temporal_id == 2)
		{
			temporal_id = 1;
		}
		
		p_dst[5] = p_dst[5] | temporal_id;
		p_dst[6] = 0x80;
		uvc_memcpy(p_dst+8, nalu_header, 4);
		p_dst[12] = 0x65;
		uvc_memcpy(p_dst+13, p_src+8, slice_size - 8);
		
		dst_count = src_count + 5 * count;
		
    }

    return dst_count;
}

int uvc_get_stream_on_status(struct uvc *uvc, uint32_t uvc_stream_num, uint32_t *stream_on_status)
{
	if(uvc == NULL)
	{
		return -ENOMEM;
	}

	if(uvc_stream_num >= uvc->config.uvc_video_stream_max)
		return -1;
	
	if(uvc->uvc_dev[uvc_stream_num] == NULL)
	{
		return -ENOMEM;
	}

	*stream_on_status = uvc->uvc_dev[uvc_stream_num]->uvc_stream_on;

	return 0;
}

int uvc_get_uvc_all_stream_on_status(struct uvc *uvc, uint32_t *stream_on_status)
{
	struct uvc_dev *uvc_dev = NULL;
	struct ucam_list_head *pos;

	if(uvc == NULL)
	{
		return -ENOMEM;
	}

	ucam_list_for_each(pos, &uvc->uvc_dev_list)
	{
		uvc_dev = ucam_list_entry(pos, struct uvc_dev, list);
		if(uvc_dev)
		{
			if(uvc_dev->uvc_stream_on)
			{
				*stream_on_status = 1;
				return 0;
			}											
		}
	}

	*stream_on_status = 0;
	return 0;
}

int uvc_video_dqbuf_lock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf, void **mem)
{
	fd_set fds;
	int ret;
	int usb_link_state;

#if 0
	if(uvc == NULL || buf == NULL)
	{
		return -ENOMEM;
	}

	if(uvc_stream_num >= uvc->config.uvc_video_stream_max)
		return -1;

	dev = uvc->uvc_dev[uvc_stream_num];
#endif

	if(dev == NULL || buf == NULL)
	{
		return -ENOMEM;
	}

	if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	if(dev->uvc->usb_connet_status == 0 || dev->uvc->usb_suspend)
	{
		return -EMLINK;
	}

	pthread_mutex_lock(&dev->uvc_reqbufs_mutex);
	*mem = NULL;

	FD_ZERO(&fds);
	FD_SET(dev->fd, &fds);
	struct timeval to;
	
	while( dev->uvc_stream_on ){
		fd_set wfds = fds;
		to.tv_sec = 0; to.tv_usec = UVC_SEND_SELECT_TIMEOUT;
		ret = select(dev->fd + 1, NULL, &wfds, NULL, &to);
		if ( ret == - 1 ){
			ucam_trace("select fail!");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			return -1;
		}
		else if(ret == 0)
		{
			ucam_error("select timeout\n");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			ret = uvc_ioctl_get_usb_link_state(dev , &usb_link_state);
			if(ret == 0 && usb_link_state != 0)
			{
				ucam_error("usb_link_state:%d\n",usb_link_state);					
				//uvc_streamoff_all(dev);
				// if(dev->uvc_stream_on)
				// {
				// 	//uvc_video_stream(dev, 0);
				// 	dev->uvc->usb_connet_status = 0;
				// }
				return -EMLINK;
			}	
			return -1;
		}


		if (FD_ISSET(dev->fd , &wfds)){
			//ucam_trace("enter");
				
			memset(buf, 0, sizeof(struct v4l2_buffer));
			buf->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf->memory = dev->config.reqbufs_memory_type;

			if(dev->uvc_stream_on == 0){
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -EALREADY;	
			}

			if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, buf)) < 0) {
				ucam_error("Unable to dequeue buffer: ret=%d, %s (%d), uvc_stream_on:%d", ret, strerror(errno),
					errno, dev->uvc_stream_on);
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -1;
			}

			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				*mem = dev->mem[buf->index] + dev->reqbufs_config.data_offset;
			}
			else
			{
				*mem = dev->mem[buf->index];
			}
			
			return 0;
		}
		else
		{
			//usleep(1000);
		}
	}

	if(*mem == NULL || dev->uvc_stream_on == 0)
	{
		ucam_error("error mem=NULL\n");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return -1;
	}

	return 0;
}

int uvc_video_dqbuf_nolock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf, void **mem)
{
	fd_set fds;
	int ret;
	int usb_link_state;

#if 0
	if(uvc == NULL || buf == NULL)
	{
		return -ENOMEM;
	}

	if(uvc_stream_num >= uvc->config.uvc_video_stream_max)
		return -1;

	dev = uvc->uvc_dev[uvc_stream_num];
#endif

	if(dev == NULL || buf == NULL)
	{
		return -ENOMEM;
	}

	if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	if(dev->uvc->usb_connet_status == 0 || dev->uvc->usb_suspend)
	{
		return -EMLINK;
	}

	//pthread_mutex_lock(&dev->uvc_reqbufs_mutex);
	*mem = NULL;

	FD_ZERO(&fds);
	FD_SET(dev->fd, &fds);
	struct timeval to;
	while( dev->uvc_stream_on ){
		fd_set wfds = fds;
		to.tv_sec = 0; to.tv_usec = UVC_SEND_SELECT_TIMEOUT;
		ret = select(dev->fd + 1, NULL, &wfds, NULL, &to);
		if ( ret == - 1 ){
			ucam_trace("select fail!");
			//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			return -1;
		}
		else if(ret == 0)
		{
			ucam_error("select timeout\n");
			//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

			ret = uvc_ioctl_get_usb_link_state(dev , &usb_link_state);
			if(ret == 0 && usb_link_state != 0)
			{
				ucam_error("usb_link_state:%d\n",usb_link_state);	
				//uvc_streamoff_all(dev);
				// if(dev->uvc_stream_on)
				// {
				// 	//uvc_video_stream(dev, 0);
				// 	dev->uvc->usb_connet_status = 0;
				// }
				return -EMLINK;
			}	
			return -1;
		}

		if (FD_ISSET(dev->fd , &wfds)){
			//ucam_trace("enter");
				
			memset(buf, 0, sizeof(struct v4l2_buffer));
			buf->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf->memory = dev->config.reqbufs_memory_type;

			if(dev->uvc_stream_on == 0){
				//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -EALREADY;	
			}

			if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, buf)) < 0) {
				ucam_error("Unable to dequeue buffer: ret=%d, %s (%d), uvc_stream_on:%d", ret, strerror(errno),
					errno, dev->uvc_stream_on);
				//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -1;
			}

			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				*mem = dev->mem[buf->index] + dev->reqbufs_config.data_offset;
			}
			else
			{
				*mem = dev->mem[buf->index];
			}
			return 0;
		}
		else
		{
			//usleep(1000);
		}
	}

	if(*mem == NULL || dev->uvc_stream_on == 0)
	{
		ucam_error("error mem=NULL\n");
		//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return -1;
	}

	return 0;
}


int uvc_video_qbuf_lock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf)
{
	int ret;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;

#if 0
	if(uvc == NULL)
	{
		return -ENOMEM;
	}

	if(uvc_stream_num >= uvc->config.uvc_video_stream_max)
		return -1;

	dev = uvc->uvc_dev[uvc_stream_num];
#endif
	if(dev == NULL || buf == NULL)
	{
		return -ENOMEM;
	}

	if(dev->uvc_stream_on == 0)
	{
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return -EALREADY;
	}

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		buf->m.userptr = (unsigned long)dev->mem[buf->index];
		buf->length = dev->video_reqbufs_length[buf->index];
	}

	if(buf->bytesused > buf->length)
	{
		ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", buf->bytesused, buf->length);
		buf->bytesused = 0;
		goto qbuf;
	}

	
	if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && dev->stream_num && dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon)
	{
		if(get_simulcast_start_or_stop_layer(dev, (uint32_t)venc_chn) != 0)
		{
			*((uint32_t *)(dev->mem[buf->index] + buf->bytesused + dev->reqbufs_config.data_offset)) = venc_chn_to_simulcast_uvc_header_id(dev, (uint32_t)venc_chn);
		}
	}

qbuf:
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_video_reqbufs_flush_cache(dev, buf->index);
		reqbufs_buf_used.index = buf->index;
		reqbufs_buf_used.buf_used = buf->bytesused;
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			//goto error;
		}
	}

	if ((ret = ioctl(dev->fd, VIDIOC_QBUF, buf)) < 0) {
		ucam_error("Unable to requeue buffer: ret=%d, %s (%d), uvc_stream_on:%d", ret, strerror(errno),
					errno, dev->uvc_stream_on);
		ucam_error("bytesused = %d\n",buf->bytesused);
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return -1;
	}

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return 0;
}

int uvc_video_qbuf_nolock(struct uvc_dev *dev, int32_t venc_chn, struct v4l2_buffer *buf)
{
	int ret;

#if 0
	if(uvc == NULL)
	{
		return -ENOMEM;
	}

	if(uvc_stream_num >= uvc->config.uvc_video_stream_max)
		return -1;

	dev = uvc->uvc_dev[uvc_stream_num];
#endif
	if(dev == NULL || buf == NULL)
	{
		return -ENOMEM;
	}

	if(dev->uvc_stream_on == 0)
	{
		//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return -EALREADY;
	}

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		buf->m.userptr = (unsigned long)dev->mem[buf->index];
		buf->length = dev->video_reqbufs_length[buf->index];
	}

	if(buf->bytesused > buf->length)
	{
		ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", buf->bytesused, buf->length);
		buf->bytesused = 0;
		goto qbuf;
	}

	
	if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && dev->stream_num && dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon)
	{
		if(get_simulcast_start_or_stop_layer(dev, venc_chn) != 0)
		{
			((uint32_t *)(dev->mem[buf->index] + buf->bytesused + dev->reqbufs_config.data_offset))[0] = venc_chn_to_simulcast_uvc_header_id(dev, venc_chn);
		}
	}

qbuf:
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_video_reqbufs_flush_cache(dev, buf->index);
	}

	if ((ret = ioctl(dev->fd, VIDIOC_QBUF, buf)) < 0) {
		ucam_error("Unable to requeue buffer: ret=%d, %s (%d), uvc_stream_on:%d", ret, strerror(errno),
				errno, dev->uvc_stream_on);
		ucam_error("bytesused = %d\n",buf->bytesused);
		//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return -1;
	}

	//pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return 0;
}

int uvc_memcpy_and_qbuf(struct uvc_dev *dev, int32_t venc_chn, 
    struct v4l2_buffer *v4l2_buf, void *uvc_mem, 
    const void *send_data, uint32_t send_size, int qbuf_en, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used)
{
    int ret;
    uint32_t offset = 0;
    int qbuf_flag = 0;
	uint32_t qbuf_transfer_size;

	if(uvc_mem == NULL || send_data == NULL || send_size == 0 )
	{
		ucam_error("data error");
		goto error;
	}
	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
	{
		qbuf_transfer_size = send_size;
	}
	else
	{
		qbuf_transfer_size = dev->PayloadTransferSize << 5;
		if(qbuf_transfer_size > 512*1024)
		{
			qbuf_transfer_size = 512*1024;
		}
		qbuf_transfer_size = ALIGN_DOWN(qbuf_transfer_size,32);
		if(qbuf_transfer_size < 32)
		{
			qbuf_transfer_size = 32;
		}
	}
    
    //v4l2_buf->bytesused = send_size;
    //reqbufs_buf_used->index = v4l2_buf->index;
	//reqbufs_buf_used->buf_used = buf_used;

	

    while(send_size)
    {
        if(send_size <= qbuf_transfer_size)
        {
            uvc_memcpy(uvc_mem + offset, send_data + offset, send_size);
            reqbufs_buf_used->buf_used += send_size;
			send_size = 0;
#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif
            ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
            if(ret != 0)
            {
                ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
                goto error;
            }

           	if(qbuf_en && qbuf_flag == 0)
            {
                qbuf_flag = 1;
                ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
                if(ret != 0)
                {
                    ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
                    return -1;
                }	
            }

            return 0;
        }
        else
        {
            uvc_memcpy(uvc_mem + offset, send_data + offset, qbuf_transfer_size);
            send_size -= qbuf_transfer_size;
            offset += qbuf_transfer_size;
            reqbufs_buf_used->buf_used += qbuf_transfer_size;
#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif			
            ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
            if(ret != 0)
            {
                ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
                goto error;
            }

           	if(qbuf_en && qbuf_flag == 0)
            {
                qbuf_flag = 1;
                ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
                if(ret != 0)
                {
                    ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
                    return -1;
                }	
            }
        }
    }
  
    return 0;
    
error:
    v4l2_buf->bytesused = 0;
    if(qbuf_en && qbuf_flag == 0)
    {
        qbuf_flag = 1;
        ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
        if(ret != 0)
        {
            ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
            return -1;
        }	
    }
	return -1;
}


int uvc_dma_memcpy(
	struct uvc *uvc,
	unsigned long long phyDst, 
	unsigned long long phySrc,
	int size)
{

	if(!uvc->uvc_dam_memcpy_callback)
		return -1;

	if(size < uvc->uvc_dam_align)
		return 0;

	if(uvc->uvc_dam_align > 1)
		size &= (0xFFFFFFFF - (uvc->uvc_dam_align - 1));

	uvc->uvc_dam_memcpy_callback(phyDst, phySrc, size);
	return size;
}

int uvc_dma_memcpy_and_qbuf(struct uvc_dev *dev, int32_t venc_chn, 
    struct v4l2_buffer *v4l2_buf,  unsigned long long uvc_phyAddr, void *uvc_pVirAddr,
    unsigned long long send_data_phyAddr, void *send_data_pVirAddr, 
	uint32_t send_size, int qbuf_en, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used)
{
    int ret = 0;
	int s32Ret;
    uint32_t offset = 0;
    int qbuf_flag = 0;
	uint32_t r_4k,n_4k;
	uint32_t qbuf_transfer_size = 0;

	if(uvc_phyAddr == 0 || send_data_phyAddr == 0 || send_size == 0 )
	{
		ucam_error("data error");
		goto error;
	}

	//r_4k = send_size & 0x00000FFF;
	//n_4k = send_size & 0xFFFFF000;
	r_4k = send_size & ((dev->uvc->uvc_dam_align - 1));
	n_4k = send_size & (0xFFFFFFFF - (dev->uvc->uvc_dam_align - 1));

	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
	{
		if(n_4k > 0)
		{
			s32Ret = uvc_dma_memcpy(dev->uvc, uvc_phyAddr + offset, 
				send_data_phyAddr + offset, n_4k);
			if(s32Ret != n_4k)
			{
				ucam_error("memcpy error, ret:0x%x",s32Ret);
				goto error;
			}
			reqbufs_buf_used->buf_used += n_4k;
			offset += n_4k;
		}
		if(r_4k > 0)
		{
			uvc_memcpy(uvc_pVirAddr + offset, send_data_pVirAddr + offset, r_4k);
			reqbufs_buf_used->buf_used += r_4k;
		}
#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error;
		}
		if(qbuf_en && qbuf_flag == 0)
		{
			qbuf_flag = 1;
			ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				return -1;
			}
		}
		return ret;	
	}
	else
	{
		qbuf_transfer_size = dev->PayloadTransferSize << 5;
		if(qbuf_transfer_size > 512*1024)
		{
			qbuf_transfer_size = 512*1024;
		}
		qbuf_transfer_size = ALIGN_DOWN(qbuf_transfer_size,dev->uvc->uvc_dam_align);
		if(qbuf_transfer_size < dev->uvc->uvc_dam_align)
		{
			qbuf_transfer_size = dev->uvc->uvc_dam_align;
		}
	}
	


	if(n_4k > 0)
	{
		while(n_4k)
		{
			if(n_4k <= qbuf_transfer_size)
			{
				//uvc_memcpy(uvc_mem + offset, send_data + offset, n_4k);
				s32Ret = uvc_dma_memcpy(dev->uvc, uvc_phyAddr + offset, 
					send_data_phyAddr + offset, n_4k);
				if(s32Ret != n_4k)
				{
					ucam_error("memcpy error, ret:0x%x",s32Ret);
					goto error;
				}
				reqbufs_buf_used->buf_used += n_4k;
				offset += n_4k;
				n_4k = 0;
#if REQBUFS_CACHE_ENABLE == 1
				uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error;
				}

				if(qbuf_en && qbuf_flag == 0)
				{
					qbuf_flag = 1;
					ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						return -1;
					}	
				}

				break;
			}
			else
			{
				//uvc_memcpy(uvc_mem + offset, send_data + offset, qbuf_transfer_size);
				ret = uvc_dma_memcpy(dev->uvc, uvc_phyAddr + offset, 
					send_data_phyAddr + offset, qbuf_transfer_size);
				if(ret != qbuf_transfer_size)
				{
					ucam_error("memcpy error, ret:0x%x",ret);
					goto error;
				}
				n_4k -= qbuf_transfer_size;
				offset += qbuf_transfer_size;
				reqbufs_buf_used->buf_used += qbuf_transfer_size;
#if REQBUFS_CACHE_ENABLE == 1
				uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif			
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error;
				}

				if(qbuf_en && qbuf_flag == 0)
				{
					qbuf_flag = 1;
					ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						return -1;
					}	
				}
			}
		}
	}


	if(r_4k > 0)
	{
		qbuf_transfer_size = r_4k;//dev->PayloadTransferSize << 3;
		while(r_4k)
		{
			if(r_4k <= qbuf_transfer_size)
			{
				uvc_memcpy(uvc_pVirAddr + offset, send_data_pVirAddr + offset, r_4k);
				reqbufs_buf_used->buf_used += r_4k;
				offset += n_4k;
				r_4k = 0;
#if REQBUFS_CACHE_ENABLE == 1
				uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error;
				}

				if(qbuf_en && qbuf_flag == 0)
				{
					qbuf_flag = 1;
					ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						return -1;
					}	
				}
				break;
			}
			else
			{
				uvc_memcpy(uvc_pVirAddr + offset, send_data_pVirAddr + offset, qbuf_transfer_size);
				r_4k -= qbuf_transfer_size;
				offset += qbuf_transfer_size;
				reqbufs_buf_used->buf_used += qbuf_transfer_size;
#if REQBUFS_CACHE_ENABLE == 1
				uvc_video_reqbufs_flush_cache(dev, v4l2_buf->index);
#endif			

				ret = uvc_ioctl_set_reqbufs_buf_used(dev, reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error;
				}

				if(qbuf_en && qbuf_flag == 0)
				{
					qbuf_flag = 1;
					ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						return -1;
					}	
				}
			}
		}
	}

    return ret;
    
error:
    v4l2_buf->bytesused = 0;
    if(qbuf_en && qbuf_flag == 0)
    {
        qbuf_flag = 1;
        ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
        if(ret != 0)
        {
            ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
            return -1;
        }	
    }
	return ret;
}

int uvc_send_stream(struct uvc_dev *dev, int32_t venc_chn, 
	const void *buf, uint32_t buf_size, uint64_t u64Pts)
{
	fd_set fds;
	int ret;
    struct v4l2_buffer v4l2_buf;
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api
	int usb_link_state;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;


#if 0
	if(uvc == NULL)
	{
		return -ENOMEM;
	}

	if(uvc_stream_num >= uvc->config.uvc_video_stream_max)
		return -1;

	dev = uvc->uvc_dev[uvc_stream_num];
#endif
	if(dev == NULL || buf == NULL)
	{
		ucam_error("buf error");
		return -ENOMEM;
	}

	if(dev->uvc_stream_on == 0)
	{
		ucam_error("stream off");
		return -EALREADY;
	}

	if(dev->uvc->usb_connet_status == 0 || dev->uvc->usb_suspend)
	{
		ucam_error("disconnet");
		return -EMLINK;
	}

	ret = uvc_api_get_stream_status(dev, venc_chn, &stream_status);
	if(ret != 0 || stream_status == 0)
	{
		ucam_error("disconnet");
		return -1;
	}

	pthread_mutex_lock(&dev->uvc_reqbufs_mutex);

	FD_ZERO(&fds);
	FD_SET(dev->fd, &fds);
	struct timeval to;
	while( dev->uvc_stream_on){
		fd_set wfds = fds;
		to.tv_sec = 0; to.tv_usec = UVC_SEND_SELECT_TIMEOUT;
		ret = select(dev->fd + 1, NULL, &wfds, NULL, &to);
		if ( ret == - 1 ){
			ucam_trace("select fail!");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			return -1;
		}
		else if(ret == 0)
		{
			ucam_error("select timeout\n");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			ret = uvc_ioctl_get_usb_link_state(dev , &usb_link_state);
			if(ret == 0 && usb_link_state != 0)
			{
				ucam_error("usb_link_state:%d\n",usb_link_state);
        		//uvc_streamoff_all(dev);
				// if(dev->uvc_stream_on)
				// {
				// 	//uvc_video_stream(dev, 0);
				// 	dev->uvc->usb_connet_status = 0;
				// }
				return -EMLINK;
			}	
			return -1;
		}

		if (FD_ISSET(dev->fd , &wfds)){
			//ucam_trace("enter");
				
			memset(&v4l2_buf, 0, sizeof v4l2_buf);
			v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			v4l2_buf.memory = dev->config.reqbufs_memory_type;

			if(dev->uvc_stream_on == 0){
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -EALREADY;	
			}

			if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &v4l2_buf)) < 0) {
				ucam_error("Unable to dequeue buffer: %s (%d).", strerror(errno),
					errno);
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -1;
			}
			
			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				v4l2_buf.m.userptr = (unsigned long)dev->mem[v4l2_buf.index];
				v4l2_buf.length = dev->video_reqbufs_length[v4l2_buf.index];
			}

			if(buf_size > v4l2_buf.length)
			{
				ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", buf_size, v4l2_buf.length);
				v4l2_buf.bytesused = 0;
				goto error_qbuf;
			}

			if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && dev->stream_num && dev->config.uvc150_simulcast_en && dev->uvc150_simulcast_streamon)
			{
				if(get_simulcast_start_or_stop_layer(dev, venc_chn) != 0)
				{
					((uint32_t *)(dev->mem[v4l2_buf.index] + v4l2_buf.bytesused + dev->reqbufs_config.data_offset))[0] = venc_chn_to_simulcast_uvc_header_id(dev, venc_chn);
				}
			}

            v4l2_buf.bytesused = buf_size;
			uvc_us_to_timeval(u64Pts, &v4l2_buf.timestamp);
            
			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				reqbufs_buf_used.index = v4l2_buf.index;
				reqbufs_buf_used.buf_used = 0;
				ret = uvc_memcpy_and_qbuf(dev, venc_chn, &v4l2_buf, 
					dev->mem[v4l2_buf.index] + dev->reqbufs_config.data_offset, 
					buf, buf_size, 1, &reqbufs_buf_used);
					
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return ret;
			}
			else
			{
				uvc_memcpy(dev->mem[v4l2_buf.index], buf, buf_size);
				if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &v4l2_buf)) < 0) {
					ucam_error("Unable to requeue buffer: %s (%d).", strerror(errno),
						errno);
					ucam_error("bytesused = %d\n",v4l2_buf.bytesused);
					pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
					return -1;
				}
				//return 0;
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return 0;
			}							
		}
		else{
		}
		//usleep(1000);
	}


	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	return 0;

error_qbuf:
    v4l2_buf.bytesused = 0;

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);
	}

    if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &v4l2_buf)) < 0) {
        ucam_error("Unable to requeue buffer: %s (%d).", strerror(errno),
            errno);
        ucam_error("bytesused = %d\n",v4l2_buf.bytesused);
        pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
        return -1;
    }
    //return 0;
    pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
    return -1;

}

int uvc_send_stream_extern_cp(struct uvc_dev *dev, int32_t venc_chn, 
	const void *handle,
	int (*extern_cp)(const void *handle, struct uvc_dev *dev, void *uvc_buf, struct v4l2_buffer *v4l2_buf),
	uint64_t u64Pts)
{
	fd_set fds;
	int ret;
    struct v4l2_buffer v4l2_buf;
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api
	int usb_link_state;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	void *uvc_mem;


	if(dev == NULL || handle == NULL)
	{
		return -ENOMEM;
	}

	if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	if(dev->uvc->usb_connet_status == 0 || dev->uvc->usb_suspend)
	{
		return -EMLINK;
	}

	ret = uvc_api_get_stream_status(dev, venc_chn, &stream_status);
	if(ret != 0 || stream_status == 0)
	{
		return -1;
	}

	pthread_mutex_lock(&dev->uvc_reqbufs_mutex);

	FD_ZERO(&fds);
	FD_SET(dev->fd, &fds);
	struct timeval to;
	while( dev->uvc_stream_on){
		fd_set wfds = fds;
		to.tv_sec = 0; to.tv_usec = UVC_SEND_SELECT_TIMEOUT;
		ret = select(dev->fd + 1, NULL, &wfds, NULL, &to);
		if ( ret == - 1 ){
			ucam_trace("select fail!");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			return -1;
		}
		else if(ret == 0)
		{
			ucam_error("select timeout\n");
			pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
			ret = uvc_ioctl_get_usb_link_state(dev , &usb_link_state);
			if(ret == 0 && usb_link_state != 0)
			{
				ucam_error("usb_link_state:%d\n",usb_link_state);
        		//uvc_streamoff_all(dev);
				//if(dev->uvc_stream_on)
				//{
				//	//uvc_video_stream(dev, 0);
				//	dev->uvc->usb_connet_status = 0;
				//}
				return -EMLINK;
			}	
			return -1;
		}

		if (FD_ISSET(dev->fd , &wfds)){
			//ucam_trace("enter");
				
			memset(&v4l2_buf, 0, sizeof v4l2_buf);
			v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			v4l2_buf.memory = dev->config.reqbufs_memory_type;

			if(dev->uvc_stream_on == 0){
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -EALREADY;	
			}

			if ((ret = ioctl(dev->fd, VIDIOC_DQBUF, &v4l2_buf)) < 0) {
				ucam_error("Unable to dequeue buffer: %s (%d).", strerror(errno),
					errno);
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return -1;
			}
			
			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				v4l2_buf.m.userptr = (unsigned long)dev->mem[v4l2_buf.index];
				v4l2_buf.length = dev->video_reqbufs_length[v4l2_buf.index];
			}


            
			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				uvc_mem = dev->mem[v4l2_buf.index] + dev->reqbufs_config.data_offset;
			}
			else
			{
				uvc_mem = dev->mem[v4l2_buf.index];
				
			}

			uvc_us_to_timeval(u64Pts, &v4l2_buf.timestamp);

			if(extern_cp(handle, dev, uvc_mem, &v4l2_buf) == 0)
			{
				if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
				{
					reqbufs_buf_used.index = v4l2_buf.index;
					reqbufs_buf_used.buf_used = v4l2_buf.bytesused;
					ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
					if(ret != 0)
					{
						ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
						goto error_qbuf;
					}
				}
				if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &v4l2_buf)) < 0) {
					ucam_error("Unable to requeue buffer: %s (%d).", strerror(errno),
						errno);
					ucam_error("bytesused = %d\n",v4l2_buf.bytesused);
					pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
					return -1;
				}
				//return 0;
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return 0;
			}
			else
			{
				ucam_error("extern_cp error");
				goto error_qbuf;
			}	
		}
		else{
		}
		//usleep(1000);
	}


	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	return 0;

error_qbuf:
    v4l2_buf.bytesused = 0;

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);
	}

    if ((ret = ioctl(dev->fd, VIDIOC_QBUF, &v4l2_buf)) < 0) {
        ucam_error("Unable to requeue buffer: %s (%d).", strerror(errno),
            errno);
        ucam_error("bytesused = %d\n",v4l2_buf.bytesused);
        pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
        return -1;
    }
    //return 0;
    pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
    return -1;

}

int set_uvc_send_stream_ops(struct uvc_dev *dev, 
	struct uvc_send_stream_ops *send_stream_ops)
{
	if(send_stream_ops == NULL)
	{
		return -ENOMEM;
	}
	memcpy(&dev->send_stream_ops, send_stream_ops, sizeof(struct uvc_send_stream_ops));
	
	return 0;
}


int uvc_send_stream_encode(struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc, VENC_DATA_S_DEFINE* pVStreamData)
{
	if(dev->send_stream_ops.send_encode)
		return dev->send_stream_ops.send_encode(dev, venc_chn, fcc, (void *)pVStreamData);
	else
		return -ENOMEM;
}

int uvc_send_stream_yuy2(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf)
{
	if(dev->send_stream_ops.send_yuy2)
		return dev->send_stream_ops.send_yuy2(dev, (void *)pVBuf);
	else
		return -ENOMEM;	
}


int uvc_send_stream_nv12(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf)
{
	if(dev->send_stream_ops.send_nv12)
		return dev->send_stream_ops.send_nv12(dev, (void *)pVBuf);
	else
		return -ENOMEM;
}

int uvc_send_stream_i420(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf)
{
	if(dev->send_stream_ops.send_i420)
		return dev->send_stream_ops.send_i420(dev, (void *)pVBuf);
	else
		return -ENOMEM;
}

int uvc_send_stream_l8_ir(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf, void *pMetadata)
{
	if(dev->send_stream_ops.send_l8_ir)
		return dev->send_stream_ops.send_l8_ir(dev, (void *)pVBuf, pMetadata);
	else
		return -ENOMEM;
}

#if !(defined HISI_SDK || defined GK_SDK || defined SIGMASTAR_SDK || defined AMBARELLA_SDK)
int uvc_send_stream_ops_int(struct uvc_dev *dev)
{
	//暂时不支持
	return 0;
}
#endif

int uvc_us_to_timeval(const uint64_t usec, struct timeval *timestamp)
{
	if(timestamp == NULL)
	{
		return -1;
	}
	timestamp->tv_sec = usec / 1000000L;
	timestamp->tv_usec = usec % 1000000L;
	
    return 0; 
}
