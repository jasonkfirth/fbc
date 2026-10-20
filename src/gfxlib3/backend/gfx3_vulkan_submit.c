/*
 * FreeBASIC gfxlib3: gfx3_vulkan_submit.c
 * Purpose: Own Vulkan queue submission, fences and transfer storage.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_vulkan_private.h"

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || defined(HOST_DARWIN)

void fb_gfx3_vk_buffer_allocation_destroy_immediate(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation);
int fb_gfx3_vk_submission_slot_wait(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot);
int fb_gfx3_vk_batch_flush_commands(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);

/* ------------------------------------------------------------------------- */
/* Command recording and submission ownership                               */
/* ------------------------------------------------------------------------- */

static void vulkan_submission_mark_recorded(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot)
{
	if ((implementation == NULL) || (slot == NULL))
		return;
	slot->pending_submission = TRUE;
	slot->submitted = FALSE;
	slot->submission_fence = 0;
	slot->first_command_sequence = 0;
	slot->command_sequence = 0;
	slot->submission_serial = ++implementation->next_submission_serial;
	implementation->latest_submission_slot =
		implementation->active_submission_slot;
	implementation->latest_submission_serial = slot->submission_serial;
}

static void vulkan_submission_mark_batch_submitted(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_FENCE submission_fence)
{
	uint32_t index;

	if ((implementation == NULL) || (submission_fence == 0))
		return;
	for (index = 0u; index < implementation->batch_submission_slot_count;
	     index++) {
		FB_GFX3_VULKAN_SUBMISSION_SLOT *slot =
			&implementation->submission_slots[
			implementation->batch_submission_slots[index]];

		slot->pending_submission = FALSE;
		slot->submitted = TRUE;
		slot->submission_fence = submission_fence;
	}
	implementation->batch_submission_slot_count = 0u;
	implementation->batch_command_owner_slot = UINT32_MAX;
	implementation->batch_command_recording = FALSE;
}

int fb_gfx3_vk_begin_commands(FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	FB_GFX3_VK_COMMAND_BUFFER_BEGIN_INFO begin_info;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VK_RESULT result;
	uint32_t slot_index;

	if (implementation == NULL)
		return FB_GFX3_INVALID;
	if (implementation->batch_enabled &&
	    (implementation->batch_submission_slot_count >=
	     implementation->batch_operation_limit)) {
		if (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	slot_index = implementation->next_submission_slot;
	implementation->next_submission_slot++;
	if (implementation->next_submission_slot >=
	    FB_GFX3_VK_SUBMISSION_SLOT_COUNT)
		implementation->next_submission_slot = 0;
	slot = &implementation->submission_slots[slot_index];
	if (slot->submitted) {
		if (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	implementation->active_submission_slot = slot_index;
	implementation->image_available = slot->image_available;
	implementation->compute_descriptor_set = slot->descriptor_sets[0];
	if (implementation->batch_enabled &&
	    implementation->batch_command_recording) {
		implementation->batch_submission_slots[
			implementation->batch_submission_slot_count++] = slot_index;
		return FB_GFX3_OK;
	}
	implementation->command_pool = slot->command_pool;
	implementation->command_buffer = slot->command_buffer;
	implementation->fence = slot->fence;
	result = implementation->reset_command_pool(implementation->device,
		implementation->command_pool, 0);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	memset(&begin_info, 0, sizeof(begin_info));
	begin_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags =
		FB_GFX3_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	result = implementation->begin_command_buffer(
		implementation->command_buffer, &begin_info);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	if (implementation->batch_enabled) {
		implementation->batch_command_owner_slot = slot_index;
		implementation->batch_submission_slots[0] = slot_index;
		implementation->batch_submission_slot_count = 1u;
		implementation->batch_command_recording = TRUE;
	}
	return FB_GFX3_OK;
}

int fb_gfx3_vk_batch_flush_commands(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	FB_GFX3_VK_SUBMIT_INFO submit_info;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *owner;
	FB_GFX3_VK_RESULT result;

	if (implementation == NULL)
		return FB_GFX3_INVALID;
	if (!implementation->batch_command_recording)
		return FB_GFX3_OK;
	if ((implementation->batch_command_owner_slot >=
	     FB_GFX3_VK_SUBMISSION_SLOT_COUNT) ||
	    (implementation->batch_submission_slot_count == 0u) ||
	    (implementation->batch_submission_slot_count >
	     implementation->batch_operation_limit))
		return FB_GFX3_FAILED;
	owner = &implementation->submission_slots[
		implementation->batch_command_owner_slot];
	result = implementation->end_command_buffer(
		implementation->command_buffer);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	result = implementation->reset_fences(implementation->device, 1,
		&owner->fence);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.structure_type = FB_GFX3_VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.command_buffer_count = 1;
	submit_info.command_buffers = &implementation->command_buffer;
	result = implementation->queue_submit(implementation->queue, 1,
		&submit_info, owner->fence);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	implementation->queue_submit_count++;
	vulkan_submission_mark_batch_submitted(implementation, owner->fence);
	return FB_GFX3_OK;
}

int fb_gfx3_vk_end_commands(FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	int wait_for_completion)
{
	FB_GFX3_VK_SUBMIT_INFO submit_info;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) ||
	    (implementation->active_submission_slot >=
	     FB_GFX3_VK_SUBMISSION_SLOT_COUNT))
		return FB_GFX3_INVALID;
	slot = &implementation->submission_slots[
		implementation->active_submission_slot];
	if (implementation->batch_enabled) {
		vulkan_submission_mark_recorded(implementation, slot);
		if (!wait_for_completion)
			return FB_GFX3_OK;
		if (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
		return fb_gfx3_vk_submission_slot_wait(implementation, slot);
	}
	result = implementation->end_command_buffer(
		implementation->command_buffer);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	result = implementation->reset_fences(implementation->device, 1,
		&implementation->fence);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.structure_type = FB_GFX3_VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.command_buffer_count = 1;
	submit_info.command_buffers = &implementation->command_buffer;
	result = implementation->queue_submit(implementation->queue, 1,
		&submit_info, implementation->fence);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	implementation->queue_submit_count++;
	slot->submitted = TRUE;
	slot->pending_submission = FALSE;
	slot->submission_fence = slot->fence;
	slot->first_command_sequence = 0;
	slot->command_sequence = 0;
	slot->submission_serial = ++implementation->next_submission_serial;
	implementation->latest_submission_slot =
		implementation->active_submission_slot;
	implementation->latest_submission_serial = slot->submission_serial;
	if (!wait_for_completion)
		return FB_GFX3_OK;
	return fb_gfx3_vk_submission_slot_wait(implementation, slot);
}

int fb_gfx3_vk_end_present_commands(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation, uint32_t image_index,
	int *recreate)
{
	FB_GFX3_VK_SUBMIT_INFO submit_info;
	FB_GFX3_VK_PRESENT_INFO present_info;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VK_FENCE submission_fence;
	FB_GFX3_VK_FLAGS wait_stage = FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT;
	FB_GFX3_VK_RESULT present_result;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) || (recreate == NULL) ||
	    (image_index >= implementation->swapchain_image_count) ||
	    (implementation->active_submission_slot >=
	     FB_GFX3_VK_SUBMISSION_SLOT_COUNT))
		return FB_GFX3_INVALID;
	slot = &implementation->submission_slots[
		implementation->active_submission_slot];
	if (implementation->batch_enabled) {
		if ((implementation->batch_command_owner_slot >=
		     FB_GFX3_VK_SUBMISSION_SLOT_COUNT) ||
		    !implementation->batch_command_recording)
			return FB_GFX3_FAILED;
		vulkan_submission_mark_recorded(implementation, slot);
		submission_fence = implementation->submission_slots[
			implementation->batch_command_owner_slot].fence;
	} else {
		submission_fence = slot->fence;
	}
	*recreate = FALSE;
	result = implementation->end_command_buffer(
		implementation->command_buffer);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	/*
		The slot fence owns the command pool, acquire semaphore, descriptors,
		and mapped per-frame buffers. The render-finished semaphore is not slot
		owned: vkQueuePresentKHR may retain it after this fence signals, so the
		acquired swapchain image selects that separate semaphore.
	*/
	result = implementation->reset_fences(implementation->device, 1,
		&submission_fence);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.structure_type = FB_GFX3_VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.wait_semaphore_count = 1;
	submit_info.wait_semaphores = &implementation->image_available;
	submit_info.wait_stage_masks = &wait_stage;
	submit_info.command_buffer_count = 1;
	submit_info.command_buffers = &implementation->command_buffer;
	submit_info.signal_semaphore_count = 1;
	submit_info.signal_semaphores = &implementation->rendering_finished;
	result = implementation->queue_submit(implementation->queue, 1,
		&submit_info, submission_fence);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	implementation->queue_submit_count++;
	if (implementation->batch_enabled) {
		vulkan_submission_mark_batch_submitted(implementation,
			submission_fence);
	} else {
		slot->submitted = TRUE;
		slot->pending_submission = FALSE;
		slot->submission_fence = submission_fence;
		slot->first_command_sequence = 0;
		slot->command_sequence = 0;
		slot->submission_serial = ++implementation->next_submission_serial;
		implementation->latest_submission_slot =
			implementation->active_submission_slot;
		implementation->latest_submission_serial = slot->submission_serial;
	}
	memset(&present_info, 0, sizeof(present_info));
	present_info.structure_type = FB_GFX3_VK_STRUCTURE_TYPE_PRESENT_INFO;
	present_info.wait_semaphore_count = 1;
	present_info.wait_semaphores = &implementation->rendering_finished;
	present_info.swapchain_count = 1;
	present_info.swapchains = &implementation->swapchain;
	present_info.image_indices = &image_index;
	present_result = implementation->queue_present(implementation->queue,
		&present_info);
	if ((present_result != FB_GFX3_VK_SUCCESS) &&
	    (present_result != FB_GFX3_VK_SUBOPTIMAL) &&
	    (present_result != FB_GFX3_VK_ERROR_OUT_OF_DATE))
		return FB_GFX3_FAILED;

	if ((present_result == FB_GFX3_VK_SUBOPTIMAL) ||
	    (present_result == FB_GFX3_VK_ERROR_OUT_OF_DATE)) {
		*recreate = TRUE;
		return FB_GFX3_OK;
	}
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Fences and deferred storage                                              */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vk_find_memory_type(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	uint32_t allowed_types, FB_GFX3_VK_FLAGS required_properties,
	uint32_t *memory_type_index)
{
	FB_GFX3_VK_PHYSICAL_DEVICE_MEMORY_PROPERTIES properties;
	uint32_t index;

	if ((implementation == NULL) || (memory_type_index == NULL) ||
	    (implementation->get_physical_device_memory_properties == NULL))
		return FB_GFX3_INVALID;
	memset(&properties, 0, sizeof(properties));
	implementation->get_physical_device_memory_properties(
		implementation->physical_device, &properties);
	if (properties.memory_type_count > 32)
		return FB_GFX3_FAILED;
	for (index = 0; index < properties.memory_type_count; index++) {
		if (((allowed_types & (1u << index)) != 0) &&
		    ((properties.memory_types[index].property_flags &
		      required_properties) == required_properties)) {
			*memory_type_index = index;
			return FB_GFX3_OK;
		}
	}
	return FB_GFX3_UNSUPPORTED;
}

void fb_gfx3_vk_buffer_allocation_destroy_immediate(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation)
{
	if ((implementation == NULL) || (allocation == NULL))
		return;
	if (allocation->mapped != NULL)
		implementation->unmap_memory(implementation->device,
			allocation->memory);
	if (allocation->buffer != 0)
		implementation->destroy_buffer(implementation->device,
			allocation->buffer, NULL);
	if (allocation->memory != 0)
		implementation->free_memory(implementation->device,
			allocation->memory, NULL);
	memset(allocation, 0, sizeof(*allocation));
}

void fb_gfx3_vk_submission_slot_release_deferred(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot)
{
	size_t index;

	if ((implementation == NULL) || (slot == NULL))
		return;
	for (index = 0; index < slot->deferred_allocation_count; index++)
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->deferred_allocations[index]);
	slot->deferred_allocation_count = 0;
}

static void vulkan_submission_complete_fence(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_FENCE submission_fence)
{
	uint32_t index;

	if ((implementation == NULL) || (submission_fence == 0))
		return;
	for (index = 0u; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		FB_GFX3_VULKAN_SUBMISSION_SLOT *slot =
			&implementation->submission_slots[index];

		if (!slot->submitted ||
		    (slot->submission_fence != submission_fence))
			continue;
		if (slot->command_sequence >
		    implementation->completed_command_sequence)
			implementation->completed_command_sequence =
				slot->command_sequence;
		fb_gfx3_vk_submission_slot_release_deferred(implementation, slot);
		slot->first_command_sequence = 0;
		slot->command_sequence = 0;
		slot->submission_fence = 0;
		slot->submitted = FALSE;
	}
}

int fb_gfx3_vk_submission_slot_wait(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot)
{
	FB_GFX3_VK_FENCE submission_fence;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) || (slot == NULL))
		return FB_GFX3_INVALID;
	if (slot->pending_submission) {
		if (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	if (!slot->submitted)
		return FB_GFX3_OK;
	submission_fence = slot->submission_fence;
	if (submission_fence == 0)
		return FB_GFX3_FAILED;
	result = implementation->wait_for_fences(implementation->device, 1,
		&submission_fence, TRUE, UINT64_MAX);
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	vulkan_submission_complete_fence(implementation, submission_fence);
	return FB_GFX3_OK;
}

static int vulkan_submission_slot_poll(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot)
{
	FB_GFX3_VK_FENCE submission_fence;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) || (slot == NULL))
		return FB_GFX3_INVALID;
	if (slot->pending_submission)
		return FB_GFX3_EXHAUSTED;
	if (!slot->submitted)
		return FB_GFX3_OK;
	submission_fence = slot->submission_fence;
	if (submission_fence == 0)
		return FB_GFX3_FAILED;
	if (implementation->get_fence_status == NULL)
		return FB_GFX3_FAILED;
	result = implementation->get_fence_status(implementation->device,
		submission_fence);
	if (result == FB_GFX3_VK_NOT_READY)
		return FB_GFX3_EXHAUSTED;
	if (result != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	vulkan_submission_complete_fence(implementation, submission_fence);
	return FB_GFX3_OK;
}

int fb_gfx3_vk_submission_poll_next(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	uint32_t slot_index;
	int result;

	if (implementation == NULL)
		return FB_GFX3_INVALID;
	slot_index = implementation->next_poll_submission_slot;
	implementation->next_poll_submission_slot++;
	if (implementation->next_poll_submission_slot >=
	    FB_GFX3_VK_SUBMISSION_SLOT_COUNT)
		implementation->next_poll_submission_slot = 0u;
	slot = &implementation->submission_slots[slot_index];
	result = vulkan_submission_slot_poll(implementation, slot);
	return (result == FB_GFX3_EXHAUSTED) ? FB_GFX3_OK : result;
}

int fb_gfx3_vk_submission_wait_all(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	uint32_t index;

	if (implementation == NULL)
		return FB_GFX3_INVALID;
	if (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		if (fb_gfx3_vk_submission_slot_wait(implementation,
		    &implementation->submission_slots[index]) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	implementation->active_submission_slot = UINT32_MAX;
	return FB_GFX3_OK;
}

static uint32_t vulkan_submission_in_flight_count(
	const FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	uint32_t count = 0;
	uint32_t index;

	if (implementation == NULL)
		return 0;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		if (implementation->submission_slots[index].submitted ||
		    implementation->submission_slots[index].pending_submission)
			count++;
	}
	return count;
}

void fb_gfx3_vk_runtime_update_submission_telemetry(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	const FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	uint32_t count;

	if (runtime == NULL)
		return;
	count = vulkan_submission_in_flight_count(implementation);
	runtime->in_flight_submission_count = count;
	runtime->queue_submit_count = (implementation != NULL) ?
		implementation->queue_submit_count : 0u;
	if (count > runtime->maximum_in_flight_submission_count)
		runtime->maximum_in_flight_submission_count = count;
}

void fb_gfx3_vk_buffer_allocation_destroy(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation)
{
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;

	if ((implementation == NULL) || (allocation == NULL))
		return;
	if ((implementation->active_submission_slot <
	     FB_GFX3_VK_SUBMISSION_SLOT_COUNT)) {
		slot = &implementation->submission_slots[
			implementation->active_submission_slot];
		if (slot->submitted || slot->pending_submission) {
			if (slot->deferred_allocation_count >=
			    FB_GFX3_VK_DEFERRED_ALLOCATION_LIMIT) {
				/* Preserve correctness if an unusually large batch exhausts a slot. */
				(void)fb_gfx3_vk_submission_slot_wait(implementation, slot);
			} else {
				slot->deferred_allocations[slot->deferred_allocation_count++] =
					*allocation;
				memset(allocation, 0, sizeof(*allocation));
				return;
			}
		}
	}
	fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation, allocation);
}

/* ------------------------------------------------------------------------- */
/* Buffer allocation and visibility barriers                                */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vk_buffer_allocation_create(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t size,
	FB_GFX3_VK_FLAGS usage, FB_GFX3_VK_FLAGS memory_properties,
	int map_memory)
{
	FB_GFX3_VK_BUFFER_CREATE_INFO buffer_create_info;
	FB_GFX3_VK_MEMORY_REQUIREMENTS memory_requirements;
	FB_GFX3_VK_MEMORY_ALLOCATE_INFO memory_allocate_info;
	uint32_t memory_type_index;
	int result;

	if ((implementation == NULL) || (allocation == NULL) || (size == 0))
		return FB_GFX3_INVALID;
	memset(allocation, 0, sizeof(*allocation));
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharing_mode = FB_GFX3_VK_SHARING_MODE_EXCLUSIVE;
	if (implementation->create_buffer(implementation->device,
	    &buffer_create_info, NULL, &allocation->buffer) !=
	    FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	memset(&memory_requirements, 0, sizeof(memory_requirements));
	implementation->get_buffer_memory_requirements(implementation->device,
		allocation->buffer, &memory_requirements);
	result = fb_gfx3_vk_find_memory_type(implementation,
		memory_requirements.memory_type_bits, memory_properties,
		&memory_type_index);
	if (result != FB_GFX3_OK)
		goto fail;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocation_size = memory_requirements.size;
	memory_allocate_info.memory_type_index = memory_type_index;
	if (implementation->allocate_memory(implementation->device,
	    &memory_allocate_info, NULL, &allocation->memory) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail;
	}
	if (implementation->bind_buffer_memory(implementation->device,
	    allocation->buffer, allocation->memory, 0) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto fail;
	}
	if (map_memory &&
	    (implementation->map_memory(implementation->device,
	     allocation->memory, 0, size, 0, &allocation->mapped) !=
	     FB_GFX3_VK_SUCCESS)) {
		result = FB_GFX3_FAILED;
		goto fail;
	}
	allocation->size = size;
	return FB_GFX3_OK;

fail:
	fb_gfx3_vk_buffer_allocation_destroy(implementation, allocation);
	return result;
}

int fb_gfx3_vk_host_buffer_ensure(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t required_size)
{
	if ((implementation == NULL) || (allocation == NULL) ||
	    (required_size == 0u))
		return FB_GFX3_INVALID;
	if ((allocation->buffer != 0) && (allocation->mapped != NULL) &&
	    (allocation->size >= required_size))
		return FB_GFX3_OK;
	/* The caller has waited for the owning submission slot before growing it. */
	fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation, allocation);
	return fb_gfx3_vk_buffer_allocation_create(implementation, allocation,
		required_size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
}

int fb_gfx3_vk_device_storage_buffer_ensure(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t required_size)
{
	if ((implementation == NULL) || (allocation == NULL) ||
	    (required_size == 0u))
		return FB_GFX3_INVALID;
	if ((allocation->buffer != 0) && (allocation->size >= required_size))
		return FB_GFX3_OK;
	/* The caller has waited for the owning submission slot before growing it. */
	fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation, allocation);
	return fb_gfx3_vk_buffer_allocation_create(implementation, allocation,
		required_size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
}

void fb_gfx3_vk_record_memory_barrier(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_FLAGS source_access_mask,
	FB_GFX3_VK_FLAGS destination_access_mask,
	FB_GFX3_VK_FLAGS source_stage_mask,
	FB_GFX3_VK_FLAGS destination_stage_mask)
{
	FB_GFX3_VK_MEMORY_BARRIER barrier;

	memset(&barrier, 0, sizeof(barrier));
	barrier.structure_type = FB_GFX3_VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.source_access_mask = source_access_mask;
	barrier.destination_access_mask = destination_access_mask;
	implementation->command_pipeline_barrier(
		implementation->command_buffer, source_stage_mask,
		destination_stage_mask, 0, 1, &barrier, 0, NULL, 0, NULL);
}

void fb_gfx3_vk_record_buffer_barrier(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_BUFFER buffer, uint64_t size,
	FB_GFX3_VK_FLAGS source_access_mask,
	FB_GFX3_VK_FLAGS destination_access_mask,
	FB_GFX3_VK_FLAGS source_stage_mask,
	FB_GFX3_VK_FLAGS destination_stage_mask)
{
	FB_GFX3_VK_BUFFER_MEMORY_BARRIER barrier;

	memset(&barrier, 0, sizeof(barrier));
	barrier.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.source_access_mask = source_access_mask;
	barrier.destination_access_mask = destination_access_mask;
	barrier.source_queue_family_index = UINT32_MAX;
	barrier.destination_queue_family_index = UINT32_MAX;
	barrier.buffer = buffer;
	barrier.size = size;
	implementation->command_pipeline_barrier(
		implementation->command_buffer, source_stage_mask,
		destination_stage_mask, 0, 0, NULL, 1, &barrier, 0, NULL);
}

uint32_t fb_gfx3_vk_surface_bytes_per_pixel(uint32_t depth)
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

uint32_t fb_gfx3_vk_surface_color_mask(uint32_t depth)
{
	return (depth >= 32) ? UINT32_MAX : (1u << depth) - 1u;
}


/* ------------------------------------------------------------------------- */
/* Runtime completion and transfer probes                                   */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vulkan_runtime_wait_idle(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	/*
		A recorded batch is not visible to vkDeviceWaitIdle until it has been
		submitted. Flush first, then use the device-wide wait as the final
		external-idle guarantee.
	*/
	if (fb_gfx3_vk_batch_flush_commands(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	if ((implementation->device_wait_idle == NULL) ||
	    (implementation->device_wait_idle(implementation->device) !=
	     FB_GFX3_VK_SUCCESS))
		return FB_GFX3_FAILED;
	if (fb_gfx3_vk_submission_wait_all(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	fb_gfx3_vk_runtime_update_submission_telemetry(runtime, implementation);
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_runtime_tag_submission(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint64_t sequence)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL) || (sequence == 0))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	if (implementation->latest_submission_slot >=
	    FB_GFX3_VK_SUBMISSION_SLOT_COUNT)
		return FB_GFX3_FAILED;
	slot = &implementation->submission_slots[
		implementation->latest_submission_slot];
	if (slot->submission_serial != implementation->latest_submission_serial)
		return FB_GFX3_FAILED;
	if (implementation->latest_submission_serial !=
	    implementation->latest_tagged_submission_serial) {
		slot->first_command_sequence = sequence;
		implementation->latest_tagged_submission_serial =
			implementation->latest_submission_serial;
	}
	/* Adjacent PUT commands may share one submission; retain its full range. */
	slot->command_sequence = sequence;
	/*
		Some transfer helpers intentionally wait before returning. Their slot is
		already idle by the time the backend attaches the renderer sequence, so
		publish that completed sequence immediately instead of waiting for a
		slot reuse which will never occur for this submission.
	*/
	if (!slot->submitted && !slot->pending_submission &&
	    (sequence > implementation->completed_command_sequence))
		implementation->completed_command_sequence = sequence;
	return FB_GFX3_OK;
}

uint64_t fb_gfx3_vulkan_runtime_completed_sequence(
	FB_GFX3_VULKAN_RUNTIME *runtime)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL))
		return 0;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	return implementation->completed_command_sequence;
}

int fb_gfx3_vulkan_runtime_wait_sequence(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint64_t sequence)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	uint32_t index;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL) || (sequence == 0))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		slot = &implementation->submission_slots[index];
		if ((!slot->submitted && !slot->pending_submission) ||
		    (slot->first_command_sequence == 0) ||
		    (slot->first_command_sequence > sequence))
			continue;
		if (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	fb_gfx3_vk_runtime_update_submission_telemetry(runtime, implementation);
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_runtime_poll(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	int result;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	result = fb_gfx3_vk_submission_poll_next(implementation);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_vk_runtime_update_submission_telemetry(runtime, implementation);
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_runtime_submit_empty(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	int result;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	if ((implementation->device == NULL) ||
	    (implementation->queue == NULL) ||
	    (implementation->command_pool == 0) ||
	    (implementation->command_buffer == NULL) ||
	    (implementation->fence == 0))
		return FB_GFX3_INVALID;

	/*
	    The render thread is the sole owner of this pool and command buffer.
	    Waiting before reset makes the pool's external synchronization rule
	    explicit and leaves no previous submission pending during recording.
	*/
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	result = fb_gfx3_vk_end_commands(implementation, FALSE);
	if (result != FB_GFX3_OK)
		return result;
	runtime->completed_submission_count++;
	fb_gfx3_vk_runtime_update_submission_telemetry(runtime, implementation);
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_runtime_fill_u32(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t *values, size_t value_count, uint32_t value)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VK_BUFFER_CREATE_INFO buffer_create_info;
	FB_GFX3_VK_MEMORY_REQUIREMENTS memory_requirements;
	FB_GFX3_VK_MEMORY_ALLOCATE_INFO memory_allocate_info;
	FB_GFX3_VK_BUFFER_MEMORY_BARRIER barrier;
	FB_GFX3_VK_BUFFER buffer = 0;
	FB_GFX3_VK_DEVICE_MEMORY memory = 0;
	void *mapped = NULL;
	size_t byte_size;
	uint32_t memory_type_index;
	int result;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL) || (values == NULL) ||
	    (value_count == 0) ||
	    (fb_gfx3_vk_size_multiply(value_count, sizeof(values[0]),
	     &byte_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = (uint64_t)byte_size;
	buffer_create_info.usage = FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_create_info.sharing_mode = FB_GFX3_VK_SHARING_MODE_EXCLUSIVE;
	if (implementation->create_buffer(implementation->device,
	    &buffer_create_info, NULL, &buffer) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(&memory_requirements, 0, sizeof(memory_requirements));
	implementation->get_buffer_memory_requirements(implementation->device,
		buffer, &memory_requirements);
	result = fb_gfx3_vk_find_memory_type(implementation,
		memory_requirements.memory_type_bits,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&memory_type_index);
	if (result != FB_GFX3_OK)
		goto cleanup;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocation_size = memory_requirements.size;
	memory_allocate_info.memory_type_index = memory_type_index;
	if (implementation->allocate_memory(implementation->device,
	    &memory_allocate_info, NULL, &memory) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if (implementation->bind_buffer_memory(implementation->device, buffer,
	    memory, 0) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	if (implementation->map_memory(implementation->device, memory, 0,
	    (uint64_t)byte_size, 0, &mapped) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(mapped, 0, byte_size);

	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->command_fill_buffer(implementation->command_buffer,
		buffer, 0, (uint64_t)byte_size, value);
	memset(&barrier, 0, sizeof(barrier));
	barrier.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.source_access_mask = FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.destination_access_mask = FB_GFX3_VK_ACCESS_HOST_READ_BIT;
	barrier.source_queue_family_index = UINT32_MAX;
	barrier.destination_queue_family_index = UINT32_MAX;
	barrier.buffer = buffer;
	barrier.size = (uint64_t)byte_size;
	implementation->command_pipeline_barrier(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1, &barrier,
		0, NULL);
	result = fb_gfx3_vk_end_commands(implementation, TRUE);
	if (result != FB_GFX3_OK)
		goto cleanup;
	memcpy(values, mapped, byte_size);
	runtime->completed_submission_count++;

cleanup:
	if (mapped != NULL)
		implementation->unmap_memory(implementation->device, memory);
	if (buffer != 0)
		implementation->destroy_buffer(implementation->device, buffer,
			NULL);
	if (memory != 0)
		implementation->free_memory(implementation->device, memory, NULL);
	return result;
}

int fb_gfx3_vulkan_runtime_compute_add_u32(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t *values, size_t value_count, uint32_t addend)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VK_BUFFER_CREATE_INFO buffer_create_info;
	FB_GFX3_VK_MEMORY_REQUIREMENTS memory_requirements;
	FB_GFX3_VK_MEMORY_ALLOCATE_INFO memory_allocate_info;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO descriptor_buffer_info;
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_write;
	FB_GFX3_VK_BUFFER_MEMORY_BARRIER barrier;
	FB_GFX3_VK_BUFFER buffer = 0;
	FB_GFX3_VK_DEVICE_MEMORY memory = 0;
	uint32_t *mapped = NULL;
	size_t total_words;
	size_t byte_size;
	uint32_t memory_type_index;
	uint32_t group_count_x;
	int result;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL) || (values == NULL) ||
	    (value_count == 0) || (value_count > UINT32_MAX) ||
	    (fb_gfx3_vk_size_add(value_count, 2, &total_words) != FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(total_words, sizeof(values[0]),
	     &byte_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	memset(&buffer_create_info, 0, sizeof(buffer_create_info));
	buffer_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = (uint64_t)byte_size;
	buffer_create_info.usage = FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	buffer_create_info.sharing_mode = FB_GFX3_VK_SHARING_MODE_EXCLUSIVE;
	if (implementation->create_buffer(implementation->device,
	    &buffer_create_info, NULL, &buffer) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(&memory_requirements, 0, sizeof(memory_requirements));
	implementation->get_buffer_memory_requirements(implementation->device,
		buffer, &memory_requirements);
	result = fb_gfx3_vk_find_memory_type(implementation,
		memory_requirements.memory_type_bits,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&memory_type_index);
	if (result != FB_GFX3_OK)
		goto cleanup;
	memset(&memory_allocate_info, 0, sizeof(memory_allocate_info));
	memory_allocate_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocation_size = memory_requirements.size;
	memory_allocate_info.memory_type_index = memory_type_index;
	if (implementation->allocate_memory(implementation->device,
	    &memory_allocate_info, NULL, &memory) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if (implementation->bind_buffer_memory(implementation->device, buffer,
	    memory, 0) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	if (implementation->map_memory(implementation->device, memory, 0,
	    (uint64_t)byte_size, 0, (void **)&mapped) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	mapped[0] = (uint32_t)value_count;
	mapped[1] = addend;
	memcpy(&mapped[2], values, value_count * sizeof(values[0]));
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;

	memset(&descriptor_buffer_info, 0, sizeof(descriptor_buffer_info));
	descriptor_buffer_info.buffer = buffer;
	descriptor_buffer_info.range = (uint64_t)byte_size;
	memset(&descriptor_write, 0, sizeof(descriptor_write));
	descriptor_write.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.destination_set =
		implementation->compute_descriptor_set;
	descriptor_write.descriptor_count = 1;
	descriptor_write.descriptor_type =
		FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_write.buffer_info = &descriptor_buffer_info;
	implementation->update_descriptor_sets(implementation->device, 1,
		&descriptor_write, 0, NULL);
	memset(&barrier, 0, sizeof(barrier));
	barrier.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.source_access_mask = FB_GFX3_VK_ACCESS_HOST_WRITE_BIT;
	barrier.destination_access_mask = FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT;
	barrier.source_queue_family_index = UINT32_MAX;
	barrier.destination_queue_family_index = UINT32_MAX;
	barrier.buffer = buffer;
	barrier.size = (uint64_t)byte_size;
	implementation->command_pipeline_barrier(
		implementation->command_buffer, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 1,
		&barrier, 0, NULL);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&implementation->compute_descriptor_set, 0, NULL);
	group_count_x = ((uint32_t)value_count - 1u) / 64u + 1u;
	implementation->command_dispatch(implementation->command_buffer,
		group_count_x, 1, 1);
	barrier.source_access_mask = FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT;
	barrier.destination_access_mask = FB_GFX3_VK_ACCESS_HOST_READ_BIT;
	implementation->command_pipeline_barrier(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1, &barrier,
		0, NULL);
	result = fb_gfx3_vk_end_commands(implementation, TRUE);
	if (result != FB_GFX3_OK)
		goto cleanup;
	memcpy(values, &mapped[2], value_count * sizeof(values[0]));
	runtime->completed_submission_count++;

cleanup:
	if (mapped != NULL)
		implementation->unmap_memory(implementation->device, memory);
	if (buffer != 0)
		implementation->destroy_buffer(implementation->device, buffer,
			NULL);
	if (memory != 0)
		implementation->free_memory(implementation->device, memory, NULL);
	return result;
}
#endif

/* end of gfx3_vulkan_submit.c */
