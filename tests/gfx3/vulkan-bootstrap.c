/*
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: vulkan-bootstrap.c

    Purpose:

        Verify the real Vulkan runtime, queue submission, compute dispatch,
        device-local surfaces, and transfer lifecycle without requiring
        Vulkan SDK headers or link libraries.

    Responsibilities:

        - open a real Vulkan runtime twice
        - verify and report physical-device and compute-queue discovery
        - verify fenced queue reuse, GPU fill, and storage-buffer compute
        - verify device-local clear, descriptor isolation, deferred staging,
          and pitched upload/download pixels
        - verify close clears all published runtime state

    This file intentionally does NOT contain:

        - a presentation surface or visible window
        - common renderer protocol or FreeBASIC API tests
        - fallback to the OpenGL renderer
*/

#include "../../src/gfxlib3/gfx3_vulkan.h"

#include <stdio.h>

int main(void)
{
	FB_GFX3_VULKAN_RUNTIME runtime;
	FB_GFX3_VULKAN_SURFACE surface;
	uint32_t fill_values[257];
	uint32_t expected_surface[13][19];
	uint32_t upload_source[4][7];
	unsigned char surface_download[13][(19 * 4) + 8];
	FB_GFX3_POINT descriptor_points[3];
	FB_GFX3_RECT descriptor_clip;
	uint32_t fill_value;
	uint32_t compute_addend;
	int iteration;
	int submission;
	size_t value_index;
	size_t x;
	size_t y;
	int result;

	memset(&runtime, 0, sizeof(runtime));
	for (iteration = 0; iteration < 2; iteration++) {
		result = fb_gfx3_vulkan_runtime_open(&runtime);
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan bootstrap failed: %d\n", result);
			return 1;
		}
		if (!runtime.initialized || (runtime.implementation == NULL) ||
		    (runtime.physical_device_count == 0) ||
		    ((runtime.queue_flags & 0x00000002u) == 0)) {
			fprintf(stderr, "Vulkan bootstrap returned invalid state\n");
			return 2;
		}
		printf("Vulkan bootstrap: version %u.%u, devices %u, selected %u "
			"(%04x:%04x), queue %u\n",
			(runtime.loader_api_version >> 22) & 0x7Fu,
			(runtime.loader_api_version >> 12) & 0x3FFu,
			runtime.physical_device_count,
			runtime.selected_physical_device_index,
			runtime.selected_vendor_id, runtime.selected_device_id,
			runtime.queue_family_index);
		for (submission = 0; submission < 4; submission++) {
			result = fb_gfx3_vulkan_runtime_submit_empty(&runtime);
			if (result != FB_GFX3_OK) {
				fprintf(stderr, "Vulkan submit failed: %d\n", result);
				return 3;
			}
			result = fb_gfx3_vulkan_runtime_tag_submission(&runtime,
				(submission + 1) * 10);
			if (result != FB_GFX3_OK) {
				fprintf(stderr, "Vulkan sequence tag failed: %d\n", result);
				return 19;
			}
		}
		/*
			The first three operations share one driver submission. The fourth
			starts a second batch in a different slot, so no completed fence is
			required before the renderer can continue recording.
		*/
		if ((runtime.in_flight_submission_count != 4) ||
		    (runtime.maximum_in_flight_submission_count < 4) ||
		    (runtime.queue_submit_count != 1u) ||
		    (fb_gfx3_vulkan_runtime_completed_sequence(&runtime) != 0u)) {
			fprintf(stderr, "Vulkan submission batching is inactive\n");
			return 18;
		}
		result = fb_gfx3_vulkan_runtime_wait_sequence(&runtime, 20);
		if ((result != FB_GFX3_OK) ||
		    (runtime.in_flight_submission_count != 1) ||
		    (runtime.queue_submit_count != 1u) ||
		    (fb_gfx3_vulkan_runtime_completed_sequence(&runtime) < 20u) ||
		    (fb_gfx3_vulkan_runtime_completed_sequence(&runtime) > 30u)) {
			fprintf(stderr, "Vulkan sequence wait drained later work\n");
			return 20;
		}
		/*
			The render thread polls fences between commands. It must accept both
			an active slot and a slot which completed while this test was waiting,
			without turning the poll into an implicit queue drain.
		*/
		result = fb_gfx3_vulkan_runtime_poll(&runtime);
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan submission poll failed: %d\n", result);
			return 22;
		}
		result = fb_gfx3_vulkan_runtime_wait_sequence(&runtime, 40);
		if ((result != FB_GFX3_OK) ||
		    (runtime.in_flight_submission_count != 0) ||
		    (runtime.queue_submit_count != 2u)) {
			fprintf(stderr, "Vulkan sequence wait did not drain tagged work\n");
			return 21;
		}
		fill_value = 0x13579BDFu ^ (uint32_t)iteration;
		memset(fill_values, 0xA5, sizeof(fill_values));
		result = fb_gfx3_vulkan_runtime_fill_u32(&runtime, fill_values,
			sizeof(fill_values) / sizeof(fill_values[0]), fill_value);
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan GPU fill failed: %d\n", result);
			return 4;
		}
		for (value_index = 0;
		     value_index < sizeof(fill_values) / sizeof(fill_values[0]);
		     value_index++) {
			if (fill_values[value_index] != fill_value) {
				fprintf(stderr,
					"Vulkan GPU fill mismatch at %zu\n",
					value_index);
				return 5;
			}
		}
		compute_addend = 0x10203040u + (uint32_t)iteration;
		for (value_index = 0;
		     value_index < sizeof(fill_values) / sizeof(fill_values[0]);
		     value_index++)
			fill_values[value_index] =
				(uint32_t)value_index * 0x01010101u;
		result = fb_gfx3_vulkan_runtime_compute_add_u32(&runtime,
			fill_values, sizeof(fill_values) / sizeof(fill_values[0]),
			compute_addend);
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan compute dispatch failed: %d\n", result);
			return 6;
		}
		for (value_index = 0;
		     value_index < sizeof(fill_values) / sizeof(fill_values[0]);
		     value_index++) {
			uint32_t expected = (uint32_t)value_index * 0x01010101u +
				compute_addend;

			if (fill_values[value_index] != expected) {
				fprintf(stderr,
					"Vulkan compute mismatch at %zu\n",
					value_index);
				return 7;
			}
		}

		memset(&surface, 0, sizeof(surface));
		result = fb_gfx3_vulkan_surface_create(&runtime, &surface,
			4097, 1, 32, 0);
		if ((result != FB_GFX3_UNSUPPORTED) ||
		    (surface.implementation != NULL)) {
			fprintf(stderr,
				"Vulkan conservative surface limit failed: %d\n",
				result);
			return 8;
		}
		for (y = 0; y < 13; y++) {
			for (x = 0; x < 19; x++)
				expected_surface[y][x] = 0x11223344u;
		}
		for (y = 0; y < 4; y++) {
			for (x = 0; x < 7; x++)
				upload_source[y][x] = (x < 5) ?
					(0x80000000u | ((uint32_t)y << 8) |
					 (uint32_t)x) : 0xDEADBEEFu;
		}
		result = fb_gfx3_vulkan_surface_create(&runtime, &surface,
			19, 13, 32, 0x11223344u);
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan surface create failed: %d\n", result);
			return 9;
		}
		result = fb_gfx3_vulkan_surface_upload(&runtime, &surface, 3, 2,
			5, 4, upload_source, sizeof(upload_source[0]));
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan surface upload failed: %d\n", result);
			return 10;
		}
		for (y = 0; y < 4; y++) {
			for (x = 0; x < 5; x++)
				expected_surface[y + 2][x + 3] =
					upload_source[y][x];
		}
		result = fb_gfx3_vulkan_surface_clear(&runtime, &surface,
			-2, 9, 6, 20, 0x55667788u);
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan surface clear failed: %d\n", result);
			return 11;
		}
		for (y = 9; y < 13; y++) {
			for (x = 0; x <= 6; x++)
				expected_surface[y][x] = 0x55667788u;
		}
		/*
			These three point streams occupy distinct submission slots. Each
			stream owns a host-visible staging buffer and descriptor set while
			it is in flight. A later descriptor update must not redirect an
			earlier point dispatch to the newest stream or surface binding.
		*/
		descriptor_clip.x1 = 0;
		descriptor_clip.y1 = 0;
		descriptor_clip.x2 = 18;
		descriptor_clip.y2 = 12;
		for (x = 0; x < 3; x++) {
			descriptor_points[x].x = 10;
			descriptor_points[x].y = (int32_t)x;
			descriptor_points[x].color = 0xAA000000u |
				((uint32_t)x * 0x00010101u);
			descriptor_points[x].flags = 0;
			result = fb_gfx3_vulkan_surface_points(&runtime, &surface,
				&descriptor_clip, &descriptor_points[x], 1);
			if (result != FB_GFX3_OK) {
				fprintf(stderr, "Vulkan descriptor point dispatch failed: %d\n",
					result);
				return 22;
			}
			expected_surface[x][10] = descriptor_points[x].color;
		}
		memset(surface_download, 0xCD, sizeof(surface_download));
		result = fb_gfx3_vulkan_surface_download(&runtime, &surface,
			0, 0, 19, 13, surface_download,
			sizeof(surface_download[0]));
		if (result != FB_GFX3_OK) {
			fprintf(stderr, "Vulkan surface download failed: %d\n", result);
			return 12;
		}
		for (y = 0; y < 13; y++) {
			for (x = 0; x < 19; x++) {
				uint32_t actual;

				memcpy(&actual, &surface_download[y][x * 4], 4);
				if (actual != expected_surface[y][x]) {
					fprintf(stderr,
						"Vulkan surface mismatch at %zu,%zu\n",
						x, y);
					return 13;
				}
			}
			for (x = 19 * 4; x < sizeof(surface_download[0]); x++) {
				if (surface_download[y][x] != 0xCD) {
					fprintf(stderr,
						"Vulkan download overwrote row padding\n");
					return 14;
				}
			}
		}
		fb_gfx3_vulkan_surface_destroy(&runtime, &surface);
		if (surface.implementation != NULL) {
			fprintf(stderr, "Vulkan surface destroy retained state\n");
			return 15;
		}
		if (runtime.completed_submission_count != 13) {
			fprintf(stderr, "Vulkan submission count is incorrect\n");
			return 16;
		}
		fb_gfx3_vulkan_runtime_close(&runtime);
		if (runtime.initialized || (runtime.implementation != NULL) ||
		    (runtime.completed_submission_count != 0)) {
			fprintf(stderr, "Vulkan close retained published state\n");
			return 17;
		}
	}
	return 0;
}

/* end of vulkan-bootstrap.c */
