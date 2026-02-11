/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:37
 * @FilePath    : \lib_ucam\ucam\include\ucam\uac\uac_alsa.h
 * @Description : 
 */ 
#ifndef UAC_ALSA_H 
#define UAC_ALSA_H 

#include "asoundlib.h"

int uac_alsa_init(snd_pcm_t **alsa_handle, int alsa_card_id,
	int mode,
    snd_pcm_stream_t stream,
    uint32_t samplepsec, 
    uint32_t channels, 
	snd_pcm_format_t format, 
    snd_pcm_uframes_t period_size,
    snd_pcm_uframes_t buf_size,
	snd_pcm_uframes_t start_threshold,
	snd_pcm_uframes_t stop_threshold
	);

int uac_pcm_write(snd_pcm_t *handle, const void *data, size_t count, int timeout_ms);
int uac_alsa_pcm_write(snd_pcm_t *handle, const void *data, size_t count, int timeout_ms);

int uac_alsa_playback_init(snd_pcm_t **alsa_playback_handle, char *alsa_card_name,  
    uint32_t samplepsec, uint32_t channels, 
    snd_pcm_format_t format, snd_pcm_uframes_t playbackPeriodSize,
    int mode);

int uac_alsa_playback_close(snd_pcm_t *alsa_playback_handle);

int uac_alsa_capture_init(snd_pcm_t **alsa_capture_handle, char *alsa_card_name,  
    uint32_t samplepsec, uint32_t channels,
    snd_pcm_format_t format, snd_pcm_uframes_t playbackPeriodSize,
    int mode);

int uac_pcm_read(snd_pcm_t *handle, u_char *data, size_t count, int timeout_ms);
int uac_alsa_pcm_read(snd_pcm_t *handle, u_char *data, size_t count, int timeout_ms);

int uac_alsa_capture_close(snd_pcm_t *alsa_capture_handle);

#endif /*UAC_ALSA_H*/