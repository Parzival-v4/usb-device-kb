/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_api.h
 * @Description : 
 */ 
#ifndef UVC_API_H 
#define UVC_API_H 

#ifdef __cplusplus
extern "C" {
#endif

extern const char *usb_speed_string(enum ucam_usb_device_speed usb_speed);
extern const char *uvc_request_to_string(uint8_t bRequest);
extern const char *uvc_request_error_code_string(UvcRequestErrorCode_t ErrorCode);
extern const char *uvc_video_format_string(uint32_t fcc);

extern int uvc_ctrl_init(struct uvc_dev *dev, const struct uvc_ctrl_attr_t * const *ctrl_attrs);
extern UvcRequestErrorCode_t uvc_requests_ctrl ( struct uvc_dev *dev, 
    struct usb_ctrlrequest *ctrl, struct uvc_request_data *data, int event_data );

/**
 * @description: 设置UVC申请帧缓存的方式，一定要在create_uvc模块初始化之前设置
 * @param 
	{uvc_frame_buf_mode_e :
		UVC_FRAME_BUF_MODE_DYNAMIC: 打开码流时申请内存，关闭码流时释放内存 , 批量模式会在USB连接时申请
		UVC_FRAME_BUF_MODE_STATIC: 初始化时申请内存，去初始化时释放内存
		UVC_FRAME_BUF_MODE_DYNAMIC_USB_CONNECT: USB连接时申请内存，USB断开时释放内存 
		UVC_FRAME_BUF_MODE_STATIC_USB_CONNECT: USB连接时申请内存，去初始化时释放内存
	} 
 * @return: true:success; NULL:error
 * @author: QinHaiCheng
 */
int set_uvc_frame_buf_mode(enum uvc_frame_buf_mode_e mode);


/**
 * @description: 创建uvc句柄和内存
 * @param {uvc_video_stream_max:支持最大多少路码流，目前最大支持2路码流} 
 * @return: true:success; NULL:error
 * @author: QinHaiCheng
 */
struct uvc * create_uvc(struct ucam *ucam, uint32_t uvc_video_stream_max);

/**
 * @description: 释放uvc句柄和关闭设备节点
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_free(struct uvc *uvc);

/**
 * @description: 注册V4L2 video设备节点
 * @param {video_dev_index：设备节点号；stream_num：第几路码流，0-第一路，1-第二路} 
 * @return: true:success; NULL:error
 * @author: QinHaiCheng
 */
struct uvc_dev *uvc_add_dev(struct uvc *uvc, uint8_t video_dev_index, uint32_t stream_num);

/**
 * @description: 添加UVC扩展命令到list
 * @param {uvc_xu_ctrl_attr：扩展命令参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_add_xu_ctrl_list(struct uvc *uvc, struct uvc_xu_ctrl_attr_t *uvc_xu_ctrl_attr);

/**
 * @description: 将uvc参数设置为默认值，注册设备的时候会自动调用一次
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_set_dev_config_default(struct uvc_dev *dev);

/**
 * @description: 获取uvc设备的配置参数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_dev_config(struct uvc_dev *dev, struct uvc_dev_config *dev_config);

/**
 * @description: 设置uvc设备的配置参数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_set_dev_config(struct uvc_dev *dev, struct uvc_dev_config *dev_config);

/**
 * @description: uvc设备初始化
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_dev_init(struct uvc_dev *dev);

/**
 * @description: 初始化所有已经注册的uvc设备
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_dev_init_all(struct uvc *uvc);

/**
 * @description: 打开usb连接
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_usb_connet (struct uvc *uvc);

/**
 * @description: 断开usb连接后再重新连接usb
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_usb_reconnet (struct uvc *uvc);

/**
 * @description: 断开usb连接
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_usb_disconnet (struct uvc *uvc);

/**
 * @description: 获取usb的速率
 * @param {usb_speed：enum ucam_usb_device_speed} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_usb_speed(struct uvc *uvc, uint32_t *usb_speed);

/**
 * @description: 获取usb的连接状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_usb_connet_status(struct uvc *uvc, uint32_t *connet_status);

/**
 * @description: 获取当前工作在哪个usb配置下
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_usb_config_index(struct uvc *uvc, uint32_t *config_index);

/**
 * @description: 获取uvc当前协议版本
 * @param {uvc_protocol：0x0100-uvc1.0, 0x0110-uvc1.1, 0x0150-uvc1.5} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_protocol(struct uvc *uvc, uint32_t *uvc_protocol);

/**
 * @description: 动态设置uvc当前协议版本，要USB初始化后才允许设置
 * @param {uvc_protocol：0x0100-uvc1.0, 0x0110-uvc1.1, 0x0150-uvc1.5} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_set_protocol(struct uvc *uvc, uint32_t uvc_protocol);

/**
 * @description: 发送EP0零数据包
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int usb_ep0_set_zlp(struct uvc_dev *dev);

/**
 * @description: 发送EP0 STALL
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int usb_ep0_set_halt(struct uvc_dev *dev);

/**
 * @description: 发送uvc控制状态，UVC中断端点打开后才有效
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_video_control_status(struct uvc_dev *dev, uint8_t *data, uint32_t len);

/**
 * @description: 发送uvc控制失败状态，UVC中断端点打开后才有效
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_video_control_status_failure(struct uvc_dev *dev, struct usb_ctrlrequest *ctrl);

/**
 * @description: 主动关闭所有的uvc码流
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int32_t uvc_streamoff_all(struct uvc_dev *dev);

/**
 * @description: 主动释放所有的uvc buf
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int32_t uvc_free_all_reqbufs(struct uvc_dev *dev);

/**
 * @description: 设置UVC USB3.0 ISOC快速发送模式
 * @param {mode：1-打开，0-关闭} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_usb3_isoc_speed_fast_mode(struct uvc *uvc, int mode);

/**
 * @description: 屏蔽uvc描述符的分辨率，在uvc初始化之前设置才有效
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_bypass_resolution(uint16_t width, uint16_t height);

/**
 * @description: 清除掉被屏蔽的UVC分辨率
 * @param 	width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_clear_bypass_resolution(uint16_t width, uint16_t height);

/**
 * @description: uvc描述符分辨率恢复默认设置
 * @param 	width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_restore_bypass_resolution(uint16_t width, uint16_t height);

/**
* @description: 清除所有屏蔽的uvc描述符分辨率
* @param {type} 
* @return: 0:success; other:error
* @author: HuXiang
*/
int uvc_clear_all_bypass_resolution(void);


/**
 * @description: 所有屏蔽的uvc描述符分辨率恢复默认设置
 * @return: 0:success; other:error
 */
int uvc_restore_all_bypass_resolution(void);


/**
 * @description: 屏蔽指定的视频分辨率
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * 			width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_bypass_specific_resolution(int speed, 
	int config, int stream_num, uint32_t fcc, 
	uint16_t width, uint16_t height);

/**
 * @description: 使能指定的视频分辨率
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * 			width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_clear_bypass_specific_resolution(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height);

/**
 * @description: 指定的视频分辨率使能恢复默认设置
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * 			width:分辨率宽
 * 			height：分辨率高
 * @return: 0:success; other:error
 */
int uvc_restore_bypass_specific_resolution(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height);

/**
 * @description: 设置UVC默认的视频格式
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_uvc_default_format(enum ucam_usb_device_speed speed, uint32_t fcc);

/**
 * @description: 设置UVC指定默认的视频格式, 在UVC初始化前设置才有用
 * @param 	speed:USB的速率, -1为所有的速率都被设置
 * 			config：第几个USB配置, 1第一个配置，2第二个配置，-1为所有的配置都被设置
 * 			stream_num：第几路码流，1第一路码流，2第二路码流，-1为所有的码流都被设置
 * 			fcc：视频格式
 * @return: 0:success; other:error
 */
int set_uvc_specific_default_format(int speed, 
	int config, int stream_num, uint32_t fcc);

/**
 * @description: 设置UVC默认视频格式的宽高, 在UVC初始化前设置才有用
 * @param speed:USB的速率, -1为所有的速率都被设置
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_uvc_default_width_height_fps(int speed, 
	uint16_t width, uint16_t height, uint32_t fps);

/**
 * @description: 设置UVC默认视频格式的宽高, 在UVC初始化前设置才有用
 * @param speed:USB的速率, -1为所有的速率都被设置
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */	
int set_uvc_default_format_v2(int speed, 
	uint32_t fcc, uint16_t width, uint16_t height, uint32_t fps);

/**
 * @description: 设置UVC默认视频格式的宽高, 在UVC初始化前设置才有用
 * @param speed:USB的速率, -1为所有的速率都被设置；width, height, fps只跟speed有关，跟config, stream_num无关
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_uvc_specific_default_format_v2(int speed, 
	int config, int stream_num, uint32_t fcc,
	uint16_t width, uint16_t height, uint32_t fps);
	
/**
 * @description: 注册usb_connet_event回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_register_usb_connet_event_callback(struct uvc *uvc, usb_connet_event_callback_t callback);

/**
 * @description: 注册stream_event回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_register_stream_event_callback(struct uvc *uvc, uvc_stream_event_callback_t callback);

/**
 * @description: 注册uvc控制命令回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_register_uvc_requests_ctrl_event_callback(struct uvc *uvc, uvc_requests_ctrl_fn_t callback);

/**
 * @description: 注册uvc usb suspend回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_register_usb_suspend_event_callback(struct uvc *uvc, uvc_usb_suspend_event_callback_t callback);

/**
 * @description: 注册uvc usb resume回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_register_usb_resume_event_callback(struct uvc *uvc, uvc_usb_resume_event_callback_t callback);

/**
 * @description: 注册uvc DMA拷贝回调函数
 * @param {align 数据长度对齐的边界大小} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_register_dam_memcpy_callback(struct uvc *uvc, uvc_dam_memcpy_callback_t callback, uint32_t align);


/**
 * @description: 获取码流的打开状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_stream_on_status(struct uvc *uvc, uint32_t uvc_stream_num, uint32_t *stream_on_status);

/**
 * @description: 获取所有码流的打开状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_get_uvc_all_stream_on_status(struct uvc *uvc, uint32_t *stream_on_status);

/**
 * @description: 发送uvc视频数据，必须在stream on状态下去才允许发送
 * @param {buf：数据的指针；buf_size：数据的大小} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream(struct uvc_dev *dev, int32_t venc_chn, 
	const void *buf, uint32_t buf_size,
	uint64_t u64Pts);

/**
 * @description: 发送uvc视频数据，必须在stream on状态下去才允许发送
 * @param {extern_cp：外部拷贝函数指针} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_extern_cp(struct uvc_dev *dev, int32_t venc_chn, 
	const void *handle,
	int (*extern_cp)(const void *handle, struct uvc_dev *dev, void *uvc_buf, struct v4l2_buffer *v4l2_buf),
	uint64_t u64Pts);
/**
 * @description: 设置第一个USB配置的UVC USB3.0 ISOC win7模式, ucam初始化完成后调用才有效
 * @param {usb3_isoc_win7_mode：1--win7, 0--win10} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_set_usb3_isoc_win7_mode(struct uvc *uvc, uint8_t usb3_isoc_win7_mode);

/**
 * @description: 翻转UVC分辨率的宽高
 * @param {flip} 0-正常，1-翻转
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int flip_uvc_frame_width_height(struct uvc *uvc, uint32_t flip);

/**
 * @description: 限制帧率的最大值，当分辨率超过设置的分辨率时按照设置的帧率进行限制，
 * 				 必须在初始化之前调用才生效
 * @param {width * height} 超过这个分辨率时才被限制  {fps_max} 能达到的最大帧率
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int limit_uvc_frame_width_height_fps(uint16_t width, uint16_t height, uint32_t fps_max);

/**
 * @description: 屏蔽掉某些UVC的帧率
 * @param {fps} 支持的有：60 50 30 25 24 20 15 10 7 5
 		  {bypass} 0-不屏蔽，1-屏蔽
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_bypass_fps(uint32_t fps, uint8_t bypass);


#if (defined HISI_SDK)
//海思平台
#include "hi_comm_video.h"
#if defined CHIP_HI3559V100
#include "hi_mapi_venc_define.h"
#define FRAME_DATA_S_DEFINE HI_FRAME_DATA_S
#define VENC_DATA_S_DEFINE	HI_VENC_DATA_S
#else
#include "hi_comm_venc.h"
#define FRAME_DATA_S_DEFINE VIDEO_FRAME_S
#define VENC_DATA_S_DEFINE	VENC_STREAM_S
#endif /* #if defined CHIP_HI3559V100 */

#define uvc_send_stream_hisi_encode uvc_send_stream_encode
#define uvc_send_stream_hisi_nv21_to_yuy2 uvc_send_stream_yuy2
#define uvc_send_stream_hisi_nv21_to_nv12 uvc_send_stream_nv12

#elif (defined GK_SDK)
//国科微
#include "comm_video.h"
#include "comm_venc.h"

#define FRAME_DATA_S_DEFINE VIDEO_FRAME_S
#define VENC_DATA_S_DEFINE	VENC_STREAM_S

#define uvc_send_stream_hisi_encode uvc_send_stream_encode
#define uvc_send_stream_hisi_nv21_to_yuy2 uvc_send_stream_yuy2
#define uvc_send_stream_hisi_nv21_to_nv12 uvc_send_stream_nv12

#elif (defined SIGMASTAR_SDK)
//SIGMASTAR
#include "mi_sys_datatype.h"
#include "mi_venc_datatype.h"
#define FRAME_DATA_S_DEFINE MI_SYS_BufInfo_t
#define VENC_DATA_S_DEFINE	MI_VENC_Stream_t

#elif (defined AMBARELLA_SDK)
//AMBARELLA
struct uvc_yuv_frame_handle {
    uint32_t format;
    uint16_t width;
    uint16_t height;
    void     *buf;
	uint64_t u64PTS;
};

struct uvc_encode_frame_handle {
	uint32_t size;
    void     *buf;
	uint64_t u64PTS;
};

#define FRAME_DATA_S_DEFINE struct uvc_yuv_frame_handle
#define VENC_DATA_S_DEFINE	struct uvc_encode_frame_handle
#else 
//其他平台
#define FRAME_DATA_S_DEFINE void//未支持
#define VENC_DATA_S_DEFINE	void//未支持
#endif

/**
 * @description: 设置UVC发送函数的ops
 * @param {send_stream_ops:ops的指针} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int set_uvc_send_stream_ops(struct uvc_dev *dev, 
	struct uvc_send_stream_ops *send_stream_ops);

/**
 * @description: 将编码视频发给USB主机，必须在stream on状态下去才允许发送
 * @param {venc_chn：venc通道号；fcc：视频格式；pVStreamData：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_encode(struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc, VENC_DATA_S_DEFINE* pVStreamData);

/**
 * @description: 将YUY2发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_yuy2(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf);

/**
 * @description: 将YUY2发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_nv12(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf);

/**
 * @description: 将I420发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_i420(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf);

/**
 * @description: 将NV12/NV21的Y分量发给USB主机，必须在stream on状态下去才允许发送
 * @param {pVBuf：视频参数, pMetadata长度必须为15字节}
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uvc_send_stream_l8_ir(struct uvc_dev *dev, FRAME_DATA_S_DEFINE* pVBuf, void *pMetadata);

#ifdef __cplusplus
}
#endif

#endif /*UVC_API_H*/