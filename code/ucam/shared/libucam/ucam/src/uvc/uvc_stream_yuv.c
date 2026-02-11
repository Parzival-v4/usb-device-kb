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
#include <stdbool.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <signal.h>

#include <linux/usb/ch9.h>
//#include <ucam/videodev2.h>

#include <ucam/uvc/video.h>
#include <ucam/uvc/uvc_device.h>
#include <ucam/uvc/uvc_ctrl_vs.h>
#include <ucam/uvc/uvc_api.h>
#include <ucam/uvc/uvc_api_stream.h>
#include <ucam/uvc/uvc_api_config.h>
#include <ucam/uvc/uvc_api_eu.h>
#include "uvc_stream.h"

// This module is for GCC Neon armv8 64 bit.
#if defined(__aarch64__)
void UVC_UVSwapRow_NEON_16(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
    asm volatile (
	"1:                             		\n"
	"ld2    {v0.16b, v1.16b}, [%0], #32		\n"  // load 16 vu
	"orr    v2.16b, v0.16b, v0.16b      	\n" 
	"subs   %w2, %w2, #16         			\n"  
	"st2    {v1.16b, v2.16b}, [%1], #32	    \n"  // load 16 uv
	"b.gt        1b                  		\n"
	:   "+r"(src_uv),           // %0
	    "+r"(dst_vu),           // %1
	    "+r"(width)     		// %2
	:
	: "cc", "memory", "v0", "v1", "v2"
    ); 
}


void UVC_UVSplitRow_NEON_16(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width) {
  
  asm volatile(
      "1:                                       \n"
      "ld2         {v0.8b,v1.8b}, [%0], #16    	\n"  // load 16 pairs of UV
      "subs        %w3, %w3, #16                \n"  // 16 processed per loop
      "st1         {v0.8b}, [%1], #8           	\n"  // store U
      "st1         {v1.8b}, [%2], #8           	\n"  // store V
      "b.gt        1b                           \n"
      : "+r"(src_uv),               // %0
        "+r"(dst_u),                // %1
        "+r"(dst_v),                // %2
        "+r"(width)                 // %3  // Output registers
      :                             // Input registers
      : "cc", "memory", "v0", "v1"  // Clobber List
  );
}

void UVC_NV21ToYUY2Row_NEON_16(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) {
	//YUVSP420
	//Y0Y1 
	//V0U0 
	//YUY2
	//Y0U0Y1V0 
		
	asm volatile(
      "1:                                        	\n"
      "ld2     {v0.8b,v1.8b}, [%0], #16            	\n"  // load 16 YY
	  "ld2     {v3.8b,v4.8b}, [%1], #16             \n"  // load 16 VU
	  "orr     v2.8b, v1.8b, v1.8b     				\n"	
	  "orr     v1.8b, v4.8b, v4.8b     				\n"
      "subs    %w3, %w3, #16                    	\n"  // 16 pixels
      "st4     {v0.8b,v1.8b,v2.8b,v3.8b}, [%2], #32	\n"  // Store 8 YUY2/16 pixels.	
      "b.gt        1b                             	\n"
      : "+r"(src_y),     	// %0
	  	"+r"(src_vu),    	// %1
        "+r"(dst_yuy2),  	// %2
        "+r"(width)      	// %3
      :
      : "cc", "memory", "v0", "v1", "v2", "v3", "v4");
}

void UVC_NV12ToYUY2Row_NEON_16(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width) {
	
	asm volatile(
      "1:                                        	\n"
      "ld2     {v0.8b,v1.8b}, [%0], #16            	\n"  // load 16 YY
	  "ld2     {v2.8b,v3.8b}, [%1], #16             \n"  // load 16 UV
      "orr     v4.8b, v1.8b, v1.8b     				\n"  //Y
      "orr     v1.8b, v2.8b, v2.8b     				\n"  //U->Y
	  "orr     v2.8b, v4.8b, v4.8b     				\n"	 //Y->U
      "subs    %w3, %w3, #16                    	\n"  // 16 pixels
      "st4     {v0.8b,v1.8b,v2.8b,v3.8b}, [%2], #32	\n"  // Store 8 YUY2/16 pixels.	YUYV
      "b.gt        1b                             	\n"
      : "+r"(src_y),     	// %0
	  	"+r"(src_uv),    	// %1
        "+r"(dst_yuy2),  	// %2
        "+r"(width)      	// %3
      :
      : "cc", "memory", "v0", "v1", "v2", "v3", "v4");
}
#elif defined __x86_64__

#define UVC_UVSwapRow_NEON_16 		UVC_UVSwapRow_C
#define UVC_UVSplitRow_NEON_16 		UVC_UVSplitRow_C
#define UVC_NV21ToYUY2Row_NEON_16 	UVC_NV21ToYUY2Row_C
#define UVC_NV12ToYUY2Row_NEON_16 	UVC_NV12ToYUY2Row_C

#else

void UVC_UVSwapRow_NEON_16(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
    asm volatile (
	"1:                             \n"
	"vld2.8    {d0, d1}, [%0]!		\n"  // load 16 vu
	"vswp.u8    d0, d1              \n"
	"subs       %2, %2, #16         \n"  
	"vst2.8    {d0, d1}, [%1]!	    \n"  // load 16 uv
	"bgt        1b                  \n"
	:   "+r"(src_uv),           // %0
	    "+r"(dst_vu),           // %1
	    "+r"(width)     		// %2
	:
	: "cc", "memory", "d0", "d1"
    ); 
}

void UVC_UVSplitRow_NEON_16(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width) {
    asm volatile (
	"1:                             \n"
	"vld2.8    {d0, d1}, [%0]!		\n"  // load 16 vu
	"subs       %3, %3, #16         \n"  
	"vst1.8    {d0}, [%1]!	    	\n"  // load u
	"vst1.8    {d1}, [%2]!	    	\n"  // load v
	"bgt        1b                  \n"
	:   "+r"(src_uv),           // %0
	    "+r"(dst_u),            // %1
		"+r"(dst_v),            // %2
	    "+r"(width)     		// %3
	:
	: "cc", "memory", "d0", "d1"
    ); 
}

void UVC_NV21ToYUY2Row_NEON_16(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) {
	//YUVSP420
	//Y0Y1 
	//V0U0 
	//YUY2
	//Y0U0Y1V0 
		
	asm volatile(
      "1:                                        \n"
      "vld2.8     {d0, d2}, [%0]!                \n"  // load 16 YY
      "vld2.8     {d1, d3}, [%1]!                \n"  // load 16 VU
	  "vswp.u8    d1, d3                         \n"
      "subs       %3, %3, #16                    \n"  // 16 pixels
      "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  // Store 8 YUY2/16 pixels.	
      "bgt        1b                             \n"
      : "+r"(src_y),     	// %0
	  	"+r"(src_vu),    	// %1
        "+r"(dst_yuy2),  	// %2
        "+r"(width)      	// %3
      :
      : "cc", "memory", "d0", "d1", "d2", "d3");
}

void UVC_NV12ToYUY2Row_NEON_16(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width) {
	//YUVSP420
	//Y0Y1 
	//V0U0 
	//YUY2
	//Y0U0Y1V0 
		
	asm volatile(
      "1:                                        \n"
      "vld2.8     {d0, d2}, [%0]!                \n"  // load 16 YY
      "vld2.8     {d1, d3}, [%1]!                \n"  // load 16 UV
	  //"vswp.u8    d1, d3                         \n"
      "subs       %3, %3, #16                    \n"  // 16 pixels
      "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  // Store 8 YUY2/16 pixels.	
      "bgt        1b                             \n"
      : "+r"(src_y),     	// %0
	  	"+r"(src_uv),    	// %1
        "+r"(dst_yuy2),  	// %2
        "+r"(width)      	// %3
      :
      : "cc", "memory", "d0", "d1", "d2", "d3");
}
#endif

static void UVC_UVSwapRow_C(const uint8_t* src_uv, uint8_t* dst_vu, int width) {
	int x;
	for(x = 0; x < width - 1; x+=2)
	{
		dst_vu[0] = src_uv[1];
		dst_vu[1] = src_uv[0];

		dst_vu	+= 2;
		src_uv	+= 2;
	} 
}

void UVC_UVSwapRow_Any_NEON(const uint8_t* src_uv, uint8_t* dst_vu, int width)
{
	int r = width & 0xF;                                                    
    int n = width & ~0xF;

	if(n > 0)
	{
		UVC_UVSwapRow_NEON_16(src_uv, dst_vu, n);
	}

	if(r > 0)
	{
		UVC_UVSwapRow_C(src_uv + n, dst_vu + n, r);
	}
}

int UVC_NV21ToNV12(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_vu,
               int src_stride_vu,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_uv,
               int dst_stride_uv,
               int width,
               int height) 
{
	int y;

	if (!src_y || !src_vu || !dst_y || !dst_uv || width <= 0 || height == 0) {
		return -1;
	}

	void (*UVC_UVSwapRow)(const uint8_t* src_uv, 
            uint8_t* dst_vu, int width) = UVC_UVSwapRow_Any_NEON;
	

	if(IS_ALIGNED(width, 16))
		UVC_UVSwapRow = UVC_UVSwapRow_NEON_16;

    for (y = 0; y < height; y ++) 
	{
		uvc_memcpy_neon(dst_y, src_y, width);
		src_y += src_stride_y;
		dst_y += dst_stride_y;
    }

	for (y = 0; y < height - 1; y += 2) {
		UVC_UVSwapRow(src_vu, dst_uv, width);
		src_vu += src_stride_vu;
        dst_uv += dst_stride_uv;
	}

	return 0;
}

static void UVC_UVSplitRow_C(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width) {
	int x;
	for(x = 0; x < width - 1; x+=2)
	{
		dst_u[0] = src_uv[0];
		dst_v[0] = src_uv[1];

		dst_u	+= 1;
		dst_v	+= 1;
		src_uv	+= 2;
	} 
}

void UVC_UVSplitRow_Any_NEON(const uint8_t* src_uv, uint8_t* dst_u, uint8_t* dst_v, int width)
{
	int r = width & 0xF;                                                    
    int n = width & ~0xF;

	if(n > 0)
	{
		UVC_UVSplitRow_NEON_16(src_uv, dst_u, dst_v, n);
	}

	if(r > 0)
	{
		UVC_UVSplitRow_C(src_uv + n, dst_u + n/2, dst_v + n/2, r);
	}
}

static void UVC_NV21ToYUY2Row_C(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) {
	//YUVSP420
	//Y0Y1 
	//V0U0 
	//YUY2
	//Y0U0Y1V0 
	int x;
	for (x = 0; x < width - 1; x += 2) {
		dst_yuy2[0] = src_y[0];		//y
		dst_yuy2[1] = src_vu[1];	//u
		dst_yuy2[2] = src_y[1];		//y	
		dst_yuy2[3] = src_vu[0];	//v
		
		dst_yuy2 += 4;
		src_y += 2;
		src_vu += 2;
	}
}


void UVC_NV21ToYUY2Row_Any_NEON(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) {
							
	int r = width & 0xF;                                                    
    int n = width & ~0xF;

	//ucam_trace("width:%d, r:%d, n:%d", width, r, n);

	if(n > 0)
	{
		UVC_NV21ToYUY2Row_NEON_16(src_y,                      
							src_vu,
							dst_yuy2,
							n);
	}
	
	if(r > 0)
	{
		UVC_NV21ToYUY2Row_C(src_y + n,                      
					src_vu + n,
					dst_yuy2 + (n << 1),
					r);
	}
	
}

int UVC_NV21ToYUY2(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_vu,
               int src_stride_vu,
               uint8_t* dst_yuy2,
               int dst_stride_yuy2,
               int width,
               int height)
{
	int y;

	if (!src_y || !src_vu || !dst_yuy2 || width <= 0 || height == 0) {
		return -1;
	}

	void (*UVC_NV21ToYUY2Row)(const uint8_t* src_y,                      
						const uint8_t* src_vu,
                        uint8_t* dst_yuy2,
                        int width) = UVC_NV21ToYUY2Row_Any_NEON;
	

	if(IS_ALIGNED(width, 16))
		UVC_NV21ToYUY2Row = UVC_NV21ToYUY2Row_NEON_16;

	for (y = 0; y < height - 1; y += 2) {
		UVC_NV21ToYUY2Row(src_y, src_vu, 
					dst_yuy2, width);
					
		UVC_NV21ToYUY2Row(src_y + src_stride_y, src_vu, 
					dst_yuy2 + dst_stride_yuy2, width);

		src_y += src_stride_y << 1;
		src_vu += src_stride_vu;
		dst_yuy2 += dst_stride_yuy2 << 1;
	}

	return 0; 
}

static void UVC_NV12ToYUY2Row_C(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width) 
{
	int x;
	for (x = 0; x < width - 1; x += 2) {
		dst_yuy2[0] = src_y[0];		//y
		dst_yuy2[1] = src_uv[0];	//u
		dst_yuy2[2] = src_y[1];		//y	
		dst_yuy2[3] = src_uv[1];	//v
		
		dst_yuy2 += 4;
		src_y += 2;
		src_uv += 2;
	}

	if(width & 0x01)
	{
		dst_yuy2[0] = src_y[0];		//y
		dst_yuy2[1] = src_uv[0];	//u
	}
}

void UVC_NV12ToYUY2Row_Any_NEON(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width) {
							
	int r = width & 0xF;                                                    
    int n = width & ~0xF;

	//ucam_trace("width:%d, r:%d, n:%d", width, r, n);

	if(n > 0)
	{
		UVC_NV12ToYUY2Row_NEON_16(src_y,                      
							src_uv,
							dst_yuy2,
							n);
	}
	
	if(r > 0)
	{
		UVC_NV12ToYUY2Row_C(src_y + n,                      
					src_uv + n,
					dst_yuy2 + (n << 1),
					r);
	}
	
}

int UVC_NV12ToYUY2(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_uv,
               int src_stride_uv,
               uint8_t* dst_yuy2,
               int dst_stride_yuy2,
               int width,
               int height)
{
	int y;

	if (!src_y || !src_uv || !dst_yuy2 || width <= 0 || height <= 0) {
		return -1;
	}

	void (*UVC_NV12ToYUY2Row)(const uint8_t* src_y,                      
						const uint8_t* src_uv,
                        uint8_t* dst_yuy2,
                        int width) = UVC_NV12ToYUY2Row_Any_NEON;
	

	if(IS_ALIGNED(width, 16))
		UVC_NV12ToYUY2Row = UVC_NV12ToYUY2Row_NEON_16;

	for (y = 0; y < height - 1; y += 2) {
		UVC_NV12ToYUY2Row(src_y, src_uv, 
					dst_yuy2, width);
					
		UVC_NV12ToYUY2Row(src_y + src_stride_y, src_uv, 
					dst_yuy2 + dst_stride_yuy2, width);

		src_y += src_stride_y << 1;
		src_uv += src_stride_uv;
		dst_yuy2 += dst_stride_yuy2 << 1;
	}

	return 0; 
}