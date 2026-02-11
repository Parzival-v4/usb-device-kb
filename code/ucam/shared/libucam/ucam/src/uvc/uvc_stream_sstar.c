/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 13:45:47
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream_sstar.c
 * @Description : 
 */ 
#ifdef SIGMASTAR_SDK

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



#if (defined CHIP_SSC268 || CHIP_SSC369)
extern MI_S32 MI_SYS_MemcpyPa(MI_U16 u16SocId, MI_PHY phyDst, MI_PHY phySrc, MI_U32 u32Lenth);
#define VHD_MI_SYS_MemcpyPa(phyDst, phySrc, u32Lenth) MI_SYS_MemcpyPa(0, phyDst, phySrc, u32Lenth)
#else
extern MI_S32 MI_SYS_MemcpyPa(MI_PHY phyDst, MI_PHY phySrc, MI_U32 u32Lenth);
#define VHD_MI_SYS_MemcpyPa(phyDst, phySrc, u32Lenth) MI_SYS_MemcpyPa(phyDst, phySrc, u32Lenth)
#endif


static int dma_memcpy_and_qbuf(struct uvc_dev *dev, int32_t venc_chn, 
    struct v4l2_buffer *v4l2_buf,  unsigned long long uvc_phyAddr, void *uvc_pVirAddr,
    unsigned long long send_data_phyAddr, void *send_data_pVirAddr, 
	uint32_t send_size, int qbuf_en, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used)
{
    int ret = 0;
	MI_S32 s32Ret;
    uint32_t offset = 0;
    int qbuf_flag = 0;
	uint32_t r_4k,n_4k;
	uint32_t qbuf_transfer_size = 0;

	if(uvc_phyAddr == 0 || send_data_phyAddr == NULL || send_size == 0 )
	{
		ucam_error("data error");
		goto error;
	}

	r_4k = send_size & 0x00000FFF;
	n_4k = send_size & 0xFFFFF000;

	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
	{
		if(n_4k > 0)
		{
			s32Ret = VHD_MI_SYS_MemcpyPa(uvc_phyAddr + offset, 
				send_data_phyAddr + offset, n_4k);
			if(s32Ret != 0)
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
		qbuf_transfer_size = ALIGN_DOWN(qbuf_transfer_size,4096);
		if(qbuf_transfer_size < 4096)
		{
			qbuf_transfer_size = 4096;
		}
	}
	


	if(n_4k > 0)
	{
		while(n_4k)
		{
			if(n_4k <= qbuf_transfer_size)
			{
				//uvc_memcpy(uvc_mem + offset, send_data + offset, n_4k);
				s32Ret = VHD_MI_SYS_MemcpyPa(uvc_phyAddr + offset, 
					send_data_phyAddr + offset, n_4k);
				if(s32Ret != 0)
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
				ret = VHD_MI_SYS_MemcpyPa(uvc_phyAddr + offset, 
					send_data_phyAddr + offset, qbuf_transfer_size);
				if(ret != 0)
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


/**
 * @description: 将编码视频发给USB主机，必须在stream on状态下去才允许发送
 * @param {venc_chn：venc通道号；fcc：视频格式；pVStreamData：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int sstar_send_stream_encode(struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc, void* pStreamData)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	MI_VENC_Pack_t *pstPack;
	uint8_t *pack_buf;
	uint32_t pack_buf_len;
	uint32_t i;

	int qbuf_flag = 1;

	//uint16_t uvc_h264_profile;
	uint8_t* naluData = NULL;
	uint8_t nalu_type = 0;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	VENC_DATA_S_DEFINE *pVStreamData = (VENC_DATA_S_DEFINE *)pStreamData;
	
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
			ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
			if(ret != 0)
			{
				ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
				return ret;
			}
			uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);
  
			v4l2_buf.bytesused = 0;
			for(i = 0;i < pVStreamData->u32PackCount; i++)
			{
				v4l2_buf.bytesused += pVStreamData->pstPack[i].u32Len;
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
				for(i = 0;i < pVStreamData->u32PackCount; i++)
				{
					if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
					{
						if((i + 1) == pVStreamData->u32PackCount)
						{
							qbuf_flag = 1;
						}
						
					}
					ret = dma_memcpy_and_qbuf(dev, venc_chn, &v4l2_buf, phyAddr, uvc_mem,  
						pVStreamData->pstPack[i].phyAddr + pVStreamData->pstPack[i].u32Offset,
						pVStreamData->pstPack[i].pu8Addr + pVStreamData->pstPack[i].u32Offset,
						pVStreamData->pstPack[i].u32Len, qbuf_flag, &reqbufs_buf_used);
					if(ret != 0)
					{
						break;
					}
					phyAddr += pVStreamData->pstPack[i].u32Len;
					uvc_mem += pVStreamData->pstPack[i].u32Len;
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
				for(i = 0;i < pVStreamData->u32PackCount; i++)
				{
					uvc_memcpy(uvc_mem,pVStreamData->pstPack[i].pu8Addr + pVStreamData->pstPack[i].u32Offset, 
						pVStreamData->pstPack[i].u32Len);
					uvc_mem += pVStreamData->pstPack[i].u32Len;
				}
				
				ret = uvc_video_qbuf_lock(dev, venc_chn, &v4l2_buf);
				if(ret != 0)
				{
					ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
					return -1;
				}	
				return ret;
			}

			//break;
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
					if(!(E_MI_VENC_BASE_IDR == pVStreamData->stH264Info.enRefType ||   
						E_MI_VENC_BASE_P_REFBYBASE == pVStreamData->stH264Info.enRefType ||
						E_MI_VENC_BASE_P_REFBYENHANCE == pVStreamData->stH264Info.enRefType))
					{
						//windows skype for business只支持2层SVC
						return 0;
					}
#endif
					if(dev->first_IDR_flag)
					{

						pstPack = &pVStreamData->pstPack[0];
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
					for(i = 0;i < pVStreamData->u32PackCount; i++)
					{
						v4l2_buf.bytesused += pVStreamData->pstPack[i].u32Len;
					}

					if(v4l2_buf.bytesused  > v4l2_buf.length)
					{
						ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
						ret = -1;
						goto error_release;
					}
					
					v4l2_buf.bytesused = 0;
					for (i = 0; i < pVStreamData->u32PackCount; i++) {				
						pstPack = &pVStreamData->pstPack[i];
						pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
						pack_buf_len = pstPack->u32Len - pstPack->u32Offset;

						//去掉SEI
						if(dev->uvc_h264_sei_en == 0)
						{
							nalu_type = pack_buf[4] & 0x1f;
							if (nalu_type == 0x06) // skip sei
							{
								continue;
							}

						}

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
					pstPack = &pVStreamData->pstPack[0];
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
				uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);

				v4l2_buf.bytesused = 0;
				for (i = 0; i < pVStreamData->u32PackCount; i++) {
					pstPack = &pVStreamData->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;

					//去掉SEI
					if(dev->uvc_h264_sei_en == 0)
					{
						nalu_type = pack_buf[4] & 0x1f;
						if (nalu_type == 0x06) // skip sei
						{
							continue;
						}

					}

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
				pstPack = &pVStreamData->pstPack[0];
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

			uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);
			v4l2_buf.bytesused = 0;
			for(i = 0;i < pVStreamData->u32PackCount; i++)
			{
				v4l2_buf.bytesused += pVStreamData->pstPack[i].u32Len;
			}

			if(v4l2_buf.bytesused  > v4l2_buf.length)
            {
                ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
				ret = -1;
                goto error_release;
            }

			v4l2_buf.bytesused = 0;
			for (i = 0; i < pVStreamData->u32PackCount; i++) {
				pstPack = &pVStreamData->pstPack[i];
				pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
				pack_buf_len = pstPack->u32Len - pstPack->u32Offset;

				//去掉SEI
				if(dev->uvc_h264_sei_en == 0)
				{
					nalu_type = (pack_buf[4] & 0x7e)>>1;
					if (nalu_type == 39) { // skip sei
						continue;
					}
				}	

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
		default:
            ucam_error("unknown format %d", fcc);
			return -1;
            //break;
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
int sstar_send_stream_yuy2_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*)pBuf;
	
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

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	void *phy_virt_addr = NULL;
	uint32_t phy_addr_size;
	uint32_t align_4k_remainder_size;

	v4l2_buf.bytesused = pVBuf->stFrameData.u16Width * pVBuf->stFrameData.u16Height * 2;
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);
	
	align_4k_remainder_size = v4l2_buf.bytesused & 0x00000FFF;
	phy_addr_size = v4l2_buf.bytesused & ~0x00000FFF;
	
	if(phy_addr_size > 0)
	{
		phy_virt_addr = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_addr_size);
		if(phy_virt_addr == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(uvc_mem, phy_virt_addr, phy_addr_size);
	}

	if(align_4k_remainder_size > 0)
	{
		uvc_memcpy(uvc_mem + phy_addr_size, pVBuf->stFrameData.pVirAddr[0] + phy_addr_size, align_4k_remainder_size);	
	}
	
	
	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}

	if(phy_virt_addr)
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr, phy_addr_size);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
	}
	
	return ret;


error_release:
	if(phy_virt_addr)
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr, phy_addr_size);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
	}

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
int sstar_send_stream_nv12_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	void *phy_virt_addr[2] = {0, 0};

	uint32_t phy_addr_size[2];

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	//phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u32Stride[0];
	//phy_addr_size[1] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u32Stride[1]/2;
	//v4l2_buf.bytesused = pVBuf->stFrameData.u16Width * pVBuf->stFrameData.u16Height *3/2;

	phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u16Width;
	phy_addr_size[1] = phy_addr_size[0] >> 1;
	v4l2_buf.bytesused = phy_addr_size[0] + phy_addr_size[1];
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);

	uint32_t align_4k_remainder_size[2];
	void *p_uvc_mem = NULL;
	
	align_4k_remainder_size[0] = phy_addr_size[0] & 0x00000FFF;
	align_4k_remainder_size[1] = phy_addr_size[1] & 0x00000FFF;
	phy_addr_size[0] &= ~0x00000FFF;
	phy_addr_size[1] &= ~0x00000FFF;
	p_uvc_mem = uvc_mem;

	if(phy_addr_size[0])
	{
		phy_virt_addr[0] = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_addr_size[0]);
		if(phy_virt_addr[0] == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(p_uvc_mem, phy_virt_addr[0], phy_addr_size[0]);
		p_uvc_mem += phy_addr_size[0];
	}

	if(align_4k_remainder_size[0] > 0)
	{
		uvc_memcpy(p_uvc_mem, 
			pVBuf->stFrameData.pVirAddr[0] + phy_addr_size[0],  align_4k_remainder_size[0]);
		p_uvc_mem += align_4k_remainder_size[0];
	}

	if(phy_addr_size[1])
	{
		phy_virt_addr[1] = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[1], phy_addr_size[1]);
		if(phy_virt_addr[1] == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(p_uvc_mem, phy_virt_addr[1], phy_addr_size[1]);
		p_uvc_mem += phy_addr_size[1];
	}

	if(align_4k_remainder_size[1] > 0)
	{
		uvc_memcpy(p_uvc_mem, 
			pVBuf->stFrameData.pVirAddr[1] + phy_addr_size[1],  align_4k_remainder_size[1]);
	}
	
	
	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}

	if(phy_virt_addr[0])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr[0], phy_addr_size[0]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[0], phy_addr_size[0]);
	}

	if(phy_virt_addr[1])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[1], phy_virt_addr[1], phy_addr_size[1]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[1], phy_addr_size[1]);
	}
	return ret;


error_release:
	if(phy_virt_addr[0])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr[0], phy_addr_size[0]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[0], phy_addr_size[0]);
	}

	if(phy_virt_addr[1])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[1], phy_virt_addr[1], phy_addr_size[1]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[1], phy_addr_size[1]);
	}

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

#if !(defined CHIP_SSC9381 || defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211) 
/**
 * @description: 将YUY2发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int sstar_send_stream_i420_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	void *phy_virt_addr[3] = {0, 0};

	uint32_t phy_addr_size[3];

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(dev->fcc != V4L2_PIX_FMT_YUV420)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV420_PLANAR)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u16Width;
	phy_addr_size[1] = phy_addr_size[0] >> 2;
	phy_addr_size[2] = phy_addr_size[1];

	v4l2_buf.bytesused = phy_addr_size[0] + phy_addr_size[1] + phy_addr_size[2];
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);

	uint32_t align_4k_remainder_size[3];
	void *p_uvc_mem = NULL;
	
	align_4k_remainder_size[0] = phy_addr_size[0] & 0x00000FFF;
	align_4k_remainder_size[1] = phy_addr_size[1] & 0x00000FFF;
	align_4k_remainder_size[2] = phy_addr_size[2] & 0x00000FFF;
	phy_addr_size[0] &= ~0x00000FFF;
	phy_addr_size[1] &= ~0x00000FFF;
	phy_addr_size[2] &= ~0x00000FFF;
	p_uvc_mem = uvc_mem;

	if(phy_addr_size[0])
	{
		phy_virt_addr[0] = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_addr_size[0]);
		if(phy_virt_addr[0] == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(p_uvc_mem, phy_virt_addr[0], phy_addr_size[0]);
		p_uvc_mem += phy_addr_size[0];
	}

	if(align_4k_remainder_size[0] > 0)
	{
		uvc_memcpy(p_uvc_mem, 
			pVBuf->stFrameData.pVirAddr[0] + phy_addr_size[0],  align_4k_remainder_size[0]);
		p_uvc_mem += align_4k_remainder_size[0];
	}

	if(phy_addr_size[1])
	{
		phy_virt_addr[1] = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[1], phy_addr_size[1]);
		if(phy_virt_addr[1] == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(p_uvc_mem, phy_virt_addr[1], phy_addr_size[1]);
		p_uvc_mem += phy_addr_size[1];
	}

	if(align_4k_remainder_size[1] > 0)
	{
		uvc_memcpy(p_uvc_mem, 
			pVBuf->stFrameData.pVirAddr[1] + phy_addr_size[1],  align_4k_remainder_size[1]);
		p_uvc_mem += align_4k_remainder_size[1];
	}

	if(phy_addr_size[2])
	{
		phy_virt_addr[2] = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[2], phy_addr_size[2]);
		if(phy_virt_addr[2] == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(p_uvc_mem, phy_virt_addr[2], phy_addr_size[2]);
		p_uvc_mem += phy_addr_size[2];
	}

	if(align_4k_remainder_size[2] > 0)
	{
		uvc_memcpy(p_uvc_mem, 
			pVBuf->stFrameData.pVirAddr[2] + phy_addr_size[2],  align_4k_remainder_size[2]);
		p_uvc_mem += align_4k_remainder_size[2];
	}
	
	
	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}

	if(phy_virt_addr[0])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr[0], phy_addr_size[0]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[0], phy_addr_size[0]);
	}

	if(phy_virt_addr[1])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[1], phy_virt_addr[1], phy_addr_size[1]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[1], phy_addr_size[1]);
	}

	if(phy_virt_addr[2])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[2], phy_virt_addr[2], phy_addr_size[2]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[2], phy_addr_size[2]);
	}
	return ret;


error_release:
	if(phy_virt_addr[0])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr[0], phy_addr_size[0]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[0], phy_addr_size[0]);
	}

	if(phy_virt_addr[1])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[1], phy_virt_addr[1], phy_addr_size[1]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[1], phy_addr_size[1]);
	}

	if(phy_virt_addr[2])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[2], phy_virt_addr[2], phy_addr_size[2]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[2], phy_addr_size[2]);
	}

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
#endif

/**
 * @description: 将L8发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int sstar_send_stream_l8_ir_mmap(struct uvc_dev *dev, void* pBuf, void *pMetadata)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	void *phy_virt_addr[1] = {0};

	uint32_t phy_addr_size[1];

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(dev->fcc != V4L2_PIX_FMT_L8_IR)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	//phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u32Stride[0];
	//phy_addr_size[1] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u32Stride[1]/2;
	//v4l2_buf.bytesused = pVBuf->stFrameData.u16Width * pVBuf->stFrameData.u16Height *3/2;

	phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u16Width;
	v4l2_buf.bytesused = phy_addr_size[0];
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);

	uint32_t align_4k_remainder_size[1];
	void *p_uvc_mem = NULL;
	
	align_4k_remainder_size[0] = phy_addr_size[0] & 0x00000FFF;
	phy_addr_size[0] &= ~0x00000FFF;
	p_uvc_mem = uvc_mem;

	if(pMetadata)
	{
		memcpy(uvc_mem + v4l2_buf.bytesused, pMetadata, 16);
	}
	else
	{
		memset(uvc_mem + v4l2_buf.bytesused, 0, 16);
	}

	if(phy_addr_size[0])
	{
		phy_virt_addr[0] = dev->uvc->uvc_mmap_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_addr_size[0]);
		if(phy_virt_addr[0] == NULL)
		{
			ucam_error("uvc_mmap_cache_callback error");
			ret = -ENOMEM;
			goto error_release;
		}
		uvc_memcpy(p_uvc_mem, phy_virt_addr[0], phy_addr_size[0]);
		p_uvc_mem += phy_addr_size[0];
	}

	if(align_4k_remainder_size[0] > 0)
	{
		uvc_memcpy(p_uvc_mem, 
			pVBuf->stFrameData.pVirAddr[0] + phy_addr_size[0],  align_4k_remainder_size[0]);
		p_uvc_mem += align_4k_remainder_size[0];
	}
	
	
	ret = uvc_video_qbuf_lock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
	}

	if(phy_virt_addr[0])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr[0], phy_addr_size[0]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[0], phy_addr_size[0]);
	}

	return ret;


error_release:
	if(phy_virt_addr[0])
	{
		dev->uvc->uvc_mmz_flush_cache_callback(dev, pVBuf->stFrameData.phyAddr[0], phy_virt_addr[0], phy_addr_size[0]);
		dev->uvc->uvc_munmap_callback(dev, phy_virt_addr[0], phy_addr_size[0]);
	}

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
int sstar_send_stream_yuy2_userptr(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV422_YUYV)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	unsigned long long uvc_phy_addr;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	
	uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
		dev->reqbufs_config.data_offset;
	v4l2_buf.bytesused = (pVBuf->stFrameData.u16Width * pVBuf->stFrameData.u16Height) << 1;
	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;

	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);
	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
            uvc_phy_addr, uvc_mem,
			pVBuf->stFrameData.phyAddr[0], pVBuf->stFrameData.pVirAddr[0],
			v4l2_buf.bytesused, 1, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}
	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	return ret;
}

/**
 * @description: 将YUY2发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int sstar_send_stream_nv12_userptr(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	uint32_t phy_addr_size[2];
	int qbuf_flag = 1;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	//phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u32Stride[0];
	//phy_addr_size[1] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u32Stride[1]/2;
	//v4l2_buf.bytesused = pVBuf->stFrameData.u16Width * pVBuf->stFrameData.u16Height *3/2;

	phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u16Width;
	phy_addr_size[1] = phy_addr_size[0] >> 1;
	v4l2_buf.bytesused = phy_addr_size[0] + phy_addr_size[1];

	unsigned long long uvc_phy_addr;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	
	uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
		dev->reqbufs_config.data_offset;
		
	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);


	qbuf_flag = 0;

	
	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
		uvc_phy_addr, uvc_mem,
		pVBuf->stFrameData.phyAddr[0], pVBuf->stFrameData.pVirAddr[0],
		phy_addr_size[0], qbuf_flag, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}

	qbuf_flag = 1;
	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
		uvc_phy_addr + phy_addr_size[0],
		uvc_mem + phy_addr_size[0],
		pVBuf->stFrameData.phyAddr[1], pVBuf->stFrameData.pVirAddr[1], phy_addr_size[1], qbuf_flag, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}	

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	return ret;
}

#if !(defined CHIP_SSC9381 || defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211) 
/**
 * @description: 将YUY2发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int sstar_send_stream_i420_userptr(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	uint32_t phy_addr_size[3];
	int qbuf_flag = 1;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(dev->fcc != V4L2_PIX_FMT_YUV420)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV420_PLANAR)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}

	phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u16Width;
	phy_addr_size[1] = phy_addr_size[0] >> 2;
	phy_addr_size[2] = phy_addr_size[1];
	v4l2_buf.bytesused = phy_addr_size[0] + phy_addr_size[1] + phy_addr_size[2];

	unsigned long long uvc_phy_addr;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	
	uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
		dev->reqbufs_config.data_offset;
		
	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);


	qbuf_flag = 0;
	//Y
	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
		uvc_phy_addr, uvc_mem,
		pVBuf->stFrameData.phyAddr[0], pVBuf->stFrameData.pVirAddr[0],
		phy_addr_size[0], qbuf_flag, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}

	//U
	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
		uvc_phy_addr + phy_addr_size[0],
		uvc_mem + phy_addr_size[0],
		pVBuf->stFrameData.phyAddr[1], pVBuf->stFrameData.pVirAddr[1], phy_addr_size[1], qbuf_flag, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}


	qbuf_flag = 1;

	//V
	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
		uvc_phy_addr + phy_addr_size[0] + phy_addr_size[1],
		uvc_mem + phy_addr_size[0] + phy_addr_size[1],
		pVBuf->stFrameData.phyAddr[2], pVBuf->stFrameData.pVirAddr[2], phy_addr_size[2], qbuf_flag, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}	

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	return ret;
}
#endif

/**
 * @description: 将Y发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int sstar_send_stream_l8_ir_userptr(struct uvc_dev *dev, void* pBuf, void *pMetadata)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	
	uint32_t phy_addr_size[1];

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*) pBuf;
	
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

	if(dev->fcc != V4L2_PIX_FMT_L8_IR)
	{
		ucam_error("uvc format error, uvc format:%s",
			uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->eBufType != E_MI_SYS_BUFDATA_FRAME)
	{
		ucam_error("eBufType error, eBufType:%d",pVBuf->eBufType);
		return -1;
	}

	if(pVBuf->stFrameData.ePixelFormat != E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420)
	{
		ucam_error("ePixelFormat error, ePixelFormat:%d",pVBuf->stFrameData.ePixelFormat);
		return -1;
	}

	if(pVBuf->stFrameData.u16Width != dev->width || pVBuf->stFrameData.u16Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", 
			pVBuf->stFrameData.u16Width, pVBuf->stFrameData.u16Height, dev->width, dev->height);
		return -1;
	}

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		return ret;
	}


	phy_addr_size[0] = pVBuf->stFrameData.u16Height * pVBuf->stFrameData.u16Width;
	v4l2_buf.bytesused = phy_addr_size[0];

	unsigned long long uvc_phy_addr;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;
	
	uvc_phy_addr = dev->video_reqbufs_addr[v4l2_buf.index] + 
		dev->reqbufs_config.data_offset;
		
	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	uvc_us_to_timeval(pVBuf->u64Pts, &v4l2_buf.timestamp);

	if(pMetadata)
	{
		memcpy(uvc_mem + v4l2_buf.bytesused, pMetadata, 16);
	}
	else
	{
		memset(uvc_mem + v4l2_buf.bytesused, 0, 16);
	}

	ret = dma_memcpy_and_qbuf(dev, dev->config.venc_chn, &v4l2_buf, 
		uvc_phy_addr, uvc_mem,
		pVBuf->stFrameData.phyAddr[0], pVBuf->stFrameData.pVirAddr[0],
		phy_addr_size[0], 1, &reqbufs_buf_used);
	if(ret != 0)
	{
		ucam_error("memcpy_and_qbuf error");
		pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
		return ret;
	}

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);
	return ret;
}


int uvc_send_stream_ops_int(struct uvc_dev *dev)
{
	switch (dev->config.reqbufs_memory_type)
	{
	case V4L2_REQBUFS_MEMORY_TYPE_MMAP:
		dev->send_stream_ops.send_encode = sstar_send_stream_encode;
		dev->send_stream_ops.send_yuy2 = sstar_send_stream_yuy2_mmap;
		dev->send_stream_ops.send_nv12 = sstar_send_stream_nv12_mmap;
#if !(defined CHIP_SSC9381 || defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211) 
		dev->send_stream_ops.send_i420 = sstar_send_stream_i420_mmap;
#endif
		dev->send_stream_ops.send_l8_ir = sstar_send_stream_l8_ir_mmap;
		break;
	case V4L2_REQBUFS_MEMORY_TYPE_USERPTR:
		dev->send_stream_ops.send_encode = sstar_send_stream_encode;
		dev->send_stream_ops.send_yuy2 = sstar_send_stream_yuy2_userptr;
		dev->send_stream_ops.send_nv12 = sstar_send_stream_nv12_userptr;
#if !(defined CHIP_SSC9381 || defined CHIP_SSC333 || defined CHIP_SSC337 || defined CHIP_SSC9211) 
		dev->send_stream_ops.send_i420 = sstar_send_stream_i420_userptr;
#endif
		dev->send_stream_ops.send_l8_ir = sstar_send_stream_l8_ir_userptr;
		break;	
	default:
		//不支持其他的类型
		return -1;
	}

	//uvc_set_uvc_pts_copy_enable(dev, 1);
	
	return 0;
}

#endif /* #ifdef SIGMASTAR_SDK */
