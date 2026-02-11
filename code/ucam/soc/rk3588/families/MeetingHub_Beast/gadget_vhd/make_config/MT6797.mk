TARGET_ARCH := arm64
TARGET_CROSS := aarch64-linux-android-
LINUX_CODE_PATH := /home/$(shell whoami)/work_space/alps/alps/kernel-3.18
LINUX_OBJ_PATH := /home/$(shell whoami)/work_space/alps/alps/out/target/product/nb6797_6c_m/obj/KERNEL_OBJ
TARGET_EXTRA_CFLAGS +=-DUSB_CHIP_MUSB3

export LINUX_CODE_PATH

TARGET_STRIP=$(TARGET_CROSS)strip
KO_OUT_DIR=./ko



CONFIG_USB_GADGET_VBUS_DRAW=500
CONFIG_USB_GADGET_STORAGE_NUM_BUFFERS=2

CONFIG_USB_GADGET=m
CONFIG_USB_LIBCOMPOSITE=m
#CONFIG_VIDEOBUF2_CORE=m
#CONFIG_PHY_HISI_USB3=m

CONFIG_USB_F_UVC=m
CONFIG_USB_F_UVC_S2=m
CONFIG_USB_F_UAC1=m
CONFIG_USB_F_UAC1_SPEAK=m
#CONFIG_USB_G_HID=m
#CONFIG_USB_G_DFU=m
CONFIG_USB_G_WEBCAM=m
export CONFIG_USB_F_UVC
export CONFIG_USB_F_UVC_S2
export CONFIG_USB_F_UAC1
export CONFIG_USB_F_UAC1_SPEAK
#export CONFIG_USB_G_HID
#export CONFIG_USB_G_DFU
export CONFIG_USB_G_WEBCAM

ccflags-y			+= -I$(PWD)/include
ccflags-y			+= -I$(LINUX_CODE_PATH)/include/linux
ccflags-y			+= -I$(PWD)/udc
EXTRA_CFLAGS 		+=$(TARGET_EXTRA_CFLAGS)


obj-$(CONFIG_USB_LIBCOMPOSITE)	+= libcomposite.o
libcomposite-y			:= usbstring.o config.o epautoconf.o
libcomposite-y			+= composite.o functions.o u_f.o

obj-$(CONFIG_USB_GADGET)		+= udc/ function/ legacy/
obj-$(CONFIG_VIDEOBUF2_CORE)	+= v4l2-core/v4l2-core_linux-3.18/

default:	
	@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget.h $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h
	@cp $(PWD)/include/usb/gadget_vhd.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h	
	@$(MAKE) ARCH=$(TARGET_ARCH) CROSS_COMPILE=$(TARGET_CROSS) -C $(LINUX_OBJ_PATH) M=$(PWD) modules
	@cp $(LINUX_CODE_PATH)/include/linux/usb/gadget_bak.h $(LINUX_CODE_PATH)/include/linux/usb/gadget.h
	@cp $(PWD)/*.ko  $(KO_OUT_DIR)
	@cp $(PWD)/legacy/*.ko	$(KO_OUT_DIR)
	@cp $(PWD)/function/*.ko  $(KO_OUT_DIR)	
	@cp $(PWD)/v4l2-core/v4l2-core_linux-3.18/*.ko $(KO_OUT_DIR)
	@cp $(PWD)/udc/*.ko $(KO_OUT_DIR)
	$(TARGET_STRIP) $(KO_OUT_DIR)/*.ko --strip-unneeded


clean:
	@make -C $(LINUX_OBJ_PATH) M=$(PWD) clean 
	@rm -f *.bak *.o 
	@rm -f *.bak *.o 
	@rm -f *.o modules.* *.symvers *.mod.c *.o.cmd
