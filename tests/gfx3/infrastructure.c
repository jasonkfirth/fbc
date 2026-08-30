/*
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: infrastructure.c

    Purpose:

		Exercise the gfxlib3 command queue, completion object, checked size
		helpers, generation-tagged resources, compatibility state, and renderer
		backends.

    Responsibilities:

		- provide small standalone mutex and condition implementations
		- provide standalone thread and bounded-delay services
		- verify blocking queue behavior and submission order
		- verify completion wakeups and deferred resource destruction
		- verify surfaces, drawing, pages, compatibility transforms, and readback
		- verify real OpenGL partial-context shutdown through a test platform

    This file intentionally does NOT contain:

		- Vulkan window-system presentation tests
		- exported FreeBASIC language ABI tests
		- full gfxlib2 pixel comparison tests
*/

#include "../../src/gfxlib3/gfx3_command.h"
#include "../../src/gfxlib3/gfx3_backend_gles.h"
#include "../../src/gfxlib3/gfx3_backend_null.h"
#include "../../src/gfxlib3/gfx3_backend_opengl.h"
#include "../../src/gfxlib3/gfx3_backend_select.h"
#include "../../src/gfxlib3/gfx3_backend_vulkan.h"
#include "../../src/gfxlib3/gfx3_compat.h"
#include "../../src/gfxlib3/gfx3_context.h"
#include "../../src/gfxlib3/gfx3_debug.h"
#include "../../src/gfxlib3/gfx3_input.h"
#include "../../src/gfxlib3/gfx3_platform.h"
#include "../../src/gfxlib3/gfx3_protocol.h"
#include "../../src/gfxlib3/gfx3_renderer.h"
#include "../../src/gfxlib3/gfx3_resource.h"
#include "../../src/gfxlib3/gfx3_vulkan.h"

#include <stdatomic.h>

#ifdef HOST_WIN32
	#include <windows.h>
#else
	#include <pthread.h>
	#include <sched.h>
	#include <unistd.h>
#endif

/* ------------------------------------------------------------------------- */
/* Platform override for backend lifecycle tests                             */
/* ------------------------------------------------------------------------- */

/*
	The production platform selector is a single stable seam between common
	backend code and the operating-system adapters. The standalone test supplies
	the same selector so it can substitute one deliberately failing platform for
	one renderer initialization, then return to the real adapter for normal
	OpenGL and Vulkan coverage below.

	The override is set before renderer creation and cleared only after the
	render thread has joined, so no test thread observes a concurrent change.
*/
static const FB_GFX3_PLATFORM_VTABLE *platform_test_override;

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_default(void)
{
	if (platform_test_override != NULL)
		return platform_test_override;
#if defined(HOST_ANDROID)
	return &__fb_gfx3_platform_android;
#elif defined(HOST_WIN32)
	return &__fb_gfx3_platform_win32;
#elif defined(HOST_LINUX) && !defined(DISABLE_X11)
	return &__fb_gfx3_platform_x11;
#else
	return NULL;
#endif
}

/* ------------------------------------------------------------------------- */
/* Standalone runtime synchronization                                        */
/* ------------------------------------------------------------------------- */

#ifdef HOST_WIN32

struct _FBMUTEX {
	CRITICAL_SECTION value;
};

struct _FBCOND {
	CONDITION_VARIABLE value;
};

FBCALL FBMUTEX *fb_MutexCreate(void)
{
	FBMUTEX *mutex = (FBMUTEX *)malloc(sizeof(*mutex));

	if (mutex != NULL)
		InitializeCriticalSection(&mutex->value);
	return mutex;
}

FBCALL void fb_MutexDestroy(FBMUTEX *mutex)
{
	if (mutex == NULL)
		return;
	DeleteCriticalSection(&mutex->value);
	free(mutex);
}

FBCALL void fb_MutexLock(FBMUTEX *mutex)
{
	EnterCriticalSection(&mutex->value);
}

FBCALL void fb_MutexUnlock(FBMUTEX *mutex)
{
	LeaveCriticalSection(&mutex->value);
}

FBCALL FBCOND *fb_CondCreate(void)
{
	FBCOND *condition = (FBCOND *)malloc(sizeof(*condition));

	if (condition != NULL)
		InitializeConditionVariable(&condition->value);
	return condition;
}

FBCALL void fb_CondDestroy(FBCOND *condition)
{
	free(condition);
}

FBCALL void fb_CondSignal(FBCOND *condition)
{
	WakeConditionVariable(&condition->value);
}

FBCALL void fb_CondBroadcast(FBCOND *condition)
{
	WakeAllConditionVariable(&condition->value);
}

FBCALL void fb_CondWait(FBCOND *condition, FBMUTEX *mutex)
{
	SleepConditionVariableCS(&condition->value, &mutex->value, INFINITE);
}

typedef HANDLE TEST_THREAD;
typedef DWORD (WINAPI *TEST_THREAD_PROC)(void *parameter);

static int test_thread_create(TEST_THREAD *thread, TEST_THREAD_PROC proc,
	void *parameter)
{
	*thread = CreateThread(NULL, 0, proc, parameter, 0, NULL);
	return (*thread != NULL) ? 0 : -1;
}

static void test_thread_join(TEST_THREAD thread)
{
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
}

static void test_thread_yield(void)
{
	SwitchToThread();
}

static void test_delay(void)
{
	Sleep(10);
}

#define TEST_THREAD_RESULT DWORD WINAPI
#define TEST_THREAD_RETURN return 0

#else

struct _FBMUTEX {
	pthread_mutex_t value;
};

struct _FBCOND {
	pthread_cond_t value;
};

FBCALL FBMUTEX *fb_MutexCreate(void)
{
	FBMUTEX *mutex = (FBMUTEX *)malloc(sizeof(*mutex));

	if ((mutex != NULL) && (pthread_mutex_init(&mutex->value, NULL) != 0)) {
		free(mutex);
		mutex = NULL;
	}
	return mutex;
}

FBCALL void fb_MutexDestroy(FBMUTEX *mutex)
{
	if (mutex == NULL)
		return;
	pthread_mutex_destroy(&mutex->value);
	free(mutex);
}

FBCALL void fb_MutexLock(FBMUTEX *mutex)
{
	pthread_mutex_lock(&mutex->value);
}

FBCALL void fb_MutexUnlock(FBMUTEX *mutex)
{
	pthread_mutex_unlock(&mutex->value);
}

FBCALL FBCOND *fb_CondCreate(void)
{
	FBCOND *condition = (FBCOND *)malloc(sizeof(*condition));

	if ((condition != NULL) &&
	    (pthread_cond_init(&condition->value, NULL) != 0)) {
		free(condition);
		condition = NULL;
	}
	return condition;
}

FBCALL void fb_CondDestroy(FBCOND *condition)
{
	if (condition == NULL)
		return;
	pthread_cond_destroy(&condition->value);
	free(condition);
}

FBCALL void fb_CondSignal(FBCOND *condition)
{
	pthread_cond_signal(&condition->value);
}

FBCALL void fb_CondBroadcast(FBCOND *condition)
{
	pthread_cond_broadcast(&condition->value);
}

FBCALL void fb_CondWait(FBCOND *condition, FBMUTEX *mutex)
{
	pthread_cond_wait(&condition->value, &mutex->value);
}

typedef pthread_t TEST_THREAD;
typedef void *(*TEST_THREAD_PROC)(void *parameter);

static int test_thread_create(TEST_THREAD *thread, TEST_THREAD_PROC proc,
	void *parameter)
{
	return pthread_create(thread, NULL, proc, parameter);
}

static void test_thread_join(TEST_THREAD thread)
{
	pthread_join(thread, NULL);
}

static void test_thread_yield(void)
{
	sched_yield();
}

static void test_delay(void)
{
	usleep(10000);
}

#define TEST_THREAD_RESULT void *
#define TEST_THREAD_RETURN return NULL

#endif

struct _FBTHREAD {
	TEST_THREAD thread;
	FB_THREADPROC proc;
	void *parameter;
};

static TEST_THREAD_RESULT runtime_thread_start(void *parameter)
{
	FBTHREAD *thread = (FBTHREAD *)parameter;

	thread->proc(thread->parameter);
	TEST_THREAD_RETURN;
}

FBCALL FBTHREAD *fb_ThreadCreate(FB_THREADPROC proc, void *parameter,
	ssize_t stack_size)
{
	FBTHREAD *thread;

	(void)stack_size;
	if (proc == NULL)
		return NULL;

	thread = (FBTHREAD *)calloc(1, sizeof(*thread));
	if (thread == NULL)
		return NULL;
	thread->proc = proc;
	thread->parameter = parameter;
	if (test_thread_create(&thread->thread, runtime_thread_start, thread) != 0) {
		free(thread);
		return NULL;
	}
	return thread;
}

FBCALL void fb_ThreadWait(FBTHREAD *thread)
{
	if (thread == NULL)
		return;
	test_thread_join(thread->thread);
	free(thread);
}

FBCALL void fb_Delay(int milliseconds)
{
	if (milliseconds <= 0)
		return;
#ifdef HOST_WIN32
	Sleep((DWORD)milliseconds);
#else
	while (milliseconds > 0) {
		int interval = (milliseconds > 1000) ? 1000 : milliseconds;

		usleep((unsigned int)interval * 1000u);
		milliseconds -= interval;
	}
#endif
}

#ifdef HOST_WIN32
/*
    The standalone infrastructure executable has no console runtime. Native
    key messages are outside this test; these two adapters only satisfy the
    platform module's normal rtlib boundary while OpenGL work is exercised.
*/
int fb_hVirtualToScancode(int virtual_key)
{
	(void)virtual_key;
	return 0;
}

int fb_hConsoleTranslateKey(char ascii, WORD virtual_scan, WORD virtual_key,
	DWORD control_state, int enhanced_only)
{
	(void)virtual_scan;
	(void)virtual_key;
	(void)control_state;
	if (enhanced_only || (ascii == 0))
		return -1;
	return (unsigned char)ascii;
}
#endif

/* ------------------------------------------------------------------------- */
/* Test support                                                              */
/* ------------------------------------------------------------------------- */

static int failures;
static atomic_int destroyed_resources;
static atomic_int failing_backend_started;
static atomic_int failing_backend_continue;
static atomic_int startup_failure_stage;
static atomic_int startup_failure_probe_calls;
static atomic_int startup_failure_init_calls;
static atomic_int startup_failure_shutdown_calls;
static atomic_int startup_failure_shutdown_saw_state;
static atomic_int batch_backend_ready;
static atomic_int batch_backend_continue;
static atomic_int batch_backend_execute_calls;
static atomic_size_t batch_backend_first_command_count;
static atomic_uint_fast64_t batch_backend_last_wait_sequence;
static int logged_messages;
static char last_log_message[FB_GFX3_LOG_MESSAGE_SIZE];

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "%s:%d: check failed: %s\n", \
				__FILE__, __LINE__, #condition); \
			failures++; \
		} \
	} while (0)

typedef struct QUEUE_THREAD_DATA {
	FB_GFX3_COMMAND_QUEUE *queue;
	FB_GFX3_COMMAND *command;
	FB_GFX3_COMPLETION *completion;
	atomic_int started;
	atomic_int done;
	int result;
	uint64_t sequence;
} QUEUE_THREAD_DATA;

typedef struct RENDERER_INIT_THREAD_DATA {
	FB_GFX3_RENDERER *renderer;
	const FB_GFX3_RENDERER_CONFIG *config;
	atomic_int started;
	int result;
} RENDERER_INIT_THREAD_DATA;

static TEST_THREAD_RESULT submit_thread(void *parameter)
{
	QUEUE_THREAD_DATA *data = (QUEUE_THREAD_DATA *)parameter;

	atomic_store(&data->started, TRUE);
	data->result = fb_gfx3_queue_submit(data->queue, data->command,
		&data->sequence);
	atomic_store(&data->done, TRUE);
	TEST_THREAD_RETURN;
}

static TEST_THREAD_RESULT completion_thread(void *parameter)
{
	QUEUE_THREAD_DATA *data = (QUEUE_THREAD_DATA *)parameter;

	atomic_store(&data->started, TRUE);
	data->result = fb_gfx3_completion_wait(data->completion,
		&data->sequence);
	atomic_store(&data->done, TRUE);
	TEST_THREAD_RETURN;
}

static TEST_THREAD_RESULT pop_thread(void *parameter)
{
	QUEUE_THREAD_DATA *data = (QUEUE_THREAD_DATA *)parameter;

	atomic_store(&data->started, TRUE);
	data->result = fb_gfx3_queue_pop(data->queue, &data->command);
	atomic_store(&data->done, TRUE);
	TEST_THREAD_RETURN;
}

static TEST_THREAD_RESULT renderer_init_thread(void *parameter)
{
	RENDERER_INIT_THREAD_DATA *data =
		(RENDERER_INIT_THREAD_DATA *)parameter;

	atomic_store(&data->started, TRUE);
	data->result = fb_gfx3_renderer_init(data->renderer, data->config);
	TEST_THREAD_RETURN;
}

static void wait_until_started(QUEUE_THREAD_DATA *data)
{
	while (!atomic_load(&data->started))
		test_thread_yield();
}

static void destroy_test_resource(void *resource)
{
	free(resource);
	atomic_fetch_add(&destroyed_resources, 1);
}

static void capture_log_message(int level, const char *message,
	void *user_data)
{
	(void)level;
	(void)user_data;
	logged_messages++;
	strncpy(last_log_message, message, sizeof(last_log_message) - 1);
	last_log_message[sizeof(last_log_message) - 1] = '\0';
}

static int failing_backend_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	memset(caps, 0, sizeof(*caps));
	caps->abi_version = FB_GFX3_BACKEND_ABI_VERSION;
	caps->max_surface_width = 1024;
	caps->max_surface_height = 1024;
	caps->max_batch_commands = 1;
	return FB_GFX3_OK;
}

static int failing_backend_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	(void)config;
	backend->state = backend;
	return FB_GFX3_OK;
}

static void failing_backend_shutdown(FB_GFX3_BACKEND *backend)
{
	backend->state = NULL;
}

static int batch_backend_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	memset(caps, 0, sizeof(*caps));
	caps->abi_version = FB_GFX3_BACKEND_ABI_VERSION;
	caps->max_surface_width = 1024;
	caps->max_surface_height = 1024;
	caps->max_batch_commands = 64;
	return FB_GFX3_OK;
}

static int batch_backend_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	(void)config;
	backend->state = backend;
	atomic_store(&batch_backend_ready, TRUE);
	while (!atomic_load(&batch_backend_continue))
		test_thread_yield();
	return FB_GFX3_OK;
}

static void batch_backend_shutdown(FB_GFX3_BACKEND *backend)
{
	backend->state = NULL;
}

static int batch_backend_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	size_t expected = 0;

	(void)backend;
	if ((commands == NULL) || (count == 0) ||
	    (submitted_sequence == NULL))
		return FB_GFX3_INVALID;
	atomic_compare_exchange_strong(&batch_backend_first_command_count,
		&expected, count);
	atomic_fetch_add(&batch_backend_execute_calls, 1);
	*submitted_sequence = commands[count - 1]->sequence;
	return FB_GFX3_OK;
}

static uint64_t batch_backend_completed(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return UINT64_MAX;
}

static int batch_backend_wait_sequence(FB_GFX3_BACKEND *backend,
	uint64_t sequence)
{
	(void)backend;
	atomic_store(&batch_backend_last_wait_sequence, sequence);
	return FB_GFX3_OK;
}

static int batch_backend_wait_idle(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return FB_GFX3_OK;
}

static const FB_GFX3_BACKEND_VTABLE batch_backend = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"Batch test backend",
	batch_backend_probe,
	batch_backend_init,
	batch_backend_shutdown,
	batch_backend_execute,
	batch_backend_completed,
	batch_backend_wait_sequence,
	batch_backend_wait_idle,
	NULL
};

static int failing_backend_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	(void)backend;
	(void)commands;
	(void)count;
	(void)submitted_sequence;
	atomic_store(&failing_backend_started, TRUE);
	while (!atomic_load(&failing_backend_continue))
		test_thread_yield();
	return -777;
}

static uint64_t failing_backend_completed(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return 0;
}

static int failing_backend_wait_sequence(FB_GFX3_BACKEND *backend,
	uint64_t sequence)
{
	(void)backend;
	(void)sequence;
	return -777;
}

static int failing_backend_wait_idle(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return FB_GFX3_OK;
}

static const FB_GFX3_BACKEND_VTABLE failing_backend = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"Failing test backend",
	failing_backend_probe,
	failing_backend_init,
	failing_backend_shutdown,
	failing_backend_execute,
	failing_backend_completed,
	failing_backend_wait_sequence,
	failing_backend_wait_idle,
	NULL
};

/* ------------------------------------------------------------------------- */
/* Startup-failure backend                                                   */
/* ------------------------------------------------------------------------- */

/*
	Real graphics initialization can fail either before a platform object exists
	or after a backend has allocated its first object.  Keep both cases in one
	controlled backend so the common render-thread cleanup contract is tested
	without depending on a particular driver's failure behavior.
*/
enum {
	STARTUP_FAILURE_PROBE = 1,
	STARTUP_FAILURE_INIT = 2
};

static int startup_failure_backend_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	atomic_fetch_add(&startup_failure_probe_calls, 1);
	if (atomic_load(&startup_failure_stage) == STARTUP_FAILURE_PROBE)
		return -611;
	if (caps == NULL)
		return FB_GFX3_INVALID;
	memset(caps, 0, sizeof(*caps));
	caps->abi_version = FB_GFX3_BACKEND_ABI_VERSION;
	caps->max_surface_width = 1024;
	caps->max_surface_height = 1024;
	caps->max_batch_commands = 1;
	return FB_GFX3_OK;
}

static int startup_failure_backend_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	(void)config;
	atomic_fetch_add(&startup_failure_init_calls, 1);
	if (backend == NULL)
		return FB_GFX3_INVALID;
	if (atomic_load(&startup_failure_stage) == STARTUP_FAILURE_INIT) {
		/* Model a backend which must clean up an already-owned object. */
		backend->state = backend;
		return -612;
	}
	return FB_GFX3_OK;
}

static void startup_failure_backend_shutdown(FB_GFX3_BACKEND *backend)
{
	atomic_fetch_add(&startup_failure_shutdown_calls, 1);
	if ((backend != NULL) && (backend->state != NULL)) {
		atomic_store(&startup_failure_shutdown_saw_state, TRUE);
		backend->state = NULL;
	}
}

static int startup_failure_backend_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	(void)backend;
	(void)commands;
	(void)count;
	(void)submitted_sequence;
	return FB_GFX3_FAILED;
}

static uint64_t startup_failure_backend_completed(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return 0;
}

static int startup_failure_backend_wait(FB_GFX3_BACKEND *backend,
	uint64_t sequence)
{
	(void)backend;
	(void)sequence;
	return FB_GFX3_FAILED;
}

static int startup_failure_backend_wait_idle(FB_GFX3_BACKEND *backend)
{
	(void)backend;
	return FB_GFX3_FAILED;
}

static const FB_GFX3_BACKEND_VTABLE startup_failure_backend = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"Startup failure test backend",
	startup_failure_backend_probe,
	startup_failure_backend_init,
	startup_failure_backend_shutdown,
	startup_failure_backend_execute,
	startup_failure_backend_completed,
	startup_failure_backend_wait,
	startup_failure_backend_wait_idle,
	NULL
};

/* ------------------------------------------------------------------------- */
/* OpenGL partial-platform failure                                           */
/* ------------------------------------------------------------------------- */

/*
	This adapter models a platform that has successfully allocated a live OpenGL
	context before one required entry point cannot be resolved. It deliberately
	returns a distinct owned allocation from create_opengl(). The real OpenGL
	backend must retain that allocation until its common render-thread failure
	path invokes the backend shutdown callback exactly once.
*/
static atomic_int partial_opengl_create_calls;
static atomic_int partial_opengl_load_calls;
static atomic_int partial_opengl_destroy_calls;

static int partial_opengl_probe(void)
{
	return FB_GFX3_OK;
}

static int partial_opengl_create(void **platform,
	const FB_GFX3_PLATFORM_OPENGL_CONFIG *config)
{
	int *owned_context;

	if ((platform == NULL) || (config == NULL) || (config->width == 0) ||
	    (config->height == 0))
		return FB_GFX3_INVALID;
	owned_context = (int *)malloc(sizeof(*owned_context));
	if (owned_context == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	*owned_context = 0x47334F50; /* "G3OP": owned only by partial_opengl_destroy. */
	*platform = owned_context;
	atomic_fetch_add(&partial_opengl_create_calls, 1);
	return FB_GFX3_OK;
}

static void partial_opengl_destroy(void *platform)
{
	int *owned_context = (int *)platform;

	if (owned_context == NULL)
		return;
	CHECK(*owned_context == 0x47334F50);
	*owned_context = 0;
	free(owned_context);
	atomic_fetch_add(&partial_opengl_destroy_calls, 1);
}

static int partial_opengl_load_function(void *platform, const char *name,
	void *destination, size_t destination_size)
{
	(void)destination;
	(void)destination_size;
	if ((platform == NULL) || (name == NULL) || (name[0] == '\0'))
		return FB_GFX3_INVALID;
	atomic_fetch_add(&partial_opengl_load_calls, 1);
	return FB_GFX3_UNSUPPORTED;
}

static const FB_GFX3_PLATFORM_VTABLE partial_opengl_platform = {
	"Partial OpenGL test platform",
	partial_opengl_probe,
	NULL,
	partial_opengl_create,
	NULL,
	partial_opengl_destroy,
	partial_opengl_load_function,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/* ------------------------------------------------------------------------- */
/* Vulkan native-window handoff failure                                      */
/* ------------------------------------------------------------------------- */

static atomic_int partial_vulkan_create_calls;
static atomic_int partial_vulkan_handles_calls;
static atomic_int partial_vulkan_destroy_calls;

static int partial_vulkan_create(void **platform,
	const FB_GFX3_PLATFORM_WINDOW_CONFIG *config)
{
	int *owned_window;

	if ((platform == NULL) || (config == NULL) || (config->width == 0) ||
	    (config->height == 0))
		return FB_GFX3_INVALID;
	owned_window = (int *)malloc(sizeof(*owned_window));
	if (owned_window == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	*owned_window = 0x4733564B; /* "G3VK": released only by the destroy hook. */
	*platform = owned_window;
	atomic_fetch_add(&partial_vulkan_create_calls, 1);
	return FB_GFX3_OK;
}

static int partial_vulkan_native_handles(void *platform, uintptr_t *instance,
	uintptr_t *window)
{
	if ((platform == NULL) || (instance == NULL) || (window == NULL))
		return FB_GFX3_INVALID;
	atomic_fetch_add(&partial_vulkan_handles_calls, 1);
	return FB_GFX3_UNSUPPORTED;
}

static void partial_vulkan_destroy(void *platform)
{
	int *owned_window = (int *)platform;

	if (owned_window == NULL)
		return;
	CHECK(*owned_window == 0x4733564B);
	*owned_window = 0;
	free(owned_window);
	atomic_fetch_add(&partial_vulkan_destroy_calls, 1);
}

static const FB_GFX3_PLATFORM_VTABLE partial_vulkan_platform = {
	"Partial Vulkan test platform",
	NULL,
	partial_vulkan_create,
	NULL,
	partial_vulkan_native_handles,
	partial_vulkan_destroy,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

static void test_checked_sizes_and_commands(void)
{
	FB_GFX3_COMMAND *command;
	size_t result = 0;

	CHECK(fb_gfx3_size_add(10, 20, &result) == FB_GFX3_OK);
	CHECK(result == 30);
	CHECK(fb_gfx3_size_add(SIZE_MAX, 1, &result) == FB_GFX3_INVALID);
	CHECK(fb_gfx3_size_multiply(10, 20, &result) == FB_GFX3_OK);
	CHECK(result == 200);
	CHECK(fb_gfx3_size_multiply(SIZE_MAX, 2, &result) == FB_GFX3_INVALID);

	CHECK(fb_gfx3_command_create(FB_GFX3_COMMAND_INVALID, 0) == NULL);
	CHECK(fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR,
		FB_GFX3_COMMAND_MAX_SIZE) == NULL);

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR, 37);
	CHECK(command != NULL);
	if (command != NULL) {
		CHECK(command->type == FB_GFX3_COMMAND_CLEAR);
		CHECK(command->sequence == 0);
		CHECK(fb_gfx3_command_payload_size(command) == 37);
		fb_gfx3_command_destroy(command);
	}
}

static void test_queue_order_and_back_pressure(void)
{
	FB_GFX3_COMMAND_QUEUE queue;
	FB_GFX3_COMMAND *first;
	FB_GFX3_COMMAND *second;
	FB_GFX3_COMMAND *popped = NULL;
	QUEUE_THREAD_DATA data;
	TEST_THREAD thread;
	uint64_t sequence = 0;

	CHECK(fb_gfx3_queue_init(&queue, 1) == FB_GFX3_OK);
	/* The render thread uses this non-blocking form only after it has work. */
	CHECK(fb_gfx3_queue_try_pop(&queue, &popped) == FB_GFX3_EXHAUSTED);
	CHECK(popped == NULL);
	first = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR, 0);
	second = fb_gfx3_command_create(FB_GFX3_COMMAND_PRESENT, 0);
	CHECK((first != NULL) && (second != NULL));
	if ((first == NULL) || (second == NULL)) {
		fb_gfx3_command_destroy(first);
		fb_gfx3_command_destroy(second);
		fb_gfx3_queue_destroy(&queue);
		return;
	}

	CHECK(fb_gfx3_queue_submit(&queue, first, &sequence) == FB_GFX3_OK);
	CHECK(sequence == 1);

	memset(&data, 0, sizeof(data));
	data.queue = &queue;
	data.command = second;
	atomic_init(&data.started, FALSE);
	atomic_init(&data.done, FALSE);
	CHECK(test_thread_create(&thread, submit_thread, &data) == 0);
	wait_until_started(&data);
	test_delay();
	CHECK(!atomic_load(&data.done));

	CHECK(fb_gfx3_queue_pop(&queue, &popped) == FB_GFX3_OK);
	CHECK(popped == first);
	fb_gfx3_command_destroy(popped);

	test_thread_join(thread);
	CHECK(data.result == FB_GFX3_OK);
	CHECK(data.sequence == 2);
	CHECK(atomic_load(&data.done));

	fb_gfx3_queue_close(&queue);
	CHECK(fb_gfx3_queue_pop(&queue, &popped) == FB_GFX3_OK);
	CHECK(popped == second);
	fb_gfx3_command_destroy(popped);
	CHECK(fb_gfx3_queue_pop(&queue, &popped) == FB_GFX3_CLOSED);
	CHECK(popped == NULL);
	CHECK(fb_gfx3_queue_try_pop(&queue, &popped) == FB_GFX3_CLOSED);
	CHECK(popped == NULL);
	fb_gfx3_queue_destroy(&queue);
}

static void test_queue_close_and_failure_wakeups(void)
{
	FB_GFX3_COMMAND_QUEUE queue;
	FB_GFX3_COMMAND *first;
	FB_GFX3_COMMAND *second;
	QUEUE_THREAD_DATA data;
	TEST_THREAD thread;

	CHECK(fb_gfx3_queue_init(&queue, 1) == FB_GFX3_OK);
	first = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR, 0);
	second = fb_gfx3_command_create(FB_GFX3_COMMAND_PRESENT, 0);
	CHECK((first != NULL) && (second != NULL));
	if ((first == NULL) || (second == NULL)) {
		fb_gfx3_command_destroy(first);
		fb_gfx3_command_destroy(second);
		fb_gfx3_queue_destroy(&queue);
		return;
	}

	CHECK(fb_gfx3_queue_submit(&queue, first, NULL) == FB_GFX3_OK);
	memset(&data, 0, sizeof(data));
	data.queue = &queue;
	data.command = second;
	atomic_init(&data.started, FALSE);
	atomic_init(&data.done, FALSE);
	CHECK(test_thread_create(&thread, submit_thread, &data) == 0);
	wait_until_started(&data);
	test_delay();
	CHECK(!atomic_load(&data.done));
	fb_gfx3_queue_close(&queue);
	test_thread_join(thread);
	CHECK(data.result == FB_GFX3_CLOSED);
	CHECK(data.command == second);
	fb_gfx3_command_destroy(second);
	fb_gfx3_queue_destroy(&queue);

	CHECK(fb_gfx3_queue_init(&queue, 1) == FB_GFX3_OK);
	memset(&data, 0, sizeof(data));
	data.queue = &queue;
	atomic_init(&data.started, FALSE);
	atomic_init(&data.done, FALSE);
	CHECK(test_thread_create(&thread, pop_thread, &data) == 0);
	wait_until_started(&data);
	test_delay();
	CHECK(!atomic_load(&data.done));
	fb_gfx3_queue_fail(&queue, -321);
	test_thread_join(thread);
	CHECK(data.result == FB_GFX3_FAILED);
	CHECK(data.command == NULL);
	CHECK(queue.failure_code == -321);
	fb_gfx3_queue_destroy(&queue);
}

static void test_completion(void)
{
	FB_GFX3_COMPLETION completion;
	QUEUE_THREAD_DATA data;
	TEST_THREAD thread;
	uint64_t value = 0;

	CHECK(fb_gfx3_completion_init(&completion) == FB_GFX3_OK);
	memset(&data, 0, sizeof(data));
	data.completion = &completion;
	atomic_init(&data.started, FALSE);
	atomic_init(&data.done, FALSE);
	CHECK(test_thread_create(&thread, completion_thread, &data) == 0);
	wait_until_started(&data);
	test_delay();
	CHECK(!atomic_load(&data.done));

	CHECK(fb_gfx3_completion_set_value(&completion, 0,
		UINT64_C(0x123456789ABCDEF0)) == FB_GFX3_OK);
	CHECK(fb_gfx3_completion_finish(&completion, 42, -123) == FB_GFX3_OK);
	test_thread_join(thread);
	CHECK(data.result == -123);
	CHECK(data.sequence == 42);
	CHECK(fb_gfx3_completion_get_value(&completion, 0, &value) == FB_GFX3_OK);
	CHECK(value == UINT64_C(0x123456789ABCDEF0));
	CHECK(fb_gfx3_completion_set_value(&completion, 0, 1) == FB_GFX3_INVALID);
	CHECK(fb_gfx3_completion_finish(&completion, 43, 0) == FB_GFX3_INVALID);
	fb_gfx3_completion_destroy(&completion);
}

static void test_resource_registry(void)
{
	FB_GFX3_RESOURCE_REGISTRY registry;
	FB_GFX3_HANDLE first_handle;
	FB_GFX3_HANDLE second_handle;
	void *first;
	void *second;
	void *retained = NULL;

	atomic_store(&destroyed_resources, 0);
	CHECK(fb_gfx3_resources_init(&registry, 1) == FB_GFX3_OK);
	first = malloc(4);
	second = malloc(4);
	CHECK((first != NULL) && (second != NULL));
	if ((first == NULL) || (second == NULL)) {
		free(first);
		free(second);
		fb_gfx3_resources_destroy(&registry);
		return;
	}

	first_handle = fb_gfx3_resource_register(&registry, 10, first,
		destroy_test_resource);
	second_handle = fb_gfx3_resource_register(&registry, 20, second,
		destroy_test_resource);
	CHECK(first_handle != 0);
	CHECK(second_handle != 0);
	CHECK(first_handle != second_handle);
	CHECK(registry.capacity >= 2);

	CHECK(fb_gfx3_resource_retain(&registry, first_handle, 10,
		&retained) == FB_GFX3_OK);
	CHECK(retained == first);
	CHECK(fb_gfx3_resource_retain(&registry, first_handle, 20,
		&retained) == FB_GFX3_INVALID);
	CHECK(retained == NULL);
	CHECK(fb_gfx3_resource_mark_used(&registry, first_handle, 5) ==
		FB_GFX3_OK);
	CHECK(fb_gfx3_resource_release(&registry, first_handle) == FB_GFX3_OK);
	CHECK(fb_gfx3_resource_release(&registry, first_handle) == FB_GFX3_OK);
	CHECK(fb_gfx3_resources_collect(&registry, 4) == 0);
	CHECK(fb_gfx3_resources_collect(&registry, 5) == 1);
	CHECK(atomic_load(&destroyed_resources) == 1);
	CHECK(fb_gfx3_resource_retain(&registry, first_handle,
		FB_GFX3_RESOURCE_ANY, &retained) == FB_GFX3_INVALID);

	CHECK(fb_gfx3_resource_release(&registry, second_handle) == FB_GFX3_OK);
	CHECK(fb_gfx3_resources_collect(&registry, 0) == 1);
	CHECK(atomic_load(&destroyed_resources) == 2);
	fb_gfx3_resources_destroy(&registry);
}

static int submit_and_wait(FB_GFX3_RENDERER *renderer,
	FB_GFX3_COMMAND *command, uint64_t *sequence, uint64_t *value)
{
	FB_GFX3_COMPLETION completion;
	int result;

	if (command == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	result = fb_gfx3_completion_init(&completion);
	if (result != FB_GFX3_OK) {
		fb_gfx3_command_destroy(command);
		return result;
	}

	/*
	 * Submission and completion waiting are one synchronous operation here.
	 * The renderer signals this object before the wait returns, so the worker
	 * cannot retain the stack address after this function exits.
	 */
	/* cppcheck-suppress autoVariables */
	command->completion = &completion;
	result = fb_gfx3_renderer_submit(renderer, command, sequence);
	if (result != FB_GFX3_OK) {
		fb_gfx3_command_destroy(command);
		fb_gfx3_completion_destroy(&completion);
		return result;
	}

	result = fb_gfx3_completion_wait(&completion, NULL);
	if ((result == FB_GFX3_OK) && (value != NULL))
		result = fb_gfx3_completion_get_value(&completion, 0, value);
	fb_gfx3_completion_destroy(&completion);
	return result;
}

static void test_renderer_command_batch(void)
{
	FB_GFX3_RENDERER renderer;
	FB_GFX3_RENDERER_CONFIG config;
	RENDERER_INIT_THREAD_DATA init_data;
	FB_GFX3_COMMAND *command;
	FB_GFX3_COMPLETION destroy_completion;
	TEST_THREAD thread;
	uint64_t destroy_sequence = 0;
	int destroy_completion_ready;

	memset(&renderer, 0, sizeof(renderer));
	memset(&config, 0, sizeof(config));
	memset(&init_data, 0, sizeof(init_data));
	config.backend = &batch_backend;
	config.queue_capacity = 8;
	config.resource_capacity = 1;
	init_data.renderer = &renderer;
	init_data.config = &config;
	atomic_init(&init_data.started, FALSE);
	atomic_store(&batch_backend_ready, FALSE);
	atomic_store(&batch_backend_continue, FALSE);
	atomic_store(&batch_backend_execute_calls, 0);
	atomic_store(&batch_backend_first_command_count, 0);
	atomic_store(&batch_backend_last_wait_sequence, 0);

	if (test_thread_create(&thread, renderer_init_thread, &init_data) != 0) {
		CHECK(FALSE);
		return;
	}
	while (!atomic_load(&init_data.started))
		test_thread_yield();
	while (!atomic_load(&batch_backend_ready))
		test_thread_yield();

	/*
		Hold startup so the destroy completion and later work are available in
		one batch. The test backend records the exact completion sequence it
		was asked to retire.
	*/
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR, 0);
	CHECK(command != NULL);
	if ((command != NULL) &&
	    (fb_gfx3_renderer_submit(&renderer, command, NULL) != FB_GFX3_OK)) {
		CHECK(FALSE);
		fb_gfx3_command_destroy(command);
	}
	destroy_completion_ready =
		(fb_gfx3_completion_init(&destroy_completion) == FB_GFX3_OK);
	CHECK(destroy_completion_ready);
	if (destroy_completion_ready) {
		command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_DESTROY,
			0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->completion = &destroy_completion;
			if (fb_gfx3_renderer_submit(&renderer, command,
			    &destroy_sequence) != FB_GFX3_OK) {
				CHECK(FALSE);
				fb_gfx3_command_destroy(command);
				destroy_sequence = 0;
			}
		}
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR, 0);
	CHECK(command != NULL);
	if ((command != NULL) &&
	    (fb_gfx3_renderer_submit(&renderer, command, NULL) != FB_GFX3_OK)) {
		CHECK(FALSE);
		fb_gfx3_command_destroy(command);
	}

	atomic_store(&batch_backend_continue, TRUE);
	test_thread_join(thread);
	CHECK(init_data.result == FB_GFX3_OK);
	if (init_data.result != FB_GFX3_OK) {
		if (destroy_completion_ready)
			fb_gfx3_completion_destroy(&destroy_completion);
		return;
	}

	if (destroy_completion_ready && (destroy_sequence != 0)) {
		CHECK(fb_gfx3_completion_wait(&destroy_completion, NULL) ==
			FB_GFX3_OK);
		CHECK(atomic_load(&batch_backend_last_wait_sequence) ==
			destroy_sequence);
	}
	if (destroy_completion_ready)
		fb_gfx3_completion_destroy(&destroy_completion);

	/* Ordinary completion requests remain an independent batch boundary. */
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_BARRIER, 0);
	CHECK(submit_and_wait(&renderer, command, NULL, NULL) == FB_GFX3_OK);
	CHECK(atomic_load(&batch_backend_first_command_count) == 3);
	CHECK(atomic_load(&batch_backend_execute_calls) >= 2);
	CHECK(fb_gfx3_renderer_shutdown(&renderer) == FB_GFX3_OK);
}

static FB_GFX3_HANDLE create_test_surface(FB_GFX3_RENDERER *renderer,
	uint64_t *sequence, uint32_t depth, uint32_t clear_color)
{
	FB_GFX3_SURFACE_CREATE_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	uint64_t value = 0;
	int result;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_CREATE,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return 0;
	payload = (FB_GFX3_SURFACE_CREATE_COMMAND *)command->payload;
	payload->width = 8;
	payload->height = 8;
	payload->depth = depth;
	payload->usage = FB_GFX3_SURFACE_RENDER_TARGET |
		FB_GFX3_SURFACE_TRANSFER_SOURCE;
	payload->clear_color = clear_color;

	result = submit_and_wait(renderer, command, sequence, &value);
	CHECK(result == FB_GFX3_OK);
	CHECK(value != 0);
	return (result == FB_GFX3_OK) ? (FB_GFX3_HANDLE)value : 0;
}

static uint32_t read_test_pixel(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int x, int y)
{
	FB_GFX3_READ_PIXEL_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	uint64_t value = UINT64_MAX;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_READ_PIXEL,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return UINT32_MAX;
	command->target = surface;
	payload = (FB_GFX3_READ_PIXEL_COMMAND *)command->payload;
	payload->x = x;
	payload->y = y;
	CHECK(submit_and_wait(renderer, command, NULL, &value) == FB_GFX3_OK);
	return (uint32_t)value;
}

static void clear_test_surface(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, uint32_t color)
{
	FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_CLEAR_COMMAND *)command->payload;
	payload->clip.x1 = 0;
	payload->clip.y1 = 0;
	payload->clip.x2 = 7;
	payload->clip.y2 = 7;
	payload->color = color;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void set_test_pixel(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int x, int y, uint32_t color)
{
	FB_GFX3_POINTS_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t payload_size = offsetof(FB_GFX3_POINTS_COMMAND, point) +
		sizeof(FB_GFX3_POINT);

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_POINTS, payload_size);
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_POINTS_COMMAND *)command->payload;
	payload->clip.x1 = 0;
	payload->clip.y1 = 0;
	payload->clip.x2 = 7;
	payload->clip.y2 = 7;
	payload->count = 1;
	payload->point[0].x = x;
	payload->point[0].y = y;
	payload->point[0].color = color;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void draw_test_rectangle(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style, int filled)
{
	FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_RECTANGLE,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_RECTANGLE_COMMAND *)command->payload;
	payload->clip.x1 = 0;
	payload->clip.y1 = 0;
	payload->clip.x2 = 7;
	payload->clip.y2 = 7;
	payload->x1 = x1;
	payload->y1 = y1;
	payload->x2 = x2;
	payload->y2 = y2;
	payload->color = color;
	payload->style = style;
	payload->filled = filled;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void draw_test_ellipse(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int center_x, int center_y,
	float radius_x, float radius_y, uint32_t color, int filled)
{
	FB_GFX3_ELLIPSE_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_ELLIPSE,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_ELLIPSE_COMMAND *)command->payload;
	payload->clip.x1 = 0;
	payload->clip.y1 = 0;
	payload->clip.x2 = 7;
	payload->clip.y2 = 7;
	payload->center_x = center_x;
	payload->center_y = center_y;
	payload->radius_x = radius_x;
	payload->radius_y = radius_y;
	payload->color = color;
	payload->filled = filled;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void draw_test_line(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int x1, int y1, int x2, int y2,
	uint32_t color, uint32_t style)
{
	FB_GFX3_LINE_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_LINE,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_LINE_COMMAND *)command->payload;
	payload->clip.x1 = 0;
	payload->clip.y1 = 0;
	payload->clip.x2 = 7;
	payload->clip.y2 = 7;
	payload->x1 = x1;
	payload->y1 = y1;
	payload->x2 = x2;
	payload->y2 = y2;
	payload->color = color;
	payload->style = style;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void blit_test_surface(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE destination, FB_GFX3_HANDLE source,
	int source_x1, int source_y1, int source_x2, int source_y2,
	int destination_x, int destination_y, uint32_t mode, uint32_t alpha)
{
	FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_COMMAND *command;

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_BLIT,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = destination;
	payload = (FB_GFX3_BLIT_COMMAND *)command->payload;
	payload->source = source;
	payload->clip.x1 = 0;
	payload->clip.y1 = 0;
	payload->clip.x2 = 7;
	payload->clip.y2 = 7;
	payload->source_rect.x1 = source_x1;
	payload->source_rect.y1 = source_y1;
	payload->source_rect.x2 = source_x2;
	payload->source_rect.y2 = source_y2;
	payload->destination_x = destination_x;
	payload->destination_y = destination_y;
	payload->mode = mode;
	payload->alpha = alpha;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void upload_test_surface(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int x, int y, uint32_t width, uint32_t height,
	uint32_t pitch, const void *data)
{
	FB_GFX3_SURFACE_UPLOAD_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t data_size;
	size_t payload_size;

	CHECK(fb_gfx3_size_multiply(pitch, height, &data_size) == FB_GFX3_OK);
	CHECK(fb_gfx3_size_add(offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data),
		data_size, &payload_size) == FB_GFX3_OK);
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_UPLOAD,
		payload_size);
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
	payload->destination_x = x;
	payload->destination_y = y;
	payload->width = width;
	payload->height = height;
	payload->source_pitch = pitch;
	payload->data_size = (uint32_t)data_size;
	memcpy(payload->data, data, data_size);
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void download_test_surface(FB_GFX3_RENDERER *renderer,
	FB_GFX3_HANDLE surface, int x, int y, uint32_t width, uint32_t height,
	uint32_t pitch, void *destination)
{
	FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload;
	FB_GFX3_COMMAND *command;
	size_t destination_size;

	CHECK(fb_gfx3_size_multiply(pitch, height, &destination_size) ==
		FB_GFX3_OK);
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_DOWNLOAD,
		sizeof(*payload));
	CHECK(command != NULL);
	if (command == NULL)
		return;
	command->target = surface;
	payload = (FB_GFX3_SURFACE_DOWNLOAD_COMMAND *)command->payload;
	payload->source_x = x;
	payload->source_y = y;
	payload->width = width;
	payload->height = height;
	payload->destination_pitch = pitch;
	payload->destination_size = (uint32_t)destination_size;
	payload->destination_address = (uint64_t)(uintptr_t)destination;
	CHECK(submit_and_wait(renderer, command, NULL, NULL) == FB_GFX3_OK);
}

static void test_renderer_lifecycle(void)
{
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER renderer;
	FB_GFX3_CLEAR_COMMAND *clear_payload;
	FB_GFX3_POINTS_COMMAND *points_payload;
	FB_GFX3_LINE_COMMAND *line_payload;
	FB_GFX3_COMMAND *command;
	FB_GFX3_HANDLE surface;
	FB_GFX3_HANDLE source_surface;
	FB_GFX3_HANDLE depth_surface;
	uint64_t sequence = 0;
	size_t payload_size;
	static const uint32_t depths[] = { 1, 2, 4, 8, 16 };
	static const uint32_t masks[] = { 1, 3, 15, 255, 65535 };
	uint32_t upload_pixels[6] = {
		0x10111213u, 0x20212223u, 0xEEEEEEEEu,
		0x30313233u, 0x40414243u, 0xEEEEEEEEu
	};
	uint32_t download_pixels[6];
	int i;
	int result;

	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_null;
	config.backend_config.width = 64;
	config.backend_config.height = 64;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 2;
	config.queue_capacity = 4;
	CHECK(fb_gfx3_renderer_init(&renderer, &config) == FB_GFX3_OK);

	surface = create_test_surface(&renderer, &sequence, 32, 0x11223344u);
	CHECK(surface != 0);
	CHECK(sequence == 1);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x11223344u);
	CHECK(read_test_pixel(&renderer, surface, -1, 0) == UINT32_MAX);

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR,
		sizeof(*clear_payload));
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = surface;
		clear_payload = (FB_GFX3_CLEAR_COMMAND *)command->payload;
		clear_payload->clip.x1 = 1;
		clear_payload->clip.y1 = 1;
		clear_payload->clip.x2 = 6;
		clear_payload->clip.y2 = 6;
		clear_payload->color = 0x01020304u;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x11223344u);
	CHECK(read_test_pixel(&renderer, surface, 1, 1) == 0x01020304u);

	payload_size = offsetof(FB_GFX3_POINTS_COMMAND, point) +
		(3 * sizeof(FB_GFX3_POINT));
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_POINTS, payload_size);
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = surface;
		points_payload = (FB_GFX3_POINTS_COMMAND *)command->payload;
		points_payload->clip.x1 = 0;
		points_payload->clip.y1 = 0;
		points_payload->clip.x2 = 7;
		points_payload->clip.y2 = 7;
		points_payload->count = 3;
		points_payload->point[0].x = 0;
		points_payload->point[0].y = 0;
		points_payload->point[0].color = 0xA0A0A0A0u;
		points_payload->point[1].x = 7;
		points_payload->point[1].y = 7;
		points_payload->point[1].color = 0xB0B0B0B0u;
		points_payload->point[2].x = 8;
		points_payload->point[2].y = 8;
		points_payload->point[2].color = 0xC0C0C0C0u;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xA0A0A0A0u);
	CHECK(read_test_pixel(&renderer, surface, 7, 7) == 0xB0B0B0B0u);
	memset(download_pixels, 0xCC, sizeof(download_pixels));
	upload_test_surface(&renderer, surface, 2, 4, 2, 2, 12,
		upload_pixels);
	download_test_surface(&renderer, surface, 2, 4, 2, 2, 12,
		download_pixels);
	CHECK(download_pixels[0] == upload_pixels[0]);
	CHECK(download_pixels[1] == upload_pixels[1]);
	CHECK(download_pixels[2] == 0xCCCCCCCCu);
	CHECK(download_pixels[3] == upload_pixels[3]);
	CHECK(download_pixels[4] == upload_pixels[4]);
	CHECK(download_pixels[5] == 0xCCCCCCCCu);

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_LINE,
		sizeof(*line_payload));
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = surface;
		line_payload = (FB_GFX3_LINE_COMMAND *)command->payload;
		line_payload->clip.x1 = 0;
		line_payload->clip.y1 = 0;
		line_payload->clip.x2 = 7;
		line_payload->clip.y2 = 7;
		line_payload->x1 = 0;
		line_payload->y1 = 7;
		line_payload->x2 = 7;
		line_payload->y2 = 0;
		line_payload->color = 0xDEADBEEFu;
		line_payload->style = 0xFFFFu;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	for (i = 0; i < 8; i++)
		CHECK(read_test_pixel(&renderer, surface, i, 7 - i) ==
			0xDEADBEEFu);

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_LINE,
		sizeof(*line_payload));
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = surface;
		line_payload = (FB_GFX3_LINE_COMMAND *)command->payload;
		line_payload->clip.x1 = 0;
		line_payload->clip.y1 = 0;
		line_payload->clip.x2 = 7;
		line_payload->clip.y2 = 7;
		line_payload->x1 = 0;
		line_payload->y1 = 3;
		line_payload->x2 = 7;
		line_payload->y2 = 3;
		line_payload->color = 0x55667788u;
		line_payload->style = 0xAAAAu;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	for (i = 0; i < 8; i++) {
		if ((i & 1) == 0)
			CHECK(read_test_pixel(&renderer, surface, i, 3) ==
				0x55667788u);
	}

	clear_test_surface(&renderer, surface, 0);
	draw_test_rectangle(&renderer, surface, -2, -2, 2, 2,
		0x89ABCDEFu, 0xFFFFu, TRUE);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x89ABCDEFu);
	CHECK(read_test_pixel(&renderer, surface, 2, 2) == 0x89ABCDEFu);
	CHECK(read_test_pixel(&renderer, surface, 3, 3) == 0);

	clear_test_surface(&renderer, surface, 0);
	draw_test_rectangle(&renderer, surface, 1, 1, 6, 6,
		0x76543210u, 0xAAAAu, FALSE);
	CHECK(read_test_pixel(&renderer, surface, 1, 6) == 0x76543210u);
	CHECK(read_test_pixel(&renderer, surface, 2, 6) == 0);
	CHECK(read_test_pixel(&renderer, surface, 1, 1) == 0x76543210u);
	CHECK(read_test_pixel(&renderer, surface, 6, 6) == 0);

	source_surface = create_test_surface(&renderer, NULL, 32, 0x00FF00FFu);
	CHECK(source_surface != 0);
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_POINTS,
		offsetof(FB_GFX3_POINTS_COMMAND, point) + sizeof(FB_GFX3_POINT));
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = source_surface;
		points_payload = (FB_GFX3_POINTS_COMMAND *)command->payload;
		points_payload->clip.x1 = 0;
		points_payload->clip.y1 = 0;
		points_payload->clip.x2 = 7;
		points_payload->clip.y2 = 7;
		points_payload->count = 1;
		points_payload->point[0].x = 0;
		points_payload->point[0].y = 0;
		points_payload->point[0].color = 0x12345678u;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	clear_test_surface(&renderer, surface, 0xAABBCCDDu);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 1, 0,
		3, 3, FB_GFX3_BLIT_TRANS, 255);
	CHECK(read_test_pixel(&renderer, surface, 3, 3) == 0x00345678u);
	CHECK(read_test_pixel(&renderer, surface, 4, 3) == 0xAABBCCDDu);

	clear_test_surface(&renderer, surface, 0);
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_POINTS,
		offsetof(FB_GFX3_POINTS_COMMAND, point) +
			(4 * sizeof(FB_GFX3_POINT)));
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = surface;
		points_payload = (FB_GFX3_POINTS_COMMAND *)command->payload;
		points_payload->clip.x1 = 0;
		points_payload->clip.y1 = 0;
		points_payload->clip.x2 = 7;
		points_payload->clip.y2 = 7;
		points_payload->count = 4;
		for (i = 0; i < 4; i++) {
			points_payload->point[i].x = i;
			points_payload->point[i].y = 0;
			points_payload->point[i].color = (uint32_t)(i + 1);
		}
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	blit_test_surface(&renderer, surface, surface, 0, 0, 3, 0,
		1, 0, FB_GFX3_BLIT_PSET, 255);
	for (i = 0; i < 4; i++)
		CHECK(read_test_pixel(&renderer, surface, i + 1, 0) ==
			(uint32_t)(i + 1));

	set_test_pixel(&renderer, source_surface, 0, 0, 0x00F00F00u);
	clear_test_surface(&renderer, surface, 0x000FF000u);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_PRESET, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF0FF0FFu);
	clear_test_surface(&renderer, surface, 0x000FF000u);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_AND, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0);
	clear_test_surface(&renderer, surface, 0x000FF000u);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_OR, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x00FFFF00u);
	clear_test_surface(&renderer, surface, 0x000FF000u);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_XOR, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x00FFFF00u);

	set_test_pixel(&renderer, source_surface, 0, 0, 0xFF112233u);
	clear_test_surface(&renderer, surface, 0x88776655u);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_ALPHA, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF112233u);
	clear_test_surface(&renderer, surface, 0x88776655u);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_BLEND, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF112233u);
	clear_test_surface(&renderer, surface, 0xFFFFFFFFu);
	blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
		0, 0, FB_GFX3_BLIT_ADD, 255);
	CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFFFFFFFFu);

	for (i = 0; i < (int)(sizeof(depths) / sizeof(depths[0])); i++) {
		depth_surface = create_test_surface(&renderer, NULL, depths[i],
			UINT32_MAX);
		CHECK(depth_surface != 0);
		if (depth_surface == 0)
			continue;
		CHECK(read_test_pixel(&renderer, depth_surface, 0, 0) == masks[i]);
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = depth_surface;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}

	for (i = 0; i < 32; i++) {
		command = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR,
			sizeof(*clear_payload));
		CHECK(command != NULL);
		if (command == NULL)
			break;
		command->target = surface;
		clear_payload = (FB_GFX3_CLEAR_COMMAND *)command->payload;
		clear_payload->clip.x1 = 0;
		clear_payload->clip.y1 = 0;
		clear_payload->clip.x2 = 7;
		clear_payload->clip.y2 = 7;
		clear_payload->color = (uint32_t)i;
		result = fb_gfx3_renderer_submit(&renderer, command, &sequence);
		CHECK(result == FB_GFX3_OK);
		if (result != FB_GFX3_OK) {
			fb_gfx3_command_destroy(command);
			break;
		}
	}

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = surface;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	command = fb_gfx3_command_create(FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
	CHECK(command != NULL);
	if (command != NULL) {
		command->target = source_surface;
		CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
			FB_GFX3_OK);
	}
	CHECK(fb_gfx3_renderer_shutdown(&renderer) == FB_GFX3_OK);

	config.backend_config.depth = 3;
	CHECK(fb_gfx3_renderer_init(&renderer, &config) == FB_GFX3_INVALID);
	CHECK(renderer.thread == NULL);
	CHECK(renderer.queue.slots == NULL);
}

static void test_renderer_failure_wakes_waiters(void)
{
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER renderer;
	FB_GFX3_COMPLETION first_completion;
	FB_GFX3_COMPLETION second_completion;
	FB_GFX3_COMMAND *first;
	FB_GFX3_COMMAND *second;

	atomic_store(&failing_backend_started, FALSE);
	atomic_store(&failing_backend_continue, FALSE);
	memset(&config, 0, sizeof(config));
	config.backend = &failing_backend;
	config.backend_config.width = 32;
	config.backend_config.height = 32;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 1;
	config.queue_capacity = 4;
	CHECK(fb_gfx3_renderer_init(&renderer, &config) == FB_GFX3_OK);
	CHECK(fb_gfx3_completion_init(&first_completion) == FB_GFX3_OK);
	CHECK(fb_gfx3_completion_init(&second_completion) == FB_GFX3_OK);

	first = fb_gfx3_command_create(FB_GFX3_COMMAND_CLEAR, 0);
	second = fb_gfx3_command_create(FB_GFX3_COMMAND_PRESENT, 0);
	CHECK((first != NULL) && (second != NULL));
	if ((first == NULL) || (second == NULL)) {
		fb_gfx3_command_destroy(first);
		fb_gfx3_command_destroy(second);
		atomic_store(&failing_backend_continue, TRUE);
		fb_gfx3_renderer_shutdown(&renderer);
		fb_gfx3_completion_destroy(&second_completion);
		fb_gfx3_completion_destroy(&first_completion);
		return;
	}

	first->completion = &first_completion;
	second->completion = &second_completion;
	CHECK(fb_gfx3_renderer_submit(&renderer, first, NULL) == FB_GFX3_OK);
	while (!atomic_load(&failing_backend_started))
		test_thread_yield();
	CHECK(fb_gfx3_renderer_submit(&renderer, second, NULL) == FB_GFX3_OK);
	atomic_store(&failing_backend_continue, TRUE);

	CHECK(fb_gfx3_completion_wait(&first_completion, NULL) == -777);
	CHECK(fb_gfx3_completion_wait(&second_completion, NULL) == -777);
	CHECK(fb_gfx3_renderer_shutdown(&renderer) == -777);
	fb_gfx3_completion_destroy(&second_completion);
	fb_gfx3_completion_destroy(&first_completion);
}

static void test_renderer_startup_failure_cleanup(void)
{
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER renderer;
	int result;

	memset(&config, 0, sizeof(config));
	config.backend = &startup_failure_backend;
	config.backend_config.width = 32;
	config.backend_config.height = 32;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 1;
	config.queue_capacity = 4;

	/* Probe rejection has no backend-owned state, but shutdown is still safe. */
	atomic_store(&startup_failure_stage, STARTUP_FAILURE_PROBE);
	atomic_store(&startup_failure_probe_calls, 0);
	atomic_store(&startup_failure_init_calls, 0);
	atomic_store(&startup_failure_shutdown_calls, 0);
	atomic_store(&startup_failure_shutdown_saw_state, FALSE);
	result = fb_gfx3_renderer_init(&renderer, &config);
	CHECK(result == -611);
	CHECK(atomic_load(&startup_failure_probe_calls) == 1);
	CHECK(atomic_load(&startup_failure_init_calls) == 0);
	CHECK(atomic_load(&startup_failure_shutdown_calls) == 1);
	CHECK(!atomic_load(&startup_failure_shutdown_saw_state));
	CHECK(renderer.thread == NULL);
	CHECK(renderer.queue.slots == NULL);
	CHECK(renderer.resources.slots == NULL);

	/* Init rejection must release the object it had already claimed. */
	atomic_store(&startup_failure_stage, STARTUP_FAILURE_INIT);
	atomic_store(&startup_failure_probe_calls, 0);
	atomic_store(&startup_failure_init_calls, 0);
	atomic_store(&startup_failure_shutdown_calls, 0);
	atomic_store(&startup_failure_shutdown_saw_state, FALSE);
	result = fb_gfx3_renderer_init(&renderer, &config);
	CHECK(result == -612);
	CHECK(atomic_load(&startup_failure_probe_calls) == 1);
	CHECK(atomic_load(&startup_failure_init_calls) == 1);
	CHECK(atomic_load(&startup_failure_shutdown_calls) == 1);
	CHECK(atomic_load(&startup_failure_shutdown_saw_state));
	CHECK(renderer.thread == NULL);
	CHECK(renderer.queue.slots == NULL);
	CHECK(renderer.resources.slots == NULL);
}

static void test_opengl_partial_context_failure_cleanup(void)
{
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER renderer;
	int result;

	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_opengl;
	config.backend_config.width = 32;
	config.backend_config.height = 32;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 1;
	config.queue_capacity = 4;
	atomic_store(&partial_opengl_create_calls, 0);
	atomic_store(&partial_opengl_load_calls, 0);
	atomic_store(&partial_opengl_destroy_calls, 0);

	platform_test_override = &partial_opengl_platform;
	result = fb_gfx3_renderer_init(&renderer, &config);
	platform_test_override = NULL;

	CHECK(result == FB_GFX3_UNSUPPORTED);
	CHECK(atomic_load(&partial_opengl_create_calls) == 1);
	CHECK(atomic_load(&partial_opengl_load_calls) == 1);
	CHECK(atomic_load(&partial_opengl_destroy_calls) == 1);
	CHECK(renderer.thread == NULL);
	CHECK(renderer.queue.slots == NULL);
	CHECK(renderer.resources.slots == NULL);
	printf("gfxlib3 OpenGL partial context cleanup passed\n");
}

static void test_vulkan_native_window_failure_cleanup(void)
{
	FB_GFX3_BACKEND_CAPS caps;
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER renderer;
	int result;

	memset(&caps, 0, sizeof(caps));
	if (__fb_gfx3_backend_vulkan.probe(&caps) != FB_GFX3_OK) {
		printf("gfxlib3 Vulkan native window cleanup skipped\n");
		return;
	}
	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_vulkan;
	config.backend_config.width = 32;
	config.backend_config.height = 32;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 1;
	config.queue_capacity = 4;
	atomic_store(&partial_vulkan_create_calls, 0);
	atomic_store(&partial_vulkan_handles_calls, 0);
	atomic_store(&partial_vulkan_destroy_calls, 0);

	platform_test_override = &partial_vulkan_platform;
	result = fb_gfx3_renderer_init(&renderer, &config);
	platform_test_override = NULL;

	CHECK(result == FB_GFX3_UNSUPPORTED);
	CHECK(atomic_load(&partial_vulkan_create_calls) == 1);
	CHECK(atomic_load(&partial_vulkan_handles_calls) == 1);
	CHECK(atomic_load(&partial_vulkan_destroy_calls) == 1);
	CHECK(renderer.thread == NULL);
	CHECK(renderer.queue.slots == NULL);
	CHECK(renderer.resources.slots == NULL);
	printf("gfxlib3 Vulkan native window cleanup passed\n");
}

static void test_typed_context_api(void)
{
	FB_GFX3_CONTEXT_CONFIG config;
	FB_GFX3_CONTEXT context;
	FB_GFX3_SURFACE source;
	FB_GFX3_SURFACE destination;
	FB_GFX3_RECT clip = { 0, 0, 7, 7 };
	FB_GFX3_RECT source_rect = { 0, 0, 3, 3 };
	FB_GFX3_POINT point;
	uint32_t upload_pixels[6] = {
		0x11111111u, 0x22222222u, 0xEEEEEEEEu,
		0x33333333u, 0x44444444u, 0xEEEEEEEEu
	};
	uint32_t download_pixels[6];
	uint32_t color = 0;

	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_null;
	config.width = 8;
	config.height = 8;
	config.depth = 32;
	config.page_count = 1;
	config.queue_capacity = 4;
	config.log_level = FB_GFX3_LOG_WARNING;
	CHECK(fb_gfx3_context_init(&context, &config) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_create(&context, &source, 8, 8, 32,
		FB_GFX3_SURFACE_RENDER_TARGET | FB_GFX3_SURFACE_SAMPLED |
		FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_create(&context, &destination, 8, 8, 32,
		FB_GFX3_SURFACE_RENDER_TARGET | FB_GFX3_SURFACE_SAMPLED |
		FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION, 0) == FB_GFX3_OK);

	CHECK(fb_gfx3_surface_clear(&source, &clip, 0x01020304u, 0) ==
		FB_GFX3_OK);
	point.x = 2;
	point.y = 3;
	point.color = 0xAABBCCDDu;
	point.flags = 0;
	CHECK(fb_gfx3_surface_points(&source, &clip, &point, 1) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_line(&source, &clip, 0, 7, 7, 0,
		0xDEADBEEFu, 0xFFFFu, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_rectangle(&source, &clip, 1, 1, 6, 6,
		0x55667788u, 0xAAAAu, FALSE, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_ellipse(&source, &clip, 4, 4, 2.0f, 1.0f,
		0x13579BDFu, FALSE, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&source, 6, 4, &color) == FB_GFX3_OK);
	CHECK(color == 0x13579BDFu);
	CHECK(fb_gfx3_surface_read_pixel(&source, 2, 3, &color) == FB_GFX3_OK);
	CHECK(color == 0xAABBCCDDu);

	CHECK(fb_gfx3_surface_blit(&destination, &clip, &source, &source_rect,
		2, 2, FB_GFX3_BLIT_PSET, 255) == FB_GFX3_OK);
	CHECK(fb_gfx3_context_flush(&context) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&destination, 4, 5, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0xAABBCCDDu);

	CHECK(fb_gfx3_surface_upload(&destination, 1, 4, 2, 2, 12,
		upload_pixels) == FB_GFX3_OK);
	memset(download_pixels, 0xCC, sizeof(download_pixels));
	CHECK(fb_gfx3_surface_download(&destination, 1, 4, 2, 2, 12,
		download_pixels) == FB_GFX3_OK);
	CHECK(download_pixels[0] == upload_pixels[0]);
	CHECK(download_pixels[1] == upload_pixels[1]);
	CHECK(download_pixels[2] == 0xCCCCCCCCu);
	CHECK(download_pixels[3] == upload_pixels[3]);
	CHECK(download_pixels[4] == upload_pixels[4]);
	CHECK(download_pixels[5] == 0xCCCCCCCCu);

	CHECK(fb_gfx3_surface_destroy(&destination) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_destroy(&source) == FB_GFX3_OK);
	CHECK(fb_gfx3_context_shutdown(&context) == FB_GFX3_OK);
}

/* ------------------------------------------------------------------------- */
/* Exact alpha primitive command semantics                                   */
/* ------------------------------------------------------------------------- */

static void test_alpha_primitive_null_backend(void)
{
	FB_GFX3_CONTEXT_CONFIG config;
	FB_GFX3_CONTEXT context;
	FB_GFX3_SURFACE surface;
	FB_GFX3_RECT clip = { 0, 0, 7, 7 };
	FB_GFX3_POINT point;
	const uint32_t destination = 0xFF204060u;
	const uint32_t source = 0x8030C080u;
	const uint32_t transparent_source = 0x0030C080u;
	const uint32_t expected = fb_gfx3_alpha_primitive_pixel(source,
		destination);
	uint32_t color = 0;

	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_null;
	config.width = 8;
	config.height = 8;
	config.depth = 32;
	config.page_count = 1;
	config.queue_capacity = 8;
	config.log_level = FB_GFX3_LOG_WARNING;
	CHECK(fb_gfx3_context_init(&context, &config) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_create(&context, &surface, 8, 8, 32,
		FB_GFX3_SURFACE_RENDER_TARGET | FB_GFX3_SURFACE_SAMPLED |
		FB_GFX3_SURFACE_TRANSFER_SOURCE |
		FB_GFX3_SURFACE_TRANSFER_DESTINATION, destination) == FB_GFX3_OK);

	point.x = 1;
	point.y = 1;
	point.color = source;
	point.flags = FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	CHECK(fb_gfx3_surface_points(&surface, &clip, &point, 1) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_line(&surface, &clip, 2, 2, 5, 2, source,
		0xFFFFu, FB_GFX3_PRIMITIVE_ALPHA_BLEND) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_rectangle(&surface, &clip, 2, 4, 5, 6, source,
		0xFFFFu, TRUE, FB_GFX3_PRIMITIVE_ALPHA_BLEND) == FB_GFX3_OK);
	CHECK(fb_gfx3_context_flush(&context) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&surface, 1, 1, &color) == FB_GFX3_OK);
	CHECK(color == expected);
	CHECK(fb_gfx3_surface_read_pixel(&surface, 4, 2, &color) == FB_GFX3_OK);
	CHECK(color == expected);
	CHECK(fb_gfx3_surface_read_pixel(&surface, 4, 5, &color) == FB_GFX3_OK);
	CHECK(color == expected);

	point.x = 7;
	point.y = 7;
	point.color = transparent_source;
	point.flags = FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	CHECK(fb_gfx3_surface_points(&surface, &clip, &point, 1) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&surface, 7, 7, &color) == FB_GFX3_OK);
	CHECK(color == (destination & 0x00FFFFFFu));

	CHECK(fb_gfx3_surface_destroy(&surface) == FB_GFX3_OK);
	CHECK(fb_gfx3_context_shutdown(&context) == FB_GFX3_OK);
}

static void test_compatibility_state(void)
{
	FB_GFX3_CONTEXT_CONFIG config;
	FB_GFX3_MODE mode;
	FB_GFX3_DRAW_STATE state;
	FB_GFX3_DRAW_STATE second_state;
	uint32_t color = 0;
	float coordinate = 0.0f;
	int previous_pages = -1;

	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_null;
	config.width = 8;
	config.height = 8;
	config.depth = 32;
	config.page_count = 3;
	config.queue_capacity = 4;
	config.log_level = FB_GFX3_LOG_WARNING;
	CHECK(fb_gfx3_mode_init(&mode, &config, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_draw_state_init(&mode, &state) == FB_GFX3_OK);
	CHECK(fb_gfx3_draw_state_init(&mode, &second_state) == FB_GFX3_OK);

	CHECK(fb_gfx3_compat_pset(&state, 1.0f, 2.0f, 0x12345678u,
		FB_GFX3_COORDINATE_AA, FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 1.0f, 2.0f, &color) == FB_GFX3_OK);
	CHECK(color == 0x12345678u);
	CHECK(fb_gfx3_compat_pset(&state, 2.0f, 1.0f, 0x87654321u,
		FB_GFX3_COORDINATE_R, FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 3.0f, 3.0f, &color) == FB_GFX3_OK);
	CHECK(color == 0x87654321u);
	CHECK(fb_gfx3_compat_cursor(&state, 2, &coordinate) == FB_GFX3_OK);
	CHECK(coordinate == 3.0f);

	CHECK(fb_gfx3_compat_line(&state, 0.0f, 7.0f, 7.0f, 0.0f,
		0xABCDEF01u, FB_GFX3_LINE_TYPE_LINE, 0xFFFFu,
		FB_GFX3_COORDINATE_AA) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 4.0f, 3.0f, &color) == FB_GFX3_OK);
	CHECK(color == 0xABCDEF01u);
	CHECK(fb_gfx3_compat_ellipse(&state, 4.0f, 4.0f, 2.0f,
		0x0BADBEEFu, 0.5f, FALSE, FB_GFX3_COORDINATE_A) ==
		FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 6.0f, 4.0f, &color) == FB_GFX3_OK);
	CHECK(color == 0x0BADBEEFu);
	CHECK(fb_gfx3_compat_arc(&state, 4.0f, 4.0f, 2.0f,
		0x10203040u, 1.0f, 0.0f, 1.5707963f,
		FB_GFX3_COORDINATE_A) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 6.0f, 4.0f, &color) == FB_GFX3_OK);
	CHECK(color == 0x10203040u);
	CHECK(fb_gfx3_compat_arc(&state, 4.0f, 4.0f, 2.0f,
		0x40302010u, 1.0f, -0.5f, 1.0f,
		FB_GFX3_COORDINATE_A) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 4.0f, 4.0f, &color) == FB_GFX3_OK);
	CHECK(color == 0x40302010u);

	CHECK(fb_gfx3_compat_view(&state, 2, 2, 6, 6, 0x11223344u,
		0x55667788u, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&mode.pages[0], 1, 2, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0x55667788u);
	CHECK(fb_gfx3_surface_read_pixel(&mode.pages[0], 3, 3, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0x11223344u);
	CHECK(fb_gfx3_compat_pset(&state, 0.0f, 0.0f, 0xCAFEBABEu,
		FB_GFX3_COORDINATE_AA, FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&mode.pages[0], 2, 2, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0xCAFEBABEu);
	CHECK(fb_gfx3_compat_view(&state, INT16_MIN, INT16_MIN, INT16_MIN,
		INT16_MIN, 0, 0, 0) == FB_GFX3_OK);

	CHECK(fb_gfx3_compat_window(&state, 0.0f, 0.0f, 7.0f, 7.0f,
		TRUE) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_pset(&state, 7.0f, 0.0f, 0x01020304u,
		FB_GFX3_COORDINATE_A, FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&mode.pages[0], 7, 0, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0x01020304u);
	CHECK(fb_gfx3_compat_window(&state, 0.0f, 0.0f, 7.0f, 7.0f,
		FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_pset(&state, 0.0f, 0.0f, 0x05060708u,
		FB_GFX3_COORDINATE_A, FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&mode.pages[0], 0, 7, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0x05060708u);
	CHECK(fb_gfx3_compat_pmap(&state, 3.5f, 0, &coordinate) == FB_GFX3_OK);
	CHECK(coordinate == 4.0f);
	CHECK(fb_gfx3_compat_window(&state, 0.0f, 0.0f, 0.0f, 0.0f,
		FALSE) == FB_GFX3_OK);

	CHECK(fb_gfx3_page_set(&state, 1, 2, &previous_pages) == FB_GFX3_OK);
	CHECK(previous_pages == 0);
	CHECK(state.work_page == 1);
	CHECK(mode.visible_page == 2);
	CHECK(second_state.work_page == 0);
	CHECK(fb_gfx3_compat_pset(&state, 2.0f, 5.0f, 0xA1B2C3D4u,
		FB_GFX3_COORDINATE_AA, FALSE) == FB_GFX3_OK);
	CHECK(fb_gfx3_page_copy(&state, 1, 2) == FB_GFX3_OK);
	CHECK(fb_gfx3_surface_read_pixel(&mode.pages[2], 2, 5, &color) ==
		FB_GFX3_OK);
	CHECK(color == 0xA1B2C3D4u);
	CHECK(fb_gfx3_page_set(&state, -1, -1, &previous_pages) == FB_GFX3_OK);
	CHECK(previous_pages == (1 | (2 << 8)));
	CHECK(state.work_page == 0);
	CHECK(mode.visible_page == 0);

	CHECK(fb_gfx3_mode_shutdown(&mode) == FB_GFX3_OK);
	CHECK(fb_gfx3_compat_point(&state, 0.0f, 0.0f, &color) ==
		FB_GFX3_INVALID);
}

static void test_vulkan_surface_backend(void)
{
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER_CONFIG reference_config;
	FB_GFX3_RENDERER renderer;
	FB_GFX3_RENDERER reference_renderer;
	FB_GFX3_HANDLE surface;
	FB_GFX3_HANDLE reference_surface = 0;
	FB_GFX3_HANDLE source_surface = 0;
	FB_GFX3_HANDLE reference_source = 0;
	FB_GFX3_HANDLE surface8;
	FB_GFX3_HANDLE surface16;
	FB_GFX3_COMMAND *command;
	FB_GFX3_PAGE_SET_COMMAND *page_payload;
	uint32_t upload_pixels[6] = {
		0x10111213u, 0x20212223u, 0xEEEEEEEEu,
		0x30313233u, 0x40414243u, 0xEEEEEEEEu
	};
	uint32_t download_pixels[6];
	uint32_t vulkan_pixels[64];
	uint32_t reference_pixels[64];
	uint32_t blit_source_pixels[64];
	static const uint32_t blit_modes[] = {
		FB_GFX3_BLIT_TRANS, FB_GFX3_BLIT_PSET,
		FB_GFX3_BLIT_PRESET, FB_GFX3_BLIT_AND,
		FB_GFX3_BLIT_OR, FB_GFX3_BLIT_XOR,
		FB_GFX3_BLIT_ALPHA, FB_GFX3_BLIT_ADD,
		FB_GFX3_BLIT_BLEND
	};
	size_t mode_index;
	int reference_initialized = FALSE;
	int result;

	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_vulkan;
	config.backend_config.width = 32;
	config.backend_config.height = 32;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 1;
	config.queue_capacity = 4;
	result = fb_gfx3_renderer_init(&renderer, &config);
	if (result == FB_GFX3_UNSUPPORTED) {
		printf("gfxlib3 Vulkan smoke: unsupported on this machine\n");
		return;
	}
	CHECK(result == FB_GFX3_OK);
	if (result != FB_GFX3_OK)
		return;
	reference_config = config;
	reference_config.backend = &__fb_gfx3_backend_null;
	result = fb_gfx3_renderer_init(&reference_renderer, &reference_config);
	CHECK(result == FB_GFX3_OK);
	if (result == FB_GFX3_OK)
		reference_initialized = TRUE;

	surface = create_test_surface(&renderer, NULL, 32, 0x12345678u);
	source_surface = create_test_surface(&renderer, NULL, 32, 0);
	if (reference_initialized)
		reference_surface = create_test_surface(&reference_renderer, NULL,
			32, 0x12345678u);
	if (reference_initialized)
		reference_source = create_test_surface(&reference_renderer, NULL,
			32, 0);
	CHECK(surface != 0);
	CHECK(source_surface != 0);
	CHECK(!reference_initialized || (reference_surface != 0));
	CHECK(!reference_initialized || (reference_source != 0));
	if (surface != 0) {
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x12345678u);
		CHECK(read_test_pixel(&renderer, surface, -1, 0) == UINT32_MAX);
		upload_test_surface(&renderer, surface, 3, 4, 2, 2, 12,
			upload_pixels);
		CHECK(read_test_pixel(&renderer, surface, 3, 4) ==
			upload_pixels[0]);
		CHECK(read_test_pixel(&renderer, surface, 4, 4) ==
			upload_pixels[1]);
		CHECK(read_test_pixel(&renderer, surface, 3, 5) ==
			upload_pixels[3]);
		memset(download_pixels, 0xCC, sizeof(download_pixels));
		download_test_surface(&renderer, surface, 3, 4, 2, 2, 12,
			download_pixels);
		CHECK(download_pixels[0] == upload_pixels[0]);
		CHECK(download_pixels[1] == upload_pixels[1]);
		CHECK(download_pixels[2] == 0xCCCCCCCCu);
		CHECK(download_pixels[3] == upload_pixels[3]);
		CHECK(download_pixels[4] == upload_pixels[4]);
		CHECK(download_pixels[5] == 0xCCCCCCCCu);
		clear_test_surface(&renderer, surface, 0xAABBCCDDu);
		CHECK(read_test_pixel(&renderer, surface, 7, 7) == 0xAABBCCDDu);
		set_test_pixel(&renderer, surface, 1, 1, 0xDEADBEEFu);
		CHECK(read_test_pixel(&renderer, surface, 1, 1) == 0xDEADBEEFu);

		if (reference_surface != 0) {
			clear_test_surface(&renderer, surface, 0);
			clear_test_surface(&reference_renderer, reference_surface, 0);
			draw_test_line(&renderer, surface, 0, 7, 7, 0,
				0xDEADBEEFu, 0xFFFFu);
			draw_test_line(&reference_renderer, reference_surface,
				0, 7, 7, 0, 0xDEADBEEFu, 0xFFFFu);
			draw_test_line(&renderer, surface, 0, 3, 7, 3,
				0x55667788u, 0xAAAAu);
			draw_test_line(&reference_renderer, reference_surface,
				0, 3, 7, 3, 0x55667788u, 0xAAAAu);
			draw_test_line(&renderer, surface, -3, 5, 5, 5,
				0x10203040u, 0x9249u);
			draw_test_line(&reference_renderer, reference_surface,
				-3, 5, 5, 5, 0x10203040u, 0x9249u);
			draw_test_rectangle(&renderer, surface, -2, -2, 2, 2,
				0x89ABCDEFu, 0xFFFFu, TRUE);
			draw_test_rectangle(&reference_renderer,
				reference_surface, -2, -2, 2, 2,
				0x89ABCDEFu, 0xFFFFu, TRUE);
			draw_test_rectangle(&renderer, surface, 1, 1, 6, 6,
				0x76543210u, 0xA55Au, FALSE);
			draw_test_rectangle(&reference_renderer,
				reference_surface, 1, 1, 6, 6,
				0x76543210u, 0xA55Au, FALSE);
			draw_test_ellipse(&renderer, surface, 4, 4, 3.0f, 2.0f,
				0xABCDEF01u, FALSE);
			draw_test_ellipse(&reference_renderer, reference_surface,
				4, 4, 3.0f, 2.0f, 0xABCDEF01u, FALSE);
			draw_test_ellipse(&renderer, surface, 7, 0, 2.0f, 1.0f,
				0x13572468u, TRUE);
			draw_test_ellipse(&reference_renderer, reference_surface,
				7, 0, 2.0f, 1.0f, 0x13572468u, TRUE);
			download_test_surface(&renderer, surface, 0, 0, 8, 8,
				8 * sizeof(uint32_t), vulkan_pixels);
			download_test_surface(&reference_renderer,
				reference_surface, 0, 0, 8, 8,
				8 * sizeof(uint32_t), reference_pixels);
			CHECK(memcmp(vulkan_pixels, reference_pixels,
				sizeof(vulkan_pixels)) == 0);

			if ((source_surface != 0) && (reference_source != 0)) {
				for (mode_index = 0; mode_index < 64;
				     mode_index++) {
					blit_source_pixels[mode_index] =
						0x20000000u |
						((uint32_t)mode_index * 0x00030507u);
				}
				blit_source_pixels[9] = 0x00FF00FFu;
				upload_test_surface(&renderer, source_surface,
					0, 0, 8, 8, 8 * sizeof(uint32_t),
					blit_source_pixels);
				upload_test_surface(&reference_renderer,
					reference_source, 0, 0, 8, 8,
					8 * sizeof(uint32_t), blit_source_pixels);
				for (mode_index = 0;
				     mode_index < sizeof(blit_modes) /
					     sizeof(blit_modes[0]); mode_index++) {
					clear_test_surface(&renderer, surface,
						0x20304050u);
					clear_test_surface(&reference_renderer,
						reference_surface, 0x20304050u);
					blit_test_surface(&renderer, surface,
						source_surface, 1, 1, 6, 6, 1, 0,
						blit_modes[mode_index], 127);
					blit_test_surface(&reference_renderer,
						reference_surface, reference_source,
						1, 1, 6, 6, 1, 0,
						blit_modes[mode_index], 127);
					download_test_surface(&renderer, surface,
						0, 0, 8, 8, 8 * sizeof(uint32_t),
						vulkan_pixels);
					download_test_surface(&reference_renderer,
						reference_surface, 0, 0, 8, 8,
						8 * sizeof(uint32_t), reference_pixels);
					CHECK(memcmp(vulkan_pixels,
						reference_pixels,
						sizeof(vulkan_pixels)) == 0);
				}

				upload_test_surface(&renderer, surface, 0, 0,
					8, 8, 8 * sizeof(uint32_t),
					blit_source_pixels);
				upload_test_surface(&reference_renderer,
					reference_surface, 0, 0, 8, 8,
					8 * sizeof(uint32_t), blit_source_pixels);
				blit_test_surface(&renderer, surface, surface,
					0, 0, 5, 5, 2, 2, FB_GFX3_BLIT_PSET, 255);
				blit_test_surface(&reference_renderer,
					reference_surface, reference_surface,
					0, 0, 5, 5, 2, 2, FB_GFX3_BLIT_PSET, 255);
				download_test_surface(&renderer, surface, 0, 0,
					8, 8, 8 * sizeof(uint32_t), vulkan_pixels);
				download_test_surface(&reference_renderer,
					reference_surface, 0, 0, 8, 8,
					8 * sizeof(uint32_t), reference_pixels);
				CHECK(memcmp(vulkan_pixels, reference_pixels,
					sizeof(vulkan_pixels)) == 0);
			}
		}

		command = fb_gfx3_command_create(FB_GFX3_COMMAND_PAGE_SET,
			sizeof(*page_payload));
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = surface;
			page_payload = (FB_GFX3_PAGE_SET_COMMAND *)command->payload;
			page_payload->width = 8;
			page_payload->height = 8;
			page_payload->depth = 32;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
		command = fb_gfx3_command_create(FB_GFX3_COMMAND_PRESENT, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = surface;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}

	surface8 = create_test_surface(&renderer, NULL, 8, 0x1234u);
	CHECK(surface8 != 0);
	if (surface8 != 0)
		CHECK(read_test_pixel(&renderer, surface8, 2, 2) == 0x34u);
	surface16 = create_test_surface(&renderer, NULL, 16, 0x12345678u);
	CHECK(surface16 != 0);
	if (surface16 != 0)
		CHECK(read_test_pixel(&renderer, surface16, 2, 2) == 0x5678u);

	if (surface16 != 0) {
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = surface16;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}
	if (surface8 != 0) {
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = surface8;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}
	if (surface != 0) {
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = surface;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}
	if (source_surface != 0) {
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = source_surface;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}
	if (reference_surface != 0) {
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = reference_surface;
			CHECK(submit_and_wait(&reference_renderer, command, NULL,
				NULL) == FB_GFX3_OK);
		}
	}
	if (reference_source != 0) {
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = reference_source;
			CHECK(submit_and_wait(&reference_renderer, command, NULL,
				NULL) == FB_GFX3_OK);
		}
	}
	CHECK(fb_gfx3_renderer_shutdown(&renderer) == FB_GFX3_OK);
	if (reference_initialized)
		CHECK(fb_gfx3_renderer_shutdown(&reference_renderer) ==
			FB_GFX3_OK);
	printf("gfxlib3 Vulkan smoke: device-local surfaces passed\n");
}

static void test_opengl_compute_backend(void)
{
	FB_GFX3_RENDERER_CONFIG config;
	FB_GFX3_RENDERER_CONFIG reference_config;
	FB_GFX3_RENDERER renderer;
	FB_GFX3_RENDERER reference_renderer;
	FB_GFX3_LOGGER logger;
	FB_GFX3_HANDLE surface;
	FB_GFX3_HANDLE source_surface;
	FB_GFX3_HANDLE reference_surface;
	FB_GFX3_COMMAND *command;
	struct OPENGL_LINE_CASE {
		int x1;
		int y1;
		int x2;
		int y2;
		uint32_t color;
		uint32_t style;
	};
	struct OPENGL_ELLIPSE_CASE {
		int center_x;
		int center_y;
		float radius_x;
		float radius_y;
		uint32_t color;
		int filled;
	};
	static const struct OPENGL_LINE_CASE line_cases[] = {
		{ -3, 0, 7, 5, 0x01010101u, 0xFFFFu },
		{ 7, 7, 0, 2, 0x02020202u, 0xA55Au },
		{ 3, -3, 3, 7, 0x03030303u, 0xF0F0u },
		{ 7, 4, -2, 4, 0x04040404u, 0xCCCCu },
		{ 0, 0, 3, 7, 0x05050505u, 0xFFFFu },
		{ 7, 0, 0, 7, 0x06060606u, 0xAAAAu }
	};
	static const struct OPENGL_ELLIPSE_CASE ellipse_cases[] = {
		{ 4, 4, 3.0f, 2.0f, 0x11111111u, FALSE },
		{ 2, 2, 2.75f, 1.5f, 0x22222222u, TRUE },
		{ 7, 3, 3.0f, 3.0f, 0x33333333u, FALSE },
		{ 3, 6, 2.0f, 0.0f, 0x44444444u, FALSE }
	};
	uint32_t upload_pixels[6] = {
		0x10111213u, 0x20212223u, 0xEEEEEEEEu,
		0x30313233u, 0x40414243u, 0xEEEEEEEEu
	};
	uint32_t download_pixels[6];
	int i;
	int x;
	int y;
	int result;

	CHECK(fb_gfx3_log_init(&logger) == FB_GFX3_OK);
	CHECK(fb_gfx3_log_set(&logger, FB_GFX3_LOG_INFO,
		capture_log_message, NULL) == FB_GFX3_OK);
	memset(&config, 0, sizeof(config));
	config.backend = &__fb_gfx3_backend_opengl;
	config.backend_config.logger = &logger;
	config.backend_config.width = 32;
	config.backend_config.height = 32;
	config.backend_config.depth = 32;
	config.backend_config.page_count = 1;
	config.queue_capacity = 4;
	result = fb_gfx3_renderer_init(&renderer, &config);
	if (result == FB_GFX3_UNSUPPORTED) {
		printf("gfxlib3 OpenGL smoke: unsupported on this machine\n");
		fb_gfx3_log_destroy(&logger);
		return;
	}
	CHECK(result == FB_GFX3_OK);
	if (result != FB_GFX3_OK) {
		fprintf(stderr, "gfxlib3 OpenGL initialization failed: %s\n",
			last_log_message);
		fb_gfx3_log_destroy(&logger);
		return;
	}
	printf("gfxlib3 OpenGL smoke: %s\n", last_log_message);

	surface = create_test_surface(&renderer, NULL, 32, 0x12345678u);
	CHECK(surface != 0);
	if (surface != 0) {
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x12345678u);
		clear_test_surface(&renderer, surface, 0x01020304u);
		set_test_pixel(&renderer, surface, 3, 5, 0xAABBCCDDu);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x01020304u);
		CHECK(read_test_pixel(&renderer, surface, 3, 5) == 0xAABBCCDDu);
		memset(download_pixels, 0xCC, sizeof(download_pixels));
		upload_test_surface(&renderer, surface, 2, 4, 2, 2, 12,
			upload_pixels);
		download_test_surface(&renderer, surface, 2, 4, 2, 2, 12,
			download_pixels);
		CHECK(download_pixels[0] == upload_pixels[0]);
		CHECK(download_pixels[1] == upload_pixels[1]);
		CHECK(download_pixels[2] == 0xCCCCCCCCu);
		CHECK(download_pixels[3] == upload_pixels[3]);
		CHECK(download_pixels[4] == upload_pixels[4]);
		CHECK(download_pixels[5] == 0xCCCCCCCCu);

		source_surface = create_test_surface(&renderer, NULL, 32,
			0x00FF00FFu);
		CHECK(source_surface != 0);
		set_test_pixel(&renderer, source_surface, 0, 0, 0x12345678u);
		clear_test_surface(&renderer, surface, 0xAABBCCDDu);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 1, 0,
			3, 3, FB_GFX3_BLIT_TRANS, 255);
		CHECK(read_test_pixel(&renderer, surface, 3, 3) == 0x00345678u);
		CHECK(read_test_pixel(&renderer, surface, 4, 3) == 0xAABBCCDDu);

		set_test_pixel(&renderer, source_surface, 0, 0, 0x00F00F00u);
		clear_test_surface(&renderer, surface, 0x000FF000u);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_PRESET, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF0FF0FFu);
		clear_test_surface(&renderer, surface, 0x000FF000u);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_AND, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0);
		clear_test_surface(&renderer, surface, 0x000FF000u);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_OR, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x00FFFF00u);
		clear_test_surface(&renderer, surface, 0x000FF000u);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_XOR, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0x00FFFF00u);

		set_test_pixel(&renderer, source_surface, 0, 0, 0xFF112233u);
		clear_test_surface(&renderer, surface, 0x88776655u);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_ALPHA, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF112233u);
		clear_test_surface(&renderer, surface, 0x88776655u);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_BLEND, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF112233u);
		clear_test_surface(&renderer, surface, 0xFFFFFFFFu);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_ADD, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFFFFFFFFu);
		clear_test_surface(&renderer, surface, 0);
		blit_test_surface(&renderer, surface, source_surface, 0, 0, 0, 0,
			0, 0, FB_GFX3_BLIT_PSET, 255);
		CHECK(read_test_pixel(&renderer, surface, 0, 0) == 0xFF112233u);
		clear_test_surface(&renderer, surface, 0);
		for (i = 0; i < 4; i++)
			set_test_pixel(&renderer, surface, i, 0, (uint32_t)(i + 1));
		blit_test_surface(&renderer, surface, surface, 0, 0, 3, 0,
			1, 0, FB_GFX3_BLIT_PSET, 255);
		for (i = 0; i < 4; i++)
			CHECK(read_test_pixel(&renderer, surface, i + 1, 0) ==
				(uint32_t)(i + 1));

		clear_test_surface(&renderer, surface, 0);
		draw_test_line(&renderer, surface, 0, 7, 7, 0,
			0xDEADBEEFu, 0xFFFFu);
		for (i = 0; i < 8; i++)
			CHECK(read_test_pixel(&renderer, surface, i, 7 - i) ==
				0xDEADBEEFu);
		draw_test_line(&renderer, surface, 0, 3, 7, 3,
			0x55667788u, 0xAAAAu);
		for (i = 0; i < 8; i += 2)
			CHECK(read_test_pixel(&renderer, surface, i, 3) ==
				0x55667788u);

		memset(&reference_config, 0, sizeof(reference_config));
		reference_config.backend = &__fb_gfx3_backend_null;
		reference_config.backend_config.width = 32;
		reference_config.backend_config.height = 32;
		reference_config.backend_config.depth = 32;
		reference_config.backend_config.page_count = 1;
		reference_config.queue_capacity = 4;
		CHECK(fb_gfx3_renderer_init(&reference_renderer,
			&reference_config) == FB_GFX3_OK);
		reference_surface = create_test_surface(&reference_renderer, NULL,
			32, 0);
		CHECK(reference_surface != 0);
		clear_test_surface(&renderer, surface, 0);
		for (i = 0; i < (int)(sizeof(line_cases) /
		    sizeof(line_cases[0])); i++) {
			draw_test_line(&renderer, surface, line_cases[i].x1,
				line_cases[i].y1, line_cases[i].x2, line_cases[i].y2,
				line_cases[i].color, line_cases[i].style);
			draw_test_line(&reference_renderer, reference_surface,
				line_cases[i].x1, line_cases[i].y1, line_cases[i].x2,
				line_cases[i].y2, line_cases[i].color,
				line_cases[i].style);
		}
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				CHECK(read_test_pixel(&renderer, surface, x, y) ==
					read_test_pixel(&reference_renderer,
						reference_surface, x, y));
			}
		}
		clear_test_surface(&renderer, surface, 0);
		clear_test_surface(&reference_renderer, reference_surface, 0);
		draw_test_rectangle(&renderer, surface, -2, -2, 2, 2,
			0x89ABCDEFu, 0xFFFFu, TRUE);
		draw_test_rectangle(&reference_renderer, reference_surface,
			-2, -2, 2, 2, 0x89ABCDEFu, 0xFFFFu, TRUE);
		draw_test_rectangle(&renderer, surface, 1, 1, 6, 6,
			0x76543210u, 0xA55Au, FALSE);
		draw_test_rectangle(&reference_renderer, reference_surface,
			1, 1, 6, 6, 0x76543210u, 0xA55Au, FALSE);
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				CHECK(read_test_pixel(&renderer, surface, x, y) ==
					read_test_pixel(&reference_renderer,
						reference_surface, x, y));
			}
		}
		clear_test_surface(&renderer, surface, 0);
		clear_test_surface(&reference_renderer, reference_surface, 0);
		for (i = 0; i < (int)(sizeof(ellipse_cases) /
		    sizeof(ellipse_cases[0])); i++) {
			draw_test_ellipse(&renderer, surface,
				ellipse_cases[i].center_x, ellipse_cases[i].center_y,
				ellipse_cases[i].radius_x, ellipse_cases[i].radius_y,
				ellipse_cases[i].color, ellipse_cases[i].filled);
			draw_test_ellipse(&reference_renderer, reference_surface,
				ellipse_cases[i].center_x, ellipse_cases[i].center_y,
				ellipse_cases[i].radius_x, ellipse_cases[i].radius_y,
				ellipse_cases[i].color, ellipse_cases[i].filled);
		}
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				CHECK(read_test_pixel(&renderer, surface, x, y) ==
					read_test_pixel(&reference_renderer,
						reference_surface, x, y));
			}
		}
		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = reference_surface;
			CHECK(submit_and_wait(&reference_renderer, command, NULL,
				NULL) == FB_GFX3_OK);
		}
		CHECK(fb_gfx3_renderer_shutdown(&reference_renderer) ==
			FB_GFX3_OK);

		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = source_surface;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}

		command = fb_gfx3_command_create(
			FB_GFX3_COMMAND_SURFACE_DESTROY, 0);
		CHECK(command != NULL);
		if (command != NULL) {
			command->target = surface;
			CHECK(submit_and_wait(&renderer, command, NULL, NULL) ==
				FB_GFX3_OK);
		}
	}
	CHECK(fb_gfx3_renderer_shutdown(&renderer) == FB_GFX3_OK);
	fb_gfx3_log_destroy(&logger);
}

static void test_central_logger(void)
{
	FB_GFX3_LOGGER logger;

	logged_messages = 0;
	memset(last_log_message, 0, sizeof(last_log_message));
	CHECK(fb_gfx3_log_init(&logger) == FB_GFX3_OK);
	CHECK(fb_gfx3_log_set(&logger, FB_GFX3_LOG_INFO,
		capture_log_message, NULL) == FB_GFX3_OK);
	fb_gfx3_log_write(&logger, FB_GFX3_LOG_TRACE, "not visible");
	CHECK(logged_messages == 0);
	fb_gfx3_log_write(&logger, FB_GFX3_LOG_INFO, "surface %d ready", 7);
	CHECK(logged_messages == 1);
	CHECK(strcmp(last_log_message, "surface 7 ready") == 0);
	fb_gfx3_log_destroy(&logger);
}

static void test_backend_selection(void)
{
	const FB_GFX3_BACKEND_VTABLE *plan[FB_GFX3_BACKEND_PLAN_CAPACITY];
	int attempt_flags[FB_GFX3_BACKEND_PLAN_CAPACITY];
	const FB_GFX3_BACKEND_VTABLE *preferred;
	const FB_GFX3_BACKEND_VTABLE *fallback;
	size_t count;
	char long_name[256];

#ifdef HOST_ANDROID
	preferred = &__fb_gfx3_backend_vulkan;
	fallback = &__fb_gfx3_backend_gles;
#else
	preferred = &__fb_gfx3_backend_opengl;
	fallback = &__fb_gfx3_backend_vulkan;
#endif

	count = fb_gfx3_backend_plan(0, NULL, plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 2);
	CHECK(plan[0] == preferred);
	CHECK(plan[1] == fallback);

	count = fb_gfx3_backend_attempt_plan(0, NULL, plan, attempt_flags,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 4);
	CHECK(plan[0] == preferred);
	CHECK(plan[1] == fallback);
	CHECK(plan[2] == preferred);
	CHECK(plan[3] == fallback);
	CHECK(attempt_flags[0] == 0);
	CHECK(attempt_flags[1] == 0);
	CHECK(attempt_flags[2] == 1);
	CHECK(attempt_flags[3] == 1);

	count = fb_gfx3_backend_attempt_plan(1, NULL, plan, attempt_flags,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 4);
	CHECK(attempt_flags[0] == 1);
	CHECK(attempt_flags[1] == 1);
	CHECK(attempt_flags[2] == 0);
	CHECK(attempt_flags[3] == 0);

	count = fb_gfx3_backend_plan(0, fallback->name, plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 2);
	CHECK(plan[0] == fallback);
	CHECK(plan[1] == preferred);
	count = fb_gfx3_backend_attempt_plan(0, fallback->name, plan,
		attempt_flags, FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 6);
	CHECK(plan[0] == fallback);
	CHECK(plan[1] == preferred);
	CHECK(plan[2] == fallback);
	CHECK(plan[3] == fallback);
	CHECK(plan[4] == preferred);
	CHECK(plan[5] == fallback);
	CHECK(attempt_flags[0] == 0);
	CHECK(attempt_flags[1] == 0);
	CHECK(attempt_flags[2] == 0);
	CHECK(attempt_flags[3] == 1);
	CHECK(attempt_flags[4] == 1);
	CHECK(attempt_flags[5] == 1);

	count = fb_gfx3_backend_plan(0, "unavailable backend", plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 2);
	CHECK(plan[0] == preferred);
	CHECK(plan[1] == fallback);
	count = fb_gfx3_backend_attempt_plan(0, "unavailable backend", plan,
		attempt_flags, FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 4);
	CHECK(plan[0] == preferred);
	CHECK(plan[1] == fallback);
	CHECK(plan[2] == preferred);
	CHECK(plan[3] == fallback);
	CHECK(attempt_flags[0] == 0);
	CHECK(attempt_flags[1] == 0);
	CHECK(attempt_flags[2] == 1);
	CHECK(attempt_flags[3] == 1);

	count = fb_gfx3_backend_plan(0x00000200, NULL, plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 1);
	CHECK(plan[0] == &__fb_gfx3_backend_vulkan);
	count = fb_gfx3_backend_attempt_plan(0x00000200, NULL, plan,
		attempt_flags, FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 2);
	CHECK(plan[0] == &__fb_gfx3_backend_vulkan);
	CHECK(plan[1] == &__fb_gfx3_backend_vulkan);
	CHECK(attempt_flags[0] == 0x00000200);
	CHECK(attempt_flags[1] == 0x00000201);

	count = fb_gfx3_backend_plan(0x00000002, NULL, plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 1);
#ifdef HOST_ANDROID
	CHECK(plan[0] == &__fb_gfx3_backend_gles);
#else
	CHECK(plan[0] == &__fb_gfx3_backend_opengl);
#endif

	count = fb_gfx3_backend_plan(-1, NULL, plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 1);
	CHECK(plan[0] == &__fb_gfx3_backend_null);
	count = fb_gfx3_backend_attempt_plan(-1, NULL, plan, attempt_flags,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 1);
	CHECK(plan[0] == &__fb_gfx3_backend_null);
	CHECK(attempt_flags[0] == -1);

	count = fb_gfx3_backend_plan(0, "null", plan,
		FB_GFX3_BACKEND_PLAN_CAPACITY);
	CHECK(count == 1);
	CHECK(plan[0] == &__fb_gfx3_backend_null);

	memset(long_name, 'x', sizeof(long_name));
	CHECK(fb_gfx3_backend_set_requested_name(long_name,
		sizeof(long_name)) == FB_GFX3_OK);
	CHECK(fb_gfx3_backend_requested_name() != NULL);
	CHECK(strlen(fb_gfx3_backend_requested_name()) == 127);
	CHECK(fb_gfx3_backend_set_requested_name(NULL, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_backend_requested_name() == NULL);
}

static void test_vulkan_adapter_ranking(void)
{
	uint64_t float64_integrated;
	uint64_t float64_discrete;
	uint64_t non_float64_discrete;

	float64_integrated = fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_INTEGRATED_GPU, TRUE,
		FB_GFX3_VULKAN_QUEUE_GRAPHICS | FB_GFX3_VULKAN_QUEUE_COMPUTE);
	float64_discrete = fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_DISCRETE_GPU, TRUE,
		FB_GFX3_VULKAN_QUEUE_GRAPHICS | FB_GFX3_VULKAN_QUEUE_COMPUTE);
	non_float64_discrete = fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_DISCRETE_GPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_GRAPHICS | FB_GFX3_VULKAN_QUEUE_COMPUTE);

	/* Float64 preserves the exact ellipse path selected by the original code. */
	CHECK(float64_integrated > non_float64_discrete);
	CHECK(float64_discrete > float64_integrated);
	CHECK(fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_INTEGRATED_GPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE) >
		fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_VIRTUAL_GPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE));
	CHECK(fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_VIRTUAL_GPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE) >
		fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_CPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE));
	CHECK(fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_CPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE) >
		fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_OTHER, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE));
	CHECK(fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_DISCRETE_GPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_GRAPHICS | FB_GFX3_VULKAN_QUEUE_COMPUTE) >
		fb_gfx3_vulkan_device_score(
		FB_GFX3_VULKAN_DEVICE_TYPE_DISCRETE_GPU, FALSE,
		FB_GFX3_VULKAN_QUEUE_COMPUTE));
}

static void test_gamepad_snapshot_lifecycle(void)
{
	FB_GFX3_INPUT_STATE input;
	FB_GFX3_GAMEPAD_STATE snapshot;
	float axis[FB_GFX3_INPUT_GAMEPAD_AXIS_COUNT] = {
		-1.0f, 1.0f, 0.25f, -0.25f, 0.0f, 0.0f, 0.0f, 0.0f
	};

	CHECK(fb_gfx3_input_init(&input, 16, 16) == FB_GFX3_OK);
	CHECK(!fb_gfx3_input_gamepad_snapshot(&input, 5, &snapshot));
	CHECK(fb_gfx3_input_platform_gamepad_replace(&input, 5, TRUE,
		XPAD_BUTTON_A | XPAD_BUTTON_L2, axis, 0.4f, 0.8f,
		XPAD_DPAD_UP | XPAD_DPAD_RIGHT) == FB_GFX3_OK);
	CHECK(fb_gfx3_input_gamepad_snapshot(&input, 0, &snapshot));
	CHECK(snapshot.device_id == 5);
	CHECK(snapshot.seen && snapshot.connected);
	CHECK(snapshot.buttons == (XPAD_BUTTON_A | XPAD_BUTTON_L2));
	CHECK(snapshot.dpad == (XPAD_DPAD_UP | XPAD_DPAD_RIGHT));
	CHECK(snapshot.axis[0] == -1.0f);
	CHECK(snapshot.axis[3] == -0.25f);
	CHECK(snapshot.left_trigger == 0.4f);
	CHECK(snapshot.right_trigger == 0.8f);
	CHECK(fb_gfx3_input_platform_gamepad_replace(&input, 5, FALSE,
		0, NULL, 0.0f, 0.0f, 0) == FB_GFX3_OK);
	CHECK(fb_gfx3_input_gamepad_snapshot(&input, 0, &snapshot));
	CHECK(snapshot.seen && !snapshot.connected);
	CHECK(snapshot.buttons == (XPAD_BUTTON_A | XPAD_BUTTON_L2));
	CHECK(fb_gfx3_input_platform_gamepad_replace(&input, 6, FALSE,
		0, NULL, 0.0f, 0.0f, 0) == FB_GFX3_OK);
	CHECK(!fb_gfx3_input_gamepad_snapshot(&input, 1, &snapshot));
	fb_gfx3_input_destroy(&input);
}

int main(void)
{
	test_backend_selection();
	test_vulkan_adapter_ranking();
	test_gamepad_snapshot_lifecycle();
	test_checked_sizes_and_commands();
	test_queue_order_and_back_pressure();
	test_queue_close_and_failure_wakeups();
	test_completion();
	test_resource_registry();
	test_renderer_command_batch();
	test_renderer_lifecycle();
	test_renderer_failure_wakes_waiters();
	test_renderer_startup_failure_cleanup();
	test_opengl_partial_context_failure_cleanup();
	test_vulkan_native_window_failure_cleanup();
	test_typed_context_api();
	test_alpha_primitive_null_backend();
	test_compatibility_state();
	test_vulkan_surface_backend();
	test_opengl_compute_backend();
	test_central_logger();

	if (failures != 0) {
		fprintf(stderr, "gfxlib3 infrastructure: %d failure(s)\n", failures);
		return 1;
	}

	printf("gfxlib3 infrastructure: all checks passed\n");
	return 0;
}

/* end of infrastructure.c */
