/*
 * FreeBASIC gfxlib3: gfx3_vulkan_draw.c
 * Purpose: Record Vulkan point, glyph, geometric and paint commands.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_vulkan_private.h"

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || defined(HOST_DARWIN)

/* ------------------------------------------------------------------------- */
/* Points and glyphs                                                        */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vulkan_surface_points(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	const FB_GFX3_POINT *points, uint32_t point_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer;
	FB_GFX3_VULKAN_POINT_COMMAND *point_command;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[2];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[2];
	FB_GFX3_RECT clipped;
	size_t points_size;
	size_t command_size;
	uint32_t group_count_x;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if ((result != FB_GFX3_OK) || (clip == NULL) ||
	    ((point_count != 0) && (points == NULL)) ||
	    (fb_gfx3_vk_size_multiply(point_count, sizeof(points[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(offsetof(FB_GFX3_VULKAN_POINT_COMMAND, points),
	     points_size, &command_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (point_count == 0)
		return FB_GFX3_OK;
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)surface->width)
		clipped.x2 = (int32_t)surface->width - 1;
	if (clipped.y2 >= (int32_t)surface->height)
		clipped.y2 = (int32_t)surface->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
		return FB_GFX3_OK;
	/*
		The next submission owns this mapped packet until its fence signals.
		Reusing it avoids one VkBuffer and VkDeviceMemory allocation for every
		automatic PSET batch.
	*/
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->pending_submission &&
	    (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->point_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK)
		return result;
	command_buffer = &slot->point_command_buffer;
	point_command = (FB_GFX3_VULKAN_POINT_COMMAND *)command_buffer->mapped;
	point_command->surface[0] = surface->width;
	point_command->surface[1] = surface->height;
	point_command->surface[2] = point_count;
	point_command->surface[3] = fb_gfx3_vk_surface_color_mask(surface->depth);
	point_command->clip = clipped;
	memcpy(point_command->points, points, points_size);
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;

	memset(buffer_info, 0, sizeof(buffer_info));
	buffer_info[0].buffer = storage->storage.buffer;
	buffer_info[0].range = storage->storage.size;
	buffer_info[1].buffer = command_buffer->buffer;
	buffer_info[1].range = command_size;
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	descriptor_writes[0].structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].destination_set =
		slot->descriptor_sets[0];
	descriptor_writes[0].descriptor_count = 1;
	descriptor_writes[0].descriptor_type =
		FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[0].buffer_info = &buffer_info[0];
	descriptor_writes[1] = descriptor_writes[0];
	descriptor_writes[1].destination_binding = 1;
	descriptor_writes[1].buffer_info = &buffer_info[1];
	implementation->update_descriptor_sets(implementation->device, 2,
		descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->points_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&slot->descriptor_sets[0], 0, NULL);
	group_count_x = (point_count - 1u) / 64u + 1u;
	implementation->command_dispatch(implementation->command_buffer,
		group_count_x, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

/*
	Record a FIFO run of point commands in one command-buffer submission.  The
	point shader is not allowed to reorder operations: each dispatch has its
	own descriptor range and a compute write-to-read barrier before the next
	dispatch.  This is useful for arcs and PSET streams, whose commands can
	overlap even when every individual point is opaque.
*/
int fb_gfx3_vulkan_surface_points_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_POINTS *operations, size_t operation_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 2];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 2];
	FB_GFX3_RECT clipped[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	const FB_GFX3_VULKAN_POINTS *active[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t offsets[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t sizes[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	uint32_t groups[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t active_count = 0;
	size_t command_size = 0;
	size_t index;
	int result;

	if ((operations == NULL) || (operation_count < 2u) ||
		(operation_count > FB_GFX3_VK_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < operation_count; index++) {
		const FB_GFX3_VULKAN_POINTS *operation = &operations[index];
		size_t points_size;
		size_t size;

		if ((operation->point_count == 0u) || (operation->points == NULL))
			continue;
		if ((fb_gfx3_vk_size_multiply(operation->point_count,
		     sizeof(operation->points[0]), &points_size) != FB_GFX3_OK) ||
		    (fb_gfx3_vk_size_add(offsetof(FB_GFX3_VULKAN_POINT_COMMAND, points),
		     points_size, &size) != FB_GFX3_OK) ||
		    (fb_gfx3_vk_size_add(command_size, size, &command_size) !=
		     FB_GFX3_OK))
			return FB_GFX3_UNSUPPORTED;
		clipped[active_count] = operation->clip;
		if (clipped[active_count].x1 < 0)
			clipped[active_count].x1 = 0;
		if (clipped[active_count].y1 < 0)
			clipped[active_count].y1 = 0;
		if (clipped[active_count].x2 >= (int32_t)surface->width)
			clipped[active_count].x2 = (int32_t)surface->width - 1;
		if (clipped[active_count].y2 >= (int32_t)surface->height)
			clipped[active_count].y2 = (int32_t)surface->height - 1;
		if ((clipped[active_count].x1 > clipped[active_count].x2) ||
		    (clipped[active_count].y1 > clipped[active_count].y2)) {
			command_size -= size;
			continue;
		}
		offsets[active_count] = command_size - size;
		sizes[active_count] = size;
		groups[active_count] = (operation->point_count - 1u) / 64u + 1u;
		active[active_count] = operation;
		active_count++;
	}
	if (active_count == 0u)
		return FB_GFX3_OK;
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->pending_submission &&
	    (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->point_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK)
		return result;
	command_buffer = &slot->point_command_buffer;
	for (index = 0; index < active_count; index++) {
		const FB_GFX3_VULKAN_POINTS *operation = active[index];
		FB_GFX3_VULKAN_POINT_COMMAND *point_command =
			(FB_GFX3_VULKAN_POINT_COMMAND *)((uint8_t *)command_buffer->mapped +
			offsets[index]);

		point_command->surface[0] = surface->width;
		point_command->surface[1] = surface->height;
		point_command->surface[2] = operation->point_count;
		point_command->surface[3] = fb_gfx3_vk_surface_color_mask(surface->depth);
		point_command->clip = clipped[index];
		memcpy(point_command->points, operation->points,
			sizes[index] - offsetof(FB_GFX3_VULKAN_POINT_COMMAND, points));
	}
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	for (index = 0; index < active_count; index++) {
		size_t write_index = index * 2u;

		buffer_info[write_index].buffer = storage->storage.buffer;
		buffer_info[write_index].range = storage->storage.size;
		buffer_info[write_index + 1u].buffer = command_buffer->buffer;
		buffer_info[write_index + 1u].offset = offsets[index];
		buffer_info[write_index + 1u].range = sizes[index];
		descriptor_writes[write_index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[write_index].destination_set =
			slot->descriptor_sets[index];
		descriptor_writes[write_index].descriptor_count = 1;
		descriptor_writes[write_index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[write_index].buffer_info = &buffer_info[write_index];
		descriptor_writes[write_index + 1u] = descriptor_writes[write_index];
		descriptor_writes[write_index + 1u].destination_binding = 1;
		descriptor_writes[write_index + 1u].buffer_info =
			&buffer_info[write_index + 1u];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(active_count * 2u), descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, implementation->points_pipeline);
	for (index = 0; index < active_count; index++) {
		FB_GFX3_VK_DESCRIPTOR_SET descriptor_set =
			slot->descriptor_sets[index];

		implementation->command_bind_descriptor_sets(implementation->command_buffer,
			FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
			implementation->compute_pipeline_layout, 0, 1, &descriptor_set, 0,
			NULL);
		implementation->command_dispatch(implementation->command_buffer,
			groups[index], 1, 1);
		fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
			storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

/*
	Glyph tile replay

	A glyph can straddle at most four 16 by 16 tiles. The CPU builds only that
	small ordered index, then each shader invocation owns one destination pixel
	and decodes all foreground and background decisions for it. No destination
	atomics are required, and overlapping console writes remain deterministic on
	integrated and discrete Vulkan implementations.
*/
int fb_gfx3_vulkan_surface_glyphs(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	const FB_GFX3_GLYPH *glyphs, uint32_t glyph_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *range_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *index_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[4];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[4];
	FB_GFX3_VULKAN_GLYPH_COMMAND *command;
	FB_GFX3_RECT clipped;
	uint32_t *tile_counts = NULL;
	uint32_t *tile_ranges = NULL;
	uint32_t *tile_cursors = NULL;
	uint32_t *tile_indices = NULL;
	uint32_t first_tile_x = UINT32_MAX;
	uint32_t first_tile_y = UINT32_MAX;
	uint32_t last_tile_x = 0u;
	uint32_t last_tile_y = 0u;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tile_count;
	uint32_t index_count = 0u;
	uint32_t index;
	size_t glyph_bytes;
	size_t command_size;
	uint64_t command_capacity;
	uint64_t range_capacity;
	uint64_t index_capacity;
	int have_work = FALSE;
	int result;

	if ((clip == NULL) || (glyphs == NULL) || (glyph_count == 0u) ||
	    (glyph_count > FB_GFX3_VK_GLYPH_BATCH_LIMIT) ||
	    (sizeof(FB_GFX3_GLYPH) != 96u) ||
	    (offsetof(FB_GFX3_VULKAN_GLYPH_COMMAND, glyph) != 48u) ||
	    (fb_gfx3_vk_size_multiply(glyph_count, sizeof(glyphs[0]),
	     &glyph_bytes) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(offsetof(FB_GFX3_VULKAN_GLYPH_COMMAND, glyph),
	     glyph_bytes, &command_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (implementation->glyph_tile_pipeline == 0)
		return FB_GFX3_UNSUPPORTED;
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)storage->width)
		clipped.x2 = (int32_t)storage->width - 1;
	if (clipped.y2 >= (int32_t)storage->height)
		clipped.y2 = (int32_t)storage->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
		return FB_GFX3_OK;
	for (index = 0u; index < glyph_count; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64;
		int64_t bottom64;
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
		uint32_t tile_x;
		uint32_t tile_y;

		if ((glyph->width == 0u) || (glyph->width > 8u) ||
		    (glyph->height == 0u) || (glyph->height > 16u) ||
		    ((glyph->flags &
		      ~(uint32_t)FB_GFX3_GLYPH_BACKGROUND) != 0u))
			return FB_GFX3_INVALID;
		right64 = (int64_t)glyph->x + glyph->width - 1u;
		bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		left = (glyph->x > clipped.x1) ? glyph->x : clipped.x1;
		top = (glyph->y > clipped.y1) ? glyph->y : clipped.y1;
		right = (right64 < clipped.x2) ? (int32_t)right64 : clipped.x2;
		bottom = (bottom64 < clipped.y2) ? (int32_t)bottom64 : clipped.y2;
		if ((left > right) || (top > bottom))
			continue;
		tile_x = (uint32_t)left / FB_GFX3_VK_GLYPH_TILE_SIZE;
		tile_y = (uint32_t)top / FB_GFX3_VK_GLYPH_TILE_SIZE;
		if (tile_x < first_tile_x)
			first_tile_x = tile_x;
		if (tile_y < first_tile_y)
			first_tile_y = tile_y;
		tile_x = (uint32_t)right / FB_GFX3_VK_GLYPH_TILE_SIZE;
		tile_y = (uint32_t)bottom / FB_GFX3_VK_GLYPH_TILE_SIZE;
		if (tile_x > last_tile_x)
			last_tile_x = tile_x;
		if (tile_y > last_tile_y)
			last_tile_y = tile_y;
		have_work = TRUE;
	}
	if (!have_work)
		return FB_GFX3_OK;
	tiles_x = last_tile_x - first_tile_x + 1u;
	tiles_y = last_tile_y - first_tile_y + 1u;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tile_count = tiles_x * tiles_y) > 65536u))
		return FB_GFX3_UNSUPPORTED;
	tile_counts = (uint32_t *)calloc(tile_count, sizeof(tile_counts[0]));
	if (tile_counts == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	for (index = 0u; index < glyph_count; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64 = (int64_t)glyph->x + glyph->width - 1u;
		int64_t bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		int32_t left = (glyph->x > clipped.x1) ? glyph->x : clipped.x1;
		int32_t top = (glyph->y > clipped.y1) ? glyph->y : clipped.y1;
		int32_t right = (right64 < clipped.x2) ? (int32_t)right64 : clipped.x2;
		int32_t bottom = (bottom64 < clipped.y2) ? (int32_t)bottom64 : clipped.y2;
		uint32_t tile_y;

		if ((left > right) || (top > bottom))
			continue;
		for (tile_y = (uint32_t)top / FB_GFX3_VK_GLYPH_TILE_SIZE;
		     tile_y <= (uint32_t)bottom / FB_GFX3_VK_GLYPH_TILE_SIZE;
		     ++tile_y) {
			uint32_t tile_x;

			for (tile_x = (uint32_t)left / FB_GFX3_VK_GLYPH_TILE_SIZE;
			     tile_x <= (uint32_t)right / FB_GFX3_VK_GLYPH_TILE_SIZE;
			     ++tile_x) {
				uint32_t tile;

				if ((tile_x < first_tile_x) || (tile_x > last_tile_x) ||
				    (tile_y < first_tile_y) || (tile_y > last_tile_y)) {
					result = FB_GFX3_INVALID;
					goto cleanup;
				}
				tile = (tile_y - first_tile_y) * tiles_x +
					(tile_x - first_tile_x);
				if (tile >= tile_count) {
					result = FB_GFX3_INVALID;
					goto cleanup;
				}
				if (tile_counts[tile] == UINT32_MAX) {
					result = FB_GFX3_OUT_OF_MEMORY;
					goto cleanup;
				}
				tile_counts[tile]++;
			}
		}
	}
	for (index = 0u; index < tile_count; ++index) {
		if (tile_counts[index] > UINT32_MAX - index_count) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto cleanup;
		}
		index_count += tile_counts[index];
	}
	if (index_count == 0u) {
		result = FB_GFX3_OK;
		goto cleanup;
	}
	tile_ranges = (uint32_t *)calloc((size_t)tile_count * 2u,
		sizeof(tile_ranges[0]));
	tile_cursors = (uint32_t *)calloc(tile_count, sizeof(tile_cursors[0]));
	tile_indices = (uint32_t *)malloc((size_t)index_count *
		sizeof(tile_indices[0]));
	if ((tile_ranges == NULL) || (tile_cursors == NULL) ||
	    (tile_indices == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	{
		uint32_t cursor = 0u;

		for (index = 0u; index < tile_count; ++index) {
			tile_ranges[index * 2u] = cursor;
			tile_ranges[index * 2u + 1u] = tile_counts[index];
			tile_cursors[index] = cursor;
			cursor += tile_counts[index];
		}
	}
	for (index = 0u; index < glyph_count; ++index) {
		const FB_GFX3_GLYPH *glyph = &glyphs[index];
		int64_t right64 = (int64_t)glyph->x + glyph->width - 1u;
		int64_t bottom64 = (int64_t)glyph->y + glyph->height - 1u;
		int32_t left = (glyph->x > clipped.x1) ? glyph->x : clipped.x1;
		int32_t top = (glyph->y > clipped.y1) ? glyph->y : clipped.y1;
		int32_t right = (right64 < clipped.x2) ? (int32_t)right64 : clipped.x2;
		int32_t bottom = (bottom64 < clipped.y2) ? (int32_t)bottom64 : clipped.y2;
		uint32_t tile_y;

		if ((left > right) || (top > bottom))
			continue;
		for (tile_y = (uint32_t)top / FB_GFX3_VK_GLYPH_TILE_SIZE;
		     tile_y <= (uint32_t)bottom / FB_GFX3_VK_GLYPH_TILE_SIZE;
		     ++tile_y) {
			uint32_t tile_x;

			for (tile_x = (uint32_t)left / FB_GFX3_VK_GLYPH_TILE_SIZE;
			     tile_x <= (uint32_t)right / FB_GFX3_VK_GLYPH_TILE_SIZE;
			     ++tile_x) {
				uint32_t tile;

				if ((tile_x < first_tile_x) || (tile_x > last_tile_x) ||
				    (tile_y < first_tile_y) || (tile_y > last_tile_y)) {
					result = FB_GFX3_INVALID;
					goto cleanup;
				}
				tile = (tile_y - first_tile_y) * tiles_x +
					(tile_x - first_tile_x);
				if ((tile >= tile_count) ||
				    (tile_cursors[tile] >= index_count)) {
					result = FB_GFX3_INVALID;
					goto cleanup;
				}
				tile_indices[tile_cursors[tile]++] = index;
			}
		}
	}
	/*
		Reserve the maximum glyph and index capacities on first use. The range
		buffer follows the surface dimensions, so subsequent packets on the same
		or a smaller page need no Vulkan allocation calls.
	*/
	command_capacity = offsetof(FB_GFX3_VULKAN_GLYPH_COMMAND, glyph) +
		((uint64_t)FB_GFX3_VK_GLYPH_BATCH_LIMIT * sizeof(glyphs[0]));
	/* An unaligned glyph can touch at most four 16 by 16 tiles. */
	index_capacity = (uint64_t)FB_GFX3_VK_GLYPH_BATCH_LIMIT * 4u *
		sizeof(tile_indices[0]);
	range_capacity = (((uint64_t)storage->width +
		FB_GFX3_VK_GLYPH_TILE_SIZE - 1u) /
		FB_GFX3_VK_GLYPH_TILE_SIZE) *
		(((uint64_t)storage->height + FB_GFX3_VK_GLYPH_TILE_SIZE - 1u) /
		FB_GFX3_VK_GLYPH_TILE_SIZE) * 2u *
		sizeof(tile_ranges[0]);
	slot = &implementation->submission_slots[implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->glyph_command_buffer, command_capacity);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->glyph_range_buffer, range_capacity);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->glyph_index_buffer, index_capacity);
	if (result != FB_GFX3_OK)
		goto cleanup;
	command_buffer = &slot->glyph_command_buffer;
	range_buffer = &slot->glyph_range_buffer;
	index_buffer = &slot->glyph_index_buffer;
	memset(command_buffer->mapped, 0, command_size);
	command = (FB_GFX3_VULKAN_GLYPH_COMMAND *)command_buffer->mapped;
	command->surface[0] = storage->width;
	command->surface[1] = storage->height;
	command->surface[2] = fb_gfx3_vk_surface_color_mask(storage->depth);
	command->surface[3] = glyph_count;
	command->clip = clipped;
	command->tile_grid[0] = first_tile_x;
	command->tile_grid[1] = first_tile_y;
	command->tile_grid[2] = tiles_x;
	command->tile_grid[3] = tiles_y;
	memcpy(command->glyph, glyphs, glyph_bytes);
	memcpy(range_buffer->mapped, tile_ranges,
		(size_t)tile_count * 2u * sizeof(tile_ranges[0]));
	memcpy(index_buffer->mapped, tile_indices,
		(size_t)index_count * sizeof(tile_indices[0]));
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	buffer_info[0].buffer = storage->storage.buffer;
	buffer_info[0].range = storage->storage.size;
	buffer_info[1].buffer = command_buffer->buffer;
	buffer_info[1].range = command_size;
	buffer_info[2].buffer = range_buffer->buffer;
	buffer_info[2].range = (uint64_t)tile_count * 2u * sizeof(tile_ranges[0]);
	buffer_info[3].buffer = index_buffer->buffer;
	buffer_info[3].range = (uint64_t)index_count * sizeof(tile_indices[0]);
	for (index = 0u; index < 4u; ++index) {
		descriptor_writes[index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[index].destination_set =
			implementation->compute_descriptor_set;
		descriptor_writes[index].destination_binding =
			(index < 2u) ? index : index + 1u;
		descriptor_writes[index].descriptor_count = 1u;
		descriptor_writes[index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[index].buffer_info = &buffer_info[index];
	}
	implementation->update_descriptor_sets(implementation->device, 4u,
		descriptor_writes, 0u, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, range_buffer->buffer,
		(uint64_t)tile_count * 2u * sizeof(tile_ranges[0]),
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, index_buffer->buffer,
		(uint64_t)index_count * sizeof(tile_indices[0]),
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->glyph_tile_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0u, 1u,
		&implementation->compute_descriptor_set, 0u, NULL);
	implementation->command_dispatch(implementation->command_buffer,
		tiles_x, tiles_y, 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	free(tile_indices);
	free(tile_cursors);
	free(tile_ranges);
	free(tile_counts);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Lines and rectangles                                                     */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vulkan_surface_line(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	uint32_t color, uint32_t style, uint32_t flags)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION command_buffer;
	FB_GFX3_VULKAN_LINE_COMMAND *line_command;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[2];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[2];
	FB_GFX3_RECT clipped;
	int64_t difference_x;
	int64_t difference_y;
	uint32_t point_count;
	uint32_t group_count_x;
	int result;

	memset(&command_buffer, 0, sizeof(command_buffer));
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if ((result != FB_GFX3_OK) || (clip == NULL))
		return FB_GFX3_INVALID;
	difference_x = (int64_t)x2 - x1;
	difference_y = (int64_t)y2 - y1;
	if (difference_x < 0)
		difference_x = -difference_x;
	if (difference_y < 0)
		difference_y = -difference_y;
	/*
		The compute shader uses signed 32-bit products when calculating the
		minor coordinate. Matching the OpenGL backend's 32767-step limit
		keeps those products defined for every accepted line.
	*/
	if ((difference_x > 32767) || (difference_y > 32767))
		return FB_GFX3_UNSUPPORTED;
	point_count = (uint32_t)((difference_x > difference_y) ?
		difference_x : difference_y) + 1u;
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)surface->width)
		clipped.x2 = (int32_t)surface->width - 1;
	if (clipped.y2 >= (int32_t)surface->height)
		clipped.y2 = (int32_t)surface->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2) ||
	    ((x1 < clipped.x1) && (x2 < clipped.x1)) ||
	    ((x1 > clipped.x2) && (x2 > clipped.x2)) ||
	    ((y1 < clipped.y1) && (y2 < clipped.y1)) ||
	    ((y1 > clipped.y2) && (y2 > clipped.y2)))
		return FB_GFX3_OK;

	result = fb_gfx3_vk_buffer_allocation_create(implementation,
		&command_buffer, sizeof(*line_command),
		FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	line_command = (FB_GFX3_VULKAN_LINE_COMMAND *)command_buffer.mapped;
	line_command->endpoints[0] = x1;
	line_command->endpoints[1] = y1;
	line_command->endpoints[2] = x2;
	line_command->endpoints[3] = y2;
	line_command->clip = clipped;
	line_command->parameters[0] = surface->width;
	line_command->parameters[1] = point_count;
	line_command->parameters[2] = color;
	line_command->parameters[3] = style & 0xFFFFu;
	line_command->format[0] = fb_gfx3_vk_surface_color_mask(surface->depth);
	line_command->format[1] = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	line_command->format[2] = 0;
	line_command->format[3] = 0;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;

	memset(buffer_info, 0, sizeof(buffer_info));
	buffer_info[0].buffer = storage->storage.buffer;
	buffer_info[0].range = storage->storage.size;
	buffer_info[1].buffer = command_buffer.buffer;
	buffer_info[1].range = command_buffer.size;
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	descriptor_writes[0].structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].destination_set =
		implementation->compute_descriptor_set;
	descriptor_writes[0].descriptor_count = 1;
	descriptor_writes[0].descriptor_type =
		FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[0].buffer_info = &buffer_info[0];
	descriptor_writes[1] = descriptor_writes[0];
	descriptor_writes[1].destination_binding = 1;
	descriptor_writes[1].buffer_info = &buffer_info[1];
	implementation->update_descriptor_sets(implementation->device, 2,
		descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->line_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&implementation->compute_descriptor_set, 0, NULL);
	group_count_x = (point_count - 1u) / 64u + 1u;
	implementation->command_dispatch(implementation->command_buffer,
		group_count_x, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &command_buffer);
	return result;
}

/*
	Record an adjacent FIFO run of lines in one Vulkan submission.  Each line
	still receives its own descriptor set and dispatch, because line coverage can
	overlap and the later BASIC command must win at those pixels.  The reduction
	is therefore submission and allocation overhead, never a raster-order change.
*/
int fb_gfx3_vulkan_surface_line_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_VULKAN_LINE *lines,
	size_t line_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION command_buffer;
	FB_GFX3_VULKAN_LINE_COMMAND *commands;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 2];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 2];
	uint32_t group_count_x[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t command_size;
	size_t operation_count = 0;
	size_t index;
	int result;

	memset(&command_buffer, 0, sizeof(command_buffer));
	if ((lines == NULL) || (line_count < 2) ||
		(line_count > FB_GFX3_VK_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (fb_gfx3_vk_size_multiply(line_count, sizeof(*commands), &command_size) !=
		FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;
	result = fb_gfx3_vk_buffer_allocation_create(implementation, &command_buffer,
		command_size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	commands = (FB_GFX3_VULKAN_LINE_COMMAND *)command_buffer.mapped;

	for (index = 0; index < line_count; index++) {
		const FB_GFX3_VULKAN_LINE *line = &lines[index];
		FB_GFX3_RECT clipped = line->clip;
		int64_t difference_x = (int64_t)line->x2 - line->x1;
		int64_t difference_y = (int64_t)line->y2 - line->y1;
		uint32_t point_count;

		if (difference_x < 0)
			difference_x = -difference_x;
		if (difference_y < 0)
			difference_y = -difference_y;
		if ((difference_x > 32767) || (difference_y > 32767)) {
			result = FB_GFX3_UNSUPPORTED;
			goto cleanup;
		}
		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)surface->width)
			clipped.x2 = (int32_t)surface->width - 1;
		if (clipped.y2 >= (int32_t)surface->height)
			clipped.y2 = (int32_t)surface->height - 1;
		if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2) ||
			((line->x1 < clipped.x1) && (line->x2 < clipped.x1)) ||
			((line->x1 > clipped.x2) && (line->x2 > clipped.x2)) ||
			((line->y1 < clipped.y1) && (line->y2 < clipped.y1)) ||
			((line->y1 > clipped.y2) && (line->y2 > clipped.y2)))
			continue;
		point_count = (uint32_t)((difference_x > difference_y) ?
			difference_x : difference_y) + 1u;
		commands[operation_count].endpoints[0] = line->x1;
		commands[operation_count].endpoints[1] = line->y1;
		commands[operation_count].endpoints[2] = line->x2;
		commands[operation_count].endpoints[3] = line->y2;
		commands[operation_count].clip = clipped;
		commands[operation_count].parameters[0] = surface->width;
		commands[operation_count].parameters[1] = point_count;
		commands[operation_count].parameters[2] = line->color;
		commands[operation_count].parameters[3] = line->style & 0xFFFFu;
		commands[operation_count].format[0] =
			fb_gfx3_vk_surface_color_mask(surface->depth);
		commands[operation_count].format[1] =
			line->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
		commands[operation_count].format[2] = 0;
		commands[operation_count].format[3] = 0;
		group_count_x[operation_count] = (point_count - 1u) / 64u + 1u;
		operation_count++;
	}
	if (operation_count == 0) {
		result = FB_GFX3_OK;
		goto cleanup;
	}
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	for (index = 0; index < operation_count; index++) {
		size_t write_index = index * 2;

		buffer_info[write_index].buffer = storage->storage.buffer;
		buffer_info[write_index].range = storage->storage.size;
		buffer_info[write_index + 1].buffer = command_buffer.buffer;
		buffer_info[write_index + 1].offset = index * sizeof(*commands);
		buffer_info[write_index + 1].range = sizeof(*commands);
		descriptor_writes[write_index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[write_index].destination_set =
			implementation->submission_slots[
			implementation->next_submission_slot].descriptor_sets[index];
		descriptor_writes[write_index].descriptor_count = 1;
		descriptor_writes[write_index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[write_index].buffer_info = &buffer_info[write_index];
		descriptor_writes[write_index + 1] = descriptor_writes[write_index];
		descriptor_writes[write_index + 1].destination_binding = 1;
		descriptor_writes[write_index + 1].buffer_info =
			&buffer_info[write_index + 1];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(operation_count * 2), descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, implementation->line_pipeline);
	for (index = 0; index < operation_count; index++) {
		FB_GFX3_VK_DESCRIPTOR_SET descriptor_set =
			implementation->submission_slots[
			implementation->active_submission_slot].descriptor_sets[index];

		implementation->command_bind_descriptor_sets(implementation->command_buffer,
			FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
			implementation->compute_pipeline_layout, 0, 1, &descriptor_set, 0,
			NULL);
		implementation->command_dispatch(implementation->command_buffer,
			group_count_x[index], 1, 1);
		fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
			storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &command_buffer);
	return result;
}

int fb_gfx3_vulkan_surface_rectangle(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_RECTANGLE_COMMAND rectangle_command;
	FB_GFX3_RECT clipped;
	int64_t width64;
	int64_t height64;
	uint32_t width;
	uint32_t height;
	uint32_t point_count;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if ((result != FB_GFX3_OK) || (clip == NULL) ||
	    (x1 > x2) || (y1 > y2))
		return FB_GFX3_INVALID;
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)surface->width)
		clipped.x2 = (int32_t)surface->width - 1;
	if (clipped.y2 >= (int32_t)surface->height)
		clipped.y2 = (int32_t)surface->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2) ||
	    (x2 < clipped.x1) || (y2 < clipped.y1) ||
	    (x1 > clipped.x2) || (y1 > clipped.y2))
		return FB_GFX3_OK;
	if (filled != 0) {
		if (x1 > clipped.x1)
			clipped.x1 = x1;
		if (y1 > clipped.y1)
			clipped.y1 = y1;
		if (x2 < clipped.x2)
			clipped.x2 = x2;
		if (y2 < clipped.y2)
			clipped.y2 = y2;
		if ((flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) == 0)
			return fb_gfx3_vulkan_surface_clear(runtime, surface,
				clipped.x1, clipped.y1, clipped.x2, clipped.y2, color);
		x1 = clipped.x1;
		y1 = clipped.y1;
		x2 = clipped.x2;
		y2 = clipped.y2;
	}

	width64 = (int64_t)x2 - x1 + 1;
	height64 = (int64_t)y2 - y1 + 1;
	if ((width64 > 32767) || (height64 > 32767))
		return FB_GFX3_UNSUPPORTED;
	width = (uint32_t)width64;
	height = (uint32_t)height64;
	if (filled != 0) {
		if ((height != 0) && (width > UINT32_MAX / height))
			return FB_GFX3_UNSUPPORTED;
		point_count = width * height;
	} else {
		point_count = (width + height) * 2u;
	}
	memset(&rectangle_command, 0, sizeof(rectangle_command));
	rectangle_command.box[0] = x1;
	rectangle_command.box[1] = y1;
	rectangle_command.box[2] = x2;
	rectangle_command.box[3] = y2;
	rectangle_command.clip = clipped;
	rectangle_command.dimensions[0] = surface->width;
	rectangle_command.dimensions[1] = width;
	rectangle_command.dimensions[2] = height;
	rectangle_command.dimensions[3] = point_count;
	rectangle_command.parameters[0] = color;
	rectangle_command.parameters[1] = style & 0xFFFFu;
	rectangle_command.parameters[2] =
		fb_gfx3_vk_surface_color_mask(surface->depth);
	rectangle_command.parameters[3] =
		(flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) |
		((filled != 0) ? 0x80000000u : 0u);
	return fb_gfx3_vk_surface_dispatch_command(runtime, implementation, storage,
		&rectangle_command, sizeof(rectangle_command),
		implementation->rectangle_pipeline, point_count, 1u);
}

/*
	A same-colour opaque run has no observable overlap order: every covered
	pixel receives the same native value. Execute it as a two-dimensional
	compute dispatch instead of recording a transfer-engine fill for every row.
	The y workgroup indexes one rectangle command, while x covers its pixels.
	This is the hot path produced by a cached uniform FB.IMAGE sprite.
*/
static int vulkan_surface_rectangle_tile_submit(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage,
	FB_GFX3_VULKAN_RECTANGLE_COMMAND *commands, size_t command_count)
{
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *ranges_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *indices_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *tiles_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO infos[5];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET writes[5];
	uint32_t *counts = NULL;
	uint32_t *ranges = NULL;
	uint32_t *cursors = NULL;
	uint32_t *indices = NULL;
	uint32_t *tile_coordinates = NULL;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tiles;
	uint32_t active_tiles = 0u;
	uint32_t index_count = 0u;
	uint32_t index;
	size_t count_size;
	size_t cursor_size;
	size_t index_size;
	size_t range_size;
	size_t tile_coordinate_size;
	size_t scratch_size;
	size_t command_size;
	int result = FB_GFX3_OK;

	if ((runtime == NULL) || (implementation == NULL) || (storage == NULL) ||
	    (commands == NULL) || (command_count < 2u) ||
	    (command_count > FB_GFX3_VK_RECTANGLE_COMPUTE_BATCH_LIMIT) ||
	    (implementation->rectangle_tile_pipeline == 0))
		return FB_GFX3_UNSUPPORTED;
	tiles_x = (storage->width + 15u) / 16u;
	tiles_y = (storage->height + 15u) / 16u;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tiles = tiles_x * tiles_y) > 65536u))
		return FB_GFX3_UNSUPPORTED;
	if (fb_gfx3_vk_size_multiply(tiles, sizeof(*counts), &count_size) !=
	    FB_GFX3_OK)
		return FB_GFX3_INVALID;
	if (implementation->rectangle_tile_count_scratch_size < count_size) {
		void *replacement = realloc(
			implementation->rectangle_tile_count_scratch, count_size);

		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		implementation->rectangle_tile_count_scratch = replacement;
		implementation->rectangle_tile_count_scratch_size = count_size;
	}
	counts = (uint32_t *)implementation->rectangle_tile_count_scratch;
	memset(counts, 0, count_size);
	for (index = 0u; index < (uint32_t)command_count; ++index) {
		const FB_GFX3_VULKAN_RECTANGLE_COMMAND *command = &commands[index];
		int32_t first_x = (command->box[0] > command->clip.x1) ?
			command->box[0] : command->clip.x1;
		int32_t first_y = (command->box[1] > command->clip.y1) ?
			command->box[1] : command->clip.y1;
		int32_t last_x = (command->box[2] < command->clip.x2) ?
			command->box[2] : command->clip.x2;
		int32_t last_y = (command->box[3] < command->clip.y2) ?
			command->box[3] : command->clip.y2;
		uint32_t tile_y;

		if ((first_x > last_x) || (first_y > last_y))
			continue;
		for (tile_y = (uint32_t)first_y / 16u;
		     tile_y <= (uint32_t)last_y / 16u; ++tile_y) {
			uint32_t tile_x;
			for (tile_x = (uint32_t)first_x / 16u;
			     tile_x <= (uint32_t)last_x / 16u; ++tile_x) {
				uint32_t tile = tile_y * tiles_x + tile_x;
				if (counts[tile] == UINT32_MAX) {
					result = FB_GFX3_OUT_OF_MEMORY; goto cleanup;
				}
				counts[tile]++;
			}
		}
	}
	for (index = 0u; index < tiles; ++index) {
		if (counts[index] != 0u)
			active_tiles++;
		if (counts[index] > UINT32_MAX - index_count) {
			result = FB_GFX3_OUT_OF_MEMORY; goto cleanup;
		}
		index_count += counts[index];
	}
	if (index_count == 0u)
		goto cleanup;
	/*
		Vulkan 1.0 guarantees at least 65535 workgroups in one dispatch
		dimension. A surface large enough to touch more tiles falls back to the
		ordinary rectangle path instead of relying on a device-specific limit.
	*/
	if (active_tiles > 65535u) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	if ((fb_gfx3_vk_size_multiply((size_t)active_tiles * 2u, sizeof(*ranges),
	     &range_size) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(tiles, sizeof(*cursors), &cursor_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(index_count, sizeof(*indices), &index_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply((size_t)active_tiles * 2u,
	     sizeof(*tile_coordinates), &tile_coordinate_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(range_size, cursor_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(scratch_size, index_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(scratch_size, tile_coordinate_size, &scratch_size) !=
	     FB_GFX3_OK)) {
		result = FB_GFX3_INVALID;
		goto cleanup;
	}
	if (implementation->rectangle_tile_output_scratch_size < scratch_size) {
		void *replacement = realloc(
			implementation->rectangle_tile_output_scratch, scratch_size);

		if (replacement == NULL) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto cleanup;
		}
		implementation->rectangle_tile_output_scratch = replacement;
		implementation->rectangle_tile_output_scratch_size = scratch_size;
	}
	ranges = (uint32_t *)implementation->rectangle_tile_output_scratch;
	cursors = (uint32_t *)((uint8_t *)ranges + range_size);
	indices = (uint32_t *)((uint8_t *)cursors + cursor_size);
	tile_coordinates = (uint32_t *)((uint8_t *)indices + index_size);
	memset(ranges, 0, range_size);
	memset(cursors, 0, cursor_size);
	{
		uint32_t cursor = 0u;
		uint32_t active_tile = 0u;

		for (index = 0u; index < tiles; ++index) {
			if (counts[index] == 0u)
				continue;
			ranges[active_tile * 2u] = cursor;
			ranges[active_tile * 2u + 1u] = counts[index];
			cursors[index] = cursor;
			tile_coordinates[active_tile * 2u] = index % tiles_x;
			tile_coordinates[active_tile * 2u + 1u] = index / tiles_x;
			cursor += counts[index];
			active_tile++;
		}
	}
	for (index = 0u; index < (uint32_t)command_count; ++index) {
		const FB_GFX3_VULKAN_RECTANGLE_COMMAND *command = &commands[index];
		int32_t first_x = (command->box[0] > command->clip.x1) ?
			command->box[0] : command->clip.x1;
		int32_t first_y = (command->box[1] > command->clip.y1) ?
			command->box[1] : command->clip.y1;
		int32_t last_x = (command->box[2] < command->clip.x2) ?
			command->box[2] : command->clip.x2;
		int32_t last_y = (command->box[3] < command->clip.y2) ?
			command->box[3] : command->clip.y2;
		uint32_t tile_y;

		if ((first_x > last_x) || (first_y > last_y))
			continue;
		for (tile_y = (uint32_t)first_y / 16u;
		     tile_y <= (uint32_t)last_y / 16u; ++tile_y) {
			uint32_t tile_x;
			for (tile_x = (uint32_t)first_x / 16u;
		     tile_x <= (uint32_t)last_x / 16u; ++tile_x)
				indices[cursors[tile_y * tiles_x + tile_x]++] = index;
		}
	}
	for (index = 0u; index < (uint32_t)command_count; ++index)
		commands[index].dimensions[3] = storage->height;
	slot = &implementation->submission_slots[implementation->next_submission_slot];
	if (slot->pending_submission &&
	    (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED; goto cleanup;
	}
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED; goto cleanup;
	}
	command_size = command_count * sizeof(*commands);
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK) goto cleanup;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_range_buffer,
		(uint64_t)active_tiles * 2u * sizeof(*ranges));
	if (result != FB_GFX3_OK) goto cleanup;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_index_buffer,
		(uint64_t)index_count * sizeof(*indices));
	if (result != FB_GFX3_OK) goto cleanup;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_tile_buffer, (uint64_t)tile_coordinate_size);
	if (result != FB_GFX3_OK) goto cleanup;
	command_buffer = &slot->rectangle_command_buffer;
	ranges_buffer = &slot->rectangle_range_buffer;
	indices_buffer = &slot->rectangle_index_buffer;
	tiles_buffer = &slot->rectangle_tile_buffer;
	memcpy(command_buffer->mapped, commands, command_size);
	memcpy(ranges_buffer->mapped, ranges,
		(size_t)active_tiles * 2u * sizeof(*ranges));
	memcpy(indices_buffer->mapped, indices,
		(size_t)index_count * sizeof(*indices));
	memcpy(tiles_buffer->mapped, tile_coordinates, tile_coordinate_size);
	memset(infos, 0, sizeof(infos));
	memset(writes, 0, sizeof(writes));
	infos[0].buffer = storage->storage.buffer; infos[0].range = storage->storage.size;
	infos[1].buffer = command_buffer->buffer; infos[1].range = command_size;
	infos[2].buffer = tiles_buffer->buffer; infos[2].range = tile_coordinate_size;
	infos[3].buffer = ranges_buffer->buffer; infos[3].range = ranges_buffer->size;
	infos[4].buffer = indices_buffer->buffer; infos[4].range = indices_buffer->size;
	for (index = 0u; index < 5u; ++index) {
		writes[index].structure_type = FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[index].destination_set = slot->descriptor_sets[0];
		writes[index].destination_binding = index;
		writes[index].descriptor_count = 1;
		writes[index].descriptor_type = FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[index].buffer_info = &infos[index];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK) goto cleanup;
	implementation->update_descriptor_sets(implementation->device, 5, writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, tiles_buffer->buffer,
		tile_coordinate_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, ranges_buffer->buffer, ranges_buffer->size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, indices_buffer->buffer, indices_buffer->size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer, storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, implementation->rectangle_tile_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, implementation->compute_pipeline_layout,
		0, 1, &slot->descriptor_sets[0], 0, NULL);
	implementation->command_dispatch(implementation->command_buffer,
		active_tiles, 1u, 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer, storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK) runtime->completed_submission_count++;

cleanup:
	return result;
}

int fb_gfx3_vulkan_surface_opaque_rectangle_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangles, size_t rectangle_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_RECTANGLE_COMMAND
		commands[FB_GFX3_VK_RECTANGLE_COMPUTE_BATCH_LIMIT];
	uint32_t color = 0;
	uint32_t maximum_points = 0;
	size_t command_size;
	size_t operation_count = 0u;
	size_t index;
	int have_color = FALSE;
	int have_work = FALSE;
	int mixed_colors = FALSE;
	int result;

	if ((rectangles == NULL) || (rectangle_count < 2u) ||
	    (rectangle_count > FB_GFX3_VK_RECTANGLE_COMPUTE_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (implementation->rectangle_pipeline == 0)
		return FB_GFX3_UNSUPPORTED;
	memset(commands, 0, sizeof(commands));
	for (index = 0u; index < rectangle_count; ++index) {
		const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangle =
			&rectangles[index];
		uint64_t width;
		uint64_t height;
		uint64_t point_count;
		uint32_t masked_color;

		if ((rectangle->x1 < 0) || (rectangle->y1 < 0) ||
		    (rectangle->x2 >= (int32_t)storage->width) ||
		    (rectangle->y2 >= (int32_t)storage->height))
			return FB_GFX3_INVALID;
		if ((rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2))
			continue;
		masked_color = rectangle->color &
			fb_gfx3_vk_surface_color_mask(storage->depth);
		if (!have_color) {
			color = masked_color;
			have_color = TRUE;
		} else if (masked_color != color)
			mixed_colors = TRUE;
		width = (uint64_t)(uint32_t)(rectangle->x2 - rectangle->x1) + 1u;
		height = (uint64_t)(uint32_t)(rectangle->y2 - rectangle->y1) + 1u;
		point_count = width * height;
		if (point_count > UINT32_MAX)
			return FB_GFX3_INVALID;
		commands[operation_count].box[0] = rectangle->x1;
		commands[operation_count].box[1] = rectangle->y1;
		commands[operation_count].box[2] = rectangle->x2;
		commands[operation_count].box[3] = rectangle->y2;
		commands[operation_count].clip.x1 = rectangle->x1;
		commands[operation_count].clip.y1 = rectangle->y1;
		commands[operation_count].clip.x2 = rectangle->x2;
		commands[operation_count].clip.y2 = rectangle->y2;
		commands[operation_count].dimensions[0] = storage->width;
		commands[operation_count].dimensions[1] = (uint32_t)width;
		commands[operation_count].dimensions[2] = (uint32_t)height;
		commands[operation_count].dimensions[3] = (uint32_t)point_count;
		commands[operation_count].parameters[0] = masked_color;
		commands[operation_count].parameters[1] = 0xFFFFu;
		commands[operation_count].parameters[2] =
			fb_gfx3_vk_surface_color_mask(storage->depth);
		commands[operation_count].parameters[3] = 0x80000000u;
		if ((uint32_t)point_count > maximum_points)
			maximum_points = (uint32_t)point_count;
		have_work = TRUE;
		operation_count++;
	}
	if (!have_work)
		return FB_GFX3_OK;
	if (mixed_colors) {
		result = vulkan_surface_rectangle_tile_submit(runtime, implementation,
			storage, commands, operation_count);
		if (result != FB_GFX3_UNSUPPORTED)
			return result;
	}
	if (fb_gfx3_vk_size_multiply(operation_count, sizeof(commands[0]),
		&command_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;
	return fb_gfx3_vk_surface_dispatch_command(runtime, implementation, storage,
		commands, command_size, implementation->rectangle_pipeline,
		maximum_points, (uint32_t)operation_count);
}

int fb_gfx3_vulkan_surface_rectangle_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_RECTANGLE *rectangles, size_t rectangle_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_RECTANGLE_COMMAND
		commands[FB_GFX3_VK_RECTANGLE_COMPUTE_BATCH_LIMIT];
	size_t operation_count = 0u;
	size_t index;
	int result;

	if ((rectangles == NULL) || (rectangle_count == 0u) ||
	    (rectangle_count > FB_GFX3_VK_RECTANGLE_COMPUTE_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (implementation->rectangle_tile_pipeline == 0)
		return FB_GFX3_UNSUPPORTED;
	memset(commands, 0, sizeof(commands));
	for (index = 0u; index < rectangle_count; ++index) {
		const FB_GFX3_VULKAN_RECTANGLE *rectangle = &rectangles[index];
		FB_GFX3_VULKAN_RECTANGLE_COMMAND *command;
		FB_GFX3_RECT clipped = rectangle->clip;
		int64_t width;
		int64_t height;

		if ((rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2) ||
		    ((rectangle->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0u))
			return FB_GFX3_INVALID;
		width = (int64_t)rectangle->x2 - rectangle->x1 + 1;
		height = (int64_t)rectangle->y2 - rectangle->y1 + 1;
		if ((width <= 0) || (width > 32767) ||
		    (height <= 0) || (height > 32767))
			return FB_GFX3_UNSUPPORTED;
		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)storage->width)
			clipped.x2 = (int32_t)storage->width - 1;
		if (clipped.y2 >= (int32_t)storage->height)
			clipped.y2 = (int32_t)storage->height - 1;
		if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2) ||
		    (rectangle->x2 < clipped.x1) ||
		    (rectangle->y2 < clipped.y1) ||
		    (rectangle->x1 > clipped.x2) ||
		    (rectangle->y1 > clipped.y2))
			continue;
		command = &commands[operation_count++];
		command->box[0] = rectangle->x1;
		command->box[1] = rectangle->y1;
		command->box[2] = rectangle->x2;
		command->box[3] = rectangle->y2;
		command->clip = clipped;
		command->dimensions[0] = storage->width;
		command->dimensions[1] = (uint32_t)width;
		command->dimensions[2] = (uint32_t)height;
		command->dimensions[3] = storage->height;
		command->parameters[0] = rectangle->color &
			fb_gfx3_vk_surface_color_mask(storage->depth);
		command->parameters[1] = rectangle->style & 0xFFFFu;
		command->parameters[2] =
			fb_gfx3_vk_surface_color_mask(storage->depth);
		command->parameters[3] =
			(rectangle->filled != 0u) ? 0x80000000u : 0u;
	}
	if (operation_count == 0u)
		return FB_GFX3_OK;
	result = vulkan_surface_rectangle_tile_submit(runtime, implementation,
		storage, commands, operation_count);
	if (result != FB_GFX3_UNSUPPORTED)
		return result;
	for (index = 0u; index < rectangle_count; ++index) {
		const FB_GFX3_VULKAN_RECTANGLE *rectangle = &rectangles[index];

		result = fb_gfx3_vulkan_surface_rectangle(runtime, surface,
			&rectangle->clip, rectangle->x1, rectangle->y1,
			rectangle->x2, rectangle->y2, rectangle->color,
			rectangle->style, rectangle->filled != 0u, rectangle->flags);
		if (result != FB_GFX3_OK)
			return result;
	}
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Ellipses and ordered mixed primitives                                    */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vulkan_surface_ellipse(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t center_x, int32_t center_y, float radius_x, float radius_y,
	uint32_t color, int filled, uint32_t flags)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_ELLIPSE_COMMAND ellipse_command;
	FB_GFX3_RECT clipped;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if ((result != FB_GFX3_OK) || (clip == NULL) ||
	    !(radius_x >= 0.0f) || !(radius_x <= 32767.0f) ||
	    !(radius_y >= 0.0f) || !(radius_y <= 32767.0f))
		return FB_GFX3_INVALID;
	if (implementation->ellipse_pipeline == 0)
		return FB_GFX3_UNSUPPORTED;
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)surface->width)
		clipped.x2 = (int32_t)surface->width - 1;
	if (clipped.y2 >= (int32_t)surface->height)
		clipped.y2 = (int32_t)surface->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
		return FB_GFX3_OK;
	memset(&ellipse_command, 0, sizeof(ellipse_command));
	ellipse_command.center[0] = center_x;
	ellipse_command.center[1] = center_y;
	ellipse_command.center[2] = (int32_t)surface->width;
	ellipse_command.clip = clipped;
	ellipse_command.radii[0] = radius_x;
	ellipse_command.radii[1] = radius_y;
	/* The existing ellipse shader reserves radii.z and radii.w. */
	ellipse_command.radii[2] = (float)surface->width;
	ellipse_command.radii[3] = (float)surface->height;
	ellipse_command.parameters[0] = color;
	ellipse_command.parameters[1] = filled != 0;
	ellipse_command.parameters[2] =
		fb_gfx3_vk_surface_color_mask(surface->depth);
	ellipse_command.parameters[3] = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	return fb_gfx3_vk_surface_dispatch_command(runtime, implementation, storage,
		&ellipse_command, sizeof(ellipse_command),
		implementation->ellipse_pipeline, 1u, 1u);
}

/*
	An opaque filled ellipse does not depend on the destination surface. The
	winner pass consequently records the final public command for each covered
	pixel, then the resolve pass writes that colour once. This retains midpoint
	coverage and Basic draw order while avoiding a storage-buffer dependency for
	every individual ellipse.
*/
static int vulkan_surface_ellipse_winner_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage,
	const FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer,
	size_t command_count)
{
	FB_GFX3_VULKAN_BUFFER_ALLOCATION winner_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[3];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[3];
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	uint32_t groups_x;
	uint32_t groups_y;
	int result;

	memset(&winner_buffer, 0, sizeof(winner_buffer));
	/*
		The RTX path completes the unordered atomic winner image quickly. Intel's
		current Windows Vulkan driver makes that global atomic workload progress
		too slowly for an interactive renderer, although it supports the normal
		ordered Float64 ellipse shader. Keep that portable fallback until the
		atomic path has been validated on additional driver families.

		0x10DE is the PCI vendor identifier assigned to NVIDIA.
	*/
	if (runtime->selected_vendor_id != 0x10DEu)
		return FB_GFX3_UNSUPPORTED;
	if ((command_count == 0u) ||
	    (implementation->ellipse_winner_pipeline == 0) ||
	    (implementation->ellipse_resolve_pipeline == 0))
		return FB_GFX3_UNSUPPORTED;
	if ((storage->width == 0u) || (storage->height == 0u))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_buffer_allocation_create(implementation, &winner_buffer,
		storage->storage.size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	slot = &implementation->submission_slots[
		implementation->active_submission_slot];
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	buffer_info[0].buffer = winner_buffer.buffer;
	buffer_info[0].range = winner_buffer.size;
	buffer_info[1].buffer = command_buffer->buffer;
	buffer_info[1].range = command_buffer->size;
	buffer_info[2].buffer = storage->storage.buffer;
	buffer_info[2].range = storage->storage.size;
	for (uint32_t index = 0u; index < 3u; index++) {
		descriptor_writes[index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[index].descriptor_count = 1;
		descriptor_writes[index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
	/* Winner selection consumes bindings zero and one in descriptor set zero. */
	descriptor_writes[0].destination_set = slot->descriptor_sets[0];
	descriptor_writes[0].destination_binding = 0;
	descriptor_writes[0].buffer_info = &buffer_info[0];
	descriptor_writes[1].destination_set = slot->descriptor_sets[0];
	descriptor_writes[1].destination_binding = 1;
	descriptor_writes[1].buffer_info = &buffer_info[1];
	/* Resolve requires a separate set because Vulkan records descriptor bindings. */
	descriptor_writes[2].destination_set = slot->descriptor_sets[1];
	descriptor_writes[2].destination_binding = 0;
	descriptor_writes[2].buffer_info = &buffer_info[2];
	implementation->update_descriptor_sets(implementation->device, 2,
		descriptor_writes, 0, NULL);
	descriptor_writes[0].destination_set = slot->descriptor_sets[1];
	descriptor_writes[0].destination_binding = 1;
	descriptor_writes[0].buffer_info = &buffer_info[0];
	descriptor_writes[1].destination_set = slot->descriptor_sets[1];
	descriptor_writes[1].destination_binding = 2;
	descriptor_writes[1].buffer_info = &buffer_info[1];
	implementation->update_descriptor_sets(implementation->device, 2,
		descriptor_writes, 0, NULL);
	implementation->command_fill_buffer(implementation->command_buffer,
		winner_buffer.buffer, 0, winner_buffer.size, 0u);
	fb_gfx3_vk_record_buffer_barrier(implementation, winner_buffer.buffer,
		winner_buffer.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		command_buffer->size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->ellipse_winner_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&slot->descriptor_sets[0], 0, NULL);
	implementation->command_dispatch(implementation->command_buffer, 1,
		(uint32_t)command_count, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, winner_buffer.buffer,
		winner_buffer.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	groups_x = (storage->width + 15u) / 16u;
	groups_y = (storage->height + 15u) / 16u;
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->ellipse_resolve_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&slot->descriptor_sets[1], 0, NULL);
	implementation->command_dispatch(implementation->command_buffer, groups_x,
		groups_y, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &winner_buffer);
	return result;
}

/*
	Return the visible conservative bounds and useful 64-pixel workgroup count
	for one mixed primitive. The host performs no coverage or style evaluation;
	it only builds scheduling metadata and a bounded resolve rectangle.
*/
static int vulkan_primitive_measure(
	const FB_GFX3_VULKAN_PRIMITIVE *primitive,
	uint32_t surface_width, uint32_t surface_height,
	uint32_t *visible_x1, uint32_t *visible_y1,
	uint32_t *visible_x2, uint32_t *visible_y2,
	uint32_t *workgroup_count)
{
	FB_GFX3_RECT clipped;
	int64_t left;
	int64_t top;
	int64_t right;
	int64_t bottom;
	uint64_t coverage = 1u;

	if ((primitive == NULL) || (surface_width == 0u) ||
	    (surface_height == 0u) || (visible_x1 == NULL) ||
	    (visible_y1 == NULL) || (visible_x2 == NULL) ||
	    (visible_y2 == NULL) || (workgroup_count == NULL))
		return FB_GFX3_INVALID;
	if (primitive->parameters[1] == FB_GFX3_VULKAN_PRIMITIVE_LINE) {
		int64_t difference_x =
			(int64_t)primitive->geometry[2] - primitive->geometry[0];
		int64_t difference_y =
			(int64_t)primitive->geometry[3] - primitive->geometry[1];

		if (difference_x < 0)
			difference_x = -difference_x;
		if (difference_y < 0)
			difference_y = -difference_y;
		if ((difference_x > 32767) || (difference_y > 32767))
			return FB_GFX3_UNSUPPORTED;
		coverage = (uint64_t)((difference_x > difference_y) ?
			difference_x : difference_y) + 1u;
		left = (primitive->geometry[0] < primitive->geometry[2]) ?
			primitive->geometry[0] : primitive->geometry[2];
		right = (primitive->geometry[0] > primitive->geometry[2]) ?
			primitive->geometry[0] : primitive->geometry[2];
		top = (primitive->geometry[1] < primitive->geometry[3]) ?
			primitive->geometry[1] : primitive->geometry[3];
		bottom = (primitive->geometry[1] > primitive->geometry[3]) ?
			primitive->geometry[1] : primitive->geometry[3];
	} else if (primitive->parameters[1] ==
	    FB_GFX3_VULKAN_PRIMITIVE_ELLIPSE) {
		float radius_x;
		float radius_y;
		int64_t radius_bound_x;
		int64_t radius_bound_y;

		memcpy(&radius_x, &primitive->geometry[2], sizeof(radius_x));
		memcpy(&radius_y, &primitive->geometry[3], sizeof(radius_y));
		if (!(radius_x >= 0.0f) || !(radius_x <= 32767.0f) ||
		    !(radius_y >= 0.0f) || !(radius_y <= 32767.0f))
			return FB_GFX3_INVALID;
		radius_bound_x = (int64_t)radius_x;
		radius_bound_y = (int64_t)radius_y;
		if ((float)radius_bound_x < radius_x)
			radius_bound_x++;
		if ((float)radius_bound_y < radius_y)
			radius_bound_y++;
		left = (int64_t)primitive->geometry[0] - radius_bound_x;
		right = (int64_t)primitive->geometry[0] + radius_bound_x;
		top = (int64_t)primitive->geometry[1] - radius_bound_y;
		bottom = (int64_t)primitive->geometry[1] + radius_bound_y;
	} else if (primitive->parameters[1] ==
	    FB_GFX3_VULKAN_PRIMITIVE_POINT) {
		left = primitive->geometry[0];
		right = primitive->geometry[0];
		top = primitive->geometry[1];
		bottom = primitive->geometry[1];
	} else if (primitive->parameters[1] ==
	    FB_GFX3_VULKAN_PRIMITIVE_RECTANGLE) {
		uint64_t width;
		uint64_t height;

		if ((primitive->geometry[0] > primitive->geometry[2]) ||
		    (primitive->geometry[1] > primitive->geometry[3]))
			return FB_GFX3_INVALID;
		width = (uint64_t)((int64_t)primitive->geometry[2] -
			primitive->geometry[0]) + 1u;
		height = (uint64_t)((int64_t)primitive->geometry[3] -
			primitive->geometry[1]) + 1u;
		if ((width > 32767u) || (height > 32767u))
			return FB_GFX3_UNSUPPORTED;
		if ((primitive->parameters[2] & 0x80000000u) != 0u)
			coverage = width * height;
		else
			coverage = (width + height) * 2u;
		left = primitive->geometry[0];
		right = primitive->geometry[2];
		top = primitive->geometry[1];
		bottom = primitive->geometry[3];
	} else {
		return FB_GFX3_INVALID;
	}
	clipped = primitive->clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)surface_width)
		clipped.x2 = (int32_t)surface_width - 1;
	if (clipped.y2 >= (int32_t)surface_height)
		clipped.y2 = (int32_t)surface_height - 1;
	if (left < clipped.x1)
		left = clipped.x1;
	if (top < clipped.y1)
		top = clipped.y1;
	if (right > clipped.x2)
		right = clipped.x2;
	if (bottom > clipped.y2)
		bottom = clipped.y2;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2) ||
	    (left > right) || (top > bottom)) {
		*workgroup_count = 0u;
		return FB_GFX3_OK;
	}
	*visible_x1 = (uint32_t)left;
	*visible_y1 = (uint32_t)top;
	*visible_x2 = (uint32_t)right;
	*visible_y2 = (uint32_t)bottom;
	*workgroup_count = (uint32_t)((coverage + 63u) / 64u);
	return FB_GFX3_OK;
}

/*
	Mixed primitive tile submission

	When rectangles are present, a per-pixel atomic winner is needlessly
	expensive for broad panels. Build conservative 16 by 16 tile lists instead.
	Each shader invocation owns one destination pixel and replays only the
	ordered point, line, and rectangle candidates which may touch its tile.
*/
static int vulkan_surface_primitive_tile_submit(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage,
	const FB_GFX3_VULKAN_PRIMITIVE *primitives, size_t primitive_count)
{
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_PRIMITIVE *commands;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *ranges_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *indices_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *tiles_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO infos[5];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET writes[5];
	uint32_t *counts;
	uint32_t *ranges;
	uint32_t *cursors;
	uint32_t *indices;
	uint32_t *tile_coordinates;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tiles;
	uint32_t active_tiles = 0u;
	uint32_t index_count = 0u;
	uint32_t index;
	size_t count_size;
	size_t cursor_size;
	size_t index_size;
	size_t range_size;
	size_t tile_coordinate_size;
	size_t scratch_size;
	size_t command_size;
	int result = FB_GFX3_OK;

	if ((runtime == NULL) || (implementation == NULL) || (storage == NULL) ||
	    (primitives == NULL) || (primitive_count < 2u) ||
	    (primitive_count > FB_GFX3_VK_PRIMITIVE_BATCH_LIMIT) ||
	    (implementation->primitive_tile_pipeline == 0))
		return FB_GFX3_UNSUPPORTED;
	tiles_x = (storage->width + 15u) / 16u;
	tiles_y = (storage->height + 15u) / 16u;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tiles = tiles_x * tiles_y) > 65536u))
		return FB_GFX3_UNSUPPORTED;
	if (fb_gfx3_vk_size_multiply(tiles, sizeof(*counts), &count_size) !=
	    FB_GFX3_OK)
		return FB_GFX3_INVALID;
	if (implementation->rectangle_tile_count_scratch_size < count_size) {
		void *replacement = realloc(
			implementation->rectangle_tile_count_scratch, count_size);

		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		implementation->rectangle_tile_count_scratch = replacement;
		implementation->rectangle_tile_count_scratch_size = count_size;
	}
	counts = (uint32_t *)implementation->rectangle_tile_count_scratch;
	memset(counts, 0, count_size);
	for (index = 0u; index < (uint32_t)primitive_count; index++) {
		const FB_GFX3_VULKAN_PRIMITIVE *primitive = &primitives[index];
		uint32_t first_x;
		uint32_t first_y;
		uint32_t last_x;
		uint32_t last_y;
		uint32_t workgroups;
		uint32_t tile_y;

		if (primitive->parameters[1] ==
		    FB_GFX3_VULKAN_PRIMITIVE_ELLIPSE)
			return FB_GFX3_UNSUPPORTED;
		result = vulkan_primitive_measure(primitive, storage->width,
			storage->height, &first_x, &first_y, &last_x, &last_y,
			&workgroups);
		if (result != FB_GFX3_OK)
			return result;
		if (workgroups == 0u)
			continue;
		for (tile_y = first_y / 16u; tile_y <= last_y / 16u;
		     tile_y++) {
			uint32_t tile_x;

			for (tile_x = first_x / 16u; tile_x <= last_x / 16u;
			     tile_x++) {
				uint32_t tile = tile_y * tiles_x + tile_x;

				if (counts[tile] == UINT32_MAX)
					return FB_GFX3_OUT_OF_MEMORY;
				counts[tile]++;
			}
		}
	}
	for (index = 0u; index < tiles; index++) {
		if (counts[index] != 0u)
			active_tiles++;
		if (counts[index] > UINT32_MAX - index_count)
			return FB_GFX3_OUT_OF_MEMORY;
		index_count += counts[index];
	}
	if (index_count == 0u)
		return FB_GFX3_OK;
	if (active_tiles > 65535u)
		return FB_GFX3_UNSUPPORTED;
	if ((fb_gfx3_vk_size_multiply((size_t)active_tiles * 2u, sizeof(*ranges),
	     &range_size) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(tiles, sizeof(*cursors), &cursor_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(index_count, sizeof(*indices), &index_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply((size_t)active_tiles * 2u,
	     sizeof(*tile_coordinates), &tile_coordinate_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(range_size, cursor_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(scratch_size, index_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(scratch_size, tile_coordinate_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(primitive_count, sizeof(*commands),
	     &command_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	if (implementation->rectangle_tile_output_scratch_size < scratch_size) {
		void *replacement = realloc(
			implementation->rectangle_tile_output_scratch, scratch_size);

		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		implementation->rectangle_tile_output_scratch = replacement;
		implementation->rectangle_tile_output_scratch_size = scratch_size;
	}
	ranges = (uint32_t *)implementation->rectangle_tile_output_scratch;
	cursors = (uint32_t *)((uint8_t *)ranges + range_size);
	indices = (uint32_t *)((uint8_t *)cursors + cursor_size);
	tile_coordinates = (uint32_t *)((uint8_t *)indices + index_size);
	memset(ranges, 0, range_size);
	memset(cursors, 0, cursor_size);
	{
		uint32_t cursor = 0u;
		uint32_t active_tile = 0u;

		for (index = 0u; index < tiles; index++) {
			if (counts[index] == 0u)
				continue;
			ranges[active_tile * 2u] = cursor;
			ranges[active_tile * 2u + 1u] = counts[index];
			cursors[index] = cursor;
			tile_coordinates[active_tile * 2u] = index % tiles_x;
			tile_coordinates[active_tile * 2u + 1u] = index / tiles_x;
			cursor += counts[index];
			active_tile++;
		}
	}
	for (index = 0u; index < (uint32_t)primitive_count; index++) {
		uint32_t first_x;
		uint32_t first_y;
		uint32_t last_x;
		uint32_t last_y;
		uint32_t workgroups;
		uint32_t tile_y;

		result = vulkan_primitive_measure(&primitives[index],
			storage->width, storage->height, &first_x, &first_y,
			&last_x, &last_y, &workgroups);
		if (result != FB_GFX3_OK)
			return result;
		if (workgroups == 0u)
			continue;
		for (tile_y = first_y / 16u; tile_y <= last_y / 16u;
		     tile_y++) {
			uint32_t tile_x;

			for (tile_x = first_x / 16u; tile_x <= last_x / 16u;
			     tile_x++)
				indices[cursors[tile_y * tiles_x + tile_x]++] = index;
		}
	}
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->pending_submission &&
	    (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->primitive_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_range_buffer, (uint64_t)range_size);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_index_buffer, (uint64_t)index_size);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->rectangle_tile_buffer, (uint64_t)tile_coordinate_size);
	if (result != FB_GFX3_OK)
		return result;
	command_buffer = &slot->primitive_command_buffer;
	ranges_buffer = &slot->rectangle_range_buffer;
	indices_buffer = &slot->rectangle_index_buffer;
	tiles_buffer = &slot->rectangle_tile_buffer;
	commands = (FB_GFX3_VULKAN_PRIMITIVE *)command_buffer->mapped;
	for (index = 0u; index < (uint32_t)primitive_count; index++) {
		commands[index] = primitives[index];
		if (commands[index].clip.x1 < 0)
			commands[index].clip.x1 = 0;
		if (commands[index].clip.y1 < 0)
			commands[index].clip.y1 = 0;
		if (commands[index].clip.x2 >= (int32_t)storage->width)
			commands[index].clip.x2 = (int32_t)storage->width - 1;
		if (commands[index].clip.y2 >= (int32_t)storage->height)
			commands[index].clip.y2 = (int32_t)storage->height - 1;
		commands[index].parameters[3] = index + 1u;
		commands[index].format[0] = storage->width;
		commands[index].format[1] = storage->height;
		commands[index].format[2] =
			fb_gfx3_vk_surface_color_mask(storage->depth);
		commands[index].format[3] = 0u;
		memset(commands[index].batch, 0, sizeof(commands[index].batch));
	}
	memcpy(ranges_buffer->mapped, ranges, range_size);
	memcpy(indices_buffer->mapped, indices, index_size);
	memcpy(tiles_buffer->mapped, tile_coordinates, tile_coordinate_size);
	memset(infos, 0, sizeof(infos));
	memset(writes, 0, sizeof(writes));
	infos[0].buffer = storage->storage.buffer;
	infos[0].range = storage->storage.size;
	infos[1].buffer = command_buffer->buffer;
	infos[1].range = command_size;
	infos[2].buffer = tiles_buffer->buffer;
	infos[2].range = tile_coordinate_size;
	infos[3].buffer = ranges_buffer->buffer;
	infos[3].range = range_size;
	infos[4].buffer = indices_buffer->buffer;
	infos[4].range = index_size;
	for (index = 0u; index < 5u; index++) {
		writes[index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[index].destination_set = slot->descriptor_sets[0];
		writes[index].destination_binding = index;
		writes[index].descriptor_count = 1u;
		writes[index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[index].buffer_info = &infos[index];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device, 5u,
		writes, 0u, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, tiles_buffer->buffer,
		tile_coordinate_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, ranges_buffer->buffer,
		range_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, indices_buffer->buffer,
		index_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->primitive_tile_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0u, 1u,
		&slot->descriptor_sets[0], 0u, NULL);
	implementation->command_dispatch(implementation->command_buffer,
		active_tiles, 1u, 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

/*
	Mixed opaque primitive packet

	POINTS, LINE, rectangle, and CIRCLE frequently alternate in software-style
	game renderers. Recording one Vulkan dispatch for every public command leaves
	the work on the GPU but spends most of the frame building and scheduling tiny
	dispatches.

	Ellipse-free packets containing rectangles use 16 by 16 tile replay. Each
	shader invocation owns one destination pixel and applies the tile's FIFO
	candidates, so no atomic winner or second resolve pass is needed. Packets
	without rectangles use the compact atomic winner table on the qualified
	NVIDIA and Intel drivers. A rectangle adjacent to an ellipse retains the
	established exact type-specific fallback instead of approximating midpoint
	coverage in the tile shader.
*/
int fb_gfx3_vulkan_surface_primitive_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_PRIMITIVE *primitives, size_t primitive_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_PRIMITIVE *commands;
	FB_GFX3_VULKAN_PRIMITIVE_WORKGROUP *workgroups;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[4];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[3];
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	size_t command_size;
	size_t workgroup_size;
	size_t index;
	uint32_t resolve_x1 = UINT32_MAX;
	uint32_t resolve_y1 = UINT32_MAX;
	uint32_t resolve_x2 = 0u;
	uint32_t resolve_y2 = 0u;
	uint32_t total_workgroups = 0u;
	uint32_t workgroup_index = 0u;
	uint32_t generation;
	uint32_t groups_x;
	uint32_t groups_y;
	int reset_winner = FALSE;
	int result;

	if ((primitives == NULL) || (primitive_count < 2u) ||
	    (primitive_count > FB_GFX3_VK_PRIMITIVE_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	{
		int has_rectangle = FALSE;
		int has_ellipse = FALSE;

		for (index = 0u; index < primitive_count; index++) {
			if (primitives[index].parameters[1] ==
			    FB_GFX3_VULKAN_PRIMITIVE_RECTANGLE)
				has_rectangle = TRUE;
			else if (primitives[index].parameters[1] ==
			    FB_GFX3_VULKAN_PRIMITIVE_ELLIPSE)
				has_ellipse = TRUE;
		}
		if (has_rectangle) {
			/*
				Ellipses use shared midpoint state and remain on the winner
				path. Do not send rectangles through destination atomics merely
				to bridge that uncommon boundary.
			*/
			if (has_ellipse)
				return FB_GFX3_UNSUPPORTED;
			return vulkan_surface_primitive_tile_submit(runtime,
				implementation, storage, primitives, primitive_count);
		}
	}
	if (((runtime->selected_vendor_id != 0x10DEu) &&
	     (runtime->selected_vendor_id != 0x8086u)) ||
	    (implementation->primitive_winner_pipeline == 0) ||
	    (implementation->primitive_resolve_pipeline == 0))
		return FB_GFX3_UNSUPPORTED;
	if ((surface->width == 0u) || (surface->height == 0u) ||
	    (fb_gfx3_vk_size_multiply(primitive_count, sizeof(*commands),
	     &command_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	/*
		Find only a conservative resolve rectangle and useful workgroup count
		here. Exact coverage, style, and VIEW clipping remain shader work.
	*/
	for (index = 0u; index < primitive_count; index++) {
		const FB_GFX3_VULKAN_PRIMITIVE *source = &primitives[index];
		uint32_t visible_x1;
		uint32_t visible_y1;
		uint32_t visible_x2;
		uint32_t visible_y2;
		uint32_t primitive_workgroups;

		result = vulkan_primitive_measure(source, surface->width,
			surface->height, &visible_x1, &visible_y1, &visible_x2,
			&visible_y2, &primitive_workgroups);
		if (result != FB_GFX3_OK)
			return result;
		if (primitive_workgroups == 0u)
			continue;
		if (primitive_workgroups >
		    FB_GFX3_VK_PRIMITIVE_WORKGROUP_LIMIT - total_workgroups)
			return FB_GFX3_UNSUPPORTED;
		total_workgroups += primitive_workgroups;
		if (visible_x1 < resolve_x1)
			resolve_x1 = visible_x1;
		if (visible_y1 < resolve_y1)
			resolve_y1 = visible_y1;
		if (visible_x2 > resolve_x2)
			resolve_x2 = visible_x2;
		if (visible_y2 > resolve_y2)
			resolve_y2 = visible_y2;
	}
	if ((total_workgroups == 0u) ||
	    (resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2))
		return FB_GFX3_OK;
	if ((resolve_x2 > 0xFFFFu) || (resolve_y2 > 0xFFFFu))
		return FB_GFX3_UNSUPPORTED;
	if (fb_gfx3_vk_size_multiply(total_workgroups, sizeof(*workgroups),
	    &workgroup_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;

	/*
		The next begin call selects this slot. Wait before rewriting mapped
		memory, then retain allocations for later turns through the slot ring.
	*/
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->pending_submission) {
		if (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->primitive_command_buffer.size < command_size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_command_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->primitive_command_buffer,
			(uint64_t)FB_GFX3_VK_PRIMITIVE_BATCH_LIMIT *
				sizeof(*commands),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			return result;
	}
	if (slot->primitive_winner_buffer.size < storage->storage.size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_winner_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->primitive_winner_buffer, storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			return result;
		slot->primitive_generation = 0u;
		reset_winner = TRUE;
	}
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->primitive_workgroup_buffer, (uint64_t)workgroup_size);
	if (result != FB_GFX3_OK)
		return result;
	generation = slot->primitive_generation + 1u;
	if ((generation == 0u) || (generation > (UINT32_MAX >> 13u))) {
		generation = 1u;
		reset_winner = TRUE;
	}
	slot->primitive_generation = generation;
	commands = (FB_GFX3_VULKAN_PRIMITIVE *)
		slot->primitive_command_buffer.mapped;
	for (index = 0u; index < primitive_count; index++) {
		const FB_GFX3_VULKAN_PRIMITIVE *source = &primitives[index];
		FB_GFX3_VULKAN_PRIMITIVE *destination = &commands[index];
		uint32_t visible_x1;
		uint32_t visible_y1;
		uint32_t visible_x2;
		uint32_t visible_y2;
		uint32_t primitive_workgroups;

		*destination = *source;
		result = vulkan_primitive_measure(source, surface->width,
			surface->height, &visible_x1, &visible_y1, &visible_x2,
			&visible_y2, &primitive_workgroups);
		if (result != FB_GFX3_OK)
			return result;
		if (destination->clip.x1 < 0)
			destination->clip.x1 = 0;
		if (destination->clip.y1 < 0)
			destination->clip.y1 = 0;
		if (destination->clip.x2 >= (int32_t)surface->width)
			destination->clip.x2 = (int32_t)surface->width - 1;
		if (destination->clip.y2 >= (int32_t)surface->height)
			destination->clip.y2 = (int32_t)surface->height - 1;
		destination->parameters[3] = (uint32_t)index + 1u;
		destination->format[0] = surface->width;
		destination->format[1] = surface->height;
		destination->format[2] =
			fb_gfx3_vk_surface_color_mask(surface->depth);
		destination->format[3] = 0u;
		destination->batch[0] = generation;
		destination->batch[1] = resolve_x1;
		destination->batch[2] = resolve_y1;
		destination->batch[3] = primitive_workgroups;
	}
	workgroups = (FB_GFX3_VULKAN_PRIMITIVE_WORKGROUP *)
		slot->primitive_workgroup_buffer.mapped;
	for (index = 0u; index < primitive_count; index++) {
		uint32_t group;

		for (group = 0u; group < commands[index].batch[3]; group++) {
			workgroups[workgroup_index].primitive_index = (uint32_t)index;
			workgroups[workgroup_index].first_coverage_index = group * 64u;
			workgroup_index++;
		}
	}
	if (workgroup_index != total_workgroups)
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	buffer_info[0].buffer = slot->primitive_winner_buffer.buffer;
	buffer_info[0].range = storage->storage.size;
	buffer_info[1].buffer = slot->primitive_command_buffer.buffer;
	buffer_info[1].range = command_size;
	buffer_info[2].buffer = slot->primitive_workgroup_buffer.buffer;
	buffer_info[2].range = workgroup_size;
	buffer_info[3].buffer = storage->storage.buffer;
	buffer_info[3].range = storage->storage.size;
	for (uint32_t binding = 0u; binding < 3u; binding++) {
		descriptor_writes[binding].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[binding].descriptor_count = 1u;
		descriptor_writes[binding].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}
	descriptor_writes[0].destination_set = slot->descriptor_sets[0];
	descriptor_writes[0].destination_binding = 0u;
	descriptor_writes[0].buffer_info = &buffer_info[0];
	descriptor_writes[1].destination_set = slot->descriptor_sets[0];
	descriptor_writes[1].destination_binding = 1u;
	descriptor_writes[1].buffer_info = &buffer_info[1];
	descriptor_writes[2].destination_set = slot->descriptor_sets[0];
	descriptor_writes[2].destination_binding = 2u;
	descriptor_writes[2].buffer_info = &buffer_info[2];
	implementation->update_descriptor_sets(implementation->device, 3u,
		descriptor_writes, 0u, NULL);
	descriptor_writes[0].destination_set = slot->descriptor_sets[1];
	descriptor_writes[0].destination_binding = 0u;
	descriptor_writes[0].buffer_info = &buffer_info[3];
	descriptor_writes[1].destination_set = slot->descriptor_sets[1];
	descriptor_writes[1].destination_binding = 1u;
	descriptor_writes[1].buffer_info = &buffer_info[0];
	descriptor_writes[2].destination_set = slot->descriptor_sets[1];
	descriptor_writes[2].destination_binding = 2u;
	descriptor_writes[2].buffer_info = &buffer_info[1];
	implementation->update_descriptor_sets(implementation->device, 3u,
		descriptor_writes, 0u, NULL);
	if (reset_winner) {
		implementation->command_fill_buffer(implementation->command_buffer,
			slot->primitive_winner_buffer.buffer, 0u,
			storage->storage.size, 0u);
		fb_gfx3_vk_record_buffer_barrier(implementation,
			slot->primitive_winner_buffer.buffer, storage->storage.size,
			FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	} else {
		/*
			The generation makes old values harmless, but their writes still
			need to be made available before this submission performs atomics
			against the same storage.
		*/
		fb_gfx3_vk_record_buffer_barrier(implementation,
			slot->primitive_winner_buffer.buffer, storage->storage.size,
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->primitive_command_buffer.buffer, command_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->primitive_workgroup_buffer.buffer, workgroup_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->primitive_winner_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0u, 1u,
		&slot->descriptor_sets[0], 0u, NULL);
	implementation->command_dispatch(implementation->command_buffer,
		total_workgroups, 1u, 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->primitive_winner_buffer.buffer, storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->primitive_resolve_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0u, 1u,
		&slot->descriptor_sets[1], 0u, NULL);
	groups_x = (resolve_x2 - resolve_x1 + 16u) / 16u;
	groups_y = (resolve_y2 - resolve_y1 + 16u) / 16u;
	implementation->command_dispatch(implementation->command_buffer, groups_x,
		groups_y, 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

/*
	Fallback for outlines and alpha-blended ellipses. Their output depends on
	the prior pixel value, so the dispatches remain intentionally ordered.
*/
int fb_gfx3_vulkan_surface_ellipse_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_ELLIPSE *ellipses, size_t ellipse_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION command_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 2];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 2];
	FB_GFX3_VULKAN_ELLIPSE_COMMAND *commands;
	size_t command_size;
	size_t operation_count = 0u;
	size_t index;
	int opaque_filled = TRUE;
	int result;

	memset(&command_buffer, 0, sizeof(command_buffer));
	if ((ellipses == NULL) || (ellipse_count < 2u) ||
	    (ellipse_count > FB_GFX3_VK_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (implementation->ellipse_pipeline == 0)
		return FB_GFX3_UNSUPPORTED;
	for (index = 0u; index < ellipse_count; ++index) {
		const FB_GFX3_VULKAN_ELLIPSE *ellipse = &ellipses[index];
		FB_GFX3_RECT clipped = ellipse->clip;

		if (!(ellipse->radius_x >= 0.0f) ||
		    !(ellipse->radius_x <= 32767.0f) ||
		    !(ellipse->radius_y >= 0.0f) ||
		    !(ellipse->radius_y <= 32767.0f))
			return FB_GFX3_INVALID;
		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)surface->width)
			clipped.x2 = (int32_t)surface->width - 1;
		if (clipped.y2 >= (int32_t)surface->height)
			clipped.y2 = (int32_t)surface->height - 1;
		if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
			continue;
		operation_count++;
	}
	if (operation_count == 0u)
		return FB_GFX3_OK;
	if (fb_gfx3_vk_size_multiply(operation_count, sizeof(*commands),
		&command_size) != FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;
	result = fb_gfx3_vk_buffer_allocation_create(implementation, &command_buffer,
		(uint64_t)command_size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	commands = (FB_GFX3_VULKAN_ELLIPSE_COMMAND *)command_buffer.mapped;
	operation_count = 0u;
	for (index = 0u; index < ellipse_count; ++index) {
		const FB_GFX3_VULKAN_ELLIPSE *ellipse = &ellipses[index];
		FB_GFX3_RECT clipped = ellipse->clip;
		FB_GFX3_VULKAN_ELLIPSE_COMMAND *command;

		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)surface->width)
			clipped.x2 = (int32_t)surface->width - 1;
		if (clipped.y2 >= (int32_t)surface->height)
			clipped.y2 = (int32_t)surface->height - 1;
		if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
			continue;
		command = &commands[operation_count++];
		memset(command, 0, sizeof(*command));
		command->center[0] = ellipse->center_x;
		command->center[1] = ellipse->center_y;
		command->center[2] = (int32_t)surface->width;
		command->clip = clipped;
		command->radii[0] = ellipse->radius_x;
		command->radii[1] = ellipse->radius_y;
		command->radii[2] = (float)surface->width;
		command->radii[3] = (float)surface->height;
		command->parameters[0] = ellipse->color;
		command->parameters[1] = ellipse->filled != 0;
		command->parameters[2] = fb_gfx3_vk_surface_color_mask(surface->depth);
		command->parameters[3] =
			ellipse->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
		if ((ellipse->filled == 0) ||
		    ((ellipse->flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) != 0))
			opaque_filled = FALSE;
	}
	if (opaque_filled) {
		result = vulkan_surface_ellipse_winner_batch(runtime, implementation,
			storage, &command_buffer, operation_count);
		if (result != FB_GFX3_UNSUPPORTED)
			goto cleanup;
	}
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	for (index = 0u; index < operation_count; ++index) {
		size_t write_index = index * 2u;

		buffer_info[write_index].buffer = storage->storage.buffer;
		buffer_info[write_index].range = storage->storage.size;
		buffer_info[write_index + 1u].buffer = command_buffer.buffer;
		buffer_info[write_index + 1u].offset = index * sizeof(*commands);
		buffer_info[write_index + 1u].range = sizeof(*commands);
		descriptor_writes[write_index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[write_index].destination_set =
			implementation->submission_slots[
			implementation->next_submission_slot].descriptor_sets[index];
		descriptor_writes[write_index].descriptor_count = 1;
		descriptor_writes[write_index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[write_index].buffer_info = &buffer_info[write_index];
		descriptor_writes[write_index + 1u] = descriptor_writes[write_index];
		descriptor_writes[write_index + 1u].destination_binding = 1;
		descriptor_writes[write_index + 1u].buffer_info =
			&buffer_info[write_index + 1u];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(operation_count * 2u), descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, implementation->ellipse_pipeline);
	for (index = 0u; index < operation_count; ++index) {
		FB_GFX3_VK_DESCRIPTOR_SET descriptor_set =
			implementation->submission_slots[
			implementation->active_submission_slot].descriptor_sets[index];

		implementation->command_bind_descriptor_sets(
			implementation->command_buffer, FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
			implementation->compute_pipeline_layout, 0, 1, &descriptor_set, 0,
			NULL);
		implementation->command_dispatch(implementation->command_buffer, 1, 1, 1);
		fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
			storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &command_buffer);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Flood fill                                                               */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vulkan_surface_paint(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x, int32_t y, uint32_t color, uint32_t border_color,
	uint32_t flags, const FB_GFX3_PAINT_COMMAND *paint_payload)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *scratch;
	FB_GFX3_VULKAN_PAINT_COMMAND *paint_command;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[3];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[3];
	FB_GFX3_RECT clipped;
	size_t pixel_count_size;
	uint64_t scratch_size;
	uint64_t phase_offset;
	uint32_t pixel_count;
	uint32_t rectangle_groups_x;
	uint32_t rectangle_groups_y;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if ((result != FB_GFX3_OK) || (clip == NULL) ||
	    (paint_payload == NULL))
		return FB_GFX3_INVALID;
	if (implementation->paint_pipeline == 0)
		return FB_GFX3_UNSUPPORTED;
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)surface->width)
		clipped.x2 = (int32_t)surface->width - 1;
	if (clipped.y2 >= (int32_t)surface->height)
		clipped.y2 = (int32_t)surface->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2) ||
	    (x < clipped.x1) || (y < clipped.y1) ||
	    (x > clipped.x2) || (y > clipped.y2))
		return FB_GFX3_OK;
	if ((fb_gfx3_vk_size_multiply(surface->width, surface->height,
	     &pixel_count_size) != FB_GFX3_OK) ||
	    (pixel_count_size > FB_GFX3_VK_PAINT_MAX_PIXELS) ||
	    (pixel_count_size > UINT32_MAX))
		return FB_GFX3_UNSUPPORTED;
	pixel_count = (uint32_t)pixel_count_size;
	/*
		PAINT owns one visited flag and one scanline-seed queue entry per target
		pixel. Keep this allocation device-local because the shader never
		exposes it to the Basic thread.
	*/
	phase_offset = (uint64_t)pixel_count * 2u * sizeof(uint32_t);
	scratch_size = phase_offset +
		(FB_GFX3_VK_PAINT_METADATA_WORDS * sizeof(uint32_t));
	rectangle_groups_x = ((uint32_t)(clipped.x2 - clipped.x1 + 1) +
		FB_GFX3_VK_PAINT_LOCAL_SIZE_X - 1u) /
		FB_GFX3_VK_PAINT_LOCAL_SIZE_X;
	rectangle_groups_y = ((uint32_t)(clipped.y2 - clipped.y1 + 1) +
		FB_GFX3_VK_PAINT_LOCAL_SIZE_Y - 1u) /
		FB_GFX3_VK_PAINT_LOCAL_SIZE_Y;
	slot = &implementation->submission_slots[implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	result = fb_gfx3_vk_host_buffer_ensure(implementation,
		&slot->paint_command_buffer, sizeof(*paint_command));
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = fb_gfx3_vk_device_storage_buffer_ensure(implementation,
		&slot->paint_scratch_buffer, scratch_size);
	if (result != FB_GFX3_OK)
		goto cleanup;
	command_buffer = &slot->paint_command_buffer;
	scratch = &slot->paint_scratch_buffer;
	paint_command = (FB_GFX3_VULKAN_PAINT_COMMAND *)command_buffer->mapped;
	memset(paint_command, 0, sizeof(*paint_command));
	paint_command->seed[0] = x;
	paint_command->seed[1] = y;
	paint_command->seed[2] = (int32_t)surface->width;
	paint_command->seed[3] = (int32_t)surface->height;
	paint_command->clip = clipped;
	paint_command->format[0] = color;
	paint_command->format[1] = border_color;
	paint_command->format[2] = fb_gfx3_vk_surface_color_mask(surface->depth);
	paint_command->format[3] = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	paint_command->pattern[0] = paint_payload->paint_mode;
	paint_command->pattern[1] = paint_payload->pattern_size;
	paint_command->pattern[2] = paint_payload->pattern_origin_x;
	paint_command->pattern[3] = paint_payload->pattern_origin_y;
	memcpy(paint_command->pattern_word, paint_payload->pattern_word,
		sizeof(paint_command->pattern_word));
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	memset(buffer_info, 0, sizeof(buffer_info));
	buffer_info[0].buffer = storage->storage.buffer;
	buffer_info[0].range = storage->storage.size;
	buffer_info[1].buffer = command_buffer->buffer;
	buffer_info[1].range = sizeof(*paint_command);
	buffer_info[2].buffer = scratch->buffer;
	buffer_info[2].range = scratch_size;
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	descriptor_writes[0].structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].destination_set =
		implementation->compute_descriptor_set;
	descriptor_writes[0].descriptor_count = 1;
	descriptor_writes[0].descriptor_type =
		FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[0].buffer_info = &buffer_info[0];
	descriptor_writes[1] = descriptor_writes[0];
	descriptor_writes[1].destination_binding = 1;
	descriptor_writes[1].buffer_info = &buffer_info[1];
	descriptor_writes[2] = descriptor_writes[0];
	descriptor_writes[2].destination_binding = 2;
	descriptor_writes[2].buffer_info = &buffer_info[2];
	implementation->update_descriptor_sets(implementation->device, 3,
		descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer->buffer,
		sizeof(*paint_command), FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		0, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->paint_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&implementation->compute_descriptor_set, 0, NULL);
	/*
		A dispatch is the device-wide synchronization boundary. Phase one finds
		candidate bounds, phase two verifies them with all available workgroups,
		phase three fills a verified rectangle, and phase four invokes the exact
		scanline fallback only after a rejection. The phase word lives beyond the
		visited map and queue so changing it never aliases flood state.
	*/
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer, 1, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT |
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT |
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 2u);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer,
		rectangle_groups_x, rectangle_groups_y, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT |
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT |
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 3u);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer,
		rectangle_groups_x, rectangle_groups_y, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT |
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT |
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 4u);
	fb_gfx3_vk_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer, 1, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	return result;
}

#endif

/* end of gfx3_vulkan_draw.c */
