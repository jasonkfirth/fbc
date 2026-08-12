/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: android/gfx3_platform.c

    Purpose:

        Bind gfxlib3's render thread to the ANativeWindow supplied by the
        packaged FreeBASIC NativeActivity and create an EGL/GLES context.

    Responsibilities:

        - retain Android window objects across NativeActivity callbacks
		- create and own an OpenGL ES 3.0 EGL context on the render thread
		- expose the ANativeWindow handle to the Vulkan backend
		- publish Android focus, touch, and keyboard state to gfxlib3 input
		- satisfy the package Java input bridge's native entry points
		- swap the EGL window surface and report its current dimensions

    This file intentionally does NOT contain:

        - GLES rendering commands or shader programs
		- Vulkan surface creation
		- Java activity, looper, or APK packaging code
*/

#include "../gfx3_platform.h"

#include <string.h>

#if defined(HOST_ANDROID)

#include "../gfx3_input.h"

#include <EGL/egl.h>
#include <android/input.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <jni.h>
#include <pthread.h>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#ifndef AMOTION_EVENT_ACTION_POINTER_INDEX_MASK
#define AMOTION_EVENT_ACTION_POINTER_INDEX_MASK 0x0000ff00
#endif

#ifndef AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT
#define AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT 8
#endif

#define FB_GFX3_ANDROID_GAMEPAD_HAT_THRESHOLD 0.50f
#define FB_GFX3_ANDROID_KEYBOARD_BUTTON_WIDTH 56
#define FB_GFX3_ANDROID_KEYBOARD_BUTTON_HEIGHT 40

typedef struct FB_GFX3_ANDROID_PLATFORM {
	ANativeWindow *window;
	FB_GFX3_INPUT_STATE *input;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int owns_egl;
} FB_GFX3_ANDROID_PLATFORM;

/*
	NativeActivity callbacks and the renderer run on different threads.  The
	window reference is always retained while stored here, so a renderer which
	is starting at the same time as a surface callback cannot receive a dangling
	ANativeWindow pointer.  Activity and keyboard-policy state are also kept
	under this lock because package callbacks may update them while the graphics
	thread is opening or closing a mode.  The policy fields are the native side
	of the package ABI.  GLES reads the published button state during
	presentation and draws the optional control after converting the BASIC
	surface; Java owns only the hidden IME bridge.
*/
static pthread_mutex_t android_window_mutex = PTHREAD_MUTEX_INITIALIZER;
static ANativeWindow *android_window;
static ANativeActivity *android_activity;
static FB_GFX3_INPUT_STATE *android_input;
static int android_started;
static int android_resumed;
static int android_focused;
static int android_keyboard_enabled = TRUE;
static int android_keyboard_button_visible = TRUE;
static int android_keyboard_visible;
static int android_keyboard_button_down;
static int android_keyboard_button_pointer_id = -1;

static int android_request_keyboard_visibility(ANativeActivity *activity,
	int visible);
static int android_keyboard_button_rect_locked(int window_width,
	int window_height, int *x0, int *y0, int *x1, int *y1);

static ANativeWindow *android_retain_window(void)
{
	ANativeWindow *window;

	pthread_mutex_lock(&android_window_mutex);
	window = android_window;
	if (window != NULL)
		ANativeWindow_acquire(window);
	pthread_mutex_unlock(&android_window_mutex);
	return window;
}

void fb_hAndroidSetWindow(ANativeWindow *window)
{
	ANativeWindow *previous;

	if (window != NULL)
		ANativeWindow_acquire(window);
	pthread_mutex_lock(&android_window_mutex);
	previous = android_window;
	android_window = window;
	pthread_mutex_unlock(&android_window_mutex);
	if (previous != NULL)
		ANativeWindow_release(previous);
}

void fb_hAndroidSetActivity(ANativeActivity *activity)
{
	int hide_keyboard;

	pthread_mutex_lock(&android_window_mutex);
	android_activity = activity;
	if (activity == NULL) {
		android_keyboard_visible = FALSE;
		android_keyboard_button_down = FALSE;
		android_keyboard_button_pointer_id = -1;
	}
	hide_keyboard = (activity != NULL) && !android_keyboard_enabled;
	pthread_mutex_unlock(&android_window_mutex);
	if (hide_keyboard)
		android_request_keyboard_visibility(activity, FALSE);
}

void fb_hAndroidSetKeyboardEnabled(int enabled)
{
	ANativeActivity *activity;
	int hide_keyboard;

	pthread_mutex_lock(&android_window_mutex);
	android_keyboard_enabled = enabled != FALSE;
	if (!android_keyboard_enabled) {
		android_keyboard_button_visible = FALSE;
		android_keyboard_button_down = FALSE;
		android_keyboard_button_pointer_id = -1;
	}
	activity = android_activity;
	hide_keyboard = (activity != NULL) && !android_keyboard_enabled;
	pthread_mutex_unlock(&android_window_mutex);
	if (hide_keyboard)
		android_request_keyboard_visibility(activity, FALSE);
}

void fb_hAndroidSetKeyboardButtonVisible(int visible)
{
	ANativeActivity *activity;
	int hide_keyboard;

	pthread_mutex_lock(&android_window_mutex);
	android_keyboard_button_visible = visible != FALSE;
	if (!android_keyboard_button_visible) {
		android_keyboard_enabled = FALSE;
		android_keyboard_button_down = FALSE;
		android_keyboard_button_pointer_id = -1;
	}
	activity = android_activity;
	hide_keyboard = (activity != NULL) && !android_keyboard_enabled;
	pthread_mutex_unlock(&android_window_mutex);
	if (hide_keyboard)
		android_request_keyboard_visibility(activity, FALSE);
}

/*
	The control deliberately uses native-window pixels, as gfxlib2 does.  The
	logical renderer may stretch a 320 by 200 page across the screen, but this
	presentation-only target remains comfortably touchable on a small device and
	never becomes part of the program's logical framebuffer.
*/
static int android_keyboard_button_rect_locked(int window_width,
	int window_height, int *x0, int *y0, int *x1, int *y1)
{
	int left;
	int right;
	int bottom;

	if ((window_width <= 0) || (window_height <= 0) ||
	    !android_keyboard_button_visible)
		return FALSE;
	left = window_width - FB_GFX3_ANDROID_KEYBOARD_BUTTON_WIDTH;
	right = window_width - 8;
	bottom = 8 + FB_GFX3_ANDROID_KEYBOARD_BUTTON_HEIGHT;
	if (left < 0)
		left = 0;
	if (right > window_width)
		right = window_width;
	if (bottom > window_height)
		bottom = window_height;
	if ((left >= right) || (8 >= bottom) || ((right - left) < 8))
		return FALSE;
	*x0 = left;
	*y0 = 8;
	*x1 = right;
	*y1 = bottom;
	return TRUE;
}

int fb_hAndroidKeyboardButtonHit(float x, float y)
{
	int window_width;
	int window_height;
	int x0;
	int y0;
	int x1;
	int y1;
	int hit = FALSE;

	pthread_mutex_lock(&android_window_mutex);
	window_width = (android_window != NULL) ?
		ANativeWindow_getWidth(android_window) : 0;
	window_height = (android_window != NULL) ?
		ANativeWindow_getHeight(android_window) : 0;
	if (android_keyboard_button_rect_locked(window_width, window_height,
	    &x0, &y0, &x1, &y1))
		hit = ((int)x >= x0) && ((int)x < x1) &&
			((int)y >= y0) && ((int)y < y1);
	pthread_mutex_unlock(&android_window_mutex);
	return hit;
}

/*
	The package owns the Java InputMethodManager interaction because Android
	requires UI-view changes on its activity thread.  The renderer only asks the
	activity to change visibility; Java owns the hidden EditText that receives
	IME commits and forwards them through the existing nativeDispatchImeKey ABI.
*/
static JNIEnv *android_get_jni_env(ANativeActivity *activity, int *attached)
{
	JNIEnv *environment = NULL;
	jint status;

	if (attached != NULL)
		*attached = FALSE;
	if ((activity == NULL) || (activity->vm == NULL))
		return NULL;
	status = (*activity->vm)->GetEnv(activity->vm, (void **)&environment,
		JNI_VERSION_1_6);
	if (status == JNI_EDETACHED) {
		if ((*activity->vm)->AttachCurrentThread(activity->vm,
		    (JNIEnv **)&environment, NULL) != JNI_OK)
			return NULL;
		if (attached != NULL)
			*attached = TRUE;
	} else if (status != JNI_OK) {
		return NULL;
	}
	return environment;
}

static void android_release_jni_env(ANativeActivity *activity, int attached)
{
	if (attached && (activity != NULL) && (activity->vm != NULL))
		(*activity->vm)->DetachCurrentThread(activity->vm);
}

static int android_request_keyboard_visibility(ANativeActivity *activity,
	int visible)
{
	JNIEnv *environment;
	jclass activity_class;
	jmethodID method;
	int attached = FALSE;
	int result = FALSE;

	if ((activity == NULL) || (activity->clazz == NULL))
		return FALSE;
	environment = android_get_jni_env(activity, &attached);
	if (environment == NULL)
		return FALSE;
	activity_class = (*environment)->GetObjectClass(environment,
		activity->clazz);
	if (activity_class == NULL)
		goto done;
	method = (*environment)->GetMethodID(environment, activity_class,
		"setKeyboardVisibleFromNative", "(Z)V");
	if (method == NULL) {
		(*environment)->ExceptionClear(environment);
		goto done;
	}
	(*environment)->CallVoidMethod(environment, activity->clazz, method,
		visible ? JNI_TRUE : JNI_FALSE);
	if ((*environment)->ExceptionCheck(environment)) {
		(*environment)->ExceptionClear(environment);
		goto done;
	}
	result = TRUE;

done:
	if (activity_class != NULL)
		(*environment)->DeleteLocalRef(environment, activity_class);
	android_release_jni_env(activity, attached);
	return result;
}

void fb_hAndroidToggleKeyboard(void)
{
	ANativeActivity *activity;
	int visible;

	pthread_mutex_lock(&android_window_mutex);
	activity = android_activity;
	if ((activity == NULL) || !android_keyboard_enabled) {
		pthread_mutex_unlock(&android_window_mutex);
		return;
	}
	visible = !android_keyboard_visible;
	android_keyboard_visible = visible;
	pthread_mutex_unlock(&android_window_mutex);
	if (!android_request_keyboard_visibility(activity, visible)) {
		pthread_mutex_lock(&android_window_mutex);
		if (android_activity == activity)
			android_keyboard_visible = !visible;
		pthread_mutex_unlock(&android_window_mutex);
	}
}

/*
	A keyboard-target touch is consumed before coordinate scaling.  Letting it
	enter the normal touch map would create a spurious BASIC mouse click in the
	upper-right corner whenever a program merely asks Android to show its IME.
*/
static int android_keyboard_button_event_locked(float x, float y, int action,
	int pointer_id, int *toggle_keyboard)
{
	int window_width;
	int window_height;
	int x0;
	int y0;
	int x1;
	int y1;
	int hit;

	if (toggle_keyboard != NULL)
		*toggle_keyboard = FALSE;
	window_width = (android_window != NULL) ?
		ANativeWindow_getWidth(android_window) : 0;
	window_height = (android_window != NULL) ?
		ANativeWindow_getHeight(android_window) : 0;
	if (!android_keyboard_button_rect_locked(window_width, window_height,
	    &x0, &y0, &x1, &y1))
		return FALSE;
	hit = ((int)x >= x0) && ((int)x < x1) &&
		((int)y >= y0) && ((int)y < y1);
	if ((action == AMOTION_EVENT_ACTION_DOWN) ||
	    (action == AMOTION_EVENT_ACTION_POINTER_DOWN)) {
		if (!hit || (android_keyboard_button_pointer_id >= 0))
			return FALSE;
		android_keyboard_button_down = TRUE;
		android_keyboard_button_pointer_id = pointer_id;
		return TRUE;
	}
	if (pointer_id != android_keyboard_button_pointer_id)
		return FALSE;
	if (action == AMOTION_EVENT_ACTION_MOVE) {
		if (!hit)
			android_keyboard_button_down = FALSE;
		return TRUE;
	}
	if ((action == AMOTION_EVENT_ACTION_UP) ||
	    (action == AMOTION_EVENT_ACTION_POINTER_UP)) {
		if (android_keyboard_button_down && hit &&
	    (toggle_keyboard != NULL))
			*toggle_keyboard = TRUE;
		android_keyboard_button_down = FALSE;
		android_keyboard_button_pointer_id = -1;
		return TRUE;
	}
	if (action == AMOTION_EVENT_ACTION_CANCEL) {
		android_keyboard_button_down = FALSE;
		android_keyboard_button_pointer_id = -1;
		return TRUE;
	}
	return FALSE;
}

void fb_hAndroidGfxSetLifecycle(int started, int resumed, int focused)
{
	pthread_mutex_lock(&android_window_mutex);
	android_started = started != FALSE;
	android_resumed = resumed != FALSE;
	android_focused = focused != FALSE;
	if (android_input != NULL)
		fb_gfx3_input_platform_focus(android_input, focused != FALSE);
	pthread_mutex_unlock(&android_window_mutex);
}

int fb_hAndroidIsGraphicsActive(void)
{
	int active;

	pthread_mutex_lock(&android_window_mutex);
	active = android_started && android_resumed && android_focused &&
		(android_window != NULL);
	pthread_mutex_unlock(&android_window_mutex);
	return active;
}

void fb_hAndroidTouch(float x, float y, int action)
{
	FB_GFX3_TOUCH_CONTACT contact;
	int logical_x;
	int logical_y;
	int window_width;
	int window_height;
	int active;
	int previous_count;
	int toggle_keyboard;

	pthread_mutex_lock(&android_window_mutex);
	if (android_input == NULL) {
		pthread_mutex_unlock(&android_window_mutex);
		return;
	}
	if (android_keyboard_button_event_locked(x, y, action, 0,
	    &toggle_keyboard)) {
		pthread_mutex_unlock(&android_window_mutex);
		if (toggle_keyboard)
			fb_hAndroidToggleKeyboard();
		return;
	}
	window_width = (android_window != NULL) ?
		ANativeWindow_getWidth(android_window) : (int)android_input->width;
	window_height = (android_window != NULL) ?
		ANativeWindow_getHeight(android_window) : (int)android_input->height;
	logical_x = (window_width > 0) ?
		(int)(x * (float)android_input->width / (float)window_width) : (int)x;
	logical_y = (window_height > 0) ?
		(int)(y * (float)android_input->height / (float)window_height) : (int)y;
	if (logical_x < 0)
		logical_x = 0;
	else if (logical_x >= (int)android_input->width)
		logical_x = (int)android_input->width - 1;
	if (logical_y < 0)
		logical_y = 0;
	else if (logical_y >= (int)android_input->height)
		logical_y = (int)android_input->height - 1;
	contact.id = 0;
	contact.x = logical_x;
	contact.y = logical_y;
	active = (action != AMOTION_EVENT_ACTION_UP) &&
		(action != AMOTION_EVENT_ACTION_POINTER_UP) &&
		(action != AMOTION_EVENT_ACTION_CANCEL);
	previous_count = fb_gfx3_input_platform_touch_replace(android_input,
		active ? &contact : NULL, active ? 1u : 0u);
	if (active) {
		/*
			The compatibility mouse position must be visible before its button
			event is queued.  FreeBASIC programs commonly call GetMouse while
			handling EVENT_MOUSE_BUTTON_PRESS, including after Android has already
			delivered the corresponding release.  Publishing the button first
			made that query fail with the initial (-1, -1) position.
		*/
		fb_gfx3_input_platform_mouse_move(android_input, logical_x, logical_y);
		if (previous_count == 0)
			fb_gfx3_input_platform_mouse_button(android_input, BUTTON_LEFT, TRUE,
				FALSE);
	}
	else if (previous_count > 0)
		fb_gfx3_input_platform_mouse_button(android_input, BUTTON_LEFT, FALSE,
			FALSE);
	pthread_mutex_unlock(&android_window_mutex);
}

void fb_hAndroidTouchEvent(const AInputEvent *event)
{
	FB_GFX3_TOUCH_CONTACT contacts[FB_GFX3_INPUT_TOUCH_MAX];
	int action_full;
	int action;
	int action_index;
	int pointer_count;
	int contact_count = 0;
	int previous_count;
	int window_width;
	int window_height;
	int action_pointer_id;
	int keyboard_pointer_id;
	int toggle_keyboard = FALSE;
	int i;

	if ((event == NULL) || (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION))
		return;
	action_full = AMotionEvent_getAction(event);
	action = action_full & AMOTION_EVENT_ACTION_MASK;
	action_index = (action_full & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
		AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
	pointer_count = (int)AMotionEvent_getPointerCount(event);
	if (pointer_count <= 0)
		return;
	if ((action_index < 0) || (action_index >= pointer_count))
		action_index = 0;
	action_pointer_id = (int)AMotionEvent_getPointerId(event, action_index);

	pthread_mutex_lock(&android_window_mutex);
	if (android_input == NULL) {
		pthread_mutex_unlock(&android_window_mutex);
		return;
	}
	android_keyboard_button_event_locked(AMotionEvent_getX(event, action_index),
		AMotionEvent_getY(event, action_index), action, action_pointer_id,
		&toggle_keyboard);
	keyboard_pointer_id = android_keyboard_button_pointer_id;
	window_width = (android_window != NULL) ? ANativeWindow_getWidth(android_window) :
		(int)android_input->width;
	window_height = (android_window != NULL) ? ANativeWindow_getHeight(android_window) :
		(int)android_input->height;
	if (action != AMOTION_EVENT_ACTION_CANCEL) {
		for (i = 0; (i < pointer_count) &&
		     (contact_count < (int)FB_GFX3_INPUT_TOUCH_MAX); ++i) {
			int logical_x;
			int logical_y;

			if (((action == AMOTION_EVENT_ACTION_UP) ||
			     (action == AMOTION_EVENT_ACTION_POINTER_UP)) &&
			    (i == action_index))
				continue;
			if ((int)AMotionEvent_getPointerId(event, i) == keyboard_pointer_id)
				continue;
			logical_x = (window_width > 0) ? (int)(AMotionEvent_getX(event, i) *
				(float)android_input->width / (float)window_width) :
				(int)AMotionEvent_getX(event, i);
			logical_y = (window_height > 0) ? (int)(AMotionEvent_getY(event, i) *
				(float)android_input->height / (float)window_height) :
				(int)AMotionEvent_getY(event, i);
			if (logical_x < 0)
				logical_x = 0;
			else if (logical_x >= (int)android_input->width)
				logical_x = (int)android_input->width - 1;
			if (logical_y < 0)
				logical_y = 0;
			else if (logical_y >= (int)android_input->height)
				logical_y = (int)android_input->height - 1;
			contacts[contact_count].id = (int)AMotionEvent_getPointerId(event, i);
			contacts[contact_count].x = logical_x;
			contacts[contact_count].y = logical_y;
			contact_count++;
		}
	}
	previous_count = fb_gfx3_input_platform_touch_replace(android_input,
		contacts, (uint32_t)contact_count);
	if (contact_count > 0) {
		fb_gfx3_input_platform_mouse_move(android_input, contacts[0].x,
			contacts[0].y);
		if (previous_count == 0)
			fb_gfx3_input_platform_mouse_button(android_input, BUTTON_LEFT,
				TRUE, FALSE);
	}
	else if (previous_count > 0)
		fb_gfx3_input_platform_mouse_button(android_input, BUTTON_LEFT, FALSE,
			FALSE);
	pthread_mutex_unlock(&android_window_mutex);
	if (toggle_keyboard)
		fb_hAndroidToggleKeyboard();
}

/* ------------------------------------------------------------------------- */
/* Android controller bridge                                                 */
/* ------------------------------------------------------------------------- */

static float android_gamepad_clamp_axis(float value)
{
	if (value < -1.0f)
		return -1.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static float android_gamepad_clamp_trigger(float value)
{
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static ssize_t android_gamepad_key_button(int32_t keycode)
{
	switch (keycode) {
	case AKEYCODE_BUTTON_A: return XPAD_BUTTON_A;
	case AKEYCODE_BUTTON_B: return XPAD_BUTTON_B;
	case AKEYCODE_BUTTON_X: return XPAD_BUTTON_X;
	case AKEYCODE_BUTTON_Y: return XPAD_BUTTON_Y;
	case AKEYCODE_BUTTON_L1: return XPAD_BUTTON_L1;
	case AKEYCODE_BUTTON_R1: return XPAD_BUTTON_R1;
	case AKEYCODE_BUTTON_THUMBL: return XPAD_BUTTON_L3;
	case AKEYCODE_BUTTON_THUMBR: return XPAD_BUTTON_R3;
	case AKEYCODE_BUTTON_START: return XPAD_BUTTON_START;
	case AKEYCODE_BUTTON_SELECT: return XPAD_BUTTON_SELECT;
	case AKEYCODE_BUTTON_MODE: return XPAD_BUTTON_GUIDE;
	default: return 0;
	}
}

static ssize_t android_gamepad_key_dpad(int32_t keycode)
{
	switch (keycode) {
	case AKEYCODE_DPAD_UP: return XPAD_DPAD_UP;
	case AKEYCODE_DPAD_RIGHT: return XPAD_DPAD_RIGHT;
	case AKEYCODE_DPAD_DOWN: return XPAD_DPAD_DOWN;
	case AKEYCODE_DPAD_LEFT: return XPAD_DPAD_LEFT;
	default: return 0;
	}
}

void fb_hAndroidGamepadMotion(const AInputEvent *event)
{
	float axis[FB_GFX3_INPUT_GAMEPAD_AXIS_COUNT];
	float hat_x;
	float hat_y;
	ssize_t dpad = 0;

	if ((event == NULL) ||
	    (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION))
		return;
	axis[0] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_X, 0));
	axis[1] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_Y, 0));
	axis[2] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_Z, 0));
	axis[3] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_RZ, 0));
	axis[4] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_RX, 0));
	axis[5] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_RY, 0));
	axis[6] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_BRAKE, 0));
	axis[7] = android_gamepad_clamp_axis(AMotionEvent_getAxisValue(event,
		AMOTION_EVENT_AXIS_GAS, 0));
	hat_x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
	hat_y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);
	if (hat_y < -FB_GFX3_ANDROID_GAMEPAD_HAT_THRESHOLD)
		dpad |= XPAD_DPAD_UP;
	if (hat_y > FB_GFX3_ANDROID_GAMEPAD_HAT_THRESHOLD)
		dpad |= XPAD_DPAD_DOWN;
	if (hat_x < -FB_GFX3_ANDROID_GAMEPAD_HAT_THRESHOLD)
		dpad |= XPAD_DPAD_LEFT;
	if (hat_x > FB_GFX3_ANDROID_GAMEPAD_HAT_THRESHOLD)
		dpad |= XPAD_DPAD_RIGHT;
	pthread_mutex_lock(&android_window_mutex);
	if (android_input != NULL)
		fb_gfx3_input_platform_gamepad_motion(android_input,
			AInputEvent_getDeviceId(event), axis,
			android_gamepad_clamp_trigger(AMotionEvent_getAxisValue(event,
				AMOTION_EVENT_AXIS_LTRIGGER, 0)),
			android_gamepad_clamp_trigger(AMotionEvent_getAxisValue(event,
				AMOTION_EVENT_AXIS_RTRIGGER, 0)), dpad);
	pthread_mutex_unlock(&android_window_mutex);
}

void fb_hAndroidGamepadKey(const AInputEvent *event)
{
	int action;
	ssize_t button;
	ssize_t dpad;

	if ((event == NULL) ||
	    (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_KEY))
		return;
	action = AKeyEvent_getAction(event);
	if ((action != AKEY_EVENT_ACTION_DOWN) &&
	    (action != AKEY_EVENT_ACTION_UP))
		return;
	button = android_gamepad_key_button(AKeyEvent_getKeyCode(event));
	dpad = android_gamepad_key_dpad(AKeyEvent_getKeyCode(event));
	if ((button == 0) && (dpad == 0))
		return;
	pthread_mutex_lock(&android_window_mutex);
	if (android_input != NULL)
		fb_gfx3_input_platform_gamepad_key(android_input,
			AInputEvent_getDeviceId(event), button, dpad,
			action == AKEY_EVENT_ACTION_DOWN);
	pthread_mutex_unlock(&android_window_mutex);
}

/* ------------------------------------------------------------------------- */
/* Android keyboard and Java input bridge                                    */
/* ------------------------------------------------------------------------- */

static int android_key_to_scancode(int32_t keycode)
{
	if ((keycode >= AKEYCODE_A) && (keycode <= AKEYCODE_Z))
		return SC_A + (keycode - AKEYCODE_A);
	if ((keycode >= AKEYCODE_1) && (keycode <= AKEYCODE_9))
		return SC_1 + (keycode - AKEYCODE_1);
	if (keycode == AKEYCODE_0)
		return SC_0;

	switch (keycode) {
	case AKEYCODE_ESCAPE: return SC_ESCAPE;
	case AKEYCODE_DEL: return SC_BACKSPACE;
	case AKEYCODE_TAB: return SC_TAB;
	case AKEYCODE_ENTER: return SC_ENTER;
	case AKEYCODE_SPACE: return SC_SPACE;
	case AKEYCODE_MINUS: return SC_MINUS;
	case AKEYCODE_EQUALS: return SC_EQUALS;
	case AKEYCODE_COMMA: return SC_COMMA;
	case AKEYCODE_PERIOD: return SC_PERIOD;
	case AKEYCODE_GRAVE: return SC_TILDE;
	case AKEYCODE_LEFT_BRACKET: return SC_LEFTBRACKET;
	case AKEYCODE_RIGHT_BRACKET: return SC_RIGHTBRACKET;
	case AKEYCODE_BACKSLASH: return SC_BACKSLASH;
	case AKEYCODE_SEMICOLON: return SC_SEMICOLON;
	case AKEYCODE_APOSTROPHE: return SC_QUOTE;
	case AKEYCODE_SLASH: return SC_SLASH;
	case AKEYCODE_DPAD_LEFT: return SC_LEFT;
	case AKEYCODE_DPAD_RIGHT: return SC_RIGHT;
	case AKEYCODE_DPAD_UP: return SC_UP;
	case AKEYCODE_DPAD_DOWN: return SC_DOWN;
	default: return 0;
	}
}

static int android_key_to_ascii(int32_t keycode)
{
	if ((keycode >= AKEYCODE_A) && (keycode <= AKEYCODE_Z))
		return 'a' + (keycode - AKEYCODE_A);
	if ((keycode >= AKEYCODE_0) && (keycode <= AKEYCODE_9))
		return '0' + (keycode - AKEYCODE_0);

	switch (keycode) {
	case AKEYCODE_DEL: return KEY_BACKSPACE;
	case AKEYCODE_TAB: return KEY_TAB;
	case AKEYCODE_ENTER: return '\r';
	case AKEYCODE_SPACE: return ' ';
	case AKEYCODE_MINUS: return '-';
	case AKEYCODE_EQUALS: return '=';
	case AKEYCODE_COMMA: return ',';
	case AKEYCODE_PERIOD: return '.';
	case AKEYCODE_GRAVE: return '`';
	case AKEYCODE_LEFT_BRACKET: return '[';
	case AKEYCODE_RIGHT_BRACKET: return ']';
	case AKEYCODE_BACKSLASH: return '\\';
	case AKEYCODE_SEMICOLON: return ';';
	case AKEYCODE_APOSTROPHE: return '\'';
	case AKEYCODE_SLASH: return '/';
	default: return 0;
	}
}

void fb_hAndroidKey(int32_t keycode, int action, int unicode)
{
	int scancode;
	int key;
	int type;

	if (action == AKEY_EVENT_ACTION_DOWN)
		type = EVENT_KEY_PRESS;
	else if (action == AKEY_EVENT_ACTION_UP)
		type = EVENT_KEY_RELEASE;
	else if (action == AKEY_EVENT_ACTION_MULTIPLE)
		type = EVENT_KEY_REPEAT;
	else
		return;

	scancode = android_key_to_scancode(keycode);
	if ((unicode > 0) && (unicode <= 0xFF))
		key = unicode;
	else
		key = android_key_to_ascii(keycode);
	if ((key == 0) && (scancode != 0))
		key = fb_hScancodeToExtendedKey(scancode);
	if ((scancode == 0) && (key == 0))
		return;

	pthread_mutex_lock(&android_window_mutex);
	if (android_input != NULL) {
		fb_gfx3_input_platform_key(android_input, type, scancode,
			((key > 0) && (key <= 0xFF)) ? key : 0);
		if ((type != EVENT_KEY_RELEASE) && (key > 0))
			fb_gfx3_input_platform_character(android_input, key, 1);
	}
	pthread_mutex_unlock(&android_window_mutex);
}

JNIEXPORT jboolean JNICALL
Java_org_freebasic_android_FreeBasicNativeActivity_nativeDispatchImeKey(
	JNIEnv *environment, jclass class_object, jint keycode, jint action,
	jint unicode)
{
	(void)environment;
	(void)class_object;
	fb_hAndroidKey((int32_t)keycode, (int)action, (int)unicode);
	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_org_freebasic_android_FreeBasicInputView_nativeDispatchImeKey(
	JNIEnv *environment, jclass class_object, jint keycode, jint action,
	jint unicode)
{
	(void)environment;
	(void)class_object;
	fb_hAndroidKey((int32_t)keycode, (int)action, (int)unicode);
	return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_org_freebasic_android_FreeBasicNativeActivity_nativeSetKeyboardButtonVisible(
	JNIEnv *environment, jclass class_object, jboolean visible)
{
	(void)environment;
	(void)class_object;
	fb_hAndroidSetKeyboardButtonVisible(visible == JNI_TRUE);
}

/* ------------------------------------------------------------------------- */
/* EGL lifecycle                                                             */
/* ------------------------------------------------------------------------- */

static int android_probe_opengl(void)
{
	return (eglGetDisplay(EGL_DEFAULT_DISPLAY) != EGL_NO_DISPLAY) ?
		FB_GFX3_OK : FB_GFX3_UNSUPPORTED;
}

static int android_allocate_platform(void **platform, void *input,
	FB_GFX3_ANDROID_PLATFORM **created)
{
	FB_GFX3_ANDROID_PLATFORM *state;

	if ((platform == NULL) || (created == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_ANDROID_PLATFORM *)calloc(1, sizeof(*state));
	if (state == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	state->display = EGL_NO_DISPLAY;
	state->surface = EGL_NO_SURFACE;
	state->context = EGL_NO_CONTEXT;
	state->input = (FB_GFX3_INPUT_STATE *)input;
	state->window = android_retain_window();
	if (state->window == NULL) {
		free(state);
		return FB_GFX3_UNSUPPORTED;
	}
	pthread_mutex_lock(&android_window_mutex);
	android_input = state->input;
	/*
		The activity can receive its focus callback before SCREENRES creates the
		gfxlib3 input state.  Apply the saved lifecycle value here so GetMouse and
		GetTouch do not reject valid contacts until another focus transition occurs.
	*/
	fb_gfx3_input_platform_focus(android_input, android_focused);
	pthread_mutex_unlock(&android_window_mutex);
	*platform = state;
	*created = state;
	return FB_GFX3_OK;
}

static int android_create_window(void **platform,
	const FB_GFX3_PLATFORM_WINDOW_CONFIG *config)
{
	FB_GFX3_ANDROID_PLATFORM *state;
	int result;

	if (config == NULL)
		return FB_GFX3_INVALID;
	result = android_allocate_platform(platform, config->input, &state);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_input_platform_window_info(state->input,
		(uintptr_t)state->window, 0, 0, 0,
		ANativeWindow_getWidth(state->window),
		ANativeWindow_getHeight(state->window));
	return FB_GFX3_OK;
}

static int android_create_opengl(void **platform,
	const FB_GFX3_PLATFORM_OPENGL_CONFIG *config)
{
	const EGLint attributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};
	const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	FB_GFX3_ANDROID_PLATFORM *state;
	EGLConfig egl_config;
	EGLint config_count;
	EGLint native_format;
	EGLint major;
	EGLint minor;
	int result;

	if ((config == NULL) || (config->major_version > 3u))
		return FB_GFX3_UNSUPPORTED;
	result = android_allocate_platform(platform, config->input, &state);
	if (result != FB_GFX3_OK)
		return result;
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if ((state->display == EGL_NO_DISPLAY) ||
	    !eglInitialize(state->display, &major, &minor) ||
	    !eglBindAPI(EGL_OPENGL_ES_API) ||
	    !eglChooseConfig(state->display, attributes, &egl_config, 1,
		&config_count) || (config_count < 1) ||
	    !eglGetConfigAttrib(state->display, egl_config,
		EGL_NATIVE_VISUAL_ID, &native_format))
		return FB_GFX3_UNSUPPORTED;
	state->context = eglCreateContext(state->display, egl_config,
		EGL_NO_CONTEXT, context_attributes);
	if ((state->context == EGL_NO_CONTEXT) ||
	    (ANativeWindow_setBuffersGeometry(state->window, 0, 0,
		native_format) != 0))
		return FB_GFX3_UNSUPPORTED;
	state->surface = eglCreateWindowSurface(state->display, egl_config,
		state->window, NULL);
	if ((state->surface == EGL_NO_SURFACE) ||
	    !eglMakeCurrent(state->display, state->surface, state->surface,
		state->context))
		return FB_GFX3_UNSUPPORTED;
	state->owns_egl = TRUE;
	/*
		Like the desktop backends, gfxlib3 presents on the program's explicit
		SCREENUPDATE/page-copy boundary.  Do not add an implicit display-refresh
		stall to that command stream.  Android's compositor may still pace a
		window when required by the device, but EGL must be asked for immediate
		presentation first.
	*/
	eglSwapInterval(state->display, 0);
	fb_gfx3_input_platform_window_info(state->input,
		(uintptr_t)state->window, (uintptr_t)state->display, 0, 0,
		ANativeWindow_getWidth(state->window),
		ANativeWindow_getHeight(state->window));
	return FB_GFX3_OK;
}

static int android_native_handles(void *platform, uintptr_t *instance,
	uintptr_t *window)
{
	FB_GFX3_ANDROID_PLATFORM *state = (FB_GFX3_ANDROID_PLATFORM *)platform;

	if ((state == NULL) || (state->window == NULL) || (instance == NULL) ||
	    (window == NULL))
		return FB_GFX3_INVALID;
	*instance = 0;
	*window = (uintptr_t)state->window;
	return FB_GFX3_OK;
}

static void android_destroy(void *platform)
{
	FB_GFX3_ANDROID_PLATFORM *state = (FB_GFX3_ANDROID_PLATFORM *)platform;

	if (state == NULL)
		return;
	if (state->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(state->display, EGL_NO_SURFACE, EGL_NO_SURFACE,
			EGL_NO_CONTEXT);
		if (state->context != EGL_NO_CONTEXT)
			eglDestroyContext(state->display, state->context);
		if (state->surface != EGL_NO_SURFACE)
			eglDestroySurface(state->display, state->surface);
		eglTerminate(state->display);
	}
	pthread_mutex_lock(&android_window_mutex);
	if (android_input == state->input)
		android_input = NULL;
	pthread_mutex_unlock(&android_window_mutex);
	if (state->window != NULL)
		ANativeWindow_release(state->window);
	free(state);
}

static int android_load_opengl_function(void *platform, const char *name,
	void *destination, size_t destination_size)
{
	__eglMustCastToProperFunctionPointerType function;

	(void)platform;
	if ((name == NULL) || (destination == NULL) ||
	    (destination_size != sizeof(function)))
		return FB_GFX3_INVALID;
	function = eglGetProcAddress(name);
	if (function == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, &function, sizeof(function));
	return FB_GFX3_OK;
}

static int android_client_size(void *platform, uint32_t *width,
	uint32_t *height)
{
	FB_GFX3_ANDROID_PLATFORM *state = (FB_GFX3_ANDROID_PLATFORM *)platform;
	int native_width;
	int native_height;

	if ((state == NULL) || (state->window == NULL) || (width == NULL) ||
	    (height == NULL))
		return FB_GFX3_INVALID;
	native_width = ANativeWindow_getWidth(state->window);
	native_height = ANativeWindow_getHeight(state->window);
	if ((native_width <= 0) || (native_height <= 0))
		return FB_GFX3_FAILED;
	*width = (uint32_t)native_width;
	*height = (uint32_t)native_height;
	return FB_GFX3_OK;
}

static int android_desktop_info(ssize_t *width, ssize_t *height,
	ssize_t *depth, ssize_t *refresh)
{
	ANativeWindow *window = android_retain_window();
	int native_width;
	int native_height;

	if (window == NULL)
		return FB_GFX3_UNSUPPORTED;
	native_width = ANativeWindow_getWidth(window);
	native_height = ANativeWindow_getHeight(window);
	ANativeWindow_release(window);
	if ((native_width <= 0) || (native_height <= 0))
		return FB_GFX3_FAILED;
	if (width != NULL)
		*width = native_width;
	if (height != NULL)
		*height = native_height;
	if (depth != NULL)
		*depth = 32;
	if (refresh != NULL)
		*refresh = 0;
	return FB_GFX3_OK;
}

int fb_gfx3_platform_keyboard_overlay(void *platform,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay)
{
	FB_GFX3_ANDROID_PLATFORM *state = (FB_GFX3_ANDROID_PLATFORM *)platform;
	int window_width;
	int window_height;

	if ((state == NULL) || (state->window == NULL) || (overlay == NULL))
		return FB_GFX3_INVALID;
	memset(overlay, 0, sizeof(*overlay));
	window_width = ANativeWindow_getWidth(state->window);
	window_height = ANativeWindow_getHeight(state->window);
	pthread_mutex_lock(&android_window_mutex);
	if (android_keyboard_button_rect_locked(window_width, window_height,
	    &overlay->x0, &overlay->y0, &overlay->x1, &overlay->y1)) {
		overlay->visible = TRUE;
		overlay->keyboard_visible = android_keyboard_visible;
		overlay->pressed = android_keyboard_button_down;
	}
	pthread_mutex_unlock(&android_window_mutex);
	return FB_GFX3_OK;
}

static int android_swap_buffers(void *platform)
{
	FB_GFX3_ANDROID_PLATFORM *state = (FB_GFX3_ANDROID_PLATFORM *)platform;

	if ((state == NULL) || !state->owns_egl)
		return FB_GFX3_INVALID;
	return eglSwapBuffers(state->display, state->surface) ?
		FB_GFX3_OK : FB_GFX3_FAILED;
}

static void android_pump_events(void *platform)
{
	(void)platform;
	/* The package's NativeActivity looper owns Android event dispatch. */
}

static int android_show_window(void *platform)
{
	return (platform != NULL) ? FB_GFX3_OK : FB_GFX3_INVALID;
}

static int android_set_window_title(void *platform, const char *title)
{
	(void)title;
	return (platform != NULL) ? FB_GFX3_OK : FB_GFX3_INVALID;
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_android = {
	"Android NativeActivity/EGL",
	android_probe_opengl,
	android_create_window,
	android_create_opengl,
	android_native_handles,
	android_destroy,
	android_load_opengl_function,
	android_client_size,
	android_desktop_info,
	android_swap_buffers,
	android_pump_events,
	android_show_window,
	android_set_window_title
};

#else

int fb_gfx3_platform_keyboard_overlay(void *platform,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay)
{
	(void)platform;
	if (overlay != NULL)
		memset(overlay, 0, sizeof(*overlay));
	return FB_GFX3_UNSUPPORTED;
}

/* Keep the common platform selector linkable in cross-platform archives. */
static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_android = {
	"Android unavailable", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

#endif

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_default(void)
{
	return &__fb_gfx3_platform_android;
}

/* end of android/gfx3_platform.c */
