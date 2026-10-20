/*
 * FreeBASIC gfxlib3: gfx3_vulkan_present.c
 * Purpose: Own Vulkan swapchain creation and presentation resources.
 * Responsibilities: render-thread operations within this module.
 * This file intentionally does NOT contain BASIC compatibility or native input dispatch.
 */

#include "gfx3_vulkan_private.h"

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || defined(HOST_DARWIN)

/* ------------------------------------------------------------------------- */
/* Window presentation resources                                             */
/* ------------------------------------------------------------------------- */

static void vulkan_present_buffers_destroy(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	FB_GFX3_VULKAN_BUFFER_ALLOCATION allocation;
	uint32_t index;

	if (implementation == NULL)
		return;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++)
		fb_gfx3_vk_buffer_allocation_destroy(implementation,
			&implementation->submission_slots[index].
			present_command_buffer);
	memset(&allocation, 0, sizeof(allocation));
	allocation.buffer = implementation->present_buffer;
	allocation.memory = implementation->present_memory;
	allocation.size = implementation->present_buffer_size;
	fb_gfx3_vk_buffer_allocation_destroy(implementation, &allocation);
	implementation->present_buffer = 0;
	implementation->present_memory = 0;
	implementation->present_buffer_size = 0;
}

void fb_gfx3_vk_swapchain_destroy(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	uint32_t index;

	if (implementation == NULL)
		return;
	vulkan_present_buffers_destroy(implementation);
	if ((implementation->swapchain_rendering_finished != NULL) &&
	    (implementation->destroy_semaphore != NULL)) {
		for (index = 0; index < implementation->swapchain_image_count;
		     index++) {
			if (implementation->swapchain_rendering_finished[index] != 0)
				implementation->destroy_semaphore(
					implementation->device,
					implementation->swapchain_rendering_finished[index],
					NULL);
		}
	}
	free(implementation->swapchain_rendering_finished);
	free(implementation->swapchain_image_initialized);
	free(implementation->swapchain_images);
	implementation->swapchain_rendering_finished = NULL;
	implementation->swapchain_image_initialized = NULL;
	implementation->swapchain_images = NULL;
	implementation->rendering_finished = 0;
	implementation->swapchain_image_count = 0;
	implementation->swapchain_width = 0;
	implementation->swapchain_height = 0;
	if ((implementation->swapchain != 0) &&
	    (implementation->destroy_swapchain != NULL))
		implementation->destroy_swapchain(implementation->device,
			implementation->swapchain, NULL);
	implementation->swapchain = 0;
}

static uint32_t vulkan_clamp_u32(uint32_t value, uint32_t minimum,
	uint32_t maximum)
{
	if (value < minimum)
		return minimum;
	if (value > maximum)
		return maximum;
	return value;
}

static int vulkan_choose_surface_format(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_SURFACE_FORMAT *selected)
{
	FB_GFX3_VK_SURFACE_FORMAT *formats = NULL;
	uint32_t count = 0;
	uint32_t written;
	uint32_t index;
	size_t allocation_size;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) || (selected == NULL) ||
	    (implementation->get_surface_formats == NULL))
		return FB_GFX3_INVALID;
	result = implementation->get_surface_formats(
		implementation->physical_device, implementation->surface,
		&count, NULL);
	if ((result != FB_GFX3_VK_SUCCESS) || (count == 0) ||
	    (fb_gfx3_vk_size_multiply(count, sizeof(formats[0]),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_UNSUPPORTED;
	formats = (FB_GFX3_VK_SURFACE_FORMAT *)calloc(1, allocation_size);
	if (formats == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	written = count;
	result = implementation->get_surface_formats(
		implementation->physical_device, implementation->surface,
		&written, formats);
	if ((result != FB_GFX3_VK_SUCCESS) || (written == 0)) {
		free(formats);
		return FB_GFX3_FAILED;
	}
	if ((written == 1) && (formats[0].format == 0)) {
		selected->format = FB_GFX3_VK_FORMAT_B8G8R8A8_UNORM;
		selected->color_space = formats[0].color_space;
		free(formats);
		return FB_GFX3_OK;
	}
	for (index = 0; index < written; index++) {
		if (formats[index].format == FB_GFX3_VK_FORMAT_B8G8R8A8_UNORM) {
			*selected = formats[index];
			free(formats);
			return FB_GFX3_OK;
		}
	}
	free(formats);
	return FB_GFX3_UNSUPPORTED;
}

static int vulkan_choose_present_mode(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation, uint32_t *selected)
{
	uint32_t *modes = NULL;
	uint32_t count = 0;
	uint32_t written;
	uint32_t index;
	size_t allocation_size;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) || (selected == NULL) ||
	    (implementation->get_surface_present_modes == NULL))
		return FB_GFX3_INVALID;
	result = implementation->get_surface_present_modes(
		implementation->physical_device, implementation->surface,
		&count, NULL);
	if ((result != FB_GFX3_VK_SUCCESS) || (count == 0) ||
	    (fb_gfx3_vk_size_multiply(count, sizeof(modes[0]),
	     &allocation_size) != FB_GFX3_OK))
		return FB_GFX3_UNSUPPORTED;
	modes = (uint32_t *)calloc(1, allocation_size);
	if (modes == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	written = count;
	result = implementation->get_surface_present_modes(
		implementation->physical_device, implementation->surface,
		&written, modes);
	if ((result != FB_GFX3_VK_SUCCESS) || (written == 0)) {
		free(modes);
		return FB_GFX3_FAILED;
	}
	/*
		Prefer the lowest-latency mode available.  FIFO is the Vulkan-required
		fallback, but choosing it when IMMEDIATE is advertised imposes a refresh
		wait that the gfxlib2 page-copy contract does not require.
	*/
	*selected = FB_GFX3_VK_PRESENT_MODE_FIFO;
	for (index = 0; index < written; index++) {
		if (modes[index] == FB_GFX3_VK_PRESENT_MODE_IMMEDIATE) {
			*selected = FB_GFX3_VK_PRESENT_MODE_IMMEDIATE;
			break;
		}
		if (modes[index] == FB_GFX3_VK_PRESENT_MODE_MAILBOX)
			*selected = FB_GFX3_VK_PRESENT_MODE_MAILBOX;
	}
	free(modes);
	return FB_GFX3_OK;
}

int fb_gfx3_vk_swapchain_create(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	static const uint32_t composite_alpha_modes[] = {
		FB_GFX3_VK_COMPOSITE_ALPHA_OPAQUE_BIT,
		FB_GFX3_VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT,
		FB_GFX3_VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT,
		FB_GFX3_VK_COMPOSITE_ALPHA_INHERIT_BIT
	};
	FB_GFX3_VK_SURFACE_CAPABILITIES capabilities;
	FB_GFX3_VK_SURFACE_FORMAT surface_format;
	FB_GFX3_VK_SWAPCHAIN_CREATE_INFO create_info;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION present_allocation;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION command_allocation;
	FB_GFX3_VK_EXTENT_2D extent;
	uint32_t present_mode;
	uint32_t composite_alpha = 0;
	uint32_t image_count;
	uint32_t written;
	uint32_t index;
	size_t pixel_count;
	size_t buffer_size;
	size_t image_allocation_size;
	size_t semaphore_allocation_size;
	int result;

	if ((implementation == NULL) || (implementation->surface == 0) ||
	    (implementation->create_swapchain == NULL) ||
	    (implementation->get_swapchain_images == NULL) ||
	    (implementation->get_surface_capabilities == NULL))
		return FB_GFX3_INVALID;
	if (implementation->device_wait_idle(implementation->device) !=
	    FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	if (fb_gfx3_vk_submission_wait_all(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	fb_gfx3_vk_swapchain_destroy(implementation);
	memset(&capabilities, 0, sizeof(capabilities));
	if (implementation->get_surface_capabilities(
	    implementation->physical_device, implementation->surface,
	    &capabilities) != FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	if ((capabilities.supported_usage_flags &
	    FB_GFX3_VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
		return FB_GFX3_UNSUPPORTED;
	if (capabilities.current_extent.width != UINT32_MAX) {
		extent = capabilities.current_extent;
	} else {
		extent.width = vulkan_clamp_u32(implementation->desired_width,
			capabilities.minimum_image_extent.width,
			capabilities.maximum_image_extent.width);
		extent.height = vulkan_clamp_u32(implementation->desired_height,
			capabilities.minimum_image_extent.height,
			capabilities.maximum_image_extent.height);
	}
	if ((extent.width == 0) || (extent.height == 0))
		return FB_GFX3_EXHAUSTED;
	result = vulkan_choose_surface_format(implementation, &surface_format);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_choose_present_mode(implementation, &present_mode);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0;
	     index < (sizeof(composite_alpha_modes) /
	     sizeof(composite_alpha_modes[0])); index++) {
		if (capabilities.supported_composite_alpha &
		    composite_alpha_modes[index]) {
			composite_alpha = composite_alpha_modes[index];
			break;
		}
	}
	if (composite_alpha == 0)
		return FB_GFX3_UNSUPPORTED;
	image_count = capabilities.minimum_image_count + 1;
	if (image_count < capabilities.minimum_image_count)
		return FB_GFX3_INVALID;
	if ((capabilities.maximum_image_count != 0) &&
	    (image_count > capabilities.maximum_image_count))
		image_count = capabilities.maximum_image_count;
	memset(&create_info, 0, sizeof(create_info));
	create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO;
	create_info.surface = implementation->surface;
	create_info.minimum_image_count = image_count;
	create_info.image_format = surface_format.format;
	create_info.image_color_space = surface_format.color_space;
	create_info.image_extent = extent;
	create_info.image_array_layers = 1;
	create_info.image_usage = FB_GFX3_VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create_info.image_sharing_mode = FB_GFX3_VK_SHARING_MODE_EXCLUSIVE;
	create_info.pre_transform = capabilities.current_transform;
	create_info.composite_alpha = composite_alpha;
	create_info.present_mode = present_mode;
	create_info.clipped = TRUE;
	if (implementation->create_swapchain(implementation->device,
	    &create_info, NULL, &implementation->swapchain) !=
	    FB_GFX3_VK_SUCCESS)
		return FB_GFX3_FAILED;
	written = 0;
	if ((implementation->get_swapchain_images(implementation->device,
	    implementation->swapchain, &written, NULL) !=
	    FB_GFX3_VK_SUCCESS) || (written == 0) ||
	    (fb_gfx3_vk_size_multiply(written,
	     sizeof(implementation->swapchain_images[0]),
	     &image_allocation_size) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto fail;
	}
	implementation->swapchain_images = (FB_GFX3_VK_IMAGE *)
		calloc(1, image_allocation_size);
	implementation->swapchain_image_initialized = (unsigned char *)
		calloc(written, sizeof(unsigned char));
	if ((implementation->swapchain_images == NULL) ||
	    (implementation->swapchain_image_initialized == NULL)) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail;
	}
	image_count = written;
	if ((implementation->get_swapchain_images(implementation->device,
	    implementation->swapchain, &written,
	    implementation->swapchain_images) != FB_GFX3_VK_SUCCESS) ||
	    (written == 0) || (written > image_count)) {
		result = FB_GFX3_FAILED;
		goto fail;
	}
	implementation->swapchain_image_count = written;
	if ((implementation->create_semaphore == NULL) ||
	    (fb_gfx3_vk_size_multiply(written,
	     sizeof(implementation->swapchain_rendering_finished[0]),
	     &semaphore_allocation_size) != FB_GFX3_OK)) {
		result = FB_GFX3_INVALID;
		goto fail;
	}
	implementation->swapchain_rendering_finished =
		(FB_GFX3_VK_SEMAPHORE *)calloc(1, semaphore_allocation_size);
	if (implementation->swapchain_rendering_finished == NULL) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto fail;
	}
	for (index = 0; index < written; index++) {
		FB_GFX3_VK_SEMAPHORE_CREATE_INFO semaphore_create_info;

		memset(&semaphore_create_info, 0, sizeof(semaphore_create_info));
		semaphore_create_info.structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (implementation->create_semaphore(implementation->device,
		    &semaphore_create_info, NULL,
		    &implementation->swapchain_rendering_finished[index]) !=
		    FB_GFX3_VK_SUCCESS) {
			result = FB_GFX3_FAILED;
			goto fail;
		}
	}
	implementation->rendering_finished =
		implementation->swapchain_rendering_finished[0];
	if ((fb_gfx3_vk_size_multiply(extent.width, extent.height, &pixel_count) !=
	     FB_GFX3_OK) ||
	    (fb_gfx3_vk_size_multiply(pixel_count, sizeof(uint32_t),
	     &buffer_size) != FB_GFX3_OK) || (buffer_size == 0)) {
		result = FB_GFX3_INVALID;
		goto fail;
	}
	memset(&present_allocation, 0, sizeof(present_allocation));
	result = fb_gfx3_vk_buffer_allocation_create(implementation,
		&present_allocation, (uint64_t)buffer_size,
		FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
	if (result != FB_GFX3_OK)
		goto fail;
	implementation->present_buffer = present_allocation.buffer;
	implementation->present_memory = present_allocation.memory;
	implementation->present_buffer_size = present_allocation.size;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		memset(&command_allocation, 0, sizeof(command_allocation));
		result = fb_gfx3_vk_buffer_allocation_create(implementation,
			&command_allocation,
			sizeof(FB_GFX3_VULKAN_PRESENT_COMMAND),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			goto fail;
		implementation->submission_slots[index].
			present_command_buffer = command_allocation;
	}
	implementation->swapchain_width = extent.width;
	implementation->swapchain_height = extent.height;
	implementation->swapchain_format = surface_format.format;
	implementation->swapchain_color_space = surface_format.color_space;
	return FB_GFX3_OK;

fail:
	fb_gfx3_vk_swapchain_destroy(implementation);
	return result;
}

int fb_gfx3_vk_presentation_device_create(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	FB_GFX3_VK_SEMAPHORE_CREATE_INFO create_info;
	uint32_t index;

	FB_GFX3_VK_RESOLVE_DEVICE(implementation->create_swapchain,
		implementation, FB_GFX3_VK_CREATE_SWAPCHAIN, vkCreateSwapchainKHR);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_swapchain,
		implementation, FB_GFX3_VK_DESTROY_SWAPCHAIN,
		vkDestroySwapchainKHR);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->get_swapchain_images,
		implementation, FB_GFX3_VK_GET_SWAPCHAIN_IMAGES,
		vkGetSwapchainImagesKHR);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->acquire_next_image,
		implementation, FB_GFX3_VK_ACQUIRE_NEXT_IMAGE,
		vkAcquireNextImageKHR);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->queue_present,
		implementation, FB_GFX3_VK_QUEUE_PRESENT, vkQueuePresentKHR);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->create_semaphore,
		implementation,
		FB_GFX3_VK_CREATE_SEMAPHORE, vkCreateSemaphore);
	FB_GFX3_VK_RESOLVE_DEVICE(implementation->destroy_semaphore,
		implementation, FB_GFX3_VK_DESTROY_SEMAPHORE,
		vkDestroySemaphore);
	if ((implementation->create_swapchain == NULL) ||
	    (implementation->destroy_swapchain == NULL) ||
	    (implementation->get_swapchain_images == NULL) ||
	    (implementation->acquire_next_image == NULL) ||
	    (implementation->queue_present == NULL) ||
	    (implementation->create_semaphore == NULL) ||
	    (implementation->destroy_semaphore == NULL))
		return FB_GFX3_UNSUPPORTED;
	memset(&create_info, 0, sizeof(create_info));
	create_info.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		if (implementation->create_semaphore(implementation->device,
		    &create_info, NULL,
		    &implementation->submission_slots[index].image_available) !=
		    FB_GFX3_VK_SUCCESS)
			return FB_GFX3_FAILED;
	}
	implementation->image_available =
		implementation->submission_slots[0].image_available;
	return fb_gfx3_vk_swapchain_create(implementation);
}


int fb_gfx3_vulkan_runtime_resize(FB_GFX3_VULKAN_RUNTIME *runtime,
	uint32_t width, uint32_t height)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	int result;

	if ((runtime == NULL) || !runtime->initialized || !runtime->windowed ||
	    (runtime->implementation == NULL))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	implementation->desired_width = width;
	implementation->desired_height = height;
	if ((width == 0) || (height == 0))
		return FB_GFX3_OK;
	if ((implementation->swapchain_width == width) &&
	    (implementation->swapchain_height == height))
		return FB_GFX3_OK;

	/*
		Xlib implementations are allowed to keep an old swapchain usable for a
		short time after the native window changes size.  Recreating from the
		window-system notification avoids relying on an eventual OUT_OF_DATE
		result and prevents the old image from occupying only one corner of the
		new client area.
	*/
	result = fb_gfx3_vk_swapchain_create(implementation);
	if (result == FB_GFX3_EXHAUSTED)
		return FB_GFX3_OK;
	if (result != FB_GFX3_OK)
		return result;
	runtime->present_width = implementation->swapchain_width;
	runtime->present_height = implementation->swapchain_height;
	runtime->swapchain_image_count = implementation->swapchain_image_count;
	return FB_GFX3_OK;
}

int fb_gfx3_vulkan_surface_present(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface, const uint32_t *palette,
	size_t palette_count, const int32_t keyboard_button_rect[4],
	uint32_t keyboard_button_state)
{
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation;
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage;
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot;
	FB_GFX3_VULKAN_PRESENT_COMMAND *command;
	FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO buffer_infos[3];
	FB_GFX3_VK_WRITE_DESCRIPTOR_SET descriptor_writes[3];
	FB_GFX3_VK_IMAGE_MEMORY_BARRIER image_barrier;
	FB_GFX3_VK_BUFFER_IMAGE_COPY copy_region;
	FB_GFX3_VK_RESULT acquire_result = FB_GFX3_VK_ERROR_OUT_OF_DATE;
	FB_GFX3_PRESENTATION_LAYOUT layout;
	uint32_t image_index = 0;
	uint32_t group_count;
	size_t pixel_count = 0;
	int recreate = FALSE;
	int acquire_suboptimal = FALSE;
	int attempt;
	int result;

	if ((runtime == NULL) || !runtime->initialized || !runtime->windowed ||
	    (runtime->implementation == NULL) || (surface == NULL) ||
	    (surface->implementation == NULL) || (palette == NULL) ||
	    (palette_count < 256))
		return FB_GFX3_INVALID;
	implementation = (FB_GFX3_VULKAN_IMPLEMENTATION *)
		runtime->implementation;
	storage = (FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *)
		surface->implementation;
	if ((storage->owner != implementation) ||
	    (fb_gfx3_vk_surface_bytes_per_pixel(storage->depth) == 0) ||
	    (implementation->present_pipeline == 0))
		return FB_GFX3_INVALID;
	if (implementation->swapchain == 0) {
		result = fb_gfx3_vk_swapchain_create(implementation);
		if (result == FB_GFX3_EXHAUSTED)
			return FB_GFX3_OK;
		if (result != FB_GFX3_OK)
			return result;
	}
	/*
		vkAcquireNextImageKHR signals the next slot's image_available
		semaphore. Prove that the slot's previous presentation marker completed
		before asking Vulkan to signal that binary semaphore again.
	*/
	if (implementation->next_submission_slot >=
	    FB_GFX3_VK_SUBMISSION_SLOT_COUNT)
		return FB_GFX3_FAILED;
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (fb_gfx3_vk_submission_slot_wait(implementation, slot) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	fb_gfx3_vk_runtime_update_submission_telemetry(runtime, implementation);
	for (attempt = 0; attempt < 2; attempt++) {
		if ((fb_gfx3_vk_size_multiply(implementation->swapchain_width,
		    implementation->swapchain_height, &pixel_count) !=
		    FB_GFX3_OK) || (pixel_count == 0))
			return FB_GFX3_INVALID;
		acquire_result = implementation->acquire_next_image(
			implementation->device, implementation->swapchain,
			UINT64_MAX,
			implementation->submission_slots[
				implementation->next_submission_slot].image_available,
			0,
			&image_index);
		if (acquire_result != FB_GFX3_VK_ERROR_OUT_OF_DATE)
			break;
		result = fb_gfx3_vk_swapchain_create(implementation);
		if (result == FB_GFX3_EXHAUSTED)
			return FB_GFX3_OK;
		if (result != FB_GFX3_OK)
			return result;
	}
	if ((acquire_result != FB_GFX3_VK_SUCCESS) &&
	    (acquire_result != FB_GFX3_VK_SUBOPTIMAL))
		return FB_GFX3_FAILED;
	if ((image_index >= implementation->swapchain_image_count) ||
	    (implementation->swapchain_rendering_finished == NULL) ||
	    (implementation->swapchain_rendering_finished[image_index] == 0))
		return FB_GFX3_FAILED;
	/*
		Reacquiring this image proves that its previous presentation operation
		has completed, including the wait on this image-owned semaphore.
	*/
	implementation->rendering_finished =
		implementation->swapchain_rendering_finished[image_index];
	acquire_suboptimal = (acquire_result == FB_GFX3_VK_SUBOPTIMAL);
	result = fb_gfx3_vk_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	slot = &implementation->submission_slots[
		implementation->active_submission_slot];
	if ((slot->present_command_buffer.buffer == 0) ||
	    (slot->present_command_buffer.mapped == NULL) ||
	    (slot->present_command_buffer.size <
	     sizeof(FB_GFX3_VULKAN_PRESENT_COMMAND)))
		return FB_GFX3_FAILED;
	command = (FB_GFX3_VULKAN_PRESENT_COMMAND *)
		slot->present_command_buffer.mapped;
	memset(command, 0, sizeof(*command));
	command->source[0] = storage->width;
	command->source[1] = storage->height;
	command->source[2] = storage->depth;
	command->destination[0] = implementation->swapchain_width;
	command->destination[1] = implementation->swapchain_height;
	result = fb_gfx3_presentation_layout_calculate(storage->width,
		storage->height, implementation->swapchain_width,
		implementation->swapchain_height, &layout);
	if (result != FB_GFX3_OK)
		return result;
	command->presentation_rect[0] = layout.x;
	command->presentation_rect[1] = layout.y;
	command->presentation_rect[2] = (int32_t)layout.width;
	command->presentation_rect[3] = (int32_t)layout.height;
	memcpy(command->palette, palette, sizeof(command->palette));
	if ((keyboard_button_rect != NULL) &&
	    (keyboard_button_state >= 1u) && (keyboard_button_state <= 3u)) {
		memcpy(command->keyboard_button_rect, keyboard_button_rect,
			sizeof(command->keyboard_button_rect));
		command->keyboard_button_state = keyboard_button_state;
	}
	memset(buffer_infos, 0, sizeof(buffer_infos));
	buffer_infos[0].buffer = implementation->present_buffer;
	buffer_infos[0].range = implementation->present_buffer_size;
	buffer_infos[1].buffer = storage->storage.buffer;
	buffer_infos[1].range = storage->storage.size;
	buffer_infos[2].buffer = slot->present_command_buffer.buffer;
	buffer_infos[2].range = slot->present_command_buffer.size;
	memset(descriptor_writes, 0, sizeof(descriptor_writes));
	for (attempt = 0; attempt < 3; attempt++) {
		descriptor_writes[attempt].structure_type =
			FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[attempt].destination_set =
			implementation->compute_descriptor_set;
		descriptor_writes[attempt].destination_binding =
			(uint32_t)attempt;
		descriptor_writes[attempt].descriptor_count = 1;
		descriptor_writes[attempt].descriptor_type =
			FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_writes[attempt].buffer_info = &buffer_infos[attempt];
	}
	implementation->update_descriptor_sets(implementation->device, 3,
		descriptor_writes, 0, NULL);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		implementation->present_buffer,
		implementation->present_buffer_size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		slot->present_command_buffer.buffer,
		slot->present_command_buffer.size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_bind_pipeline(implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->present_pipeline);
	implementation->command_bind_descriptor_sets(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE,
		implementation->compute_pipeline_layout, 0, 1,
		&implementation->compute_descriptor_set, 0, NULL);
	group_count = (uint32_t)((pixel_count + 63) / 64);
	if (group_count > 65535)
		group_count = 65535;
	implementation->command_dispatch(implementation->command_buffer,
		group_count, 1, 1);
	fb_gfx3_vk_record_buffer_barrier(implementation,
		implementation->present_buffer,
		implementation->present_buffer_size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	memset(&image_barrier, 0, sizeof(image_barrier));
	image_barrier.structure_type =
		FB_GFX3_VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_barrier.source_access_mask =
		implementation->swapchain_image_initialized[image_index] ?
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT : 0;
	image_barrier.destination_access_mask =
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT;
	image_barrier.old_layout =
		implementation->swapchain_image_initialized[image_index] ?
		FB_GFX3_VK_IMAGE_LAYOUT_PRESENT_SOURCE :
		FB_GFX3_VK_IMAGE_LAYOUT_UNDEFINED;
	image_barrier.new_layout =
		FB_GFX3_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_barrier.source_queue_family_index = UINT32_MAX;
	image_barrier.destination_queue_family_index = UINT32_MAX;
	image_barrier.image = implementation->swapchain_images[image_index];
	image_barrier.subresource_range.aspect_mask =
		FB_GFX3_VK_IMAGE_ASPECT_COLOR_BIT;
	image_barrier.subresource_range.level_count = 1;
	image_barrier.subresource_range.layer_count = 1;
	implementation->command_pipeline_barrier(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL,
		1, &image_barrier);
	memset(&copy_region, 0, sizeof(copy_region));
	copy_region.image_subresource.aspect_mask =
		FB_GFX3_VK_IMAGE_ASPECT_COLOR_BIT;
	copy_region.image_subresource.layer_count = 1;
	copy_region.image_extent.width = implementation->swapchain_width;
	copy_region.image_extent.height = implementation->swapchain_height;
	copy_region.image_extent.depth = 1;
	implementation->command_copy_buffer_to_image(
		implementation->command_buffer, implementation->present_buffer,
		implementation->swapchain_images[image_index],
		FB_GFX3_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
	image_barrier.source_access_mask =
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT;
	image_barrier.destination_access_mask =
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT;
	image_barrier.old_layout =
		FB_GFX3_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	image_barrier.new_layout = FB_GFX3_VK_IMAGE_LAYOUT_PRESENT_SOURCE;
	implementation->command_pipeline_barrier(
		implementation->command_buffer,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL,
		1, &image_barrier);
	result = fb_gfx3_vk_end_present_commands(implementation, image_index,
		&recreate);
	if (result != FB_GFX3_OK)
		return result;
	fb_gfx3_vk_runtime_update_submission_telemetry(runtime, implementation);
	implementation->swapchain_image_initialized[image_index] = TRUE;
	runtime->completed_submission_count++;
	if (recreate || acquire_suboptimal) {
		result = fb_gfx3_vk_swapchain_create(implementation);
		if ((result != FB_GFX3_OK) && (result != FB_GFX3_EXHAUSTED))
			return result;
	}
	runtime->present_width = implementation->swapchain_width;
	runtime->present_height = implementation->swapchain_height;
	runtime->swapchain_image_count =
		implementation->swapchain_image_count;
	return FB_GFX3_OK;
}
#endif

/* end of gfx3_vulkan_present.c */
