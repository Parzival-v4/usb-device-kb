/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uac\uac_device.h
 * @Description : 
 */ 
#ifndef __AUDIO_DEVICE_H
#define __AUDIO_DEVICE_H
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

#include <pthread.h>
#include <signal.h>

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>
//#include <ucam/videodev2.h>
#include <linux/videodev2.h>

#include <sys/time.h>
#include <stdbool.h> 


//#include <ucam/uvc_config.h>
//#include <ucam/ucam.h>
///#include <ucam/video.h>
//#include <ucam/uvc_device.h>
#include <ucam/ucam_dbgout.h>
#include "asoundlib.h"

#include <ucam/ucam_list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UAC_V4L2_EVENT_PRIVATE_START 0x08000000
#define UAC_EVENT_FIRST			(UAC_V4L2_EVENT_PRIVATE_START + 0)
#define UAC_EVENT_CONNECT		(UAC_V4L2_EVENT_PRIVATE_START + 0)
#define UAC_EVENT_DISCONNECT		(UAC_V4L2_EVENT_PRIVATE_START + 1)
#define UAC_EVENT_STREAMON		(UAC_V4L2_EVENT_PRIVATE_START + 2)
#define UAC_EVENT_STREAMOFF		(UAC_V4L2_EVENT_PRIVATE_START + 3)
#define UAC_EVENT_SETUP			(UAC_V4L2_EVENT_PRIVATE_START + 4)
#define UAC_EVENT_DATA			(UAC_V4L2_EVENT_PRIVATE_START + 5)
#define UAC_MIC_VOLUME			(UAC_V4L2_EVENT_PRIVATE_START + 6)
#define UAC_MIC_MUTE			(UAC_V4L2_EVENT_PRIVATE_START + 7)


#define UAC_EVENT_STREAMON_SPEAK		(UAC_V4L2_EVENT_PRIVATE_START + 8)
#define UAC_EVENT_STREAMOFF_SPEAK		(UAC_V4L2_EVENT_PRIVATE_START + 9)
#define UAC_SPEAK_VOLUME		(UAC_V4L2_EVENT_PRIVATE_START + 10)
#define UAC_SPEAK_MUTE			(UAC_V4L2_EVENT_PRIVATE_START + 11)


#define UAC_EVENT_SUSPEND   	(UAC_V4L2_EVENT_PRIVATE_START + 12)
#define UAC_EVENT_RESUME   		(UAC_V4L2_EVENT_PRIVATE_START + 13)
#define UAC_MIC_SAMPLERATE	    (V4L2_EVENT_PRIVATE_START + 14)
#define UAC_SPEAK_SAMPLERATE	(V4L2_EVENT_PRIVATE_START + 15)
#define UAC_EVENT_SPEAK_LAST	(UAC_V4L2_EVENT_PRIVATE_START + 15)

#define UAC_EVENT_LAST		UAC_EVENT_SPEAK_LAST


struct uac_request_data
{
	__s32 length;
	__u8 data[60];
};

struct uac_event
{
	union {
		enum usb_device_speed speed;
		struct usb_ctrlrequest req;
		struct uac_request_data data;
	};
};

enum uac_dir_type_t {
	UAC_DIR_MIC = 0,
	UAC_DIR_SPEAK,
};


#define UACIOC_SEND_RESPONSE            		_IOW('U', 1, struct uac_request_data)


//#define UAC_MAX_VOLUME 0
//#define UAC_MIN_VOLUME (-20)
//#define UAC_DEF_VOLUME 0

#define UAC_FRAME_SIZE(SAMPLEPSEC, BITS_PER_SAMPLE, CHANNELS, n)		(10 * n * (SAMPLEPSEC * CHANNELS * (BITS_PER_SAMPLE/8) /1000))//10ms

struct uac;

typedef int(*uac_stream_event_callback_t) (struct uac *uac, enum uac_dir_type_t dir, uint32_t stream_status);
typedef int(*uac_mute_callback_t) (struct uac *uac, enum uac_dir_type_t dir, uint32_t mute);
typedef int(*uac_volume_callback_t) (struct uac *uac, enum uac_dir_type_t dir, int volume_dB);
typedef int(*uac_usb_connet_event_callback_t) (struct uac *uac, uint32_t connet_status, uint32_t usb_speed, uint32_t bConfigurationValue);
typedef int(*uac_usb_suspend_event_callback_t) (struct uac *uac);
typedef int(*uac_usb_resume_event_callback_t) (struct uac *uac);
typedef int(*uac_set_rate_callback_t) (struct uac *uac, enum uac_dir_type_t dir, uint32_t sample_rate);

struct uac_attrs
{
	uint32_t samplepsec;		/* 采样率设置 */
	snd_pcm_format_t format;	/* alsa宽度设置 */
	uint32_t channels;			/* 支持多少通道设置 */
	unsigned long uframes;		/* alsa uframes设置 */

	uint8_t stream_on_status;	/* 码流的打开状态 */
	uint8_t mute;				/* 静音状态 */
	int volume;					/* 音量，单位：dB */
	int mode; //Open mode (see #SND_PCM_NONBLOCK, #SND_PCM_ASYNC)
	uint8_t start_delay;
	uint8_t delay;
};

struct uac_dev
{
	struct uac *uac;
	int bConfigurationValue;
	uint32_t usb_speed;
    int fd;
	uint32_t video_index;
	struct v4l2_event v4l2_event;

	struct ucam_list_head 	list;

	uint8_t uac_mic_enable;
	uint8_t uac_speak_enable;

	struct uac_attrs mic_attrs;
	struct uac_attrs speak_attrs;


	int uac_init_flag;
    pthread_t uac_event_thread_pid;
    int uac_event_thread_exit;

	//alsa
	uint32_t alsa_card_id;
	snd_pcm_t *alsa_playback_handle;	//USB采集设备相对alsa来说是播放设备
	snd_pcm_t *alsa_capture_handle;		//USB播放设备相对alsa来说是采集设备
	pthread_mutex_t alsa_playback_mutex;
	pthread_mutex_t alsa_capture_mutex;
};


struct uac
{
	struct ucam *ucam;
    struct uac_dev *uac_dev;
	int bConfigurationValue;

	uint8_t mic_stream_on_trigger;
	uint8_t speak_stream_on_trigger;

	struct ucam_list_head 	uac_dev_list; 

	uac_stream_event_callback_t uac_stream_event_callback;
    uac_mute_callback_t uac_mute_callback;
    uac_volume_callback_t uac_volume_callback;
	uac_usb_connet_event_callback_t uac_usb_connet_event_callback; 
	
	uint8_t usb_suspend;
	uac_usb_suspend_event_callback_t usb_suspend_event_callback;
	uac_usb_resume_event_callback_t usb_resume_event_callback;
	uac_set_rate_callback_t uac_set_rate_callback;
};




/**
 * @description: 创建uac句柄的内存
 * @param {ucam} 
 * @return: true:success; NULL:error
 * @author: QinHaiCheng
 */
struct uac *create_uac(struct ucam *ucam);

/**
 * @description: 释放uac内存和关闭设备节点
 * @param {uac} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_free (struct uac *uac);

/**
 * @description: 注册和初始化uac设备
 * @param {bConfigurationValue：第几个usb配置；video_index：V4L2 video的设备节点
 * 			alsa_card_id：alsa的id; uac_mic_enable:mic使能；uac_speak_enable：speak使能；
 * 			mic_attrs：mic配置参数；speak_attrs：speak配置参数} 
 * @return: true:success; NULL:error
 * @author: QinHaiCheng
 */
struct uac_dev *uac_add_dev(struct uac *uac, int bConfigurationValue, int video_index, uint32_t alsa_card_id, 
    uint8_t uac_mic_enable, uint8_t uac_speak_enable, struct uac_attrs *mic_attrs, struct uac_attrs *speak_attrs);

/**
 * @description: 设置uac工作在哪个bConfigurationValue下
 * @param {val：usb的bConfigurationValue} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_set_usb_configuration_value (struct uac *uac, int val);

/**
 * @description: 设置静音
 * @param {dir：mic or speak；val：1-静音，0-不静音} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_set_mute_status (struct uac *uac, enum uac_dir_type_t dir, int val);

/**
 * @description: 获取静音状态
 * @param {dir：mic or speak} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_mute_status (struct uac *uac, enum uac_dir_type_t dir, uint8_t *mute_status);

/**
 * @description: 获取mic静音状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_mic_mute_status(struct uac *uac, uint8_t *mute_status);

/**
 * @description: 获取speak静音状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_speak_mute_status(struct uac *uac, uint8_t *mute_status);

/**
 * @description: 获取码流打开状态
 * @param {dir：mic or speak} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_stream_on_status (struct uac *uac, enum uac_dir_type_t dir, uint8_t *stream_on_status);

/**
 * @description: 获取mic码流打开状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_mic_stream_on_status(struct uac *uac, uint8_t *stream_on_status);

/**
 * @description: 获取speak码流打开状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_speak_stream_on_status(struct uac *uac, uint8_t *stream_on_status);

/**
 * @description: 获取mic和speak码流打开状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_all_stream_on_status (struct uac *uac, uint8_t *stream_on_status);

/**
 * @description: 获取mic使能状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_mic_enable (struct uac *uac, uint8_t *enable);

/**
 * @description: 获取speack使能状态
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_get_speack_enable (struct uac *uac, uint8_t *enable);

/**
 * @description: 发送mic码流，必须在stream on状态下才允许发送
 * @param {data：数据的指针，count：数据的大小} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_send_mic_stream (struct uac *uac, const void *data, size_t count);
int uac_send_mic_stream_timeout(struct uac *uac, const void *data, size_t count, int timeout_ms);

/**
 * @description: 获取speak码流，必须在stream on状态下才允许发送
 * @param {data：数据的指针，count：数据的大小} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_read_speak_stream (struct uac *uac, void *data, size_t count);

/**
 * @description: 发送mic码流，必须在stream on状态下才允许发送
 * @param {data：数据的指针，count：数据的大小} 
 * @return: 实际写入的大小，小于0为错误
 * @author: QinHaiCheng
 */
int uac_send_mic_stream_v2 (struct uac *uac, const void *data, size_t count);
int uac_send_mic_stream_timeout_v2(struct uac *uac, const void *data, size_t count, int timeout_ms);

/**
 * @description: 获取speak码流，必须在stream on状态下才允许发送
 * @param {data：数据的指针，count：数据的大小} 
 * @return: 实际读到的大小，小于0为错误
 * @author: QinHaiCheng
 */
int uac_read_speak_stream_v2(struct uac *uac, void *data, size_t count);
int uac_read_speak_stream_timeout_v2(struct uac *uac, void *data, size_t count, int timeout_ms);


/**
 * @description: uac 检测AO buf修补数据防止出现xrun
 * @param {} 
 * @return: 0:success; other:error
 */
int uac_speak_ao_buf_avail_check(struct uac *uac,
    snd_pcm_uframes_t avail_frames,
    snd_pcm_uframes_t buffer_size,
    snd_pcm_uframes_t period_size,
    void *data, size_t data_count, 
    void *fix_buf, int *fix_count);
	
/**
 * @description: 打开USB连接
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_usb_connet (struct uac *uac);

/**
 * @description: 断开USB后再重新连接
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_usb_reconnet (struct uac *uac);

/**
 * @description: 断开USB连接
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_usb_disconnet (struct uac *uac);

/**
 * @description:注册uac stream event回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_register_stream_event_callback(struct uac *uac, uac_stream_event_callback_t callback);

/**
 * @description:注册uac mute event回调函数 
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_register_mute_event_callback(struct uac *uac, uac_mute_callback_t callback);

/**
 * @description: 注册uac volume event回调函数 
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_register_volume_event_callback(struct uac *uac, uac_volume_callback_t callback);

/**
 * @description: 注册uac usb插拔 event回调函数 
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_register_usb_connet_event_callback(struct uac *uac, uac_usb_connet_event_callback_t callback);

/**
 * @description: 注册uac usb suspend回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_register_usb_suspend_event_callback(struct uac *uac, uac_usb_suspend_event_callback_t callback);

/**
 * @description: 注册uac usb resume回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
int uac_register_usb_resume_event_callback(struct uac *uac, uac_usb_resume_event_callback_t callback);

/**
 * @description: 主动关闭所有的UAC码流
 * @param {type} 
 * @return: 0:success; other:error
 * @author: QinHaiCheng
 */
void uac_streamoff_all(struct uac_dev* dev);

/**
 * @description: 注册uac set rate回调函数
 * @param {type} 
 * @return: 0:success; other:error
 * @author: WangYuCheng
 */
int uac_register_set_rate_event_callback(struct uac *uac, uac_set_rate_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_DEVICE_H */

