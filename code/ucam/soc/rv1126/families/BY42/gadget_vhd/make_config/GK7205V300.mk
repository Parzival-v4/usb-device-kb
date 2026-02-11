CROSS_COMPILE := /data_ext/qhc/GK_SDK020/tools/toolchains/arm-gcc6.3-linux-uclibceabi/bin/arm-gcc6.3-linux-uclibceabi-
TARGET_ARCH := arm
TARGET_CROSS := $(CROSS_COMPILE)
#LINUX_CODE_PATH := /data/$(shell whoami)/work_space/kernel/linux-4.9.y_hi3516ev300_sdk010
LINUX_CODE_PATH := /data_ext/qhc/GK_SDK020/source/kernel/linux-4.9.y
LINUX_OBJ_PATH := /data_ext/qhc/GK_SDK020/out/gk7205v300/linux-4.9.y
TARGET_EXTRA_CFLAGS +=-DUSB_CHIP_DWC3

export LINUX_CODE_PATH


EXTRA_CFLAGS 			+=$(TARGET_EXTRA_CFLAGS)


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
#CONFIG_USB_G_DFU=m
CONFIG_USB_G_WEBCAM=m
CONFIG_USB_VBUS_GPIO=m
CONFIG_USB_DWC3_GOKE=m
export CONFIG_USB_F_UVC
export CONFIG_USB_F_UVC_S2
export CONFIG_USB_F_UAC1
export CONFIG_USB_F_UAC1_SPEAK
#export CONFIG_USB_G_HID
#export CONFIG_USB_G_DFU
export CONFIG_USB_G_WEBCAM
export CONFIG_USB_VBUS_GPIO
export CONFIG_USB_DWC3_GOKE

ccflags-y			+= -I$(PWD)/include
ccflags-y			+= -I$(LINUX_CODE_PATH)/include/linux
ccflags-y			+= -I$(PWD)/udc


obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o u_f.o

obj-$(CONFIG_USB_GADGET)		+= udc/ function/ legacy/
obj-$(CONFIG_USB_DWC3)			+= udc/dwc3_hisi/
obj-$(CONFIG_PHY_GOKE_USBP2) 	+= phy/phy_gk7205v300/
obj-$(CONFIG_VIDEOBUF2_CORE)	+= v4l2-core/v4l2-core_linux-4.9/
obj-$(CONFIG_USB_COMMON)	  	+= udc/common/



default:
	@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget.h $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h
	@cp $(PWD)/include/usb/gadget_vhd.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h	
	@$(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h
	@cp $(PWD)/*.ko  $(KO_OUT_DIR)
	@cp $(PWD)/legacy/*.ko	$(KO_OUT_DIR)
	@cp $(PWD)/function/*.ko  $(KO_OUT_DIR)	
	@cp $(PWD)/v4l2-core/v4l2-core_linux-4.9/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/dwc3_hisi/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/phy/phy_gk7205v300/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/common/*.ko $(KO_OUT_DIR)
	$(TARGET_STRIP) $(KO_OUT_DIR)/*.ko --strip-unneeded

	
clean:
	@make -C $(LINUX_OBJ_PATH) M=$(PWD) clean 
	@rm -f *.bak *.o 
	@rm -f *.bak *.o 
	@rm -f *.o modules.* *.symvers *.mod.c *.o.cmd
