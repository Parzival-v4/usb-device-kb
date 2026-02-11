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
#include "uac_ex.h"

struct snd_uac_chip {
	struct uac_device *uac_dev;
	struct uac_rtd_params p_prm;

	struct snd_card *card;
	struct snd_pcm *pcm;
};


#define BUFF_SIZE_MAX	(PAGE_SIZE * 6)
#define PRD_SIZE_MAX	PAGE_SIZE
#define MIN_PERIODS	2
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
    //printk("uac_pcm_trigger enter\n");

	uac_dev = uac->uac_dev;
	params = &uac_dev->params;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
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
		//dump_stack();
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
	{
		//printk("Clear uac buffer\n");
		memset(prm->rbuf, 0, prm->max_psize * params->req_number);
	}
#endif

	return err;
}

static snd_pcm_uframes_t uac_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct uac_rtd_params *prm;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
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
    printk("uac_pcm_hw_params enter\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
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

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
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
	p_srate = params->p_srate;
	c_srate = params->c_srate;
	p_chmask = params->p_chmask;
	c_chmask = params->c_chmask;
	uac_dev->p_residue = 0;


	uac_pcm_hardware.buffer_bytes_max = uac_dev->alsa_buf_num * PAGE_SIZE;
	uac_pcm_hardware.periods_max = uac_pcm_hardware.buffer_bytes_max/PRD_SIZE_MAX;
	runtime->hw = uac_pcm_hardware;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		spin_lock_init(&uac->p_prm.lock);
		runtime->hw.rate_min = p_srate;
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

#if 0
static void
__uac_audio_complete_v4l2(struct usb_ep *ep, struct usb_request *req)
{
	struct uac_req_play *ur = req->context;
	//struct snd_pcm_substream *substream;
	struct uac_rtd_params *prm = ur->pp;
	struct snd_uac_chip *uac = prm->uac;

	//struct uac_device *uac_dev = req->context;
	struct uac_device *uac_dev = uac->uac_dev;
	struct uac_queue *queue = &uac_dev->queue;
	struct uac_buffer *buf;

	unsigned long flags;
	int ret;

	switch (req->status) {
	case 0:
		break;

	case -ESHUTDOWN:	/* disconnect from host. */
		printk(KERN_DEBUG "AS request cancelled.\n");
		uac_queue_cancel(queue, 1);
		goto err;

	default:
		printk(KERN_INFO "AS request completed with status %d.\n",
			req->status);
		uac_queue_cancel(queue, 0);
		goto err;
	}

	if (!prm->ep_enabled)
	{
		uac_queue_cancel(queue, 0);
		goto err;
	}

	spin_lock_irqsave(&uac_dev->queue.irqlock, flags);
	buf = uac_queue_head(&uac_dev->queue);
	if (buf == NULL) {
		req->length = 0;
		goto tran_zero;
	}
	else
	{
		unsigned int nbytes;
		void *mem;

		/* Copy video data to the USB buffer. */
		mem = buf->mem + queue->buf_used;
		nbytes = min((unsigned int)req->length, buf->bytesused - queue->buf_used);


#if 0
		char *p = (char *)mem;
		printk(KERN_INFO "req->buf[0]:%x nbytes:%d req->length:%d uac_dev->queue.buf_used: %d buf->bytesused: %d \n", *p, nbytes, req->length, uac_dev->queue.buf_used, buf->bytesused);
#endif

		memcpy(req->buf, mem, nbytes);
		queue->buf_used += nbytes;

		req->length = nbytes;
		//req->actual = req->length;

		if (buf->bytesused == uac_dev->queue.buf_used) {
			uac_dev->queue.buf_used = 0;
			buf->state = UVC_BUF_STATE_DONE;
			uac_queue_next_buffer(&uac_dev->queue, buf);
		}
	}

tran_zero:

	//req->actual = req->length;

	if ((ret = usb_ep_queue(ep, req, GFP_ATOMIC)) < 0) {
		printk(KERN_INFO "Failed to queue request (%d).\n", ret);
		usb_ep_set_halt(ep);
		//spin_unlock_irqrestore(&uac_dev->queue.irqlock, flags);
		uac_queue_cancel(queue, 0);
	}
	spin_unlock_irqrestore(&uac_dev->queue.irqlock, flags);

err:
	return;
}
#endif

static void
__uac_audio_complete_alsa(struct usb_ep *ep, struct usb_request *req)
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

#if 0
	if (req->actual < req->length)
	{
		printk("uac complete error: actual:%d, length:%d\n",
			req->actual, req->length);
	}
#endif

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
    //printk("xiapengcheng prm->hw_ptr = %d ,hw_ptr = %d\n",prm->hw_ptr,hw_ptr);
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

void
__uac_audio_complete_test(struct usb_ep *ep, struct usb_request *req)
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
	params = &dev->params;

	for (i = 0; i < params->req_number; i++) {
		if (prm->ureq[i].req) {
			usb_ep_dequeue(ep, prm->ureq[i].req);
			usb_ep_free_request(ep, prm->ureq[i].req);
			prm->ureq[i].req = NULL;
		}
	}

	if (usb_ep_disable(ep))
		printk(KERN_EMERG "%s:%d Error!\n", __func__, __LINE__);
	
	/* Reset */
	prm->hw_ptr = 0;
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
	config_ep_by_speed(gadget, &uac_dev->func, ep);
	ep_desc = ep->desc;

	/* pre-calculate the playback endpoint's interval */
	if (gadget->speed == USB_SPEED_FULL)
		factor = 1000;
	else
		factor = 8000;

	/* pre-compute some values for iso_complete() */
    
    printk("params->p_chmask:%d params->p_ssize:%d\n", params->p_chmask, params->p_ssize);
	uac_dev->p_framesize = params->p_ssize *
		num_channels(params->p_chmask);

    printk("uac_dev->p_framesize:%d\n", uac_dev->p_framesize);
	rate = params->p_srate * uac_dev->p_framesize;
    printk("params->p_srate:%d\n", params->p_srate);
    printk("rate:%d\n", rate);
	uac_dev->p_interval = factor / (1 << (ep_desc->bInterval - 1));
    printk("ep_desc->bInterval:%d\n", ep_desc->bInterval);
    printk("uac_dev->p_interval:%d\n", uac_dev->p_interval);
	uac_dev->p_pktsize = min_t(unsigned int, rate / uac_dev->p_interval,prm->max_psize);

    printk("uac_dev->max_psize:%d\n", prm->max_psize);

	if (uac_dev->p_pktsize < prm->max_psize)
		uac_dev->p_pktsize_residue = rate % uac_dev->p_interval;
	else
		uac_dev->p_pktsize_residue = 0;

	req_len = uac_dev->p_pktsize;
    //
    //req_len = 64;
    //
	uac_dev->p_residue = 0;
	/* Reset */
	prm->hw_ptr = 0;

	prm->ep_enabled = true;
	usb_ep_enable(ep);

	//printk(KERN_EMERG "p_framesize=%d p_interval=%d p_pktsize=%d p_pktsize_residue=%d m=%d \n",uac->p_framesize, uac->p_interval,uac->p_pktsize,uac->p_pktsize_residue, uac->max_psize);

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
				req->complete = __uac_audio_complete_test;
			}
			else
			{
				req->complete = __uac_audio_complete_alsa;
			}	

			req->buf = prm->rbuf + i * prm->max_psize;
			memset(req->buf, 0, req->length);
            //printk("req->length:%d\n", req->length);
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

int uac_device_setup(struct uac_device *uac_dev)
{
	struct uac_params *params;
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

	err = snd_card_register(card);
	//printk("xiapengcheng g_audio_setup OK\n");
	

	
	if (!err)
		return 0;

snd_fail:
	snd_card_free(card);
		

fail:
	kfree(uac->p_prm.ureq);
	kfree(uac->p_prm.rbuf);
	kfree(uac);
	return err;
}

void uac_device_cleanup(struct uac_device *uac_dev)
{
	if (!uac_dev)
		return;

	kfree(uac_dev->ureq);
	kfree(uac_dev->rbuf);
}
