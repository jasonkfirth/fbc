/*
    Project: FreeBASIC gfxlib3 tests
    --------------------------------

    File: vulkan-adapter-ranking.c

    Purpose:

        Verify gfxlib3's deterministic Vulkan physical-device preference
        policy without requiring a particular adapter configuration.

    Responsibilities:

        - preserve the Float64 exact-ellipse preference
        - rank discrete, integrated, virtual, CPU, and other adapters
        - prefer a graphics-and-compute queue over compute-only

    This file intentionally does NOT contain:

        - Vulkan loader initialization
        - logical-device creation
        - presentation tests
*/

#include "../../src/gfxlib3/gfx3_vulkan.h"

#include <stdio.h>

static int failures = 0;

#define CHECK(expression) \
	do { \
		if (!(expression)) { \
			fprintf(stderr, "failed: %s at line %d\n", #expression, \
				__LINE__); \
			failures++; \
		} \
	} while (0)

int main(void)
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

	if (failures != 0)
		return 1;
	puts("gfxlib3 Vulkan adapter ranking: all checks passed");
	return 0;
}

/* end of vulkan-adapter-ranking.c */
