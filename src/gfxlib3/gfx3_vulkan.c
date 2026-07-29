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
        - own reusable command, fence, descriptor, and compute-pipeline state
        - allocate device-local logical pixel surfaces and staging buffers
        - execute synchronized transfers, clears, points, lines, and boxes
        - convert logical pages on the GPU and copy them to a native swapchain
        - destroy every acquired object in reverse ownership order

    This file intentionally does NOT contain:

        - native window creation or message dispatch
        - FreeBASIC compatibility or common renderer command dispatch
*/

#include "gfx3_vulkan.h"
#include "gfx3_presentation.h"
#include "gfx3_vulkan_platform.h"

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

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID)

#define FB_GFX3_VK_CALL FBCALL

#include "gfx3_vulkan_shader.h"


/* ------------------------------------------------------------------------- */
/* Checked host-size arithmetic                                              */
/* ------------------------------------------------------------------------- */

static int vulkan_size_add(size_t left, size_t right, size_t *result)
{
	if ((result == NULL) || (left > (SIZE_MAX - right)))
		return FB_GFX3_INVALID;
	*result = left + right;
	return FB_GFX3_OK;
}

static int vulkan_size_multiply(size_t left, size_t right, size_t *result)
{
	if ((result == NULL) || ((right != 0) && (left > (SIZE_MAX / right))))
		return FB_GFX3_INVALID;
	*result = left * right;
	return FB_GFX3_OK;
}

typedef int32_t FB_GFX3_VK_RESULT;
typedef uint32_t FB_GFX3_VK_FLAGS;
typedef struct FB_GFX3_VK_INSTANCE_T *FB_GFX3_VK_INSTANCE;
typedef struct FB_GFX3_VK_PHYSICAL_DEVICE_T *FB_GFX3_VK_PHYSICAL_DEVICE;
typedef struct FB_GFX3_VK_DEVICE_T *FB_GFX3_VK_DEVICE;
typedef struct FB_GFX3_VK_QUEUE_T *FB_GFX3_VK_QUEUE;
typedef struct FB_GFX3_VK_COMMAND_BUFFER_T *FB_GFX3_VK_COMMAND_BUFFER;
typedef uint64_t FB_GFX3_VK_COMMAND_POOL;
typedef uint64_t FB_GFX3_VK_FENCE;
typedef uint64_t FB_GFX3_VK_BUFFER;
typedef uint64_t FB_GFX3_VK_DEVICE_MEMORY;
typedef uint64_t FB_GFX3_VK_SHADER_MODULE;
typedef uint64_t FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT;
typedef uint64_t FB_GFX3_VK_PIPELINE_LAYOUT;
typedef uint64_t FB_GFX3_VK_PIPELINE;
typedef uint64_t FB_GFX3_VK_PIPELINE_CACHE;
typedef uint64_t FB_GFX3_VK_DESCRIPTOR_POOL;
typedef uint64_t FB_GFX3_VK_DESCRIPTOR_SET;
typedef uint64_t FB_GFX3_VK_SURFACE;
typedef uint64_t FB_GFX3_VK_SWAPCHAIN;
typedef uint64_t FB_GFX3_VK_IMAGE;
typedef uint64_t FB_GFX3_VK_SEMAPHORE;

typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_VOID_FUNCTION)(void);
typedef FB_GFX3_VK_VOID_FUNCTION (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_INSTANCE_PROC_ADDR)(FB_GFX3_VK_INSTANCE instance,
	const char *name);
typedef FB_GFX3_VK_VOID_FUNCTION (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_DEVICE_PROC_ADDR)(FB_GFX3_VK_DEVICE device,
	const char *name);

typedef struct FB_GFX3_VK_APPLICATION_INFO {
	uint32_t structure_type;
	const void *next;
	const char *application_name;
	uint32_t application_version;
	const char *engine_name;
	uint32_t engine_version;
	uint32_t api_version;
} FB_GFX3_VK_APPLICATION_INFO;

typedef struct FB_GFX3_VK_INSTANCE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	const FB_GFX3_VK_APPLICATION_INFO *application_info;
	uint32_t enabled_layer_count;
	const char *const *enabled_layer_names;
	uint32_t enabled_extension_count;
	const char *const *enabled_extension_names;
} FB_GFX3_VK_INSTANCE_CREATE_INFO;

typedef struct FB_GFX3_VK_EXTENT_2D {
	uint32_t width;
	uint32_t height;
} FB_GFX3_VK_EXTENT_2D;

typedef struct FB_GFX3_VK_EXTENT_3D {
	uint32_t width;
	uint32_t height;
	uint32_t depth;
} FB_GFX3_VK_EXTENT_3D;

typedef struct FB_GFX3_VK_SURFACE_CAPABILITIES {
	uint32_t minimum_image_count;
	uint32_t maximum_image_count;
	FB_GFX3_VK_EXTENT_2D current_extent;
	FB_GFX3_VK_EXTENT_2D minimum_image_extent;
	FB_GFX3_VK_EXTENT_2D maximum_image_extent;
	uint32_t maximum_image_array_layers;
	FB_GFX3_VK_FLAGS supported_transforms;
	uint32_t current_transform;
	FB_GFX3_VK_FLAGS supported_composite_alpha;
	FB_GFX3_VK_FLAGS supported_usage_flags;
} FB_GFX3_VK_SURFACE_CAPABILITIES;

typedef struct FB_GFX3_VK_SURFACE_FORMAT {
	uint32_t format;
	uint32_t color_space;
} FB_GFX3_VK_SURFACE_FORMAT;

typedef struct FB_GFX3_VK_SWAPCHAIN_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	FB_GFX3_VK_SURFACE surface;
	uint32_t minimum_image_count;
	uint32_t image_format;
	uint32_t image_color_space;
	FB_GFX3_VK_EXTENT_2D image_extent;
	uint32_t image_array_layers;
	FB_GFX3_VK_FLAGS image_usage;
	uint32_t image_sharing_mode;
	uint32_t queue_family_index_count;
	const uint32_t *queue_family_indices;
	uint32_t pre_transform;
	uint32_t composite_alpha;
	uint32_t present_mode;
	uint32_t clipped;
	FB_GFX3_VK_SWAPCHAIN old_swapchain;
} FB_GFX3_VK_SWAPCHAIN_CREATE_INFO;

typedef struct FB_GFX3_VK_SEMAPHORE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
} FB_GFX3_VK_SEMAPHORE_CREATE_INFO;

typedef struct FB_GFX3_VK_PRESENT_INFO {
	uint32_t structure_type;
	const void *next;
	uint32_t wait_semaphore_count;
	const FB_GFX3_VK_SEMAPHORE *wait_semaphores;
	uint32_t swapchain_count;
	const FB_GFX3_VK_SWAPCHAIN *swapchains;
	const uint32_t *image_indices;
	FB_GFX3_VK_RESULT *results;
} FB_GFX3_VK_PRESENT_INFO;

typedef struct FB_GFX3_VK_QUEUE_FAMILY_PROPERTIES {
	FB_GFX3_VK_FLAGS queue_flags;
	uint32_t queue_count;
	uint32_t timestamp_valid_bits;
	FB_GFX3_VK_EXTENT_3D minimum_image_transfer_granularity;
} FB_GFX3_VK_QUEUE_FAMILY_PROPERTIES;

typedef struct FB_GFX3_VK_DEVICE_QUEUE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint32_t queue_family_index;
	uint32_t queue_count;
	const float *queue_priorities;
} FB_GFX3_VK_DEVICE_QUEUE_CREATE_INFO;

typedef struct FB_GFX3_VK_DEVICE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint32_t queue_create_info_count;
	const FB_GFX3_VK_DEVICE_QUEUE_CREATE_INFO *queue_create_infos;
	uint32_t enabled_layer_count;
	const char *const *enabled_layer_names;
	uint32_t enabled_extension_count;
	const char *const *enabled_extension_names;
	const void *enabled_features;
} FB_GFX3_VK_DEVICE_CREATE_INFO;

/*
	Vulkan 1.0 exposes core device features as a fixed sequence of VkBool32
	fields. The complete layout is repeated here because passing a shortened
	structure to vkGetPhysicalDeviceFeatures would let the driver overwrite it.
*/
typedef struct FB_GFX3_VK_PHYSICAL_DEVICE_FEATURES {
	uint32_t robust_buffer_access;
	uint32_t full_draw_index_uint32;
	uint32_t image_cube_array;
	uint32_t independent_blend;
	uint32_t geometry_shader;
	uint32_t tessellation_shader;
	uint32_t sample_rate_shading;
	uint32_t dual_source_blend;
	uint32_t logic_operation;
	uint32_t multi_draw_indirect;
	uint32_t draw_indirect_first_instance;
	uint32_t depth_clamp;
	uint32_t depth_bias_clamp;
	uint32_t fill_mode_non_solid;
	uint32_t depth_bounds;
	uint32_t wide_lines;
	uint32_t large_points;
	uint32_t alpha_to_one;
	uint32_t multi_viewport;
	uint32_t sampler_anisotropy;
	uint32_t texture_compression_etc2;
	uint32_t texture_compression_astc_ldr;
	uint32_t texture_compression_bc;
	uint32_t occlusion_query_precise;
	uint32_t pipeline_statistics_query;
	uint32_t vertex_pipeline_stores_and_atomics;
	uint32_t fragment_stores_and_atomics;
	uint32_t shader_tessellation_and_geometry_point_size;
	uint32_t shader_image_gather_extended;
	uint32_t shader_storage_image_extended_formats;
	uint32_t shader_storage_image_multisample;
	uint32_t shader_storage_image_read_without_format;
	uint32_t shader_storage_image_write_without_format;
	uint32_t shader_uniform_buffer_array_dynamic_indexing;
	uint32_t shader_sampled_image_array_dynamic_indexing;
	uint32_t shader_storage_buffer_array_dynamic_indexing;
	uint32_t shader_storage_image_array_dynamic_indexing;
	uint32_t shader_clip_distance;
	uint32_t shader_cull_distance;
	uint32_t shader_float64;
	uint32_t shader_int64;
	uint32_t shader_int16;
	uint32_t shader_resource_residency;
	uint32_t shader_resource_minimum_lod;
	uint32_t sparse_binding;
	uint32_t sparse_residency_buffer;
	uint32_t sparse_residency_image_2d;
	uint32_t sparse_residency_image_3d;
	uint32_t sparse_residency_2_samples;
	uint32_t sparse_residency_4_samples;
	uint32_t sparse_residency_8_samples;
	uint32_t sparse_residency_16_samples;
	uint32_t sparse_residency_aliased;
	uint32_t variable_multisample_rate;
	uint32_t inherited_queries;
} FB_GFX3_VK_PHYSICAL_DEVICE_FEATURES;

/*
	vkGetPhysicalDeviceProperties writes the complete Vulkan 1.0 structure.
	gfxlib3 needs the adapter identity and the early portion of
	VkPhysicalDeviceLimits containing maxStorageBufferRange. Surfaces are
	storage buffers, not textures, so maxImageDimension2D is not their limit.

	Keep these fields in Vulkan 1.0 declaration order and retain the generously
	aligned, full-size scratch object below. Drivers are entitled to write the
	complete properties object even though gfxlib3 reads only this prefix.
*/
typedef struct FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_PREFIX {
	uint32_t api_version;
	uint32_t driver_version;
	uint32_t vendor_id;
	uint32_t device_id;
	uint32_t device_type;
	char device_name[256];
	uint8_t pipeline_cache_uuid[16];
	/*
		VkPhysicalDeviceLimits contains VkDeviceSize members and is eight-byte
		aligned. VkPhysicalDeviceProperties therefore inserts four bytes after
		the UUID before this nested structure.
	*/
	uint32_t limits_alignment_padding;
	uint32_t max_image_dimension_1d;
	uint32_t max_image_dimension_2d;
	uint32_t max_image_dimension_3d;
	uint32_t max_image_dimension_cube;
	uint32_t max_image_array_layers;
	uint32_t max_texel_buffer_elements;
	uint32_t max_uniform_buffer_range;
	uint32_t max_storage_buffer_range;
} FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_PREFIX;

#define FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_BYTES 4096u

_Static_assert(offsetof(FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_PREFIX,
	max_storage_buffer_range) == 324u,
	"Vulkan physical-device limits prefix has the wrong layout");

typedef union FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_STORAGE {
	uint64_t alignment;
	uint8_t bytes[FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_BYTES];
} FB_GFX3_VK_PHYSICAL_DEVICE_PROPERTIES_STORAGE;

typedef struct FB_GFX3_VK_COMMAND_POOL_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint32_t queue_family_index;
} FB_GFX3_VK_COMMAND_POOL_CREATE_INFO;

typedef struct FB_GFX3_VK_COMMAND_BUFFER_ALLOCATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_COMMAND_POOL command_pool;
	uint32_t level;
	uint32_t command_buffer_count;
} FB_GFX3_VK_COMMAND_BUFFER_ALLOCATE_INFO;

typedef struct FB_GFX3_VK_COMMAND_BUFFER_BEGIN_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	const void *inheritance_info;
} FB_GFX3_VK_COMMAND_BUFFER_BEGIN_INFO;

typedef struct FB_GFX3_VK_FENCE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
} FB_GFX3_VK_FENCE_CREATE_INFO;

typedef struct FB_GFX3_VK_SUBMIT_INFO {
	uint32_t structure_type;
	const void *next;
	uint32_t wait_semaphore_count;
	const uint64_t *wait_semaphores;
	const FB_GFX3_VK_FLAGS *wait_stage_masks;
	uint32_t command_buffer_count;
	const FB_GFX3_VK_COMMAND_BUFFER *command_buffers;
	uint32_t signal_semaphore_count;
	const uint64_t *signal_semaphores;
} FB_GFX3_VK_SUBMIT_INFO;

typedef struct FB_GFX3_VK_BUFFER_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint64_t size;
	FB_GFX3_VK_FLAGS usage;
	uint32_t sharing_mode;
	uint32_t queue_family_index_count;
	const uint32_t *queue_family_indices;
} FB_GFX3_VK_BUFFER_CREATE_INFO;

typedef struct FB_GFX3_VK_MEMORY_REQUIREMENTS {
	uint64_t size;
	uint64_t alignment;
	uint32_t memory_type_bits;
} FB_GFX3_VK_MEMORY_REQUIREMENTS;

typedef struct FB_GFX3_VK_MEMORY_TYPE {
	FB_GFX3_VK_FLAGS property_flags;
	uint32_t heap_index;
} FB_GFX3_VK_MEMORY_TYPE;

typedef struct FB_GFX3_VK_MEMORY_HEAP {
	uint64_t size;
	FB_GFX3_VK_FLAGS flags;
} FB_GFX3_VK_MEMORY_HEAP;

typedef struct FB_GFX3_VK_PHYSICAL_DEVICE_MEMORY_PROPERTIES {
	uint32_t memory_type_count;
	FB_GFX3_VK_MEMORY_TYPE memory_types[32];
	uint32_t memory_heap_count;
	FB_GFX3_VK_MEMORY_HEAP memory_heaps[16];
} FB_GFX3_VK_PHYSICAL_DEVICE_MEMORY_PROPERTIES;

typedef struct FB_GFX3_VK_MEMORY_ALLOCATE_INFO {
	uint32_t structure_type;
	const void *next;
	uint64_t allocation_size;
	uint32_t memory_type_index;
} FB_GFX3_VK_MEMORY_ALLOCATE_INFO;

typedef struct FB_GFX3_VK_MEMORY_BARRIER {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS source_access_mask;
	FB_GFX3_VK_FLAGS destination_access_mask;
} FB_GFX3_VK_MEMORY_BARRIER;

typedef struct FB_GFX3_VK_BUFFER_MEMORY_BARRIER {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS source_access_mask;
	FB_GFX3_VK_FLAGS destination_access_mask;
	uint32_t source_queue_family_index;
	uint32_t destination_queue_family_index;
	FB_GFX3_VK_BUFFER buffer;
	uint64_t offset;
	uint64_t size;
} FB_GFX3_VK_BUFFER_MEMORY_BARRIER;

typedef struct FB_GFX3_VK_IMAGE_SUBRESOURCE_RANGE {
	FB_GFX3_VK_FLAGS aspect_mask;
	uint32_t base_mip_level;
	uint32_t level_count;
	uint32_t base_array_layer;
	uint32_t layer_count;
} FB_GFX3_VK_IMAGE_SUBRESOURCE_RANGE;

typedef struct FB_GFX3_VK_IMAGE_MEMORY_BARRIER {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS source_access_mask;
	FB_GFX3_VK_FLAGS destination_access_mask;
	uint32_t old_layout;
	uint32_t new_layout;
	uint32_t source_queue_family_index;
	uint32_t destination_queue_family_index;
	FB_GFX3_VK_IMAGE image;
	FB_GFX3_VK_IMAGE_SUBRESOURCE_RANGE subresource_range;
} FB_GFX3_VK_IMAGE_MEMORY_BARRIER;

typedef struct FB_GFX3_VK_IMAGE_SUBRESOURCE_LAYERS {
	FB_GFX3_VK_FLAGS aspect_mask;
	uint32_t mip_level;
	uint32_t base_array_layer;
	uint32_t layer_count;
} FB_GFX3_VK_IMAGE_SUBRESOURCE_LAYERS;

typedef struct FB_GFX3_VK_OFFSET_3D {
	int32_t x;
	int32_t y;
	int32_t z;
} FB_GFX3_VK_OFFSET_3D;

typedef struct FB_GFX3_VK_BUFFER_IMAGE_COPY {
	uint64_t buffer_offset;
	uint32_t buffer_row_length;
	uint32_t buffer_image_height;
	FB_GFX3_VK_IMAGE_SUBRESOURCE_LAYERS image_subresource;
	FB_GFX3_VK_OFFSET_3D image_offset;
	FB_GFX3_VK_EXTENT_3D image_extent;
} FB_GFX3_VK_BUFFER_IMAGE_COPY;

typedef struct FB_GFX3_VK_SHADER_MODULE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	size_t code_size;
	const uint32_t *code;
} FB_GFX3_VK_SHADER_MODULE_CREATE_INFO;

typedef struct FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_BINDING {
	uint32_t binding;
	uint32_t descriptor_type;
	uint32_t descriptor_count;
	FB_GFX3_VK_FLAGS stage_flags;
	const uint64_t *immutable_samplers;
} FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_BINDING;

typedef struct FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint32_t binding_count;
	const FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_BINDING *bindings;
} FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

typedef struct FB_GFX3_VK_PIPELINE_LAYOUT_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint32_t set_layout_count;
	const FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT *set_layouts;
	uint32_t push_constant_range_count;
	const void *push_constant_ranges;
} FB_GFX3_VK_PIPELINE_LAYOUT_CREATE_INFO;

typedef struct FB_GFX3_VK_PIPELINE_SHADER_STAGE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	FB_GFX3_VK_FLAGS stage;
	FB_GFX3_VK_SHADER_MODULE module;
	const char *name;
	const void *specialization_info;
} FB_GFX3_VK_PIPELINE_SHADER_STAGE_CREATE_INFO;

typedef struct FB_GFX3_VK_COMPUTE_PIPELINE_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	FB_GFX3_VK_PIPELINE_SHADER_STAGE_CREATE_INFO stage;
	FB_GFX3_VK_PIPELINE_LAYOUT layout;
	FB_GFX3_VK_PIPELINE base_pipeline_handle;
	int32_t base_pipeline_index;
} FB_GFX3_VK_COMPUTE_PIPELINE_CREATE_INFO;

typedef struct FB_GFX3_VK_DESCRIPTOR_POOL_SIZE {
	uint32_t type;
	uint32_t descriptor_count;
} FB_GFX3_VK_DESCRIPTOR_POOL_SIZE;

typedef struct FB_GFX3_VK_DESCRIPTOR_POOL_CREATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_FLAGS flags;
	uint32_t maximum_sets;
	uint32_t pool_size_count;
	const FB_GFX3_VK_DESCRIPTOR_POOL_SIZE *pool_sizes;
} FB_GFX3_VK_DESCRIPTOR_POOL_CREATE_INFO;

typedef struct FB_GFX3_VK_DESCRIPTOR_SET_ALLOCATE_INFO {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_DESCRIPTOR_POOL descriptor_pool;
	uint32_t descriptor_set_count;
	const FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT *set_layouts;
} FB_GFX3_VK_DESCRIPTOR_SET_ALLOCATE_INFO;

typedef struct FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO {
	FB_GFX3_VK_BUFFER buffer;
	uint64_t offset;
	uint64_t range;
} FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO;

typedef struct FB_GFX3_VK_WRITE_DESCRIPTOR_SET {
	uint32_t structure_type;
	const void *next;
	FB_GFX3_VK_DESCRIPTOR_SET destination_set;
	uint32_t destination_binding;
	uint32_t destination_array_element;
	uint32_t descriptor_count;
	uint32_t descriptor_type;
	const void *image_info;
	const FB_GFX3_VK_DESCRIPTOR_BUFFER_INFO *buffer_info;
	const uint64_t *texel_buffer_view;
} FB_GFX3_VK_WRITE_DESCRIPTOR_SET;

typedef struct FB_GFX3_VK_BUFFER_COPY {
	uint64_t source_offset;
	uint64_t destination_offset;
	uint64_t size;
} FB_GFX3_VK_BUFFER_COPY;

typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_ENUMERATE_INSTANCE_VERSION)(uint32_t *api_version);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_CREATE_INSTANCE)(
	const FB_GFX3_VK_INSTANCE_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_INSTANCE *instance);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_INSTANCE)(
	FB_GFX3_VK_INSTANCE instance, const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_PLATFORM_SURFACE)(FB_GFX3_VK_INSTANCE instance,
	const void *create_info,
	const void *allocator, FB_GFX3_VK_SURFACE *surface);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_SURFACE)(
	FB_GFX3_VK_INSTANCE instance, FB_GFX3_VK_SURFACE surface,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_ENUMERATE_PHYSICAL_DEVICES)(FB_GFX3_VK_INSTANCE instance,
	uint32_t *count, FB_GFX3_VK_PHYSICAL_DEVICE *devices);
typedef void (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_QUEUE_FAMILY_PROPERTIES)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, uint32_t *count,
	FB_GFX3_VK_QUEUE_FAMILY_PROPERTIES *properties);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_GET_PHYSICAL_DEVICE_FEATURES)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device,
	FB_GFX3_VK_PHYSICAL_DEVICE_FEATURES *features);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_GET_PHYSICAL_DEVICE_PROPERTIES)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, void *properties);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_SUPPORT)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, uint32_t queue_family_index,
	FB_GFX3_VK_SURFACE surface, uint32_t *supported);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, FB_GFX3_VK_SURFACE surface,
	FB_GFX3_VK_SURFACE_CAPABILITIES *capabilities);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_FORMATS)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, FB_GFX3_VK_SURFACE surface,
	uint32_t *count, FB_GFX3_VK_SURFACE_FORMAT *formats);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device, FB_GFX3_VK_SURFACE surface,
	uint32_t *count, uint32_t *present_modes);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_CREATE_DEVICE)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device,
	const FB_GFX3_VK_DEVICE_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_DEVICE *device);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_DEVICE)(
	FB_GFX3_VK_DEVICE device, const void *allocator);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_GET_DEVICE_QUEUE)(
	FB_GFX3_VK_DEVICE device, uint32_t queue_family_index,
	uint32_t queue_index, FB_GFX3_VK_QUEUE *queue);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_COMMAND_POOL)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_COMMAND_POOL_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_COMMAND_POOL *command_pool);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_COMMAND_POOL)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_COMMAND_POOL command_pool,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_ALLOCATE_COMMAND_BUFFERS)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_COMMAND_BUFFER_ALLOCATE_INFO *allocate_info,
	FB_GFX3_VK_COMMAND_BUFFER *command_buffers);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_BEGIN_COMMAND_BUFFER)(FB_GFX3_VK_COMMAND_BUFFER command_buffer,
	const FB_GFX3_VK_COMMAND_BUFFER_BEGIN_INFO *begin_info);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_END_COMMAND_BUFFER)(FB_GFX3_VK_COMMAND_BUFFER command_buffer);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_CREATE_FENCE)(
	FB_GFX3_VK_DEVICE device, const FB_GFX3_VK_FENCE_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_FENCE *fence);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_FENCE)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_FENCE fence,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_WAIT_FOR_FENCES)(
	FB_GFX3_VK_DEVICE device, uint32_t fence_count,
	const FB_GFX3_VK_FENCE *fences, uint32_t wait_all, uint64_t timeout);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_GET_FENCE_STATUS)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_FENCE fence);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_RESET_FENCES)(
	FB_GFX3_VK_DEVICE device, uint32_t fence_count,
	const FB_GFX3_VK_FENCE *fences);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_RESET_COMMAND_POOL)(FB_GFX3_VK_DEVICE device,
	FB_GFX3_VK_COMMAND_POOL command_pool, FB_GFX3_VK_FLAGS flags);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_QUEUE_SUBMIT)(
	FB_GFX3_VK_QUEUE queue, uint32_t submit_count,
	const FB_GFX3_VK_SUBMIT_INFO *submits, FB_GFX3_VK_FENCE fence);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_DEVICE_WAIT_IDLE)(
	FB_GFX3_VK_DEVICE device);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_CREATE_SWAPCHAIN)(
	FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_SWAPCHAIN_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_SWAPCHAIN *swapchain);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_SWAPCHAIN)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_SWAPCHAIN swapchain,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_SWAPCHAIN_IMAGES)(FB_GFX3_VK_DEVICE device,
	FB_GFX3_VK_SWAPCHAIN swapchain, uint32_t *count,
	FB_GFX3_VK_IMAGE *images);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_ACQUIRE_NEXT_IMAGE)(FB_GFX3_VK_DEVICE device,
	FB_GFX3_VK_SWAPCHAIN swapchain, uint64_t timeout,
	FB_GFX3_VK_SEMAPHORE semaphore, FB_GFX3_VK_FENCE fence,
	uint32_t *image_index);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_QUEUE_PRESENT)(
	FB_GFX3_VK_QUEUE queue, const FB_GFX3_VK_PRESENT_INFO *present_info);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_CREATE_SEMAPHORE)(
	FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_SEMAPHORE_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_SEMAPHORE *semaphore);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_SEMAPHORE)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_SEMAPHORE semaphore,
	const void *allocator);
typedef void (FB_GFX3_VK_CALL
	*FB_GFX3_VK_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES)(
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device,
	FB_GFX3_VK_PHYSICAL_DEVICE_MEMORY_PROPERTIES *properties);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_CREATE_BUFFER)(
	FB_GFX3_VK_DEVICE device, const FB_GFX3_VK_BUFFER_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_BUFFER *buffer);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_BUFFER)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_BUFFER buffer,
	const void *allocator);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_GET_BUFFER_MEMORY_REQUIREMENTS)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_BUFFER buffer,
	FB_GFX3_VK_MEMORY_REQUIREMENTS *requirements);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_ALLOCATE_MEMORY)(
	FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_MEMORY_ALLOCATE_INFO *allocate_info,
	const void *allocator, FB_GFX3_VK_DEVICE_MEMORY *memory);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_FREE_MEMORY)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_DEVICE_MEMORY memory,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_BIND_BUFFER_MEMORY)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_BUFFER buffer,
	FB_GFX3_VK_DEVICE_MEMORY memory, uint64_t memory_offset);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL *FB_GFX3_VK_MAP_MEMORY)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_DEVICE_MEMORY memory,
	uint64_t offset, uint64_t size, FB_GFX3_VK_FLAGS flags, void **data);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_UNMAP_MEMORY)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_DEVICE_MEMORY memory);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_FILL_BUFFER)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer, FB_GFX3_VK_BUFFER buffer,
	uint64_t destination_offset, uint64_t size, uint32_t data);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_COPY_BUFFER)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer,
	FB_GFX3_VK_BUFFER source_buffer, FB_GFX3_VK_BUFFER destination_buffer,
	uint32_t region_count, const FB_GFX3_VK_BUFFER_COPY *regions);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_COPY_BUFFER_TO_IMAGE)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer,
	FB_GFX3_VK_BUFFER source_buffer, FB_GFX3_VK_IMAGE destination_image,
	uint32_t destination_layout, uint32_t region_count,
	const FB_GFX3_VK_BUFFER_IMAGE_COPY *regions);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_PIPELINE_BARRIER)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer,
	FB_GFX3_VK_FLAGS source_stage_mask,
	FB_GFX3_VK_FLAGS destination_stage_mask,
	FB_GFX3_VK_FLAGS dependency_flags, uint32_t memory_barrier_count,
	const void *memory_barriers, uint32_t buffer_memory_barrier_count,
	const FB_GFX3_VK_BUFFER_MEMORY_BARRIER *buffer_memory_barriers,
	uint32_t image_memory_barrier_count,
	const FB_GFX3_VK_IMAGE_MEMORY_BARRIER *image_memory_barriers);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_SHADER_MODULE)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_SHADER_MODULE_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_SHADER_MODULE *shader_module);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_SHADER_MODULE)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_SHADER_MODULE shader_module,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_DESCRIPTOR_SET_LAYOUT)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT *set_layout);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_DESCRIPTOR_SET_LAYOUT)(
	FB_GFX3_VK_DEVICE device,
	FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT set_layout, const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_PIPELINE_LAYOUT)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_PIPELINE_LAYOUT_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_PIPELINE_LAYOUT *pipeline_layout);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_PIPELINE_LAYOUT)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_PIPELINE_LAYOUT pipeline_layout,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_COMPUTE_PIPELINES)(FB_GFX3_VK_DEVICE device,
	FB_GFX3_VK_PIPELINE_CACHE pipeline_cache, uint32_t create_info_count,
	const FB_GFX3_VK_COMPUTE_PIPELINE_CREATE_INFO *create_infos,
	const void *allocator, FB_GFX3_VK_PIPELINE *pipelines);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_PIPELINE)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_PIPELINE pipeline,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_CREATE_DESCRIPTOR_POOL)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_DESCRIPTOR_POOL_CREATE_INFO *create_info,
	const void *allocator, FB_GFX3_VK_DESCRIPTOR_POOL *descriptor_pool);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_DESTROY_DESCRIPTOR_POOL)(
	FB_GFX3_VK_DEVICE device, FB_GFX3_VK_DESCRIPTOR_POOL descriptor_pool,
	const void *allocator);
typedef FB_GFX3_VK_RESULT (FB_GFX3_VK_CALL
	*FB_GFX3_VK_ALLOCATE_DESCRIPTOR_SETS)(FB_GFX3_VK_DEVICE device,
	const FB_GFX3_VK_DESCRIPTOR_SET_ALLOCATE_INFO *allocate_info,
	FB_GFX3_VK_DESCRIPTOR_SET *descriptor_sets);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_UPDATE_DESCRIPTOR_SETS)(
	FB_GFX3_VK_DEVICE device, uint32_t descriptor_write_count,
	const FB_GFX3_VK_WRITE_DESCRIPTOR_SET *descriptor_writes,
	uint32_t descriptor_copy_count, const void *descriptor_copies);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_BIND_PIPELINE)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer, uint32_t bind_point,
	FB_GFX3_VK_PIPELINE pipeline);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_BIND_DESCRIPTOR_SETS)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer, uint32_t bind_point,
	FB_GFX3_VK_PIPELINE_LAYOUT layout, uint32_t first_set,
	uint32_t descriptor_set_count,
	const FB_GFX3_VK_DESCRIPTOR_SET *descriptor_sets,
	uint32_t dynamic_offset_count, const uint32_t *dynamic_offsets);
typedef void (FB_GFX3_VK_CALL *FB_GFX3_VK_CMD_DISPATCH)(
	FB_GFX3_VK_COMMAND_BUFFER command_buffer, uint32_t group_count_x,
	uint32_t group_count_y, uint32_t group_count_z);

enum FB_GFX3_VK_CONSTANT {
	FB_GFX3_VK_SUCCESS = 0,
	FB_GFX3_VK_NOT_READY = 1,
	FB_GFX3_VK_INCOMPLETE = 5,
	FB_GFX3_VK_SUBOPTIMAL = 1000001003,
	FB_GFX3_VK_ERROR_OUT_OF_DATE = -1000001004,
	FB_GFX3_VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
	FB_GFX3_VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
	FB_GFX3_VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
	FB_GFX3_VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
	FB_GFX3_VK_STRUCTURE_TYPE_SUBMIT_INFO = 4,
	FB_GFX3_VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
	FB_GFX3_VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8,
	FB_GFX3_VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO = 9,
	FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
	FB_GFX3_VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO = 16,
	FB_GFX3_VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO = 18,
	FB_GFX3_VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO = 28,
	FB_GFX3_VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO = 30,
	FB_GFX3_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32,
	FB_GFX3_VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO = 33,
	FB_GFX3_VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO = 34,
	FB_GFX3_VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET = 35,
	FB_GFX3_VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39,
	FB_GFX3_VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40,
	FB_GFX3_VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42,
	FB_GFX3_VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER = 44,
	FB_GFX3_VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 45,
	FB_GFX3_VK_STRUCTURE_TYPE_MEMORY_BARRIER = 46,
	FB_GFX3_VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO = 1000001000,
	FB_GFX3_VK_STRUCTURE_TYPE_PRESENT_INFO = 1000001001,
	FB_GFX3_VK_QUEUE_GRAPHICS_BIT = FB_GFX3_VULKAN_QUEUE_GRAPHICS,
	FB_GFX3_VK_QUEUE_COMPUTE_BIT = FB_GFX3_VULKAN_QUEUE_COMPUTE,
	FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002u,
	FB_GFX3_VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001u,
	FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020u,
	FB_GFX3_VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002u,
	FB_GFX3_VK_SHARING_MODE_EXCLUSIVE = 0,
	FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001u,
	FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002u,
	FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004u,
	FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT = 0x00001000u,
	FB_GFX3_VK_ACCESS_HOST_READ_BIT = 0x00002000u,
	FB_GFX3_VK_ACCESS_HOST_WRITE_BIT = 0x00004000u,
	FB_GFX3_VK_ACCESS_SHADER_READ_BIT = 0x00000020u,
	FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT = 0x00000040u,
	FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT = 0x00000800u,
	FB_GFX3_VK_ACCESS_MEMORY_READ_BIT = 0x00008000u,
	FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT = 0x00010000u,
	FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000u,
	FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT = 0x00004000u,
	FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800u,
	FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT = 0x00010000u,
	FB_GFX3_VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001u,
	FB_GFX3_VK_IMAGE_LAYOUT_UNDEFINED = 0,
	FB_GFX3_VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
	FB_GFX3_VK_IMAGE_LAYOUT_PRESENT_SOURCE = 1000001002,
	FB_GFX3_VK_FORMAT_B8G8R8A8_UNORM = 44,
	FB_GFX3_VK_COLOR_SPACE_SRGB_NONLINEAR = 0,
	FB_GFX3_VK_COMPOSITE_ALPHA_OPAQUE_BIT = 0x00000001u,
	FB_GFX3_VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT = 0x00000002u,
	FB_GFX3_VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT = 0x00000004u,
	FB_GFX3_VK_COMPOSITE_ALPHA_INHERIT_BIT = 0x00000008u,
	FB_GFX3_VK_PRESENT_MODE_IMMEDIATE = 0,
	FB_GFX3_VK_PRESENT_MODE_MAILBOX = 1,
	FB_GFX3_VK_PRESENT_MODE_FIFO = 2,
	FB_GFX3_VK_SHADER_STAGE_COMPUTE_BIT = 0x00000020u,
	FB_GFX3_VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
	FB_GFX3_VK_PIPELINE_BIND_POINT_COMPUTE = 1,
	FB_GFX3_VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0,
	FB_GFX3_VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001u,
	FB_GFX3_VK_FENCE_CREATE_SIGNALED_BIT = 0x00000001u,
	FB_GFX3_VK_API_VERSION_1_0 = 1u << 22,
	FB_GFX3_VK_ENUMERATION_ATTEMPTS = 4,
	/*
		Vulkan 1.0 requires maxStorageBufferRange to be at least 128 MiB. This
		fallback is used only if a loader omits the core properties function.
	*/
	FB_GFX3_VK_MINIMUM_STORAGE_BUFFER_RANGE = 128u * 1024u * 1024u,
	/*
		Two three-operation batches retain independent descriptors and mapped
		parameters. The renderer can record the next batch while the GPU consumes
		the previous one instead of waiting at every slot-ring wrap.
	*/
	/*
		Most BASIC animation loops issue a clear, text packet, and several
		primitive packets before the page-flip completion boundary. Eight slots
		let that common frame become one Vulkan queue submission while retaining
		a distinct descriptor and parameter area for every recorded operation.
	*/
	FB_GFX3_VK_SUBMISSION_SLOT_COUNT = 8,
	FB_GFX3_VK_SUBMISSION_BATCH_LIMIT = 8,
	FB_GFX3_VK_DEFERRED_ALLOCATION_LIMIT = 256,
	/*
		One submission records this many strictly ordered PUT dispatches. The
		renderer producer queue is bounded at 1,024 commands, so matching it
		allows a complete compatible sprite frame to use one descriptor update
		and one queue submission rather than four partial batches.
	*/
	FB_GFX3_VK_BLIT_BATCH_LIMIT = 1024,
	/* The tiled path uses one descriptor set for as many as eight packets. */
	FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT = 8192,
	/* Keep each tile's ordered replay list bounded under heavy overdraw. */
	FB_GFX3_VK_GLYPH_BATCH_LIMIT = 8191,
	FB_GFX3_VK_GLYPH_TILE_SIZE = 16,
	/* Winner tags reserve zero and therefore expose 1..8191 FIFO entries. */
	FB_GFX3_VK_PRIMITIVE_BATCH_LIMIT = 8191,
	/*
		Vulkan 1.0 guarantees 65,535 workgroups in each dispatch dimension.
		The primitive work table contains only useful 64-pixel chunks.
	*/
	FB_GFX3_VK_PRIMITIVE_WORKGROUP_LIMIT = 65535,
	/* One same-colour rectangle compute dispatch per renderer queue drain. */
	FB_GFX3_VK_RECTANGLE_COMPUTE_BATCH_LIMIT = 1024,
	/* Irregular topology keeps one workgroup, so bound its watchdog exposure. */
	FB_GFX3_VK_PAINT_MAX_PIXELS = 1048576,
	FB_GFX3_VK_PAINT_LOCAL_SIZE_X = 16,
	FB_GFX3_VK_PAINT_LOCAL_SIZE_Y = 8,
	/* phase, rectangular-valid flag, and four inclusive bounds */
	FB_GFX3_VK_PAINT_METADATA_WORDS = 6
};

typedef struct FB_GFX3_VULKAN_BUFFER_ALLOCATION {
	FB_GFX3_VK_BUFFER buffer;
	FB_GFX3_VK_DEVICE_MEMORY memory;
	void *mapped;
	uint64_t size;
} FB_GFX3_VULKAN_BUFFER_ALLOCATION;

typedef struct FB_GFX3_VULKAN_SUBMISSION_SLOT {
	FB_GFX3_VK_COMMAND_POOL command_pool;
	FB_GFX3_VK_COMMAND_BUFFER command_buffer;
	FB_GFX3_VK_FENCE fence;
	FB_GFX3_VK_SEMAPHORE image_available;
	FB_GFX3_VK_DESCRIPTOR_SET descriptor_sets[FB_GFX3_VK_BLIT_BATCH_LIMIT];
	/*
		PUT command records are rewritten only after this slot's fence signals.
		Keeping this host-visible buffer with the slot avoids a Vulkan buffer and
		memory allocation for every compatible sprite batch.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION blit_command_buffer;
	/*
		Destination-independent PUT batches reuse this device-local winner image.
		The slot fence owns it until the corresponding resolve pass completes.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION blit_winner_buffer;
	/*
		Tiled sprite batches rewrite these host-visible lists only after the
		slot fence signals. Keeping them with the slot avoids two Vulkan memory
		allocations for every frame containing a sprite packet.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION blit_range_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION blit_index_buffer;
	/*
		Mixed primitive winner and command storage follows the same ownership
		rule as sprite batching: the slot fence must signal before either buffer
		is rewritten. The generation avoids clearing the winner surface for
		every small primitive packet.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION primitive_command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION primitive_winner_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION primitive_workgroup_buffer;
	uint32_t primitive_generation;
	/*
		Point packets are frequent in software-style text and particle loops.
		The slot fence protects this mapped storage until every point dispatch
		which references it has completed.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION point_command_buffer;
	/*
		Rectangle and mixed-primitive tile packets share compact tile
		coordinates, ranges, and command indices. Their command records remain
		in the type-specific buffers above. Tile storage is separate from blit
		storage because primitives and sprites can be recorded into the same
		in-flight submission.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION rectangle_command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION rectangle_range_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION rectangle_index_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION rectangle_tile_buffer;
	/* Transform records are larger than ordinary PUT records and stay separate. */
	FB_GFX3_VULKAN_BUFFER_ALLOCATION transform_command_buffer;
	/*
		Glyph packets use three persistent host-visible buffers per in-flight
		slot. Reusing them removes VkBuffer and VkDeviceMemory allocation from
		the normal DRAW STRING and console paths.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION glyph_command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION glyph_range_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION glyph_index_buffer;
	/* PAINT scratch is private to the slot until its submission fence signals. */
	FB_GFX3_VULKAN_BUFFER_ALLOCATION paint_command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION paint_scratch_buffer;
	/*
		Presentation parameters are host-written immediately before submission.
		Each frame-in-flight slot therefore owns its own mapped buffer. Sharing
		one buffer would let the CPU overwrite parameters still being consumed
		by an older GPU submission.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION present_command_buffer;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION
		deferred_allocations[FB_GFX3_VK_DEFERRED_ALLOCATION_LIMIT];
	size_t deferred_allocation_count;
	/* Inclusive renderer command-sequence range represented by this slot. */
	uint64_t first_command_sequence;
	uint64_t command_sequence;
	/* Monotonic runtime submission order, independent of renderer sequences. */
	uint64_t submission_serial;
	/* A batched renderer drain may share one fence across several slots. */
	FB_GFX3_VK_FENCE submission_fence;
	int pending_submission;
	int submitted;
} FB_GFX3_VULKAN_SUBMISSION_SLOT;

typedef struct FB_GFX3_VULKAN_IMPLEMENTATION {
	FB_GFX3_VULKAN_LIBRARY library;
	FB_GFX3_VK_INSTANCE instance;
	FB_GFX3_VK_SURFACE surface;
	FB_GFX3_VK_PHYSICAL_DEVICE physical_device;
	FB_GFX3_VK_DEVICE device;
	FB_GFX3_VK_QUEUE queue;
	/* Exact VkPhysicalDeviceLimits::maxStorageBufferRange for this adapter. */
	uint64_t maximum_storage_buffer_range;
	FB_GFX3_VK_SWAPCHAIN swapchain;
	FB_GFX3_VK_IMAGE *swapchain_images;
	unsigned char *swapchain_image_initialized;
	/*
		A queue-submit fence does not prove that vkQueuePresentKHR has finished
		using its wait semaphore. Index render-finished semaphores by acquired
		swapchain image instead. Reacquiring that image proves its previous
		presentation wait completed, which makes this reuse valid on Vulkan 1.0.
	*/
	FB_GFX3_VK_SEMAPHORE *swapchain_rendering_finished;
	uint32_t swapchain_image_count;
	uint32_t swapchain_width;
	uint32_t swapchain_height;
	uint32_t desired_width;
	uint32_t desired_height;
	uint32_t swapchain_format;
	uint32_t swapchain_color_space;
	FB_GFX3_VULKAN_SUBMISSION_SLOT
		submission_slots[FB_GFX3_VK_SUBMISSION_SLOT_COUNT];
	uint32_t active_submission_slot;
	uint32_t next_submission_slot;
	/*
		Idle renderer polls inspect one fence at a time. Slot reuse and explicit
		completions still wait on the exact owning fence, while round-robin
		polling avoids three Vulkan driver calls for every otherwise idle pass.
	*/
	uint32_t next_poll_submission_slot;
	uint32_t latest_submission_slot;
	uint64_t next_submission_serial;
	uint64_t latest_submission_serial;
	uint64_t latest_tagged_submission_serial;
	uint64_t completed_command_sequence;
	/* Counts driver submissions, not the operations recorded into them. */
	uint64_t queue_submit_count;
	/*
		Renderer-drain submission batch

		Each recorded operation still owns a separate descriptor and mapped-data
		slot. Their GPU commands are appended to the first slot's command buffer
		and submitted together, which removes queue-submit calls without allowing
		the CPU to overwrite any data still referenced by the GPU.
	*/
	uint32_t batch_submission_slots[FB_GFX3_VK_SUBMISSION_SLOT_COUNT];
	uint32_t batch_submission_slot_count;
	uint32_t batch_operation_limit;
	uint32_t batch_command_owner_slot;
	int batch_enabled;
	int batch_command_recording;
	/*
		These aliases name the slot currently recording work.  Keeping the
		legacy field names localizes the primitive recording code while slot
		selection owns their lifetime.
	*/
	FB_GFX3_VK_SEMAPHORE image_available;
	FB_GFX3_VK_SEMAPHORE rendering_finished;
	FB_GFX3_VK_COMMAND_POOL command_pool;
	FB_GFX3_VK_COMMAND_BUFFER command_buffer;
	FB_GFX3_VK_FENCE fence;
	FB_GFX3_VK_GET_INSTANCE_PROC_ADDR get_instance_proc_address;
	FB_GFX3_VK_GET_DEVICE_PROC_ADDR get_device_proc_address;
	FB_GFX3_VK_DESTROY_INSTANCE destroy_instance;
	FB_GFX3_VK_DESTROY_SURFACE destroy_surface;
	FB_GFX3_VK_DESTROY_DEVICE destroy_device;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_CAPABILITIES
		get_surface_capabilities;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_FORMATS get_surface_formats;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_SURFACE_PRESENT_MODES
		get_surface_present_modes;
	FB_GFX3_VK_CREATE_SWAPCHAIN create_swapchain;
	FB_GFX3_VK_DESTROY_SWAPCHAIN destroy_swapchain;
	FB_GFX3_VK_GET_SWAPCHAIN_IMAGES get_swapchain_images;
	FB_GFX3_VK_ACQUIRE_NEXT_IMAGE acquire_next_image;
	FB_GFX3_VK_QUEUE_PRESENT queue_present;
	FB_GFX3_VK_CREATE_SEMAPHORE create_semaphore;
	FB_GFX3_VK_DESTROY_SEMAPHORE destroy_semaphore;
	FB_GFX3_VK_DESTROY_COMMAND_POOL destroy_command_pool;
	FB_GFX3_VK_DESTROY_FENCE destroy_fence;
	FB_GFX3_VK_BEGIN_COMMAND_BUFFER begin_command_buffer;
	FB_GFX3_VK_END_COMMAND_BUFFER end_command_buffer;
	FB_GFX3_VK_WAIT_FOR_FENCES wait_for_fences;
	FB_GFX3_VK_GET_FENCE_STATUS get_fence_status;
	FB_GFX3_VK_RESET_FENCES reset_fences;
	FB_GFX3_VK_RESET_COMMAND_POOL reset_command_pool;
	FB_GFX3_VK_QUEUE_SUBMIT queue_submit;
	FB_GFX3_VK_DEVICE_WAIT_IDLE device_wait_idle;
	FB_GFX3_VK_GET_PHYSICAL_DEVICE_MEMORY_PROPERTIES
		get_physical_device_memory_properties;
	FB_GFX3_VK_CREATE_BUFFER create_buffer;
	FB_GFX3_VK_DESTROY_BUFFER destroy_buffer;
	FB_GFX3_VK_GET_BUFFER_MEMORY_REQUIREMENTS get_buffer_memory_requirements;
	FB_GFX3_VK_ALLOCATE_MEMORY allocate_memory;
	FB_GFX3_VK_FREE_MEMORY free_memory;
	FB_GFX3_VK_BIND_BUFFER_MEMORY bind_buffer_memory;
	FB_GFX3_VK_MAP_MEMORY map_memory;
	FB_GFX3_VK_UNMAP_MEMORY unmap_memory;
	FB_GFX3_VK_CMD_FILL_BUFFER command_fill_buffer;
	FB_GFX3_VK_CMD_COPY_BUFFER command_copy_buffer;
	FB_GFX3_VK_CMD_COPY_BUFFER_TO_IMAGE command_copy_buffer_to_image;
	FB_GFX3_VK_CMD_PIPELINE_BARRIER command_pipeline_barrier;
	FB_GFX3_VK_BUFFER present_buffer;
	FB_GFX3_VK_DEVICE_MEMORY present_memory;
	uint64_t present_buffer_size;
	/*
		GET waits for its transfer submission before returning to BASIC.  A single
		host-visible staging allocation can therefore be safely reused by the
		render thread and avoids one Vulkan allocation pair per GET call.
	*/
	FB_GFX3_VULKAN_BUFFER_ALLOCATION download_buffer;
	/*
		Sprite command preparation runs only on the renderer thread and is copied
		into a fence-owned submission slot before the call returns. Retain one
		host allocation across packets instead of invoking the process allocator
		for every sprite run.
	*/
	void *blit_prepare_scratch;
	size_t blit_prepare_scratch_size;
	/*
		Intel's ordered tile path constructs compact bin metadata on the renderer
		thread. These arrays are copied into fence-owned mapped buffers before the
		call returns, so one reusable host allocation of each class is sufficient.
	*/
	void *blit_tile_count_scratch;
	size_t blit_tile_count_scratch_size;
	void *blit_tile_output_scratch;
	size_t blit_tile_output_scratch_size;
	/*
		Primitive tile indexing has the same renderer-thread lifetime as sprite
		indexing. Retain its zeroed counts and packed output storage so a GUI
		frame does not call the process allocator for every rectangle or mixed
		primitive packet.
	*/
	void *rectangle_tile_count_scratch;
	size_t rectangle_tile_count_scratch_size;
	void *rectangle_tile_output_scratch;
	size_t rectangle_tile_output_scratch_size;
	FB_GFX3_VK_DESCRIPTOR_SET_LAYOUT compute_descriptor_set_layout;
	FB_GFX3_VK_PIPELINE_LAYOUT compute_pipeline_layout;
	FB_GFX3_VK_PIPELINE compute_pipeline;
	FB_GFX3_VK_PIPELINE points_pipeline;
	FB_GFX3_VK_PIPELINE line_pipeline;
	FB_GFX3_VK_PIPELINE rectangle_pipeline;
	FB_GFX3_VK_PIPELINE rectangle_tile_pipeline;
	FB_GFX3_VK_PIPELINE blit_pipeline;
	FB_GFX3_VK_PIPELINE transform_blit_pipeline;
	FB_GFX3_VK_PIPELINE blit_winner_pipeline;
	FB_GFX3_VK_PIPELINE blit_resolve_pipeline;
	FB_GFX3_VK_PIPELINE blit_tile_pipeline;
	FB_GFX3_VK_PIPELINE blit_tile_nvidia_pipeline;
	FB_GFX3_VK_PIPELINE blit_tile_trans_pipeline[3];
	FB_GFX3_VK_PIPELINE glyph_tile_pipeline;
	FB_GFX3_VK_PIPELINE ellipse_pipeline;
	FB_GFX3_VK_PIPELINE ellipse_winner_pipeline;
	FB_GFX3_VK_PIPELINE ellipse_resolve_pipeline;
	FB_GFX3_VK_PIPELINE primitive_winner_pipeline;
	FB_GFX3_VK_PIPELINE primitive_resolve_pipeline;
	FB_GFX3_VK_PIPELINE primitive_tile_pipeline;
	FB_GFX3_VK_PIPELINE paint_pipeline;
	FB_GFX3_VK_PIPELINE present_pipeline;
	int shader_float64;
	FB_GFX3_VK_DESCRIPTOR_POOL compute_descriptor_pool;
	FB_GFX3_VK_DESCRIPTOR_SET compute_descriptor_set;
	FB_GFX3_VK_DESTROY_DESCRIPTOR_SET_LAYOUT destroy_descriptor_set_layout;
	FB_GFX3_VK_DESTROY_PIPELINE_LAYOUT destroy_pipeline_layout;
	FB_GFX3_VK_DESTROY_PIPELINE destroy_pipeline;
	FB_GFX3_VK_DESTROY_DESCRIPTOR_POOL destroy_descriptor_pool;
	FB_GFX3_VK_UPDATE_DESCRIPTOR_SETS update_descriptor_sets;
	FB_GFX3_VK_CMD_BIND_PIPELINE command_bind_pipeline;
	FB_GFX3_VK_CMD_BIND_DESCRIPTOR_SETS command_bind_descriptor_sets;
	FB_GFX3_VK_CMD_DISPATCH command_dispatch;
} FB_GFX3_VULKAN_IMPLEMENTATION;

typedef struct FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION {
	FB_GFX3_VULKAN_IMPLEMENTATION *owner;
	FB_GFX3_VULKAN_BUFFER_ALLOCATION storage;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
} FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION;

typedef struct FB_GFX3_VULKAN_POINT_COMMAND {
	uint32_t surface[4];
	FB_GFX3_RECT clip;
	FB_GFX3_POINT points[];
} FB_GFX3_VULKAN_POINT_COMMAND;

typedef struct FB_GFX3_VULKAN_LINE_COMMAND {
	int32_t endpoints[4];
	FB_GFX3_RECT clip;
	uint32_t parameters[4];
	uint32_t format[4];
} FB_GFX3_VULKAN_LINE_COMMAND;

typedef struct FB_GFX3_VULKAN_RECTANGLE_COMMAND {
	int32_t box[4];
	FB_GFX3_RECT clip;
	uint32_t dimensions[4];
	uint32_t parameters[4];
} FB_GFX3_VULKAN_RECTANGLE_COMMAND;

/*
	One compact record assigns a 64-invocation workgroup to a primitive and to
	the first coverage index handled by that group. The shader performs the
	actual line, rectangle, ellipse, or point coverage calculation.
*/
typedef struct FB_GFX3_VULKAN_PRIMITIVE_WORKGROUP {
	uint32_t primitive_index;
	uint32_t first_coverage_index;
} FB_GFX3_VULKAN_PRIMITIVE_WORKGROUP;

typedef struct FB_GFX3_VULKAN_BLIT_COMMAND {
	int32_t source_rect[4];
	FB_GFX3_RECT clip;
	int32_t destination[4];
	uint32_t format[4];
	/* Used by the tiled batch shader; single-operation shaders ignore it. */
	uint32_t dimensions[4];
} FB_GFX3_VULKAN_BLIT_COMMAND;

/*
	Each native-depth TRANS tile command carries one 16-bit column mask, one
	16-bit row mask, and a signed source-address bias. The tile owns its output,
	so these masks replace four absolute bounds and keep the record at 8 bytes.
*/
typedef struct FB_GFX3_VULKAN_BLIT_TRANS_TILE_COMMAND {
	uint32_t coverage;
	int32_t source_bias;
} FB_GFX3_VULKAN_BLIT_TRANS_TILE_COMMAND;

/* Row vectors are padded to sixteen bytes to exactly match GLSL std430. */
typedef struct FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND {
	int32_t source_rect[4];
	FB_GFX3_RECT clip;
	FB_GFX3_RECT bounds;
	float inverse_row_0[4];
	float inverse_row_1[4];
	float inverse_row_2[4];
	uint32_t format[4];
	uint32_t options[4];
	uint32_t dimensions[4];
} FB_GFX3_VULKAN_TRANSFORM_BLIT_COMMAND;

typedef struct FB_GFX3_VULKAN_ELLIPSE_COMMAND {
	int32_t center[4];
	FB_GFX3_RECT clip;
	float radii[4];
	uint32_t parameters[4];
} FB_GFX3_VULKAN_ELLIPSE_COMMAND;

/*
	The three 16-byte header vectors deliberately match std430.  FB_GFX3_GLYPH
	is also scalar-packed to 96 bytes, so the copied producer records can be read
	directly by both Intel and NVIDIA Vulkan drivers.
*/
typedef struct FB_GFX3_VULKAN_GLYPH_COMMAND {
	uint32_t surface[4];
	FB_GFX3_RECT clip;
	uint32_t tile_grid[4];
	FB_GFX3_GLYPH glyph[];
} FB_GFX3_VULKAN_GLYPH_COMMAND;

typedef struct FB_GFX3_VULKAN_PAINT_COMMAND {
	int32_t seed[4];
	FB_GFX3_RECT clip;
	uint32_t format[4];
	uint32_t pattern[4]; /* mode, byte count, absolute x, absolute y */
	uint32_t pattern_word[64];
} FB_GFX3_VULKAN_PAINT_COMMAND;

typedef struct FB_GFX3_VULKAN_PRESENT_COMMAND {
	uint32_t source[4];
	uint32_t destination[4];
	int32_t presentation_rect[4];
	uint32_t palette[256];
	int32_t keyboard_button_rect[4];
	uint32_t keyboard_button_state;
} FB_GFX3_VULKAN_PRESENT_COMMAND;

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

#define FB_GFX3_VK_RESOLVE_DEVICE(result, implementation, type, name) \
	do { \
		union { \
			FB_GFX3_VK_VOID_FUNCTION generic; \
			type typed; \
		} symbol; \
		symbol.generic = (implementation)->get_device_proc_address( \
			(implementation)->device, #name); \
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
		if (vulkan_size_multiply(count, sizeof(array[0]),
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
	if ((count == 0) || (vulkan_size_multiply(count,
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

static void vulkan_buffer_allocation_destroy_immediate(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation);
static int vulkan_submission_slot_wait(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot);
static int vulkan_batch_flush_commands(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);

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

static int vulkan_begin_commands(FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
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
		if (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	slot_index = implementation->next_submission_slot;
	implementation->next_submission_slot++;
	if (implementation->next_submission_slot >=
	    FB_GFX3_VK_SUBMISSION_SLOT_COUNT)
		implementation->next_submission_slot = 0;
	slot = &implementation->submission_slots[slot_index];
	if (slot->submitted) {
		if (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)
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

static int vulkan_batch_flush_commands(
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

static int vulkan_end_commands(FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
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
		if (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
		return vulkan_submission_slot_wait(implementation, slot);
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
	return vulkan_submission_slot_wait(implementation, slot);
}

static int vulkan_end_present_commands(
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

static int vulkan_find_memory_type(
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

static void vulkan_buffer_allocation_destroy_immediate(
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

static void vulkan_submission_slot_release_deferred(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot)
{
	size_t index;

	if ((implementation == NULL) || (slot == NULL))
		return;
	for (index = 0; index < slot->deferred_allocation_count; index++)
		vulkan_buffer_allocation_destroy_immediate(implementation,
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
		vulkan_submission_slot_release_deferred(implementation, slot);
		slot->first_command_sequence = 0;
		slot->command_sequence = 0;
		slot->submission_fence = 0;
		slot->submitted = FALSE;
	}
}

static int vulkan_submission_slot_wait(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot)
{
	FB_GFX3_VK_FENCE submission_fence;
	FB_GFX3_VK_RESULT result;

	if ((implementation == NULL) || (slot == NULL))
		return FB_GFX3_INVALID;
	if (slot->pending_submission) {
		if (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)
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

static int vulkan_submission_poll_next(
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

static int vulkan_submission_wait_all(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation)
{
	uint32_t index;

	if (implementation == NULL)
		return FB_GFX3_INVALID;
	if (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		if (vulkan_submission_slot_wait(implementation,
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

static void vulkan_runtime_update_submission_telemetry(
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

static void vulkan_buffer_allocation_destroy(
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
				(void)vulkan_submission_slot_wait(implementation, slot);
			} else {
				slot->deferred_allocations[slot->deferred_allocation_count++] =
					*allocation;
				memset(allocation, 0, sizeof(*allocation));
				return;
			}
		}
	}
	vulkan_buffer_allocation_destroy_immediate(implementation, allocation);
}

static int vulkan_buffer_allocation_create(
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
	result = vulkan_find_memory_type(implementation,
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
	vulkan_buffer_allocation_destroy(implementation, allocation);
	return result;
}

static int vulkan_host_buffer_ensure(
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
	vulkan_buffer_allocation_destroy_immediate(implementation, allocation);
	return vulkan_buffer_allocation_create(implementation, allocation,
		required_size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
}

static int vulkan_device_storage_buffer_ensure(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t required_size)
{
	if ((implementation == NULL) || (allocation == NULL) ||
	    (required_size == 0u))
		return FB_GFX3_INVALID;
	if ((allocation->buffer != 0) && (allocation->size >= required_size))
		return FB_GFX3_OK;
	/* The caller has waited for the owning submission slot before growing it. */
	vulkan_buffer_allocation_destroy_immediate(implementation, allocation);
	return vulkan_buffer_allocation_create(implementation, allocation,
		required_size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
}

static void vulkan_record_memory_barrier(
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

static void vulkan_record_buffer_barrier(
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

static uint32_t vulkan_surface_bytes_per_pixel(uint32_t depth)
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

static uint32_t vulkan_surface_color_mask(uint32_t depth)
{
	return (depth >= 32) ? UINT32_MAX : (1u << depth) - 1u;
}

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
		vulkan_buffer_allocation_destroy(implementation,
			&implementation->submission_slots[index].
			present_command_buffer);
	memset(&allocation, 0, sizeof(allocation));
	allocation.buffer = implementation->present_buffer;
	allocation.memory = implementation->present_memory;
	allocation.size = implementation->present_buffer_size;
	vulkan_buffer_allocation_destroy(implementation, &allocation);
	implementation->present_buffer = 0;
	implementation->present_memory = 0;
	implementation->present_buffer_size = 0;
}

static void vulkan_swapchain_destroy(
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
	    (vulkan_size_multiply(count, sizeof(formats[0]),
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
	    (vulkan_size_multiply(count, sizeof(modes[0]),
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

static int vulkan_swapchain_create(
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
	if (vulkan_submission_wait_all(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	vulkan_swapchain_destroy(implementation);
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
	    (vulkan_size_multiply(written,
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
	    (vulkan_size_multiply(written,
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
	if ((vulkan_size_multiply(extent.width, extent.height, &pixel_count) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_multiply(pixel_count, sizeof(uint32_t),
	     &buffer_size) != FB_GFX3_OK) || (buffer_size == 0)) {
		result = FB_GFX3_INVALID;
		goto fail;
	}
	memset(&present_allocation, 0, sizeof(present_allocation));
	result = vulkan_buffer_allocation_create(implementation,
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
		result = vulkan_buffer_allocation_create(implementation,
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
	vulkan_swapchain_destroy(implementation);
	return result;
}

static int vulkan_presentation_device_create(
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
	return vulkan_swapchain_create(implementation);
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
	(void)vulkan_batch_flush_commands(implementation);
	if ((implementation->device != NULL) &&
	    (implementation->device_wait_idle != NULL))
		implementation->device_wait_idle(implementation->device);
	(void)vulkan_submission_wait_all(implementation);
	vulkan_swapchain_destroy(implementation);
	for (index = 0; index < FB_GFX3_VK_SUBMISSION_SLOT_COUNT; index++) {
		FB_GFX3_VULKAN_SUBMISSION_SLOT *slot =
			&implementation->submission_slots[index];

		vulkan_submission_slot_release_deferred(implementation, slot);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_command_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_winner_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_index_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_range_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_command_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_winner_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_workgroup_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->point_command_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_command_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_range_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_index_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->rectangle_tile_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->transform_command_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->glyph_index_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->glyph_range_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->glyph_command_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->paint_scratch_buffer);
		vulkan_buffer_allocation_destroy_immediate(implementation,
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
	vulkan_buffer_allocation_destroy_immediate(implementation,
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
	const char *instance_extensions[2] = {
		"VK_KHR_surface",
		fb_gfx3_vulkan_platform_instance_extension()
	};
	const char *device_extensions[1] = { "VK_KHR_swapchain" };
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
	if (windowed) {
		instance_create_info.enabled_extension_count =
			sizeof(instance_extensions) /
			sizeof(instance_extensions[0]);
		instance_create_info.enabled_extension_names = instance_extensions;
	}
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
	if (windowed) {
		device_create_info.enabled_extension_count =
			sizeof(device_extensions) / sizeof(device_extensions[0]);
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
		result = vulkan_presentation_device_create(implementation);
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

int fb_gfx3_vulkan_runtime_open_windowed(FB_GFX3_VULKAN_RUNTIME *runtime,
	uintptr_t native_instance, uintptr_t native_window, uint32_t width,
	uint32_t height)
{
	return vulkan_runtime_open_internal(runtime, native_instance,
		native_window, width, height);
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
	result = vulkan_swapchain_create(implementation);
	if (result == FB_GFX3_EXHAUSTED)
		return FB_GFX3_OK;
	if (result != FB_GFX3_OK)
		return result;
	runtime->present_width = implementation->swapchain_width;
	runtime->present_height = implementation->swapchain_height;
	runtime->swapchain_image_count = implementation->swapchain_image_count;
	return FB_GFX3_OK;
}

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
	if (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	if ((implementation->device_wait_idle == NULL) ||
	    (implementation->device_wait_idle(implementation->device) !=
	     FB_GFX3_VK_SUCCESS))
		return FB_GFX3_FAILED;
	if (vulkan_submission_wait_all(implementation) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	vulkan_runtime_update_submission_telemetry(runtime, implementation);
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
		if (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	vulkan_runtime_update_submission_telemetry(runtime, implementation);
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
	result = vulkan_submission_poll_next(implementation);
	if (result != FB_GFX3_OK)
		return result;
	vulkan_runtime_update_submission_telemetry(runtime, implementation);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_end_commands(implementation, FALSE);
	if (result != FB_GFX3_OK)
		return result;
	runtime->completed_submission_count++;
	vulkan_runtime_update_submission_telemetry(runtime, implementation);
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
	    (vulkan_size_multiply(value_count, sizeof(values[0]),
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
	result = vulkan_find_memory_type(implementation,
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

	result = vulkan_begin_commands(implementation);
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
	result = vulkan_end_commands(implementation, TRUE);
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
	    (vulkan_size_add(value_count, 2, &total_words) != FB_GFX3_OK) ||
	    (vulkan_size_multiply(total_words, sizeof(values[0]),
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
	result = vulkan_find_memory_type(implementation,
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
	result = vulkan_begin_commands(implementation);
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
	result = vulkan_end_commands(implementation, TRUE);
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

/* ------------------------------------------------------------------------- */
/* Device-local surface foundation                                           */
/* ------------------------------------------------------------------------- */

static int vulkan_surface_validate(FB_GFX3_VULKAN_RUNTIME *runtime,
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
	    (vulkan_surface_bytes_per_pixel(depth) == 0) ||
	    (vulkan_size_multiply(width, height, &pixel_count) != FB_GFX3_OK) ||
	    (vulkan_size_multiply(pixel_count, sizeof(uint32_t),
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
	result = vulkan_buffer_allocation_create(implementation,
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

	result = vulkan_surface_validate(runtime, surface, &implementation,
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
	color &= vulkan_surface_color_mask(storage->depth);
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
				rectangle->color & vulkan_surface_color_mask(storage->depth));
		}
	}
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;
	return result;
}

static int vulkan_surface_dispatch_command(
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
	result = vulkan_buffer_allocation_create(implementation,
		&command_buffer, (uint64_t)command_size,
		FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	memcpy(command_buffer.mapped, command_data, command_size);
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &command_buffer);
	return result;
}

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

	result = vulkan_surface_validate(runtime, surface, &implementation,
		&storage);
	if ((result != FB_GFX3_OK) || (clip == NULL) ||
	    ((point_count != 0) && (points == NULL)) ||
	    (vulkan_size_multiply(point_count, sizeof(points[0]),
	     &points_size) != FB_GFX3_OK) ||
	    (vulkan_size_add(offsetof(FB_GFX3_VULKAN_POINT_COMMAND, points),
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
	    (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->point_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK)
		return result;
	command_buffer = &slot->point_command_buffer;
	point_command = (FB_GFX3_VULKAN_POINT_COMMAND *)command_buffer->mapped;
	point_command->surface[0] = surface->width;
	point_command->surface[1] = surface->height;
	point_command->surface[2] = point_count;
	point_command->surface[3] = vulkan_surface_color_mask(surface->depth);
	point_command->clip = clipped;
	memcpy(point_command->points, points, points_size);
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	for (index = 0; index < operation_count; index++) {
		const FB_GFX3_VULKAN_POINTS *operation = &operations[index];
		size_t points_size;
		size_t size;

		if ((operation->point_count == 0u) || (operation->points == NULL))
			continue;
		if ((vulkan_size_multiply(operation->point_count,
		     sizeof(operation->points[0]), &points_size) != FB_GFX3_OK) ||
		    (vulkan_size_add(offsetof(FB_GFX3_VULKAN_POINT_COMMAND, points),
		     points_size, &size) != FB_GFX3_OK) ||
		    (vulkan_size_add(command_size, size, &command_size) !=
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
	    (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	result = vulkan_host_buffer_ensure(implementation,
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
		point_command->surface[3] = vulkan_surface_color_mask(surface->depth);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(active_count * 2u), descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
		vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
			storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = vulkan_end_commands(implementation, FALSE);
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
	    (vulkan_size_multiply(glyph_count, sizeof(glyphs[0]),
	     &glyph_bytes) != FB_GFX3_OK) ||
	    (vulkan_size_add(offsetof(FB_GFX3_VULKAN_GLYPH_COMMAND, glyph),
	     glyph_bytes, &command_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	result = vulkan_surface_validate(runtime, surface, &implementation,
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
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	result = vulkan_host_buffer_ensure(implementation,
		&slot->glyph_command_buffer, command_capacity);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->glyph_range_buffer, range_capacity);
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = vulkan_host_buffer_ensure(implementation,
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
	command->surface[2] = vulkan_surface_color_mask(storage->depth);
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
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, range_buffer->buffer,
		(uint64_t)tile_count * 2u * sizeof(tile_ranges[0]),
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, index_buffer->buffer,
		(uint64_t)index_count * sizeof(tile_indices[0]),
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	free(tile_indices);
	free(tile_cursors);
	free(tile_ranges);
	free(tile_counts);
	return result;
}

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
	result = vulkan_surface_validate(runtime, surface, &implementation,
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

	result = vulkan_buffer_allocation_create(implementation,
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
	line_command->format[0] = vulkan_surface_color_mask(surface->depth);
	line_command->format[1] = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	line_command->format[2] = 0;
	line_command->format[3] = 0;
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &command_buffer);
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
		&storage);
	if (result != FB_GFX3_OK)
		return result;
	if (vulkan_size_multiply(line_count, sizeof(*commands), &command_size) !=
		FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;
	result = vulkan_buffer_allocation_create(implementation, &command_buffer,
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
			vulkan_surface_color_mask(surface->depth);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(operation_count * 2), descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
		vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
			storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &command_buffer);
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

	result = vulkan_surface_validate(runtime, surface, &implementation,
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
		vulkan_surface_color_mask(surface->depth);
	rectangle_command.parameters[3] =
		(flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND) |
		((filled != 0) ? 0x80000000u : 0u);
	return vulkan_surface_dispatch_command(runtime, implementation, storage,
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
	if (vulkan_size_multiply(tiles, sizeof(*counts), &count_size) !=
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
	if ((vulkan_size_multiply((size_t)active_tiles * 2u, sizeof(*ranges),
	     &range_size) != FB_GFX3_OK) ||
	    (vulkan_size_multiply(tiles, sizeof(*cursors), &cursor_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_multiply(index_count, sizeof(*indices), &index_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_multiply((size_t)active_tiles * 2u,
	     sizeof(*tile_coordinates), &tile_coordinate_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(range_size, cursor_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(scratch_size, index_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(scratch_size, tile_coordinate_size, &scratch_size) !=
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
	    (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED; goto cleanup;
	}
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED; goto cleanup;
	}
	command_size = command_count * sizeof(*commands);
	result = vulkan_host_buffer_ensure(implementation,
		&slot->rectangle_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK) goto cleanup;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->rectangle_range_buffer,
		(uint64_t)active_tiles * 2u * sizeof(*ranges));
	if (result != FB_GFX3_OK) goto cleanup;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->rectangle_index_buffer,
		(uint64_t)index_count * sizeof(*indices));
	if (result != FB_GFX3_OK) goto cleanup;
	result = vulkan_host_buffer_ensure(implementation,
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK) goto cleanup;
	implementation->update_descriptor_sets(implementation->device, 5, writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, tiles_buffer->buffer,
		tile_coordinate_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, ranges_buffer->buffer, ranges_buffer->size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, indices_buffer->buffer, indices_buffer->size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer, storage->storage.size,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer, storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
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
			vulkan_surface_color_mask(storage->depth);
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
			vulkan_surface_color_mask(storage->depth);
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
	if (vulkan_size_multiply(operation_count, sizeof(commands[0]),
		&command_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;
	return vulkan_surface_dispatch_command(runtime, implementation, storage,
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
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
			vulkan_surface_color_mask(storage->depth);
		command->parameters[1] = rectangle->style & 0xFFFFu;
		command->parameters[2] =
			vulkan_surface_color_mask(storage->depth);
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

	result = vulkan_surface_validate(runtime, surface, &implementation,
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
		vulkan_surface_color_mask(surface->depth);
	ellipse_command.parameters[3] = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	return vulkan_surface_dispatch_command(runtime, implementation, storage,
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
	result = vulkan_buffer_allocation_create(implementation, &winner_buffer,
		storage->storage.size, FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, winner_buffer.buffer,
		winner_buffer.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
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
	vulkan_record_buffer_barrier(implementation, winner_buffer.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &winner_buffer);
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
	if (vulkan_size_multiply(tiles, sizeof(*counts), &count_size) !=
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
	if ((vulkan_size_multiply((size_t)active_tiles * 2u, sizeof(*ranges),
	     &range_size) != FB_GFX3_OK) ||
	    (vulkan_size_multiply(tiles, sizeof(*cursors), &cursor_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_multiply(index_count, sizeof(*indices), &index_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_multiply((size_t)active_tiles * 2u,
	     sizeof(*tile_coordinates), &tile_coordinate_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(range_size, cursor_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(scratch_size, index_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(scratch_size, tile_coordinate_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_multiply(primitive_count, sizeof(*commands),
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
	    (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->primitive_command_buffer, (uint64_t)command_size);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->rectangle_range_buffer, (uint64_t)range_size);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_host_buffer_ensure(implementation,
		&slot->rectangle_index_buffer, (uint64_t)index_size);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_host_buffer_ensure(implementation,
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
			vulkan_surface_color_mask(storage->depth);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device, 5u,
		writes, 0u, NULL);
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, tiles_buffer->buffer,
		tile_coordinate_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, ranges_buffer->buffer,
		range_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, indices_buffer->buffer,
		index_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
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
	    (vulkan_size_multiply(primitive_count, sizeof(*commands),
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
	if (vulkan_size_multiply(total_workgroups, sizeof(*workgroups),
	    &workgroup_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;

	/*
		The next begin call selects this slot. Wait before rewriting mapped
		memory, then retain allocations for later turns through the slot ring.
	*/
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->pending_submission) {
		if (vulkan_batch_flush_commands(implementation) != FB_GFX3_OK)
			return FB_GFX3_FAILED;
	}
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->primitive_command_buffer.size < command_size) {
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_command_buffer);
		result = vulkan_buffer_allocation_create(implementation,
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
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->primitive_winner_buffer);
		result = vulkan_buffer_allocation_create(implementation,
			&slot->primitive_winner_buffer, storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			return result;
		slot->primitive_generation = 0u;
		reset_winner = TRUE;
	}
	result = vulkan_host_buffer_ensure(implementation,
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
			vulkan_surface_color_mask(surface->depth);
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
	result = vulkan_begin_commands(implementation);
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
		vulkan_record_buffer_barrier(implementation,
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
		vulkan_record_buffer_barrier(implementation,
			slot->primitive_winner_buffer.buffer, storage->storage.size,
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	vulkan_record_buffer_barrier(implementation,
		slot->primitive_command_buffer.buffer, command_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
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
	if (vulkan_size_multiply(operation_count, sizeof(*commands),
		&command_size) != FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;
	result = vulkan_buffer_allocation_create(implementation, &command_buffer,
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
		command->parameters[2] = vulkan_surface_color_mask(surface->depth);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(operation_count * 2u), descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
		vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
			storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &command_buffer);
	return result;
}

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

	result = vulkan_surface_validate(runtime, surface, &implementation,
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
	if ((vulkan_size_multiply(surface->width, surface->height,
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
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	result = vulkan_host_buffer_ensure(implementation,
		&slot->paint_command_buffer, sizeof(*paint_command));
	if (result != FB_GFX3_OK)
		goto cleanup;
	result = vulkan_device_storage_buffer_ensure(implementation,
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
	paint_command->format[2] = vulkan_surface_color_mask(surface->depth);
	paint_command->format[3] = flags & FB_GFX3_PRIMITIVE_ALPHA_BLEND;
	paint_command->pattern[0] = paint_payload->paint_mode;
	paint_command->pattern[1] = paint_payload->pattern_size;
	paint_command->pattern[2] = paint_payload->pattern_origin_x;
	paint_command->pattern[3] = paint_payload->pattern_origin_y;
	memcpy(paint_command->pattern_word, paint_payload->pattern_word,
		sizeof(paint_command->pattern_word));
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, command_buffer->buffer,
		sizeof(*paint_command), FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		0, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer, 1, 1, 1);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT |
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT |
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 2u);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer,
		rectangle_groups_x, rectangle_groups_y, 1);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT |
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT |
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 3u);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer,
		rectangle_groups_x, rectangle_groups_y, 1);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT |
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT |
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_fill_buffer(implementation->command_buffer,
		scratch->buffer, phase_offset, sizeof(uint32_t), 4u);
	vulkan_record_buffer_barrier(implementation, scratch->buffer, scratch_size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT | FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	implementation->command_dispatch(implementation->command_buffer, 1, 1, 1);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	return result;
}

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
	result = vulkan_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_surface_validate(runtime, source,
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
	if ((vulkan_size_multiply(width, height, &pixel_count_size) !=
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
		result = vulkan_buffer_allocation_create(implementation, &snapshot,
			source_storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		result = vulkan_begin_commands(implementation);
		if (result != FB_GFX3_OK)
			goto cleanup;
		vulkan_record_buffer_barrier(implementation,
			source_storage->storage.buffer,
			source_storage->storage.size,
			FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
			FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
			FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		vulkan_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, 0, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&copy_region, 0, sizeof(copy_region));
		copy_region.size = source_storage->storage.size;
		implementation->command_copy_buffer(
			implementation->command_buffer,
			source_storage->storage.buffer, snapshot.buffer, 1,
			&copy_region);
		vulkan_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		result = vulkan_end_commands(implementation, FALSE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		runtime->completed_submission_count++;
		source_buffer = snapshot.buffer;
	}
	result = vulkan_buffer_allocation_create(implementation,
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
		vulkan_surface_color_mask(destination->depth);
	blit_command->dimensions[0] = source->width;
	blit_command->dimensions[1] = destination->width;
	blit_command->dimensions[2] = destination->height;
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation, command_buffer.buffer,
		command_buffer.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, source_buffer,
		source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &command_buffer);
	vulkan_buffer_allocation_destroy(implementation, &snapshot);
	return result;
}

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
		result = vulkan_surface_validate(runtime, copies[index].destination,
			&destination_implementation, &destination_storage[index]);
		if (result != FB_GFX3_OK)
			return result;
		result = vulkan_surface_validate(runtime, copies[index].source,
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	/* Make all earlier shader or transfer writes available to this copy run. */
	vulkan_record_memory_barrier(implementation,
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
			vulkan_record_memory_barrier(implementation,
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
	vulkan_record_memory_barrier(implementation,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT |
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_surface_validate(runtime, source,
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
	if ((vulkan_size_multiply(width, height, &pixel_count_size) !=
	     FB_GFX3_OK) || (pixel_count_size > UINT32_MAX))
		return FB_GFX3_UNSUPPORTED;
	pixel_count = (uint32_t)pixel_count_size;
	group_count_x = (pixel_count - 1u) / 64u + 1u;

	self_transform = source_storage == destination_storage;
	source_buffer = source_storage->storage.buffer;
	if (self_transform) {
		result = vulkan_buffer_allocation_create(implementation, &snapshot,
			source_storage->storage.size,
			FB_GFX3_VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FALSE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		result = vulkan_begin_commands(implementation);
		if (result != FB_GFX3_OK)
			goto cleanup;
		vulkan_record_buffer_barrier(implementation,
			source_storage->storage.buffer, source_storage->storage.size,
			FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
			FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
			FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		vulkan_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, 0, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
		memset(&copy_region, 0, sizeof(copy_region));
		copy_region.size = source_storage->storage.size;
		implementation->command_copy_buffer(implementation->command_buffer,
			source_storage->storage.buffer, snapshot.buffer, 1, &copy_region);
		vulkan_record_buffer_barrier(implementation, snapshot.buffer,
			snapshot.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		result = vulkan_end_commands(implementation, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
		runtime->completed_submission_count++;
		source_buffer = snapshot.buffer;
	}

	/* vulkan_begin_commands() will select this same checked slot. */
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	if (slot->blit_command_buffer.buffer == 0) {
		result = vulkan_buffer_allocation_create(implementation,
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
	command->format[3] = vulkan_surface_color_mask(destination->depth);
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

	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device, 3,
		descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation,
		slot->blit_command_buffer.buffer, sizeof(*command),
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, source_buffer,
		source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, self_transform);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &snapshot);
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

	result = vulkan_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_surface_validate(runtime, source,
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
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->transform_command_buffer.buffer == 0) {
		result = vulkan_buffer_allocation_create(implementation,
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
		if ((vulkan_size_multiply(width, height, &pixel_count_size) !=
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
		command->format[3] = vulkan_surface_color_mask(destination->depth);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device,
		operation_count * 3u, descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation,
		slot->transform_command_buffer.buffer, command_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
		source_storage->storage.buffer, source_storage->storage.size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
		vulkan_record_buffer_barrier(implementation,
			destination_storage->storage.buffer,
			destination_storage->storage.size,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = vulkan_end_commands(implementation, FALSE);
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
	if (vulkan_size_multiply(operation_count, sizeof(*commands),
	    &command_size) != FB_GFX3_OK)
		return FB_GFX3_UNSUPPORTED;

	/* The next begin call selects this slot, so its fence owns all reuse below. */
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK))
		return FB_GFX3_FAILED;
	if (slot->blit_command_buffer.size < command_size) {
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_command_buffer);
		result = vulkan_buffer_allocation_create(implementation,
			&slot->blit_command_buffer,
			(uint64_t)FB_GFX3_VK_BLIT_TILE_BATCH_LIMIT * sizeof(*commands),
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			return result;
	}
	if (slot->blit_winner_buffer.size < destination_storage->storage.size) {
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_winner_buffer);
		result = vulkan_buffer_allocation_create(implementation,
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

	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		return result;
	implementation->update_descriptor_sets(implementation->device, 7u,
		descriptor_writes, 0, NULL);
	implementation->command_fill_buffer(implementation->command_buffer,
		slot->blit_winner_buffer.buffer, 0,
		destination_storage->storage.size, 0u);
	vulkan_record_buffer_barrier(implementation,
		slot->blit_winner_buffer.buffer, destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
		slot->blit_command_buffer.buffer, command_size,
		FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation,
		slot->blit_winner_buffer.buffer, destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation,
		destination_storage->storage.buffer,
		destination_storage->storage.size,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	if (vulkan_size_multiply(tile_count, sizeof(*tile_counts),
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
	if (vulkan_size_multiply(index_count, (trans_pipeline_index >= 0) ?
	    sizeof(*trans_tile_commands) : sizeof(*tile_indices),
	    &index_size) != FB_GFX3_OK) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if (vulkan_size_multiply((trans_pipeline_index >= 0) ?
	    (size_t)active_tile_count * 4u : (size_t)tile_count * 2u,
	    sizeof(*tile_ranges), &range_size) != FB_GFX3_OK) {
		result = FB_GFX3_OUT_OF_MEMORY;
		goto cleanup;
	}
	if ((vulkan_size_multiply(tile_count, sizeof(*tile_cursors),
	     &cursor_size) != FB_GFX3_OK) ||
	    (vulkan_size_add(range_size, cursor_size, &scratch_size) !=
	     FB_GFX3_OK) ||
	    (vulkan_size_add(scratch_size, index_size, &scratch_size) !=
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
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	range_buffer = &slot->blit_range_buffer;
	index_buffer = &slot->blit_index_buffer;
	if (range_buffer->size < range_size) {
		vulkan_buffer_allocation_destroy_immediate(implementation,
			range_buffer);
		result = vulkan_buffer_allocation_create(implementation, range_buffer,
			range_size,
			FB_GFX3_VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
		if (result != FB_GFX3_OK)
			goto cleanup;
	}
	if (index_buffer->size < index_size) {
		vulkan_buffer_allocation_destroy_immediate(implementation,
			index_buffer);
		result = vulkan_buffer_allocation_create(implementation, index_buffer,
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
		vulkan_buffer_allocation_destroy_immediate(implementation,
			&slot->blit_command_buffer);
		result = vulkan_buffer_allocation_create(implementation,
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	implementation->update_descriptor_sets(implementation->device, 5u,
		descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation, slot->blit_command_buffer.buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, range_buffer->buffer,
		range_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, index_buffer->buffer,
		index_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, source_storage->storage.buffer,
		source_storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, destination_storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, destination_storage->storage.buffer,
		destination_storage->storage.size, FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT | FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
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
	result = vulkan_surface_validate(runtime, destination, &implementation,
		&destination_storage);
	if (result != FB_GFX3_OK)
		return result;
	result = vulkan_surface_validate(runtime, source, &source_implementation,
		&source_storage);
	if (result != FB_GFX3_OK)
		return result;
	/* Same-surface PUT needs a snapshot per operation and stays on the safe path. */
	if ((implementation != source_implementation) ||
	    (destination->depth != source->depth) ||
	    (destination_storage == source_storage))
		return FB_GFX3_UNSUPPORTED;
	if (vulkan_size_multiply(blit_count, sizeof(*commands), &command_size) !=
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
		if ((vulkan_size_multiply(width, height, &pixel_count_size) !=
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
			vulkan_surface_color_mask(destination->depth);
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
		The next command slot is the one vulkan_begin_commands() will select.
		Wait before reusing its mapped records, then retain the allocation for
		all future batches in that slot.  This matches descriptor-set lifetime
		and does not add a wait that the old begin path did not already require.
	*/
	slot = &implementation->submission_slots[
		implementation->next_submission_slot];
	if (slot->submitted &&
	    (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)) {
		result = FB_GFX3_FAILED;
		goto cleanup;
	}
	if (slot->blit_command_buffer.buffer == 0) {
		result = vulkan_buffer_allocation_create(implementation,
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	/* The selected slot has completed before its descriptor sets are rewritten. */
	implementation->update_descriptor_sets(implementation->device,
		(uint32_t)(operation_count * 3), descriptor_writes, 0, NULL);
	vulkan_record_buffer_barrier(implementation, slot->blit_command_buffer.buffer,
		command_size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT, FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, source_storage->storage.buffer,
		source_storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT, FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
		vulkan_record_buffer_barrier(implementation,
			destination_storage->storage.buffer,
			destination_storage->storage.size,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_ACCESS_SHADER_READ_BIT |
			FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
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
	result = vulkan_surface_validate(runtime, surface, &implementation,
		&storage);
	bytes_per_pixel = (surface != NULL) ?
		vulkan_surface_bytes_per_pixel(surface->depth) : 0;
	if ((result != FB_GFX3_OK) || (source == NULL) || (width == 0) ||
	    (height == 0) || (destination_x < 0) || (destination_y < 0) ||
	    ((uint64_t)(uint32_t)destination_x + width > surface->width) ||
	    ((uint64_t)(uint32_t)destination_y + height > surface->height) ||
	    (vulkan_size_multiply(width, bytes_per_pixel,
	     &source_row_size) != FB_GFX3_OK) ||
	    (source_pitch < source_row_size) ||
	    (vulkan_size_multiply(width, height,
	     &staging_pixel_count) != FB_GFX3_OK) ||
	    (vulkan_size_multiply(staging_pixel_count, sizeof(uint32_t),
	     &staging_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	result = vulkan_buffer_allocation_create(implementation, &staging,
		(uint64_t)staging_size, FB_GFX3_VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		FB_GFX3_VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, TRUE);
	if (result != FB_GFX3_OK)
		return result;
	staging_pixels = (uint32_t *)staging.mapped;
	mask = vulkan_surface_color_mask(surface->depth);
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
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	vulkan_record_buffer_barrier(implementation, staging.buffer,
		staging.size, FB_GFX3_VK_ACCESS_HOST_WRITE_BIT,
		FB_GFX3_VK_ACCESS_TRANSFER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT |
		FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	result = vulkan_end_commands(implementation, FALSE);
	if (result == FB_GFX3_OK)
		runtime->completed_submission_count++;

cleanup:
	vulkan_buffer_allocation_destroy(implementation, &staging);
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
	vulkan_buffer_allocation_destroy_immediate(implementation, buffer);
	result = vulkan_buffer_allocation_create(implementation, buffer,
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

	result = vulkan_surface_validate(runtime, surface, &implementation,
		&storage);
	bytes_per_pixel = (surface != NULL) ?
		vulkan_surface_bytes_per_pixel(surface->depth) : 0;
	if ((result != FB_GFX3_OK) || (destination == NULL) || (width == 0) ||
	    (height == 0) || (source_x < 0) || (source_y < 0) ||
	    ((uint64_t)(uint32_t)source_x + width > surface->width) ||
	    ((uint64_t)(uint32_t)source_y + height > surface->height) ||
	    (vulkan_size_multiply(width, bytes_per_pixel,
	     &destination_row_size) != FB_GFX3_OK) ||
	    (destination_pitch < destination_row_size) ||
	    (vulkan_size_multiply(width, height,
	     &staging_pixel_count) != FB_GFX3_OK) ||
	    (vulkan_size_multiply(staging_pixel_count, sizeof(uint32_t),
	     &staging_size) != FB_GFX3_OK))
		return FB_GFX3_INVALID;
	result = vulkan_download_buffer_ensure(implementation,
		(uint64_t)staging_size);
	if (result != FB_GFX3_OK)
		return result;
	staging = &implementation->download_buffer;
	result = vulkan_begin_commands(implementation);
	if (result != FB_GFX3_OK)
		goto cleanup;
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
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
	vulkan_record_buffer_barrier(implementation, staging->buffer,
		staging->size, FB_GFX3_VK_ACCESS_TRANSFER_WRITE_BIT,
		FB_GFX3_VK_ACCESS_HOST_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_TRANSFER_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_HOST_BIT);
	result = vulkan_end_commands(implementation, TRUE);
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
	    (vulkan_surface_bytes_per_pixel(storage->depth) == 0) ||
	    (implementation->present_pipeline == 0))
		return FB_GFX3_INVALID;
	if (implementation->swapchain == 0) {
		result = vulkan_swapchain_create(implementation);
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
	if (vulkan_submission_slot_wait(implementation, slot) != FB_GFX3_OK)
		return FB_GFX3_FAILED;
	vulkan_runtime_update_submission_telemetry(runtime, implementation);
	for (attempt = 0; attempt < 2; attempt++) {
		if ((vulkan_size_multiply(implementation->swapchain_width,
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
		result = vulkan_swapchain_create(implementation);
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
	result = vulkan_begin_commands(implementation);
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
	vulkan_record_buffer_barrier(implementation,
		implementation->present_buffer,
		implementation->present_buffer_size,
		FB_GFX3_VK_ACCESS_MEMORY_READ_BIT,
		FB_GFX3_VK_ACCESS_SHADER_WRITE_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation, storage->storage.buffer,
		storage->storage.size, FB_GFX3_VK_ACCESS_MEMORY_WRITE_BIT,
		FB_GFX3_VK_ACCESS_SHADER_READ_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		FB_GFX3_VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	vulkan_record_buffer_barrier(implementation,
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
	vulkan_record_buffer_barrier(implementation,
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
	result = vulkan_end_present_commands(implementation, image_index,
		&recreate);
	if (result != FB_GFX3_OK)
		return result;
	vulkan_runtime_update_submission_telemetry(runtime, implementation);
	implementation->swapchain_image_initialized[image_index] = TRUE;
	runtime->completed_submission_count++;
	if (recreate || acquire_suboptimal) {
		result = vulkan_swapchain_create(implementation);
		if ((result != FB_GFX3_OK) && (result != FB_GFX3_EXHAUSTED))
			return result;
	}
	runtime->present_width = implementation->swapchain_width;
	runtime->present_height = implementation->swapchain_height;
	runtime->swapchain_image_count =
		implementation->swapchain_image_count;
	return FB_GFX3_OK;
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
	vulkan_buffer_allocation_destroy(implementation, &storage->storage);
	free(storage);
	memset(surface, 0, sizeof(*surface));
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
