/*
 * FreeBASIC gfxlib3: gfx3_vulkan_blit.c
 * Purpose: Record Vulkan image copies, PUT modes and transformed blits.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_vulkan_private.h"

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || defined(HOST_DARWIN)

/* ------------------------------------------------------------------------- */
/* Single image transfers                                                   */
/* ------------------------------------------------------------------------- */

static int vulkan_surface_blit_single(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source, const FB_GFX3_RECT *clip,
	const FB_GFX3_RECT *source_rect, int32_t destination_x,
	int32_t destination_y, uint32_t mode, uint32_t alpha)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_IMPLEMENTATION *source_implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION snapshot;
	FB_GFX3_VULKAN_BLIT_COMMAND *blit_command;
	FB_GFX3_VK_BUFFER source_buffer;
	FB_GFX3_VK_BUFFER_COPY copy_region;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[3];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[3];
	FB_GFX3_RECT clipped;
	int64_t destination_x2;
	int64_t destination_y2;
	size_t pixel_count_size;
	uint32_t width;
	uint32_t height;
	uint32_t pixel_count;
	uint32_t group_count_x;
	int result;

	memset(&command_buffer, 0, sizeof(command_buffer));
	memset(&snapshot, 0, sizeof(snapshot));
	result = fb_gfx3_vk_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_surface_validate(runtime, source,
		&source_implementation, &source_storage);
	if (result != FB_GFX3_OK)
		return result;
	if ((clip == NULL) || (source_rect == NULL) ||
	    (implementation != source_implementation) ||
	    (destination->depth != source->depth) ||
	    (source_rect->x1 < 0) || (source_rect->y1 < 0) ||
	    (source_rect->x1 > source_rect->x2) ||
	    (source_rect->y1 > source_rect->y2) ||
	    (source_rect->x2 >= (int32_t)source->width) ||
	    (source_rect->y2 >= (int32_t)source->height))
		return FB_GFX3_INVALID;
	switch (mode) {
	case FB_GFX3_BLIT_TRANS:
	case FB_GFX3_BLIT_PSET:
	case FB_GFX3_BLIT_PRESET:
	case FB_GFX3_BLIT_AND:
	case FB_GFX3_BLIT_OR:
	case FB_GFX3_BLIT_XOR:
	case FB_GFX3_BLIT_ALPHA:
	case FB_GFX3_BLIT_ADD:
	case FB_GFX3_BLIT_BLEND:
		break;
	default:
		return FB_GFX3_UNSUPPORTED;
	}
	clipped = *clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)destination->width)
		clipped.x2 = (int32_t)destination->width - 1;
	if (clipped.y2 >= (int32_t)destination->height)
		clipped.y2 = (int32_t)destination->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
		return FB_GFX3_OK;
	width = (uint32_t)(source_rect->x2 - source_rect->x1 + 1);
	height = (uint32_t)(source_rect->y2 - source_rect->y1 + 1);
	destination_x2 = (int64_t)destination_x + width - 1;
	destination_y2 = (int64_t)destination_y + height - 1;
	if ((destination_x2 < clipped.x1) ||
	    (destination_y2 < clipped.y1) ||
	    (destination_x > clipped.x2) ||
	    (destination_y > clipped.y2))
		return FB_GFX3_OK;
	if ((fb_gfx3_vk_size_multiply(width, height, &pixel_count_size) !=
	     FB_GFX3_OK) || (pixel_count_size > UINT32_MAX))
		return FB_GFX3_UNSUPPORTED;
	pixel_count = (uint32_t)pixel_count_size;

	source_buffer = source_storage->storage.buffer;
	if (source_storage == destination_storage) {
		/*
			Vulkan compute invocations may run in any order. A same-surface
			PUT therefore snapshots the source in device memory before any
			destination invocation can overwrite a later source pixel.
		*/
		result = fb_gfx3_vk_buffer_allocation_create(implementation, &snapshot,
			source_storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		result = fb_gfx3_vk_begin_commands(implementation);
		if (result != FB_GFX3_OK)
			goto cleanup;
		fb_gfx3_vk_record_buffer_barrier(implementation,
			source_storage->storage.buffer,
			source_storage->storage.size,
			FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
			FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
			FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		fb_gfx3_vk_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, 0, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&copy_region, 0, sizeof(copy_region));
		copy_region.size = source_storage->storage.size;
		implementation->command_copy_buffer(
			implementation->command_buffer,
			source_storage->storage.buffer, snapshot.buffer, 1,
			&copy_region);
		fb_gfx3_vk_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		result = fb_gfx3_vk_end_commands(implementation, FALSE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		runtime->completed_submission_count++;
		source_buffer = snapshot.buffer;
	}
	result = fb_gfx3_vk_buffer_allocation_create(implementation,
		&command_buffer, sizeof(*blit_command),
		FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		goto cleanup;
	blit_command = (FB_GFX3_VULKAN_BLIT_COMMAND *)command_buffer.mapped;
	blit_command->source_rect[0] = source_rect->x1;
	blit_command->source_rect[1] = source_rect->y1;
	blit_command->source_rect[2] = (int32_t)width;
	blit_command->source_rect[3] = (int32_t)height;
	blit_command->clip = clipped;
	blit_command->destination[0] = destination_x;
	blit_command->destination[1] = destination_y;
	blit_command->destination[2] = (int32_t)source->width;
	blit_command->destination[3] = (int32_t)destination->width;
	blit_command->format[0] = destination->depth;
	blit_command->format[1] = mode;
	blit_command->format[2] = alpha;
	blit_command->format[3] =
		fb_gfx3_vk_surface_color_mask(destination->depth);
	blit_command->dimensions[0] = source->width;
	blit_command->dimensions[1] = destination->width;
	blit_command->dimensions[2] = destination->height;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;

	memset(buffer_info, 0, sizeof(buffer_info));
	buffer_info[0].buffer = destination_storage->storage.buffer;
	buffer_info[0].range = destination_storage->storage.size;
	buffer_info[1].buffer = source_buffer;
	buffer_info[1].range = source_storage->storage.size;
	buffer_info[2].buffer = command_buffer.buffer;
	buffer_info[2].range = command_buffer.size;
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
	fb_gfx3_vk_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, source_buffer,
		source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->blit_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&implementation->compute_descriptor_set, 0, NULL);
	group_count_x = (pixel_count - 1u) / 64u + 1u;
	implementation->command_dispatch(implementation->command_buffer,
		group_count_x, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &command_buffer);
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &snapshot);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Copies and transformed blits                                             */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vulkan_surface_copy_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	const FB_GFX3_VULKAN_SURFACE_COPY *copies, size_t copy_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation = NULL;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage[
		FB_GFX3_VK_BLIT_BATCH_LIMIT];
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage[
		FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t index;
	int result;

	if ((runtime == NULL) || (copies == NULL) || (copy_count == 0u) ||
	    (copy_count > FB_GFX3_VK_BLIT_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	/*
		Validate the complete run before recording anything. This keeps the
		operation atomic from the backend's point of view if a stale or malformed
		surface handle reaches the renderer.
	*/
	for (index = 0u; index < copy_count; index++) {
		FB_GFX3_VULKAN_IMPLEMENTATION *destination_implementation;
		FB_GFX3_VULKAN_IMPLEMENTATION *source_implementation;

		if ((copies[index].destination == NULL) ||
		    (copies[index].source == NULL) ||
		    (copies[index].destination == copies[index].source))
			return FB_GFX3_INVALID;
		result = fb_gfx3_vk_surface_validate(runtime, copies[index].destination,
			&destination_implementation, &destination_storage[index]);
		if (result != FB_GFX3_OK)
			return result;
		result = fb_gfx3_vk_surface_validate(runtime, copies[index].source,
			&source_implementation, &source_storage[index]);
		if (result != FB_GFX3_OK)
			return result;
		if ((destination_implementation != source_implementation) ||
		    ((implementation != NULL) &&
		     (destination_implementation != implementation)) ||
		    (copies[index].destination->width != copies[index].source->width) ||
		    (copies[index].destination->height != copies[index].source->height) ||
		    (copies[index].destination->depth != copies[index].source->depth) ||
		    (destination_storage[index]->storage.size !=
		     source_storage[index]->storage.size))
			return FB_GFX3_INVALID;
		implementation = destination_implementation;
	}
	if (implementation == NULL)
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	/* Make all earlier shader or transfer writes available to this copy run. */
	fb_gfx3_vk_record_memory_barrier(implementation,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT |
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	for (index = 0u; index < copy_count; index++) {
		FB_GFX3_VK_BUFFER_COPY copy_region;

		/*
			Alternating page flips commonly use the previous destination as the
			next source. One global transfer dependency between copies retains that
			FIFO relationship without recording four buffer barriers per page.
		*/
		if (index != 0u)
			fb_gfx3_vk_record_memory_barrier(implementation,
				FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
				FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT |
				FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
				FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
				FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&copy_region, 0, sizeof(copy_region));
		copy_region.size = destination_storage[index]->storage.size;
		implementation->command_copy_buffer(implementation->command_buffer,
			source_storage[index]->storage.buffer,
			destination_storage[index]->storage.buffer, 1u, &copy_region);
	}
	fb_gfx3_vk_record_memory_barrier(implementation,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT |
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

int fb_gfx3_vulkan_surface_blit(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source, const FB_GFX3_RECT *clip,
	const FB_GFX3_RECT *source_rect, int32_t destination_x,
	int32_t destination_y, uint32_t mode, uint32_t alpha)
{
	return vulkan_surface_blit_single(runtime, destination, source, clip,
		source_rect, destination_x, destination_y, mode, alpha);
}

/*
	Projective surface transform

	The command record and descriptor set belong to the next in-flight slot.
	That lets ordinary animation reuse mapped command storage without allocating
	a VkBuffer for every sprite. Only a same-surface operation is synchronous:
	it needs a device-local snapshot to keep unordered compute invocations from
	reading pixels another invocation has already replaced.
*/
int fb_gfx3_vulkan_surface_transform_blit(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source,
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *transform)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_IMPLEMENTATION *source_implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND *command;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION snapshot;
	FB_GFX3_VK_BUFFER source_buffer;
	FB_GFX3_VK_BUFFER_COPY copy_region;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[3];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[3];
	FB_GFX3_VK_DESCRIPTOR_SET descriptor_set;
	FB_GFX3_RECT clipped;
	FB_GFX3_RECT bounds;
	size_t pixel_count_size;
	uint32_t width;
	uint32_t height;
	uint32_t pixel_count;
	uint32_t group_count_x;
	int self_transform;
	int result;

	memset(&snapshot, 0, sizeof(snapshot));
	result = fb_gfx3_vk_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_surface_validate(runtime, source,
		&source_implementation, &source_storage);
	if (result != FB_GFX3_OK)
		return result;
	if ((transform == NULL) || (implementation != source_implementation) ||
	    (destination->depth != source->depth) ||
	    (transform->source_rect.x1 < 0) ||
	    (transform->source_rect.y1 < 0) ||
	    (transform->source_rect.x1 > transform->source_rect.x2) ||
	    (transform->source_rect.y1 > transform->source_rect.y2) ||
	    (transform->source_rect.x2 >= (int32_t)source->width) ||
	    (transform->source_rect.y2 >= (int32_t)source->height) ||
	    (transform->destination_bounds.x1 > transform->destination_bounds.x2) ||
	    (transform->destination_bounds.y1 > transform->destination_bounds.y2) ||
	    (transform->filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
	    (transform->wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT))
		return FB_GFX3_INVALID;
	switch (transform->mode) {
	case FB_GFX3_BLIT_TRANS:
	case FB_GFX3_BLIT_PSET:
	case FB_GFX3_BLIT_PRESET:
	case FB_GFX3_BLIT_AND:
	case FB_GFX3_BLIT_OR:
	case FB_GFX3_BLIT_XOR:
	case FB_GFX3_BLIT_ALPHA:
	case FB_GFX3_BLIT_ADD:
	case FB_GFX3_BLIT_BLEND:
		break;
	default:
		return FB_GFX3_UNSUPPORTED;
	}
	clipped = transform->clip;
	if (clipped.x1 < 0)
		clipped.x1 = 0;
	if (clipped.y1 < 0)
		clipped.y1 = 0;
	if (clipped.x2 >= (int32_t)destination->width)
		clipped.x2 = (int32_t)destination->width - 1;
	if (clipped.y2 >= (int32_t)destination->height)
		clipped.y2 = (int32_t)destination->height - 1;
	if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
		return FB_GFX3_OK;
	bounds = transform->destination_bounds;
	if (bounds.x1 < clipped.x1)
		bounds.x1 = clipped.x1;
	if (bounds.y1 < clipped.y1)
		bounds.y1 = clipped.y1;
	if (bounds.x2 > clipped.x2)
		bounds.x2 = clipped.x2;
	if (bounds.y2 > clipped.y2)
		bounds.y2 = clipped.y2;
	if ((bounds.x1 > bounds.x2) || (bounds.y1 > bounds.y2))
		return FB_GFX3_OK;
	width = (uint32_t)(bounds.x2 - bounds.x1 + 1);
	height = (uint32_t)(bounds.y2 - bounds.y1 + 1);
	if ((fb_gfx3_vk_size_multiply(width, height, &pixel_count_size) !=
	     FB_GFX3_OK) || (pixel_count_size > UINT32_MAX))
		return FB_GFX3_UNSUPPORTED;
	pixel_count = (uint32_t)pixel_count_size;
	group_count_x = (pixel_count - 1u) / 64u + 1u;

	self_transform = source_storage == destination_storage;
	source_buffer = source_storage->storage.buffer;
	if (self_transform) {
		result = fb_gfx3_vk_buffer_allocation_create(implementation, &snapshot,
			source_storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		result = fb_gfx3_vk_begin_commands(implementation);
		if (result != FB_GFX3_OK)
			goto cleanup;
		fb_gfx3_vk_record_buffer_barrier(implementation,
			source_storage->storage.buffer, source_storage->storage.size,
			FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
			FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
			FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		fb_gfx3_vk_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, 0, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&copy_region, 0, sizeof(copy_region));
		copy_region.size = source_storage->storage.size;
		implementation->command_copy_buffer(implementation->command_buffer,
			source_storage->storage.buffer, snapshot.buffer, 1, &copy_region);
		fb_gfx3_vk_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		result = fb_gfx3_vk_end_commands(implementation, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		runtime->completed_submission_count++;
		source_buffer = snapshot.buffer;
	}

	/* fb_gfx3_vk_begin_commands() will select this same checked slot. */
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	if (slot->blit_command_buffer.buffer == 0) {
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->blit_command_buffer,
			(uint64_t)FB_GFX3_VK_BLIT_BATCH_LIMIT *
				sizeof(FB_GFX3_VULKAN_BLIT_COMMAND),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
	}
	if (slot->blit_command_buffer.size < sizeof(*command)) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	command = (FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND *)
		slot->blit_command_buffer.mapped;
	memset(command, 0, sizeof(*command));
	command->source_rect[0] = transform->source_rect.x1;
	command->source_rect[1] = transform->source_rect.y1;
	command->source_rect[2] = transform->source_rect.x2;
	command->source_rect[3] = transform->source_rect.y2;
	command->clip = clipped;
	command->bounds = bounds;
	memcpy(command->inverse_row_0, transform->inverse,
		3u * sizeof(transform->inverse[0]));
	memcpy(command->inverse_row_1, transform->inverse + 3,
		3u * sizeof(transform->inverse[0]));
	memcpy(command->inverse_row_2, transform->inverse + 6,
		3u * sizeof(transform->inverse[0]));
	command->format[0] = destination->depth;
	command->format[1] = transform->mode;
	command->format[2] = transform->alpha;
	command->format[3] = fb_gfx3_vk_surface_color_mask(destination->depth);
	command->options[0] = transform->filter;
	command->options[1] = transform->wrap;
	command->options[2] = source->width;
	command->options[3] = destination->width;
	command->dimensions[0] = source->height;
	command->dimensions[1] = destination->height;

	memset(buffer_info, 0, sizeof(buffer_info));
	buffer_info[0].buffer = destination_storage->storage.buffer;
	buffer_info[0].range = destination_storage->storage.size;
	buffer_info[1].buffer = source_buffer;
	buffer_info[1].range = source_storage->storage.size;
	buffer_info[2].buffer = slot->blit_command_buffer.buffer;
	buffer_info[2].range = sizeof(*command);
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	descriptor_writes[0].structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].destination_set = slot->descriptor_sets[0];
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

	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device, 3,
		descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->blit_command_buffer.buffer, sizeof(*command),
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, source_buffer,
		source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->transform_blit_pipeline);
	descriptor_set = slot->descriptor_sets[0];
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&descriptor_set, 0, NULL);
	implementation->command_dispatch(implementation->command_buffer,
		group_count_x, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, self_transform);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &snapshot);
	return result;
}

int fb_gfx3_vulkan_surface_transform_blit_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source,
	const FB_GFX3_TRANSFORM_BLIT_COMMAND *const *transforms,
	size_t transform_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_IMPLEMENTATION *source_implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND *prepared;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 3u];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 3u];
	uint32_t group_count_x[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t command_size;
	size_t index;
	uint32_t operation_count = 0u;
	int result;

	result = fb_gfx3_vk_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_surface_validate(runtime, source,
		&source_implementation, &source_storage);
	if (result != FB_GFX3_OK)
		return result;
	if ((transforms == NULL) || (transform_count < 2u) ||
	    (transform_count > FB_GFX3_VK_BLIT_BATCH_LIMIT) ||
	    (implementation != source_implementation) ||
	    (source_storage == destination_storage) ||
	    (destination->depth != source->depth) ||
	    (implementation->transform_blit_pipeline == 0))
		return FB_GFX3_INVALID;
	for (index = 0u; index < transform_count; ++index) {
		const FB_GFX3_TRANSFORM_BLIT_COMMAND *transform = transforms[index];

		if ((transform == NULL) ||
		    (transform->source_rect.x1 < 0) ||
		    (transform->source_rect.y1 < 0) ||
		    (transform->source_rect.x1 > transform->source_rect.x2) ||
		    (transform->source_rect.y1 > transform->source_rect.y2) ||
		    (transform->source_rect.x2 >= (int32_t)source->width) ||
		    (transform->source_rect.y2 >= (int32_t)source->height) ||
		    (transform->destination_bounds.x1 >
		     transform->destination_bounds.x2) ||
		    (transform->destination_bounds.y1 >
		     transform->destination_bounds.y2) ||
		    (transform->mode > FB_GFX3_BLIT_BLEND) ||
		    (transform->mode == FB_GFX3_BLIT_CUSTOM) ||
		    (transform->filter > FB_GFX3_TRANSFORM_FILTER_LINEAR) ||
		    (transform->wrap > FB_GFX3_TRANSFORM_WRAP_REPEAT))
			return FB_GFX3_INVALID;
	}

	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->transform_command_buffer.buffer == 0) {
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->transform_command_buffer,
			(uint64_t)FB_GFX3_VK_BLIT_BATCH_LIMIT *
				sizeof(FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			return result;
	}
	prepared = (FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND *)
		slot->transform_command_buffer.mapped;
	for (index = 0u; index < transform_count; ++index) {
		const FB_GFX3_TRANSFORM_BLIT_COMMAND *transform = transforms[index];
		FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND *command;
		FB_GFX3_RECT clipped = transform->clip;
		FB_GFX3_RECT bounds = transform->destination_bounds;
		size_t pixel_count_size;
		uint32_t width;
		uint32_t height;

		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)destination->width)
			clipped.x2 = (int32_t)destination->width - 1;
		if (clipped.y2 >= (int32_t)destination->height)
			clipped.y2 = (int32_t)destination->height - 1;
		if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
			continue;
		if (bounds.x1 < clipped.x1)
			bounds.x1 = clipped.x1;
		if (bounds.y1 < clipped.y1)
			bounds.y1 = clipped.y1;
		if (bounds.x2 > clipped.x2)
			bounds.x2 = clipped.x2;
		if (bounds.y2 > clipped.y2)
			bounds.y2 = clipped.y2;
		if ((bounds.x1 > bounds.x2) || (bounds.y1 > bounds.y2))
			continue;
		width = (uint32_t)(bounds.x2 - bounds.x1 + 1);
		height = (uint32_t)(bounds.y2 - bounds.y1 + 1);
		if ((fb_gfx3_vk_size_multiply(width, height, &pixel_count_size) !=
		     FB_GFX3_OK) || (pixel_count_size > UINT32_MAX))
			return FB_GFX3_UNSUPPORTED;
		command = &prepared[operation_count];
		memset(command, 0, sizeof(*command));
		command->source_rect[0] = transform->source_rect.x1;
		command->source_rect[1] = transform->source_rect.y1;
		command->source_rect[2] = transform->source_rect.x2;
		command->source_rect[3] = transform->source_rect.y2;
		command->clip = clipped;
		command->bounds = bounds;
		memcpy(command->inverse_row_0, transform->inverse,
			3u * sizeof(transform->inverse[0]));
		memcpy(command->inverse_row_1, transform->inverse + 3,
			3u * sizeof(transform->inverse[0]));
		memcpy(command->inverse_row_2, transform->inverse + 6,
			3u * sizeof(transform->inverse[0]));
		command->format[0] = destination->depth;
		command->format[1] = transform->mode;
		command->format[2] = transform->alpha;
		command->format[3] = fb_gfx3_vk_surface_color_mask(destination->depth);
		command->options[0] = transform->filter;
		command->options[1] = transform->wrap;
		command->options[2] = source->width;
		command->options[3] = destination->width;
		command->dimensions[0] = source->height;
		command->dimensions[1] = destination->height;
		group_count_x[operation_count] =
			((uint32_t)pixel_count_size - 1u) / 64u + 1u;
		operation_count++;
	}
	if (operation_count == 0u)
		return FB_GFX3_OK;
	command_size = (size_t)operation_count * sizeof(prepared[0]);
	if (command_size > slot->transform_command_buffer.size)
		return FB_GFX3_UNSUPPORTED;

	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	for (index = 0u; index < operation_count; ++index) {
		size_t write_index = index * 3u;

		buffer_info[write_index].buffer = destination_storage->storage.buffer;
		buffer_info[write_index].range = destination_storage->storage.size;
		buffer_info[write_index + 1u].buffer = source_storage->storage.buffer;
		buffer_info[write_index + 1u].range = source_storage->storage.size;
		buffer_info[write_index + 2u].buffer =
			slot->transform_command_buffer.buffer;
		buffer_info[write_index + 2u].offset = index * sizeof(prepared[0]);
		buffer_info[write_index + 2u].range = sizeof(prepared[0]);
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
		descriptor_writes[write_index + 2u] = descriptor_writes[write_index];
		descriptor_writes[write_index + 2u].destination_binding = 2;
		descriptor_writes[write_index + 2u].buffer_info =
			&buffer_info[write_index + 2u];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device,
		operation_count * 3u, descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->transform_command_buffer.buffer, command_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		source_storage->storage.buffer, source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->transform_blit_pipeline);
	for (index = 0u; index < operation_count; ++index) {
		FB_GFX3_VK_DESCRIPTOR_SET descriptor_set =
			slot->descriptor_sets[index];

		implementation->command_bind_descriptor_sets(
			implementation->command_buffer,
			FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
			implementation->compute_pipeline_layout, 0, 1,
			&descriptor_set, 0, NULL);
		implementation->command_dispatch(implementation->command_buffer,
			group_count_x[index], 1, 1);
		fb_gfx3_vk_record_buffer_barrier(implementation,
			destination_storage->storage.buffer,
			destination_storage->storage.size,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
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
	TRANS, PSET and PRESET do not read the previous destination colour. A source
	pixel shader can therefore select the last contributing public command with
	an atomic maximum. A second shader resolves that winner once per destination
	pixel. This keeps overlap ordering exact without CPU tile construction or a
	destination thread replaying every sprite assigned to its tile.
*/
/* ------------------------------------------------------------------------- */
/* Ordered blit batches                                                     */
/* ------------------------------------------------------------------------- */

static int vulkan_surface_blit_winner_submit(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage,
	const FB_GFX3_VULKAN_BLIT_COMMAND *prepared_commands,
	size_t operation_count)
{
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[7];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[7];
	FB_GFX3_VULKAN_BLIT_COMMAND *commands;
	uint32_t maximum_pixel_count = 0u;
	uint32_t resolve_x1 = UINT32_MAX;
	uint32_t resolve_y1 = UINT32_MAX;
	uint32_t resolve_x2 = 0u;
	uint32_t resolve_y2 = 0u;
	uint32_t groups_x;
	uint32_t groups_y;
	uint32_t index;
	size_t command_size;
	int all_commands_clipped = TRUE;
	int result;

	if ((runtime == NULL) || (implementation == NULL) ||
	    (destination_storage == NULL) || (source_storage == NULL) ||
	    (prepared_commands == NULL) || (operation_count < 2u) ||
	    (operation_count > FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT) ||
	    (implementation->blit_winner_pipeline == 0) ||
	    (implementation->blit_resolve_pipeline == 0))
		return FB_GFX3_UNSUPPORTED;
	for (index = 0u; index < (uint32_t)operation_count; index++) {
		uint32_t mode = prepared_commands[index].format[1];
		uint32_t width = (uint32_t)prepared_commands[index].source_rect[2];
		uint32_t height = (uint32_t)prepared_commands[index].source_rect[3];
		uint32_t pixel_count;
		uint32_t left = (uint32_t)prepared_commands[index].destination[0];
		uint32_t top = (uint32_t)prepared_commands[index].destination[1];
		uint32_t right;
		uint32_t bottom;

		if ((mode != FB_GFX3_BLIT_TRANS) &&
		    (mode != FB_GFX3_BLIT_PSET) &&
		    (mode != FB_GFX3_BLIT_PRESET))
			return FB_GFX3_UNSUPPORTED;
		if ((width == 0u) || (height == 0u) ||
		    (width > UINT32_MAX / height))
			return FB_GFX3_INVALID;
		if (prepared_commands[index].dimensions[3] == 0u)
			all_commands_clipped = FALSE;
		pixel_count = width * height;
		right = left + width - 1u;
		bottom = top + height - 1u;
		if (pixel_count > maximum_pixel_count)
			maximum_pixel_count = pixel_count;
		if (left < resolve_x1)
			resolve_x1 = left;
		if (top < resolve_y1)
			resolve_y1 = top;
		if (right > resolve_x2)
			resolve_x2 = right;
		if (bottom > resolve_y2)
			resolve_y2 = bottom;
	}
	if ((maximum_pixel_count == 0u) ||
	    (destination_storage->width == 0u) ||
	    (destination_storage->height == 0u) ||
	    (resolve_x1 > resolve_x2) || (resolve_y1 > resolve_y2))
		return FB_GFX3_INVALID;
	/*
		The RTX driver resolves the global atomic winner image substantially faster
		than ordered tile replay. Intel normally prefers its depth-specific tile
		shader, but commands trimmed to no more than one 64-lane wave leave too little
		work to amortize a full tile's ordered command replay. Keep ordinary Intel
		sprites on the tile path and admit only batches where every command was
		actually clipped to this sparse size.

		0x10DE and 0x8086 are the NVIDIA and Intel PCI vendor identifiers.
	*/
	if ((runtime->selected_vendor_id != 0x10DEu) &&
	    !((runtime->selected_vendor_id == 0x8086u) &&
	      all_commands_clipped && (maximum_pixel_count <= 64u)))
		return FB_GFX3_UNSUPPORTED;
	/* Vulkan guarantees at least 65,535 workgroups in each dispatch axis. */
	if ((maximum_pixel_count > 65535u * 64u) ||
	    (resolve_x2 > 0xFFFFu) || (resolve_y2 > 0xFFFFu))
		return FB_GFX3_UNSUPPORTED;
	if (fb_gfx3_vk_size_multiply(operation_count, sizeof(*commands),
	    &command_size) != FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;

	/* The next begin call selects this slot, so its fence owns all reuse below. */
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->blit_command_buffer.size < command_size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_command_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->blit_command_buffer,
			(uint64_t)FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT * sizeof(*commands),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			return result;
	}
	if (slot->blit_winner_buffer.size < destination_storage->storage.size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_winner_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->blit_winner_buffer, destination_storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			return result;
	}
	commands = (FB_GFX3_VULKAN_BLIT_COMMAND *)
		slot->blit_command_buffer.mapped;
	memcpy(commands, prepared_commands, command_size);
	/* The public surface limit keeps both resolve-origin coordinates in 16 bits. */
	commands[0].dimensions[3] = (resolve_y1 << 16u) | resolve_x1;

	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	/* Winner set: winner image, source image, command array. */
	buffer_info[0].buffer = slot->blit_winner_buffer.buffer;
	buffer_info[0].range = destination_storage->storage.size;
	buffer_info[1].buffer = source_storage->storage.buffer;
	buffer_info[1].range = source_storage->storage.size;
	buffer_info[2].buffer = slot->blit_command_buffer.buffer;
	buffer_info[2].range = command_size;
	/* Resolve set: destination, source, winner image, command array. */
	buffer_info[3].buffer = destination_storage->storage.buffer;
	buffer_info[3].range = destination_storage->storage.size;
	buffer_info[4] = buffer_info[1];
	buffer_info[5] = buffer_info[0];
	buffer_info[6] = buffer_info[2];
	for (index = 0u; index < 7u; index++) {
		descriptor_writes[index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[index].descriptor_count = 1;
		descriptor_writes[index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[index].buffer_info = &buffer_info[index];
	}
	for (index = 0u; index < 3u; index++) {
		descriptor_writes[index].destination_set = slot->descriptor_sets[0];
		descriptor_writes[index].destination_binding = index;
	}
	for (index = 3u; index < 7u; index++) {
		descriptor_writes[index].destination_set = slot->descriptor_sets[1];
		descriptor_writes[index].destination_binding = index - 3u;
	}

	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device, 7u,
		descriptor_writes, 0, NULL);
	implementation->command_fill_buffer(implementation->command_buffer,
		slot->blit_winner_buffer.buffer, 0,
		destination_storage->storage.size, 0u);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->blit_winner_buffer.buffer, destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->blit_command_buffer.buffer, command_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		source_storage->storage.buffer, source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->blit_winner_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&slot->descriptor_sets[0], 0, NULL);
	groups_x = (maximum_pixel_count + 63u) / 64u;
	implementation->command_dispatch(implementation->command_buffer,
		groups_x, 1u, (uint32_t)operation_count);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->blit_winner_buffer.buffer, destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->blit_resolve_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&slot->descriptor_sets[1], 0, NULL);
	groups_x = (resolve_x2 - resolve_x1 + 16u) / 16u;
	groups_y = (resolve_y2 - resolve_y1 + 16u) / 16u;
	implementation->command_dispatch(implementation->command_buffer,
		groups_x, groups_y, 1u);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
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
	Each tile is written by exactly one workgroup. The CPU only bins ordered
	command indices; all per-pixel PUT decisions remain in the shader. This is
	the safe way to collapse adjacent overlapping sprites into one dispatch.
*/
static int vulkan_surface_blit_tile_submit(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage,
	const FB_GFX3_VULKAN_BLIT_COMMAND *prepared_commands,
	size_t operation_count)
{
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *range_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *index_buffer;
	FB_GFX3_VK_PIPELINE tile_pipeline;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[5];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[5];
	FB_GFX3_VULKAN_BLIT_COMMAND *commands;
	uint32_t *tile_counts = NULL;
	uint32_t *tile_ranges = NULL;
	uint32_t *tile_cursors = NULL;
	uint32_t *tile_indices = NULL;
	FB_GFX3_VULKAN_BLIT_TRANS_TILE_COMMAND *trans_tile_commands = NULL;
	uint32_t tiles_x;
	uint32_t tiles_y;
	uint32_t tile_count;
	uint32_t groups_x;
	uint32_t groups_y;
	uint32_t bin_size;
	uint32_t index_count = 0;
	uint32_t active_tile_count = 0;
	uint32_t index;
	size_t command_size;
	size_t count_size;
	size_t cursor_size;
	size_t index_size;
	size_t range_size;
	size_t scratch_size;
	int trans_pipeline_index = -1;
	int result = FB_GFX3_OK;

	if ((runtime == NULL) || (implementation == NULL) ||
	    (destination_storage == NULL) || (source_storage == NULL) ||
	    (prepared_commands == NULL) || (operation_count < 2u) ||
	    (operation_count > FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT) ||
	    (implementation->blit_tile_pipeline == 0))
		return FB_GFX3_UNSUPPORTED;
	/* 0x8086 is Intel's PCI vendor identifier. */
	if ((runtime->selected_vendor_id == 0x8086u) &&
	    (prepared_commands[0].format[1] == FB_GFX3_BLIT_TRANS)) {
		uint32_t depth = prepared_commands[0].format[0];

		trans_pipeline_index = (depth <= 8u) ? 0 :
			((depth == 16u) ? 1 : 2);
		for (index = 1u; index < (uint32_t)operation_count; index++) {
			if ((prepared_commands[index].format[1] != FB_GFX3_BLIT_TRANS) ||
			    (prepared_commands[index].format[0] != depth)) {
				trans_pipeline_index = -1;
				break;
			}
		}
	}
	/*
		The fixed 8 by 8 SPIR-V variant reduced RTX coverage work, while Intel's
		driver strongly preferred the fixed 16 by 16 variant. Keep the choice out
		of shader control flow so both compilers see constant shifts. 0x10DE is
		the PCI vendor identifier assigned to NVIDIA.
	*/
	if ((runtime->selected_vendor_id == 0x10DEu) &&
	    (implementation->blit_tile_nvidia_pipeline != 0)) {
		bin_size = 8u;
		tile_pipeline = implementation->blit_tile_nvidia_pipeline;
	} else {
		bin_size = 16u;
		tile_pipeline = implementation->blit_tile_pipeline;
	}
	if ((trans_pipeline_index >= 0) &&
	    (implementation->blit_tile_trans_pipeline[
		trans_pipeline_index] != 0)) {
		/* Intel's compact TRANS programs use one 16 by 16 workgroup per bin. */
		bin_size = 16u;
		tile_pipeline = implementation->blit_tile_trans_pipeline[
			trans_pipeline_index];
	}
	tiles_x = (prepared_commands[0].dimensions[1] + bin_size - 1u) /
		bin_size;
	tiles_y = (prepared_commands[0].dimensions[2] + bin_size - 1u) /
		bin_size;
	groups_x = (prepared_commands[0].dimensions[1] + 15u) / 16u;
	groups_y = (prepared_commands[0].dimensions[2] + 15u) / 16u;
	if ((tiles_x == 0u) || (tiles_y == 0u) ||
	    (groups_x == 0u) || (groups_y == 0u) ||
	    (tiles_x > UINT32_MAX / tiles_y) ||
	    ((tile_count = tiles_x * tiles_y) > 65536u))
		return FB_GFX3_UNSUPPORTED;
	if (fb_gfx3_vk_size_multiply(tile_count, sizeof(*tile_counts),
	    &count_size) != FB_GFX3_OK)
		return FB_GFX3_OUT_OF_MEMORY;
	if (implementation->blit_tile_count_scratch_size < count_size) {
		void *replacement = realloc(
			implementation->blit_tile_count_scratch, count_size);

		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		implementation->blit_tile_count_scratch = replacement;
		implementation->blit_tile_count_scratch_size = count_size;
	}
	tile_counts = (uint32_t *)implementation->blit_tile_count_scratch;
	memset(tile_counts, 0, count_size);
	for (index = 0u; index < (uint32_t)operation_count; ++index) {
		const FB_GFX3_VULKAN_BLIT_COMMAND *command = &prepared_commands[index];
		int32_t left = (command->destination[0] > command->clip.x1) ?
			command->destination[0] : command->clip.x1;
		int32_t top = (command->destination[1] > command->clip.y1) ?
			command->destination[1] : command->clip.y1;
		int64_t right64 = (int64_t)command->destination[0] +
			command->source_rect[2] - 1;
		int64_t bottom64 = (int64_t)command->destination[1] +
			command->source_rect[3] - 1;
		int32_t right = (right64 < command->clip.x2) ? (int32_t)right64 :
			command->clip.x2;
		int32_t bottom = (bottom64 < command->clip.y2) ? (int32_t)bottom64 :
			command->clip.y2;
		uint32_t tile_y;

		if ((left > right) || (top > bottom))
			continue;
		for (tile_y = (uint32_t)top / bin_size;
		     tile_y <= (uint32_t)bottom / bin_size;
		     ++tile_y) {
			uint32_t tile_x;
			for (tile_x = (uint32_t)left / bin_size;
			     tile_x <= (uint32_t)right / bin_size;
			     ++tile_x) {
				uint32_t tile = tile_y * tiles_x + tile_x;
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
		if (tile_counts[index] != 0u)
			active_tile_count++;
	}
	if (index_count == 0u)
		goto cleanup;
	if ((trans_pipeline_index >= 0) && (active_tile_count > 65535u)) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	if (fb_gfx3_vk_size_multiply(index_count, (trans_pipeline_index >= 0) ?
	    sizeof(*trans_tile_commands) : sizeof(*tile_indices),
	    &index_size) != FB_GFX3_OK) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if (fb_gfx3_vk_size_multiply((trans_pipeline_index >= 0) ?
	    (size_t)active_tile_count * 4u : (size_t)tile_count * 2u,
	    sizeof(*tile_ranges), &range_size) != FB_GFX3_OK) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if ((fb_gfx3_vk_size_multiply(tile_count, sizeof(*tile_cursors),
	     &cursor_size) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(range_size, cursor_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_add(scratch_size, index_size, &scratch_size) !=
	     FB_GFX3_OK)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if (implementation->blit_tile_output_scratch_size < scratch_size) {
		void *replacement = realloc(
			implementation->blit_tile_output_scratch, scratch_size);

		if (replacement == NULL) {
			result = FB_GFX3_OUT_OF_MEMORY;
			goto cleanup;
		}
		implementation->blit_tile_output_scratch = replacement;
		implementation->blit_tile_output_scratch_size = scratch_size;
	}
	tile_ranges = (uint32_t *)implementation->blit_tile_output_scratch;
	tile_cursors = (uint32_t *)((unsigned char *)tile_ranges + range_size);
	if (trans_pipeline_index >= 0) {
		trans_tile_commands = (FB_GFX3_VULKAN_BLIT_TRANS_TILE_COMMAND *)
			((unsigned char *)tile_cursors + cursor_size);
	} else {
		tile_indices = (uint32_t *)
			((unsigned char *)tile_cursors + cursor_size);
	}
	{
		uint32_t cursor = 0u;
		uint32_t active_index = 0u;
		for (index = 0u; index < tile_count; index++) {
			if (trans_pipeline_index >= 0) {
				if (tile_counts[index] != 0u) {
					tile_ranges[active_index * 4u] = index % tiles_x;
					tile_ranges[active_index * 4u + 1u] = index / tiles_x;
					tile_ranges[active_index * 4u + 2u] = cursor;
					tile_ranges[active_index * 4u + 3u] = tile_counts[index];
					active_index++;
				}
			} else {
				tile_ranges[index * 2u] = cursor;
				tile_ranges[index * 2u + 1u] = tile_counts[index];
			}
			tile_cursors[index] = cursor;
			cursor += tile_counts[index];
		}
	}
	if (trans_pipeline_index >= 0) {
		groups_x = active_tile_count;
		groups_y = 1u;
	}
	for (index = 0u; index < (uint32_t)operation_count; ++index) {
		const FB_GFX3_VULKAN_BLIT_COMMAND *command = &prepared_commands[index];
		int32_t left = (command->destination[0] > command->clip.x1) ?
			command->destination[0] : command->clip.x1;
		int32_t top = (command->destination[1] > command->clip.y1) ?
			command->destination[1] : command->clip.y1;
		int64_t right64 = (int64_t)command->destination[0] +
			command->source_rect[2] - 1;
		int64_t bottom64 = (int64_t)command->destination[1] +
			command->source_rect[3] - 1;
		int32_t right = (right64 < command->clip.x2) ?
			(int32_t)right64 : command->clip.x2;
		int32_t bottom = (bottom64 < command->clip.y2) ?
			(int32_t)bottom64 : command->clip.y2;
		int64_t source_bias =
			((int64_t)command->source_rect[1] - command->destination[1]) *
			command->destination[2] + command->source_rect[0] -
			command->destination[0];
		uint32_t tile_y;

		if ((left > right) || (top > bottom))
			continue;
		if ((trans_pipeline_index >= 0) &&
		    ((source_bias < INT32_MIN) || (source_bias > INT32_MAX))) {
			result = FB_GFX3_UNSUPPORTED;
			goto cleanup;
		}
		for (tile_y = (uint32_t)top / bin_size;
		     tile_y <= (uint32_t)bottom / bin_size;
		     tile_y++) {
			uint32_t tile_x;
			for (tile_x = (uint32_t)left / bin_size;
			     tile_x <= (uint32_t)right / bin_size;
			     tile_x++) {
				uint32_t tile = tile_y * tiles_x + tile_x;
				uint32_t cursor = tile_cursors[tile]++;

				if (trans_pipeline_index >= 0) {
					int32_t tile_left = (int32_t)(tile_x * bin_size);
					int32_t tile_top = (int32_t)(tile_y * bin_size);
					int32_t coverage_left = (left > tile_left) ?
						left : tile_left;
					int32_t coverage_top = (top > tile_top) ? top : tile_top;
					int32_t tile_right = tile_left + (int32_t)bin_size - 1;
					int32_t tile_bottom = tile_top + (int32_t)bin_size - 1;
					int32_t coverage_right = (right < tile_right) ?
						right : tile_right;
					int32_t coverage_bottom = (bottom < tile_bottom) ?
						bottom : tile_bottom;
					uint32_t coverage_width = (uint32_t)
						(coverage_right - coverage_left + 1);
					uint32_t coverage_height = (uint32_t)
						(coverage_bottom - coverage_top + 1);
					uint32_t column_mask = ((1u << coverage_width) - 1u) <<
						(uint32_t)(coverage_left - tile_left);
					uint32_t row_mask = ((1u << coverage_height) - 1u) <<
						(uint32_t)(coverage_top - tile_top);

					trans_tile_commands[cursor].coverage =
						column_mask | (row_mask << 16);
					trans_tile_commands[cursor].source_bias =
						(int32_t)source_bias;
				} else {
					tile_indices[cursor] = index;
				}
			}
		}
	}
	slot = &implementation->submission_slots[implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	range_buffer = &slot->blit_range_buffer;
	index_buffer = &slot->blit_index_buffer;
	if (range_buffer->size < range_size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			range_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation, range_buffer,
			range_size,
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
	}
	if (index_buffer->size < index_size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			index_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation, index_buffer,
			index_size,
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
	}
	memcpy(range_buffer->mapped, tile_ranges, range_size);
	if (trans_pipeline_index >= 0)
		memcpy(index_buffer->mapped, trans_tile_commands, index_size);
	else
		memcpy(index_buffer->mapped, tile_indices, index_size);
	command_size = (trans_pipeline_index >= 0) ? sizeof(*commands) :
		operation_count * sizeof(*commands);
	if (slot->blit_command_buffer.size < command_size) {
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_command_buffer);
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->blit_command_buffer,
			(trans_pipeline_index >= 0) ? sizeof(*commands) :
			(uint64_t)FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT * sizeof(*commands),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK) goto cleanup;
	}
	commands = (FB_GFX3_VULKAN_BLIT_COMMAND *)slot->blit_command_buffer.mapped;
	memcpy(commands, prepared_commands, command_size);
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	buffer_info[0].buffer = destination_storage->storage.buffer;
	buffer_info[0].range = destination_storage->storage.size;
	buffer_info[1].buffer = source_storage->storage.buffer;
	buffer_info[1].range = source_storage->storage.size;
	buffer_info[2].buffer = slot->blit_command_buffer.buffer;
	buffer_info[2].range = command_size;
	buffer_info[3].buffer = range_buffer->buffer;
	buffer_info[3].range = range_size;
	buffer_info[4].buffer = index_buffer->buffer;
	buffer_info[4].range = index_size;
	for (index = 0u; index < 5u; index++) {
		descriptor_writes[index].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[index].destination_set = slot->descriptor_sets[0];
		descriptor_writes[index].destination_binding = index;
		descriptor_writes[index].descriptor_count = 1;
		descriptor_writes[index].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[index].buffer_info = &buffer_info[index];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device, 5u,
		descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, slot->blit_command_buffer.buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, range_buffer->buffer,
		range_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, index_buffer->buffer,
		index_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, source_storage->storage.buffer,
		source_storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, destination_storage->storage.buffer,
		destination_storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT, FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, tile_pipeline);
	implementation->command_bind_descriptor_sets(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&slot->descriptor_sets[0], 0, NULL);
	implementation->command_dispatch(implementation->command_buffer,
		groups_x, groups_y, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation, destination_storage->storage.buffer,
		destination_storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK) runtime->completed_submission_count++;

cleanup:
	return result;
}

int fb_gfx3_vulkan_surface_blit_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source, const FB_GFX3_VULKAN_BLIT *blits,
	size_t blit_count)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_IMPLEMENTATION *source_implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *destination_storage;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *source_storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_BLIT_COMMAND *prepared_commands = NULL;
	FB_GFX3_VULKAN_BLIT_COMMAND *commands;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_info[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 3];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[
		FB_GFX3_VK_BLIT_BATCH_LIMIT * 3];
	uint32_t group_count_x[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	size_t operation_count = 0;
	size_t index;
	size_t command_size;
	int result;

	if ((blits == NULL) || (blit_count < 2) ||
	    (blit_count > FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT))
		return FB_GFX3_INVALID;
	result = fb_gfx3_vk_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_surface_validate(runtime, source, &source_implementation,
		&source_storage);
	if (result != FB_GFX3_OK)
		return result;
	/* Same-surface PUT needs a snapshot per operation and stays on the safe path. */
	if ((implementation != source_implementation) ||
	    (destination->depth != source->depth) ||
	    (destination_storage == source_storage))
		return FB_GFX3_UNSUPPORTED;
	if (fb_gfx3_vk_size_multiply(blit_count, sizeof(*commands), &command_size) !=
	    FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;
	if (implementation->blit_prepare_scratch_size < command_size) {
		void *replacement = realloc(implementation->blit_prepare_scratch,
			command_size);

		if (replacement == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		implementation->blit_prepare_scratch = replacement;
		implementation->blit_prepare_scratch_size = command_size;
	}
	prepared_commands = (FB_GFX3_VULKAN_BLIT_COMMAND *)
		implementation->blit_prepare_scratch;
	for (index = 0; index < blit_count; index++) {
		const FB_GFX3_VULKAN_BLIT *blit = &blits[index];
		FB_GFX3_RECT clipped;
		int64_t destination_x2;
		int64_t destination_y2;
		int32_t visible_left;
		int32_t visible_top;
		int32_t visible_right;
		int32_t visible_bottom;
		uint32_t source_skip_x;
		uint32_t source_skip_y;
		size_t pixel_count_size;
		uint32_t original_width;
		uint32_t original_height;
		uint32_t width;
		uint32_t height;

		if ((blit->source_rect.x1 < 0) || (blit->source_rect.y1 < 0) ||
		    (blit->source_rect.x1 > blit->source_rect.x2) ||
		    (blit->source_rect.y1 > blit->source_rect.y2) ||
		    (blit->source_rect.x2 >= (int32_t)source->width) ||
		    (blit->source_rect.y2 >= (int32_t)source->height)) {
			result = FB_GFX3_INVALID;
			goto cleanup;
		}
		switch (blit->mode) {
		case FB_GFX3_BLIT_TRANS:
		case FB_GFX3_BLIT_PSET:
		case FB_GFX3_BLIT_PRESET:
		case FB_GFX3_BLIT_AND:
		case FB_GFX3_BLIT_OR:
		case FB_GFX3_BLIT_XOR:
		case FB_GFX3_BLIT_ALPHA:
		case FB_GFX3_BLIT_ADD:
		case FB_GFX3_BLIT_BLEND:
			break;
		default:
			result = FB_GFX3_UNSUPPORTED;
			goto cleanup;
		}
		clipped = blit->clip;
		if (clipped.x1 < 0)
			clipped.x1 = 0;
		if (clipped.y1 < 0)
			clipped.y1 = 0;
		if (clipped.x2 >= (int32_t)destination->width)
			clipped.x2 = (int32_t)destination->width - 1;
		if (clipped.y2 >= (int32_t)destination->height)
			clipped.y2 = (int32_t)destination->height - 1;
		if ((clipped.x1 > clipped.x2) || (clipped.y1 > clipped.y2))
			continue;
		original_width = (uint32_t)(blit->source_rect.x2 -
			blit->source_rect.x1 + 1);
		original_height = (uint32_t)(blit->source_rect.y2 -
			blit->source_rect.y1 + 1);
		width = original_width;
		height = original_height;
		destination_x2 = (int64_t)blit->destination_x + width - 1;
		destination_y2 = (int64_t)blit->destination_y + height - 1;
		if ((destination_x2 < clipped.x1) ||
		    (destination_y2 < clipped.y1) ||
		    (blit->destination_x > clipped.x2) ||
		    (blit->destination_y > clipped.y2))
			continue;
		/*
			The public producer deliberately leaves partial GPU sprites untrimmed.
			Reduce only the compute dispatch rectangle here on the renderer thread.
			Shaders still own transparent-key tests, source sampling, overlap order,
			and every destination write.
		*/
		visible_left = (blit->destination_x > clipped.x1) ?
			blit->destination_x : clipped.x1;
		visible_top = (blit->destination_y > clipped.y1) ?
			blit->destination_y : clipped.y1;
		visible_right = (destination_x2 < clipped.x2) ?
			(int32_t)destination_x2 : clipped.x2;
		visible_bottom = (destination_y2 < clipped.y2) ?
			(int32_t)destination_y2 : clipped.y2;
		source_skip_x = (uint32_t)((int64_t)visible_left -
			blit->destination_x);
		source_skip_y = (uint32_t)((int64_t)visible_top -
			blit->destination_y);
		width = (uint32_t)(visible_right - visible_left) + 1u;
		height = (uint32_t)(visible_bottom - visible_top) + 1u;
		if ((fb_gfx3_vk_size_multiply(width, height, &pixel_count_size) !=
		     FB_GFX3_OK) || (pixel_count_size > UINT32_MAX)) {
			result = FB_GFX3_UNSUPPORTED;
			goto cleanup;
		}
		prepared_commands[operation_count].source_rect[0] =
			blit->source_rect.x1 + (int32_t)source_skip_x;
		prepared_commands[operation_count].source_rect[1] =
			blit->source_rect.y1 + (int32_t)source_skip_y;
		prepared_commands[operation_count].source_rect[2] = (int32_t)width;
		prepared_commands[operation_count].source_rect[3] = (int32_t)height;
		prepared_commands[operation_count].clip = clipped;
		prepared_commands[operation_count].destination[0] = visible_left;
		prepared_commands[operation_count].destination[1] = visible_top;
		prepared_commands[operation_count].destination[2] = (int32_t)source->width;
		prepared_commands[operation_count].destination[3] = (int32_t)destination->width;
		prepared_commands[operation_count].format[0] = destination->depth;
		prepared_commands[operation_count].format[1] = blit->mode;
		prepared_commands[operation_count].format[2] = blit->alpha;
		prepared_commands[operation_count].format[3] =
			fb_gfx3_vk_surface_color_mask(destination->depth);
		prepared_commands[operation_count].dimensions[0] = source->width;
		prepared_commands[operation_count].dimensions[1] = destination->width;
		prepared_commands[operation_count].dimensions[2] = destination->height;
		/* Reserved until winner submission replaces command zero with its origin. */
		prepared_commands[operation_count].dimensions[3] =
			((width != original_width) || (height != original_height)) ? 1u : 0u;
		if (operation_count < FB_GFX3_VK_BLIT_BATCH_LIMIT) {
			group_count_x[operation_count] =
				((uint32_t)pixel_count_size - 1u) / 64u + 1u;
		}
		operation_count++;
	}
	if (operation_count == 0) {
		result = FB_GFX3_OK;
		goto cleanup;
	}
	command_size = operation_count * sizeof(*commands);
	result = vulkan_surface_blit_winner_submit(runtime, implementation,
		destination_storage, source_storage, prepared_commands,
		operation_count);
	if (result != FB_GFX3_UNSUPPORTED)
		goto cleanup;
	result = vulkan_surface_blit_tile_submit(runtime, implementation,
		destination_storage, source_storage, prepared_commands, operation_count);
	if (result != FB_GFX3_UNSUPPORTED)
		goto cleanup;
	if (operation_count > FB_GFX3_VK_BLIT_BATCH_LIMIT) {
		result = FB_GFX3_UNSUPPORTED;
		goto cleanup;
	}
	/*
		The next command slot is the one fb_gfx3_vk_begin_commands() will select.
		Wait before reusing its mapped records, then retain the allocation for
		all future batches in that slot.  This matches descriptor-set lifetime
		and does not add a wait that the old begin path did not already require.
	*/
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	if (slot->blit_command_buffer.buffer == 0) {
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&slot->blit_command_buffer,
			(uint64_t)FB_GFX3_VK_BLIT_BATCH_LIMIT * sizeof(*commands),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
	}
	commands = (FB_GFX3_VULKAN_BLIT_COMMAND *)
		slot->blit_command_buffer.mapped;
	memcpy(commands, prepared_commands,
		operation_count * sizeof(prepared_commands[0]));
	memset(buffer_info, 0, sizeof(buffer_info));
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	for (index = 0; index < operation_count; index++) {
		size_t write_index = index * 3;

		buffer_info[write_index].buffer = destination_storage->storage.buffer;
		buffer_info[write_index].range = destination_storage->storage.size;
		buffer_info[write_index + 1].buffer = source_storage->storage.buffer;
		buffer_info[write_index + 1].range = source_storage->storage.size;
		buffer_info[write_index + 2].buffer = slot->blit_command_buffer.buffer;
		buffer_info[write_index + 2].offset = index * sizeof(*commands);
		buffer_info[write_index + 2].range = sizeof(*commands);
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
		descriptor_writes[write_index + 2] = descriptor_writes[write_index];
		descriptor_writes[write_index + 2].destination_binding = 2;
		descriptor_writes[write_index + 2].buffer_info =
			&buffer_info[write_index + 2];
	}
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	/* The selected slot has completed before its descriptor sets are rewritten. */
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(operation_count * 3), descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation, slot->blit_command_buffer.buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, source_storage->storage.buffer,
		source_storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		destination_storage->storage.buffer, destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE, implementation->blit_pipeline);
	for (index = 0; index < operation_count; index++) {
		FB_GFX3_VK_DESCRIPTOR_SET descriptor_set =
			implementation->submission_slots[
			implementation->active_submission_slot].descriptor_sets[index];

		implementation->command_bind_descriptor_sets(
			implementation->command_buffer, FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
			implementation->compute_pipeline_layout, 0, 1, &descriptor_set, 0,
			NULL);
		implementation->command_dispatch(implementation->command_buffer,
			group_count_x[index], 1, 1);
		fb_gfx3_vk_record_buffer_barrier(implementation,
			destination_storage->storage.buffer,
			destination_storage->storage.size,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
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
	return result;
}

#endif

/* end of gfx3_vulkan_blit.c */
