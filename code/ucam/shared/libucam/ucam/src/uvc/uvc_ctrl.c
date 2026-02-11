/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uvc\uvc_ctrl.c
 * @Description : 
 */ 
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_vs.h>

extern struct uvc_ctrl_attr_t uvc_ctrl_ct;
extern struct uvc_ctrl_attr_t uvc_ctrl_pu;
extern struct uvc_ctrl_attr_t uvc_ctrl_vc;
extern struct uvc_ctrl_attr_t uvc_ctrl_eu;
extern struct uvc_ctrl_attr_t uvc_ctrl_xu_h264;
extern struct uvc_ctrl_attr_t uvc_ctrl_xu_h264_s2;
extern struct uvc_ctrl_attr_t uvc_ctrl_xu_ct;

//extern struct uvc_ctrl_attr_t uvc_ctrl_xu_menu;
//extern struct uvc_ctrl_attr_t uvc_ctrl_xu_custom_cmd;

#define UVC_CTRL_ATTRS_XU_H264_INDEX    5
#define UVC_CTRL_ATTRS_XU_H264_S2_INDEX 6

const struct uvc_ctrl_attr_t * uvc_ctrl_attrs[] = {
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_ct,
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_pu,
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_vc,
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_eu,
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_xu_ct,
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_xu_h264,
    (const struct uvc_ctrl_attr_t *) &uvc_ctrl_xu_h264_s2,  
    //(const struct uvc_ctrl_attr_t *) &uvc_ctrl_xu_menu,
    //(const struct uvc_ctrl_attr_t *) &uvc_ctrl_xu_custom_cmd,
    NULL
};

int uvc_add_xu_ctrl_list(struct uvc *uvc,
        struct uvc_xu_ctrl_attr_t *uvc_xu_ctrl_attr)
{
    if (uvc == NULL || uvc_xu_ctrl_attr == NULL)
    {
        return -1;
    }

	//如果注册了外部的XU H264则采用外部的定义
    if (uvc_xu_ctrl_attr->ctrl_attr->id == 0x0C ||
        uvc_xu_ctrl_attr->ctrl_attr->id == 0x0D)
    {
        uvc->config.uvc_xu_h264_ctrl_enable = 0;
        uvc_ctrl_attrs[UVC_CTRL_ATTRS_XU_H264_INDEX] = NULL;
        uvc_ctrl_attrs[UVC_CTRL_ATTRS_XU_H264_S2_INDEX] = NULL;
    }

    ucam_list_add_tail(&uvc_xu_ctrl_attr->list, &uvc->uvc_xu_ctrl_list);

    return 0;
}

int uvc_ctrl_init(struct uvc_dev *dev, const struct uvc_ctrl_attr_t *const *ctrl_attrs)
{
    const struct uvc_ctrl_attr_t *const *src;
    struct ucam_list_head *pos;
    const struct uvc_xu_ctrl_attr_t *uvc_xu_ctrl_attr;

    ucam_strace("enter");

    get_uvc_ctrl_vs()->init(dev);

    if (ctrl_attrs == NULL)
    {
        dev->uvc->uvc_ctrl_attrs = (const struct uvc_ctrl_attr_t *const *)uvc_ctrl_attrs;
    }
    else
    {
        dev->uvc->uvc_ctrl_attrs = ctrl_attrs;
    }
    if (dev->uvc->uvc_ctrl_attrs == NULL)
    {
        ucam_error("uvc_ctrl_init error, uvc_ctrl_attrs == NULL");
        return -1;
    }

    for (src = (const struct uvc_ctrl_attr_t **)dev->uvc->uvc_ctrl_attrs; *src; ++src)
    {
        // ucam_strace("p: %p", *src);
        if ((*src)->init)
            (*src)->init(dev);
    }

    ucam_list_for_each(pos, &dev->uvc->uvc_xu_ctrl_list)
    {
        uvc_xu_ctrl_attr = ucam_list_entry(pos, struct uvc_xu_ctrl_attr_t, list);
        if (uvc_xu_ctrl_attr)
        {
            if (!uvc_xu_ctrl_attr->disable && uvc_xu_ctrl_attr->ctrl_attr)
            {
                if (uvc_xu_ctrl_attr->ctrl_attr->init)
                    uvc_xu_ctrl_attr->ctrl_attr->init(dev);
            }
        }
    }

    return 0;
}

static UvcRequestErrorCode_t uvc_events_process_uvcctrl(struct uvc_dev *dev,
                                                        struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data)
{
    UvcRequestErrorCode_t ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
    uint8_t id = ctrl->wIndex >> 8;
    uint8_t cs = ctrl->wValue >> 8;
    const struct uvc_ctrl_attr_t *const *src;
    struct uvc_ctrl_attr_t *uvc_ctrl = NULL;
    struct ucam_list_head *pos;
    const struct uvc_xu_ctrl_attr_t *uvc_xu_ctrl_attr;

    ucam_strace("enter");

    if (id == UVC_ENTITY_ID_LOGITECH_UPGRADE) // for WIN 10 罗技驱动会用影响
    {
        if (ctrl->wValue == 0x0100 && ctrl->bRequest == UVC_GET_CUR)
        {
            if (ctrl->bRequest == UVC_GET_CUR)
            {
                ((uint8_t *)data->data)[0] = 0x00;
                ((uint8_t *)data->data)[1] = 0x08;
                ((uint8_t *)data->data)[2] = 0x94;
                ((uint8_t *)data->data)[3] = 0x03;
                data->length = ctrl->wLength;
                ret = 0;
            }
            else if (ctrl->bRequest == UVC_GET_LEN)
            {
                ((uint32_t *)data->data)[0] = 4;
                data->length = ctrl->wLength;
                ret = 0;
            }
            else
                ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
        }
        else
            ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;

        return ret;
    }

    for (src = (const struct uvc_ctrl_attr_t **)dev->uvc->uvc_ctrl_attrs; *src; ++src)
    {
        if (id == (*src)->id)
        {
            uvc_ctrl = (struct uvc_ctrl_attr_t *)*src;
            break;
        }
    }

    if (uvc_ctrl == NULL)
    {
        // return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
        goto uvc_xu_ctrl_handle;
    }

    if (cs >= uvc_ctrl->selectors_max_size)
    {
        // return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
        goto uvc_xu_ctrl_handle;
    }

    if (uvc_ctrl->requests_ctrl_fn[cs] == NULL)
    {
        // return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
        goto uvc_xu_ctrl_handle;
    }

    if (event_data && data->length == 0 /*< ctrl->wLength*/)
    {
        ucam_error("EP0 data length error, data:%d, request:%d", data->length, ctrl->wLength);
        return ret;
    }
    ret = uvc_ctrl->requests_ctrl_fn[cs](dev, ctrl, data, event_data);
    if (ret != UVC_REQUEST_ERROR_CODE_NO_ERROR)
    {
        ucam_error("uvcctrl error, ret:%d", ret);
    }

    return ret;

uvc_xu_ctrl_handle:
    ucam_list_for_each(pos, &dev->uvc->uvc_xu_ctrl_list)
    {
        uvc_xu_ctrl_attr = ucam_list_entry(pos, struct uvc_xu_ctrl_attr_t, list);
        if (uvc_xu_ctrl_attr)
        {
            if (!uvc_xu_ctrl_attr->disable && uvc_xu_ctrl_attr->ctrl_attr)
            {
                if (id == uvc_xu_ctrl_attr->ctrl_attr->id)
                {
                    if (cs >= uvc_xu_ctrl_attr->ctrl_attr->selectors_max_size)
                    {
                        return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
                    }

                    if (uvc_xu_ctrl_attr->ctrl_attr->requests_ctrl_fn[cs] == NULL)
                    {
                        return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
                    }

                    ret = uvc_xu_ctrl_attr->ctrl_attr->requests_ctrl_fn[cs](dev, ctrl, data, event_data);
                    if (ret != UVC_REQUEST_ERROR_CODE_NO_ERROR)
                    {
                        ucam_error("uvcctrl error:%s", uvc_request_error_code_string(ret));
                    }
                    return ret;
                }
            }
        }
    }

    return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
}

static UvcRequestErrorCode_t uvc_events_process_uvcstreaming( struct uvc_dev *dev, 
    struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
    UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
    uint8_t 	cs = ctrl->wValue >> 8;

    ucam_strace("enter");

    if(cs >= get_uvc_ctrl_vs()->selectors_max_size)
    {
        return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
    }

    if(get_uvc_ctrl_vs()->requests_ctrl_fn[cs] == NULL)
    {
        return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
    }
    
    if(event_data && data->length == 0/*< ctrl->wLength*/)
    {
        ucam_error("EP0 data length error, data:%d, request:%d", data->length , ctrl->wLength);
        return ret;
    }
    ret = get_uvc_ctrl_vs()->requests_ctrl_fn[cs](dev, ctrl, data, event_data);
    if(ret != UVC_REQUEST_ERROR_CODE_NO_ERROR)
    {
        ucam_error("uvcstreaming error:%s", uvc_request_error_code_string(ret));
    }

    return ret;
}


UvcRequestErrorCode_t uvc_requests_ctrl ( struct uvc_dev *dev, 
    struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data )
{
	UvcRequestErrorCode_t 	ret = UVC_REQUEST_ERROR_CODE_NO_ERROR;
    unsigned int  intf = ctrl->wIndex & 0xff;
    uint8_t		id = ctrl->wIndex >> 8;
    uint8_t 	cs = ctrl->wValue >> 8;

    ucam_strace("%s: intf:%d, id:%d, cs:%d, length:%d", uvc_request_to_string(ctrl->bRequest), intf, id, cs, ctrl->wLength);

    //if(ctrl->wLength > EP0_DATA_SIZE)
    if(ctrl->wLength > UVC_EP0_DATA_LEN)
    {
        return UVC_REQUEST_ERROR_CODE_INVALID_REQUEST;
    }	
	else
    {
        if(!event_data)
        {
            data->length = ctrl->wLength;
            dev->uvc_set_ctrl = *ctrl;   
        }   
        else 
        {
            if(ctrl->bRequest != UVC_SET_CUR)
                return UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;  
        }
    }
		

    if(intf == dev->control_intf)
    {
        ret = uvc_events_process_uvcctrl(dev, ctrl, data, event_data);
    }
    else if(intf == dev->streaming_intf)
    {
        ret = uvc_events_process_uvcstreaming(dev, ctrl, data, event_data);	
    }
    else
    {
        ret = UVC_REQUEST_ERROR_CODE_INVALID_CONTROL;
    }

    if(ret != UVC_REQUEST_ERROR_CODE_NO_ERROR)
    {
        ucam_error("uvc_requests_ctrl error:%s", uvc_request_error_code_string(ret));
    }

	return ret;
}
