/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: linux/gfx3_platform_drm.c

    Purpose:

        Own a Linux KMS display and GLES context without a window toolkit.

    Responsibilities:

        - select a connected DRM connector and compatible CRTC
        - create a GBM scanout surface and EGL OpenGL ES context
        - present frames through DRM page flips
        - restore the previous CRTC state at renderer shutdown

    This file intentionally does NOT contain:

        - SDL, X11, Wayland, or FreeBASIC drawing commands
        - GLES shaders or renderer resources
        - persistent display mode changes after renderer shutdown
*/

#include "gfx3_platform_drm.h"
#include "gfx3_platform_drm_input.h"
#include "../gfx3_input.h"

#include <stdlib.h>
#include <string.h>

int fb_gfx3_platform_drm_should_default(void)
{
	const char *requested = getenv("FBGFX_GFX3_PLATFORM");
	const char *display = getenv("DISPLAY");
	const char *wayland_display = getenv("WAYLAND_DISPLAY");

	if ((requested != NULL) && (strcmp(requested, "drm") == 0))
		return TRUE;
	if ((requested != NULL) && (strcmp(requested, "x11") == 0))
		return FALSE;
	if ((display != NULL) && (display[0] != '\0'))
		return FALSE;
	if ((wayland_display != NULL) && (wayland_display[0] != '\0'))
		return FALSE;
	return TRUE;
}

#if defined(HOST_LINUX) && !defined(DISABLE_OPENGL) && \
	defined(__has_include)
#if __has_include(<xf86drm.h>) && __has_include(<xf86drmMode.h>) && \
	__has_include(<drm_fourcc.h>) && __has_include(<gbm.h>) && \
	__has_include(<EGL/egl.h>) && __has_include(<EGL/eglext.h>)
#define FB_GFX3_HAVE_DRM_EGL_GBM 1
#endif
#endif

#if defined(FB_GFX3_HAVE_DRM_EGL_GBM)

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif
#ifndef EGL_PLATFORM_GBM_KHR
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

#define FB_GFX3_DRM_DEVICE_DEFAULT "/dev/dri/card0"
#define FB_GFX3_DRM_EVENT_TIMEOUT_MS 3000

typedef EGLDisplay (EGLAPIENTRYP FB_GFX3_EGL_GET_PLATFORM_DISPLAY)(
	EGLenum platform, void *native_display, const EGLint *attributes);

typedef struct FB_GFX3_DRM_API {
	void *drm_library;
	void *gbm_library;
	void *egl_library;
	void *gles_library;
	__typeof__(&drmModeGetResources) mode_get_resources;
	__typeof__(&drmModeFreeResources) mode_free_resources;
	__typeof__(&drmModeGetConnector) mode_get_connector;
	__typeof__(&drmModeFreeConnector) mode_free_connector;
	__typeof__(&drmModeGetEncoder) mode_get_encoder;
	__typeof__(&drmModeFreeEncoder) mode_free_encoder;
	__typeof__(&drmModeGetCrtc) mode_get_crtc;
	__typeof__(&drmModeFreeCrtc) mode_free_crtc;
	__typeof__(&drmModeSetCrtc) mode_set_crtc;
	__typeof__(&drmModeAddFB2) mode_add_fb2;
	__typeof__(&drmModeAddFB) mode_add_fb;
	__typeof__(&drmModeRmFB) mode_rm_fb;
	__typeof__(&drmModePageFlip) mode_page_flip;
	__typeof__(&drmHandleEvent) handle_event;
	__typeof__(&drmSetMaster) set_master;
	__typeof__(&drmDropMaster) drop_master;
	__typeof__(&drmIsMaster) is_master;
	__typeof__(&gbm_create_device) gbm_create_device;
	__typeof__(&gbm_device_destroy) gbm_device_destroy;
	__typeof__(&gbm_surface_create) gbm_surface_create;
	__typeof__(&gbm_surface_destroy) gbm_surface_destroy;
	__typeof__(&gbm_surface_lock_front_buffer) gbm_surface_lock_front_buffer;
	__typeof__(&gbm_surface_release_buffer) gbm_surface_release_buffer;
	__typeof__(&gbm_bo_get_handle) gbm_bo_get_handle;
	__typeof__(&gbm_bo_get_stride) gbm_bo_get_stride;
	__typeof__(&eglGetDisplay) egl_get_display;
	__typeof__(&eglInitialize) egl_initialize;
	__typeof__(&eglTerminate) egl_terminate;
	__typeof__(&eglChooseConfig) egl_choose_config;
	__typeof__(&eglBindAPI) egl_bind_api;
	__typeof__(&eglCreateContext) egl_create_context;
	__typeof__(&eglDestroyContext) egl_destroy_context;
	__typeof__(&eglCreateWindowSurface) egl_create_window_surface;
	__typeof__(&eglDestroySurface) egl_destroy_surface;
	__typeof__(&eglMakeCurrent) egl_make_current;
	__typeof__(&eglSwapBuffers) egl_swap_buffers;
	__typeof__(&eglGetProcAddress) egl_get_proc_address;
	FB_GFX3_EGL_GET_PLATFORM_DISPLAY egl_get_platform_display;
} FB_GFX3_DRM_API;

typedef struct FB_GFX3_PLATFORM_DRM {
	FB_GFX3_DRM_API api;
	FB_GFX3_DRM_INPUT_ADAPTER *input_adapter;
	int drm_descriptor;
	int owns_master;
	int initialized_egl;
	int modeset_active;
	int flip_pending;
	int flip_complete;
	uint32_t connector_id;
	uint32_t crtc_id;
	uint32_t connector_ids[1];
	uint32_t current_framebuffer;
	uint32_t pending_framebuffer;
	uint32_t width;
	uint32_t height;
	struct gbm_device *gbm_device;
	struct gbm_surface *gbm_surface;
	struct gbm_bo *current_buffer;
	struct gbm_bo *pending_buffer;
	EGLDisplay egl_display;
	EGLSurface egl_surface;
	EGLContext egl_context;
	drmModeModeInfo mode;
	drmModeCrtc *saved_crtc;
	drmEventContext event_context;
} FB_GFX3_PLATFORM_DRM;

/* ------------------------------------------------------------------------- */
/* Optional runtime libraries                                                */
/* ------------------------------------------------------------------------- */

static int drm_load_function(void *library, const char *name,
	void *destination, size_t destination_size)
{
	void *symbol;

	if ((library == NULL) || (name == NULL) || (destination == NULL) ||
	    (destination_size != sizeof(symbol)))
		return FALSE;
	symbol = dlsym(library, name);
	if (symbol == NULL)
		return FALSE;
	memcpy(destination, &symbol, sizeof(symbol));
	return TRUE;
}

static void *drm_open_library(const char *first, const char *second)
{
	void *library = dlopen(first, RTLD_NOW | RTLD_LOCAL);

	if ((library == NULL) && (second != NULL))
		library = dlopen(second, RTLD_NOW | RTLD_LOCAL);
	return library;
}

static int drm_library_has_symbols(void *library,
	const char *const *symbol_names, size_t symbol_count)
{
	size_t index;

	if ((library == NULL) || (symbol_names == NULL))
		return FALSE;
	for (index = 0; index < symbol_count; index++) {
		if (dlsym(library, symbol_names[index]) == NULL)
			return FALSE;
	}
	return TRUE;
}

static int drm_mali_library_has_required_apis(void *library)
{
	static const char *const required_symbols[] = {
		"gbm_create_device",
		"gbm_device_destroy",
		"gbm_surface_create",
		"gbm_surface_destroy",
		"gbm_surface_lock_front_buffer",
		"gbm_surface_release_buffer",
		"gbm_bo_get_handle",
		"gbm_bo_get_stride",
		"eglGetDisplay",
		"eglInitialize",
		"eglTerminate",
		"eglChooseConfig",
		"eglBindAPI",
		"eglCreateContext",
		"eglDestroyContext",
		"eglCreateWindowSurface",
		"eglDestroySurface",
		"eglMakeCurrent",
		"eglSwapBuffers",
		"eglGetProcAddress",
		"glGetString"
	};

	return drm_library_has_symbols(library, required_symbols,
		sizeof(required_symbols) / sizeof(required_symbols[0]));
}

static int drm_load_api(FB_GFX3_DRM_API *api)
{
	void *mali_library;
	int loaded = TRUE;

	if (api == NULL)
		return FALSE;
	memset(api, 0, sizeof(*api));
	api->drm_library = drm_open_library("libdrm.so.2", "libdrm.so");
	/*
		Some handheld images install Mesa's versioned EGL/GBM libraries beside
		the vendor's unversioned Mali stack. Mixing those providers yields a
		GBM device that the selected EGL implementation cannot use. Select the
		Mali library only when it exports the complete GBM and EGL entry points
		needed here; otherwise keep the standard Linux libraries.
	*/
	mali_library = drm_open_library("libMali.so", "libmali.so");
	if (drm_mali_library_has_required_apis(mali_library)) {
		api->gbm_library = mali_library;
		api->egl_library = drm_open_library("libMali.so", "libmali.so");
		api->gles_library = drm_open_library("libMali.so", "libmali.so");
	} else {
		if (mali_library != NULL)
			dlclose(mali_library);
		api->gbm_library = drm_open_library("libgbm.so.1", "libgbm.so");
		api->egl_library = drm_open_library("libEGL.so.1", "libEGL.so");
		api->gles_library = drm_open_library("libGLESv2.so.2",
			"libGLESv2.so");
	}
	if ((api->drm_library == NULL) || (api->gbm_library == NULL) ||
	    (api->egl_library == NULL) || (api->gles_library == NULL))
		return FALSE;

#define FB_GFX3_DRM_LOAD(library, field, symbol_name) \
	do { \
		if (!drm_load_function((library), (symbol_name), &api->field, \
		    sizeof(api->field))) \
			loaded = FALSE; \
	} while (0)
	FB_GFX3_DRM_LOAD(api->drm_library, mode_get_resources,
		"drmModeGetResources");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_free_resources,
		"drmModeFreeResources");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_get_connector,
		"drmModeGetConnector");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_free_connector,
		"drmModeFreeConnector");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_get_encoder,
		"drmModeGetEncoder");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_free_encoder,
		"drmModeFreeEncoder");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_get_crtc, "drmModeGetCrtc");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_free_crtc, "drmModeFreeCrtc");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_set_crtc, "drmModeSetCrtc");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_add_fb2, "drmModeAddFB2");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_add_fb, "drmModeAddFB");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_rm_fb, "drmModeRmFB");
	FB_GFX3_DRM_LOAD(api->drm_library, mode_page_flip, "drmModePageFlip");
	FB_GFX3_DRM_LOAD(api->drm_library, handle_event, "drmHandleEvent");
	FB_GFX3_DRM_LOAD(api->drm_library, set_master, "drmSetMaster");
	FB_GFX3_DRM_LOAD(api->drm_library, drop_master, "drmDropMaster");
	FB_GFX3_DRM_LOAD(api->drm_library, is_master, "drmIsMaster");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_create_device, "gbm_create_device");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_device_destroy,
		"gbm_device_destroy");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_surface_create,
		"gbm_surface_create");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_surface_destroy,
		"gbm_surface_destroy");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_surface_lock_front_buffer,
		"gbm_surface_lock_front_buffer");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_surface_release_buffer,
		"gbm_surface_release_buffer");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_bo_get_handle, "gbm_bo_get_handle");
	FB_GFX3_DRM_LOAD(api->gbm_library, gbm_bo_get_stride, "gbm_bo_get_stride");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_get_display, "eglGetDisplay");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_initialize, "eglInitialize");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_terminate, "eglTerminate");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_choose_config, "eglChooseConfig");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_bind_api, "eglBindAPI");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_create_context, "eglCreateContext");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_destroy_context,
		"eglDestroyContext");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_create_window_surface,
		"eglCreateWindowSurface");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_destroy_surface,
		"eglDestroySurface");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_make_current, "eglMakeCurrent");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_swap_buffers, "eglSwapBuffers");
	FB_GFX3_DRM_LOAD(api->egl_library, egl_get_proc_address,
		"eglGetProcAddress");
#undef FB_GFX3_DRM_LOAD
	if (loaded) {
		if (!drm_load_function(api->egl_library, "eglGetPlatformDisplay",
		    &api->egl_get_platform_display,
		    sizeof(api->egl_get_platform_display))) {
			__eglMustCastToProperFunctionPointerType procedure =
				api->egl_get_proc_address("eglGetPlatformDisplayEXT");
			if ((procedure != NULL) &&
			    (sizeof(procedure) == sizeof(api->egl_get_platform_display)))
				memcpy(&api->egl_get_platform_display, &procedure,
					sizeof(procedure));
		}
	}
	return loaded;
}

static void drm_unload_api(FB_GFX3_DRM_API *api)
{
	if (api == NULL)
		return;
	if (api->gles_library != NULL)
		dlclose(api->gles_library);
	if (api->egl_library != NULL)
		dlclose(api->egl_library);
	if (api->gbm_library != NULL)
		dlclose(api->gbm_library);
	if (api->drm_library != NULL)
		dlclose(api->drm_library);
	memset(api, 0, sizeof(*api));
}

/* ------------------------------------------------------------------------- */
/* Connector and CRTC selection                                               */
/* ------------------------------------------------------------------------- */

static int drm_find_display(FB_GFX3_PLATFORM_DRM *platform)
{
	drmModeRes *resources;
	int connector_index;
	int result = FB_GFX3_UNSUPPORTED;

	resources = platform->api.mode_get_resources(platform->drm_descriptor);
	if (resources == NULL)
		return FB_GFX3_FAILED;
	for (connector_index = 0; connector_index < resources->count_connectors;
	    connector_index++) {
		drmModeConnector *connector = platform->api.mode_get_connector(
			platform->drm_descriptor, resources->connectors[connector_index]);
		int encoder_index;
		int selected_mode = -1;
		uint32_t selected_crtc = 0u;
		int mode_index;

		if (connector == NULL)
			continue;
		if ((connector->connection != DRM_MODE_CONNECTED) ||
		    (connector->count_modes <= 0)) {
			platform->api.mode_free_connector(connector);
			continue;
		}
		for (mode_index = 0; mode_index < connector->count_modes;
		    mode_index++) {
			if (selected_mode < 0)
				selected_mode = mode_index;
			if ((connector->modes[mode_index].type &
			    DRM_MODE_TYPE_PREFERRED) != 0u) {
				selected_mode = mode_index;
				break;
			}
		}
		for (encoder_index = 0; encoder_index < connector->count_encoders;
		    encoder_index++) {
			drmModeEncoder *encoder = platform->api.mode_get_encoder(
				platform->drm_descriptor,
				connector->encoders[encoder_index]);
			uint32_t crtc_index;

			if (encoder == NULL)
				continue;
			for (crtc_index = 0u;
			    (crtc_index < (uint32_t)resources->count_crtcs) &&
			    (crtc_index < 32u); crtc_index++) {
				if ((encoder->crtc_id == resources->crtcs[crtc_index]) &&
				    ((encoder->possible_crtcs & (1u << crtc_index)) != 0u)) {
					selected_crtc = encoder->crtc_id;
					break;
				}
			}
			if (selected_crtc == 0u) {
				for (crtc_index = 0u;
				    (crtc_index < (uint32_t)resources->count_crtcs) &&
				    (crtc_index < 32u); crtc_index++) {
					if ((encoder->possible_crtcs &
					    (1u << crtc_index)) != 0u) {
						selected_crtc = resources->crtcs[crtc_index];
						break;
					}
				}
			}
			platform->api.mode_free_encoder(encoder);
			if (selected_crtc != 0u)
				break;
		}
		if ((selected_mode >= 0) && (selected_crtc != 0u)) {
			platform->mode = connector->modes[selected_mode];
			platform->connector_id = connector->connector_id;
			platform->crtc_id = selected_crtc;
			result = FB_GFX3_OK;
			platform->api.mode_free_connector(connector);
			break;
		}
		platform->api.mode_free_connector(connector);
	}
	platform->api.mode_free_resources(resources);
	return result;
}

static int drm_probe_opengl(void)
{
	FB_GFX3_PLATFORM_DRM platform;
	const char *device = getenv("FBGFX_DRM_DEVICE");
	int result = FB_GFX3_UNSUPPORTED;

	memset(&platform, 0, sizeof(platform));
	platform.drm_descriptor = -1;
	if (!drm_load_api(&platform.api)) {
		drm_unload_api(&platform.api);
		return FB_GFX3_UNSUPPORTED;
	}
	platform.drm_descriptor = open(
		((device != NULL) && (device[0] != '\0')) ? device :
		FB_GFX3_DRM_DEVICE_DEFAULT, O_RDWR | O_CLOEXEC);
	if (platform.drm_descriptor >= 0) {
		result = drm_find_display(&platform);
		close(platform.drm_descriptor);
	}
	drm_unload_api(&platform.api);
	return result;
}

/* ------------------------------------------------------------------------- */
/* Page flip lifecycle                                                        */
/* ------------------------------------------------------------------------- */

static int drm_add_framebuffer(FB_GFX3_PLATFORM_DRM *platform,
	struct gbm_bo *buffer, uint32_t *framebuffer)
{
	uint32_t handles[4] = { 0u, 0u, 0u, 0u };
	uint32_t strides[4] = { 0u, 0u, 0u, 0u };
	uint32_t offsets[4] = { 0u, 0u, 0u, 0u };
	union gbm_bo_handle handle;

	if ((platform == NULL) || (buffer == NULL) || (framebuffer == NULL))
		return FB_GFX3_INVALID;
	handle = platform->api.gbm_bo_get_handle(buffer);
	strides[0] = platform->api.gbm_bo_get_stride(buffer);
	if ((handle.u32 == 0u) || (strides[0] == 0u))
		return FB_GFX3_FAILED;
	handles[0] = handle.u32;
	if (platform->api.mode_add_fb2(platform->drm_descriptor, platform->width,
	    platform->height, DRM_FORMAT_XRGB8888, handles, strides, offsets,
	    framebuffer, 0u) == 0)
		return FB_GFX3_OK;
	if (platform->api.mode_add_fb(platform->drm_descriptor, platform->width,
	    platform->height, 24u, 32u, strides[0], handle.u32, framebuffer) == 0)
		return FB_GFX3_OK;
	return FB_GFX3_FAILED;
}

static void drm_page_flip_complete(int descriptor, unsigned int frame,
	unsigned int seconds, unsigned int microseconds, void *opaque)
{
	FB_GFX3_PLATFORM_DRM *platform = (FB_GFX3_PLATFORM_DRM *)opaque;

	(void)descriptor;
	(void)frame;
	(void)seconds;
	(void)microseconds;
	if (platform == NULL)
		return;
	if ((platform->current_buffer != NULL) &&
	    (platform->gbm_surface != NULL))
		platform->api.gbm_surface_release_buffer(platform->gbm_surface,
			platform->current_buffer);
	if (platform->current_framebuffer != 0u)
		(void)platform->api.mode_rm_fb(platform->drm_descriptor,
			platform->current_framebuffer);
	platform->current_buffer = platform->pending_buffer;
	platform->current_framebuffer = platform->pending_framebuffer;
	platform->pending_buffer = NULL;
	platform->pending_framebuffer = 0u;
	platform->flip_pending = FALSE;
	platform->flip_complete = TRUE;
}

static int drm_wait_page_flip(FB_GFX3_PLATFORM_DRM *platform)
{
	struct pollfd descriptor;
	int result;

	if (!platform->flip_pending)
		return FB_GFX3_OK;
	descriptor.fd = platform->drm_descriptor;
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	while (platform->flip_pending) {
		result = poll(&descriptor, 1u, FB_GFX3_DRM_EVENT_TIMEOUT_MS);
		if (result < 0) {
			if (errno == EINTR)
				continue;
			return FB_GFX3_FAILED;
		}
		if ((result == 0) || ((descriptor.revents & POLLIN) == 0))
			return FB_GFX3_FAILED;
		if (platform->api.handle_event(platform->drm_descriptor,
		    &platform->event_context) != 0)
			return FB_GFX3_FAILED;
	}
	return platform->flip_complete ? FB_GFX3_OK : FB_GFX3_FAILED;
}

static int drm_present(FB_GFX3_PLATFORM_DRM *platform)
{
	struct gbm_bo *buffer;
	uint32_t framebuffer = 0u;
	int result;

	if ((platform == NULL) || (platform->gbm_surface == NULL))
		return FB_GFX3_INVALID;
	if (!platform->api.egl_swap_buffers(platform->egl_display,
	    platform->egl_surface))
		return FB_GFX3_FAILED;
	buffer = platform->api.gbm_surface_lock_front_buffer(platform->gbm_surface);
	if (buffer == NULL)
		return FB_GFX3_FAILED;
	result = drm_add_framebuffer(platform, buffer, &framebuffer);
	if (result != FB_GFX3_OK) {
		platform->api.gbm_surface_release_buffer(platform->gbm_surface, buffer);
		return result;
	}
	if (!platform->modeset_active) {
		result = platform->api.mode_set_crtc(platform->drm_descriptor,
			platform->crtc_id, framebuffer, 0u, 0u, platform->connector_ids,
			1, &platform->mode);
		if (result != 0) {
			(void)platform->api.mode_rm_fb(platform->drm_descriptor, framebuffer);
			platform->api.gbm_surface_release_buffer(platform->gbm_surface,
				buffer);
			return FB_GFX3_FAILED;
		}
		platform->current_buffer = buffer;
		platform->current_framebuffer = framebuffer;
		platform->modeset_active = TRUE;
		return FB_GFX3_OK;
	}
	result = drm_wait_page_flip(platform);
	if (result != FB_GFX3_OK) {
		(void)platform->api.mode_rm_fb(platform->drm_descriptor, framebuffer);
		platform->api.gbm_surface_release_buffer(platform->gbm_surface, buffer);
		return result;
	}
	platform->flip_complete = FALSE;
	platform->pending_buffer = buffer;
	platform->pending_framebuffer = framebuffer;
	platform->flip_pending = TRUE;
	if (platform->api.mode_page_flip(platform->drm_descriptor, platform->crtc_id,
	    framebuffer, DRM_MODE_PAGE_FLIP_EVENT, platform) != 0) {
		platform->flip_pending = FALSE;
		platform->pending_buffer = NULL;
		platform->pending_framebuffer = 0u;
		(void)platform->api.mode_rm_fb(platform->drm_descriptor, framebuffer);
		platform->api.gbm_surface_release_buffer(platform->gbm_surface, buffer);
		return FB_GFX3_FAILED;
	}
	return drm_wait_page_flip(platform);
}

/* ------------------------------------------------------------------------- */
/* EGL context and platform operations                                        */
/* ------------------------------------------------------------------------- */

static int drm_create_opengl(void **destination,
	const FB_GFX3_PLATFORM_OPENGL_CONFIG *config)
{
	static const EGLint config_attributes[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
		EGL_NATIVE_VISUAL_ID, GBM_FORMAT_XRGB8888,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
		EGL_NONE
	};
	static const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE
	};
	FB_GFX3_PLATFORM_DRM *platform;
	const char *device = getenv("FBGFX_DRM_DEVICE");
	EGLConfig egl_config = NULL;
	EGLint config_count = 0;
	EGLint major = 0;
	EGLint minor = 0;
	int result;

	if ((destination == NULL) || (config == NULL) ||
	    (config->width == 0u) || (config->height == 0u) ||
	    (config->major_version != 3u))
		return FB_GFX3_UNSUPPORTED;
	*destination = NULL;
	platform = (FB_GFX3_PLATFORM_DRM *)calloc(1, sizeof(*platform));
	if (platform == NULL)
		return FB_GFX3_OUT_OF_MEMORY;
	platform->drm_descriptor = -1;
	platform->egl_display = EGL_NO_DISPLAY;
	platform->egl_surface = EGL_NO_SURFACE;
	platform->egl_context = EGL_NO_CONTEXT;
	if (!drm_load_api(&platform->api))
		goto unsupported;
	platform->drm_descriptor = open(
		((device != NULL) && (device[0] != '\0')) ? device :
		FB_GFX3_DRM_DEVICE_DEFAULT, O_RDWR | O_CLOEXEC);
	if (platform->drm_descriptor < 0)
		goto unsupported;
	result = drm_find_display(platform);
	if (result != FB_GFX3_OK)
		goto unsupported;
	platform->width = platform->mode.hdisplay;
	platform->height = platform->mode.vdisplay;
	if ((platform->width == 0u) || (platform->height == 0u) ||
	    (platform->width > INT_MAX) || (platform->height > INT_MAX))
		goto unsupported;
	platform->connector_ids[0] = platform->connector_id;
	platform->saved_crtc = platform->api.mode_get_crtc(platform->drm_descriptor,
		platform->crtc_id);
	if (platform->saved_crtc == NULL)
		goto unsupported;
	if (platform->api.is_master(platform->drm_descriptor) != 1) {
		if (platform->api.set_master(platform->drm_descriptor) != 0)
			goto unsupported;
		platform->owns_master = TRUE;
	}
	platform->gbm_device = platform->api.gbm_create_device(
		platform->drm_descriptor);
	if (platform->gbm_device == NULL)
		goto unsupported;
	platform->gbm_surface = platform->api.gbm_surface_create(
		platform->gbm_device, platform->width, platform->height,
		GBM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (platform->gbm_surface == NULL)
		goto unsupported;
	if (platform->api.egl_get_platform_display != NULL)
		platform->egl_display = platform->api.egl_get_platform_display(
			EGL_PLATFORM_GBM_KHR, platform->gbm_device, NULL);
	if (platform->egl_display == EGL_NO_DISPLAY)
		platform->egl_display = platform->api.egl_get_display(
			(EGLNativeDisplayType)platform->gbm_device);
	if ((platform->egl_display == EGL_NO_DISPLAY) ||
	    !platform->api.egl_initialize(platform->egl_display, &major, &minor))
		goto unsupported;
	platform->initialized_egl = TRUE;
	if (!platform->api.egl_bind_api(EGL_OPENGL_ES_API) ||
	    !platform->api.egl_choose_config(platform->egl_display,
		config_attributes, &egl_config, 1, &config_count) ||
	    (config_count != 1) || (egl_config == NULL))
		goto unsupported;
	platform->egl_surface = platform->api.egl_create_window_surface(
		platform->egl_display, egl_config,
		(EGLNativeWindowType)platform->gbm_surface, NULL);
	if (platform->egl_surface == EGL_NO_SURFACE)
		goto unsupported;
	platform->egl_context = platform->api.egl_create_context(
		platform->egl_display, egl_config, EGL_NO_CONTEXT, context_attributes);
	if (platform->egl_context == EGL_NO_CONTEXT)
		goto unsupported;
	if (!platform->api.egl_make_current(platform->egl_display,
	    platform->egl_surface, platform->egl_surface, platform->egl_context))
		goto unsupported;
	memset(&platform->event_context, 0, sizeof(platform->event_context));
	platform->event_context.version = 2;
	platform->event_context.page_flip_handler = drm_page_flip_complete;
	platform->input_adapter = fb_gfx3_platform_drm_input_create(
		(FB_GFX3_INPUT_STATE *)config->input);
	fb_gfx3_input_platform_window_info((FB_GFX3_INPUT_STATE *)config->input,
		0u, 0u, 0, 0, (int)platform->width, (int)platform->height);
	fb_gfx3_input_platform_focus((FB_GFX3_INPUT_STATE *)config->input, TRUE);
	*destination = platform;
	return FB_GFX3_OK;

unsupported:
	{
		const FB_GFX3_PLATFORM_VTABLE *vtable = fb_gfx3_platform_drm();
		if ((vtable != NULL) && (vtable->destroy != NULL))
			vtable->destroy(platform);
	}
	return FB_GFX3_UNSUPPORTED;
}

static int drm_create_window(void **destination,
	const FB_GFX3_PLATFORM_WINDOW_CONFIG *config)
{
	(void)config;
	if (destination != NULL)
		*destination = NULL;
	return FB_GFX3_UNSUPPORTED;
}

static int drm_native_handles(void *opaque, uintptr_t *instance,
	uintptr_t *window)
{
	(void)opaque;
	(void)instance;
	(void)window;
	return FB_GFX3_UNSUPPORTED;
}

static int drm_load_opengl_function(void *opaque, const char *name,
	void *destination, size_t destination_size)
{
	FB_GFX3_PLATFORM_DRM *platform = (FB_GFX3_PLATFORM_DRM *)opaque;
	__eglMustCastToProperFunctionPointerType procedure;
	void *symbol;

	if ((platform == NULL) || (name == NULL) || (name[0] == '\0') ||
	    (destination == NULL) || (destination_size != sizeof(symbol)))
		return FB_GFX3_INVALID;
	procedure = platform->api.egl_get_proc_address(name);
	if (procedure != NULL) {
		if (sizeof(procedure) != destination_size)
			return FB_GFX3_UNSUPPORTED;
		memcpy(destination, &procedure, sizeof(procedure));
		return FB_GFX3_OK;
	}
	symbol = dlsym(platform->api.gles_library, name);
	if (symbol == NULL)
		return FB_GFX3_UNSUPPORTED;
	memcpy(destination, &symbol, sizeof(symbol));
	return FB_GFX3_OK;
}

static int drm_client_size(void *opaque, uint32_t *width, uint32_t *height)
{
	FB_GFX3_PLATFORM_DRM *platform = (FB_GFX3_PLATFORM_DRM *)opaque;

	if ((platform == NULL) || (width == NULL) || (height == NULL))
		return FB_GFX3_INVALID;
	*width = platform->width;
	*height = platform->height;
	return FB_GFX3_OK;
}

static int drm_desktop_info(ssize_t *width, ssize_t *height, ssize_t *depth,
	ssize_t *refresh)
{
	(void)width;
	(void)height;
	(void)depth;
	(void)refresh;
	return FB_GFX3_UNSUPPORTED;
}

static int drm_swap_buffers(void *opaque)
{
	return drm_present((FB_GFX3_PLATFORM_DRM *)opaque);
}

static void drm_pump_events(void *opaque)
{
	FB_GFX3_PLATFORM_DRM *platform = (FB_GFX3_PLATFORM_DRM *)opaque;

	if (platform != NULL)
		fb_gfx3_platform_drm_input_pump(platform->input_adapter);
}

static int drm_show_window(void *opaque)
{
	return (opaque != NULL) ? FB_GFX3_OK : FB_GFX3_INVALID;
}

static int drm_set_window_title(void *opaque, const char *title)
{
	if ((opaque == NULL) || (title == NULL))
		return FB_GFX3_INVALID;
	return FB_GFX3_OK;
}

static void drm_destroy(void *opaque)
{
	FB_GFX3_PLATFORM_DRM *platform = (FB_GFX3_PLATFORM_DRM *)opaque;

	if (platform == NULL)
		return;
	(void)drm_wait_page_flip(platform);
	fb_gfx3_platform_drm_input_destroy(platform->input_adapter);
	platform->input_adapter = NULL;
	if (platform->modeset_active && (platform->saved_crtc != NULL)) {
		if (platform->saved_crtc->mode_valid &&
		    (platform->saved_crtc->buffer_id != 0u)) {
			(void)platform->api.mode_set_crtc(platform->drm_descriptor,
				platform->saved_crtc->crtc_id,
				platform->saved_crtc->buffer_id,
				platform->saved_crtc->x, platform->saved_crtc->y,
				platform->connector_ids, 1, &platform->saved_crtc->mode);
		} else {
			(void)platform->api.mode_set_crtc(platform->drm_descriptor,
				platform->crtc_id, 0u, 0u, 0u, NULL, 0, NULL);
		}
	}
	if (platform->pending_buffer != NULL)
		platform->api.gbm_surface_release_buffer(platform->gbm_surface,
			platform->pending_buffer);
	if (platform->pending_framebuffer != 0u)
		(void)platform->api.mode_rm_fb(platform->drm_descriptor,
			platform->pending_framebuffer);
	if (platform->current_buffer != NULL)
		platform->api.gbm_surface_release_buffer(platform->gbm_surface,
			platform->current_buffer);
	if (platform->current_framebuffer != 0u)
		(void)platform->api.mode_rm_fb(platform->drm_descriptor,
			platform->current_framebuffer);
	if ((platform->egl_display != EGL_NO_DISPLAY) &&
	    (platform->api.egl_make_current != NULL))
		(void)platform->api.egl_make_current(platform->egl_display,
			EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if ((platform->egl_context != EGL_NO_CONTEXT) &&
	    (platform->api.egl_destroy_context != NULL))
		(void)platform->api.egl_destroy_context(platform->egl_display,
			platform->egl_context);
	if ((platform->egl_surface != EGL_NO_SURFACE) &&
	    (platform->api.egl_destroy_surface != NULL))
		(void)platform->api.egl_destroy_surface(platform->egl_display,
			platform->egl_surface);
	if (platform->initialized_egl && (platform->api.egl_terminate != NULL))
		(void)platform->api.egl_terminate(platform->egl_display);
	if ((platform->gbm_surface != NULL) &&
	    (platform->api.gbm_surface_destroy != NULL))
		platform->api.gbm_surface_destroy(platform->gbm_surface);
	if ((platform->gbm_device != NULL) &&
	    (platform->api.gbm_device_destroy != NULL))
		platform->api.gbm_device_destroy(platform->gbm_device);
	if ((platform->saved_crtc != NULL) &&
	    (platform->api.mode_free_crtc != NULL))
		platform->api.mode_free_crtc(platform->saved_crtc);
	if (platform->owns_master && (platform->drm_descriptor >= 0) &&
	    (platform->api.drop_master != NULL))
		(void)platform->api.drop_master(platform->drm_descriptor);
	if (platform->drm_descriptor >= 0)
		close(platform->drm_descriptor);
	drm_unload_api(&platform->api);
	free(platform);
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_drm = {
	"DRM/GBM/EGL",
	drm_probe_opengl,
	drm_create_window,
	drm_create_opengl,
	drm_native_handles,
	drm_destroy,
	drm_load_opengl_function,
	drm_client_size,
	drm_desktop_info,
	drm_swap_buffers,
	drm_pump_events,
	drm_show_window,
	drm_set_window_title
};

#else

static int drm_probe_opengl(void)
{
	return FB_GFX3_UNSUPPORTED;
}

static const FB_GFX3_PLATFORM_VTABLE __fb_gfx3_platform_drm = {
	"DRM/GBM/EGL unavailable",
	drm_probe_opengl,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

#endif

const FB_GFX3_PLATFORM_VTABLE *fb_gfx3_platform_drm(void)
{
	return &__fb_gfx3_platform_drm;
}

/* end of linux/gfx3_platform_drm.c */
