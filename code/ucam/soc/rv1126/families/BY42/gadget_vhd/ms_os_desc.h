#ifndef __MS_OS_DESC_H__
#define __MS_OS_DESC_H__

#define MS_OS_20_GET_DESCRIPTOR_REQ				0xC0

/* Microsoft OS 2.0 descriptor wIndex values */
#define MS_OS_20_DESCRIPTOR_INDEX 				0x07
#define MS_OS_20_SET_ALT_ENUMERATION			0x08

/* Microsoft OS 2.0 descriptor types */
#define MS_OS_20_SET_HEADER_DESCRIPTOR 			0x00
#define MS_OS_20_SUBSET_HEADER_CONFIGURATION 	0x01
#define MS_OS_20_SUBSET_HEADER_FUNCTION 		0x02
#define MS_OS_20_FEATURE_COMPATBLE_ID 			0x03
#define MS_OS_20_FEATURE_REG_PROPERTY 			0x04
#define MS_OS_20_FEATURE_MIN_RESUME_TIME 		0x05
#define MS_OS_20_FEATURE_MODEL_ID 				0x06
#define MS_OS_20_FEATURE_CCGP_DEVICE 			0x07
#define MS_OS_20_FEATURE_VENDOR_REVISION 		0x08

/* Property Data Types */
#define MS_OS_REG_SZ 					0x01
#define MS_OS_REG_EXPAND_SZ 			0x02
#define MS_OS_REG_BINAR 				0x03
#define MS_OS_REG_DWORD_LITTLE_ENDIAN 	0x04
#define MS_OS_REG_DWORD_BIG_ENDIAN 		0x05
#define MS_OS_REG_LINK 					0x06
#define MS_OS_REG_MULTI_SZ 				0x07


#define	USB_CAP_TYPE_PLATFORM			5
#define USB_DT_USB_PLATFORM_CAP_SIZE	0x1C
struct usb_platform_cap_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDevCapabilityType;
	__u8  bReserved;
	__u8  PlatformCapabilityUUID[16];
	__u8  CapabilityData[8];
} __attribute__((packed));

struct ms_os_20_set_info_descriptor {
	__u32 dwWindowsVersion;
	__u16 wMSOSDescriptorSetTotalLength;
	__u8  bMS_VendorCode;
	__u8  bAltEnumCode;
} __attribute__((packed));

struct ms_os_20_set_header_descriptor {
	__u16 wLength;
	__u16 wDescriptorType;
	__u32 dwWindowsVersion;
	__u16 wTotalLength;
} __attribute__((packed));

struct ms_os_20_configuration_subset_header_descriptor {
	__u16 wLength;
	__u16 wDescriptorType;
	__u8  bConfigurationValue;
	__u8  bReserved;
	__u16 wTotalLength;
} __attribute__((packed));

struct ms_os_20_function_subset_header_descriptor {
	__u16 wLength;
	__u16 wDescriptorType;
	__u8  bFirstInterface;
	__u8  bReserved;
	__u16 wSubsetLength;
} __attribute__((packed));

struct ms_os_20_compatible_id_descriptor {
	__u16 wLength;
	__u16 wDescriptorType;
	__u8  CompatibleID[8];
	__u8  SubCompatibleID[8];
} __attribute__((packed));

#define MS_OS_20_REGISTRY_PROPERTY_SIZE(n,d)	(10+(n)+(d))

#define MS_OS_20_REGISTRY_PROPERTY_DESCRIPTOR(n,d) \
	ms_os_20_registry_property_descriptor_##n_##d

#define DECLARE_MS_OS_20_REGISTRY_PROPERTY_DESCRIPTOR(n,d)	\
struct  MS_OS_20_REGISTRY_PROPERTY_DESCRIPTOR(n,d) {		\
	__u16 wLength;					\
	__u16 wDescriptorType;			\
	__u16 wPropertyDataType;		\
	__u16 wPropertyNameLength;		\
	__u8  PropertyName[n];			\
	__u16 wPropertyDataLength;		\
	__u8  PropertyData[d];			\
} __attribute__ ((packed))

DECLARE_MS_OS_20_REGISTRY_PROPERTY_DESCRIPTOR(40,78);

struct ms_os_20_winusb_descriptor_set {
	struct ms_os_20_set_header_descriptor header;
	struct ms_os_20_configuration_subset_header_descriptor configuration;
	struct ms_os_20_function_subset_header_descriptor function;
	struct ms_os_20_compatible_id_descriptor compatible_id;
	struct MS_OS_20_REGISTRY_PROPERTY_DESCRIPTOR(40,78) registry_property;
} __attribute__((packed));

//0x06030000 for Windows Blue
#define MS_OS_20_WINDOWS_VERSION	0x06030000

#endif /* __MS_OS_DESC_H__ */
