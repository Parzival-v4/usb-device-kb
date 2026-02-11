# UCAM USB gadget code map (entry points & wiring)

## EP0 / Setup request handling (end-to-end)

### A) UDC driver level (receives SETUP packet on EP0)
Depending on controller, the EP0 handler reads the 8-byte setup packet and decides:
- handle standard requests in UDC (some do)
- otherwise delegate to gadget layer (`gadget_driver->setup()`)

Examples in this repo:

#### DWC3 (rk3588 families, hisi/rk variants)
- `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/udc/dwc3_hisi/ep0.c`
- `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/udc/dwc3_hisi/ep0.c`

Delegation pattern:
- standard: `dwc3_ep0_std_request(dwc, ctrl)`
- non-standard: `dwc3_ep0_delegate_req(dwc, ctrl)` → gadget driver

Note: RB10_RB20 `dwc3_hisi/ep0.c` has extra vendor logic:
- `g_ep0_setup_flag`, `g_ep0_setup_timeout`, `EP0_SETUP_TIMEOUT_JIFFIES`
- for CLASS requests with `wLength > 0`, it sets timeout/flag before delegating.

Related helper header with EP0 task hooks:
- `.../udc/dwc3_ssc9381/gadget.h` (has `ep0_setup_task_init/exit`, `ep0_setup_flag_clear`, etc.)

#### Cadence (cdns3) EP0 delegation
- `.../udc/cdns3_cv5/cdns3-ep0.c`
  - `cdns3_ep0_delegate_req()` calls:
    `priv_dev->gadget_driver->setup(&priv_dev->gadget, ctrl_req)`

#### “cdnsp” EP0 std/delegate split (another Cadence path)
- `.../udc/cdns3_cv5/cdnsp-ep0.c`
  - `cdnsp_ep0_std_request()` handles standard requests, else delegates.

(There are also other UDC examples in-tree like `udc-xilinx.c`, `bcm63xx_udc.c`, etc., but the key point is the same: delegate to gadget layer for most requests.)

---

### B) Gadget layer / composite core dispatch
Once UDC delegates, it eventually calls the gadget driver's `.setup()`.

In composite gadgets, the `.setup()` implementation is the composite framework setup handler in:
- `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/composite.c`
- `code/ucam/soc/rk3588/families/MeetingHub_Beast/gadget_vhd/composite.c`
- `code/ucam/soc/rv1126/families/BY42/gadget_vhd/composite.c`

Composite setup logic (high level, from composite.c):
1) Determine recipient (device/interface/endpoint) and locate matching `usb_function`
2) Call in order:
   - matched function `f->setup(f, ctrl)` if present
   - else current configuration `c->setup(c, ctrl)` if present
   - else (if only one function) call that function’s setup
3) Queue EP0 response via `composite_ep0_queue()` unless delayed status

Relevant snippet area (dispatch to `f->setup` / `c->setup`):
- `.../gadget_vhd/composite.c` around the big setup switch and `if (f && f->setup) value = f->setup(f, ctrl); ...`

---

### C) Function-level setup handlers (where class/vendor requests are handled)
Most class/vendor control requests end up here.

Examples:
- UVC function:
  - `code/ucam/soc/rk3588/families/RB10_RB20/gadget_vhd/function/f_uvc.c`
    - registers `uvc->func.setup = uvc_function_setup;` (via function ops table)
- UAC2 function:
  - `.../function/f_uac2.c` sets `agdev->func.setup = afunc_setup;`
- DFU function:
  - `.../function/f_dfu.c` implements `dfug_setup()` and may return `USB_GADGET_DELAYED_STATUS` in some cases

So debug EP0 issues usually means:
- confirm UDC is delegating (UDC ep0.c logs)
- confirm composite setup is reached (composite.c logs)
- confirm function setup returns expected value/queues response.

---

### D) Legacy top-level gadget (g_webcam) vs ConfigFS
- Legacy (e.g. `legacy/g_webcam.c`) is responsible for composing which functions exist in the configuration.
- ConfigFS composes functions at runtime but still uses the same underlying function drivers under `function/`.
