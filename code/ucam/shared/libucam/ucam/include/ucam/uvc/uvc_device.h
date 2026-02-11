/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-09 09:38:16
 * @FilePath    : \lib_ucam\ucam\include\ucam\uvc\uvc_device.h
 * @Description : 
 */ 
#ifndef UVC_DEVICE_H 
#define UVC_DEVICE_H 

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


#include <ucam/ucam.h>
#include <ucam/ucam_list.h>
#include <ucam/uvc/video.h>
#include <ucam/ucam_dbgout.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/uvc/uvc_ctrl.h>
#include <ucam/uvc/uvc_ctrl_eu.h>
#include <ucam/uvc/uvc_ctrl_xu_h264.h>
#include <ucam/uvc/uvc_ctrl_ct.h>
#include <ucam/uvc/uvc_ctrl_pu.h>
#include <ucam/uvc/uvc_stream_common.h>


#define V4L2_REQBUFS_MEMORY_TYPE_MMAP 		1
#define V4L2_REQBUFS_MEMORY_TYPE_USERPTR 	2

#if (defined CHIP_RV1126 || defined CHIP_NT98530)
#define V4L2_REQBUFS_MEMORY_TYPE_DEF	V4L2_REQBUFS_MEMORY_TYPE_MMAP
#else
#define V4L2_REQBUFS_MEMORY_TYPE_DEF	V4L2_REQBUFS_MEMORY_TYPE_USERPTR
#endif
#define REQBUFS_CACHE_ENABLE  		(0)
#define REQBUFS_USERPTR_DATA_OFFSET (32)

#ifndef ALIGN_UP
#define ALIGN_UP(val, alignment) ((((val)+(alignment)-1)/(alignment))*(alignment))
#endif
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, alignment) (((val)/(alignment))*(alignment))
#endif

#define SIMULCAST_MAX_LAYERS	3

DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1,2);
DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1,3);

enum uvc_frame_buf_mode_e {
	UVC_FRAME_BUF_MODE_DYNAMIC, /* 打开码流时申请内存，关闭码流时释放内存 , 批量模式会在USB连接时申请 */
	UVC_FRAME_BUF_MODE_STATIC, /* 初始化时申请内存，去初始化时释放内存 */
	UVC_FRAME_BUF_MODE_DYNAMIC_USB_CONNECT,/* USB连接时申请内存，USB断开时释放内存 */
	UVC_FRAME_BUF_MODE_STATIC_USB_CONNECT, /* USB连接时申请内存，去初始化时释放内存 */
};

#if 1
enum ucam_usb_device_speed {
	UCAM_USB_SPEED_UNKNOWN = 0,			/* enumerating */
	UCAM_USB_SPEED_LOW, UCAM_USB_SPEED_FULL,		/* usb 1.1 */
	UCAM_USB_SPEED_HIGH,				/* usb 2.0 */
	UCAM_USB_SPEED_WIRELESS,			/* wireless (usb 2.5) */
	UCAM_USB_SPEED_SUPER,				/* usb 3.0 */
	UCAM_USB_SPEED_SUPER_PLUS,			/* usb 3.1 */
};
#endif

typedef enum UsbSetupCmds
{
    USB_SC_GET_STATUS = 0x00,    /**< Get status request. */
    USB_SC_CLEAR_FEATURE,        /**< Clear feature. */
    USB_SC_RESERVED,             /**< Reserved command. */
    USB_SC_SET_FEATURE,          /**< Set feature. */
    USB_SC_SET_ADDRESS = 0x05,   /**< Set address. */
    USB_SC_GET_DESCRIPTOR,       /**< Get descriptor. */
    USB_SC_SET_DESCRIPTOR,       /**< Set descriptor. */
    USB_SC_GET_CONFIGURATION,    /**< Get configuration. */
    USB_SC_SET_CONFIGURATION,    /**< Set configuration. */
    USB_SC_GET_INTERFACE,        /**< Get interface (alternate setting). */
    USB_SC_SET_INTERFACE,        /**< Set interface (alternate setting). */
    USB_SC_SYNC_FRAME,           /**< Synch frame. */
    USB_SC_SET_SEL = 0x30,       /**< Set system exit latency. */
    USB_SC_SET_ISOC_DELAY        /**< Set isochronous delay. */
} UsbSetupCmds;

/* USB Setup Commands Recipient */
#define USB_TARGET_MASK                  (0x03)    /* The Target mask */
#define USB_TARGET_DEVICE                (0x00)    /* The USB Target Device */
#define USB_TARGET_INTF                  (0x01)    /* The USB Target Interface */
#define USB_TARGET_ENDPT                 (0x02)    /* The USB Target Endpoint */
#define USB_TARGET_OTHER                 (0x03)    /* The USB Target Other */

/*--- event ---*/

#define UVC_EVENT_FIRST				(V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_CONNECT			(V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT		(V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON			(V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF			(V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP				(V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA				(V4L2_EVENT_PRIVATE_START + 5)
#define UVC_EVENT_AUDIO_STREAMON    (V4L2_EVENT_PRIVATE_START + 6)
#define UVC_EVENT_AUDIO_STREAMOFF   (V4L2_EVENT_PRIVATE_START + 7)
#define UVC_EVENT_SWITCH_CONFIGURATION   (V4L2_EVENT_PRIVATE_START + 8)
#define UVC_EVENT_SUSPEND   		(V4L2_EVENT_PRIVATE_START + 9)
#define UVC_EVENT_RESUME   			(V4L2_EVENT_PRIVATE_START + 10)
#define UVC_EVENT_LAST				(V4L2_EVENT_PRIVATE_START + 10)

//UVC协议定义
typedef enum
{
	UVC_PROTOCOL_V100 = 0x0100,
	UVC_PROTOCOL_V110 = 0x0110,
	UVC_PROTOCOL_V150 = 0x0150,
}uvc_protocol_version;

typedef enum
{
	UVC_USB_TRANSFER_MODE_ISOC = 0x00,
	UVC_USB_TRANSFER_MODE_BULK = 0x01,	
}uvc_usb_transfer_mode;


#define EP0_DATA_SIZE     (60 - sizeof(struct usb_ctrlrequest))

#define UVC_REQUEST_DATA_AUTO_QUEUE_LEN (60 - sizeof(struct usb_ctrlrequest))	
struct uvc_request_data_auto_queue
{
	__s32 length;	
	__u8 data[UVC_REQUEST_DATA_AUTO_QUEUE_LEN];
	
	struct usb_ctrlrequest usb_ctrl;
};

#define UVC_REQUEST_DATA_LEN (60)	
struct uvc_request_data
{
	__s32 length;	
	__u8 data[UVC_REQUEST_DATA_LEN];

};



struct uvc_event
{
	union {
		enum ucam_usb_device_speed speed;
		struct usb_ctrlrequest req;
		struct uvc_request_data data;
		struct uvc_request_data_auto_queue auto_queue_data;
	};
};



typedef union usb_sn_function_bmControls {
		/** raw register data */
	uint16_t d16;//16位
		/** register bits */
	struct {//低位在前[0]...[16]
		unsigned uvc_en:1;//长度为1
		unsigned uvc_s2_en:1;
		unsigned uvc_v150_en:1;
		unsigned hid_vhd_en:1;
		unsigned hid_custom_en:1;
		unsigned dfu_en:1;
		unsigned uac_capture_en:1;
		unsigned uac_playback_en:1;
		unsigned serial_en:1;
		unsigned rndis_en:1;
		unsigned storage_en:1;
		
		unsigned reserved:5;
	} b;
} usb_sn_function_bmControls_t;

struct usb_sn_header_t {
	uint16_t usb_bcd;
	usb_sn_function_bmControls_t  bmControls;
} __attribute__ ((packed));


struct soc_version{
	uint8_t mode;
	uint8_t bigVersion;
	uint8_t littleVersion;
} __attribute__((__packed__));

struct device_version{
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	struct soc_version UsbVersion;
	struct soc_version SocVersion;
	struct soc_version FpgaVersion;
	struct soc_version BootloaderVersion;
} __attribute__((__packed__));


#define SAFE_RELEASE(x) { if (x) x->Release(); x = NULL; }
#define SAFE_FREE(x) { if (x) free(x); x = NULL; }
#define SAFE_PCLOSE(x) { if (x) pclose(x); x = NULL; }
#define SAFE_FCLOSE(x) { if (x) fclose(x); x = NULL; }

#define USB_CONFIG_MAX			3//一共支持多少个配置
#define UVC_VIDEO_STREAM_MAX	2//配置最大支持多少路码流

struct uvc_frame_info_t {
	struct uvc_descriptor_header * frame_header;
	uint8_t bypass;
	uint8_t def;
};

struct uvc_streaming_desc_attr_t {
	struct uvc_descriptor_header * format_header;
	//struct uvc_descriptor_header ** frame_header;
	struct uvc_frame_info_t * frame_info;
};


typedef int(*usb_connet_event_callback_t) (struct uvc *uvc, uint32_t connet_status, uint32_t usb_speed, uint32_t bConfigurationValue);
typedef int(*uvc_stream_event_callback_t) (struct uvc *uvc, uint32_t uvc_stream_num, uint32_t stream_status);
typedef int(*uvc_usb_suspend_event_callback_t) (struct uvc *uvc);
typedef int(*uvc_usb_resume_event_callback_t) (struct uvc *uvc);

#if defined CHIP_HI3519V101 || defined CHIP_HI3559V100
#define MMZ_PHY_ADDR_TYPE uint32_t
typedef void* (*uvc_mmap_callback_t)(struct uvc_dev *dev, uint32_t u32PhyAddr, uint32_t u32Size);
typedef void* (*uvc_mmap_cache_callback_t)(struct uvc_dev *dev, uint32_t u32PhyAddr, uint32_t u32Size);
typedef int32_t (*uvc_munmap_callback_t)(struct uvc_dev *dev, void *pVirAddr, uint32_t u32Size);
typedef int32_t (*uvc_mflush_cache_callback_t)(struct uvc_dev *dev, uint32_t u32PhyAddr, void *pVirAddr, uint32_t u32Size);
typedef int32_t (*uvc_mmz_alloc_callback_t)(struct uvc_dev *dev, uint32_t *pu32PhyAddr, void **ppVirAddr,
							const char* strMmb, const char* strZone, uint32_t u32Len);
typedef int32_t (*uvc_mmz_alloc_cached_callback_t)(struct uvc_dev *dev, uint32_t *pu32PhyAddr, void **ppVirAddr,
							const char* strMmb, const char* strZone, uint32_t u32Len);
typedef int32_t (*uvc_mmz_flush_cache_callback_t)(struct uvc_dev *dev, uint32_t u32PhyAddr, void *pVirAddr, uint32_t u32Size);
typedef int32_t (*uvc_mmz_free_callback_t)(struct uvc_dev *dev, uint32_t u32PhyAddr, void *pVirAddr, uint32_t u32Size);
#else
#define MMZ_PHY_ADDR_TYPE unsigned long long
typedef void* (*uvc_mmap_callback_t)(struct uvc_dev *dev, unsigned long long u64PhyAddr, uint32_t u32Size);
typedef void* (*uvc_mmap_cache_callback_t)(struct uvc_dev *dev, unsigned long long u64PhyAddr, uint32_t u32Size);
typedef int32_t (*uvc_munmap_callback_t)(struct uvc_dev *dev, void *pVirAddr, uint32_t u32Size);
typedef int32_t (*uvc_mflush_cache_callback_t)(struct uvc_dev *dev, unsigned long long u64PhyAddr, void *pVirAddr, uint32_t u32Size);
typedef int32_t (*uvc_mmz_alloc_callback_t)(struct uvc_dev *dev, unsigned long long *u64PhyAddr, void **ppVirAddr,
							const char* strMmb, const char* strZone, uint32_t u32Len);
typedef int32_t (*uvc_mmz_alloc_cached_callback_t)(struct uvc_dev *dev, unsigned long long *u64PhyAddr, void **ppVirAddr,
							const char* strMmb, const char* strZone, uint32_t u32Len);
typedef int32_t (*uvc_mmz_flush_cache_callback_t)(struct uvc_dev *dev, unsigned long long u64PhyAddr, void *pVirAddr, uint32_t u32Size);
typedef int32_t (*uvc_mmz_free_callback_t)(struct uvc_dev *dev, unsigned long long u64PhyAddr, void *pVirAddr, uint32_t u32Size);
#endif

typedef int (*uvc_dam_memcpy_callback_t)(unsigned long long phyDst, unsigned long long phySrc, int size);


struct uvc_send_stream_ops
{
	int (*send_encode)(struct uvc_dev *dev, int32_t venc_chn, uint32_t fcc, void* pVStreamData);
	int (*send_yuy2)(struct uvc_dev *dev, void* pVBuf);
	int (*send_nv12)(struct uvc_dev *dev, void* pVBuf);
	int (*send_i420)(struct uvc_dev *dev, void* pVBuf);
	int (*send_l8_ir)(struct uvc_dev *dev, void* pVBuf, void *pMetadata);
};

struct uvc_dev_config
{
	int32_t venc_chn;				/* MJPEG、H264、H265编码通道号绑定*/

    uint32_t resolution_width_min;	/* 分辨率宽度的最小值 */
    uint32_t resolution_height_min;	/* 分辨率高度的最小值 */
	uint32_t resolution_width_max;	/* 分辨率宽度的最大值 */
	uint32_t resolution_height_max;	/* 分辨率高度的最大值 */

	uint32_t yuv_resolution_width_min;	/* YUY2/NV12分辨率宽度的最小值，不设置就使用resolution_width_min设置的 */
    uint32_t yuv_resolution_height_min;	/* YUY2/NV12分辨率高度的最小值，不设置就使用resolution_height_min设置的 */
	uint32_t yuv_resolution_width_max;	/* YUY2/NV12分辨率宽度的最大值，不设置就使用resolution_width_max设置的 */
	uint32_t yuv_resolution_height_max;	/* YUY2/NV12分辨率高度的最大值，不设置就使用resolution_height_max设置的 */
	
	uint32_t fps_min;					/* 支持最小帧率，可设置值：60 50 30 25 24 20 15 10 7 5*/
	uint32_t fps_max;					/* 支持最大帧率，可设置值：60 50 30 25 24 20 15 10 7 5*/

	uint32_t yuv_fps_min;				/* YUY2/NV12支持最小帧率，可设置值：60 50 30 25 24 20 15 10 7 5，不设置就使用fps_min设置的*/
	uint32_t yuv_fps_max;				/* YUY2/NV12支持最大帧率，可设置值：60 50 30 25 24 20 15 10 7 5，不设置就使用fps_max设置的*/

	uint8_t  uvc_yuy2_enable;			/* YUY2使能 */
    uint8_t  uvc_nv12_enable;			/* NV12使能 */
	uint8_t  uvc_i420_enable;			/* I420使能 */
	uint8_t  uvc_mjpeg_enable;			/* MJPEG使能 */
    uint8_t  uvc110_h264_enable;		/* UVC1.1版本 H264使能 */
    uint8_t  uvc110_h265_enable;		/* UVC1.1版本 H265使能 */
	

	/*仅用于第一路判断用*/
	uint8_t  uvc110_s2_h264_enable;		/* 第二路码流是否支持UVC1.1版本H264 ，若不支持双流可忽略*/
    uint8_t  uvc110_s2_h265_enable;		/* 第二路码流是否支持UVC1.1版本H265 ，若不支持双流可忽略*/

	uint8_t uvc150_simulcast_en; 		/* UVC1.5 simulcast使能 */
    int32_t simulcast_stream_0_venc_chn;/* UVC1.5 simulcast 0 VENC通道号，若不支持UVC1.5 simulcast可忽略 */
    int32_t simulcast_stream_1_venc_chn;/* UVC1.5 simulcast 1 VENC通道号，若不支持UVC1.5 simulcast可忽略 */
    int32_t simulcast_stream_2_venc_chn;/* UVC1.5 simulcast 2 VENC通道号，若不支持UVC1.5 simulcast可忽略 */

	uint32_t MaxPayloadTransferSize_usb2;		/* USB2.0 UVC ISOC带宽最大值 */
	uint32_t MaxPayloadTransferSize_usb3;		/* USB3.0 UVC ISOC带宽最大值 */
	uint32_t MaxPayloadTransferSize_usb3_win7;	/* WIN7 USB3.0 UVC ISOC带宽最大值 */
	uint8_t usb3_isoc_speed_fast_mode;			/* USB3.0 UVC ISOC使用最大带宽使能，降低传输延时（默认打开） */
	float usb3_isoc_speed_multiple;				/* USB3.0 UVC ISOC带宽倍数（默认1.5），一般最少预留超出20%的带宽 */
	float win7_usb3_isoc_speed_multiple;		/* WIN7 USB3.0 UVC ISOC带宽倍数（默认1.2） ，一般最少预留超出20%的带宽*/
	uint8_t  usb3_isoc_win7_mode;				/* USB第一个配置USB3.0 ISOC win7模式 */
	uint16_t uvc_protocol;						/* UVC协议版本设置 */
	uint8_t uvc_ep_mode;						/* UVC端点模式设置 0-ISOC 1-BULK，同时给驱动加载使用 */
	uint8_t uvc_max_queue_num;					/* UVC视频BUF队列个数的最大值 */
	uint32_t uvc_max_format_bufsize;			/* UVC视频一帧BUF的最大值 */
	

	uint8_t processing_unit_id;	/* UVC的processing_unit_id（默认为3），如果扩展命令使用了3（例如星网版本），则需要修改该配置 */				
    uvc_camera_terminal_bmControls_t camera_terminal_bmControls; /* UVC CT控制使能配置 */
    uvc_processing_unit_bmControls_t processing_unit_bmControls; /* UVC PU控制使能配置 */

	struct uvc_ct_config_t uvc_ct_config;			/* UVC CT控制变量 */
	struct uvc_pu_config_t uvc_pu_config;			/* UVC PU控制变量 */
	struct uvc_h264_config_t uvc_xu_h264_config;	/* UVC H264控制变量 */
	struct uvc_encode_unit_attr uvc_encode_unit_attr_cur[SIMULCAST_MAX_LAYERS]; /* UVC1.5 EU H264控制变量 */

	uint32_t reqbufs_memory_type;
	uint8_t  windows_hello_enable;
	int uvc_frame_buf_mode;

    uint32_t mjpeg_compression_ratio;
    uint32_t h264_compression_ratio;
};


struct uvc_dev
{
	struct uvc *uvc;
	int fd;	
		
	uint8_t video_dev_index;
	int bConfigurationValue;
	uint32_t stream_num;

	struct uvc_dev_config config;
	struct uvc_streaming_control probe;
	struct uvc_streaming_control commit;

	struct v4l2_event v4l2_event;
	struct usb_ctrlrequest uvc_set_ctrl;
	struct usb_ctrlrequest uac_set_ctrl;

	uint8_t vc_request_error_code;

	uint32_t fcc;
	uint16_t width;
	uint16_t height;

	struct ucam_list_head 	list;


	const struct uvc_streaming_desc_attr_t ** hs_uvc_streaming_desc_attrs;
	const struct uvc_streaming_desc_attr_t ** ss_uvc_streaming_desc_attrs;
	const struct uvc_streaming_desc_attr_t ** ssp_uvc_streaming_desc_attrs;
	struct uvc_descriptor_header **hs_streaming;
	struct uvc_descriptor_header **ss_streaming;
	struct uvc_descriptor_header **ssp_streaming;

	const struct uvc_descriptor_header ** hs_controls_desc_config;
	const struct uvc_descriptor_header ** ss_controls_desc_config;
	const struct uvc_descriptor_header ** ssp_controls_desc_config;
	struct uvc_descriptor_header **hs_controls_desc;
	struct uvc_descriptor_header **ss_controls_desc;
	struct uvc_descriptor_header **ssp_controls_desc;

	void **mem;
	unsigned int nbufs;
	uint32_t bufsize;
	uint32_t sizeimage;
	uint32_t buf_used;
	uint32_t PayloadTransferSize;

	uint32_t MaxPayloadTransferSize;
	uint32_t MaxPayloadTransferSize_bulk;
	
	uint32_t init_flag;
	uint32_t uvc_stream_on;
	uint32_t uvc_stream_on_trigger_flag;

	uint8_t  first_IDR_flag;
	uint8_t  uvc_h264_sei_en;
	uint8_t  uvc150_simulcast_streamon; 
	uint8_t  H264SVCEnable;
	uint32_t h264_svc_layer;

	pthread_t uvc_ctrl_handle_thread_pid;
    int uvc_ctrl_handle_thread_exit;
    struct timespec stream_on_trigger_time;
	
	unsigned int  control_intf;
	unsigned int  streaming_intf;
	unsigned int  status_interrupt_ep_address;
	unsigned int  streaming_ep_address;

	
	pthread_mutex_t uvc_reqbufs_mutex;
	unsigned long long *video_reqbufs_addr;
	uint32_t *video_reqbufs_length;
	struct uvc_ioctl_reqbufs_config_t reqbufs_config;

	struct uvc_send_stream_ops send_stream_ops;
	uvc_send_encode_fun_t send_encode;
	struct uvc_send_raw_info send_raw_info;
	uint32_t send_stream_common_mode;
};

struct uvc_config
{
    uint32_t  chip_id;					/* 芯片的ID */
	uint32_t uvc_video_stream_max;		/* UVC支持最大的码流 */

	enum ucam_usb_device_speed usb_speed_max;/* USB最大速率限制*/

    uint8_t reset_usb_sn_enable;		/* 重新配置USB序列号，支持音频打开/关闭设置的需要用打开这个功能，定制序列号需将该功能关闭*/

	struct device_version DeviceVersion;/* 设备VID、PID、BCD等信息 */
	char usb_serial_number[128];		/* 设备的序列号 */

	uint16_t  wCompQualityMax;
	
	uint8_t  uvc_xu_h264_ctrl_enable;
};

struct uvc
{
	struct ucam *ucam;
	struct uvc_dev **uvc_dev;

	pthread_mutex_t uvc_event_mutex;

	//from host usb
	uint32_t usb_connet_status;
	int bConfigurationValue;
	enum ucam_usb_device_speed usb_speed;
	unsigned int win7_os_configuration;
	
	//from kernel
	unsigned int  dfu_enable;
	unsigned int  uac_enable;
	unsigned int  uvc_s2_enable;
	unsigned int  uvc_v150_enable;
	unsigned int  win7_usb3_enable;
	unsigned int  zte_hid_enable;

	//from code settings
	struct uvc_config config; 
	
	const struct uvc_ctrl_attr_t * const *uvc_ctrl_attrs;
    struct ucam_list_head uvc_xu_ctrl_list;
	struct ucam_list_head uvc_dev_list;

	int init_flag;

	uint8_t *h264_svc_buf[UVC_VIDEO_STREAM_MAX];
	uint32_t h264_svc_buf_len[UVC_VIDEO_STREAM_MAX];
	
    //注册usb插拔事件回调函数
    usb_connet_event_callback_t usb_connet_event_callback;
    //注册流打开关/闭事件事件回调函数
    uvc_stream_event_callback_t uvc_stream_event_callback;

	uvc_requests_ctrl_fn_t uvc_requests_ctrl_event_callback;

	//mmz
	uvc_mmap_callback_t uvc_mmap_callback;
	uvc_mmap_cache_callback_t uvc_mmap_cache_callback;
	uvc_munmap_callback_t uvc_munmap_callback;
	uvc_mflush_cache_callback_t uvc_mflush_cache_callback;
	uvc_mmz_alloc_callback_t uvc_mmz_alloc_callback;
	uvc_mmz_alloc_cached_callback_t uvc_mmz_alloc_cached_callback;
	uvc_mmz_flush_cache_callback_t uvc_mmz_flush_cache_callback;
	uvc_mmz_free_callback_t uvc_mmz_free_callback;
	
	uint8_t usb_suspend;
	uvc_usb_suspend_event_callback_t usb_suspend_event_callback;
	uvc_usb_resume_event_callback_t usb_resume_event_callback;

	uint32_t uvc_dam_align;
	uvc_dam_memcpy_callback_t uvc_dam_memcpy_callback;
};

#endif /* UVC_DEVICE_H */
