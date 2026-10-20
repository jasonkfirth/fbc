/*
 * FreeBASIC gfxlib3: gfx3_vulkan_private.h
 * Purpose: Share renderer-private types and cross-unit helpers.
 * Responsibilities: backend state layout, checked helpers and render-thread ownership.
 * This file intentionally does NOT contain public ABI declarations or native window policy.
 */

#ifndef FB_GFX3_VULKAN_PRIVATE_H
#define FB_GFX3_VULKAN_PRIVATE_H

#include "../gfx3_vulkan.h"
#include "../gfx3_vulkan_platform.h"
#include "../gfx3_presentation.h"

#if defined(HOST_WIN32) || defined(HOST_LINUX) || defined(HOST_ANDROID) || defined(HOST_DARWIN)

/*
 * Only this backend's implementation units may include this header. The
 * renderer thread owns the device/context and all fields below. Producer
 * threads communicate through copied commands and completion barriers.
 * These layouts describe backend storage, not the public FreeBASIC ABI.
 */

#define FB_GFX3_VK_CALL FBCALL

/* Device entry points are used by both command submission and presentation.
   Keep the existing typed resolver with their shared backend contract. */
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
/* Header-independent Vulkan ABI                                            */
/* ------------------------------------------------------------------------- */

/* Field widths, member order, pointer handles and calling conventions
 * below follow the Vulkan loader ABI. These declarations let the
 * runtime load Vulkan without a build-time SDK dependency.
 */
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

/* ------------------------------------------------------------------------- */
/* Renderer-owned resources and submission storage                          */
/* ------------------------------------------------------------------------- */

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

/* ------------------------------------------------------------------------- */
/* Shader-visible command records                                           */
/* ------------------------------------------------------------------------- */

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
/* Loader conversion and cross-unit operations                              */
/* ------------------------------------------------------------------------- */

/* Vulkan entry points use the same typed conversion during device and
 * swapchain setup. These declarations retain the loader's calling convention.
 */
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

/* Cross-unit helpers retain one definition. All other helpers stay static. */
int fb_gfx3_vk_batch_flush_commands(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
int fb_gfx3_vk_begin_commands(FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
int fb_gfx3_vk_buffer_allocation_create(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t size,
	FB_GFX3_VK_FLAGS usage, FB_GFX3_VK_FLAGS memory_properties,
	int map_memory);
void fb_gfx3_vk_buffer_allocation_destroy(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation);
void fb_gfx3_vk_buffer_allocation_destroy_immediate(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation);
int fb_gfx3_vk_device_storage_buffer_ensure(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t required_size);
int fb_gfx3_vk_end_commands(FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	int wait_for_completion);
int fb_gfx3_vk_end_present_commands(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation, uint32_t image_index,
	int *recreate);
int fb_gfx3_vk_find_memory_type(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	uint32_t allowed_types, FB_GFX3_VK_FLAGS required_properties,
	uint32_t *memory_type_index);
int fb_gfx3_vk_host_buffer_ensure(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_BUFFER_ALLOCATION *allocation, uint64_t required_size);
int fb_gfx3_vk_presentation_device_create(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
void fb_gfx3_vk_record_buffer_barrier(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_BUFFER buffer, uint64_t size,
	FB_GFX3_VK_FLAGS source_access_mask,
	FB_GFX3_VK_FLAGS destination_access_mask,
	FB_GFX3_VK_FLAGS source_stage_mask,
	FB_GFX3_VK_FLAGS destination_stage_mask);
void fb_gfx3_vk_record_memory_barrier(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VK_FLAGS source_access_mask,
	FB_GFX3_VK_FLAGS destination_access_mask,
	FB_GFX3_VK_FLAGS source_stage_mask,
	FB_GFX3_VK_FLAGS destination_stage_mask);
void fb_gfx3_vk_runtime_update_submission_telemetry(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	const FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
int fb_gfx3_vk_size_add(size_t left, size_t right, size_t *result);
int fb_gfx3_vk_size_multiply(size_t left, size_t right, size_t *result);
int fb_gfx3_vk_submission_poll_next(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
void fb_gfx3_vk_submission_slot_release_deferred(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot);
int fb_gfx3_vk_submission_slot_wait(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SUBMISSION_SLOT *slot);
int fb_gfx3_vk_submission_wait_all(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
uint32_t fb_gfx3_vk_surface_bytes_per_pixel(uint32_t depth);
uint32_t fb_gfx3_vk_surface_color_mask(uint32_t depth);
int fb_gfx3_vk_surface_dispatch_command(
	FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION *storage,
	const void *command_data, size_t command_size,
	FB_GFX3_VK_PIPELINE pipeline, uint32_t invocation_count,
	uint32_t invocation_count_y);
int fb_gfx3_vk_surface_validate(FB_GFX3_VULKAN_RUNTIME *runtime,
	FB_GFX3_VULKAN_SURFACE *surface,
	FB_GFX3_VULKAN_IMPLEMENTATION **implementation,
	FB_GFX3_VULKAN_SURFACE_IMPLEMENTATION **surface_implementation);
int fb_gfx3_vk_swapchain_create(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);
void fb_gfx3_vk_swapchain_destroy(
	FB_GFX3_VULKAN_IMPLEMENTATION *implementation);

#endif
#endif

/* end of gfx3_vulkan_private.h */
