CROSS_COMPILE := /opt/sigmastar/gcc-sigmastar-9.1.0-2020.07-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-sigmastar-9.1.0-
TARGET_ARCH := arm
TARGET_CROSS := $(CROSS_COMPILE)
#LINUX_CODE_PATH := /data/$(shell whoami)/sigmastar/ssd268g_super_sound/kernel
LINUX_CODE_PATH := /data/$(shell whoami)/work_space/kernel/kernel_ssc268_sdk_20211202
LINUX_OBJ_PATH := $(LINUX_CODE_PATH)
TARGET_EXTRA_CFLAGS +=-DUSB_CHIP_DWC3 -DUSB_MULTI_SUPPORT

export LINUX_CODE_PATH

TARGET_STRIP=$(TARGET_CROSS)strip
KO_OUT_DIR=./ko


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
CONFIG_USB_G_DFU=m
CONFIG_USB_G_WEBCAM=m
CONFIG_USB_VBUS_GPIO=m
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

#########################RNDIS#############################
TARGET_EXTRA_CFLAGS +=-DUSB_MULTI_SUPPORT
CONFIG_USB_F_RNDIS=m
CONFIG_USB_U_ETHER=m
CONFIG_USB_F_ACM=m
CONFIG_USB_U_SERIAL=m
export CONFIG_USB_F_RNDIS
export CONFIG_USB_U_ETHER
export CONFIG_USB_F_ACM
export CONFIG_USB_U_SERIAL
##########################################################

ccflags-y			+= -I$(PWD)/include
ccflags-y			+= -I$(LINUX_CODE_PATH)/include/linux
#ccflags-y			+= -I$(LINUX_CODE_PATH)/drivers/sstar/include
ccflags-y			+= -I$(PWD)/udc
EXTRA_CFLAGS 		+=$(TARGET_EXTRA_CFLAGS)


obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o u_f.o configfs.o

obj-$(CONFIG_USB_GADGET)			+= udc/ function/ legacy/
obj-$(CONFIG_USB_DWC3)				+= udc/dwc3_ssc9381/
obj-$(CONFIG_USB_GADGET_SSTAR_DEVICE)	+= udc/usb20_ssc333/
obj-$(CONFIG_SS_USB3_PHY)  			+= phy/phy_ssc268/
#obj-$(CONFIG_VIDEOBUF2_CORE)		+= v4l2-core/v4l2-core_linux-4.9/
obj-$(CONFIG_VIDEOBUF2_CORE)		+= v4l2-core/v4l2-core_ssc9381/
obj-$(CONFIG_USB_COMMON)	  		+= udc/common/

default:
	@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget.h $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h
	@cp $(PWD)/include/usb/gadget_vhd.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h	
	@$(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h
	@cp $(PWD)/*.ko  $(KO_OUT_DIR)
	@cp $(PWD)/legacy/*.ko	$(KO_OUT_DIR)
	@cp $(PWD)/function/*.ko  $(KO_OUT_DIR)	
	#@cp $(PWD)/v4l2-core/v4l2-core_linux-4.9/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/v4l2-core/v4l2-core_ssc9381/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/phy/phy_ssc268/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/dwc3_ssc9381/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/usb20_ssc333/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/common/*.ko $(KO_OUT_DIR)
	$(TARGET_STRIP) $(KO_OUT_DIR)/*.ko --strip-unneeded


clean:
	@make -C $(LINUX_OBJ_PATH) M=$(PWD) clean 
	@rm -f *.bak *.o 
	@rm -f *.bak *.o 
	@rm -f *.o modules.* *.symvers *.mod.c *.o.cmd
