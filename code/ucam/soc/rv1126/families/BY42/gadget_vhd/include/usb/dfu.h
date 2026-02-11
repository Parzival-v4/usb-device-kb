/*
 * dfu.h
 *
 *  Created on: 2017-10-17
 *      Author: qin
 */

#ifndef __LINUX_USB_DFU_H
#define __LINUX_USB_DFU_H

#include <usb_types.h>

//DFU handle

#define DFU_CONFIG_VAL			1
#define DFU_DT_FUNC			0x21

#define DFU_BIT_WILL_DETACH		(0x1 << 3)
#define DFU_BIT_MANIFESTATION_TOLERANT	(0x1 << 2)
#define DFU_BIT_CAN_UPLOAD		(0x1 << 1)
#define DFU_BIT_CAN_DNLOAD		0x1

/* big enough to hold our biggest descriptor */
#define DFU_USB_BUFSIZ			4096

#define USB_REQ_DFU_DETACH		0x00
#define USB_REQ_DFU_DNLOAD		0x01
#define USB_REQ_DFU_UPLOAD		0x02
#define USB_REQ_DFU_GETSTATUS		0x03
#define USB_REQ_DFU_CLRSTATUS		0x04
#define USB_REQ_DFU_GETSTATE		0x05
#define USB_REQ_DFU_ABORT		0x06

#define DFU_STATUS_OK			0x00
#define DFU_STATUS_errTARGET		0x01
#define DFU_STATUS_errFILE		0x02
#define DFU_STATUS_errWRITE		0x03
#define DFU_STATUS_errERASE		0x04
#define DFU_STATUS_errCHECK_ERASED	0x05
#define DFU_STATUS_errPROG		0x06
#define DFU_STATUS_errVERIFY		0x07
#define DFU_STATUS_errADDRESS		0x08
#define DFU_STATUS_errNOTDONE		0x09
#define DFU_STATUS_errFIRMWARE		0x0a
#define DFU_STATUS_errVENDOR		0x0b
#define DFU_STATUS_errUSBR		0x0c
#define DFU_STATUS_errPOR		0x0d
#define DFU_STATUS_errUNKNOWN		0x0e
#define DFU_STATUS_errSTALLEDPKT	0x0f

#define RET_STALL			-1
#define RET_ZLP				0
#define RET_STAT_LEN			6

enum dfu_state {
	DFU_STATE_appIDLE		= 0,
	DFU_STATE_appDETACH		= 1,
	DFU_STATE_dfuIDLE		= 2,
	DFU_STATE_dfuDNLOAD_SYNC	= 3,
	DFU_STATE_dfuDNBUSY		= 4,
	DFU_STATE_dfuDNLOAD_IDLE	= 5,
	DFU_STATE_dfuMANIFEST_SYNC	= 6,
	DFU_STATE_dfuMANIFEST		= 7,
	DFU_STATE_dfuMANIFEST_WAIT_RST	= 8,
	DFU_STATE_dfuUPLOAD_IDLE	= 9,
	DFU_STATE_dfuERROR		= 10,
};

struct dfu_status {
	__u8				bStatus;
	__u8				bwPollTimeout[3];
	__u8				bState;
	__u8				iString;
} __attribute__((__packed__));

/**************************************************/
/* Special Commands  with Download Request        */
/**************************************************/
#define DFU_CMD_GETCOMMANDS            0x00
#define DFU_CMD_SETADDRESSPOINTER      0x21
#define DFU_CMD_ERASE                  0x41

#define DFU_MEDIA_ERASE                0x00
#define DFU_MEDIA_PROGRAM              0x01

#define FLASH_ERASE_TIME    (uint16_t)0
#define FLASH_PROGRAM_TIME  (uint16_t)0

/**************************************************/
/* Other defines                                  */
/**************************************************/
/* Bit Detach capable = bit 3 in bmAttributes field */
#define DFU_DETACH_MASK                (uint8_t)(1 << 4)

#define DFU_POLL_TIMEOUT_MASK           (0xFFFFFFUL)


#define FLASH_DESC_STR      "Polycom EagleEye Cube DFU Mode Interface"//"@DFU/0x00000000/131072*0002Kg"


struct dfu_functional_descriptor{
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bmAttributes;
	__u16 wDetachTimeOut;
	__u16 wTransferSize;
	__u16 bcdDFUVersion;
} __attribute__((__packed__));

#define USB_DFU_FUNC_DESC_SIZE	9


#endif /* __LINUX_USB_DFU_H */
