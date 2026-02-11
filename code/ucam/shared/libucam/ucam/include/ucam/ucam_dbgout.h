/*
 * @Copyright   : Copyright (C) ValueHD Technologies Co., Ltd. 2014-2020. All rights reserved.
 * 
 * @Author      : QinHaiCheng
 * 
 * @Date        : 2020-07-18 15:59:19
 * @FilePath    : \lib_ucam\ucam\include\ucam\ucam_dbgout.h
 * @Description : 
 */ 
#ifndef UCAM_DBGOUT_H 
#define UCAM_DBGOUT_H

#define UCAM_DBG_ERR   0
#define UCAM_DBG_INFO  7

#define UCAM_DBG_OUT_INFO  7
#define UCAM_DBG_OUT_LEVEL 9

#define UCAM_DBG_LEVEL 9		//all message
//#define UCAM_DBG_LEVEL 6		//error message

#define UCAM_SYSLOG_BUF_MAX  256

#if !(defined CHIP_RV1126)
#define UCAM_DEBUG_OUT
#endif

#include <stdio.h>
#include <syslog.h>

#ifndef dbg_out
#define dbg_out(level, format, args...) \
	{ \
	if(level<=UCAM_DBG_LEVEL){ \
		if(level==UCAM_DBG_ERR){ \
			fprintf(stderr,#level":%s %s[%04d] " format, __FILE__, __FUNCTION__, __LINE__, ##args); \
			fflush(stderr); \
		}\
  		else{ \
  			fprintf(stdout,#level":%s %s[%04d] " format, __FILE__, __FUNCTION__, __LINE__, ##args); \
			fflush(stdout); \
		}\
	} \
	}

#endif

#ifdef UCAM_DEBUG_OUT

void set_vhd_ucam_log_level(int level);

/*
syslog level:
   
level 		值 	说明
LOG_EMERG 	0 	紧急（系统不可用）（最高优先级）
LOG_ALERT 	1 	必须立即采取行动
LOG_CRIT 	2 	严重情况（如硬件设备出错）
LOG_ERR 	3 	出错情况
LOG_WARNING 4 	警告情况
LOG_NOTICE 	5 	正常但重要的情况（默认值）
LOG_INFO 	6 	通告信息
LOG_DEBUG 	7 	调试信息（最低优先级）
*/
int  ucam_log(int level, const char* fmt, ...);
int  ucam_log_debug( const char* fmt, ...);
int  ucam_log_info( const char* fmt, ...);
int  ucam_log_notice( const char* fmt, ...);
int  ucam_log_warning( const char* fmt, ...);
int  ucam_log_error( const char* fmt, ...);


typedef int(*ucam_log_fn_t) (int level, const char *fmt, ...);
void register_ucam_log (ucam_log_fn_t fn);

#endif /* #ifdef UCAM_DEBUG_OUT */

#if (defined CHIP_RV1126)
/*
 * Android log priority values, in ascending priority order.
 */
typedef enum android_LogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} android_LogPriority;

extern int __android_log_print(int prio, const char *tag,  const char *fmt, ...);

#ifndef UCAM_ALOG_LEVEL
#define UCAM_ALOG_LEVEL	ANDROID_LOG_DEBUG
#endif

#ifndef UCAM_ALOG_TAG
#define UCAM_ALOG_TAG	"UCAM"
#endif

#define ucam_debug(format, args...) { __android_log_print(UCAM_ALOG_LEVEL, UCAM_ALOG_TAG, "%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_info(format, args...) { __android_log_print(ANDROID_LOG_INFO,    UCAM_ALOG_TAG, "%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_notice(format, args...) { __android_log_print(ANDROID_LOG_INFO,    UCAM_ALOG_TAG, "%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_warning(format, args...) { __android_log_print(ANDROID_LOG_WARN,    UCAM_ALOG_TAG, "%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_error(format, args...) { __android_log_print(ANDROID_LOG_ERROR,   UCAM_ALOG_TAG, "%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#else


#ifdef UCAM_DEBUG_OUT
#define ucam_debug(format, args...) { ucam_log_debug("%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_info(format, args...) { ucam_log_info("%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_notice(format, args...) { ucam_log_notice("%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_warning(format, args...) { ucam_log_warning("%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#define ucam_error(format, args...) { ucam_log_error("%-4d %s " format "\n", __LINE__,__FUNCTION__, ##args); }
#else
#define ucam_debug(format, args...) {}
#define ucam_info(format, args...) {}
#define ucam_notice(format, args...) {}
#define ucam_warning(format, args...) {}
#define ucam_error(format, args...) {}
#endif

#endif /* #if (defined CHIP_RV1126) */

#define ucam_trace ucam_debug
#define ucam_strace ucam_debug

#endif /* UCAM_DBGOUT_H */
