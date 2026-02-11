/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2025. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2024-02-26 15:20:00
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream_common.c
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
#include <ucam/uvc/uvc_stream_common.h>
#include "uvc_stream.h"
#include "uvc_stream_yuv.h"

static void* uvc_comm_mmap(unsigned long long phyAddr, uint32_t size, int cache)
{
    void *pVirtualAddress;
    int fd = -1;
	int flags = O_RDWR;

	if(!cache)
	{
		flags |= O_SYNC;
	}

    fd = open("/dev/mem", flags);
    if (fd < 0)
    {
        ucam_error("Open dev/mem error");
        return NULL;
    }

    pVirtualAddress = mmap ((void *)0, size, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, fd, phyAddr);
    if (NULL == pVirtualAddress )
    {
        ucam_error("mmap error");
        close(fd);
        return NULL;
    }
    close(fd);

    return pVirtualAddress;
}

static int uvc_common_munmap(void *pVirAddr, uint32_t size)
{
    return munmap(pVirAddr, size);
}


static int uvc_common_memcpy_and_qbuf(struct uvc_dev *dev, int32_t venc_chn, 
    struct v4l2_buffer *v4l2_buf,  unsigned long long uvc_phyAddr, void *uvc_pVirAddr,
    unsigned long long send_data_phyAddr, void *send_data_pVirAddr, 
	uint32_t send_size, int qbuf_en, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used,
	int dma_en)
{
	int ret = 0;

	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		if (dma_en && dev->uvc->uvc_dam_memcpy_callback && 
			dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			return uvc_dma_memcpy_and_qbuf(dev, venc_chn, 
					v4l2_buf,  uvc_phyAddr, uvc_pVirAddr,
					send_data_phyAddr, send_data_pVirAddr, 
					send_size, qbuf_en, reqbufs_buf_used);
		}
		else
		{
			return uvc_memcpy_and_qbuf(dev, venc_chn, 
					v4l2_buf, uvc_pVirAddr, 
					send_data_pVirAddr, send_size, qbuf_en, reqbufs_buf_used);
		}
	}
	else
	{
		uvc_memcpy(uvc_pVirAddr, send_data_pVirAddr, send_size);
		if(qbuf_en)
		{
			ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto error;
			}
		}
	}

	return ret;

error:
    if(qbuf_en)
    {
		v4l2_buf->bytesused = 0;
        ret = uvc_video_qbuf_nolock(dev, venc_chn, v4l2_buf);
        if(ret != 0)
        {
            ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
            return -1;
        }	
    }
	return -1;
}

static int send_encode(
    struct uvc_dev *dev, 
    int32_t venc_chn, 
    uint32_t fcc, 
    UVC_VENC_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	UVC_VENC_Pack_t *pstPack;
	uint8_t *pack_buf;
	uint32_t pack_buf_len;
	uint32_t i;

	int qbuf_flag = 1;

	//uint16_t uvc_h264_profile;
	uint8_t* naluData = NULL;
	uint8_t nalu_type = 0;

	if(fcc != data->u32Format)
	{
		if(fcc && data->u32Format)
		{
			ucam_error("uvc format error, uvc format:%s, send format:%s",
				uvc_video_format_string(fcc), uvc_video_format_string(data->u32Format));
			return -1;
		}
	}


	switch (data->u32Format)
	{
		case V4L2_PIX_FMT_H264:
#if  0
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
				if (uvc_api_get_h264_profile (dev, venc_chn, &uvc_h264_profile) != 0)
				{
					return -1;
				}

				if(dev->H264SVCEnable == 1 || 
					uvc_h264_profile == UVCX_H264_wProfile_SCALABLEBASELINEPROFILE || 
					uvc_h264_profile == UVCX_H264_wProfile_SCALABLEHIGHPROFILE)
				{
#ifdef	SVC_LAYERS_DISCARD //三层丢一层变两层SVC
					//三层丢一层变两层SVC
					if(!(E_MI_VENC_BASE_IDR == data->stH264Info.enRefType ||   
						E_MI_VENC_BASE_P_REFBYBASE == data->stH264Info.enRefType ||
						E_MI_VENC_BASE_P_REFBYENHANCE == data->stH264Info.enRefType))
					{
						//windows skype for business只支持2层SVC
						return 0;
					}
#endif
					if(dev->first_IDR_flag)
					{

						pstPack = &data->pstPack[0];
						naluData = (uint8_t*)(pstPack->pu8Addr + pstPack->u32Offset);
						nalu_type = naluData[4] & 0x1f;
						if(nalu_type != 0x07)
						{
							uvc_api_set_h264_frame_requests(dev, venc_chn, UVCX_H264_PICTURE_TYPE_IDRFRAME);
							return 0;
						}
						dev->first_IDR_flag = 0;
					}

					if(dev->uvc->h264_svc_buf[dev->stream_num] == NULL)
					{

						if(dev->config.resolution_width_max > 1920 && dev->config.resolution_height_max > 1080)
						{
							//4K
							dev->uvc->h264_svc_buf_len[dev->stream_num] = 2.5 * 1024 *1024;
						}
						else
						{
							dev->uvc->h264_svc_buf_len[dev->stream_num] = 1.5 * 1024 *1024;
						}
						dev->uvc->h264_svc_buf[dev->stream_num] = malloc(dev->uvc->h264_svc_buf_len[dev->stream_num]);
						if (dev->uvc->h264_svc_buf[dev->stream_num] == NULL) {
							ucam_error("Malloc Error!");
							return -1;
						}
					}

					ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
					if(ret != 0)
					{
						ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
						return ret;
					}

					v4l2_buf.bytesused = 0;
					for(i = 0;i < data->u32PackCount; i++)
					{
						v4l2_buf.bytesused += data->pstPack[i].u32Len - data->pstPack[i].u32Offset;
					}

					if(v4l2_buf.bytesused  > v4l2_buf.length)
					{
						ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
						ret = -1;
						goto error_release;
					}
					
					v4l2_buf.bytesused = 0;
					for (i = 0; i < data->u32PackCount; i++) {				
						pstPack = &data->pstPack[i];
						pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
						pack_buf_len = pstPack->u32Len - pstPack->u32Offset;

#if 0   //由上层发送来过滤
						//去掉SEI
						if(dev->uvc_h264_sei_en == 0)
						{
							nalu_type = pack_buf[4] & 0x1f;
							if (nalu_type == 0x06) // skip sei
							{
								continue;
							}

						}
#endif
						if((v4l2_buf.bytesused + pack_buf_len) < v4l2_buf.length)
						{
							if((v4l2_buf.bytesused + pack_buf_len) > dev->uvc->h264_svc_buf_len[dev->stream_num])
							{
								ucam_error("buf_length_error ! h264_svc_buf_len:%d, pack_buf_len:%d", 
									dev->uvc->h264_svc_buf_len[dev->stream_num],
									v4l2_buf.bytesused + pack_buf_len);	
								ret = -1;
								goto error_release;
							}
							uvc_memcpy(dev->uvc->h264_svc_buf[dev->stream_num] + v4l2_buf.bytesused, pack_buf, pack_buf_len);
							v4l2_buf.bytesused += pack_buf_len;
						}
						else
						{
							ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
						}	
					}

					if(dev->bConfigurationValue == 2 || dev->h264_svc_layer == 2)//uvc1.5
						v4l2_buf.bytesused = uvc_nalu_recode_svc_layers_3_to_2(dev->uvc->h264_svc_buf[dev->stream_num], v4l2_buf.bytesused, uvc_mem);
					else
						v4l2_buf.bytesused = uvc_nalu_recode_svc(dev->uvc->h264_svc_buf[dev->stream_num], v4l2_buf.bytesused, uvc_mem);
					
				}
			}
			else
#else
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
#endif
			{
				if(dev->first_IDR_flag)
				{
					pstPack = &data->pstPack[0];
					naluData = (uint8_t *)(pstPack->pu8Addr + pstPack->u32Offset);

					nalu_type = naluData[4] & 0x1f;
					if(nalu_type != 0x07)
					{
						uvc_api_set_h264_frame_requests(dev, venc_chn, UVCX_H264_PICTURE_TYPE_IDRFRAME);
						return 0;
					}
					dev->first_IDR_flag = 0;
				}

				ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
				if(ret != 0)
				{
					ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
					return ret;
				}	
				uvc_us_to_timeval(data->pstPack[0].u64PTS, &v4l2_buf.timestamp);

				v4l2_buf.bytesused = 0;
				for (i = 0; i < data->u32PackCount; i++) {
					pstPack = &data->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;

#if 0   //由上层发送来过滤
					//去掉SEI
					if(dev->uvc_h264_sei_en == 0)
					{
						nalu_type = pack_buf[4] & 0x1f;
						if (nalu_type == 0x06) // skip sei
						{
							continue;
						}

					}
#endif

					if((v4l2_buf.bytesused + pack_buf_len) < v4l2_buf.length)
					{
						uvc_memcpy(uvc_mem + v4l2_buf.bytesused, pack_buf, pack_buf_len);
						v4l2_buf.bytesused += pack_buf_len;
					}
					else
					{
						ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
					}	
				}				
			}

			ret = uvc_video_qbuf_lock(dev, venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				return ret;
			}
			return ret;
			//break;
		case V4L2_PIX_FMT_H265:
			if(dev->first_IDR_flag)
			{
				pstPack = &data->pstPack[0];
				naluData = (uint8_t *)(pstPack->pu8Addr + pstPack->u32Offset);				
				nalu_type = (naluData[4]>>1) & 0x3f;
				if(nalu_type != 32)
				{
					uvc_api_set_h264_frame_requests(dev, venc_chn, UVCX_H264_PICTURE_TYPE_IDRFRAME);
					return 0;
				}
				dev->first_IDR_flag = 0;
			}
			ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
			if(ret != 0)
			{
				ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
				return ret;
			}

			uvc_us_to_timeval(data->pstPack[0].u64PTS, &v4l2_buf.timestamp);
			v4l2_buf.bytesused = 0;
			for(i = 0;i < data->u32PackCount; i++)
			{
				v4l2_buf.bytesused += data->pstPack[i].u32Len - data->pstPack[i].u32Offset;
			}

			if(v4l2_buf.bytesused  > v4l2_buf.length)
            {
                ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
				ret = -1;
                goto error_release;
            }

			v4l2_buf.bytesused = 0;
			for (i = 0; i < data->u32PackCount; i++) {
				pstPack = &data->pstPack[i];
				pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
				pack_buf_len = pstPack->u32Len - pstPack->u32Offset;

#if 0   //由上层发送来过滤
				//去掉SEI
				if(dev->uvc_h264_sei_en == 0)
				{
					nalu_type = (pack_buf[4] & 0x7e)>>1;
					if (nalu_type == 39) { // skip sei
						continue;
					}
				}	
#endif
				if((v4l2_buf.bytesused + pack_buf_len) < v4l2_buf.length)
				{
					uvc_memcpy(uvc_mem + v4l2_buf.bytesused, pack_buf, pack_buf_len);
					v4l2_buf.bytesused += pack_buf_len;
				}
				else
				{
					ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
				}	
			}
			
			ret = uvc_video_qbuf_lock(dev, venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				return ret;
			}
			return ret;
			//break;
			
		case V4L2_PIX_FMT_MJPEG:
		default:
			ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
			if(ret != 0)
			{
				ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
				return ret;
			}
			uvc_us_to_timeval(data->pstPack[0].u64PTS, &v4l2_buf.timestamp);
  
			v4l2_buf.bytesused = 0;
			for(i = 0;i < data->u32PackCount; i++)
			{
				v4l2_buf.bytesused += data->pstPack[i].u32Len - data->pstPack[i].u32Offset;
			}

			if(v4l2_buf.bytesused  > v4l2_buf.length)
            {
                ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length); 
				ret = -1; 
                goto error_release;
            }

			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				unsigned long long phyAddr;
				struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
				phyAddr = dev->video_reqbufs_addr[v4l2_buf.index] + dev->reqbufs_config.data_offset;
				reqbufs_buf_used.index = v4l2_buf.index;
				reqbufs_buf_used.buf_used = 0;
				if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
				{
					qbuf_flag = 0;
				}
				for(i = 0;i < data->u32PackCount; i++)
				{
					if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
					{
						if((i + 1) == data->u32PackCount)
						{
							qbuf_flag = 1;
						}
						
					}
					pack_buf_len = data->pstPack[i].u32Len - data->pstPack[i].u32Offset;
					ret = uvc_common_memcpy_and_qbuf(dev, venc_chn, &v4l2_buf, phyAddr, uvc_mem,  
						data->pstPack[i].phyAddr + data->pstPack[i].u32Offset,
						data->pstPack[i].pu8Addr + data->pstPack[i].u32Offset,
						pack_buf_len, qbuf_flag, &reqbufs_buf_used, 1);
					if(ret != 0)
					{
						break;
					}
					phyAddr += pack_buf_len;
					uvc_mem += pack_buf_len;
					if(qbuf_flag)
					{
						qbuf_flag = 0;
					}
				}
				pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
				return 0;
			}
			else
			{
				for(i = 0;i < data->u32PackCount; i++)
				{
					pack_buf_len = data->pstPack[i].u32Len - data->pstPack[i].u32Offset;
					uvc_memcpy(uvc_mem,data->pstPack[i].pu8Addr + data->pstPack[i].u32Offset, 
						pack_buf_len);
					uvc_mem += pack_buf_len;
				}
				
				ret = uvc_video_qbuf_lock(dev, venc_chn, &v4l2_buf);
				if(ret != 0)
				{
					ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
					return -1;
				}	
				return ret;
			}
			break;
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

static int send_raw_yuy2_yuy2(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr = 0;
	void *pYUY2 = NULL;
	uint32_t YUY2_size;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int dma_en = 0;
	int cache_en = 0;
	uint32_t dst_stride_yuy2 = dev->width << 1;
	YUY2_size = data->stFrameData.u32Height * data->stFrameData.u32Stride[0];
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
			dev->reqbufs_config.data_offset;
		if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			dma_en = 1;
		}
	}

	if (data->stFrameData.u32Stride[0] != dst_stride_yuy2 || 
		data->stFrameData.u32Width != dev->width)
	{
		ucam_debug("u32Stride %d != u32RowSize %d", 
			data->stFrameData.u32Stride[0], dst_stride_yuy2);
		dma_en = 0;
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		cache_en = 1;
	}
	
	v4l2_buf.bytesused = (dev->width * dev->height) << 1;
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;

	if(cache_en)
	{
		pYUY2 = uvc_comm_mmap(data->stFrameData.phyAddr[0], YUY2_size, 1);
		if(!pYUY2)
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}
	}
	else
	{
		pYUY2 = data->stFrameData.pVirAddr[0];
	}

	unsigned long long src_yuy2_phy = data->stFrameData.phyAddr[0];
	const uint8_t* src_yuy2 = pYUY2;
	int width_offset = 0;
	int height_offset = 0;
	uint32_t src_yuy2_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_yuy2_offset = (width_offset << 1) + height_offset * data->stFrameData.u32Stride[0];
		src_yuy2 += src_yuy2_offset;
		src_yuy2_phy += src_yuy2_offset;
	}

	if(dma_en)
	{
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
				uvc_phy_addr, uvc_mem,
				src_yuy2_phy, (void *)src_yuy2,
				v4l2_buf.bytesused, 1, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto done;
		}
	}
	else
	{
		int y;
		int set_reqbufs_buf_used = 0;
		if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
		{
			if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
				set_reqbufs_buf_used = 1;
			else
				set_reqbufs_buf_used = 2;
		}

		for (y = 0; y < dev->height; y++) 
		{
			uvc_memcpy_neon(uvc_mem, src_yuy2, dst_stride_yuy2);
			src_yuy2 += data->stFrameData.u32Stride[0];
			uvc_mem += dst_stride_yuy2;
			reqbufs_buf_used.buf_used += dst_stride_yuy2;
			if (set_reqbufs_buf_used == 1)
			{
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error_qbuf;
				}

				if(y == 0)
				{	
					ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						goto done;
					}
				}
			}
		}

		if (set_reqbufs_buf_used == 2)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error_qbuf;
			}
		
			ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto done;
			}	
		}
		else if (set_reqbufs_buf_used == 0)
		{
			ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto done;
			}
		}
	}


done:
	if(cache_en && pYUY2)
	{
		uvc_common_munmap(pYUY2, YUY2_size);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en && pYUY2)
	{
		uvc_common_munmap(pYUY2, YUY2_size);
	}

	v4l2_buf.bytesused = 0;
	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}
	return -1;
}

static int send_raw_nv12_yuy2(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	int y;
	const uint8_t* src_y;
	int src_stride_y;
	const uint8_t* src_vu;
	int src_stride_vu;
	uint8_t* dst_yuy2;
	int dst_stride_yuy2;
	uint32_t temp = 0;

	void *mmap_virt_addr[2] = {NULL, NULL};
	uint32_t mmap_virt_size[2];

	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int set_reqbufs_buf_used = 0;
	int qbuf_flag = 0;
	int cache_en = 0;

	v4l2_buf.bytesused = (dev->width * dev->height) << 1;
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Width;
	mmap_virt_size[1] = mmap_virt_size[0] >> 1;

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
			set_reqbufs_buf_used = 1;
		else
			set_reqbufs_buf_used = 2;
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		//YUV转换只能使用CPU，不支持DMA
		cache_en = 1;
	}
	
	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_vu = mmap_virt_addr[1];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_vu = data->stFrameData.pVirAddr[1];
	}

	dst_yuy2 = uvc_mem;
	dst_stride_yuy2 = dev->width * 2;
	src_stride_y = data->stFrameData.u32Stride[0];
	src_stride_vu = data->stFrameData.u32Stride[1];

	int width_offset = 0;
	int height_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y += width_offset + height_offset * src_stride_y;
		src_vu += (width_offset) + ((height_offset * src_stride_vu) >> 1);
	}

	if (!src_y || !src_vu || !dst_yuy2 || data->stFrameData.u32Width  <= 0 || data->stFrameData.u32Height <= 0) {

		ucam_error("error!!!, %p,%p,%p,%d,%d",src_y,src_vu,dst_yuy2,data->stFrameData.u32Width,data->stFrameData.u32Height);
		goto error_qbuf;
	} 

	void (*UVC_NV12ToYUY2Row)(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) = UVC_NV12ToYUY2Row_Any_NEON;
						

	if(IS_ALIGNED(dev->width, 16))
		UVC_NV12ToYUY2Row = UVC_NV12ToYUY2Row_NEON_16;


	for (y = 0; y < dev->height - 1; y += 2) 
	{
		UVC_NV12ToYUY2Row(src_y, src_vu, 
				dst_yuy2, dev->width);

		UVC_NV12ToYUY2Row(src_y + src_stride_y, src_vu, 
				dst_yuy2 + dst_stride_yuy2, dev->width);
						
		src_y += src_stride_y << 1;
		src_vu += src_stride_vu;
		temp = dst_stride_yuy2 << 1;
		dst_yuy2 += temp;
		reqbufs_buf_used.buf_used += temp;
		
#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		if (set_reqbufs_buf_used == 1)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error_qbuf;
			}

			if(y == 0)
			{	
				qbuf_flag = 1;
				ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
				if(ret != 0)
				{
					ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
					goto done;
				}
			}
		}	
	}

	if (set_reqbufs_buf_used == 2)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_qbuf;
		}

		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}	
	}
	else if(qbuf_flag == 0)
	{	
		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}
	}

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}

static int send_raw_nv21_yuy2(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	int y;
	const uint8_t* src_y;
	int src_stride_y;
	const uint8_t* src_vu;
	int src_stride_vu;
	uint8_t* dst_yuy2;
	int dst_stride_yuy2;
	uint32_t temp = 0;

	void *mmap_virt_addr[2] = {NULL, NULL};
	uint32_t mmap_virt_size[2];

	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int set_reqbufs_buf_used = 0;
	int qbuf_flag = 0;
	int cache_en = 0;

	v4l2_buf.bytesused = (dev->width * dev->height) << 1;
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Width;
	mmap_virt_size[1] = mmap_virt_size[0] >> 1;

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
			set_reqbufs_buf_used = 1;
		else
			set_reqbufs_buf_used = 2;
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		//YUV转换只能使用CPU，不支持DMA
		cache_en = 1;
	}
	
	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_vu = mmap_virt_addr[1];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_vu = data->stFrameData.pVirAddr[1];
	}

	dst_yuy2 = uvc_mem;
	dst_stride_yuy2 = dev->width * 2;
	src_stride_y = data->stFrameData.u32Stride[0];
	src_stride_vu = data->stFrameData.u32Stride[1];

	int width_offset = 0;
	int height_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y += width_offset + height_offset * src_stride_y;
		src_vu += (width_offset) + ((height_offset * src_stride_vu) >> 1);
	}

	if (!src_y || !src_vu || !dst_yuy2 || data->stFrameData.u32Width  <= 0 || data->stFrameData.u32Height <= 0) {

		ucam_error("error!!!, %p,%p,%p,%d,%d",src_y,src_vu,dst_yuy2,data->stFrameData.u32Width,data->stFrameData.u32Height);
		goto error_qbuf;
	} 

	void (*UVC_NV21ToYUY2Row)(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) = UVC_NV21ToYUY2Row_Any_NEON;
						

	if(IS_ALIGNED(dev->width, 16))
		UVC_NV21ToYUY2Row = UVC_NV21ToYUY2Row_NEON_16;


	for (y = 0; y < dev->height - 1; y += 2) 
	{
		UVC_NV21ToYUY2Row(src_y, src_vu, 
				dst_yuy2, dev->width);

		UVC_NV21ToYUY2Row(src_y + src_stride_y, src_vu, 
				dst_yuy2 + dst_stride_yuy2, dev->width);
						
		src_y += src_stride_y << 1;
		src_vu += src_stride_vu;
		temp = dst_stride_yuy2 << 1;
		dst_yuy2 += temp;
		reqbufs_buf_used.buf_used += temp;
		
#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		if (set_reqbufs_buf_used == 1)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error_qbuf;
			}

			if(y == 0)
			{	
				qbuf_flag = 1;
				ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
				if(ret != 0)
				{
					ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
					goto done;
				}
			}
		}	
	}

	if (set_reqbufs_buf_used == 2)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_qbuf;
		}

		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}	
	}
	else if(qbuf_flag == 0)
	{	
		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}
	}

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}


static int send_raw_nv12_nv12(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr = 0;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int dma_en = 0;
	uint32_t row_size[2];
	int qbuf_flag = 0;

	int y;
	const uint8_t* src_y;
	const uint8_t* src_uv;
	int set_reqbufs_buf_used = 0;

	void *mmap_virt_addr[2] = {NULL, NULL};
	uint32_t mmap_virt_size[2];
	int cache_en = 0;
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
			dev->reqbufs_config.data_offset;
		
		if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			dma_en = 1;
		}
	}

	row_size[0] = dev->height * dev->width;
	row_size[1] = row_size[0] >> 1;

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Stride[0];
	mmap_virt_size[1] = data->stFrameData.u32Height/2 * data->stFrameData.u32Stride[1];
	v4l2_buf.bytesused = row_size[0] + row_size[1];
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);
	if (data->stFrameData.u32Stride[0] != dev->width ||
		data->stFrameData.u32Stride[1] != dev->width ||
		data->stFrameData.u32Width != dev->width)
	{
		ucam_debug("u32Stride [0-1]:%d, %d, u32Width %d", 
			data->stFrameData.u32Stride[0], 
			data->stFrameData.u32Stride[1],
			dev->width);
		dma_en = 0;
	}

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
			set_reqbufs_buf_used = 1;
		else
			set_reqbufs_buf_used = 2;
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		cache_en = 1;
	}

	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_uv = mmap_virt_addr[1];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_uv = data->stFrameData.pVirAddr[1];
	} 


	unsigned long long src_y_phy = data->stFrameData.phyAddr[0];
	unsigned long long src_uv_phy = data->stFrameData.phyAddr[1];
	int width_offset = 0;
	int height_offset = 0;
	uint32_t src_y_offset = 0;
	uint32_t src_uv_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y_offset = width_offset + height_offset * data->stFrameData.u32Stride[0];
		src_uv_offset = (width_offset) + ((height_offset * data->stFrameData.u32Stride[1]) >> 1);
		src_y += src_y_offset;
		src_y_phy += src_y_offset;
		src_uv += src_uv_offset;
		src_uv_phy += src_uv_offset;		
	}
	
	if(dma_en)
	{
		qbuf_flag = 0;
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr, uvc_mem,
			src_y_phy, (void *)src_y,
			row_size[0], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}

		uvc_mem += row_size[0];

		qbuf_flag = 1;
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr + row_size[0],
			uvc_mem,
			src_uv_phy, (void *)src_uv,
			row_size[1], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[1];
	}
	else
	{
		for (y = 0; y < dev->height; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_y, dev->width);
			src_y += data->stFrameData.u32Stride[0];
			uvc_mem += dev->width;
			reqbufs_buf_used.buf_used += dev->width;
			
	#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
	#endif	
			if (set_reqbufs_buf_used == 1)
			{
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error_qbuf;
				}

				if(y == 0)
				{	
					qbuf_flag = 1;
					ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						goto done;
					}
				}
			}	
		}

		for (y = 0; y < dev->height/2; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_uv, dev->width);
			src_uv += data->stFrameData.u32Stride[1];
			uvc_mem += dev->width;
			reqbufs_buf_used.buf_used += dev->width;

#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
			if (set_reqbufs_buf_used == 1)
			{
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error_qbuf;
				}
			}
		}

		if (set_reqbufs_buf_used == 2)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error_qbuf;
			}

			qbuf_flag = 1;
			ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto done;
			}	
		}
		else if(qbuf_flag == 0)
		{	
			qbuf_flag = 1;
			ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto done;
			}
		}
	}
	

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}


static int send_raw_nv21_nv12(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr = 0;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int dma_en = 0;
	uint32_t row_size[2];
	int qbuf_flag = 0;

	int y;
	const uint8_t* src_y;
	const uint8_t* src_uv;
	int set_reqbufs_buf_used = 0;

	void *mmap_virt_addr[2] = {NULL, NULL};
	uint32_t mmap_virt_size[2];
	int cache_en = 0;
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
			dev->reqbufs_config.data_offset;
		
		if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			dma_en = 1;
		}
	}

	row_size[0] = dev->height * dev->width;
	row_size[1] = row_size[0] >> 1;

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Stride[0];
	mmap_virt_size[1] = data->stFrameData.u32Height/2 * data->stFrameData.u32Stride[1];
	v4l2_buf.bytesused = row_size[0] + row_size[1];
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);
	if (data->stFrameData.u32Stride[0] != dev->width ||
		data->stFrameData.u32Stride[1] != dev->width ||
		data->stFrameData.u32Width != dev->width)
	{
		ucam_debug("u32Stride[0-1]:%d, %d, u32Width %d", 
			data->stFrameData.u32Stride[0], 
			data->stFrameData.u32Stride[1],
			dev->width);
		dma_en = 0;
	}

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
			set_reqbufs_buf_used = 1;
		else
			set_reqbufs_buf_used = 2;
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		cache_en = 1;
	}

	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_uv = mmap_virt_addr[1];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_uv = data->stFrameData.pVirAddr[1];
	}

	unsigned long long src_y_phy = data->stFrameData.phyAddr[0];
	int width_offset = 0;
	int height_offset = 0;
	uint32_t src_y_offset = 0;
	uint32_t src_uv_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y_offset = width_offset + height_offset * data->stFrameData.u32Stride[0];
		src_uv_offset = (width_offset) + ((height_offset * data->stFrameData.u32Stride[1]) >> 1);
		src_y += src_y_offset;
		src_y_phy += src_y_offset;
		src_uv += src_uv_offset; 
	}
	
	if(dma_en)
	{
		qbuf_flag = 0;
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr, uvc_mem,
			src_y_phy, (void *)src_y,
			row_size[0], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[0];
	}
	else
	{
		for (y = 0; y < dev->height; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_y, dev->width);
			src_y += data->stFrameData.u32Stride[0];
			uvc_mem += dev->width;
			reqbufs_buf_used.buf_used += dev->width;
			
	#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
	#endif	
			if (set_reqbufs_buf_used == 1)
			{
				ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
				if(ret != 0)
				{
					ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
					goto error_qbuf;
				}

				if(y == 0)
				{	
					qbuf_flag = 1;
					ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
					if(ret != 0)
					{
						ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
						goto done;
					}
				}
			}	
		}	
	}
	
	void (*UVSwapRow)(const uint8_t* src_uv, uint8_t* dst_vu, int width) = UVC_UVSwapRow_NEON_16; 
	if(IS_ALIGNED(dev->width, 16) == 0)
		UVSwapRow = UVC_UVSwapRow_Any_NEON;

	for (y = 0; y < dev->height/2; y ++) 
	{
		UVSwapRow(src_uv, uvc_mem, dev->width);
		src_uv += data->stFrameData.u32Stride[1];
		uvc_mem += dev->width;
		reqbufs_buf_used.buf_used += dev->width;

#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		if (set_reqbufs_buf_used == 1)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error_qbuf;
			}

			if(qbuf_flag == 0)
			{	
				qbuf_flag = 1;
				ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
				if(ret != 0)
				{
					ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
					goto done;
				}
			}
		}
	}

	if (set_reqbufs_buf_used == 2)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_qbuf;
		}

		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}	
	}
	else if(qbuf_flag == 0)
	{	
		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}
	}

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}

static int send_raw_i420_i420(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr = 0;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int dma_en = 0;
	uint32_t row_size[3];
	int qbuf_flag = 0;

	int y;
	const uint8_t* src_y;
	const uint8_t* src_u;
	const uint8_t* src_v;
	int set_reqbufs_buf_used = 0;

	void *mmap_virt_addr[3] = {NULL, NULL, NULL};
	uint32_t mmap_virt_size[3];
	int cache_en = 0;
	int dst_stride_u;
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
			dev->reqbufs_config.data_offset;
		
		if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			dma_en = 1;
		}
	}

	row_size[0] = dev->height * dev->width;
	row_size[1] = row_size[0] >> 2;
	row_size[2] = row_size[1];
	dst_stride_u = dev->width/2;//uv是一样的

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Stride[0];
	mmap_virt_size[1] = data->stFrameData.u32Height/2 * data->stFrameData.u32Stride[1];
	mmap_virt_size[2] = data->stFrameData.u32Height/2 * data->stFrameData.u32Stride[2];
	v4l2_buf.bytesused = row_size[0] + row_size[1] + row_size[2];
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);
	if (data->stFrameData.u32Stride[0] != dev->width ||
		data->stFrameData.u32Stride[1] != dst_stride_u ||
		data->stFrameData.u32Stride[2] != dst_stride_u ||
		data->stFrameData.u32Width != dev->width)
	{
		ucam_debug("u32Stride [0-2]:%d, %d, %d, u32Width %d", 
			data->stFrameData.u32Stride[0],
			data->stFrameData.u32Stride[1],
			data->stFrameData.u32Stride[2], 
			dev->width);
		dma_en = 0;
	}

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		set_reqbufs_buf_used = 2;//I420不能分开传输
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		cache_en = 1;
	}

	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[2] = uvc_comm_mmap(data->stFrameData.phyAddr[2], mmap_virt_size[2], 1);
		if(!mmap_virt_addr[2])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_u = mmap_virt_addr[1];
		src_v = mmap_virt_addr[2];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_u = data->stFrameData.pVirAddr[1];
		src_v = data->stFrameData.pVirAddr[2];
	}

	unsigned long long src_y_phy = data->stFrameData.phyAddr[0];
	unsigned long long src_u_phy = data->stFrameData.phyAddr[1];
	unsigned long long src_v_phy = data->stFrameData.phyAddr[2];

	int width_offset = 0;
	int height_offset = 0;
	uint32_t src_y_offset = 0;
	uint32_t src_u_offset = 0;
	uint32_t src_v_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y_offset = width_offset + height_offset * data->stFrameData.u32Stride[0];
		src_u_offset = (width_offset >> 1) + ((height_offset * data->stFrameData.u32Stride[1]) >> 1);
		src_v_offset = (width_offset >> 1) + ((height_offset * data->stFrameData.u32Stride[2]) >> 1);

		src_y += src_y_offset;
		src_y_phy += src_y_offset;
		src_u += src_u_offset;
		src_u_phy += src_u_offset;
		src_v += src_v_offset;
		src_v_phy += src_v_offset; 
	}

	if(dma_en)
	{
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr, uvc_mem,
			src_y_phy, (void *)src_y,
			row_size[0], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[0];

		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr + row_size[0],
			uvc_mem,
			src_u_phy, (void *)src_u,
			row_size[1], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[1];

		qbuf_flag = 1;
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr + row_size[0] + row_size[1],
			uvc_mem,
			src_v_phy, (void *)src_v,
			row_size[2], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[2];
	}
	else
	{
		for (y = 0; y < dev->height; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_y, dev->width);
			src_y += data->stFrameData.u32Stride[0];
			uvc_mem += dev->width;
			reqbufs_buf_used.buf_used += dev->width;
			
	#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
	#endif		
		}

		for (y = 0; y < dev->height/2; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_u, dst_stride_u);
			src_u += data->stFrameData.u32Stride[1];
			uvc_mem += dst_stride_u;
			reqbufs_buf_used.buf_used += dst_stride_u;

#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		}

		for (y = 0; y < dev->height/2; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_v, dst_stride_u);
			src_v += data->stFrameData.u32Stride[2];
			uvc_mem += dst_stride_u;
			reqbufs_buf_used.buf_used += dst_stride_u;

#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		}

		if (set_reqbufs_buf_used == 2)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error_qbuf;
			}

			qbuf_flag = 1;
			ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto done;
			}	
		}
		else if(qbuf_flag == 0)
		{	
			qbuf_flag = 1;
			ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
			if(ret != 0)
			{
				ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
				goto done;
			}
		}
	}
	

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
		uvc_common_munmap(mmap_virt_addr[2], mmap_virt_size[2]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
		
		if(mmap_virt_addr[2])
			uvc_common_munmap(mmap_virt_addr[2], mmap_virt_size[2]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}

static int send_raw_nv12_i420(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr = 0;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int dma_en = 0;
	uint32_t row_size[2];
	int qbuf_flag = 0;

	int y;
	const uint8_t* src_y;
	const uint8_t* src_uv;
	int set_reqbufs_buf_used = 0;

	void *mmap_virt_addr[2] = {NULL, NULL};
	uint32_t mmap_virt_size[2];
	int cache_en = 0;
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
			dev->reqbufs_config.data_offset;
		
		if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			dma_en = 1;
		}
	}

	row_size[0] = dev->height * dev->width;
	row_size[1] = row_size[0] >> 1;

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Stride[0];
	mmap_virt_size[1] = data->stFrameData.u32Height/2 * data->stFrameData.u32Stride[1];
	v4l2_buf.bytesused = row_size[0] + row_size[1];
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);
	if (data->stFrameData.u32Stride[0] != dev->width ||
		data->stFrameData.u32Width != dev->width)
	{
		ucam_debug("u32Stride %d != u32Width %d", 
			data->stFrameData.u32Stride[0], dev->width);
		dma_en = 0;
	}

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		set_reqbufs_buf_used = 2;//I420不能分开传输
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		cache_en = 1;
	}

	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_uv = mmap_virt_addr[1];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_uv = data->stFrameData.pVirAddr[1];
	}

	unsigned long long src_y_phy = data->stFrameData.phyAddr[0];
	int width_offset = 0;
	int height_offset = 0;
	uint32_t src_y_offset = 0;
	uint32_t src_uv_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y_offset = width_offset + height_offset * data->stFrameData.u32Stride[0];
		src_uv_offset = (width_offset) + ((height_offset * data->stFrameData.u32Stride[1]) >> 1);
		src_y += src_y_offset;
		src_y_phy += src_y_offset;
		src_uv += src_uv_offset; 
	}
	
	if(dma_en)
	{
		//Y
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr, uvc_mem,
			src_y_phy, (void *)src_y,
			row_size[0], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[0];
	}
	else
	{
		for (y = 0; y < dev->height; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_y, dev->width);
			src_y += data->stFrameData.u32Stride[0];
			uvc_mem += dev->width;
			reqbufs_buf_used.buf_used += dev->width;
			
	#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
	#endif		
		}
	}

	//YUV
	uint8_t* dst_u = uvc_mem;
	uint8_t* dst_v = dst_u + (dev->width * dev->height/4);
	uint32_t dst_stride_u = dev->width/2;

	//分离UV	
	void (*UVSplitRow)(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width) = UVC_UVSplitRow_NEON_16; 
	if(IS_ALIGNED(dev->width, 16) == 0)
		UVSplitRow = UVC_UVSplitRow_Any_NEON;

	for (y = 0; y < dev->height/2; y ++) 
	{
		UVSplitRow(src_uv, dst_u, dst_v, dev->width);
		src_uv += data->stFrameData.u32Stride[1];
		dst_u += dst_stride_u;
		dst_v += dst_stride_u;
		uvc_mem += dev->width;
		reqbufs_buf_used.buf_used += dev->width;

#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
	}

	if (set_reqbufs_buf_used == 2)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_qbuf;
		}

		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}	
	}
	else if(qbuf_flag == 0)
	{	
		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}
	}

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}

static int send_raw_nv21_i420(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr = 0;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	int dma_en = 0;
	uint32_t row_size[2];
	int qbuf_flag = 0;

	int y;
	const uint8_t* src_y;
	const uint8_t* src_uv;
	int set_reqbufs_buf_used = 0;

	void *mmap_virt_addr[2] = {NULL, NULL};
	uint32_t mmap_virt_size[2];
	int cache_en = 0;
	
	if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
			dev->reqbufs_config.data_offset;
		
		if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA)
		{
			dma_en = 1;
		}
	}

	row_size[0] = dev->height * dev->width;
	row_size[1] = row_size[0] >> 1;

	mmap_virt_size[0] = data->stFrameData.u32Height * data->stFrameData.u32Stride[0];
	mmap_virt_size[1] = data->stFrameData.u32Height/2 * data->stFrameData.u32Stride[1];
	v4l2_buf.bytesused = row_size[0] + row_size[1];
	uvc_us_to_timeval(data->u64Pts, &v4l2_buf.timestamp);
	if (data->stFrameData.u32Stride[0] != dev->width ||
		data->stFrameData.u32Width != dev->width)
	{
		ucam_debug("u32Stride %d != u32Width %d", 
			data->stFrameData.u32Stride[0], dev->width);
		dma_en = 0;
	}

	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	if (dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		set_reqbufs_buf_used = 2;//I420不能分开传输
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_CACHE)
	{
		cache_en = 1;
	}

	if(cache_en)
	{
		mmap_virt_addr[0] = uvc_comm_mmap(data->stFrameData.phyAddr[0], mmap_virt_size[0], 1);
		if(!mmap_virt_addr[0])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		mmap_virt_addr[1] = uvc_comm_mmap(data->stFrameData.phyAddr[1], mmap_virt_size[1], 1);
		if(!mmap_virt_addr[1])
		{
			ucam_error("uvc_comm_mmap error");
			goto error_qbuf;
		}

		src_y = mmap_virt_addr[0];
		src_uv = mmap_virt_addr[1];
	}
	else
	{
		src_y = data->stFrameData.pVirAddr[0];
		src_uv = data->stFrameData.pVirAddr[1];
	}

	unsigned long long src_y_phy = data->stFrameData.phyAddr[0];
	int width_offset = 0;
	int height_offset = 0;
	uint32_t src_y_offset = 0;
	uint32_t src_uv_offset = 0;
	//扣取中间的画面
	if(data->stFrameData.u32Width > dev->width)
	{
		width_offset = (data->stFrameData.u32Width - dev->width) >> 1;
	}

	if(data->stFrameData.u32Height > dev->height)
	{
		height_offset = (data->stFrameData.u32Height - dev->height) >> 1;
	}
	
	if(width_offset > 0 || height_offset > 0)
	{
		src_y_offset = width_offset + height_offset * data->stFrameData.u32Stride[0];
		src_uv_offset = (width_offset) + ((height_offset * data->stFrameData.u32Stride[1]) >> 1);
		src_y += src_y_offset;
		src_y_phy += src_y_offset;
		src_uv += src_uv_offset; 
	}
	
	if(dma_en)
	{
		//Y
		ret = uvc_common_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
			uvc_phy_addr, uvc_mem,
			src_y_phy, (void *)src_y,
			row_size[0], qbuf_flag, &reqbufs_buf_used, dma_en);
		if(ret != 0)
		{
			ucam_error("memcpy_and_qbuf error");
			goto error_qbuf;
		}
		uvc_mem += row_size[0];
	}
	else
	{
		for (y = 0; y < dev->height; y ++) 
		{
			uvc_memcpy_neon(uvc_mem, src_y, dev->width);
			src_y += data->stFrameData.u32Stride[0];
			uvc_mem += dev->width;
			reqbufs_buf_used.buf_used += dev->width;
			
	#if REQBUFS_CACHE_ENABLE == 1
			uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
	#endif		
		}
	}

	//YVU
	uint8_t* dst_v = uvc_mem;
	uint8_t* dst_u = dst_v + (dev->width * dev->height/4);
	uint32_t dst_stride_u = dev->width/2;

	//分离UV	
	void (*UVSplitRow)(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width) = UVC_UVSplitRow_NEON_16; 
	if(IS_ALIGNED(dev->width, 16) == 0)
		UVSplitRow = UVC_UVSplitRow_Any_NEON;

	for (y = 0; y < dev->height/2; y ++) 
	{
		UVSplitRow(src_uv, dst_u, dst_v, dev->width);
		src_uv += data->stFrameData.u32Stride[1];
		dst_u += dst_stride_u;
		dst_v += dst_stride_u;
		uvc_mem += dev->width;
		reqbufs_buf_used.buf_used += dev->width;

#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
	}

	if (set_reqbufs_buf_used == 2)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error_qbuf;
		}

		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}	
	}
	else if(qbuf_flag == 0)
	{	
		qbuf_flag = 1;
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}
	}

done:	
	if(cache_en)
	{
		uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
		uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error_qbuf:
	if(cache_en)
	{
		if(mmap_virt_addr[0])
			uvc_common_munmap(mmap_virt_addr[0], mmap_virt_size[0]);
	
		if(mmap_virt_addr[1])
			uvc_common_munmap(mmap_virt_addr[1], mmap_virt_size[1]);
	}

	if(!qbuf_flag)
	{
		v4l2_buf.bytesused = 0;
		ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			return -1;
		}
	}
	return -1;
}


/*
in:YUY2、NV12、NV21、I420
out:YUY2、NV12、I420
BUF:userptr、mmap
mode:DMA、cache、normal
最大分支可能有：4*3*2*3=72
 */
static struct uvc_send_raw_info raw_info_tab[] = {
	{V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_YUYV, send_raw_yuy2_yuy2},//YUY2 to YUY2
	{V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_YUYV, send_raw_nv12_yuy2},//NV12 to YUY2
	{V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_YUYV, send_raw_nv21_yuy2},//NV21 to YUY2

	{V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_NV12, send_raw_nv12_nv12},//NV12 to NV12
	{V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_NV12, send_raw_nv21_nv12},//NV21 to NV12
	
	{V4L2_PIX_FMT_YUV420,V4L2_PIX_FMT_YUV420,send_raw_i420_i420},//I420 to I420
	{V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_YUV420, send_raw_nv12_i420},//NV12 to I420
	{V4L2_PIX_FMT_NV21, V4L2_PIX_FMT_YUV420, send_raw_nv21_i420},//NV21 to I420	
};


static int uvc_send_raw(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int i;

	if (data->u32Format != dev->send_raw_info.format_in ||
		fcc != dev->send_raw_info.format_out ||
		dev->send_raw_info.send_raw == NULL)
	{
		dev->send_raw_info.send_raw = NULL;

		for(i = 0; i < sizeof(raw_info_tab)/sizeof(struct uvc_send_raw_info); i++)
		{
			if (raw_info_tab[i].format_in == data->u32Format &&
				raw_info_tab[i].format_out == fcc)
			{
				ucam_warning("format in:%s, out:%s",
					uvc_video_format_string(data->u32Format), uvc_video_format_string(fcc));
				memcpy(&dev->send_raw_info, &raw_info_tab[i], sizeof(struct uvc_send_raw_info));
				break;
			}
		}
	}

	if(dev->send_raw_info.send_raw == NULL)
	{
		ucam_warning("format not support");
		return -1;
	}
	return dev->send_raw_info.send_raw(dev, chn, fcc, data);
}

int uvc_send_stream_common_encode(
    struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_VENC_Stream_t *data)	
{
	int ret;
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	if(!dev->send_encode)
		return -ENOMEM;	

	
	if(dev == NULL || data == NULL)
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

	return dev->send_encode(dev, chn, fcc, data);
}


int uvc_send_stream_common_raw(
	struct uvc_dev *dev, 
    int32_t chn, 
    uint32_t fcc, 
    UVC_Raw_Stream_t *data)
{
	int ret;
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api
	
	if(dev == NULL || data == NULL)
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

	if(dev->fcc != fcc)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if (data->stFrameData.u32Width < dev->width || 
		data->stFrameData.u32Height < dev->height)
	{
		ucam_strace("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			data->stFrameData.u32Width, data->stFrameData.u32Height, dev->width, dev->height);
		return -1;
	}

	return uvc_send_raw(dev, chn, fcc, data);
	
}

int uvc_send_stream_common_ops_int(struct uvc_dev *dev)
{
	if(dev == NULL)
	{
		return -ENOMEM;
	}

	if (dev->send_stream_common_mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA &&
		!dev->uvc->uvc_dam_memcpy_callback && 
		dev->config.reqbufs_memory_type != V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		ucam_error("config error");
		return -1;
	}

	dev->send_encode = send_encode;
	return 0;	
}

int uvc_set_stream_common_mode(
	struct uvc_dev *dev, 
	UVC_Send_Stream_Common_Mode_e mode)
{
	if(dev == NULL)
	{
		return -ENOMEM;
	}

	if (mode == E_UVC_SEND_STREAM_COMMON_MODE_DMA &&
		!dev->uvc->uvc_dam_memcpy_callback && 
		dev->config.reqbufs_memory_type != V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
	{
		ucam_error("config error");
		return -1;
	}

	dev->send_stream_common_mode = mode;
	return 0;
}
