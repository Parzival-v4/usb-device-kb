/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_config.c
 * @Description : 
 */ 
#include <linux/usb/ch9.h>
//#include <linux/usb/gadget.h>

#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uac/uac_device.h>
#include <ucam/uvc/uvc_api_vs.h>

extern int uvc_copy_descriptors(struct uvc_dev *dev, enum ucam_usb_device_speed speed, struct uvc_ioctl_desc_t *descriptor);
	
int uvc_ioctl_get_uvc_config(struct uvc_dev *dev , struct uvc_ioctl_uvc_config_t *uvc_config)
{
	ucam_strace("uvc_ioctl_get_uvc_config");
	
	if (ioctl(dev->fd, UVCIOC_GET_UVC_CONFIG, uvc_config) < 0) {
		
		ucam_error("get_uvc_config error!");
		return -1;
	}	
	
	return 0;
}


int uvc_ioctl_set_uvc_descriptor(struct uvc_dev *dev, struct uvc_ioctl_desc_t *descriptor)
{
	ucam_notice("uvc_ioctl_set_uvc_descriptor, config : %d, length : %d",dev->bConfigurationValue, descriptor->desc_buf_length);


	if (ioctl(dev->fd, UVCIOC_SET_UVC_DESCRIPTORS, descriptor) < 0) {
		ucam_error("set uvc descriptor error!");
		return -1;
	}
	return 0;
}


int uvc_ioctl_set_uvc_desc_buf_mode(struct uvc_dev *dev, struct uvc_ioctl_desc_t *descriptor)
{
	struct uvc_ioctl_desc_buf_t uvc_desc_buf;
	void *desc_mem = NULL;
	
	struct usb_descriptor_header 	**dst;
	void *mem = NULL;
	void *kmem = NULL;
	unsigned int n_desc_length;
	unsigned int  descriptors_length;

	int i = 0;

	n_desc_length = (descriptor->n_desc + 1) * sizeof(const struct usb_descriptor_header *);
	descriptors_length = n_desc_length + descriptor->desc_buf_length;

	ucam_notice("uvc_ioctl_set_uvc_descriptor, config : %d, length : %d",dev->bConfigurationValue, descriptors_length);

	memset(&uvc_desc_buf, 0 , sizeof(struct uvc_ioctl_desc_buf_t));
	uvc_desc_buf.speed = descriptor->speed;
	uvc_desc_buf.total_length = descriptors_length;


	//先获取内核的虚拟地址
	if (ioctl(dev->fd, UVCIOC_SET_UVC_DESC_BUF_MODE, &uvc_desc_buf) < 0) {
		ucam_error("ioctl error!");
	}

	if(uvc_desc_buf.mem == NULL)
	{
		goto error_free;
	}

	desc_mem = malloc(descriptors_length);
	if(desc_mem == NULL)
	{
		ucam_error("malloc error!");
		return -1;
	}
	memset(desc_mem, 0, descriptors_length);
	mem = desc_mem;
	kmem = uvc_desc_buf.mem + n_desc_length;
	dst = mem;
	mem += n_desc_length;

	UVC_COPY_DESCRIPTORS_FROM_BUF_TO_KERNEL(kmem, mem, dst, descriptor->desc_buf, descriptor->desc_buf_length);

	for(i = 0; i < descriptors_length;)
	{
		uvc_desc_buf.mem_offset = i;
		uvc_desc_buf.buf_length = descriptors_length - i;
		if(uvc_desc_buf.buf_length > UVC_DESC_BUF_LEN)
		{
			uvc_desc_buf.buf_length = UVC_DESC_BUF_LEN;
		}
		memcpy(uvc_desc_buf.buf, desc_mem + uvc_desc_buf.mem_offset, uvc_desc_buf.buf_length);
		i += uvc_desc_buf.buf_length;
		if (ioctl(dev->fd, UVCIOC_SET_UVC_DESC_BUF_MODE, &uvc_desc_buf) < 0) {
			ucam_error("ioctl error!");
			goto error_free;
		}
	}

	free(desc_mem);
	return 0;

error_free:
	ucam_error("set uvc descriptor error!");
	if(desc_mem)
	{
		free(desc_mem);
	}

	return -1;
}

int reset_uvc_desc_buf_mode(struct uvc_dev *dev)
{
	int ret = 0;
	struct uvc_ioctl_desc_t descriptor;
	void *p = NULL;
	
	
	p = malloc(DESC_BUF_MAX_LENGTH);
	if(p == NULL)
	{
		ucam_error("malloc error!");
		return -1;
	}
	memset(p, 0, DESC_BUF_MAX_LENGTH);

	descriptor.desc_buf = p;
	descriptor.desc_buf_phy_length = DESC_BUF_MAX_LENGTH;
	
	if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_FULL, &descriptor) < 0)
	{
		ucam_error("UCAM_USB_SPEED_FULL uvc_copy_descriptors error!");
		goto error_free;
	}

	ret = uvc_ioctl_set_uvc_desc_buf_mode(dev, &descriptor);
	if(ret < 0)
	{
		ucam_error("uvc_ioctl_set_uvc_desc_buf_mode error!");
		goto error_free;
	}
	

	if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_HIGH, &descriptor) < 0)
	{
		ucam_error("UCAM_USB_SPEED_HIGH uvc_copy_descriptors error!");
		goto error_free;
	}


	ret = uvc_ioctl_set_uvc_desc_buf_mode(dev, &descriptor);
	if(ret < 0)
	{
		ucam_error("uvc_ioctl_set_uvc_desc_buf_mode error!");
		goto error_free;
	}
	

	if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
	{
		if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_SUPER, &descriptor)  < 0)
		{
			ucam_error("UCAM_USB_SPEED_SUPER uvc_copy_descriptors error!");
			goto error_free;
		}

		
		ret = uvc_ioctl_set_uvc_desc_buf_mode(dev, &descriptor);
		if(ret < 0)
		{
			ucam_error("uvc_ioctl_set_uvc_desc_buf_mode error!");
			goto error_free;
		}

		if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER_PLUS)
		{
			if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_SUPER_PLUS, &descriptor)  < 0)
			{
				ucam_error("UCAM_USB_SPEED_SUPER_PLUS uvc_copy_descriptors error!");
				goto error_free;
			}

			
			ret = uvc_ioctl_set_uvc_desc_buf_mode(dev, &descriptor);
			if(ret < 0)
			{
				ucam_error("uvc_ioctl_set_uvc_desc_buf_mode error!");
				goto error_free;
			}	
		}
	}

	free(p);
	return 0;

error_free:
	if(p)
	{
		free(p);
	}
	return -1;
}


int reset_uvc_descriptor(struct uvc_dev *dev, int controls_desc_copy, int streaming_desc_copy)
{
	int ret = 0;
	struct uvc_ioctl_desc_t descriptor;
	
	ucam_notice("reset_uvc_descriptor");
	
	//有些修改是只修改了内存中的配置(例如翻转分辨率的宽高)，不需要将配置重新拷贝到内存中
	ret = uvc_streaming_desc_init(dev, controls_desc_copy, streaming_desc_copy);
	if(ret != 0)
	{
		ucam_notice("uvc_streaming_desc_init error, ret:%d", ret);
		return ret;
	}

#if 1//!(defined SIGMASTAR_SDK || defined HISI_SDK || defined GK_SDK)
	return reset_uvc_desc_buf_mode(dev);
#endif

	if(dev->uvc->uvc_mmz_alloc_callback == NULL)
	{
		ucam_error("uvc_mmz_alloc_callback no support");
		return reset_uvc_desc_buf_mode(dev);
	}
	
	if(dev->uvc->uvc_mmz_alloc_callback)
	{
		descriptor.desc_buf = NULL;
		descriptor.desc_buf_phy_length = DESC_BUF_MAX_LENGTH;
		ret = dev->uvc->uvc_mmz_alloc_callback(dev, (MMZ_PHY_ADDR_TYPE *)&descriptor.desc_buf_phy_addr, ((void**)&descriptor.desc_buf), 
			"uvc temp buf", NULL, descriptor.desc_buf_phy_length);
		if (0 != ret || descriptor.desc_buf == NULL)
		{   
			ucam_error("uvc_mmz_alloc_callback fail, ret = 0x%x",ret);
			return -1;
		}
		memset(descriptor.desc_buf, 0, DESC_BUF_MAX_LENGTH);
	}

	if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_FULL, &descriptor) < 0)
	{
		ucam_error("UCAM_USB_SPEED_FULL uvc_copy_descriptors error!");
		goto done;
	}
	else
	{
		ret = uvc_ioctl_set_uvc_descriptor(dev, &descriptor);
		if(ret < 0)
		{
			ucam_error("uvc_ioctl_set_uvc_descriptor error!");
			goto done;
		}
	}

	if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_HIGH, &descriptor) < 0)
	{
		ucam_error("UCAM_USB_SPEED_HIGH uvc_copy_descriptors error!");
		goto done;
	}
	else
	{
		ret = uvc_ioctl_set_uvc_descriptor(dev, &descriptor);
		if(ret < 0)
		{
			ucam_error("uvc_ioctl_set_uvc_descriptor error!");
			goto done;
		}
	}

	if(dev->uvc->config.usb_speed_max >= UCAM_USB_SPEED_SUPER)
	{
		if(uvc_copy_descriptors(dev, UCAM_USB_SPEED_SUPER, &descriptor)  < 0)
		{
			ucam_error("UCAM_USB_SPEED_SUPER uvc_copy_descriptors error!");
			goto done;
		}
		else
		{
			ret = uvc_ioctl_set_uvc_descriptor(dev, &descriptor);
			if(ret < 0)
			{
				ucam_error("uvc_ioctl_set_uvc_descriptor error!");
				goto done;
			}
		}
	}

done:
	if(descriptor.desc_buf)
	{
		dev->uvc->uvc_mmz_free_callback(dev, descriptor.desc_buf_phy_addr, descriptor.desc_buf, descriptor.desc_buf_phy_length);
	}

	return ret;
}

int reset_usb_sn(struct uvc_dev *dev)
{
	struct usb_sn_header_t usb_sn_header;
	char usb_sn_header_string[32];
	struct uvc_ioctl_usb_sn_string_t usb_sn_string;
	usb_sn_string.string_buff_length = 64;

#if 0
	//如果某些定制客户需要定制SN号，则不能进行重修改
	if(strcmp(get_customer_code(), "H") == 0)
	{
		return 0;
	}
#endif

	if(!dev->uvc->config.reset_usb_sn_enable)
		return 0;

	memset(&usb_sn_header, 0, sizeof(struct usb_sn_header_t ));
	usb_sn_header.usb_bcd = dev->uvc->config.DeviceVersion.bcdDevice;
	usb_sn_header.bmControls.b.uvc_en = 1;//默认支持UVC
	usb_sn_header.bmControls.b.uvc_s2_en = dev->uvc->uvc_s2_enable;
	usb_sn_header.bmControls.b.uvc_v150_en = dev->uvc->uvc_v150_enable;
	usb_sn_header.bmControls.b.hid_vhd_en = 1;//默认支持HID升级
	usb_sn_header.bmControls.b.hid_custom_en = dev->uvc->zte_hid_enable;
	usb_sn_header.bmControls.b.dfu_en = dev->uvc->dfu_enable;
	usb_sn_header.bmControls.b.uac_capture_en = dev->uvc->uac_enable;
	usb_sn_header.bmControls.b.uac_playback_en = 0;
	
	if(dev->uvc->uac_enable)
	{
		if(dev->uvc->ucam->uac != NULL && dev->uvc->ucam->uac->uac_dev != NULL)
		{
			if(dev->uvc->ucam->uac->uac_dev->mic_attrs.samplepsec == 44100)
				usb_sn_header.bmControls.b.reserved = 1;
			else if(dev->uvc->ucam->uac->uac_dev->mic_attrs.samplepsec == 48000)
				usb_sn_header.bmControls.b.reserved = 2;
			else if(dev->uvc->ucam->uac->uac_dev->mic_attrs.samplepsec == 32000)
				usb_sn_header.bmControls.b.reserved = 3;
			else
				usb_sn_header.bmControls.b.reserved = 0;
		}	 
	}
	

	memset(usb_sn_header_string, 0, sizeof(usb_sn_header_string));
	memset(usb_sn_string.string, 0, usb_sn_string.string_buff_length);

	sprintf(usb_sn_header_string, "%04x%04x", usb_sn_header.usb_bcd, usb_sn_header.bmControls.d16);

	//拼接配置信息和SN号
	strcat(usb_sn_string.string, usb_sn_header_string);
	strcat(usb_sn_string.string, dev->uvc->config.usb_serial_number);

	ucam_notice("usb_sn_string: %s\n", usb_sn_string.string);
		
	if (ioctl(dev->fd, UVCIOC_SET_USB_SN, &usb_sn_string) < 0) {
		
		ucam_error("ioctl UVCIOC_SET_USB_SN error!");
		return -1;
	}

	return 0;
}

int uvc_ioctl_set_reqbufs_config(struct uvc_dev *dev, struct uvc_ioctl_reqbufs_config_t *reqbufs_config)
{
	//ucam_notice("enable : %d, cache_enable : %d, data_offset : %d",
	//	reqbufs_config->enable, reqbufs_config->cache_enable, reqbufs_config->data_offset);


	if (ioctl(dev->fd, UVCIOC_SET_REQBUFS_CONFIG, reqbufs_config) < 0) {
		ucam_error("set uvc reqbufs_config error!");
		return -1;
	}
	return 0;
}

int uvc_ioctl_set_reqbufs_addr(struct uvc_dev *dev, struct uvc_ioctl_reqbufs_addr_t *reqbufs_addr)
{
	//ucam_notice("index : %d, addr : 0x%llx, length : %d",
	//	reqbufs_addr->index, reqbufs_addr->addr, reqbufs_addr->length);


	if (ioctl(dev->fd, UVCIOC_SET_REQBUFS_ADDR, reqbufs_addr) < 0) {
		ucam_error("set uvc ddr_to_usb error!");
		return -1;
	}
	return 0;
}

int uvc_ioctl_set_reqbufs_buf_used(struct uvc_dev *dev, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used)
{
	//ucam_strace("index : %d, buf_used : %d",reqbufs_buf_used->index, reqbufs_buf_used->buf_used);

	if (ioctl(dev->fd, UVCIOC_SET_REQBUFS_BUF_USED, reqbufs_buf_used) < 0) {
		ucam_error("set ioctl error!");
		return -1;
	}
	return 0;
}

int uvc_ioctl_get_usb_link_state(struct uvc_dev *dev , int *usb_link_state)
{
	//ucam_strace("uvc_ioctl_get_usb_link_state");
	
	if (ioctl(dev->fd, UVCIOC_GET_USB_LINK_STATE, usb_link_state) < 0) {
		
		ucam_error("uvc_ioctl_get_usb_link_state error!");
		return -1;
	}	
	
	return 0;
}


int uvc_receive_ep0_data(struct uvc_dev *dev , struct uvc_ep0_data_t *ep0_data)
{
	uint32_t length;
	uint32_t ret;

	//ucam_strace("length : %d",ep0_data->length);

	length = ep0_data->length;
	ret = ioctl(dev->fd, UVCIOC_RECEIVE_EP0_DATA, ep0_data);
	if (ret != length) {	
		ucam_error("uvc_receive_ep0_data error!");
		return -1;
	}
	
	return 0;
}

int uvc_send_ep0_data(struct uvc_dev *dev , struct uvc_ep0_data_t *ep0_data)
{
	//ucam_strace("length : %d",ep0_data->length);
	
	if (ioctl(dev->fd, UVCIOC_SEND_EP0_DATA, ep0_data) < 0) {
		
		ucam_error("uvc_send_ep0_data error!");
		return -1;
	}	
	
	return 0;
}


int uvc_set_windows_hello_metadata_enable(struct uvc_dev * dev, uint32_t enable)
{
	uint32_t hello_enable = enable;	
	
	if (ioctl(dev->fd, UVCIOC_SET_WINDOWS_HELLO_ENABLE, &hello_enable) < 0) {
		ucam_error("UVCIOC_SET_WINDOWS_HELLO_ENABLE: %s (%d) failed", strerror(errno), errno);	
		return -1;
	}
	return 0;
}

int uvc_set_uvc_pts_copy_enable(struct uvc_dev * dev, uint32_t enable)
{
	uint32_t uvc_pts_copy = enable;	
	
	if (ioctl(dev->fd, UVCIOC_SET_UVC_PTS_COPY, &uvc_pts_copy) < 0) {
		ucam_error("UVCIOC_SET_UVC_PTS_COPY: %s (%d) failed", strerror(errno), errno);	
		return -1;
	}
	return 0;
}
int uvc_set_cma_dma_buf_enable(struct uvc_dev * dev, uint32_t enable)
{
    uint32_t cma_dma_buf_enable = enable; 
    
    if (ioctl(dev->fd, UVCIOC_CMA_DMA_BUF_ENABLE, &cma_dma_buf_enable) < 0) {
        ucam_error("UVCIOC_CMA_DMA_BUF_ENABLE: %s (%d) failed", strerror(errno), errno);  
        return -1;
    }
    return 0;
}

int uvc_cma_dma_buf_alloc(struct uvc_dev * dev, 
    struct uvc_ioctl_cma_dma_buf *cma_dma_buf)
{
    if (ioctl(dev->fd, UVCIOC_CMA_DMA_BUF_ALLOC, cma_dma_buf) < 0) {
        ucam_error("UVCIOC_CMA_DMA_BUF_ALLOC: %s (%d) failed", strerror(errno), errno);  
        return -1;
    }
    return 0;
}

int uvc_cma_dma_buf_free(struct uvc_dev * dev, 
    struct uvc_ioctl_cma_dma_buf *cma_dma_buf)
{
    if (ioctl(dev->fd, UVCIOC_CMA_DMA_BUF_FREE, cma_dma_buf) < 0) {
        ucam_error("UVCIOC_CMA_DMA_BUF_FREE: %s (%d) failed", strerror(errno), errno);  
        return -1;
    }
    return 0;
}

int uvc_set_usb_vendor_string(struct uvc_dev *dev,
	struct uvc_ioctl_usb_vendor_string_t *p_usb_vendor_string)
{
	if(dev == NULL || p_usb_vendor_string == NULL)
	{
		return -ENOMEM;
	}
		
	if (ioctl(dev->fd, UVCIOC_SET_USB_VENDOR_STR, p_usb_vendor_string) < 0) {
		
		ucam_error("ioctl UVCIOC_SET_USB_VENDOR_STR error!");
		return -1;
	}

	return 0;
}

int uvc_set_usb_product_string(struct uvc_dev *dev,
	struct uvc_ioctl_usb_product_string_t *p_usb_product_string)
{
	if(dev == NULL || p_usb_product_string == NULL)
	{
		return -ENOMEM;
	}
		
	if (ioctl(dev->fd, UVCIOC_SET_USB_PRODUCT_STR, p_usb_product_string) < 0) {
		
		ucam_error("ioctl UVCIOC_SET_USB_PRODUCT_STR error!");
		return -1;
	}

	return 0;
}

int uvc_set_usb_sn_string(struct uvc_dev *dev,
	struct uvc_ioctl_usb_sn_string_t *p_usb_sn_string)
{
	if(dev == NULL || p_usb_sn_string == NULL)
	{
		return -ENOMEM;
	}
		
	if (ioctl(dev->fd, UVCIOC_SET_USB_SN, p_usb_sn_string) < 0) {
		
		ucam_error("ioctl UVCIOC_SET_USB_SN error!");
		return -1;
	}

	return 0;
}
