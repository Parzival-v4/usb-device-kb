/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 13:45:47
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_stream_hisi.c
 * @Description : 
 */ 
#if (defined HISI_SDK || defined GK_SDK)

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
 * @description: UVC发送hisi编码视频给USB主机
 * @param {uvc:句柄, uvc_stream_num:第几路UVC码流, venc_chn:venc通道, enType:编码类型, pVStreamData:视频数据} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_hisi_encode_mmap_userptr(struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc, void* pStreamData)
{
	int ret;
	unsigned int i;
#if defined CHIP_HI3559V100
	HI_VENC_DATA_PACK_S *pstPack;
#else
	VENC_PACK_S *pstPack;
#endif
	uint8_t *pack_buf;
	uint32_t pack_buf_len;

	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;
	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;

	uint16_t uvc_h264_profile;
	uint8_t* naluData = NULL;
	uint8_t nalu_type = 0;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	VENC_DATA_S_DEFINE* pVStreamData = (VENC_DATA_S_DEFINE*)pStreamData;

	if(dev == NULL || pVStreamData == NULL)
	{
		return -ENOMEM;
	}

    if(dev->uvc_stream_on == 0)
	{
		return -EALREADY;
	}

	ret = uvc_api_get_stream_status(dev, venc_chn, &stream_status);
	if(ret != 0 || stream_status == 0)
	{
		return -EALREADY;
	}

	if((dev->fcc != V4L2_PIX_FMT_MJPEG && dev->fcc != V4L2_PIX_FMT_H264 && dev->fcc != V4L2_PIX_FMT_H265)
		|| dev->fcc != fcc)
	{
		ucam_error("uvc format error, uvc format:%s, send format:%s",
		 uvc_video_format_string(dev->fcc), uvc_video_format_string(fcc));
		return -1;	
	}

	switch (fcc)
	{
		case V4L2_PIX_FMT_MJPEG:
		{	
			ret = uvc_video_dqbuf_lock(dev, venc_chn, &v4l2_buf, &uvc_mem);
			if(ret != 0)
			{
				ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
				return -1;
			}
            v4l2_buf.bytesused = 0;
			uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);

			if(dev->config.reqbufs_memory_type == V4L2_REQBUFS_MEMORY_TYPE_USERPTR)
			{
				int qbuf_flag = 1;
				for ( i = 0 ; i < pVStreamData->u32PackCount; i++ )
				{							
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[i];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					pack_buf = pstPack->pu8Addr[0] + pstPack->u32Offset;
					pack_buf_len = pstPack->au32Len[0] - pstPack->u32Offset;
#else
					pstPack = &pVStreamData->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;
#endif
					v4l2_buf.bytesused += pack_buf_len;
				}

				if(v4l2_buf.bytesused  > v4l2_buf.length)
				{
					ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);  
					goto error_qbuf;
				}

				if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
				{
					qbuf_flag = 0;
				}

				reqbufs_buf_used.index = v4l2_buf.index;
				reqbufs_buf_used.buf_used = 0;	
				for ( i = 0 ; i < pVStreamData->u32PackCount; i++ )
				{	

					if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
					{
						if((i + 1) == pVStreamData->u32PackCount)
						{
							qbuf_flag = 1;
						}
						
					}					
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[i];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					pack_buf = pstPack->pu8Addr[0] + pstPack->u32Offset;
					pack_buf_len = pstPack->au32Len[0] - pstPack->u32Offset;
#else
					pstPack = &pVStreamData->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;
#endif
					ret = uvc_memcpy_and_qbuf(dev, venc_chn, &v4l2_buf, 
						uvc_mem, pack_buf, pack_buf_len, qbuf_flag, &reqbufs_buf_used);
					if(ret != 0)
					{
						break;
					}
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
            	for ( i = 0 ; i < pVStreamData->u32PackCount; i++ )
				{						
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[i];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					pack_buf = pstPack->pu8Addr[0] + pstPack->u32Offset;
					pack_buf_len = pstPack->au32Len[0] - pstPack->u32Offset;
#else
					pstPack = &pVStreamData->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;
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
			break;
		}

		case V4L2_PIX_FMT_H264:
		{
			if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && dev->stream_num > 0 && 
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

			if(dev->config.uvc_protocol == UVC_PROTOCOL_V150 && (dev->H264SVCEnable == 1 || 
				uvc_h264_profile == UVCX_H264_wProfile_SCALABLEBASELINEPROFILE || 
				uvc_h264_profile == UVCX_H264_wProfile_SCALABLEHIGHPROFILE))
			{
#ifdef	SVC_LAYERS_DISCARD //三层丢一层变两层SVC
				//三层丢一层变两层SVC
				if(!(BASE_IDRSLICE == pVStreamData->stH264Info.enRefType ||   
					BASE_PSLICE_REFBYBASE == pVStreamData->stH264Info.enRefType ||
					BASE_PSLICE_REFBYENHANCE == pVStreamData->stH264Info.enRefType))
				{
					//windows skype for business只支持2层SVC
					return 0;
				}
#endif
				if(dev->first_IDR_flag)
				{
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[0];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					naluData = (uint8_t *)(pstPack->pu8Addr[0] + pstPack->u32Offset);
#else
					pstPack = &pVStreamData->pstPack[0];
					naluData = (uint8_t *)(pstPack->pu8Addr + pstPack->u32Offset);
#endif
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
					return -1;
				}
				uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);
				v4l2_buf.bytesused = 0;
				for (i = 0; i < pVStreamData->u32PackCount; i++) {				
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[i];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					pack_buf = pstPack->pu8Addr[0] + pstPack->u32Offset;
					pack_buf_len = pstPack->au32Len[0] - pstPack->u32Offset;
#else
					pstPack = &pVStreamData->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;
#endif

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
							goto error_qbuf;
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
			else
			{
				if(dev->first_IDR_flag)
				{
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[0];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					naluData = (uint8_t *)(pstPack->pu8Addr[0] + pstPack->u32Offset);
#else
					pstPack = &pVStreamData->pstPack[0];
					naluData = (uint8_t *)(pstPack->pu8Addr + pstPack->u32Offset);
#endif
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
					return -1;
				}
				uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);
				v4l2_buf.bytesused = 0;
				for (i = 0; i < pVStreamData->u32PackCount; i++) {
#if defined CHIP_HI3559V100
					pstPack = &pVStreamData->astPack[i];
					//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
					pack_buf = pstPack->pu8Addr[0] + pstPack->u32Offset;
					pack_buf_len = pstPack->au32Len[0] - pstPack->u32Offset;
#else
					pstPack = &pVStreamData->pstPack[i];
					pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
					pack_buf_len = pstPack->u32Len - pstPack->u32Offset;
#endif

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
			break;
		}

		case V4L2_PIX_FMT_H265:
		{
			if(dev->first_IDR_flag)
			{
				
#if defined CHIP_HI3559V100
				pstPack = &pVStreamData->astPack[0];
				//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
				naluData = (uint8_t *)(pstPack->pu8Addr[0] + pstPack->u32Offset);
#else
				pstPack = &pVStreamData->pstPack[0];
				naluData = (uint8_t *)(pstPack->pu8Addr + pstPack->u32Offset);
#endif				
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
				return -1;
			}
			uvc_us_to_timeval(pVStreamData->pstPack[0].u64PTS, &v4l2_buf.timestamp);
			v4l2_buf.bytesused = 0;

			for (i = 0; i < pVStreamData->u32PackCount; i++) {
				
#if defined CHIP_HI3559V100
				pstPack = &pVStreamData->astPack[i];
				//u32PhyAddr 和 pu8Addr 都有两个地址，地址 0 是有效的，地址 1 作为保留
				pack_buf = pstPack->pu8Addr[0] + pstPack->u32Offset;
				pack_buf_len = pstPack->au32Len[0] - pstPack->u32Offset;
#else
				pstPack = &pVStreamData->pstPack[i];
				pack_buf = pstPack->pu8Addr + pstPack->u32Offset;
				pack_buf_len = pstPack->u32Len - pstPack->u32Offset;
#endif

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
			break;
		}

		default:
		{
			ucam_error("enType error");
			return -1;
			//break;
		}
	}


//uvc_qbuf:

	if(v4l2_buf.bytesused > v4l2_buf.length)
	{
		ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
		v4l2_buf.bytesused = 0;
	}

	ret = uvc_video_qbuf_lock(dev, venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		return -1;
	}	

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



/**
 * @description: UVC将yuv sp420视频转换成YUY2然后发给USB主机
 * @param {uvc:句柄, uvc_stream_num:第几路UVC码流} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_hisi_nv21_to_yuy2_userptr(struct uvc_dev *dev, void* pBuf)
{
	int ret = 0;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	int y;
	const uint8_t* src_y;
	int src_stride_y;
	const uint8_t* src_vu;
	int src_stride_vu;
	uint8_t* dst_yuy2;
	int dst_stride_yuy2;


	uint32_t phy_addr_size;
	unsigned long long phy_addr;
	void *phy_virt_addr;

	uint32_t temp = 0;

	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE*)pBuf;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

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

	if(pVBuf->u32Width != dev->width || pVBuf->u32Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", pVBuf->u32Width, pVBuf->u32Height, dev->width, dev->height);
		return -1;
	}

	phy_addr_size = pVBuf->u32Stride[0] * pVBuf->u32Height * 3/2; 
#if defined CHIP_HI3519V101 || defined CHIP_HI3559V100
	phy_addr = pVBuf->u32PhyAddr[0];	
#else
	phy_addr = pVBuf->u64PhyAddr[0];	
#endif

	phy_virt_addr = dev->uvc->uvc_mmap_cache_callback(dev, phy_addr, phy_addr_size);
	if (NULL == phy_virt_addr)
	{    
		ucam_error("mmap error, addr:0x%xll, size:%d", phy_addr, phy_addr_size);
		goto error;
	}
	src_y = phy_virt_addr;
	src_vu = src_y + pVBuf->u32Stride[0] * pVBuf->u32Height;

	dst_stride_yuy2 = pVBuf->u32Width * 2;
	src_stride_y = pVBuf->u32Stride[0];
	src_stride_vu = pVBuf->u32Stride[1];

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		goto error;
	}
#if defined CHIP_HI3559V100 || defined CHIP_HI3519V101
	uvc_us_to_timeval(pVBuf->u64pts, &v4l2_buf.timestamp);
#else
	uvc_us_to_timeval(pVBuf->u64PTS, &v4l2_buf.timestamp);
#endif
	v4l2_buf.bytesused = pVBuf->u32Width * pVBuf->u32Height * 2;
	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;
	dst_yuy2 = uvc_mem;

	if (!src_y || !src_vu || !dst_yuy2 || pVBuf->u32Width  <= 0 || pVBuf->u32Height <= 0) {

		ucam_error("error!!!, %p,%p,%p,%d,%d",src_y,src_vu,dst_yuy2,pVBuf->u32Width,pVBuf->u32Height);
		goto error2;
	} 

	void (*UVC_NV21ToYUY2Row)(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) = UVC_NV21ToYUY2Row_Any_NEON;
						

	if(IS_ALIGNED(pVBuf->u32Width, 16))
		UVC_NV21ToYUY2Row = UVC_NV21ToYUY2Row_NEON_16;


	for (y = 0; y < pVBuf->u32Height - 1; y += 2) 
	{
		UVC_NV21ToYUY2Row(src_y, src_vu, 
				dst_yuy2, pVBuf->u32Width);

		UVC_NV21ToYUY2Row(src_y + src_stride_y, src_vu, 
				dst_yuy2 + dst_stride_yuy2, pVBuf->u32Width);
						
		src_y += src_stride_y << 1;
		src_vu += src_stride_vu;
		temp = dst_stride_yuy2 << 1;
		dst_yuy2 += temp;
		reqbufs_buf_used.buf_used += temp;
		
#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error2;
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

	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error2;
		}

		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}
	}

done:	
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}

		phy_virt_addr = NULL;
	}

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}
	return -1;

error2:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
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

	return -2;
}

/**
 * @description: UVC将yuv sp420视频转换成NV12然后发给USB主机
 * @param {uvc:句柄, uvc_stream_num:第几路UVC码流} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_hisi_nv21_to_nv12_userptr(struct uvc_dev *dev, void* pBuf)
{
	int ret = 0;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	int y;
	const uint8_t* src_y;
	int src_stride_y;
	const uint8_t* src_vu;
	int src_stride_vu;

	uint32_t phy_addr_size;
	unsigned long long phy_addr;
	void *phy_virt_addr;

	struct uvc_ioctl_reqbufs_buf_used_t reqbufs_buf_used;

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

	if(dev->fcc != V4L2_PIX_FMT_NV12)
	{
		ucam_error("uvc format error, uvc format:%s",
		 uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->u32Width != dev->width || pVBuf->u32Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", pVBuf->u32Width, pVBuf->u32Height, dev->width, dev->height);
		return -1;
	}

	phy_addr_size = pVBuf->u32Stride[0] * pVBuf->u32Height * 3/2; 
#if defined CHIP_HI3519V101 || defined CHIP_HI3559V100
	phy_addr = pVBuf->u32PhyAddr[0];
#else
	phy_addr = pVBuf->u64PhyAddr[0];	
#endif
	phy_virt_addr = (uint8_t *)dev->uvc->uvc_mmap_cache_callback(dev, phy_addr, phy_addr_size);	
	if (NULL == phy_virt_addr)
	{    
		ucam_error("mmap error, addr:0x%xll, size:%d", phy_addr, phy_addr_size);
		goto error;
	}
	src_y = phy_virt_addr;
	src_vu = src_y + pVBuf->u32Stride[0] * pVBuf->u32Height;

	src_stride_y = pVBuf->u32Stride[0];
	src_stride_vu = pVBuf->u32Stride[1];

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		goto error;
	}
#if defined CHIP_HI3559V100 || defined CHIP_HI3519V101
	uvc_us_to_timeval(pVBuf->u64pts, &v4l2_buf.timestamp);
#else
	uvc_us_to_timeval(pVBuf->u64PTS, &v4l2_buf.timestamp);
#endif
	v4l2_buf.bytesused = pVBuf->u32Width * pVBuf->u32Height * 3 / 2;
	reqbufs_buf_used.index = v4l2_buf.index;
	reqbufs_buf_used.buf_used = 0;

	if (!src_y || !src_vu || pVBuf->u32Width  <= 0 || pVBuf->u32Height <= 0) {

		ucam_error("error!!!");
		goto error2;
	} 
						
	void (*uvc_vu_rev2uv_row)(const uint8_t* src_vu, uint8_t* dst_uv, int width) = UVC_UVSwapRow_NEON_16; 
	if(IS_ALIGNED(pVBuf->u32Width, 16) == 0)
		uvc_vu_rev2uv_row = UVC_UVSwapRow_Any_NEON;

	for (y = 0; y < pVBuf->u32Height; y ++) 
	{
		uvc_memcpy_neon(uvc_mem, src_y, pVBuf->u32Width);
		src_y += src_stride_y;
		uvc_mem += pVBuf->u32Width;
		reqbufs_buf_used.buf_used += pVBuf->u32Width;
		
#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error2;
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

	for (y = 0; y < pVBuf->u32Height/2; y ++) 
	{
		uvc_vu_rev2uv_row(src_vu, uvc_mem, pVBuf->u32Width);
		src_vu += src_stride_vu;
		uvc_mem += pVBuf->u32Width;
		reqbufs_buf_used.buf_used += pVBuf->u32Width;

#if REQBUFS_CACHE_ENABLE == 1
		uvc_video_reqbufs_flush_cache(dev, v4l2_buf.index);			
#endif	
		if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_ISOC)
		{
			ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
			if(ret != 0)
			{
				ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
				goto error2;
			}
		}
	}

	if(dev->config.uvc_ep_mode == UVC_USB_TRANSFER_MODE_BULK)
	{
		ret = uvc_ioctl_set_reqbufs_buf_used(dev, &reqbufs_buf_used);
		if(ret != 0)
		{
			ucam_error("set_reqbufs_buf_used error, ret=%d", ret);
			goto error2;
		}
	
		ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
		if(ret != 0)
		{
			ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
			goto done;
		}	
	}

done:	
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}
	return -1;

error2:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
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

	return -2;
}


/**
 * @description: UVC将yuv sp420视频转换成YUY2然后发给USB主机
 * @param {uvc:句柄, uvc_stream_num:第几路UVC码流} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_hisi_nv21_to_yuy2_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret = 0;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	const uint8_t* src_y;
	int src_stride_y;
	const uint8_t* src_vu;
	int src_stride_vu;
	uint8_t* dst_yuy2;
	int dst_stride_yuy2;


	uint32_t phy_addr_size;
	unsigned long long phy_addr;
	void *phy_virt_addr;

	uint32_t stream_status = 1;//默认打开，上层应用可能没有添加状态到这个api

	FRAME_DATA_S_DEFINE* pVBuf = (FRAME_DATA_S_DEFINE *) pBuf;

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

	if(pVBuf->u32Width != dev->width || pVBuf->u32Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", pVBuf->u32Width, pVBuf->u32Height, dev->width, dev->height);
		return -1;
	}

	phy_addr_size = pVBuf->u32Stride[0] * pVBuf->u32Height * 3/2; 
#if defined CHIP_HI3519V101 || defined CHIP_HI3559V100
	phy_addr = pVBuf->u32PhyAddr[0];
#else
	phy_addr = pVBuf->u64PhyAddr[0];	
#endif
	phy_virt_addr = (uint8_t *)dev->uvc->uvc_mmap_cache_callback(dev, phy_addr, phy_addr_size);	
	if (NULL == phy_virt_addr)
	{    
		ucam_error("mmap error, addr:0x%xll, size:%d", phy_addr, phy_addr_size);
		goto error;
	}
	src_y = phy_virt_addr;
	src_vu = src_y + pVBuf->u32Stride[0] * pVBuf->u32Height;

	dst_stride_yuy2 = pVBuf->u32Width * 2;
	src_stride_y = pVBuf->u32Stride[0];
	src_stride_vu = pVBuf->u32Stride[1];

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		goto error;
	}
#if defined CHIP_HI3559V100 || defined CHIP_HI3519V101
	uvc_us_to_timeval(pVBuf->u64pts, &v4l2_buf.timestamp);
#else
	uvc_us_to_timeval(pVBuf->u64PTS, &v4l2_buf.timestamp);
#endif

	v4l2_buf.bytesused = pVBuf->u32Width * pVBuf->u32Height * 2;
	dst_yuy2 = uvc_mem;

	if (!src_y || !src_vu || !dst_yuy2 || pVBuf->u32Width  <= 0 || pVBuf->u32Height <= 0) {

		ucam_error("error!!!");
		goto error2;
	} 

	ret = UVC_NV21ToYUY2(src_y,
               src_stride_y,
               src_vu,
               src_stride_vu,
               dst_yuy2,
               dst_stride_yuy2,
               pVBuf->u32Width,
               pVBuf->u32Height);
	if(ret != 0)
	{
		ucam_error("UVC_NV21ToYUY2 error, ret=%d", ret);
		goto error2;
	}

	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}
	
	ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		//goto error2;
	}

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return ret;

error:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}
	return -1;

error2:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
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

	return -2;
}

/**
 * @description: UVC将yuv sp420视频转换成NV12然后发给USB主机
 * @param {uvc:句柄, uvc_stream_num:第几路UVC码流} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_hisi_nv21_to_nv12_mmap(struct uvc_dev *dev, void* pBuf)
{
	int ret;
	struct v4l2_buffer v4l2_buf;
	void *uvc_mem = NULL;

	const uint8_t* src_y;
	const uint8_t* src_vu;


	uint32_t phy_addr_size;
	unsigned long long phy_addr;
	void *phy_virt_addr;

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

	if(dev->fcc != V4L2_PIX_FMT_NV12)
	{
		ucam_error("uvc format error, uvc format:%s",
		 uvc_video_format_string(dev->fcc));
		return -1;	
	}

	if(pVBuf->u32Width != dev->width || pVBuf->u32Height != dev->height)
	{
		ucam_error("frame width  height error! CAM:%dx%d, UVC:%dx%d", pVBuf->u32Width, pVBuf->u32Height, dev->width, dev->height);
		return -1;
	}

	phy_addr_size = pVBuf->u32Stride[0] * pVBuf->u32Height * 3/2; 
#if defined CHIP_HI3519V101 || defined CHIP_HI3559V100
	phy_addr = pVBuf->u32PhyAddr[0];
#else
	phy_addr = pVBuf->u64PhyAddr[0];	
#endif
	phy_virt_addr = (uint8_t *)dev->uvc->uvc_mmap_cache_callback(dev, phy_addr, phy_addr_size);	
	if (NULL == phy_virt_addr)
	{    
		ucam_error("mmap error, addr:0x%xll, size:%d", phy_addr, phy_addr_size);
		goto error;
	}
	src_y = phy_virt_addr;
	src_vu = src_y + pVBuf->u32Stride[0] * pVBuf->u32Height;

	ret = uvc_video_dqbuf_lock(dev, dev->config.venc_chn, &v4l2_buf, &uvc_mem);
	if(ret != 0)
	{
		ucam_error("uvc_video_dqbuf_lock error, ret=%d", ret);
		goto error;
	}

	void (*uvc_vu_rev2uv_row)(const uint8_t* src_vu, uint8_t* dst_uv, int width) = UVC_UVSwapRow_NEON_16; 
	if(IS_ALIGNED(pVBuf->u32Width, 16) == 0)
		uvc_vu_rev2uv_row = UVC_UVSwapRow_Any_NEON;

	uvc_memcpy_neon(uvc_mem, src_y, pVBuf->u32Width * pVBuf->u32Height);
	uvc_vu_rev2uv_row(src_vu, uvc_mem + pVBuf->u32Width * pVBuf->u32Height, pVBuf->u32Height);
	v4l2_buf.bytesused = pVBuf->u32Width * pVBuf->u32Height*3/2;
#if defined CHIP_HI3559V100 || defined CHIP_HI3519V101
	uvc_us_to_timeval(pVBuf->u64pts, &v4l2_buf.timestamp);
#else
	uvc_us_to_timeval(pVBuf->u64PTS, &v4l2_buf.timestamp);
#endif

	if(v4l2_buf.bytesused > v4l2_buf.length)
	{
		ucam_error("buf_length_error ! , buf.bytesused:%d buf.length:%d\n", v4l2_buf.bytesused, v4l2_buf.length);
		v4l2_buf.bytesused = 0;
	}

	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}
	
	ret = uvc_video_qbuf_nolock(dev, dev->config.venc_chn, &v4l2_buf);
	if(ret != 0)
	{
		ucam_error("uvc_video_qbuf_lock error, ret=%d", ret);
		//goto error2;
	}

	pthread_mutex_unlock(&dev->uvc_reqbufs_mutex);

	return 0;

error:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
	}
	return -1;

#if 0
error2:
	if(phy_virt_addr)
	{	
		ret = dev->uvc->uvc_mflush_cache_callback(dev, phy_addr, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_mflush_cache_callback error, ret=0x%x", ret);
		}
   		ret = dev->uvc->uvc_munmap_callback(dev, phy_virt_addr, phy_addr_size);
		if(ret != 0)
		{
			ucam_error("uvc_munmap_callback error, ret=0x%x", ret);
		}
		phy_virt_addr = NULL;
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

	return -2;
#endif
}


int uvc_send_stream_ops_int(struct uvc_dev *dev)
{
	switch (dev->config.reqbufs_memory_type)
	{
	case V4L2_REQBUFS_MEMORY_TYPE_MMAP:
		dev->send_stream_ops.send_encode = uvc_send_stream_hisi_encode_mmap_userptr;
		dev->send_stream_ops.send_yuy2 = uvc_send_stream_hisi_nv21_to_yuy2_mmap;
		dev->send_stream_ops.send_nv12 = uvc_send_stream_hisi_nv21_to_nv12_mmap;
		break;
	case V4L2_REQBUFS_MEMORY_TYPE_USERPTR:
		dev->send_stream_ops.send_encode = uvc_send_stream_hisi_encode_mmap_userptr;
		dev->send_stream_ops.send_yuy2 = uvc_send_stream_hisi_nv21_to_yuy2_userptr;
		dev->send_stream_ops.send_nv12 = uvc_send_stream_hisi_nv21_to_nv12_userptr;
		break;	
	default:
		//不支持其他的类型
		return -1;
	}

	//uvc_set_uvc_pts_copy_enable(dev, 1);
	return 0;
}

#endif /*#ifdef HISI_SDK*/