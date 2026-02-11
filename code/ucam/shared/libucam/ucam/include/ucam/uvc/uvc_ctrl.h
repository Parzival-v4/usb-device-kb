/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_ctrl.h
 * @Description : 
 */ 

#ifndef UVC_CTRL_H 
#define UVC_CTRL_H 

//#include <linux/usb/ch9.h>
//#include <ucam/uvc/uvc_device.h>

struct uvc_dev;
struct usb_ctrlrequest;
struct uvc_request_data;

typedef UvcRequestErrorCode_t (*uvc_requests_ctrl_fn_t) (struct uvc_dev *dev, struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data);

typedef int (*uvc_ctrl_api_fn_t) (struct uvc *uvc, uint8_t *data, uint32_t data_len);

#define UVC_CRTL_S(s) s##_ctrl_s

#define DECLARE_UVC_CRTL_S(s) \
struct UVC_CRTL_S(s) {	\
	struct s cur; \
	struct s min; \
	struct s max; \
	struct s def; \
	struct s res; \
}

struct uvc_ctrls_api_fn_t
{
	uvc_ctrl_api_fn_t set_cur;
	uvc_ctrl_api_fn_t get_cur;
	uvc_ctrl_api_fn_t get_min;
	uvc_ctrl_api_fn_t get_max;
	uvc_ctrl_api_fn_t get_def;
    uvc_ctrl_api_fn_t get_res;	
};


struct uvc_requests_ctrl_t
{
	uint32_t selector_id;
	uvc_requests_ctrl_fn_t *requests_ctrl_fn; 
};

struct uvc_ctrl_attr_t
{
	uint8_t id;
	uint32_t selectors_max_size;
    int (*init)(struct uvc_dev *dev);
	//struct uvc_requests_ctrl_t uvc_requests_ctrl;
	uvc_requests_ctrl_fn_t *requests_ctrl_fn; 
};


struct uvc_xu_ctrl_attr_t
{
	const struct uvc_descriptor_header *desc;
	const struct uvc_ctrl_attr_t *ctrl_attr;
    uint8_t disable;
    struct ucam_list_head list;
};


#endif /* UVC_CTRL_H */