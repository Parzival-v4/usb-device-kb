# Gadget modes in this repo: Legacy vs ConfigFS

## Variants
| SoC | Family | Mode | Driver root |
|-----|--------|------|-------------|
| rk3588 | MeetingHub_Beast | legacy | `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/` |
| rk3588 | RB10_RB20 | legacy | `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/` |
| rv1126 | BY42 | legacy | `code/ucam/soc/rv1126/families/BY42/gadget_vhd/` |
| rk3588 | RB10Lite_Android | configfs | (code not imported yet; uses the same `gadget_vhd/function/` style functions) |

---

## Legacy mode (kernel gadget "legacy" binding)

### What is the "main" gadget driver?
- Main driver: `gadget_vhd/legacy/g_webcam.c`
- Role:
  - acts as the gadget top-level/binder (UDC bind + config composition)
  - selects/initializes functions
  - owns the primary descriptor/config layout (then pulls details from functions)

### Where functions live
Functions are implemented under:
- `gadget_vhd/function/`

Typical function drivers:
- UVC: `gadget_vhd/function/f_uvc.c`
- UAC: `gadget_vhd/function/f_uac.c`
- (others): `f_hid.c`, `f_ncm.c`, `f_rndis.c`, `f_msd.c` ... (TODO: list actual ones used)

### How descriptors are composed (high-level)
- "Descriptor skeleton / top-level config": in `legacy/g_webcam.c`
- "Function-specific descriptors / control handlers":
  - UVC-specific descriptors & requests: `function/f_uvc.c`
  - UAC-specific descriptors & requests: `function/f_uac.c`
- Final presented USB configuration depends on which functions are enabled/bound by `g_webcam.c` (compile-time options and/or module params).

### Runtime enable (examples; fill in real commands)
- Load / enable:
 insmod_uvc.sh
 #source iniparser.sh
#!/bin/sh    
DATA_CONF_FILE_PATH="/oem/config.ini"
CONF_FILE_PATH="/oem/ucam_config.ini"
SERIAL_NUMBER_PATH="/userdata/serial_number.ini"

USB_VENDOR_ID=""    
USB_PRODUCT_ID="" 
USB_BCD="" 
USB_DFU_PRODUCT_ID=""   

USB_VENDOR_LABEL=""    
USB_VENDOR_LABEL1=""    
USB_VENDOR_LABEL2=""    
USB_VENDOR_LABEL3=""    
USB_VENDOR_LABEL4=""    

USB_PRODUCT_LABEL=""
USB_PRODUCT_LABEL1=""
USB_PRODUCT_LABEL2=""
USB_PRODUCT_LABEL3=""
USB_PRODUCT_LABEL4=""
USB_PRODUCT_LABEL5=""

AUDIO_ENABLE=""
DFU_ENABLE=""
UVC_ENABLE=""
UVC_S2_ENABLE=""
UVC_V150_ENABLE=""
WIN7_USB3_ENABLE=""
BULK_EP_ENABLE=""
ZTE_HID_ENABLE=""
H264_ENABLE=""
S1_TO_S2_ENTITY_ID_C1="-1"
S1_TO_S2_ENTITY_ID_C2="-1"
UAC_FREQ=""
RNDIS_ENABLE=""
HID_INTERRUPT_EP_ENABLE=""
SFB_HID_MODE=""
ZTE_UPGRADE_HID_ENABLE=""
DFU_LAST=""

getUserInfo()    
{  
	USB_VENDOR_ID=$(awk -F '=' '/USB/{a=1}a==1&&$1~/USB_Vendor_ID/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	USB_PRODUCT_ID=$(awk -F '=' '/USB/{a=1}a==1&&$1~/USB_Product_ID/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	USB_DFU_PRODUCT_ID=$(awk -F '=' '/USB/{a=1}a==1&&$1~/USB_DFU_Product_ID/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	USB_VENDOR_LABEL=$(awk -F '=' '/USB/{a=1}a==1&&$1~/USB_Vendor_Label/{print $2;exit}' ${DATA_CONF_FILE_PATH})   
	USB_PRODUCT_LABEL=$(awk -F '=' '/USB/{a=1}a==1&&$1~/USB_Product_Label/{print $2;exit}' ${DATA_CONF_FILE_PATH})   
	AUDIO_ENABLE=$(awk -F '=' '/Audio/{a=1}a==1&&$1~/Audio_Enable/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	BULK_EP_ENABLE=$(awk -F '=' '/Video/{a=1}a==1&&$1~/UVC_BULK_EP_Enable/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	DFU_ENABLE=$(awk -F '=' '/Video/{a=1}a==1&&$1~/DFU_Enable/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	ZTE_UPGRADE_HID_ENABLE=$(awk -F '=' '/Video/{a=1}a==1&&$1~/ZTE_UPGRADE_HID_Enable/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	MFR_STRINGS_ENABLE=$(awk -F '=' '/USB/{a=1}a==1&&$1~/MFR_Strings_Enable/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	SFB_HID_MODE=$(awk -F '=' '/Video/{a=1}a==1&&$1~/SFB_HID_Mode/{print $2;exit}' ${DATA_CONF_FILE_PATH})
	DFU_LAST=$(awk -F '=' '/Video/{a=1}a==1&&$1~/DFU_Last/{print $2;exit}' ${DATA_CONF_FILE_PATH})

	SERIAL_NUMBER=$(awk -F '=' '/SN/{a=1}a==1&&$1~/serial_number/{print $2;exit}' ${SERIAL_NUMBER_PATH}) 

	USB_BCD=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/usb_bcd/{print $2;exit}' ${CONF_FILE_PATH})  
	H264_ENABLE=$(awk -F '=' '/UVC_C1_S1/{a=1}a==1&&$1~/uvc110_h264_enable/{print $2;exit}' ${CONF_FILE_PATH})
	UVC_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/uvc_enable/{print $2;exit}' ${CONF_FILE_PATH})
	UVC_S2_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/uvc_s2_enable/{print $2;exit}' ${CONF_FILE_PATH})
	UVC_V150_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/uvc_c2_v150_enable/{print $2;exit}' ${CONF_FILE_PATH})
	WIN7_USB3_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/win7_usb3_enable/{print $2;exit}' ${CONF_FILE_PATH})
	ZTE_HID_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/hid_custom_enable/{print $2;exit}' ${CONF_FILE_PATH})
	UAC_FREQ=$(awk -F '=' '/UAC/{a=1}a==1&&$1~/uac_samplepsec/{print $2;exit}' ${CONF_FILE_PATH})
	RNDIS_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/usb_rndis_enable/{print $2;exit}' ${CONF_FILE_PATH})
	HID_INTERRUPT_EP_ENABLE=$(awk -F '=' '/UCAM/{a=1}a==1&&$1~/hid_interrupt_ep_enable/{print $2;exit}' ${CONF_FILE_PATH})
}  


getUserInfo

USB_VENDOR_LABEL1=$(echo $USB_VENDOR_LABEL | awk '{print $1}')
USB_VENDOR_LABEL2=$(echo $USB_VENDOR_LABEL | awk '{print $2}')
USB_VENDOR_LABEL3=$(echo $USB_VENDOR_LABEL | awk '{print $3}')
USB_VENDOR_LABEL4=$(echo $USB_VENDOR_LABEL | awk '{print $4}')

USB_PRODUCT_LABEL1=$(echo $USB_PRODUCT_LABEL | awk '{print $1}')
USB_PRODUCT_LABEL2=$(echo $USB_PRODUCT_LABEL | awk '{print $2}')
USB_PRODUCT_LABEL3=$(echo $USB_PRODUCT_LABEL | awk '{print $3}')
USB_PRODUCT_LABEL4=$(echo $USB_PRODUCT_LABEL | awk '{print $4}')
USB_PRODUCT_LABEL5=$(echo $USB_PRODUCT_LABEL | awk '{print $5}')


echo USB_VENDOR_LABEL:$USB_VENDOR_LABEL

echo USB_PRODUCT_LABEL:$USB_PRODUCT_LABEL
if [ "$USB_PRODUCT_LABEL"x == x ] ; then
    echo "no USB_PRODUCT_LABEL, need to dfu reupdate"
	/customer/usb_ko/run_uboot_dfu.sh
	sleep 1
	exit 0
fi
SERIAL_NUMBER=`echo $SERIAL_NUMBER|tr -d " "`
if [ "$SERIAL_NUMBER"x == x ] ; then
    SERIAL_NUMBER="1111111111111111"
fi
if [ ! -n "$AUDIO_ENABLE" ];then
    AUDIO_ENABLE="1"
fi
if [ ! -n "$H264_ENABLE" ];then
    H264_ENABLE="1"
fi
if [ ! -n "$USB_VENDOR_ID" ];then
    USB_VENDOR_ID="0x2E7E"
fi
if [ ! -n "$USB_PRODUCT_ID" ];then
    USB_PRODUCT_ID="0x0000"
fi
if [ ! -n "$USB_DFU_PRODUCT_ID" ];then
    USB_DFU_PRODUCT_ID=$USB_PRODUCT_ID
fi
if [ ! -n "$USB_BCD" ];then
    USB_BCD="0"
fi
if [ ! -n "$DFU_ENABLE" ];then
    DFU_ENABLE="1"
fi
if [ ! -n "$UVC_ENABLE" ];then
    UVC_ENABLE="1"
fi
if [ ! -n "$UVC_S2_ENABLE" ];then
    UVC_S2_ENABLE="0"
fi
if [ ! -n "$UVC_V150_ENABLE" ];then
    UVC_V150_ENABLE="1"
fi
if [ ! -n "$WIN7_USB3_ENABLE" ];then
    WIN7_USB3_ENABLE="1"
fi
if [ ! -n "$BULK_EP_ENABLE" ];then
    BULK_EP_ENABLE="0"
fi
if [ $UVC_S2_ENABLE == "1" ] && [ $H264_ENABLE == "1" ];then
    S1_TO_S2_ENTITY_ID_C1="0x0d"
fi
if [ $UVC_S2_ENABLE == "1" ] && [ $H264_ENABLE == "0" ];then
    S1_TO_S2_ENTITY_ID_C1="0x0c"
fi
if [ $UVC_V150_ENABLE == "1" ];then
    S1_TO_S2_ENTITY_ID_C2="0x0c"
fi
if [ ! -n "$UAC_FREQ" ];then
    UAC_FREQ="16000"
fi
if [ ! -n "$ZTE_HID_ENABLE" ];then
    ZTE_HID_ENABLE="0"
fi
if [ ! -n "$ZTE_UPGRADE_HID_ENABLE" ];then
    ZTE_UPGRADE_HID_ENABLE="0"
fi
if [ ! -n "$RNDIS_ENABLE" ];then
    RNDIS_ENABLE="0"
fi
if [ ! -n "$HID_INTERRUPT_EP_ENABLE" ];then
    HID_INTERRUPT_EP_ENABLE="0"
fi
if [ ! -n "$MFR_STRINGS_ENABLE" ];then
    MFR_STRINGS_ENABLE="0"
fi
if [ ! -n "$SFB_HID_MODE" ];then
    SFB_HID_MODE="0"
fi
if [ ! -n "$DFU_LAST" ];then
    DFU_LAST="0"
fi


vendor_id=$(echo $USB_VENDOR_ID)
product_id=$(echo $USB_PRODUCT_ID)
dfu_product_id=$(echo $USB_DFU_PRODUCT_ID)
device_bcd=$(echo $USB_BCD)
serial_number=$(echo $SERIAL_NUMBER)
uvc_enable=$(echo $UVC_ENABLE)
dfu_enable=$(echo $DFU_ENABLE)
audio_enable=$(echo $AUDIO_ENABLE)
uvc_s2_enable=$(echo $UVC_S2_ENABLE)
uvc_v150_enable=$(echo $UVC_V150_ENABLE)
win7_usb3_enable=$(echo $WIN7_USB3_ENABLE)
zte_hid_enable=$(echo $ZTE_HID_ENABLE)
zte_upgrade_hid_enable=$(echo $ZTE_UPGRADE_HID_ENABLE)
bulk_ep_enable=$(echo $BULK_EP_ENABLE)
s1_to_s2_id_c1=$(echo $S1_TO_S2_ENTITY_ID_C1)
s1_to_s2_id_c2=$(echo $S1_TO_S2_ENTITY_ID_C2)
uac_freq=$(echo $UAC_FREQ)
rndis_enable=$(echo $RNDIS_ENABLE)
hid_interrupt_ep_enable=$(echo $HID_INTERRUPT_EP_ENABLE)
MFR_Strings_enable=$(echo $MFR_STRINGS_ENABLE)
SFB_HID_Mode=$(echo $SFB_HID_MODE)
dfu_last=$(echo $DFU_LAST)

#./insmod_alsa.sh

#insmod usb-common.ko
#cd /opt/kernel_drv_ko
#insmod phy-rockchip-usb.ko
#insmod phy-rockchip-usbdp.ko
#insmod phy-rockchip-inno-usb2.ko
#insmod phy-rockchip-inno-usb3.ko
#cd /opt/customer/usb_ko/
insmod udc-core.ko
insmod dwc3-of-simple.ko
insmod dwc3.ko usb3_enable=1 vbus_gpio_enable=0

insmod libcomposite.ko

insmod u_ether.ko
insmod usb_f_rndis.ko

insmod usb_f_uvc.ko bulk_streaming_ep=$bulk_ep_enable bulk_max_size=65536  s2_entity_id_c1=$s1_to_s2_id_c1 s2_entity_id_c2=$s1_to_s2_id_c2
insmod usb_f_uvc_s2.ko
#insmod usb_f_uac1.ko audio_freq=$uac_freq  mic_volume_min=5120 mic_volume_max=13568 alsa_buf_num=4
insmod usb_f_uac1_speak.ko
insmod g_webcam.ko g_uvc_request_auto_enable=1 \
c_srate=48000 p_srate=48000 \
c_chmask=3 p_chmask=3 \
g_hid_enable=0 \
g_uvc_interrupt_ep_enable=0 \
g_zte_upgrade_hid_enable=$zte_upgrade_hid_enable \
g_sfb_hid_enable=$SFB_HID_Mode \
g_sfb_keyboard_enable=0 \
usb_connect=0 \
work_pump_enable=0 \
ManufacturerStrings=$MFR_Strings_enable \
g_hid_interrupt_ep_enable=0 \
g_uvc_enable=$uvc_enable \
g_dfu_enable=$dfu_enable \
g_dfu_last=$dfu_last \
g_audio_enable=$audio_enable \
req_num=1 \
g_uvc_s2_enable=$uvc_s2_enable \
g_zte_hid_enable=$zte_hid_enable \
g_win7_usb3_enable=$win7_usb3_enable \
g_uvc_v150_enable=$uvc_v150_enable \
g_rndis_enable=$rndis_enable \
g_serial_number=$serial_number \
g_vendor_id=$vendor_id \
g_customer_id=$product_id \
g_device_bcd=$device_bcd \
g_usb_vendor_label1=$USB_VENDOR_LABEL1 \
g_usb_vendor_label2=$USB_VENDOR_LABEL2 \
g_usb_vendor_label3=$USB_VENDOR_LABEL3 \
g_usb_product_label1=$USB_PRODUCT_LABEL1 \
g_usb_product_label2=$USB_PRODUCT_LABEL2 \
g_usb_product_label3=$USB_PRODUCT_LABEL3 \
g_usb_product_label4=$USB_PRODUCT_LABEL4 \
g_usb_product_label5=$USB_PRODUCT_LABEL5 


if [ $rndis_enable -eq 1 ];then
    ifconfig usb0 192.168.88.8
    telnet &
fi
ifconfig lo up




- Verify (host side):
  - `lsusb -v` (look for interfaces: Video/Audio)
- Verify (device side):
  - `dmesg | grep -i "g_webcam\|uvc\|uac\|gadget\|udc"` (keywords may vary)

---

## ConfigFS mode (Android)

### Overview
- ConfigFS composes gadget at runtime by creating:
  - one gadget instance (`/config/usb_gadget/<name>`)
  - one or more configs (`configs/c.1`, ...)
  - one or more functions (`functions/uvc.*`, `functions/uac2.*`, etc.)
  - links functions into configs, then binds to UDC by writing `UDC`.

### Where "function drivers" come from
- The underlying kernel function implementations are still in:
  - `gadget_vhd/function/` (e.g. `f_uvc.c`, `f_uac.c`, ...)

ConfigFS decides which functions are active and how they are combined.

### ConfigFS script (please paste the authoritative flow)
Paste your actual script or the command sequence here (it can be simplified, but keep key paths & names).

init.vhd-usbdevice.rc
on early-init
  insmod /vendor/lib/modules/udc-core.ko
  insmod /vendor/lib/modules/dwc3-of-simple.ko
  insmod /vendor/lib/modules/dwc3.ko
  insmod /vendor/lib/modules/libcomposite.ko
  insmod /vendor/lib/modules/usb_f_uvc.ko bulk_streaming_ep=1 bulk_max_size=65536
  #insmod /vendor/lib/modules/usb_f_uvc.ko
  insmod /vendor/lib/modules/u_audio.ko
  insmod /vendor/lib/modules/usb_f_uac1.ko
  insmod /vendor/lib/modules/usb_f_hid_consumer.ko
  insmod /vendor/lib/modules/u_ether.ko
  #insmod /vendor/lib/modules/usb_f_rndis.ko
  insmod /vendor/lib/modules/usb_f_ncm.ko


on property:sys.boot_completed=1
   chmod 0777 /config/usb_gadget
   chmod 0777 /config/usb_gadget/g1
   chmod 0777 /config/usb_gadget/g1/functions
   chmod 0777 /config/usb_gadget/g1/functions/uac1.gs7
   chmod 0777 /config/usb_gadget/g1/functions/uac1.gs7/c_volume_min
   chmod 0777 /config/usb_gadget/g1/functions/uac1.gs7/c_volume_max
   chmod 0777 /sys/kernel
   chmod 0777 /sys/kernel/debug
   chmod 0777 /sys/kernel/debug/usb
   chmod 0777 /sys/kernel/debug/usb/fc000000.usb
   chmod 0777 /sys/kernel/debug/usb/fc000000.usb/mode
   start set_hid_report_desc
   start vhd_uvc_device

service set_hid_report_desc /vendor/bin/sh /vendor/bin/set_hid_report_desc.sh
    class late_start
    user root
    oneshot
seclabel u:r:init:s0

service vhd_uvc_device /system/bin/vhd_uvc_device
   class main
   user root
   group root
   disabled



init.rk30board.usb.rc

on early-boot
    mkdir /config/usb_gadget/g1 0770 shell shell
    mkdir /config/usb_gadget/g1/strings/0x409 0770
    mkdir /config/usb_gadget/g1/configs/b.1 0770 shell shell
    mkdir /config/usb_gadget/g1/configs/b.1/strings/0x409 0770 shell shell
    write /config/usb_gadget/g1/bcdUSB 0x0200
    write /config/usb_gadget/g1/idVendor 0x2e7e
    write /config/usb_gadget/g1/strings/0x409/serialnumber ${ro.serialno}
    write /config/usb_gadget/g1/strings/0x409/manufacturer ${ro.product.manufacturer}
    write /config/usb_gadget/g1/strings/0x409/product "${ro.product.manufacturer} ${ro.product.model}"
    write /config/usb_gadget/g1/configs/b.1/MaxPower 500
    write /config/usb_gadget/g1/os_desc/b_vendor_code 0x1
    write /config/usb_gadget/g1/os_desc/qw_sign "MSFT100"

    # ffs function
    mkdir /config/usb_gadget/g1/functions/ffs.adb 0770 shell shell
    mkdir /config/usb_gadget/g1/functions/ffs.mtp
    mkdir /config/usb_gadget/g1/functions/ffs.ptp
    mkdir /dev/usb-ffs 0775 shell shell
    mkdir /dev/usb-ffs/adb 0770 shell shell
    mkdir /dev/usb-ffs/mtp 0770 mtp mtp
    mkdir /dev/usb-ffs/ptp 0770 mtp mtp
    mount functionfs adb /dev/usb-ffs/adb rmode=0770,fmode=0660,uid=2000,gid=2000,no_disconnect=1
    mount functionfs mtp /dev/usb-ffs/mtp rmode=0770,fmode=0660,uid=1024,gid=1024,no_disconnect=1
    mount functionfs ptp /dev/usb-ffs/ptp rmode=0770,fmode=0660,uid=1024,gid=1024,no_disconnect=1
    setprop sys.usb.mtp.device_type 3
    setprop sys.usb.mtp.batchcancel true
    symlink /config/usb_gadget/g1/configs/b.1 /config/usb_gadget/g1/os_desc/b.1


    # rndis function
    mkdir /config/usb_gadget/g1/functions/rndis.gs4
    # Modify class/subclass/protocol for rndis.gs4
    # Remote NDIS: Class: Wireless Controller (0xe0), Subclass: 0x1, Protocol: 0x3
    write /config/usb_gadget/g1/functions/rndis.gs4/class e0
    write /config/usb_gadget/g1/functions/rndis.gs4/subclass 01
    write /config/usb_gadget/g1/functions/rndis.gs4/protocol 03
    write /config/usb_gadget/g1/functions/rndis.gs4/os_desc/interface.ncm/compatible_id RNDIS
    write /config/usb_gadget/g1/functions/rndis.gs4/ifname rndis%d
    setprop vendor.usb.rndis.config rndis.gs4


    # ncm function
    mkdir /config/usb_gadget/g1/functions/ncm.gs9
    write /config/usb_gadget/g1/functions/ncm.gs9/os_desc/interface.ncm/compatible_id WINNCM
    write /config/usb_gadget/g1/functions/ncm.gs9/ifname ncm%d


    # uac1 function
    mkdir /config/usb_gadget/g1/functions/uac1.gs7
    mkdir /config/usb_gadget/g1/functions/hid_consumer.usb0
    write /config/usb_gadget/g1/functions/uac1.gs7/function_name "${ro.product.manufacturer} ${ro.product.model}"
    write /config/usb_gadget/g1/functions/uac1.gs7/c_volume_min "${ro.vhd.uac.min_volume.db}"

    # uvc function
    mkdir /config/usb_gadget/g1/functions/uvc.gs6
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/control/header/h
    symlink /config/usb_gadget/g1/functions/uvc.gs6/control/header/h \
    /config/usb_gadget/g1/functions/uvc.gs6/control/class/fs/h
    symlink /config/usb_gadget/g1/functions/uvc.gs6/control/header/h \
    /config/usb_gadget/g1/functions/uvc.gs6/control/class/ss/h
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/360p
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/360p/dwFrameInterval \
"666666
1000000
2000000"
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/wWidth 1280
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/wHeight 720
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/dwDefaultFrameInterval 1000000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/dwMinBitRate 73728000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/dwMaxBitRate 147456000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/dwMaxVideoFrameBufferSize 1843200
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/720p/dwFrameInterval \
"1000000
2000000"
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/wWidth 1280
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/wHeight 960
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/dwDefaultFrameInterval 1000000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/dwMinBitRate 98304000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/dwMaxBitRate 196608000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/dwMaxVideoFrameBufferSize 2457600
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u/960p/dwFrameInterval \
"1000000
2000000"
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/wWidth 1280
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/wHeight 720
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/dwDefaultFrameInterval 1000000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/dwMinBitRate 73728000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/dwMaxBitRate 147456000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/dwMaxVideoFrameBufferSize 1843200
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/720p/dwFrameInterval \
"1000000
2000000"
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p/wWidth 1280
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p/wHeight 960
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p/dwMinBitRate 98304000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p/dwMaxBitRate 196608000
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p/dwMaxVideoFrameBufferSize 2457600
    write /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m/960p/dwFrameInterval \
"1000000
2000000"
    mkdir /config/usb_gadget/g1/functions/uvc.gs6/streaming/header/h
    symlink /config/usb_gadget/g1/functions/uvc.gs6/streaming/uncompressed/u \
    /config/usb_gadget/g1/functions/uvc.gs6/streaming/header/h/u
    symlink /config/usb_gadget/g1/functions/uvc.gs6/streaming/mjpeg/m \
    /config/usb_gadget/g1/functions/uvc.gs6/streaming/header/h/m
    symlink /config/usb_gadget/g1/functions/uvc.gs6/streaming/header/h \
    /config/usb_gadget/g1/functions/uvc.gs6/streaming/class/fs/h
    symlink /config/usb_gadget/g1/functions/uvc.gs6/streaming/header/h \
    /config/usb_gadget/g1/functions/uvc.gs6/streaming/class/hs/h
    symlink /config/usb_gadget/g1/functions/uvc.gs6/streaming/header/h \
    /config/usb_gadget/g1/functions/uvc.gs6/streaming/class/ss/h

    # chown file/folder permission
    chown system system /config/usb_gadget/
    chown system system /config/usb_gadget/g1
    chown system system /config/usb_gadget/g1/UDC
    chown system system /config/usb_gadget/g1/bDeviceClass
    chown system system /config/usb_gadget/g1/bDeviceProtocol
    chown system system /config/usb_gadget/g1/bDeviceSubClass
    chown system system /config/usb_gadget/g1/bMaxPacketSize0
    chown system system /config/usb_gadget/g1/bcdDevice
    chown system system /config/usb_gadget/g1/bcdUSB
    chown system system /config/usb_gadget/g1/configs
    chown system system /config/usb_gadget/g1/configs/b.1
    chown system system /config/usb_gadget/g1/configs/b.1/MaxPower
    chown system system /config/usb_gadget/g1/configs/b.1/bmAttributes
    chown system system /config/usb_gadget/g1/configs/b.1/strings
    chown system system /config/usb_gadget/g1/functions


    chown system system /config/usb_gadget/g1/functions/rndis.gs4
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/class
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/dev_addr
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/host_addr
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/ifname
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/os_desc
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/os_desc/interface.rndis
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/os_desc/interface.rndis/compatible_id
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/os_desc/interface.rndis/sub_compatible_id
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/protocol
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/qmult
    chown system system /config/usb_gadget/g1/functions/rndis.gs4/subclass



    chown system system /config/usb_gadget/g1/functions/ffs.adb
    chown system system /config/usb_gadget/g1/functions/uvc.gs6
    chown system system /config/usb_gadget/g1/functions/uac1.gs7
    chown system system /config/usb_gadget/g1/functions/hid_consumer.usb0
    chown system system /config/usb_gadget/g1/idProduct
    chown system system /config/usb_gadget/g1/idVendor
    chown system system /config/usb_gadget/g1/max_speed
    chown system system /config/usb_gadget/g1/os_desc
    chown system system /config/usb_gadget/g1/os_desc/b.1
    chown system system /config/usb_gadget/g1/os_desc/b_vendor_code
    chown system system /config/usb_gadget/g1/os_desc/qw_sign
    chown system system /config/usb_gadget/g1/os_desc/use
    chown system system /config/usb_gadget/g1/strings
    chown system system /config/usb_gadget/g1/strings/0x409
    chown system system /config/usb_gadget/g1/strings/0x409/manufacturer
    chown system system /config/usb_gadget/g1/strings/0x409/product
    chown system system /config/usb_gadget/g1/strings/0x409/serialnumber

on boot
    write /config/usb_gadget/g1/bcdDevice 0x0310
    # Set USB timeout
    write sys/module/usbcore/parameters/initial_descriptor_timeout 500
    # Use USB Gadget HAL
    setprop sys.usb.configfs 2

on property:init.svc.adbd=stopped
    setprop sys.usb.ffs.ready 0



### Verify
- `ls -R /config/usb_gadget`
- `cat /config/usb_gadget/*/UDC`
- `dmesg | tail -n 200`
