CROSS_COMPILE := /usr/local/cortex-a76-2023.04-gcc12.2-linux5.15/bin/aarch64-linux-gnu-
TARGET_ARCH := arm64
TARGET_CROSS := $(CROSS_COMPILE)
LINUX_CODE_PATH := /data/qhc/work_space/cv5_trunk_git/cooper_linux_sdk_1.5_cv5x/ambarella/kernel/linux-5.15
LINUX_OBJ_PATH := /data/qhc/work_space/cv5_trunk_git/cooper_linux_sdk_1.5_cv5x/ambarella/out/amba_out/cv5_timn/objects/kernel/linux-5.15
TARGET_EXTRA_CFLAGS +=-DUSB_MULTI_SUPPORT -Wno-implicit-fallthrough -DUSB_SSP_SUPPORT

export LINUX_CODE_PATH

TARGET_STRIP=$(TARGET_CROSS)strip
KO_OUT_DIR=./ko


CONFIG_USB_GADGET_VBUS_DRAW=500
CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS=2

CONFIG_USB_GADGET=m
CONFIG_USB_COMMON=m
CONFIG_USB_LIBCOMPOSITE=m
#CONFIG_VIDEOBUF2_CORE=m

CONFIG_USB_F_UVC=m
CONFIG_USB_F_UVC_S2=m
CONFIG_USB_F_UAC1=m
CONFIG_USB_F_UAC1_SPEAK=m
#CONFIG_USB_G_HID=m
CONFIG_USB_G_DFU=m
CONFIG_USB_G_WEBCAM=m
CONFIG_USB_VBUS_GPIO=m
export CONFIG_USB_GADGET_VBUS_DRAW
export CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS
export CONFIG_USB_COMMON
export CONFIG_USB_GADGET
export CONFIG_USB_LIBCOMPOSITE
export CONFIG_USB_F_UVC
export CONFIG_USB_F_UVC_S2
export CONFIG_USB_F_UAC1
export CONFIG_USB_F_UAC1_SPEAK
#export CONFIG_USB_G_HID
export CONFIG_USB_G_DFU
export CONFIG_USB_G_WEBCAM
export CONFIG_USB_VBUS_GPIO

CONFIG_USB_F_MASS_STORAGE=m
export CONFIG_USB_F_MASS_STORAGE

TARGET_EXTRA_CFLAGS +=-DCONFIG_USB_GADGET_VBUS_DRAW=500
TARGET_EXTRA_CFLAGS +=-DCONFIG_USB_GADGET_STORAGE_NUM_BUFFERS=2
TARGET_EXTRA_CFLAGS +=-DCONFIG_USB_GADGET

#########################CDNS3#############################
CONFIG_USB_CDNS3=m
export CONFIG_USB_CDNS3

CONFIG_USB_CDNSP_GADGET=y
export CONFIG_USB_CDNSP_GADGET

CONFIG_USB_CDNS_HOST=y
export CONFIG_USB_CDNS_HOST

CONFIG_USB_ROLE_SWITCH=m
export CONFIG_USB_ROLE_SWITCH

CONFIG_USB_CDNS3_AMBARELLA=m
export CONFIG_USB_CDNS3_AMBARELLA
#########################RNDIS#############################
TARGET_EXTRA_CFLAGS +=-DUSB_MULTI_SUPPORT
CONFIG_USB_F_RNDIS=m
CONFIG_USB_U_ETHER=m
export CONFIG_USB_F_RNDIS
export CONFIG_USB_U_ETHER
##########################################################

ccflags-y			+= -I$(PWD)/include
ccflags-y			+= -I$(LINUX_CODE_PATH)/include/linux
ccflags-y			+= -I$(PWD)/udc
EXTRA_CFLAGS 		+=$(TARGET_EXTRA_CFLAGS)
#export EXTRA_CFLAGS


obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o u_f.o configfs_linux-5.4.o

obj-$(CONFIG_USB_GADGET)			+= udc/ function/ legacy/
obj-$(CONFIG_USB_CDNS3)				+= udc/cdns3_cv5/
obj-$(CONFIG_USB_ROLE_SWITCH)		+= udc/cdns3_cv5/roles/
obj-$(CONFIG_USB_COMMON)	  		+= udc/common/

default:
	#@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget.h $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h
	#@cp $(PWD)/include/usb/gadget_cv5.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h	
	@$(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	#@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h
	@cp $(PWD)/*.ko  $(KO_OUT_DIR)
	@cp $(PWD)/legacy/*.ko	$(KO_OUT_DIR)
	@cp $(PWD)/function/*.ko  $(KO_OUT_DIR)	
	@cp $(PWD)/udc/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/common/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/cdns3_cv5/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/cdns3_cv5/roles/*.ko $(KO_OUT_DIR)
	$(TARGET_STRIP) $(KO_OUT_DIR)/*.ko --strip-unneeded
	@cp $(KO_OUT_DIR)/*.ko /data/qhc/nfs/ucam_test/usb_ko/


clean:
	@make -C $(LINUX_OBJ_PATH) M=$(PWD) clean 
	@rm -f *.bak *.o 
	@rm -f *.bak *.o 
	@rm -f *.o modules.* *.symvers *.mod.c *.o.cmd
