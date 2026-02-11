/*
 * uac_device.c -- interface to USB gadget "ALSA sound card" utilities
 *
 * Copyright (C) 2016
 * Author: Ruslan Bilovol <ruslan.bilovol@gmail.com>
 *
 * Sound card implementation was cut-and-pasted with changes
 * from f_uac2.c and has:
 *    Copyright (C) 2011
 *    Yadwinder Singh (yadi.brar01@gmail.com)
 *    Jaswinder Singh (jaswinder.singh@linaro.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "uac_queue_ex.h"
#include "uac2_audio_ex.h"
#include "uac_common.h"

extern int get_usb_link_state(void);

//#define VHD_UAC_DEBUG
#ifdef VHD_UAC_DEBUG

#define VHD_UAC_DEBUG_PRINT_LEN_BIT		0
#define VHD_UAC_DEBUG_SAVE_FILE_BIT		1
#define VHD_UAC_DEBUG_CMP_DMA_BIT		2
#define VHD_UAC_DEBUG_PRINT_EVENT_BIT	3

static struct kobject *u_audio_kobj;
static int debug = 0;

struct save_work_data {
    struct work_struct work;  // 必须作为第一个成员
    char *buf[2];                // 数据缓冲区
    size_t buf_len;               // 数据长度
	size_t len_ready;
	size_t len_offset;
	int index;
	int i_ready;
    struct file *file;    
};
#endif

struct snd_uac_chip {
	struct uac_device *uac_dev;
	struct uac_rtd_params c_prm;
	struct uac_rtd_params p_prm;

	struct snd_card *card;
	struct snd_pcm *pcm;

	/* timekeeping for the playback endpoint */
	unsigned int p_interval;
	unsigned int p_residue;
	/* pre-calculated values for playback iso completion */
	unsigned int p_pktsize;
	unsigned int p_pktsize_residue;
	unsigned int p_framesize;
};


#define BUFF_SIZE_MAX	(PAGE_SIZE * 6)
#define PRD_SIZE_MAX	PAGE_SIZE
#define MIN_PERIODS	4
#ifdef CHIP_RK3588
#define UAC_WAIT_OUT_REQ_COMPLETE_TIMEOUT	2
#endif

static struct snd_pcm_hardware uac_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER
		 | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID
		 | SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.rates = SNDRV_PCM_RATE_CONTINUOUS,
	.periods_max = BUFF_SIZE_MAX / PRD_SIZE_MAX,
	.buffer_bytes_max = BUFF_SIZE_MAX,
	.period_bytes_max = PRD_SIZE_MAX,   //PRD_SIZE_MAX
	.periods_min = MIN_PERIODS,
};

int get_uac_test_mode(void);

static int uac_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct uac_rtd_params *prm;
	//struct g_audio *audio_dev;
	struct uac_device *uac_dev;
	struct uac_params *params;
	unsigned long flags;
	int err = 0;

	uac_dev = uac->uac_dev;
	params = &uac_dev->params;
	//printk("xiapengcheng substream->stream = %d,cmd = %d\n",substream->stream,cmd);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		prm = &uac->c_prm;
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		prm = &uac->p_prm;
	else
		return 0;
	spin_lock_irqsave(&prm->lock, flags);

	/* Reset */
	prm->hw_ptr = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		prm->ss = substream;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		prm->ss = NULL;
		break;
	default:
		err = -EINVAL;
	}

	spin_unlock_irqrestore(&prm->lock, flags);

#if 0
	/* Clear buffer after Play stops */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK && !prm->ss)
		memset(prm->rbuf, 0, prm->max_psize * params->req_number);
#endif

	return err;
}

static snd_pcm_uframes_t uac_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct uac_rtd_params *prm;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		prm = &uac->c_prm;
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		prm = &uac->p_prm;
	else
		return 0;

	return bytes_to_frames(substream->runtime, prm->hw_ptr);
}

static int uac_pcm_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *hw_params)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct uac_rtd_params *prm;
	int err;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		prm = &uac->c_prm;
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		prm = &uac->p_prm;
	else
		return 0;

	err = snd_pcm_lib_malloc_pages(substream,
					params_buffer_bytes(hw_params));
	if (err >= 0) {
		prm->dma_bytes = substream->runtime->dma_bytes;
		prm->dma_area = substream->runtime->dma_area;
		prm->period_size = params_period_bytes(hw_params);
	}
	//printk("xiapengcheng prm->period_size = %d,prm->dma_bytes = %d\n",prm->period_size,prm->dma_bytes);
	return err;
}

static int uac_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct uac_rtd_params *prm;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		prm = &uac->c_prm;
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		prm = &uac->p_prm;
	else
		return 0;

	prm->dma_area = NULL;
	prm->dma_bytes = 0;
	prm->period_size = 0;

	return snd_pcm_lib_free_pages(substream);
}

static int uac_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct uac_device *uac_dev;
	
	struct uac_params *params;
	int p_ssize, c_ssize;
	int p_srate, c_srate;
	int p_chmask, c_chmask;
	uac_dev = uac->uac_dev;

	params = &uac_dev->params;
	p_ssize = params->p_ssize;
	c_ssize = params->c_ssize;
	p_srate = params->p_srates[0];
	c_srate = params->c_srates[0];
	p_chmask = params->p_chmask;
	c_chmask = params->c_chmask;
	uac->p_residue = 0;

	uac_pcm_hardware.buffer_bytes_max = uac_dev->alsa_buf_num * PAGE_SIZE;
	uac_pcm_hardware.periods_max = uac_pcm_hardware.buffer_bytes_max/PRD_SIZE_MAX;

	runtime->hw = uac_pcm_hardware;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		spin_lock_init(&uac->c_prm.lock);
		runtime->hw.rate_min = c_srate;
		switch (c_ssize) {
		case 3:
			runtime->hw.formats = SNDRV_PCM_FMTBIT_S24_3LE;
			break;
		case 4:
			runtime->hw.formats = SNDRV_PCM_FMTBIT_S32_LE;
			break;
		default:
			runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
			break;
		}
		runtime->hw.channels_min = num_channels(c_chmask);
		runtime->hw.period_bytes_min = 2 * uac->c_prm.max_psize
						/ runtime->hw.periods_min;
	}
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		printk("2222222\n");
		spin_lock_init(&uac->p_prm.lock);
		runtime->hw.rate_min = p_srate;
		printk("33333\n");
		switch (p_ssize) {
		case 3:
			runtime->hw.formats = SNDRV_PCM_FMTBIT_S24_3LE;
			break;
		case 4:
			runtime->hw.formats = SNDRV_PCM_FMTBIT_S32_LE;
			break;
		default:
			runtime->hw.formats = SNDRV_PCM_FMTBIT_S16_LE;
			break;
		}
		runtime->hw.channels_min = num_channels(p_chmask);
		runtime->hw.period_bytes_min = 2 * uac->p_prm.max_psize
						/ runtime->hw.periods_min;
	}
	//printk("xiapengcheng runtime->hw.period_bytes_max = %d ,PAGE_SIZE = %d\n",runtime->hw.period_bytes_max,PAGE_SIZE);
	runtime->hw.rate_max = runtime->hw.rate_min;
	runtime->hw.channels_max = runtime->hw.channels_min;

	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);

	return 0;
}

/* ALSA cries without these function pointers */
static int uac_pcm_null(struct snd_pcm_substream *substream)
{
	return 0;
}

#ifdef VHD_UAC_DEBUG
struct save_work_data *work_data = NULL;
static int debug_tmp = 0;
static void uac_debug_save_buf_work(struct work_struct *work)
{
	struct save_work_data *work_data = container_of(work, struct save_work_data, work);
    loff_t pos = 0;

	if (debug & 2) {
        if (!work_data->file) {
            /* 首次调用时打开文件 */
			printk("[u_audio]open debug file\n");
            work_data->file = filp_open("/nvdata/kuac_dump.pcm", O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (IS_ERR(work_data->file)) {
                printk("Failed to open dump file /nvdata/kuac_dump.pcm, err %ld\n", PTR_ERR(work_data->file));
                work_data->file = NULL;
            }
        }

        if (work_data->file) {
            ssize_t written = kernel_write(work_data->file, work_data->buf[work_data->i_ready], work_data->len_ready, &pos);
            if (written != work_data->len_ready) {
                printk("Failed to write dump data (%zd/%lu)\n", written, work_data->len_ready);
            }
        }
    } else if (work_data->file) {
        /* 调试模式关闭时关闭文件 */
		if(work_data->file != NULL) {
			filp_close(work_data->file, NULL);
			work_data->file = NULL;
		}
    }
}

static int uac_debug_init(void)
{
	struct save_work_data *tmp_work_data = NULL;

	if(work_data) {
		printk("uac_debug_init, work_data already exist\n");
		return 0;
	}

	tmp_work_data = kzalloc(sizeof(struct save_work_data), GFP_KERNEL);

	if (!tmp_work_data) {
		printk("Failed to allocate work data\n");
		return -ENOMEM;
	}
	INIT_WORK(&tmp_work_data->work, uac_debug_save_buf_work);
	tmp_work_data->buf_len = 192 * 10;
	tmp_work_data->buf[0] = kzalloc(tmp_work_data->buf_len, GFP_KERNEL);
	tmp_work_data->buf[1] = kzalloc(tmp_work_data->buf_len, GFP_KERNEL);
	if(!tmp_work_data->buf[0] || !tmp_work_data->buf[1]) {
		printk("Failed to allocate work data\n");
		kfree(tmp_work_data->buf[0]);
		kfree(tmp_work_data->buf[1]);
		kfree(tmp_work_data);
		return -ENOMEM;
	}

	tmp_work_data->index = 0;
	tmp_work_data->i_ready = 0;
	tmp_work_data->len_offset = 0;
	tmp_work_data->len_ready = 0;
	tmp_work_data->file = NULL;
	work_data = tmp_work_data;
	printk("uac_debug_init, buf len %zu\n", work_data->buf_len);
	return 0;
}

static int uac_debug_uninit(void)
{
	if(work_data) {
		flush_work(&work_data->work);
		kfree(work_data->buf[0]);
		kfree(work_data->buf[1]);
		kfree(work_data);
		work_data = NULL;
	}
	return 0;
}

static int uac_debug_save_buf(char *buf, size_t len)
{
	if(work_data == NULL)
		return -1;

	if ((debug_tmp & 2) == 0) {
		if(work_data->file) {
			schedule_work(&work_data->work);
		}
		return 0;
	}

	if (len + work_data->len_offset > work_data->buf_len) {
		if(work_data->len_offset == 0) {
			printk("[u_audio]error %s %d\n", __func__, __LINE__);
		}
		else{
			work_data->i_ready = work_data->index;
			work_data->len_ready = work_data->len_offset;
			work_data->index = (work_data->index + 1) % 2;
			work_data->len_offset = 0;
			printk("[u_audio]save buf %d\n", work_data->i_ready);
			schedule_work(&work_data->work);
		}
	}
	memcpy(work_data->buf[work_data->index] + work_data->len_offset, buf, len);
	work_data->len_offset += len;
	printk("[u_audio]%s, len %ld, off %ld\n", __func__, len, work_data->len_offset);

	return 0;
}

static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t len = 0;

	len += sprintf(buf + len, "Current debug value: %d\n", debug);

	if (debug & 7) {
		len += sprintf(buf + len, "[Status] MIC data saving ACTIVE (output to /data/kuac_dump*.pcm)\n");
	} else {
		len += sprintf(buf + len, "[Status] MIC data saving INACTIVE\n");
	}

	if (debug & (1 << VHD_UAC_DEBUG_PRINT_EVENT_BIT)) {
		len += sprintf(buf + len, "[Status] UAC event printing ENABLED\n");
	} else {
		len += sprintf(buf + len, "[Status] UAC event printing DISABLED\n");
	}

	len += sprintf(buf + len, "\nUsage:\n");
	len += sprintf(buf + len, "  - Set to 1 to print len\n");
	len += sprintf(buf + len, "  - Set to 2 to save MIC data\n");
	len += sprintf(buf + len, "  - Set to 0 to disable all\n");

	return len;
}

static ssize_t debug_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;

	debug = val;

	if(debug & 7)
		uac_debug_init();
	else if((debug & 7) == 0)
		uac_debug_uninit();

	return count;
}

static struct kobj_attribute debug_attr = __ATTR(debug, 0644, debug_show, debug_store);

#endif

static struct snd_pcm_ops uac_pcm_ops = {
	.open = uac_pcm_open,
	.close = uac_pcm_null,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = uac_pcm_hw_params,
	.hw_free = uac_pcm_hw_free,
	.trigger = uac_pcm_trigger,
	.pointer = uac_pcm_pointer,
	.prepare = uac_pcm_null,
};

static void
__uac_audio_complete_playback(struct usb_ep *ep, struct usb_request *req)
{
	unsigned pending;
	unsigned long flags;
	unsigned int hw_ptr;
	bool update_alsa = false;
	int status = req->status;
	struct uac_req_play *ur = req->context;
	struct snd_pcm_substream *substream;
	struct uac_rtd_params *prm = ur->pp;
	struct snd_uac_chip *uac = prm->uac;
	struct uac_device *uac_dev = uac->uac_dev;

	//printk("%s, %d\n",__FUNCTION__,__LINE__);
	if (!prm->ep_enabled || req->status == -ESHUTDOWN){
		printk("%s, %d ep_enabled[%d],status[%d]\n",__FUNCTION__,__LINE__,prm->ep_enabled,req->status);
		//if (usb_ep_queue(ep, req, GFP_ATOMIC))
		//	dev_err(uac->card->dev, "%d Error!\n", __LINE__);
		return;
	}
	/*
	 * We can't really do much about bad xfers.
	 * Afterall, the ISOCH xfers could fail legitimately.
	 */
	if (status)
		pr_debug("%s: iso_complete status(%d) %d/%d\n",
			__func__, status, req->actual, req->length);

	substream = prm->ss;

	/* Do nothing if ALSA isn't active */
	if (!substream){
		//printk("%s, %d\n",__FUNCTION__,__LINE__);
		goto exit;
	}
	spin_lock_irqsave(&prm->lock, flags);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) 
	{
		/*
		 * For each IN packet, take the quotient of the current data
		 * rate and the endpoint's interval as the base packet size.
		 * If there is a residue from this division, add it to the
		 * residue accumulator.
		 */
		req->length = uac_dev->p_pktsize;
		uac_dev->p_residue += uac_dev->p_pktsize_residue;

		/*
		 * Whenever there are more bytes in the accumulator than we
		 * need to add one more sample frame, increase this packet's
		 * size and decrease the accumulator.
		 */

		if (uac_dev->p_residue / uac_dev->p_interval >= uac_dev->p_framesize)
		{
			req->length += uac_dev->p_framesize;
			uac_dev->p_residue -= uac_dev->p_framesize *
				uac_dev->p_interval;
		}

		req->actual = req->length;
        //printk("req->length:%d req->actual:%d\n", req->length, req->actual);
	}
    pending = prm->hw_ptr % prm->period_size;
    pending += req->actual;
    if (pending >= prm->period_size)
        update_alsa = true;

    hw_ptr = prm->hw_ptr;
    prm->hw_ptr = (prm->hw_ptr + req->actual) % prm->dma_bytes;
    //printk("%s %d prm->hw_ptr = %ld ,hw_ptr = %d\n",__func__,__LINE__,prm->hw_ptr,hw_ptr);
    spin_unlock_irqrestore(&prm->lock, flags);

    /* Pack USB load in ALSA ring buffer */
    pending = prm->dma_bytes - hw_ptr;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        if (unlikely(pending < req->actual)) {
            memcpy(req->buf, prm->dma_area + hw_ptr, pending);
            memcpy(req->buf + pending, prm->dma_area,
                    req->actual - pending);
        } else {
            memcpy(req->buf, prm->dma_area + hw_ptr, req->actual);
        }
    } 
    //printk("xiapengcheng pending = %d,req->actual = %d\n",pending,req->actual);
exit:
    if (usb_ep_queue(ep, req, GFP_ATOMIC))
        dev_err(uac->card->dev, "%d Error!\n", __LINE__);

    if (update_alsa)
        snd_pcm_period_elapsed(substream);
	
}

static void
__uac_audio_complete_playback_test(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	struct uac_req_play *ur = req->context;
	struct uac_rtd_params *prm = ur->pp;
	struct snd_uac_chip *uac = prm->uac;
	static int test_cnt = 0;


	//printk("%s, %d\n",__FUNCTION__,__LINE__);
	if (!prm->ep_enabled || req->status == -ESHUTDOWN){
		printk("%s, %d ep_enabled[%d],status[%d]\n",__FUNCTION__,__LINE__,prm->ep_enabled,req->status);
		//if (usb_ep_queue(ep, req, GFP_ATOMIC))
		//	dev_err(uac->card->dev, "%d Error!\n", __LINE__);
		return;
	}

	/*
	 * We can't really do much about bad xfers.
	 * Afterall, the ISOCH xfers could fail legitimately.
	 */
	if (status)
		pr_debug("%s: iso_complete status(%d) %d/%d\n",
			__func__, status, req->actual, req->length);

#if 1
	if (req->actual < req->length)
	{
		printk("uac complete error: actual:%d, length:%d\n",
			req->actual, req->length);
	}
#endif

	memset(req->buf, test_cnt, req->length);
	if(test_cnt >= 0xFF)
	{
		test_cnt = 0;
	}
	else
	{
		test_cnt ++;
	}
	
    if (usb_ep_queue(ep, req, GFP_ATOMIC))
        dev_err(uac->card->dev, "%d Error!\n", __LINE__);	
}


static void
__uac_audio_complete_capture(struct usb_ep *ep, struct usb_request *req)
{
	unsigned pending;
	unsigned long flags;
	unsigned int hw_ptr;
	bool update_alsa = false;
	//int status = req->status;
	struct uac_req_cap *ur = req->context;
	struct snd_pcm_substream *substream;
	struct uac_rtd_params *prm = ur->pp;
#ifdef CHIP_RK3588
	struct snd_uac_chip *uac = prm->uac;
	static bool out_req_start_state = false;
	struct uac_device *uac_dev = uac->uac_dev;
#endif
	//struct snd_uac_chip *uac = prm->uac;
	//printk("%s, %d ep_enabled[%d],status[%d]\n",__FUNCTION__,__LINE__,prm->ep_enabled,req->status);
	/* i/f shutting down */
	if (!prm->ep_enabled || req->status == -ESHUTDOWN){
#ifdef CHIP_RK3588
		out_req_start_state = false;
#endif
		//printk(KERN_INFO "%s, %d ep_enabled[%d],status[%d]\n",__FUNCTION__,__LINE__,prm->ep_enabled,req->status);
		//if (usb_ep_queue(ep, req, GFP_ATOMIC))
		//	dev_err(uac->card->dev, "%d Error!\n", __LINE__);
		return;
	}
	/*
	 * We can't really do much about bad xfers.
	 * Afterall, the ISOCH xfers could fail legitimately.
	 */
	//if (status)
		//printk(KERN_INFO "%s: iso_complete status(%d) %d/%d\n",
		//	__func__, status, req->actual, req->length);

	if(req->status < 0 || req->actual <= 0)
	{
		if (usb_ep_queue(ep, req, GFP_ATOMIC))
			printk(KERN_INFO "%d Error!\n", __LINE__);
		return;
	}
#ifdef CHIP_RK3588
	else if(req->status == 0)
	{
		if (!out_req_start_state) {
			/* The fisrt out req complete successfully */
			out_req_start_state = true;
		} else {
			//printk("isoc compl del timer\n");
			del_timer(&uac_dev->wait_out_req_complete_timer);
		}
	}
#endif

	substream = prm->ss;

	/* Do nothing if ALSA isn't active */
	if (!substream){
		//printk("%s, %d\n",__FUNCTION__,__LINE__);
		goto exit;
	}
	spin_lock_irqsave(&prm->lock, flags);

	pending = prm->hw_ptr % prm->period_size;
	pending += req->actual;
	if (pending >= prm->period_size)
		update_alsa = true;

	hw_ptr = prm->hw_ptr;
	prm->hw_ptr = (prm->hw_ptr + req->actual) % prm->dma_bytes;
	//printk("%s %d prm->hw_ptr = %ld ,hw_ptr = %d\n",__func__,__LINE__,prm->hw_ptr,hw_ptr);
	spin_unlock_irqrestore(&prm->lock, flags);

	/* Pack USB load in ALSA ring buffer */
	pending = prm->dma_bytes - hw_ptr;
#ifdef VHD_UAC_DEBUG
	debug_tmp = debug;
#endif

	if (unlikely(pending < req->actual)) {
		memcpy(prm->dma_area + hw_ptr, req->buf, pending);
		memcpy(prm->dma_area, req->buf + pending,
			   req->actual - pending);
	} else {
		memcpy(prm->dma_area + hw_ptr, req->buf, req->actual);
	}
	//printk("xiapengcheng pending = %d,req->actual = %d\n",pending,req->actual);
#ifdef VHD_UAC_DEBUG
		uac_debug_save_buf(req->buf, req->actual);
		if (debug & 1)
		{
			printk("b %d/%d, ptr %05d/%05zu, p %d\n", req->length, req->actual,
				hw_ptr, prm->dma_bytes, pending);
		}
#endif
exit:
	if (usb_ep_queue(ep, req, GFP_ATOMIC))
		printk(KERN_INFO "%d Error!\n", __LINE__);
#ifdef CHIP_RK3588
	else{
		mod_timer(&uac_dev->wait_out_req_complete_timer,
			  jiffies + UAC_WAIT_OUT_REQ_COMPLETE_TIMEOUT * HZ);
		//printk("isoc compl add timer\n");
	}
#endif

	if (update_alsa)
		snd_pcm_period_elapsed(substream);
}



static inline void free_ep(struct uac_device *dev, struct usb_ep *ep)
{
	struct uac_params *params;
	int i;
	struct uac_rtd_params *prm = &dev->uac->p_prm;
	if(prm == NULL){
		printk(KERN_INFO "%s:%d Error!\n", __func__, __LINE__);
	}
	if (!prm->ep_enabled)
		return;

	prm->ep_enabled = false;
	if (usb_ep_disable(ep))
		printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
	params = &dev->params;

	for (i = 0; i < params->req_number; i++) {
		if (prm->ureq[i].req) {
			usb_ep_dequeue(ep, prm->ureq[i].req);
			usb_ep_free_request(ep, prm->ureq[i].req);
			prm->ureq[i].req = NULL;
		}
	}

	
	/* Reset */
	prm->hw_ptr = 0;
}

int uac2_set_capture_srate(struct uac_device *uac_dev, int srate)
{
	struct uac_params *params = &uac_dev->params;
	struct snd_uac_chip *uac = uac_dev->uac;
	struct uac_rtd_params *prm;

	dev_dbg(&uac_dev->gadget->dev, "%s: srate %d\n", __func__, srate);
	prm = &uac->c_prm;
	params->c_srates[0] = srate; 

	return 0;
}
EXPORT_SYMBOL_GPL(uac2_set_capture_srate);

int uac2_set_playback_srate(struct uac_device *uac_dev, int srate)
{
	struct uac_params *params = &uac_dev->params;
	struct snd_uac_chip *uac = uac_dev->uac;
	struct uac_rtd_params *prm;

	dev_dbg(&uac_dev->gadget->dev, "%s: srate %d\n", __func__, srate);
	prm = &uac->p_prm;
	params->p_srates[0] = srate; 

	return 0;
}
EXPORT_SYMBOL_GPL(uac2_set_playback_srate);

int uac_device_start_capture(struct uac_device *uac_dev)
{
	struct usb_gadget *gadget = uac_dev->gadget;
	struct usb_request *req;
	struct usb_ep *ep;
	struct uac_params *params = &uac_dev->params;
	struct uac_rtd_params *prm;
	struct snd_uac_chip *uac = uac_dev->uac;
	int req_len, i;

	prm = &uac->c_prm;
	ep = uac_dev->out_ep;
	if (!ep->desc)
		config_ep_by_speed(gadget, &uac_dev->func, ep);
	if (prm->ep_enabled)
		return 0;
	req_len = prm->max_psize;
	/* Reset */
	prm->hw_ptr = 0;
	prm->ep_enabled = true;
	usb_ep_enable(ep);

	//printk(KERN_EMERG "start capture %s %d\n", __func__, __LINE__);
	for (i = 0; i < params->req_number; i++) {
		if (!prm->ureq[i].req) {
			req = usb_ep_alloc_request(ep, GFP_ATOMIC);
			if (req == NULL)
				return -ENOMEM;

			prm->ureq[i].req = req;
			prm->ureq[i].pp = prm;

			req->zero = 0;
			req->context = &prm->ureq[i];
			req->length = req_len;
			req->complete = __uac_audio_complete_capture;
			req->buf = prm->rbuf + i * prm->max_psize;
			memset(req->buf, 0, req->length);
		}

		if (usb_ep_queue(ep, prm->ureq[i].req, GFP_ATOMIC))
			printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
	}

	return 0;
}

static inline void free_ep_cap(struct uac_device *dev, struct usb_ep *ep)
{
	struct uac_params *params;
	int i;
	struct uac_rtd_params *prm = &dev->uac->c_prm;
	if(prm == NULL){
		printk(KERN_INFO "%s:%d Error!\n", __func__, __LINE__);
	}
	if (!prm->ep_enabled)
		return;

	prm->ep_enabled = false;
	if (usb_ep_disable(ep))
		printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
	params = &dev->params;

	for (i = 0; i < params->req_number; i++) {
		if (prm->ureq[i].req) {
			usb_ep_dequeue(ep, prm->ureq[i].req);
			usb_ep_free_request(ep, prm->ureq[i].req);
			prm->ureq[i].req = NULL;
		}
	}
	prm->hw_ptr = 0;
	memset(prm->dma_area, 0, prm->dma_bytes);

}

void uac_device_stop_capture(struct uac_device *uac_dev)
{
	free_ep_cap(uac_dev, uac_dev->out_ep);
}

int uac_device_start_playback(struct uac_device *uac_dev)
{
	struct usb_gadget *gadget = uac_dev->gadget;
	struct usb_request *req;
	struct usb_ep *ep;
	struct uac_params *params = &uac_dev->params;
	struct uac_rtd_params *prm;
	struct snd_uac_chip *uac = uac_dev->uac;
	const struct usb_endpoint_descriptor *ep_desc;
	int req_len, i;
	unsigned int factor, rate;
	
	printk("uac_device_start_playback\n");
	   
	prm = &uac->p_prm;
	
	if (prm->ep_enabled)
		return 0;

	ep = uac_dev->in_ep;
	if (!ep->desc)
		config_ep_by_speed(gadget, &uac_dev->func, ep);
	ep_desc = ep->desc;

	/* pre-calculate the playback endpoint's interval */
	if (gadget->speed == USB_SPEED_FULL)
		factor = 1000;
	else
		factor = 8000;

	/* pre-compute some values for iso_complete() */
    
	uac_dev->p_framesize = params->p_ssize *
		num_channels(params->p_chmask);

	rate = params->p_srates[0] * uac_dev->p_framesize;
	uac_dev->p_interval = factor / (1 << (ep_desc->bInterval - 1));
	uac_dev->p_pktsize = min_t(unsigned int, rate / uac_dev->p_interval,prm->max_psize);


	if (uac_dev->p_pktsize < prm->max_psize)
		uac_dev->p_pktsize_residue = rate % uac_dev->p_interval;
	else
		uac_dev->p_pktsize_residue = 0;

	req_len = uac_dev->p_pktsize;
	uac_dev->p_residue = 0;
	prm->hw_ptr = 0;

	prm->ep_enabled = true;
	usb_ep_enable(ep);

	/* printk(KERN_EMERG "p_framesize=%d p_interval=%d p_pktsize=%d 
					//p_pktsize_residue=%d m=%d \n", 
					uac_dev->p_framesize,	   
					uac_dev->p_interval,	   
					uac_dev->p_pktsize,
					uac_dev->p_pktsize_residue,
					uac_dev->max_psize); */

	for (i = 0; i < params->req_number; i++) {
		if (!prm->ureq[i].req) {
			req = usb_ep_alloc_request(ep, GFP_ATOMIC);
			if (req == NULL)
				return -ENOMEM;

			prm->ureq[i].req = req;
			req->zero = 0;
			req->length = req_len;

			req->context = &prm->ureq[i];
			prm->ureq[i].pp = prm;
			if(get_uac_test_mode() == 1)
			{
				req->complete = __uac_audio_complete_playback_test;
			}
			else
			{
				req->complete = __uac_audio_complete_playback;
			}
			req->buf = prm->rbuf + i * prm->max_psize;
			memset(req->buf, 0, req->length);
		}

		if (usb_ep_queue(ep, prm->ureq[i].req, GFP_ATOMIC))
			printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
	}

	return 0;
}

void uac_device_stop_playback(struct uac_device *uac_dev)
{
	free_ep(uac_dev, uac_dev->in_ep);
}

#ifdef CHIP_RK3588
static void audio_wait_out_req_complete_watchdog(struct timer_list *t)
{
	struct uac_device *uac_dev = from_timer(uac_dev, t,
					     wait_out_req_complete_timer);
	struct uac_rtd_params *prm;
	struct snd_uac_chip *uac = uac_dev->uac;

	prm = &uac->c_prm;

	printk("%s: line %d\n", __func__, __LINE__);
	if (!uac_dev || !uac_dev->uac  ||
	    uac_dev->gadget->state != USB_STATE_CONFIGURED || get_usb_link_state() != 0)
		return;

	//printk("%s: line %d\n", __func__, __LINE__);
	if (prm->ep_enabled) {
		//printk("%s: line %d\n", __func__, __LINE__);
		uac_device_stop_capture(uac_dev);
		uac_device_start_capture(uac_dev);
	}
}
#endif

int uac_device_setup(struct uac_device *uac_dev)
{
	struct uac_params *params;
	//struct uac_params *params_cap;
	int p_chmask,c_chmask;
	struct snd_card *card;
	struct snd_pcm *pcm;
	char *card_name = "UAC1_Gadget";
	char *pcm_name = "UAC1_PCM";
	int err = 0;
	struct snd_uac_chip *uac;
	if (!uac_dev)
		return -EINVAL;

	uac = kzalloc(sizeof(*uac), GFP_KERNEL);
	if (!uac)
		return -ENOMEM;

	uac_dev->uac = uac;
	uac->uac_dev = uac_dev;
	params = &uac_dev->params;
	p_chmask = params->p_chmask;
	c_chmask = params->c_chmask;
	if (c_chmask) {
		struct uac_rtd_params *prm = &uac->c_prm;

		uac->c_prm.uac = uac;
		prm->max_psize = uac_dev->out_ep_maxpsize;

		prm->ureq = kcalloc(params->req_number, sizeof(struct uac_req_cap),
				GFP_KERNEL);
		if (!prm->ureq) {
			err = -ENOMEM;
			printk(KERN_INFO "xiapengcheng g_audio_setup fail 1111\n");
			goto fail;
		}

		prm->rbuf = kcalloc(params->req_number, prm->max_psize,
				GFP_KERNEL);
		if (!prm->rbuf) {
			prm->max_psize = 0;
			err = -ENOMEM;
			printk(KERN_INFO "xiapengcheng g_audio_setup fail 2222\n");
			goto fail;
		}
	}
	if (p_chmask) {
		
			struct uac_rtd_params *prm = &uac->p_prm;
			uac->p_prm.uac = uac;
			//prm->max_psize = uac_dev->out_ep_maxpsize;
			prm->max_psize = uac_dev->in_ep_maxpsize;
			
			prm->ep_enabled = false;
	
			prm->ureq = kcalloc(params->req_number, sizeof(struct uac_req_play),
					GFP_KERNEL);
			if (!prm->ureq) {
				err = -ENOMEM;
				printk(KERN_INFO "xiapengcheng g_audio_setup fail 1111\n");
				goto fail;
			}
	
			prm->rbuf = kcalloc(params->req_number, prm->max_psize,
					GFP_KERNEL);
			if (!prm->rbuf) {
				prm->max_psize = 0;
				err = -ENOMEM;
				printk(KERN_INFO "xiapengcheng g_audio_setup fail 2222\n");
				goto fail;
			}
	}

	/* Choose any slot, with no id */
	err = snd_card_new(&uac_dev->gadget->dev,
			-1, NULL, THIS_MODULE, 0, &card);
	if (err < 0){
		printk("xiapengcheng g_audio_setup fail 5555\n");
		goto fail;
	}
	uac->card = card;

	/*
	 * Create first PCM device
	 * Create a substream only for non-zero channel streams
	 */
	err = snd_pcm_new(uac->card, pcm_name, 0,
			       p_chmask ? 1 : 0, c_chmask ? 1 : 0, &pcm);
	if (err < 0){
		printk("xiapengcheng g_audio_setup fail 6666\n");
		goto snd_fail;
	}
	strcpy(pcm->name, pcm_name);
	pcm->private_data = uac;
	uac->pcm = pcm;

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &uac_pcm_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &uac_pcm_ops);

	strcpy(card->driver, card_name);
	strcpy(card->shortname, card_name);
	sprintf(card->longname, "%s %i", card_name, card->dev->id);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
        snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
               NULL, 0, uac_pcm_hardware.buffer_bytes_max);
#else
      snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
               snd_dma_continuous_data(GFP_KERNEL), 0, uac_pcm_hardware.buffer_bytes_max);
#endif

#ifdef VHD_UAC_DEBUG
    u_audio_kobj = kobject_create_and_add("u_audio", kernel_kobj);
    if (u_audio_kobj) {
        if (sysfs_create_file(u_audio_kobj, &debug_attr.attr)) {
            pr_err("Failed to create sysfs file\n");
            kobject_put(u_audio_kobj);
        }
    }
#endif

	err = snd_card_register(card);
#ifdef CHIP_RK3588
	if (err)
		goto snd_fail;
	timer_setup(&uac_dev->wait_out_req_complete_timer,
	    audio_wait_out_req_complete_watchdog, 0);
	return 0;
#else
	if (!err)
		return 0;
#endif

snd_fail:
		snd_card_free(card);
		

fail:
	kfree(uac_dev->ureq);
	kfree(uac_dev->rbuf);
	kfree(uac->c_prm.ureq);
	kfree(uac->c_prm.rbuf);
	kfree(uac->p_prm.ureq);
	kfree(uac->p_prm.rbuf);
	kfree(uac);
	return err;
}

void uac_device_cleanup(struct uac_device *uac_dev)
{
	struct snd_card *card;
	if (!uac_dev)
		return;

#ifdef VHD_UAC_DEBUG
	if (u_audio_kobj)
        kobject_put(u_audio_kobj);
#endif

#ifdef CHIP_RK3588
	del_timer_sync(&uac_dev->wait_out_req_complete_timer);
	printk("%s: line %d\n", __func__, __LINE__);
#endif
	if(uac_dev->uac){
		card = uac_dev->uac->card;
		if (card)
			snd_card_free(card);
		kfree(uac_dev->uac->c_prm.ureq);
		kfree(uac_dev->uac->c_prm.rbuf);
		kfree(uac_dev->uac->p_prm.ureq);
		kfree(uac_dev->uac->p_prm.rbuf);
		kfree(uac_dev->uac);
	}
	kfree(uac_dev->ureq);
	kfree(uac_dev->rbuf);
	
}

#ifdef VHD_UAC_DEBUG
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif
