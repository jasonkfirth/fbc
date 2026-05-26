#include "fb_gfx_android.h"

#include <android/api-level.h>
#include <android/keycodes.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android/native_activity.h>
#include <jni.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FB_ANDROID_LOG_TAG "FreeBASIC"
#define FB_ANDROID_CONSOLE_LINES 256
#define FB_ANDROID_CONSOLE_COLS 256
#define FB_ANDROID_FONT_W 6
#define FB_ANDROID_FONT_H 8
#define FB_ANDROID_KEYBOARD_BUTTON_W 56
#define FB_ANDROID_KEYBOARD_BUTTON_H 40
#define FB_ANDROID_GAMEPAD_MAX 16
#define FB_ANDROID_GAMEPAD_TRIGGER_BUTTON_THRESHOLD 0.20f
#define FB_ANDROID_GAMEPAD_HAT_THRESHOLD 0.50f

#ifndef ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT
#define ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT 0x00000001
#endif

#ifndef ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED
#define ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED 0x00000002
#endif

#ifndef AWINDOW_FLAG_ALT_FOCUSABLE_IM
#define AWINDOW_FLAG_ALT_FOCUSABLE_IM 0x00020000
#endif

#ifndef ANATIVEACTIVITY_HIDE_SOFT_INPUT_IMPLICIT_ONLY
#define ANATIVEACTIVITY_HIDE_SOFT_INPUT_IMPLICIT_ONLY 0x00000001
#endif

#ifndef ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS
#define ANATIVEACTIVITY_HIDE_SOFT_INPUT_NOT_ALWAYS 0x00000002
#endif

#ifndef INPUT_METHOD_MANAGER_HIDE_NOT_ALWAYS
#define INPUT_METHOD_MANAGER_HIDE_NOT_ALWAYS 0x00000002
#endif

#define ANDROID_R_ID_CONTENT 0x01020002
#define FB_ANDROID_INPUT_VIEW_ID 0x0fb60001
#define FB_ANDROID_INPUT_TYPE_TEXT 0x00000001
#define FB_ANDROID_INPUT_TYPE_TEXT_FLAG_MULTI_LINE 0x00020000
#define FB_ANDROID_INPUT_TYPE_TEXT_FLAG_NO_SUGGESTIONS 0x00080000
#define FB_ANDROID_IME_ACTION_NONE 0x00000001
#define FB_ANDROID_IME_FLAG_NO_EXTRACT_UI 0x10000000
#define FB_ANDROID_INPUT_BACKSPACE_PAD "        " "        " "        " "        "


typedef struct FB_ANDROID_GAMEPAD_STATE
{
	int device_id;
	int seen;
	ssize_t buttons;
	ssize_t dpad;
	float axis[8];
	float left_trigger;
	float right_trigger;
} FB_ANDROID_GAMEPAD_STATE;

typedef struct FB_ANDROID_GFX_STATE
{
	pthread_mutex_t mutex;
	ANativeActivity *activity;
	ANativeWindow *window;
	int active;
	int width;
	int height;
	int depth;
	int refresh_rate;
	int mouse_x;
	int mouse_y;
	int mouse_z;
	int mouse_buttons;
	int mouse_latched_buttons;
	int window_width;
	int window_height;
	BLITTER *blitter;
	unsigned char *scale_buffer;
	size_t scale_buffer_size;
	int started;
	int resumed;
	int focused;
	unsigned surface_generation;
	int render_suspended;
	int console_enabled;
	char console[FB_ANDROID_CONSOLE_LINES][FB_ANDROID_CONSOLE_COLS];
	int console_line;
	int console_col;
	int keyboard_button_visible;
	int keyboard_button_down;
	int keyboard_enabled;
	int keyboard_visible;
	int pending_keyboard_visible;
	int display_locked;
	jobject input_view;
	char *input_text;
	FB_ANDROID_GAMEPAD_STATE gamepad[FB_ANDROID_GAMEPAD_MAX];
} FB_ANDROID_GFX_STATE;

static FB_ANDROID_GFX_STATE fb_android =
{
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.activity = NULL,
	.window = NULL,
	.active = 0,
	.width = 0,
	.height = 0,
	.depth = 0,
	.refresh_rate = 60,
	.mouse_x = 0,
	.mouse_y = 0,
	.mouse_z = 0,
	.mouse_buttons = 0,
	.mouse_latched_buttons = 0,
	.window_width = 0,
	.window_height = 0,
	.blitter = NULL,
	.scale_buffer = NULL,
	.scale_buffer_size = 0,
	.started = 1,
	.resumed = 1,
	.focused = 1,
	.surface_generation = 0,
	.render_suspended = 1,
	.console_enabled = 1,
	.console = {{0}},
	.console_line = 0,
	.console_col = 0,
	.keyboard_button_visible = 1,
	.keyboard_button_down = 0,
	.keyboard_enabled = 1,
	.keyboard_visible = 0,
	.pending_keyboard_visible = -1,
	.display_locked = 0,
	.input_view = NULL,
	.input_text = NULL,
	.gamepad = {{0}},
};

extern void fb_hPostKey(int key);

static void android_log(const char *text)
{
	if (text)
		__android_log_write(ANDROID_LOG_INFO, FB_ANDROID_LOG_TAG, text);
}

static void android_log_debug(const char *text, int x0, int y0, int x1, int y1, int action, int x, int y)
{
	char text_buffer[192];

	snprintf(text_buffer, sizeof(text_buffer), text, x0, y0, x1, y1, action, x, y);
	__android_log_print(ANDROID_LOG_DEBUG, FB_ANDROID_LOG_TAG, "%s", text_buffer);
}

static int framebuffer_viewport_locked(int dst_w, int dst_h, int *scale, int *skip,
	int *out_w, int *out_h, int *off_x, int *off_y);
static void android_reset_input_text_locked(void);
static void android_set_input_text_jni(ANativeActivity *activity, jobject input_view, const char *text);

static uint32_t rgba(unsigned r, unsigned g, unsigned b)
{
	return 0xff000000u | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu);
}

static void sleep_ms(int ms)
{
	struct timespec req;

	req.tv_sec = ms / 1000;
	req.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&req, NULL);
}

static JNIEnv *android_get_jni_env(ANativeActivity *activity, int *attached)
{
	JNIEnv *env = NULL;
	jint get_env_result;

	if (attached)
		*attached = 0;

	if (!activity || !activity->vm)
		return NULL;

	get_env_result = (*activity->vm)->GetEnv(activity->vm, (void **)&env, JNI_VERSION_1_6);
	if (get_env_result == JNI_EDETACHED)
	{
		if ((*activity->vm)->AttachCurrentThread(activity->vm, (JNIEnv **)&env, NULL) != JNI_OK)
			return NULL;
		if (attached)
			*attached = 1;
	}
	else if (get_env_result != JNI_OK)
	{
		return NULL;
	}

	return env;
}

static void android_release_jni_env(ANativeActivity *activity, int attached)
{
	if (attached && activity && activity->vm)
		(*activity->vm)->DetachCurrentThread(activity->vm);
}

static void update_window_size_locked(void)
{
	if (fb_android.window)
	{
		fb_android.window_width = ANativeWindow_getWidth(fb_android.window);
		fb_android.window_height = ANativeWindow_getHeight(fb_android.window);
	}
	else
	{
		fb_android.window_width = 0;
		fb_android.window_height = 0;
	}
}

static int can_render_locked(void)
{
	return fb_android.window && fb_android.started && fb_android.resumed && !fb_android.render_suspended;
}

static int can_post_input_locked(void)
{
	return fb_android.active && fb_android.started && fb_android.resumed && fb_android.focused;
}

static void update_render_suspended_locked(void)
{
	fb_android.render_suspended = (!fb_android.window || !fb_android.started || !fb_android.resumed);
}

static void configure_window_locked(void)
{
	if (!fb_android.window)
	{
		update_window_size_locked();
		update_render_suspended_locked();
		return;
	}

	ANativeWindow_setBuffersGeometry(fb_android.window, 0, 0, WINDOW_FORMAT_RGBA_8888);

	update_window_size_locked();
	update_render_suspended_locked();
}

static void android_delete_global_ref(ANativeActivity *activity, jobject ref)
{
	JNIEnv *env;
	int attached = 0;

	if (!ref)
		return;

	env = android_get_jni_env(activity, &attached);
	if (env)
		(*env)->DeleteGlobalRef(env, ref);
	android_release_jni_env(activity, attached);
}

static jclass android_find_app_class_jni(JNIEnv *env, ANativeActivity *activity, const char *class_name)
{
	jclass activity_class = NULL;
	jclass class_class = NULL;
	jclass class_loader_class = NULL;
	jobject class_loader = NULL;
	jstring name = NULL;
	jclass result = NULL;
	jmethodID get_class_loader = NULL;
	jmethodID load_class = NULL;

	if (!env || !activity || !activity->clazz || !class_name)
		return NULL;

	/*
	 * App class lookup
	 *
	 * NativeActivity callbacks run on threads created by Android.  On those
	 * threads, JNI FindClass() can resolve framework classes but may not use
	 * the APK's class loader for application classes.  Load helper classes
	 * through the activity's own class loader instead.
	 */
	activity_class = (*env)->GetObjectClass(env, activity->clazz);
	if (!activity_class)
		goto cleanup;

	class_class = (*env)->FindClass(env, "java/lang/Class");
	if ((*env)->ExceptionCheck(env) || !class_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	get_class_loader = (*env)->GetMethodID(env, class_class,
		"getClassLoader", "()Ljava/lang/ClassLoader;");
	if (!get_class_loader)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	class_loader = (*env)->CallObjectMethod(env, activity_class, get_class_loader);
	if ((*env)->ExceptionCheck(env) || !class_loader)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	class_loader_class = (*env)->FindClass(env, "java/lang/ClassLoader");
	if ((*env)->ExceptionCheck(env) || !class_loader_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	load_class = (*env)->GetMethodID(env, class_loader_class,
		"loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	if (!load_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	name = (*env)->NewStringUTF(env, class_name);
	if (!name)
		goto cleanup;

	result = (jclass)(*env)->CallObjectMethod(env, class_loader, load_class, name);
	if ((*env)->ExceptionCheck(env))
	{
		(*env)->ExceptionClear(env);
		result = NULL;
	}

cleanup:
	if (name)
		(*env)->DeleteLocalRef(env, name);
	if (class_loader)
		(*env)->DeleteLocalRef(env, class_loader);
	if (class_loader_class)
		(*env)->DeleteLocalRef(env, class_loader_class);
	if (class_class)
		(*env)->DeleteLocalRef(env, class_class);
	if (activity_class)
		(*env)->DeleteLocalRef(env, activity_class);

	return result;
}

static jobject android_install_input_view_jni(ANativeActivity *activity)
{
	JNIEnv *env;
	int attached = 0;
	jobject result = NULL;
	jobject content_root = NULL;
	jobject edit_text = NULL;
	jobject layout_params = NULL;
	jclass activity_class = NULL;
	jclass edit_text_class = NULL;
	jclass view_class = NULL;
	jclass view_group_class = NULL;
	jclass input_bridge_class = NULL;
	jclass layout_params_class = NULL;
	jmethodID find_view_by_id = NULL;
	jmethodID edit_text_ctor = NULL;
	jmethodID attach_input_view = NULL;
	jmethodID layout_params_ctor = NULL;
	jmethodID add_view = NULL;
	jmethodID set_id = NULL;
	jmethodID set_focusable = NULL;
	jmethodID set_focusable_in_touch_mode = NULL;
	jmethodID set_background_color = NULL;
	jmethodID set_alpha = NULL;
	jmethodID request_focus = NULL;
	jmethodID set_input_type = NULL;
	jmethodID set_ime_options = NULL;
	jmethodID set_single_line = NULL;
	jmethodID set_cursor_visible = NULL;
	jmethodID set_text_color = NULL;

	if (!activity || !activity->clazz)
		return NULL;

	env = android_get_jni_env(activity, &attached);
	if (!env)
		return NULL;

	activity_class = (*env)->GetObjectClass(env, activity->clazz);
	if (!activity_class)
		goto cleanup;

	find_view_by_id = (*env)->GetMethodID(env, activity_class, "findViewById", "(I)Landroid/view/View;");
	if (!find_view_by_id)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	content_root = (*env)->CallObjectMethod(env, activity->clazz, find_view_by_id, ANDROID_R_ID_CONTENT);
	if ((*env)->ExceptionCheck(env) || !content_root)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	view_group_class = (*env)->FindClass(env, "android/view/ViewGroup");
	if ((*env)->ExceptionCheck(env) || !view_group_class ||
	    !(*env)->IsInstanceOf(env, content_root, view_group_class))
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	edit_text_class = android_find_app_class_jni(env, activity, "org.freebasic.android.FreeBasicInputView");
	if ((*env)->ExceptionCheck(env) || !edit_text_class)
	{
		/*
		 * The packaged helper view catches IME-only keys such as Gboard's
		 * Backspace.  Fall back to a stock EditText so text entry still works
		 * if an older package template is used during development.
		 */
		(*env)->ExceptionClear(env);
		edit_text_class = (*env)->FindClass(env, "android/widget/EditText");
	}
	if ((*env)->ExceptionCheck(env) || !edit_text_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	edit_text_ctor = (*env)->GetMethodID(env, edit_text_class, "<init>", "(Landroid/content/Context;)V");
	if (!edit_text_ctor)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	edit_text = (*env)->NewObject(env, edit_text_class, edit_text_ctor, activity->clazz);
	if ((*env)->ExceptionCheck(env) || !edit_text)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	view_class = (*env)->FindClass(env, "android/view/View");
	if ((*env)->ExceptionCheck(env) || !view_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	set_id = (*env)->GetMethodID(env, view_class, "setId", "(I)V");
	if (set_id)
		(*env)->CallVoidMethod(env, edit_text, set_id, FB_ANDROID_INPUT_VIEW_ID);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_focusable = (*env)->GetMethodID(env, view_class, "setFocusable", "(Z)V");
	if (set_focusable)
		(*env)->CallVoidMethod(env, edit_text, set_focusable, JNI_TRUE);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_focusable_in_touch_mode = (*env)->GetMethodID(env, view_class, "setFocusableInTouchMode", "(Z)V");
	if (set_focusable_in_touch_mode)
		(*env)->CallVoidMethod(env, edit_text, set_focusable_in_touch_mode, JNI_TRUE);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_background_color = (*env)->GetMethodID(env, view_class, "setBackgroundColor", "(I)V");
	if (set_background_color)
		(*env)->CallVoidMethod(env, edit_text, set_background_color, 0);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_alpha = (*env)->GetMethodID(env, view_class, "setAlpha", "(F)V");
	if (set_alpha)
		(*env)->CallVoidMethod(env, edit_text, set_alpha, 0.01f);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_input_type = (*env)->GetMethodID(env, edit_text_class, "setInputType", "(I)V");
	if (set_input_type)
		(*env)->CallVoidMethod(env, edit_text, set_input_type,
			FB_ANDROID_INPUT_TYPE_TEXT |
			FB_ANDROID_INPUT_TYPE_TEXT_FLAG_MULTI_LINE |
			FB_ANDROID_INPUT_TYPE_TEXT_FLAG_NO_SUGGESTIONS);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_ime_options = (*env)->GetMethodID(env, edit_text_class, "setImeOptions", "(I)V");
	if (set_ime_options)
		(*env)->CallVoidMethod(env, edit_text, set_ime_options,
			FB_ANDROID_IME_ACTION_NONE | FB_ANDROID_IME_FLAG_NO_EXTRACT_UI);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_single_line = (*env)->GetMethodID(env, edit_text_class, "setSingleLine", "(Z)V");
	if (set_single_line)
		(*env)->CallVoidMethod(env, edit_text, set_single_line, JNI_FALSE);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_cursor_visible = (*env)->GetMethodID(env, edit_text_class, "setCursorVisible", "(Z)V");
	if (set_cursor_visible)
		(*env)->CallVoidMethod(env, edit_text, set_cursor_visible, JNI_FALSE);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_text_color = (*env)->GetMethodID(env, edit_text_class, "setTextColor", "(I)V");
	if (set_text_color)
		(*env)->CallVoidMethod(env, edit_text, set_text_color, 0);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	input_bridge_class = android_find_app_class_jni(env, activity, "org.freebasic.android.FreeBasicInputBridge");
	if ((*env)->ExceptionCheck(env) || !input_bridge_class)
	{
		(*env)->ExceptionClear(env);
	}
	else
	{
		/*
		 * Some software keyboards send Backspace as an IME-only key event to
		 * the served Java view instead of through NativeActivity's input
		 * queue.  The Java bridge attaches to the real EditText instance so
		 * that path can still produce normal FreeBASIC key events.
		 */
		attach_input_view = (*env)->GetStaticMethodID(env, input_bridge_class,
			"attach", "(Landroid/widget/EditText;)V");
		if (attach_input_view)
			(*env)->CallStaticVoidMethod(env, input_bridge_class, attach_input_view, edit_text);
		if ((*env)->ExceptionCheck(env))
			(*env)->ExceptionClear(env);
	}

	layout_params_class = (*env)->FindClass(env, "android/view/ViewGroup$LayoutParams");
	if ((*env)->ExceptionCheck(env) || !layout_params_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	layout_params_ctor = (*env)->GetMethodID(env, layout_params_class, "<init>", "(II)V");
	if (!layout_params_ctor)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	layout_params = (*env)->NewObject(env, layout_params_class, layout_params_ctor, 1, 1);
	if ((*env)->ExceptionCheck(env) || !layout_params)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	add_view = (*env)->GetMethodID(env, view_group_class,
		"addView", "(Landroid/view/View;Landroid/view/ViewGroup$LayoutParams;)V");
	if (!add_view)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	(*env)->CallVoidMethod(env, content_root, add_view, edit_text, layout_params);
	if ((*env)->ExceptionCheck(env))
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	/*
	 * The hidden EditText is only a keyboard/input bridge.  It must not take
	 * focus during activity startup, because Android may interpret that as a
	 * request to show the IME or handwriting/stylus UI over a graphics
	 * program before the program has asked for text input.
	 *
	 * android_toggle_soft_input_jni() focuses the view explicitly when the
	 * on-screen keyboard is requested.
	 */

	result = (*env)->NewGlobalRef(env, edit_text);

cleanup:
	if (layout_params)
		(*env)->DeleteLocalRef(env, layout_params);
	if (layout_params_class)
		(*env)->DeleteLocalRef(env, layout_params_class);
	if (edit_text)
		(*env)->DeleteLocalRef(env, edit_text);
	if (view_class)
		(*env)->DeleteLocalRef(env, view_class);
	if (input_bridge_class)
		(*env)->DeleteLocalRef(env, input_bridge_class);
	if (edit_text_class)
		(*env)->DeleteLocalRef(env, edit_text_class);
	if (view_group_class)
		(*env)->DeleteLocalRef(env, view_group_class);
	if (content_root)
		(*env)->DeleteLocalRef(env, content_root);
	if (activity_class)
		(*env)->DeleteLocalRef(env, activity_class);

	android_release_jni_env(activity, attached);
	return result;
}

static int android_toggle_soft_input_jni(ANativeActivity *activity, int show, int flags)
{
	JNIEnv *env = NULL;
	int attached = 0;
	int result = 0;
	int input_view_own_ref = 0;
	jobject input_manager = NULL;
	jstring service_name = NULL;
	jclass activity_class = NULL;
	jclass window_class = NULL;
	jclass input_view_class = NULL;
	jclass view_group_class = NULL;
	jclass input_manager_class = NULL;
	jobject window = NULL;
	jobject decor_view = NULL;
	jobject content_root = NULL;
	jobject content_child = NULL;
	jobject global_input_view = NULL;
	jobject input_view = NULL;
	jobject window_token = NULL;
	jmethodID get_system_service = NULL;
	jmethodID get_window = NULL;
	jmethodID get_decor_view = NULL;
	jmethodID find_view_by_id = NULL;
	jmethodID get_child_count = NULL;
	jmethodID get_child_at = NULL;
	jmethodID request_focus = NULL;
	jmethodID set_focusable = NULL;
	jmethodID set_focusable_in_touch_mode = NULL;
	jmethodID get_window_token = NULL;
	jmethodID show_soft_input = NULL;
	jmethodID hide_soft_input = NULL;
	jmethodID toggle_soft_input = NULL;

	if (!activity || !activity->vm || !activity->clazz)
		return 0;

	env = android_get_jni_env(activity, &attached);
	if (!env)
		return 0;

	activity_class = (*env)->GetObjectClass(env, activity->clazz);
	if (!activity_class)
		goto cleanup;

	get_system_service = (*env)->GetMethodID(env, activity_class, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	if (!get_system_service)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	service_name = (*env)->NewStringUTF(env, "input_method");
	if (!service_name)
		goto cleanup;

	input_manager = (*env)->CallObjectMethod(env, activity->clazz, get_system_service, service_name);
	if ((*env)->ExceptionCheck(env) || !input_manager)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	input_manager_class = (*env)->GetObjectClass(env, input_manager);
	if (!input_manager_class)
		goto cleanup;

	pthread_mutex_lock(&fb_android.mutex);
	if (fb_android.input_view)
		global_input_view = fb_android.input_view;
	pthread_mutex_unlock(&fb_android.mutex);

	if (global_input_view)
	{
		input_view = (*env)->NewLocalRef(env, global_input_view);
		input_view_own_ref = input_view ? 1 : 0;
	}

	/*
	 * NativeActivity installs an internal NativeContentView under
	 * android.R.id.content.  The IME must be shown for that view, not for
	 * the window decor view, otherwise Android can reject the request
	 * because the served view and the requested view do not match.
	 */
	if (!input_view)
	{
		find_view_by_id = (*env)->GetMethodID(env, activity_class, "findViewById", "(I)Landroid/view/View;");
		if (find_view_by_id)
		{
			input_view = (*env)->CallObjectMethod(env, activity->clazz, find_view_by_id, FB_ANDROID_INPUT_VIEW_ID);
			if ((*env)->ExceptionCheck(env))
			{
				(*env)->ExceptionClear(env);
				input_view = NULL;
			}
			else if (input_view)
			{
				input_view_own_ref = 1;
			}
			if (!input_view)
			{
				content_root = (*env)->CallObjectMethod(env, activity->clazz, find_view_by_id, ANDROID_R_ID_CONTENT);
				if ((*env)->ExceptionCheck(env))
				{
					(*env)->ExceptionClear(env);
					content_root = NULL;
				}
			}
		}
		else
		{
			(*env)->ExceptionClear(env);
		}
	}

	if (!input_view && content_root)
	{
		input_view = content_root;
		view_group_class = (*env)->FindClass(env, "android/view/ViewGroup");
		if ((*env)->ExceptionCheck(env))
		{
			(*env)->ExceptionClear(env);
			view_group_class = NULL;
		}

		if (view_group_class && (*env)->IsInstanceOf(env, content_root, view_group_class))
		{
			get_child_count = (*env)->GetMethodID(env, view_group_class, "getChildCount", "()I");
			get_child_at = (*env)->GetMethodID(env, view_group_class, "getChildAt", "(I)Landroid/view/View;");
			if (get_child_count && get_child_at &&
			    (*env)->CallIntMethod(env, content_root, get_child_count) > 0)
			{
				content_child = (*env)->CallObjectMethod(env, content_root, get_child_at, 0);
				if ((*env)->ExceptionCheck(env))
				{
					(*env)->ExceptionClear(env);
					content_child = NULL;
				}
				else if (content_child)
				{
					input_view = content_child;
				}
			}
			if ((*env)->ExceptionCheck(env))
				(*env)->ExceptionClear(env);
		}
	}

	get_window = (*env)->GetMethodID(env, activity_class, "getWindow", "()Landroid/view/Window;");
	if (!get_window)
	{
		(*env)->ExceptionClear(env);
		if (!input_view)
			goto cleanup;
	}

	if (get_window)
	{
		window = (*env)->CallObjectMethod(env, activity->clazz, get_window);
		if ((*env)->ExceptionCheck(env))
		{
			(*env)->ExceptionClear(env);
			window = NULL;
		}
	}

	if (!input_view && window)
	{
		window_class = (*env)->GetObjectClass(env, window);
		if (!window_class)
			goto cleanup;

		get_decor_view = (*env)->GetMethodID(env, window_class, "getDecorView", "()Landroid/view/View;");
		if (!get_decor_view)
		{
			(*env)->ExceptionClear(env);
			goto cleanup;
		}

		decor_view = (*env)->CallObjectMethod(env, window, get_decor_view);
		if ((*env)->ExceptionCheck(env) || !decor_view)
		{
			(*env)->ExceptionClear(env);
			goto cleanup;
		}

		input_view = decor_view;
	}

	if (!input_view)
		goto cleanup;

	input_view_class = (*env)->GetObjectClass(env, input_view);
	if (!input_view_class)
		goto cleanup;

	set_focusable = (*env)->GetMethodID(env, input_view_class, "setFocusable", "(Z)V");
	if (set_focusable)
		(*env)->CallVoidMethod(env, input_view, set_focusable, JNI_TRUE);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_focusable_in_touch_mode = (*env)->GetMethodID(env, input_view_class, "setFocusableInTouchMode", "(Z)V");
	if (set_focusable_in_touch_mode)
		(*env)->CallVoidMethod(env, input_view, set_focusable_in_touch_mode, JNI_TRUE);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	request_focus = (*env)->GetMethodID(env, input_view_class, "requestFocus", "()Z");
	if (request_focus)
		(*env)->CallBooleanMethod(env, input_view, request_focus);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	if (show)
	{
		/*
		 * Use showSoftInput() for explicit show requests.  toggleSoftInput()
		 * depends on Android's current IME state, which can drift from our
		 * cached keyboard flag and turn a requested show into a hide.
		 */
		show_soft_input = (*env)->GetMethodID(env, input_manager_class,
			"showSoftInput", "(Landroid/view/View;I)Z");
		if (show_soft_input)
		{
			result = ((*env)->CallBooleanMethod(env, input_manager,
				show_soft_input, input_view, flags) == JNI_TRUE) ? 1 : 0;
			if ((*env)->ExceptionCheck(env))
			{
				(*env)->ExceptionClear(env);
				result = 0;
			}
		}
	}
	else
	{
		get_window_token = (*env)->GetMethodID(env, input_view_class, "getWindowToken", "()Landroid/os/IBinder;");
		if (!get_window_token)
		{
			(*env)->ExceptionClear(env);
			goto cleanup;
		}

		window_token = (*env)->CallObjectMethod(env, input_view, get_window_token);
		if ((*env)->ExceptionCheck(env) || !window_token)
		{
			(*env)->ExceptionClear(env);
			goto cleanup;
		}

		hide_soft_input = (*env)->GetMethodID(env, input_manager_class, "hideSoftInputFromWindow", "(Landroid/os/IBinder;I)Z");
		if (!hide_soft_input)
		{
			(*env)->ExceptionClear(env);
			goto cleanup;
		}

		result = ((*env)->CallBooleanMethod(env, input_manager, hide_soft_input, window_token, flags) == JNI_TRUE) ? 1 : 0;
		if ((*env)->ExceptionCheck(env))
		{
			(*env)->ExceptionClear(env);
			result = 0;
		}

		if (!result)
		{
			toggle_soft_input = (*env)->GetMethodID(env, input_manager_class, "toggleSoftInput", "(II)V");
			if (toggle_soft_input)
			{
				(*env)->CallVoidMethod(env, input_manager, toggle_soft_input,
					0, INPUT_METHOD_MANAGER_HIDE_NOT_ALWAYS);
				if ((*env)->ExceptionCheck(env))
				{
					(*env)->ExceptionClear(env);
				}
				else
				{
					result = 1;
				}
			}
		}
	}

cleanup:
	if (window_token)
		(*env)->DeleteLocalRef(env, window_token);
	if (input_view_own_ref && input_view)
		(*env)->DeleteLocalRef(env, input_view);
	if (content_child)
		(*env)->DeleteLocalRef(env, content_child);
	if (content_root)
		(*env)->DeleteLocalRef(env, content_root);
	if (decor_view)
		(*env)->DeleteLocalRef(env, decor_view);
	if (input_view_class)
		(*env)->DeleteLocalRef(env, input_view_class);
	if (view_group_class)
		(*env)->DeleteLocalRef(env, view_group_class);
	if (window)
		(*env)->DeleteLocalRef(env, window);
	if (window_class)
		(*env)->DeleteLocalRef(env, window_class);
	if (input_manager)
		(*env)->DeleteLocalRef(env, input_manager);
	if (input_manager_class)
		(*env)->DeleteLocalRef(env, input_manager_class);
	if (service_name)
		(*env)->DeleteLocalRef(env, service_name);
	if (activity_class)
		(*env)->DeleteLocalRef(env, activity_class);

	android_release_jni_env(activity, attached);

	return result;
}

static int android_show_keyboard_jni(ANativeActivity *activity)
{
	if (!activity)
		return 0;

	if (!android_toggle_soft_input_jni(activity, 1, ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT | ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED))
		ANativeActivity_showSoftInput(activity, ANATIVEACTIVITY_SHOW_SOFT_INPUT_IMPLICIT | ANATIVEACTIVITY_SHOW_SOFT_INPUT_FORCED);

	return 1;
}

static int android_hide_keyboard_jni(ANativeActivity *activity)
{
	if (!activity)
		return 0;

	if (!android_toggle_soft_input_jni(activity, 0, 0))
		ANativeActivity_hideSoftInput(activity, 0);

	return 1;
}

static int fb_hAndroidApplyPendingKeyboardVisibility(void)
{
	ANativeActivity *activity = NULL;
	int current;
	int request;
	int changed = 0;

	pthread_mutex_lock(&fb_android.mutex);
	request = fb_android.pending_keyboard_visible;
	fb_android.pending_keyboard_visible = -1;
	current = fb_android.keyboard_visible;
	activity = fb_android.activity;
	pthread_mutex_unlock(&fb_android.mutex);

	if (request != 0 && request != 1)
		return 0;

	__android_log_print(ANDROID_LOG_INFO, FB_ANDROID_LOG_TAG,
		"keyboard request apply: current=%d requested=%d", current, request);

	if (request)
	{
		android_log_debug("keyboard request action=show raw=%d", 0, 0, 0, 0, 0, request, request);
		if (!activity || !android_show_keyboard_jni(activity))
			return 0;
	}
	else
	{
		android_log_debug("keyboard request action=hide raw=%d", 0, 0, 0, 0, 0, request, request);
		if (!activity || !android_hide_keyboard_jni(activity))
			return 0;
	}

	pthread_mutex_lock(&fb_android.mutex);
	if (fb_android.keyboard_visible != request)
	{
		fb_android.keyboard_visible = request;
		changed = 1;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	return changed;
}

static int keyboard_button_rect_locked(int *x0, int *y0, int *x1, int *y1)
{
	int win_w = fb_android.window_width > 0 ? fb_android.window_width : fb_android.width;
	int win_h = fb_android.window_height > 0 ? fb_android.window_height : fb_android.height;
	int scale;
	int skip;
	int out_w;
	int out_h;
	int off_x;
	int off_y;
	int x_left;
	int x_right;
	int y_top;
	int y_bottom;

	if (win_w <= 0 || win_h <= 0)
		return 0;

	if (!fb_android.keyboard_button_visible)
		return 0;

	if (framebuffer_viewport_locked(win_w, win_h, &scale, &skip, &out_w, &out_h, &off_x, &off_y))
	{
		x_left = off_x + out_w - FB_ANDROID_KEYBOARD_BUTTON_W;
		x_right = off_x + out_w - 8;
		y_top = 8;
		y_bottom = y_top + FB_ANDROID_KEYBOARD_BUTTON_H;
	}
	else
	{
		x_left = win_w - FB_ANDROID_KEYBOARD_BUTTON_W;
		y_top = 8;
		x_right = win_w - 8;
		y_bottom = y_top + FB_ANDROID_KEYBOARD_BUTTON_H;
	}

	if (x_left < 0)
		x_left = 0;
	if (y_top < 0)
		y_top = 0;
	if (x_right > win_w)
		x_right = win_w;
	if (y_bottom > win_h)
		y_bottom = win_h;

	if (x_left >= x_right || y_top >= y_bottom)
		return 0;

	if (x_right - x_left < 8)
		return 0;

	*x0 = x_left;
	*y0 = y_top;
	*x1 = x_right;
	*y1 = y_bottom;
	return 1;
}

int fb_hAndroidKeyboardButtonHit(float x, float y)
{
	int x0, y0, x1, y1;
	int hit = 0;

	pthread_mutex_lock(&fb_android.mutex);
	if (keyboard_button_rect_locked(&x0, &y0, &x1, &y1))
		hit = ((int)x >= x0 && (int)x < x1 && (int)y >= y0 && (int)y < y1);
	pthread_mutex_unlock(&fb_android.mutex);

	return hit;
}

void fb_hAndroidToggleKeyboard(void)
{
	int changed = 0;

	pthread_mutex_lock(&fb_android.mutex);
	if (fb_android.activity && fb_android.keyboard_enabled)
	{
		android_log_debug("request keyboard toggle from visible=%d", 0, 0, 0, 0, 0, fb_android.keyboard_visible, 0);
		fb_android.pending_keyboard_visible = fb_android.keyboard_visible ? 0 : 1;
		changed = 1;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	if (changed)
	{
		__android_log_print(ANDROID_LOG_INFO, FB_ANDROID_LOG_TAG,
			"keyboard toggle requested by touch");
		fb_hAndroidApplyPendingKeyboardVisibility();
	}
}

void fb_hAndroidSetKeyboardEnabled(int enabled)
{
	int redraw_active;
	int redraw_console;
	int hide_keyboard;

	pthread_mutex_lock(&fb_android.mutex);
	fb_android.keyboard_enabled = enabled ? 1 : 0;
	if (!fb_android.keyboard_enabled)
	{
		fb_android.keyboard_button_visible = 0;
		fb_android.keyboard_button_down = 0;
		fb_android.pending_keyboard_visible = 0;
	}
	hide_keyboard = fb_android.activity && !fb_android.keyboard_enabled;
	redraw_active = fb_android.active && can_render_locked();
	redraw_console = fb_android.console_enabled && !fb_android.active && can_render_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (hide_keyboard)
		fb_hAndroidApplyPendingKeyboardVisibility();

	if (redraw_active)
		fb_hAndroidUpdate();
	else if (redraw_console)
		fb_hAndroidConsoleRender();
}

void fb_hAndroidSetKeyboardButtonVisible(int visible)
{
	int redraw_active;
	int redraw_console;
	int hide_keyboard;

	pthread_mutex_lock(&fb_android.mutex);
	fb_android.keyboard_button_visible = visible ? 1 : 0;
	if (!fb_android.keyboard_button_visible)
	{
		fb_android.keyboard_button_down = 0;
		fb_android.keyboard_enabled = 0;
		fb_android.pending_keyboard_visible = 0;
	}
	hide_keyboard = fb_android.activity && !fb_android.keyboard_enabled;
	redraw_active = fb_android.active && can_render_locked();
	redraw_console = fb_android.console_enabled && !fb_android.active && can_render_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (hide_keyboard)
		fb_hAndroidApplyPendingKeyboardVisibility();

	if (redraw_active)
		fb_hAndroidUpdate();
	else if (redraw_console)
		fb_hAndroidConsoleRender();
}

static float gamepad_clamp_axis(float value)
{
	if (value < -1.0f)
		return -1.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static float gamepad_clamp_trigger(float value)
{
	if (value < 0.0f)
		return 0.0f;
	if (value > 1.0f)
		return 1.0f;
	return value;
}

static FB_ANDROID_GAMEPAD_STATE *gamepad_slot_locked(int device_id, int create)
{
	FB_ANDROID_GAMEPAD_STATE *free_slot = NULL;
	int i;

	if (device_id < 0)
		return NULL;

	for (i = 0; i < FB_ANDROID_GAMEPAD_MAX; ++i)
	{
		if (fb_android.gamepad[i].seen &&
		    fb_android.gamepad[i].device_id == device_id)
			return &fb_android.gamepad[i];

		if (!fb_android.gamepad[i].seen && !free_slot)
			free_slot = &fb_android.gamepad[i];
	}

	if (!create || !free_slot)
		return NULL;

	free_slot->seen = 1;
	free_slot->device_id = device_id;
	free_slot->buttons = 0;
	free_slot->dpad = 0;
	free_slot->left_trigger = 0.0f;
	free_slot->right_trigger = 0.0f;
	memset(free_slot->axis, 0, sizeof(free_slot->axis));
	return free_slot;
}

static ssize_t gamepad_key_button(int32_t keycode)
{
	switch (keycode)
	{
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

static ssize_t gamepad_key_dpad(int32_t keycode)
{
	switch (keycode)
	{
	case AKEYCODE_DPAD_UP: return XPAD_DPAD_UP;
	case AKEYCODE_DPAD_RIGHT: return XPAD_DPAD_RIGHT;
	case AKEYCODE_DPAD_DOWN: return XPAD_DPAD_DOWN;
	case AKEYCODE_DPAD_LEFT: return XPAD_DPAD_LEFT;
	default: return 0;
	}
}

void fb_hAndroidGamepadMotion(const AInputEvent *event)
{
	FB_ANDROID_GAMEPAD_STATE *pad;
	float hat_x;
	float hat_y;

	if (!event)
		return;

	pthread_mutex_lock(&fb_android.mutex);
	pad = gamepad_slot_locked(AInputEvent_getDeviceId(event), TRUE);
	if (!pad)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	pad->axis[0] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0));
	pad->axis[1] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0));
	pad->axis[2] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0));
	pad->axis[3] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0));
	pad->axis[4] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RX, 0));
	pad->axis[5] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RY, 0));
	pad->axis[6] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_BRAKE, 0));
	pad->axis[7] = gamepad_clamp_axis(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_GAS, 0));
	pad->left_trigger = gamepad_clamp_trigger(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_LTRIGGER, 0));
	pad->right_trigger = gamepad_clamp_trigger(AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RTRIGGER, 0));

	hat_x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
	hat_y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);
	pad->dpad = 0;
	if (hat_y < -FB_ANDROID_GAMEPAD_HAT_THRESHOLD)
		pad->dpad |= XPAD_DPAD_UP;
	if (hat_y > FB_ANDROID_GAMEPAD_HAT_THRESHOLD)
		pad->dpad |= XPAD_DPAD_DOWN;
	if (hat_x < -FB_ANDROID_GAMEPAD_HAT_THRESHOLD)
		pad->dpad |= XPAD_DPAD_LEFT;
	if (hat_x > FB_ANDROID_GAMEPAD_HAT_THRESHOLD)
		pad->dpad |= XPAD_DPAD_RIGHT;

	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidGamepadKey(const AInputEvent *event)
{
	FB_ANDROID_GAMEPAD_STATE *pad;
	ssize_t button;
	ssize_t dpad;
	int action;
	int pressed;

	if (!event)
		return;

	action = AKeyEvent_getAction(event);
	if (action != AKEY_EVENT_ACTION_DOWN &&
	    action != AKEY_EVENT_ACTION_UP)
		return;

	pressed = (action == AKEY_EVENT_ACTION_DOWN);
	button = gamepad_key_button(AKeyEvent_getKeyCode(event));
	dpad = gamepad_key_dpad(AKeyEvent_getKeyCode(event));
	if (!button && !dpad)
		return;

	pthread_mutex_lock(&fb_android.mutex);
	pad = gamepad_slot_locked(AInputEvent_getDeviceId(event), TRUE);
	if (!pad)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	if (button) {
		if (pressed)
			pad->buttons |= button;
		else
			pad->buttons &= ~button;
	}

	if (dpad) {
		if (pressed)
			pad->dpad |= dpad;
		else
			pad->dpad &= ~dpad;
	}

	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidSetActivity(ANativeActivity *activity)
{
	ANativeActivity *old_activity;
	jobject old_input_view;
	jobject new_input_view = NULL;
	int keyboard_enabled;

	pthread_mutex_lock(&fb_android.mutex);
	keyboard_enabled = fb_android.keyboard_enabled;
	pthread_mutex_unlock(&fb_android.mutex);

	if (activity && keyboard_enabled)
		new_input_view = android_install_input_view_jni(activity);

	pthread_mutex_lock(&fb_android.mutex);
	old_activity = fb_android.activity;
	old_input_view = fb_android.input_view;
	fb_android.activity = activity;
	fb_android.input_view = new_input_view;
	android_reset_input_text_locked();
	if (activity)
	{
		ANativeActivity_setWindowFlags(activity, 0, AWINDOW_FLAG_ALT_FOCUSABLE_IM);
		fb_android.pending_keyboard_visible = -1;
	}
	if (!activity)
	{
		fb_android.keyboard_visible = 0;
		fb_android.pending_keyboard_visible = -1;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	if (activity && new_input_view)
		android_set_input_text_jni(activity, new_input_view, FB_ANDROID_INPUT_BACKSPACE_PAD);

	android_delete_global_ref(activity ? activity : old_activity, old_input_view);
}

void fb_hAndroidSetWindow(ANativeWindow *window)
{
	ANativeWindow *old_window;
	int redraw_active;
	int redraw_console;

	if (window)
		ANativeWindow_acquire(window);

	pthread_mutex_lock(&fb_android.mutex);
	old_window = fb_android.window;
	fb_android.window = window;
	fb_android.surface_generation++;
	configure_window_locked();
	redraw_active = fb_android.active && can_render_locked();
	redraw_console = fb_android.console_enabled && !fb_android.active && can_render_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (old_window)
		ANativeWindow_release(old_window);

	if (redraw_active)
		fb_hAndroidUpdate();
	else if (redraw_console)
		fb_hAndroidConsoleRender();
}

void fb_hAndroidGfxSetLifecycle(int started, int resumed, int focused)
{
	int redraw_active;
	int redraw_console;

	pthread_mutex_lock(&fb_android.mutex);
	fb_android.started = started ? 1 : 0;
	fb_android.resumed = resumed ? 1 : 0;
	fb_android.focused = focused ? 1 : 0;
	update_render_suspended_locked();
	redraw_active = fb_android.active && can_render_locked();
	redraw_console = fb_android.console_enabled && !fb_android.active && can_render_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (redraw_active)
		fb_hAndroidUpdate();
	else if (redraw_console)
		fb_hAndroidConsoleRender();
}

int fb_hAndroidIsGraphicsActive(void)
{
	int active;

	pthread_mutex_lock(&fb_android.mutex);
	active = fb_android.active;
	pthread_mutex_unlock(&fb_android.mutex);

	return active;
}

int fb_hAndroidInit(char *title, int w, int h, int depth, int refresh_rate, int flags, int require_api26)
{
	(void)title;

	if (w <= 0 || h <= 0 || depth <= 0)
		return 0;

	if (flags & DRIVER_OPENGL)
		return -1;

	if (require_api26 && android_get_device_api_level() < 26)
		return -1;

	pthread_mutex_lock(&fb_android.mutex);

	if (!fb_android.window)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return -1;
	}

	fb_android.width = w;
	fb_android.height = h;
	fb_android.depth = depth;
	fb_android.refresh_rate = refresh_rate > 0 ? refresh_rate : 60;
	fb_android.mouse_x = w / 2;
	fb_android.mouse_y = h / 2;
	fb_android.mouse_z = 0;
	fb_android.mouse_buttons = 0;
	fb_android.mouse_latched_buttons = 0;
	fb_android.blitter = fb_hGetBlitter(32, TRUE);

	if (!fb_android.blitter)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return -1;
	}

	fb_android.active = 1;
	configure_window_locked();
	__fb_gfx->refresh_rate = fb_android.refresh_rate;

	pthread_mutex_unlock(&fb_android.mutex);

	android_log("Android gfx driver initialized");
	fb_hAndroidUpdate();
	return 0;
}

void fb_hAndroidExit(void)
{
	pthread_mutex_lock(&fb_android.mutex);
	fb_android.active = 0;
	fb_android.width = 0;
	fb_android.height = 0;
	fb_android.depth = 0;
	fb_android.blitter = NULL;
	free(fb_android.scale_buffer);
	fb_android.scale_buffer = NULL;
	fb_android.scale_buffer_size = 0;
	update_render_suspended_locked();
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidLock(void)
{
	pthread_mutex_lock(&fb_android.mutex);
	fb_android.display_locked++;
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidUnlock(void)
{
	int needs_update = 0;

	/*
		Android window posts are much more expensive than normal software
		framebuffer writes.  The gfx core marks dirty lines only when the
		visible framebuffer changed, such as during SCREENCOPY or a visible
		page switch.  Drawing into a hidden work page must therefore only
		release the driver lock here; otherwise page-flipped programs end up
		posting the NativeWindow once per primitive.
	*/
	pthread_mutex_lock(&fb_android.mutex);
	if (fb_android.display_locked > 0)
		fb_android.display_locked--;

	if (__fb_gfx && __fb_gfx->dirty)
	{
		int y;

		for (y = 0; y < __fb_gfx->h; y++)
		{
			if (__fb_gfx->dirty[y])
			{
				needs_update = 1;
				break;
			}
		}
	}

	pthread_mutex_unlock(&fb_android.mutex);

	if (needs_update)
		fb_hAndroidUpdate();
}

void fb_hAndroidSetPalette(int index, int r, int g, int b)
{
	(void)index;
	(void)r;
	(void)g;
	(void)b;

	/*
		Android always renders through a 32-bit RGBA window buffer.
		The gfx core owns the software palette in device_palette, and
		the normal 8-bit to 32-bit blitters use that palette directly.
		There is therefore no platform palette to update here.
	*/
}

void fb_hAndroidWaitVSync(void)
{
	int refresh;

	pthread_mutex_lock(&fb_android.mutex);
	refresh = fb_android.refresh_rate > 0 ? fb_android.refresh_rate : 60;
	pthread_mutex_unlock(&fb_android.mutex);

	sleep_ms(1000 / refresh);
}

int fb_hAndroidGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	/*
	 * The gfx core calls driver mouse hooks while DRIVER_LOCK() is held.
	 * On Android that lock is fb_android.mutex, so taking it again here
	 * would deadlock GETMOUSE before the input queue can be drained.
	 */
	*x = fb_android.mouse_x;
	*y = fb_android.mouse_y;
	*z = fb_android.mouse_z;
	*buttons = fb_android.mouse_buttons | fb_android.mouse_latched_buttons;
	fb_android.mouse_latched_buttons = 0;
	*clip = 0;

	return 0;
}

void fb_hAndroidSetMouse(int x, int y, int cursor, int clip)
{
	(void)cursor;
	(void)clip;

	/* Called by the gfx core with DRIVER_LOCK() already held. */
	if (x >= 0)
		fb_android.mouse_x = x;
	if (y >= 0)
		fb_android.mouse_y = y;
}

void fb_hAndroidSetWindowTitle(char *title)
{
	(void)title;
}

int fb_hAndroidSetWindowPos(int x, int y)
{
	(void)x;
	(void)y;
	return 0;
}

int *fb_hAndroidFetchModes(int depth, int *size)
{
	int *modes;

	(void)depth;

	modes = (int *)malloc(sizeof(int) * 3);
	if (!modes)
	{
		*size = 0;
		return NULL;
	}

	modes[0] = (480 << 16) | 320;
	modes[1] = (800 << 16) | 480;
	modes[2] = (1280 << 16) | 720;
	*size = 3;
	return modes;
}

static void android_post_key_to_runtime(int key, int scancode)
{
	EVENT e;
	int can_post;

	if (key <= 0)
		return;

	pthread_mutex_lock(&fb_android.mutex);
	can_post = can_post_input_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (!can_post)
		return;

	DRIVER_LOCK();
	fb_hPostKey(key);
	DRIVER_UNLOCK();

	memset(&e, 0, sizeof(e));
	e.type = EVENT_KEY_PRESS;
	e.scancode = scancode;
	e.ascii = (key >= 32 && key < 127) ? key : 0;
	fb_hPostEvent(&e);

	e.type = EVENT_KEY_RELEASE;
	fb_hPostEvent(&e);
}

static char *android_get_input_text_jni(ANativeActivity *activity, jobject input_view)
{
	JNIEnv *env;
	int attached = 0;
	char *result = NULL;
	const char *utf = NULL;
	jobject local_input_view = NULL;
	jobject editable = NULL;
	jstring string = NULL;
	jclass input_view_class = NULL;
	jclass object_class = NULL;
	jmethodID get_text = NULL;
	jmethodID to_string = NULL;

	if (!activity || !input_view)
		return NULL;

	env = android_get_jni_env(activity, &attached);
	if (!env)
		return NULL;

	local_input_view = (*env)->NewLocalRef(env, input_view);
	if (!local_input_view)
		goto cleanup;

	input_view_class = (*env)->GetObjectClass(env, local_input_view);
	if (!input_view_class)
		goto cleanup;

	get_text = (*env)->GetMethodID(env, input_view_class, "getText", "()Landroid/text/Editable;");
	if (!get_text)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	editable = (*env)->CallObjectMethod(env, local_input_view, get_text);
	if ((*env)->ExceptionCheck(env) || !editable)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	object_class = (*env)->FindClass(env, "java/lang/Object");
	if ((*env)->ExceptionCheck(env) || !object_class)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	to_string = (*env)->GetMethodID(env, object_class, "toString", "()Ljava/lang/String;");
	if (!to_string)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	string = (jstring)(*env)->CallObjectMethod(env, editable, to_string);
	if ((*env)->ExceptionCheck(env) || !string)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	utf = (*env)->GetStringUTFChars(env, string, NULL);
	if (!utf)
		goto cleanup;

	result = strdup(utf);

cleanup:
	if (utf && string)
		(*env)->ReleaseStringUTFChars(env, string, utf);
	if (string)
		(*env)->DeleteLocalRef(env, string);
	if (object_class)
		(*env)->DeleteLocalRef(env, object_class);
	if (editable)
		(*env)->DeleteLocalRef(env, editable);
	if (input_view_class)
		(*env)->DeleteLocalRef(env, input_view_class);
	if (local_input_view)
		(*env)->DeleteLocalRef(env, local_input_view);

	android_release_jni_env(activity, attached);
	return result;
}

static void android_set_input_text_jni(ANativeActivity *activity, jobject input_view, const char *text)
{
	JNIEnv *env;
	int attached = 0;
	jobject local_input_view = NULL;
	jstring value = NULL;
	jclass input_view_class = NULL;
	jmethodID set_text = NULL;
	jmethodID set_selection = NULL;

	if (!activity || !input_view || !text)
		return;

	env = android_get_jni_env(activity, &attached);
	if (!env)
		return;

	local_input_view = (*env)->NewLocalRef(env, input_view);
	if (!local_input_view)
		goto cleanup;

	input_view_class = (*env)->GetObjectClass(env, local_input_view);
	if (!input_view_class)
		goto cleanup;

	set_text = (*env)->GetMethodID(env, input_view_class, "setText", "(Ljava/lang/CharSequence;)V");
	if (!set_text)
	{
		(*env)->ExceptionClear(env);
		goto cleanup;
	}

	value = (*env)->NewStringUTF(env, text);
	if (!value)
		goto cleanup;

	(*env)->CallVoidMethod(env, local_input_view, set_text, value);
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

	set_selection = (*env)->GetMethodID(env, input_view_class, "setSelection", "(I)V");
	if (set_selection)
		(*env)->CallVoidMethod(env, local_input_view, set_selection, (jint)strlen(text));
	if ((*env)->ExceptionCheck(env))
		(*env)->ExceptionClear(env);

cleanup:
	if (value)
		(*env)->DeleteLocalRef(env, value);
	if (input_view_class)
		(*env)->DeleteLocalRef(env, input_view_class);
	if (local_input_view)
		(*env)->DeleteLocalRef(env, local_input_view);

	android_release_jni_env(activity, attached);
}

static void android_reset_input_text_locked(void)
{
	free(fb_android.input_text);
	fb_android.input_text = strdup(FB_ANDROID_INPUT_BACKSPACE_PAD);
}

static int android_process_input_text(const char *text)
{
	char *old_text;
	size_t old_len;
	size_t new_len;
	size_t prefix = 0;
	size_t i;
	int changed = 0;
	int clear_after_enter = 0;

	if (!text)
		return 0;

	pthread_mutex_lock(&fb_android.mutex);
	old_text = fb_android.input_text ? strdup(fb_android.input_text) : strdup("");
	pthread_mutex_unlock(&fb_android.mutex);

	if (!old_text)
		return 0;

	old_len = strlen(old_text);
	new_len = strlen(text);

	while (prefix < old_len && prefix < new_len && old_text[prefix] == text[prefix])
		prefix++;

	for (i = old_len; i > prefix; --i)
	{
		android_post_key_to_runtime(KEY_BACKSPACE, SC_BACKSPACE);
		changed = 1;
	}

	for (i = prefix; i < new_len; ++i)
	{
		unsigned char ch = (unsigned char)text[i];

		if (ch == '\r' || ch == '\n')
		{
			android_post_key_to_runtime('\r', SC_ENTER);
			clear_after_enter = 1;
			changed = 1;
			continue;
		}

		if (ch == '\b')
		{
			android_post_key_to_runtime(KEY_BACKSPACE, SC_BACKSPACE);
			changed = 1;
			continue;
		}

		if (ch == '\t' || (ch >= 32 && ch < 127))
		{
			android_post_key_to_runtime(ch, 0);
			changed = 1;
		}
	}

	pthread_mutex_lock(&fb_android.mutex);
	free(fb_android.input_text);
	fb_android.input_text = clear_after_enter ? strdup("") : strdup(text);
	pthread_mutex_unlock(&fb_android.mutex);

	free(old_text);
	return changed;
}

static int fb_hAndroidPollTextInput(void)
{
	ANativeActivity *activity;
	jobject input_view;
	char *text;
	int changed;

	pthread_mutex_lock(&fb_android.mutex);
	activity = fb_android.activity;
	input_view = fb_android.input_view;
	pthread_mutex_unlock(&fb_android.mutex);

	if (!activity || !input_view)
		return 0;

	text = android_get_input_text_jni(activity, input_view);
	if (!text)
		return 0;

	changed = android_process_input_text(text);
	free(text);

	/*
	 * The hidden EditText exists only as an IME bridge.  It is not the line
	 * editor's source of truth.
	 *
	 * Gboard sends Backspace to the served Java view instead of to the native
	 * input queue.  Keep a small invisible pad in the EditText so Backspace
	 * produces an observable text deletion.  After harvesting characters or
	 * pad deletions, restore the pad and reset the native snapshot.
	 */
	if (changed)
	{
		android_set_input_text_jni(activity, input_view, FB_ANDROID_INPUT_BACKSPACE_PAD);
		pthread_mutex_lock(&fb_android.mutex);
		android_reset_input_text_locked();
		pthread_mutex_unlock(&fb_android.mutex);
	}

	return changed;
}

void fb_hAndroidPollEvents(void)
{
	int redraw_active;
	int redraw_console;
	int keyboard_changed;

	keyboard_changed = fb_hAndroidApplyPendingKeyboardVisibility();
	fb_hAndroidPollTextInput();

	if (!keyboard_changed)
		return;

	pthread_mutex_lock(&fb_android.mutex);
	redraw_active = fb_android.active && can_render_locked();
	redraw_console = fb_android.console_enabled && !fb_android.active && can_render_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (redraw_active)
		fb_hAndroidUpdate();
	else if (redraw_console)
		fb_hAndroidConsoleRender();
}

static void draw_rect(ANativeWindow_Buffer *buffer, int x0, int y0, int x1, int y1, uint32_t color)
{
	int x, y;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > buffer->width) x1 = buffer->width;
	if (y1 > buffer->height) y1 = buffer->height;

	for (y = y0; y < y1; ++y)
	{
		uint32_t *row = (uint32_t *)buffer->bits + (y * buffer->stride);
		for (x = x0; x < x1; ++x)
			row[x] = color;
	}
}

static int ensure_scale_buffer_locked(size_t bytes)
{
	unsigned char *new_buffer;

	if (bytes == 0)
		return 0;

	if (fb_android.scale_buffer_size >= bytes)
		return 1;

	new_buffer = (unsigned char *)realloc(fb_android.scale_buffer, bytes);
	if (!new_buffer)
	{
		free(fb_android.scale_buffer);
		fb_android.scale_buffer = NULL;
		fb_android.scale_buffer_size = 0;
		return 0;
	}

	fb_android.scale_buffer = new_buffer;
	fb_android.scale_buffer_size = bytes;
	return 1;
}

static void mark_framebuffer_dirty_locked(void)
{
	if (__fb_gfx && __fb_gfx->dirty)
		fb_hMemSet(__fb_gfx->dirty, TRUE, __fb_gfx->h);
}

static void clear_framebuffer_dirty_locked(void)
{
	if (__fb_gfx && __fb_gfx->dirty)
		fb_hMemSet(__fb_gfx->dirty, FALSE, __fb_gfx->h);
}

static void make_pixels_opaque(uint32_t *pixels, int width, int height)
{
	size_t count;
	size_t i;

	if (!pixels || width <= 0 || height <= 0)
		return;

	if ((size_t)width > SIZE_MAX / (size_t)height)
		return;

	/*
		The shared gfxlib blitters produce 32-bit RGB pixels with no
		alpha channel. Android RGBA_8888 surfaces are composited by the
		window manager, so an alpha value of zero makes valid legacy
		palette output disappear.
	*/
	count = (size_t)width * (size_t)height;
	for (i = 0; i < count; i++)
		pixels[i] |= 0xff000000u;
}

static int divide_round_up(int value, int divisor)
{
	if (divisor <= 0)
		return value;
	return (value + divisor - 1) / divisor;
}

static int framebuffer_viewport_locked(int dst_w, int dst_h, int *scale, int *skip,
	int *out_w, int *out_h, int *off_x, int *off_y)
{
	int src_w = fb_android.width;
	int src_h = fb_android.height;
	int local_scale;
	int local_skip;

	if (src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return 0;

	local_scale = dst_w / src_w;
	if (dst_h / src_h < local_scale)
		local_scale = dst_h / src_h;

	if (local_scale >= 1)
	{
		*scale = local_scale;
		*skip = 1;
		*out_w = src_w * local_scale;
		*out_h = src_h * local_scale;
		*off_x = (dst_w - *out_w) / 2;
		*off_y = (dst_h - *out_h) / 2;
		return 1;
	}

	local_skip = divide_round_up(src_w, dst_w);
	if (divide_round_up(src_h, dst_h) > local_skip)
		local_skip = divide_round_up(src_h, dst_h);
	if (local_skip < 1)
		local_skip = 1;

	*scale = 0;
	*skip = local_skip;
	*out_w = divide_round_up(src_w, local_skip);
	*out_h = divide_round_up(src_h, local_skip);
	if (*out_w > dst_w)
		*out_w = dst_w;
	if (*out_h > dst_h)
		*out_h = dst_h;
	*off_x = (dst_w - *out_w) / 2;
	*off_y = (dst_h - *out_h) / 2;
	return 1;
}

static int clamp_framebuffer_x_locked(int x)
{
	if (x < 0)
		return 0;
	if (x >= fb_android.width)
		return fb_android.width - 1;
	return x;
}

static int clamp_framebuffer_y_locked(int y)
{
	if (y < 0)
		return 0;
	if (y >= fb_android.height)
		return fb_android.height - 1;
	return y;
}

static void map_window_to_framebuffer_locked(float x, float y, int *mapped_x, int *mapped_y)
{
	int win_w = fb_android.window_width > 0 ? fb_android.window_width : fb_android.width;
	int win_h = fb_android.window_height > 0 ? fb_android.window_height : fb_android.height;
	int scale, skip, out_w, out_h, off_x, off_y;
	int local_x, local_y;

	if (!framebuffer_viewport_locked(win_w, win_h, &scale, &skip, &out_w, &out_h, &off_x, &off_y))
	{
		*mapped_x = (int)x;
		*mapped_y = (int)y;
		return;
	}

	local_x = (int)x - off_x;
	local_y = (int)y - off_y;

	if ((local_x < 0) || (local_y < 0) || (local_x >= out_w) || (local_y >= out_h))
	{
		*mapped_x = -1;
		*mapped_y = -1;
		return;
	}

	if (scale >= 1)
	{
		*mapped_x = clamp_framebuffer_x_locked(local_x / scale);
		*mapped_y = clamp_framebuffer_y_locked(local_y / scale);
		return;
	}

	*mapped_x = clamp_framebuffer_x_locked(local_x * skip);
	*mapped_y = clamp_framebuffer_y_locked(local_y * skip);
}

static void draw_scaled_framebuffer_locked(ANativeWindow_Buffer *buffer)
{
	int src_w = fb_android.width;
	int src_h = fb_android.height;
	int src_pitch;
	int scale;
	int skip;
	int out_w;
	int out_h;
	int off_x;
	int off_y;
	int x, y;
	size_t bytes;
	uint32_t *src32;

	if (!buffer || !fb_android.blitter || src_w <= 0 || src_h <= 0 ||
	    buffer->width <= 0 || buffer->height <= 0)
		return;

	if ((size_t)src_w > (SIZE_MAX / 4u) / (size_t)src_h)
		return;

	bytes = (size_t)src_w * (size_t)src_h * 4u;
	if (!ensure_scale_buffer_locked(bytes))
		return;

	mark_framebuffer_dirty_locked();

	src_pitch = src_w * 4;
	fb_android.blitter(fb_android.scale_buffer, src_pitch);
	src32 = (uint32_t *)fb_android.scale_buffer;
	make_pixels_opaque(src32, src_w, src_h);
	clear_framebuffer_dirty_locked();

	draw_rect(buffer, 0, 0, buffer->width, buffer->height, rgba(0, 0, 0));

	if (!framebuffer_viewport_locked(buffer->width, buffer->height, &scale, &skip,
	    &out_w, &out_h, &off_x, &off_y))
		return;

	if (scale >= 1)
	{
		for (y = 0; y < out_h; ++y)
		{
			uint32_t *dst = (uint32_t *)buffer->bits + ((off_y + y) * buffer->stride) + off_x;
			uint32_t *src = src32 + ((y / scale) * src_w);

			for (x = 0; x < out_w; ++x)
				dst[x] = src[x / scale];
		}
		return;
	}

	for (y = 0; y < out_h; ++y)
	{
		uint32_t *dst = (uint32_t *)buffer->bits + ((off_y + y) * buffer->stride) + off_x;
		uint32_t *src = src32 + ((y * skip) * src_w);

		for (x = 0; x < out_w; ++x)
			dst[x] = src[x * skip];
	}
}

static unsigned glyph_row(char c, int row)
{
	static const unsigned blank[7] = {0, 0, 0, 0, 0, 0, 0};
	static const unsigned dash[7] = {0, 0, 0, 31, 0, 0, 0};
	static const unsigned colon[7] = {0, 4, 4, 0, 4, 4, 0};
	static const unsigned dot[7] = {0, 0, 0, 0, 0, 12, 12};
	static const unsigned zero[7] = {14, 17, 19, 21, 25, 17, 14};
	static const unsigned one[7] = {4, 12, 4, 4, 4, 4, 14};
	static const unsigned two[7] = {14, 17, 1, 2, 4, 8, 31};
	static const unsigned three[7] = {30, 1, 1, 14, 1, 1, 30};
	static const unsigned four[7] = {2, 6, 10, 18, 31, 2, 2};
	static const unsigned five[7] = {31, 16, 30, 1, 1, 17, 14};
	static const unsigned six[7] = {6, 8, 16, 30, 17, 17, 14};
	static const unsigned seven[7] = {31, 1, 2, 4, 8, 8, 8};
	static const unsigned eight[7] = {14, 17, 17, 14, 17, 17, 14};
	static const unsigned nine[7] = {14, 17, 17, 15, 1, 2, 12};
	static const unsigned letters[26][7] =
	{
		{14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
		{30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
		{14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
		{7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
		{17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
		{30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
		{15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
		{17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
		{17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
	};
	const unsigned *g = blank;

	if (row <= 0 || row >= 8)
		return 0;

	row--;
	if (c >= 'a' && c <= 'z')
		c = (char)(c - 'a' + 'A');

	if (c >= 'A' && c <= 'Z')
		g = letters[c - 'A'];
	else if (c >= '0' && c <= '9')
	{
		switch (c)
		{
		case '0':
			g = zero;
			break;
		case '1':
			g = one;
			break;
		case '2':
			g = two;
			break;
		case '3':
			g = three;
			break;
		case '4':
			g = four;
			break;
		case '5':
			g = five;
			break;
		case '6':
			g = six;
			break;
		case '7':
			g = seven;
			break;
		case '8':
			g = eight;
			break;
		default:
			g = nine;
			break;
		}
	}
	else if (c == '-')
		g = dash;
	else if (c == ':')
		g = colon;
	else if (c == '.')
		g = dot;

	return g[row];
}

static void draw_char(ANativeWindow_Buffer *buffer, int px, int py, char ch, uint32_t fg, uint32_t bg)
{
	int x, y;

	draw_rect(buffer, px, py, px + FB_ANDROID_FONT_W, py + FB_ANDROID_FONT_H, bg);

	for (y = 0; y < 8; ++y)
	{
		unsigned bits = glyph_row(ch, y);
		int yy = py + y;

		if (yy < 0 || yy >= buffer->height)
			continue;

		for (x = 0; x < 5; ++x)
		{
			int xx = px + x;
			if (xx < 0 || xx >= buffer->width)
				continue;
			if (bits & (1u << (4 - x)))
				*((uint32_t *)buffer->bits + yy * buffer->stride + xx) = fg;
		}
	}
}

static void draw_text(ANativeWindow_Buffer *buffer, int x, int y, const char *text, uint32_t fg, uint32_t bg)
{
	while (*text)
	{
		draw_char(buffer, x, y, *text, fg, bg);
		x += FB_ANDROID_FONT_W;
		text++;
	}
}

static void draw_keyboard_button_locked(ANativeWindow_Buffer *buffer)
{
	int x0, y0, x1, y1;
	uint32_t fill;

	if (!keyboard_button_rect_locked(&x0, &y0, &x1, &y1))
		return;

	if (fb_android.keyboard_button_down)
		fill = rgba(94, 128, 200);
	else if (fb_android.keyboard_visible)
		fill = rgba(40, 120, 180);
	else
		fill = rgba(54, 59, 68);
	draw_rect(buffer, x0, y0, x1, y1, rgba(16, 18, 24));
	draw_rect(buffer, x0 + 2, y0 + 2, x1 - 2, y1 - 2, fill);
	draw_text(buffer, x0 + 10, y0 + 16, "KB", rgba(245, 246, 250), fill);
}

void fb_hAndroidUpdate(void)
{
	ANativeWindow_Buffer buffer;
	ANativeWindow *window;

	pthread_mutex_lock(&fb_android.mutex);
	window = fb_android.window;
	if (fb_android.display_locked || !fb_android.active || !can_render_locked() ||
	    !window || !fb_android.blitter || !__fb_gfx || !__fb_gfx->framebuffer)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	if (ANativeWindow_lock(window, &buffer, NULL) != 0)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	draw_scaled_framebuffer_locked(&buffer);
	draw_keyboard_button_locked(&buffer);
	ANativeWindow_unlockAndPost(window);
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidSetConsoleEnabled(int enabled)
{
	int redraw_console;

	pthread_mutex_lock(&fb_android.mutex);
	fb_android.console_enabled = enabled ? 1 : 0;
	redraw_console = fb_android.console_enabled && !fb_android.active && can_render_locked();
	pthread_mutex_unlock(&fb_android.mutex);

	if (redraw_console)
		fb_hAndroidConsoleRender();
}

void fb_hAndroidConsoleRender(void)
{
	ANativeWindow_Buffer buffer;
	ANativeWindow *window;
	int cols, rows, first_line, i, line;

	pthread_mutex_lock(&fb_android.mutex);
	window = fb_android.window;
	if (!fb_android.console_enabled || fb_android.active || !can_render_locked() || !window)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);
	update_window_size_locked();
	if (ANativeWindow_lock(window, &buffer, NULL) != 0)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	draw_rect(&buffer, 0, 0, buffer.width, buffer.height, rgba(9, 12, 18));
	cols = buffer.width / FB_ANDROID_FONT_W;
	rows = buffer.height / FB_ANDROID_FONT_H;
	if (cols > FB_ANDROID_CONSOLE_COLS - 1)
		cols = FB_ANDROID_CONSOLE_COLS - 1;

	first_line = fb_android.console_line - rows + 2;
	if (first_line < 0)
		first_line += FB_ANDROID_CONSOLE_LINES;

	for (i = 0; i < rows - 1; ++i)
	{
		line = (first_line + i) % FB_ANDROID_CONSOLE_LINES;
		fb_android.console[line][cols] = '\0';
		draw_text(&buffer, 6, 6 + i * FB_ANDROID_FONT_H, fb_android.console[line],
			rgba(224, 235, 245), rgba(9, 12, 18));
	}

	draw_keyboard_button_locked(&buffer);
	ANativeWindow_unlockAndPost(window);
	pthread_mutex_unlock(&fb_android.mutex);
}

void fb_hAndroidConsoleWrite(const char *text, size_t length)
{
	size_t i;

	if (!text || length == 0)
		return;

	pthread_mutex_lock(&fb_android.mutex);
	if (!fb_android.console_enabled)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	for (i = 0; i < length; ++i)
	{
		unsigned char ch = (unsigned char)text[i];

		if (ch == '\r')
			continue;

		if (ch == '\n')
		{
			fb_android.console_line = (fb_android.console_line + 1) % FB_ANDROID_CONSOLE_LINES;
			fb_android.console_col = 0;
			memset(fb_android.console[fb_android.console_line], 0, FB_ANDROID_CONSOLE_COLS);
			continue;
		}

		if (ch == '\b')
		{
			if (fb_android.console_col > 0)
				fb_android.console[fb_android.console_line][--fb_android.console_col] = '\0';
			continue;
		}

		if (fb_android.console_col >= FB_ANDROID_CONSOLE_COLS - 1)
		{
			fb_android.console_line = (fb_android.console_line + 1) % FB_ANDROID_CONSOLE_LINES;
			fb_android.console_col = 0;
			memset(fb_android.console[fb_android.console_line], 0, FB_ANDROID_CONSOLE_COLS);
		}

		if (ch >= 32 && ch < 127)
			fb_android.console[fb_android.console_line][fb_android.console_col++] = (char)ch;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	fb_hAndroidConsoleRender();
}

void fb_hAndroidTouch(float x, float y, int action)
{
	EVENT e;
	int mapped_x, mapped_y;
	int x0, y0, x1, y1;
	int toggle_keyboard = 0;
	int is_button_hit = 0;
	int should_redraw = 0;

	pthread_mutex_lock(&fb_android.mutex);
	is_button_hit = keyboard_button_rect_locked(&x0, &y0, &x1, &y1) &&
	    (int)x >= x0 && (int)x < x1 && (int)y >= y0 && (int)y < y1;
	if (is_button_hit)
	{
		android_log_debug("touch hit keyboard btn rect=%d,%d-%d,%d action=%d pt=%d,%d", x0, y0, x1, y1, action, (int)x, (int)y);
		__android_log_print(ANDROID_LOG_INFO, FB_ANDROID_LOG_TAG,
			"keyboard button hit action=%d pt=%d,%d rect=%d,%d-%d,%d",
			action, (int)x, (int)y, x0, y0, x1, y1);
		if (action == AMOTION_EVENT_ACTION_DOWN)
		{
			fb_android.keyboard_button_down = 1;
			should_redraw = 1;
		}
		else if (action == AMOTION_EVENT_ACTION_MOVE && fb_android.keyboard_button_down)
		{
			if ((int)x < x0 || (int)x >= x1 || (int)y < y0 || (int)y >= y1)
			{
				fb_android.keyboard_button_down = 0;
				should_redraw = 1;
			}
		}
		else if (action == AMOTION_EVENT_ACTION_UP && fb_android.keyboard_button_down)
		{
			fb_android.keyboard_button_down = 0;
			should_redraw = 1;
			toggle_keyboard = 1;
		}
		else if (action == AMOTION_EVENT_ACTION_CANCEL)
		{
			fb_android.keyboard_button_down = 0;
			should_redraw = 1;
		}

		pthread_mutex_unlock(&fb_android.mutex);

		if (should_redraw)
			fb_hAndroidUpdate();

		if (toggle_keyboard)
		{
			fb_hAndroidToggleKeyboard();
		}
		return;
	}

	map_window_to_framebuffer_locked(x, y, &mapped_x, &mapped_y);
	if ((mapped_x < 0) || (mapped_y < 0))
	{
		fb_android.mouse_buttons &= ~BUTTON_LEFT;
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}

	fb_android.mouse_x = mapped_x;
	fb_android.mouse_y = mapped_y;

	if (action == AMOTION_EVENT_ACTION_DOWN)
	{
		fb_android.mouse_buttons |= BUTTON_LEFT;
		fb_android.mouse_latched_buttons |= BUTTON_LEFT;
	}
	else if (action == AMOTION_EVENT_ACTION_MOVE)
	{
		fb_android.mouse_buttons |= BUTTON_LEFT;
	}
	else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
	{
		fb_android.mouse_buttons &= ~BUTTON_LEFT;
	}

	if (!can_post_input_locked())
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	memset(&e, 0, sizeof(e));
	if (action == AMOTION_EVENT_ACTION_MOVE)
	{
		e.type = EVENT_MOUSE_MOVE;
		e.x = mapped_x;
		e.y = mapped_y;
	}
	else if (action == AMOTION_EVENT_ACTION_DOWN)
	{
		e.type = EVENT_MOUSE_BUTTON_PRESS;
		e.button = BUTTON_LEFT;
		e.x = mapped_x;
		e.y = mapped_y;
	}
	else if (action == AMOTION_EVENT_ACTION_UP)
	{
		e.type = EVENT_MOUSE_BUTTON_RELEASE;
		e.button = BUTTON_LEFT;
		e.x = mapped_x;
		e.y = mapped_y;
	}
	else
		return;
	fb_hPostEvent(&e);
}

static int android_key_to_scancode(int32_t keycode)
{
	if (keycode >= AKEYCODE_A && keycode <= AKEYCODE_Z)
		return SC_A + (keycode - AKEYCODE_A);
	if (keycode >= AKEYCODE_1 && keycode <= AKEYCODE_9)
		return SC_1 + (keycode - AKEYCODE_1);
	if (keycode == AKEYCODE_0)
		return SC_0;

	switch (keycode)
	{
	case AKEYCODE_ESCAPE: return SC_ESCAPE;
	case AKEYCODE_DEL: return SC_BACKSPACE;
	case AKEYCODE_TAB: return SC_TAB;
	case AKEYCODE_ENTER: return SC_ENTER;
	case AKEYCODE_SPACE: return SC_SPACE;
	case AKEYCODE_MINUS: return SC_MINUS;
	case AKEYCODE_EQUALS: return SC_EQUALS;
	case AKEYCODE_DPAD_LEFT: return SC_LEFT;
	case AKEYCODE_DPAD_RIGHT: return SC_RIGHT;
	case AKEYCODE_DPAD_UP: return SC_UP;
	case AKEYCODE_DPAD_DOWN: return SC_DOWN;
	default: return 0;
	}
}

static int android_key_to_ascii(int32_t keycode)
{
	switch (keycode)
	{
	case AKEYCODE_DEL: return KEY_BACKSPACE;
	case AKEYCODE_TAB: return KEY_TAB;
	case AKEYCODE_ENTER: return '\r';
	case AKEYCODE_SPACE: return ' ';
	default: return 0;
	}
}

void fb_hAndroidKey(int32_t keycode, int action, int unicode)
{
	EVENT e;
	int scancode;
	int key = 0;

	pthread_mutex_lock(&fb_android.mutex);
	if (!can_post_input_locked())
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return;
	}
	pthread_mutex_unlock(&fb_android.mutex);

	scancode = android_key_to_scancode(keycode);
	if (!scancode && unicode <= 0)
		return;

	if (unicode > 0 && unicode <= 255)
		key = unicode;
	else
		key = android_key_to_ascii(keycode);

	if (!key && scancode)
		key = fb_hScancodeToExtendedKey(scancode);

	memset(&e, 0, sizeof(e));
	if (action == AKEY_EVENT_ACTION_DOWN)
	{
		e.type = EVENT_KEY_PRESS;
		if (key > 0)
		{
			DRIVER_LOCK();
			fb_hPostKey(key);
			DRIVER_UNLOCK();
		}
	}
	else if (action == AKEY_EVENT_ACTION_UP)
		e.type = EVENT_KEY_RELEASE;
	else
		return;

	e.scancode = scancode;
	e.ascii = (unicode >= 32 && unicode < 127) ? unicode : 0;
	fb_hPostEvent(&e);
}

JNIEXPORT jboolean JNICALL Java_org_freebasic_android_FreeBasicNativeActivity_nativeDispatchImeKey
	(JNIEnv *env, jclass cls, jint keycode, jint action, jint unicode)
{
	(void)env;
	(void)cls;

	fb_hAndroidKey((int32_t)keycode, (int)action, (int)unicode);
	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL Java_org_freebasic_android_FreeBasicInputView_nativeDispatchImeKey
	(JNIEnv *env, jclass cls, jint keycode, jint action, jint unicode)
{
	(void)env;
	(void)cls;

	fb_hAndroidKey((int32_t)keycode, (int)action, (int)unicode);
	return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_org_freebasic_android_FreeBasicNativeActivity_nativeSetKeyboardButtonVisible
	(JNIEnv *env, jclass cls, jboolean visible)
{
	(void)env;
	(void)cls;

	fb_hAndroidSetKeyboardButtonVisible(visible == JNI_TRUE);
}

int fb_hAndroidGetJoystick(int id, ssize_t *buttons,
						   float *a1, float *a2,
						   float *a3, float *a4,
						   float *a5, float *a6,
						   float *a7, float *a8)
{
	FB_ANDROID_GAMEPAD_STATE *pad;

	if ((id < 0) || (id >= FB_ANDROID_GAMEPAD_MAX))
		return FALSE;

	pthread_mutex_lock(&fb_android.mutex);
	pad = &fb_android.gamepad[id];
	if (!pad->seen)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return FALSE;
	}

	if (buttons)
		*buttons = pad->buttons;
	if (a1)
		*a1 = pad->axis[0];
	if (a2)
		*a2 = pad->axis[1];
	if (a3)
		*a3 = pad->axis[2];
	if (a4)
		*a4 = pad->axis[3];
	if (a5)
		*a5 = pad->axis[4];
	if (a6)
		*a6 = pad->axis[5];
	if (a7)
		*a7 = pad->axis[6];
	if (a8)
		*a8 = pad->axis[7];

	pthread_mutex_unlock(&fb_android.mutex);
	return TRUE;
}

int fb_hAndroidGetXPad(int id, ssize_t *buttons,
					   float *lstick_x, float *lstick_y,
					   float *rstick_x, float *rstick_y,
					   float *ltrigger, float *rtrigger,
					   ssize_t *dpad)
{
	FB_ANDROID_GAMEPAD_STATE *pad;

	if ((id < 0) || (id >= FB_ANDROID_GAMEPAD_MAX))
		return XPAD_STATUS_MISSING;

	pthread_mutex_lock(&fb_android.mutex);
	pad = &fb_android.gamepad[id];
	if (!pad->seen)
	{
		pthread_mutex_unlock(&fb_android.mutex);
		return XPAD_STATUS_MISSING;
	}

	if (buttons) {
		*buttons = pad->buttons;
		if (pad->left_trigger > FB_ANDROID_GAMEPAD_TRIGGER_BUTTON_THRESHOLD)
			*buttons |= XPAD_BUTTON_L2;
		if (pad->right_trigger > FB_ANDROID_GAMEPAD_TRIGGER_BUTTON_THRESHOLD)
			*buttons |= XPAD_BUTTON_R2;
	}
	if (lstick_x)
		*lstick_x = pad->axis[0];
	if (lstick_y)
		*lstick_y = pad->axis[1];
	if (rstick_x)
		*rstick_x = pad->axis[2];
	if (rstick_y)
		*rstick_y = pad->axis[3];
	if (ltrigger)
		*ltrigger = pad->left_trigger;
	if (rtrigger)
		*rtrigger = pad->right_trigger;
	if (dpad)
		*dpad = pad->dpad;

	pthread_mutex_unlock(&fb_android.mutex);
	return XPAD_STATUS_CONNECTED;
}

void fb_hAndroidScreenInfo(ssize_t *width, ssize_t *height, ssize_t *depth, ssize_t *refresh)
{
	pthread_mutex_lock(&fb_android.mutex);
	update_window_size_locked();
	*width = fb_android.window_width > 0 ? fb_android.window_width : 800;
	*height = fb_android.window_height > 0 ? fb_android.window_height : 480;
	*depth = 32;
	*refresh = 60;
	pthread_mutex_unlock(&fb_android.mutex);
}

ssize_t fb_hGetWindowHandle(void)
{
	return 0;
}

ssize_t fb_hGetDisplayHandle(void)
{
	return 0;
}
