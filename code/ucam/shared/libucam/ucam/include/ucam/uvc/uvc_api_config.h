/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api_config.h
 * @Description : 
 */ 
#ifndef UVC_API_CONFIG_H 
#define UVC_API_CONFIG_H 

#ifdef __cplusplus
extern "C" {
#endif

extern int uvc_ioctl_get_uvc_config(struct uvc_dev *dev , struct uvc_ioctl_uvc_config_t *uvc_config);
extern int uvc_ioctl_set_uvc_descriptor(struct uvc_dev *dev, struct uvc_ioctl_desc_t *descriptor);
extern int reset_uvc_descriptor(struct uvc_dev *dev, int controls_desc_copy, int streaming_desc_copy);
extern int reset_usb_sn(struct uvc_dev *dev);
extern int uvc_ioctl_set_reqbufs_config(struct uvc_dev *dev, struct uvc_ioctl_reqbufs_config_t *reqbufs_config);
extern int uvc_ioctl_set_reqbufs_addr(struct uvc_dev *dev, struct uvc_ioctl_reqbufs_addr_t *reqbufs_addr);
extern int uvc_ioctl_set_reqbufs_buf_used(struct uvc_dev *dev, struct uvc_ioctl_reqbufs_buf_used_t *reqbufs_buf_used);
extern int uvc_ioctl_get_usb_link_state(struct uvc_dev *dev , int *usb_link_state);
extern int uvc_receive_ep0_data(struct uvc_dev *dev , struct uvc_ep0_data_t *ep0_data);
extern int uvc_send_ep0_data(struct uvc_dev *dev , struct uvc_ep0_data_t *ep0_data);
extern int uvc_set_windows_hello_metadata_enable(struct uvc_dev * dev, uint32_t enable);
extern int uvc_set_uvc_pts_copy_enable(struct uvc_dev * dev, uint32_t enable);
extern int uvc_set_cma_dma_buf_enable(struct uvc_dev * dev, uint32_t enable);
extern int uvc_cma_dma_buf_alloc(struct uvc_dev * dev, 
    struct uvc_ioctl_cma_dma_buf *cma_dma_buf);
extern int uvc_cma_dma_buf_free(struct uvc_dev * dev, 
    struct uvc_ioctl_cma_dma_buf *cma_dma_buf);

extern int uvc_set_usb_vendor_string(struct uvc_dev *dev,
	struct uvc_ioctl_usb_vendor_string_t *p_usb_vendor_string);
extern int uvc_set_usb_product_string(struct uvc_dev *dev,
	struct uvc_ioctl_usb_product_string_t *p_usb_product_string);
extern int uvc_set_usb_sn_string(struct uvc_dev *dev,
	struct uvc_ioctl_usb_sn_string_t *p_usb_sn_string);

#ifdef __cplusplus
}
#endif

#endif /*UVC_API_CONFIG_H*/