/*
 * FreeBASIC gfxlib3: gfx3_vulkan_surface.c
 * Purpose: Validate and allocate Vulkan logical surfaces.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_vulkan_private.h"

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || defined(HOST_DARWIN)

/* ------------------------------------------------------------------------- */
/* Device-local surface foundation                                           */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vk_surface_validate(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	FB_GFX3_VULKAN_IMPLEMENTATION **implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION **surface_implementation)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *owner;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL) || (surface == NULL) ||
	    (surface->implementation == NULL) || (implementation == NULL) ||
	    (surface_implementation == NULL))
		return FB_GFX3_INVALID;
	owner = (FB_GFX3_VULKAN_IMPLEMENTATION *)runtime->implementation;
	storage = (FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *)
		surface->implementation;
	if ((storage->owner != owner) || (storage->width != surface->width) ||
	    (storage->height != surface->height) ||
	    (storage->depth != surface->depth))
		return FB_GFX3_INVALID;
	*implementation = owner;
	*surface_implementation = storage;
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_surface_create(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, uint32_t width, uint32_t height,
	uint32_t depth, uint32_t clear_color)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	size_t pixel_count;
	size_t byte_size;
	int result;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL) || (surface == NULL) ||
	    (surface->implementation != NULL) || (width == 0) ||
	    (height == 0) ||
	    (fb_gfx3_vk_surface_bytes_per_pixel(depth) == 0) ||
	    (fb_gfx3_vk_size_multiply(width, height, &pixel_count) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(pixel_count, sizeof(uint32_t),
	     &byte_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	/*
		A gfxlib3 surface is a linear uint storage buffer. Long, shallow sprite
		sheets are therefore constrained by maxStorageBufferRange, not by the
		device's square-texture dimensions. QFAK, for example, uses a
		10,960-by-40 tile strip which occupies less than two MiB in this layout.
	*/
	if ((implementation->maximum_storage_buffer_range == 0u) ||
	    ((uint64_t)byte_size >
	     implementation->maximum_storage_buffer_range))
		return FB_GFX3_UNSUPPORTED;
	storage = (FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *)
		calloc(1, sizeof(*storage));
	if (storage == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	storage->owner = implementation;
	storage->width = width;
	storage->height = height;
	storage->depth = depth;
	result = fb_gfx3_vk_buffer_allocation_create(implementation,
		&storage->storage, (uint64_t)byte_size,
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
	if (result != FB_GFX3_OK) {
		free(storage);
		return result;
	}
	surface->implementation = storage;
	surface->width = width;
	surface->height = height;
	surface->depth = depth;
	result = fb_gfx3_vulkan_surface_clear(runtime, surface, 0, 0,
		(int32_t)width - 1, (int32_t)height - 1, clear_color);
	if (result != FB_GFX3_OK)
		fb_gfx3_vulkan_surface_destroy(runtime, surface);
	return result;
}

int fb_gfx3_vulkan_surface_clear(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t x1, int32_t y1,
	int32_t x2, int32_t y2, uint32_t color)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	uint64_t row_offset;
	uint64_t row_size;
	int32_t y;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 >= (int32_t)storage->width)
		x2 = (int32_t)storage->width - 1;
	if (y2 >= (int32_t)storage->height)
		y2 = (int32_t)storage->height - 1;
	if ((x1 > x2) || (y1 > y2))
		return FB_GFX3_OK;
	row_size = ((uint64_t)(uint32_t)(x2 - x1) + 1u) * sizeof(uint32_t);
	color &= fb_gfx3_vk_surface_color_mask(storage->depth);
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	for (y = y1; y <= y2; y++) {
		row_offset = (((uint64_t)(uint32_t)y * storage->width) +
			(uint32_t)x1) * sizeof(uint32_t);
		implementation->command_fill_buffer(implementation->command_buffer,
			storage->storage.buffer, row_offset, row_size, color);
	}
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

/*
	The command buffer is ordered, so successive fills to overlapping rows have
	the same last-writer-wins result as separate submissions.  There is no read
	between fills; one transfer-to-general barrier after the complete batch is
	the dependency required by the next graphics command.
*/
int fb_gfx3_vulkan_surface_clear_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangles,
	size_t rectangle_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	size_t index;
	int have_work = FALSE;
	int result;

	if ((rectangles == NULL) || (rectangle_count == 0))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < rectangle_count; index++) {
		if ((rectangles[index].x1 < 0) || (rectangles[index].y1 < 0) ||
		    (rectangles[index].x2 >= (int32_t)storage->width) ||
		    (rectangles[index].y2 >= (int32_t)storage->height))
			return FB_GFX3_INVALID;
		if ((rectangles[index].x1 <= rectangles[index].x2) &&
		    (rectangles[index].y1 <= rectangles[index].y2)) {
			have_work = TRUE;
			break;
		}
	}
	if (!have_work)
		return FB_GFX3_OK;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	for (index = 0; index < rectangle_count; index++) {
		const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangle =
			&rectangles[index];
		uint64_t row_offset;
		uint64_t row_size;
		int32_t y;

		if ((rectangle->x1 > rectangle->x2) ||
		    (rectangle->y1 > rectangle->y2))
			continue;
		row_size = ((uint64_t)(uint32_t)(rectangle->x2 - rectangle->x1) +
			1u) * sizeof(uint32_t);
		for (y = rectangle->y1; y <= rectangle->y2; y++) {
			row_offset = (((uint64_t)(uint32_t)y * storage->width) +
				(uint32_t)rectangle->x1) * sizeof(uint32_t);
			implementation->command_fill_buffer(implementation->command_buffer,
				storage->storage.buffer, row_offset, row_size,
				rectangle->color & fb_gfx3_vk_surface_color_mask(storage->depth));
		}
	}
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

int fb_gfx3_vk_surface_dispatch_command(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage,
	const void *command_data, size_t command_size,
	FB_GFX3_VK_PIPELINE pipeline, uint32_t invocation_count,
	uint32_t invocation_count_y)
{
	FB_GFX3_VULKAN_BUFFER_ALLOCATION command_buffer;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[2];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[2];
	uint32_t group_count_x;
	int result;

	if ((runtime == NULL) || (implementation == NULL) ||
	    (storage == NULL) || (command_data == NULL) ||
	    (command_size == 0) || (pipeline == 0) ||
	    (invocation_count == 0) || (invocation_count_y == 0))
		return FB_GFX3_INVALID;
	memset(&command_buffer, 0, sizeof(command_buffer));
	result = fb_gfx3_vk_buffer_allocation_create(implementation,
		&command_buffer, (uint64_t)command_size,
		FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	memcpy(command_buffer.mapped, command_data, command_size);
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
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&implementation->compute_descriptor_set, 0, NULL);
	group_count_x = (invocation_count - 1u) / 64u + 1u;
	implementation->command_dispatch(implementation->command_buffer,
		group_count_x, invocation_count_y, 1);
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


int fb_gfx3_vulkan_surface_upload(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t destination_x,
	int32_t destination_y, uint32_t width, uint32_t height,
	const void *source, size_t source_pitch)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION staging;
	FB_GFX3_VK_BUFFER_COPY copy;
	const unsigned char *source_row;
	uint32_t *staging_pixels;
	size_t source_row_size;
	size_t staging_pixel_count;
	size_t staging_size;
	uint32_t bytes_per_pixel;
	uint32_t mask;
	uint32_t x;
	uint32_t y;
	int result;

	memset(&staging, 0, sizeof(staging));
	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	bytes_per_pixel = (surface != NULL) ?
		fb_gfx3_vk_surface_bytes_per_pixel(surface->depth) : 0;
	if ((result != FB_GFX3_OK) || (source == NULL) || (width == 0) ||
	    (height == 0) || (destination_x < 0) || (destination_y < 0) ||
	    ((uint64_t)(uint32_t)destination_x + width > surface->width) ||
	    ((uint64_t)(uint32_t)destination_y + height > surface->height) ||
	    (fb_gfx3_vk_size_multiply(width, bytes_per_pixel,
	     &source_row_size) != FB_GFX3_OK) ||
	    (source_pitch < source_row_size) ||
	    (fb_gfx3_vk_size_multiply(width, height,
	     &staging_pixel_count) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(staging_pixel_count, sizeof(uint32_t),
	     &staging_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_buffer_allocation_create(implementation, &staging,
		(uint64_t)staging_size, FB_GFX3_VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	staging_pixels = (uint32_t *)staging.mapped;
	mask = fb_gfx3_vk_surface_color_mask(surface->depth);
	for (y = 0; y < height; y++) {
		source_row = (const unsigned char *)source +
			((size_t)y * source_pitch);
		for (x = 0; x < width; x++) {
			uint32_t pixel = 0;

			switch (bytes_per_pixel) {
			case 1:
				pixel = source_row[x];
				break;
			case 2:
				memcpy(&pixel, source_row + ((size_t)x * 2), 2);
				pixel &= 0xFFFFu;
				break;
			default:
				memcpy(&pixel, source_row + ((size_t)x * 4), 4);
				break;
			}
			staging_pixels[((size_t)y * width) + x] = pixel & mask;
		}
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	fb_gfx3_vk_record_buffer_barrier(implementation, staging.buffer,
		staging.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	memset(&copy, 0, sizeof(copy));
	copy.size = (uint64_t)width * sizeof(uint32_t);
	for (y = 0; y < height; y++) {
		copy.source_offset = (uint64_t)y * copy.size;
		copy.destination_offset =
			(((uint64_t)((uint32_t)destination_y + y) *
			  storage->width) + (uint32_t)destination_x) *
			sizeof(uint32_t);
		implementation->command_copy_buffer(implementation->command_buffer,
			staging.buffer, storage->storage.buffer, 1, &copy);
	}
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &staging);
	return result;
}

/*
	The Vulkan render thread serializes public command execution.  Downloads
	wait for their fence before the caller can touch the destination image, so
	this allocation is neither visible to another thread nor in flight when it
	is resized or reused.
*/
static int vulkan_download_buffer_ensure(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation, uint64_t required_size)
{
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *buffer;
	int result;

	if ((implementation == NULL) || (required_size == 0))
		return FB_GFX3_INVALID;
	buffer = &implementation->download_buffer;
	if ((buffer->buffer != 0) && (buffer->mapped != NULL) &&
	    (buffer->size >= required_size))
		return FB_GFX3_OK;
	fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation, buffer);
	result = fb_gfx3_vk_buffer_allocation_create(implementation, buffer,
		required_size, FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	return result;
}

int fb_gfx3_vulkan_surface_download(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t source_x, int32_t source_y,
	uint32_t width, uint32_t height, void *destination,
	size_t destination_pitch)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *staging;
	FB_GFX3_VK_BUFFER_COPY copy;
	const uint32_t *staging_pixels;
	unsigned char *destination_row;
	size_t destination_row_size;
	size_t staging_pixel_count;
	size_t staging_size;
	uint32_t bytes_per_pixel;
	uint32_t x;
	uint32_t y;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, surface, &implementation,
		&storage);
	bytes_per_pixel = (surface != NULL) ?
		fb_gfx3_vk_surface_bytes_per_pixel(surface->depth) : 0;
	if ((result != FB_GFX3_OK) || (destination == NULL) || (width == 0) ||
	    (height == 0) || (source_x < 0) || (source_y < 0) ||
	    ((uint64_t)(uint32_t)source_x + width > surface->width) ||
	    ((uint64_t)(uint32_t)source_y + height > surface->height) ||
	    (fb_gfx3_vk_size_multiply(width, bytes_per_pixel,
	     &destination_row_size) != FB_GFX3_OK) ||
	    (destination_pitch < destination_row_size) ||
	    (fb_gfx3_vk_size_multiply(width, height,
	     &staging_pixel_count) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(staging_pixel_count, sizeof(uint32_t),
	     &staging_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	result = vulkan_download_buffer_ensure(implementation,
		(uint64_t)staging_size);
	if (result != FB_GFX3_OK)
		return result;
	staging = &implementation->download_buffer;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	memset(&copy, 0, sizeof(copy));
	copy.size = (uint64_t)width * sizeof(uint32_t);
	for (y = 0; y < height; y++) {
		copy.source_offset =
			(((uint64_t)((uint32_t)source_y + y) * storage->width) +
			 (uint32_t)source_x) * sizeof(uint32_t);
		copy.destination_offset = (uint64_t)y * copy.size;
		implementation->command_copy_buffer(implementation->command_buffer,
			storage->storage.buffer, staging->buffer, 1, &copy);
	}
	fb_gfx3_vk_record_buffer_barrier(implementation, staging->buffer,
		staging->size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_HOST_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT);
	result = fb_gfx3_vk_end_commands(implementation, TRUE);
	if (result != FB_GFX3_OK)
		goto cleanup;
	staging_pixels = (const uint32_t *)staging->mapped;
	for (y = 0; y < height; y++) {
		destination_row = (unsigned char *)destination +
			((size_t)y * destination_pitch);
		for (x = 0; x < width; x++) {
			uint32_t pixel = staging_pixels[((size_t)y * width) + x];

			switch (bytes_per_pixel) {
			case 1:
				destination_row[x] = (unsigned char)pixel;
				break;
			case 2: {
				uint16_t pixel16 = (uint16_t)pixel;

				memcpy(destination_row + ((size_t)x * 2),
					&pixel16, 2);
				break;
			}
			default:
				memcpy(destination_row + ((size_t)x * 4), &pixel, 4);
				break;
			}
		}
	}
	runtime->completed_submission_count++;

cleanup:
	return result;
}

void fb_gfx3_vulkan_surface_destroy(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;

	if ((runtime == NULL) || (runtime->implementation == NULL) ||
	    (surface == NULL) || (surface->implementation == NULL))
		return;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	storage = (FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *)
		surface->implementation;
	if (storage->owner != implementation)
		return;
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &storage->storage);
	free(storage);
	memset(surface, 0, sizeof(*surface));
}
#endif

/* end of gfx3_vulkan_surface.c */
