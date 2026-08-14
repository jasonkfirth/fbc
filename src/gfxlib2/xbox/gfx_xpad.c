/*
    FreeBASIC gfxlib2 Xbox controller polling
    -----------------------------------------

    File: xbox/gfx_xpad.c

    Purpose:

        Poll original Xbox controllers through the nxdk USB/XID stack.

    Responsibilities:

        - track connected Xbox gamepads
        - keep interrupt transfers queued
        - normalize controller state for GETXPAD

    This file intentionally does NOT contain:

        - other platform controller backends
        - platform-selection branches
*/

#include "../fb_gfx.h"
#include <stdint.h>

#define XPAD_MAX_DEVICES 16

static void xpad_clear_outputs(ssize_t *buttons,
							   float *lstick_x, float *lstick_y,
							   float *rstick_x, float *rstick_y,
							   float *ltrigger, float *rtrigger,
							   ssize_t *dpad)
{
	if (buttons)
		*buttons = 0;
	if (lstick_x)
		*lstick_x = 0.0f;
	if (lstick_y)
		*lstick_y = 0.0f;
	if (rstick_x)
		*rstick_x = 0.0f;
	if (rstick_y)
		*rstick_y = 0.0f;
	if (ltrigger)
		*ltrigger = 0.0f;
	if (rtrigger)
		*rtrigger = 0.0f;
	if (dpad)
		*dpad = 0;
}

static float xpad_normalize_axis(int value)
{
	if (value <= -32768)
		return -1.0f;
	if (value >= 32767)
		return 1.0f;
	return (float)value / 32767.0f;
}

#define FB_XID_MAX_TRANSFER_QUEUE 4
#define FB_XID_TYPE_GAMECONTROLLER 0x01
#define FB_USBH_OK 0
#define FB_HID_RET_XFER_IS_RUNNING -1089
#define FB_XBOX_INIT_POLL_MSECS 500

typedef struct FB_XID_DESCRIPTOR_ {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdXid;
	uint8_t bType;
	uint8_t bSubType;
	uint8_t bMaxInputReportSize;
	uint8_t bMaxOutputReportSize;
	uint16_t wAlternateProductIds[4];
} FBPACKED FB_XID_DESCRIPTOR;

typedef struct FB_XID_GAMEPAD_IN_ {
	uint8_t startByte;
	uint8_t bLength;
	uint16_t dButtons;
	uint8_t a;
	uint8_t b;
	uint8_t x;
	uint8_t y;
	uint8_t black;
	uint8_t white;
	uint8_t l;
	uint8_t r;
	int16_t leftStickX;
	int16_t leftStickY;
	int16_t rightStickX;
	int16_t rightStickY;
} FBPACKED FB_XID_GAMEPAD_IN;

typedef struct FB_XID_DEV_ {
	uint16_t idVendor;
	uint16_t idProduct;
	FB_XID_DESCRIPTOR xid_desc;
	void *utr_list[FB_XID_MAX_TRANSFER_QUEUE];
	void *iface;
	uint32_t uid;
	struct FB_XID_DEV_ *next;
	void *user_data;
} FB_XID_DEV;

typedef struct FB_XBOX_UTR_ {
	void *udev;
	unsigned char setup[8];
	void *ep;
	uint8_t *buff;
	volatile uint8_t bIsTransferDone;
	uint32_t data_len;
	uint32_t xfer_len;
	uint8_t bIsoNewSched;
	uint16_t iso_sf;
	uint16_t iso_xlen[8];
	uint8_t *iso_buff[8];
	int iso_status[8];
	int td_cnt;
	int status;
	int interval;
	void *context;
	void *func;
	struct FB_XBOX_UTR_ *next;
} FB_XBOX_UTR;

typedef struct FB_XBOX_XPAD_SLOT_ {
	int seen;
	int has_state;
	int read_running;
	uint32_t uid;
	FB_XID_DEV *dev;
	FB_XID_GAMEPAD_IN state;
} FB_XBOX_XPAD_SLOT;

extern void usbh_core_init(void);
extern int usbh_pooling_hubs(void);
extern void usbh_xid_init(void);
extern FB_XID_DEV *usbh_xid_get_device_list(void);
extern int32_t usbh_xid_read(FB_XID_DEV *xid_dev, uint8_t ep_addr, void *rx_complete_callback);
extern int usbh_int_xfer(FB_XBOX_UTR *utr);

static int xbox_usb_inited;
static FB_XBOX_XPAD_SLOT xbox_xpad[4];

static void xpad_xbox_copy_packet(FB_XID_GAMEPAD_IN *dst, const uint8_t *src, uint32_t bytes)
{
	memset(dst, 0, sizeof(*dst));
	if (bytes > sizeof(*dst))
		bytes = sizeof(*dst);
	if (bytes > 0)
		memcpy(dst, src, bytes);
}

static FB_XBOX_XPAD_SLOT *xpad_xbox_find_slot(FB_XID_DEV *dev)
{
	uint32_t uid;
	int i;

	if (!dev)
		return NULL;

	uid = dev->uid;

	for (i = 0; i < 4; ++i) {
		if ((xbox_xpad[i].dev == dev) || (xbox_xpad[i].uid == uid))
			return &xbox_xpad[i];
	}

	return NULL;
}

static void xpad_xbox_requeue_read(FB_XBOX_XPAD_SLOT *slot, FB_XBOX_UTR *utr)
{
	int result;

	if (!slot || !utr)
		return;

	/*
		The NXDK USB stack is designed for interrupt transfers to be kept
		alive from their completion callback.  Reusing the completed UTR is
		also how NXDK's SDL joystick backend tracks Xbox controllers.

		Waiting for the next GETXPAD call to allocate a replacement transfer
		can leave us dependent on queue cleanup timing.  Keeping the transfer
		running here gives the backend a continuous stream of controller
		reports while the program polls the last completed state.
	*/
	utr->xfer_len = 0;
	utr->bIsTransferDone = 0;

	result = usbh_int_xfer(utr);
	slot->read_running = (result == FB_USBH_OK);
}

static void xpad_xbox_read_callback(FB_XBOX_UTR *utr)
{
	FB_XID_DEV *dev;
	FB_XBOX_XPAD_SLOT *slot;

	if (!utr)
		return;

	dev = (FB_XID_DEV *)utr->context;
	slot = xpad_xbox_find_slot(dev);
	if (!slot)
		return;

	/*
		A completed transfer must always release the slot, even when the USB
		stack reports an error.  Leaving read_running set after an error turns
		a transient interrupt-transfer failure into a permanent input stall:
		GETXPAD will continue to report a connected controller, but no future
		read will be queued and the button/axis state will stay at zero.
	*/
	slot->read_running = FALSE;

	if ((utr->status < 0) || !utr->buff) {
		slot->has_state = FALSE;
		return;
	}

	if (utr->xfer_len > 0) {
		xpad_xbox_copy_packet(&slot->state, utr->buff, utr->xfer_len);
		slot->has_state = TRUE;
	}

	xpad_xbox_requeue_read(slot, utr);
}

static void xpad_xbox_init(void)
{
	int i;

	if (xbox_usb_inited)
		return;

	usbh_core_init();
	usbh_xid_init();

	for (i = 0; i < FB_XBOX_INIT_POLL_MSECS; ++i) {
		usbh_pooling_hubs();
		fb_Delay(1);
	}

	xbox_usb_inited = TRUE;
}

static FB_XID_DEV *xpad_xbox_find_device(int id)
{
	FB_XID_DEV *dev;
	int index;

	dev = usbh_xid_get_device_list();
	index = 0;
	while (dev) {
		if (dev->xid_desc.bType == FB_XID_TYPE_GAMECONTROLLER) {
			if (index == id)
				return dev;
			++index;
		}
		dev = dev->next;
	}

	return NULL;
}

static void xpad_xbox_queue_read(FB_XBOX_XPAD_SLOT *slot)
{
	int32_t result;

	if (!slot->dev || slot->read_running)
		return;

	result = usbh_xid_read(slot->dev, 0, xpad_xbox_read_callback);
	if ((result == FB_USBH_OK) || (result == FB_HID_RET_XFER_IS_RUNNING))
		slot->read_running = TRUE;
}

static ssize_t xpad_xbox_buttons(const FB_XID_GAMEPAD_IN *pad)
{
	ssize_t buttons = 0;

	if (pad->a > 30)
		buttons |= XPAD_BUTTON_A;
	if (pad->b > 30)
		buttons |= XPAD_BUTTON_B;
	if (pad->x > 30)
		buttons |= XPAD_BUTTON_X;
	if (pad->y > 30)
		buttons |= XPAD_BUTTON_Y;
	if (pad->white > 30)
		buttons |= XPAD_BUTTON_L1;
	if (pad->black > 30)
		buttons |= XPAD_BUTTON_R1;
	if (pad->dButtons & (1u << 6))
		buttons |= XPAD_BUTTON_L3;
	if (pad->dButtons & (1u << 7))
		buttons |= XPAD_BUTTON_R3;
	if (pad->dButtons & (1u << 4))
		buttons |= XPAD_BUTTON_START;
	if (pad->dButtons & (1u << 5))
		buttons |= XPAD_BUTTON_SELECT;
	if (pad->l > 30)
		buttons |= XPAD_BUTTON_L2;
	if (pad->r > 30)
		buttons |= XPAD_BUTTON_R2;

	return buttons;
}

static ssize_t xpad_xbox_dpad(const FB_XID_GAMEPAD_IN *pad)
{
	ssize_t dpad = 0;

	if (pad->dButtons & (1u << 0))
		dpad |= XPAD_DPAD_UP;
	if (pad->dButtons & (1u << 1))
		dpad |= XPAD_DPAD_DOWN;
	if (pad->dButtons & (1u << 2))
		dpad |= XPAD_DPAD_LEFT;
	if (pad->dButtons & (1u << 3))
		dpad |= XPAD_DPAD_RIGHT;

	return dpad;
}

static int xpad_xbox_get(int id, ssize_t *buttons,
						 float *lstick_x, float *lstick_y,
						 float *rstick_x, float *rstick_y,
						 float *ltrigger, float *rtrigger,
						 ssize_t *dpad)
{
	FB_XBOX_XPAD_SLOT *slot;
	FB_XID_DEV *dev;

	if ((id < 0) || (id >= 4))
		return XPAD_STATUS_MISSING;

	xpad_xbox_init();
	usbh_pooling_hubs();

	slot = &xbox_xpad[id];
	dev = xpad_xbox_find_device(id);
	if (!dev) {
		slot->dev = NULL;
		slot->has_state = FALSE;
		slot->read_running = FALSE;
		return slot->seen ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;
	}

	slot->dev = dev;
	slot->uid = dev->uid;
	slot->seen = TRUE;
	xpad_xbox_queue_read(slot);

	if (buttons)
		*buttons = slot->has_state ? xpad_xbox_buttons(&slot->state) : 0;
	if (lstick_x)
		*lstick_x = slot->has_state ? xpad_normalize_axis(slot->state.leftStickX) : 0.0f;
	if (lstick_y)
		*lstick_y = slot->has_state ? xpad_normalize_axis(slot->state.leftStickY) : 0.0f;
	if (rstick_x)
		*rstick_x = slot->has_state ? xpad_normalize_axis(slot->state.rightStickX) : 0.0f;
	if (rstick_y)
		*rstick_y = slot->has_state ? xpad_normalize_axis(slot->state.rightStickY) : 0.0f;
	if (ltrigger)
		*ltrigger = slot->has_state ? ((float)slot->state.l / 255.0f) : 0.0f;
	if (rtrigger)
		*rtrigger = slot->has_state ? ((float)slot->state.r / 255.0f) : 0.0f;
	if (dpad)
		*dpad = slot->has_state ? xpad_xbox_dpad(&slot->state) : 0;

	return XPAD_STATUS_CONNECTED;
}

FBCALL int fb_GfxGetXPad(int id, ssize_t *buttons,
						 float *lstick_x, float *lstick_y,
						 float *rstick_x, float *rstick_y,
						 float *ltrigger, float *rtrigger,
						 ssize_t *dpad)
{
	int status;

	FB_GRAPHICS_LOCK( );

	xpad_clear_outputs(buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);

	if ((id < 0) || (id >= XPAD_MAX_DEVICES)) {
		FB_GRAPHICS_UNLOCK( );
		return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
	}

	status = xpad_xbox_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
	if (status != XPAD_STATUS_CONNECTED)
		xpad_clear_outputs(buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);

	fb_ErrorSetNum(FB_RTERROR_OK);
	FB_GRAPHICS_UNLOCK( );
	return status;
}

/* end of xbox/gfx_xpad.c */
