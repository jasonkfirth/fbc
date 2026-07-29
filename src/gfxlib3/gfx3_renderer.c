/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_renderer.c

    Purpose:

        Run the selected renderer backend on one dedicated thread and preserve
        command order from the common queue.

    Responsibilities:

        - initialize GPU ownership on the render thread
		- execute accepted commands in sequence order
		- wake windowed backends periodically for idle native event pumping
		- complete synchronous requests only after backend visibility
        - fail and release every queued waiter after a renderer error

    This file intentionally does NOT contain:

        - backend selection policy
        - graphics primitive implementations
        - platform window creation
*/

#include "gfx3_renderer.h"
#include "gfx3_debug.h"
#include "gfx3_protocol.h"

#define FB_GFX3_RENDER_THREAD_STACK_BYTES (4u * 1024u * 1024u)

/*
	This thread-local pointer has a deliberately tiny lifetime.  It is set only
	while the renderer invokes a user-requested interop callback, which makes a
	SCREENGLPROC result useful without falsely implying that OpenGL is safe from
	the BASIC application thread.
*/
static _Thread_local FB_GFX3_RENDERER *renderer_callback_owner;

/*
	A 1024-command drain keeps the render thread responsive to input, readback,
	and shutdown while allowing a normal sprite frame to reach the GPU as one
	ordered submission. Smaller drains made the OpenGL winner/resolve path scan
	the same framebuffer once per 64 sprites, which is correct but needlessly
	expensive on the hardware renderer.
*/
#define FB_GFX3_RENDERER_BATCH_LIMIT 1024u

/*
	Renderer profile

	FBGFX3_PROFILE is intended for profiling unchanged applications.  Keeping
	the counters on the renderer thread avoids atomics in normal drawing paths,
	and disabling the environment option reduces the cost to one predictable
	branch per drained batch.  Reports describe completed command submission,
	not merely work accepted by the BASIC thread.
*/
#define FB_GFX3_PROFILE_COMMAND_SLOTS 32u
#define FB_GFX3_PROFILE_INTERVAL_SECONDS 1.0

typedef struct FB_GFX3_RENDER_PROFILE {
	uint64_t command_count[FB_GFX3_PROFILE_COMMAND_SLOTS];
	uint64_t commands;
	uint64_t command_bytes;
	uint64_t drains;
	uint64_t completion_commands;
	uint64_t completion_waits;
	uint64_t point_items;
	uint64_t line_items;
	uint64_t rectangle_items;
	uint64_t blit_items;
	uint64_t glyph_items;
	uint64_t upload_bytes;
	uint64_t download_bytes;
	size_t largest_drain;
	double interval_start;
	double execute_seconds;
	double completion_wait_seconds;
	int enabled;
} FB_GFX3_RENDER_PROFILE;

/* ------------------------------------------------------------------------- */
/* Render-thread execution                                                   */
/* ------------------------------------------------------------------------- */

static int fb_gfx3_renderer_profile_requested(void)
{
	const char *value = getenv("FBGFX3_PROFILE");

	if ((value == NULL) || (value[0] == '\0') || (strcmp(value, "0") == 0) ||
	    (strcmp(value, "off") == 0) || (strcmp(value, "OFF") == 0) ||
	    (strcmp(value, "false") == 0) || (strcmp(value, "FALSE") == 0))
		return FALSE;
	return TRUE;
}

static uint64_t fb_gfx3_renderer_profile_payload_count(
	const FB_GFX3_COMMAND *command, size_t member_offset, size_t member_size,
	uint32_t stated_count)
{
	size_t payload_size;
	size_t available_count;

	if ((command == NULL) || (member_size == 0u))
		return 0;
	payload_size = fb_gfx3_command_payload_size(command);
	if (payload_size < member_offset)
		return 0;
	available_count = (payload_size - member_offset) / member_size;
	if ((uint64_t)stated_count > (uint64_t)available_count)
		return (uint64_t)available_count;
	return stated_count;
}

static void fb_gfx3_renderer_profile_command(FB_GFX3_RENDER_PROFILE *profile,
	const FB_GFX3_COMMAND *command)
{
	size_t payload_size;

	if ((profile == NULL) || !profile->enabled || (command == NULL))
		return;
	if (command->type < FB_GFX3_PROFILE_COMMAND_SLOTS)
		profile->command_count[command->type]++;
	profile->commands++;
	profile->command_bytes += command->size;
	if (command->completion != NULL)
		profile->completion_commands++;

	payload_size = fb_gfx3_command_payload_size(command);
	switch (command->type) {
	case FB_GFX3_COMMAND_POINTS:
		if (payload_size >= offsetof(FB_GFX3_POINTS_COMMAND, point)) {
			const FB_GFX3_POINTS_COMMAND *payload =
				(const FB_GFX3_POINTS_COMMAND *)command->payload;

			profile->point_items += fb_gfx3_renderer_profile_payload_count(
				command, offsetof(FB_GFX3_POINTS_COMMAND, point),
				sizeof(payload->point[0]), payload->count);
		}
		break;
	case FB_GFX3_COMMAND_LINE:
		profile->line_items++;
		break;
	case FB_GFX3_COMMAND_LINES:
		if (payload_size >= offsetof(FB_GFX3_LINES_COMMAND, line)) {
			const FB_GFX3_LINES_COMMAND *payload =
				(const FB_GFX3_LINES_COMMAND *)command->payload;

			profile->line_items += fb_gfx3_renderer_profile_payload_count(
				command, offsetof(FB_GFX3_LINES_COMMAND, line),
				sizeof(payload->line[0]), payload->count);
		}
		break;
	case FB_GFX3_COMMAND_RECTANGLE:
		profile->rectangle_items++;
		break;
	case FB_GFX3_COMMAND_RECTANGLES:
		if (payload_size >= offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle)) {
			const FB_GFX3_RECTANGLES_COMMAND *payload =
				(const FB_GFX3_RECTANGLES_COMMAND *)command->payload;

			profile->rectangle_items +=
				fb_gfx3_renderer_profile_payload_count(command,
					offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle),
					sizeof(payload->rectangle[0]), payload->count);
		}
		break;
	case FB_GFX3_COMMAND_BLIT:
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
		profile->blit_items++;
		break;
	case FB_GFX3_COMMAND_BLITS:
		if (payload_size >= offsetof(FB_GFX3_BLITS_COMMAND, blit)) {
			const FB_GFX3_BLITS_COMMAND *payload =
				(const FB_GFX3_BLITS_COMMAND *)command->payload;

			profile->blit_items += fb_gfx3_renderer_profile_payload_count(
				command, offsetof(FB_GFX3_BLITS_COMMAND, blit),
				sizeof(payload->blit[0]), payload->count);
		}
		break;
	case FB_GFX3_COMMAND_GLYPHS:
		if (payload_size >= offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)) {
			const FB_GFX3_GLYPHS_COMMAND *payload =
				(const FB_GFX3_GLYPHS_COMMAND *)command->payload;

			profile->glyph_items += fb_gfx3_renderer_profile_payload_count(
				command, offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
				sizeof(payload->glyph[0]), payload->count);
		}
		break;
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
		if (payload_size >= offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data)) {
			const FB_GFX3_SURFACE_UPLOAD_COMMAND *payload =
				(const FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
			size_t available_bytes = payload_size -
				offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data);

			profile->upload_bytes +=
				(payload->data_size < available_bytes) ?
				payload->data_size : available_bytes;
		}
		break;
	case FB_GFX3_COMMAND_SURFACE_DOWNLOAD:
		if (payload_size == sizeof(FB_GFX3_SURFACE_DOWNLOAD_COMMAND)) {
			const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload =
				(const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *)command->payload;

			profile->download_bytes += payload->destination_size;
		}
		break;
	default:
		break;
	}
}

static void fb_gfx3_renderer_profile_reset(FB_GFX3_RENDER_PROFILE *profile,
	double now)
{
	int enabled;

	if (profile == NULL)
		return;
	enabled = profile->enabled;
	memset(profile, 0, sizeof(*profile));
	profile->enabled = enabled;
	profile->interval_start = now;
}

static void fb_gfx3_renderer_profile_report(FB_GFX3_RENDERER *renderer,
	FB_GFX3_RENDER_PROFILE *profile, int force)
{
	double now;
	double elapsed;
	double average_drain;

	if ((renderer == NULL) || (profile == NULL) || !profile->enabled)
		return;
	now = fb_Timer();
	elapsed = now - profile->interval_start;
	if (!force && (elapsed < FB_GFX3_PROFILE_INTERVAL_SECONDS))
		return;
	if (elapsed <= 0.0)
		elapsed = FB_GFX3_PROFILE_INTERVAL_SECONDS;
	average_drain = (profile->drains != 0u) ?
		(double)profile->commands / (double)profile->drains : 0.0;

	fb_gfx3_log_write(renderer->backend_config.logger, FB_GFX3_LOG_INFO,
		"profile %.3fs %s: %llu commands in %llu drains "
		"(average %.1f, maximum %u), %.3f ms execute, %.3f ms GPU waits",
		elapsed, renderer->backend_vtable->name,
		(unsigned long long)profile->commands,
		(unsigned long long)profile->drains, average_drain,
		(unsigned int)profile->largest_drain,
		profile->execute_seconds * 1000.0,
		profile->completion_wait_seconds * 1000.0);
	fb_gfx3_log_write(renderer->backend_config.logger, FB_GFX3_LOG_INFO,
		"profile work: %llu points, %llu lines, %llu rectangles, %llu blits, "
		"%llu glyphs, %.3f MiB upload, %.3f MiB download",
		(unsigned long long)profile->point_items,
		(unsigned long long)profile->line_items,
		(unsigned long long)profile->rectangle_items,
		(unsigned long long)profile->blit_items,
		(unsigned long long)profile->glyph_items,
		(double)profile->upload_bytes / (1024.0 * 1024.0),
		(double)profile->download_bytes / (1024.0 * 1024.0));
	fb_gfx3_log_write(renderer->backend_config.logger, FB_GFX3_LOG_INFO,
		"profile commands: clear=%llu points=%llu line=%llu rectangle=%llu "
		"ellipse=%llu paint=%llu blit=%llu blits=%llu transform=%llu "
		"present=%llu page=%llu read=%llu download=%llu input=%llu "
		"poll=%llu completions=%llu waits=%llu bytes=%llu",
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_CLEAR],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_POINTS],
		(unsigned long long)(profile->command_count[FB_GFX3_COMMAND_LINE] +
			profile->command_count[FB_GFX3_COMMAND_LINES]),
		(unsigned long long)(profile->command_count[
			FB_GFX3_COMMAND_RECTANGLE] + profile->command_count[
			FB_GFX3_COMMAND_RECTANGLES]),
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_ELLIPSE],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_PAINT],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_BLIT],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_BLITS],
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_TRANSFORM_BLIT],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_PRESENT],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_PAGE_SET],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_READ_PIXEL],
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_SURFACE_DOWNLOAD],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_INPUT_POLL],
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_PLATFORM_POLL],
		(unsigned long long)profile->completion_commands,
		(unsigned long long)profile->completion_waits,
		(unsigned long long)profile->command_bytes);
	fb_gfx3_log_write(renderer->backend_config.logger, FB_GFX3_LOG_INFO,
		"profile resources: create=%llu destroy=%llu upload=%llu "
		"glyph-packets=%llu palette=%llu barrier=%llu title=%llu",
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_SURFACE_CREATE],
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_SURFACE_DESTROY],
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_SURFACE_UPLOAD],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_GLYPHS],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_PALETTE],
		(unsigned long long)profile->command_count[FB_GFX3_COMMAND_BARRIER],
		(unsigned long long)profile->command_count[
			FB_GFX3_COMMAND_WINDOW_TITLE]);
	fb_gfx3_renderer_profile_reset(profile, now);
}

static void fb_gfx3_renderer_fail(FB_GFX3_RENDERER *renderer, int status)
{
	fb_gfx3_queue_fail(&renderer->queue, status);
	fb_gfx3_queue_discard(&renderer->queue, status);
}

static int fb_gfx3_renderer_backend_init(FB_GFX3_RENDERER *renderer)
{
	FB_GFX3_BACKEND_CAPS caps;
	int result;

	memset(&caps, 0, sizeof(caps));
	result = renderer->backend_vtable->probe(&caps);
	if (result != FB_GFX3_OK)
		return result;
	if (caps.abi_version != FB_GFX3_BACKEND_ABI_VERSION)
		return FB_GFX3_INVALID;

	renderer->backend.caps = caps;
	result = renderer->backend_vtable->init(&renderer->backend,
		&renderer->backend_config);
	if (result == FB_GFX3_OK)
		renderer->backend_initialized = TRUE;
	return result;
}

void *fb_gfx3_renderer_callback_gl_proc(const char *name)
{
	FB_GFX3_RENDERER *renderer = renderer_callback_owner;

	if ((name == NULL) || (name[0] == '\0') || (renderer == NULL) ||
	    !renderer->backend_initialized ||
	    (renderer->backend_vtable == NULL) ||
	    (renderer->backend_vtable->get_opengl_proc == NULL))
		return NULL;
	return renderer->backend_vtable->get_opengl_proc(&renderer->backend,
		name);
}

static int fb_gfx3_renderer_execute_interop_callback(
	FB_GFX3_RENDERER *renderer, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_INTEROP_CALLBACK_COMMAND *payload;
	FB_GFX3_INTEROP_CALLBACK callback;
	int result;

	if ((renderer == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)) ||
	    (renderer->backend_vtable == NULL) ||
	    (renderer->backend_vtable->get_opengl_proc == NULL))
		return FB_GFX3_UNSUPPORTED;
	payload = (const FB_GFX3_INTEROP_CALLBACK_COMMAND *)command->payload;
	callback = (FB_GFX3_INTEROP_CALLBACK)(uintptr_t)payload->callback;
	if (callback == NULL)
		return FB_GFX3_INVALID;

	/* Finish prior queued work before application code observes the context. */
	result = renderer->backend_vtable->wait_idle(&renderer->backend);
	if (result != FB_GFX3_OK)
		return result;
	renderer_callback_owner = renderer;
	callback((void *)(uintptr_t)payload->user_data);
	renderer_callback_owner = NULL;

	/* Make callback GL writes visible before gfxlib3 resumes ordered drawing. */
	return renderer->backend_vtable->wait_idle(&renderer->backend);
}

static int fb_gfx3_renderer_execute(FB_GFX3_RENDERER *renderer,
	FB_GFX3_COMMAND *const *commands, size_t count,
	FB_GFX3_RENDER_PROFILE *profile)
{
	FB_GFX3_COMMAND *command;
	uint64_t submitted_sequence = 0;
	double start_time = 0.0;
	size_t index;
	int result;

	if ((renderer == NULL) || (commands == NULL) || (count == 0))
		return FB_GFX3_INVALID;
	if ((count == 1) &&
	    (commands[0]->type == FB_GFX3_COMMAND_INTEROP_CALLBACK))
		return fb_gfx3_renderer_execute_interop_callback(renderer, commands[0]);
	for (index = 0; index < count; index++) {
		if ((commands[index] == NULL) ||
		    (commands[index]->type == FB_GFX3_COMMAND_INTEROP_CALLBACK))
			return FB_GFX3_INVALID;
	}
	if ((profile != NULL) && profile->enabled)
		start_time = fb_Timer();
	result = renderer->backend_vtable->execute(&renderer->backend,
		commands, count, &submitted_sequence);
	if ((profile != NULL) && profile->enabled)
		profile->execute_seconds += fb_Timer() - start_time;
	if (result != FB_GFX3_OK) {
		command = commands[0];
		fb_gfx3_log_write(renderer->backend_config.logger,
			FB_GFX3_LOG_ERROR,
			"%s command %u sequence %llu execution failed: %d",
			renderer->backend_vtable->name, command->type,
			(unsigned long long)command->sequence, result);
		return result;
	}
	command = commands[count - 1];
	if (submitted_sequence < command->sequence) {
		fb_gfx3_log_write(renderer->backend_config.logger,
			FB_GFX3_LOG_ERROR,
			"%s submitted sequence %llu behind command sequence %llu",
			renderer->backend_vtable->name,
			(unsigned long long)submitted_sequence,
			(unsigned long long)command->sequence);
		return FB_GFX3_FAILED;
	}
	for (index = 0; index < count; index++) {
		command = commands[index];
		if (command->completion == NULL)
			continue;
		/*
			INPUT_POLL completion reports that the window-owning thread has
			processed native messages. The backend has already submitted all older
			render commands in FIFO order, but input state does not depend on those
			GPU commands finishing. Waiting on their fence here made SCREENEVENT a
			graphics synchronization operation.
		*/
		if (command->type == FB_GFX3_COMMAND_INPUT_POLL)
			continue;
		if ((profile != NULL) && profile->enabled)
			start_time = fb_Timer();
		result = renderer->backend_vtable->wait_sequence(&renderer->backend,
			command->sequence);
		if ((profile != NULL) && profile->enabled) {
			profile->completion_waits++;
			profile->completion_wait_seconds += fb_Timer() - start_time;
		}
		if (result != FB_GFX3_OK) {
			fb_gfx3_log_write(renderer->backend_config.logger,
				FB_GFX3_LOG_ERROR,
				"%s command %u sequence %llu wait failed: %d",
				renderer->backend_vtable->name, command->type,
				(unsigned long long)command->sequence, result);
			return result;
		}
	}
	return FB_GFX3_OK;
}

static int fb_gfx3_renderer_command_batchable(
	const FB_GFX3_COMMAND *command)
{
	if (command == NULL)
		return FALSE;
	/*
		Surface destruction has no caller-visible result data. It may share an
		ordered batch with later asynchronous work because wait_sequence() will
		retire only its final use, not the tail of the batch. Other completion
		commands remain boundaries so their observable results retain the
		existing request/response behavior.
	*/
	return ((command->completion == NULL) ||
		(command->type == FB_GFX3_COMMAND_SURFACE_DESTROY)) &&
		(command->type != FB_GFX3_COMMAND_RENDERER_SHUTDOWN) &&
		(command->type != FB_GFX3_COMMAND_INTEROP_CALLBACK);
}

static void FBCALL fb_gfx3_idle_pump_thread(void *parameter)
{
	FB_GFX3_RENDERER *renderer = (FB_GFX3_RENDERER *)parameter;

	while (!atomic_load_explicit(&renderer->idle_pump_stop,
	       memory_order_acquire)) {
		FB_GFX3_COMMAND *command;
		uint64_t activity_epoch;
		int result;

		activity_epoch = atomic_load_explicit(&renderer->activity_epoch,
			memory_order_acquire);
		fb_Delay((int)renderer->idle_poll_milliseconds);
		if (atomic_load_explicit(&renderer->idle_pump_stop,
		    memory_order_acquire))
			break;
		/*
			Backend execution pumps the native window before processing commands.
			Only wake an otherwise idle renderer: periodic polls interleaved with
			a running game add queue traffic without improving input latency.
		*/
		if (atomic_load_explicit(&renderer->activity_epoch,
		    memory_order_acquire) != activity_epoch)
			continue;
		command = fb_gfx3_command_create(FB_GFX3_COMMAND_PLATFORM_POLL, 0);
		if (command == NULL)
			continue;
		result = fb_gfx3_queue_submit(&renderer->queue, command, NULL);
		if (result != FB_GFX3_OK) {
			fb_gfx3_command_destroy(command);
			break;
		}
	}
}

static void FBCALL fb_gfx3_render_thread(void *parameter)
{
	FB_GFX3_RENDERER *renderer = (FB_GFX3_RENDERER *)parameter;
	FB_GFX3_RENDER_PROFILE profile;
	FB_GFX3_COMMAND *commands[FB_GFX3_RENDERER_BATCH_LIMIT];
	FB_GFX3_COMMAND *command = NULL;
	FB_GFX3_COMMAND *pending_command = NULL;
	uint32_t command_type;
	size_t command_count;
	size_t command_index;
	int queue_closed = FALSE;
	int queue_result;
	int result;

	memset(&profile, 0, sizeof(profile));
	profile.enabled = fb_gfx3_renderer_profile_requested();
	profile.interval_start = fb_Timer();
	result = fb_gfx3_renderer_backend_init(renderer);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(renderer->backend_config.logger,
			(result == FB_GFX3_UNSUPPORTED) ? FB_GFX3_LOG_INFO :
			FB_GFX3_LOG_ERROR,
			"%s backend initialization failed: %d",
			renderer->backend_vtable->name, result);
		/* Backends must make shutdown safe after every partial init failure. */
		renderer->backend_vtable->shutdown(&renderer->backend);
		fb_gfx3_completion_finish(&renderer->startup, 0, result);
		fb_gfx3_renderer_fail(renderer, result);
		return;
	}

	fb_gfx3_completion_finish(&renderer->startup, 0, FB_GFX3_OK);
	for (;;) {
		if (pending_command != NULL) {
			command = pending_command;
			pending_command = NULL;
		} else {
			result = fb_gfx3_queue_pop(&renderer->queue, &command);
			if (result != FB_GFX3_OK) {
				if (result != FB_GFX3_CLOSED)
					fb_gfx3_log_write(renderer->backend_config.logger,
						FB_GFX3_LOG_ERROR,
						"%s command queue stopped: %d",
						renderer->backend_vtable->name, result);
				break;
			}
		}

		if (command->type == FB_GFX3_COMMAND_RENDERER_SHUTDOWN) {
			fb_gfx3_renderer_profile_report(renderer, &profile, TRUE);
			result = renderer->backend_vtable->wait_idle(&renderer->backend);
			/* GPU resource destructors require the owning context to be live. */
			fb_gfx3_resources_destroy(&renderer->resources);
			renderer->backend_vtable->shutdown(&renderer->backend);
			renderer->backend_initialized = FALSE;
			if (command->completion != NULL)
				fb_gfx3_completion_finish(command->completion,
					command->sequence, result);
			fb_gfx3_command_destroy(command);
			command = NULL;
			break;
		}

		commands[0] = command;
		command_count = 1;
		command_type = command->type;
		queue_result = FB_GFX3_EXHAUSTED;
		if (fb_gfx3_renderer_command_batchable(command)) {
			/*
				The first command wakes this thread immediately.  A sprite loop is
				usually still submitting its remaining commands at that instant;
				yield once so the producer can populate the FIFO before the
				non-blocking drain below.  fb_Delay(0) yields only to a ready peer,
				not a timed sleep, so isolated readback-bound commands retain their
				existing low-latency behavior.

				An asynchronous PRESENT is batchable too. The backend preserves all
				drawing and page-copy commands, but can omit an intermediate window
				swap when a newer frame is already in this drain. This prevents a
				fast producer from building a queue of obsolete displayed frames.
				POINT, SCREENSYNC, and every other completion command remain hard
				boundaries through fb_gfx3_renderer_command_batchable().
			*/
			fb_Delay(0);
			while ((command_count < FB_GFX3_RENDERER_BATCH_LIMIT) &&
			       (command_count < renderer->backend.caps.max_batch_commands)) {
				command = NULL;
				queue_result = fb_gfx3_queue_try_pop(&renderer->queue,
					&command);
				if (queue_result != FB_GFX3_OK)
					break;
				if (!fb_gfx3_renderer_command_batchable(command)) {
					pending_command = command;
					break;
				}
				commands[command_count++] = command;
			}
		}

		if ((queue_result != FB_GFX3_OK) &&
		    (queue_result != FB_GFX3_EXHAUSTED) &&
		    (queue_result != FB_GFX3_CLOSED)) {
			result = queue_result;
		} else {
			if (queue_result == FB_GFX3_CLOSED)
				queue_closed = TRUE;
			if (profile.enabled) {
				profile.drains++;
				if (command_count > profile.largest_drain)
					profile.largest_drain = command_count;
				for (command_index = 0; command_index < command_count;
				     command_index++)
					fb_gfx3_renderer_profile_command(&profile,
						commands[command_index]);
			}
			result = fb_gfx3_renderer_execute(renderer, commands, command_count,
				&profile);
			/*
				A periodic poll must not count as the activity which suppresses
				the next periodic poll. Otherwise a quiet window alternates
				between polling and skipping, stretching the intended 10 ms
				message-pump cadence toward 20 ms. Any other command already made
				the owning thread pump the platform before execution.
			*/
			for (command_index = 0; command_index < command_count;
			     command_index++) {
				if (commands[command_index]->type ==
				    FB_GFX3_COMMAND_PLATFORM_POLL)
					continue;
				atomic_fetch_add_explicit(&renderer->activity_epoch, 1u,
					memory_order_release);
				break;
			}
		}
		for (command_index = 0; command_index < command_count;
		     command_index++) {
			command = commands[command_index];
			if (command->completion != NULL)
				fb_gfx3_completion_finish(command->completion,
					command->sequence, result);
			fb_gfx3_command_destroy(command);
		}
		command = NULL;

		/*
			An explicit interop request on Null or Vulkan is a normal API-level
			unsupported result, not a device failure. Complete that caller and
			keep the queue usable so it can still close or continue drawing.
		*/
		if ((result != FB_GFX3_OK) &&
		    !((command_type == FB_GFX3_COMMAND_INTEROP_CALLBACK) &&
		      (result == FB_GFX3_UNSUPPORTED))) {
			fb_gfx3_renderer_fail(renderer, result);
			break;
		}

		fb_gfx3_resources_collect(&renderer->resources,
			renderer->backend_vtable->completed_sequence(&renderer->backend));
		fb_gfx3_renderer_profile_report(renderer, &profile, FALSE);
		if (queue_closed && (pending_command == NULL))
			break;
	}

	if (renderer->backend_initialized) {
		renderer->backend_vtable->wait_idle(&renderer->backend);
		fb_gfx3_resources_destroy(&renderer->resources);
		renderer->backend_vtable->shutdown(&renderer->backend);
		renderer->backend_initialized = FALSE;
	}
}

/* ------------------------------------------------------------------------- */
/* Renderer lifecycle                                                        */
/* ------------------------------------------------------------------------- */

static void fb_gfx3_renderer_stop_idle_pump(FB_GFX3_RENDERER *renderer)
{
	if ((renderer == NULL) || (renderer->idle_pump_thread == NULL))
		return;
	atomic_store_explicit(&renderer->idle_pump_stop, TRUE,
		memory_order_release);
	fb_ThreadWait(renderer->idle_pump_thread);
	renderer->idle_pump_thread = NULL;
}

static int fb_gfx3_renderer_cleanup(FB_GFX3_RENDERER *renderer, int status)
{
	fb_gfx3_renderer_stop_idle_pump(renderer);
	if (renderer->thread != NULL) {
		fb_ThreadWait(renderer->thread);
		renderer->thread = NULL;
	}

	fb_gfx3_queue_discard(&renderer->queue, status);
	fb_gfx3_resources_destroy(&renderer->resources);
	fb_gfx3_queue_destroy(&renderer->queue);
	memset(renderer, 0, sizeof(*renderer));
	return status;
}

int fb_gfx3_renderer_init(FB_GFX3_RENDERER *renderer,
	const FB_GFX3_RENDERER_CONFIG *config)
{
	size_t queue_capacity;
	int result;

	if ((renderer == NULL) || (config == NULL) || (config->backend == NULL) ||
	    (config->idle_poll_milliseconds > 1000u) ||
	    (config->backend->abi_version != FB_GFX3_BACKEND_ABI_VERSION) ||
	    (config->backend->probe == NULL) || (config->backend->init == NULL) ||
	    (config->backend->shutdown == NULL) ||
	    (config->backend->execute == NULL) ||
	    (config->backend->completed_sequence == NULL) ||
	    (config->backend->wait_sequence == NULL) ||
	    (config->backend->wait_idle == NULL))
		return FB_GFX3_INVALID;

	memset(renderer, 0, sizeof(*renderer));
	atomic_init(&renderer->idle_pump_stop, FALSE);
	atomic_init(&renderer->activity_epoch, 0u);
	renderer->idle_poll_milliseconds = config->idle_poll_milliseconds;
	queue_capacity = config->queue_capacity;
	if (queue_capacity == 0)
		queue_capacity = FB_GFX3_DEFAULT_QUEUE_CAPACITY;

	result = fb_gfx3_queue_init(&renderer->queue, queue_capacity);
	if (result != FB_GFX3_OK)
		return result;

	result = fb_gfx3_resources_init(&renderer->resources,
		config->resource_capacity);
	if (result != FB_GFX3_OK)
		goto fail_queue;

	result = fb_gfx3_completion_init(&renderer->startup);
	if (result != FB_GFX3_OK)
		goto fail_resources;

	renderer->backend_vtable = config->backend;
	renderer->backend_config = config->backend_config;
	renderer->backend_config.resources = &renderer->resources;
	/*
	    Android's PTHREAD_STACK_MIN can be only 16 KiB.  That is insufficient
	    for older EGL and Vulkan vendor call chains, even before shader compiler
	    stack use is considered.  The renderer owns one long-lived GPU thread,
	    so reserve the same 4 MiB stack used by the Android BASIC program thread.
	*/
	renderer->thread = fb_ThreadCreate(fb_gfx3_render_thread, renderer,
		FB_GFX3_RENDER_THREAD_STACK_BYTES);
	if (renderer->thread == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail_completion;
	}

	result = fb_gfx3_completion_wait(&renderer->startup, NULL);
	if (result != FB_GFX3_OK) {
		fb_ThreadWait(renderer->thread);
		renderer->thread = NULL;
		goto fail_completion;
	}

	fb_gfx3_completion_destroy(&renderer->startup);
	if (renderer->idle_poll_milliseconds != 0) {
		renderer->idle_pump_thread = fb_ThreadCreate(
			fb_gfx3_idle_pump_thread, renderer, 0);
		if (renderer->idle_pump_thread == NULL) {
			fb_gfx3_renderer_shutdown(renderer);
			return FB_GFX3_OUT_OF_MEMORY;
		}
	}
	return FB_GFX3_OK;

fail_completion:
	fb_gfx3_completion_destroy(&renderer->startup);
fail_resources:
	fb_gfx3_resources_destroy(&renderer->resources);
fail_queue:
	fb_gfx3_queue_destroy(&renderer->queue);
	memset(renderer, 0, sizeof(*renderer));
	return result;
}

int fb_gfx3_renderer_submit(FB_GFX3_RENDERER *renderer,
	FB_GFX3_COMMAND *command, uint64_t *sequence)
{
	if ((renderer == NULL) || (renderer->thread == NULL))
		return FB_GFX3_INVALID;

	/* Command ownership transfers to the renderer only after success. */
	return fb_gfx3_queue_submit(&renderer->queue, command, sequence);
}

int fb_gfx3_renderer_submit_many(FB_GFX3_RENDERER *renderer,
	FB_GFX3_COMMAND *const *commands, size_t count, uint64_t *sequence)
{
	if ((renderer == NULL) || (renderer->thread == NULL))
		return FB_GFX3_INVALID;

	/* Ownership transfers for every entry only after the whole FIFO is accepted. */
	return fb_gfx3_queue_submit_many(&renderer->queue, commands, count,
		sequence);
}

int fb_gfx3_renderer_shutdown(FB_GFX3_RENDERER *renderer)
{
	FB_GFX3_COMMAND *command;
	FB_GFX3_COMPLETION completion;
	int failure_code;
	int result;

	if ((renderer == NULL) || (renderer->thread == NULL))
		return FB_GFX3_INVALID;
	fb_gfx3_renderer_stop_idle_pump(renderer);

	result = fb_gfx3_completion_init(&completion);
	if (result != FB_GFX3_OK) {
		fb_gfx3_queue_close(&renderer->queue);
		return fb_gfx3_renderer_cleanup(renderer, result);
	}

	command = fb_gfx3_command_create(FB_GFX3_COMMAND_RENDERER_SHUTDOWN, 0);
	if (command == NULL) {
		fb_gfx3_queue_close(&renderer->queue);
		fb_gfx3_completion_destroy(&completion);
		return fb_gfx3_renderer_cleanup(renderer,
			FB_GFX3_OUT_OF_MEMORY);
	}
	command->completion = &completion;

	result = fb_gfx3_queue_submit_final(&renderer->queue, command, NULL);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_completion_wait(&completion, NULL);
	} else {
		fb_gfx3_command_destroy(command);
		failure_code = renderer->queue.failure_code;
		if ((result == FB_GFX3_FAILED) && (failure_code != FB_GFX3_OK))
			result = failure_code;
	}

	fb_gfx3_completion_destroy(&completion);
	return fb_gfx3_renderer_cleanup(renderer, result);
}

/* end of gfx3_renderer.c */
