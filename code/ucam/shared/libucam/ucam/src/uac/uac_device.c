/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-08 19:50:33
 * @FilePath    : \lib_ucam\ucam\src\uac\uac_device.c
 * @Description : 
 */ 
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <memory.h>
#include <unistd.h>
#include <malloc.h>

#include <sys/prctl.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <linux/usb/ch9.h>
#include <pthread.h>
#include <signal.h>


#include <ucam/uac/uac_device.h>
#include <ucam/uac/uac_alsa.h>
#include <ucam/uvc/uvc_config.h>
#include <ucam/ucam_api.h>

void *g_uac_mic_temp_buf = NULL;//这可能是个BUG的存在

static struct uac_dev* uac_open(const char* devname)
{
    struct uac_dev *dev;
    struct v4l2_capability cap;
    int ret;
    int fd;

    fd = open(devname, O_RDWR | O_NONBLOCK);

    if (fd == -1)
    {
        return NULL;
    }

    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);

    if (ret < 0)
    {
        ucam_trace("unable to query device: %s (%d)\n", strerror(errno),
            errno);
        close(fd);
        return NULL;
    }

    ucam_trace("open succeeded(%s:caps=0x%04x)\n", devname, cap.capabilities);
#if 0
    /* check the devie is a audio or not*/
    if (!(cap.capabilities & 0x00020000))
    {
        close(fd);
        return NULL;
    }
#endif
    ucam_trace("device is %s on bus %s\n", cap.card, cap.bus_info);

    dev = (struct uac_dev*)malloc( sizeof(struct uac_dev) );

    if (dev == NULL)
    {
        close(fd);
        return NULL;
    }

    memset(dev, 0, sizeof * dev);
    dev->fd = fd;

    return dev;
}

static void uac_close(struct uac_dev* dev)
{
    dev->mic_attrs.stream_on_status = 0;
    dev->speak_attrs.stream_on_status = 0;

    if(dev->fd != -1)
        close(dev->fd);  

    if(dev)   
        free(dev);
}




static void uac_events_process_setup ( struct uac_dev *dev,
        struct usb_ctrlrequest *ctrl, struct uac_request_data *resp)
{
    ucam_trace("bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
                    "wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
                    ctrl->wValue, ctrl->wIndex, ctrl->wLength);
	//uvc_events_process_uacctrl(dev, ctrl, resp);

}

void uac_streamoff_all(struct uac_dev* dev)
{
	if(dev->uac_mic_enable)
	{
		if(dev->alsa_playback_handle && dev->mic_attrs.stream_on_status)
		{
			dev->uac->mic_stream_on_trigger = 0;
			dev->mic_attrs.stream_on_status = 0;
			
			usleep(100*1000);
			pthread_mutex_lock(&dev->alsa_playback_mutex);
			if(dev->alsa_playback_handle)
			{
				if(uac_alsa_playback_close(dev->alsa_playback_handle) != 0)
				{
					ucam_error("alsa_playback_close error!");
				}
				dev->alsa_playback_handle = NULL;
			}
			if(g_uac_mic_temp_buf != NULL)
			{
				free(g_uac_mic_temp_buf);
				g_uac_mic_temp_buf = NULL;
			}
			pthread_mutex_unlock(&dev->alsa_playback_mutex);
		}
	}

	if(dev->uac_speak_enable)
	{

#if 0            
		if(dev->alsa_capture_handle && dev->uac->speak_stream_on_trigger)
		{
			dev->uac->speak_stream_on_trigger = 0;
			dev->speak_attrs.stream_on_status = 0;
			
			usleep(100*1000);
			pthread_mutex_lock(&dev->alsa_capture_mutex);
			if(dev->alsa_capture_handle)
			{
				if(uac_alsa_capture_close(dev->alsa_capture_handle) != 0)
				{
					ucam_error("alsa_capture_close error!");
				}
				dev->alsa_capture_handle = NULL;
			}

			memset(alsa_card_name, 0, sizeof(alsa_card_name));
			sprintf(alsa_card_name, "plughw:%d,0", dev->alsa_card_id);
			if(alsa_capture_init(&dev->alsa_capture_handle, alsa_card_name,  
				dev->speak_attrs.samplepsec, dev->speak_attrs.channels, 
				dev->speak_attrs.format, dev->speak_attrs.uframes,
				dev->speak_attrs.mode) != 0)
			{
				ucam_error("salsa_playback_init fail") ;
			}
			usleep(10*1000);
			pthread_mutex_unlock(&dev->alsa_capture_mutex);
		}  
#else
		dev->uac->speak_stream_on_trigger = 0;
		dev->speak_attrs.stream_on_status = 0;	
#endif
	}
	
}

static void uac_events_process(struct uac_dev* dev)
{
    //struct v4l2_event v4l2_event;
    struct uac_event *uvc_event = (void *)&dev->v4l2_event.u.data;
    struct uac_request_data resp = {0};
    int ret;
    float volume_dB;
    char alsa_card_name[32];
    int error_cnt = 0;
    uint32_t new_rate = 0;

    ret = ioctl(dev->fd, VIDIOC_DQEVENT, &dev->v4l2_event);

    if (ret < 0)
    {
        ucam_error("VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno),
            errno);
        return;
    }

    ucam_trace("dev index : %d, v4l2_event.type:%d\n", dev->video_index, dev->v4l2_event.type);
    switch (dev->v4l2_event.type)
    {
        case UAC_EVENT_CONNECT:
            ucam_trace("handle connect event\n");
            dev->usb_speed = uvc_event->speed;
            if(dev->uac->uac_usb_connet_event_callback)
            {
                dev->uac->uac_usb_connet_event_callback(dev->uac, 1, dev->usb_speed, dev->bConfigurationValue);
            }
            dev->mic_attrs.stream_on_status = 0;
            dev->speak_attrs.stream_on_status = 0;
            if(dev->uac->uac_stream_event_callback)
            {
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_MIC, dev->mic_attrs.stream_on_status);
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_SPEAK, dev->speak_attrs.stream_on_status);
            }
            break;
        case UAC_EVENT_DISCONNECT:
            ucam_trace("handle disconnect event\n");
            if(dev->uac->uac_usb_connet_event_callback)
            {
                dev->uac->uac_usb_connet_event_callback(dev->uac, 0, dev->usb_speed, dev->bConfigurationValue);
            }
            dev->mic_attrs.stream_on_status = 0;
            dev->speak_attrs.stream_on_status = 0;
            if(dev->uac->uac_stream_event_callback)
            {
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_MIC, dev->mic_attrs.stream_on_status);
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_SPEAK, dev->speak_attrs.stream_on_status);
            }
            return;
			
		case UAC_EVENT_SUSPEND:
			ucam_trace("UAC_EVENT_SUSPEND\n");
			
			dev->uac->usb_suspend = 1;
			uac_streamoff_all(dev);
            if(dev->uac->usb_suspend_event_callback)
            {
                dev->uac->usb_suspend_event_callback(dev->uac); 
            }
			break;
			
		case UAC_EVENT_RESUME:
			ucam_trace("UAC_EVENT_RESUME\n");
			
			dev->uac->usb_suspend = 0;
            if(dev->uac->usb_resume_event_callback)
            {
                dev->uac->usb_resume_event_callback(dev->uac); 
            }
			break;
			
        case UAC_EVENT_STREAMON:
            ucam_trace("handle streamon event\n");
			dev->uac->usb_suspend = 0;
            pthread_mutex_lock(&dev->alsa_playback_mutex);
            if(dev->alsa_playback_handle == NULL)
            {
             
playback_init:
                memset(alsa_card_name, 0, sizeof(alsa_card_name));
                sprintf(alsa_card_name, "plughw:%d,0", dev->alsa_card_id);
                if(uac_alsa_playback_init(&dev->alsa_playback_handle, alsa_card_name,  
                    dev->mic_attrs.samplepsec, dev->mic_attrs.channels,
                    dev->mic_attrs.format, dev->mic_attrs.uframes,
                    dev->mic_attrs.mode) != 0)
                {
                    ucam_error("alsa_playback_init fail") ;
                    dev->alsa_playback_handle = NULL;
                    error_cnt ++;
                    if(error_cnt < 5)
                    {
                        usleep(50*1000);
                        goto playback_init;
                    }
                    //goto error;
                }
                usleep(10*1000);
            }
            if(g_uac_mic_temp_buf != NULL)
            {
                free(g_uac_mic_temp_buf);
                g_uac_mic_temp_buf = NULL;
            }
#if (defined CHIP_SSC333 || defined CHIP_SSC9211 || defined CHIP_HI3516EV300 || defined CHIP_GK7205V300)
		    g_uac_mic_temp_buf = malloc(16*1024);
#else
		    g_uac_mic_temp_buf = malloc(32*1024);
#endif
            if(g_uac_mic_temp_buf == NULL)
            {
                ucam_error("malloc fail") ;
            }
            pthread_mutex_unlock(&dev->alsa_playback_mutex);

            if(dev->alsa_playback_handle)
            {
                dev->uac->mic_stream_on_trigger = 1;
                dev->mic_attrs.stream_on_status = 1;
            }

            
            if(dev->uac->uac_stream_event_callback)
            {
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_MIC, dev->mic_attrs.stream_on_status);
            }
            return;
        case UAC_EVENT_STREAMOFF:
            ucam_trace("handle streamoff\n");
            dev->uac->mic_stream_on_trigger = 0;
            dev->mic_attrs.stream_on_status = 0;
            
            if(dev->alsa_playback_handle)
            {
                usleep(100*1000);
                pthread_mutex_lock(&dev->alsa_playback_mutex);
				if(dev->alsa_playback_handle)
				{
	                if(uac_alsa_playback_close(dev->alsa_playback_handle) != 0)
	                {
	                    ucam_error("alsa_playback_close error!");
	                }
	                dev->alsa_playback_handle = NULL;
				}
                if(g_uac_mic_temp_buf != NULL)
                {
                    free(g_uac_mic_temp_buf);
                    g_uac_mic_temp_buf = NULL;
                }
                pthread_mutex_unlock(&dev->alsa_playback_mutex);
            }
            
            if(dev->uac->uac_stream_event_callback)
            {
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_MIC, dev->mic_attrs.stream_on_status);
            }
            return;
        case UAC_EVENT_SETUP:
			dev->uac->usb_suspend = 0;
            uac_events_process_setup(dev, &uvc_event->req, &resp);
            ucam_trace("respons with %d bytes:", resp.length );
            break;
        case UAC_EVENT_DATA:
            ucam_trace("handle data\n");
            break;
        case UAC_MIC_VOLUME:
			dev->uac->usb_suspend = 0;
            dev->mic_attrs.volume = ((short *)&dev->v4l2_event.u.data)[0];
            volume_dB = dev->mic_attrs.volume/256.0f;
            ucam_trace("UAC setMicVol = %d (0x%04x), %.2f dB",dev->mic_attrs.volume, dev->mic_attrs.volume, volume_dB);

            volume_dB += 0.5; 
            //setMicVol((int)volume_dB);
            if(dev->uac->uac_volume_callback)
            {
                dev->uac->uac_volume_callback(dev->uac, UAC_DIR_MIC, (int)volume_dB);
            }
            break;
        case UAC_MIC_MUTE:
            ucam_trace("set mic mute");
			dev->uac->usb_suspend = 0;
            dev->mic_attrs.mute = ((unsigned char *)&dev->v4l2_event.u.data)[0];
            if(dev->uac->uac_mute_callback)
            {
                dev->uac->uac_mute_callback(dev->uac, UAC_DIR_MIC, dev->mic_attrs.mute);
            }
            break;

        case UAC_EVENT_STREAMON_SPEAK:
            ucam_trace("set Speak streamon");
			dev->uac->usb_suspend = 0;
            dev->uac->speak_stream_on_trigger = 1;
            dev->speak_attrs.stream_on_status = 1;
            
            if(dev->uac->uac_stream_event_callback)
            {
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_SPEAK, dev->speak_attrs.stream_on_status);
            }
            break;

        case UAC_EVENT_STREAMOFF_SPEAK:
            ucam_trace("set Speak streamoff");
            dev->uac->speak_stream_on_trigger = 0;
            dev->speak_attrs.stream_on_status = 0;
#if 0            
            if(dev->alsa_capture_handle)
            {
                usleep(100*1000);
                pthread_mutex_lock(&dev->alsa_capture_mutex);
				if(dev->alsa_capture_handle)
				{
	                if(uac_alsa_capture_close(dev->alsa_capture_handle) != 0)
	                {
	                    ucam_error("alsa_capture_close error!");
	                }
	                dev->alsa_capture_handle = NULL;
				}

                memset(alsa_card_name, 0, sizeof(alsa_card_name));
                sprintf(alsa_card_name, "plughw:%d,0", dev->alsa_card_id);
                if(alsa_capture_init(&dev->alsa_capture_handle, alsa_card_name,  
                    dev->speak_attrs.samplepsec, dev->speak_attrs.channels, 
                    dev->speak_attrs.format, dev->speak_attrs.uframes,
                    dev->speak_attrs.mode) != 0)
                {
                    ucam_error("salsa_playback_init fail") ;
                }
                usleep(10*1000);
                pthread_mutex_unlock(&dev->alsa_capture_mutex);
            }
            
#endif     
            if(dev->uac->uac_stream_event_callback)
            {
                dev->uac->uac_stream_event_callback(dev->uac, UAC_DIR_SPEAK, dev->speak_attrs.stream_on_status);
            }
            break;
            
        case UAC_SPEAK_VOLUME:
			dev->uac->usb_suspend = 0;
            dev->speak_attrs.volume = ((short *)&dev->v4l2_event.u.data)[0];
            volume_dB = dev->speak_attrs.volume/256.0f;
            ucam_trace("UAC setSpeakVol = %d (0x%04x), %.2f dB",dev->speak_attrs.volume, dev->speak_attrs.volume, volume_dB);
            volume_dB += 0.5; 
            if(dev->uac->uac_volume_callback)
            {
                dev->uac->uac_volume_callback(dev->uac, UAC_DIR_SPEAK, (int)volume_dB);
            }

            break;
        case UAC_SPEAK_MUTE:
            ucam_trace("set Speak mute");
			dev->uac->usb_suspend = 0;
            dev->speak_attrs.mute = ((unsigned char *)&dev->v4l2_event.u.data)[0];
            if(dev->uac->uac_mute_callback)
            {
                dev->uac->uac_mute_callback(dev->uac, UAC_DIR_SPEAK, dev->speak_attrs.mute);
            }
            break;
        case UAC_MIC_SAMPLERATE:
            ucam_trace("set Mic sample rate");
            new_rate = *(uint32_t *)&dev->v4l2_event.u.data;
            if (new_rate != 96000 && new_rate != 48000 && new_rate != 44100 && new_rate != 32000 && new_rate != 16000) {
                ucam_error("Unsupported sample rate: %d", new_rate);
                break;
            }
            if (dev->mic_attrs.samplepsec != new_rate)
            {
                dev->mic_attrs.samplepsec = new_rate;
                dev->mic_attrs.uframes = (dev->mic_attrs.samplepsec * 15) / 1000;

                if(dev->mic_attrs.uframes > 720)
                {
                    dev->mic_attrs.uframes = 720;
                }
                
                ucam_trace("UAC set mic sample rate =: %d, uframes: %ld", dev->mic_attrs.samplepsec, dev->mic_attrs.uframes);
                
                if(dev->uac->uac_set_rate_callback)
                {
                    dev->uac->uac_set_rate_callback(dev->uac, UAC_DIR_MIC, dev->mic_attrs.samplepsec);
                }
            }

            break;
        case UAC_SPEAK_SAMPLERATE:
            ucam_trace("set Speak sample rate");
            new_rate = *(uint32_t *)&dev->v4l2_event.u.data;
            if (new_rate != 96000 && new_rate != 48000 && new_rate != 44100 && new_rate != 32000 && new_rate != 16000) {
                ucam_trace("Unsupported sample rate: %d", new_rate);
                break;
            }
            if (dev->speak_attrs.samplepsec != new_rate)
            {
                dev->speak_attrs.samplepsec = new_rate;
                ucam_trace("UAC set speak sample rate =: %d", dev->speak_attrs.samplepsec);
                if(dev->uac->uac_set_rate_callback)
                {
                    dev->uac->uac_set_rate_callback(dev->uac, UAC_DIR_SPEAK, dev->speak_attrs.samplepsec);
                }
            }
            
            break;
        default:
            break;
    }

    if ( resp.length > 0 )
    { 
        ucam_trace("respons with %d bytes cmd:%x", resp.length , UACIOC_SEND_RESPONSE);
        //printf("respons with %d bytes:\n", resp.length );
        //hexout((char*)resp.data, resp.length );

        ret = ioctl(dev->fd, UACIOC_SEND_RESPONSE, &resp);
        if (ret < 0) { 
            ucam_error("UVCIOC_S_EVENT failed: %s (%d)", strerror(errno), errno);
            return;
        }    

    }    
    else
    {
	    ucam_trace("no respons");
    }    

}

static void uac_events_init(struct uac_dev* dev)
{
    struct v4l2_event_subscription sub;

    memset(&sub, 0, sizeof sub);
	int i = UAC_EVENT_FIRST;
    for ( i = UAC_EVENT_FIRST; i <= UAC_EVENT_LAST; i++ ){
		sub.type = i;
		ioctl(dev->fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
	}
}

static struct uac_dev * open_uac_dev(const char *devpath)
{
    struct uac_dev* dev;

    char* device = (char*)devpath;

    dev = uac_open(device);
    if (dev == 0)
    {
        return NULL;
    }

    //uac_video_init(dev);
    uac_events_init(dev);

    return dev;
}

static int close_uac_dev(struct uac_dev *dev)
{
	if(g_uac_mic_temp_buf != NULL)
	{
		free(g_uac_mic_temp_buf);
		g_uac_mic_temp_buf = NULL;
	}
    if (dev != NULL)
    {
        uac_close(dev);
    }

    dev = NULL;

    return 0;
}

static void *uac_event_thread(void *args)
{
	fd_set fds;
	struct timeval tv;
	int ret;

    struct uac_dev *uac_dev = (struct uac_dev *)args;
	
	FD_ZERO(&fds);
	FD_SET(uac_dev->fd, &fds);
		
	prctl(PR_SET_NAME, "uac_thread");

    uac_dev->uac_event_thread_exit = 0;
	while(!uac_dev->uac_event_thread_exit)
	{
        fd_set efds = fds;
        tv.tv_sec=0;
		tv.tv_usec=10000;
		//ret = select(uac_dev->fd + 1, NULL, NULL, &efds, NULL);
		ret = select(uac_dev->fd + 1, NULL, NULL, &efds, &tv);//必须得用超时，否则会由于不同步导致select不到命令问题
        if ( ret == - 1 ){
			ucam_trace("select fail!");
			continue;
		}
		if (FD_ISSET(uac_dev->fd, &efds))
			uac_events_process(uac_dev);
		
	}

    uac_dev->uac_event_thread_exit = 1;
    ucam_debug("thread exit");
    ret = 0;
    pthread_exit(&ret); 
	return NULL;
}


struct uac_dev *uac_add_dev(struct uac *uac, int bConfigurationValue, int video_index, uint32_t alsa_card_id, 
    uint8_t uac_mic_enable, uint8_t uac_speak_enable, struct uac_attrs *mic_attrs, struct uac_attrs *speak_attrs)
{
    struct uac_dev *uac_dev = 0;
    struct ucam_list_head *pos;
    char device[16];
    char alsa_card_name[32];

    if(uac_mic_enable == 0 && uac_speak_enable == 0)
    {
        ucam_trace("setings error");
	    return NULL; 
    }

    if(uac_mic_enable == 1 && mic_attrs == NULL)
    {
        ucam_trace("setings error");
	    return NULL; 
    }

    if(uac_speak_enable == 1 && speak_attrs == NULL)
    {
        ucam_trace("setings error");
	    return NULL; 
    }

    memset(device, 0, sizeof(device));
	sprintf(device, "/dev/video%d", video_index);
    
    memset(alsa_card_name, 0, sizeof(alsa_card_name));
    sprintf(alsa_card_name, "plughw:%d,0", alsa_card_id);
	


    ucam_list_for_each(pos, &uac->uac_dev_list) {
        uac_dev = ucam_list_entry(pos, struct uac_dev, list);

        //判断是否已经注册过了
        if(uac_dev && uac_dev->bConfigurationValue == bConfigurationValue)
        {
            return NULL;
        }
    }
	
    uac_dev = NULL;
	uac_dev = open_uac_dev(device);
	if (uac_dev == NULL)
	{
		ucam_trace("open uac device: %s fail",device);
		return NULL;
	}

	uac_dev->uac = uac;
	uac_dev->uac_init_flag = 1;
	uac_dev->video_index = video_index;
    uac_dev->bConfigurationValue = bConfigurationValue;
    uac_dev->alsa_card_id = alsa_card_id;
    uac_dev->uac_mic_enable = uac_mic_enable;
    uac_dev->uac_speak_enable = uac_speak_enable;
	
	ucam_trace("uac device path: %s, index: %d",device, uac_dev->video_index);
   

   pthread_mutex_init(&uac_dev->alsa_playback_mutex, 0);
   pthread_mutex_init(&uac_dev->alsa_capture_mutex, 0);
	
	//SetUacMicVolGain(UAC_DEF_VOLUME);
    //uac->format = SND_PCM_FORMAT_S16_LE;
    //uac->uframes = UAC_FRAME_SIZE(uac->samplepsec, 16, uac->channels, 1);

    if(uac_dev->uac_mic_enable)
    {
        memcpy(&uac_dev->mic_attrs,mic_attrs,sizeof(struct uac_attrs));
        uac_dev->mic_attrs.stream_on_status = 0;
        uac_dev->mic_attrs.mute = 0;
        if(uac_alsa_playback_init(&uac_dev->alsa_playback_handle, alsa_card_name,  
            uac_dev->mic_attrs.samplepsec, uac_dev->mic_attrs.channels,
            uac_dev->mic_attrs.format, uac_dev->mic_attrs.uframes,
            uac_dev->mic_attrs.mode) != 0)
        {
            ucam_error("salsa_playback_init fail") ;
            goto error;
        }
    }
    
    if(uac_dev->uac_speak_enable)
    {
        memcpy(&uac_dev->speak_attrs,speak_attrs,sizeof(struct uac_attrs));
        uac_dev->speak_attrs.stream_on_status = 0;
        uac_dev->speak_attrs.mute = 0;
        if(uac_alsa_capture_init(&uac_dev->alsa_capture_handle, alsa_card_name,  
            uac_dev->speak_attrs.samplepsec, uac_dev->speak_attrs.channels,
            uac_dev->speak_attrs.format, uac_dev->speak_attrs.uframes,
            uac_dev->speak_attrs.mode) != 0)
        {
            ucam_error("alsa_capture_init fail") ;
            goto error;
        }
    }


    if (0 != pthread_create(&uac_dev->uac_event_thread_pid, NULL, uac_event_thread, (void *)uac_dev)) {
            ucam_error("pthread_create fail") ;
			fprintf(stderr,"uac_c1_pthread_create failed!\n");
			goto error;
	}

    if(uac->uac_dev == NULL || uac_dev->bConfigurationValue == 1)
    {
       uac->uac_dev = uac_dev;   
    } 
    ucam_list_add_tail(&uac_dev->list, &uac->uac_dev_list);

    return uac_dev;

error:
    close_uac_dev(uac_dev);
    return NULL;
}


int uac_set_usb_configuration_value(struct uac *uac, int val)
{
    struct uac_dev *uac_dev;
    struct ucam_list_head *pos;

    if(uac == NULL)
        return -ENOMEM;
           

    ucam_list_for_each(pos, &uac->uac_dev_list) {
        uac_dev = ucam_list_entry(pos, struct uac_dev, list);

        if(uac_dev && uac_dev->bConfigurationValue == val)
        {
            uac->bConfigurationValue = val;
            uac->uac_dev = uac_dev;
            ucam_trace("bConfigurationValue: %d",val);
            return 0;
        }
    }

    return -1;
}

int uac_set_mute_status(struct uac *uac, enum uac_dir_type_t dir, int val)
{
    if(uac == NULL)
        return -ENOMEM;

    if(dir == UAC_DIR_MIC)
        uac->uac_dev->mic_attrs.mute = val;
    else if(dir == UAC_DIR_SPEAK)
        uac->uac_dev->speak_attrs.mute = val;
    else
        return -1;

    return 0;
}

int uac_get_mute_status(struct uac *uac, enum uac_dir_type_t dir, uint8_t *mute_status)
{
    if(uac == NULL)
        return -ENOMEM;

    if(dir == UAC_DIR_MIC)
        *mute_status = uac->uac_dev->mic_attrs.mute;
    else if(dir == UAC_DIR_SPEAK)
        *mute_status = uac->uac_dev->speak_attrs.mute;
    else
        return -1;

    return 0;
}

int uac_get_mic_mute_status(struct uac *uac, uint8_t *mute_status)
{
    if(uac == NULL)
        return -ENOMEM;


    *mute_status = uac->uac_dev->mic_attrs.mute;

    return 0;
}

int uac_get_speak_mute_status(struct uac *uac, uint8_t *mute_status)
{
    if(uac == NULL)
        return -ENOMEM;


    *mute_status = uac->uac_dev->speak_attrs.mute;

    return 0;
}

int uac_get_stream_on_status(struct uac *uac, enum uac_dir_type_t dir, uint8_t *stream_on_status)
{
    if(uac == NULL)
        return -ENOMEM;

    if(dir == UAC_DIR_MIC)
    {
        *stream_on_status = uac->uac_dev->mic_attrs.stream_on_status && uac->uac_dev->uac_mic_enable;
    }
    else if(dir == UAC_DIR_SPEAK)
    {
        *stream_on_status = uac->uac_dev->speak_attrs.stream_on_status && uac->uac_dev->uac_speak_enable;
    }
    else
    {
        return -1;
    }

	return 0;
}

int uac_get_mic_stream_on_status(struct uac *uac, uint8_t *stream_on_status)
{
    if(uac == NULL)
        return -ENOMEM;

    *stream_on_status = uac->uac_dev->mic_attrs.stream_on_status;

	return 0;
}

int uac_get_speak_stream_on_status(struct uac *uac, uint8_t *stream_on_status)
{
    if(uac == NULL)
        return -ENOMEM;

    *stream_on_status = uac->uac_dev->speak_attrs.stream_on_status;

	return 0;
}

int uac_get_all_stream_on_status(struct uac *uac, uint8_t *stream_on_status)
{
    if(uac == NULL)
        return -ENOMEM;

    *stream_on_status = (uac->uac_dev->mic_attrs.stream_on_status && uac->uac_dev->uac_mic_enable) ||
        (uac->uac_dev->speak_attrs.stream_on_status && uac->uac_dev->uac_speak_enable);

	return 0;
}

int uac_get_mic_enable(struct uac *uac, uint8_t *enable)
{
    if(uac == NULL)
        return -ENOMEM;

    *enable = uac->uac_dev->uac_mic_enable;
    
    return 0;
}

int uac_get_speak_enable(struct uac *uac, uint8_t *enable)
{
    if(uac == NULL)
        return -ENOMEM;

    *enable = uac->uac_dev->uac_speak_enable;
    
    return 0;
}


int audio_data_16bit_1ch_fix_point(const int16_t* src, int16_t* dst, int data_count, int point)
{
    int fix_point = 0;
    int i,j;
    int step;
    int16_t* p_dst;

    //丢数据不能超过一个data_count
    if (point > data_count || (point + data_count) <= 0 || point == 0 || data_count < 2)
    {
        return -1;
    }

    if (point > 0)
    {
        //补数据
        //间隔多少点插入一个点
        step = (data_count - 1) / point;

        j = 0;
        p_dst = dst;
        if (point == data_count)
        {
            p_dst[0] = 2 * src[0] - (src[0] + src[1])/2;
            p_dst++;
            fix_point++;
        }
        for (i = 0; i < data_count; i++)
        {
            p_dst[0] = src[i];
            j++;
            if(j >= step && fix_point < point)
            {
                //插入这个点
                p_dst[1] = (src[i] + src[i + 1]) >> 1;
                p_dst++;
                fix_point++;
                j = 0;
            }
            p_dst++;
        }     
    }
    else if (point < 0)
    {
        //丢数据
        //间隔多少点丢一个点,step必须大于0
        step = (data_count -1) / (-1 * point);
        
        j = 0;
        p_dst = dst;
        for (i = 0; i < data_count; i++)
        {
            j++;
            if (j >= step && fix_point > point)
            {
                //丢掉这个点
                fix_point --;
                j = 0;
            }
            else
            {
                //p_dst[0] = (src[i] + src[i + 1]) >> 1;
                p_dst[0] = src[i];
                p_dst++;
            }

        }
    }

    if(fix_point != point)
    {
        ucam_error("error data_count:%d, point:%d, fix_point:%d\n", data_count,point, fix_point);
        return -1;
    }

    return 0;
}

int audio_data_16bit_2ch_fix_point(const int16_t* src, int16_t* dst, int data_count, int point)
{
    int fix_point = 0;
    int i,j;
    int step;
    int16_t *p_src, *p_dst;

    //丢数据不能超过一个data_count
    if (point > data_count || (point + data_count) <= 0 || point == 0 || data_count < 2)
    {
        return -1;
    }

    if(point > 0)
    {
        //补数据
        //间隔多少点插入一个点
        step = (data_count - 1) / point;

        j = 0;
        p_src = (int16_t*)src;
        p_dst = dst;

        if (point == data_count)
        {
            p_dst[0] = 2 * src[0] - (src[0] + src[2])/2;
            p_dst[1] = 2 * src[1] - (src[1] + src[3])/2;
            p_dst += 2;
            fix_point++;
        }
        for (i = 0; i < data_count; i++)
        {
            p_dst[0] = p_src[0];//ch 1
            p_dst[1] = p_src[1];//ch 2

            j++;
            if(j >= step && fix_point < point)
            {
                //插入这个点
                p_dst[2] = (p_src[0] + p_src[2]) >> 1;
                p_dst[3] = (p_src[1] + p_src[3]) >> 1;
                p_dst += 2;
                fix_point++;
                j = 0;
            }
            p_src += 2;
            p_dst += 2;
        }
    }
    else if(point < 0)
    {
        //丢数据
        //间隔多少点丢一个点,step必须大于0
        step = (data_count -1) / (-1 * point);

        j = 0;
        p_src = (int16_t*)src;
        p_dst = dst;
        for (i = 0; i < data_count; i++)
        {
            j++;
            if (j >= step && fix_point > point)
            {
                //丢掉这个点
                fix_point --;
                j = 0;
            }
            else
            {
                p_dst[0] = p_src[0];
                p_dst[1] = p_src[1];
                p_dst += 2;
            }
            p_src += 2;  
        }
    }

    if(fix_point != point)
    {
        ucam_error("error data_count:%d, point:%d, fix_point:%d\n", data_count,point, fix_point);
        return -1;
    }

    return 0;
}

/*
低水位为2个period_size,最高水位为6个period_size
开始启动时为2个period_size，也就是延时为2个period_size
低于3 period_size时补数据，高于4 period_size时丢数据
丢或者补数据默认按照时钟每秒相差10us来算
10us = samplepsec/100000, 每1S补samplepsec/100000 , 例如48K 100S补48个点
*/

int pcm_playback_avail_check_write(struct uac *uac, const void *data, size_t data_count, int timeout_ms)
{
    int ret = 0;
    snd_pcm_uframes_t avail_frames,buffer_size,period_size, used_buf;
    static snd_pcm_uframes_t last_used_buf = 0;
    uint8_t i;
    size_t count;
    static int fix_count = 0;
    static int fix_flag = 0;
    static struct timeval last_tv;
    struct timeval cur_tv;
    long long time_diff;

    static uint32_t time_cnt = 0;
	
	float min_error_offset = 0;

    if(uac->uac_dev->alsa_playback_handle == NULL)
    {
        return -1;
    }
	if(uac->uac_dev->mic_attrs.channels > 2)
    {
        ucam_error(" error: not support channels %d", uac->uac_dev->mic_attrs.channels);
        return -1;
    }

    gettimeofday(&cur_tv,NULL);


    ret = snd_pcm_get_params(uac->uac_dev->alsa_playback_handle,
                    &buffer_size,
                    &period_size);
    if(ret != 0)
    {
        ucam_error("snd_pcm_get_params error");
        return -1;
    }

    if(uac->mic_stream_on_trigger == 1)
    {
        uac->mic_stream_on_trigger = 0;
        count = uac->uac_dev->mic_attrs.start_delay;
        goto __pcm_write_start;
    }
    
    avail_frames = snd_pcm_avail(uac->uac_dev->alsa_playback_handle);
    if (avail_frames == -EPIPE)
    {
        ucam_error("avail underrun occurred, avail = %d\n", avail_frames);
        count = uac->uac_dev->mic_attrs.delay;
        goto __pcm_write_start;
    }
    else
    {
        //avail_frames代表空闲的buf， buffer_size - avail_frames 为已经使用的buf
        used_buf = buffer_size - avail_frames;
		if(uac->uac_dev->mic_attrs.delay < 2)
		{
			min_error_offset = 0.5;
		}
        //ucam_trace("used_buf = %d, period_size = %d\n", 
        //        used_buf,period_size);
        if(used_buf < period_size)
        {
            //小于1 period_size，即将要空了，填到最低水位
            time_diff = get_time_difference(&last_tv, &cur_tv);
            if((time_diff/1000) >= 2*uac->uac_dev->mic_attrs.samplepsec/(10 * period_size))
            {
                //线程调度不及时，很快就来第二帧数据，先不补
                if(used_buf > (period_size/10))
                    goto __pcm_write;
            }
            ucam_error("underrun occurred, avail = %d, period_size = %d, buffer_size = %d\n", 
                avail_frames,period_size,buffer_size);
                
            fix_count = period_size - used_buf;
            goto __pcm_write_fix;
        }
        else if(avail_frames < period_size)
        {
            //大于5 period_size，即将要满了，只允许写到最高水位
            ucam_error("overrun occurred, avail = %d, period_size = %d, buffer_size = %d\n", 
                avail_frames,period_size,buffer_size);

            //丢数据为负数  
            fix_count =  avail_frames - data_count;
            goto __pcm_write_fix;
        }
#if 1
        else if(used_buf > (period_size  * (uac->uac_dev->mic_attrs.delay + 1 + min_error_offset)))
        {
            //大于3 period_size，上升趋势，要丢数据
            time_cnt += period_size;
            if(time_cnt < (uac->uac_dev->mic_attrs.samplepsec) >> 2)
            {
                //没到时间
                goto __pcm_write;
            }
            
            if(fix_flag == 0)
            {
                fix_count = -1;
            }
            if(fix_flag == -1 && last_used_buf < used_buf)
            {
                //第二次进来,比上次还上升得多加大丢数据
                fix_count = fix_count << 1;
            }
            fix_flag = -1;
            last_used_buf = used_buf;
            goto __pcm_write_fix;
        }
        else if(used_buf < (period_size * (uac->uac_dev->mic_attrs.delay + min_error_offset)))
        {
            //小于3 period_size，下降趋势，要补数据
            time_cnt += period_size;
            if(time_cnt < (uac->uac_dev->mic_attrs.samplepsec) >> 2)
            {
                //没到时间
                goto __pcm_write;
            }

            if(fix_flag == 0)
            {
                fix_count = 1;
            }  
            if(fix_flag == 1 && last_used_buf > used_buf)
            {
                //第二次进来，比上次还下降得多加大补数据
                fix_count = fix_count << 1;
            }
            fix_flag = 1;
            last_used_buf = used_buf;
            goto __pcm_write_fix;
        }
#endif
        else
        {
            //正常3 ~ 4 period_size，不需要处理，
            time_cnt = 0;
            fix_flag = 0;
            goto __pcm_write;
        }
    }

    return 0;

//启动时处理
__pcm_write_start:
    time_cnt = 0;
    fix_flag = 0;
    last_tv = cur_tv;
    if(count < 1)
    {
        return 0;
    }
    
    if(g_uac_mic_temp_buf == NULL)
    {
        ucam_error("buf error");
        return 1;
    }
    memset(g_uac_mic_temp_buf, 0, period_size * uac->uac_dev->mic_attrs.channels * snd_pcm_format_width(uac->uac_dev->mic_attrs.format)/8);
    for(i = 0; i < count; i++)
    {
        ret = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, g_uac_mic_temp_buf, period_size, 20);
        if(ret != period_size)
        {
            ucam_error("alsa pcm_write error");
            return 1;
        }
    }

    ret = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, data, data_count, timeout_ms);
    if(ret != count)
    {
        ucam_error("alsa pcm_write error");
        return 1;
    }
    
    return 0;

//正常处理
__pcm_write:
    last_tv = cur_tv;
    ret = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, data, data_count, timeout_ms);
    if(ret != data_count)
    {
        ucam_error("alsa pcm_write error");
        return 1;
    }
    return 0;

//xrun修补处理
__pcm_write_fix:
    ucam_trace("pcm_write_fix:%d",fix_count);
    time_cnt = 0;
    last_tv = cur_tv;
	
    if(g_uac_mic_temp_buf == NULL)
    {
        ucam_error("buf error");
        return 1;
    }

    if(fix_count == 0)
    {
        ret = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, data, data_count, timeout_ms);
        if(ret != data_count)
        {
            ucam_error("alsa pcm_write error");
            return 1;
        }
        return 0;
    }
    else if((int)(data_count + fix_count) <= 0)
    {
        return 0;
    }
  
    if(uac->uac_dev->mic_attrs.channels == 2)
    {
        ret = audio_data_16bit_2ch_fix_point((const int16_t*)data, 
            (int16_t *)g_uac_mic_temp_buf, data_count, fix_count);
    }
    else
    {
        ret = audio_data_16bit_1ch_fix_point((const int16_t*)data, 
            (int16_t *)g_uac_mic_temp_buf, data_count, fix_count);
    }

    if(ret == 0)
    {
        ret = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, 
            (int16_t *)g_uac_mic_temp_buf, data_count + fix_count, timeout_ms);
        if(ret != (data_count + fix_count))
        {
            ucam_error("alsa pcm_write error");
            return 1;
        }
    }
    
    return 0;
}

int uac_speak_ao_buf_avail_check(struct uac *uac,
    snd_pcm_uframes_t avail_frames,
    snd_pcm_uframes_t buffer_size,
    snd_pcm_uframes_t period_size,
    void *data, size_t data_count, 
    void *fix_buf, int *fix_count)
{
    int ret = 0;
    snd_pcm_uframes_t used_buf;
    static snd_pcm_uframes_t last_used_buf = 0;

    static int g_fix_count = 0;
    static int fix_flag = 0;
    static struct timeval last_tv;
    struct timeval cur_tv;
    long long time_diff;

    static uint32_t time_cnt = 0;
	float min_error_offset = 0;

    if(uac->uac_dev->alsa_capture_handle == NULL)
    {
        return -1;
    }
    if(uac->uac_dev->speak_attrs.channels > 2)
    {
        ucam_error(" error: not support channels %d", uac->uac_dev->speak_attrs.channels);
        return -1;
    }
	
    *fix_count = 0;

    gettimeofday(&cur_tv,NULL);

    //avail_frames代表空闲的buf， buffer_size - avail_frames 为已经使用的buf
    used_buf = buffer_size - avail_frames;
	if(uac->uac_dev->speak_attrs.delay < 2)
	{
		min_error_offset = 0.5;
	}
    //ucam_trace("used_buf = %d, period_size = %d\n", 
    //        used_buf,period_size);
    if(used_buf < period_size)
    {
        //小于1 period_size，即将要空了，填到最低水位
        time_diff = get_time_difference(&last_tv, &cur_tv)/1000;
 		//播放被暂停过了，重新播放	
		if((time_diff > 100 && used_buf == 0) || uac->uac_dev->uac->speak_stream_on_trigger == 1)
		{
            if(uac->uac_dev->speak_attrs.stream_on_status == 0)
            {
                return -1;
            }
            uac->uac_dev->uac->speak_stream_on_trigger = 0;
			time_cnt = 0;
			fix_flag = 0;
			g_fix_count = period_size * (uac->uac_dev->speak_attrs.delay);
			memset(fix_buf, 0,snd_pcm_frames_to_bytes(uac->uac_dev->alsa_capture_handle, g_fix_count));
			memcpy(fix_buf + snd_pcm_frames_to_bytes(uac->uac_dev->alsa_capture_handle,g_fix_count), 
				data, snd_pcm_frames_to_bytes(uac->uac_dev->alsa_capture_handle,data_count));
			ucam_error("uac ao start");
			*fix_count = g_fix_count;
			goto __pcm_write;
		}

        if(time_diff >= 2*uac->uac_dev->speak_attrs.samplepsec/(10 * period_size))
        {
            //线程调度不及时，很快就来第二帧数据，先不补
            if(used_buf > (period_size/10))
                goto __pcm_write;
        }
        ucam_error("underrun occurred, avail = %d, period_size = %d, buffer_size = %d\n", 
            avail_frames,period_size,buffer_size);
            
        g_fix_count = period_size - used_buf;
        goto __pcm_write_fix;
    }
    else if(avail_frames < period_size)
    {
        //大于5 period_size，即将要满了，只允许写到最高水位
        ucam_error("overrun occurred, avail = %d, period_size = %d, buffer_size = %d\n", 
            avail_frames,period_size,buffer_size);

        //丢数据为负数  
        g_fix_count =  avail_frames - data_count;
        goto __pcm_write_fix;
    }
#if 1
    else if(used_buf > (period_size * (uac->uac_dev->speak_attrs.delay + 1 + min_error_offset)))
    {
        //大于3 period_size，上升趋势，要丢数据
        time_cnt += period_size;
        if(time_cnt < (uac->uac_dev->speak_attrs.samplepsec) >> 2)
        {
            //没到时间
            goto __pcm_write;
        }
        
        if(fix_flag == 0)
        {
            g_fix_count = -1;
        }
        if(fix_flag == -1 && last_used_buf < used_buf)
        {
            //第二次进来,比上次还上升得多加大丢数据
            g_fix_count = g_fix_count << 1;
        }
        fix_flag = -1;
        last_used_buf = used_buf;
        goto __pcm_write_fix;
    }
    else if(used_buf < (period_size * (uac->uac_dev->speak_attrs.delay + min_error_offset)))
    {
        //小于3 period_size，下降趋势，要补数据
        time_cnt += period_size;
        if(time_cnt < (uac->uac_dev->speak_attrs.samplepsec) >> 2)
        {
            //没到时间
            goto __pcm_write;
        }

        if(fix_flag == 0)
        {
            g_fix_count = 1;
        }  
        if(fix_flag == 1 && last_used_buf > used_buf)
        {
            //第二次进来，比上次还下降得多加大补数据
            g_fix_count = g_fix_count << 1;
        }
        fix_flag = 1;
        last_used_buf = used_buf;
        goto __pcm_write_fix;
    }
#endif
    else
    {
        //正常3 ~ 4 period_size，不需要处理，
        time_cnt = 0;
        fix_flag = 0;
        goto __pcm_write;
    }
    return 0;

//正常处理
__pcm_write:
    last_tv = cur_tv;
    return 0;

//xrun修补处理
__pcm_write_fix:
    ucam_trace("ao_fix:%d",g_fix_count);
    time_cnt = 0;
    last_tv = cur_tv;
	
    if(fix_buf == NULL)
    {
        ucam_error("buf error");
        return -1;
    }

    if(g_fix_count == 0)
    {
        return 0;
    }
    else if((int)(data_count + g_fix_count) <= 0)
    {
        *fix_count = g_fix_count;//全部丢弃
        return -1;
    }
  
    if(uac->uac_dev->speak_attrs.channels == 2)
    {
        ret = audio_data_16bit_2ch_fix_point((const int16_t*)data, 
            (int16_t *)fix_buf, data_count, g_fix_count);
    }
    else
    {
        ret = audio_data_16bit_1ch_fix_point((const int16_t*)data, 
            (int16_t *)fix_buf, data_count, g_fix_count);
    }
    *fix_count = g_fix_count;

    return ret;
}


int uac_send_mic_stream(struct uac *uac, const void *data, size_t count)
{
    int ret;
    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }
    if(uac->uac_dev->mic_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_playback_mutex);
        ret = pcm_playback_avail_check_write(uac, data, count, 20);     
        pthread_mutex_unlock(&uac->uac_dev->alsa_playback_mutex);
        return ret;
    }
    else
        return -1;
}

int uac_send_mic_stream_timeout(struct uac *uac, const void *data, size_t count, int timeout_ms)
{
    int ret;
    
    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev->mic_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_playback_mutex);
        ret = pcm_playback_avail_check_write(uac, data, count, timeout_ms);
        pthread_mutex_unlock(&uac->uac_dev->alsa_playback_mutex);
        return ret;
    }
    else
        return -1;
}

int uac_read_speak_stream(struct uac *uac, void *data, size_t count)
{
    int ret;
    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev->speak_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_capture_mutex);
        ret = uac_pcm_read(uac->uac_dev->alsa_capture_handle, data, count, 0);
        pthread_mutex_unlock(&uac->uac_dev->alsa_capture_mutex);
        return ret;
    }
    else
        return -1;
}

int uac_send_mic_stream_v2(struct uac *uac, const void *data, size_t count)
{
    //int ret = 0;
    size_t write_count;
    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }
    if(uac->uac_dev->mic_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_playback_mutex);
        write_count = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, data, count, 20);   
        pthread_mutex_unlock(&uac->uac_dev->alsa_playback_mutex);
        return write_count;
    }
    else
        return -1;
}

int uac_send_mic_stream_timeout_v2(struct uac *uac, const void *data, size_t count, int timeout_ms)
{
    //int ret = 0;
    size_t write_count;

    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev->mic_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_playback_mutex);
        write_count = uac_alsa_pcm_write(uac->uac_dev->alsa_playback_handle, data, count, timeout_ms);
        pthread_mutex_unlock(&uac->uac_dev->alsa_playback_mutex);
        return write_count;
    }
    else
        return -1;
}

int uac_read_speak_stream_v2(struct uac *uac, void *data, size_t count)
{
    size_t read_count;
    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev->speak_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_capture_mutex);
        read_count = uac_alsa_pcm_read(uac->uac_dev->alsa_capture_handle, data, count, 0);
        pthread_mutex_unlock(&uac->uac_dev->alsa_capture_mutex);
        return read_count;
    }
    else
        return -1;
}

int uac_read_speak_stream_timeout_v2(struct uac *uac, void *data, size_t count, int timeout_ms)
{
    size_t read_count;
    if(uac == NULL)
    {
        ucam_error("uac == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev == NULL)
    {
        ucam_error("uac->uac_dev == NULL\n");
        return -ENOMEM;
    }

    if(uac->uac_dev->speak_attrs.stream_on_status && uac->usb_suspend == 0)
    {
        pthread_mutex_lock(&uac->uac_dev->alsa_capture_mutex);
        read_count = uac_alsa_pcm_read(uac->uac_dev->alsa_capture_handle, data, count, timeout_ms);
        pthread_mutex_unlock(&uac->uac_dev->alsa_capture_mutex);
        return read_count;
    }
    else
        return -1;
}

int uac_register_stream_event_callback(struct uac *uac, uac_stream_event_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uac->uac_stream_event_callback = callback;

    return 0;
}

int uac_register_mute_event_callback(struct uac *uac, uac_mute_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uac->uac_mute_callback = callback;

    return 0;
}

int uac_register_volume_event_callback(struct uac *uac, uac_volume_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }

    uac->uac_volume_callback = callback;

    return 0;
}

int uac_register_usb_connet_event_callback(struct uac *uac, uac_usb_connet_event_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }

    uac->uac_usb_connet_event_callback = callback;

    return 0;
}

int uac_register_usb_suspend_event_callback(struct uac *uac, uac_usb_suspend_event_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uac->usb_suspend_event_callback = callback;

    return 0;
}

int uac_register_usb_resume_event_callback(struct uac *uac, uac_usb_resume_event_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uac->usb_resume_event_callback = callback;

    return 0;
}

int uac_register_set_rate_event_callback(struct uac *uac, uac_set_rate_callback_t callback)
{
    if(uac == NULL || callback == NULL)
    {
        return -ENOMEM;
    }
    uac->uac_set_rate_callback = callback;

    return 0;
}

int uac_usb_connet (struct uac *uac)
{
	int ret;
	int type;

	if(uac == NULL && uac->uac_dev == NULL)
	{
		return -ENOMEM;
	}

	ucam_notice("connec usb\n");
	//重新连接USB
	type = USB_STATE_CONNECT;	
	if ((ret = ioctl(uac->uac_dev->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
		ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
		strerror(errno), errno);	
		return -1;
	}
	return 0;
}

int uac_usb_reconnet (struct uac *uac)
{
	int ret;
	int type;

	if(uac == NULL && uac->uac_dev == NULL)
	{
		return -ENOMEM;
	}

	ucam_notice("reconnec usb\n");
	//重新连接USB
	type = USB_STATE_CONNECT_RESET;	
	if ((ret = ioctl(uac->uac_dev->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
		ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
		strerror(errno), errno);	
		return -1;
	}
	return 0;
}

int uac_usb_disconnet (struct uac *uac)
{
	int ret;
	int type;

	if(uac == NULL && uac->uac_dev == NULL)
	{
		return -ENOMEM;
	}

	ucam_notice("connec usb\n");
	//重新连接USB
	type = USB_STATE_DISCONNECT;	
	if ((ret = ioctl(uac->uac_dev->fd, UVCIOC_USB_CONNECT_STATE, &type)) < 0){
		ucam_error("Unable to set USB_STATE_CONNECT_RESET: %s (%d).",
		strerror(errno), errno);	
		return -1;
	}
	return 0;
}

struct uac * create_uac(struct ucam *ucam)
{
    struct uac *uac = NULL;

 

    uac = malloc(sizeof(*uac));
	if (uac == NULL) {
        ucam_error("Malloc Error!");
		return NULL;
	}

    memset(uac, 0, sizeof (*uac));

    UCAM_INIT_LIST_HEAD(&uac->uac_dev_list);
    uac->bConfigurationValue = 1;
   
    uac->ucam = ucam;

    return uac;
}

int uac_free(struct uac *uac)
{
    struct uac_dev *uac_dev;
    struct ucam_list_head *pos, *npos;
    
    if(uac == NULL)
        return -ENOMEM;

    ucam_list_for_each_safe(pos, npos,  &uac->uac_dev_list)
    {
        uac_dev = ucam_list_entry(pos, struct uac_dev, list);
        if(uac_dev)
        {
            if(uac_dev->uac_event_thread_pid)
            {
                ucam_list_del(&uac_dev->list);
               
                if(uac_dev->uac_mic_enable)
                {
                    pthread_mutex_lock(&uac->uac_dev->alsa_playback_mutex);
                    uac_alsa_playback_close(uac_dev->alsa_playback_handle);
                    uac_dev->alsa_playback_handle = NULL;
                    pthread_mutex_unlock(&uac->uac_dev->alsa_playback_mutex);
                }
                
                if(uac_dev->uac_speak_enable)
                {
                    pthread_mutex_lock(&uac->uac_dev->alsa_capture_mutex);
                    uac_alsa_capture_close(uac_dev->alsa_capture_handle);
                    uac_dev->alsa_capture_handle = NULL;
                    pthread_mutex_unlock(&uac->uac_dev->alsa_capture_mutex);
                }
                
                if (!uac_dev->uac_event_thread_exit && 
                    uac_dev->uac_event_thread_pid)
                {
                    uac_dev->uac_event_thread_exit = 1;
                    pthread_join(uac_dev->uac_event_thread_pid, NULL);
                    uac_dev->uac_event_thread_pid = 0;
                }

                close_uac_dev(uac_dev); 
            }
        }
    }

    uac->uac_dev = NULL;

    if(uac)
    {
        free(uac);
        uac = NULL;
    }
   
    return 0;
}


void uac_audio_buf_16bit_1ch_to_2ch_c(const uint8_t* src1, const uint8_t *src2, uint8_t* dst, int size) 
{
    int i;

    for(i=0; i < size - 1; i+=2)
    {
        ((int16_t *)dst)[0] =((int16_t *)(src1))[0];
        ((int16_t *)dst)[1] =((int16_t *)(src2))[0];

        src1 += 2;
        src2 += 2;
        dst += 4;
    }

    if(size & 1)
    {
        ((int16_t *)dst)[0] =((int16_t *)(src1))[0];
    }
}

#if defined __x86_64__
void uac_audio_buf_16bit_1ch_to_2ch_neon_16(const uint8_t* src1, const uint8_t *src2, uint8_t* dst, int size)
{
    uac_audio_buf_16bit_1ch_to_2ch_c(src1, src2, dst, size);  
}
#elif defined(__aarch64__)
void uac_audio_buf_16bit_1ch_to_2ch_neon_16(const uint8_t* src1, const uint8_t *src2, uint8_t* dst, int size) 
{
    //R--16bit
    //L--16bit
    //RL--32bit
    asm volatile (
	"1:                                     \n"
	"ld2        {v0.16b, v1.16b}, [%0], #32	\n"  // load 16 R
	"ld2        {v2.16b, v3.16b}, [%1], #32	\n"  // load 16 L
    "orr        v4.16b, v1.16b, v1.16b     	  \n"
    "orr        v1.16b, v2.16b, v2.16b     	  \n"
    "orr        v2.16b, v4.16b, v4.16b     	  \n"
    "subs        %w3, %w3, #16                \n"  // 16bit size = size - 16
	"st4        {v0.16b, v1.16b, v2.16b, v3.16b}, [%2], #64	\n"  // load 32bit RL
	"b.gt        1b                          \n"
	:   "+r"(src1), // %0
	    "+r"(src2), // %1
	    "+r"(dst),  // %2
	    "+r"(size)  // %3
	:
	: "cc", "memory", "v0", "v1", "v2", "v3"
    ); 

}
#else
void uac_audio_buf_16bit_1ch_to_2ch_neon_16(const uint8_t* src1, const uint8_t *src2, uint8_t* dst, int size) 
{
    //R--16bit
    //L--16bit
    //RL--32bit
    asm volatile (
	"1:                                     \n"
	"vld2.16     {d0, d2}, [%0]!			\n"  // load 16 R
	"vld2.16     {d1, d3}, [%1]!			\n"  // load 16 L
	"subs        %3, %3, #16                \n"  // 16bit size = size - 16
	"vst4.16     {d0, d1, d2, d3}, [%2]!	\n"  // load 32bit RL
	"bgt        1b                          \n"
	:   "+r"(src1), // %0
	    "+r"(src2), // %1
	    "+r"(dst),  // %2
	    "+r"(size)  // %3
	:
	: "cc", "memory", "d0", "d1", "d2", "d3"
    ); 

}
#endif

void uac_audio_buf_16bit_1ch_to_2ch_neon(const uint8_t* src1, const uint8_t *src2, uint8_t* dst, int size) 
{
    //R--16bit
    //L--16bit
    //RL--32bit

    int r = size & 0xF;                                                    
    int n = size & ~0xF;

	if(n > 0)
	{
		uac_audio_buf_16bit_1ch_to_2ch_neon_16(src1, src2, dst, n);
	}

	if(r > 0)
	{
		uac_audio_buf_16bit_1ch_to_2ch_c(src1 + n, src2 + n, dst + (n << 1), r);
	}

}



