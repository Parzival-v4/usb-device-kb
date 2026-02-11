/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-08 16:44:31
 * @FilePath    : \lib_ucam\ucam\src\ucam_dbgout.c
 * @Description : 
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>

#include <ucam/ucam_dbgout.h>


#ifdef UCAM_DEBUG_OUT
static int g_vhd_ucam_log_level = 8;
static ucam_log_fn_t g_ucam_log_fn = NULL;

static int ucam_format_time(char *buf, int size)
{
	int ret;
	struct timeval tv;

	// clock time
	if (gettimeofday(&tv, NULL) == -1) {
		return 0;
	}

	// to calendar time
	struct tm* tm;
	if ((tm = localtime(&tv.tv_sec)) == NULL) {
		return 0;
	}

	memset(buf, 0, size);
	
	ret = snprintf(buf, size,
			"%d-%02d-%02d %02d:%02d:%02d.%03d ",
			1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec,
			(int)(tv.tv_usec / 1000));
			
	return ret;
}


void set_vhd_ucam_log_level(int level)
{
	g_vhd_ucam_log_level = level;
}

int  ucam_log(int level, const char* fmt, ...)
{
	int ret;
	va_list args;
	char strBuffer[UCAM_SYSLOG_BUF_MAX] = { 0 };
	int time_str_len;

	if(g_vhd_ucam_log_level < level)
	{
		return 0;
	}

	if(g_ucam_log_fn)
	{
		va_start(args, fmt);
		ret = g_ucam_log_fn(level, fmt, args);
		va_end(args);	
		return ret;
	}
	
	time_str_len = ucam_format_time(strBuffer, 32);

    va_start(args, fmt);
	vsnprintf(strBuffer + time_str_len, sizeof(strBuffer) - 1 - time_str_len, fmt, args);
	va_end(args);
	
	syslog (level, (const char *)strBuffer);

    return 0;
}

int  ucam_log_debug( const char* fmt, ...)
{
	int ret;
	va_list args;
	char strBuffer[UCAM_SYSLOG_BUF_MAX] = { 0 };
	int time_str_len;

	if(g_vhd_ucam_log_level < LOG_DEBUG)
	{
		return 0;
	}

	if(g_ucam_log_fn)
	{
		va_start(args, fmt);
		ret = g_ucam_log_fn(LOG_DEBUG, fmt, args);
		va_end(args);	
		return ret;
	}
	
	time_str_len = ucam_format_time(strBuffer, 32);

    va_start(args, fmt);
	vsnprintf(strBuffer + time_str_len, sizeof(strBuffer) - 1 - time_str_len, fmt, args);
	va_end(args);
	
	syslog (LOG_DEBUG, (const char *)strBuffer);

    return 0;	
}

int  ucam_log_info( const char* fmt, ...)
{
	int ret;
	va_list args;
	char strBuffer[UCAM_SYSLOG_BUF_MAX] = { 0 };
	int time_str_len;

	if(g_vhd_ucam_log_level < LOG_INFO)
	{
		return 0;
	}

	if(g_ucam_log_fn)
	{
		va_start(args, fmt);
		ret = g_ucam_log_fn(LOG_INFO, fmt, args);
		va_end(args);	
		return ret;
	}

	time_str_len = ucam_format_time(strBuffer, 32);

    va_start(args, fmt);
	vsnprintf(strBuffer + time_str_len, sizeof(strBuffer) - 1 - time_str_len, fmt, args);
	va_end(args);
	
	syslog (LOG_INFO, (const char *)strBuffer);

    return 0;
}

int  ucam_log_notice( const char* fmt, ...)
{
	int ret;
	va_list args;
	char strBuffer[UCAM_SYSLOG_BUF_MAX] = { 0 };
	int time_str_len;

	if(g_vhd_ucam_log_level < LOG_NOTICE)
	{
		return 0;
	}

	if(g_ucam_log_fn)
	{
		va_start(args, fmt);
		ret = g_ucam_log_fn(LOG_NOTICE, fmt, args);
		va_end(args);	
		return ret;
	}

	time_str_len = ucam_format_time(strBuffer, 32);

    va_start(args, fmt);
	vsnprintf(strBuffer + time_str_len, sizeof(strBuffer) - 1 - time_str_len, fmt, args);
	va_end(args);
	
	syslog (LOG_NOTICE, (const char *)strBuffer);

    return 0;
}

int  ucam_log_warning( const char* fmt, ...)
{
	int ret;
	va_list args;
	char strBuffer[UCAM_SYSLOG_BUF_MAX] = { 0 };
	int time_str_len;

	if(g_vhd_ucam_log_level < LOG_WARNING)
	{
		return 0;
	}

	if(g_ucam_log_fn)
	{
		va_start(args, fmt);
		ret = g_ucam_log_fn(LOG_WARNING, fmt, args);
		va_end(args);	
		return ret;
	}

	time_str_len = ucam_format_time(strBuffer, 32);

    va_start(args, fmt);
	vsnprintf(strBuffer + time_str_len, sizeof(strBuffer) - 1 - time_str_len, fmt, args);
	va_end(args);
	
	syslog (LOG_WARNING, (const char *)strBuffer);	

    return 0;
}

int  ucam_log_error( const char* fmt, ...)
{
	int ret;
	va_list args;
	char strBuffer[UCAM_SYSLOG_BUF_MAX] = { 0 };
	int time_str_len;

	if(g_vhd_ucam_log_level < LOG_ERR)
	{
		return 0;
	}

	if(g_ucam_log_fn)
	{
		va_start(args, fmt);
		ret = g_ucam_log_fn(LOG_ERR, fmt, args);
		va_end(args);	
		return ret;
	}

	time_str_len = ucam_format_time(strBuffer, 32);

    va_start(args, fmt);
	vsnprintf(strBuffer + time_str_len, sizeof(strBuffer) - 1 - time_str_len, fmt, args);
	va_end(args);
	
	syslog (LOG_ERR, (const char *)strBuffer);

    return 0;	
}



void register_ucam_log (ucam_log_fn_t fn)
{
	g_ucam_log_fn = fn;
}

#endif