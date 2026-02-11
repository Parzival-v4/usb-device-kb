
CROSS_COMPILE := /data/hx/rk3576/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
TARGET_ARCH :=arm64
TARGET_CROSS :=$(CROSS_COMPILE)
LINUX_CODE_PATH := /data/hx/rk3576/kernel
LINUX_OBJ_PATH := $(LINUX_CODE_PATH)
TARGET_EXTRA_CFLAGS +=-DUSB_CHIP_DWC3

export LINUX_CODE_PATH

TARGET_STRIP=$(TARGET_CROSS)strip
KO_OUT_DIR=./ko


CONFIG_USB_COMMON=m
CONFIG_USB_GADGET_VBUS_DRAW=500
CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS=2

CONFIG_USB_GADGET=m
CONFIG_USB_LIBCOMPOSITE=m
#CONFIG_VIDEOBUF2_CORE=m

CONFIG_USB_F_UVC=m
CONFIG_USB_F_UVC_S2=m
CONFIG_USB_F_UAC1=m
CONFIG_USB_F_UAC1_SPEAK=m
#CONFIG_USB_G_HID=m
#CONFIG_USB_G_DFU=m
CONFIG_USB_G_WEBCAM=m
CONFIG_USB_VBUS_GPIO=m
CONFIG_USB_DWC3_OF_SIMPLE=m
CONFIG_USB_DWC3=m
CONFIG_USB_DWC3_GADGET=y
CONFIG_USB_DEBUG_FS=y
CONFIG_USB_DWC3_HAPS=m
export CONFIG_USB_F_UVC
export CONFIG_USB_F_UVC_S2
export CONFIG_USB_F_UAC1
export CONFIG_USB_F_UAC1_SPEAK
#export CONFIG_USB_G_HID
#export CONFIG_USB_G_DFU
export CONFIG_USB_G_WEBCAM
export CONFIG_USB_VBUS_GPIO
export CONFIG_USB_DWC3_OF_SIMPLE
export CONFIG_USB_DWC3
export CONFIG_USB_DWC3_GADGET
export CONFIG_USB_DEBUG_FS
export CONFIG_USB_COMMON
export CONFIG_USB_DWC3_HAPS

#########################RNDIS#############################
TARGET_EXTRA_CFLAGS +=-DUSB_MULTI_SUPPORT -DDEBUG
CONFIG_USB_F_RNDIS=m
CONFIG_USB_U_ETHER=m
export CONFIG_USB_F_RNDIS
export CONFIG_USB_U_ETHER
##########################################################


ccflags-y			+= -I$(PWD)/include
ccflags-y			+= -I$(LINUX_CODE_PATH)/include/linux
ccflags-y			+= -I$(PWD)/udc
EXTRA_CFLAGS 		+=$(TARGET_EXTRA_CFLAGS)

obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o u_f.o

obj-$(CONFIG_USB_GADGET)		+= udc/ function/ legacy/
obj-$(CONFIG_USB_DWC3)			+= udc/dwc3_rk3588/
obj-$(CONFIG_USB_COMMON)	  		+= udc/common/

default:
	echo $(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	@cp $(PWD)/include/vhd_usb_id.h  udc/dwc3_rk3588
	@$(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	@cp $(PWD)/*.ko  $(KO_OUT_DIR)
	@cp $(PWD)/legacy/*.ko	$(KO_OUT_DIR)
	@cp $(PWD)/function/*.ko  $(KO_OUT_DIR)	
	@cp $(PWD)/udc/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/dwc3_rk3588/*.ko $(KO_OUT_DIR)
	#@cp $(PWD)/udc/common/*.ko $(KO_OUT_DIR)
	$(TARGET_STRIP) $(KO_OUT_DIR)/*.ko --strip-unneeded


clean:
	@make -C $(LINUX_OBJ_PATH) M=$(PWD) clean 
	@rm -f *.bak *.o 
	@rm -f *.bak *.o 
	@rm -f *.o modules.* *.symvers *.mod.c *.o.cmd

