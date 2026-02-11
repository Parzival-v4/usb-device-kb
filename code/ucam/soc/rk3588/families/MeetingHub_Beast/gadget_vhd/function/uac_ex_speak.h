/*
 * The file defines UAC API for user space
 */

#ifndef _UAC_EX_H_
#define _UAC_EX_H_

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/usb/ch9.h>

#define UAC_EVENT_FIRST			(V4L2_EVENT_PRIVATE_START + 0)
#define UAC_EVENT_CONNECT		(V4L2_EVENT_PRIVATE_START + 0)
#define UAC_EVENT_DISCONNECT		(V4L2_EVENT_PRIVATE_START + 1)
#define UAC_EVENT_STREAMON		(V4L2_EVENT_PRIVATE_START + 2)
#define UAC_EVENT_STREAMOFF		(V4L2_EVENT_PRIVATE_START + 3)
#define UAC_EVENT_SETUP			(V4L2_EVENT_PRIVATE_START + 4)
#define UAC_EVENT_DATA			(V4L2_EVENT_PRIVATE_START + 5)
#define UAC_EVENT_STARTCAP		(V4L2_EVENT_PRIVATE_START + 8)
#define UAC_EVENT_STOPCAP		(V4L2_EVENT_PRIVATE_START + 9)
#define UAC_MIC_VOLUME			(V4L2_EVENT_PRIVATE_START + 6)
#define UAC_MIC_MUTE			(V4L2_EVENT_PRIVATE_START + 7)
#define UAC_SPEAK_VOLUME		(V4L2_EVENT_PRIVATE_START + 10)
#define UAC_SPEAK_MUTE			(V4L2_EVENT_PRIVATE_START + 11)

#define UAC_EVENT_SUSPEND   	(V4L2_EVENT_PRIVATE_START + 12)
#define UAC_EVENT_RESUME   		(V4L2_EVENT_PRIVATE_START + 13)
#define UAC_EVENT_LAST			(V4L2_EVENT_PRIVATE_START + 13)

#define SAFE_KFREE(x) { if (x) kfree(x); x = NULL; }


#ifdef __KERNEL__

#include <linux/usb.h>	/* For usb_endpoint_* */
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-device.h>
#include <linux/usb/audio.h>

#include "uac_queue_ex.h"
#include "gadget_ex.h"

/* ------------------------------------------------------------------------
 * Structures
 */

struct uac_params {
	/* playback */
	int p_chmask;	/* channel mask */
	int p_srate;	/* rate in Hz */
	int p_ssize;	/* sample size */

	/* capture */
	int c_chmask;	/* channel mask */
	int c_srate;	/* rate in Hz */
	int c_ssize;	/* sample size */

	int req_number; /* number of preallocated requests */
};

struct uac_req {
	struct uac_device *dev; /* parent param */
	struct usb_request *req;
};

struct uac_req_cap {
	struct uac_rtd_params *pp; /* parent param */
	struct usb_request *req;
};

struct uac_req_play {
	struct uac_rtd_params *pp; /* parent param */
	struct usb_request *req;
};

struct uac_rtd_params {
	struct snd_uac_chip *uac; /* parent chip */
	bool ep_enabled; /* if the ep is enabled */
	/* Size of the ring buffer */
	size_t dma_bytes;
	unsigned char *dma_area;

	struct snd_pcm_substream *ss;

	/* Ring buffer */
	ssize_t hw_ptr;

	void *rbuf;

	size_t period_size;

	unsigned max_psize;	/* MaxPacketSize of endpoint */
	struct uac_req_cap *ureq;

	spinlock_t lock;
};

struct uac_device
{
	struct usb_function func;
	struct usb_gadget *gadget;
	struct usb_ep *in_ep;
	struct usb_ep *out_ep;
	struct uac_params params;
	struct video_device *vdev;
	struct v4l2_device v4l2_dev;
	struct uac_queue queue;
	struct uac_req *ureq;
	void *rbuf;
	struct snd_uac_chip *uac;
	unsigned int in_ep_maxpsize;
	unsigned int out_ep_maxpsize;
	bool ep_enabled; /* if the ep is enabled */
	unsigned max_psize;	/* MaxPacketSize of endpoint */

	/* timekeeping for the playback endpoint */
	unsigned int p_interval;
	unsigned int p_residue;

	/* pre-calculated values for playback iso completion */
	unsigned int p_pktsize;
	unsigned int p_pktsize_residue;
	unsigned int p_framesize;
	unsigned int alsa_buf_num;
#ifdef CHIP_RK3588
	struct timer_list wait_out_req_complete_timer;
#endif
};



struct uac_file_handle
{
	struct v4l2_fh vfh;
	struct uac_device *dev;
};

#define to_uac_file_handle(handle) \
	container_of(handle, struct uac_file_handle, vfh)

static inline struct uac_device *func_to_uac_device(struct usb_function *f)
{
	return container_of(f, struct uac_device, func);
}

static inline uint num_channels(uint chanmask)
{
	uint num = 0;

	while (chanmask) {
		num += (chanmask & 1);
		chanmask >>= 1;
	}

	return num;
}

int uac_device_setup(struct uac_device *uac_dev);
void uac_device_cleanup(struct uac_device *uac_dev);
int uac_device_start_playback(struct uac_device *uac_dev);
void uac_device_stop_playback(struct uac_device *uac_dev);
int uac_device_start_capture(struct uac_device *uac_dev);
void uac_device_stop_capture(struct uac_device *uac_dev);

#endif /* __KERNEL__ */

#endif /* _UAC_EX_H_ */

