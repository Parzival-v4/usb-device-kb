/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-06-01 20:05:38
 * @FilePath    : \lib_ucam\ucam\src\uac\uac_alsa.c
 * @Description : 
 */ 
#include <sys/time.h>
#include <ucam/uac/uac_alsa.h>
#include <ucam/ucam_dbgout.h>
#include <ucam/ucam_api.h>
#include "asoundlib.h"

static int set_alsa_params(snd_pcm_t *handle, 
    snd_pcm_stream_t stream,
    uint32_t samplepsec, 
    uint32_t channels, 
    snd_pcm_format_t format, 
    snd_pcm_uframes_t period_size,
    snd_pcm_uframes_t buf_size,
	snd_pcm_uframes_t start_threshold,
	snd_pcm_uframes_t stop_threshold
    )
{
    snd_pcm_hw_params_t *hw_params = NULL;
	snd_pcm_sw_params_t *sw_params = NULL;
	int err = 0;
	int ret = 0;
    snd_pcm_hw_params_t * paramsTemp;
	snd_pcm_info_t *info;

	snd_pcm_info_alloca(&info);
	if ((err = snd_pcm_info(handle, info)) < 0) {
		ucam_error("info error: %s", snd_strerror(err));
		return -1;
	}

	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_alloca(&hw_params);

	/* Init hwparams with full configuration space */
	if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0)
	{
		ucam_error( "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
		return -1;
	}

	/* Set access type. */
	if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED )) < 0)
	{
		ucam_error( "cannot set access type (%s)\n", snd_strerror(err));
		return -1;
	}
	/* Set sample format */
	if ((err = snd_pcm_hw_params_set_format (handle, hw_params, format)) < 0)
	{
		ucam_error( "cannot set sample format (%s)\n", snd_strerror(err));
		return -1;
	}

    ret = snd_pcm_hw_params_set_rate(handle, hw_params, samplepsec, 0);
	if(ret < 0){
		ucam_error("snd_pcm_hw_params_set_rate failed\n");
		return -1;
	}

	/* Set number of channels */
	if ((err = snd_pcm_hw_params_set_channels (handle,hw_params,channels)) < 0)
	{
		ucam_error( "cannot set channel count (%s)\n", snd_strerror(err));
		return -1;
	}

	//period size
	if((err = snd_pcm_hw_params_set_period_size(handle, hw_params, period_size, 0)) < 0)
	{
		ucam_error("Unable to set the period size %d (%s)",period_size,snd_strerror(err));
        snd_pcm_hw_params_get_period_size_min(hw_params, &period_size, 0);
        ucam_error("get_period_size_min %d",period_size);
		return -1;
	}

    ret = snd_pcm_hw_params_set_buffer_size(handle, hw_params, buf_size);
	if(ret < 0){
		ucam_error("snd_pcm_hw_params_set_buffer_size failed\n");
		return -1;
	}

	/* Apply HW parameter settings to PCM device and prepare device. */
	if ((err = snd_pcm_hw_params (handle, hw_params)) < 0)
	{
		ucam_error( "cannot set parameters (%s)\n", snd_strerror(err));
		return -1;
	}

	snd_pcm_sw_params_alloca(&sw_params);
	ret = snd_pcm_sw_params_current(handle, sw_params);
	if(ret < 0){
		ucam_error("snd_pcm_sw_params_current failed\n");
		return -1;
	}


	ret = snd_pcm_sw_params_set_start_threshold(handle, sw_params, start_threshold);
	if(ret < 0){
		ucam_error("snd_pcm_sw_params_set_start_threshold failed\n");
		return ret;
	}
	
	
	ret = snd_pcm_sw_params_set_stop_threshold(handle, sw_params, stop_threshold);
	if(ret < 0){
		ucam_error("snd_pcm_sw_params_set_stop_threshold failed\n");
		return ret;
	}
	
	ret = snd_pcm_sw_params(handle, sw_params);
	if(ret < 0){
		ucam_error("snd_pcm_sw_params failed\n");
		return ret;
	}

	/* Get the parameters from the driver */
	snd_pcm_hw_params_alloca(&paramsTemp);
	err = snd_pcm_hw_params_current(handle, paramsTemp);
	if (err < 0)
	{
		ucam_error("info error: %s", snd_strerror(err));
		ret = -1;
	}

	if (ret != 0)
	{
		ucam_error("alsa inited failed\n");
	}
	else
	{
		ucam_trace("alsa player inited\n");
	}


    return ret;
}

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
	)
{

	char alsa_card_name[128];

	if(alsa_card_id < 0)
	{
		ucam_error("alsa_card_id error!");
		return -1;
	}

	memset(alsa_card_name, 0, sizeof(alsa_card_name));
    sprintf(alsa_card_name, "plughw:%d,0", alsa_card_id);

	int err = snd_pcm_open(alsa_handle, alsa_card_name, stream, mode);
	if (err < 0) {
		ucam_error("alsa_card_name:%s open error: %s \n", alsa_card_name, snd_strerror(err));
		return -1;
	}

	return set_alsa_params(*alsa_handle, stream, 
        samplepsec, channels, format, period_size, buf_size,
		start_threshold, stop_threshold);
}


static int set_playback_params(snd_pcm_t *handle, uint32_t samplepsec, uint32_t channels, snd_pcm_format_t format, snd_pcm_uframes_t playbackPeriodSize)
{
    snd_pcm_hw_params_t *g_hw_params = NULL;

	//snd_pcm_uframes_t bufSize = IHI_PLAY_BUF_SAMPLES;
	int err = 0;
	/* Playback stream */
	//unsigned int val;
	int dir;
	int ret = 0;
	//snd_pcm_uframes_t bufSize;

	//snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
    snd_pcm_hw_params_t * paramsTemp;
	snd_pcm_info_t *info;
	
	//snd_pcm_uframes_t frames = 0;

	
	snd_pcm_info_alloca(&info);
	if ((err = snd_pcm_info(handle, info)) < 0) {
			ucam_error("info error: %s", snd_strerror(err));
			return -1;
	}

	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_alloca(&g_hw_params);

	/* Init hwparams with full configuration space */
	if ((err = snd_pcm_hw_params_any (handle, g_hw_params)) < 0)
	{
		ucam_error( "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
		ret = -1;
	}

	/* Set access type. */
	if ((err = snd_pcm_hw_params_set_access (handle, g_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED )) < 0)
	{
		ucam_error( "cannot set access type (%s)\n", snd_strerror(err));
		return -1;
	}
	/* Set sample format */
	if ((err = snd_pcm_hw_params_set_format (handle, g_hw_params, format)) < 0)
	{
		ucam_error( "cannot set sample format (%s)\n", snd_strerror(err));
		ret = -1;
	}

	/* Set number of channels */
	if ((err = snd_pcm_hw_params_set_channels (handle,g_hw_params,channels)) < 0)
	{
		ucam_error( "cannot set channel count (%s)\n", snd_strerror(err));
		ret = -1;
	}
	//period size
	ucam_trace("playbackPeriodSize:%d", playbackPeriodSize);
	dir = SND_PCM_STREAM_PLAYBACK;
	if((err = snd_pcm_hw_params_set_period_size(handle, g_hw_params, playbackPeriodSize, dir)) < 0)
	{
		ucam_error("Unable to set the period size (%s)",snd_strerror(err));
	}
#if 0    
	err = snd_pcm_hw_params_get_period_size(g_hw_params, &frames, &dir);
	if (err > 0 && frames == 0)
	{
	   frames = err;
	}
	ucam_error("test1 frames:%d", frames);
#endif

#if 0
	/* Set sample rate. If the exact rate is not supported by the
	hardware, use nearest possible rate. */
	if ((err = snd_pcm_hw_params_set_rate_near (handle, g_hw_params,	&samplepsec, 0)) < 0)
	{
		ucam_error( "cannot set sample rate (%s)\n", snd_strerror(err));
		ret = -1;
	}
#else
	err = snd_pcm_hw_params_set_rate(handle, g_hw_params, samplepsec, dir);
	if(err < 0){
		ucam_error("snd_pcm_hw_params_set_rate failed\n");
		return -1;
	}
#endif

#if 0
	dir = 0;
	if ((err = snd_pcm_hw_params_set_period_size_min(handle, g_hw_params, &playbackPeriodSize, &dir)) < 0) {
		ucam_error("fail to set min periodsize %d, err %s", (int) playbackPeriodSize, snd_strerror(err));
		ret = err;
	}
	
	bufSize = playbackPeriodSize;
	if ((err = snd_pcm_hw_params_set_period_size_max(handle, g_hw_params, &bufSize, &dir))	< 0) {
		ucam_error("fail to set max period size %d, err %s", (int)playbackPeriodSize, snd_strerror(err));
		ret = err;
	}
	
	bufSize = playbackPeriodSize * 5;
	if ((err = snd_pcm_hw_params_set_buffer_size_near(handle, g_hw_params, &bufSize)) < 0) {
		ucam_error("Unable to set the buffer size (%s)", snd_strerror(err));
	}
#endif

	/* Apply HW parameter settings to PCM device and prepare device. */
	if ((err = snd_pcm_hw_params (handle, g_hw_params)) < 0)
	{
		ucam_error( "cannot set parameters (%s)\n", snd_strerror(err));
		ret = -1;
	}
	/* Get the parameters from the driver */
	snd_pcm_hw_params_alloca(&paramsTemp);
	err = snd_pcm_hw_params_current(handle, paramsTemp);
	if (err < 0)
	{
		ucam_error("info error: %s", snd_strerror(err));
		ret = -1;
	}

	if (ret != 0)
	{
		ucam_error("audio player inited failed\n");
	}
	else
	{
		ucam_trace("audio player inited\n");
	}
	ucam_trace ("leave");

    return ret;
}


int uac_pcm_write(snd_pcm_t *handle, const void *data, size_t count, int timeout_ms)
{

	//int size;
	int err;
	//int dir;
	//unsigned int channels = 0;
	//snd_pcm_uframes_t frames = 0;
	int bStatusSendFinish = 0;
	struct timeval tvStart, tvEnd;

	long long time_diff;
	//int underrun_flag = 0;
	//int underrun_cnt = 0;
	
    if(handle == NULL)
	{
		ucam_error("handle == NULL\n");
		return -ENOMEM;
	}

//underrun_process:
	if(timeout_ms > 0)
	{
		gettimeofday(&tvStart,NULL);
	}

	/* send data to alsa driver */
	while(!bStatusSendFinish)
	{
		err = snd_pcm_writei(handle, data, count);
		if (err == -EPIPE)
		{
			/* EPIPE means underrun */
			ucam_error("underrun occurred, err = %d\n", err);
			snd_pcm_prepare(handle);
			//underrun_flag = 1;
		}
		else if (err < 0)
		{
			ucam_error("error from writei: %s\n", snd_strerror(err));
			return -1;
		}
		//else if (err != (int)frames)
		else if (err != count)
		{
			ucam_error("short write, write %d frames， count = %d\n", err, count);
			return -1;
		}
		else
		{
			//ucam_error("write successful");
			bStatusSendFinish = 1;
			return 0;
		}

		if(timeout_ms == 0)
		{
			if(err < 0)
			{
				return err;
			}
		}
		else if (timeout_ms > 0)
		{
			gettimeofday(&tvEnd,NULL);
			time_diff = (long long)get_time_difference(&tvStart, &tvEnd)/1000;
			if(time_diff > timeout_ms)
			{
				ucam_error("pcm_write timeout");
				return -1;
			}
		}

        //if(err_cnt > 10)
        //   return -1;
	}

#if 0
	if(underrun_flag == 1 && underrun_cnt < 2)
	{
		//出现underrun说明BUF已经空了，需要向BUF写入两帧的数据添加延时防止再次underrun
		underrun_cnt ++;
		timeout_ms = 20;
		goto underrun_process;
	}
#endif
	return 0;

}

int uac_alsa_pcm_write(snd_pcm_t *handle, const void *data, size_t count, int timeout_ms)
{
	int ret;
	size_t offset = 0;
	size_t size = count;

	struct timeval tvStart, tvEnd;
	long long time_diff;


    if(handle == NULL)
	{
		ucam_error("handle == NULL\n");
		return -ENOMEM;
	}

	if(timeout_ms > 0)
	{
		gettimeofday(&tvStart,NULL);
	}

	while(size > 0)
	{
		ret = snd_pcm_writei(handle, data + offset, size);
		if (ret == -EAGAIN || (ret >= 0 && (size_t)ret < size)) {
			snd_pcm_wait(handle, 1);
		} else if (ret == -EPIPE) {
			ucam_error("%s underrun occurred, ret = %d", snd_pcm_name(handle), ret);
			snd_pcm_prepare(handle);
		} else if (ret == -ESTRPIPE) {
			ucam_error("stram ret pipe occurred(%s)",snd_strerror(ret));

			while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
				snd_pcm_wait(handle, 1);  /* wait until resume flag is released */
			if (ret < 0)
				ret = snd_pcm_prepare(handle);
		} else if (ret < 0) {
			ucam_error("error: [%s]",snd_strerror(ret));
			return ret;
		}

		if (ret > 0) {
			offset += snd_pcm_frames_to_bytes(handle, ret);
			size -= ret;
		}

		if(timeout_ms == 0)
		{
			if(ret < 0)
				return ret;
		}
		else if (timeout_ms > 0)
		{
			gettimeofday(&tvEnd,NULL);
			time_diff = (long long)get_time_difference(&tvStart, &tvEnd)/1000;
			if(time_diff > timeout_ms)
			{
				ucam_error("pcm_write timeout");
				return count - size;
			}
		}
	}

	return count;
}

int uac_alsa_playback_init(snd_pcm_t **alsa_playback_handle, char *alsa_card_name,  
    uint32_t samplepsec, uint32_t channels, 
	snd_pcm_format_t format, snd_pcm_uframes_t playbackPeriodSize,
	int mode)
{
	ucam_trace("alsa_audio_init enter");

	//SND_PCM_NONBLOCK 非阻塞方式  否则在插拔HDMI的时候会发生阻塞    默认打开是阻塞方式0
	int err = snd_pcm_open(alsa_playback_handle, alsa_card_name, SND_PCM_STREAM_PLAYBACK, mode);
	if (err < 0) {
		ucam_error("alsa_card_name:%s open error: %s \n", alsa_card_name, snd_strerror(err));
		return -1;
	}

	return set_playback_params(*alsa_playback_handle, samplepsec, channels, format, playbackPeriodSize);
}


int uac_alsa_playback_close(snd_pcm_t *alsa_playback_handle)
{
    if(alsa_playback_handle)
		return snd_pcm_close(alsa_playback_handle);
    else
        return -ENOMEM;
}


/****************************************************************************************************************************************/


static int set_capture_params(snd_pcm_t *handle, uint32_t samplepsec, uint32_t channels, snd_pcm_format_t format, snd_pcm_uframes_t playbackPeriodSize)
{
    snd_pcm_hw_params_t *g_hw_params = NULL;

	//snd_pcm_uframes_t bufSize = IHI_PLAY_BUF_SAMPLES;
	int err = 0;
	/* Playback stream */
	//unsigned int val;
	int dir;
	int ret = 0;

	//snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
    snd_pcm_hw_params_t * paramsTemp;
	snd_pcm_info_t *info;
	
	//snd_pcm_uframes_t frames = 0;

	
	snd_pcm_info_alloca(&info);
	if ((err = snd_pcm_info(handle, info)) < 0) {
			ucam_error("info error: %s", snd_strerror(err));
			return -1;
	}

	/* Allocate the snd_pcm_hw_params_t structure on the stack. */
	snd_pcm_hw_params_alloca(&g_hw_params);

	/* Init hwparams with full configuration space */
	if ((err = snd_pcm_hw_params_any (handle, g_hw_params)) < 0)
	{
		ucam_error( "cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
		ret = -1;
	}

	/* Set access type. */
	if ((err = snd_pcm_hw_params_set_access (handle, g_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED )) < 0)
	{
		ucam_error( "cannot set access type (%s)\n", snd_strerror(err));
		return -1;
	}
	/* Set sample format */
	if ((err = snd_pcm_hw_params_set_format (handle, g_hw_params, format)) < 0)
	{
		ucam_error( "cannot set sample format (%s)\n", snd_strerror(err));
		ret = -1;
	}

	/* Set number of channels */
	if ((err = snd_pcm_hw_params_set_channels (handle,g_hw_params,channels)) < 0)
	{
		ucam_error( "cannot set channel count (%s)\n", snd_strerror(err));
		ret = -1;
	}
	//period size
	ucam_trace("playbackPeriodSize:%d", playbackPeriodSize);
	dir = 0;//SND_PCM_STREAM_CAPTURE;
	if((err = snd_pcm_hw_params_set_period_size(handle, g_hw_params, playbackPeriodSize, dir)) < 0)
	{
		ucam_error("Unable to set the period size (%s)",snd_strerror(err));
	}
#if 0    
	err = snd_pcm_hw_params_get_period_size(g_hw_params, &frames, &dir);
	if (err > 0 && frames == 0)
	{
	   frames = err;
	}
	ucam_error("test1 frames:%d", frames);
#endif

#if 0
	/* Set sample rate. If the exact rate is not supported by the
	hardware, use nearest possible rate. */
	if ((err = snd_pcm_hw_params_set_rate_near (handle, g_hw_params,	&samplepsec, 0)) < 0)
	{
		ucam_error( "cannot set sample rate (%s)\n", snd_strerror(err));
		ret = -1;
	}
#else
	err = snd_pcm_hw_params_set_rate(handle, g_hw_params, samplepsec, dir);
	if(err < 0){
		ucam_error("snd_pcm_hw_params_set_rate failed\n");
		return -1;
	}
#endif

	/* Apply HW parameter settings to PCM device and prepare device. */
	if ((err = snd_pcm_hw_params (handle, g_hw_params)) < 0)
	{
		ucam_error( "cannot set parameters (%s)\n", snd_strerror(err));
		ret = -1;
	}
	/* Get the parameters from the driver */
	snd_pcm_hw_params_alloca(&paramsTemp);
	err = snd_pcm_hw_params_current(handle, paramsTemp);
	if (err < 0)
	{
		ucam_error("info error: %s", snd_strerror(err));
		ret = -1;
	}

	if (ret != 0)
	{
		ucam_error("audio player inited failed\n");
	}
	else
	{
		ucam_trace("audio player inited\n");
	}
	ucam_trace ("leave");

    return ret;
}

int uac_pcm_read(snd_pcm_t *handle, u_char *data, size_t count, int timeout_ms)
{

	//int size;
	int err;
	//int dir;
	//unsigned int channels = 0;
	//snd_pcm_uframes_t frames = 0;
	int bStatusSendFinish = 0;

	struct timeval tvStart, tvEnd;
	long long time_diff;


    if(handle == NULL)
	{
		ucam_error("handle == NULL\n");
		return -ENOMEM;
	}

	if(timeout_ms > 0)
	{
		gettimeofday(&tvStart,NULL);
	}


	/* send data to alsa driver */
	while(!bStatusSendFinish)
	{
		err = snd_pcm_readi(handle, data, count);
		if (err == -EPIPE)
		{
			/* EPIPE means underrun */
			ucam_error("underrun occurred, err = %d", err);
			snd_pcm_prepare(handle);
		}
		else if (err < 0)
		{
			//ucam_error("error from readi: %s", snd_strerror(err));
			return -1;
		}
		//else if (err != (int)frames)
		else if (err != count)
		{
			ucam_error("short read, read %d frames", err);
			return -1;
		}
		else if(err > 0)
		{
			//ucam_error("read successful");
			bStatusSendFinish = 1;
		}

		if(timeout_ms == 0)
		{
			if(err < 0)
			{
				return err;
			}
		}
		else if (timeout_ms > 0)
		{
			gettimeofday(&tvEnd,NULL);
			time_diff = (long long)get_time_difference(&tvStart, &tvEnd)/1000;
			if(time_diff > timeout_ms)
			{
				ucam_error("pcm_write timeout");
				return -1;
			}
		}
	}

	return 0;

}

int uac_alsa_pcm_read(snd_pcm_t *handle, u_char *data, size_t count, int timeout_ms)
{
	int ret;
    size_t avail_frames;
    int timeout_count = 0;
    snd_pcm_state_t pcm_state = SND_PCM_STATE_PREPARED;
	

	if(handle == NULL)
	{
		ucam_error("handle == NULL\n");
		return -ENOMEM;
	}


    while(1)
    {
        avail_frames = snd_pcm_avail(handle); 
        if (avail_frames == -EPIPE)
        {
            ucam_error("avail underrun occurred, avail = %d\n", avail_frames);
            snd_pcm_prepare(handle);
            return -EPIPE;
        }
        else if (avail_frames == -ESTRPIPE) {
			ucam_error("stram ret pipe occurred(%s)",snd_strerror(avail_frames));
			while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
				snd_pcm_wait(handle, 1);  /* wait until resume flag is released */
			if (ret < 0)
				ret = snd_pcm_prepare(handle);    
            return -ESTRPIPE;
		} 
        else if(avail_frames < count || avail_frames == -EAGAIN)
        {
            if(avail_frames == 0)
            {
                pcm_state = snd_pcm_state(handle);
                if(pcm_state != SND_PCM_STATE_RUNNING)
                {
                    snd_pcm_start(handle);
                }
            }
            timeout_count ++;
            if(timeout_ms == 0)
            {
                return 0;
            }
            else if(timeout_ms < 0)
            {
                //一直等待
                snd_pcm_wait(handle, 1);
                continue;
            }
            else if(timeout_count < timeout_ms)
            {
                //ucam_error("avail = %d\n", avail_frames);
                snd_pcm_wait(handle, 1); 
                continue;
            }
            else if(timeout_count >= timeout_ms)
            {
                //ucam_error("timeout avail = %d\n", avail_frames);
                return -ETIMEDOUT;
            }
        }
        else
        {
            break;
        }
	}

    ret = snd_pcm_readi(handle, data, count);
    if (ret == -EPIPE) {
        ucam_error("%s underrun occurred, ret = %d", snd_pcm_name(handle), ret);
        snd_pcm_prepare(handle);
        return -EPIPE;
    } else if (ret == -ESTRPIPE) {
        ucam_error("stram ret pipe occurred(%s)",snd_strerror(ret));
        while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
            snd_pcm_wait(handle, 1);  /* wait until resume flag is released */
        if (ret < 0)
            ret = snd_pcm_prepare(handle);
        return -ESTRPIPE;
    } else if (ret < 0 || ret != count) {
        ucam_error("ret: %d, error: [%s]",ret, snd_strerror(ret));
        return ret;
    }
    
    return ret;
}

int uac_alsa_capture_init(snd_pcm_t **alsa_capture_handle, char *alsa_card_name,  
    uint32_t samplepsec, uint32_t channels, 
	snd_pcm_format_t format, snd_pcm_uframes_t playbackPeriodSize,
	int mode)
{
	ucam_trace("enter");

	//SND_PCM_NONBLOCK 非阻塞方式  否则在插拔HDMI的时候会发生阻塞    默认打开是阻塞方式0
	int err = snd_pcm_open(alsa_capture_handle, alsa_card_name, SND_PCM_STREAM_CAPTURE, mode);
	if (err < 0) {
		ucam_error("alsa_card_name:%s open error: %s \n", alsa_card_name, snd_strerror(err));
		return -1;
	}

	return set_capture_params(*alsa_capture_handle, samplepsec, channels, format, playbackPeriodSize);
}


int uac_alsa_capture_close(snd_pcm_t *alsa_capture_handle)
{
    if(alsa_capture_handle)
		return snd_pcm_close(alsa_capture_handle);
    else
        return -ENOMEM;
}