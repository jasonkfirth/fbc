#include <android/input.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FB_ANDROID_LOG_TAG "FreeBASIC"
#define FB_ANDROID_LOOPER_INPUT 1
#define FB_ANDROID_LOOPER_STDOUT 2

#ifndef FB_ANDROID_KEYBOARD_ENABLED
#define FB_ANDROID_KEYBOARD_ENABLED 1
#endif

#if defined(__GNUC__)
#define FB_ANDROID_WEAK __attribute__((weak))
#else
#define FB_ANDROID_WEAK
#endif

int fb_android_program_main(int argc, char **argv) FB_ANDROID_WEAK;
int FB_ANDROID_PROGRAM_MAIN(int argc, char **argv) FB_ANDROID_WEAK;
extern const int __fb_app_mode_gui_enabled FB_ANDROID_WEAK;
void fb_AllocateMainFBThread(void) FB_ANDROID_WEAK;
void fb_hAndroidSetActivity(ANativeActivity *activity) FB_ANDROID_WEAK;
void fb_hAndroidSetKeyboardEnabled(int enabled) FB_ANDROID_WEAK;
void fb_hAndroidSetWindow(ANativeWindow *window) FB_ANDROID_WEAK;
void fb_hAndroidTouch(float x, float y, int action) FB_ANDROID_WEAK;
void fb_hAndroidTouchEvent(const AInputEvent *event) FB_ANDROID_WEAK;
void fb_hAndroidKey(int32_t keycode, int action, int unicode) FB_ANDROID_WEAK;
void fb_hAndroidGamepadMotion(const AInputEvent *event) FB_ANDROID_WEAK;
void fb_hAndroidGamepadKey(const AInputEvent *event) FB_ANDROID_WEAK;
void fb_hAndroidSetConsoleEnabled(int enabled) FB_ANDROID_WEAK;
void fb_hAndroidConsoleWrite(const char *text, size_t length) FB_ANDROID_WEAK;
void fb_hAndroidConsoleRender(void) FB_ANDROID_WEAK;
void fb_hAndroidUpdate(void) FB_ANDROID_WEAK;
void fb_hAndroidGfxSetLifecycle(int started, int resumed, int focused) FB_ANDROID_WEAK;
void fb_hAndroidSfxSetLifecycle(int started, int resumed) FB_ANDROID_WEAK;
int fb_sfxEnsureInit(void) FB_ANDROID_WEAK;

/*
 * FreeBASIC's generated entry calls fb_End(), which calls exit(). In an
 * Android NativeActivity that would kill the whole process from the program
 * thread before the bridge can report the program's result to logcat.
 */
static __thread jmp_buf *fb_android_exit_jump;
static __thread int fb_android_exit_status;

typedef struct FB_ANDROID_APP
{
	ANativeActivity *activity;
	ALooper *looper;
	AInputQueue *input_queue;
	ANativeWindow *window;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t looper_thread;
	pthread_t program_thread;
	int looper_thread_started;
	int program_thread_started;
	int destroyed;
	int stdio_ready;
	int started;
	int resumed;
	int focused;
	int stdout_pipe[2];
} FB_ANDROID_APP;

static void fb_android_log(const char *text)
{
	if (text && *text)
		__android_log_write(ANDROID_LOG_INFO, FB_ANDROID_LOG_TAG, text);
}

static int fb_android_console_enabled(void)
{
	return &__fb_app_mode_gui_enabled ? 0 : 1;
}

static void fb_android_apply_app_mode(void)
{
	if (fb_hAndroidSetConsoleEnabled)
		fb_hAndroidSetConsoleEnabled(fb_android_console_enabled());
}

static void fb_android_render_console_if_enabled(void)
{
	if (fb_android_console_enabled() && fb_hAndroidConsoleRender)
		fb_hAndroidConsoleRender();
}

static int fb_android_mkdir_p(const char *path)
{
	char buffer[1024];
	size_t len;
	char *p;

	if (!path || !*path)
		return -1;

	len = strlen(path);
	if (len >= sizeof(buffer))
		return -1;

	memcpy(buffer, path, len + 1);
	for (p = buffer + 1; *p; ++p)
	{
		if (*p != '/')
			continue;
		*p = '\0';
		if (mkdir(buffer, 0700) != 0 && errno != EEXIST)
			return -1;
		*p = '/';
	}

	if (mkdir(buffer, 0700) != 0 && errno != EEXIST)
		return -1;

	return 0;
}

static int fb_android_join_path(char *dst, size_t dst_len, const char *left, const char *right)
{
	int written;

	if (!dst || !left || !right)
		return -1;

	if (*right == '\0')
		written = snprintf(dst, dst_len, "%s", left);
	else if (*left == '\0')
		written = snprintf(dst, dst_len, "%s", right);
	else
		written = snprintf(dst, dst_len, "%s/%s", left, right);

	if (written < 0 || (size_t)written >= dst_len)
		return -1;

	return 0;
}

static int fb_android_extract_asset_file(AAssetManager *assets, const char *asset_name, const char *dst_name)
{
	AAsset *asset;
	FILE *out;
	char parent[1024];
	char *slash;
	char buffer[4096];
	int ok = 1;

	asset = AAssetManager_open(assets, asset_name, AASSET_MODE_STREAMING);
	if (!asset)
		return 0;

	if (strlen(dst_name) >= sizeof(parent))
	{
		AAsset_close(asset);
		return -1;
	}

	strcpy(parent, dst_name);
	slash = strrchr(parent, '/');
	if (slash)
	{
		*slash = '\0';
		if (fb_android_mkdir_p(parent) != 0)
		{
			AAsset_close(asset);
			return -1;
		}
	}

	out = fopen(dst_name, "wb");
	if (!out)
	{
		AAsset_close(asset);
		return -1;
	}

	for (;;)
	{
		int got = AAsset_read(asset, buffer, sizeof(buffer));
		if (got > 0)
		{
			if (fwrite(buffer, 1, (size_t)got, out) != (size_t)got)
			{
				ok = 0;
				break;
			}
			continue;
		}
		if (got < 0)
			ok = 0;
		break;
	}

	fclose(out);
	AAsset_close(asset);
	return ok ? 1 : -1;
}

static void fb_android_extract_asset_tree(AAssetManager *assets, const char *asset_dir, const char *dst_dir)
{
	AAssetDir *dir;
	const char *entry;

	dir = AAssetManager_openDir(assets, asset_dir);
	if (!dir)
		return;

	while ((entry = AAssetDir_getNextFileName(dir)) != NULL)
	{
		char child_asset[1024];
		char child_dst[1024];
		int extracted;

		if (fb_android_join_path(child_asset, sizeof(child_asset), asset_dir, entry) != 0)
			continue;
		if (fb_android_join_path(child_dst, sizeof(child_dst), dst_dir, entry) != 0)
			continue;

		extracted = fb_android_extract_asset_file(assets, child_asset, child_dst);
		if (extracted == 0)
		{
			if (fb_android_mkdir_p(child_dst) == 0)
				fb_android_extract_asset_tree(assets, child_asset, child_dst);
		}
	}

	AAssetDir_close(dir);
}

static void fb_android_extract_asset_manifest(AAssetManager *assets, const char *dst_dir)
{
	AAsset *manifest;
	char line[1024];
	size_t used = 0;
	char ch;

	manifest = AAssetManager_open(assets, "_freebasic_asset_manifest.txt", AASSET_MODE_STREAMING);
	if (!manifest)
	{
		fb_android_extract_asset_tree(assets, "", dst_dir);
		return;
	}

	while (AAsset_read(manifest, &ch, 1) == 1)
	{
		if (ch == '\r')
			continue;

		if (ch != '\n')
		{
			if (used + 1 < sizeof(line))
				line[used++] = ch;
			continue;
		}

		if (used > 0)
		{
			char dst_name[1024];
			line[used] = '\0';
			if (fb_android_join_path(dst_name, sizeof(dst_name), dst_dir, line) == 0)
				fb_android_extract_asset_file(assets, line, dst_name);
			used = 0;
		}
	}

	if (used > 0)
	{
		char dst_name[1024];
		line[used] = '\0';
		if (fb_android_join_path(dst_name, sizeof(dst_name), dst_dir, line) == 0)
			fb_android_extract_asset_file(assets, line, dst_name);
	}

	AAsset_close(manifest);
}

static void fb_android_prepare_files_dir(FB_ANDROID_APP *app)
{
	const char *files_dir;

	if (!app || !app->activity)
		return;

	files_dir = app->activity->internalDataPath;
	if (!files_dir || !*files_dir)
		return;

	if (fb_android_mkdir_p(files_dir) != 0)
	{
		fb_android_log("FreeBASIC Android could not create internal files directory");
		return;
	}

	if (chdir(files_dir) != 0)
	{
		fb_android_log("FreeBASIC Android could not enter internal files directory");
		return;
	}

	if (app->activity->assetManager)
		fb_android_extract_asset_manifest(app->activity->assetManager, files_dir);
}

static void fb_android_set_nonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags >= 0)
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void fb_android_redirect_stdio(FB_ANDROID_APP *app)
{
	if (pipe(app->stdout_pipe) != 0)
	{
		app->stdout_pipe[0] = -1;
		app->stdout_pipe[1] = -1;
		return;
	}

	fb_android_set_nonblock(app->stdout_pipe[0]);
	dup2(app->stdout_pipe[1], STDOUT_FILENO);
	dup2(app->stdout_pipe[1], STDERR_FILENO);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

static void *fb_android_program_thread(void *arg)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)arg;
	char *argv[] = { (char *)"freebasic-android", NULL };
	int rc;
	char message[96];
	jmp_buf exit_jump;
	int (*entry_fn)(int, char **) = NULL;

	(void)app;

	fb_android_log("FreeBASIC Android program starting");
	fb_android_prepare_files_dir(app);
	if (fb_AllocateMainFBThread)
		fb_AllocateMainFBThread();

	/*
		Android audio warmup

		Traditional BASIC sound commands are often used as tiny gameplay
		effects. If the first SOUND or BEEP has to create the Android audio
		stream, the program appears to freeze at exactly the moment the user
		presses a button or fires a shot.

		When sfxlib is linked, initialize it before entering the BASIC main
		program. This keeps audio device startup out of menu/gameplay input
		paths while preserving normal no-sfx builds through the weak symbol.
	*/
	if (fb_sfxEnsureInit)
	{
		fb_android_log("FreeBASIC Android sfx warmup starting");
		if (fb_sfxEnsureInit() == 0)
			fb_android_log("FreeBASIC Android sfx warmup complete");
		else
			fb_android_log("FreeBASIC Android sfx warmup failed");
	}

	fb_android_exit_jump = &exit_jump;
	fb_android_exit_status = 0;
	if (fb_android_program_main)
		entry_fn = fb_android_program_main;
	else if (FB_ANDROID_PROGRAM_MAIN)
		entry_fn = FB_ANDROID_PROGRAM_MAIN;

	if (setjmp(exit_jump) == 0)
		rc = entry_fn != NULL ? entry_fn(1, argv) : -1;
	else
		rc = fb_android_exit_status;
	fb_android_exit_jump = NULL;

	snprintf(message, sizeof(message), "FREEBASIC_ANDROID_EXIT:%d", rc);
	fb_android_log(message);
	return NULL;
}

void __wrap_exit(int status)
{
	if (fb_android_exit_jump)
	{
		fb_android_exit_status = status;
		longjmp(*fb_android_exit_jump, 1);
	}
	_exit(status);
}

static void fb_android_publish_lifecycle(int started, int resumed, int focused)
{
	if (fb_hAndroidGfxSetLifecycle)
		fb_hAndroidGfxSetLifecycle(started, resumed, focused);
	if (fb_hAndroidSfxSetLifecycle)
		fb_hAndroidSfxSetLifecycle(started, resumed);
}

static void fb_android_set_lifecycle(FB_ANDROID_APP *app, int started, int resumed, int focused)
{
	pthread_mutex_lock(&app->mutex);
	app->started = started;
	app->resumed = resumed;
	app->focused = focused;
	if (app->looper)
		ALooper_wake(app->looper);
	pthread_cond_broadcast(&app->cond);
	pthread_mutex_unlock(&app->mutex);

	fb_android_publish_lifecycle(started, resumed, focused);
}

static void fb_android_maybe_start_program(FB_ANDROID_APP *app)
{
	if (app->program_thread_started || !app->window || !app->stdio_ready)
		return;

	app->program_thread_started = 1;
	if (pthread_create(&app->program_thread, NULL, fb_android_program_thread, app) == 0)
		pthread_detach(app->program_thread);
	else
		app->program_thread_started = 0;
}

static int fb_android_handle_input(FB_ANDROID_APP *app)
{
	AInputEvent *event = NULL;

	while (app->input_queue && AInputQueue_getEvent(app->input_queue, &event) >= 0)
	{
		int handled = 0;
		int type;

		if (AInputQueue_preDispatchEvent(app->input_queue, event))
			continue;

		type = AInputEvent_getType(event);
		if (type == AINPUT_EVENT_TYPE_MOTION)
		{
			int32_t source = AInputEvent_getSource(event);

			if ((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK)
			{
				if (fb_hAndroidGamepadMotion)
					fb_hAndroidGamepadMotion(event);
			}
			else
			{
				if (fb_hAndroidTouchEvent)
					fb_hAndroidTouchEvent(event);
				else if (fb_hAndroidTouch)
				{
					int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
					float x = AMotionEvent_getX(event, 0);
					float y = AMotionEvent_getY(event, 0);
					fb_hAndroidTouch(x, y, action);
				}
			}
			handled = 1;
		}
		else if (type == AINPUT_EVENT_TYPE_KEY)
		{
			int32_t source = AInputEvent_getSource(event);

			if ((source & (AINPUT_SOURCE_GAMEPAD |
			               AINPUT_SOURCE_DPAD |
			               AINPUT_SOURCE_JOYSTICK)) != 0)
			{
				if (fb_hAndroidGamepadKey)
					fb_hAndroidGamepadKey(event);
			}
			else
			{
				int32_t keycode = AKeyEvent_getKeyCode(event);
				int action = AKeyEvent_getAction(event);
				if (fb_hAndroidKey)
					fb_hAndroidKey(keycode, action, 0);
			}
			handled = 1;
		}

		AInputQueue_finishEvent(app->input_queue, event, handled);
	}

	return 1;
}

static void fb_android_handle_stdout(FB_ANDROID_APP *app)
{
	char buffer[512];
	ssize_t got;

	for (;;)
	{
		got = read(app->stdout_pipe[0], buffer, sizeof(buffer));
		if (got > 0)
		{
			char logbuf[560];
			size_t copy = (size_t)got < sizeof(logbuf) - 1 ? (size_t)got : sizeof(logbuf) - 1;
			memcpy(logbuf, buffer, copy);
			logbuf[copy] = '\0';
			fb_android_log(logbuf);
			if (fb_android_console_enabled() && fb_hAndroidConsoleWrite)
				fb_hAndroidConsoleWrite(buffer, (size_t)got);
			continue;
		}

		if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;
		break;
	}
}

static void fb_android_attach_input_locked(FB_ANDROID_APP *app)
{
	if (app->looper && app->input_queue)
		AInputQueue_attachLooper(app->input_queue, app->looper, FB_ANDROID_LOOPER_INPUT, NULL, app);
}

static void *fb_android_looper_thread(void *arg)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)arg;

	app->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
	fb_android_redirect_stdio(app);

	if (app->stdout_pipe[0] >= 0)
		ALooper_addFd(app->looper, app->stdout_pipe[0], FB_ANDROID_LOOPER_STDOUT, ALOOPER_EVENT_INPUT, NULL, app);

	pthread_mutex_lock(&app->mutex);
	app->stdio_ready = 1;
	fb_android_attach_input_locked(app);
	while (!app->destroyed)
	{
		if (app->window)
			break;
		pthread_cond_wait(&app->cond, &app->mutex);
	}
	fb_android_maybe_start_program(app);
	pthread_mutex_unlock(&app->mutex);

	fb_android_render_console_if_enabled();

	while (!app->destroyed)
	{
		int events = 0;
		void *data = NULL;
		int ident = ALooper_pollOnce(50, NULL, &events, &data);

		(void)events;
		(void)data;

		if (ident == FB_ANDROID_LOOPER_INPUT)
			fb_android_handle_input(app);
		else if (ident == FB_ANDROID_LOOPER_STDOUT)
			fb_android_handle_stdout(app);
	}

	return NULL;
}

static void fb_android_on_start(ANativeActivity *activity)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (app)
		fb_android_set_lifecycle(app, 1, app->resumed, app->focused);
}

static void fb_android_on_resume(ANativeActivity *activity)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (app)
		fb_android_set_lifecycle(app, app->started, 1, app->focused);
}

static void fb_android_on_pause(ANativeActivity *activity)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (app)
		fb_android_set_lifecycle(app, app->started, 0, app->focused);
}

static void fb_android_on_stop(ANativeActivity *activity)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (app)
		fb_android_set_lifecycle(app, 0, 0, app->focused);
}

static void fb_android_on_window_focus_changed(ANativeActivity *activity, int has_focus)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (app)
		fb_android_set_lifecycle(app, app->started, app->resumed, has_focus ? 1 : 0);
}

static void fb_android_on_destroy(ANativeActivity *activity)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (!app)
		return;

	pthread_mutex_lock(&app->mutex);
	app->destroyed = 1;
	app->started = 0;
	app->resumed = 0;
	app->focused = 0;
	pthread_cond_broadcast(&app->cond);
	if (app->looper)
		ALooper_wake(app->looper);
	pthread_mutex_unlock(&app->mutex);

	fb_android_publish_lifecycle(0, 0, 0);

	if (app->looper_thread_started)
		pthread_join(app->looper_thread, NULL);

	if (app->stdout_pipe[0] >= 0)
		close(app->stdout_pipe[0]);
	if (app->stdout_pipe[1] >= 0)
		close(app->stdout_pipe[1]);

	if (fb_hAndroidSetWindow)
		fb_hAndroidSetWindow(NULL);
	if (fb_hAndroidSetActivity)
		fb_hAndroidSetActivity(NULL);
	pthread_cond_destroy(&app->cond);
	pthread_mutex_destroy(&app->mutex);
	free(app);
	activity->instance = NULL;
}

static void fb_android_on_native_window_created(ANativeActivity *activity, ANativeWindow *window)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	pthread_mutex_lock(&app->mutex);
	app->window = window;
	pthread_cond_broadcast(&app->cond);
	pthread_mutex_unlock(&app->mutex);

	if (fb_hAndroidSetWindow)
		fb_hAndroidSetWindow(window);

	pthread_mutex_lock(&app->mutex);
	fb_android_maybe_start_program(app);
	pthread_cond_broadcast(&app->cond);
	pthread_mutex_unlock(&app->mutex);

	fb_android_render_console_if_enabled();
	if (fb_hAndroidUpdate)
		fb_hAndroidUpdate();
}

static void fb_android_on_native_window_resized(ANativeActivity *activity, ANativeWindow *window)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	if (!app)
		return;

	pthread_mutex_lock(&app->mutex);
	app->window = window;
	pthread_mutex_unlock(&app->mutex);

	if (fb_hAndroidSetWindow)
		fb_hAndroidSetWindow(window);
	fb_android_render_console_if_enabled();
	if (fb_hAndroidUpdate)
		fb_hAndroidUpdate();
}

static void fb_android_on_native_window_redraw_needed(ANativeActivity *activity, ANativeWindow *window)
{
	(void)activity;
	(void)window;

	fb_android_render_console_if_enabled();
	if (fb_hAndroidUpdate)
		fb_hAndroidUpdate();
}

static void fb_android_on_native_window_destroyed(ANativeActivity *activity, ANativeWindow *window)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	(void)window;

	pthread_mutex_lock(&app->mutex);
	app->window = NULL;
	pthread_mutex_unlock(&app->mutex);

	if (fb_hAndroidSetWindow)
		fb_hAndroidSetWindow(NULL);
}

static void fb_android_on_input_queue_created(ANativeActivity *activity, AInputQueue *queue)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	pthread_mutex_lock(&app->mutex);
	app->input_queue = queue;
	fb_android_attach_input_locked(app);
	pthread_mutex_unlock(&app->mutex);
}

static void fb_android_on_input_queue_destroyed(ANativeActivity *activity, AInputQueue *queue)
{
	FB_ANDROID_APP *app = (FB_ANDROID_APP *)activity->instance;

	pthread_mutex_lock(&app->mutex);
	if (app->input_queue == queue)
	{
		if (app->looper)
			AInputQueue_detachLooper(queue);
		app->input_queue = NULL;
	}
	pthread_mutex_unlock(&app->mutex);
}

void ANativeActivity_onCreate(ANativeActivity *activity, void *saved_state, size_t saved_state_size)
{
	FB_ANDROID_APP *app;

	(void)saved_state;
	(void)saved_state_size;

	app = (FB_ANDROID_APP *)calloc(1, sizeof(*app));
	if (!app)
		return;

	app->activity = activity;
	app->started = 0;
	app->resumed = 0;
	app->focused = 0;
	app->stdout_pipe[0] = -1;
	app->stdout_pipe[1] = -1;
	pthread_mutex_init(&app->mutex, NULL);
	pthread_cond_init(&app->cond, NULL);

	activity->instance = app;
	activity->callbacks->onStart = fb_android_on_start;
	activity->callbacks->onResume = fb_android_on_resume;
	activity->callbacks->onPause = fb_android_on_pause;
	activity->callbacks->onStop = fb_android_on_stop;
	activity->callbacks->onDestroy = fb_android_on_destroy;
	activity->callbacks->onWindowFocusChanged = fb_android_on_window_focus_changed;
	activity->callbacks->onNativeWindowCreated = fb_android_on_native_window_created;
	activity->callbacks->onNativeWindowResized = fb_android_on_native_window_resized;
	activity->callbacks->onNativeWindowRedrawNeeded = fb_android_on_native_window_redraw_needed;
	activity->callbacks->onNativeWindowDestroyed = fb_android_on_native_window_destroyed;
	activity->callbacks->onInputQueueCreated = fb_android_on_input_queue_created;
	activity->callbacks->onInputQueueDestroyed = fb_android_on_input_queue_destroyed;

	if (fb_hAndroidSetKeyboardEnabled)
		fb_hAndroidSetKeyboardEnabled(FB_ANDROID_KEYBOARD_ENABLED);
	if (fb_hAndroidSetActivity)
		fb_hAndroidSetActivity(activity);
	fb_android_apply_app_mode();
	fb_android_publish_lifecycle(0, 0, 0);
	if (pthread_create(&app->looper_thread, NULL, fb_android_looper_thread, app) == 0)
		app->looper_thread_started = 1;
	else
		fb_android_log("FreeBASIC Android looper thread failed to start");
	fb_android_log("FreeBASIC Android activity created");
}
