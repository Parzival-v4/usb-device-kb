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
#include <linux/ktime.h>
#include <media/v4l2-event.h>

#include "uac_queue_ex.h"
#include "uac_ex_speak.h"

extern int get_usb_link_state(void);

//#define VHD_UAC_DEBUG
#ifdef VHD_UAC_DEBUG

#define VHD_UAC_DEBUG_PRINT_LEN_BIT		0
#define VHD_UAC_DEBUG_SAVE_FILE_BIT		1
#define VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT	2

static struct kobject *u_audio_kobj;
static int debug = 0;

struct save_work_data {
    struct work_struct work;  // 必须作为第一个成员
    
    // 捕获数据相关
    char *cap_buf[2];            // 捕获数据缓冲区
    size_t cap_buf_len;          // 捕获数据缓冲区长度
    size_t cap_len_ready;        // 准备写入的捕获数据长度
    size_t cap_len_offset;       // 当前捕获缓冲区偏移
    int cap_index;               // 当前使用的捕获缓冲区索引
    int cap_i_ready;             // 准备写入文件的捕获缓冲区索引
    struct file *cap_file;       // 捕获数据文件指针
    
    // 播放数据相关
    char *play_buf[2];           // 播放数据缓冲区
    size_t play_buf_len;         // 播放数据缓冲区长度
    size_t play_len_ready;       // 准备写入的播放数据长度
    size_t play_len_offset;      // 当前播放缓冲区偏移
    int play_index;              // 当前使用的播放缓冲区索引
    int play_i_ready;            // 准备写入文件的播放缓冲区索引
    struct file *play_file;      // 播放数据文件指针
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

#define BUFF_SIZE_MAX		(PAGE_SIZE * 24) //最大支持48k，8声道，16bit，最大max预设为48000*2*8/1000*20*6/4096=22.5
#define BUFF_SIZE_MAX_DEF	(PAGE_SIZE * 6)
#define PRD_SIZE_MAX_DEF	PAGE_SIZE
#define MIN_PERIODS	4
#define MAX_PERIODS_MS		20

#ifdef CHIP_RK3588
#define UAC_WAIT_OUT_REQ_COMPLETE_TIMEOUT	2
#endif

static struct snd_pcm_hardware uac_pcm_hardware = {
	.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER
		 | SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID
		 | SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.rates = SNDRV_PCM_RATE_CONTINUOUS,
	.periods_max = BUFF_SIZE_MAX_DEF / PRD_SIZE_MAX_DEF,
	.buffer_bytes_max = BUFF_SIZE_MAX_DEF,
	.period_bytes_max = PRD_SIZE_MAX_DEF,
	.periods_min = MIN_PERIODS,
};

int get_uac_test_mode(void);

static int uac_pcm_hardware_resize(struct uac_device *uac_dev)
{
	size_t period_bytes_max_ctmp, period_bytes_max_ptmp, period_bytes_max_tmp;
	size_t buffer_bytes_max_tmp;
	struct uac_params *params;
	int p_ssize, c_ssize;
	int p_srate, c_srate;
	int p_chmask, c_chmask;

	params = &uac_dev->params;
	p_ssize = params->p_ssize;
	c_ssize = params->c_ssize;
	p_srate = params->p_srate;
	c_srate = params->c_srate;
	p_chmask = params->p_chmask;
	c_chmask = params->c_chmask;

	uac_pcm_hardware.periods_max = (uac_dev->alsa_buf_num >= MIN_PERIODS) ? uac_dev->alsa_buf_num : MIN_PERIODS;
	period_bytes_max_ctmp = (c_srate * c_ssize * num_channels(c_chmask)) * MAX_PERIODS_MS / 1000;
	period_bytes_max_ptmp = (p_srate * p_ssize * num_channels(p_chmask)) * MAX_PERIODS_MS / 1000;
	period_bytes_max_tmp = (period_bytes_max_ctmp > period_bytes_max_ptmp) ? period_bytes_max_ctmp : period_bytes_max_ptmp;
	period_bytes_max_tmp = ALIGN(period_bytes_max_tmp, PAGE_SIZE);
	period_bytes_max_tmp = (period_bytes_max_tmp > PRD_SIZE_MAX_DEF) ? period_bytes_max_tmp : PRD_SIZE_MAX_DEF;
	buffer_bytes_max_tmp = period_bytes_max_tmp * uac_pcm_hardware.periods_max;
	if(buffer_bytes_max_tmp > BUFF_SIZE_MAX)
	{
		printk("uac error, not support\n");
		return -1;
	}
	uac_pcm_hardware.period_bytes_max = period_bytes_max_tmp;
	uac_pcm_hardware.buffer_bytes_max = buffer_bytes_max_tmp;
	return 0;
}

static int uac_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_uac_chip *uac = snd_pcm_substream_chip(substream);
	struct uac_rtd_params *prm;
	struct uac_device *uac_dev;
	struct uac_params *params;
	unsigned long flags;
	int err = 0;

	uac_dev = uac->uac_dev;
	params = &uac_dev->params;
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
	// printk("%s prm->period_size = %ld,prm->dma_bytes = %ld\n",
	// 	(substream->stream == SNDRV_PCM_STREAM_CAPTURE) ? "capture" : "playback",
	// 	prm->period_size,prm->dma_bytes);
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

static u64 uac_ssize_to_fmt(int ssize)
{
	u64 ret;

	switch (ssize) {
	case 3:
		ret = SNDRV_PCM_FMTBIT_S24_3LE;
		break;
	case 4:
		ret = SNDRV_PCM_FMTBIT_S32_LE;
		break;
	default:
		ret = SNDRV_PCM_FMTBIT_S16_LE;
		break;
	}

	return ret;
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
	uac->p_residue = 0;

	runtime->hw = uac_pcm_hardware;

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		spin_lock_init(&uac->c_prm.lock);
		runtime->hw.rate_min = c_srate;
		runtime->hw.formats = uac_ssize_to_fmt(c_ssize);
		runtime->hw.channels_min = num_channels(c_chmask);
		runtime->hw.period_bytes_min = 2 * uac->c_prm.max_psize
						/ runtime->hw.periods_min;

	}
	else if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		spin_lock_init(&uac->p_prm.lock);
		runtime->hw.rate_min = p_srate;
		runtime->hw.formats = uac_ssize_to_fmt(p_ssize);
		runtime->hw.channels_min = num_channels(p_chmask);
		runtime->hw.period_bytes_min = 2 * uac->p_prm.max_psize
						/ runtime->hw.periods_min;

	}
	else{
		printk("%s error, unknown stream %d\n", __func__, substream->stream);
		return -EINVAL;
	}
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

	/* 处理捕获数据保存 */
	if (debug & (1 << VHD_UAC_DEBUG_SAVE_FILE_BIT)) {
        if (!work_data->cap_file) {
            /* 首次调用时打开捕获文件 */
			printk("[u_audio]open capture debug file\n");
            work_data->cap_file = filp_open("/nvdata/kuac_dump.pcm", O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (IS_ERR(work_data->cap_file)) {
                printk("Failed to open capture dump file /nvdata/kuac_dump.pcm, err %ld\n", PTR_ERR(work_data->cap_file));
                work_data->cap_file = NULL;
            }
        }

        if (work_data->cap_file && work_data->cap_len_ready > 0) {
            ssize_t written = kernel_write(work_data->cap_file, work_data->cap_buf[work_data->cap_i_ready], work_data->cap_len_ready, &pos);
            if (written != work_data->cap_len_ready) {
                printk("Failed to write capture dump data (%zd/%lu)\n", written, work_data->cap_len_ready);
            }
            work_data->cap_len_ready = 0;
        }
    } else if (work_data->cap_file) {
        /* 调试模式关闭时关闭捕获文件 */
		if(work_data->cap_file != NULL) {
			filp_close(work_data->cap_file, NULL);
			work_data->cap_file = NULL;
		}
    }

	/* 处理播放数据保存 */
	if (debug & (1 << VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT)) {
        if (!work_data->play_file) {
            /* 首次调用时打开播放文件 */
			printk("[u_audio]open playback debug file\n");
            work_data->play_file = filp_open("/nvdata/kuac_play_dump.pcm", O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (IS_ERR(work_data->play_file)) {
                printk("Failed to open playback dump file /nvdata/kuac_play_dump.pcm, err %ld\n", PTR_ERR(work_data->play_file));
                work_data->play_file = NULL;
            }
        }

        if (work_data->play_file && work_data->play_len_ready > 0) {
            ssize_t written = kernel_write(work_data->play_file, work_data->play_buf[work_data->play_i_ready], work_data->play_len_ready, &pos);
            if (written != work_data->play_len_ready) {
                printk("Failed to write playback dump data (%zd/%lu)\n", written, work_data->play_len_ready);
            }
            work_data->play_len_ready = 0;
        }
    } else if (work_data->play_file) {
        /* 调试模式关闭时关闭播放文件 */
		if(work_data->play_file != NULL) {
			filp_close(work_data->play_file, NULL);
			work_data->play_file = NULL;
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
	
	/* 初始化捕获数据缓冲区 */
	tmp_work_data->cap_buf_len = 192 * 10;
	tmp_work_data->cap_buf[0] = kzalloc(tmp_work_data->cap_buf_len, GFP_KERNEL);
	tmp_work_data->cap_buf[1] = kzalloc(tmp_work_data->cap_buf_len, GFP_KERNEL);
	if(!tmp_work_data->cap_buf[0] || !tmp_work_data->cap_buf[1]) {
		printk("Failed to allocate capture buffers\n");
		kfree(tmp_work_data->cap_buf[0]);
		kfree(tmp_work_data->cap_buf[1]);
		kfree(tmp_work_data);
		return -ENOMEM;
	}
	
	/* 初始化播放数据缓冲区 */
	tmp_work_data->play_buf_len = 192 * 10;
	tmp_work_data->play_buf[0] = kzalloc(tmp_work_data->play_buf_len, GFP_KERNEL);
	tmp_work_data->play_buf[1] = kzalloc(tmp_work_data->play_buf_len, GFP_KERNEL);
	if(!tmp_work_data->play_buf[0] || !tmp_work_data->play_buf[1]) {
		printk("Failed to allocate playback buffers\n");
		kfree(tmp_work_data->cap_buf[0]);
		kfree(tmp_work_data->cap_buf[1]);
		kfree(tmp_work_data->play_buf[0]);
		kfree(tmp_work_data->play_buf[1]);
		kfree(tmp_work_data);
		return -ENOMEM;
	}

	/* 初始化捕获数据索引和偏移 */
	tmp_work_data->cap_index = 0;
	tmp_work_data->cap_i_ready = 0;
	tmp_work_data->cap_len_offset = 0;
	tmp_work_data->cap_len_ready = 0;
	tmp_work_data->cap_file = NULL;
	
	/* 初始化播放数据索引和偏移 */
	tmp_work_data->play_index = 0;
	tmp_work_data->play_i_ready = 0;
	tmp_work_data->play_len_offset = 0;
	tmp_work_data->play_len_ready = 0;
	tmp_work_data->play_file = NULL;
	
	work_data = tmp_work_data;
	printk("uac_debug_init, capture buf len %zu, playback buf len %zu\n", 
		work_data->cap_buf_len, work_data->play_buf_len);
	return 0;
}

static int uac_debug_uninit(void)
{
	if(work_data) {
		flush_work(&work_data->work);
		
		/* 释放捕获数据缓冲区 */
		kfree(work_data->cap_buf[0]);
		kfree(work_data->cap_buf[1]);
		
		/* 释放播放数据缓冲区 */
		kfree(work_data->play_buf[0]);
		kfree(work_data->play_buf[1]);
		
		/* 关闭文件 */
		if(work_data->cap_file) {
			filp_close(work_data->cap_file, NULL);
		}
		if(work_data->play_file) {
			filp_close(work_data->play_file, NULL);
		}
		
		kfree(work_data);
		work_data = NULL;
	}
	return 0;
}

// data_type: 0=捕获数据, 1=播放数据
static int uac_debug_save_buf(char *buf, size_t len, int data_type)
{
	if(work_data == NULL)
		return -1;

	if (data_type == 0) { // 捕获数据
		if ((debug_tmp & (1 << VHD_UAC_DEBUG_SAVE_FILE_BIT)) == 0) {
			if(work_data->cap_file) {
				schedule_work(&work_data->work);
			}
			return 0;
		}

		if (len + work_data->cap_len_offset > work_data->cap_buf_len) {
			if(work_data->cap_len_offset == 0) {
				printk("[u_audio]error %s %d\n", __func__, __LINE__);
			}
			else{
				work_data->cap_i_ready = work_data->cap_index;
				work_data->cap_len_ready = work_data->cap_len_offset;
				work_data->cap_index = (work_data->cap_index + 1) % 2;
				work_data->cap_len_offset = 0;
				//printk("[u_audio]save cap buf %d\n", work_data->cap_i_ready);
				schedule_work(&work_data->work);
			}
		}
		memcpy(work_data->cap_buf[work_data->cap_index] + work_data->cap_len_offset, buf, len);
		work_data->cap_len_offset += len;
		//printk("[u_audio]%s, cap len %ld, off %ld\n", __func__, len, work_data->cap_len_offset);
	} else if (data_type == 1) { // 播放数据
		if ((debug_tmp & (1 << VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT)) == 0) {
			if(work_data->play_file) {
				schedule_work(&work_data->work);
			}
			return 0;
		}

		if (len + work_data->play_len_offset > work_data->play_buf_len) {
			if(work_data->play_len_offset == 0) {
				printk("[u_audio]error %s %d\n", __func__, __LINE__);
			}
			else{
				work_data->play_i_ready = work_data->play_index;
				work_data->play_len_ready = work_data->play_len_offset;
				work_data->play_index = (work_data->play_index + 1) % 2;
				work_data->play_len_offset = 0;
				//printk("[u_audio]save play buf %d\n", work_data->play_i_ready);
				schedule_work(&work_data->work);
			}
		}
		memcpy(work_data->play_buf[work_data->play_index] + work_data->play_len_offset, buf, len);
		work_data->play_len_offset += len;
		//printk("[u_audio]%s, play len %ld, off %ld\n", __func__, len, work_data->play_len_offset);
	}

	return 0;
}

static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	ssize_t len = 0;

	len += sprintf(buf + len, "Current debug value: %d\n", debug);

	/* 显示捕获数据保存状态 */
	if (debug & (1 << VHD_UAC_DEBUG_SAVE_FILE_BIT)) {
		len += sprintf(buf + len, "[Status] Capture (MIC) data saving ACTIVE (output to /nvdata/kuac_dump.pcm)\n");
	} else {
		len += sprintf(buf + len, "[Status] Capture (MIC) data saving INACTIVE\n");
	}

	/* 显示播放数据保存状态 */
	if (debug & (1 << VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT)) {
		len += sprintf(buf + len, "[Status] Playback data saving ACTIVE (output to /nvdata/kuac_play_dump.pcm)\n");
	} else {
		len += sprintf(buf + len, "[Status] Playback data saving INACTIVE\n");
	}

	len += sprintf(buf + len, "\nUsage:\n");
	len += sprintf(buf + len, "  - Set to %d (0x%x) to print data length\n", VHD_UAC_DEBUG_PRINT_LEN_BIT, VHD_UAC_DEBUG_PRINT_LEN_BIT);
	len += sprintf(buf + len, "  - Set to %d (0x%x) to save capture data\n", VHD_UAC_DEBUG_SAVE_FILE_BIT, VHD_UAC_DEBUG_SAVE_FILE_BIT);
	len += sprintf(buf + len, "  - Set to %d (0x%x) to save playback data\n", VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT, VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT);
	len += sprintf(buf + len, "  - Set to %d (0x%x) to save both capture and playback data\n", VHD_UAC_DEBUG_SAVE_FILE_BIT | VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT, VHD_UAC_DEBUG_SAVE_FILE_BIT | VHD_UAC_DEBUG_SAVE_PLAY_FILE_BIT);
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
    //printk("pending = %d,req->actual = %d\n",pending,req->actual);

#ifdef VHD_UAC_DEBUG
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
        debug_tmp = debug;
        uac_debug_save_buf(req->buf, req->actual, 1); // 保存播放数据，data_type=1
    }
#endif

exit:
    if (usb_ep_queue(ep, req, GFP_ATOMIC))
        dev_err(uac->card->dev, "%d Error!\n", __LINE__);

    if (update_alsa)
		snd_pcm_period_elapsed(substream);
	
}

#define CLK_PPM_GROUP_SIZE 20
static void ppm_calculate_work(struct work_struct *work)
{
	struct uac_device *uac_dev = container_of(work, struct uac_device,
					       ppm_work.work);
	struct usb_gadget *gadget = uac_dev->gadget;
	uint32_t frame_number, fn_msec, clk_msec;
	uint64_t time_now, time_msec_tmp;
	int32_t ppm;
	static int32_t ppms[CLK_PPM_GROUP_SIZE];
	static int32_t ppm_sum;
	int32_t cnt;

	time_now = ktime_get_raw();
	frame_number = gadget->ops->get_frame(gadget);

	if (!uac_dev->ppm_fn) {
		uac_dev->ppm_fn = kzalloc(sizeof(*uac_dev->ppm_fn), GFP_KERNEL);
		if (!uac_dev->ppm_fn)
			goto out;
	}

	if (uac_dev->ppm_fn->time_last &&
	    time_now - uac_dev->ppm_fn->time_last > 1500000000ULL)
		pr_debug("%s: PPM work scheduled too slow!\n", __func__);

	uac_dev->ppm_fn->time_last = time_now;

	if (gadget->state != USB_STATE_CONFIGURED  || (get_usb_link_state() != 0)) {
		memset(uac_dev->ppm_fn, 0, sizeof(*uac_dev->ppm_fn));
		//pr_debug("%s: Disconnect. frame number is cleared\n", __func__);
		goto out;
	}

	if (!uac_dev->ppm_fn->second++) {
		uac_dev->ppm_fn->time_begin = uac_dev->ppm_fn->time_last;
		uac_dev->ppm_fn->fn_begin = frame_number;
		uac_dev->ppm_fn->fn_last = frame_number;
		goto out;
	}

	if (frame_number <= uac_dev->ppm_fn->fn_last)
		uac_dev->ppm_fn->fn_overflow++;
	uac_dev->ppm_fn->fn_last = frame_number;

	if (!uac_dev->ppm_fn->fn_overflow)
		goto out;

	fn_msec = (((uac_dev->ppm_fn->fn_overflow - 1) << 14) +
		   (BIT(14) + uac_dev->ppm_fn->fn_last - uac_dev->ppm_fn->fn_begin) + BIT(2)) >> 3;
	time_msec_tmp = uac_dev->ppm_fn->time_last - uac_dev->ppm_fn->time_begin + 500000ULL;
	do_div(time_msec_tmp, 1000000U);
	clk_msec = (uint32_t)time_msec_tmp;

	ppm = (fn_msec > clk_msec) ?
	      (fn_msec - clk_msec) * 1000000L / clk_msec :
	      -((clk_msec - fn_msec) * 1000000L / clk_msec);

	cnt = uac_dev->ppm_fn->second % CLK_PPM_GROUP_SIZE;
	ppm_sum = ppm_sum - ppms[cnt] + ppm;
	ppms[cnt] = ppm;

	// pr_debug("%s: frame %u msec %u ppm_calc %d ppm_average(%d) %d\n",
	// 	__func__, fn_msec, clk_msec, ppm, CLK_PPM_GROUP_SIZE, ppm_sum / CLK_PPM_GROUP_SIZE);

	if (abs(ppm_sum / CLK_PPM_GROUP_SIZE - ppm) < 3) {
		ppm = ppm_sum > 0 ?
		      (ppm_sum + CLK_PPM_GROUP_SIZE/2) / CLK_PPM_GROUP_SIZE :
		      (ppm_sum - CLK_PPM_GROUP_SIZE/2) / CLK_PPM_GROUP_SIZE;

		if (uac_dev->vdev && uac_dev->ppm_value != ppm) {
			struct v4l2_event event;
			uac_dev->ppm_value = ppm;
			//pr_debug("%s: uac_dev->ppm_value = %d\n", __func__, uac_dev->ppm_value);
			memset(&event, 0, sizeof(event));
			event.type = UAC_EVENT_PPM;
            memcpy(&event.u.data, &uac_dev->ppm_value, sizeof(uac_dev->ppm_value));
			v4l2_event_queue(uac_dev->vdev, &event);
		}
	}

out:
	schedule_delayed_work(&uac_dev->ppm_work, 1 * HZ);
}

static void
__uac_audio_complete_playback_test(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	struct uac_req_play *ur = req->context;
	struct uac_rtd_params *prm = ur->pp;
	struct snd_uac_chip *uac = prm->uac;
	static int test_cnt = 0;

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
	//printk("pending = %d,req->actual = %d\n",pending,req->actual);
#ifdef VHD_UAC_DEBUG
		uac_debug_save_buf(req->buf, req->actual, 0); // 保存捕获数据，data_type=0
		if (debug & (1 << VHD_UAC_DEBUG_PRINT_LEN_BIT))
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
	    uac_dev->gadget->state != USB_STATE_CONFIGURED)
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
			printk(KERN_INFO "g_audio_setup fail %d\n", __LINE__);
			goto fail;
		}

		prm->rbuf = kcalloc(params->req_number, prm->max_psize,
				GFP_KERNEL);
		if (!prm->rbuf) {
			prm->max_psize = 0;
			err = -ENOMEM;
			printk(KERN_INFO "g_audio_setup fail %d\n", __LINE__);
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
				printk(KERN_INFO "g_audio_setup fail %d\n", __LINE__);
				goto fail;
			}
	
			prm->rbuf = kcalloc(params->req_number, prm->max_psize,
					GFP_KERNEL);
			if (!prm->rbuf) {
				prm->max_psize = 0;
				err = -ENOMEM;
				printk(KERN_INFO "g_audio_setup fail %d\n", __LINE__);
				goto fail;
			}
	}

	/* Choose any slot, with no id */
	err = snd_card_new(&uac_dev->gadget->dev,
			-1, NULL, THIS_MODULE, 0, &card);
	if (err < 0){
		printk("g_audio_setup fail %d\n", __LINE__);
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
		printk("g_audio_setup fail %d\n", __LINE__);
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

	err = uac_pcm_hardware_resize(uac_dev);
	if (err < 0){
		goto snd_fail;
	}
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
	if (err)
		goto snd_fail;

	INIT_DELAYED_WORK(&uac_dev->ppm_work, ppm_calculate_work);
	ppm_calculate_work(&uac_dev->ppm_work.work);

#ifdef CHIP_RK3588
	timer_setup(&uac_dev->wait_out_req_complete_timer,
	    audio_wait_out_req_complete_watchdog, 0);
#endif
	return 0;

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
	cancel_delayed_work_sync(&uac_dev->ppm_work);
	kfree(uac_dev->ppm_fn);
	uac_dev->ppm_fn = NULL;
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
