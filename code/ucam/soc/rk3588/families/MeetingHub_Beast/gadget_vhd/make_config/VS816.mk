CROSS_COMPILE := /opt/vs-linux/x86-arm/gcc-linaro-7.5.0-aarch64-linux-gnu/bin/aarch64-linux-gnu-
TARGET_ARCH := arm64
TARGET_CROSS := $(CROSS_COMPILE)
LINUX_CODE_PATH := /data/qinhaicheng/work_space/vs_sdk/VS816_PostCS_Release/VS816_release/customer-rel/board/package/opensource/kernel/linux
LINUX_OBJ_PATH := $(LINUX_CODE_PATH)
TARGET_EXTRA_CFLAGS +=-DUSB_MULTI_SUPPORT -Wno-implicit-fallthrough -DUSB_CHIP_DWC3

#TARGET_EXTRA_CFLAGS +=-fstack-protector-all

export LINUX_CODE_PATH

TARGET_STRIP=$(TARGET_CROSS)strip
KO_OUT_DIR=./ko


#CONFIG_USB_GADGET_VBUS_DRAW=500
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
#export CONFIG_USB_GADGET_VBUS_DRAW
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

#TARGET_EXTRA_CFLAGS +=-DCONFIG_USB_GADGET_VBUS_DRAW=500
TARGET_EXTRA_CFLAGS +=-DCONFIG_USB_GADGET_STORAGE_NUM_BUFFERS=2
TARGET_EXTRA_CFLAGS +=-DCONFIG_USB_GADGET

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
obj-$(CONFIG_USB_DWC3)				+= udc/dwc3_hisi/
obj-$(CONFIG_USB_DWC3_OF_SIMPLE)	+= udc/dwc3_vs816/
obj-$(CONFIG_USB_COMMON)	  		+= udc/common/

default:
	@$(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	@cp $(PWD)/*.ko  $(KO_OUT_DIR)
	@cp $(PWD)/legacy/*.ko	$(KO_OUT_DIR)
	@cp $(PWD)/function/*.ko  $(KO_OUT_DIR)	
	@cp $(PWD)/udc/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/common/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/dwc3_hisi/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/dwc3_vs816/*.ko $(KO_OUT_DIR)
	$(TARGET_STRIP) $(KO_OUT_DIR)/*.ko --strip-unneeded
	#@cp $(KO_OUT_DIR)/*.ko ~/nfs/ucam_test/usb_ko/


clean:
	@make -C $(LINUX_OBJ_PATH) M=$(PWD) clean 
	@rm -f *.bak *.o 
	@rm -f *.bak *.o 
	@rm -f *.o modules.* *.symvers *.mod.c *.o.cmd
