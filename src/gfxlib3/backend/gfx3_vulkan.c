/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_vulkan.c

    Purpose:

        Implement the header-independent Vulkan runtime used by the gfxlib3
        render thread.

    Responsibilities:

        - load the platform Vulkan loader dynamically
        - resolve exported global commands directly, with vkGetInstanceProcAddr
          fallback for older Vulkan loaders
        - create a Vulkan 1.0 instance with only required presentation
          extensions when a native window is supplied
        - enumerate physical devices and compute-capable queue families
        - create one logical device and acquire one queue
        - create descriptor and compute-pipeline state
        - coordinate teardown with submission and presentation owners
        - destroy every acquired object in reverse ownership order

    This file intentionally does NOT contain:

        - native window creation or message dispatch
        - FreeBASIC compatibility or common renderer command dispatch
        - surface drawing, transfers, or swapchain implementation
*/

#include "../gfx3_vulkan.h"
#include "../gfx3_presentation.h"
#include "../gfx3_vulkan_platform.h"

#include <errno.h>

uint64_t fb_gfx3_vulkan_device_score(uint32_t device_type,
	int shader_float64, uint32_t queue_flags)
{
	uint64_t score = 0;

	/*
		Float64 keeps the exact midpoint ellipse compute path available.  It
		therefore remains a stronger preference than adapter class, matching
		the original two-pass selection policy.
	*/
	if (shader_float64)
		score |= UINT64_C(1) << 62;
	switch (device_type) {
	case FB_GFX3_VULKAN_DEVICE_TYPE_DISCRETE_GPU:
		score |= UINT64_C(5) << 56;
		break;
	case FB_GFX3_VULKAN_DEVICE_TYPE_INTEGRATED_GPU:
		score |= UINT64_C(4) << 56;
		break;
	case FB_GFX3_VULKAN_DEVICE_TYPE_VIRTUAL_GPU:
		score |= UINT64_C(3) << 56;
		break;
	case FB_GFX3_VULKAN_DEVICE_TYPE_CPU:
		score |= UINT64_C(2) << 56;
		break;
	default:
		score |= UINT64_C(1) << 56;
		break;
	}
	if ((queue_flags & FB_GFX3_VULKAN_QUEUE_GRAPHICS) != 0)
		score |= UINT64_C(1) << 48;
	if ((queue_flags & FB_GFX3_VULKAN_QUEUE_COMPUTE) != 0)
		score |= UINT64_C(1) << 47;
	return score;
}

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || \
	defined(HOST_DARWIN)

#include "gfx3_vulkan_private.h"

#include "../gfx3_vulkan_shader.h"


/* ------------------------------------------------------------------------- */
/* Checked host-size arithmetic                                              */
/* ------------------------------------------------------------------------- */

int fb_gfx3_vk_size_add(size_t left, size_t right, size_t *result)
{
	if ((result == NULL) || (left > (SIZE_MAX - right)))
		return FB_GFX3_INVALID;
	*result = left + right;
	return FB_GFX3_OK;
}

int fb_gfx3_vk_size_multiply(size_t left, size_t right, size_t *result)
{
	if ((result == NULL) || ((right != 0) && (left > (SIZE_MAX / right))))
		return FB_GFX3_INVALID;
	*result = left * right;
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Loader symbol conversion                                                  */
/* ------------------------------------------------------------------------- */

static FB_GFX3_VK_VOID_FUNCTION vulkan_load_global_entry(
	FB_GFX3_VULKAN_LIBRARY library, const char *name)
{
	FB_GFX3_VK_VOID_FUNCTION result = NULL;

	if ((library == NULL) || (name == NULL))
		return NULL;
	if (fb_gfx3_vulkan_platform_load_library_function(library, name,
	    (void *)&result, sizeof(result)) != FB_GFX3_OK)
		return NULL;
	return result;
}

static FB_GFX3_VK_GET_INSTANCE_PROC_ADDR vulkan_load_entry(
	FB_GFX3_VULKAN_LIBRARY library)
{
	FB_GFX3_VK_GET_INSTANCE_PROC_ADDR result = NULL;
	FB_GFX3_VK_VOID_FUNCTION symbol;

	symbol = vulkan_load_global_entry(library, "vkGetInstanceProcAddr");
	if ((symbol != NULL) && (sizeof(result) == sizeof(symbol)))
		memcpy((void *)&result, (const void *)&symbol, sizeof(result));
	return result;
}

#define FB_GFX3_VK_RESOLVE(result, implementation, instance, type, name) \
	do { \
		union { \
			FB_GFX3_VK_VOID_FUNCTION generic; \
			type typed; \
		} symbol; \
		symbol.generic = (implementation)->get_instance_proc_address( \
			(instance), #name); \
		(result) = symbol.typed; \
	} while (0)

#define FB_GFX3_VK_RESOLVE_NAMED(result, implementation, instance, type, name) \
	do { \
		union { \
			FB_GFX3_VK_VOID_FUNCTION generic; \
			type typed; \
		} symbol; \
		symbol.generic = (implementation)->get_instance_proc_address( \
			(instance), (name)); \
		(result) = symbol.typed; \
	} while (0)

/* ------------------------------------------------------------------------- */
/* Checked enumeration and selection                                         */
/* ------------------------------------------------------------------------- */

static int vulkan_enumerate_physical_devices(
	FB_GFX3_VK_ENUMERATE_PHYSICAL_DEVICES enumerate,
	FB_GFX3_VK_INSTANCE instance, FB_GFX3_VK_PHYSICAL_DEVICE **devices,
	uint32_t *device_count)
{
	FB_GFX3_VK_PHYSICAL_DEVICE *array = NULL;
	uint32_t count;
	size_t allocation_size;
	int attempt;
	FB_GFX3_VK_RESULT result;

	if ((enumerate == NULL) || (devices == NULL) || (device_count == NULL))
		return FB_GFX3_INVALID;
	for (attempt = 0; attempt < FB_GFX3_VK_ENUMERATION_ATTEMPTS; attempt++) {
		count = 0;
		result = enumerate(instance, &count, NULL);
		if ((result != FB_GFX3_VK_SUCCESS) || (count == 0))
			return FB_GFX3_UNSUPPORTED;
		if (fb_gfx3_vk_size_multiply(count, sizeof(array[0]),
		    &allocation_size) != FB_GFX3_OK)
			return FB_GFX3_INVALID;
		array = (FB_GFX3_VK_PHYSICAL_DEVICE *)malloc(allocation_size);
		if (array == NULL)
			return FB_GFX3_OUT_OF_MEMORY;
		result = enumerate(instance, &count, array);
		if (result == FB_GFX3_VK_SUCCESS) {
			*devices = array;
			*device_count = count;
			return FB_GFX3_OK;
		}
		free((void *)array);
		array = NULL;
		if (result != FB_GFX3_VK_INCOMPLETE)
			return FB_GFX3_FAILED;
	}
	return FB_GFX3_EXHAUSTED;
}

static int vulkan_select_queue_family(
	FB_GFX3_VK_GET_QUEUE_FAMILY_PROPERTIES get_properties,
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_SUPPORT get_surface_support,
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, FB_GFX3_VK_SURFACE surface,
	uint32_t *family_index, uint32_t *queue_flags)
{
	FB_GFX3_VK_QUEUE_FAMILY_PROPERTIES *properties;
	uint32_t count = 0;
	uint32_t written;
	uint32_t index;
	uint32_t best_index = UINT32_MAX;
	uint32_t best_score = 0;
	size_t allocation_size;

	if ((get_properties == NULL) || (family_index == NULL) ||
	    (queue_flags == NULL))
		return FB_GFX3_INVALID;
	get_properties(physical_device, &count, NULL);
	if ((count == 0) || (fb_gfx3_vk_size_multiply(count,
	    sizeof(properties[0]), &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_UNSUPPORTED;
	properties = (FB_GFX3_VK_QUEUE_FAMILY_PROPERTIES *)
		calloc(1, allocation_size);
	if (properties == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	written = count;
	get_properties(physical_device, &written, properties);
	if (written < count)
		count = written;
	for (index = 0; index < count; index++) {
		uint32_t score;
		uint32_t present_supported = FALSE;

		if ((properties[index].queue_count == 0) ||
		    ((properties[index].queue_flags &
		      FB_GFX3_VK_QUEUE_COMPUTE_BIT) == 0))
			continue;
		if (surface != 0) {
			if ((get_surface_support == NULL) ||
			    (get_surface_support(physical_device, index, surface,
			     &present_supported) != FB_GFX3_VK_SUCCESS) ||
			    !present_supported)
				continue;
		}
		score = (properties[index].queue_flags &
			FB_GFX3_VK_QUEUE_GRAPHICS_BIT) ? 2u : 1u;
		if (score > best_score) {
			best_score = score;
			best_index = index;
		}
	}
	if (best_index != UINT32_MAX) {
		*family_index = best_index;
		*queue_flags = properties[best_index].queue_flags;
	}
	free(properties);
	return (best_index == UINT32_MAX) ? FB_GFX3_UNSUPPORTED : FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Queue submission ownership                                                */
/* ------------------------------------------------------------------------- */

static int vulkan_create_submission_objects(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	uint32_t queue_family_index)
{
	FB_GFX3_VK_CREATE_COMMAND_POOL create_command_pool;
	FB_GFX3_VK_ALLOCATE_COMMAND_BUFFERS allocate_command_buffers;
	FB_GFX3_VK_CREATE_FENCE create_fence;
	FB_GFX3_VK_COMMAND_POOL_CREATE_INFO pool_create_info;
	FB_GFX3_VK_COMMAND_BUFFER_ALLOCATE_INFO buffer_allocate_info;
	FB_GFX3_VK_FENCE_CREATE_INFO fence_create_info;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	uint32_t index;

	if ((implementation == NULL) ||
	    (implementation->get_device_proc_address == NULL))
		return FB_GFX3_INVALID;
	FB_GFX3_VK_RESOLVE_DEVICE(create_command_pool, implementation,
		FB_GFX3_VK_CREATE_COMMAND_POOL, vkCreateCommandPool);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_command_pool,
		implementation, FB_GFX3_VK_DESTROY_COMMAND_POOL,
		vkDestroyCommandPool);
	FB_GFX3_VK_RESOLVE_DEVICE(allocate_command_buffers, implementation,
		FB_GFX3_VK_ALLOCATE_COMMAND_BUFFERS, vkAllocateCommandBuffers);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->begin_command_buffer,
		implementation, FB_GFX3_VK_BEGIN_COMMAND_BUFFER,
		vkBeginCommandBuffer);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->end_command_buffer,
		implementation, FB_GFX3_VK_END_COMMAND_BUFFER,
		vkEndCommandBuffer);
	FB_GFX3_VK_RESOLVE_DEVICE(create_fence, implementation,
		FB_GFX3_VK_CREATE_FENCE, vkCreateFence);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_fence,
		implementation, FB_GFX3_VK_DESTROY_FENCE, vkDestroyFence);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->wait_for_fences,
		implementation, FB_GFX3_VK_WAIT_FOR_FENCES, vkWaitForFences);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->get_fence_status,
		implementation, FB_GFX3_VK_GET_FENCE_STATUS, vkGetFenceStatus);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->reset_fences,
		implementation, FB_GFX3_VK_RESET_FENCES, vkResetFences);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->reset_command_pool,
		implementation, FB_GFX3_VK_RESET_COMMAND_POOL,
		vkResetCommandPool);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->queue_submit,
		implementation, FB_GFX3_VK_QUEUE_SUBMIT, vkQueueSubmit);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->device_wait_idle,
		implementation, FB_GFX3_VK_DEVICE_WAIT_IDLE, vkDeviceWaitIdle);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->create_buffer,
		implementation, FB_GFX3_VK_CREATE_BUFFER, vkCreateBuffer);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_buffer,
		implementation, FB_GFX3_VK_DESTROY_BUFFER, vkDestroyBuffer);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->get_buffer_memory_requirements,
		implementation, FB_GFX3_VK_GET_BUFFER_MEMORY_REQUIREMENTS,
		vkGetBufferMemoryRequirements);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->allocate_memory,
		implementation, FB_GFX3_VK_ALLOCATE_MEMORY, vkAllocateMemory);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->free_memory,
		implementation, FB_GFX3_VK_FREE_MEMORY, vkFreeMemory);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->bind_buffer_memory,
		implementation, FB_GFX3_VK_BIND_BUFFER_MEMORY,
		vkBindBufferMemory);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->map_memory,
		implementation, FB_GFX3_VK_MAP_MEMORY, vkMapMemory);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->unmap_memory,
		implementation, FB_GFX3_VK_UNMAP_MEMORY, vkUnmapMemory);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_fill_buffer,
		implementation, FB_GFX3_VK_CMD_FILL_BUFFER, vkCmdFillBuffer);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_copy_buffer,
		implementation, FB_GFX3_VK_CMD_COPY_BUFFER, vkCmdCopyBuffer);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_copy_buffer_to_image,
		implementation, FB_GFX3_VK_CMD_COPY_BUFFER_TO_IMAGE,
		vkCmdCopyBufferToImage);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_pipeline_barrier,
		implementation, FB_GFX3_VK_CMD_PIPELINE_BARRIER,
		vkCmdPipelineBarrier);
	if ((create_command_pool == NULL) ||
	    (implementation->destroy_command_pool == NULL) ||
	    (allocate_command_buffers == NULL) ||
	    (implementation->begin_command_buffer == NULL) ||
	    (implementation->end_command_buffer == NULL) ||
	    (create_fence == NULL) || (implementation->destroy_fence == NULL) ||
	    (implementation->wait_for_fences == NULL) ||
	    (implementation->get_fence_status == NULL) ||
	    (implementation->reset_fences == NULL) ||
	    (implementation->reset_command_pool == NULL) ||
	    (implementation->queue_submit == NULL) ||
	    (implementation->device_wait_idle == NULL) ||
	    (implementation->create_buffer == NULL) ||
	    (implementation->destroy_buffer == NULL) ||
	    (implementation->get_buffer_memory_requirements == NULL) ||
	    (implementation->allocate_memory == NULL) ||
	    (implementation->free_memory == NULL) ||
	    (implementation->bind_buffer_memory == NULL) ||
	    (implementation->map_memory == NULL) ||
	    (implementation->unmap_memory == NULL) ||
	    (implementation->command_fill_buffer == NULL) ||
	    (implementation->command_copy_buffer == NULL) ||
	    ((implementation->surface != 0) &&
	     (implementation->command_copy_buffer_to_image == NULL)) ||
	    (implementation->command_pipeline_barrier == NULL))
		return FB_GFX3_UNSUPPORTED;

	memset(&pool_create_info, 0, sizeof(pool_create_info));
	pool_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queue_family_index = queue_family_index;
	memset(&fence_create_info, 0, sizeof(fence_create_info));
	fence_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = FB_GFX3_VK_FENCE_CREATE_SIGNALED_BIT;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		slot = &implementation->submission_slots[index];
		if (create_command_pool(implementation->device, &pool_create_info,
		    NULL, &slot->command_pool) != FB_GFX3_VK_SUCCESS)
			return FB_GFX3_FAILED;
		memset(&buffer_allocate_info, 0, sizeof(buffer_allocate_info));
		buffer_allocate_info.structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buffer_allocate_info.command_pool = slot->command_pool;
		buffer_allocate_info.level = FB_GFX3_VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buffer_allocate_info.command_buffer_count = 1;
		if (allocate_command_buffers(implementation->device,
		    &buffer_allocate_info, &slot->command_buffer) !=
		    FB_GFX3_VK_SUCCESS)
			return FB_GFX3_FAILED;
		if (create_fence(implementation->device, &fence_create_info, NULL,
		    &slot->fence) != FB_GFX3_VK_SUCCESS)
			return FB_GFX3_FAILED;
	}
	implementation->active_submission_slot = UINT32_MAX;
	implementation->next_submission_slot = 0;
	implementation->next_poll_submission_slot = 0;
	implementation->latest_submission_slot = UINT32_MAX;
	implementation->batch_command_owner_slot = UINT32_MAX;
	implementation->batch_operation_limit =
		FB_GFX3_VK_SUBMISSION_BATCH_LIMIT;
	/*
		Each operation retains its own slot-backed descriptors and mapped
		parameters, so up to eight ordered operations can safely share the first
		slot's command buffer and queue submission. Presentation and explicit
		completion flush the current batch.
	*/
	/*
		FBGFX3_VULKAN_DISABLE_BATCH is a diagnostic switch used for paired
		profiling. Production runs batch unless the variable is set to a
		non-empty value other than "0". FBGFX3_VULKAN_BATCH_SIZE accepts values
		from 2 through 8 so the synchronization policy can be compared without
		rebuilding.
	*/
	{
		const char *disable_batch =
			getenv("FBGFX3_VULKAN_DISABLE_BATCH");
		const char *batch_size_text =
			getenv("FBGFX3_VULKAN_BATCH_SIZE");
		char *batch_size_end = NULL;
		unsigned long batch_size = 0u;

		implementation->batch_enabled =
			(disable_batch == NULL) || (disable_batch[0] == '\0') ||
			(strcmp(disable_batch, "0") == 0);
		if ((batch_size_text != NULL) && (batch_size_text[0] != '\0')) {
			errno = 0;
			batch_size = strtoul(batch_size_text, &batch_size_end, 10);
			if ((errno == 0) && (batch_size_end != batch_size_text) &&
			    (*batch_size_end == '\0') && (batch_size >= 2u) &&
			    (batch_size <= FB_GFX3_VK_SUBMISSION_BATCH_LIMIT))
				implementation->batch_operation_limit =
					(uint32_t)batch_size;
		}
	}
	/* Compatibility aliases are selected again before the first recording. */
	implementation->command_pool = implementation->submission_slots[0].command_pool;
	implementation->command_buffer =
		implementation->submission_slots[0].command_buffer;
	implementation->fence = implementation->submission_slots[0].fence;
	return FB_GFX3_OK;
}

static int vulkan_create_compute_objects(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	FB_GFX3_VK_CREATE_SHADER_MODULE create_shader_module;
	FB_GFX3_VK_DESTROY_SHADER_MODULE destroy_shader_module;
	FB_GFX3_VK_CREATE_DESCRIPTOR_SET_LAYOUT create_descriptor_set_layout;
	FB_GFX3_VK_CREATE_PIPELINE_LAYOUT create_pipeline_layout;
	FB_GFX3_VK_CREATE_COMPUTE_PIPELINES create_compute_pipelines;
	FB_GFX3_VK_CREATE_DESCRIPTOR_POOL create_descriptor_pool;
	FB_GFX3_VK_ALLOCATE_DESCRIPTOR_SETS allocate_descriptor_sets;
	FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_BINDING bindings[5];
	FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO set_layout_create_info;
	FB_GFX3_VK_PIPELINE_LAYOUT_CREATE_INFO pipeline_layout_create_info;
	FB_GFX3_VK_SHADER_MODULE_CREATE_INFO shader_module_create_info;
	FB_GFX3_VK_COMPUTE_PIPELINE_CREATE_INFO pipeline_create_info;
	FB_GFX3_VK_DESCRIPTOR_POOL_SIZE pool_size;
	FB_GFX3_VK_DESCRIPTOR_POOL_CREATE_INFO pool_create_info;
	FB_GFX3_VK_DESCRIPTOR_SET_ALLOCATE_INFO set_allocate_info;
	FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT
		set_layouts[FB_GFX3_VK_SUBMISSION_SLOT_COUNT *
			FB_GFX3_VK_BLIT_BATCH_LIMIT];
	FB_GFX3_VK_DESCRIPTOR_SET
		descriptor_sets[FB_GFX3_VK_SUBMISSION_SLOT_COUNT *
			FB_GFX3_VK_BLIT_BATCH_LIMIT];
	FB_GFX3_VK_SHADER_MODULE shader_module = 0;
	uint32_t index;
	int result = FB_GFX3_UNSUPPORTED;

	FB_GFX3_VK_RESOLVE_DEVICE(create_shader_module, implementation,
		FB_GFX3_VK_CREATE_SHADER_MODULE, vkCreateShaderModule);
	FB_GFX3_VK_RESOLVE_DEVICE(destroy_shader_module, implementation,
		FB_GFX3_VK_DESTROY_SHADER_MODULE, vkDestroyShaderModule);
	FB_GFX3_VK_RESOLVE_DEVICE(create_descriptor_set_layout, implementation,
		FB_GFX3_VK_CREATE_DESCRIPTOR_SET_LAYOUT,
		vkCreateDescriptorSetLayout);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_descriptor_set_layout,
		implementation, FB_GFX3_VK_DESTROY_DESCRIPTOR_SET_LAYOUT,
		vkDestroyDescriptorSetLayout);
	FB_GFX3_VK_RESOLVE_DEVICE(create_pipeline_layout, implementation,
		FB_GFX3_VK_CREATE_PIPELINE_LAYOUT, vkCreatePipelineLayout);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_pipeline_layout,
		implementation, FB_GFX3_VK_DESTROY_PIPELINE_LAYOUT,
		vkDestroyPipelineLayout);
	FB_GFX3_VK_RESOLVE_DEVICE(create_compute_pipelines, implementation,
		FB_GFX3_VK_CREATE_COMPUTE_PIPELINES, vkCreateComputePipelines);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_pipeline,
		implementation, FB_GFX3_VK_DESTROY_PIPELINE, vkDestroyPipeline);
	FB_GFX3_VK_RESOLVE_DEVICE(create_descriptor_pool, implementation,
		FB_GFX3_VK_CREATE_DESCRIPTOR_POOL, vkCreateDescriptorPool);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_descriptor_pool,
		implementation, FB_GFX3_VK_DESTROY_DESCRIPTOR_POOL,
		vkDestroyDescriptorPool);
	FB_GFX3_VK_RESOLVE_DEVICE(allocate_descriptor_sets, implementation,
		FB_GFX3_VK_ALLOCATE_DESCRIPTOR_SETS, vkAllocateDescriptorSets);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->update_descriptor_sets,
		implementation, FB_GFX3_VK_UPDATE_DESCRIPTOR_SETS,
		vkUpdateDescriptorSets);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_bind_pipeline,
		implementation, FB_GFX3_VK_CMD_BIND_PIPELINE,
		vkCmdBindPipeline);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_bind_descriptor_sets,
		implementation, FB_GFX3_VK_CMD_BIND_DESCRIPTOR_SETS,
		vkCmdBindDescriptorSets);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->command_dispatch,
		implementation, FB_GFX3_VK_CMD_DISPATCH, vkCmdDispatch);
	if ((create_shader_module == NULL) || (destroy_shader_module == NULL) ||
	    (create_descriptor_set_layout == NULL) ||
	    (implementation->destroy_descriptor_set_layout == NULL) ||
	    (create_pipeline_layout == NULL) ||
	    (implementation->destroy_pipeline_layout == NULL) ||
	    (create_compute_pipelines == NULL) ||
	    (implementation->destroy_pipeline == NULL) ||
	    (create_descriptor_pool == NULL) ||
	    (implementation->destroy_descriptor_pool == NULL) ||
	    (allocate_descriptor_sets == NULL) ||
	    (implementation->update_descriptor_sets == NULL) ||
	    (implementation->command_bind_pipeline == NULL) ||
	    (implementation->command_bind_descriptor_sets == NULL) ||
	    (implementation->command_dispatch == NULL))
		return FB_GFX3_UNSUPPORTED;

	memset(bindings, 0, sizeof(bindings));
	bindings[0].descriptor_type = FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptor_count = 1;
	bindings[0].stage_flags = FB_GFX3_VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[1] = bindings[0];
	bindings[1].binding = 1;
	bindings[2] = bindings[0];
	bindings[2].binding = 2;
	bindings[3] = bindings[0];
	bindings[3].binding = 3;
	bindings[4] = bindings[0];
	bindings[4].binding = 4;
	memset(&set_layout_create_info, 0, sizeof(set_layout_create_info));
	set_layout_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_create_info.binding_count = 5;
	set_layout_create_info.bindings = bindings;
	if (create_descriptor_set_layout(implementation->device,
	    &set_layout_create_info, NULL,
	    &implementation->compute_descriptor_set_layout) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(&pipeline_layout_create_info, 0,
		sizeof(pipeline_layout_create_info));
	pipeline_layout_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.set_layout_count = 1;
	pipeline_layout_create_info.set_layouts =
		&implementation->compute_descriptor_set_layout;
	if (create_pipeline_layout(implementation->device,
	    &pipeline_layout_create_info, NULL,
	    &implementation->compute_pipeline_layout) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(&shader_module_create_info, 0,
		sizeof(shader_module_create_info));
	shader_module_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_compute_smoke_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_compute_smoke_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(&pipeline_create_info, 0, sizeof(pipeline_create_info));
	pipeline_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_create_info.stage.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipeline_create_info.stage.stage = FB_GFX3_VK_SHADER_STAGE_COMPUTE_BIT;
	pipeline_create_info.stage.module = shader_module;
	pipeline_create_info.stage.name = "main";
	pipeline_create_info.layout = implementation->compute_pipeline_layout;
	pipeline_create_info.base_pipeline_index = -1;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->compute_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_points_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_points_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->points_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_line_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_line_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->line_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_rectangle_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_rectangle_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
		&pipeline_create_info, NULL,
		&implementation->rectangle_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_rectangle_tile_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_rectangle_tile_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->rectangle_tile_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_primitive_tile_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_primitive_tile_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->primitive_tile_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
		&pipeline_create_info, NULL, &implementation->blit_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_transform_blit_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_transform_blit_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->transform_blit_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_winner_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_winner_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->blit_winner_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_resolve_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_resolve_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->blit_resolve_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_tile_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_tile_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->blit_tile_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_tile_nvidia_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_tile_nvidia_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->blit_tile_nvidia_pipeline) != FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_tile_trans8_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_tile_trans8_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->blit_tile_trans_pipeline[0]) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_tile_trans16_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_tile_trans16_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->blit_tile_trans_pipeline[1]) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_blit_tile_trans32_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_blit_tile_trans32_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL,
	    &implementation->blit_tile_trans_pipeline[2]) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_glyph_tile_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_glyph_tile_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->glyph_tile_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_paint_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_paint_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->paint_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	shader_module_create_info.code_size =
		sizeof(fb_gfx3_vulkan_present_spirv);
	shader_module_create_info.code = fb_gfx3_vulkan_present_spirv;
	if (create_shader_module(implementation->device,
	    &shader_module_create_info, NULL, &shader_module) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	pipeline_create_info.stage.module = shader_module;
	if (create_compute_pipelines(implementation->device, 0, 1,
	    &pipeline_create_info, NULL, &implementation->present_pipeline) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	destroy_shader_module(implementation->device, shader_module, NULL);
	shader_module = 0;
	if (implementation->shader_float64) {
		shader_module_create_info.code_size =
			sizeof(fb_gfx3_vulkan_ellipse_spirv);
		shader_module_create_info.code = fb_gfx3_vulkan_ellipse_spirv;
		if (create_shader_module(implementation->device,
		    &shader_module_create_info, NULL, &shader_module) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		pipeline_create_info.stage.module = shader_module;
		if (create_compute_pipelines(implementation->device, 0, 1,
		    &pipeline_create_info, NULL,
		    &implementation->ellipse_pipeline) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		destroy_shader_module(implementation->device, shader_module, NULL);
		shader_module = 0;
		shader_module_create_info.code_size =
			sizeof(fb_gfx3_vulkan_ellipse_winner_spirv);
		shader_module_create_info.code = fb_gfx3_vulkan_ellipse_winner_spirv;
		if (create_shader_module(implementation->device,
		    &shader_module_create_info, NULL, &shader_module) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		pipeline_create_info.stage.module = shader_module;
		if (create_compute_pipelines(implementation->device, 0, 1,
		    &pipeline_create_info, NULL,
		    &implementation->ellipse_winner_pipeline) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		destroy_shader_module(implementation->device, shader_module, NULL);
		shader_module = 0;
		shader_module_create_info.code_size =
			sizeof(fb_gfx3_vulkan_ellipse_resolve_spirv);
		shader_module_create_info.code = fb_gfx3_vulkan_ellipse_resolve_spirv;
		if (create_shader_module(implementation->device,
		    &shader_module_create_info, NULL, &shader_module) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		pipeline_create_info.stage.module = shader_module;
		if (create_compute_pipelines(implementation->device, 0, 1,
		    &pipeline_create_info, NULL,
		    &implementation->ellipse_resolve_pipeline) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		destroy_shader_module(implementation->device, shader_module, NULL);
		shader_module = 0;
		shader_module_create_info.code_size =
			sizeof(fb_gfx3_vulkan_primitive_winner_spirv);
		shader_module_create_info.code =
			fb_gfx3_vulkan_primitive_winner_spirv;
		if (create_shader_module(implementation->device,
		    &shader_module_create_info, NULL, &shader_module) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		pipeline_create_info.stage.module = shader_module;
		if (create_compute_pipelines(implementation->device, 0, 1,
		    &pipeline_create_info, NULL,
		    &implementation->primitive_winner_pipeline) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		destroy_shader_module(implementation->device, shader_module, NULL);
		shader_module = 0;
		shader_module_create_info.code_size =
			sizeof(fb_gfx3_vulkan_primitive_resolve_spirv);
		shader_module_create_info.code =
			fb_gfx3_vulkan_primitive_resolve_spirv;
		if (create_shader_module(implementation->device,
		    &shader_module_create_info, NULL, &shader_module) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		pipeline_create_info.stage.module = shader_module;
		if (create_compute_pipelines(implementation->device, 0, 1,
		    &pipeline_create_info, NULL,
		    &implementation->primitive_resolve_pipeline) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto cleanup;
		}
		destroy_shader_module(implementation->device, shader_module, NULL);
		shader_module = 0;
	}

	memset(&pool_size, 0, sizeof(pool_size));
	pool_size.type = FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptor_count = 5 * FB_GFX3_VK_SUBMISSION_SLOT_COUNT *
		FB_GFX3_VK_BLIT_BATCH_LIMIT;
	memset(&pool_create_info, 0, sizeof(pool_create_info));
	pool_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.maximum_sets = FB_GFX3_VK_SUBMISSION_SLOT_COUNT *
		FB_GFX3_VK_BLIT_BATCH_LIMIT;
	pool_create_info.pool_size_count = 1;
	pool_create_info.pool_sizes = &pool_size;
	if (create_descriptor_pool(implementation->device, &pool_create_info,
	    NULL, &implementation->compute_descriptor_pool) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	memset(&set_allocate_info, 0, sizeof(set_allocate_info));
	set_allocate_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_allocate_info.descriptor_pool =
		implementation->compute_descriptor_pool;
	set_allocate_info.descriptor_set_count = FB_GFX3_VK_SUBMISSION_SLOT_COUNT *
		FB_GFX3_VK_BLIT_BATCH_LIMIT;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT *
		FB_GFX3_VK_BLIT_BATCH_LIMIT; index++)
		set_layouts[index] = implementation->compute_descriptor_set_layout;
	set_allocate_info.set_layouts = set_layouts;
	if (allocate_descriptor_sets(implementation->device,
	    &set_allocate_info, descriptor_sets) !=
	    FB_GFX3_VK_SUCCESS) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		uint32_t descriptor_index;

		for (descriptor_index = 0;
		     descriptor_index < FB_GFX3_VK_BLIT_BATCH_LIMIT;
		     descriptor_index++)
			implementation->submission_slots[index].descriptor_sets[
				descriptor_index] = descriptor_sets[(index *
				FB_GFX3_VK_BLIT_BATCH_LIMIT) + descriptor_index];
	}
	implementation->compute_descriptor_set = descriptor_sets[0];
	return FB_GFX3_OK;

cleanup:
	if (shader_module != 0)
		destroy_shader_module(implementation->device, shader_module, NULL);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Runtime lifecycle                                                         */
/* ------------------------------------------------------------------------- */

static void vulkan_implementation_close(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	uint32_t index;

	if (implementation == NULL)
		return;
	/*
		Close may receive a partially initialized runtime, so flushing is best
		effort. Submit any valid recorded batch before asking Vulkan to make the
		device idle.
	*/
	(void)fb_gfx3_vk_batch_flush_commands(implementation);
	if ((implementation->device != NULL) &&
	    (implementation->device_wait_idle != NULL))
		implementation->device_wait_idle(implementation->device);
	(void)fb_gfx3_vk_submission_wait_all(implementation);
	fb_gfx3_vk_swapchain_destroy(implementation);
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		FB_GFX3_VULKAN_SUBMISSION_SLOT *slot =
			&implementation->submission_slots[index];

		fb_gfx3_vk_submission_slot_release_deferred(implementation, slot);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_command_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_winner_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_index_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_range_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_command_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_winner_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_workgroup_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->point_command_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_command_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_range_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_index_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_tile_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->transform_command_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->glyph_index_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->glyph_range_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->glyph_command_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->paint_scratch_buffer);
		fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
			&slot->paint_command_buffer);
		if ((slot->image_available != 0) &&
		    (implementation->destroy_semaphore != NULL))
			implementation->destroy_semaphore(implementation->device,
				slot->image_available, NULL);
		if ((slot->fence != 0) && (implementation->destroy_fence != NULL))
			implementation->destroy_fence(implementation->device,
				slot->fence, NULL);
		if ((slot->command_pool != 0) &&
		    (implementation->destroy_command_pool != NULL))
			implementation->destroy_command_pool(implementation->device,
				slot->command_pool, NULL);
	}
	fb_gfx3_vk_buffer_allocation_destroy_immediate(implementation,
		&implementation->download_buffer);
	free(implementation->blit_prepare_scratch);
	implementation->blit_prepare_scratch = NULL;
	implementation->blit_prepare_scratch_size = 0u;
	free(implementation->blit_tile_count_scratch);
	implementation->blit_tile_count_scratch = NULL;
	implementation->blit_tile_count_scratch_size = 0u;
	free(implementation->blit_tile_output_scratch);
	implementation->blit_tile_output_scratch = NULL;
	implementation->blit_tile_output_scratch_size = 0u;
	free(implementation->rectangle_tile_count_scratch);
	implementation->rectangle_tile_count_scratch = NULL;
	implementation->rectangle_tile_count_scratch_size = 0u;
	free(implementation->rectangle_tile_output_scratch);
	implementation->rectangle_tile_output_scratch = NULL;
	implementation->rectangle_tile_output_scratch_size = 0u;
	if ((implementation->compute_descriptor_pool != 0) &&
	    (implementation->destroy_descriptor_pool != NULL))
		implementation->destroy_descriptor_pool(implementation->device,
			implementation->compute_descriptor_pool, NULL);
	if ((implementation->ellipse_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->ellipse_pipeline, NULL);
	if ((implementation->ellipse_winner_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->ellipse_winner_pipeline, NULL);
	if ((implementation->ellipse_resolve_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->ellipse_resolve_pipeline, NULL);
	if ((implementation->primitive_winner_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->primitive_winner_pipeline, NULL);
	if ((implementation->primitive_resolve_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->primitive_resolve_pipeline, NULL);
	if ((implementation->paint_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->paint_pipeline, NULL);
	if ((implementation->present_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->present_pipeline, NULL);
	if ((implementation->blit_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->blit_pipeline, NULL);
	if ((implementation->transform_blit_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->transform_blit_pipeline, NULL);
	if ((implementation->blit_winner_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->blit_winner_pipeline, NULL);
	if ((implementation->blit_resolve_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->blit_resolve_pipeline, NULL);
	if ((implementation->blit_tile_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->blit_tile_pipeline, NULL);
	if ((implementation->blit_tile_nvidia_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->blit_tile_nvidia_pipeline, NULL);
	for (index = 0u; index < 3u; index++) {
		if ((implementation->blit_tile_trans_pipeline[index] != 0) &&
		    (implementation->destroy_pipeline != NULL))
			implementation->destroy_pipeline(implementation->device,
				implementation->blit_tile_trans_pipeline[index], NULL);
	}
	if ((implementation->glyph_tile_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->glyph_tile_pipeline, NULL);
	if ((implementation->rectangle_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->rectangle_pipeline, NULL);
	if ((implementation->rectangle_tile_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->rectangle_tile_pipeline, NULL);
	if ((implementation->primitive_tile_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->primitive_tile_pipeline, NULL);
	if ((implementation->line_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->line_pipeline, NULL);
	if ((implementation->points_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->points_pipeline, NULL);
	if ((implementation->compute_pipeline != 0) &&
	    (implementation->destroy_pipeline != NULL))
		implementation->destroy_pipeline(implementation->device,
			implementation->compute_pipeline, NULL);
	if ((implementation->compute_pipeline_layout != 0) &&
	    (implementation->destroy_pipeline_layout != NULL))
		implementation->destroy_pipeline_layout(implementation->device,
			implementation->compute_pipeline_layout, NULL);
	if ((implementation->compute_descriptor_set_layout != 0) &&
	    (implementation->destroy_descriptor_set_layout != NULL))
		implementation->destroy_descriptor_set_layout(
			implementation->device,
			implementation->compute_descriptor_set_layout, NULL);
	if ((implementation->device != NULL) &&
	    (implementation->destroy_device != NULL))
		implementation->destroy_device(implementation->device, NULL);
	if ((implementation->surface != 0) &&
	    (implementation->destroy_surface != NULL))
		implementation->destroy_surface(implementation->instance,
			implementation->surface, NULL);
	if ((implementation->instance != NULL) &&
	    (implementation->destroy_instance != NULL))
		implementation->destroy_instance(implementation->instance, NULL);
	if (implementation->library != NULL)
		fb_gfx3_vulkan_platform_library_close(implementation->library);
	free(implementation);
}

static int vulkan_runtime_open_internal(FB_GFX3_VULKAN_RUNTIME *runtime,
	uintptr_t native_instance, uintptr_t native_window, uint32_t width,
	uint32_t height)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VK_ENUMERATE_INSTANCE_VERSION enumerate_instance_version;
	FB_GFX3_VK_CREATE_INSTANCE create_instance;
	FB_GFX3_VK_CREATE_PLATFORM_SURFACE create_platform_surface = NULL;
	FB_GFX3_VK_ENUMERATE_PHYSICAL_DEVICES enumerate_physical_devices;
	FB_GFX3_VK_GET_QUEUE_FAMILY_PROPERTIES get_queue_family_properties;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_FEATURES get_physical_device_features;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_PROPERTIES get_physical_device_properties;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_SUPPORT get_surface_support = NULL;
	FB_GFX3_VK_CREATE_DEVICE create_device;
	FB_GFX3_VK_GET_DEVICE_QUEUE get_device_queue;
	FB_GFX3_VK_PHYSICAL_DEVICE *physical_devices = NULL;
	FB_GFX3_VK_APPLICATION_INFO application_info;
	FB_GFX3_VK_INSTANCE_CREATE_INFO instance_create_info;
	FB_GFX3_VULKAN_SURFACE_CREATE_INFO surface_create_info;
	FB_GFX3_VK_DEVICE_QUEUE_CREATE_INFO queue_create_info;
	FB_GFX3_VK_DEVICE_CREATE_INFO device_create_info;
	FB_GFX3_VK_PHYSICAL_DEVICE_FEATURES supported_features;
	FB_GFX3_VK_PHYSICAL_DEVICE_FEATURES enabled_features;
	uint8_t *device_attempted = NULL;
	uint32_t physical_device_count = 0;
	uint32_t selected_physical_device_index = UINT32_MAX;
	uint32_t selected_vendor_id = 0;
	uint32_t selected_device_id = 0;
	uint32_t selected_device_type = FB_GFX3_VULKAN_DEVICE_TYPE_OTHER;
	char selected_device_name[256] = { 0 };
	uint32_t selected_maximum_storage_buffer_range =
		FB_GFX3_VK_MINIMUM_STORAGE_BUFFER_RANGE;
	uint32_t queue_family_index = UINT32_MAX;
	uint32_t queue_flags = 0;
	uint32_t device_index;
	int feature_pass;
	const char *requested_device_text;
	char *requested_device_end;
	unsigned long requested_device_value;
	int device_is_forced = FALSE;
	int windowed = (native_instance != 0) || (native_window != 0);
	float queue_priority = 1.0f;
	const char *instance_extensions[3];
	const char *device_extensions[2];
	const char *portability_instance_extension;
	const char *portability_device_extension;
	uint32_t instance_extension_count = 0u;
	uint32_t device_extension_count = 0u;
	int result;

	if ((runtime == NULL) || (runtime->implementation != NULL) ||
	    runtime->initialized ||
	    (windowed && !fb_gfx3_vulkan_platform_window_valid(native_instance,
	     native_window, width, height)))
		return FB_GFX3_INVALID;
	memset(runtime, 0, sizeof(*runtime));
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		calloc(1, sizeof(*implementation));
	if (implementation == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	implementation->desired_width = width;
	implementation->desired_height = height;
	implementation->library = fb_gfx3_vulkan_platform_library_open();
	if (implementation->library == NULL) {
		vulkan_implementation_close(implementation);
		return FB_GFX3_UNSUPPORTED;
	}
	implementation->get_instance_proc_address =
		vulkan_load_entry(implementation->library);
	if (implementation->get_instance_proc_address == NULL) {
		vulkan_implementation_close(implementation);
		return FB_GFX3_UNSUPPORTED;
	}

	runtime->loader_api_version = FB_GFX3_VK_API_VERSION_1_0;
	/*
		Android's no-HAL stub reports a diagnostic when this optional 1.1
		query is requested through vkGetInstanceProcAddr with VK_NULL_HANDLE.
		The loader export is the preferred Vulkan 1.1 entry point. Android 1.0
		loaders and no-HAL stubs may lack that export, so gfxlib3 retains its
		Vulkan 1.0 baseline there instead of issuing the diagnostic query.
		Desktop loaders keep the resolver fallback for old export layouts.
	*/
	enumerate_instance_version = NULL;
	{
		FB_GFX3_VK_VOID_FUNCTION symbol = vulkan_load_global_entry(
			implementation->library, "vkEnumerateInstanceVersion");

		if ((symbol != NULL) &&
		    (sizeof(enumerate_instance_version) == sizeof(symbol)))
			memcpy((void *)&enumerate_instance_version,
				(const void *)&symbol,
				sizeof(enumerate_instance_version));
	}
	if ((enumerate_instance_version == NULL) &&
	    fb_gfx3_vulkan_platform_resolve_instance_version()) {
		FB_GFX3_VK_RESOLVE(enumerate_instance_version, implementation, NULL,
			FB_GFX3_VK_ENUMERATE_INSTANCE_VERSION,
			vkEnumerateInstanceVersion);
	}
	if ((enumerate_instance_version != NULL) &&
	    (enumerate_instance_version(&runtime->loader_api_version) !=
	     FB_GFX3_VK_SUCCESS)) {
		vulkan_implementation_close(implementation);
		memset(runtime, 0, sizeof(*runtime));
		return FB_GFX3_FAILED;
	}
	FB_GFX3_VK_RESOLVE(create_instance, implementation, NULL,
		FB_GFX3_VK_CREATE_INSTANCE, vkCreateInstance);
	if (create_instance == NULL) {
		vulkan_implementation_close(implementation);
		memset(runtime, 0, sizeof(*runtime));
		return FB_GFX3_UNSUPPORTED;
	}
	memset(&application_info, 0, sizeof(application_info));
	application_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.application_name = "FreeBASIC gfxlib3";
	application_info.application_version = 1;
	application_info.engine_name = "FreeBASIC gfxlib3";
	application_info.engine_version = 1;
	application_info.api_version = FB_GFX3_VK_API_VERSION_1_0;
	memset(&instance_create_info, 0, sizeof(instance_create_info));
	instance_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.application_info = &application_info;
	instance_create_info.flags =
		fb_gfx3_vulkan_platform_instance_create_flags();
	if (windowed) {
		instance_extensions[instance_extension_count++] = "VK_KHR_surface";
		instance_extensions[instance_extension_count++] =
			fb_gfx3_vulkan_platform_instance_extension();
		device_extensions[device_extension_count++] = "VK_KHR_swapchain";
	}
	portability_instance_extension =
		fb_gfx3_vulkan_platform_portability_instance_extension();
	if (portability_instance_extension != NULL)
		instance_extensions[instance_extension_count++] =
			portability_instance_extension;
	portability_device_extension =
		fb_gfx3_vulkan_platform_portability_device_extension();
	if (portability_device_extension != NULL)
		device_extensions[device_extension_count++] =
			portability_device_extension;
	instance_create_info.enabled_extension_count = instance_extension_count;
	instance_create_info.enabled_extension_names =
		(instance_extension_count != 0u) ? instance_extensions : NULL;
	if (create_instance(&instance_create_info, NULL,
	    &implementation->instance) != FB_GFX3_VK_SUCCESS) {
		vulkan_implementation_close(implementation);
		memset(runtime, 0, sizeof(*runtime));
		return FB_GFX3_FAILED;
	}

	FB_GFX3_VK_RESOLVE(implementation->destroy_instance, implementation,
		implementation->instance, FB_GFX3_VK_DESTROY_INSTANCE,
		vkDestroyInstance);
	if (windowed) {
		FB_GFX3_VK_RESOLVE_NAMED(create_platform_surface, implementation,
			implementation->instance,
			FB_GFX3_VK_CREATE_PLATFORM_SURFACE,
			fb_gfx3_vulkan_platform_create_surface_function());
		FB_GFX3_VK_RESOLVE(implementation->destroy_surface,
			implementation, implementation->instance,
			FB_GFX3_VK_DESTROY_SURFACE, vkDestroySurfaceKHR);
		FB_GFX3_VK_RESOLVE(get_surface_support, implementation,
			implementation->instance,
			FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_SUPPORT,
			vkGetPhysicalDeviceSurfaceSupportKHR);
		FB_GFX3_VK_RESOLVE(implementation->get_surface_capabilities,
			implementation, implementation->instance,
			FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES,
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
		FB_GFX3_VK_RESOLVE(implementation->get_surface_formats,
			implementation, implementation->instance,
			FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_FORMATS,
			vkGetPhysicalDeviceSurfaceFormatsKHR);
		FB_GFX3_VK_RESOLVE(implementation->get_surface_present_modes,
			implementation, implementation->instance,
			FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES,
			vkGetPhysicalDeviceSurfacePresentModesKHR);
		if ((create_platform_surface == NULL) ||
		    (implementation->destroy_surface == NULL) ||
		    (get_surface_support == NULL) ||
		    (implementation->get_surface_capabilities == NULL) ||
		    (implementation->get_surface_formats == NULL) ||
		    (implementation->get_surface_present_modes == NULL)) {
			result = FB_GFX3_UNSUPPORTED;
			goto fail;
		}
		if (fb_gfx3_vulkan_platform_surface_create_info(
		    &surface_create_info, native_instance, native_window) !=
		    FB_GFX3_OK) {
			result = FB_GFX3_INVALID;
			goto fail;
		}
		if (create_platform_surface(implementation->instance,
		    &surface_create_info, NULL, &implementation->surface) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto fail;
		}
	}
	FB_GFX3_VK_RESOLVE(enumerate_physical_devices, implementation,
		implementation->instance,
		FB_GFX3_VK_ENUMERATE_PHYSICAL_DEVICES,
		vkEnumeratePhysicalDevices);
	FB_GFX3_VK_RESOLVE(get_queue_family_properties, implementation,
		implementation->instance,
		FB_GFX3_VK_GET_QUEUE_FAMILY_PROPERTIES,
		vkGetPhysicalDeviceQueueFamilyProperties);
	FB_GFX3_VK_RESOLVE(get_physical_device_features, implementation,
		implementation->instance,
		FB_GFX3_VK_GET_PHYSICAL_DEVICE_FEATURES,
		vkGetPhysicalDeviceFeatures);
	FB_GFX3_VK_RESOLVE(get_physical_device_properties, implementation,
		implementation->instance,
		FB_GFX3_VK_GET_PHYSICAL_DEVICE_PROPERTIES,
		vkGetPhysicalDeviceProperties);
	FB_GFX3_VK_RESOLVE(implementation->get_physical_device_memory_properties,
		implementation, implementation->instance,
		FB_GFX3_VK_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES,
		vkGetPhysicalDeviceMemoryProperties);
	FB_GFX3_VK_RESOLVE(create_device, implementation,
		implementation->instance, FB_GFX3_VK_CREATE_DEVICE,
		vkCreateDevice);
	FB_GFX3_VK_RESOLVE(implementation->get_device_proc_address,
		implementation, implementation->instance,
		FB_GFX3_VK_GET_DEVICE_PROC_ADDR, vkGetDeviceProcAddr);
	FB_GFX3_VK_RESOLVE(implementation->destroy_device, implementation,
		implementation->instance, FB_GFX3_VK_DESTROY_DEVICE,
		vkDestroyDevice);
	FB_GFX3_VK_RESOLVE(get_device_queue, implementation,
		implementation->instance, FB_GFX3_VK_GET_DEVICE_QUEUE,
		vkGetDeviceQueue);
	if ((implementation->destroy_instance == NULL) ||
	    (enumerate_physical_devices == NULL) ||
	    (get_queue_family_properties == NULL) ||
	    (get_physical_device_features == NULL) ||
	    (create_device == NULL) ||
	    (implementation->get_physical_device_memory_properties == NULL) ||
	    (implementation->get_device_proc_address == NULL) ||
	    (implementation->destroy_device == NULL) ||
	    (get_device_queue == NULL)) {
		result = FB_GFX3_UNSUPPORTED;
		goto fail;
	}
	result = vulkan_enumerate_physical_devices(enumerate_physical_devices,
		implementation->instance, &physical_devices,
		&physical_device_count);
	if (result != FB_GFX3_OK)
		goto fail;
	/*
		A numbered override makes multi-adapter diagnostics reproducible without
		changing the normal strongest-compatible-adapter policy. The index is the
		loader order reported by vulkaninfo, not a Windows display ordinal.
	*/
	requested_device_text = getenv("FBGFX3_VULKAN_DEVICE_INDEX");
	if ((requested_device_text != NULL) && (requested_device_text[0] != '\0')) {
		requested_device_value = strtoul(requested_device_text,
			&requested_device_end, 10);
		if ((*requested_device_end != '\0') ||
		    (requested_device_value >= physical_device_count)) {
			result = FB_GFX3_INVALID;
			goto fail;
		}
		selected_physical_device_index = (uint32_t)requested_device_value;
		device_is_forced = TRUE;
	}
	device_attempted = (uint8_t *)calloc(physical_device_count,
		sizeof(device_attempted[0]));
	if (device_attempted == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail;
	}

	memset(&queue_create_info, 0, sizeof(queue_create_info));
	queue_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queue_count = 1;
	queue_create_info.queue_priorities = &queue_priority;
	memset(&device_create_info, 0, sizeof(device_create_info));
	device_create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queue_create_info_count = 1;
	device_create_info.queue_create_infos = &queue_create_info;
	if (device_extension_count != 0u) {
		device_create_info.enabled_extension_count = device_extension_count;
		device_create_info.enabled_extension_names = device_extensions;
	}
	result = FB_GFX3_UNSUPPORTED;
	/*
		Choose the strongest compatible physical device, not the first loader
		entry.  Device creation remains a real capability check: a rejected
		adapter is remembered and the next ranked candidate is tried.
	*/
	for (feature_pass = 0;
	     (feature_pass < 2) && (result != FB_GFX3_OK); feature_pass++) {
		for (;;) {
			uint32_t best_device_index = UINT32_MAX;
			uint32_t best_queue_family_index = UINT32_MAX;
			uint32_t best_queue_flags = 0;
			uint64_t best_score = 0;

			for (device_index = 0; device_index < physical_device_count;
			     device_index++) {
				FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_STORAGE properties;
				FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_PREFIX prefix;
				uint32_t candidate_queue_family_index;
				uint32_t candidate_queue_flags;
				uint32_t device_type = FB_GFX3_VULKAN_DEVICE_TYPE_OTHER;
				uint64_t score;

				if (device_attempted[device_index] ||
				    (device_is_forced &&
				     (device_index != selected_physical_device_index)))
					continue;
			memset(&supported_features, 0,
					sizeof(supported_features));
			get_physical_device_features(physical_devices[device_index],
				&supported_features);
			if ((feature_pass == 0) &&
			    (supported_features.shader_float64 == 0))
				continue;
			if (vulkan_select_queue_family(get_queue_family_properties,
			    get_surface_support, physical_devices[device_index],
			    implementation->surface, &candidate_queue_family_index,
			    &candidate_queue_flags) != FB_GFX3_OK)
				continue;
			if (get_physical_device_properties != NULL) {
				memset(&properties, 0, sizeof(properties));
				get_physical_device_properties(physical_devices[device_index],
					properties.bytes);
				memcpy(&prefix, properties.bytes, sizeof(prefix));
				device_type = prefix.device_type;
			}
			score = fb_gfx3_vulkan_device_score(device_type,
				supported_features.shader_float64 != 0,
				candidate_queue_flags);
			if ((best_device_index == UINT32_MAX) ||
			    (score > best_score)) {
				best_device_index = device_index;
				best_queue_family_index = candidate_queue_family_index;
				best_queue_flags = candidate_queue_flags;
				best_score = score;
			}
			}
			if (best_device_index == UINT32_MAX)
				break;
			device_attempted[best_device_index] = TRUE;
			memset(&supported_features, 0, sizeof(supported_features));
			get_physical_device_features(physical_devices[best_device_index],
				&supported_features);
			memset(&enabled_features, 0, sizeof(enabled_features));
			enabled_features.shader_float64 =
				supported_features.shader_float64;
			device_create_info.enabled_features = &enabled_features;
			queue_create_info.queue_family_index = best_queue_family_index;
			if (create_device(physical_devices[best_device_index],
			    &device_create_info, NULL, &implementation->device) ==
			    FB_GFX3_VK_SUCCESS) {
				implementation->physical_device =
					physical_devices[best_device_index];
				selected_physical_device_index = best_device_index;
				if (get_physical_device_properties != NULL) {
					FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_STORAGE properties;
					FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_PREFIX prefix;

					memset(&properties, 0, sizeof(properties));
					get_physical_device_properties(
						physical_devices[best_device_index], properties.bytes);
					memcpy(&prefix, properties.bytes, sizeof(prefix));
					selected_vendor_id = prefix.vendor_id;
					selected_device_id = prefix.device_id;
					selected_device_type = prefix.device_type;
					memcpy(selected_device_name, prefix.device_name,
						sizeof(selected_device_name));
					selected_device_name[
						sizeof(selected_device_name) - 1u] = '\0';
					if (prefix.max_storage_buffer_range != 0u)
						selected_maximum_storage_buffer_range =
							prefix.max_storage_buffer_range;
				}
				implementation->maximum_storage_buffer_range =
					selected_maximum_storage_buffer_range;
				implementation->shader_float64 =
					enabled_features.shader_float64 != 0;
				queue_family_index = best_queue_family_index;
				queue_flags = best_queue_flags;
				result = FB_GFX3_OK;
				break;
			}
		}
	}
	if (result != FB_GFX3_OK)
		goto fail;
	get_device_queue(implementation->device, queue_family_index, 0,
		&implementation->queue);
	if (implementation->queue == NULL) {
		result = FB_GFX3_FAILED;
		goto fail;
	}
	result = vulkan_create_submission_objects(implementation,
		queue_family_index);
	if (result != FB_GFX3_OK)
		goto fail;
	result = vulkan_create_compute_objects(implementation);
	if (result != FB_GFX3_OK)
		goto fail;
	if (windowed) {
		result = fb_gfx3_vk_presentation_device_create(implementation);
		if (result != FB_GFX3_OK)
			goto fail;
	}

	free(device_attempted);
	free((void *)physical_devices);
	runtime->implementation = implementation;
	runtime->physical_device_count = physical_device_count;
	runtime->selected_physical_device_index = selected_physical_device_index;
	runtime->selected_vendor_id = selected_vendor_id;
	runtime->selected_device_id = selected_device_id;
	runtime->selected_device_type = selected_device_type;
	memcpy(runtime->selected_device_name, selected_device_name,
		sizeof(runtime->selected_device_name));
	runtime->maximum_storage_buffer_range =
		implementation->maximum_storage_buffer_range;
	runtime->queue_family_index = queue_family_index;
	runtime->queue_flags = queue_flags;
	runtime->present_width = implementation->swapchain_width;
	runtime->present_height = implementation->swapchain_height;
	runtime->swapchain_image_count =
		implementation->swapchain_image_count;
	runtime->windowed = windowed;
	runtime->initialized = TRUE;
	return FB_GFX3_OK;

fail:
	free(device_attempted);
	free((void *)physical_devices);
	vulkan_implementation_close(implementation);
	memset(runtime, 0, sizeof(*runtime));
	return result;
}

int fb_gfx3_vulkan_runtime_open(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	return vulkan_runtime_open_internal(runtime, 0, 0, 0, 0);
}

int fb_gfx3_vulkan_runtime_supports_float64(
	const FB_GFX3_VULKAN_RUNTIME *runtime)
{
	const FB_GFX3_VULKAN_IMPLEMENTATION *implementation;

	if ((runtime == NULL) || !runtime->initialized ||
	    (runtime->implementation == NULL))
		return FALSE;
	implementation = (const FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	return implementation->shader_float64 ? TRUE : FALSE;
}

int fb_gfx3_vulkan_runtime_open_windowed(FB_GFX3_VULKAN_RUNTIME *runtime,
	uintptr_t native_instance, uintptr_t native_window, uint32_t width,
	uint32_t height)
{
	return vulkan_runtime_open_internal(runtime, native_instance,
		native_window, width, height);
}

void fb_gfx3_vulkan_runtime_close(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	if (runtime == NULL)
		return;
	vulkan_implementation_close(
		(FB_GFX3_VULKAN_IMPLEMENTATION *)runtime->implementation);
	memset(runtime, 0, sizeof(*runtime));
}

#else

int fb_gfx3_vulkan_runtime_open(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	if (runtime != NULL)
		memset(runtime, 0, sizeof(*runtime));
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_supports_float64(
	const FB_GFX3_VULKAN_RUNTIME *runtime)
{
	(void)runtime;
	return FALSE;
}

int fb_gfx3_vulkan_runtime_open_windowed(FB_GFX3_VULKAN_RUNTIME *runtime,
	uintptr_t native_instance, uintptr_t native_window, uint32_t width,
	uint32_t height)
{
	(void)native_instance;
	(void)native_window;
	(void)width;
	(void)height;
	if (runtime != NULL)
		memset(runtime, 0, sizeof(*runtime));
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_resize(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t width, uint32_t height)
{
	(void)runtime;
	(void)width;
	(void)height;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_wait_idle(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	(void)runtime;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_tag_submission(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint64_t sequence)
{
	(void)runtime;
	(void)sequence;
	return FB_GFX3_UNSUPPORTED;
}

uint64_t fb_gfx3_vulkan_runtime_completed_sequence(
	FB_GFX3_VULKAN_RUNTIME *runtime)
{
	(void)runtime;
	return 0;
}

int fb_gfx3_vulkan_runtime_wait_sequence(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint64_t sequence)
{
	(void)runtime;
	(void)sequence;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_poll(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	(void)runtime;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_submit_empty(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	(void)runtime;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_fill_u32(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t *values, size_t value_count, uint32_t value)
{
	(void)runtime;
	(void)values;
	(void)value_count;
	(void)value;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_runtime_compute_add_u32(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t *values, size_t value_count, uint32_t addend)
{
	(void)runtime;
	(void)values;
	(void)value_count;
	(void)addend;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_create(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, uint32_t width, uint32_t height,
	uint32_t depth, uint32_t clear_color)
{
	(void)runtime;
	(void)surface;
	(void)width;
	(void)height;
	(void)depth;
	(void)clear_color;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_clear(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t x1, int32_t y1,
	int32_t x2, int32_t y2, uint32_t color)
{
	(void)runtime;
	(void)surface;
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
	(void)color;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_clear_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangles,
	size_t rectangle_count)
{
	(void)runtime;
	(void)surface;
	(void)rectangles;
	(void)rectangle_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_opaque_rectangle_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_CLEAR_RECTANGLE *rectangles, size_t rectangle_count)
{
	(void)runtime;
	(void)surface;
	(void)rectangles;
	(void)rectangle_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_rectangle_batch(
	FB_GFX3_VULKAN_RUNTIME *runtime, FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_RECTANGLE *rectangles, size_t rectangle_count)
{
	(void)runtime;
	(void)surface;
	(void)rectangles;
	(void)rectangle_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_points(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	const FB_GFX3_POINT *points, uint32_t point_count)
{
	(void)runtime;
	(void)surface;
	(void)clip;
	(void)points;
	(void)point_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_points_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_POINTS *operations, size_t operation_count)
{
	(void)runtime;
	(void)surface;
	(void)operations;
	(void)operation_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_line(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	uint32_t color, uint32_t style, uint32_t flags)
{
	(void)runtime;
	(void)surface;
	(void)clip;
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
	(void)color;
	(void)style;
	(void)flags;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_rectangle(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t x1, int32_t y1, int32_t x2, int32_t y2,
	uint32_t color, uint32_t style, int filled, uint32_t flags)
{
	(void)runtime;
	(void)surface;
	(void)clip;
	(void)x1;
	(void)y1;
	(void)x2;
	(void)y2;
	(void)color;
	(void)style;
	(void)filled;
	(void)flags;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_ellipse(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const FB_GFX3_RECT *clip,
	int32_t center_x, int32_t center_y, float radius_x, float radius_y,
	uint32_t color, int filled, uint32_t flags)
{
	(void)runtime;
	(void)surface;
	(void)clip;
	(void)center_x;
	(void)center_y;
	(void)radius_x;
	(void)radius_y;
	(void)color;
	(void)filled;
	(void)flags;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_ellipse_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	const FB_GFX3_VULKAN_ELLIPSE *ellipses, size_t ellipse_count)
{
	(void)runtime;
	(void)surface;
	(void)ellipses;
	(void)ellipse_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_blit(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *destination,
	FB_GFX3_VULKAN_SURFACE *source, const FB_GFX3_RECT *clip,
	const FB_GFX3_RECT *source_rect, int32_t destination_x,
	int32_t destination_y, uint32_t mode, uint32_t alpha)
{
	(void)runtime;
	(void)destination;
	(void)source;
	(void)clip;
	(void)source_rect;
	(void)destination_x;
	(void)destination_y;
	(void)mode;
	(void)alpha;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_copy_batch(FB_GFX3_VULKAN_RUNTIME *runtime,
	const FB_GFX3_VULKAN_SURFACE_COPY *copies, size_t copy_count)
{
	(void)runtime;
	(void)copies;
	(void)copy_count;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_upload(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t destination_x,
	int32_t destination_y, uint32_t width, uint32_t height,
	const void *source, size_t source_pitch)
{
	(void)runtime;
	(void)surface;
	(void)destination_x;
	(void)destination_y;
	(void)width;
	(void)height;
	(void)source;
	(void)source_pitch;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_download(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, int32_t source_x, int32_t source_y,
	uint32_t width, uint32_t height, void *destination,
	size_t destination_pitch)
{
	(void)runtime;
	(void)surface;
	(void)source_x;
	(void)source_y;
	(void)width;
	(void)height;
	(void)destination;
	(void)destination_pitch;
	return FB_GFX3_UNSUPPORTED;
}

int fb_gfx3_vulkan_surface_present(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const uint32_t *palette,
	size_t palette_count, const int32_t keyboard_button_rect[4],
	uint32_t keyboard_button_state)
{
	(void)runtime;
	(void)surface;
	(void)palette;
	(void)palette_count;
	(void)keyboard_button_rect;
	(void)keyboard_button_state;
	return FB_GFX3_UNSUPPORTED;
}

void fb_gfx3_vulkan_surface_destroy(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface)
{
	(void)runtime;
	if (surface != NULL)
		memset(surface, 0, sizeof(*surface));
}

void fb_gfx3_vulkan_runtime_close(FB_GFX3_VULKAN_RUNTIME *runtime)
{
	if (runtime != NULL)
		memset(runtime, 0, sizeof(*runtime));
}

#endif

/* end of gfx3_vulkan.c */
