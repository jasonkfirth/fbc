/*
    FreeBASIC gfxlib2 controller polling
    ------------------------------------

    File: gfx_xpad.c

    Purpose:

        Expose Xbox-style controller state through GETXPAD.

    Responsibilities:

        - report whether a pad is present, connected, or disconnected
        - normalize stick axes to -1.0 through 1.0
        - normalize triggers to 0.0 through 1.0
        - map platform controller buttons into one stable bitfield

    This file intentionally does NOT contain:

        - event queue delivery
        - rumble output
        - keyboard or mouse input handling
*/

#include "fb_gfx.h"
#include <stdint.h>

#if defined(HOST_LINUX) || defined(HOST_FREEBSD) || defined(HOST_NETBSD) || \
	defined(HOST_DRAGONFLY)
#define FB_XPAD_HAS_JOYDEV
#endif

#if defined(HOST_WIN32)
#include <windows.h>
#endif

#if defined(FB_XPAD_HAS_JOYDEV)
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#if defined(HOST_JS)
#include <emscripten/emscripten.h>
#endif

#if defined(HOST_WII)
#include <ogc/pad.h>
#include <wiiuse/wpad.h>

/*
	GameCube controller polling on Wii goes through libogc's PAD subsystem.
	Dolphin currently trips an instruction-storage exception when this path is
	polled from the generic GETXPAD hook before a real GC controller is active.

	Keep the Wii GETXPAD path on WPAD for now.  Wiimote, Nunchuk, and Classic
	Controller input still travel through WPAD, and avoiding PAD_ScanPads()
	keeps ordinary BASIC code from crashing just because it asks for a portable
	controller state.
*/
#define FB_WII_ENABLE_GC_PAD 0
#endif

#define XPAD_MAX_DEVICES 16
#define XPAD_TRIGGER_DIGITAL_THRESHOLD (30.0f / 255.0f)

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

#if defined(FB_XPAD_HAS_JOYDEV) || defined(HOST_JS) || defined(HOST_WII)

static float xpad_clamp_unit(float value)
{
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

#endif

#if defined(HOST_WIN32)

#define FB_XINPUT_GAMEPAD_DPAD_UP          0x0001
#define FB_XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define FB_XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define FB_XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define FB_XINPUT_GAMEPAD_START            0x0010
#define FB_XINPUT_GAMEPAD_BACK             0x0020
#define FB_XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define FB_XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define FB_XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define FB_XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define FB_XINPUT_GAMEPAD_GUIDE            0x0400
#define FB_XINPUT_GAMEPAD_A                0x1000
#define FB_XINPUT_GAMEPAD_B                0x2000
#define FB_XINPUT_GAMEPAD_X                0x4000
#define FB_XINPUT_GAMEPAD_Y                0x8000

typedef struct FB_XINPUT_GAMEPAD_ {
	WORD wButtons;
	BYTE bLeftTrigger;
	BYTE bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT sThumbRX;
	SHORT sThumbRY;
} FB_XINPUT_GAMEPAD;

typedef struct FB_XINPUT_STATE_ {
	DWORD dwPacketNumber;
	FB_XINPUT_GAMEPAD Gamepad;
} FB_XINPUT_STATE;

typedef DWORD (WINAPI *FB_XINPUT_GET_STATE)(DWORD dwUserIndex, FB_XINPUT_STATE *pState);

static HMODULE xinput_module;
static FB_XINPUT_GET_STATE xinput_get_state;
static int xinput_load_tried;
static int xinput_seen[4];

static void xpad_win32_load_xinput(void)
{
	static const char *const dll_names[] = {
		"xinput1_4.dll",
		"xinput1_3.dll",
		"xinput9_1_0.dll",
		"xinput1_2.dll",
		"xinput1_1.dll",
		NULL
	};
	union {
		FARPROC proc;
		FB_XINPUT_GET_STATE get_state;
	} symbol;
	int i;

	if (xinput_load_tried)
		return;

	xinput_load_tried = TRUE;

	for (i = 0; dll_names[i]; ++i) {
		xinput_module = LoadLibraryA(dll_names[i]);
		if (!xinput_module)
			continue;

		symbol.proc = GetProcAddress(xinput_module, "XInputGetState");
		xinput_get_state = symbol.get_state;
		if (xinput_get_state)
			return;

		FreeLibrary(xinput_module);
		xinput_module = NULL;
	}
}

static ssize_t xpad_win32_buttons(const FB_XINPUT_GAMEPAD *pad)
{
	ssize_t buttons = 0;

	if (pad->wButtons & FB_XINPUT_GAMEPAD_A)
		buttons |= XPAD_BUTTON_A;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_B)
		buttons |= XPAD_BUTTON_B;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_X)
		buttons |= XPAD_BUTTON_X;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_Y)
		buttons |= XPAD_BUTTON_Y;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_LEFT_SHOULDER)
		buttons |= XPAD_BUTTON_L1;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_RIGHT_SHOULDER)
		buttons |= XPAD_BUTTON_R1;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_LEFT_THUMB)
		buttons |= XPAD_BUTTON_L3;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_RIGHT_THUMB)
		buttons |= XPAD_BUTTON_R3;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_START)
		buttons |= XPAD_BUTTON_START;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_BACK)
		buttons |= XPAD_BUTTON_SELECT;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_GUIDE)
		buttons |= XPAD_BUTTON_GUIDE;
	if (pad->bLeftTrigger > 30)
		buttons |= XPAD_BUTTON_L2;
	if (pad->bRightTrigger > 30)
		buttons |= XPAD_BUTTON_R2;

	return buttons;
}

static ssize_t xpad_win32_dpad(const FB_XINPUT_GAMEPAD *pad)
{
	ssize_t dpad = 0;

	if (pad->wButtons & FB_XINPUT_GAMEPAD_DPAD_UP)
		dpad |= XPAD_DPAD_UP;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_DPAD_RIGHT)
		dpad |= XPAD_DPAD_RIGHT;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_DPAD_DOWN)
		dpad |= XPAD_DPAD_DOWN;
	if (pad->wButtons & FB_XINPUT_GAMEPAD_DPAD_LEFT)
		dpad |= XPAD_DPAD_LEFT;

	return dpad;
}

static int xpad_win32_get(int id, ssize_t *buttons,
						  float *lstick_x, float *lstick_y,
						  float *rstick_x, float *rstick_y,
						  float *ltrigger, float *rtrigger,
						  ssize_t *dpad)
{
	FB_XINPUT_STATE state;
	DWORD result;

	if ((id < 0) || (id >= 4))
		return XPAD_STATUS_MISSING;

	xpad_win32_load_xinput();
	if (!xinput_get_state)
		return xinput_seen[id] ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;

	memset(&state, 0, sizeof(state));
	result = xinput_get_state((DWORD)id, &state);
	if (result != ERROR_SUCCESS)
		return xinput_seen[id] ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;

	xinput_seen[id] = TRUE;

	if (buttons)
		*buttons = xpad_win32_buttons(&state.Gamepad);
	if (lstick_x)
		*lstick_x = xpad_normalize_axis(state.Gamepad.sThumbLX);
	if (lstick_y)
		*lstick_y = xpad_normalize_axis(state.Gamepad.sThumbLY);
	if (rstick_x)
		*rstick_x = xpad_normalize_axis(state.Gamepad.sThumbRX);
	if (rstick_y)
		*rstick_y = xpad_normalize_axis(state.Gamepad.sThumbRY);
	if (ltrigger)
		*ltrigger = (float)state.Gamepad.bLeftTrigger / 255.0f;
	if (rtrigger)
		*rtrigger = (float)state.Gamepad.bRightTrigger / 255.0f;
	if (dpad)
		*dpad = xpad_win32_dpad(&state.Gamepad);

	return XPAD_STATUS_CONNECTED;
}

#endif

#if defined(FB_XPAD_HAS_JOYDEV)

#define JS_EVENT_BUTTON         0x01
#define JS_EVENT_AXIS           0x02
#define JS_EVENT_INIT           0x80
#define JSIOCGVERSION           _IOR('j', 0x01, unsigned int)

typedef struct FB_JS_EVENT_ {
	unsigned int time;
	short value;
	unsigned char type;
	unsigned char number;
} FB_JS_EVENT;

typedef struct FB_JOYDEV_XPAD_ {
	int fd;
	int seen;
	float axis[8];
	unsigned char axis_seen[8];
	uint32_t buttons;
} FB_JOYDEV_XPAD;

static FB_JOYDEV_XPAD joydev_xpad[XPAD_MAX_DEVICES];
static int joydev_xpad_inited;

static void xpad_joydev_init(void)
{
	int i;

	if (joydev_xpad_inited)
		return;

	memset(joydev_xpad, 0, sizeof(joydev_xpad));
	for (i = 0; i < XPAD_MAX_DEVICES; ++i)
		joydev_xpad[i].fd = -1;

	joydev_xpad_inited = TRUE;
}

static int xpad_joydev_open(int id)
{
	static const char *const device_path[] = {
		"/dev/input/js",
		"/dev/js",
		NULL
	};
	FB_JOYDEV_XPAD *pad;
	char device_name[32];
	unsigned int version;
	int i;

	pad = &joydev_xpad[id];
	if (pad->fd >= 0)
		return TRUE;

	for (i = 0; device_path[i]; ++i) {
		snprintf(device_name, sizeof(device_name), "%s%d", device_path[i], id);
		pad->fd = open(device_name, O_RDONLY | O_NONBLOCK);
		if (pad->fd < 0)
			continue;

		version = 0;
		if ((ioctl(pad->fd, JSIOCGVERSION, &version) < 0) || (version < 0x10000)) {
			close(pad->fd);
			pad->fd = -1;
			continue;
		}

		memset(pad->axis, 0, sizeof(pad->axis));
		memset(pad->axis_seen, 0, sizeof(pad->axis_seen));
		pad->buttons = 0;
		pad->seen = TRUE;
		return TRUE;
	}

	return FALSE;
}

static void xpad_joydev_close(FB_JOYDEV_XPAD *pad)
{
	if (pad->fd >= 0)
		close(pad->fd);
	pad->fd = -1;
	pad->buttons = 0;
	memset(pad->axis, 0, sizeof(pad->axis));
	memset(pad->axis_seen, 0, sizeof(pad->axis_seen));
}

static int xpad_joydev_poll(FB_JOYDEV_XPAD *pad)
{
	FB_JS_EVENT event;
	ssize_t bytes;

	while ((bytes = read(pad->fd, &event, sizeof(event))) == sizeof(event)) {
		switch (event.type & ~JS_EVENT_INIT) {
		case JS_EVENT_AXIS:
			if (event.number < 8) {
				pad->axis[event.number] = xpad_normalize_axis(event.value);
				pad->axis_seen[event.number] = TRUE;
			}
			break;

		case JS_EVENT_BUTTON:
			if (event.number < 32) {
				if (event.value)
					pad->buttons |= (1u << event.number);
				else
					pad->buttons &= ~(1u << event.number);
			}
			break;

		default:
			break;
		}
	}

	if ((bytes < 0) &&
		((errno == ENODEV) || (errno == ENXIO) || (errno == EIO))) {
		xpad_joydev_close(pad);
		return FALSE;
	}

	return TRUE;
}

static float xpad_joydev_trigger(FB_JOYDEV_XPAD *pad, int axis)
{
	if ((axis < 0) || (axis >= 8) || !pad->axis_seen[axis])
		return 0.0f;

	return xpad_clamp_unit((pad->axis[axis] + 1.0f) * 0.5f);
}

static ssize_t xpad_joydev_buttons(FB_JOYDEV_XPAD *pad)
{
	ssize_t buttons = 0;
	float left_trigger;
	float right_trigger;

	if (pad->buttons & (1u << 0))
		buttons |= XPAD_BUTTON_A;
	if (pad->buttons & (1u << 1))
		buttons |= XPAD_BUTTON_B;
	if (pad->buttons & (1u << 2))
		buttons |= XPAD_BUTTON_X;
	if (pad->buttons & (1u << 3))
		buttons |= XPAD_BUTTON_Y;
	if (pad->buttons & (1u << 4))
		buttons |= XPAD_BUTTON_L1;
	if (pad->buttons & (1u << 5))
		buttons |= XPAD_BUTTON_R1;
	if (pad->buttons & (1u << 6))
		buttons |= XPAD_BUTTON_SELECT;
	if (pad->buttons & (1u << 7))
		buttons |= XPAD_BUTTON_START;
	if (pad->buttons & (1u << 8))
		buttons |= XPAD_BUTTON_GUIDE;
	if (pad->buttons & (1u << 9))
		buttons |= XPAD_BUTTON_L3;
	if (pad->buttons & (1u << 10))
		buttons |= XPAD_BUTTON_R3;

	left_trigger = xpad_joydev_trigger(pad, 2);
	right_trigger = xpad_joydev_trigger(pad, 5);
	if (left_trigger > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_L2;
	if (right_trigger > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_R2;

	return buttons;
}

static ssize_t xpad_joydev_dpad(FB_JOYDEV_XPAD *pad)
{
	ssize_t dpad = 0;

	if (pad->axis_seen[7]) {
		if (pad->axis[7] < -0.5f)
			dpad |= XPAD_DPAD_UP;
		else if (pad->axis[7] > 0.5f)
			dpad |= XPAD_DPAD_DOWN;
	}

	if (pad->axis_seen[6]) {
		if (pad->axis[6] < -0.5f)
			dpad |= XPAD_DPAD_LEFT;
		else if (pad->axis[6] > 0.5f)
			dpad |= XPAD_DPAD_RIGHT;
	}

	return dpad;
}

static int xpad_joydev_get(int id, ssize_t *buttons,
						  float *lstick_x, float *lstick_y,
						  float *rstick_x, float *rstick_y,
						  float *ltrigger, float *rtrigger,
						  ssize_t *dpad)
{
	FB_JOYDEV_XPAD *pad;

	if ((id < 0) || (id >= XPAD_MAX_DEVICES))
		return XPAD_STATUS_MISSING;

	xpad_joydev_init();
	pad = &joydev_xpad[id];

	if (!xpad_joydev_open(id))
		return pad->seen ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;

	if (!xpad_joydev_poll(pad))
		return XPAD_STATUS_DISCONNECTED;

	if (buttons)
		*buttons = xpad_joydev_buttons(pad);
	if (lstick_x)
		*lstick_x = pad->axis_seen[0] ? pad->axis[0] : 0.0f;
	if (lstick_y)
		*lstick_y = pad->axis_seen[1] ? pad->axis[1] : 0.0f;
	if (rstick_x)
		*rstick_x = pad->axis_seen[3] ? pad->axis[3] : 0.0f;
	if (rstick_y)
		*rstick_y = pad->axis_seen[4] ? pad->axis[4] : 0.0f;
	if (ltrigger)
		*ltrigger = xpad_joydev_trigger(pad, 2);
	if (rtrigger)
		*rtrigger = xpad_joydev_trigger(pad, 5);
	if (dpad)
		*dpad = xpad_joydev_dpad(pad);

	return XPAD_STATUS_CONNECTED;
}

#endif

#if defined(HOST_XBOX)

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

#endif

#if defined(HOST_JS)

#define FB_JS_ENABLE_GAMEPAD_API 0

#if FB_JS_ENABLE_GAMEPAD_API

static int js_xpad_seen[XPAD_MAX_DEVICES];

static int xpad_js_connected(int id)
{
	return EM_ASM_INT({
		var pads;
		if ((typeof navigator === 'undefined') || !navigator.getGamepads)
			return 0;
		pads = navigator.getGamepads();
		return (pads[$0] && pads[$0].connected) ? 1 : 0;
	}, id);
}

static float xpad_js_axis(int id, int axis)
{
	return (float)EM_ASM_DOUBLE({
		var pads;
		var pad;
		if ((typeof navigator === 'undefined') || !navigator.getGamepads)
			return 0.0;
		pads = navigator.getGamepads();
		pad = pads[$0];
		if (!pad || !pad.connected || (pad.axes.length <= $1))
			return 0.0;
		return pad.axes[$1];
	}, id, axis);
}

static float xpad_js_button_value(int id, int button)
{
	return (float)EM_ASM_DOUBLE({
		var pads;
		var pad;
		var b;
		if ((typeof navigator === 'undefined') || !navigator.getGamepads)
			return 0.0;
		pads = navigator.getGamepads();
		pad = pads[$0];
		if (!pad || !pad.connected || (pad.buttons.length <= $1))
			return 0.0;
		b = pad.buttons[$1];
		return (typeof b === 'object') ? b.value : b;
	}, id, button);
}

static int xpad_js_button_pressed(int id, int button)
{
	return EM_ASM_INT({
		var pads;
		var pad;
		var b;
		if ((typeof navigator === 'undefined') || !navigator.getGamepads)
			return 0;
		pads = navigator.getGamepads();
		pad = pads[$0];
		if (!pad || !pad.connected || (pad.buttons.length <= $1))
			return 0;
		b = pad.buttons[$1];
		return ((typeof b === 'object') ? b.pressed : (b > 0.5)) ? 1 : 0;
	}, id, button);
}

static ssize_t xpad_js_buttons(int id)
{
	ssize_t buttons = 0;
	float left_trigger;
	float right_trigger;

	if (xpad_js_button_pressed(id, 0))
		buttons |= XPAD_BUTTON_A;
	if (xpad_js_button_pressed(id, 1))
		buttons |= XPAD_BUTTON_B;
	if (xpad_js_button_pressed(id, 2))
		buttons |= XPAD_BUTTON_X;
	if (xpad_js_button_pressed(id, 3))
		buttons |= XPAD_BUTTON_Y;
	if (xpad_js_button_pressed(id, 4))
		buttons |= XPAD_BUTTON_L1;
	if (xpad_js_button_pressed(id, 5))
		buttons |= XPAD_BUTTON_R1;
	if (xpad_js_button_pressed(id, 10))
		buttons |= XPAD_BUTTON_L3;
	if (xpad_js_button_pressed(id, 11))
		buttons |= XPAD_BUTTON_R3;
	if (xpad_js_button_pressed(id, 9))
		buttons |= XPAD_BUTTON_START;
	if (xpad_js_button_pressed(id, 8))
		buttons |= XPAD_BUTTON_SELECT;
	if (xpad_js_button_pressed(id, 16))
		buttons |= XPAD_BUTTON_GUIDE;

	left_trigger = xpad_js_button_value(id, 6);
	right_trigger = xpad_js_button_value(id, 7);
	if (left_trigger > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_L2;
	if (right_trigger > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_R2;

	return buttons;
}

static ssize_t xpad_js_dpad(int id)
{
	ssize_t dpad = 0;

	if (xpad_js_button_pressed(id, 12))
		dpad |= XPAD_DPAD_UP;
	if (xpad_js_button_pressed(id, 15))
		dpad |= XPAD_DPAD_RIGHT;
	if (xpad_js_button_pressed(id, 13))
		dpad |= XPAD_DPAD_DOWN;
	if (xpad_js_button_pressed(id, 14))
		dpad |= XPAD_DPAD_LEFT;

	return dpad;
}

static int xpad_js_get(int id, ssize_t *buttons,
					   float *lstick_x, float *lstick_y,
					   float *rstick_x, float *rstick_y,
					   float *ltrigger, float *rtrigger,
					   ssize_t *dpad)
{
	if ((id < 0) || (id >= XPAD_MAX_DEVICES))
		return XPAD_STATUS_MISSING;

	if (!xpad_js_connected(id))
		return js_xpad_seen[id] ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;

	js_xpad_seen[id] = TRUE;

	if (buttons)
		*buttons = xpad_js_buttons(id);
	if (lstick_x)
		*lstick_x = xpad_js_axis(id, 0);
	if (lstick_y)
		*lstick_y = xpad_js_axis(id, 1);
	if (rstick_x)
		*rstick_x = xpad_js_axis(id, 2);
	if (rstick_y)
		*rstick_y = xpad_js_axis(id, 3);
	if (ltrigger)
		*ltrigger = xpad_clamp_unit(xpad_js_button_value(id, 6));
	if (rtrigger)
		*rtrigger = xpad_clamp_unit(xpad_js_button_value(id, 7));
	if (dpad)
		*dpad = xpad_js_dpad(id);

	return XPAD_STATUS_CONNECTED;
}

#else

static int xpad_js_get(int id, ssize_t *buttons,
					   float *lstick_x, float *lstick_y,
					   float *rstick_x, float *rstick_y,
					   float *ltrigger, float *rtrigger,
					   ssize_t *dpad)
{
	(void)id;
	(void)buttons;
	(void)lstick_x;
	(void)lstick_y;
	(void)rstick_x;
	(void)rstick_y;
	(void)ltrigger;
	(void)rtrigger;
	(void)dpad;

	/*
		The browser Gamepad API is only exposed after user activation, and the
		current EM_ASM bridge can trap inside wasm on hard validation programs
		when it is polled before a browser has made a pad visible.

		Keep GETXPAD safe for portable programs by reporting "missing" instead
		of crashing.  Keyboard and mouse input remain available on JS, and this
		backend can be re-enabled once it has a non-trapping Gamepad bridge.
	*/
	return XPAD_STATUS_MISSING;
}

#endif

#endif

#if defined(HOST_WII)

static int wii_xpad_seen[4];
static gforce_t wii_xpad_last_gforce[4];
static int wii_xpad_have_gforce[4];

static float xpad_wii_gc_axis(s8 value)
{
	return xpad_normalize_axis((int)value * 256);
}

static float xpad_wii_absf(float value)
{
	return (value < 0.0f) ? -value : value;
}

static float xpad_wii_signed_unit(float value)
{
	if (value < -1.0f)
		return -1.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static void xpad_wii_dpad_as_stick(ssize_t dpad, float *x, float *y)
{
	if (x) {
		if (dpad & XPAD_DPAD_LEFT)
			*x = -1.0f;
		else if (dpad & XPAD_DPAD_RIGHT)
			*x = 1.0f;
		else
			*x = 0.0f;
	}

	if (y) {
		if (dpad & XPAD_DPAD_UP)
			*y = 1.0f;
		else if (dpad & XPAD_DPAD_DOWN)
			*y = -1.0f;
		else
			*y = 0.0f;
	}
}

static ssize_t xpad_wii_motion(int id, float *x, float *y)
{
	gforce_t gforce;
	float change;
	ssize_t buttons = 0;

	gforce.x = 0.0f;
	gforce.y = 0.0f;
	gforce.z = 0.0f;
	WPAD_GForce(id, &gforce);

	/*
		There is no standard Xbox-style motion control, but exposing Wiimote
		tilt on the right stick and shake on the thumb buttons gives portable
		BASIC programs a simple way to notice "waggling" through GETXPAD.
		The thresholds are intentionally loose because real controllers, bars,
		and emulators all report slightly different accelerometer noise.
	*/
	if (x)
		*x = xpad_wii_signed_unit(gforce.x * 0.5f);
	if (y)
		*y = xpad_wii_signed_unit(-gforce.y * 0.5f);

	if (wii_xpad_have_gforce[id]) {
		change = xpad_wii_absf(gforce.x - wii_xpad_last_gforce[id].x);
		change += xpad_wii_absf(gforce.y - wii_xpad_last_gforce[id].y);
		change += xpad_wii_absf(gforce.z - wii_xpad_last_gforce[id].z);

		if (change > 0.75f)
			buttons |= XPAD_BUTTON_L3;
		if (change > 1.50f)
			buttons |= XPAD_BUTTON_R3;
	}

	wii_xpad_last_gforce[id] = gforce;
	wii_xpad_have_gforce[id] = TRUE;
	return buttons;
}

static int xpad_wii_connected(int id)
{
	u32 wpad_type;
	u32 gc_mask = 0;

	if ((id < 0) || (id >= 4))
		return FALSE;

#if FB_WII_ENABLE_GC_PAD
	gc_mask = PAD_ScanPads();
#endif
	if (gc_mask & (PAD_CHAN0_BIT >> id))
		return TRUE;

	return (WPAD_Probe(id, &wpad_type) == WPAD_ERR_NONE);
}

static ssize_t xpad_wii_buttons(int id)
{
	ssize_t buttons = 0;
	u32 wpad_buttons = WPAD_ButtonsHeld(id);
	u16 gc_buttons = 0;

#if FB_WII_ENABLE_GC_PAD
	gc_buttons = PAD_ButtonsHeld(id);
#endif

	if (wpad_buttons & WPAD_BUTTON_A)
		buttons |= XPAD_BUTTON_A;
	if (wpad_buttons & WPAD_BUTTON_B)
		buttons |= XPAD_BUTTON_B;
	if (wpad_buttons & WPAD_BUTTON_1)
		buttons |= XPAD_BUTTON_X;
	if (wpad_buttons & WPAD_BUTTON_2)
		buttons |= XPAD_BUTTON_Y;
	if (wpad_buttons & WPAD_BUTTON_PLUS)
		buttons |= XPAD_BUTTON_START;
	if (wpad_buttons & WPAD_BUTTON_MINUS)
		buttons |= XPAD_BUTTON_SELECT;
	if (wpad_buttons & WPAD_BUTTON_HOME)
		buttons |= XPAD_BUTTON_GUIDE;
	if (wpad_buttons & WPAD_NUNCHUK_BUTTON_C)
		buttons |= XPAD_BUTTON_L1;
	if (wpad_buttons & WPAD_NUNCHUK_BUTTON_Z)
		buttons |= XPAD_BUTTON_R1;

#ifdef WPAD_CLASSIC_BUTTON_A
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_A)
		buttons |= XPAD_BUTTON_A;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_B)
		buttons |= XPAD_BUTTON_B;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_X)
		buttons |= XPAD_BUTTON_X;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_Y)
		buttons |= XPAD_BUTTON_Y;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_ZL)
		buttons |= XPAD_BUTTON_L2;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_ZR)
		buttons |= XPAD_BUTTON_R2;
#endif

	if (gc_buttons & PAD_BUTTON_A)
		buttons |= XPAD_BUTTON_A;
	if (gc_buttons & PAD_BUTTON_B)
		buttons |= XPAD_BUTTON_B;
	if (gc_buttons & PAD_BUTTON_X)
		buttons |= XPAD_BUTTON_X;
	if (gc_buttons & PAD_BUTTON_Y)
		buttons |= XPAD_BUTTON_Y;
	if (gc_buttons & PAD_BUTTON_START)
		buttons |= XPAD_BUTTON_START;
	if (gc_buttons & PAD_TRIGGER_L)
		buttons |= XPAD_BUTTON_L1;
	if (gc_buttons & PAD_TRIGGER_R)
		buttons |= XPAD_BUTTON_R1;
	if (gc_buttons & PAD_TRIGGER_Z)
		buttons |= XPAD_BUTTON_R2;

#if FB_WII_ENABLE_GC_PAD
	if (((float)PAD_TriggerL(id) / 255.0f) > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_L2;
	if (((float)PAD_TriggerR(id) / 255.0f) > XPAD_TRIGGER_DIGITAL_THRESHOLD)
		buttons |= XPAD_BUTTON_R2;
#endif

	return buttons;
}

static ssize_t xpad_wii_dpad(int id)
{
	ssize_t dpad = 0;
	u32 wpad_buttons = WPAD_ButtonsHeld(id);
	u16 gc_buttons = 0;

#if FB_WII_ENABLE_GC_PAD
	gc_buttons = PAD_ButtonsHeld(id);
#endif

	if ((wpad_buttons & WPAD_BUTTON_UP) || (gc_buttons & PAD_BUTTON_UP))
		dpad |= XPAD_DPAD_UP;
	if ((wpad_buttons & WPAD_BUTTON_RIGHT) || (gc_buttons & PAD_BUTTON_RIGHT))
		dpad |= XPAD_DPAD_RIGHT;
	if ((wpad_buttons & WPAD_BUTTON_DOWN) || (gc_buttons & PAD_BUTTON_DOWN))
		dpad |= XPAD_DPAD_DOWN;
	if ((wpad_buttons & WPAD_BUTTON_LEFT) || (gc_buttons & PAD_BUTTON_LEFT))
		dpad |= XPAD_DPAD_LEFT;

#ifdef WPAD_CLASSIC_BUTTON_UP
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_UP)
		dpad |= XPAD_DPAD_UP;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_RIGHT)
		dpad |= XPAD_DPAD_RIGHT;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_DOWN)
		dpad |= XPAD_DPAD_DOWN;
	if (wpad_buttons & WPAD_CLASSIC_BUTTON_LEFT)
		dpad |= XPAD_DPAD_LEFT;
#endif

	return dpad;
}

static int xpad_wii_get(int id, ssize_t *buttons,
					   float *lstick_x, float *lstick_y,
					   float *rstick_x, float *rstick_y,
					   float *ltrigger, float *rtrigger,
					   ssize_t *dpad)
{
	int gc_connected;
	int wpad_connected;
	u32 wpad_type;
	u32 wpad_buttons;
	ssize_t button_value;
	ssize_t dpad_value;
	ssize_t motion_buttons;
	float motion_x;
	float motion_y;
	float left_trigger;
	float right_trigger;

	if ((id < 0) || (id >= 4))
		return XPAD_STATUS_MISSING;

	WPAD_ScanPads();

	if (!xpad_wii_connected(id)) {
		wii_xpad_have_gforce[id] = FALSE;
		return wii_xpad_seen[id] ? XPAD_STATUS_DISCONNECTED : XPAD_STATUS_MISSING;
	}

	wii_xpad_seen[id] = TRUE;

	gc_connected = FALSE;
#if FB_WII_ENABLE_GC_PAD
	gc_connected = ((PAD_ScanPads() & (PAD_CHAN0_BIT >> id)) != 0);
#endif
	wpad_connected = (WPAD_Probe(id, &wpad_type) == WPAD_ERR_NONE);
	if (!wpad_connected)
		wii_xpad_have_gforce[id] = FALSE;
	wpad_buttons = wpad_connected ? WPAD_ButtonsHeld(id) : 0;
	dpad_value = xpad_wii_dpad(id);
	button_value = xpad_wii_buttons(id);
	motion_x = 0.0f;
	motion_y = 0.0f;
	motion_buttons = wpad_connected ? xpad_wii_motion(id, &motion_x, &motion_y) : 0;
	button_value |= motion_buttons;

	if (buttons)
		*buttons = button_value;

	if (gc_connected) {
		if (lstick_x)
			*lstick_x = xpad_wii_gc_axis(PAD_StickX(id));
		if (lstick_y)
			*lstick_y = xpad_wii_gc_axis(PAD_StickY(id));
		if (rstick_x)
			*rstick_x = xpad_wii_gc_axis(PAD_SubStickX(id));
		if (rstick_y)
			*rstick_y = xpad_wii_gc_axis(PAD_SubStickY(id));
		left_trigger = xpad_clamp_unit((float)PAD_TriggerL(id) / 255.0f);
		right_trigger = xpad_clamp_unit((float)PAD_TriggerR(id) / 255.0f);
	} else {
		xpad_wii_dpad_as_stick(dpad_value, lstick_x, lstick_y);
		if (rstick_x)
			*rstick_x = motion_x;
		if (rstick_y)
			*rstick_y = motion_y;
		left_trigger = 0.0f;
		right_trigger = 0.0f;
	}

	if (wpad_buttons & WPAD_BUTTON_B)
		left_trigger = 1.0f;
	if (wpad_buttons & WPAD_BUTTON_A)
		right_trigger = 1.0f;

	if (ltrigger)
		*ltrigger = left_trigger;
	if (rtrigger)
		*rtrigger = right_trigger;
	if (dpad)
		*dpad = dpad_value;

	return XPAD_STATUS_CONNECTED;
}

#endif

static int xpad_platform_get(int id, ssize_t *buttons,
							 float *lstick_x, float *lstick_y,
							 float *rstick_x, float *rstick_y,
							 float *ltrigger, float *rtrigger,
							 ssize_t *dpad)
{
#if defined(HOST_WIN32)
	return xpad_win32_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
#elif defined(FB_XPAD_HAS_JOYDEV)
	return xpad_joydev_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
#elif defined(HOST_XBOX)
	return xpad_xbox_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
#elif defined(HOST_JS)
	return xpad_js_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
#elif defined(HOST_WII)
	return xpad_wii_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
#else
	(void)id;
	(void)buttons;
	(void)lstick_x;
	(void)lstick_y;
	(void)rstick_x;
	(void)rstick_y;
	(void)ltrigger;
	(void)rtrigger;
	(void)dpad;
	return XPAD_STATUS_MISSING;
#endif
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

	status = xpad_platform_get(id, buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);
	if (status != XPAD_STATUS_CONNECTED)
		xpad_clear_outputs(buttons, lstick_x, lstick_y, rstick_x, rstick_y, ltrigger, rtrigger, dpad);

	fb_ErrorSetNum(FB_RTERROR_OK);
	FB_GRAPHICS_UNLOCK( );
	return status;
}

/* end of gfx_xpad.c */
