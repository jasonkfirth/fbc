/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_backend_vulkan.c

    Purpose:

        Connect the common gfxlib3 command protocol to the Vulkan runtime and
        its device-local surface implementation.

    Responsibilities:

        - own the Vulkan runtime on the render thread
        - register generation-tagged Vulkan surfaces
        - execute surface creation, destruction, transfer, clear, and readback
        - route point, styled-line, and rectangle commands to compute shaders
        - own the Win32 window and visible swapchain on the render thread
        - convert and present indexed, RGB565, and 32-bit pages on the GPU
        - preserve command sequence and synchronous completion behavior

    This file intentionally does NOT contain:

        - Vulkan loader or ABI declarations
        - Vulkan loader, surface, swapchain, or shader ABI declarations
*/

#include "gfx3_backend_vulkan.h"
#include "gfx3_debug.h"
#include "gfx3_platform.h"
#include "gfx3_protocol.h"
#include "gfx3_resource.h"
#include "gfx3_vulkan.h"

typedef struct FB_GFX3_VULKAN_BACKEND_STATE {
	FB_GFX3_RESOURCE_REGISTRY *resources;
	FB_GFX3_LOGGER *logger;
	const FB_GFX3_PLATFORM_VTABLE *platform_vtable;
	void *platform;
	FB_GFX3_VULKAN_RUNTIME runtime;
	FB_GFX3_HANDLE visible_surface;
	uint32_t palette[256];
	uint32_t client_width;
	uint32_t client_height;
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY keyboard_overlay;
	uint32_t keyboard_overlay_state;
	int keyboard_overlay_known;
	uint64_t submitted_sequence;
	uint64_t completed_sequence;
	int presentation_dirty;
	/*
		Mixed primitive conversion runs only on the render thread. Retaining the
		largest host array avoids returning to the process allocator for every
		editor frame.
	*/
	FB_GFX3_VULKAN_PRIMITIVE *primitive_scratch;
	size_t primitive_scratch_size;
} FB_GFX3_VULKAN_BACKEND_STATE;

typedef struct FB_GFX3_VULKAN_BACKEND_SURFACE {
	FB_GFX3_VULKAN_BACKEND_STATE *owner;
	FB_GFX3_VULKAN_SURFACE surface;
} FB_GFX3_VULKAN_BACKEND_SURFACE;

/* One tiled dispatch accepts eight former 1,024-operation producer packets. */
#define FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT 8192u
/*
	Same-colour fills use one descriptor set and one workgroup row per command.
	Match the producer queue so one 1,024-sprite frame can reach one GPU
	dispatch instead of being split into four host submissions.
*/
#define FB_GFX3_VULKAN_BACKEND_RECTANGLE_BATCH_LIMIT 1024u
/* Must match the runtime's descriptor-set capacity for ordered line dispatches. */
#define FB_GFX3_VULKAN_BACKEND_LINE_BATCH_LIMIT 256u
/* Must match the runtime descriptor-set capacity for ordered ellipse dispatches. */
#define FB_GFX3_VULKAN_BACKEND_ELLIPSE_BATCH_LIMIT 256u
/*
	The runtime keeps each opaque POINTS command as an ordered compute dispatch,
	with a storage-write barrier before the following command. This preserves
	Basic's FIFO overwrite rules even when arcs, text, or PSET commands overlap.
	The backend limit must match the runtime descriptor-set capacity.
*/
#define FB_GFX3_VULKAN_BACKEND_POINTS_BATCH_LIMIT 256u
/*
	The mixed winner shader stores a one-based primitive index. Thirteen bits
	cover a complete renderer drain after packed LINE and POINTS commands are
	flattened.
*/
#define FB_GFX3_VULKAN_BACKEND_PRIMITIVE_BATCH_LIMIT 8191u
/*
	The Vulkan 1.0 minimum for maxComputeWorkGroupCount[0] is 65,535. The mixed
	shader uses a compact workgroup table, so this is a useful-work limit rather
	than a padded rectangle of the longest primitive by the command count.
*/
#define FB_GFX3_VULKAN_BACKEND_PRIMITIVE_WORKGROUP_LIMIT 65535u
/* One tiled dispatch uses the compact 13-bit ordered glyph index space. */
/* Bound per-pixel tile replay when console output repeatedly overwrites cells. */
#define FB_GFX3_VULKAN_BACKEND_GLYPH_BATCH_LIMIT 8191u
/* The renderer hands at most 1,024 ordered commands to one backend drain. */
#define FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT 1024u

typedef struct FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION {
	FB_GFX3_HANDLE source;
	FB_GFX3_RECT clip;
	FB_GFX3_RECT source_rect;
	int32_t destination_x;
	int32_t destination_y;
} FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION;

/* ------------------------------------------------------------------------- */
/* Surface ownership                                                         */
/* ------------------------------------------------------------------------- */

static uint32_t vulkan_backend_bytes_per_pixel(uint32_t depth)
{
	switch (depth) {
	case 1:
	case 2:
	case 4:
	case 8:
		return 1;
	case 16:
		return 2;
	case 32:
		return 4;
	default:
		return 0;
	}
}

static void vulkan_backend_surface_destroy(void *resource)
{
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface =
		(FB_GFX3_VULKAN_BACKEND_SURFACE *)resource;

	if (surface == NULL)
		return;
	if (surface->owner != NULL)
		fb_gfx3_vulkan_surface_destroy(&surface->owner->runtime,
			&surface->surface);
	free(surface);
}

static int vulkan_backend_surface_retain_handle(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_HANDLE handle,
	uint64_t sequence, FB_GFX3_VULKAN_BACKEND_SURFACE **surface)
{
	int result;

	result = fb_gfx3_resource_retain(state->resources, handle,
		FB_GFX3_RESOURCE_SURFACE, (void **)surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_resource_mark_used(state->resources, handle, sequence);
	if (result != FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		*surface = NULL;
	}
	return result;
}

static int vulkan_backend_surface_retain(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command,
	FB_GFX3_VULKAN_BACKEND_SURFACE **surface)
{
	return vulkan_backend_surface_retain_handle(state, command->target,
		command->sequence, surface);
}

static int vulkan_backend_surface_create(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_CREATE_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	FB_GFX3_HANDLE handle;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_CREATE_COMMAND *)command->payload;
	surface = (FB_GFX3_VULKAN_BACKEND_SURFACE *)
		calloc(1, sizeof(*surface));
	if (surface == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	surface->owner = state;
	result = fb_gfx3_vulkan_surface_create(&state->runtime,
		&surface->surface, payload->width, payload->height,
		payload->depth, payload->clear_color);
	if (result != FB_GFX3_OK) {
		fb_gfx3_log_write(state->logger, FB_GFX3_LOG_ERROR,
			"Vulkan surface allocation failed: result=%d size=%ux%u "
			"depth=%u storage-limit=%llu",
			result, payload->width, payload->height, payload->depth,
			(unsigned long long)
				state->runtime.maximum_storage_buffer_range);
		free(surface);
		return result;
	}
	handle = fb_gfx3_resource_register(state->resources,
		FB_GFX3_RESOURCE_SURFACE, surface,
		vulkan_backend_surface_destroy);
	if (handle == 0) {
		vulkan_backend_surface_destroy(surface);
		return FB_GFX3_OUT_OF_MEMORY;
	}
	if (fb_gfx3_completion_set_value(command->completion, 0, handle) !=
	    FB_GFX3_OK) {
		fb_gfx3_resource_release(state->resources, handle);
		fb_gfx3_resources_collect(state->resources, UINT64_MAX);
		return FB_GFX3_INVALID;
	}
	return FB_GFX3_OK;
}

static int vulkan_backend_surface_upload(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_UPLOAD_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	size_t header_size = offsetof(FB_GFX3_SURFACE_UPLOAD_COMMAND, data);
	size_t expected_data_size;
	size_t row_size;
	int result;

	if (fb_gfx3_command_payload_size(command) < header_size)
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_UPLOAD_COMMAND *)command->payload;
	if ((payload->width == 0) || (payload->height == 0) ||
	    (payload->data_size !=
	     fb_gfx3_command_payload_size(command) - header_size))
		return FB_GFX3_INVALID;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((fb_gfx3_size_multiply(payload->width,
	     vulkan_backend_bytes_per_pixel(surface->surface.depth),
	     &row_size) != FB_GFX3_OK) ||
	    (payload->source_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->source_pitch, payload->height,
	     &expected_data_size) != FB_GFX3_OK) ||
	    (expected_data_size != payload->data_size)) {
		result = FB_GFX3_INVALID;
	} else {
		result = fb_gfx3_vulkan_surface_upload(&state->runtime,
			&surface->surface, payload->destination_x,
			payload->destination_y, payload->width, payload->height,
			payload->data, payload->source_pitch);
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_surface_download(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	void *destination;
	size_t expected_size;
	size_t row_size;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_SURFACE_DOWNLOAD_COMMAND *)command->payload;
	if ((payload->destination_address == 0) ||
	    (payload->destination_address > UINTPTR_MAX))
		return FB_GFX3_INVALID;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((fb_gfx3_size_multiply(payload->width,
	     vulkan_backend_bytes_per_pixel(surface->surface.depth),
	     &row_size) != FB_GFX3_OK) ||
	    (payload->destination_pitch < row_size) ||
	    (fb_gfx3_size_multiply(payload->destination_pitch,
	     payload->height, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != payload->destination_size)) {
		result = FB_GFX3_INVALID;
	} else {
		destination = (void *)(uintptr_t)payload->destination_address;
		result = fb_gfx3_vulkan_surface_download(&state->runtime,
			&surface->surface, payload->source_x, payload->source_y,
			payload->width, payload->height, destination,
			payload->destination_pitch);
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Commands currently backed by device-local storage                         */
/* ------------------------------------------------------------------------- */

static int vulkan_backend_clear(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_CLEAR_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_CLEAR_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) {
		result = fb_gfx3_vulkan_surface_rectangle(&state->runtime,
			&surface->surface, &payload->clip, payload->clip.x1,
			payload->clip.y1, payload->clip.x2, payload->clip.y2,
			payload->color, 0xFFFFu, TRUE, payload->flags);
	} else {
		result = fb_gfx3_vulkan_surface_clear(&state->runtime,
			&surface->surface, payload->clip.x1, payload->clip.y1,
			payload->clip.x2, payload->clip.y2, payload->color);
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_points(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_POINTS_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	size_t points_size;
	size_t expected_size;
	int result;

	if (fb_gfx3_command_payload_size(command) <
	    offsetof(FB_GFX3_POINTS_COMMAND, point))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_POINTS_COMMAND *)command->payload;
	if ((fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
	     points_size, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_points(&state->runtime,
		&surface->surface, &payload->clip, payload->point,
		payload->count);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	The Vulkan runtime emits every POINTS command as its own dispatch and
	inserts a compute write-to-read barrier before the next dispatch. This is
	also the ordering required for alpha points: a later packet must blend
	against the result of the earlier packet. Points within one public packet
	already follow the same parallel rules as the single-command path.
*/
static size_t vulkan_backend_points_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_POINTS_COMMAND *first;
	size_t index;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_POINTS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return 1;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	if (available > FB_GFX3_VULKAN_BACKEND_POINTS_BATCH_LIMIT)
		available = FB_GFX3_VULKAN_BACKEND_POINTS_BATCH_LIMIT;
	/*
		The runtime batch records each opaque command as a separate dispatch and
		barriers those dispatches in FIFO order.  Unlike the older merge path,
		it is therefore safe when two commands cover the same pixel.
	*/
	for (index = 0; index < available; index++) {
		const FB_GFX3_POINTS_COMMAND *payload;
		size_t points_size;
		size_t expected_size;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point)))
			break;
		payload = (const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		if ((payload->count == 0u) ||
		    (memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
		     &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
		     points_size, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(commands[index])))
			break;
	}
	return (index > 1u) ? index : 1u;
}

static int vulkan_backend_points_batch(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	const FB_GFX3_POINTS_COMMAND *first;
	FB_GFX3_VULKAN_POINTS operations[
		FB_GFX3_VULKAN_BACKEND_POINTS_BATCH_LIMIT];
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_VULKAN_BACKEND_POINTS_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_POINTS_COMMAND, point)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_POINTS_COMMAND *)commands[0]->payload;
	/*
		Keep every public POINTS command distinct. The Vulkan runtime records
		them as ordered dispatches in one submission, avoiding a queue submit and
		fence cycle per arc without allowing overlapping writes to race.
	*/
	for (index = 0; index < count; index++) {
		const FB_GFX3_POINTS_COMMAND *payload;
		size_t points_size;
		size_t expected_size;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_POINTS) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) <
		     offsetof(FB_GFX3_POINTS_COMMAND, point)))
			return FB_GFX3_INVALID;
		payload = (const FB_GFX3_POINTS_COMMAND *)commands[index]->payload;
		if ((payload->count == 0u) ||
			(memcmp(&payload->clip, &first->clip, sizeof(payload->clip)) != 0) ||
			(fb_gfx3_size_multiply(payload->count, sizeof(payload->point[0]),
			 &points_size) != FB_GFX3_OK) ||
			(fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
			 points_size, &expected_size) != FB_GFX3_OK) ||
			(expected_size != fb_gfx3_command_payload_size(commands[index])))
			return FB_GFX3_INVALID;
		operations[index].clip = payload->clip;
		operations[index].points = payload->point;
		operations[index].point_count = payload->count;
	}
	result = vulkan_backend_surface_retain_handle(state, commands[0]->target,
		commands[count - 1u]->sequence, &surface);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_points_batch(&state->runtime,
			&surface->surface, operations, count);
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	}
	return result;
}

static int vulkan_backend_rectangle_workgroups(
	const FB_GFX3_RECTANGLE_COMMAND *rectangle, uint64_t *workgroups)
{
	uint64_t width;
	uint64_t height;
	uint64_t coverage;

	if ((rectangle == NULL) || (workgroups == NULL) ||
	    (rectangle->x1 > rectangle->x2) ||
	    (rectangle->y1 > rectangle->y2) ||
	    ((rectangle->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
		return FALSE;
	width = (uint64_t)((int64_t)rectangle->x2 - rectangle->x1) + 1u;
	height = (uint64_t)((int64_t)rectangle->y2 - rectangle->y1) + 1u;
	if ((width > 32767u) || (height > 32767u))
		return FALSE;
	if (rectangle->filled != 0u) {
		coverage = width * height;
		*workgroups = (coverage + 63u) / 64u;
	} else {
		coverage = (width + height) * 2u;
		*workgroups = (coverage + 63u) / 64u;
	}
	return TRUE;
}

/*
	Flatten one adjacent opaque point, line, rectangle, and ellipse stream. The
	public command count remains separate from the primitive count because
	POINTS, LINES, and RECTANGLES packets already carry many independent GPU
	operations.
*/
static size_t vulkan_backend_primitive_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available,
	size_t *primitive_count_result)
{
	FB_GFX3_HANDLE target;
	size_t command_count = 0u;
	size_t primitive_count = 0u;
	uint64_t useful_workgroups = 0u;

	if (primitive_count_result != NULL)
		*primitive_count_result = 0u;
	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (primitive_count_result == NULL))
		return 1u;
	target = commands[0]->target;
	while ((command_count < available) &&
	       (primitive_count < FB_GFX3_VULKAN_BACKEND_PRIMITIVE_BATCH_LIMIT)) {
		FB_GFX3_COMMAND *command = commands[command_count];
		size_t added_count = 0u;
		uint64_t added_workgroups = 0u;

		if ((command == NULL) || (command->target != target))
			break;
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			const FB_GFX3_POINTS_COMMAND *points;
			size_t point_bytes;
			size_t expected_size;
			uint32_t point_index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_POINTS_COMMAND, point))
				break;
			points = (const FB_GFX3_POINTS_COMMAND *)command->payload;
			if ((points->count == 0u) ||
			    (fb_gfx3_size_multiply(points->count,
			     sizeof(points->point[0]), &point_bytes) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(offsetof(FB_GFX3_POINTS_COMMAND, point),
			     point_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (point_index = 0u; point_index < points->count;
			     point_index++) {
				if ((points->point[point_index].flags &
				     FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u)
					break;
			}
			if (point_index != points->count)
				break;
			added_count = points->count;
			added_workgroups = points->count;
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			const FB_GFX3_LINE_COMMAND *line;
			uint64_t difference_x;
			uint64_t difference_y;
			uint64_t coverage;

			if (fb_gfx3_command_payload_size(command) != sizeof(*line))
				break;
			line = (const FB_GFX3_LINE_COMMAND *)command->payload;
			if (((line->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
			    (llabs((long long)line->x2 - line->x1) > 32767) ||
			    (llabs((long long)line->y2 - line->y1) > 32767))
				break;
			difference_x = (uint64_t)llabs(
				(long long)line->x2 - line->x1);
			difference_y = (uint64_t)llabs(
				(long long)line->y2 - line->y1);
			coverage = ((difference_x > difference_y) ?
				difference_x : difference_y) + 1u;
			added_count = 1u;
			added_workgroups = (coverage + 63u) / 64u;
		} else if (command->type == FB_GFX3_COMMAND_LINES) {
			const FB_GFX3_LINES_COMMAND *lines;
			size_t line_bytes;
			size_t expected_size;
			uint32_t line_index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_LINES_COMMAND, line))
				break;
			lines = (const FB_GFX3_LINES_COMMAND *)command->payload;
			if ((lines->count == 0u) ||
			    (fb_gfx3_size_multiply(lines->count,
			     sizeof(lines->line[0]), &line_bytes) != FB_GFX3_OK) ||
			    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line),
			     line_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (line_index = 0u; line_index < lines->count; line_index++) {
				const FB_GFX3_LINE_COMMAND *line =
					&lines->line[line_index];
				uint64_t difference_x;
				uint64_t difference_y;
				uint64_t coverage;
				uint64_t workgroups;

				if (((line->flags &
				      FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
				    (llabs((long long)line->x2 - line->x1) > 32767) ||
				    (llabs((long long)line->y2 - line->y1) > 32767))
					break;
				difference_x = (uint64_t)llabs(
					(long long)line->x2 - line->x1);
				difference_y = (uint64_t)llabs(
					(long long)line->y2 - line->y1);
				coverage = ((difference_x > difference_y) ?
					difference_x : difference_y) + 1u;
				workgroups = (coverage + 63u) / 64u;
				added_workgroups += workgroups;
			}
			if (line_index != lines->count)
				break;
			added_count = lines->count;
		} else if (command->type == FB_GFX3_COMMAND_RECTANGLE) {
			const FB_GFX3_RECTANGLE_COMMAND *rectangle;

			if (fb_gfx3_command_payload_size(command) !=
			    sizeof(*rectangle))
				break;
			rectangle =
				(const FB_GFX3_RECTANGLE_COMMAND *)command->payload;
			if (!vulkan_backend_rectangle_workgroups(rectangle,
			    &added_workgroups))
				break;
			added_count = 1u;
		} else if (command->type == FB_GFX3_COMMAND_RECTANGLES) {
			const FB_GFX3_RECTANGLES_COMMAND *rectangles;
			size_t rectangle_bytes;
			size_t expected_size;
			uint32_t rectangle_index;

			if (fb_gfx3_command_payload_size(command) <
			    offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle))
				break;
			rectangles =
				(const FB_GFX3_RECTANGLES_COMMAND *)command->payload;
			if ((rectangles->count == 0u) ||
			    (fb_gfx3_size_multiply(rectangles->count,
			     sizeof(rectangles->rectangle[0]), &rectangle_bytes) !=
			     FB_GFX3_OK) ||
			    (fb_gfx3_size_add(
			     offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle),
			     rectangle_bytes, &expected_size) != FB_GFX3_OK) ||
			    (expected_size != fb_gfx3_command_payload_size(command)))
				break;
			for (rectangle_index = 0u;
			     rectangle_index < rectangles->count;
			     rectangle_index++) {
				uint64_t workgroups;

				if (!vulkan_backend_rectangle_workgroups(
				    &rectangles->rectangle[rectangle_index],
				    &workgroups) ||
				    (workgroups > UINT64_MAX - added_workgroups))
					break;
				added_workgroups += workgroups;
			}
			if (rectangle_index != rectangles->count)
				break;
			added_count = rectangles->count;
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			const FB_GFX3_ELLIPSE_COMMAND *ellipse;

			if (fb_gfx3_command_payload_size(command) != sizeof(*ellipse))
				break;
			ellipse = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
			if (((ellipse->flags &
			      FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
			    !(ellipse->radius_x >= 0.0f) ||
			    !(ellipse->radius_x <= 32767.0f) ||
			    !(ellipse->radius_y >= 0.0f) ||
			    !(ellipse->radius_y <= 32767.0f))
				break;
			added_count = 1u;
			added_workgroups = 1u;
		} else {
			break;
		}
		if (added_count >
		    FB_GFX3_VULKAN_BACKEND_PRIMITIVE_BATCH_LIMIT - primitive_count)
			break;
		/*
			The workgroup table contains only useful 64-pixel chunks. Refuse a
			candidate which cannot fit in the portable Vulkan dispatch bound;
			the established type-specific path remains the exact fallback.
		*/
		if (added_workgroups >
		    FB_GFX3_VULKAN_BACKEND_PRIMITIVE_WORKGROUP_LIMIT -
		    useful_workgroups)
			break;
		primitive_count += added_count;
		useful_workgroups += added_workgroups;
		command_count++;
	}
	if (command_count < 2u)
		return 1u;
	*primitive_count_result = primitive_count;
	return command_count;
}

static void vulkan_backend_primitive_add_line(
	FB_GFX3_VULKAN_PRIMITIVE *primitive,
	const FB_GFX3_LINE_COMMAND *line, uint32_t order)
{
	memset(primitive, 0, sizeof(*primitive));
	primitive->geometry[0] = line->x1;
	primitive->geometry[1] = line->y1;
	primitive->geometry[2] = line->x2;
	primitive->geometry[3] = line->y2;
	primitive->clip = line->clip;
	primitive->parameters[0] = line->color;
	primitive->parameters[1] = FB_GFX3_VULKAN_PRIMITIVE_LINE;
	primitive->parameters[2] = line->style & 0xFFFFu;
	primitive->parameters[3] = order;
}

static void vulkan_backend_primitive_add_rectangle(
	FB_GFX3_VULKAN_PRIMITIVE *primitive,
	const FB_GFX3_RECTANGLE_COMMAND *rectangle, uint32_t order)
{
	memset(primitive, 0, sizeof(*primitive));
	primitive->geometry[0] = rectangle->x1;
	primitive->geometry[1] = rectangle->y1;
	primitive->geometry[2] = rectangle->x2;
	primitive->geometry[3] = rectangle->y2;
	primitive->clip = rectangle->clip;
	primitive->parameters[0] = rectangle->color;
	primitive->parameters[1] = FB_GFX3_VULKAN_PRIMITIVE_RECTANGLE;
	primitive->parameters[2] = (rectangle->style & 0xFFFFu) |
		((rectangle->filled != 0u) ? 0x80000000u : 0u);
	primitive->parameters[3] = order;
}

static int vulkan_backend_primitive_batch(
	FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t command_count,
	size_t primitive_count)
{
	FB_GFX3_VULKAN_PRIMITIVE *primitives;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface = NULL;
	size_t primitive_index = 0u;
	size_t command_index;
	size_t allocation_size;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count < 2u) ||
	    (primitive_count < 2u) ||
	    (primitive_count > FB_GFX3_VULKAN_BACKEND_PRIMITIVE_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_size_multiply(primitive_count, sizeof(*primitives),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (state->primitive_scratch_size < allocation_size) {
		void *replacement = realloc(state->primitive_scratch,
			allocation_size);

		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		state->primitive_scratch =
			(FB_GFX3_VULKAN_PRIMITIVE *)replacement;
		state->primitive_scratch_size = allocation_size;
	}
	primitives = state->primitive_scratch;
	memset(primitives, 0, allocation_size);
	for (command_index = 0u; command_index < command_count; command_index++) {
		FB_GFX3_COMMAND *command = commands[command_index];

		if ((command == NULL) ||
		    (command->target != commands[0]->target) ||
		    ((command_index > 0u) &&
		     (command->sequence <= commands[command_index - 1u]->sequence))) {
			result = FB_GFX3_INVALID;
			goto done;
		}
		if (command->type == FB_GFX3_COMMAND_POINTS) {
			const FB_GFX3_POINTS_COMMAND *points =
				(const FB_GFX3_POINTS_COMMAND *)command->payload;
			uint32_t point_index;

			for (point_index = 0u; point_index < points->count;
			     point_index++) {
				FB_GFX3_VULKAN_PRIMITIVE *primitive =
					&primitives[primitive_index];

				primitive->geometry[0] = points->point[point_index].x;
				primitive->geometry[1] = points->point[point_index].y;
				primitive->clip = points->clip;
				primitive->parameters[0] =
					points->point[point_index].color;
				primitive->parameters[1] =
					FB_GFX3_VULKAN_PRIMITIVE_POINT;
				primitive->parameters[3] =
					(uint32_t)primitive_index + 1u;
				primitive_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			const FB_GFX3_LINE_COMMAND *line =
				(const FB_GFX3_LINE_COMMAND *)command->payload;

			vulkan_backend_primitive_add_line(
				&primitives[primitive_index], line,
				(uint32_t)primitive_index + 1u);
			primitive_index++;
		} else if (command->type == FB_GFX3_COMMAND_LINES) {
			const FB_GFX3_LINES_COMMAND *lines =
				(const FB_GFX3_LINES_COMMAND *)command->payload;
			uint32_t line_index;

			for (line_index = 0u; line_index < lines->count; line_index++) {
				vulkan_backend_primitive_add_line(
					&primitives[primitive_index],
					&lines->line[line_index],
					(uint32_t)primitive_index + 1u);
				primitive_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_RECTANGLE) {
			const FB_GFX3_RECTANGLE_COMMAND *rectangle =
				(const FB_GFX3_RECTANGLE_COMMAND *)command->payload;

			vulkan_backend_primitive_add_rectangle(
				&primitives[primitive_index], rectangle,
				(uint32_t)primitive_index + 1u);
			primitive_index++;
		} else if (command->type == FB_GFX3_COMMAND_RECTANGLES) {
			const FB_GFX3_RECTANGLES_COMMAND *rectangles =
				(const FB_GFX3_RECTANGLES_COMMAND *)command->payload;
			uint32_t rectangle_index;

			for (rectangle_index = 0u;
			     rectangle_index < rectangles->count;
			     rectangle_index++) {
				vulkan_backend_primitive_add_rectangle(
					&primitives[primitive_index],
					&rectangles->rectangle[rectangle_index],
					(uint32_t)primitive_index + 1u);
				primitive_index++;
			}
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			const FB_GFX3_ELLIPSE_COMMAND *ellipse =
				(const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
			FB_GFX3_VULKAN_PRIMITIVE *primitive =
				&primitives[primitive_index];

			primitive->geometry[0] = ellipse->center_x;
			primitive->geometry[1] = ellipse->center_y;
			memcpy(&primitive->geometry[2], &ellipse->radius_x,
				sizeof(ellipse->radius_x));
			memcpy(&primitive->geometry[3], &ellipse->radius_y,
				sizeof(ellipse->radius_y));
			primitive->clip = ellipse->clip;
			primitive->parameters[0] = ellipse->color;
			primitive->parameters[1] =
				FB_GFX3_VULKAN_PRIMITIVE_ELLIPSE;
			primitive->parameters[2] = ellipse->filled != 0u;
			primitive->parameters[3] =
				(uint32_t)primitive_index + 1u;
			primitive_index++;
		} else {
			result = FB_GFX3_INVALID;
			goto done;
		}
	}
	if (primitive_index != primitive_count) {
		result = FB_GFX3_INVALID;
		goto done;
	}
	result = vulkan_backend_surface_retain_handle(state, commands[0]->target,
		commands[command_count - 1u]->sequence, &surface);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_primitive_batch(&state->runtime,
			&surface->surface, primitives, primitive_count);
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	}

done:
	return result;
}

static size_t vulkan_backend_glyph_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	const FB_GFX3_GLYPHS_COMMAND *first;
	size_t glyph_bytes;
	size_t expected_size;
	uint32_t total;
	size_t count = 1u;

	if ((commands == NULL) || (available == 0u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_GLYPHS) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
		return 1u;
	first = (const FB_GFX3_GLYPHS_COMMAND *)commands[0]->payload;
	if ((first->count == 0u) ||
	    (first->count > FB_GFX3_VULKAN_BACKEND_GLYPH_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(first->count, sizeof(first->glyph[0]),
	     &glyph_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
	     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(commands[0])))
		return 1u;
	total = first->count;
	while ((count < available) &&
	       (total < FB_GFX3_VULKAN_BACKEND_GLYPH_BATCH_LIMIT)) {
		const FB_GFX3_GLYPHS_COMMAND *candidate;
		FB_GFX3_COMMAND *command = commands[count];

		if ((command == NULL) ||
		    (command->type != FB_GFX3_COMMAND_GLYPHS) ||
		    (command->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(command) <
		     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
			break;
		candidate = (const FB_GFX3_GLYPHS_COMMAND *)command->payload;
		if ((candidate->count == 0u) ||
		    (candidate->count >
		     FB_GFX3_VULKAN_BACKEND_GLYPH_BATCH_LIMIT - total) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0) ||
		    (fb_gfx3_size_multiply(candidate->count,
		     sizeof(candidate->glyph[0]), &glyph_bytes) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
		     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(command)))
			break;
		total += candidate->count;
		count++;
	}
	return count;
}

static int vulkan_backend_glyphs(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t command_count)
{
	const FB_GFX3_GLYPHS_COMMAND *first;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface = NULL;
	FB_GFX3_GLYPH *combined = NULL;
	const FB_GFX3_GLYPH *glyphs;
	uint32_t total = 0u;
	uint32_t destination_index = 0u;
	size_t glyph_bytes;
	size_t expected_size;
	size_t command_index;
	int result;

	if ((state == NULL) || (commands == NULL) || (command_count == 0u) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) <
	     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_GLYPHS_COMMAND *)commands[0]->payload;
	for (command_index = 0u; command_index < command_count; ++command_index) {
		const FB_GFX3_GLYPHS_COMMAND *payload;
		uint32_t item;

		if ((commands[command_index] == NULL) ||
		    (commands[command_index]->type != FB_GFX3_COMMAND_GLYPHS) ||
		    (commands[command_index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[command_index]) <
		     offsetof(FB_GFX3_GLYPHS_COMMAND, glyph)))
			return FB_GFX3_INVALID;
		payload = (const FB_GFX3_GLYPHS_COMMAND *)
			commands[command_index]->payload;
		if ((payload->count == 0u) ||
		    (payload->count >
		     FB_GFX3_VULKAN_BACKEND_GLYPH_BATCH_LIMIT - total) ||
		    (memcmp(&payload->clip, &first->clip,
		     sizeof(payload->clip)) != 0) ||
		    (fb_gfx3_size_multiply(payload->count,
		     sizeof(payload->glyph[0]), &glyph_bytes) != FB_GFX3_OK) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_GLYPHS_COMMAND, glyph),
		     glyph_bytes, &expected_size) != FB_GFX3_OK) ||
		    (expected_size !=
		     fb_gfx3_command_payload_size(commands[command_index])))
			return FB_GFX3_INVALID;
		for (item = 0u; item < payload->count; ++item) {
			const FB_GFX3_GLYPH *glyph = &payload->glyph[item];

			if ((glyph->width == 0u) || (glyph->width > 8u) ||
			    (glyph->height == 0u) || (glyph->height > 16u) ||
		    ((glyph->flags &
		      ~(uint32_t)FB_GFX3_GLYPH_BACKGROUND) != 0u))
				return FB_GFX3_INVALID;
		}
		total += payload->count;
	}
	if ((total == 0u) ||
	    (fb_gfx3_size_multiply(total, sizeof(FB_GFX3_GLYPH),
	     &glyph_bytes) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (command_count == 1u) {
		glyphs = first->glyph;
	} else {
		combined = (FB_GFX3_GLYPH *)malloc(glyph_bytes);
		if (combined == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		for (command_index = 0u; command_index < command_count;
		     ++command_index) {
			const FB_GFX3_GLYPHS_COMMAND *payload =
				(const FB_GFX3_GLYPHS_COMMAND *)
				commands[command_index]->payload;

			memcpy(combined + destination_index, payload->glyph,
				(size_t)payload->count * sizeof(payload->glyph[0]));
			destination_index += payload->count;
		}
		glyphs = combined;
	}
	result = vulkan_backend_surface_retain_handle(state, commands[0]->target,
		commands[command_count - 1u]->sequence, &surface);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_glyphs(&state->runtime,
			&surface->surface, &first->clip, glyphs, total);
		fb_gfx3_resource_release(state->resources, commands[0]->target);
	}
	free(combined);
	return result;
}

static int vulkan_backend_line(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINE_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINE_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_line(&state->runtime,
		&surface->surface, &payload->clip, payload->x1, payload->y1,
		payload->x2, payload->y2, payload->color, payload->style,
		payload->flags);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_line_batch(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	FB_GFX3_VULKAN_LINE lines[FB_GFX3_VULKAN_BACKEND_LINE_BATCH_LIMIT];
	FB_GFX3_HANDLE target;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2) ||
	    (count > (sizeof(lines) / sizeof(lines[0]))) ||
	    (commands[count - 1u] == NULL))
		return FB_GFX3_INVALID;
	target = commands[count - 1u]->target;
	result = vulkan_backend_surface_retain(state, commands[count - 1], &surface);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < count; index++) {
		const FB_GFX3_LINE_COMMAND *payload;

		if ((commands[index] == NULL) ||
			(commands[index]->type != FB_GFX3_COMMAND_LINE) ||
			(commands[index]->target != target) ||
			(fb_gfx3_command_payload_size(commands[index]) !=
			 sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto cleanup;
		}
		payload = (const FB_GFX3_LINE_COMMAND *)commands[index]->payload;
		lines[index].clip = payload->clip;
		lines[index].x1 = payload->x1;
		lines[index].y1 = payload->y1;
		lines[index].x2 = payload->x2;
		lines[index].y2 = payload->y2;
		lines[index].color = payload->color;
		lines[index].style = payload->style;
		lines[index].flags = payload->flags;
	}
	result = fb_gfx3_vulkan_surface_line_batch(&state->runtime,
		&surface->surface, lines, count);

cleanup:
	fb_gfx3_resource_release(state->resources, target);
	return result;
}

static int vulkan_backend_lines(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_LINES_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	FB_GFX3_VULKAN_LINE lines[FB_GFX3_VULKAN_BACKEND_LINE_BATCH_LIMIT];
	size_t expected_size;
	size_t line_bytes;
	size_t source_index = 0u;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_LINES_COMMAND, line)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_LINES_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->line[0]),
	     &line_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_LINES_COMMAND, line), line_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = vulkan_backend_surface_retain_handle(state, command->target,
		command->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	while (source_index < payload->count) {
		size_t count = payload->count - source_index;
		size_t index;

		if (count > FB_GFX3_VULKAN_BACKEND_LINE_BATCH_LIMIT)
			count = FB_GFX3_VULKAN_BACKEND_LINE_BATCH_LIMIT;
		for (index = 0u; index < count; ++index) {
			const FB_GFX3_LINE_COMMAND *source =
				&payload->line[source_index + index];

			lines[index].clip = source->clip;
			lines[index].x1 = source->x1;
			lines[index].y1 = source->y1;
			lines[index].x2 = source->x2;
			lines[index].y2 = source->y2;
			lines[index].color = source->color;
			lines[index].style = source->style;
			lines[index].flags = source->flags;
		}
		if (count == 1u) {
			result = fb_gfx3_vulkan_surface_line(&state->runtime,
				&surface->surface, &lines[0].clip, lines[0].x1,
				lines[0].y1, lines[0].x2, lines[0].y2,
				lines[0].color, lines[0].style, lines[0].flags);
		} else {
			result = fb_gfx3_vulkan_surface_line_batch(&state->runtime,
				&surface->surface, lines, count);
		}
		if (result != FB_GFX3_OK)
			break;
		source_index += count;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_rectangle(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLE_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLE_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_rectangle(&state->runtime,
		&surface->surface, &payload->clip, payload->x1, payload->y1,
		payload->x2, payload->y2, payload->color, payload->style,
		payload->filled != 0, payload->flags);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_ellipse(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_ELLIPSE_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_ELLIPSE_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_ellipse(&state->runtime,
		&surface->surface, &payload->clip, payload->center_x,
		payload->center_y, payload->radius_x, payload->radius_y,
		payload->color, payload->filled != 0, payload->flags);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static size_t vulkan_backend_ellipse_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	size_t count = 1u;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_ELLIPSE) ||
	    (fb_gfx3_command_payload_size(commands[0]) !=
	     sizeof(FB_GFX3_ELLIPSE_COMMAND)))
		return 1u;
	while ((count < available) &&
	       (count < FB_GFX3_VULKAN_BACKEND_ELLIPSE_BATCH_LIMIT)) {
		FB_GFX3_COMMAND *candidate = commands[count];

		if ((candidate == NULL) ||
		    (candidate->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (candidate->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(candidate) !=
		     sizeof(FB_GFX3_ELLIPSE_COMMAND)))
			break;
		count++;
	}
	return count;
}

static int vulkan_backend_ellipse_batch(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	FB_GFX3_VULKAN_ELLIPSE ellipses[
		FB_GFX3_VULKAN_BACKEND_ELLIPSE_BATCH_LIMIT];
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface = NULL;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_VULKAN_BACKEND_ELLIPSE_BATCH_LIMIT) ||
	    (commands[0] == NULL))
		return FB_GFX3_INVALID;
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_ELLIPSE_COMMAND *payload;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_ELLIPSE) ||
		    (commands[index]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload)))
			return FB_GFX3_INVALID;
		payload = (const FB_GFX3_ELLIPSE_COMMAND *)commands[index]->payload;
		if (!(payload->radius_x >= 0.0f) ||
		    !(payload->radius_x <= 32767.0f) ||
		    !(payload->radius_y >= 0.0f) ||
		    !(payload->radius_y <= 32767.0f))
			return FB_GFX3_INVALID;
		ellipses[index].clip = payload->clip;
		ellipses[index].center_x = payload->center_x;
		ellipses[index].center_y = payload->center_y;
		ellipses[index].radius_x = payload->radius_x;
		ellipses[index].radius_y = payload->radius_y;
		ellipses[index].color = payload->color;
		ellipses[index].filled = payload->filled != 0u;
		ellipses[index].flags = payload->flags;
	}
	result = vulkan_backend_surface_retain_handle(state, commands[0]->target,
		commands[count - 1u]->sequence, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_ellipse_batch(&state->runtime,
		&surface->surface, ellipses, count);
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

static int vulkan_backend_paint(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAINT_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAINT_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vulkan_surface_paint(&state->runtime,
		&surface->surface, &payload->clip, payload->x, payload->y,
		payload->color, payload->border_color, payload->flags, payload);
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_blit(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLIT_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *destination;
	FB_GFX3_VULKAN_BACKEND_SURFACE *source;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLIT_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_backend_surface_retain_handle(state, payload->source,
		command->sequence, &source);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_blit(&state->runtime,
			&destination->surface, &source->surface, &payload->clip,
			&payload->source_rect, payload->destination_x,
			payload->destination_y, payload->mode, payload->alpha);
		fb_gfx3_resource_release(state->resources, payload->source);
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_transform_blit(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *destination;
	FB_GFX3_VULKAN_BACKEND_SURFACE *source;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &destination);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_backend_surface_retain_handle(state, payload->source,
		command->sequence, &source);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_transform_blit(&state->runtime,
			&destination->surface, &source->surface, payload);
		fb_gfx3_resource_release(state->resources, payload->source);
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_transform_blit_batch(
	FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *first;
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *transforms[
		FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT];
	FB_GFX3_VULKAN_BACKEND_SURFACE *destination;
	FB_GFX3_VULKAN_BACKEND_SURFACE *source;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2u) ||
	    (count > FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT) ||
	    (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return FB_GFX3_INVALID;
	first = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)commands[0]->payload;
	if (first->source == commands[0]->target)
		return FB_GFX3_UNSUPPORTED;
	for (index = 0u; index < count; ++index) {
		const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_TRANSFORM_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload)))
			return FB_GFX3_INVALID;
		payload = (const FB_GFX3_TRANSFORM_BLIT_COMMAND *)
			commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != first->source) ||
		    ((index > 0u) && (commands[index]->sequence <=
		     commands[index - 1u]->sequence)))
			return FB_GFX3_INVALID;
		transforms[index] = payload;
	}
	result = vulkan_backend_surface_retain(state, commands[count - 1u],
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_backend_surface_retain_handle(state, first->source,
		commands[count - 1u]->sequence, &source);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_transform_blit_batch(
			&state->runtime, &destination->surface, &source->surface,
			transforms, count);
		fb_gfx3_resource_release(state->resources, first->source);
	}
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

/*
	A BLITS packet retains a complete FIFO sprite stream. Vulkan descriptors bind
	one source image for a batch, so execute adjacent records with the same source
	together while preserving their public order.
*/
static int vulkan_backend_blits(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_BLITS_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *destination;
	FB_GFX3_VULKAN_BLIT blits[FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT];
	size_t blit_bytes;
	size_t expected_size;
	uint32_t run_start;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_BLITS_COMMAND, blit)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_BLITS_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->blit[0]),
	     &blit_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit), blit_bytes,
	     &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)) ||
	    (payload->source != payload->blit[0].source) ||
	    (payload->mode != payload->blit[0].mode) ||
	    (payload->alpha != payload->blit[0].alpha))
		return FB_GFX3_INVALID;
	result = vulkan_backend_surface_retain(state, command, &destination);
	if (result != FB_GFX3_OK)
		return result;
	run_start = 0u;
	while (run_start < payload->count) {
		FB_GFX3_VULKAN_BACKEND_SURFACE *source;
		FB_GFX3_HANDLE source_handle = payload->blit[run_start].source;
		uint32_t run_end = run_start + 1u;
		uint32_t index;
		uint32_t run_count;

		if ((source_handle == 0) || (source_handle == command->target)) {
			result = FB_GFX3_INVALID;
			break;
		}
		while ((run_end < payload->count) &&
		       (payload->blit[run_end].source == source_handle))
			run_end++;
		run_count = run_end - run_start;
		result = vulkan_backend_surface_retain_handle(state, source_handle,
			command->sequence, &source);
		if (result != FB_GFX3_OK)
			break;
		if (source->surface.depth != destination->surface.depth) {
			result = FB_GFX3_INVALID;
			fb_gfx3_resource_release(state->resources, source_handle);
			break;
		}
		for (index = 0u; index < run_count; index++) {
			const FB_GFX3_BLIT_COMMAND *blit =
				&payload->blit[run_start + index];

			blits[index].clip = blit->clip;
			blits[index].source_rect = blit->source_rect;
			blits[index].destination_x = blit->destination_x;
			blits[index].destination_y = blit->destination_y;
			blits[index].mode = blit->mode;
			blits[index].alpha = blit->alpha;
		}
		if (run_count == 1u)
			result = fb_gfx3_vulkan_surface_blit(&state->runtime,
				&destination->surface, &source->surface, &blits[0].clip,
				&blits[0].source_rect, blits[0].destination_x,
				blits[0].destination_y, blits[0].mode, blits[0].alpha);
		else
			result = fb_gfx3_vulkan_surface_blit_batch(&state->runtime,
				&destination->surface, &source->surface, blits, run_count);
		fb_gfx3_resource_release(state->resources, source_handle);
		if (result != FB_GFX3_OK)
			break;
		run_start = run_end;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

/*
	The renderer preserves FIFO order inside each execution batch. Adjacent PUT
	commands with one source and destination can therefore share a Vulkan
	submission. The runtime still records one dispatch and dependency per PUT,
	which retains overlapping-sprite and blend ordering without a CPU fallback.
*/
static int vulkan_backend_blit_batch(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t count)
{
	const FB_GFX3_BLIT_COMMAND *first_payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *destination;
	FB_GFX3_VULKAN_BACKEND_SURFACE *source;
	FB_GFX3_VULKAN_BLIT blits[FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT];
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2) ||
	    (count > (sizeof(blits) / sizeof(blits[0]))))
		return FB_GFX3_INVALID;
	if (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first_payload))
		return FB_GFX3_INVALID;
	first_payload = (const FB_GFX3_BLIT_COMMAND *)commands[0]->payload;
	if (first_payload->source == commands[0]->target)
		return FB_GFX3_UNSUPPORTED;
	for (index = 0; index < count; index++) {
		const FB_GFX3_BLIT_COMMAND *payload;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_BLIT) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload)))
			return FB_GFX3_INVALID;
		payload = (const FB_GFX3_BLIT_COMMAND *)commands[index]->payload;
		if ((commands[index]->target != commands[0]->target) ||
		    (payload->source != first_payload->source))
			return FB_GFX3_INVALID;
		blits[index].clip = payload->clip;
		blits[index].source_rect = payload->source_rect;
		blits[index].destination_x = payload->destination_x;
		blits[index].destination_y = payload->destination_y;
		blits[index].mode = payload->mode;
		blits[index].alpha = payload->alpha;
	}
	/* The highest sequence is the only lifetime fence the shared resources need. */
	result = vulkan_backend_surface_retain(state, commands[count - 1],
		&destination);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_backend_surface_retain_handle(state, first_payload->source,
		commands[count - 1]->sequence, &source);
	if (result == FB_GFX3_OK) {
		result = fb_gfx3_vulkan_surface_blit_batch(&state->runtime,
			&destination->surface, &source->surface, blits, count);
		fb_gfx3_resource_release(state->resources, first_payload->source);
	}
	fb_gfx3_resource_release(state->resources, commands[0]->target);
	return result;
}

/*
	LINE ... , BF is an opaque transfer fill in the Vulkan runtime.  Keep an
	adjacent run on one target in one command buffer.  Clipping is performed
	here so the runtime's fill records only valid buffer offsets.
*/
static int vulkan_backend_rectangle_clear_batch(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *const *commands,
	size_t count)
{
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	FB_GFX3_VULKAN_CLEAR_RECTANGLE rectangles[
		FB_GFX3_VULKAN_BACKEND_RECTANGLE_BATCH_LIMIT];
	FB_GFX3_HANDLE target;
	size_t index;
	int result;

	if ((state == NULL) || (commands == NULL) || (count < 2) ||
	    (count > (sizeof(rectangles) / sizeof(rectangles[0]))) ||
	    (commands[count - 1u] == NULL))
		return FB_GFX3_INVALID;
	target = commands[count - 1u]->target;
	result = vulkan_backend_surface_retain(state, commands[count - 1],
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < count; index++) {
		const FB_GFX3_RECTANGLE_COMMAND *payload;
		FB_GFX3_RECT clipped;

		if ((commands[index] == NULL) ||
		    (commands[index]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (commands[index]->target != target) ||
		    (fb_gfx3_command_payload_size(commands[index]) !=
		     sizeof(*payload))) {
			result = FB_GFX3_INVALID;
			goto cleanup;
		}
		payload = (const FB_GFX3_RECTANGLE_COMMAND *)commands[index]->payload;
		if ((payload->filled == 0) ||
		    ((payload->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)) {
			result = FB_GFX3_INVALID;
			goto cleanup;
		}
		clipped = payload->clip;
		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)surface->surface.width)
			clipped.x2 = (int32_t)surface->surface.width - 1;
		if (clipped.y2 >= (int32_t)surface->surface.height)
			clipped.y2 = (int32_t)surface->surface.height - 1;
		rectangles[index].x1 = clipped.x1;
		rectangles[index].y1 = clipped.y1;
		rectangles[index].x2 = clipped.x2;
		rectangles[index].y2 = clipped.y2;
		if (payload->x1 > rectangles[index].x1)
			rectangles[index].x1 = payload->x1;
		if (payload->y1 > rectangles[index].y1)
			rectangles[index].y1 = payload->y1;
		if (payload->x2 < rectangles[index].x2)
			rectangles[index].x2 = payload->x2;
		if (payload->y2 < rectangles[index].y2)
			rectangles[index].y2 = payload->y2;
		rectangles[index].color = payload->color;
	}
	/*
		Uniform cached CPU images become same-colour rectangles. Their overlap
		order cannot alter the final pixels, so prefer the two-dimensional
		compute batch and leave the transfer fill path for mixed colours.
	*/
	result = fb_gfx3_vulkan_surface_opaque_rectangle_batch(&state->runtime,
		&surface->surface, rectangles, count);
	if (result == FB_GFX3_UNSUPPORTED)
		result = fb_gfx3_vulkan_surface_clear_batch(&state->runtime,
			&surface->surface, rectangles, count);

cleanup:
	fb_gfx3_resource_release(state->resources, target);
	return result;
}

/*
	The producer collects consecutive opaque rectangles in one bounded command.
	The Vulkan tile shader accepts filled and outline boxes in their original
	order. Exact clip and style decisions stay on the GPU; host work is limited
	to validating the packet and preparing fixed-size command records.
*/
static int vulkan_backend_rectangles(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_RECTANGLES_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	FB_GFX3_VULKAN_RECTANGLE rectangles[
		FB_GFX3_VULKAN_BACKEND_RECTANGLE_BATCH_LIMIT];
	size_t rectangle_bytes;
	size_t expected_size;
	size_t index;
	int result;

	if ((state == NULL) || (command == NULL) ||
	    (fb_gfx3_command_payload_size(command) <
	     offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_RECTANGLES_COMMAND *)command->payload;
	if ((payload->count == 0u) ||
	    (payload->count > FB_GFX3_VULKAN_BACKEND_RECTANGLE_BATCH_LIMIT) ||
	    (fb_gfx3_size_multiply(payload->count, sizeof(payload->rectangle[0]),
	     &rectangle_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(offsetof(FB_GFX3_RECTANGLES_COMMAND, rectangle),
	     rectangle_bytes, &expected_size) != FB_GFX3_OK) ||
	    (expected_size != fb_gfx3_command_payload_size(command)))
		return FB_GFX3_INVALID;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < payload->count; ++index) {
		const FB_GFX3_RECTANGLE_COMMAND *rectangle = &payload->rectangle[index];

		if (((rectangle->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    (rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2)) {
			result = FB_GFX3_INVALID;
			goto cleanup;
		}
		rectangles[index].clip = rectangle->clip;
		rectangles[index].x1 = rectangle->x1;
		rectangles[index].y1 = rectangle->y1;
		rectangles[index].x2 = rectangle->x2;
		rectangles[index].y2 = rectangle->y2;
		rectangles[index].color = rectangle->color;
		rectangles[index].style = rectangle->style;
		rectangles[index].filled = rectangle->filled != 0u;
		rectangles[index].flags = rectangle->flags;
	}
	result = fb_gfx3_vulkan_surface_rectangle_batch(&state->runtime,
		&surface->surface, rectangles, payload->count);

cleanup:
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_read_pixel(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command)
{
	const FB_GFX3_READ_PIXEL_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	uint32_t color = UINT32_MAX;
	int result;

	if ((command->completion == NULL) ||
	    (fb_gfx3_command_payload_size(command) != sizeof(*payload)))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_READ_PIXEL_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->x >= 0) && (payload->y >= 0) &&
	    (payload->x < (int32_t)surface->surface.width) &&
	    (payload->y < (int32_t)surface->surface.height)) {
		color = 0;
		result = fb_gfx3_vulkan_surface_download(&state->runtime,
			&surface->surface, payload->x, payload->y, 1, 1,
			&color, sizeof(color));
	}
	fb_gfx3_resource_release(state->resources, command->target);
	if (result != FB_GFX3_OK)
		return result;
	return fb_gfx3_completion_set_value(command->completion, 0, color);
}

static int vulkan_backend_page_set(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PAGE_SET_COMMAND *payload;
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	int result;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PAGE_SET_COMMAND *)command->payload;
	result = vulkan_backend_surface_retain(state, command, &surface);
	if (result != FB_GFX3_OK)
		return result;
	if ((payload->width != surface->surface.width) ||
	    (payload->height != surface->surface.height) ||
	    (payload->depth != surface->surface.depth)) {
		result = FB_GFX3_INVALID;
	} else {
		state->visible_surface = command->target;
		state->presentation_dirty = TRUE;
	}
	fb_gfx3_resource_release(state->resources, command->target);
	return result;
}

static int vulkan_backend_palette(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_PALETTE_COMMAND *payload;

	if (fb_gfx3_command_payload_size(command) != sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_PALETTE_COMMAND *)command->payload;
	memcpy(state->palette, payload->color, sizeof(state->palette));
	if (state->visible_surface != 0)
		state->presentation_dirty = TRUE;
	return FB_GFX3_OK;
}

static void vulkan_backend_read_keyboard_overlay(
	FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY *overlay, uint32_t *overlay_state)
{
	memset(overlay, 0, sizeof(*overlay));
	*overlay_state = 0;
	if ((fb_gfx3_platform_keyboard_overlay(state->platform, overlay) !=
	     FB_GFX3_OK) || !overlay->visible)
		return;
	*overlay_state = overlay->pressed ? 3u :
		(overlay->keyboard_visible ? 2u : 1u);
}

static int vulkan_backend_present_handle(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_HANDLE handle,
	uint64_t sequence)
{
	FB_GFX3_VULKAN_BACKEND_SURFACE *surface;
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY keyboard_overlay;
	int32_t keyboard_button_rect[4];
	uint32_t keyboard_state;
	uint64_t presentation_count;
	int result;

	result = vulkan_backend_surface_retain_handle(state, handle, sequence,
		&surface);
	if (result != FB_GFX3_OK)
		return result;
	vulkan_backend_read_keyboard_overlay(state, &keyboard_overlay,
		&keyboard_state);
	keyboard_button_rect[0] = keyboard_overlay.x0;
	keyboard_button_rect[1] = keyboard_overlay.y0;
	keyboard_button_rect[2] = keyboard_overlay.x1;
	keyboard_button_rect[3] = keyboard_overlay.y1;
	state->keyboard_overlay = keyboard_overlay;
	state->keyboard_overlay_state = keyboard_state;
	state->keyboard_overlay_known = TRUE;
	presentation_count = state->runtime.completed_submission_count;
	result = fb_gfx3_vulkan_surface_present(&state->runtime,
		&surface->surface, state->palette,
		sizeof(state->palette) / sizeof(state->palette[0]),
		keyboard_button_rect, keyboard_state);
	/*
		The presentation marker is the final GPU use of the visible surface for
		this frame. Attach the renderer sequence to that slot so resource
		retirement cannot run after primitive completion but before the queued
		presentation conversion and copy have finished.
	*/
	if ((result == FB_GFX3_OK) &&
	    (state->runtime.completed_submission_count != presentation_count))
		result = fb_gfx3_vulkan_runtime_tag_submission(&state->runtime,
			sequence);
	if (result == FB_GFX3_OK)
		result = state->platform_vtable->show_window(state->platform);
	if (result == FB_GFX3_OK)
		state->presentation_dirty = FALSE;
	fb_gfx3_resource_release(state->resources, handle);
	state->platform_vtable->pump_events(state->platform);
	return result;
}

static int vulkan_backend_present(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	if (fb_gfx3_command_payload_size(command) != 0)
		return FB_GFX3_INVALID;
	return vulkan_backend_present_handle(state, command->target,
		command->sequence);
}

static int vulkan_backend_window_title(FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *command)
{
	const FB_GFX3_WINDOW_TITLE_COMMAND *payload;
	size_t expected_size;
	size_t payload_size = fb_gfx3_command_payload_size(command);

	if (payload_size < sizeof(*payload))
		return FB_GFX3_INVALID;
	payload = (const FB_GFX3_WINDOW_TITLE_COMMAND *)command->payload;
	if ((fb_gfx3_size_add(sizeof(*payload), payload->length,
	     &expected_size) != FB_GFX3_OK) ||
	    (fb_gfx3_size_add(expected_size, 1, &expected_size) != FB_GFX3_OK) ||
	    (payload_size != expected_size) ||
	    (payload->title[payload->length] != '\0'))
		return FB_GFX3_INVALID;
	return state->platform_vtable->set_window_title(state->platform,
		payload->title);
}

static int vulkan_backend_poll_platform(
	FB_GFX3_VULKAN_BACKEND_STATE *state)
{
	FB_GFX3_ANDROID_KEYBOARD_OVERLAY keyboard_overlay;
	uint32_t width;
	uint32_t height;
	uint32_t keyboard_state;
	int result;

	state->platform_vtable->pump_events(state->platform);
	/*
		Do not block the render thread merely to reclaim a completed transient
		buffer. The Vulkan runtime checks only already-signaled slot fences;
		sequence completion and surface retirement remain ordered separately.
	*/
	result = fb_gfx3_vulkan_runtime_poll(&state->runtime);
	if (result != FB_GFX3_OK)
		return result;
	if (state->platform_vtable->client_size(state->platform, &width,
	    &height) != FB_GFX3_OK)
		return FB_GFX3_OK;
	if ((width != state->client_width) || (height != state->client_height)) {
		result = fb_gfx3_vulkan_runtime_resize(&state->runtime, width,
			height);
		if (result != FB_GFX3_OK)
			return result;
		state->client_width = width;
		state->client_height = height;
		if ((width != 0) && (height != 0) &&
		    (state->visible_surface != 0))
			state->presentation_dirty = TRUE;
	}
	vulkan_backend_read_keyboard_overlay(state, &keyboard_overlay,
		&keyboard_state);
	if (!state->keyboard_overlay_known ||
	    (keyboard_state != state->keyboard_overlay_state) ||
	    (memcmp(&keyboard_overlay, &state->keyboard_overlay,
	     sizeof(keyboard_overlay)) != 0)) {
		state->keyboard_overlay = keyboard_overlay;
		state->keyboard_overlay_state = keyboard_state;
		state->keyboard_overlay_known = TRUE;
		if (state->visible_surface != 0)
			state->presentation_dirty = TRUE;
	}
	return FB_GFX3_OK;
}

static int vulkan_backend_execute_one(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_COMMAND *command)
{
	int result;

	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_CREATE:
		return vulkan_backend_surface_create(state, command);
	case FB_GFX3_COMMAND_SURFACE_DESTROY:
		if (fb_gfx3_command_payload_size(command) != 0)
			return FB_GFX3_INVALID;
		if (command->target == state->visible_surface) {
			state->visible_surface = 0;
			state->presentation_dirty = FALSE;
		}
		result = fb_gfx3_resource_mark_used(state->resources,
			command->target, command->sequence);
		if (result != FB_GFX3_OK)
			return result;
		return fb_gfx3_resource_release(state->resources,
			command->target);
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
		return vulkan_backend_surface_upload(state, command);
	case FB_GFX3_COMMAND_SURFACE_DOWNLOAD:
		return vulkan_backend_surface_download(state, command);
	case FB_GFX3_COMMAND_CLEAR:
		return vulkan_backend_clear(state, command);
	case FB_GFX3_COMMAND_POINTS:
		return vulkan_backend_points(state, command);
	case FB_GFX3_COMMAND_GLYPHS:
		return vulkan_backend_glyphs(state, &command, 1u);
	case FB_GFX3_COMMAND_LINE:
		return vulkan_backend_line(state, command);
	case FB_GFX3_COMMAND_LINES:
		return vulkan_backend_lines(state, command);
	case FB_GFX3_COMMAND_RECTANGLE:
		return vulkan_backend_rectangle(state, command);
	case FB_GFX3_COMMAND_RECTANGLES:
		return vulkan_backend_rectangles(state, command);
	case FB_GFX3_COMMAND_ELLIPSE:
		return vulkan_backend_ellipse(state, command);
	case FB_GFX3_COMMAND_PAINT:
		return vulkan_backend_paint(state, command);
	case FB_GFX3_COMMAND_BLIT:
		return vulkan_backend_blit(state, command);
	case FB_GFX3_COMMAND_BLITS:
		return vulkan_backend_blits(state, command);
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
		return vulkan_backend_transform_blit(state, command);
	case FB_GFX3_COMMAND_READ_PIXEL:
		return vulkan_backend_read_pixel(state, command);
	case FB_GFX3_COMMAND_PALETTE:
		return vulkan_backend_palette(state, command);
	case FB_GFX3_COMMAND_PAGE_SET:
		return vulkan_backend_page_set(state, command);
	case FB_GFX3_COMMAND_PRESENT:
		return vulkan_backend_present(state, command);
	case FB_GFX3_COMMAND_WINDOW_TITLE:
		return vulkan_backend_window_title(state, command);
	case FB_GFX3_COMMAND_BARRIER:
		return (fb_gfx3_command_payload_size(command) == 0) ?
			FB_GFX3_OK : FB_GFX3_INVALID;
	case FB_GFX3_COMMAND_PLATFORM_POLL:
		if (fb_gfx3_command_payload_size(command) != 0)
			return FB_GFX3_INVALID;
		return vulkan_backend_poll_platform(state);
	default:
		return FB_GFX3_UNSUPPORTED;
	}
}

/* ------------------------------------------------------------------------- */
/* Backend interface                                                         */
/* ------------------------------------------------------------------------- */

static int vulkan_backend_probe(FB_GFX3_BACKEND_CAPS *caps)
{
	FB_GFX3_VULKAN_RUNTIME runtime;
	int result;

	if (caps == NULL)
		return FB_GFX3_INVALID;
	memset(&runtime, 0, sizeof(runtime));
	result = fb_gfx3_vulkan_runtime_open(&runtime);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_vulkan_runtime_close(&runtime);
	memset(caps, 0, sizeof(*caps));
	caps->abi_version = FB_GFX3_BACKEND_ABI_VERSION;
	caps->features = FB_GFX3_FEATURE_INDEXED_SURFACES |
		FB_GFX3_FEATURE_COMPUTE | FB_GFX3_FEATURE_PACKED_BLITS |
		FB_GFX3_FEATURE_HETEROGENEOUS_BLITS |
		FB_GFX3_FEATURE_PACKED_RECTANGLES |
		FB_GFX3_FEATURE_PACKED_STYLED_RECTANGLES |
		FB_GFX3_FEATURE_PACKED_LINES;
	/*
		Surfaces use linear storage buffers. Advertise the same practical
		per-axis compatibility bound as the desktop OpenGL backend; allocation
		still validates width times height against the selected adapter's exact
		maxStorageBufferRange.
	*/
	caps->max_surface_width = 16384;
	caps->max_surface_height = 16384;
	caps->max_batch_commands = 4096;
	/*
		The retained 8,192-entry context packet matches the Vulkan tile capacity.
		It halves queue submissions for long sprite streams and improved both
		desktop adapters in paired completed-work measurements.
	*/
	caps->max_packed_blits = 8192u;
	return FB_GFX3_OK;
}

static int vulkan_backend_init(FB_GFX3_BACKEND *backend,
	const FB_GFX3_BACKEND_CONFIG *config)
{
	FB_GFX3_VULKAN_BACKEND_STATE *state;
	FB_GFX3_PLATFORM_WINDOW_CONFIG platform_config;
	uintptr_t native_instance;
	uintptr_t native_window;
	int result;

	if ((backend == NULL) || (config == NULL) ||
	    (config->resources == NULL) || (config->width == 0) ||
	    (config->height == 0) || (config->page_count == 0) ||
	    (vulkan_backend_bytes_per_pixel(config->depth) == 0))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_VULKAN_BACKEND_STATE *)calloc(1, sizeof(*state));
	if (state == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	state->resources = config->resources;
	state->logger = config->logger;
	state->platform_vtable = fb_gfx3_platform_default();
	backend->state = state;
	if ((state->platform_vtable == NULL) ||
	    (state->platform_vtable->create_window == NULL) ||
	    (state->platform_vtable->native_handles == NULL)) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	memset(&platform_config, 0, sizeof(platform_config));
	platform_config.input = config->platform;
	platform_config.width = config->width;
	platform_config.height = config->height;
	platform_config.flags = config->flags;
	platform_config.title = (config->title != NULL) ? config->title :
		"FreeBASIC gfxlib3 Vulkan";
	result = state->platform_vtable->create_window(&state->platform,
		&platform_config);
	if (result != FB_GFX3_OK)
		goto fail;
	result = state->platform_vtable->native_handles(state->platform,
		&native_instance, &native_window);
	if (result != FB_GFX3_OK)
		goto fail;
	result = fb_gfx3_vulkan_runtime_open_windowed(&state->runtime,
		native_instance, native_window, config->width, config->height);
	if (result != FB_GFX3_OK)
		goto fail;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_INFO,
		"Vulkan gfxlib3 backend initialized: device %u, %s "
		"(vendor %04x, device %04x)",
		state->runtime.selected_physical_device_index,
		state->runtime.selected_device_name[0] != '\0' ?
			state->runtime.selected_device_name : "unnamed adapter",
		state->runtime.selected_vendor_id,
		state->runtime.selected_device_id);
	state->client_width = config->width;
	state->client_height = config->height;
	return FB_GFX3_OK;

fail:
	fb_gfx3_vulkan_runtime_close(&state->runtime);
	if ((state->platform_vtable != NULL) &&
	    (state->platform_vtable->destroy != NULL))
		state->platform_vtable->destroy(state->platform);
	free(state);
	backend->state = NULL;
	return result;
}

static void vulkan_backend_shutdown(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_VULKAN_BACKEND_STATE *state;

	if ((backend == NULL) || (backend->state == NULL))
		return;
	state = (FB_GFX3_VULKAN_BACKEND_STATE *)backend->state;
	fb_gfx3_log_write(state->logger, FB_GFX3_LOG_INFO,
		"Vulkan submissions: %llu vkQueueSubmit calls for %llu "
		"completed runtime operations",
		(unsigned long long)state->runtime.queue_submit_count,
		(unsigned long long)state->runtime.completed_submission_count);
	fb_gfx3_vulkan_runtime_close(&state->runtime);
	if ((state->platform_vtable != NULL) &&
	    (state->platform_vtable->destroy != NULL))
		state->platform_vtable->destroy(state->platform);
	free(state->primitive_scratch);
	state->primitive_scratch = NULL;
	state->primitive_scratch_size = 0u;
	free(state);
	backend->state = NULL;
}

static int vulkan_backend_command_writes_visible(
	const FB_GFX3_VULKAN_BACKEND_STATE *state,
	const FB_GFX3_COMMAND *command)
{
	if ((state->visible_surface == 0) ||
	    (command->target != state->visible_surface))
		return FALSE;
	switch (command->type) {
	case FB_GFX3_COMMAND_SURFACE_UPLOAD:
	case FB_GFX3_COMMAND_CLEAR:
	case FB_GFX3_COMMAND_POINTS:
	case FB_GFX3_COMMAND_GLYPHS:
	case FB_GFX3_COMMAND_LINE:
	case FB_GFX3_COMMAND_LINES:
	case FB_GFX3_COMMAND_RECTANGLE:
	case FB_GFX3_COMMAND_RECTANGLES:
	case FB_GFX3_COMMAND_ELLIPSE:
	case FB_GFX3_COMMAND_BLIT:
	case FB_GFX3_COMMAND_BLITS:
	case FB_GFX3_COMMAND_TRANSFORM_BLIT:
		return TRUE;
	default:
		return FALSE;
	}
}

/* ------------------------------------------------------------------------- */
/* Ordered page-copy transfer batches                                        */
/* ------------------------------------------------------------------------- */

static int vulkan_backend_page_copy_description(
	const FB_GFX3_COMMAND *command,
	FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION *description)
{
	const FB_GFX3_BLIT_COMMAND *blit;

	if ((command == NULL) || (description == NULL))
		return FALSE;
	if (command->type == FB_GFX3_COMMAND_BLIT) {
		if (fb_gfx3_command_payload_size(command) != sizeof(*blit))
			return FALSE;
		blit = (const FB_GFX3_BLIT_COMMAND *)command->payload;
	} else if (command->type == FB_GFX3_COMMAND_BLITS) {
		const FB_GFX3_BLITS_COMMAND *blits;
		size_t expected_size;

		if (fb_gfx3_command_payload_size(command) <
		    offsetof(FB_GFX3_BLITS_COMMAND, blit))
			return FALSE;
		blits = (const FB_GFX3_BLITS_COMMAND *)command->payload;
		if ((blits->count != 1u) ||
		    (fb_gfx3_size_add(offsetof(FB_GFX3_BLITS_COMMAND, blit),
		     sizeof(blits->blit[0]), &expected_size) != FB_GFX3_OK) ||
		    (expected_size != fb_gfx3_command_payload_size(command)))
			return FALSE;
		blit = &blits->blit[0];
		if ((blit->source != blits->source) ||
		    (blit->mode != blits->mode) ||
		    (blit->alpha != blits->alpha))
			return FALSE;
	} else {
		return FALSE;
	}
	if ((blit->source == command->target) ||
	    (blit->mode != FB_GFX3_BLIT_PSET) ||
	    (blit->source_rect.x1 != 0) || (blit->source_rect.y1 != 0) ||
	    (blit->destination_x != 0) || (blit->destination_y != 0) ||
	    (blit->clip.x1 > 0) || (blit->clip.y1 > 0))
		return FALSE;
	description->source = blit->source;
	description->clip = blit->clip;
	description->source_rect = blit->source_rect;
	description->destination_x = blit->destination_x;
	description->destination_y = blit->destination_y;
	return TRUE;
}

static int vulkan_backend_command_is_deferred_full_page(
	const FB_GFX3_COMMAND *command)
{
	FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION description;

	if (command == NULL)
		return FALSE;
	if (command->type == FB_GFX3_COMMAND_PAGE_SET)
		return fb_gfx3_command_payload_size(command) ==
			sizeof(FB_GFX3_PAGE_SET_COMMAND);
	return vulkan_backend_page_copy_description(command, &description);
}

static size_t vulkan_backend_page_copy_run_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION description;
	size_t copy_count = 0u;
	size_t count = 0u;

	if ((commands == NULL) || (available == 0u))
		return 1u;
	while ((count < available) &&
	       (count < FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT)) {
		FB_GFX3_COMMAND *command = commands[count];

		if (command == NULL)
			break;
		if (command->type == FB_GFX3_COMMAND_PAGE_SET) {
			if (fb_gfx3_command_payload_size(command) !=
			    sizeof(FB_GFX3_PAGE_SET_COMMAND))
				break;
		} else if (vulkan_backend_page_copy_description(command,
		    &description)) {
			copy_count++;
		} else {
			break;
		}
		count++;
	}
	/* PAGE_SET alone is state work and does not need a transfer submission. */
	return (copy_count == 0u) ? 1u : count;
}

static int vulkan_backend_page_copy_retain(
	FB_GFX3_VULKAN_BACKEND_STATE *state, FB_GFX3_HANDLE handle,
	uint64_t sequence, FB_GFX3_HANDLE *handles,
	FB_GFX3_VULKAN_BACKEND_SURFACE **surfaces, size_t *surface_count,
	FB_GFX3_VULKAN_BACKEND_SURFACE **surface, size_t *surface_index)
{
	size_t index;
	int result;

	if ((state == NULL) || (handle == 0) || (handles == NULL) ||
	    (surfaces == NULL) || (surface_count == NULL) || (surface == NULL) ||
	    (surface_index == NULL))
		return FB_GFX3_INVALID;
	for (index = 0u; index < *surface_count; index++) {
		if (handles[index] == handle) {
			*surface = surfaces[index];
			*surface_index = index;
			return FB_GFX3_OK;
		}
	}
	if (*surface_count >=
	    (FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT * 2u))
		return FB_GFX3_EXHAUSTED;
	result = vulkan_backend_surface_retain_handle(state, handle, sequence,
		surface);
	if (result != FB_GFX3_OK)
		return result;
	handles[*surface_count] = handle;
	surfaces[*surface_count] = *surface;
	*surface_index = *surface_count;
	(*surface_count)++;
	return FB_GFX3_OK;
}

static int vulkan_backend_page_copy_run(
	FB_GFX3_VULKAN_BACKEND_STATE *state,
	FB_GFX3_COMMAND *const *commands, size_t command_count)
{
	FB_GFX3_VULKAN_SURFACE_COPY copies[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT];
	uint32_t copy_destination_index[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT];
	uint32_t copy_source_index[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT];
	unsigned char copy_needed[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT];
	FB_GFX3_HANDLE retained_handles[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT * 2u];
	FB_GFX3_VULKAN_BACKEND_SURFACE *retained_surfaces[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT * 2u];
	uint32_t retained_content[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT * 2u];
	unsigned char surface_needed[
		FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT * 2u];
	FB_GFX3_HANDLE final_visible_surface;
	size_t retained_count = 0u;
	size_t copy_count = 0u;
	size_t index;
	int visible_changed = FALSE;
	int result = FB_GFX3_OK;

	if ((state == NULL) || (commands == NULL) || (command_count == 0u) ||
	    (command_count > FB_GFX3_VULKAN_BACKEND_PAGE_COPY_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	memset(retained_content, 0, sizeof(retained_content));
	final_visible_surface = state->visible_surface;
	for (index = 0u; index < command_count; index++) {
		FB_GFX3_COMMAND *command = commands[index];
		FB_GFX3_VULKAN_BACKEND_SURFACE *destination;

		if (command->type == FB_GFX3_COMMAND_PAGE_SET) {
			const FB_GFX3_PAGE_SET_COMMAND *payload =
				(const FB_GFX3_PAGE_SET_COMMAND *)command->payload;
			size_t destination_index;

			result = vulkan_backend_page_copy_retain(state, command->target,
				commands[command_count - 1u]->sequence, retained_handles,
				retained_surfaces, &retained_count, &destination,
				&destination_index);
			if (result != FB_GFX3_OK)
				goto cleanup;
			if (retained_content[destination_index] == 0u)
				retained_content[destination_index] =
					(uint32_t)destination_index + 1u;
			if ((payload->width != destination->surface.width) ||
			    (payload->height != destination->surface.height) ||
			    (payload->depth != destination->surface.depth)) {
				result = FB_GFX3_INVALID;
				goto cleanup;
			}
			final_visible_surface = command->target;
			visible_changed = TRUE;
		} else {
			FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION description;
			FB_GFX3_VULKAN_BACKEND_SURFACE *source;
			size_t destination_index;
			size_t source_index;

			if (!vulkan_backend_page_copy_description(command, &description)) {
				result = FB_GFX3_INVALID;
				goto cleanup;
			}
			result = vulkan_backend_page_copy_retain(state, command->target,
				commands[command_count - 1u]->sequence, retained_handles,
				retained_surfaces, &retained_count, &destination,
				&destination_index);
			if (result != FB_GFX3_OK)
				goto cleanup;
			result = vulkan_backend_page_copy_retain(state, description.source,
				commands[command_count - 1u]->sequence, retained_handles,
				retained_surfaces, &retained_count, &source, &source_index);
			if (result != FB_GFX3_OK)
				goto cleanup;
			if (retained_content[destination_index] == 0u)
				retained_content[destination_index] =
					(uint32_t)destination_index + 1u;
			if (retained_content[source_index] == 0u)
				retained_content[source_index] =
					(uint32_t)source_index + 1u;
			if ((destination->surface.width != source->surface.width) ||
			    (destination->surface.height != source->surface.height) ||
			    (destination->surface.depth != source->surface.depth) ||
			    (description.source_rect.x2 !=
			     (int32_t)source->surface.width - 1) ||
			    (description.source_rect.y2 !=
			     (int32_t)source->surface.height - 1) ||
			    (description.clip.x2 <
			     (int32_t)destination->surface.width - 1) ||
			    (description.clip.y2 <
			     (int32_t)destination->surface.height - 1)) {
				result = FB_GFX3_UNSUPPORTED;
				goto cleanup;
			}
			/*
				A full PSET makes both pages byte-for-byte identical. Later full
				copies between the same logical contents are exact no-ops, so do not
				spend GPU bandwidth replaying them. The run contains no drawing
				commands, which makes this local content-token propagation complete.
			*/
			if (retained_content[destination_index] !=
			    retained_content[source_index]) {
				copies[copy_count].destination = &destination->surface;
				copies[copy_count].source = &source->surface;
				copy_destination_index[copy_count] =
					(uint32_t)destination_index;
				copy_source_index[copy_count] = (uint32_t)source_index;
				copy_count++;
				retained_content[destination_index] =
					retained_content[source_index];
			}
		}
	}
	if (copy_count == 0u) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	/*
		Only the final contents of each surface can be observed after this ordered
		drain. Walk backwards and discard a copy whose destination is overwritten
		before any later copy reads it. If a retained copy reads an earlier
		destination, that source becomes live and preserves the dependency.
	*/
	memset(copy_needed, 0, sizeof(copy_needed));
	memset(surface_needed, 1, retained_count * sizeof(surface_needed[0]));
	for (index = copy_count; index != 0u; index--) {
		size_t copy_index = index - 1u;
		uint32_t destination_index =
			copy_destination_index[copy_index];
		uint32_t source_index = copy_source_index[copy_index];

		if (!surface_needed[destination_index])
			continue;
		copy_needed[copy_index] = TRUE;
		surface_needed[destination_index] = FALSE;
		surface_needed[source_index] = TRUE;
	}
	{
		size_t output_count = 0u;

		for (index = 0u; index < copy_count; index++) {
			if (copy_needed[index])
				copies[output_count++] = copies[index];
		}
		copy_count = output_count;
	}
	result = fb_gfx3_vulkan_surface_copy_batch(&state->runtime, copies,
		copy_count);
	if (result == FB_GFX3_OK) {
		state->visible_surface = final_visible_surface;
		if (visible_changed)
			state->presentation_dirty = TRUE;
	}

cleanup:
	for (index = 0u; index < retained_count; index++)
		fb_gfx3_resource_release(state->resources, retained_handles[index]);
	return result;
}

/* See the matching OpenGL helper. This only elides opaque clears with an
   identical target and clip, so no intervening public operation can observe
   a colour which the retained final clear would not produce. */
static size_t vulkan_backend_clear_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	const FB_GFX3_CLEAR_COMMAND *first;
	size_t count = 1;

	if ((commands == NULL) || (available == 0) || (commands[0] == NULL) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_CLEAR_COMMAND *)commands[0]->payload;
	if ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0)
		return 1;
	while (count < available) {
		const FB_GFX3_CLEAR_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_CLEAR) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_CLEAR_COMMAND *)commands[count]->payload;
		if (((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
		count++;
	}
	return count;
}

/*
	See opengl_paint_batch_count(). The retained command is the final opaque
	solid recolour of an unchanged flood topology. Keeping this predicate in the
	backend avoids teaching the public command queue Vulkan submission details.
*/
static size_t vulkan_backend_paint_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	const FB_GFX3_PAINT_COMMAND *first;
	size_t count = 1u;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_PAINT) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1u;
	first = (const FB_GFX3_PAINT_COMMAND *)commands[0]->payload;
	if ((first->paint_mode != 0u) || (first->pattern_size != 0u) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
	    (first->color == first->border_color))
		return 1u;
	while (count < available) {
		const FB_GFX3_PAINT_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_PAINT) ||
		    (commands[count]->target != commands[0]->target) ||
		    (commands[count]->sequence <= commands[count - 1u]->sequence) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_PAINT_COMMAND *)commands[count]->payload;
		if ((candidate->paint_mode != 0u) ||
		    (candidate->pattern_size != 0u) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u) ||
		    (candidate->color == candidate->border_color) ||
		    (candidate->x != first->x) || (candidate->y != first->y) ||
		    (candidate->border_color != first->border_color) ||
		    (memcmp(&candidate->clip, &first->clip,
		     sizeof(candidate->clip)) != 0))
			break;
		count++;
	}
	return count;
}

static size_t vulkan_backend_palette_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	size_t count = 1;

	if ((commands == NULL) || (available < 2u) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_PALETTE) ||
	    (fb_gfx3_command_payload_size(commands[0]) !=
	     sizeof(FB_GFX3_PALETTE_COMMAND)))
		return 1u;
	while ((count < available) && (commands[count] != NULL) &&
	       (commands[count]->type == FB_GFX3_COMMAND_PALETTE) &&
	       (fb_gfx3_command_payload_size(commands[count]) ==
	        sizeof(FB_GFX3_PALETTE_COMMAND)))
		count++;
	return count;
}

static size_t vulkan_backend_rectangle_clear_batch_count(
	FB_GFX3_COMMAND *const *commands, size_t available)
{
	const FB_GFX3_RECTANGLE_COMMAND *first;
	size_t count = 1;

	if ((commands == NULL) || (available == 0) || (commands[0] == NULL) ||
	    (commands[0]->type != FB_GFX3_COMMAND_RECTANGLE) ||
	    (fb_gfx3_command_payload_size(commands[0]) != sizeof(*first)))
		return 1;
	first = (const FB_GFX3_RECTANGLE_COMMAND *)commands[0]->payload;
	if ((first->filled == 0) ||
	    ((first->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
		return 1;
	while ((count < available) &&
	       (count < FB_GFX3_VULKAN_BACKEND_RECTANGLE_BATCH_LIMIT)) {
		const FB_GFX3_RECTANGLE_COMMAND *candidate;

		if ((commands[count] == NULL) ||
		    (commands[count]->type != FB_GFX3_COMMAND_RECTANGLE) ||
		    (commands[count]->target != commands[0]->target) ||
		    (fb_gfx3_command_payload_size(commands[count]) !=
		     sizeof(*candidate)))
			break;
		candidate = (const FB_GFX3_RECTANGLE_COMMAND *)commands[count]->payload;
		if ((candidate->filled == 0) ||
		    ((candidate->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
			break;
		count++;
	}
	return count;
}

static size_t vulkan_backend_line_batch_count(FB_GFX3_COMMAND *const *commands,
	size_t available)
{
	size_t count = 1;

	if ((commands == NULL) || (available == 0) || (commands[0] == NULL) ||
		(commands[0]->type != FB_GFX3_COMMAND_LINE) ||
		(fb_gfx3_command_payload_size(commands[0]) !=
		 sizeof(FB_GFX3_LINE_COMMAND)))
		return 1;
	while ((count < available) &&
	       (count < FB_GFX3_VULKAN_BACKEND_LINE_BATCH_LIMIT)) {
		FB_GFX3_COMMAND *candidate = commands[count];

		if ((candidate == NULL) ||
			(candidate->type != FB_GFX3_COMMAND_LINE) ||
			(candidate->target != commands[0]->target) ||
			(fb_gfx3_command_payload_size(candidate) !=
			 sizeof(FB_GFX3_LINE_COMMAND)))
			break;
		count++;
	}
	return count;
}

static int vulkan_backend_execute(FB_GFX3_BACKEND *backend,
	FB_GFX3_COMMAND *const *commands, size_t count,
	uint64_t *submitted_sequence)
{
	FB_GFX3_VULKAN_BACKEND_STATE *state;
	FB_GFX3_COMMAND *command;
	size_t index;
	uint64_t runtime_completed_sequence;
	int defer_page_presentation = TRUE;
	int result;

	if ((backend == NULL) || (backend->state == NULL) ||
	    (commands == NULL) || (count == 0))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_VULKAN_BACKEND_STATE *)backend->state;
	/*
		SCREENEVENT needs the native queue, not a Vulkan progress scan, resize
		query, or keyboard-overlay comparison. Keep this ordered through the render
		thread because that thread owns the window, then return before touching the
		GPU runtime.
	*/
	if ((count == 1u) && (commands[0] != NULL) &&
	    (commands[0]->type == FB_GFX3_COMMAND_INPUT_POLL)) {
		command = commands[0];
		if ((fb_gfx3_command_payload_size(command) != 0u) ||
		    (command->sequence <= state->submitted_sequence))
			return FB_GFX3_INVALID;
		state->platform_vtable->pump_events(state->platform);
		state->submitted_sequence = command->sequence;
		if (submitted_sequence != NULL)
			*submitted_sequence = command->sequence;
		return FB_GFX3_OK;
	}
	for (index = 0u; index < count; index++) {
		if (!vulkan_backend_command_is_deferred_full_page(commands[index])) {
			defer_page_presentation = FALSE;
			break;
		}
	}
	result = vulkan_backend_poll_platform(state);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < count; index++) {
		FB_GFX3_VULKAN_PAGE_COPY_DESCRIPTION page_copy_description;
		size_t execute_count = 1;
		size_t execute_index;
		size_t page_copy_run_count;

		command = commands[index];
		if ((command == NULL) ||
		    (command->type == FB_GFX3_COMMAND_INVALID) ||
		    (command->type == FB_GFX3_COMMAND_RENDERER_SHUTDOWN) ||
		    (command->size < offsetof(FB_GFX3_COMMAND, payload)) ||
		    (command->size > FB_GFX3_COMMAND_MAX_SIZE) ||
		    (command->sequence <= state->submitted_sequence))
			return FB_GFX3_INVALID;
		/*
			SCREENSET may be interleaved with full-page SCREENCOPY operations.
			Those state changes do not require an intermediate presentation, so one
			ordered Vulkan transfer buffer can cover the whole run. This removes a
			queue submission and a per-blit descriptor allocation from every flip.
		*/
		page_copy_run_count = vulkan_backend_page_copy_run_count(
			commands + index, count - index);
		if ((page_copy_run_count > 1u) ||
		    vulkan_backend_page_copy_description(command,
		     &page_copy_description)) {
			execute_count = page_copy_run_count;
			result = vulkan_backend_page_copy_run(state, commands + index,
				execute_count);
			if (result != FB_GFX3_UNSUPPORTED)
				goto command_executed;
			execute_count = 1u;
		}
		/*
			A Basic drawing loop commonly interleaves PSET, LINE, rectangle,
			and CIRCLE commands.
			Executing those commands through their individual compute pipelines
			creates one dispatch and barrier chain per primitive. The unified
			winner pass preserves exact command order while exposing the complete
			primitive stream to the GPU in two dispatches.

			Some Vulkan drivers deliberately decline this atomic winner path.
			FB_GFX3_UNSUPPORTED therefore means that execution must continue
			through the established type-specific implementations below.
		*/
		if ((command->type == FB_GFX3_COMMAND_POINTS) ||
		    (command->type == FB_GFX3_COMMAND_LINE) ||
		    (command->type == FB_GFX3_COMMAND_LINES) ||
		    (command->type == FB_GFX3_COMMAND_RECTANGLE) ||
		    (command->type == FB_GFX3_COMMAND_RECTANGLES) ||
		    (command->type == FB_GFX3_COMMAND_ELLIPSE)) {
			size_t primitive_count = 0u;

			execute_count = vulkan_backend_primitive_batch_count(
				commands + index, count - index, &primitive_count);
			if (execute_count > 1u) {
				result = vulkan_backend_primitive_batch(state,
					&commands[index], execute_count, primitive_count);
				if (result != FB_GFX3_UNSUPPORTED)
					goto command_executed;
				execute_count = 1u;
			}
		}
		/*
			Only adjacent PUT operations with identical surface handles can share
			a submission. The runtime owns one descriptor set per ordered dispatch
			in a submission slot, bounded to the documented 256-entry capacity.
		*/
		if (command->type == FB_GFX3_COMMAND_CLEAR) {
			execute_count = vulkan_backend_clear_batch_count(commands + index,
				count - index);
			if (execute_count > 1)
				result = vulkan_backend_clear(state,
					commands[index + execute_count - 1]);
			else
				result = vulkan_backend_execute_one(state, command);
		} else if (command->type == FB_GFX3_COMMAND_RECTANGLE) {
			execute_count = vulkan_backend_rectangle_clear_batch_count(
				commands + index, count - index);
			if (execute_count > 1)
				result = vulkan_backend_rectangle_clear_batch(state,
					&commands[index], execute_count);
			else
				result = vulkan_backend_execute_one(state, command);
		} else if (command->type == FB_GFX3_COMMAND_LINE) {
			execute_count = vulkan_backend_line_batch_count(commands + index,
				count - index);
			if (execute_count > 1)
				result = vulkan_backend_line_batch(state, &commands[index],
					execute_count);
			else
				result = vulkan_backend_execute_one(state, command);
		} else if (command->type == FB_GFX3_COMMAND_POINTS) {
			execute_count = vulkan_backend_points_batch_count(commands + index,
				count - index);
		} else if (command->type == FB_GFX3_COMMAND_GLYPHS) {
			execute_count = vulkan_backend_glyph_batch_count(commands + index,
				count - index);
		} else if (command->type == FB_GFX3_COMMAND_ELLIPSE) {
			execute_count = vulkan_backend_ellipse_batch_count(commands + index,
				count - index);
		} else if (command->type == FB_GFX3_COMMAND_PAINT) {
			execute_count = vulkan_backend_paint_batch_count(commands + index,
				count - index);
		} else if (command->type == FB_GFX3_COMMAND_PALETTE) {
			execute_count = vulkan_backend_palette_batch_count(commands + index,
				count - index);
		} else if ((command->type == FB_GFX3_COMMAND_BLIT) &&
		    (fb_gfx3_command_payload_size(command) ==
		     sizeof(FB_GFX3_BLIT_COMMAND))) {
			const FB_GFX3_BLIT_COMMAND *payload =
				(const FB_GFX3_BLIT_COMMAND *)command->payload;

			while ((payload->source != command->target) &&
			       (index + execute_count < count) &&
			       (execute_count < FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT)) {
				FB_GFX3_COMMAND *candidate =
					commands[index + execute_count];
				const FB_GFX3_BLIT_COMMAND *candidate_payload;

				if ((candidate == NULL) ||
				    (candidate->type != FB_GFX3_COMMAND_BLIT) ||
				    (fb_gfx3_command_payload_size(candidate) !=
				     sizeof(*candidate_payload)))
					break;
				candidate_payload =
					(const FB_GFX3_BLIT_COMMAND *)candidate->payload;
				if ((candidate->target != command->target) ||
				    (candidate_payload->source != payload->source))
					break;
				execute_count++;
			}
		} else if ((command->type == FB_GFX3_COMMAND_TRANSFORM_BLIT) &&
		    (fb_gfx3_command_payload_size(command) ==
		     sizeof(FB_GFX3_TRANSFORM_BLIT_COMMAND))) {
			const FB_GFX3_TRANSFORM_BLIT_COMMAND *payload =
				(const FB_GFX3_TRANSFORM_BLIT_COMMAND *)command->payload;

			while ((payload->source != command->target) &&
			       (index + execute_count < count) &&
			       (execute_count <
			        FB_GFX3_VULKAN_BACKEND_BLIT_BATCH_LIMIT)) {
				FB_GFX3_COMMAND *candidate =
					commands[index + execute_count];
				const FB_GFX3_TRANSFORM_BLIT_COMMAND *candidate_payload;

				if ((candidate == NULL) ||
				    (candidate->type != FB_GFX3_COMMAND_TRANSFORM_BLIT) ||
				    (fb_gfx3_command_payload_size(candidate) !=
				     sizeof(*candidate_payload)))
					break;
				candidate_payload =
					(const FB_GFX3_TRANSFORM_BLIT_COMMAND *)
					candidate->payload;
				if ((candidate->target != command->target) ||
				    (candidate_payload->source != payload->source))
					break;
				execute_count++;
			}
		}
		if ((command->type == FB_GFX3_COMMAND_BLIT) && (execute_count > 1))
			result = vulkan_backend_blit_batch(state, &commands[index],
				execute_count);
		else if ((command->type == FB_GFX3_COMMAND_TRANSFORM_BLIT) &&
		    (execute_count > 1))
			result = vulkan_backend_transform_blit_batch(state,
				&commands[index], execute_count);
		else if ((command->type == FB_GFX3_COMMAND_POINTS) &&
		    (execute_count > 1))
			result = vulkan_backend_points_batch(state, &commands[index],
				execute_count);
		else if (command->type == FB_GFX3_COMMAND_GLYPHS)
			result = vulkan_backend_glyphs(state, &commands[index],
				execute_count);
		else if ((command->type == FB_GFX3_COMMAND_ELLIPSE) &&
		    (execute_count > 1))
			result = vulkan_backend_ellipse_batch(state, &commands[index],
				execute_count);
		else if ((command->type == FB_GFX3_COMMAND_PAINT) &&
		    (execute_count > 1u))
			result = vulkan_backend_execute_one(state,
				commands[index + execute_count - 1u]);
		else if ((command->type == FB_GFX3_COMMAND_PALETTE) &&
		    (execute_count > 1))
			result = vulkan_backend_palette(state,
				commands[index + execute_count - 1u]);
		else if ((command->type != FB_GFX3_COMMAND_CLEAR) &&
		    (command->type != FB_GFX3_COMMAND_RECTANGLE) &&
		    (command->type != FB_GFX3_COMMAND_LINE) &&
		    (command->type != FB_GFX3_COMMAND_GLYPHS) &&
		    !((command->type == FB_GFX3_COMMAND_PAINT) &&
		      (execute_count > 1u))) {
			/*
				SCREENSET and SCREENCOPY emit asynchronous PRESENT commands. When
				another command follows in this renderer drain, no Basic caller can
				observe that intermediate front buffer. Keep the final visible page
				and perform one real swap at the ordered end of the drain. A caller
				that requested a synchronous present is a completion boundary and
				therefore reaches this code as a one-command batch.
			*/
			if ((command->type == FB_GFX3_COMMAND_PRESENT) &&
			    (index + 1u < count) &&
			    (command->target == state->visible_surface)) {
				state->presentation_dirty = TRUE;
				result = FB_GFX3_OK;
			} else
				result = vulkan_backend_execute_one(state, command);
		}

command_executed:
		if (result != FB_GFX3_OK)
			return result;
		/*
			Only runtime calls that submitted GPU work advance this tag. Pure
			state commands leave the latest tag unchanged, so a later wait does
			not accidentally claim they owned an earlier command buffer.
		*/
		for (execute_index = 0; execute_index < execute_count;
		     execute_index++) {
			command = commands[index + execute_index];
			result = fb_gfx3_vulkan_runtime_tag_submission(&state->runtime,
				command->sequence);
			if (result != FB_GFX3_OK)
				return result;
			if (vulkan_backend_command_writes_visible(state, command))
				state->presentation_dirty = TRUE;
			state->submitted_sequence = command->sequence;
		}
		index += execute_count - 1;
	}
	if (state->presentation_dirty && (state->visible_surface != 0) &&
	    !defer_page_presentation) {
		result = vulkan_backend_present_handle(state,
			state->visible_surface, state->submitted_sequence);
		if (result != FB_GFX3_OK)
			return result;
	}
	/*
		Completed sequence publication follows slot fences, not command
		submission. This keeps resource retirement correct while presentation
		and offscreen work remain queued in the six-slot Vulkan pipeline.
		SCREENSYNC and readback still use the targeted wait path below.
	*/
	runtime_completed_sequence =
		fb_gfx3_vulkan_runtime_completed_sequence(&state->runtime);
	if (runtime_completed_sequence > state->completed_sequence)
		state->completed_sequence = runtime_completed_sequence;
	if (submitted_sequence != NULL)
		*submitted_sequence = state->submitted_sequence;
	return FB_GFX3_OK;
}

static uint64_t vulkan_backend_completed_sequence(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_VULKAN_BACKEND_STATE *state;
	uint64_t runtime_completed_sequence;

	if ((backend == NULL) || (backend->state == NULL))
		return 0;
	state = (FB_GFX3_VULKAN_BACKEND_STATE *)backend->state;
	runtime_completed_sequence =
		fb_gfx3_vulkan_runtime_completed_sequence(&state->runtime);
	if (runtime_completed_sequence > state->completed_sequence)
		state->completed_sequence = runtime_completed_sequence;
	return state->completed_sequence;
}

static int vulkan_backend_wait_sequence(FB_GFX3_BACKEND *backend,
	uint64_t sequence)
{
	FB_GFX3_VULKAN_BACKEND_STATE *state;
	int result;

	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_VULKAN_BACKEND_STATE *)backend->state;
	state->completed_sequence = vulkan_backend_completed_sequence(backend);
	if (sequence <= state->completed_sequence)
		return FB_GFX3_OK;
	result = fb_gfx3_vulkan_runtime_wait_sequence(&state->runtime, sequence);
	if (result != FB_GFX3_OK)
		return result;
	/*
		A renderer batch may already contain later asynchronous commands. The
		targeted slot wait proves this requested sequence, not the tail of that
		batch, so resource collection must not retire those later users early.
	*/
	state->completed_sequence =
		fb_gfx3_vulkan_runtime_completed_sequence(&state->runtime);
	if (sequence > state->completed_sequence)
		state->completed_sequence = sequence;
	return (sequence <= state->completed_sequence) ?
		FB_GFX3_OK : FB_GFX3_FAILED;
}

static int vulkan_backend_wait_idle(FB_GFX3_BACKEND *backend)
{
	FB_GFX3_VULKAN_BACKEND_STATE *state;
	int result;

	if ((backend == NULL) || (backend->state == NULL))
		return FB_GFX3_INVALID;
	state = (FB_GFX3_VULKAN_BACKEND_STATE *)backend->state;
	result = fb_gfx3_vulkan_runtime_wait_idle(&state->runtime);
	if (result == FB_GFX3_OK)
		state->completed_sequence = state->submitted_sequence;
	return result;
}

const FB_GFX3_BACKEND_VTABLE __fb_gfx3_backend_vulkan = {
	FB_GFX3_BACKEND_ABI_VERSION,
	"Vulkan compute",
	vulkan_backend_probe,
	vulkan_backend_init,
	vulkan_backend_shutdown,
	vulkan_backend_execute,
	vulkan_backend_completed_sequence,
	vulkan_backend_wait_sequence,
	vulkan_backend_wait_idle,
	NULL
};

/* end of gfx3_backend_vulkan.c */
