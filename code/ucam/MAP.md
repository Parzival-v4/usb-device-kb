# UCAM USB gadget code map (entry points & wiring)

> This is a navigation index. It is not a full design doc.

## 0. Where the code lives (imported)
- Shared userspace library (common):
  - `code/ucam/shared/libucam/`
- Kernel gadget driver trees (legacy, per family):
  - rk3588 / MeetingHub_Beast:
    - `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/`
  - rk3588 / RB10_RB20:
    - `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/`
  - rv1126 / BY42:
    - `code/ucam/soc/rv1126/families/BY42/gadget_vhd/`

---

## 1. Legacy gadget top-level (“binder” / composition)

### 1.1 Main legacy gadget driver
- Declared main entry (per `GADGET_MODES.md`): `gadget_vhd/legacy/g_webcam.c`
  - NOTE: `g_webcam.c` exists in the original trees (confirmed via build cmd artifacts under legacy/ for MeetingHub_Beast),
    but the current repo search results that surfaced were mainly the generated build files, not the source file itself.

MeetingHub_Beast legacy build artifacts referencing g_webcam:
- `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/legacy/.g_webcam.o.cmd`
- `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/legacy/.g_webcam.ko.cmd`
- `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/legacy/.g_webcam.mod.cmd`
- `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/legacy/.g_webcam.mod.o.cmd`

Module list for MeetingHub_Beast shows the expected stacking:
- `.../gadget_vhd/modules.order` includes:
  - `.../udc/udc-core.ko`
  - `.../function/usb_f_uvc.ko`, `.../function/usb_f_uac1.ko`, `.../function/usb_f_uac2.ko`, ...
  - `.../legacy/g_webcam.ko`

---

## 2. Function drivers: UVC / UAC wiring

### 2.1 UVC function driver
RB10_RB20 UVC function registration:
- `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/function/f_uvc.c`
  - function ops are assigned (bind/unbind/get_alt/set_alt/disable/setup/suspend/resume)
  - declares function init: `DECLARE_USB_FUNCTION_INIT(uvc, uvc_alloc_inst, uvc_alloc);`

Related headers/options:
- `.../gadget_vhd/function/f_uvc.h`
- `.../gadget_vhd/function/u_uvc.h`
  - shows many runtime opts, e.g. `streaming_interval/maxpacket/maxburst`, `uac_enable`, `uvc_s2_enable`, `win7_usb3_enable`, etc.

### 2.2 UAC function driver(s)
You have multiple UAC flavors in-tree:
- UAC1:
  - `code/ucam/soc/rv1126/families/BY42/gadget_vhd/function/f_uac1.c`
  - plus speak variant: `.../function/f_uac1_speak.c`
- UAC2:
  - `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/function/f_uac2.c`
- UAC2 helper/options:
  - `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/function/u_uac2.h`

UAC legacy binder example (shows how function instances are acquired):
- `.../gadget_vhd/legacy/audio.c` and `.../legacy/audio.inl` (multiple families)
  - uses `usb_get_function_instance("uac2")` or `usb_get_function_instance("uac1")` or `"uac1_legacy"` depending on config macros.

---

## 3. UDC / core glue (useful for bind/uevent/debug)
Example UDC core:
- `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/udc/core_v6.1.c`
  - adds uevent vars `USB_UDC_NAME`, `USB_UDC_DRIVER`
  - defines `gadget_bus_type` with `.probe = gadget_bind_driver`

---

## 4. Userspace “libucam” (UVC/UAC logic that interacts with kernel)
UVC control & descriptor buffer mode:
- `code/ucam/shared/libucam/ucam/src/uvc/uvc_ctrl.c`
- `code/ucam/shared/libucam/ucam/src/uvc/uvc_config.c`
  - contains `UVCIOC_SET_UVC_DESC_BUF_MODE` ioctl usage (descriptor buffer mode)

UVC stream helpers:
- `code/ucam/shared/libucam/ucam/src/uvc/uvc_stream.h`
- `code/ucam/shared/libucam/ucam/src/uvc/uvc_stream_yuv.c`

UAC userspace ALSA helper:
- `code/ucam/shared/libucam/ucam/src/uac/uac_alsa.c`

---

## 5. Build/config entry points (for reference)
Per-family make config examples (these hint which modules are expected):
- `code/ucam/soc/rv1126/families/BY42/gadget_vhd/make_config/*.mk`
  - e.g. `.../make_config/GK7205V300.mk` has a `default:` rule building kernel modules with `M=$(PWD)`
  - exports `CONFIG_USB_F_UVC`, `CONFIG_USB_F_UAC1`, `CONFIG_USB_G_WEBCAM`, etc.
