'' Project: FreeBASIC SDL3 bindings
'' File: SDL.bi
'' Purpose: Declare the SDL3-3.4.18 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''
'' Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
''    claim that you wrote the original software. If you use this software
''    in a product, an acknowledgment in the product documentation would be
''    appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
''    misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL3"

#include once "crt/long.bi"
#include once "crt/stdarg.bi"
#ifdef __FB_WIN32__
	#include once "crt/process.bi"
#endif




extern "C"

type SDLTest_TestSuiteRunner as SDLTest_TestSuiteRunner_
type SDL_AsyncIO as SDL_AsyncIO_
type SDL_AsyncIOQueue as SDL_AsyncIOQueue_
type SDL_AudioStream as SDL_AudioStream_
type SDL_Camera as SDL_Camera_
type SDL_Condition as SDL_Condition_
type SDL_Cursor as SDL_Cursor_
type SDL_DisplayModeData as SDL_DisplayModeData_
type SDL_Environment as SDL_Environment_
type SDL_GPUBuffer as SDL_GPUBuffer_
type SDL_GPUCommandBuffer as SDL_GPUCommandBuffer_
type SDL_GPUComputePass as SDL_GPUComputePass_
type SDL_GPUComputePipeline as SDL_GPUComputePipeline_
type SDL_GPUCopyPass as SDL_GPUCopyPass_
type SDL_GPUDevice as SDL_GPUDevice_
type SDL_GPUFence as SDL_GPUFence_
type SDL_GPUGraphicsPipeline as SDL_GPUGraphicsPipeline_
type SDL_GPURenderPass as SDL_GPURenderPass_
type SDL_GPURenderState as SDL_GPURenderState_
type SDL_GPUSampler as SDL_GPUSampler_
type SDL_GPUShader as SDL_GPUShader_
type SDL_GPUTexture as SDL_GPUTexture_
type SDL_GPUTransferBuffer as SDL_GPUTransferBuffer_
type SDL_Gamepad as SDL_Gamepad_
type SDL_Haptic as SDL_Haptic_
type SDL_IOStream as SDL_IOStream_
type SDL_Joystick as SDL_Joystick_
type SDL_Mutex as SDL_Mutex_
type SDL_Process as SDL_Process_
type SDL_RWLock as SDL_RWLock_
type SDL_Renderer as SDL_Renderer_
type SDL_Semaphore as SDL_Semaphore_
type SDL_Sensor as SDL_Sensor_
type SDL_SharedObject as SDL_SharedObject_
type SDL_Storage as SDL_Storage_
type SDL_Thread as SDL_Thread_
type SDL_Tray as SDL_Tray_
type SDL_TrayEntry as SDL_TrayEntry_
type SDL_TrayMenu as SDL_TrayMenu_
type SDL_Window as SDL_Window_
type SDL_hid_device as SDL_hid_device_

#define SDL_h_
'' -------------------------------------------------------------------------
'' SDL_stdinc.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_platform_defines.h
'' -------------------------------------------------------------------------
#ifdef __FB_LINUX__
	const SDL_PLATFORM_LINUX = 1
#elseif defined(__FB_FREEBSD__)
	const SDL_PLATFORM_FREEBSD = 1
#endif

#if defined(__FB_DARWIN__) or defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__)
	const SDL_PLATFORM_UNIX = 1
#endif

#ifdef __FB_OPENBSD__
	const SDL_PLATFORM_OPENBSD = 1
#elseif defined(__FB_NETBSD__)
	const SDL_PLATFORM_NETBSD = 1
#elseif defined(__FB_DARWIN__)
	const SDL_PLATFORM_APPLE = 1
	#define __has_extension(x) 0
	#undef __has_extension
	const TARGET_OS_MACCATALYST = 0
	const TARGET_OS_IOS = 0
	const TARGET_OS_IPHONE = 0
	const TARGET_OS_TV = 0
	const TARGET_OS_SIMULATOR = 0
	const TARGET_OS_VISION = 0
	const SDL_PLATFORM_MACOS = 1
#elseif defined(__FB_WIN32__)
	const SDL_PLATFORM_WINDOWS = 1
	const HAVE_WINAPIFAMILY_H = 0
	const WINAPI_FAMILY_WINRT = 0
	const SDL_WINAPI_FAMILY_PHONE = 0
	const SDL_PLATFORM_WIN32 = 1
#endif

const SDL_SIZE_MAX = cuint(-1)
#define SDL_arraysize(array) (ubound(array) - lbound(array) + 1)
#define SDL_STRINGIFY_ARG(arg) #arg
#define SDL_reinterpret_cast(type, expression) cast(type, expression)
#define SDL_static_cast(type, expression) cast(type, expression)
#define SDL_const_cast(type, expression) cast(type, expression)
#define SDL_FOURCC(A, B, C, D) ((((SDL_static_cast(Uint32, SDL_static_cast(Uint8, (A))) shl 0) or (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (B))) shl 8)) or (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (C))) shl 16)) or (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (D))) shl 24))

#if (defined(__FB_WIN32__) and defined(__FB_64BIT__)) or (not defined(__FB_64BIT__))
	#define SDL_SINT64_C(c) c##LL
	#define SDL_UINT64_C(c) c##ULL
#else
	#define SDL_SINT64_C(c) c##LL
	#define SDL_UINT64_C(c) c##ULL
#endif

type Sint8 as byte
const SDL_MAX_SINT8 = cast(Sint8, &h7F)
const SDL_MIN_SINT8 = cast(Sint8, not &h7F)
type Uint8 as ubyte
const SDL_MAX_UINT8 = cast(Uint8, &hFF)
const SDL_MIN_UINT8 = cast(Uint8, &h00)
type Sint16 as short
const SDL_MAX_SINT16 = cast(Sint16, &h7FFF)
const SDL_MIN_SINT16 = cast(Sint16, not &h7FFF)
type Uint16 as ushort
const SDL_MAX_UINT16 = cast(Uint16, &hFFFF)
const SDL_MIN_UINT16 = cast(Uint16, &h0000)
type Sint32 as long
const SDL_MAX_SINT32 = cast(Sint32, &h7FFFFFFF)
const SDL_MIN_SINT32 = cast(Sint32, not &h7FFFFFFF)
type Uint32 as ulong
const SDL_MAX_UINT32 = cast(Uint32, &hFFFFFFFFu)
const SDL_MIN_UINT32 = cast(Uint32, &h00000000)
type Sint64 as longint
#define SDL_MAX_SINT64 SDL_SINT64_C(&h7FFFFFFFFFFFFFFF)
#define SDL_MIN_SINT64 (not SDL_SINT64_C(&h7FFFFFFFFFFFFFFF))
type Uint64 as ulongint
#define SDL_MAX_UINT64 SDL_UINT64_C(&hFFFFFFFFFFFFFFFF)
#define SDL_MIN_UINT64 SDL_UINT64_C(&h0000000000000000)
type SDL_Time as Sint64
#define SDL_MAX_TIME SDL_MAX_SINT64
#define SDL_MIN_TIME SDL_MIN_SINT64
const SDL_FLT_EPSILON = 1.1920928955078125e-07f

#define SDL_PRIs64 SDL_PRILLd
#define SDL_PRIu64 SDL_PRILLu
#define SDL_PRIx64 SDL_PRILLx
#define SDL_PRIX64_ SDL_PRILLX_

#define SDL_PRIs32 "d"
#define SDL_PRIu32 "u"
#define SDL_PRIx32 "x"
#define SDL_PRIX32_ "X"

#ifdef __FB_WIN32__
	#define SDL_PRILL_PREFIX "I64"
#else
	#define SDL_PRILL_PREFIX "ll"
#endif

#define SDL_PRILLd SDL_PRILL_PREFIX "d"
#define SDL_PRILLu SDL_PRILL_PREFIX "u"
#define SDL_PRILLx SDL_PRILL_PREFIX "x"
#define SDL_PRILLX_ SDL_PRILL_PREFIX "X"
#define SDL_IN_BYTECAP(x)
#define SDL_INOUT_Z_CAP(x)
#define SDL_OUT_Z_CAP(x)
#define SDL_OUT_CAP(x)
#define SDL_OUT_BYTECAP(x)
#define SDL_OUT_Z_BYTECAP(x)
#define SDL_PRINTF_FORMAT_STRING
#define SDL_SCANF_FORMAT_STRING
#define SDL_WPRINTF_VARARG_FUNC(fmtargnumber)
#define SDL_WPRINTF_VARARG_FUNCV(fmtargnumber)

type SDL_alignment_test
	a as Uint8
	b as any ptr
end type

type SDL_DUMMY_ENUM as long
enum
	DUMMY_ENUM_VALUE
end enum
#define SDL_HAS_BUILTIN(x) 0

#ifdef __FB_WIN32__
	#define SDL_DECLSPEC
#endif

#define SDLCALL
#define SDL_ANALYZER_NORETURN


const SDL_HAS_FALLTHROUGH = 0
#macro SDL_FALLTHROUGH
	scope
	end scope
#endmacro
#undef SDL_HAS_FALLTHROUGH
#macro SDL_INIT_INTERFACE(iface)
	scope
		SDL_zerop(iface)
		(iface)->version = sizeof(*(iface))
	end scope
#endmacro
#define SDL_stack_alloc(type, count) cptr(type ptr, SDL_malloc(sizeof(type) * (count)))
#define SDL_stack_free(data) SDL_free(data)

declare function SDL_malloc(byval size as uinteger) as any ptr
declare function SDL_calloc(byval nmemb as uinteger, byval size as uinteger) as any ptr
declare function SDL_realloc(byval mem as any ptr, byval size as uinteger) as any ptr
declare sub SDL_free(byval mem as any ptr)

type SDL_malloc_func as function(byval size as uinteger) as any ptr
type SDL_calloc_func as function(byval nmemb as uinteger, byval size as uinteger) as any ptr
type SDL_realloc_func as function(byval mem as any ptr, byval size as uinteger) as any ptr
type SDL_free_func as sub(byval mem as any ptr)

declare sub SDL_GetOriginalMemoryFunctions(byval malloc_func as SDL_malloc_func ptr, byval calloc_func as SDL_calloc_func ptr, byval realloc_func as SDL_realloc_func ptr, byval free_func as SDL_free_func ptr)
declare sub SDL_GetMemoryFunctions(byval malloc_func as SDL_malloc_func ptr, byval calloc_func as SDL_calloc_func ptr, byval realloc_func as SDL_realloc_func ptr, byval free_func as SDL_free_func ptr)
declare function SDL_SetMemoryFunctions(byval malloc_func as SDL_malloc_func, byval calloc_func as SDL_calloc_func, byval realloc_func as SDL_realloc_func, byval free_func as SDL_free_func) as boolean
declare function SDL_aligned_alloc(byval alignment as uinteger, byval size as uinteger) as any ptr
declare sub SDL_aligned_free(byval mem as any ptr)
declare function SDL_GetNumAllocations() as long
declare function SDL_GetEnvironment() as SDL_Environment ptr
declare function SDL_CreateEnvironment(byval populated as boolean) as SDL_Environment ptr
declare function SDL_GetEnvironmentVariable(byval env as SDL_Environment ptr, byval name as const zstring ptr) as const zstring ptr
declare function SDL_GetEnvironmentVariables(byval env as SDL_Environment ptr) as zstring ptr ptr
declare function SDL_SetEnvironmentVariable(byval env as SDL_Environment ptr, byval name as const zstring ptr, byval value as const zstring ptr, byval overwrite as boolean) as boolean
declare function SDL_UnsetEnvironmentVariable(byval env as SDL_Environment ptr, byval name as const zstring ptr) as boolean
declare sub SDL_DestroyEnvironment(byval env as SDL_Environment ptr)
declare function SDL_getenv(byval name as const zstring ptr) as const zstring ptr
declare function SDL_getenv_unsafe(byval name as const zstring ptr) as const zstring ptr
declare function SDL_setenv_unsafe(byval name as const zstring ptr, byval value as const zstring ptr, byval overwrite as long) as long
declare function SDL_unsetenv_unsafe(byval name as const zstring ptr) as long
type SDL_CompareCallback as function(byval a as const any ptr, byval b as const any ptr) as long
declare sub SDL_qsort(byval base as any ptr, byval nmemb as uinteger, byval size as uinteger, byval compare as SDL_CompareCallback)
declare function SDL_bsearch(byval key as const any ptr, byval base as const any ptr, byval nmemb as uinteger, byval size as uinteger, byval compare as SDL_CompareCallback) as any ptr
type SDL_CompareCallback_r as function(byval userdata as any ptr, byval a as const any ptr, byval b as const any ptr) as long
declare sub SDL_qsort_r(byval base as any ptr, byval nmemb as uinteger, byval size as uinteger, byval compare as SDL_CompareCallback_r, byval userdata as any ptr)
declare function SDL_bsearch_r(byval key as const any ptr, byval base as const any ptr, byval nmemb as uinteger, byval size as uinteger, byval compare as SDL_CompareCallback_r, byval userdata as any ptr) as any ptr
declare function SDL_abs(byval x as long) as long

#define SDL_min(x, y) iif((x) < (y), (x), (y))
#define SDL_max(x, y) iif((x) > (y), (x), (y))
#define SDL_clamp(x, a, b) iif((x) < (a), (a), iif((x) > (b), (b), (x)))

declare function SDL_isalpha(byval x as long) as long
declare function SDL_isalnum(byval x as long) as long
declare function SDL_isblank(byval x as long) as long
declare function SDL_iscntrl(byval x as long) as long
declare function SDL_isdigit(byval x as long) as long
declare function SDL_isxdigit(byval x as long) as long
declare function SDL_ispunct(byval x as long) as long
declare function SDL_isspace(byval x as long) as long
declare function SDL_isupper(byval x as long) as long
declare function SDL_islower(byval x as long) as long
declare function SDL_isprint(byval x as long) as long
declare function SDL_isgraph(byval x as long) as long
declare function SDL_toupper(byval x as long) as long
declare function SDL_tolower(byval x as long) as long
declare function SDL_crc16(byval crc as Uint16, byval data as const any ptr, byval len as uinteger) as Uint16
declare function SDL_crc32(byval crc as Uint32, byval data as const any ptr, byval len as uinteger) as Uint32
declare function SDL_murmur3_32(byval data as const any ptr, byval len as uinteger, byval seed as Uint32) as Uint32
declare function SDL_memcpy(byval dst as any ptr, byval src as const any ptr, byval len as uinteger) as any ptr
declare function SDL_memmove(byval dst as any ptr, byval src as const any ptr, byval len as uinteger) as any ptr
declare function SDL_memset(byval dst as any ptr, byval c as long, byval len as uinteger) as any ptr
declare function SDL_memset4(byval dst as any ptr, byval val as Uint32, byval dwords as uinteger) as any ptr

#define SDL_zero(x) SDL_memset(@(x), 0, sizeof(x))
#define SDL_zerop(x) SDL_memset((x), 0, sizeof(*(x)))

declare function SDL_memcmp(byval s1 as const any ptr, byval s2 as const any ptr, byval len as uinteger) as long
declare function SDL_wcslen(byval wstr as const wstring ptr) as uinteger
declare function SDL_wcsnlen(byval wstr as const wstring ptr, byval maxlen as uinteger) as uinteger
declare function SDL_wcslcpy(byval dst as wstring ptr, byval src as const wstring ptr, byval maxlen as uinteger) as uinteger
declare function SDL_wcslcat(byval dst as wstring ptr, byval src as const wstring ptr, byval maxlen as uinteger) as uinteger
declare function SDL_wcsdup(byval wstr as const wstring ptr) as wstring ptr
declare function SDL_wcsstr(byval haystack as const wstring ptr, byval needle as const wstring ptr) as wstring ptr
declare function SDL_wcsnstr(byval haystack as const wstring ptr, byval needle as const wstring ptr, byval maxlen as uinteger) as wstring ptr
declare function SDL_wcscmp(byval str1 as const wstring ptr, byval str2 as const wstring ptr) as long
declare function SDL_wcsncmp(byval str1 as const wstring ptr, byval str2 as const wstring ptr, byval maxlen as uinteger) as long
declare function SDL_wcscasecmp(byval str1 as const wstring ptr, byval str2 as const wstring ptr) as long
declare function SDL_wcsncasecmp(byval str1 as const wstring ptr, byval str2 as const wstring ptr, byval maxlen as uinteger) as long
declare function SDL_wcstol(byval str as const wstring ptr, byval endp as wstring ptr ptr, byval base as long) as clong
declare function SDL_strlen(byval str as const zstring ptr) as uinteger
declare function SDL_strnlen(byval str as const zstring ptr, byval maxlen as uinteger) as uinteger
declare function SDL_strlcpy(byval dst as zstring ptr, byval src as const zstring ptr, byval maxlen as uinteger) as uinteger
declare function SDL_utf8strlcpy(byval dst as zstring ptr, byval src as const zstring ptr, byval dst_bytes as uinteger) as uinteger
declare function SDL_strlcat(byval dst as zstring ptr, byval src as const zstring ptr, byval maxlen as uinteger) as uinteger
declare function SDL_strdup(byval str as const zstring ptr) as zstring ptr
declare function SDL_strndup(byval str as const zstring ptr, byval maxlen as uinteger) as zstring ptr
declare function SDL_strrev(byval str as zstring ptr) as zstring ptr
declare function SDL_strupr(byval str as zstring ptr) as zstring ptr
declare function SDL_strlwr(byval str as zstring ptr) as zstring ptr
declare function SDL_strchr(byval str as const zstring ptr, byval c as long) as zstring ptr
declare function SDL_strrchr(byval str as const zstring ptr, byval c as long) as zstring ptr
declare function SDL_strstr(byval haystack as const zstring ptr, byval needle as const zstring ptr) as zstring ptr
declare function SDL_strnstr(byval haystack as const zstring ptr, byval needle as const zstring ptr, byval maxlen as uinteger) as zstring ptr
declare function SDL_strcasestr(byval haystack as const zstring ptr, byval needle as const zstring ptr) as zstring ptr
declare function SDL_strtok_r(byval str as zstring ptr, byval delim as const zstring ptr, byval saveptr as zstring ptr ptr) as zstring ptr
declare function SDL_utf8strlen(byval str as const zstring ptr) as uinteger
declare function SDL_utf8strnlen(byval str as const zstring ptr, byval bytes as uinteger) as uinteger
declare function SDL_itoa(byval value as long, byval str as zstring ptr, byval radix as long) as zstring ptr
declare function SDL_uitoa(byval value as ulong, byval str as zstring ptr, byval radix as long) as zstring ptr
declare function SDL_ltoa(byval value as clong, byval str as zstring ptr, byval radix as long) as zstring ptr
declare function SDL_ultoa(byval value as culong, byval str as zstring ptr, byval radix as long) as zstring ptr
declare function SDL_lltoa(byval value as longint, byval str as zstring ptr, byval radix as long) as zstring ptr
declare function SDL_ulltoa(byval value as ulongint, byval str as zstring ptr, byval radix as long) as zstring ptr
declare function SDL_atoi(byval str as const zstring ptr) as long
declare function SDL_atof(byval str as const zstring ptr) as double
declare function SDL_strtol(byval str as const zstring ptr, byval endp as zstring ptr ptr, byval base as long) as clong
declare function SDL_strtoul(byval str as const zstring ptr, byval endp as zstring ptr ptr, byval base as long) as culong
declare function SDL_strtoll(byval str as const zstring ptr, byval endp as zstring ptr ptr, byval base as long) as longint
declare function SDL_strtoull(byval str as const zstring ptr, byval endp as zstring ptr ptr, byval base as long) as ulongint
declare function SDL_strtod(byval str as const zstring ptr, byval endp as zstring ptr ptr) as double
declare function SDL_strcmp(byval str1 as const zstring ptr, byval str2 as const zstring ptr) as long
declare function SDL_strncmp(byval str1 as const zstring ptr, byval str2 as const zstring ptr, byval maxlen as uinteger) as long
declare function SDL_strcasecmp(byval str1 as const zstring ptr, byval str2 as const zstring ptr) as long
declare function SDL_strncasecmp(byval str1 as const zstring ptr, byval str2 as const zstring ptr, byval maxlen as uinteger) as long
declare function SDL_strpbrk(byval str as const zstring ptr, byval breakset as const zstring ptr) as zstring ptr
const SDL_INVALID_UNICODE_CODEPOINT = &hFFFD
declare function SDL_StepUTF8(byval pstr as const zstring ptr ptr, byval pslen as uinteger ptr) as Uint32
declare function SDL_StepBackUTF8(byval start as const zstring ptr, byval pstr as const zstring ptr ptr) as Uint32
declare function SDL_UCS4ToUTF8(byval codepoint as Uint32, byval dst as zstring ptr) as zstring ptr
declare function SDL_sscanf(byval text as const zstring ptr, byval fmt as const zstring ptr, ...) as long
declare function SDL_vsscanf(byval text as const zstring ptr, byval fmt as const zstring ptr, byval ap as va_list) as long
declare function SDL_snprintf(byval text as zstring ptr, byval maxlen as uinteger, byval fmt as const zstring ptr, ...) as long
declare function SDL_swprintf(byval text as wstring ptr, byval maxlen as uinteger, byval fmt as const wstring ptr, ...) as long
declare function SDL_vsnprintf(byval text as zstring ptr, byval maxlen as uinteger, byval fmt as const zstring ptr, byval ap as va_list) as long
declare function SDL_vswprintf(byval text as wstring ptr, byval maxlen as uinteger, byval fmt as const wstring ptr, byval ap as va_list) as long
declare function SDL_asprintf(byval strp as zstring ptr ptr, byval fmt as const zstring ptr, ...) as long
declare function SDL_vasprintf(byval strp as zstring ptr ptr, byval fmt as const zstring ptr, byval ap as va_list) as long
declare sub SDL_srand(byval seed as Uint64)
declare function SDL_rand(byval n as Sint32) as Sint32
declare function SDL_randf() as single
declare function SDL_rand_bits() as Uint32
declare function SDL_rand_r(byval state as Uint64 ptr, byval n as Sint32) as Sint32
declare function SDL_randf_r(byval state as Uint64 ptr) as single
declare function SDL_rand_bits_r(byval state as Uint64 ptr) as Uint32
const SDL_PI_D = 3.141592653589793238462643383279502884
const SDL_PI_F = 3.141592653589793238462643383279502884f
declare function SDL_acos(byval x as double) as double
declare function SDL_acosf(byval x as single) as single
declare function SDL_asin(byval x as double) as double
declare function SDL_asinf(byval x as single) as single
declare function SDL_atan(byval x as double) as double
declare function SDL_atanf(byval x as single) as single
declare function SDL_atan2(byval y as double, byval x as double) as double
declare function SDL_atan2f(byval y as single, byval x as single) as single
declare function SDL_ceil(byval x as double) as double
declare function SDL_ceilf(byval x as single) as single
declare function SDL_copysign(byval x as double, byval y as double) as double
declare function SDL_copysignf(byval x as single, byval y as single) as single
declare function SDL_cos(byval x as double) as double
declare function SDL_cosf(byval x as single) as single
declare function SDL_exp(byval x as double) as double
declare function SDL_expf(byval x as single) as single
declare function SDL_fabs(byval x as double) as double
declare function SDL_fabsf(byval x as single) as single
declare function SDL_floor(byval x as double) as double
declare function SDL_floorf(byval x as single) as single
declare function SDL_trunc(byval x as double) as double
declare function SDL_truncf(byval x as single) as single
declare function SDL_fmod(byval x as double, byval y as double) as double
declare function SDL_fmodf(byval x as single, byval y as single) as single
declare function SDL_isinf(byval x as double) as long
declare function SDL_isinff(byval x as single) as long
declare function SDL_isnan(byval x as double) as long
declare function SDL_isnanf(byval x as single) as long
declare function SDL_log(byval x as double) as double
declare function SDL_logf(byval x as single) as single
declare function SDL_log10(byval x as double) as double
declare function SDL_log10f(byval x as single) as single
declare function SDL_modf(byval x as double, byval y as double ptr) as double
declare function SDL_modff(byval x as single, byval y as single ptr) as single
declare function SDL_pow(byval x as double, byval y as double) as double
declare function SDL_powf(byval x as single, byval y as single) as single
declare function SDL_round(byval x as double) as double
declare function SDL_roundf(byval x as single) as single
declare function SDL_lround(byval x as double) as clong
declare function SDL_lroundf(byval x as single) as clong
declare function SDL_scalbn(byval x as double, byval n as long) as double
declare function SDL_scalbnf(byval x as single, byval n as long) as single
declare function SDL_sin(byval x as double) as double
declare function SDL_sinf(byval x as single) as single
declare function SDL_sqrt(byval x as double) as double
declare function SDL_sqrtf(byval x as single) as single
declare function SDL_tan(byval x as double) as double
declare function SDL_tanf(byval x as single) as single
type SDL_iconv_t as SDL_iconv_data_t ptr
declare function SDL_iconv_open(byval tocode as const zstring ptr, byval fromcode as const zstring ptr) as SDL_iconv_t
declare function SDL_iconv_close(byval cd as SDL_iconv_t) as long
declare function SDL_iconv(byval cd as SDL_iconv_t, byval inbuf as const zstring ptr ptr, byval inbytesleft as uinteger ptr, byval outbuf as zstring ptr ptr, byval outbytesleft as uinteger ptr) as uinteger

const SDL_ICONV_ERROR = cuint(-1)
const SDL_ICONV_E2BIG = cuint(-2)
const SDL_ICONV_EILSEQ = cuint(-3)
const SDL_ICONV_EINVAL = cuint(-4)
declare function SDL_iconv_string(byval tocode as const zstring ptr, byval fromcode as const zstring ptr, byval inbuf as const zstring ptr, byval inbytesleft as uinteger) as zstring ptr
#define SDL_iconv_utf8_locale(S) SDL_iconv_string("", "UTF-8", S, SDL_strlen(S) + 1)

type SDL_FunctionPointer as sub()
'' -------------------------------------------------------------------------
'' SDL_assert.h
'' -------------------------------------------------------------------------
#if ((not defined(__FB_ARM__)) and (defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__))) or defined(__FB_DARWIN__)
#elseif defined(__FB_ARM__) and (defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__))
#else
#endif

#define SDL_FILE __FILE__
#define SDL_ASSERT_FILE SDL_FILE
#define SDL_LINE __LINE__

type SDL_AssertState as long
enum
	SDL_ASSERTION_RETRY
	SDL_ASSERTION_BREAK
	SDL_ASSERTION_ABORT
	SDL_ASSERTION_IGNORE
	SDL_ASSERTION_ALWAYS_IGNORE
end enum

type SDL_AssertData
	always_ignore as boolean
	trigger_count as ulong
	condition as const zstring ptr
	filename as const zstring ptr
	linenum as long
	function as const zstring ptr
	next as const SDL_AssertData ptr
end type

declare function SDL_ReportAssertion(byval data as SDL_AssertData ptr, byval func as const zstring ptr, byval file as const zstring ptr, byval line as long) as SDL_AssertState
type SDL_AssertionHandler as function(byval data as const SDL_AssertData ptr, byval userdata as any ptr) as SDL_AssertState

declare sub SDL_SetAssertionHandler(byval handler as SDL_AssertionHandler, byval userdata as any ptr)
declare function SDL_GetDefaultAssertionHandler() as SDL_AssertionHandler
declare function SDL_GetAssertionHandler(byval puserdata as any ptr ptr) as SDL_AssertionHandler
declare function SDL_GetAssertionReport() as const SDL_AssertData ptr
declare sub SDL_ResetAssertionReport()
'' -------------------------------------------------------------------------
'' SDL_asyncio.h
'' -------------------------------------------------------------------------
type SDL_AsyncIOTaskType as long
enum
	SDL_ASYNCIO_TASK_READ
	SDL_ASYNCIO_TASK_WRITE
	SDL_ASYNCIO_TASK_CLOSE
end enum

type SDL_AsyncIOResult as long
enum
	SDL_ASYNCIO_COMPLETE
	SDL_ASYNCIO_FAILURE
	SDL_ASYNCIO_CANCELED
end enum

type SDL_AsyncIOOutcome
	asyncio as SDL_AsyncIO ptr
	as SDL_AsyncIOTaskType type
	result as SDL_AsyncIOResult
	buffer as any ptr
	offset as Uint64
	bytes_requested as Uint64
	bytes_transferred as Uint64
	userdata as any ptr
end type

declare function SDL_AsyncIOFromFile(byval file as const zstring ptr, byval mode as const zstring ptr) as SDL_AsyncIO ptr
declare function SDL_GetAsyncIOSize(byval asyncio as SDL_AsyncIO ptr) as Sint64
declare function SDL_ReadAsyncIO(byval asyncio as SDL_AsyncIO ptr, byval ptr as any ptr, byval offset as Uint64, byval size as Uint64, byval queue as SDL_AsyncIOQueue ptr, byval userdata as any ptr) as boolean
declare function SDL_WriteAsyncIO(byval asyncio as SDL_AsyncIO ptr, byval ptr as any ptr, byval offset as Uint64, byval size as Uint64, byval queue as SDL_AsyncIOQueue ptr, byval userdata as any ptr) as boolean
declare function SDL_CloseAsyncIO(byval asyncio as SDL_AsyncIO ptr, byval flush as boolean, byval queue as SDL_AsyncIOQueue ptr, byval userdata as any ptr) as boolean
declare function SDL_CreateAsyncIOQueue() as SDL_AsyncIOQueue ptr
declare sub SDL_DestroyAsyncIOQueue(byval queue as SDL_AsyncIOQueue ptr)
declare function SDL_GetAsyncIOResult(byval queue as SDL_AsyncIOQueue ptr, byval outcome as SDL_AsyncIOOutcome ptr) as boolean
declare function SDL_WaitAsyncIOResult(byval queue as SDL_AsyncIOQueue ptr, byval outcome as SDL_AsyncIOOutcome ptr, byval timeoutMS as Sint32) as boolean
declare sub SDL_SignalAsyncIOQueue(byval queue as SDL_AsyncIOQueue ptr)
declare function SDL_LoadFileAsync(byval file as const zstring ptr, byval queue as SDL_AsyncIOQueue ptr, byval userdata as any ptr) as boolean
'' -------------------------------------------------------------------------
'' SDL_atomic.h
'' -------------------------------------------------------------------------
type SDL_SpinLock as long
declare function SDL_TryLockSpinlock(byval lock as SDL_SpinLock ptr) as boolean
declare sub SDL_LockSpinlock(byval lock as SDL_SpinLock ptr)
declare sub SDL_UnlockSpinlock(byval lock as SDL_SpinLock ptr)
declare sub SDL_MemoryBarrierReleaseFunction()
declare sub SDL_MemoryBarrierAcquireFunction()

#if defined(__FB_ARM__) and (defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__))
#else
#endif

#if (not defined(__FB_64BIT__)) and defined(__FB_ARM__) and (defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__))
#elseif defined(__FB_64BIT__) and defined(__FB_ARM__) and (defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__))
#endif


type SDL_AtomicInt
	value as long
end type

declare function SDL_CompareAndSwapAtomicInt(byval a as SDL_AtomicInt ptr, byval oldval as long, byval newval as long) as boolean
declare function SDL_SetAtomicInt(byval a as SDL_AtomicInt ptr, byval v as long) as long
declare function SDL_GetAtomicInt(byval a as SDL_AtomicInt ptr) as long
declare function SDL_AddAtomicInt(byval a as SDL_AtomicInt ptr, byval v as long) as long
#define SDL_AtomicIncRef(a) SDL_AddAtomicInt(a, 1)
#define SDL_AtomicDecRef(a) (SDL_AddAtomicInt(a, -1) = 1)

type SDL_AtomicU32
	value as Uint32
end type

declare function SDL_CompareAndSwapAtomicU32(byval a as SDL_AtomicU32 ptr, byval oldval as Uint32, byval newval as Uint32) as boolean
declare function SDL_SetAtomicU32(byval a as SDL_AtomicU32 ptr, byval v as Uint32) as Uint32
declare function SDL_GetAtomicU32(byval a as SDL_AtomicU32 ptr) as Uint32
declare function SDL_AddAtomicU32(byval a as SDL_AtomicU32 ptr, byval v as long) as Uint32
declare function SDL_CompareAndSwapAtomicPointer(byval a as any ptr ptr, byval oldval as any ptr, byval newval as any ptr) as boolean
declare function SDL_SetAtomicPointer(byval a as any ptr ptr, byval v as any ptr) as any ptr
declare function SDL_GetAtomicPointer(byval a as any ptr ptr) as any ptr

'' -------------------------------------------------------------------------
'' SDL_audio.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_endian.h
'' -------------------------------------------------------------------------
const SDL_LIL_ENDIAN = 1234
const SDL_BIG_ENDIAN = 4321

'' -------------------------------------------------------------------------
'' SDL_error.h
'' -------------------------------------------------------------------------
declare function SDL_SetError(byval fmt as const zstring ptr, ...) as boolean
declare function SDL_SetErrorV(byval fmt as const zstring ptr, byval ap as va_list) as boolean
declare function SDL_OutOfMemory() as boolean
declare function SDL_GetError() as const zstring ptr
declare function SDL_ClearError() as boolean
#define SDL_Unsupported() SDL_SetError("That operation is not supported")
#define SDL_InvalidParamError(param) SDL_SetError("Parameter '%s' is invalid", (param))

'' -------------------------------------------------------------------------
'' SDL_mutex.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_thread.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_properties.h
'' -------------------------------------------------------------------------
type SDL_PropertiesID as Uint32

type SDL_PropertyType as long
enum
	SDL_PROPERTY_TYPE_INVALID
	SDL_PROPERTY_TYPE_POINTER
	SDL_PROPERTY_TYPE_STRING
	SDL_PROPERTY_TYPE_NUMBER
	SDL_PROPERTY_TYPE_FLOAT
	SDL_PROPERTY_TYPE_BOOLEAN
end enum

#define SDL_PROP_NAME_STRING "SDL.name"
declare function SDL_GetGlobalProperties() as SDL_PropertiesID
declare function SDL_CreateProperties() as SDL_PropertiesID
declare function SDL_CopyProperties(byval src as SDL_PropertiesID, byval dst as SDL_PropertiesID) as boolean
declare function SDL_LockProperties(byval props as SDL_PropertiesID) as boolean
declare sub SDL_UnlockProperties(byval props as SDL_PropertiesID)
type SDL_CleanupPropertyCallback as sub(byval userdata as any ptr, byval value as any ptr)
declare function SDL_SetPointerPropertyWithCleanup(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval value as any ptr, byval cleanup as SDL_CleanupPropertyCallback, byval userdata as any ptr) as boolean
declare function SDL_SetPointerProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval value as any ptr) as boolean
declare function SDL_SetStringProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval value as const zstring ptr) as boolean
declare function SDL_SetNumberProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval value as Sint64) as boolean
declare function SDL_SetFloatProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval value as single) as boolean
declare function SDL_SetBooleanProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval value as boolean) as boolean
declare function SDL_HasProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr) as boolean
declare function SDL_GetPropertyType(byval props as SDL_PropertiesID, byval name as const zstring ptr) as SDL_PropertyType
declare function SDL_GetPointerProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval default_value as any ptr) as any ptr
declare function SDL_GetStringProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval default_value as const zstring ptr) as const zstring ptr
declare function SDL_GetNumberProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval default_value as Sint64) as Sint64
declare function SDL_GetFloatProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval default_value as single) as single
declare function SDL_GetBooleanProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr, byval default_value as boolean) as boolean
declare function SDL_ClearProperty(byval props as SDL_PropertiesID, byval name as const zstring ptr) as boolean
type SDL_EnumeratePropertiesCallback as sub(byval userdata as any ptr, byval props as SDL_PropertiesID, byval name as const zstring ptr)
declare function SDL_EnumerateProperties(byval props as SDL_PropertiesID, byval callback as SDL_EnumeratePropertiesCallback, byval userdata as any ptr) as boolean
declare sub SDL_DestroyProperties(byval props as SDL_PropertiesID)
type SDL_ThreadID as Uint64
type SDL_TLSID as SDL_AtomicInt

type SDL_ThreadPriority as long
enum
	SDL_THREAD_PRIORITY_LOW
	SDL_THREAD_PRIORITY_NORMAL
	SDL_THREAD_PRIORITY_HIGH
	SDL_THREAD_PRIORITY_TIME_CRITICAL
end enum

type SDL_ThreadState as long
enum
	SDL_THREAD_UNKNOWN
	SDL_THREAD_ALIVE
	SDL_THREAD_DETACHED
	SDL_THREAD_COMPLETE
end enum

type SDL_ThreadFunction as function(byval data as any ptr) as long

#ifdef __FB_WIN32__
	#define SDL_BeginThreadFunction @_beginthreadex
	#define SDL_EndThreadFunction @_endthreadex
#else
	#define SDL_BeginThreadFunction 0
	#define SDL_EndThreadFunction 0
#endif

declare function SDL_CreateThreadRuntime(byval fn as SDL_ThreadFunction, byval name as const zstring ptr, byval data as any ptr, byval pfnBeginThread as SDL_FunctionPointer, byval pfnEndThread as SDL_FunctionPointer) as SDL_Thread ptr
declare function SDL_CreateThreadWithPropertiesRuntime(byval props as SDL_PropertiesID, byval pfnBeginThread as SDL_FunctionPointer, byval pfnEndThread as SDL_FunctionPointer) as SDL_Thread ptr
#define SDL_CreateThread(fn, name, data) SDL_CreateThreadRuntime((fn), (name), (data), cast(SDL_FunctionPointer, SDL_BeginThreadFunction), cast(SDL_FunctionPointer, SDL_EndThreadFunction))
#define SDL_CreateThreadWithProperties(props) SDL_CreateThreadWithPropertiesRuntime((props), cast(SDL_FunctionPointer, SDL_BeginThreadFunction), cast(SDL_FunctionPointer, SDL_EndThreadFunction))
#define SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER "SDL.thread.create.entry_function"
#define SDL_PROP_THREAD_CREATE_NAME_STRING "SDL.thread.create.name"
#define SDL_PROP_THREAD_CREATE_USERDATA_POINTER "SDL.thread.create.userdata"
#define SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER "SDL.thread.create.stacksize"

declare function SDL_GetThreadName(byval thread as SDL_Thread ptr) as const zstring ptr
declare function SDL_GetCurrentThreadID() as SDL_ThreadID
declare function SDL_GetThreadID(byval thread as SDL_Thread ptr) as SDL_ThreadID
declare function SDL_SetCurrentThreadPriority(byval priority as SDL_ThreadPriority) as boolean
declare sub SDL_WaitThread(byval thread as SDL_Thread ptr, byval status as long ptr)
declare function SDL_GetThreadState(byval thread as SDL_Thread ptr) as SDL_ThreadState
declare sub SDL_DetachThread(byval thread as SDL_Thread ptr)
declare function SDL_GetTLS(byval id as SDL_TLSID ptr) as any ptr
type SDL_TLSDestructorCallback as sub(byval value as any ptr)
declare function SDL_SetTLS(byval id as SDL_TLSID ptr, byval value as const any ptr, byval destructor as SDL_TLSDestructorCallback) as boolean
declare sub SDL_CleanupTLS()

#define SDL_THREAD_ANNOTATION_ATTRIBUTE__(x)
#define SDL_CAPABILITY(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(capability(x))
#define SDL_SCOPED_CAPABILITY SDL_THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)
#define SDL_GUARDED_BY(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))
#define SDL_PT_GUARDED_BY(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))
#define SDL_ACQUIRED_BEFORE(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(x))
#define SDL_ACQUIRED_AFTER(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(x))
#define SDL_REQUIRES(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(x))
#define SDL_REQUIRES_SHARED(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(x))
#define SDL_ACQUIRE(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(x))
#define SDL_ACQUIRE_SHARED(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(x))
#define SDL_RELEASE(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(release_capability(x))
#define SDL_RELEASE_SHARED(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(x))
#define SDL_RELEASE_GENERIC(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(release_generic_capability(x))
#define SDL_TRY_ACQUIRE(x, y) SDL_THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(x, y))
#define SDL_TRY_ACQUIRE_SHARED(x, y) SDL_THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(x, y))
#define SDL_EXCLUDES(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(x))
#define SDL_ASSERT_CAPABILITY(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))
#define SDL_ASSERT_SHARED_CAPABILITY(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))
#define SDL_RETURN_CAPABILITY(x) SDL_THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))
#define SDL_NO_THREAD_SAFETY_ANALYSIS SDL_THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

declare function SDL_CreateMutex() as SDL_Mutex ptr
declare sub SDL_LockMutex(byval mutex as SDL_Mutex ptr)
declare function SDL_TryLockMutex(byval mutex as SDL_Mutex ptr) as boolean
declare sub SDL_UnlockMutex(byval mutex as SDL_Mutex ptr)
declare sub SDL_DestroyMutex(byval mutex as SDL_Mutex ptr)
declare function SDL_CreateRWLock() as SDL_RWLock ptr
declare sub SDL_LockRWLockForReading(byval rwlock as SDL_RWLock ptr)
declare sub SDL_LockRWLockForWriting(byval rwlock as SDL_RWLock ptr)
declare function SDL_TryLockRWLockForReading(byval rwlock as SDL_RWLock ptr) as boolean
declare function SDL_TryLockRWLockForWriting(byval rwlock as SDL_RWLock ptr) as boolean
declare sub SDL_UnlockRWLock(byval rwlock as SDL_RWLock ptr)
declare sub SDL_DestroyRWLock(byval rwlock as SDL_RWLock ptr)
declare function SDL_CreateSemaphore(byval initial_value as Uint32) as SDL_Semaphore ptr
declare sub SDL_DestroySemaphore(byval sem as SDL_Semaphore ptr)
declare sub SDL_WaitSemaphore(byval sem as SDL_Semaphore ptr)
declare function SDL_TryWaitSemaphore(byval sem as SDL_Semaphore ptr) as boolean
declare function SDL_WaitSemaphoreTimeout(byval sem as SDL_Semaphore ptr, byval timeoutMS as Sint32) as boolean
declare sub SDL_SignalSemaphore(byval sem as SDL_Semaphore ptr)
declare function SDL_GetSemaphoreValue(byval sem as SDL_Semaphore ptr) as Uint32
declare function SDL_CreateCondition() as SDL_Condition ptr
declare sub SDL_DestroyCondition(byval cond as SDL_Condition ptr)
declare sub SDL_SignalCondition(byval cond as SDL_Condition ptr)
declare sub SDL_BroadcastCondition(byval cond as SDL_Condition ptr)
declare sub SDL_WaitCondition(byval cond as SDL_Condition ptr, byval mutex as SDL_Mutex ptr)
declare function SDL_WaitConditionTimeout(byval cond as SDL_Condition ptr, byval mutex as SDL_Mutex ptr, byval timeoutMS as Sint32) as boolean

type SDL_InitStatus as long
enum
	SDL_INIT_STATUS_UNINITIALIZED
	SDL_INIT_STATUS_INITIALIZING
	SDL_INIT_STATUS_INITIALIZED
	SDL_INIT_STATUS_UNINITIALIZING
end enum

type SDL_InitState
	status as SDL_AtomicInt
	thread as SDL_ThreadID
	reserved as any ptr
end type

declare function SDL_ShouldInit(byval state as SDL_InitState ptr) as boolean
declare function SDL_ShouldQuit(byval state as SDL_InitState ptr) as boolean
declare sub SDL_SetInitialized(byval state as SDL_InitState ptr, byval initialized as boolean)
'' -------------------------------------------------------------------------
'' SDL_iostream.h
'' -------------------------------------------------------------------------
type SDL_IOStatus as long
enum
	SDL_IO_STATUS_READY
	SDL_IO_STATUS_ERROR
	SDL_IO_STATUS_EOF
	SDL_IO_STATUS_NOT_READY
	SDL_IO_STATUS_READONLY
	SDL_IO_STATUS_WRITEONLY
end enum

type SDL_IOWhence as long
enum
	SDL_IO_SEEK_SET
	SDL_IO_SEEK_CUR
	SDL_IO_SEEK_END
end enum

type SDL_IOStreamInterface
	version as Uint32
	size as function(byval userdata as any ptr) as Sint64
	seek as function(byval userdata as any ptr, byval offset as Sint64, byval whence as SDL_IOWhence) as Sint64
	read as function(byval userdata as any ptr, byval ptr as any ptr, byval size as uinteger, byval status as SDL_IOStatus ptr) as uinteger
	write as function(byval userdata as any ptr, byval ptr as const any ptr, byval size as uinteger, byval status as SDL_IOStatus ptr) as uinteger
	flush as function(byval userdata as any ptr, byval status as SDL_IOStatus ptr) as boolean
	close as function(byval userdata as any ptr) as boolean
end type

declare function SDL_IOFromFile(byval file as const zstring ptr, byval mode as const zstring ptr) as SDL_IOStream ptr
#define SDL_PROP_IOSTREAM_WINDOWS_HANDLE_POINTER "SDL.iostream.windows.handle"
#define SDL_PROP_IOSTREAM_STDIO_FILE_POINTER "SDL.iostream.stdio.file"
#define SDL_PROP_IOSTREAM_FILE_DESCRIPTOR_NUMBER "SDL.iostream.file_descriptor"
#define SDL_PROP_IOSTREAM_ANDROID_AASSET_POINTER "SDL.iostream.android.aasset"
declare function SDL_IOFromMem(byval mem as any ptr, byval size as uinteger) as SDL_IOStream ptr
#define SDL_PROP_IOSTREAM_MEMORY_POINTER "SDL.iostream.memory.base"
#define SDL_PROP_IOSTREAM_MEMORY_SIZE_NUMBER "SDL.iostream.memory.size"
#define SDL_PROP_IOSTREAM_MEMORY_FREE_FUNC_POINTER "SDL.iostream.memory.free"
declare function SDL_IOFromConstMem(byval mem as const any ptr, byval size as uinteger) as SDL_IOStream ptr
declare function SDL_IOFromDynamicMem() as SDL_IOStream ptr
#define SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER "SDL.iostream.dynamic.memory"
#define SDL_PROP_IOSTREAM_DYNAMIC_CHUNKSIZE_NUMBER "SDL.iostream.dynamic.chunksize"

declare function SDL_OpenIO(byval iface as const SDL_IOStreamInterface ptr, byval userdata as any ptr) as SDL_IOStream ptr
declare function SDL_CloseIO(byval context as SDL_IOStream ptr) as boolean
declare function SDL_GetIOProperties(byval context as SDL_IOStream ptr) as SDL_PropertiesID
declare function SDL_GetIOStatus(byval context as SDL_IOStream ptr) as SDL_IOStatus
declare function SDL_GetIOSize(byval context as SDL_IOStream ptr) as Sint64
declare function SDL_SeekIO(byval context as SDL_IOStream ptr, byval offset as Sint64, byval whence as SDL_IOWhence) as Sint64
declare function SDL_TellIO(byval context as SDL_IOStream ptr) as Sint64
declare function SDL_ReadIO(byval context as SDL_IOStream ptr, byval ptr as any ptr, byval size as uinteger) as uinteger
declare function SDL_WriteIO(byval context as SDL_IOStream ptr, byval ptr as const any ptr, byval size as uinteger) as uinteger
declare function SDL_IOprintf(byval context as SDL_IOStream ptr, byval fmt as const zstring ptr, ...) as uinteger
declare function SDL_IOvprintf(byval context as SDL_IOStream ptr, byval fmt as const zstring ptr, byval ap as va_list) as uinteger
declare function SDL_FlushIO(byval context as SDL_IOStream ptr) as boolean
declare function SDL_LoadFile_IO(byval src as SDL_IOStream ptr, byval datasize as uinteger ptr, byval closeio as boolean) as any ptr
declare function SDL_LoadFile(byval file as const zstring ptr, byval datasize as uinteger ptr) as any ptr
declare function SDL_SaveFile_IO(byval src as SDL_IOStream ptr, byval data as const any ptr, byval datasize as uinteger, byval closeio as boolean) as boolean
declare function SDL_SaveFile(byval file as const zstring ptr, byval data as const any ptr, byval datasize as uinteger) as boolean
declare function SDL_ReadU8(byval src as SDL_IOStream ptr, byval value as Uint8 ptr) as boolean
declare function SDL_ReadS8(byval src as SDL_IOStream ptr, byval value as Sint8 ptr) as boolean
declare function SDL_ReadU16LE(byval src as SDL_IOStream ptr, byval value as Uint16 ptr) as boolean
declare function SDL_ReadS16LE(byval src as SDL_IOStream ptr, byval value as Sint16 ptr) as boolean
declare function SDL_ReadU16BE(byval src as SDL_IOStream ptr, byval value as Uint16 ptr) as boolean
declare function SDL_ReadS16BE(byval src as SDL_IOStream ptr, byval value as Sint16 ptr) as boolean
declare function SDL_ReadU32LE(byval src as SDL_IOStream ptr, byval value as Uint32 ptr) as boolean
declare function SDL_ReadS32LE(byval src as SDL_IOStream ptr, byval value as Sint32 ptr) as boolean
declare function SDL_ReadU32BE(byval src as SDL_IOStream ptr, byval value as Uint32 ptr) as boolean
declare function SDL_ReadS32BE(byval src as SDL_IOStream ptr, byval value as Sint32 ptr) as boolean
declare function SDL_ReadU64LE(byval src as SDL_IOStream ptr, byval value as Uint64 ptr) as boolean
declare function SDL_ReadS64LE(byval src as SDL_IOStream ptr, byval value as Sint64 ptr) as boolean
declare function SDL_ReadU64BE(byval src as SDL_IOStream ptr, byval value as Uint64 ptr) as boolean
declare function SDL_ReadS64BE(byval src as SDL_IOStream ptr, byval value as Sint64 ptr) as boolean
declare function SDL_WriteU8(byval dst as SDL_IOStream ptr, byval value as Uint8) as boolean
declare function SDL_WriteS8(byval dst as SDL_IOStream ptr, byval value as Sint8) as boolean
declare function SDL_WriteU16LE(byval dst as SDL_IOStream ptr, byval value as Uint16) as boolean
declare function SDL_WriteS16LE(byval dst as SDL_IOStream ptr, byval value as Sint16) as boolean
declare function SDL_WriteU16BE(byval dst as SDL_IOStream ptr, byval value as Uint16) as boolean
declare function SDL_WriteS16BE(byval dst as SDL_IOStream ptr, byval value as Sint16) as boolean
declare function SDL_WriteU32LE(byval dst as SDL_IOStream ptr, byval value as Uint32) as boolean
declare function SDL_WriteS32LE(byval dst as SDL_IOStream ptr, byval value as Sint32) as boolean
declare function SDL_WriteU32BE(byval dst as SDL_IOStream ptr, byval value as Uint32) as boolean
declare function SDL_WriteS32BE(byval dst as SDL_IOStream ptr, byval value as Sint32) as boolean
declare function SDL_WriteU64LE(byval dst as SDL_IOStream ptr, byval value as Uint64) as boolean
declare function SDL_WriteS64LE(byval dst as SDL_IOStream ptr, byval value as Sint64) as boolean
declare function SDL_WriteU64BE(byval dst as SDL_IOStream ptr, byval value as Uint64) as boolean
declare function SDL_WriteS64BE(byval dst as SDL_IOStream ptr, byval value as Sint64) as boolean
const SDL_AUDIO_MASK_BITSIZE as long = &hFFu
const SDL_AUDIO_MASK_FLOAT as long = culng(1u shl 8)
const SDL_AUDIO_MASK_BIG_ENDIAN as long = culng(1u shl 12)
const SDL_AUDIO_MASK_SIGNED as long = culng(1u shl 15)

type SDL_AudioFormat as long
enum
	SDL_AUDIO_UNKNOWN = &h0000u
	SDL_AUDIO_U8 = &h0008u
	SDL_AUDIO_S8 = &h8008u
	SDL_AUDIO_S16LE = &h8010u
	SDL_AUDIO_S16BE = &h9010u
	SDL_AUDIO_S32LE = &h8020u
	SDL_AUDIO_S32BE = &h9020u
	SDL_AUDIO_F32LE = &h8120u
	SDL_AUDIO_F32BE = &h9120u
#ifdef __FB_BIGENDIAN__
	SDL_AUDIO_S16 = SDL_AUDIO_S16BE
#else
	SDL_AUDIO_S16 = SDL_AUDIO_S16LE
#endif
#ifdef __FB_BIGENDIAN__
	SDL_AUDIO_S32 = SDL_AUDIO_S32BE
#else
	SDL_AUDIO_S32 = SDL_AUDIO_S32LE
#endif
#ifdef __FB_BIGENDIAN__
	SDL_AUDIO_F32 = SDL_AUDIO_F32BE
#else
	SDL_AUDIO_F32 = SDL_AUDIO_F32LE
#endif
end enum

#define SDL_AUDIO_BITSIZE(x) ((x) and SDL_AUDIO_MASK_BITSIZE)
#define SDL_AUDIO_BYTESIZE(x) clng(SDL_AUDIO_BITSIZE(x) \ 8L)
#define SDL_AUDIO_ISFLOAT(x) ((x) and SDL_AUDIO_MASK_FLOAT)
#define SDL_AUDIO_ISBIGENDIAN(x) ((x) and SDL_AUDIO_MASK_BIG_ENDIAN)
#define SDL_AUDIO_ISLITTLEENDIAN(x) (SDL_AUDIO_ISBIGENDIAN(x) = 0)
#define SDL_AUDIO_ISSIGNED(x) ((x) and SDL_AUDIO_MASK_SIGNED)
#define SDL_AUDIO_ISINT(x) (SDL_AUDIO_ISFLOAT(x) = 0)
#define SDL_AUDIO_ISUNSIGNED(x) (SDL_AUDIO_ISSIGNED(x) = 0)
type SDL_AudioDeviceID as Uint32
const SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK = cast(SDL_AudioDeviceID, &hFFFFFFFFu)
const SDL_AUDIO_DEVICE_DEFAULT_RECORDING = cast(SDL_AudioDeviceID, &hFFFFFFFEu)

type SDL_AudioSpec
	format as SDL_AudioFormat
	channels as long
	freq as long
end type

#define SDL_AUDIO_FRAMESIZE(x) clng(SDL_AUDIO_BYTESIZE((x).format) * (x).channels)
declare function SDL_GetNumAudioDrivers() as long
declare function SDL_GetAudioDriver(byval index as long) as const zstring ptr
declare function SDL_GetCurrentAudioDriver() as const zstring ptr
declare function SDL_GetAudioPlaybackDevices(byval count as long ptr) as SDL_AudioDeviceID ptr
declare function SDL_GetAudioRecordingDevices(byval count as long ptr) as SDL_AudioDeviceID ptr
declare function SDL_GetAudioDeviceName(byval devid as SDL_AudioDeviceID) as const zstring ptr
declare function SDL_GetAudioDeviceFormat(byval devid as SDL_AudioDeviceID, byval spec as SDL_AudioSpec ptr, byval sample_frames as long ptr) as boolean
declare function SDL_GetAudioDeviceChannelMap(byval devid as SDL_AudioDeviceID, byval count as long ptr) as long ptr
declare function SDL_OpenAudioDevice(byval devid as SDL_AudioDeviceID, byval spec as const SDL_AudioSpec ptr) as SDL_AudioDeviceID
declare function SDL_IsAudioDevicePhysical(byval devid as SDL_AudioDeviceID) as boolean
declare function SDL_IsAudioDevicePlayback(byval devid as SDL_AudioDeviceID) as boolean
declare function SDL_PauseAudioDevice(byval devid as SDL_AudioDeviceID) as boolean
declare function SDL_ResumeAudioDevice(byval devid as SDL_AudioDeviceID) as boolean
declare function SDL_AudioDevicePaused(byval devid as SDL_AudioDeviceID) as boolean
declare function SDL_GetAudioDeviceGain(byval devid as SDL_AudioDeviceID) as single
declare function SDL_SetAudioDeviceGain(byval devid as SDL_AudioDeviceID, byval gain as single) as boolean
declare sub SDL_CloseAudioDevice(byval devid as SDL_AudioDeviceID)
declare function SDL_BindAudioStreams(byval devid as SDL_AudioDeviceID, byval streams as SDL_AudioStream const ptr ptr, byval num_streams as long) as boolean
declare function SDL_BindAudioStream(byval devid as SDL_AudioDeviceID, byval stream as SDL_AudioStream ptr) as boolean
declare sub SDL_UnbindAudioStreams(byval streams as SDL_AudioStream const ptr ptr, byval num_streams as long)
declare sub SDL_UnbindAudioStream(byval stream as SDL_AudioStream ptr)
declare function SDL_GetAudioStreamDevice(byval stream as SDL_AudioStream ptr) as SDL_AudioDeviceID
declare function SDL_CreateAudioStream(byval src_spec as const SDL_AudioSpec ptr, byval dst_spec as const SDL_AudioSpec ptr) as SDL_AudioStream ptr
declare function SDL_GetAudioStreamProperties(byval stream as SDL_AudioStream ptr) as SDL_PropertiesID
#define SDL_PROP_AUDIOSTREAM_AUTO_CLEANUP_BOOLEAN "SDL.audiostream.auto_cleanup"
declare function SDL_GetAudioStreamFormat(byval stream as SDL_AudioStream ptr, byval src_spec as SDL_AudioSpec ptr, byval dst_spec as SDL_AudioSpec ptr) as boolean
declare function SDL_SetAudioStreamFormat(byval stream as SDL_AudioStream ptr, byval src_spec as const SDL_AudioSpec ptr, byval dst_spec as const SDL_AudioSpec ptr) as boolean
declare function SDL_GetAudioStreamFrequencyRatio(byval stream as SDL_AudioStream ptr) as single
declare function SDL_SetAudioStreamFrequencyRatio(byval stream as SDL_AudioStream ptr, byval ratio as single) as boolean
declare function SDL_GetAudioStreamGain(byval stream as SDL_AudioStream ptr) as single
declare function SDL_SetAudioStreamGain(byval stream as SDL_AudioStream ptr, byval gain as single) as boolean
declare function SDL_GetAudioStreamInputChannelMap(byval stream as SDL_AudioStream ptr, byval count as long ptr) as long ptr
declare function SDL_GetAudioStreamOutputChannelMap(byval stream as SDL_AudioStream ptr, byval count as long ptr) as long ptr
declare function SDL_SetAudioStreamInputChannelMap(byval stream as SDL_AudioStream ptr, byval chmap as const long ptr, byval count as long) as boolean
declare function SDL_SetAudioStreamOutputChannelMap(byval stream as SDL_AudioStream ptr, byval chmap as const long ptr, byval count as long) as boolean
declare function SDL_PutAudioStreamData(byval stream as SDL_AudioStream ptr, byval buf as const any ptr, byval len as long) as boolean
type SDL_AudioStreamDataCompleteCallback as sub(byval userdata as any ptr, byval buf as const any ptr, byval buflen as long)
declare function SDL_PutAudioStreamDataNoCopy(byval stream as SDL_AudioStream ptr, byval buf as const any ptr, byval len as long, byval callback as SDL_AudioStreamDataCompleteCallback, byval userdata as any ptr) as boolean
declare function SDL_PutAudioStreamPlanarData(byval stream as SDL_AudioStream ptr, byval channel_buffers as const any const ptr ptr, byval num_channels as long, byval num_samples as long) as boolean
declare function SDL_GetAudioStreamData(byval stream as SDL_AudioStream ptr, byval buf as any ptr, byval len as long) as long
declare function SDL_GetAudioStreamAvailable(byval stream as SDL_AudioStream ptr) as long
declare function SDL_GetAudioStreamQueued(byval stream as SDL_AudioStream ptr) as long
declare function SDL_FlushAudioStream(byval stream as SDL_AudioStream ptr) as boolean
declare function SDL_ClearAudioStream(byval stream as SDL_AudioStream ptr) as boolean
declare function SDL_PauseAudioStreamDevice(byval stream as SDL_AudioStream ptr) as boolean
declare function SDL_ResumeAudioStreamDevice(byval stream as SDL_AudioStream ptr) as boolean
declare function SDL_AudioStreamDevicePaused(byval stream as SDL_AudioStream ptr) as boolean
declare function SDL_LockAudioStream(byval stream as SDL_AudioStream ptr) as boolean
declare function SDL_UnlockAudioStream(byval stream as SDL_AudioStream ptr) as boolean
type SDL_AudioStreamCallback as sub(byval userdata as any ptr, byval stream as SDL_AudioStream ptr, byval additional_amount as long, byval total_amount as long)
declare function SDL_SetAudioStreamGetCallback(byval stream as SDL_AudioStream ptr, byval callback as SDL_AudioStreamCallback, byval userdata as any ptr) as boolean
declare function SDL_SetAudioStreamPutCallback(byval stream as SDL_AudioStream ptr, byval callback as SDL_AudioStreamCallback, byval userdata as any ptr) as boolean
declare sub SDL_DestroyAudioStream(byval stream as SDL_AudioStream ptr)
declare function SDL_OpenAudioDeviceStream(byval devid as SDL_AudioDeviceID, byval spec as const SDL_AudioSpec ptr, byval callback as SDL_AudioStreamCallback, byval userdata as any ptr) as SDL_AudioStream ptr
type SDL_AudioPostmixCallback as sub(byval userdata as any ptr, byval spec as const SDL_AudioSpec ptr, byval buffer as single ptr, byval buflen as long)
declare function SDL_SetAudioPostmixCallback(byval devid as SDL_AudioDeviceID, byval callback as SDL_AudioPostmixCallback, byval userdata as any ptr) as boolean
declare function SDL_LoadWAV_IO(byval src as SDL_IOStream ptr, byval closeio as boolean, byval spec as SDL_AudioSpec ptr, byval audio_buf as Uint8 ptr ptr, byval audio_len as Uint32 ptr) as boolean
declare function SDL_LoadWAV(byval path as const zstring ptr, byval spec as SDL_AudioSpec ptr, byval audio_buf as Uint8 ptr ptr, byval audio_len as Uint32 ptr) as boolean
declare function SDL_MixAudio(byval dst as Uint8 ptr, byval src as const Uint8 ptr, byval format as SDL_AudioFormat, byval len as Uint32, byval volume as single) as boolean
declare function SDL_ConvertAudioSamples(byval src_spec as const SDL_AudioSpec ptr, byval src_data as const Uint8 ptr, byval src_len as long, byval dst_spec as const SDL_AudioSpec ptr, byval dst_data as Uint8 ptr ptr, byval dst_len as long ptr) as boolean
declare function SDL_GetAudioFormatName(byval format as SDL_AudioFormat) as const zstring ptr
declare function SDL_GetSilenceValueForFormat(byval format as SDL_AudioFormat) as long
'' -------------------------------------------------------------------------
'' SDL_bits.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_blendmode.h
'' -------------------------------------------------------------------------
type SDL_BlendMode as Uint32
const SDL_BLENDMODE_NONE = &h00000000u
const SDL_BLENDMODE_BLEND = &h00000001u
const SDL_BLENDMODE_BLEND_PREMULTIPLIED = &h00000010u
const SDL_BLENDMODE_ADD = &h00000002u
const SDL_BLENDMODE_ADD_PREMULTIPLIED = &h00000020u
const SDL_BLENDMODE_MOD = &h00000004u
const SDL_BLENDMODE_MUL = &h00000008u
const SDL_BLENDMODE_INVALID = &h7FFFFFFFu

type SDL_BlendOperation as long
enum
	SDL_BLENDOPERATION_ADD = &h1
	SDL_BLENDOPERATION_SUBTRACT = &h2
	SDL_BLENDOPERATION_REV_SUBTRACT = &h3
	SDL_BLENDOPERATION_MINIMUM = &h4
	SDL_BLENDOPERATION_MAXIMUM = &h5
end enum

type SDL_BlendFactor as long
enum
	SDL_BLENDFACTOR_ZERO = &h1
	SDL_BLENDFACTOR_ONE = &h2
	SDL_BLENDFACTOR_SRC_COLOR = &h3
	SDL_BLENDFACTOR_ONE_MINUS_SRC_COLOR = &h4
	SDL_BLENDFACTOR_SRC_ALPHA = &h5
	SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA = &h6
	SDL_BLENDFACTOR_DST_COLOR = &h7
	SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR = &h8
	SDL_BLENDFACTOR_DST_ALPHA = &h9
	SDL_BLENDFACTOR_ONE_MINUS_DST_ALPHA = &hA
end enum

declare function SDL_ComposeCustomBlendMode(byval srcColorFactor as SDL_BlendFactor, byval dstColorFactor as SDL_BlendFactor, byval colorOperation as SDL_BlendOperation, byval srcAlphaFactor as SDL_BlendFactor, byval dstAlphaFactor as SDL_BlendFactor, byval alphaOperation as SDL_BlendOperation) as SDL_BlendMode
'' -------------------------------------------------------------------------
'' SDL_camera.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_pixels.h
'' -------------------------------------------------------------------------
const SDL_ALPHA_OPAQUE = 255
const SDL_ALPHA_OPAQUE_FLOAT = 1.0f
const SDL_ALPHA_TRANSPARENT = 0
const SDL_ALPHA_TRANSPARENT_FLOAT = 0.0f

type SDL_PixelType_ as long
enum
	SDL_PIXELTYPE_UNKNOWN
	SDL_PIXELTYPE_INDEX1
	SDL_PIXELTYPE_INDEX4
	SDL_PIXELTYPE_INDEX8
	SDL_PIXELTYPE_PACKED8
	SDL_PIXELTYPE_PACKED16
	SDL_PIXELTYPE_PACKED32
	SDL_PIXELTYPE_ARRAYU8
	SDL_PIXELTYPE_ARRAYU16
	SDL_PIXELTYPE_ARRAYU32
	SDL_PIXELTYPE_ARRAYF16
	SDL_PIXELTYPE_ARRAYF32
	SDL_PIXELTYPE_INDEX2
end enum

type SDL_BitmapOrder as long
enum
	SDL_BITMAPORDER_NONE
	SDL_BITMAPORDER_4321
	SDL_BITMAPORDER_1234
end enum

type SDL_PackedOrder as long
enum
	SDL_PACKEDORDER_NONE
	SDL_PACKEDORDER_XRGB
	SDL_PACKEDORDER_RGBX
	SDL_PACKEDORDER_ARGB
	SDL_PACKEDORDER_RGBA
	SDL_PACKEDORDER_XBGR
	SDL_PACKEDORDER_BGRX
	SDL_PACKEDORDER_ABGR
	SDL_PACKEDORDER_BGRA
end enum

type SDL_ArrayOrder as long
enum
	SDL_ARRAYORDER_NONE
	SDL_ARRAYORDER_RGB
	SDL_ARRAYORDER_RGBA
	SDL_ARRAYORDER_ARGB
	SDL_ARRAYORDER_BGR
	SDL_ARRAYORDER_BGRA
	SDL_ARRAYORDER_ABGR
end enum

type SDL_PackedLayout as long
enum
	SDL_PACKEDLAYOUT_NONE
	SDL_PACKEDLAYOUT_332
	SDL_PACKEDLAYOUT_4444
	SDL_PACKEDLAYOUT_1555
	SDL_PACKEDLAYOUT_5551
	SDL_PACKEDLAYOUT_565
	SDL_PACKEDLAYOUT_8888
	SDL_PACKEDLAYOUT_2101010
	SDL_PACKEDLAYOUT_1010102
end enum

#define SDL_DEFINE_PIXELFOURCC(A, B, C, D) SDL_FOURCC(A, B, C, D)
#define SDL_DEFINE_PIXELFORMAT(type, order, layout, bits, bytes) ((((((1 shl 28) or ((type) shl 24)) or ((order) shl 20)) or ((layout) shl 16)) or ((bits) shl 8)) or ((bytes) shl 0))
#define SDL_PIXELFLAG(format) (((format) shr 28) and &h0F)
#define SDL_PIXELTYPE(format) (((format) shr 24) and &h0F)
#define SDL_PIXELORDER(format) (((format) shr 20) and &h0F)
#define SDL_PIXELLAYOUT(format) (((format) shr 16) and &h0F)
#define SDL_BITSPERPIXEL(format) iif(SDL_ISPIXELFORMAT_FOURCC(format), 0, ((format) shr 8) and &hFF)
#define SDL_BYTESPERPIXEL(format) iif(SDL_ISPIXELFORMAT_FOURCC(format), iif(((((format) = SDL_PIXELFORMAT_YUY2) orelse ((format) = SDL_PIXELFORMAT_UYVY)) orelse ((format) = SDL_PIXELFORMAT_YVYU)) orelse ((format) = SDL_PIXELFORMAT_P010), 2, 1), ((format) shr 0) and &hFF)
#define SDL_ISPIXELFORMAT_INDEXED(format) ((SDL_ISPIXELFORMAT_FOURCC(format) = 0) andalso ((((SDL_PIXELTYPE(format) = SDL_PIXELTYPE_INDEX1) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_INDEX2)) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_INDEX4)) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_INDEX8)))
#define SDL_ISPIXELFORMAT_PACKED(format) ((SDL_ISPIXELFORMAT_FOURCC(format) = 0) andalso (((SDL_PIXELTYPE(format) = SDL_PIXELTYPE_PACKED8) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_PACKED16)) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_PACKED32)))
#define SDL_ISPIXELFORMAT_ARRAY(format) ((SDL_ISPIXELFORMAT_FOURCC(format) = 0) andalso (((((SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYU8) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYU16)) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYU32)) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYF16)) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYF32)))
#define SDL_ISPIXELFORMAT_10BIT(format) ((SDL_ISPIXELFORMAT_FOURCC(format) = 0) andalso ((SDL_PIXELTYPE(format) = SDL_PIXELTYPE_PACKED32) andalso (SDL_PIXELLAYOUT(format) = SDL_PACKEDLAYOUT_2101010)))
#define SDL_ISPIXELFORMAT_FLOAT(format) ((SDL_ISPIXELFORMAT_FOURCC(format) = 0) andalso ((SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYF16) orelse (SDL_PIXELTYPE(format) = SDL_PIXELTYPE_ARRAYF32)))
#define SDL_ISPIXELFORMAT_ALPHA(format) ((SDL_ISPIXELFORMAT_PACKED(format) andalso ((((SDL_PIXELORDER(format) = SDL_PACKEDORDER_ARGB) orelse (SDL_PIXELORDER(format) = SDL_PACKEDORDER_RGBA)) orelse (SDL_PIXELORDER(format) = SDL_PACKEDORDER_ABGR)) orelse (SDL_PIXELORDER(format) = SDL_PACKEDORDER_BGRA))) orelse (SDL_ISPIXELFORMAT_ARRAY(format) andalso ((((SDL_PIXELORDER(format) = SDL_ARRAYORDER_ARGB) orelse (SDL_PIXELORDER(format) = SDL_ARRAYORDER_RGBA)) orelse (SDL_PIXELORDER(format) = SDL_ARRAYORDER_ABGR)) orelse (SDL_PIXELORDER(format) = SDL_ARRAYORDER_BGRA))))
#define SDL_ISPIXELFORMAT_FOURCC(format) ((format) andalso (SDL_PIXELFLAG(format) <> 1))

type SDL_PixelFormat as long
enum
	SDL_PIXELFORMAT_UNKNOWN = 0
	SDL_PIXELFORMAT_INDEX1LSB = &h11100100u
	SDL_PIXELFORMAT_INDEX1MSB = &h11200100u
	SDL_PIXELFORMAT_INDEX2LSB = &h1c100200u
	SDL_PIXELFORMAT_INDEX2MSB = &h1c200200u
	SDL_PIXELFORMAT_INDEX4LSB = &h12100400u
	SDL_PIXELFORMAT_INDEX4MSB = &h12200400u
	SDL_PIXELFORMAT_INDEX8 = &h13000801u
	SDL_PIXELFORMAT_RGB332 = &h14110801u
	SDL_PIXELFORMAT_XRGB4444 = &h15120c02u
	SDL_PIXELFORMAT_XBGR4444 = &h15520c02u
	SDL_PIXELFORMAT_XRGB1555 = &h15130f02u
	SDL_PIXELFORMAT_XBGR1555 = &h15530f02u
	SDL_PIXELFORMAT_ARGB4444 = &h15321002u
	SDL_PIXELFORMAT_RGBA4444 = &h15421002u
	SDL_PIXELFORMAT_ABGR4444 = &h15721002u
	SDL_PIXELFORMAT_BGRA4444 = &h15821002u
	SDL_PIXELFORMAT_ARGB1555 = &h15331002u
	SDL_PIXELFORMAT_RGBA5551 = &h15441002u
	SDL_PIXELFORMAT_ABGR1555 = &h15731002u
	SDL_PIXELFORMAT_BGRA5551 = &h15841002u
	SDL_PIXELFORMAT_RGB565 = &h15151002u
	SDL_PIXELFORMAT_BGR565 = &h15551002u
	SDL_PIXELFORMAT_RGB24 = &h17101803u
	SDL_PIXELFORMAT_BGR24 = &h17401803u
	SDL_PIXELFORMAT_XRGB8888 = &h16161804u
	SDL_PIXELFORMAT_RGBX8888 = &h16261804u
	SDL_PIXELFORMAT_XBGR8888 = &h16561804u
	SDL_PIXELFORMAT_BGRX8888 = &h16661804u
	SDL_PIXELFORMAT_ARGB8888 = &h16362004u
	SDL_PIXELFORMAT_RGBA8888 = &h16462004u
	SDL_PIXELFORMAT_ABGR8888 = &h16762004u
	SDL_PIXELFORMAT_BGRA8888 = &h16862004u
	SDL_PIXELFORMAT_XRGB2101010 = &h16172004u
	SDL_PIXELFORMAT_XBGR2101010 = &h16572004u
	SDL_PIXELFORMAT_ARGB2101010 = &h16372004u
	SDL_PIXELFORMAT_ABGR2101010 = &h16772004u
	SDL_PIXELFORMAT_RGB48 = &h18103006u
	SDL_PIXELFORMAT_BGR48 = &h18403006u
	SDL_PIXELFORMAT_RGBA64 = &h18204008u
	SDL_PIXELFORMAT_ARGB64 = &h18304008u
	SDL_PIXELFORMAT_BGRA64 = &h18504008u
	SDL_PIXELFORMAT_ABGR64 = &h18604008u
	SDL_PIXELFORMAT_RGB48_FLOAT = &h1a103006u
	SDL_PIXELFORMAT_BGR48_FLOAT = &h1a403006u
	SDL_PIXELFORMAT_RGBA64_FLOAT = &h1a204008u
	SDL_PIXELFORMAT_ARGB64_FLOAT = &h1a304008u
	SDL_PIXELFORMAT_BGRA64_FLOAT = &h1a504008u
	SDL_PIXELFORMAT_ABGR64_FLOAT = &h1a604008u
	SDL_PIXELFORMAT_RGB96_FLOAT = &h1b10600cu
	SDL_PIXELFORMAT_BGR96_FLOAT = &h1b40600cu
	SDL_PIXELFORMAT_RGBA128_FLOAT = &h1b208010u
	SDL_PIXELFORMAT_ARGB128_FLOAT = &h1b308010u
	SDL_PIXELFORMAT_BGRA128_FLOAT = &h1b508010u
	SDL_PIXELFORMAT_ABGR128_FLOAT = &h1b608010u
	SDL_PIXELFORMAT_YV12 = &h32315659u
	SDL_PIXELFORMAT_IYUV = &h56555949u
	SDL_PIXELFORMAT_YUY2 = &h32595559u
	SDL_PIXELFORMAT_UYVY = &h59565955u
	SDL_PIXELFORMAT_YVYU = &h55595659u
	SDL_PIXELFORMAT_NV12 = &h3231564eu
	SDL_PIXELFORMAT_NV21 = &h3132564eu
	SDL_PIXELFORMAT_P010 = &h30313050u
	SDL_PIXELFORMAT_EXTERNAL_OES = &h2053454fu
	SDL_PIXELFORMAT_MJPG = &h47504a4du
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_RGBA32 = SDL_PIXELFORMAT_RGBA8888
#else
SDL_PIXELFORMAT_RGBA32 = SDL_PIXELFORMAT_ABGR8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_ARGB32 = SDL_PIXELFORMAT_ARGB8888
#else
SDL_PIXELFORMAT_ARGB32 = SDL_PIXELFORMAT_BGRA8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_BGRA32 = SDL_PIXELFORMAT_BGRA8888
#else
SDL_PIXELFORMAT_BGRA32 = SDL_PIXELFORMAT_ARGB8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_ABGR32 = SDL_PIXELFORMAT_ABGR8888
#else
SDL_PIXELFORMAT_ABGR32 = SDL_PIXELFORMAT_RGBA8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_RGBX32 = SDL_PIXELFORMAT_RGBX8888
#else
SDL_PIXELFORMAT_RGBX32 = SDL_PIXELFORMAT_XBGR8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_XRGB32 = SDL_PIXELFORMAT_XRGB8888
#else
SDL_PIXELFORMAT_XRGB32 = SDL_PIXELFORMAT_BGRX8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_BGRX32 = SDL_PIXELFORMAT_BGRX8888
#else
SDL_PIXELFORMAT_BGRX32 = SDL_PIXELFORMAT_XRGB8888
#endif
#ifdef __FB_BIGENDIAN__
	SDL_PIXELFORMAT_XBGR32 = SDL_PIXELFORMAT_XBGR8888
#else
SDL_PIXELFORMAT_XBGR32 = SDL_PIXELFORMAT_RGBX8888
#endif
end enum

type SDL_ColorType as long
enum
	SDL_COLOR_TYPE_UNKNOWN = 0
	SDL_COLOR_TYPE_RGB = 1
	SDL_COLOR_TYPE_YCBCR = 2
end enum

type SDL_ColorRange as long
enum
	SDL_COLOR_RANGE_UNKNOWN = 0
	SDL_COLOR_RANGE_LIMITED = 1
	SDL_COLOR_RANGE_FULL = 2
end enum

type SDL_ColorPrimaries as long
enum
	SDL_COLOR_PRIMARIES_UNKNOWN = 0
	SDL_COLOR_PRIMARIES_BT709 = 1
	SDL_COLOR_PRIMARIES_UNSPECIFIED = 2
	SDL_COLOR_PRIMARIES_BT470M = 4
	SDL_COLOR_PRIMARIES_BT470BG = 5
	SDL_COLOR_PRIMARIES_BT601 = 6
	SDL_COLOR_PRIMARIES_SMPTE240 = 7
	SDL_COLOR_PRIMARIES_GENERIC_FILM = 8
	SDL_COLOR_PRIMARIES_BT2020 = 9
	SDL_COLOR_PRIMARIES_XYZ = 10
	SDL_COLOR_PRIMARIES_SMPTE431 = 11
	SDL_COLOR_PRIMARIES_SMPTE432 = 12
	SDL_COLOR_PRIMARIES_EBU3213 = 22
	SDL_COLOR_PRIMARIES_CUSTOM = 31
end enum

type SDL_TransferCharacteristics as long
enum
	SDL_TRANSFER_CHARACTERISTICS_UNKNOWN = 0
	SDL_TRANSFER_CHARACTERISTICS_BT709 = 1
	SDL_TRANSFER_CHARACTERISTICS_UNSPECIFIED = 2
	SDL_TRANSFER_CHARACTERISTICS_GAMMA22 = 4
	SDL_TRANSFER_CHARACTERISTICS_GAMMA28 = 5
	SDL_TRANSFER_CHARACTERISTICS_BT601 = 6
	SDL_TRANSFER_CHARACTERISTICS_SMPTE240 = 7
	SDL_TRANSFER_CHARACTERISTICS_LINEAR = 8
	SDL_TRANSFER_CHARACTERISTICS_LOG100 = 9
	SDL_TRANSFER_CHARACTERISTICS_LOG100_SQRT10 = 10
	SDL_TRANSFER_CHARACTERISTICS_IEC61966 = 11
	SDL_TRANSFER_CHARACTERISTICS_BT1361 = 12
	SDL_TRANSFER_CHARACTERISTICS_SRGB = 13
	SDL_TRANSFER_CHARACTERISTICS_BT2020_10BIT = 14
	SDL_TRANSFER_CHARACTERISTICS_BT2020_12BIT = 15
	SDL_TRANSFER_CHARACTERISTICS_PQ = 16
	SDL_TRANSFER_CHARACTERISTICS_SMPTE428 = 17
	SDL_TRANSFER_CHARACTERISTICS_HLG = 18
	SDL_TRANSFER_CHARACTERISTICS_CUSTOM = 31
end enum

type SDL_MatrixCoefficients as long
enum
	SDL_MATRIX_COEFFICIENTS_IDENTITY = 0
	SDL_MATRIX_COEFFICIENTS_BT709 = 1
	SDL_MATRIX_COEFFICIENTS_UNSPECIFIED = 2
	SDL_MATRIX_COEFFICIENTS_FCC = 4
	SDL_MATRIX_COEFFICIENTS_BT470BG = 5
	SDL_MATRIX_COEFFICIENTS_BT601 = 6
	SDL_MATRIX_COEFFICIENTS_SMPTE240 = 7
	SDL_MATRIX_COEFFICIENTS_YCGCO = 8
	SDL_MATRIX_COEFFICIENTS_BT2020_NCL = 9
	SDL_MATRIX_COEFFICIENTS_BT2020_CL = 10
	SDL_MATRIX_COEFFICIENTS_SMPTE2085 = 11
	SDL_MATRIX_COEFFICIENTS_CHROMA_DERIVED_NCL = 12
	SDL_MATRIX_COEFFICIENTS_CHROMA_DERIVED_CL = 13
	SDL_MATRIX_COEFFICIENTS_ICTCP = 14
	SDL_MATRIX_COEFFICIENTS_CUSTOM = 31
end enum

type SDL_ChromaLocation as long
enum
	SDL_CHROMA_LOCATION_NONE = 0
	SDL_CHROMA_LOCATION_LEFT = 1
	SDL_CHROMA_LOCATION_CENTER = 2
	SDL_CHROMA_LOCATION_TOPLEFT = 3
end enum

#define SDL_DEFINE_COLORSPACE(type, range, primaries, transfer, matrix, chroma) ((((((cast(Uint32, (type)) shl 28) or (cast(Uint32, (range)) shl 24)) or (cast(Uint32, (chroma)) shl 20)) or (cast(Uint32, (primaries)) shl 10)) or (cast(Uint32, (transfer)) shl 5)) or (cast(Uint32, (matrix)) shl 0))
#define SDL_COLORSPACETYPE(cspace) cast(SDL_ColorType, ((cspace) shr 28) and &h0F)
#define SDL_COLORSPACERANGE(cspace) cast(SDL_ColorRange, ((cspace) shr 24) and &h0F)
#define SDL_COLORSPACECHROMA(cspace) cast(SDL_ChromaLocation, ((cspace) shr 20) and &h0F)
#define SDL_COLORSPACEPRIMARIES(cspace) cast(SDL_ColorPrimaries, ((cspace) shr 10) and &h1F)
#define SDL_COLORSPACETRANSFER(cspace) cast(SDL_TransferCharacteristics, ((cspace) shr 5) and &h1F)
#define SDL_COLORSPACEMATRIX(cspace) cast(SDL_MatrixCoefficients, (cspace) and &h1F)
#define SDL_ISCOLORSPACE_MATRIX_BT601(cspace) ((SDL_COLORSPACEMATRIX(cspace) = SDL_MATRIX_COEFFICIENTS_BT601) orelse (SDL_COLORSPACEMATRIX(cspace) = SDL_MATRIX_COEFFICIENTS_BT470BG))
#define SDL_ISCOLORSPACE_MATRIX_BT709(cspace) (SDL_COLORSPACEMATRIX(cspace) = SDL_MATRIX_COEFFICIENTS_BT709)
#define SDL_ISCOLORSPACE_MATRIX_BT2020_NCL(cspace) (SDL_COLORSPACEMATRIX(cspace) = SDL_MATRIX_COEFFICIENTS_BT2020_NCL)
#define SDL_ISCOLORSPACE_LIMITED_RANGE(cspace) (SDL_COLORSPACERANGE(cspace) <> SDL_COLOR_RANGE_FULL)
#define SDL_ISCOLORSPACE_FULL_RANGE(cspace) (SDL_COLORSPACERANGE(cspace) = SDL_COLOR_RANGE_FULL)

type SDL_Colorspace as long
enum
	SDL_COLORSPACE_UNKNOWN = 0
	SDL_COLORSPACE_SRGB = &h120005a0u
	SDL_COLORSPACE_SRGB_LINEAR = &h12000500u
	SDL_COLORSPACE_HDR10 = &h12002600u
	SDL_COLORSPACE_JPEG = &h220004c6u
	SDL_COLORSPACE_BT601_LIMITED = &h211018c6u
	SDL_COLORSPACE_BT601_FULL = &h221018c6u
	SDL_COLORSPACE_BT709_LIMITED = &h21100421u
	SDL_COLORSPACE_BT709_FULL = &h22100421u
	SDL_COLORSPACE_BT2020_LIMITED = &h21102609u
	SDL_COLORSPACE_BT2020_FULL = &h22102609u
	SDL_COLORSPACE_RGB_DEFAULT = SDL_COLORSPACE_SRGB
	SDL_COLORSPACE_YUV_DEFAULT = SDL_COLORSPACE_BT601_LIMITED
end enum

type SDL_Color
	r as Uint8
	g as Uint8
	b as Uint8
	a as Uint8
end type

type SDL_FColor
	r as single
	g as single
	b as single
	a as single
end type

type SDL_Palette
	ncolors as long
	colors as SDL_Color ptr
	version as Uint32
	refcount as long
end type

type SDL_PixelFormatDetails
	format as SDL_PixelFormat
	bits_per_pixel as Uint8
	bytes_per_pixel as Uint8
	padding(0 to 1) as Uint8
	Rmask as Uint32
	Gmask as Uint32
	Bmask as Uint32
	Amask as Uint32
	Rbits as Uint8
	Gbits as Uint8
	Bbits as Uint8
	Abits as Uint8
	Rshift as Uint8
	Gshift as Uint8
	Bshift as Uint8
	Ashift as Uint8
end type

declare function SDL_GetPixelFormatName(byval format as SDL_PixelFormat) as const zstring ptr
declare function SDL_GetMasksForPixelFormat(byval format as SDL_PixelFormat, byval bpp as long ptr, byval Rmask as Uint32 ptr, byval Gmask as Uint32 ptr, byval Bmask as Uint32 ptr, byval Amask as Uint32 ptr) as boolean
declare function SDL_GetPixelFormatForMasks(byval bpp as long, byval Rmask as Uint32, byval Gmask as Uint32, byval Bmask as Uint32, byval Amask as Uint32) as SDL_PixelFormat
declare function SDL_GetPixelFormatDetails(byval format as SDL_PixelFormat) as const SDL_PixelFormatDetails ptr
declare function SDL_CreatePalette(byval ncolors as long) as SDL_Palette ptr
declare function SDL_SetPaletteColors(byval palette as SDL_Palette ptr, byval colors as const SDL_Color ptr, byval firstcolor as long, byval ncolors as long) as boolean
declare sub SDL_DestroyPalette(byval palette as SDL_Palette ptr)
declare function SDL_MapRGB(byval format as const SDL_PixelFormatDetails ptr, byval palette as const SDL_Palette ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8) as Uint32
declare function SDL_MapRGBA(byval format as const SDL_PixelFormatDetails ptr, byval palette as const SDL_Palette ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as Uint32
declare sub SDL_GetRGB(byval pixelvalue as Uint32, byval format as const SDL_PixelFormatDetails ptr, byval palette as const SDL_Palette ptr, byval r as Uint8 ptr, byval g as Uint8 ptr, byval b as Uint8 ptr)
declare sub SDL_GetRGBA(byval pixelvalue as Uint32, byval format as const SDL_PixelFormatDetails ptr, byval palette as const SDL_Palette ptr, byval r as Uint8 ptr, byval g as Uint8 ptr, byval b as Uint8 ptr, byval a as Uint8 ptr)

'' -------------------------------------------------------------------------
'' SDL_surface.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_rect.h
'' -------------------------------------------------------------------------
type SDL_Point
	x as long
	y as long
end type

type SDL_FPoint
	x as single
	y as single
end type

type SDL_Rect
	x as long
	y as long
	w as long
	h as long
end type

type SDL_FRect
	x as single
	y as single
	w as single
	h as single
end type

declare function SDL_HasRectIntersection(byval A as const SDL_Rect ptr, byval B as const SDL_Rect ptr) as boolean
declare function SDL_GetRectIntersection(byval A as const SDL_Rect ptr, byval B as const SDL_Rect ptr, byval result as SDL_Rect ptr) as boolean
declare function SDL_GetRectUnion(byval A as const SDL_Rect ptr, byval B as const SDL_Rect ptr, byval result as SDL_Rect ptr) as boolean
declare function SDL_GetRectEnclosingPoints(byval points as const SDL_Point ptr, byval count as long, byval clip as const SDL_Rect ptr, byval result as SDL_Rect ptr) as boolean
declare function SDL_GetRectAndLineIntersection(byval rect as const SDL_Rect ptr, byval X1 as long ptr, byval Y1 as long ptr, byval X2 as long ptr, byval Y2 as long ptr) as boolean

declare function SDL_HasRectIntersectionFloat(byval A as const SDL_FRect ptr, byval B as const SDL_FRect ptr) as boolean
declare function SDL_GetRectIntersectionFloat(byval A as const SDL_FRect ptr, byval B as const SDL_FRect ptr, byval result as SDL_FRect ptr) as boolean
declare function SDL_GetRectUnionFloat(byval A as const SDL_FRect ptr, byval B as const SDL_FRect ptr, byval result as SDL_FRect ptr) as boolean
declare function SDL_GetRectEnclosingPointsFloat(byval points as const SDL_FPoint ptr, byval count as long, byval clip as const SDL_FRect ptr, byval result as SDL_FRect ptr) as boolean
declare function SDL_GetRectAndLineIntersectionFloat(byval rect as const SDL_FRect ptr, byval X1 as single ptr, byval Y1 as single ptr, byval X2 as single ptr, byval Y2 as single ptr) as boolean
type SDL_SurfaceFlags as Uint32

const SDL_SURFACE_PREALLOCATED = &h00000001u
const SDL_SURFACE_LOCK_NEEDED = &h00000002u
const SDL_SURFACE_LOCKED = &h00000004u
const SDL_SURFACE_SIMD_ALIGNED = &h00000008u
#define SDL_MUSTLOCK(S) (((S)->flags and SDL_SURFACE_LOCK_NEEDED) = SDL_SURFACE_LOCK_NEEDED)

type SDL_ScaleMode as long
enum
	SDL_SCALEMODE_INVALID = -1
	SDL_SCALEMODE_NEAREST
	SDL_SCALEMODE_LINEAR
	SDL_SCALEMODE_PIXELART
end enum

type SDL_FlipMode as long
enum
	SDL_FLIP_NONE
	SDL_FLIP_HORIZONTAL
	SDL_FLIP_VERTICAL
	SDL_FLIP_HORIZONTAL_AND_VERTICAL = SDL_FLIP_HORIZONTAL or SDL_FLIP_VERTICAL
end enum

type SDL_Surface
	flags as SDL_SurfaceFlags
	format as SDL_PixelFormat
	w as long
	h as long
	pitch as long
	pixels as any ptr
	refcount as long
	reserved as any ptr
end type

declare function SDL_CreateSurface(byval width as long, byval height as long, byval format as SDL_PixelFormat) as SDL_Surface ptr
declare function SDL_CreateSurfaceFrom(byval width as long, byval height as long, byval format as SDL_PixelFormat, byval pixels as any ptr, byval pitch as long) as SDL_Surface ptr
declare sub SDL_DestroySurface(byval surface as SDL_Surface ptr)
declare function SDL_GetSurfaceProperties(byval surface as SDL_Surface ptr) as SDL_PropertiesID

#define SDL_PROP_SURFACE_SDR_WHITE_POINT_FLOAT "SDL.surface.SDR_white_point"
#define SDL_PROP_SURFACE_HDR_HEADROOM_FLOAT "SDL.surface.HDR_headroom"
#define SDL_PROP_SURFACE_TONEMAP_OPERATOR_STRING "SDL.surface.tonemap"
#define SDL_PROP_SURFACE_HOTSPOT_X_NUMBER "SDL.surface.hotspot.x"
#define SDL_PROP_SURFACE_HOTSPOT_Y_NUMBER "SDL.surface.hotspot.y"
#define SDL_PROP_SURFACE_ROTATION_FLOAT "SDL.surface.rotation"

declare function SDL_SetSurfaceColorspace(byval surface as SDL_Surface ptr, byval colorspace as SDL_Colorspace) as boolean
declare function SDL_GetSurfaceColorspace(byval surface as SDL_Surface ptr) as SDL_Colorspace
declare function SDL_CreateSurfacePalette(byval surface as SDL_Surface ptr) as SDL_Palette ptr
declare function SDL_SetSurfacePalette(byval surface as SDL_Surface ptr, byval palette as SDL_Palette ptr) as boolean
declare function SDL_GetSurfacePalette(byval surface as SDL_Surface ptr) as SDL_Palette ptr
declare function SDL_AddSurfaceAlternateImage(byval surface as SDL_Surface ptr, byval image as SDL_Surface ptr) as boolean
declare function SDL_SurfaceHasAlternateImages(byval surface as SDL_Surface ptr) as boolean
declare function SDL_GetSurfaceImages(byval surface as SDL_Surface ptr, byval count as long ptr) as SDL_Surface ptr ptr
declare sub SDL_RemoveSurfaceAlternateImages(byval surface as SDL_Surface ptr)
declare function SDL_LockSurface(byval surface as SDL_Surface ptr) as boolean
declare sub SDL_UnlockSurface(byval surface as SDL_Surface ptr)
declare function SDL_LoadSurface_IO(byval src as SDL_IOStream ptr, byval closeio as boolean) as SDL_Surface ptr
declare function SDL_LoadSurface(byval file as const zstring ptr) as SDL_Surface ptr
declare function SDL_LoadBMP_IO(byval src as SDL_IOStream ptr, byval closeio as boolean) as SDL_Surface ptr
declare function SDL_LoadBMP(byval file as const zstring ptr) as SDL_Surface ptr
declare function SDL_SaveBMP_IO(byval surface as SDL_Surface ptr, byval dst as SDL_IOStream ptr, byval closeio as boolean) as boolean
declare function SDL_SaveBMP(byval surface as SDL_Surface ptr, byval file as const zstring ptr) as boolean
declare function SDL_LoadPNG_IO(byval src as SDL_IOStream ptr, byval closeio as boolean) as SDL_Surface ptr
declare function SDL_LoadPNG(byval file as const zstring ptr) as SDL_Surface ptr
declare function SDL_SavePNG_IO(byval surface as SDL_Surface ptr, byval dst as SDL_IOStream ptr, byval closeio as boolean) as boolean
declare function SDL_SavePNG(byval surface as SDL_Surface ptr, byval file as const zstring ptr) as boolean
declare function SDL_SetSurfaceRLE(byval surface as SDL_Surface ptr, byval enabled as boolean) as boolean
declare function SDL_SurfaceHasRLE(byval surface as SDL_Surface ptr) as boolean
declare function SDL_SetSurfaceColorKey(byval surface as SDL_Surface ptr, byval enabled as boolean, byval key as Uint32) as boolean
declare function SDL_SurfaceHasColorKey(byval surface as SDL_Surface ptr) as boolean
declare function SDL_GetSurfaceColorKey(byval surface as SDL_Surface ptr, byval key as Uint32 ptr) as boolean
declare function SDL_SetSurfaceColorMod(byval surface as SDL_Surface ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8) as boolean
declare function SDL_GetSurfaceColorMod(byval surface as SDL_Surface ptr, byval r as Uint8 ptr, byval g as Uint8 ptr, byval b as Uint8 ptr) as boolean
declare function SDL_SetSurfaceAlphaMod(byval surface as SDL_Surface ptr, byval alpha as Uint8) as boolean
declare function SDL_GetSurfaceAlphaMod(byval surface as SDL_Surface ptr, byval alpha as Uint8 ptr) as boolean
declare function SDL_SetSurfaceBlendMode(byval surface as SDL_Surface ptr, byval blendMode as SDL_BlendMode) as boolean
declare function SDL_GetSurfaceBlendMode(byval surface as SDL_Surface ptr, byval blendMode as SDL_BlendMode ptr) as boolean
declare function SDL_SetSurfaceClipRect(byval surface as SDL_Surface ptr, byval rect as const SDL_Rect ptr) as boolean
declare function SDL_GetSurfaceClipRect(byval surface as SDL_Surface ptr, byval rect as SDL_Rect ptr) as boolean
declare function SDL_FlipSurface(byval surface as SDL_Surface ptr, byval flip as SDL_FlipMode) as boolean
declare function SDL_RotateSurface(byval surface as SDL_Surface ptr, byval angle as single) as SDL_Surface ptr
declare function SDL_DuplicateSurface(byval surface as SDL_Surface ptr) as SDL_Surface ptr
declare function SDL_ScaleSurface(byval surface as SDL_Surface ptr, byval width as long, byval height as long, byval scaleMode as SDL_ScaleMode) as SDL_Surface ptr
declare function SDL_ConvertSurface(byval surface as SDL_Surface ptr, byval format as SDL_PixelFormat) as SDL_Surface ptr
declare function SDL_ConvertSurfaceAndColorspace(byval surface as SDL_Surface ptr, byval format as SDL_PixelFormat, byval palette as SDL_Palette ptr, byval colorspace as SDL_Colorspace, byval props as SDL_PropertiesID) as SDL_Surface ptr
declare function SDL_ConvertPixels(byval width as long, byval height as long, byval src_format as SDL_PixelFormat, byval src as const any ptr, byval src_pitch as long, byval dst_format as SDL_PixelFormat, byval dst as any ptr, byval dst_pitch as long) as boolean
declare function SDL_ConvertPixelsAndColorspace(byval width as long, byval height as long, byval src_format as SDL_PixelFormat, byval src_colorspace as SDL_Colorspace, byval src_properties as SDL_PropertiesID, byval src as const any ptr, byval src_pitch as long, byval dst_format as SDL_PixelFormat, byval dst_colorspace as SDL_Colorspace, byval dst_properties as SDL_PropertiesID, byval dst as any ptr, byval dst_pitch as long) as boolean
declare function SDL_PremultiplyAlpha(byval width as long, byval height as long, byval src_format as SDL_PixelFormat, byval src as const any ptr, byval src_pitch as long, byval dst_format as SDL_PixelFormat, byval dst as any ptr, byval dst_pitch as long, byval linear as boolean) as boolean
declare function SDL_PremultiplySurfaceAlpha(byval surface as SDL_Surface ptr, byval linear as boolean) as boolean
declare function SDL_ClearSurface(byval surface as SDL_Surface ptr, byval r as single, byval g as single, byval b as single, byval a as single) as boolean
declare function SDL_FillSurfaceRect(byval dst as SDL_Surface ptr, byval rect as const SDL_Rect ptr, byval color as Uint32) as boolean
declare function SDL_FillSurfaceRects(byval dst as SDL_Surface ptr, byval rects as const SDL_Rect ptr, byval count as long, byval color as Uint32) as boolean
declare function SDL_BlitSurface(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr) as boolean
declare function SDL_BlitSurfaceUnchecked(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr) as boolean
declare function SDL_BlitSurfaceScaled(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr, byval scaleMode as SDL_ScaleMode) as boolean
declare function SDL_BlitSurfaceUncheckedScaled(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr, byval scaleMode as SDL_ScaleMode) as boolean
declare function SDL_StretchSurface(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr, byval scaleMode as SDL_ScaleMode) as boolean
declare function SDL_BlitSurfaceTiled(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr) as boolean
declare function SDL_BlitSurfaceTiledWithScale(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval scale as single, byval scaleMode as SDL_ScaleMode, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr) as boolean
declare function SDL_BlitSurface9Grid(byval src as SDL_Surface ptr, byval srcrect as const SDL_Rect ptr, byval left_width as long, byval right_width as long, byval top_height as long, byval bottom_height as long, byval scale as single, byval scaleMode as SDL_ScaleMode, byval dst as SDL_Surface ptr, byval dstrect as const SDL_Rect ptr) as boolean
declare function SDL_MapSurfaceRGB(byval surface as SDL_Surface ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8) as Uint32
declare function SDL_MapSurfaceRGBA(byval surface as SDL_Surface ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as Uint32
declare function SDL_ReadSurfacePixel(byval surface as SDL_Surface ptr, byval x as long, byval y as long, byval r as Uint8 ptr, byval g as Uint8 ptr, byval b as Uint8 ptr, byval a as Uint8 ptr) as boolean
declare function SDL_ReadSurfacePixelFloat(byval surface as SDL_Surface ptr, byval x as long, byval y as long, byval r as single ptr, byval g as single ptr, byval b as single ptr, byval a as single ptr) as boolean
declare function SDL_WriteSurfacePixel(byval surface as SDL_Surface ptr, byval x as long, byval y as long, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function SDL_WriteSurfacePixelFloat(byval surface as SDL_Surface ptr, byval x as long, byval y as long, byval r as single, byval g as single, byval b as single, byval a as single) as boolean
type SDL_CameraID as Uint32

type SDL_CameraSpec
	format as SDL_PixelFormat
	colorspace as SDL_Colorspace
	width as long
	height as long
	framerate_numerator as long
	framerate_denominator as long
end type

type SDL_CameraPosition as long
enum
	SDL_CAMERA_POSITION_UNKNOWN
	SDL_CAMERA_POSITION_FRONT_FACING
	SDL_CAMERA_POSITION_BACK_FACING
end enum

type SDL_CameraPermissionState as long
enum
	SDL_CAMERA_PERMISSION_STATE_DENIED = -1
	SDL_CAMERA_PERMISSION_STATE_PENDING
	SDL_CAMERA_PERMISSION_STATE_APPROVED
end enum

declare function SDL_GetNumCameraDrivers() as long
declare function SDL_GetCameraDriver(byval index as long) as const zstring ptr
declare function SDL_GetCurrentCameraDriver() as const zstring ptr
declare function SDL_GetCameras(byval count as long ptr) as SDL_CameraID ptr
declare function SDL_GetCameraSupportedFormats(byval instance_id as SDL_CameraID, byval count as long ptr) as SDL_CameraSpec ptr ptr
declare function SDL_GetCameraName(byval instance_id as SDL_CameraID) as const zstring ptr
declare function SDL_GetCameraPosition(byval instance_id as SDL_CameraID) as SDL_CameraPosition
declare function SDL_OpenCamera(byval instance_id as SDL_CameraID, byval spec as const SDL_CameraSpec ptr) as SDL_Camera ptr
declare function SDL_GetCameraPermissionState(byval camera as SDL_Camera ptr) as SDL_CameraPermissionState
declare function SDL_GetCameraID(byval camera as SDL_Camera ptr) as SDL_CameraID
declare function SDL_GetCameraProperties(byval camera as SDL_Camera ptr) as SDL_PropertiesID
declare function SDL_GetCameraFormat(byval camera as SDL_Camera ptr, byval spec as SDL_CameraSpec ptr) as boolean
declare function SDL_AcquireCameraFrame(byval camera as SDL_Camera ptr, byval timestampNS as Uint64 ptr) as SDL_Surface ptr
declare sub SDL_ReleaseCameraFrame(byval camera as SDL_Camera ptr, byval frame as SDL_Surface ptr)
declare sub SDL_CloseCamera(byval camera as SDL_Camera ptr)
'' -------------------------------------------------------------------------
'' SDL_clipboard.h
'' -------------------------------------------------------------------------
declare function SDL_SetClipboardText(byval text as const zstring ptr) as boolean
declare function SDL_GetClipboardText() as zstring ptr
declare function SDL_HasClipboardText() as boolean
declare function SDL_SetPrimarySelectionText(byval text as const zstring ptr) as boolean
declare function SDL_GetPrimarySelectionText() as zstring ptr
declare function SDL_HasPrimarySelectionText() as boolean
type SDL_ClipboardDataCallback as function(byval userdata as any ptr, byval mime_type as const zstring ptr, byval size as uinteger ptr) as const any ptr
type SDL_ClipboardCleanupCallback as sub(byval userdata as any ptr)
declare function SDL_SetClipboardData(byval callback as SDL_ClipboardDataCallback, byval cleanup as SDL_ClipboardCleanupCallback, byval userdata as any ptr, byval mime_types as const zstring const ptr ptr, byval num_mime_types as uinteger) as boolean
declare function SDL_ClearClipboardData() as boolean
declare function SDL_GetClipboardData(byval mime_type as const zstring ptr, byval size as uinteger ptr) as any ptr
declare function SDL_HasClipboardData(byval mime_type as const zstring ptr) as boolean
declare function SDL_GetClipboardMimeTypes(byval num_mime_types as uinteger ptr) as zstring ptr ptr

'' -------------------------------------------------------------------------
'' SDL_cpuinfo.h
'' -------------------------------------------------------------------------
const SDL_CACHELINE_SIZE = 128

declare function SDL_GetNumLogicalCPUCores() as long
declare function SDL_GetCPUCacheLineSize() as long
declare function SDL_HasAltiVec() as boolean
declare function SDL_HasMMX() as boolean
declare function SDL_HasSSE() as boolean
declare function SDL_HasSSE2() as boolean
declare function SDL_HasSSE3() as boolean
declare function SDL_HasSSE41() as boolean
declare function SDL_HasSSE42() as boolean
declare function SDL_HasAVX() as boolean
declare function SDL_HasAVX2() as boolean
declare function SDL_HasAVX512F() as boolean
declare function SDL_HasARMSIMD() as boolean
declare function SDL_HasNEON() as boolean
declare function SDL_HasLSX() as boolean
declare function SDL_HasLASX() as boolean
declare function SDL_GetSystemRAM() as long
declare function SDL_GetSIMDAlignment() as uinteger
declare function SDL_GetSystemPageSize() as long

'' -------------------------------------------------------------------------
'' SDL_dialog.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_video.h
'' -------------------------------------------------------------------------
type SDL_DisplayID as Uint32
type SDL_WindowID as Uint32
#define SDL_PROP_GLOBAL_VIDEO_WAYLAND_WL_DISPLAY_POINTER "SDL.video.wayland.wl_display"

type SDL_SystemTheme as long
enum
	SDL_SYSTEM_THEME_UNKNOWN
	SDL_SYSTEM_THEME_LIGHT
	SDL_SYSTEM_THEME_DARK
end enum

type SDL_DisplayMode
	displayID as SDL_DisplayID
	format as SDL_PixelFormat
	w as long
	h as long
	pixel_density as single
	refresh_rate as single
	refresh_rate_numerator as long
	refresh_rate_denominator as long
	internal as SDL_DisplayModeData ptr
end type

type SDL_DisplayOrientation as long
enum
	SDL_ORIENTATION_UNKNOWN
	SDL_ORIENTATION_LANDSCAPE
	SDL_ORIENTATION_LANDSCAPE_FLIPPED
	SDL_ORIENTATION_PORTRAIT
	SDL_ORIENTATION_PORTRAIT_FLIPPED
end enum

type SDL_WindowFlags as Uint64
#define SDL_WINDOW_FULLSCREEN SDL_UINT64_C(&h0000000000000001)
#define SDL_WINDOW_OPENGL SDL_UINT64_C(&h0000000000000002)
#define SDL_WINDOW_OCCLUDED SDL_UINT64_C(&h0000000000000004)
#define SDL_WINDOW_HIDDEN SDL_UINT64_C(&h0000000000000008)
#define SDL_WINDOW_BORDERLESS SDL_UINT64_C(&h0000000000000010)
#define SDL_WINDOW_RESIZABLE SDL_UINT64_C(&h0000000000000020)
#define SDL_WINDOW_MINIMIZED SDL_UINT64_C(&h0000000000000040)
#define SDL_WINDOW_MAXIMIZED SDL_UINT64_C(&h0000000000000080)
#define SDL_WINDOW_MOUSE_GRABBED SDL_UINT64_C(&h0000000000000100)
#define SDL_WINDOW_INPUT_FOCUS SDL_UINT64_C(&h0000000000000200)
#define SDL_WINDOW_MOUSE_FOCUS SDL_UINT64_C(&h0000000000000400)
#define SDL_WINDOW_EXTERNAL SDL_UINT64_C(&h0000000000000800)
#define SDL_WINDOW_MODAL SDL_UINT64_C(&h0000000000001000)
#define SDL_WINDOW_HIGH_PIXEL_DENSITY SDL_UINT64_C(&h0000000000002000)
#define SDL_WINDOW_MOUSE_CAPTURE SDL_UINT64_C(&h0000000000004000)
#define SDL_WINDOW_MOUSE_RELATIVE_MODE SDL_UINT64_C(&h0000000000008000)
#define SDL_WINDOW_ALWAYS_ON_TOP SDL_UINT64_C(&h0000000000010000)
#define SDL_WINDOW_UTILITY SDL_UINT64_C(&h0000000000020000)
#define SDL_WINDOW_TOOLTIP SDL_UINT64_C(&h0000000000040000)
#define SDL_WINDOW_POPUP_MENU SDL_UINT64_C(&h0000000000080000)
#define SDL_WINDOW_KEYBOARD_GRABBED SDL_UINT64_C(&h0000000000100000)
#define SDL_WINDOW_FILL_DOCUMENT SDL_UINT64_C(&h0000000000200000)
#define SDL_WINDOW_VULKAN SDL_UINT64_C(&h0000000010000000)
#define SDL_WINDOW_METAL SDL_UINT64_C(&h0000000020000000)
#define SDL_WINDOW_TRANSPARENT SDL_UINT64_C(&h0000000040000000)
#define SDL_WINDOW_NOT_FOCUSABLE SDL_UINT64_C(&h0000000080000000)
const SDL_WINDOWPOS_UNDEFINED_MASK = &h1FFF0000u
#define SDL_WINDOWPOS_UNDEFINED_DISPLAY(X) (SDL_WINDOWPOS_UNDEFINED_MASK or (X))
#define SDL_WINDOWPOS_UNDEFINED SDL_WINDOWPOS_UNDEFINED_DISPLAY(0)
#define SDL_WINDOWPOS_ISUNDEFINED(X) (((X) and &hFFFF0000) = SDL_WINDOWPOS_UNDEFINED_MASK)
const SDL_WINDOWPOS_CENTERED_MASK = &h2FFF0000u
#define SDL_WINDOWPOS_CENTERED_DISPLAY(X) (SDL_WINDOWPOS_CENTERED_MASK or (X))
#define SDL_WINDOWPOS_CENTERED SDL_WINDOWPOS_CENTERED_DISPLAY(0)
#define SDL_WINDOWPOS_ISCENTERED(X) (((X) and &hFFFF0000) = SDL_WINDOWPOS_CENTERED_MASK)

type SDL_FlashOperation as long
enum
	SDL_FLASH_CANCEL
	SDL_FLASH_BRIEFLY
	SDL_FLASH_UNTIL_FOCUSED
end enum

type SDL_ProgressState as long
enum
	SDL_PROGRESS_STATE_INVALID = -1
	SDL_PROGRESS_STATE_NONE
	SDL_PROGRESS_STATE_INDETERMINATE
	SDL_PROGRESS_STATE_NORMAL
	SDL_PROGRESS_STATE_PAUSED
	SDL_PROGRESS_STATE_ERROR
end enum

type SDL_GLContext as SDL_GLContextState ptr
type SDL_EGLDisplay as any ptr
type SDL_EGLConfig as any ptr
type SDL_EGLSurface as any ptr
type SDL_EGLAttrib as integer
type SDL_EGLint as long
type SDL_EGLAttribArrayCallback as function(byval userdata as any ptr) as SDL_EGLAttrib ptr
type SDL_EGLIntArrayCallback as function(byval userdata as any ptr, byval display as SDL_EGLDisplay, byval config as SDL_EGLConfig) as SDL_EGLint ptr

type SDL_GLAttr as long
enum
	SDL_GL_RED_SIZE
	SDL_GL_GREEN_SIZE
	SDL_GL_BLUE_SIZE
	SDL_GL_ALPHA_SIZE
	SDL_GL_BUFFER_SIZE
	SDL_GL_DOUBLEBUFFER
	SDL_GL_DEPTH_SIZE
	SDL_GL_STENCIL_SIZE
	SDL_GL_ACCUM_RED_SIZE
	SDL_GL_ACCUM_GREEN_SIZE
	SDL_GL_ACCUM_BLUE_SIZE
	SDL_GL_ACCUM_ALPHA_SIZE
	SDL_GL_STEREO
	SDL_GL_MULTISAMPLEBUFFERS
	SDL_GL_MULTISAMPLESAMPLES
	SDL_GL_ACCELERATED_VISUAL
	SDL_GL_RETAINED_BACKING
	SDL_GL_CONTEXT_MAJOR_VERSION
	SDL_GL_CONTEXT_MINOR_VERSION
	SDL_GL_CONTEXT_FLAGS
	SDL_GL_CONTEXT_PROFILE_MASK
	SDL_GL_SHARE_WITH_CURRENT_CONTEXT
	SDL_GL_FRAMEBUFFER_SRGB_CAPABLE
	SDL_GL_CONTEXT_RELEASE_BEHAVIOR
	SDL_GL_CONTEXT_RESET_NOTIFICATION
	SDL_GL_CONTEXT_NO_ERROR
	SDL_GL_FLOATBUFFERS
	SDL_GL_EGL_PLATFORM
end enum

type SDL_GLProfile as Uint32
const SDL_GL_CONTEXT_PROFILE_CORE = &h0001
const SDL_GL_CONTEXT_PROFILE_COMPATIBILITY = &h0002
const SDL_GL_CONTEXT_PROFILE_ES = &h0004
type SDL_GLContextFlag as Uint32
const SDL_GL_CONTEXT_DEBUG_FLAG = &h0001
const SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG = &h0002
const SDL_GL_CONTEXT_ROBUST_ACCESS_FLAG = &h0004
const SDL_GL_CONTEXT_RESET_ISOLATION_FLAG = &h0008
type SDL_GLContextReleaseFlag as Uint32
const SDL_GL_CONTEXT_RELEASE_BEHAVIOR_NONE = &h0000
const SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH = &h0001
type SDL_GLContextResetNotification as Uint32
const SDL_GL_CONTEXT_RESET_NO_NOTIFICATION = &h0000
const SDL_GL_CONTEXT_RESET_LOSE_CONTEXT = &h0001

declare function SDL_GetNumVideoDrivers() as long
declare function SDL_GetVideoDriver(byval index as long) as const zstring ptr
declare function SDL_GetCurrentVideoDriver() as const zstring ptr
declare function SDL_GetSystemTheme() as SDL_SystemTheme
declare function SDL_GetDisplays(byval count as long ptr) as SDL_DisplayID ptr
declare function SDL_GetPrimaryDisplay() as SDL_DisplayID
declare function SDL_GetDisplayProperties(byval displayID as SDL_DisplayID) as SDL_PropertiesID

#define SDL_PROP_DISPLAY_HDR_ENABLED_BOOLEAN "SDL.display.HDR_enabled"
#define SDL_PROP_DISPLAY_KMSDRM_PANEL_ORIENTATION_NUMBER "SDL.display.KMSDRM.panel_orientation"
#define SDL_PROP_DISPLAY_WAYLAND_WL_OUTPUT_POINTER "SDL.display.wayland.wl_output"
#define SDL_PROP_DISPLAY_WINDOWS_HMONITOR_POINTER "SDL.display.windows.hmonitor"

declare function SDL_GetDisplayName(byval displayID as SDL_DisplayID) as const zstring ptr
declare function SDL_GetDisplayBounds(byval displayID as SDL_DisplayID, byval rect as SDL_Rect ptr) as boolean
declare function SDL_GetDisplayUsableBounds(byval displayID as SDL_DisplayID, byval rect as SDL_Rect ptr) as boolean
declare function SDL_GetNaturalDisplayOrientation(byval displayID as SDL_DisplayID) as SDL_DisplayOrientation
declare function SDL_GetCurrentDisplayOrientation(byval displayID as SDL_DisplayID) as SDL_DisplayOrientation
declare function SDL_GetDisplayContentScale(byval displayID as SDL_DisplayID) as single
declare function SDL_GetFullscreenDisplayModes(byval displayID as SDL_DisplayID, byval count as long ptr) as SDL_DisplayMode ptr ptr
declare function SDL_GetClosestFullscreenDisplayMode(byval displayID as SDL_DisplayID, byval w as long, byval h as long, byval refresh_rate as single, byval include_high_density_modes as boolean, byval closest as SDL_DisplayMode ptr) as boolean
declare function SDL_GetDesktopDisplayMode(byval displayID as SDL_DisplayID) as const SDL_DisplayMode ptr
declare function SDL_GetCurrentDisplayMode(byval displayID as SDL_DisplayID) as const SDL_DisplayMode ptr
declare function SDL_GetDisplayForPoint(byval point as const SDL_Point ptr) as SDL_DisplayID
declare function SDL_GetDisplayForRect(byval rect as const SDL_Rect ptr) as SDL_DisplayID
declare function SDL_GetDisplayForWindow(byval window as SDL_Window ptr) as SDL_DisplayID
declare function SDL_GetWindowPixelDensity(byval window as SDL_Window ptr) as single
declare function SDL_GetWindowDisplayScale(byval window as SDL_Window ptr) as single
declare function SDL_SetWindowFullscreenMode(byval window as SDL_Window ptr, byval mode as const SDL_DisplayMode ptr) as boolean
declare function SDL_GetWindowFullscreenMode(byval window as SDL_Window ptr) as const SDL_DisplayMode ptr
declare function SDL_GetWindowICCProfile(byval window as SDL_Window ptr, byval size as uinteger ptr) as any ptr
declare function SDL_GetWindowPixelFormat(byval window as SDL_Window ptr) as SDL_PixelFormat
declare function SDL_GetWindows(byval count as long ptr) as SDL_Window ptr ptr
declare function SDL_CreateWindow(byval title as const zstring ptr, byval w as long, byval h as long, byval flags as SDL_WindowFlags) as SDL_Window ptr
declare function SDL_CreatePopupWindow(byval parent as SDL_Window ptr, byval offset_x as long, byval offset_y as long, byval w as long, byval h as long, byval flags as SDL_WindowFlags) as SDL_Window ptr
declare function SDL_CreateWindowWithProperties(byval props as SDL_PropertiesID) as SDL_Window ptr

#define SDL_PROP_WINDOW_CREATE_ALWAYS_ON_TOP_BOOLEAN "SDL.window.create.always_on_top"
#define SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN "SDL.window.create.borderless"
#define SDL_PROP_WINDOW_CREATE_CONSTRAIN_POPUP_BOOLEAN "SDL.window.create.constrain_popup"
#define SDL_PROP_WINDOW_CREATE_FOCUSABLE_BOOLEAN "SDL.window.create.focusable"
#define SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN "SDL.window.create.external_graphics_context"
#define SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER "SDL.window.create.flags"
#define SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN "SDL.window.create.fullscreen"
#define SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER "SDL.window.create.height"
#define SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN "SDL.window.create.hidden"
#define SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN "SDL.window.create.high_pixel_density"
#define SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN "SDL.window.create.maximized"
#define SDL_PROP_WINDOW_CREATE_MENU_BOOLEAN "SDL.window.create.menu"
#define SDL_PROP_WINDOW_CREATE_METAL_BOOLEAN "SDL.window.create.metal"
#define SDL_PROP_WINDOW_CREATE_MINIMIZED_BOOLEAN "SDL.window.create.minimized"
#define SDL_PROP_WINDOW_CREATE_MODAL_BOOLEAN "SDL.window.create.modal"
#define SDL_PROP_WINDOW_CREATE_MOUSE_GRABBED_BOOLEAN "SDL.window.create.mouse_grabbed"
#define SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN "SDL.window.create.opengl"
#define SDL_PROP_WINDOW_CREATE_PARENT_POINTER "SDL.window.create.parent"
#define SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN "SDL.window.create.resizable"
#define SDL_PROP_WINDOW_CREATE_TITLE_STRING "SDL.window.create.title"
#define SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN "SDL.window.create.transparent"
#define SDL_PROP_WINDOW_CREATE_TOOLTIP_BOOLEAN "SDL.window.create.tooltip"
#define SDL_PROP_WINDOW_CREATE_UTILITY_BOOLEAN "SDL.window.create.utility"
#define SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN "SDL.window.create.vulkan"
#define SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER "SDL.window.create.width"
#define SDL_PROP_WINDOW_CREATE_X_NUMBER "SDL.window.create.x"
#define SDL_PROP_WINDOW_CREATE_Y_NUMBER "SDL.window.create.y"
#define SDL_PROP_WINDOW_CREATE_COCOA_WINDOW_POINTER "SDL.window.create.cocoa.window"
#define SDL_PROP_WINDOW_CREATE_COCOA_VIEW_POINTER "SDL.window.create.cocoa.view"
#define SDL_PROP_WINDOW_CREATE_WINDOWSCENE_POINTER "SDL.window.create.uikit.windowscene"
#define SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN "SDL.window.create.wayland.surface_role_custom"
#define SDL_PROP_WINDOW_CREATE_WAYLAND_CREATE_EGL_WINDOW_BOOLEAN "SDL.window.create.wayland.create_egl_window"
#define SDL_PROP_WINDOW_CREATE_WAYLAND_WL_SURFACE_POINTER "SDL.window.create.wayland.wl_surface"
#define SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER "SDL.window.create.win32.hwnd"
#define SDL_PROP_WINDOW_CREATE_WIN32_PIXEL_FORMAT_HWND_POINTER "SDL.window.create.win32.pixel_format_hwnd"
#define SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER "SDL.window.create.x11.window"
#define SDL_PROP_WINDOW_CREATE_EMSCRIPTEN_CANVAS_ID_STRING "SDL.window.create.emscripten.canvas_id"
#define SDL_PROP_WINDOW_CREATE_EMSCRIPTEN_KEYBOARD_ELEMENT_STRING "SDL.window.create.emscripten.keyboard_element"

declare function SDL_GetWindowID(byval window as SDL_Window ptr) as SDL_WindowID
declare function SDL_GetWindowFromID(byval id as SDL_WindowID) as SDL_Window ptr
declare function SDL_GetWindowParent(byval window as SDL_Window ptr) as SDL_Window ptr
declare function SDL_GetWindowProperties(byval window as SDL_Window ptr) as SDL_PropertiesID

#define SDL_PROP_WINDOW_SHAPE_POINTER "SDL.window.shape"
#define SDL_PROP_WINDOW_HDR_ENABLED_BOOLEAN "SDL.window.HDR_enabled"
#define SDL_PROP_WINDOW_SDR_WHITE_LEVEL_FLOAT "SDL.window.SDR_white_level"
#define SDL_PROP_WINDOW_HDR_HEADROOM_FLOAT "SDL.window.HDR_headroom"
#define SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER "SDL.window.android.window"
#define SDL_PROP_WINDOW_ANDROID_SURFACE_POINTER "SDL.window.android.surface"
#define SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER "SDL.window.uikit.window"
#define SDL_PROP_WINDOW_UIKIT_METAL_VIEW_TAG_NUMBER "SDL.window.uikit.metal_view_tag"
#define SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER "SDL.window.uikit.opengl.framebuffer"
#define SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER "SDL.window.uikit.opengl.renderbuffer"
#define SDL_PROP_WINDOW_UIKIT_OPENGL_RESOLVE_FRAMEBUFFER_NUMBER "SDL.window.uikit.opengl.resolve_framebuffer"
#define SDL_PROP_WINDOW_KMSDRM_DEVICE_INDEX_NUMBER "SDL.window.kmsdrm.dev_index"
#define SDL_PROP_WINDOW_KMSDRM_DRM_FD_NUMBER "SDL.window.kmsdrm.drm_fd"
#define SDL_PROP_WINDOW_KMSDRM_GBM_DEVICE_POINTER "SDL.window.kmsdrm.gbm_dev"
#define SDL_PROP_WINDOW_COCOA_WINDOW_POINTER "SDL.window.cocoa.window"
#define SDL_PROP_WINDOW_COCOA_METAL_VIEW_TAG_NUMBER "SDL.window.cocoa.metal_view_tag"
#define SDL_PROP_WINDOW_OPENVR_OVERLAY_ID_NUMBER "SDL.window.openvr.overlay_id"
#define SDL_PROP_WINDOW_VIVANTE_DISPLAY_POINTER "SDL.window.vivante.display"
#define SDL_PROP_WINDOW_VIVANTE_WINDOW_POINTER "SDL.window.vivante.window"
#define SDL_PROP_WINDOW_VIVANTE_SURFACE_POINTER "SDL.window.vivante.surface"
#define SDL_PROP_WINDOW_WIN32_HWND_POINTER "SDL.window.win32.hwnd"
#define SDL_PROP_WINDOW_WIN32_HDC_POINTER "SDL.window.win32.hdc"
#define SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER "SDL.window.win32.instance"
#define SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER "SDL.window.wayland.display"
#define SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER "SDL.window.wayland.surface"
#define SDL_PROP_WINDOW_WAYLAND_VIEWPORT_POINTER "SDL.window.wayland.viewport"
#define SDL_PROP_WINDOW_WAYLAND_EGL_WINDOW_POINTER "SDL.window.wayland.egl_window"
#define SDL_PROP_WINDOW_WAYLAND_XDG_SURFACE_POINTER "SDL.window.wayland.xdg_surface"
#define SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_POINTER "SDL.window.wayland.xdg_toplevel"
#define SDL_PROP_WINDOW_WAYLAND_XDG_TOPLEVEL_EXPORT_HANDLE_STRING "SDL.window.wayland.xdg_toplevel_export_handle"
#define SDL_PROP_WINDOW_WAYLAND_XDG_POPUP_POINTER "SDL.window.wayland.xdg_popup"
#define SDL_PROP_WINDOW_WAYLAND_XDG_POSITIONER_POINTER "SDL.window.wayland.xdg_positioner"
#define SDL_PROP_WINDOW_X11_DISPLAY_POINTER "SDL.window.x11.display"
#define SDL_PROP_WINDOW_X11_SCREEN_NUMBER "SDL.window.x11.screen"
#define SDL_PROP_WINDOW_X11_WINDOW_NUMBER "SDL.window.x11.window"
#define SDL_PROP_WINDOW_EMSCRIPTEN_CANVAS_ID_STRING "SDL.window.emscripten.canvas_id"
#define SDL_PROP_WINDOW_EMSCRIPTEN_KEYBOARD_ELEMENT_STRING "SDL.window.emscripten.keyboard_element"

declare function SDL_GetWindowFlags(byval window as SDL_Window ptr) as SDL_WindowFlags
declare function SDL_SetWindowTitle(byval window as SDL_Window ptr, byval title as const zstring ptr) as boolean
declare function SDL_GetWindowTitle(byval window as SDL_Window ptr) as const zstring ptr
declare function SDL_SetWindowIcon(byval window as SDL_Window ptr, byval icon as SDL_Surface ptr) as boolean
declare function SDL_SetWindowPosition(byval window as SDL_Window ptr, byval x as long, byval y as long) as boolean
declare function SDL_GetWindowPosition(byval window as SDL_Window ptr, byval x as long ptr, byval y as long ptr) as boolean
declare function SDL_SetWindowSize(byval window as SDL_Window ptr, byval w as long, byval h as long) as boolean
declare function SDL_GetWindowSize(byval window as SDL_Window ptr, byval w as long ptr, byval h as long ptr) as boolean
declare function SDL_GetWindowSafeArea(byval window as SDL_Window ptr, byval rect as SDL_Rect ptr) as boolean
declare function SDL_SetWindowAspectRatio(byval window as SDL_Window ptr, byval min_aspect as single, byval max_aspect as single) as boolean
declare function SDL_GetWindowAspectRatio(byval window as SDL_Window ptr, byval min_aspect as single ptr, byval max_aspect as single ptr) as boolean
declare function SDL_GetWindowBordersSize(byval window as SDL_Window ptr, byval top as long ptr, byval left as long ptr, byval bottom as long ptr, byval right as long ptr) as boolean
declare function SDL_GetWindowSizeInPixels(byval window as SDL_Window ptr, byval w as long ptr, byval h as long ptr) as boolean
declare function SDL_SetWindowMinimumSize(byval window as SDL_Window ptr, byval min_w as long, byval min_h as long) as boolean
declare function SDL_GetWindowMinimumSize(byval window as SDL_Window ptr, byval w as long ptr, byval h as long ptr) as boolean
declare function SDL_SetWindowMaximumSize(byval window as SDL_Window ptr, byval max_w as long, byval max_h as long) as boolean
declare function SDL_GetWindowMaximumSize(byval window as SDL_Window ptr, byval w as long ptr, byval h as long ptr) as boolean
declare function SDL_SetWindowBordered(byval window as SDL_Window ptr, byval bordered as boolean) as boolean
declare function SDL_SetWindowResizable(byval window as SDL_Window ptr, byval resizable as boolean) as boolean
declare function SDL_SetWindowAlwaysOnTop(byval window as SDL_Window ptr, byval on_top as boolean) as boolean
declare function SDL_SetWindowFillDocument(byval window as SDL_Window ptr, byval fill as boolean) as boolean
declare function SDL_ShowWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_HideWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_RaiseWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_MaximizeWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_MinimizeWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_RestoreWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_SetWindowFullscreen(byval window as SDL_Window ptr, byval fullscreen as boolean) as boolean
declare function SDL_SyncWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_WindowHasSurface(byval window as SDL_Window ptr) as boolean
declare function SDL_GetWindowSurface(byval window as SDL_Window ptr) as SDL_Surface ptr
declare function SDL_SetWindowSurfaceVSync(byval window as SDL_Window ptr, byval vsync as long) as boolean
const SDL_WINDOW_SURFACE_VSYNC_DISABLED = 0
const SDL_WINDOW_SURFACE_VSYNC_ADAPTIVE = -1
declare function SDL_GetWindowSurfaceVSync(byval window as SDL_Window ptr, byval vsync as long ptr) as boolean
declare function SDL_UpdateWindowSurface(byval window as SDL_Window ptr) as boolean
declare function SDL_UpdateWindowSurfaceRects(byval window as SDL_Window ptr, byval rects as const SDL_Rect ptr, byval numrects as long) as boolean
declare function SDL_DestroyWindowSurface(byval window as SDL_Window ptr) as boolean
declare function SDL_SetWindowKeyboardGrab(byval window as SDL_Window ptr, byval grabbed as boolean) as boolean
declare function SDL_SetWindowMouseGrab(byval window as SDL_Window ptr, byval grabbed as boolean) as boolean
declare function SDL_GetWindowKeyboardGrab(byval window as SDL_Window ptr) as boolean
declare function SDL_GetWindowMouseGrab(byval window as SDL_Window ptr) as boolean
declare function SDL_GetGrabbedWindow() as SDL_Window ptr
declare function SDL_SetWindowMouseRect(byval window as SDL_Window ptr, byval rect as const SDL_Rect ptr) as boolean
declare function SDL_GetWindowMouseRect(byval window as SDL_Window ptr) as const SDL_Rect ptr
declare function SDL_SetWindowOpacity(byval window as SDL_Window ptr, byval opacity as single) as boolean
declare function SDL_GetWindowOpacity(byval window as SDL_Window ptr) as single
declare function SDL_SetWindowParent(byval window as SDL_Window ptr, byval parent as SDL_Window ptr) as boolean
declare function SDL_SetWindowModal(byval window as SDL_Window ptr, byval modal as boolean) as boolean
declare function SDL_SetWindowFocusable(byval window as SDL_Window ptr, byval focusable as boolean) as boolean
declare function SDL_ShowWindowSystemMenu(byval window as SDL_Window ptr, byval x as long, byval y as long) as boolean

type SDL_HitTestResult as long
enum
	SDL_HITTEST_NORMAL
	SDL_HITTEST_DRAGGABLE
	SDL_HITTEST_RESIZE_TOPLEFT
	SDL_HITTEST_RESIZE_TOP
	SDL_HITTEST_RESIZE_TOPRIGHT
	SDL_HITTEST_RESIZE_RIGHT
	SDL_HITTEST_RESIZE_BOTTOMRIGHT
	SDL_HITTEST_RESIZE_BOTTOM
	SDL_HITTEST_RESIZE_BOTTOMLEFT
	SDL_HITTEST_RESIZE_LEFT
end enum

type SDL_HitTest as function(byval win as SDL_Window ptr, byval area as const SDL_Point ptr, byval data as any ptr) as SDL_HitTestResult
declare function SDL_SetWindowHitTest(byval window as SDL_Window ptr, byval callback as SDL_HitTest, byval callback_data as any ptr) as boolean
declare function SDL_SetWindowShape(byval window as SDL_Window ptr, byval shape as SDL_Surface ptr) as boolean
declare function SDL_FlashWindow(byval window as SDL_Window ptr, byval operation as SDL_FlashOperation) as boolean
declare function SDL_SetWindowProgressState(byval window as SDL_Window ptr, byval state as SDL_ProgressState) as boolean
declare function SDL_GetWindowProgressState(byval window as SDL_Window ptr) as SDL_ProgressState
declare function SDL_SetWindowProgressValue(byval window as SDL_Window ptr, byval value as single) as boolean
declare function SDL_GetWindowProgressValue(byval window as SDL_Window ptr) as single
declare sub SDL_DestroyWindow(byval window as SDL_Window ptr)
declare function SDL_ScreenSaverEnabled() as boolean
declare function SDL_EnableScreenSaver() as boolean
declare function SDL_DisableScreenSaver() as boolean
declare function SDL_GL_LoadLibrary(byval path as const zstring ptr) as boolean
declare function SDL_GL_GetProcAddress(byval proc as const zstring ptr) as SDL_FunctionPointer
declare function SDL_EGL_GetProcAddress(byval proc as const zstring ptr) as SDL_FunctionPointer
declare sub SDL_GL_UnloadLibrary()
declare function SDL_GL_ExtensionSupported(byval extension as const zstring ptr) as boolean
declare sub SDL_GL_ResetAttributes()
declare function SDL_GL_SetAttribute(byval attr as SDL_GLAttr, byval value as long) as boolean
declare function SDL_GL_GetAttribute(byval attr as SDL_GLAttr, byval value as long ptr) as boolean
declare function SDL_GL_CreateContext(byval window as SDL_Window ptr) as SDL_GLContext
declare function SDL_GL_MakeCurrent(byval window as SDL_Window ptr, byval context as SDL_GLContext) as boolean
declare function SDL_GL_GetCurrentWindow() as SDL_Window ptr
declare function SDL_GL_GetCurrentContext() as SDL_GLContext
declare function SDL_EGL_GetCurrentDisplay() as SDL_EGLDisplay
declare function SDL_EGL_GetCurrentConfig() as SDL_EGLConfig
declare function SDL_EGL_GetWindowSurface(byval window as SDL_Window ptr) as SDL_EGLSurface
declare sub SDL_EGL_SetAttributeCallbacks(byval platformAttribCallback as SDL_EGLAttribArrayCallback, byval surfaceAttribCallback as SDL_EGLIntArrayCallback, byval contextAttribCallback as SDL_EGLIntArrayCallback, byval userdata as any ptr)
declare function SDL_GL_SetSwapInterval(byval interval as long) as boolean
declare function SDL_GL_GetSwapInterval(byval interval as long ptr) as boolean
declare function SDL_GL_SwapWindow(byval window as SDL_Window ptr) as boolean
declare function SDL_GL_DestroyContext(byval context as SDL_GLContext) as boolean

type SDL_DialogFileFilter
	name as const zstring ptr
	pattern as const zstring ptr
end type

type SDL_DialogFileCallback as sub(byval userdata as any ptr, byval filelist as const zstring const ptr ptr, byval filter as long)
declare sub SDL_ShowOpenFileDialog(byval callback as SDL_DialogFileCallback, byval userdata as any ptr, byval window as SDL_Window ptr, byval filters as const SDL_DialogFileFilter ptr, byval nfilters as long, byval default_location as const zstring ptr, byval allow_many as boolean)
declare sub SDL_ShowSaveFileDialog(byval callback as SDL_DialogFileCallback, byval userdata as any ptr, byval window as SDL_Window ptr, byval filters as const SDL_DialogFileFilter ptr, byval nfilters as long, byval default_location as const zstring ptr)
declare sub SDL_ShowOpenFolderDialog(byval callback as SDL_DialogFileCallback, byval userdata as any ptr, byval window as SDL_Window ptr, byval default_location as const zstring ptr, byval allow_many as boolean)

type SDL_FileDialogType as long
enum
	SDL_FILEDIALOG_OPENFILE
	SDL_FILEDIALOG_SAVEFILE
	SDL_FILEDIALOG_OPENFOLDER
end enum

declare sub SDL_ShowFileDialogWithProperties(byval type as SDL_FileDialogType, byval callback as SDL_DialogFileCallback, byval userdata as any ptr, byval props as SDL_PropertiesID)
#define SDL_PROP_FILE_DIALOG_FILTERS_POINTER "SDL.filedialog.filters"
#define SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER "SDL.filedialog.nfilters"
#define SDL_PROP_FILE_DIALOG_WINDOW_POINTER "SDL.filedialog.window"
#define SDL_PROP_FILE_DIALOG_LOCATION_STRING "SDL.filedialog.location"
#define SDL_PROP_FILE_DIALOG_MANY_BOOLEAN "SDL.filedialog.many"
#define SDL_PROP_FILE_DIALOG_TITLE_STRING "SDL.filedialog.title"
#define SDL_PROP_FILE_DIALOG_ACCEPT_STRING "SDL.filedialog.accept"
#define SDL_PROP_FILE_DIALOG_CANCEL_STRING "SDL.filedialog.cancel"
#define SDL_dlopennote_h
#define SDL_ELF_NOTE_DLOPEN_PRIORITY_SUGGESTED "suggested"
#define SDL_ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED "recommended"
#define SDL_ELF_NOTE_DLOPEN_PRIORITY_REQUIRED "required"

#if defined(__FB_LINUX__) or defined(__FB_FREEBSD__) or defined(__FB_OPENBSD__) or defined(__FB_NETBSD__)
	#define SDL_ELF_NOTE_DLOPEN_VENDOR "FDO"
	const SDL_ELF_NOTE_DLOPEN_TYPE = &h407c0c0au
	#define SDL_ELF_NOTE_INTERNAL(json, variable_name) SDL_ELF_NOTE_INTERNAL2(json, variable_name)
	#define SDL_DLNOTE_JSON_ARRAY1(N1) "[""" N1 """]"
	#define SDL_DLNOTE_JSON_ARRAY2(N1, N2) "[""" N1 """,""" N2 """]"
	#define SDL_DLNOTE_JSON_ARRAY3(N1, N2, N3) "[""" N1 """,""" N2 """,""" N3 """]"
	#define SDL_DLNOTE_JSON_ARRAY4(N1, N2, N3, N4) "[""" N1 """,""" N2 """,""" N3 """,""" N4 """]"
	#define SDL_DLNOTE_JSON_ARRAY5(N1, N2, N3, N4, N5) "[""" N1 """,""" N2 """,""" N3 """,""" N4 """,""" N5 """]"
	#define SDL_DLNOTE_JSON_ARRAY6(N1, N2, N3, N4, N5, N6) "[""" N1 """,""" N2 """,""" N3 """,""" N4 """,""" N5 """,""" N6 """]"
	#define SDL_DLNOTE_JSON_ARRAY7(N1, N2, N3, N4, N5, N6, N7) "[""" N1 """,""" N2 """,""" N3 """,""" N4 """,""" N5 """,""" N6 """,""" N7 """]"
	#define SDL_DLNOTE_JSON_ARRAY8(N1, N2, N3, N4, N5, N6, N7, N8) "[""" N1 """,""" N2 """,""" N3 """,""" N4 """,""" N5 """,""" N6 """,""" N7 """,""" N8 """]"
	#define SDL_DLNOTE_JSON_ARRAY_GET(N1, N2, N3, N4, N5, N6, N7, N8, NAME, __VA_ARGS__...) NAME
	#define SDL_DLNOTE_JOIN2(A, B) A##B
	#define SDL_DLNOTE_JOIN(A, B) SDL_DLNOTE_JOIN2(A, B)
	#define SDL_DLNOTE_UNIQUE_NAME SDL_DLNOTE_JOIN(s_SDL_dlopen_note_, __LINE__)
#elseif defined(__FB_WIN32__)
	#define SDL_DISABLE_DLOPEN_NOTES
#endif

#if defined(__FB_DARWIN__) or defined(__FB_WIN32__)
	#define SDL_ELF_NOTE_DLOPEN(__VA_ARGS__...)
#endif

'' -------------------------------------------------------------------------
'' SDL_events.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_gamepad.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_guid.h
'' -------------------------------------------------------------------------
type SDL_GUID
	data(0 to 15) as Uint8
end type

declare sub SDL_GUIDToString(byval guid as SDL_GUID, byval pszGUID as zstring ptr, byval cbGUID as long)
declare function SDL_StringToGUID(byval pchGUID as const zstring ptr) as SDL_GUID
'' -------------------------------------------------------------------------
'' SDL_joystick.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_power.h
'' -------------------------------------------------------------------------
type SDL_PowerState as long
enum
	SDL_POWERSTATE_ERROR = -1
	SDL_POWERSTATE_UNKNOWN
	SDL_POWERSTATE_ON_BATTERY
	SDL_POWERSTATE_NO_BATTERY
	SDL_POWERSTATE_CHARGING
	SDL_POWERSTATE_CHARGED
end enum

declare function SDL_GetPowerInfo(byval seconds as long ptr, byval percent as long ptr) as SDL_PowerState
'' -------------------------------------------------------------------------
'' SDL_sensor.h
'' -------------------------------------------------------------------------
type SDL_SensorID as Uint32
const SDL_STANDARD_GRAVITY = 9.80665f

type SDL_SensorType as long
enum
	SDL_SENSOR_INVALID = -1
	SDL_SENSOR_UNKNOWN
	SDL_SENSOR_ACCEL
	SDL_SENSOR_GYRO
	SDL_SENSOR_ACCEL_L
	SDL_SENSOR_GYRO_L
	SDL_SENSOR_ACCEL_R
	SDL_SENSOR_GYRO_R
	SDL_SENSOR_COUNT
end enum

declare function SDL_GetSensors(byval count as long ptr) as SDL_SensorID ptr
declare function SDL_GetSensorNameForID(byval instance_id as SDL_SensorID) as const zstring ptr
declare function SDL_GetSensorTypeForID(byval instance_id as SDL_SensorID) as SDL_SensorType
declare function SDL_GetSensorNonPortableTypeForID(byval instance_id as SDL_SensorID) as long
declare function SDL_OpenSensor(byval instance_id as SDL_SensorID) as SDL_Sensor ptr
declare function SDL_GetSensorFromID(byval instance_id as SDL_SensorID) as SDL_Sensor ptr
declare function SDL_GetSensorProperties(byval sensor as SDL_Sensor ptr) as SDL_PropertiesID
declare function SDL_GetSensorName(byval sensor as SDL_Sensor ptr) as const zstring ptr
declare function SDL_GetSensorType(byval sensor as SDL_Sensor ptr) as SDL_SensorType
declare function SDL_GetSensorNonPortableType(byval sensor as SDL_Sensor ptr) as long
declare function SDL_GetSensorID(byval sensor as SDL_Sensor ptr) as SDL_SensorID
declare function SDL_GetSensorData(byval sensor as SDL_Sensor ptr, byval data as single ptr, byval num_values as long) as boolean
declare sub SDL_CloseSensor(byval sensor as SDL_Sensor ptr)
declare sub SDL_UpdateSensors()
type SDL_JoystickID as Uint32

type SDL_JoystickType as long
enum
	SDL_JOYSTICK_TYPE_UNKNOWN
	SDL_JOYSTICK_TYPE_GAMEPAD
	SDL_JOYSTICK_TYPE_WHEEL
	SDL_JOYSTICK_TYPE_ARCADE_STICK
	SDL_JOYSTICK_TYPE_FLIGHT_STICK
	SDL_JOYSTICK_TYPE_DANCE_PAD
	SDL_JOYSTICK_TYPE_GUITAR
	SDL_JOYSTICK_TYPE_DRUM_KIT
	SDL_JOYSTICK_TYPE_ARCADE_PAD
	SDL_JOYSTICK_TYPE_THROTTLE
	SDL_JOYSTICK_TYPE_COUNT
end enum

type SDL_JoystickConnectionState as long
enum
	SDL_JOYSTICK_CONNECTION_INVALID = -1
	SDL_JOYSTICK_CONNECTION_UNKNOWN
	SDL_JOYSTICK_CONNECTION_WIRED
	SDL_JOYSTICK_CONNECTION_WIRELESS
end enum

const SDL_JOYSTICK_AXIS_MAX = 32767
const SDL_JOYSTICK_AXIS_MIN = -32768
declare sub SDL_LockJoysticks()
declare sub SDL_UnlockJoysticks()
declare function SDL_HasJoystick() as boolean
declare function SDL_GetJoysticks(byval count as long ptr) as SDL_JoystickID ptr
declare function SDL_GetJoystickNameForID(byval instance_id as SDL_JoystickID) as const zstring ptr
declare function SDL_GetJoystickPathForID(byval instance_id as SDL_JoystickID) as const zstring ptr
declare function SDL_GetJoystickPlayerIndexForID(byval instance_id as SDL_JoystickID) as long
declare function SDL_GetJoystickGUIDForID(byval instance_id as SDL_JoystickID) as SDL_GUID
declare function SDL_GetJoystickVendorForID(byval instance_id as SDL_JoystickID) as Uint16
declare function SDL_GetJoystickProductForID(byval instance_id as SDL_JoystickID) as Uint16
declare function SDL_GetJoystickProductVersionForID(byval instance_id as SDL_JoystickID) as Uint16
declare function SDL_GetJoystickTypeForID(byval instance_id as SDL_JoystickID) as SDL_JoystickType
declare function SDL_OpenJoystick(byval instance_id as SDL_JoystickID) as SDL_Joystick ptr
declare function SDL_GetJoystickFromID(byval instance_id as SDL_JoystickID) as SDL_Joystick ptr
declare function SDL_GetJoystickFromPlayerIndex(byval player_index as long) as SDL_Joystick ptr

type SDL_VirtualJoystickTouchpadDesc
	nfingers as Uint16
	padding(0 to 2) as Uint16
end type

type SDL_VirtualJoystickSensorDesc
	as SDL_SensorType type
	rate as single
end type

type SDL_VirtualJoystickDesc
	version as Uint32
	as Uint16 type
	padding as Uint16
	vendor_id as Uint16
	product_id as Uint16
	naxes as Uint16
	nbuttons as Uint16
	nballs as Uint16
	nhats as Uint16
	ntouchpads as Uint16
	nsensors as Uint16
	padding2(0 to 1) as Uint16
	button_mask as Uint32
	axis_mask as Uint32
	name as const zstring ptr
	touchpads as const SDL_VirtualJoystickTouchpadDesc ptr
	sensors as const SDL_VirtualJoystickSensorDesc ptr
	userdata as any ptr
	Update as sub(byval userdata as any ptr)
	SetPlayerIndex as sub(byval userdata as any ptr, byval player_index as long)
	Rumble as function(byval userdata as any ptr, byval low_frequency_rumble as Uint16, byval high_frequency_rumble as Uint16) as boolean
	RumbleTriggers as function(byval userdata as any ptr, byval left_rumble as Uint16, byval right_rumble as Uint16) as boolean
	SetLED as function(byval userdata as any ptr, byval red as Uint8, byval green as Uint8, byval blue as Uint8) as boolean
	SendEffect as function(byval userdata as any ptr, byval data as const any ptr, byval size as long) as boolean
	SetSensorsEnabled as function(byval userdata as any ptr, byval enabled as boolean) as boolean
	Cleanup as sub(byval userdata as any ptr)
end type

declare function SDL_AttachVirtualJoystick(byval desc as const SDL_VirtualJoystickDesc ptr) as SDL_JoystickID
declare function SDL_DetachVirtualJoystick(byval instance_id as SDL_JoystickID) as boolean
declare function SDL_IsJoystickVirtual(byval instance_id as SDL_JoystickID) as boolean
declare function SDL_SetJoystickVirtualAxis(byval joystick as SDL_Joystick ptr, byval axis as long, byval value as Sint16) as boolean
declare function SDL_SetJoystickVirtualBall(byval joystick as SDL_Joystick ptr, byval ball as long, byval xrel as Sint16, byval yrel as Sint16) as boolean
declare function SDL_SetJoystickVirtualButton(byval joystick as SDL_Joystick ptr, byval button as long, byval down as boolean) as boolean
declare function SDL_SetJoystickVirtualHat(byval joystick as SDL_Joystick ptr, byval hat as long, byval value as Uint8) as boolean
declare function SDL_SetJoystickVirtualTouchpad(byval joystick as SDL_Joystick ptr, byval touchpad as long, byval finger as long, byval down as boolean, byval x as single, byval y as single, byval pressure as single) as boolean
declare function SDL_SendJoystickVirtualSensorData(byval joystick as SDL_Joystick ptr, byval type as SDL_SensorType, byval sensor_timestamp as Uint64, byval data as const single ptr, byval num_values as long) as boolean
declare function SDL_GetJoystickProperties(byval joystick as SDL_Joystick ptr) as SDL_PropertiesID

#define SDL_PROP_JOYSTICK_CAP_MONO_LED_BOOLEAN "SDL.joystick.cap.mono_led"
#define SDL_PROP_JOYSTICK_CAP_RGB_LED_BOOLEAN "SDL.joystick.cap.rgb_led"
#define SDL_PROP_JOYSTICK_CAP_PLAYER_LED_BOOLEAN "SDL.joystick.cap.player_led"
#define SDL_PROP_JOYSTICK_CAP_RUMBLE_BOOLEAN "SDL.joystick.cap.rumble"
#define SDL_PROP_JOYSTICK_CAP_TRIGGER_RUMBLE_BOOLEAN "SDL.joystick.cap.trigger_rumble"

declare function SDL_GetJoystickName(byval joystick as SDL_Joystick ptr) as const zstring ptr
declare function SDL_GetJoystickPath(byval joystick as SDL_Joystick ptr) as const zstring ptr
declare function SDL_GetJoystickPlayerIndex(byval joystick as SDL_Joystick ptr) as long
declare function SDL_SetJoystickPlayerIndex(byval joystick as SDL_Joystick ptr, byval player_index as long) as boolean
declare function SDL_GetJoystickGUID(byval joystick as SDL_Joystick ptr) as SDL_GUID
declare function SDL_GetJoystickVendor(byval joystick as SDL_Joystick ptr) as Uint16
declare function SDL_GetJoystickProduct(byval joystick as SDL_Joystick ptr) as Uint16
declare function SDL_GetJoystickProductVersion(byval joystick as SDL_Joystick ptr) as Uint16
declare function SDL_GetJoystickFirmwareVersion(byval joystick as SDL_Joystick ptr) as Uint16
declare function SDL_GetJoystickSerial(byval joystick as SDL_Joystick ptr) as const zstring ptr
declare function SDL_GetJoystickType(byval joystick as SDL_Joystick ptr) as SDL_JoystickType
declare sub SDL_GetJoystickGUIDInfo(byval guid as SDL_GUID, byval vendor as Uint16 ptr, byval product as Uint16 ptr, byval version as Uint16 ptr, byval crc16 as Uint16 ptr)
declare function SDL_JoystickConnected(byval joystick as SDL_Joystick ptr) as boolean
declare function SDL_GetJoystickID(byval joystick as SDL_Joystick ptr) as SDL_JoystickID
declare function SDL_GetNumJoystickAxes(byval joystick as SDL_Joystick ptr) as long
declare function SDL_GetNumJoystickBalls(byval joystick as SDL_Joystick ptr) as long
declare function SDL_GetNumJoystickHats(byval joystick as SDL_Joystick ptr) as long
declare function SDL_GetNumJoystickButtons(byval joystick as SDL_Joystick ptr) as long
declare sub SDL_SetJoystickEventsEnabled(byval enabled as boolean)
declare function SDL_JoystickEventsEnabled() as boolean
declare sub SDL_UpdateJoysticks()
declare function SDL_GetJoystickAxis(byval joystick as SDL_Joystick ptr, byval axis as long) as Sint16
declare function SDL_GetJoystickAxisInitialState(byval joystick as SDL_Joystick ptr, byval axis as long, byval state as Sint16 ptr) as boolean
declare function SDL_GetJoystickBall(byval joystick as SDL_Joystick ptr, byval ball as long, byval dx as long ptr, byval dy as long ptr) as boolean
declare function SDL_GetJoystickHat(byval joystick as SDL_Joystick ptr, byval hat as long) as Uint8

const SDL_HAT_CENTERED = &h00u
const SDL_HAT_UP = &h01u
const SDL_HAT_RIGHT = &h02u
const SDL_HAT_DOWN = &h04u
const SDL_HAT_LEFT = &h08u
const SDL_HAT_RIGHTUP = culng(SDL_HAT_RIGHT or SDL_HAT_UP)
const SDL_HAT_RIGHTDOWN = culng(SDL_HAT_RIGHT or SDL_HAT_DOWN)
const SDL_HAT_LEFTUP = culng(SDL_HAT_LEFT or SDL_HAT_UP)
const SDL_HAT_LEFTDOWN = culng(SDL_HAT_LEFT or SDL_HAT_DOWN)

declare function SDL_GetJoystickButton(byval joystick as SDL_Joystick ptr, byval button as long) as boolean
declare function SDL_RumbleJoystick(byval joystick as SDL_Joystick ptr, byval low_frequency_rumble as Uint16, byval high_frequency_rumble as Uint16, byval duration_ms as Uint32) as boolean
declare function SDL_RumbleJoystickTriggers(byval joystick as SDL_Joystick ptr, byval left_rumble as Uint16, byval right_rumble as Uint16, byval duration_ms as Uint32) as boolean
declare function SDL_SetJoystickLED(byval joystick as SDL_Joystick ptr, byval red as Uint8, byval green as Uint8, byval blue as Uint8) as boolean
declare function SDL_SendJoystickEffect(byval joystick as SDL_Joystick ptr, byval data as const any ptr, byval size as long) as boolean
declare sub SDL_CloseJoystick(byval joystick as SDL_Joystick ptr)
declare function SDL_GetJoystickConnectionState(byval joystick as SDL_Joystick ptr) as SDL_JoystickConnectionState
declare function SDL_GetJoystickPowerInfo(byval joystick as SDL_Joystick ptr, byval percent as long ptr) as SDL_PowerState

type SDL_GamepadType as long
enum
	SDL_GAMEPAD_TYPE_UNKNOWN = 0
	SDL_GAMEPAD_TYPE_STANDARD
	SDL_GAMEPAD_TYPE_XBOX360
	SDL_GAMEPAD_TYPE_XBOXONE
	SDL_GAMEPAD_TYPE_PS3
	SDL_GAMEPAD_TYPE_PS4
	SDL_GAMEPAD_TYPE_PS5
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT
	SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR
	SDL_GAMEPAD_TYPE_GAMECUBE
	SDL_GAMEPAD_TYPE_STEAM
	SDL_GAMEPAD_TYPE_COUNT
end enum

type SDL_GamepadButton as long
enum
	SDL_GAMEPAD_BUTTON_INVALID = -1
	SDL_GAMEPAD_BUTTON_SOUTH
	SDL_GAMEPAD_BUTTON_EAST
	SDL_GAMEPAD_BUTTON_WEST
	SDL_GAMEPAD_BUTTON_NORTH
	SDL_GAMEPAD_BUTTON_BACK
	SDL_GAMEPAD_BUTTON_GUIDE
	SDL_GAMEPAD_BUTTON_START
	SDL_GAMEPAD_BUTTON_LEFT_STICK
	SDL_GAMEPAD_BUTTON_RIGHT_STICK
	SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
	SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
	SDL_GAMEPAD_BUTTON_DPAD_UP
	SDL_GAMEPAD_BUTTON_DPAD_DOWN
	SDL_GAMEPAD_BUTTON_DPAD_LEFT
	SDL_GAMEPAD_BUTTON_DPAD_RIGHT
	SDL_GAMEPAD_BUTTON_MISC1
	SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1
	SDL_GAMEPAD_BUTTON_LEFT_PADDLE1
	SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2
	SDL_GAMEPAD_BUTTON_LEFT_PADDLE2
	SDL_GAMEPAD_BUTTON_TOUCHPAD
	SDL_GAMEPAD_BUTTON_MISC2
	SDL_GAMEPAD_BUTTON_MISC3
	SDL_GAMEPAD_BUTTON_MISC4
	SDL_GAMEPAD_BUTTON_MISC5
	SDL_GAMEPAD_BUTTON_MISC6
	SDL_GAMEPAD_BUTTON_COUNT
end enum

type SDL_GamepadButtonLabel as long
enum
	SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN
	SDL_GAMEPAD_BUTTON_LABEL_A
	SDL_GAMEPAD_BUTTON_LABEL_B
	SDL_GAMEPAD_BUTTON_LABEL_X
	SDL_GAMEPAD_BUTTON_LABEL_Y
	SDL_GAMEPAD_BUTTON_LABEL_CROSS
	SDL_GAMEPAD_BUTTON_LABEL_CIRCLE
	SDL_GAMEPAD_BUTTON_LABEL_SQUARE
	SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE
end enum

type SDL_GamepadAxis as long
enum
	SDL_GAMEPAD_AXIS_INVALID = -1
	SDL_GAMEPAD_AXIS_LEFTX
	SDL_GAMEPAD_AXIS_LEFTY
	SDL_GAMEPAD_AXIS_RIGHTX
	SDL_GAMEPAD_AXIS_RIGHTY
	SDL_GAMEPAD_AXIS_LEFT_TRIGGER
	SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
	SDL_GAMEPAD_AXIS_COUNT
end enum

type SDL_GamepadBindingType as long
enum
	SDL_GAMEPAD_BINDTYPE_NONE = 0
	SDL_GAMEPAD_BINDTYPE_BUTTON
	SDL_GAMEPAD_BINDTYPE_AXIS
	SDL_GAMEPAD_BINDTYPE_HAT
end enum

type SDL_GamepadBinding_input_axis
	axis as long
	axis_min as long
	axis_max as long
end type

type SDL_GamepadBinding_input_hat
	hat as long
	hat_mask as long
end type

union SDL_GamepadBinding_input
	button as long
	axis as SDL_GamepadBinding_input_axis
	hat as SDL_GamepadBinding_input_hat
end union

type SDL_GamepadBinding_output_axis
	axis as SDL_GamepadAxis
	axis_min as long
	axis_max as long
end type

union SDL_GamepadBinding_output
	button as SDL_GamepadButton
	axis as SDL_GamepadBinding_output_axis
end union

type SDL_GamepadBinding
	input_type as SDL_GamepadBindingType
	input as SDL_GamepadBinding_input
	output_type as SDL_GamepadBindingType
	output as SDL_GamepadBinding_output
end type

declare function SDL_AddGamepadMapping(byval mapping as const zstring ptr) as long
declare function SDL_AddGamepadMappingsFromIO(byval src as SDL_IOStream ptr, byval closeio as boolean) as long
declare function SDL_AddGamepadMappingsFromFile(byval file as const zstring ptr) as long
declare function SDL_ReloadGamepadMappings() as boolean
declare function SDL_GetGamepadMappings(byval count as long ptr) as zstring ptr ptr
declare function SDL_GetGamepadMappingForGUID(byval guid as SDL_GUID) as zstring ptr
declare function SDL_GetGamepadMapping(byval gamepad as SDL_Gamepad ptr) as zstring ptr
declare function SDL_SetGamepadMapping(byval instance_id as SDL_JoystickID, byval mapping as const zstring ptr) as boolean
declare function SDL_HasGamepad() as boolean
declare function SDL_GetGamepads(byval count as long ptr) as SDL_JoystickID ptr
declare function SDL_IsGamepad(byval instance_id as SDL_JoystickID) as boolean
declare function SDL_GetGamepadNameForID(byval instance_id as SDL_JoystickID) as const zstring ptr
declare function SDL_GetGamepadPathForID(byval instance_id as SDL_JoystickID) as const zstring ptr
declare function SDL_GetGamepadPlayerIndexForID(byval instance_id as SDL_JoystickID) as long
declare function SDL_GetGamepadGUIDForID(byval instance_id as SDL_JoystickID) as SDL_GUID
declare function SDL_GetGamepadVendorForID(byval instance_id as SDL_JoystickID) as Uint16
declare function SDL_GetGamepadProductForID(byval instance_id as SDL_JoystickID) as Uint16
declare function SDL_GetGamepadProductVersionForID(byval instance_id as SDL_JoystickID) as Uint16
declare function SDL_GetGamepadTypeForID(byval instance_id as SDL_JoystickID) as SDL_GamepadType
declare function SDL_GetRealGamepadTypeForID(byval instance_id as SDL_JoystickID) as SDL_GamepadType
declare function SDL_GetGamepadMappingForID(byval instance_id as SDL_JoystickID) as zstring ptr
declare function SDL_OpenGamepad(byval instance_id as SDL_JoystickID) as SDL_Gamepad ptr
declare function SDL_GetGamepadFromID(byval instance_id as SDL_JoystickID) as SDL_Gamepad ptr
declare function SDL_GetGamepadFromPlayerIndex(byval player_index as long) as SDL_Gamepad ptr
declare function SDL_GetGamepadProperties(byval gamepad as SDL_Gamepad ptr) as SDL_PropertiesID

#define SDL_PROP_GAMEPAD_CAP_MONO_LED_BOOLEAN SDL_PROP_JOYSTICK_CAP_MONO_LED_BOOLEAN
#define SDL_PROP_GAMEPAD_CAP_RGB_LED_BOOLEAN SDL_PROP_JOYSTICK_CAP_RGB_LED_BOOLEAN
#define SDL_PROP_GAMEPAD_CAP_PLAYER_LED_BOOLEAN SDL_PROP_JOYSTICK_CAP_PLAYER_LED_BOOLEAN
#define SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN SDL_PROP_JOYSTICK_CAP_RUMBLE_BOOLEAN
#define SDL_PROP_GAMEPAD_CAP_TRIGGER_RUMBLE_BOOLEAN SDL_PROP_JOYSTICK_CAP_TRIGGER_RUMBLE_BOOLEAN

declare function SDL_GetGamepadID(byval gamepad as SDL_Gamepad ptr) as SDL_JoystickID
declare function SDL_GetGamepadName(byval gamepad as SDL_Gamepad ptr) as const zstring ptr
declare function SDL_GetGamepadPath(byval gamepad as SDL_Gamepad ptr) as const zstring ptr
declare function SDL_GetGamepadType(byval gamepad as SDL_Gamepad ptr) as SDL_GamepadType
declare function SDL_GetRealGamepadType(byval gamepad as SDL_Gamepad ptr) as SDL_GamepadType
declare function SDL_GetGamepadPlayerIndex(byval gamepad as SDL_Gamepad ptr) as long
declare function SDL_SetGamepadPlayerIndex(byval gamepad as SDL_Gamepad ptr, byval player_index as long) as boolean
declare function SDL_GetGamepadVendor(byval gamepad as SDL_Gamepad ptr) as Uint16
declare function SDL_GetGamepadProduct(byval gamepad as SDL_Gamepad ptr) as Uint16
declare function SDL_GetGamepadProductVersion(byval gamepad as SDL_Gamepad ptr) as Uint16
declare function SDL_GetGamepadFirmwareVersion(byval gamepad as SDL_Gamepad ptr) as Uint16
declare function SDL_GetGamepadSerial(byval gamepad as SDL_Gamepad ptr) as const zstring ptr
declare function SDL_GetGamepadSteamHandle(byval gamepad as SDL_Gamepad ptr) as Uint64
declare function SDL_GetGamepadConnectionState(byval gamepad as SDL_Gamepad ptr) as SDL_JoystickConnectionState
declare function SDL_GetGamepadPowerInfo(byval gamepad as SDL_Gamepad ptr, byval percent as long ptr) as SDL_PowerState
declare function SDL_GamepadConnected(byval gamepad as SDL_Gamepad ptr) as boolean
declare function SDL_GetGamepadJoystick(byval gamepad as SDL_Gamepad ptr) as SDL_Joystick ptr
declare sub SDL_SetGamepadEventsEnabled(byval enabled as boolean)
declare function SDL_GamepadEventsEnabled() as boolean
declare function SDL_GetGamepadBindings(byval gamepad as SDL_Gamepad ptr, byval count as long ptr) as SDL_GamepadBinding ptr ptr
declare sub SDL_UpdateGamepads()
declare function SDL_GetGamepadTypeFromString(byval str as const zstring ptr) as SDL_GamepadType
declare function SDL_GetGamepadStringForType(byval type as SDL_GamepadType) as const zstring ptr
declare function SDL_GetGamepadAxisFromString(byval str as const zstring ptr) as SDL_GamepadAxis
declare function SDL_GetGamepadStringForAxis(byval axis as SDL_GamepadAxis) as const zstring ptr
declare function SDL_GamepadHasAxis(byval gamepad as SDL_Gamepad ptr, byval axis as SDL_GamepadAxis) as boolean
declare function SDL_GetGamepadAxis(byval gamepad as SDL_Gamepad ptr, byval axis as SDL_GamepadAxis) as Sint16
declare function SDL_GetGamepadButtonFromString(byval str as const zstring ptr) as SDL_GamepadButton
declare function SDL_GetGamepadStringForButton(byval button as SDL_GamepadButton) as const zstring ptr
declare function SDL_GamepadHasButton(byval gamepad as SDL_Gamepad ptr, byval button as SDL_GamepadButton) as boolean
declare function SDL_GetGamepadButton(byval gamepad as SDL_Gamepad ptr, byval button as SDL_GamepadButton) as boolean
declare function SDL_GetGamepadButtonLabelForType(byval type as SDL_GamepadType, byval button as SDL_GamepadButton) as SDL_GamepadButtonLabel
declare function SDL_GetGamepadButtonLabel(byval gamepad as SDL_Gamepad ptr, byval button as SDL_GamepadButton) as SDL_GamepadButtonLabel
declare function SDL_GetNumGamepadTouchpads(byval gamepad as SDL_Gamepad ptr) as long
declare function SDL_GetNumGamepadTouchpadFingers(byval gamepad as SDL_Gamepad ptr, byval touchpad as long) as long
declare function SDL_GetGamepadTouchpadFinger(byval gamepad as SDL_Gamepad ptr, byval touchpad as long, byval finger as long, byval down as boolean ptr, byval x as single ptr, byval y as single ptr, byval pressure as single ptr) as boolean
declare function SDL_GamepadHasSensor(byval gamepad as SDL_Gamepad ptr, byval type as SDL_SensorType) as boolean
declare function SDL_SetGamepadSensorEnabled(byval gamepad as SDL_Gamepad ptr, byval type as SDL_SensorType, byval enabled as boolean) as boolean
declare function SDL_GamepadSensorEnabled(byval gamepad as SDL_Gamepad ptr, byval type as SDL_SensorType) as boolean
declare function SDL_GetGamepadSensorDataRate(byval gamepad as SDL_Gamepad ptr, byval type as SDL_SensorType) as single
declare function SDL_GetGamepadSensorData(byval gamepad as SDL_Gamepad ptr, byval type as SDL_SensorType, byval data as single ptr, byval num_values as long) as boolean
declare function SDL_RumbleGamepad(byval gamepad as SDL_Gamepad ptr, byval low_frequency_rumble as Uint16, byval high_frequency_rumble as Uint16, byval duration_ms as Uint32) as boolean
declare function SDL_RumbleGamepadTriggers(byval gamepad as SDL_Gamepad ptr, byval left_rumble as Uint16, byval right_rumble as Uint16, byval duration_ms as Uint32) as boolean
declare function SDL_SetGamepadLED(byval gamepad as SDL_Gamepad ptr, byval red as Uint8, byval green as Uint8, byval blue as Uint8) as boolean
declare function SDL_SendGamepadEffect(byval gamepad as SDL_Gamepad ptr, byval data as const any ptr, byval size as long) as boolean
declare sub SDL_CloseGamepad(byval gamepad as SDL_Gamepad ptr)
declare function SDL_GetGamepadAppleSFSymbolsNameForButton(byval gamepad as SDL_Gamepad ptr, byval button as SDL_GamepadButton) as const zstring ptr
declare function SDL_GetGamepadAppleSFSymbolsNameForAxis(byval gamepad as SDL_Gamepad ptr, byval axis as SDL_GamepadAxis) as const zstring ptr

'' -------------------------------------------------------------------------
'' SDL_keyboard.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_keycode.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_scancode.h
'' -------------------------------------------------------------------------
type SDL_Scancode as long
enum
	SDL_SCANCODE_UNKNOWN = 0
	SDL_SCANCODE_A = 4
	SDL_SCANCODE_B = 5
	SDL_SCANCODE_C = 6
	SDL_SCANCODE_D = 7
	SDL_SCANCODE_E = 8
	SDL_SCANCODE_F = 9
	SDL_SCANCODE_G = 10
	SDL_SCANCODE_H = 11
	SDL_SCANCODE_I = 12
	SDL_SCANCODE_J = 13
	SDL_SCANCODE_K = 14
	SDL_SCANCODE_L = 15
	SDL_SCANCODE_M = 16
	SDL_SCANCODE_N = 17
	SDL_SCANCODE_O = 18
	SDL_SCANCODE_P = 19
	SDL_SCANCODE_Q = 20
	SDL_SCANCODE_R = 21
	SDL_SCANCODE_S = 22
	SDL_SCANCODE_T = 23
	SDL_SCANCODE_U = 24
	SDL_SCANCODE_V = 25
	SDL_SCANCODE_W = 26
	SDL_SCANCODE_X = 27
	SDL_SCANCODE_Y = 28
	SDL_SCANCODE_Z = 29
	SDL_SCANCODE_1 = 30
	SDL_SCANCODE_2 = 31
	SDL_SCANCODE_3 = 32
	SDL_SCANCODE_4 = 33
	SDL_SCANCODE_5 = 34
	SDL_SCANCODE_6 = 35
	SDL_SCANCODE_7 = 36
	SDL_SCANCODE_8 = 37
	SDL_SCANCODE_9 = 38
	SDL_SCANCODE_0 = 39
	SDL_SCANCODE_RETURN = 40
	SDL_SCANCODE_ESCAPE = 41
	SDL_SCANCODE_BACKSPACE = 42
	SDL_SCANCODE_TAB = 43
	SDL_SCANCODE_SPACE = 44
	SDL_SCANCODE_MINUS = 45
	SDL_SCANCODE_EQUALS = 46
	SDL_SCANCODE_LEFTBRACKET = 47
	SDL_SCANCODE_RIGHTBRACKET = 48
	SDL_SCANCODE_BACKSLASH = 49
	SDL_SCANCODE_NONUSHASH = 50
	SDL_SCANCODE_SEMICOLON = 51
	SDL_SCANCODE_APOSTROPHE = 52
	SDL_SCANCODE_GRAVE = 53
	SDL_SCANCODE_COMMA = 54
	SDL_SCANCODE_PERIOD = 55
	SDL_SCANCODE_SLASH = 56
	SDL_SCANCODE_CAPSLOCK = 57
	SDL_SCANCODE_F1 = 58
	SDL_SCANCODE_F2 = 59
	SDL_SCANCODE_F3 = 60
	SDL_SCANCODE_F4 = 61
	SDL_SCANCODE_F5 = 62
	SDL_SCANCODE_F6 = 63
	SDL_SCANCODE_F7 = 64
	SDL_SCANCODE_F8 = 65
	SDL_SCANCODE_F9 = 66
	SDL_SCANCODE_F10 = 67
	SDL_SCANCODE_F11 = 68
	SDL_SCANCODE_F12 = 69
	SDL_SCANCODE_PRINTSCREEN = 70
	SDL_SCANCODE_SCROLLLOCK = 71
	SDL_SCANCODE_PAUSE = 72
	SDL_SCANCODE_INSERT = 73
	SDL_SCANCODE_HOME = 74
	SDL_SCANCODE_PAGEUP = 75
	SDL_SCANCODE_DELETE = 76
	SDL_SCANCODE_END = 77
	SDL_SCANCODE_PAGEDOWN = 78
	SDL_SCANCODE_RIGHT = 79
	SDL_SCANCODE_LEFT = 80
	SDL_SCANCODE_DOWN = 81
	SDL_SCANCODE_UP = 82
	SDL_SCANCODE_NUMLOCKCLEAR = 83
	SDL_SCANCODE_KP_DIVIDE = 84
	SDL_SCANCODE_KP_MULTIPLY = 85
	SDL_SCANCODE_KP_MINUS = 86
	SDL_SCANCODE_KP_PLUS = 87
	SDL_SCANCODE_KP_ENTER = 88
	SDL_SCANCODE_KP_1 = 89
	SDL_SCANCODE_KP_2 = 90
	SDL_SCANCODE_KP_3 = 91
	SDL_SCANCODE_KP_4 = 92
	SDL_SCANCODE_KP_5 = 93
	SDL_SCANCODE_KP_6 = 94
	SDL_SCANCODE_KP_7 = 95
	SDL_SCANCODE_KP_8 = 96
	SDL_SCANCODE_KP_9 = 97
	SDL_SCANCODE_KP_0 = 98
	SDL_SCANCODE_KP_PERIOD = 99
	SDL_SCANCODE_NONUSBACKSLASH = 100
	SDL_SCANCODE_APPLICATION = 101
	SDL_SCANCODE_POWER = 102
	SDL_SCANCODE_KP_EQUALS = 103
	SDL_SCANCODE_F13 = 104
	SDL_SCANCODE_F14 = 105
	SDL_SCANCODE_F15 = 106
	SDL_SCANCODE_F16 = 107
	SDL_SCANCODE_F17 = 108
	SDL_SCANCODE_F18 = 109
	SDL_SCANCODE_F19 = 110
	SDL_SCANCODE_F20 = 111
	SDL_SCANCODE_F21 = 112
	SDL_SCANCODE_F22 = 113
	SDL_SCANCODE_F23 = 114
	SDL_SCANCODE_F24 = 115
	SDL_SCANCODE_EXECUTE = 116
	SDL_SCANCODE_HELP = 117
	SDL_SCANCODE_MENU = 118
	SDL_SCANCODE_SELECT = 119
	SDL_SCANCODE_STOP = 120
	SDL_SCANCODE_AGAIN = 121
	SDL_SCANCODE_UNDO = 122
	SDL_SCANCODE_CUT = 123
	SDL_SCANCODE_COPY = 124
	SDL_SCANCODE_PASTE = 125
	SDL_SCANCODE_FIND = 126
	SDL_SCANCODE_MUTE = 127
	SDL_SCANCODE_VOLUMEUP = 128
	SDL_SCANCODE_VOLUMEDOWN = 129
	SDL_SCANCODE_KP_COMMA = 133
	SDL_SCANCODE_KP_EQUALSAS400 = 134
	SDL_SCANCODE_INTERNATIONAL1 = 135
	SDL_SCANCODE_INTERNATIONAL2 = 136
	SDL_SCANCODE_INTERNATIONAL3 = 137
	SDL_SCANCODE_INTERNATIONAL4 = 138
	SDL_SCANCODE_INTERNATIONAL5 = 139
	SDL_SCANCODE_INTERNATIONAL6 = 140
	SDL_SCANCODE_INTERNATIONAL7 = 141
	SDL_SCANCODE_INTERNATIONAL8 = 142
	SDL_SCANCODE_INTERNATIONAL9 = 143
	SDL_SCANCODE_LANG1 = 144
	SDL_SCANCODE_LANG2 = 145
	SDL_SCANCODE_LANG3 = 146
	SDL_SCANCODE_LANG4 = 147
	SDL_SCANCODE_LANG5 = 148
	SDL_SCANCODE_LANG6 = 149
	SDL_SCANCODE_LANG7 = 150
	SDL_SCANCODE_LANG8 = 151
	SDL_SCANCODE_LANG9 = 152
	SDL_SCANCODE_ALTERASE = 153
	SDL_SCANCODE_SYSREQ = 154
	SDL_SCANCODE_CANCEL = 155
	SDL_SCANCODE_CLEAR = 156
	SDL_SCANCODE_PRIOR = 157
	SDL_SCANCODE_RETURN2 = 158
	SDL_SCANCODE_SEPARATOR = 159
	SDL_SCANCODE_OUT = 160
	SDL_SCANCODE_OPER = 161
	SDL_SCANCODE_CLEARAGAIN = 162
	SDL_SCANCODE_CRSEL = 163
	SDL_SCANCODE_EXSEL = 164
	SDL_SCANCODE_FRONT = 165
	SDL_SCANCODE_KP_00 = 176
	SDL_SCANCODE_KP_000 = 177
	SDL_SCANCODE_THOUSANDSSEPARATOR = 178
	SDL_SCANCODE_DECIMALSEPARATOR = 179
	SDL_SCANCODE_CURRENCYUNIT = 180
	SDL_SCANCODE_CURRENCYSUBUNIT = 181
	SDL_SCANCODE_KP_LEFTPAREN = 182
	SDL_SCANCODE_KP_RIGHTPAREN = 183
	SDL_SCANCODE_KP_LEFTBRACE = 184
	SDL_SCANCODE_KP_RIGHTBRACE = 185
	SDL_SCANCODE_KP_TAB = 186
	SDL_SCANCODE_KP_BACKSPACE = 187
	SDL_SCANCODE_KP_A = 188
	SDL_SCANCODE_KP_B = 189
	SDL_SCANCODE_KP_C = 190
	SDL_SCANCODE_KP_D = 191
	SDL_SCANCODE_KP_E = 192
	SDL_SCANCODE_KP_F = 193
	SDL_SCANCODE_KP_XOR = 194
	SDL_SCANCODE_KP_POWER = 195
	SDL_SCANCODE_KP_PERCENT = 196
	SDL_SCANCODE_KP_LESS = 197
	SDL_SCANCODE_KP_GREATER = 198
	SDL_SCANCODE_KP_AMPERSAND = 199
	SDL_SCANCODE_KP_DBLAMPERSAND = 200
	SDL_SCANCODE_KP_VERTICALBAR = 201
	SDL_SCANCODE_KP_DBLVERTICALBAR = 202
	SDL_SCANCODE_KP_COLON = 203
	SDL_SCANCODE_KP_HASH = 204
	SDL_SCANCODE_KP_SPACE = 205
	SDL_SCANCODE_KP_AT = 206
	SDL_SCANCODE_KP_EXCLAM = 207
	SDL_SCANCODE_KP_MEMSTORE = 208
	SDL_SCANCODE_KP_MEMRECALL = 209
	SDL_SCANCODE_KP_MEMCLEAR = 210
	SDL_SCANCODE_KP_MEMADD = 211
	SDL_SCANCODE_KP_MEMSUBTRACT = 212
	SDL_SCANCODE_KP_MEMMULTIPLY = 213
	SDL_SCANCODE_KP_MEMDIVIDE = 214
	SDL_SCANCODE_KP_PLUSMINUS = 215
	SDL_SCANCODE_KP_CLEAR = 216
	SDL_SCANCODE_KP_CLEARENTRY = 217
	SDL_SCANCODE_KP_BINARY = 218
	SDL_SCANCODE_KP_OCTAL = 219
	SDL_SCANCODE_KP_DECIMAL = 220
	SDL_SCANCODE_KP_HEXADECIMAL = 221
	SDL_SCANCODE_LCTRL = 224
	SDL_SCANCODE_LSHIFT = 225
	SDL_SCANCODE_LALT = 226
	SDL_SCANCODE_LGUI = 227
	SDL_SCANCODE_RCTRL = 228
	SDL_SCANCODE_RSHIFT = 229
	SDL_SCANCODE_RALT = 230
	SDL_SCANCODE_RGUI = 231
	SDL_SCANCODE_MODE = 257
	SDL_SCANCODE_SLEEP = 258
	SDL_SCANCODE_WAKE = 259
	SDL_SCANCODE_CHANNEL_INCREMENT = 260
	SDL_SCANCODE_CHANNEL_DECREMENT = 261
	SDL_SCANCODE_MEDIA_PLAY = 262
	SDL_SCANCODE_MEDIA_PAUSE = 263
	SDL_SCANCODE_MEDIA_RECORD = 264
	SDL_SCANCODE_MEDIA_FAST_FORWARD = 265
	SDL_SCANCODE_MEDIA_REWIND = 266
	SDL_SCANCODE_MEDIA_NEXT_TRACK = 267
	SDL_SCANCODE_MEDIA_PREVIOUS_TRACK = 268
	SDL_SCANCODE_MEDIA_STOP = 269
	SDL_SCANCODE_MEDIA_EJECT = 270
	SDL_SCANCODE_MEDIA_PLAY_PAUSE = 271
	SDL_SCANCODE_MEDIA_SELECT = 272
	SDL_SCANCODE_AC_NEW = 273
	SDL_SCANCODE_AC_OPEN = 274
	SDL_SCANCODE_AC_CLOSE = 275
	SDL_SCANCODE_AC_EXIT = 276
	SDL_SCANCODE_AC_SAVE = 277
	SDL_SCANCODE_AC_PRINT = 278
	SDL_SCANCODE_AC_PROPERTIES = 279
	SDL_SCANCODE_AC_SEARCH = 280
	SDL_SCANCODE_AC_HOME = 281
	SDL_SCANCODE_AC_BACK = 282
	SDL_SCANCODE_AC_FORWARD = 283
	SDL_SCANCODE_AC_STOP = 284
	SDL_SCANCODE_AC_REFRESH = 285
	SDL_SCANCODE_AC_BOOKMARKS = 286
	SDL_SCANCODE_SOFTLEFT = 287
	SDL_SCANCODE_SOFTRIGHT = 288
	SDL_SCANCODE_CALL = 289
	SDL_SCANCODE_ENDCALL = 290
	SDL_SCANCODE_RESERVED = 400
	SDL_SCANCODE_COUNT = 512
end enum

type SDL_Keycode as Uint32
const SDLK_EXTENDED_MASK = culng(1u shl 29)
const SDLK_SCANCODE_MASK = culng(1u shl 30)
#define SDL_SCANCODE_TO_KEYCODE(X) (X or SDLK_SCANCODE_MASK)
const SDLK_UNKNOWN = &h00000000u
const SDLK_RETURN = &h0000000du
const SDLK_ESCAPE = &h0000001bu
const SDLK_BACKSPACE = &h00000008u
const SDLK_TAB = &h00000009u
const SDLK_SPACE = &h00000020u
const SDLK_EXCLAIM = &h00000021u
const SDLK_DBLAPOSTROPHE = &h00000022u
const SDLK_HASH = &h00000023u
const SDLK_DOLLAR = &h00000024u
const SDLK_PERCENT = &h00000025u
const SDLK_AMPERSAND = &h00000026u
const SDLK_APOSTROPHE = &h00000027u
const SDLK_LEFTPAREN = &h00000028u
const SDLK_RIGHTPAREN = &h00000029u
const SDLK_ASTERISK = &h0000002au
const SDLK_PLUS = &h0000002bu
const SDLK_COMMA = &h0000002cu
const SDLK_MINUS = &h0000002du
const SDLK_PERIOD = &h0000002eu
const SDLK_SLASH = &h0000002fu
const SDLK_0 = &h00000030u
const SDLK_1 = &h00000031u
const SDLK_2 = &h00000032u
const SDLK_3 = &h00000033u
const SDLK_4 = &h00000034u
const SDLK_5 = &h00000035u
const SDLK_6 = &h00000036u
const SDLK_7 = &h00000037u
const SDLK_8 = &h00000038u
const SDLK_9 = &h00000039u
const SDLK_COLON = &h0000003au
const SDLK_SEMICOLON = &h0000003bu
const SDLK_LESS = &h0000003cu
const SDLK_EQUALS = &h0000003du
const SDLK_GREATER = &h0000003eu
const SDLK_QUESTION = &h0000003fu
const SDLK_AT = &h00000040u
const SDLK_LEFTBRACKET = &h0000005bu
const SDLK_BACKSLASH = &h0000005cu
const SDLK_RIGHTBRACKET = &h0000005du
const SDLK_CARET = &h0000005eu
const SDLK_UNDERSCORE = &h0000005fu
const SDLK_GRAVE = &h00000060u
const SDLK_A = &h00000061u
const SDLK_B = &h00000062u
const SDLK_C = &h00000063u
const SDLK_D = &h00000064u
const SDLK_E = &h00000065u
const SDLK_F = &h00000066u
const SDLK_G = &h00000067u
const SDLK_H = &h00000068u
const SDLK_I = &h00000069u
const SDLK_J = &h0000006au
const SDLK_K = &h0000006bu
const SDLK_L = &h0000006cu
const SDLK_M = &h0000006du
const SDLK_N = &h0000006eu
const SDLK_O = &h0000006fu
const SDLK_P = &h00000070u
const SDLK_Q = &h00000071u
const SDLK_R = &h00000072u
const SDLK_S = &h00000073u
const SDLK_T = &h00000074u
const SDLK_U = &h00000075u
const SDLK_V = &h00000076u
const SDLK_W = &h00000077u
const SDLK_X = &h00000078u
const SDLK_Y = &h00000079u
const SDLK_Z = &h0000007au
const SDLK_LEFTBRACE = &h0000007bu
const SDLK_PIPE = &h0000007cu
const SDLK_RIGHTBRACE = &h0000007du
const SDLK_TILDE = &h0000007eu
const SDLK_DELETE = &h0000007fu
const SDLK_PLUSMINUS = &h000000b1u
const SDLK_CAPSLOCK = &h40000039u
const SDLK_F1 = &h4000003au
const SDLK_F2 = &h4000003bu
const SDLK_F3 = &h4000003cu
const SDLK_F4 = &h4000003du
const SDLK_F5 = &h4000003eu
const SDLK_F6 = &h4000003fu
const SDLK_F7 = &h40000040u
const SDLK_F8 = &h40000041u
const SDLK_F9 = &h40000042u
const SDLK_F10 = &h40000043u
const SDLK_F11 = &h40000044u
const SDLK_F12 = &h40000045u
const SDLK_PRINTSCREEN = &h40000046u
const SDLK_SCROLLLOCK = &h40000047u
const SDLK_PAUSE = &h40000048u
const SDLK_INSERT = &h40000049u
const SDLK_HOME = &h4000004au
const SDLK_PAGEUP = &h4000004bu
const SDLK_END = &h4000004du
const SDLK_PAGEDOWN = &h4000004eu
const SDLK_RIGHT = &h4000004fu
const SDLK_LEFT = &h40000050u
const SDLK_DOWN = &h40000051u
const SDLK_UP = &h40000052u
const SDLK_NUMLOCKCLEAR = &h40000053u
const SDLK_KP_DIVIDE = &h40000054u
const SDLK_KP_MULTIPLY = &h40000055u
const SDLK_KP_MINUS = &h40000056u
const SDLK_KP_PLUS = &h40000057u
const SDLK_KP_ENTER = &h40000058u
const SDLK_KP_1 = &h40000059u
const SDLK_KP_2 = &h4000005au
const SDLK_KP_3 = &h4000005bu
const SDLK_KP_4 = &h4000005cu
const SDLK_KP_5 = &h4000005du
const SDLK_KP_6 = &h4000005eu
const SDLK_KP_7 = &h4000005fu
const SDLK_KP_8 = &h40000060u
const SDLK_KP_9 = &h40000061u
const SDLK_KP_0 = &h40000062u
const SDLK_KP_PERIOD = &h40000063u
const SDLK_APPLICATION = &h40000065u
const SDLK_POWER = &h40000066u
const SDLK_KP_EQUALS = &h40000067u
const SDLK_F13 = &h40000068u
const SDLK_F14 = &h40000069u
const SDLK_F15 = &h4000006au
const SDLK_F16 = &h4000006bu
const SDLK_F17 = &h4000006cu
const SDLK_F18 = &h4000006du
const SDLK_F19 = &h4000006eu
const SDLK_F20 = &h4000006fu
const SDLK_F21 = &h40000070u
const SDLK_F22 = &h40000071u
const SDLK_F23 = &h40000072u
const SDLK_F24 = &h40000073u
const SDLK_EXECUTE = &h40000074u
const SDLK_HELP = &h40000075u
const SDLK_MENU = &h40000076u
const SDLK_SELECT = &h40000077u
const SDLK_STOP = &h40000078u
const SDLK_AGAIN = &h40000079u
const SDLK_UNDO = &h4000007au
const SDLK_CUT = &h4000007bu
const SDLK_COPY = &h4000007cu
const SDLK_PASTE = &h4000007du
const SDLK_FIND = &h4000007eu
const SDLK_MUTE = &h4000007fu
const SDLK_VOLUMEUP = &h40000080u
const SDLK_VOLUMEDOWN = &h40000081u
const SDLK_KP_COMMA = &h40000085u
const SDLK_KP_EQUALSAS400 = &h40000086u
const SDLK_ALTERASE = &h40000099u
const SDLK_SYSREQ = &h4000009au
const SDLK_CANCEL = &h4000009bu
const SDLK_CLEAR = &h4000009cu
const SDLK_PRIOR = &h4000009du
const SDLK_RETURN2 = &h4000009eu
const SDLK_SEPARATOR = &h4000009fu
const SDLK_OUT = &h400000a0u
const SDLK_OPER = &h400000a1u
const SDLK_CLEARAGAIN = &h400000a2u
const SDLK_CRSEL = &h400000a3u
const SDLK_EXSEL = &h400000a4u
const SDLK_FRONT = &h400000a5u
const SDLK_KP_00 = &h400000b0u
const SDLK_KP_000 = &h400000b1u
const SDLK_THOUSANDSSEPARATOR = &h400000b2u
const SDLK_DECIMALSEPARATOR = &h400000b3u
const SDLK_CURRENCYUNIT = &h400000b4u
const SDLK_CURRENCYSUBUNIT = &h400000b5u
const SDLK_KP_LEFTPAREN = &h400000b6u
const SDLK_KP_RIGHTPAREN = &h400000b7u
const SDLK_KP_LEFTBRACE = &h400000b8u
const SDLK_KP_RIGHTBRACE = &h400000b9u
const SDLK_KP_TAB = &h400000bau
const SDLK_KP_BACKSPACE = &h400000bbu
const SDLK_KP_A = &h400000bcu
const SDLK_KP_B = &h400000bdu
const SDLK_KP_C = &h400000beu
const SDLK_KP_D = &h400000bfu
const SDLK_KP_E = &h400000c0u
const SDLK_KP_F = &h400000c1u
const SDLK_KP_XOR = &h400000c2u
const SDLK_KP_POWER = &h400000c3u
const SDLK_KP_PERCENT = &h400000c4u
const SDLK_KP_LESS = &h400000c5u
const SDLK_KP_GREATER = &h400000c6u
const SDLK_KP_AMPERSAND = &h400000c7u
const SDLK_KP_DBLAMPERSAND = &h400000c8u
const SDLK_KP_VERTICALBAR = &h400000c9u
const SDLK_KP_DBLVERTICALBAR = &h400000cau
const SDLK_KP_COLON = &h400000cbu
const SDLK_KP_HASH = &h400000ccu
const SDLK_KP_SPACE = &h400000cdu
const SDLK_KP_AT = &h400000ceu
const SDLK_KP_EXCLAM = &h400000cfu
const SDLK_KP_MEMSTORE = &h400000d0u
const SDLK_KP_MEMRECALL = &h400000d1u
const SDLK_KP_MEMCLEAR = &h400000d2u
const SDLK_KP_MEMADD = &h400000d3u
const SDLK_KP_MEMSUBTRACT = &h400000d4u
const SDLK_KP_MEMMULTIPLY = &h400000d5u
const SDLK_KP_MEMDIVIDE = &h400000d6u
const SDLK_KP_PLUSMINUS = &h400000d7u
const SDLK_KP_CLEAR = &h400000d8u
const SDLK_KP_CLEARENTRY = &h400000d9u
const SDLK_KP_BINARY = &h400000dau
const SDLK_KP_OCTAL = &h400000dbu
const SDLK_KP_DECIMAL = &h400000dcu
const SDLK_KP_HEXADECIMAL = &h400000ddu
const SDLK_LCTRL = &h400000e0u
const SDLK_LSHIFT = &h400000e1u
const SDLK_LALT = &h400000e2u
const SDLK_LGUI = &h400000e3u
const SDLK_RCTRL = &h400000e4u
const SDLK_RSHIFT = &h400000e5u
const SDLK_RALT = &h400000e6u
const SDLK_RGUI = &h400000e7u
const SDLK_MODE = &h40000101u
const SDLK_SLEEP = &h40000102u
const SDLK_WAKE = &h40000103u
const SDLK_CHANNEL_INCREMENT = &h40000104u
const SDLK_CHANNEL_DECREMENT = &h40000105u
const SDLK_MEDIA_PLAY = &h40000106u
const SDLK_MEDIA_PAUSE = &h40000107u
const SDLK_MEDIA_RECORD = &h40000108u
const SDLK_MEDIA_FAST_FORWARD = &h40000109u
const SDLK_MEDIA_REWIND = &h4000010au
const SDLK_MEDIA_NEXT_TRACK = &h4000010bu
const SDLK_MEDIA_PREVIOUS_TRACK = &h4000010cu
const SDLK_MEDIA_STOP = &h4000010du
const SDLK_MEDIA_EJECT = &h4000010eu
const SDLK_MEDIA_PLAY_PAUSE = &h4000010fu
const SDLK_MEDIA_SELECT = &h40000110u
const SDLK_AC_NEW = &h40000111u
const SDLK_AC_OPEN = &h40000112u
const SDLK_AC_CLOSE = &h40000113u
const SDLK_AC_EXIT = &h40000114u
const SDLK_AC_SAVE = &h40000115u
const SDLK_AC_PRINT = &h40000116u
const SDLK_AC_PROPERTIES = &h40000117u
const SDLK_AC_SEARCH = &h40000118u
const SDLK_AC_HOME = &h40000119u
const SDLK_AC_BACK = &h4000011au
const SDLK_AC_FORWARD = &h4000011bu
const SDLK_AC_STOP = &h4000011cu
const SDLK_AC_REFRESH = &h4000011du
const SDLK_AC_BOOKMARKS = &h4000011eu
const SDLK_SOFTLEFT = &h4000011fu
const SDLK_SOFTRIGHT = &h40000120u
const SDLK_CALL = &h40000121u
const SDLK_ENDCALL = &h40000122u
const SDLK_LEFT_TAB = &h20000001u
const SDLK_LEVEL5_SHIFT = &h20000002u
const SDLK_MULTI_KEY_COMPOSE = &h20000003u
const SDLK_LMETA = &h20000004u
const SDLK_RMETA = &h20000005u
const SDLK_LHYPER = &h20000006u
const SDLK_RHYPER = &h20000007u
type SDL_Keymod as Uint16
const SDL_KMOD_NONE = &h0000u
const SDL_KMOD_LSHIFT = &h0001u
const SDL_KMOD_RSHIFT = &h0002u
const SDL_KMOD_LEVEL5 = &h0004u
const SDL_KMOD_LCTRL = &h0040u
const SDL_KMOD_RCTRL = &h0080u
const SDL_KMOD_LALT = &h0100u
const SDL_KMOD_RALT = &h0200u
const SDL_KMOD_LGUI = &h0400u
const SDL_KMOD_RGUI = &h0800u
const SDL_KMOD_NUM = &h1000u
const SDL_KMOD_CAPS = &h2000u
const SDL_KMOD_MODE = &h4000u
const SDL_KMOD_SCROLL = &h8000u
const SDL_KMOD_CTRL = culng(SDL_KMOD_LCTRL or SDL_KMOD_RCTRL)
const SDL_KMOD_SHIFT = culng(SDL_KMOD_LSHIFT or SDL_KMOD_RSHIFT)
const SDL_KMOD_ALT = culng(SDL_KMOD_LALT or SDL_KMOD_RALT)
const SDL_KMOD_GUI = culng(SDL_KMOD_LGUI or SDL_KMOD_RGUI)
type SDL_KeyboardID as Uint32

declare function SDL_HasKeyboard() as boolean
declare function SDL_GetKeyboards(byval count as long ptr) as SDL_KeyboardID ptr
declare function SDL_GetKeyboardNameForID(byval instance_id as SDL_KeyboardID) as const zstring ptr
declare function SDL_GetKeyboardFocus() as SDL_Window ptr
declare function SDL_GetKeyboardState(byval numkeys as long ptr) as const byte ptr
declare sub SDL_ResetKeyboard()
declare function SDL_GetModState() as SDL_Keymod
declare sub SDL_SetModState(byval modstate as SDL_Keymod)
declare function SDL_GetKeyFromScancode(byval scancode as SDL_Scancode, byval modstate as SDL_Keymod, byval key_event as boolean) as SDL_Keycode
declare function SDL_GetScancodeFromKey(byval key as SDL_Keycode, byval modstate as SDL_Keymod ptr) as SDL_Scancode
declare function SDL_SetScancodeName(byval scancode as SDL_Scancode, byval name as const zstring ptr) as boolean
declare function SDL_GetScancodeName(byval scancode as SDL_Scancode) as const zstring ptr
declare function SDL_GetScancodeFromName(byval name as const zstring ptr) as SDL_Scancode
declare function SDL_GetKeyName(byval key as SDL_Keycode) as const zstring ptr
declare function SDL_GetKeyFromName(byval name as const zstring ptr) as SDL_Keycode
declare function SDL_StartTextInput(byval window as SDL_Window ptr) as boolean

type SDL_TextInputType as long
enum
	SDL_TEXTINPUT_TYPE_TEXT
	SDL_TEXTINPUT_TYPE_TEXT_NAME
	SDL_TEXTINPUT_TYPE_TEXT_EMAIL
	SDL_TEXTINPUT_TYPE_TEXT_USERNAME
	SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN
	SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_VISIBLE
	SDL_TEXTINPUT_TYPE_NUMBER
	SDL_TEXTINPUT_TYPE_NUMBER_PASSWORD_HIDDEN
	SDL_TEXTINPUT_TYPE_NUMBER_PASSWORD_VISIBLE
end enum

type SDL_Capitalization as long
enum
	SDL_CAPITALIZE_NONE
	SDL_CAPITALIZE_SENTENCES
	SDL_CAPITALIZE_WORDS
	SDL_CAPITALIZE_LETTERS
end enum

declare function SDL_StartTextInputWithProperties(byval window as SDL_Window ptr, byval props as SDL_PropertiesID) as boolean
#define SDL_PROP_TEXTINPUT_TYPE_NUMBER "SDL.textinput.type"
#define SDL_PROP_TEXTINPUT_CAPITALIZATION_NUMBER "SDL.textinput.capitalization"
#define SDL_PROP_TEXTINPUT_AUTOCORRECT_BOOLEAN "SDL.textinput.autocorrect"
#define SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN "SDL.textinput.multiline"
#define SDL_PROP_TEXTINPUT_ANDROID_INPUTTYPE_NUMBER "SDL.textinput.android.inputtype"

declare function SDL_TextInputActive(byval window as SDL_Window ptr) as boolean
declare function SDL_StopTextInput(byval window as SDL_Window ptr) as boolean
declare function SDL_ClearComposition(byval window as SDL_Window ptr) as boolean
declare function SDL_SetTextInputArea(byval window as SDL_Window ptr, byval rect as const SDL_Rect ptr, byval cursor as long) as boolean
declare function SDL_GetTextInputArea(byval window as SDL_Window ptr, byval rect as SDL_Rect ptr, byval cursor as long ptr) as boolean
declare function SDL_HasScreenKeyboardSupport() as boolean
declare function SDL_ScreenKeyboardShown(byval window as SDL_Window ptr) as boolean
'' -------------------------------------------------------------------------
'' SDL_mouse.h
'' -------------------------------------------------------------------------
type SDL_MouseID as Uint32

type SDL_SystemCursor as long
enum
	SDL_SYSTEM_CURSOR_DEFAULT
	SDL_SYSTEM_CURSOR_TEXT
	SDL_SYSTEM_CURSOR_WAIT
	SDL_SYSTEM_CURSOR_CROSSHAIR
	SDL_SYSTEM_CURSOR_PROGRESS
	SDL_SYSTEM_CURSOR_NWSE_RESIZE
	SDL_SYSTEM_CURSOR_NESW_RESIZE
	SDL_SYSTEM_CURSOR_EW_RESIZE
	SDL_SYSTEM_CURSOR_NS_RESIZE
	SDL_SYSTEM_CURSOR_MOVE
	SDL_SYSTEM_CURSOR_NOT_ALLOWED
	SDL_SYSTEM_CURSOR_POINTER
	SDL_SYSTEM_CURSOR_NW_RESIZE
	SDL_SYSTEM_CURSOR_N_RESIZE
	SDL_SYSTEM_CURSOR_NE_RESIZE
	SDL_SYSTEM_CURSOR_E_RESIZE
	SDL_SYSTEM_CURSOR_SE_RESIZE
	SDL_SYSTEM_CURSOR_S_RESIZE
	SDL_SYSTEM_CURSOR_SW_RESIZE
	SDL_SYSTEM_CURSOR_W_RESIZE
	SDL_SYSTEM_CURSOR_COUNT
end enum

type SDL_MouseWheelDirection as long
enum
	SDL_MOUSEWHEEL_NORMAL
	SDL_MOUSEWHEEL_FLIPPED
end enum

type SDL_CursorFrameInfo
	surface as SDL_Surface ptr
	duration as Uint32
end type

type SDL_MouseButtonFlags as Uint32
const SDL_BUTTON_LEFT = 1
const SDL_BUTTON_MIDDLE = 2
const SDL_BUTTON_RIGHT = 3
const SDL_BUTTON_X1 = 4
const SDL_BUTTON_X2 = 5
#define SDL_BUTTON_MASK(X) culng(1u shl ((X) - 1))
#define SDL_BUTTON_LMASK SDL_BUTTON_MASK(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK SDL_BUTTON_MASK(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK SDL_BUTTON_MASK(SDL_BUTTON_X2)
type SDL_MouseMotionTransformCallback as sub(byval userdata as any ptr, byval timestamp as Uint64, byval window as SDL_Window ptr, byval mouseID as SDL_MouseID, byval x as single ptr, byval y as single ptr)

declare function SDL_HasMouse() as boolean
declare function SDL_GetMice(byval count as long ptr) as SDL_MouseID ptr
declare function SDL_GetMouseNameForID(byval instance_id as SDL_MouseID) as const zstring ptr
declare function SDL_GetMouseFocus() as SDL_Window ptr
declare function SDL_GetMouseState(byval x as single ptr, byval y as single ptr) as SDL_MouseButtonFlags
declare function SDL_GetGlobalMouseState(byval x as single ptr, byval y as single ptr) as SDL_MouseButtonFlags
declare function SDL_GetRelativeMouseState(byval x as single ptr, byval y as single ptr) as SDL_MouseButtonFlags
declare sub SDL_WarpMouseInWindow(byval window as SDL_Window ptr, byval x as single, byval y as single)
declare function SDL_WarpMouseGlobal(byval x as single, byval y as single) as boolean
declare function SDL_SetRelativeMouseTransform(byval callback as SDL_MouseMotionTransformCallback, byval userdata as any ptr) as boolean
declare function SDL_SetWindowRelativeMouseMode(byval window as SDL_Window ptr, byval enabled as boolean) as boolean
declare function SDL_GetWindowRelativeMouseMode(byval window as SDL_Window ptr) as boolean
declare function SDL_CaptureMouse(byval enabled as boolean) as boolean
declare function SDL_CreateCursor(byval data as const Uint8 ptr, byval mask as const Uint8 ptr, byval w as long, byval h as long, byval hot_x as long, byval hot_y as long) as SDL_Cursor ptr
declare function SDL_CreateColorCursor(byval surface as SDL_Surface ptr, byval hot_x as long, byval hot_y as long) as SDL_Cursor ptr
declare function SDL_CreateAnimatedCursor(byval frames as SDL_CursorFrameInfo ptr, byval frame_count as long, byval hot_x as long, byval hot_y as long) as SDL_Cursor ptr
declare function SDL_CreateSystemCursor(byval id as SDL_SystemCursor) as SDL_Cursor ptr
declare function SDL_SetCursor(byval cursor as SDL_Cursor ptr) as boolean
declare function SDL_GetCursor() as SDL_Cursor ptr
declare function SDL_GetDefaultCursor() as SDL_Cursor ptr
declare sub SDL_DestroyCursor(byval cursor as SDL_Cursor ptr)
declare function SDL_ShowCursor() as boolean
declare function SDL_HideCursor() as boolean
declare function SDL_CursorVisible() as boolean

'' -------------------------------------------------------------------------
'' SDL_pen.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_touch.h
'' -------------------------------------------------------------------------
type SDL_TouchID as Uint64
type SDL_FingerID as Uint64

type SDL_TouchDeviceType as long
enum
	SDL_TOUCH_DEVICE_INVALID = -1
	SDL_TOUCH_DEVICE_DIRECT
	SDL_TOUCH_DEVICE_INDIRECT_ABSOLUTE
	SDL_TOUCH_DEVICE_INDIRECT_RELATIVE
end enum

type SDL_Finger
	id as SDL_FingerID
	x as single
	y as single
	pressure as single
end type

const SDL_TOUCH_MOUSEID = cast(SDL_MouseID, -1)
const SDL_MOUSE_TOUCHID = cast(SDL_TouchID, -1)
declare function SDL_GetTouchDevices(byval count as long ptr) as SDL_TouchID ptr
declare function SDL_GetTouchDeviceName(byval touchID as SDL_TouchID) as const zstring ptr
declare function SDL_GetTouchDeviceType(byval touchID as SDL_TouchID) as SDL_TouchDeviceType
declare function SDL_GetTouchFingers(byval touchID as SDL_TouchID, byval count as long ptr) as SDL_Finger ptr ptr
type SDL_PenID as Uint32
const SDL_PEN_MOUSEID = cast(SDL_MouseID, -2)
const SDL_PEN_TOUCHID = cast(SDL_TouchID, -2)
type SDL_PenInputFlags as Uint32

const SDL_PEN_INPUT_DOWN = culng(1u shl 0)
const SDL_PEN_INPUT_BUTTON_1 = culng(1u shl 1)
const SDL_PEN_INPUT_BUTTON_2 = culng(1u shl 2)
const SDL_PEN_INPUT_BUTTON_3 = culng(1u shl 3)
const SDL_PEN_INPUT_BUTTON_4 = culng(1u shl 4)
const SDL_PEN_INPUT_BUTTON_5 = culng(1u shl 5)
const SDL_PEN_INPUT_ERASER_TIP = culng(1u shl 30)
const SDL_PEN_INPUT_IN_PROXIMITY = culng(1u shl 31)

type SDL_PenAxis as long
enum
	SDL_PEN_AXIS_PRESSURE
	SDL_PEN_AXIS_XTILT
	SDL_PEN_AXIS_YTILT
	SDL_PEN_AXIS_DISTANCE
	SDL_PEN_AXIS_ROTATION
	SDL_PEN_AXIS_SLIDER
	SDL_PEN_AXIS_TANGENTIAL_PRESSURE
	SDL_PEN_AXIS_COUNT
end enum

type SDL_PenDeviceType as long
enum
	SDL_PEN_DEVICE_TYPE_INVALID = -1
	SDL_PEN_DEVICE_TYPE_UNKNOWN
	SDL_PEN_DEVICE_TYPE_DIRECT
	SDL_PEN_DEVICE_TYPE_INDIRECT
end enum

declare function SDL_GetPenDeviceType(byval instance_id as SDL_PenID) as SDL_PenDeviceType

type SDL_EventType as long
enum
	SDL_EVENT_FIRST = 0
	SDL_EVENT_QUIT = &h100
	SDL_EVENT_TERMINATING
	SDL_EVENT_LOW_MEMORY
	SDL_EVENT_WILL_ENTER_BACKGROUND
	SDL_EVENT_DID_ENTER_BACKGROUND
	SDL_EVENT_WILL_ENTER_FOREGROUND
	SDL_EVENT_DID_ENTER_FOREGROUND
	SDL_EVENT_LOCALE_CHANGED
	SDL_EVENT_SYSTEM_THEME_CHANGED
	SDL_EVENT_DISPLAY_ORIENTATION = &h151
	SDL_EVENT_DISPLAY_ADDED
	SDL_EVENT_DISPLAY_REMOVED
	SDL_EVENT_DISPLAY_MOVED
	SDL_EVENT_DISPLAY_DESKTOP_MODE_CHANGED
	SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED
	SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED
	SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED
	SDL_EVENT_DISPLAY_FIRST = SDL_EVENT_DISPLAY_ORIENTATION
	SDL_EVENT_DISPLAY_LAST = SDL_EVENT_DISPLAY_USABLE_BOUNDS_CHANGED
	SDL_EVENT_WINDOW_SHOWN = &h202
	SDL_EVENT_WINDOW_HIDDEN
	SDL_EVENT_WINDOW_EXPOSED
	SDL_EVENT_WINDOW_MOVED
	SDL_EVENT_WINDOW_RESIZED
	SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
	SDL_EVENT_WINDOW_METAL_VIEW_RESIZED
	SDL_EVENT_WINDOW_MINIMIZED
	SDL_EVENT_WINDOW_MAXIMIZED
	SDL_EVENT_WINDOW_RESTORED
	SDL_EVENT_WINDOW_MOUSE_ENTER
	SDL_EVENT_WINDOW_MOUSE_LEAVE
	SDL_EVENT_WINDOW_FOCUS_GAINED
	SDL_EVENT_WINDOW_FOCUS_LOST
	SDL_EVENT_WINDOW_CLOSE_REQUESTED
	SDL_EVENT_WINDOW_HIT_TEST
	SDL_EVENT_WINDOW_ICCPROF_CHANGED
	SDL_EVENT_WINDOW_DISPLAY_CHANGED
	SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED
	SDL_EVENT_WINDOW_SAFE_AREA_CHANGED
	SDL_EVENT_WINDOW_OCCLUDED
	SDL_EVENT_WINDOW_ENTER_FULLSCREEN
	SDL_EVENT_WINDOW_LEAVE_FULLSCREEN
	SDL_EVENT_WINDOW_DESTROYED
	SDL_EVENT_WINDOW_HDR_STATE_CHANGED
	SDL_EVENT_WINDOW_FIRST = SDL_EVENT_WINDOW_SHOWN
	SDL_EVENT_WINDOW_LAST = SDL_EVENT_WINDOW_HDR_STATE_CHANGED
	SDL_EVENT_KEY_DOWN = &h300
	SDL_EVENT_KEY_UP
	SDL_EVENT_TEXT_EDITING
	SDL_EVENT_TEXT_INPUT
	SDL_EVENT_KEYMAP_CHANGED
	SDL_EVENT_KEYBOARD_ADDED
	SDL_EVENT_KEYBOARD_REMOVED
	SDL_EVENT_TEXT_EDITING_CANDIDATES
	SDL_EVENT_SCREEN_KEYBOARD_SHOWN
	SDL_EVENT_SCREEN_KEYBOARD_HIDDEN
	SDL_EVENT_MOUSE_MOTION = &h400
	SDL_EVENT_MOUSE_BUTTON_DOWN
	SDL_EVENT_MOUSE_BUTTON_UP
	SDL_EVENT_MOUSE_WHEEL
	SDL_EVENT_MOUSE_ADDED
	SDL_EVENT_MOUSE_REMOVED
	SDL_EVENT_JOYSTICK_AXIS_MOTION = &h600
	SDL_EVENT_JOYSTICK_BALL_MOTION
	SDL_EVENT_JOYSTICK_HAT_MOTION
	SDL_EVENT_JOYSTICK_BUTTON_DOWN
	SDL_EVENT_JOYSTICK_BUTTON_UP
	SDL_EVENT_JOYSTICK_ADDED
	SDL_EVENT_JOYSTICK_REMOVED
	SDL_EVENT_JOYSTICK_BATTERY_UPDATED
	SDL_EVENT_JOYSTICK_UPDATE_COMPLETE
	SDL_EVENT_GAMEPAD_AXIS_MOTION = &h650
	SDL_EVENT_GAMEPAD_BUTTON_DOWN
	SDL_EVENT_GAMEPAD_BUTTON_UP
	SDL_EVENT_GAMEPAD_ADDED
	SDL_EVENT_GAMEPAD_REMOVED
	SDL_EVENT_GAMEPAD_REMAPPED
	SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN
	SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION
	SDL_EVENT_GAMEPAD_TOUCHPAD_UP
	SDL_EVENT_GAMEPAD_SENSOR_UPDATE
	SDL_EVENT_GAMEPAD_UPDATE_COMPLETE
	SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED
	SDL_EVENT_FINGER_DOWN = &h700
	SDL_EVENT_FINGER_UP
	SDL_EVENT_FINGER_MOTION
	SDL_EVENT_FINGER_CANCELED
	SDL_EVENT_PINCH_BEGIN = &h710
	SDL_EVENT_PINCH_UPDATE
	SDL_EVENT_PINCH_END
	SDL_EVENT_CLIPBOARD_UPDATE = &h900
	SDL_EVENT_DROP_FILE = &h1000
	SDL_EVENT_DROP_TEXT
	SDL_EVENT_DROP_BEGIN
	SDL_EVENT_DROP_COMPLETE
	SDL_EVENT_DROP_POSITION
	SDL_EVENT_AUDIO_DEVICE_ADDED = &h1100
	SDL_EVENT_AUDIO_DEVICE_REMOVED
	SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED
	SDL_EVENT_SENSOR_UPDATE = &h1200
	SDL_EVENT_PEN_PROXIMITY_IN = &h1300
	SDL_EVENT_PEN_PROXIMITY_OUT
	SDL_EVENT_PEN_DOWN
	SDL_EVENT_PEN_UP
	SDL_EVENT_PEN_BUTTON_DOWN
	SDL_EVENT_PEN_BUTTON_UP
	SDL_EVENT_PEN_MOTION
	SDL_EVENT_PEN_AXIS
	SDL_EVENT_CAMERA_DEVICE_ADDED = &h1400
	SDL_EVENT_CAMERA_DEVICE_REMOVED
	SDL_EVENT_CAMERA_DEVICE_APPROVED
	SDL_EVENT_CAMERA_DEVICE_DENIED
	SDL_EVENT_RENDER_TARGETS_RESET = &h2000
	SDL_EVENT_RENDER_DEVICE_RESET
	SDL_EVENT_RENDER_DEVICE_LOST
	SDL_EVENT_PRIVATE0 = &h4000
	SDL_EVENT_PRIVATE1
	SDL_EVENT_PRIVATE2
	SDL_EVENT_PRIVATE3
	SDL_EVENT_POLL_SENTINEL = &h7F00
	SDL_EVENT_USER = &h8000
	SDL_EVENT_LAST = &hFFFF
	SDL_EVENT_ENUM_PADDING = &h7FFFFFFF
end enum

type SDL_CommonEvent
	as Uint32 type
	reserved as Uint32
	timestamp as Uint64
end type

type SDL_DisplayEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	displayID as SDL_DisplayID
	data1 as Sint32
	data2 as Sint32
end type

type SDL_WindowEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	data1 as Sint32
	data2 as Sint32
end type

type SDL_KeyboardDeviceEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_KeyboardID
end type

type SDL_KeyboardEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_KeyboardID
	scancode as SDL_Scancode
	key as SDL_Keycode
	mod_ as SDL_Keymod
	raw as Uint16
	down as boolean
	repeat as boolean
end type

type SDL_TextEditingEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	text as const zstring ptr
	start as Sint32
	length as Sint32
end type

type SDL_TextEditingCandidatesEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	candidates as const zstring const ptr ptr
	num_candidates as Sint32
	selected_candidate as Sint32
	horizontal as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_TextInputEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	text as const zstring ptr
end type

type SDL_MouseDeviceEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_MouseID
end type

type SDL_MouseMotionEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_MouseID
	state as SDL_MouseButtonFlags
	x as single
	y as single
	xrel as single
	yrel as single
end type

type SDL_MouseButtonEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_MouseID
	button as Uint8
	down as boolean
	clicks as Uint8
	padding as Uint8
	x as single
	y as single
end type

type SDL_MouseWheelEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_MouseID
	x as single
	y as single
	direction as SDL_MouseWheelDirection
	mouse_x as single
	mouse_y as single
	integer_x as Sint32
	integer_y as Sint32
end type

type SDL_JoyAxisEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	axis as Uint8
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
	value as Sint16
	padding4 as Uint16
end type

type SDL_JoyBallEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	ball as Uint8
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
	xrel as Sint16
	yrel as Sint16
end type

type SDL_JoyHatEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	hat as Uint8
	value as Uint8
	padding1 as Uint8
	padding2 as Uint8
end type

type SDL_JoyButtonEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	button as Uint8
	down as boolean
	padding1 as Uint8
	padding2 as Uint8
end type

type SDL_JoyDeviceEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
end type

type SDL_JoyBatteryEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	state as SDL_PowerState
	percent as long
end type

type SDL_GamepadAxisEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	axis as Uint8
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
	value as Sint16
	padding4 as Uint16
end type

type SDL_GamepadButtonEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	button as Uint8
	down as boolean
	padding1 as Uint8
	padding2 as Uint8
end type

type SDL_GamepadDeviceEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
end type

type SDL_GamepadTouchpadEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	touchpad as Sint32
	finger as Sint32
	x as single
	y as single
	pressure as single
end type

type SDL_GamepadSensorEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_JoystickID
	sensor as Sint32
	data(0 to 2) as single
	sensor_timestamp as Uint64
end type

type SDL_AudioDeviceEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_AudioDeviceID
	recording as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_CameraDeviceEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_CameraID
end type

type SDL_RenderEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
end type

type SDL_TouchFingerEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	touchID as SDL_TouchID
	fingerID as SDL_FingerID
	x as single
	y as single
	dx as single
	dy as single
	pressure as single
	windowID as SDL_WindowID
end type

type SDL_PinchFingerEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	scale as single
	windowID as SDL_WindowID
end type

type SDL_PenProximityEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_PenID
	pen_state as SDL_PenInputFlags
	device_type as SDL_PenDeviceType
end type

type SDL_PenMotionEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_PenID
	pen_state as SDL_PenInputFlags
	x as single
	y as single
	device_type as SDL_PenDeviceType
end type

type SDL_PenTouchEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_PenID
	pen_state as SDL_PenInputFlags
	x as single
	y as single
	eraser as boolean
	down as boolean
	device_type as SDL_PenDeviceType
end type

type SDL_PenButtonEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_PenID
	pen_state as SDL_PenInputFlags
	x as single
	y as single
	button as Uint8
	down as boolean
	device_type as SDL_PenDeviceType
end type

type SDL_PenAxisEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	which as SDL_PenID
	pen_state as SDL_PenInputFlags
	x as single
	y as single
	axis as SDL_PenAxis
	value as single
	device_type as SDL_PenDeviceType
end type

type SDL_DropEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	x as single
	y as single
	source as const zstring ptr
	data as const zstring ptr
end type

type SDL_ClipboardEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	owner as boolean
	num_mime_types as Sint32
	mime_types as const zstring ptr ptr
end type

type SDL_SensorEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
	which as SDL_SensorID
	data(0 to 5) as single
	sensor_timestamp as Uint64
end type

type SDL_QuitEvent
	as SDL_EventType type
	reserved as Uint32
	timestamp as Uint64
end type

type SDL_UserEvent
	as Uint32 type
	reserved as Uint32
	timestamp as Uint64
	windowID as SDL_WindowID
	code as Sint32
	data1 as any ptr
	data2 as any ptr
end type

union SDL_Event
	as Uint32 type
	common as SDL_CommonEvent
	display as SDL_DisplayEvent
	window as SDL_WindowEvent
	kdevice as SDL_KeyboardDeviceEvent
	key as SDL_KeyboardEvent
	edit as SDL_TextEditingEvent
	edit_candidates as SDL_TextEditingCandidatesEvent
	text as SDL_TextInputEvent
	mdevice as SDL_MouseDeviceEvent
	motion as SDL_MouseMotionEvent
	button as SDL_MouseButtonEvent
	wheel as SDL_MouseWheelEvent
	jdevice as SDL_JoyDeviceEvent
	jaxis as SDL_JoyAxisEvent
	jball as SDL_JoyBallEvent
	jhat as SDL_JoyHatEvent
	jbutton as SDL_JoyButtonEvent
	jbattery as SDL_JoyBatteryEvent
	gdevice as SDL_GamepadDeviceEvent
	gaxis as SDL_GamepadAxisEvent
	gbutton as SDL_GamepadButtonEvent
	gtouchpad as SDL_GamepadTouchpadEvent
	gsensor as SDL_GamepadSensorEvent
	adevice as SDL_AudioDeviceEvent
	cdevice as SDL_CameraDeviceEvent
	sensor as SDL_SensorEvent
	quit as SDL_QuitEvent
	user as SDL_UserEvent
	tfinger as SDL_TouchFingerEvent
	pinch as SDL_PinchFingerEvent
	pproximity as SDL_PenProximityEvent
	ptouch as SDL_PenTouchEvent
	pmotion as SDL_PenMotionEvent
	pbutton as SDL_PenButtonEvent
	paxis as SDL_PenAxisEvent
	render as SDL_RenderEvent
	drop as SDL_DropEvent
	clipboard as SDL_ClipboardEvent
	padding(0 to 127) as Uint8
end union

declare sub SDL_PumpEvents()

type SDL_EventAction as long
enum
	SDL_ADDEVENT
	SDL_PEEKEVENT
	SDL_GETEVENT
end enum

declare function SDL_PeepEvents(byval events as SDL_Event ptr, byval numevents as long, byval action as SDL_EventAction, byval minType as Uint32, byval maxType as Uint32) as long
declare function SDL_HasEvent(byval type as Uint32) as boolean
declare function SDL_HasEvents(byval minType as Uint32, byval maxType as Uint32) as boolean
declare sub SDL_FlushEvent(byval type as Uint32)
declare sub SDL_FlushEvents(byval minType as Uint32, byval maxType as Uint32)
declare function SDL_PollEvent(byval event as SDL_Event ptr) as boolean
declare function SDL_WaitEvent(byval event as SDL_Event ptr) as boolean
declare function SDL_WaitEventTimeout(byval event as SDL_Event ptr, byval timeoutMS as Sint32) as boolean
declare function SDL_PushEvent(byval event as SDL_Event ptr) as boolean
type SDL_EventFilter as function(byval userdata as any ptr, byval event as SDL_Event ptr) as boolean
declare sub SDL_SetEventFilter(byval filter as SDL_EventFilter, byval userdata as any ptr)
declare function SDL_GetEventFilter(byval filter as SDL_EventFilter ptr, byval userdata as any ptr ptr) as boolean
declare function SDL_AddEventWatch(byval filter as SDL_EventFilter, byval userdata as any ptr) as boolean
declare sub SDL_RemoveEventWatch(byval filter as SDL_EventFilter, byval userdata as any ptr)
declare sub SDL_FilterEvents(byval filter as SDL_EventFilter, byval userdata as any ptr)
declare sub SDL_SetEventEnabled(byval type as Uint32, byval enabled as boolean)
declare function SDL_EventEnabled(byval type as Uint32) as boolean
declare function SDL_RegisterEvents(byval numevents as long) as Uint32
declare function SDL_GetWindowFromEvent(byval event as const SDL_Event ptr) as SDL_Window ptr
declare function SDL_GetEventDescription(byval event as const SDL_Event ptr, byval buf as zstring ptr, byval buflen as long) as long
'' -------------------------------------------------------------------------
'' SDL_filesystem.h
'' -------------------------------------------------------------------------
declare function SDL_GetBasePath() as const zstring ptr
declare function SDL_GetPrefPath(byval org as const zstring ptr, byval app as const zstring ptr) as zstring ptr

type SDL_Folder as long
enum
	SDL_FOLDER_HOME
	SDL_FOLDER_DESKTOP
	SDL_FOLDER_DOCUMENTS
	SDL_FOLDER_DOWNLOADS
	SDL_FOLDER_MUSIC
	SDL_FOLDER_PICTURES
	SDL_FOLDER_PUBLICSHARE
	SDL_FOLDER_SAVEDGAMES
	SDL_FOLDER_SCREENSHOTS
	SDL_FOLDER_TEMPLATES
	SDL_FOLDER_VIDEOS
	SDL_FOLDER_COUNT
end enum

declare function SDL_GetUserFolder(byval folder as SDL_Folder) as const zstring ptr

type SDL_PathType as long
enum
	SDL_PATHTYPE_NONE
	SDL_PATHTYPE_FILE
	SDL_PATHTYPE_DIRECTORY
	SDL_PATHTYPE_OTHER
end enum

type SDL_PathInfo
	as SDL_PathType type
	size as Uint64
	create_time as SDL_Time
	modify_time as SDL_Time
	access_time as SDL_Time
end type

type SDL_GlobFlags as Uint32
const SDL_GLOB_CASEINSENSITIVE = culng(1u shl 0)
declare function SDL_CreateDirectory(byval path as const zstring ptr) as boolean

type SDL_EnumerationResult as long
enum
	SDL_ENUM_CONTINUE
	SDL_ENUM_SUCCESS
	SDL_ENUM_FAILURE
end enum

type SDL_EnumerateDirectoryCallback as function(byval userdata as any ptr, byval dirname as const zstring ptr, byval fname as const zstring ptr) as SDL_EnumerationResult
declare function SDL_EnumerateDirectory(byval path as const zstring ptr, byval callback as SDL_EnumerateDirectoryCallback, byval userdata as any ptr) as boolean
declare function SDL_RemovePath(byval path as const zstring ptr) as boolean
declare function SDL_RenamePath(byval oldpath as const zstring ptr, byval newpath as const zstring ptr) as boolean
declare function SDL_CopyFile(byval oldpath as const zstring ptr, byval newpath as const zstring ptr) as boolean
declare function SDL_GetPathInfo(byval path as const zstring ptr, byval info as SDL_PathInfo ptr) as boolean
declare function SDL_GlobDirectory(byval path as const zstring ptr, byval pattern as const zstring ptr, byval flags as SDL_GlobFlags, byval count as long ptr) as zstring ptr ptr
declare function SDL_GetCurrentDirectory() as zstring ptr
'' -------------------------------------------------------------------------
'' SDL_gpu.h
'' -------------------------------------------------------------------------
type SDL_GPUPrimitiveType as long
enum
	SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
	SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP
	SDL_GPU_PRIMITIVETYPE_LINELIST
	SDL_GPU_PRIMITIVETYPE_LINESTRIP
	SDL_GPU_PRIMITIVETYPE_POINTLIST
end enum

type SDL_GPULoadOp as long
enum
	SDL_GPU_LOADOP_LOAD
	SDL_GPU_LOADOP_CLEAR
	SDL_GPU_LOADOP_DONT_CARE
end enum

type SDL_GPUStoreOp as long
enum
	SDL_GPU_STOREOP_STORE
	SDL_GPU_STOREOP_DONT_CARE
	SDL_GPU_STOREOP_RESOLVE
	SDL_GPU_STOREOP_RESOLVE_AND_STORE
end enum

type SDL_GPUIndexElementSize as long
enum
	SDL_GPU_INDEXELEMENTSIZE_16BIT
	SDL_GPU_INDEXELEMENTSIZE_32BIT
end enum

type SDL_GPUTextureFormat as long
enum
	SDL_GPU_TEXTUREFORMAT_INVALID
	SDL_GPU_TEXTUREFORMAT_A8_UNORM
	SDL_GPU_TEXTUREFORMAT_R8_UNORM
	SDL_GPU_TEXTUREFORMAT_R8G8_UNORM
	SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM
	SDL_GPU_TEXTUREFORMAT_R16_UNORM
	SDL_GPU_TEXTUREFORMAT_R16G16_UNORM
	SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UNORM
	SDL_GPU_TEXTUREFORMAT_R10G10B10A2_UNORM
	SDL_GPU_TEXTUREFORMAT_B5G6R5_UNORM
	SDL_GPU_TEXTUREFORMAT_B5G5R5A1_UNORM
	SDL_GPU_TEXTUREFORMAT_B4G4R4A4_UNORM
	SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM
	SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM
	SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM
	SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM
	SDL_GPU_TEXTUREFORMAT_BC4_R_UNORM
	SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM
	SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM
	SDL_GPU_TEXTUREFORMAT_BC6H_RGB_FLOAT
	SDL_GPU_TEXTUREFORMAT_BC6H_RGB_UFLOAT
	SDL_GPU_TEXTUREFORMAT_R8_SNORM
	SDL_GPU_TEXTUREFORMAT_R8G8_SNORM
	SDL_GPU_TEXTUREFORMAT_R8G8B8A8_SNORM
	SDL_GPU_TEXTUREFORMAT_R16_SNORM
	SDL_GPU_TEXTUREFORMAT_R16G16_SNORM
	SDL_GPU_TEXTUREFORMAT_R16G16B16A16_SNORM
	SDL_GPU_TEXTUREFORMAT_R16_FLOAT
	SDL_GPU_TEXTUREFORMAT_R16G16_FLOAT
	SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT
	SDL_GPU_TEXTUREFORMAT_R32_FLOAT
	SDL_GPU_TEXTUREFORMAT_R32G32_FLOAT
	SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT
	SDL_GPU_TEXTUREFORMAT_R11G11B10_UFLOAT
	SDL_GPU_TEXTUREFORMAT_R8_UINT
	SDL_GPU_TEXTUREFORMAT_R8G8_UINT
	SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UINT
	SDL_GPU_TEXTUREFORMAT_R16_UINT
	SDL_GPU_TEXTUREFORMAT_R16G16_UINT
	SDL_GPU_TEXTUREFORMAT_R16G16B16A16_UINT
	SDL_GPU_TEXTUREFORMAT_R32_UINT
	SDL_GPU_TEXTUREFORMAT_R32G32_UINT
	SDL_GPU_TEXTUREFORMAT_R32G32B32A32_UINT
	SDL_GPU_TEXTUREFORMAT_R8_INT
	SDL_GPU_TEXTUREFORMAT_R8G8_INT
	SDL_GPU_TEXTUREFORMAT_R8G8B8A8_INT
	SDL_GPU_TEXTUREFORMAT_R16_INT
	SDL_GPU_TEXTUREFORMAT_R16G16_INT
	SDL_GPU_TEXTUREFORMAT_R16G16B16A16_INT
	SDL_GPU_TEXTUREFORMAT_R32_INT
	SDL_GPU_TEXTUREFORMAT_R32G32_INT
	SDL_GPU_TEXTUREFORMAT_R32G32B32A32_INT
	SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_BC1_RGBA_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_BC2_RGBA_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_D16_UNORM
	SDL_GPU_TEXTUREFORMAT_D24_UNORM
	SDL_GPU_TEXTUREFORMAT_D32_FLOAT
	SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT
	SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT
	SDL_GPU_TEXTUREFORMAT_ASTC_4x4_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_5x4_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_5x5_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_6x5_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_6x6_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_8x5_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_8x6_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_8x8_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_10x5_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_10x6_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_10x8_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_10x10_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_12x10_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_12x12_UNORM
	SDL_GPU_TEXTUREFORMAT_ASTC_4x4_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_5x4_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_5x5_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_6x5_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_6x6_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_8x5_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_8x6_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_8x8_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_10x5_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_10x6_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_10x8_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_10x10_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_12x10_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_12x12_UNORM_SRGB
	SDL_GPU_TEXTUREFORMAT_ASTC_4x4_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_5x4_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_5x5_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_6x5_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_6x6_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_8x5_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_8x6_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_8x8_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_10x5_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_10x6_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_10x8_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_10x10_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_12x10_FLOAT
	SDL_GPU_TEXTUREFORMAT_ASTC_12x12_FLOAT
end enum

type SDL_GPUTextureUsageFlags as Uint32
const SDL_GPU_TEXTUREUSAGE_SAMPLER = culng(1u shl 0)
const SDL_GPU_TEXTUREUSAGE_COLOR_TARGET = culng(1u shl 1)
const SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET = culng(1u shl 2)
const SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ = culng(1u shl 3)
const SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ = culng(1u shl 4)
const SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE = culng(1u shl 5)
const SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE = culng(1u shl 6)

type SDL_GPUTextureType as long
enum
	SDL_GPU_TEXTURETYPE_2D
	SDL_GPU_TEXTURETYPE_2D_ARRAY
	SDL_GPU_TEXTURETYPE_3D
	SDL_GPU_TEXTURETYPE_CUBE
	SDL_GPU_TEXTURETYPE_CUBE_ARRAY
end enum

type SDL_GPUSampleCount as long
enum
	SDL_GPU_SAMPLECOUNT_1
	SDL_GPU_SAMPLECOUNT_2
	SDL_GPU_SAMPLECOUNT_4
	SDL_GPU_SAMPLECOUNT_8
end enum

type SDL_GPUCubeMapFace as long
enum
	SDL_GPU_CUBEMAPFACE_POSITIVEX
	SDL_GPU_CUBEMAPFACE_NEGATIVEX
	SDL_GPU_CUBEMAPFACE_POSITIVEY
	SDL_GPU_CUBEMAPFACE_NEGATIVEY
	SDL_GPU_CUBEMAPFACE_POSITIVEZ
	SDL_GPU_CUBEMAPFACE_NEGATIVEZ
end enum

type SDL_GPUBufferUsageFlags as Uint32
const SDL_GPU_BUFFERUSAGE_VERTEX = culng(1u shl 0)
const SDL_GPU_BUFFERUSAGE_INDEX = culng(1u shl 1)
const SDL_GPU_BUFFERUSAGE_INDIRECT = culng(1u shl 2)
const SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ = culng(1u shl 3)
const SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ = culng(1u shl 4)
const SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE = culng(1u shl 5)

type SDL_GPUTransferBufferUsage as long
enum
	SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD
	SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD
end enum

type SDL_GPUShaderStage as long
enum
	SDL_GPU_SHADERSTAGE_VERTEX
	SDL_GPU_SHADERSTAGE_FRAGMENT
end enum

type SDL_GPUShaderFormat as Uint32
const SDL_GPU_SHADERFORMAT_INVALID = 0
const SDL_GPU_SHADERFORMAT_PRIVATE = culng(1u shl 0)
const SDL_GPU_SHADERFORMAT_SPIRV = culng(1u shl 1)
const SDL_GPU_SHADERFORMAT_DXBC = culng(1u shl 2)
const SDL_GPU_SHADERFORMAT_DXIL = culng(1u shl 3)
const SDL_GPU_SHADERFORMAT_MSL = culng(1u shl 4)
const SDL_GPU_SHADERFORMAT_METALLIB = culng(1u shl 5)

type SDL_GPUVertexElementFormat as long
enum
	SDL_GPU_VERTEXELEMENTFORMAT_INVALID
	SDL_GPU_VERTEXELEMENTFORMAT_INT
	SDL_GPU_VERTEXELEMENTFORMAT_INT2
	SDL_GPU_VERTEXELEMENTFORMAT_INT3
	SDL_GPU_VERTEXELEMENTFORMAT_INT4
	SDL_GPU_VERTEXELEMENTFORMAT_UINT
	SDL_GPU_VERTEXELEMENTFORMAT_UINT2
	SDL_GPU_VERTEXELEMENTFORMAT_UINT3
	SDL_GPU_VERTEXELEMENTFORMAT_UINT4
	SDL_GPU_VERTEXELEMENTFORMAT_FLOAT
	SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2
	SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3
	SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4
	SDL_GPU_VERTEXELEMENTFORMAT_BYTE2
	SDL_GPU_VERTEXELEMENTFORMAT_BYTE4
	SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2
	SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4
	SDL_GPU_VERTEXELEMENTFORMAT_BYTE2_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_BYTE4_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_UBYTE2_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_SHORT2
	SDL_GPU_VERTEXELEMENTFORMAT_SHORT4
	SDL_GPU_VERTEXELEMENTFORMAT_USHORT2
	SDL_GPU_VERTEXELEMENTFORMAT_USHORT4
	SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM
	SDL_GPU_VERTEXELEMENTFORMAT_HALF2
	SDL_GPU_VERTEXELEMENTFORMAT_HALF4
end enum

type SDL_GPUVertexInputRate as long
enum
	SDL_GPU_VERTEXINPUTRATE_VERTEX
	SDL_GPU_VERTEXINPUTRATE_INSTANCE
end enum

type SDL_GPUFillMode as long
enum
	SDL_GPU_FILLMODE_FILL
	SDL_GPU_FILLMODE_LINE
end enum

type SDL_GPUCullMode as long
enum
	SDL_GPU_CULLMODE_NONE
	SDL_GPU_CULLMODE_FRONT
	SDL_GPU_CULLMODE_BACK
end enum

type SDL_GPUFrontFace as long
enum
	SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
	SDL_GPU_FRONTFACE_CLOCKWISE
end enum

type SDL_GPUCompareOp as long
enum
	SDL_GPU_COMPAREOP_INVALID
	SDL_GPU_COMPAREOP_NEVER
	SDL_GPU_COMPAREOP_LESS
	SDL_GPU_COMPAREOP_EQUAL
	SDL_GPU_COMPAREOP_LESS_OR_EQUAL
	SDL_GPU_COMPAREOP_GREATER
	SDL_GPU_COMPAREOP_NOT_EQUAL
	SDL_GPU_COMPAREOP_GREATER_OR_EQUAL
	SDL_GPU_COMPAREOP_ALWAYS
end enum

type SDL_GPUStencilOp as long
enum
	SDL_GPU_STENCILOP_INVALID
	SDL_GPU_STENCILOP_KEEP
	SDL_GPU_STENCILOP_ZERO
	SDL_GPU_STENCILOP_REPLACE
	SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP
	SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP
	SDL_GPU_STENCILOP_INVERT
	SDL_GPU_STENCILOP_INCREMENT_AND_WRAP
	SDL_GPU_STENCILOP_DECREMENT_AND_WRAP
end enum

type SDL_GPUBlendOp as long
enum
	SDL_GPU_BLENDOP_INVALID
	SDL_GPU_BLENDOP_ADD
	SDL_GPU_BLENDOP_SUBTRACT
	SDL_GPU_BLENDOP_REVERSE_SUBTRACT
	SDL_GPU_BLENDOP_MIN
	SDL_GPU_BLENDOP_MAX
end enum

type SDL_GPUBlendFactor as long
enum
	SDL_GPU_BLENDFACTOR_INVALID
	SDL_GPU_BLENDFACTOR_ZERO
	SDL_GPU_BLENDFACTOR_ONE
	SDL_GPU_BLENDFACTOR_SRC_COLOR
	SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR
	SDL_GPU_BLENDFACTOR_DST_COLOR
	SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR
	SDL_GPU_BLENDFACTOR_SRC_ALPHA
	SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
	SDL_GPU_BLENDFACTOR_DST_ALPHA
	SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA
	SDL_GPU_BLENDFACTOR_CONSTANT_COLOR
	SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR
	SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE
end enum

type SDL_GPUColorComponentFlags as Uint8
const SDL_GPU_COLORCOMPONENT_R = culng(1u shl 0)
const SDL_GPU_COLORCOMPONENT_G = culng(1u shl 1)
const SDL_GPU_COLORCOMPONENT_B = culng(1u shl 2)
const SDL_GPU_COLORCOMPONENT_A = culng(1u shl 3)

type SDL_GPUFilter as long
enum
	SDL_GPU_FILTER_NEAREST
	SDL_GPU_FILTER_LINEAR
end enum

type SDL_GPUSamplerMipmapMode as long
enum
	SDL_GPU_SAMPLERMIPMAPMODE_NEAREST
	SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
end enum

type SDL_GPUSamplerAddressMode as long
enum
	SDL_GPU_SAMPLERADDRESSMODE_REPEAT
	SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT
	SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
end enum

type SDL_GPUPresentMode as long
enum
	SDL_GPU_PRESENTMODE_VSYNC
	SDL_GPU_PRESENTMODE_IMMEDIATE
	SDL_GPU_PRESENTMODE_MAILBOX
end enum

type SDL_GPUSwapchainComposition as long
enum
	SDL_GPU_SWAPCHAINCOMPOSITION_SDR
	SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR
	SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR
	SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084
end enum

type SDL_GPUViewport
	x as single
	y as single
	w as single
	h as single
	min_depth as single
	max_depth as single
end type

type SDL_GPUTextureTransferInfo
	transfer_buffer as SDL_GPUTransferBuffer ptr
	offset as Uint32
	pixels_per_row as Uint32
	rows_per_layer as Uint32
end type

type SDL_GPUTransferBufferLocation
	transfer_buffer as SDL_GPUTransferBuffer ptr
	offset as Uint32
end type

type SDL_GPUTextureLocation
	texture as SDL_GPUTexture ptr
	mip_level as Uint32
	layer as Uint32
	x as Uint32
	y as Uint32
	z as Uint32
end type

type SDL_GPUTextureRegion
	texture as SDL_GPUTexture ptr
	mip_level as Uint32
	layer as Uint32
	x as Uint32
	y as Uint32
	z as Uint32
	w as Uint32
	h as Uint32
	d as Uint32
end type

type SDL_GPUBlitRegion
	texture as SDL_GPUTexture ptr
	mip_level as Uint32
	layer_or_depth_plane as Uint32
	x as Uint32
	y as Uint32
	w as Uint32
	h as Uint32
end type

type SDL_GPUBufferLocation
	buffer as SDL_GPUBuffer ptr
	offset as Uint32
end type

type SDL_GPUBufferRegion
	buffer as SDL_GPUBuffer ptr
	offset as Uint32
	size as Uint32
end type

type SDL_GPUIndirectDrawCommand
	num_vertices as Uint32
	num_instances as Uint32
	first_vertex as Uint32
	first_instance as Uint32
end type

type SDL_GPUIndexedIndirectDrawCommand
	num_indices as Uint32
	num_instances as Uint32
	first_index as Uint32
	vertex_offset as Sint32
	first_instance as Uint32
end type

type SDL_GPUIndirectDispatchCommand
	groupcount_x as Uint32
	groupcount_y as Uint32
	groupcount_z as Uint32
end type

type SDL_GPUSamplerCreateInfo
	min_filter as SDL_GPUFilter
	mag_filter as SDL_GPUFilter
	mipmap_mode as SDL_GPUSamplerMipmapMode
	address_mode_u as SDL_GPUSamplerAddressMode
	address_mode_v as SDL_GPUSamplerAddressMode
	address_mode_w as SDL_GPUSamplerAddressMode
	mip_lod_bias as single
	max_anisotropy as single
	compare_op as SDL_GPUCompareOp
	min_lod as single
	max_lod as single
	enable_anisotropy as boolean
	enable_compare as boolean
	padding1 as Uint8
	padding2 as Uint8
	props as SDL_PropertiesID
end type

type SDL_GPUVertexBufferDescription
	slot as Uint32
	pitch as Uint32
	input_rate as SDL_GPUVertexInputRate
	instance_step_rate as Uint32
end type

type SDL_GPUVertexAttribute
	location as Uint32
	buffer_slot as Uint32
	format as SDL_GPUVertexElementFormat
	offset as Uint32
end type

type SDL_GPUVertexInputState
	vertex_buffer_descriptions as const SDL_GPUVertexBufferDescription ptr
	num_vertex_buffers as Uint32
	vertex_attributes as const SDL_GPUVertexAttribute ptr
	num_vertex_attributes as Uint32
end type

type SDL_GPUStencilOpState
	fail_op as SDL_GPUStencilOp
	pass_op as SDL_GPUStencilOp
	depth_fail_op as SDL_GPUStencilOp
	compare_op as SDL_GPUCompareOp
end type

type SDL_GPUColorTargetBlendState
	src_color_blendfactor as SDL_GPUBlendFactor
	dst_color_blendfactor as SDL_GPUBlendFactor
	color_blend_op as SDL_GPUBlendOp
	src_alpha_blendfactor as SDL_GPUBlendFactor
	dst_alpha_blendfactor as SDL_GPUBlendFactor
	alpha_blend_op as SDL_GPUBlendOp
	color_write_mask as SDL_GPUColorComponentFlags
	enable_blend as boolean
	enable_color_write_mask as boolean
	padding1 as Uint8
	padding2 as Uint8
end type

type SDL_GPUShaderCreateInfo
	code_size as uinteger
	code as const Uint8 ptr
	entrypoint as const zstring ptr
	format as SDL_GPUShaderFormat
	stage as SDL_GPUShaderStage
	num_samplers as Uint32
	num_storage_textures as Uint32
	num_storage_buffers as Uint32
	num_uniform_buffers as Uint32
	props as SDL_PropertiesID
end type

type SDL_GPUTextureCreateInfo
	as SDL_GPUTextureType type
	format as SDL_GPUTextureFormat
	usage as SDL_GPUTextureUsageFlags
	width as Uint32
	height as Uint32
	layer_count_or_depth as Uint32
	num_levels as Uint32
	sample_count as SDL_GPUSampleCount
	props as SDL_PropertiesID
end type

type SDL_GPUBufferCreateInfo
	usage as SDL_GPUBufferUsageFlags
	size as Uint32
	props as SDL_PropertiesID
end type

type SDL_GPUTransferBufferCreateInfo
	usage as SDL_GPUTransferBufferUsage
	size as Uint32
	props as SDL_PropertiesID
end type

type SDL_GPURasterizerState
	fill_mode as SDL_GPUFillMode
	cull_mode as SDL_GPUCullMode
	front_face as SDL_GPUFrontFace
	depth_bias_constant_factor as single
	depth_bias_clamp as single
	depth_bias_slope_factor as single
	enable_depth_bias as boolean
	enable_depth_clip as boolean
	padding1 as Uint8
	padding2 as Uint8
end type

type SDL_GPUMultisampleState
	sample_count as SDL_GPUSampleCount
	sample_mask as Uint32
	enable_mask as boolean
	enable_alpha_to_coverage as boolean
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_GPUDepthStencilState
	compare_op as SDL_GPUCompareOp
	back_stencil_state as SDL_GPUStencilOpState
	front_stencil_state as SDL_GPUStencilOpState
	compare_mask as Uint8
	write_mask as Uint8
	enable_depth_test as boolean
	enable_depth_write as boolean
	enable_stencil_test as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_GPUColorTargetDescription
	format as SDL_GPUTextureFormat
	blend_state as SDL_GPUColorTargetBlendState
end type

type SDL_GPUGraphicsPipelineTargetInfo
	color_target_descriptions as const SDL_GPUColorTargetDescription ptr
	num_color_targets as Uint32
	depth_stencil_format as SDL_GPUTextureFormat
	has_depth_stencil_target as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_GPUGraphicsPipelineCreateInfo
	vertex_shader as SDL_GPUShader ptr
	fragment_shader as SDL_GPUShader ptr
	vertex_input_state as SDL_GPUVertexInputState
	primitive_type as SDL_GPUPrimitiveType
	rasterizer_state as SDL_GPURasterizerState
	multisample_state as SDL_GPUMultisampleState
	depth_stencil_state as SDL_GPUDepthStencilState
	target_info as SDL_GPUGraphicsPipelineTargetInfo
	props as SDL_PropertiesID
end type

type SDL_GPUComputePipelineCreateInfo
	code_size as uinteger
	code as const Uint8 ptr
	entrypoint as const zstring ptr
	format as SDL_GPUShaderFormat
	num_samplers as Uint32
	num_readonly_storage_textures as Uint32
	num_readonly_storage_buffers as Uint32
	num_readwrite_storage_textures as Uint32
	num_readwrite_storage_buffers as Uint32
	num_uniform_buffers as Uint32
	threadcount_x as Uint32
	threadcount_y as Uint32
	threadcount_z as Uint32
	props as SDL_PropertiesID
end type

type SDL_GPUColorTargetInfo
	texture as SDL_GPUTexture ptr
	mip_level as Uint32
	layer_or_depth_plane as Uint32
	clear_color as SDL_FColor
	load_op as SDL_GPULoadOp
	store_op as SDL_GPUStoreOp
	resolve_texture as SDL_GPUTexture ptr
	resolve_mip_level as Uint32
	resolve_layer as Uint32
	cycle as boolean
	cycle_resolve_texture as boolean
	padding1 as Uint8
	padding2 as Uint8
end type

type SDL_GPUDepthStencilTargetInfo
	texture as SDL_GPUTexture ptr
	clear_depth as single
	load_op as SDL_GPULoadOp
	store_op as SDL_GPUStoreOp
	stencil_load_op as SDL_GPULoadOp
	stencil_store_op as SDL_GPUStoreOp
	cycle as boolean
	clear_stencil as Uint8
	mip_level as Uint8
	layer as Uint8
end type

type SDL_GPUBlitInfo
	source as SDL_GPUBlitRegion
	destination as SDL_GPUBlitRegion
	load_op as SDL_GPULoadOp
	clear_color as SDL_FColor
	flip_mode as SDL_FlipMode
	filter as SDL_GPUFilter
	cycle as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_GPUBufferBinding
	buffer as SDL_GPUBuffer ptr
	offset as Uint32
end type

type SDL_GPUTextureSamplerBinding
	texture as SDL_GPUTexture ptr
	sampler as SDL_GPUSampler ptr
end type

type SDL_GPUStorageBufferReadWriteBinding
	buffer as SDL_GPUBuffer ptr
	cycle as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

type SDL_GPUStorageTextureReadWriteBinding
	texture as SDL_GPUTexture ptr
	mip_level as Uint32
	layer as Uint32
	cycle as boolean
	padding1 as Uint8
	padding2 as Uint8
	padding3 as Uint8
end type

declare function SDL_GPUSupportsShaderFormats(byval format_flags as SDL_GPUShaderFormat, byval name as const zstring ptr) as boolean
declare function SDL_GPUSupportsProperties(byval props as SDL_PropertiesID) as boolean
declare function SDL_CreateGPUDevice(byval format_flags as SDL_GPUShaderFormat, byval debug_mode as boolean, byval name as const zstring ptr) as SDL_GPUDevice ptr
declare function SDL_CreateGPUDeviceWithProperties(byval props as SDL_PropertiesID) as SDL_GPUDevice ptr

#define SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN "SDL.gpu.device.create.debugmode"
#define SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN "SDL.gpu.device.create.preferlowpower"
#define SDL_PROP_GPU_DEVICE_CREATE_VERBOSE_BOOLEAN "SDL.gpu.device.create.verbose"
#define SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING "SDL.gpu.device.create.name"
#define SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN "SDL.gpu.device.create.feature.clip_distance"
#define SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN "SDL.gpu.device.create.feature.depth_clamping"
#define SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN "SDL.gpu.device.create.feature.indirect_draw_first_instance"
#define SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN "SDL.gpu.device.create.feature.anisotropy"
#define SDL_PROP_GPU_DEVICE_CREATE_SHADERS_PRIVATE_BOOLEAN "SDL.gpu.device.create.shaders.private"
#define SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN "SDL.gpu.device.create.shaders.spirv"
#define SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXBC_BOOLEAN "SDL.gpu.device.create.shaders.dxbc"
#define SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN "SDL.gpu.device.create.shaders.dxil"
#define SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN "SDL.gpu.device.create.shaders.msl"
#define SDL_PROP_GPU_DEVICE_CREATE_SHADERS_METALLIB_BOOLEAN "SDL.gpu.device.create.shaders.metallib"
#define SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN "SDL.gpu.device.create.d3d12.allowtier1resourcebinding"
#define SDL_PROP_GPU_DEVICE_CREATE_D3D12_SEMANTIC_NAME_STRING "SDL.gpu.device.create.d3d12.semantic"
#define SDL_PROP_GPU_DEVICE_CREATE_D3D12_AGILITY_SDK_VERSION_NUMBER "SDL.gpu.device.create.d3d12.agility_sdk_version"
#define SDL_PROP_GPU_DEVICE_CREATE_D3D12_AGILITY_SDK_PATH_STRING "SDL.gpu.device.create.d3d12.agility_sdk_path"
#define SDL_PROP_GPU_DEVICE_CREATE_VULKAN_REQUIRE_HARDWARE_ACCELERATION_BOOLEAN "SDL.gpu.device.create.vulkan.requirehardwareacceleration"
#define SDL_PROP_GPU_DEVICE_CREATE_VULKAN_OPTIONS_POINTER "SDL.gpu.device.create.vulkan.options"
#define SDL_PROP_GPU_DEVICE_CREATE_METAL_ALLOW_MACFAMILY1_BOOLEAN "SDL.gpu.device.create.metal.allowmacfamily1"

type SDL_GPUVulkanOptions
	vulkan_api_version as Uint32
	feature_list as any ptr
	vulkan_10_physical_device_features as any ptr
	device_extension_count as Uint32
	device_extension_names as const zstring ptr ptr
	instance_extension_count as Uint32
	instance_extension_names as const zstring ptr ptr
end type

declare sub SDL_DestroyGPUDevice(byval device as SDL_GPUDevice ptr)
declare function SDL_GetNumGPUDrivers() as long
declare function SDL_GetGPUDriver(byval index as long) as const zstring ptr
declare function SDL_GetGPUDeviceDriver(byval device as SDL_GPUDevice ptr) as const zstring ptr
declare function SDL_GetGPUShaderFormats(byval device as SDL_GPUDevice ptr) as SDL_GPUShaderFormat
declare function SDL_GetGPUDeviceProperties(byval device as SDL_GPUDevice ptr) as SDL_PropertiesID

#define SDL_PROP_GPU_DEVICE_NAME_STRING "SDL.gpu.device.name"
#define SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING "SDL.gpu.device.driver_name"
#define SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING "SDL.gpu.device.driver_version"
#define SDL_PROP_GPU_DEVICE_DRIVER_INFO_STRING "SDL.gpu.device.driver_info"
declare function SDL_CreateGPUComputePipeline(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUComputePipelineCreateInfo ptr) as SDL_GPUComputePipeline ptr
#define SDL_PROP_GPU_COMPUTEPIPELINE_CREATE_NAME_STRING "SDL.gpu.computepipeline.create.name"
declare function SDL_CreateGPUGraphicsPipeline(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUGraphicsPipelineCreateInfo ptr) as SDL_GPUGraphicsPipeline ptr
#define SDL_PROP_GPU_GRAPHICSPIPELINE_CREATE_NAME_STRING "SDL.gpu.graphicspipeline.create.name"
declare function SDL_CreateGPUSampler(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUSamplerCreateInfo ptr) as SDL_GPUSampler ptr
#define SDL_PROP_GPU_SAMPLER_CREATE_NAME_STRING "SDL.gpu.sampler.create.name"
declare function SDL_CreateGPUShader(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUShaderCreateInfo ptr) as SDL_GPUShader ptr
#define SDL_PROP_GPU_SHADER_CREATE_NAME_STRING "SDL.gpu.shader.create.name"
declare function SDL_CreateGPUTexture(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUTextureCreateInfo ptr) as SDL_GPUTexture ptr
#define SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_R_FLOAT "SDL.gpu.texture.create.d3d12.clear.r"
#define SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_G_FLOAT "SDL.gpu.texture.create.d3d12.clear.g"
#define SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_B_FLOAT "SDL.gpu.texture.create.d3d12.clear.b"
#define SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_A_FLOAT "SDL.gpu.texture.create.d3d12.clear.a"
#define SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_DEPTH_FLOAT "SDL.gpu.texture.create.d3d12.clear.depth"
#define SDL_PROP_GPU_TEXTURE_CREATE_D3D12_CLEAR_STENCIL_NUMBER "SDL.gpu.texture.create.d3d12.clear.stencil"
#define SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING "SDL.gpu.texture.create.name"
declare function SDL_CreateGPUBuffer(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUBufferCreateInfo ptr) as SDL_GPUBuffer ptr
#define SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING "SDL.gpu.buffer.create.name"
declare function SDL_CreateGPUTransferBuffer(byval device as SDL_GPUDevice ptr, byval createinfo as const SDL_GPUTransferBufferCreateInfo ptr) as SDL_GPUTransferBuffer ptr
#define SDL_PROP_GPU_TRANSFERBUFFER_CREATE_NAME_STRING "SDL.gpu.transferbuffer.create.name"

declare sub SDL_SetGPUBufferName(byval device as SDL_GPUDevice ptr, byval buffer as SDL_GPUBuffer ptr, byval text as const zstring ptr)
declare sub SDL_SetGPUTextureName(byval device as SDL_GPUDevice ptr, byval texture as SDL_GPUTexture ptr, byval text as const zstring ptr)
declare sub SDL_InsertGPUDebugLabel(byval command_buffer as SDL_GPUCommandBuffer ptr, byval text as const zstring ptr)
declare sub SDL_PushGPUDebugGroup(byval command_buffer as SDL_GPUCommandBuffer ptr, byval name as const zstring ptr)
declare sub SDL_PopGPUDebugGroup(byval command_buffer as SDL_GPUCommandBuffer ptr)
declare sub SDL_ReleaseGPUTexture(byval device as SDL_GPUDevice ptr, byval texture as SDL_GPUTexture ptr)
declare sub SDL_ReleaseGPUSampler(byval device as SDL_GPUDevice ptr, byval sampler as SDL_GPUSampler ptr)
declare sub SDL_ReleaseGPUBuffer(byval device as SDL_GPUDevice ptr, byval buffer as SDL_GPUBuffer ptr)
declare sub SDL_ReleaseGPUTransferBuffer(byval device as SDL_GPUDevice ptr, byval transfer_buffer as SDL_GPUTransferBuffer ptr)
declare sub SDL_ReleaseGPUComputePipeline(byval device as SDL_GPUDevice ptr, byval compute_pipeline as SDL_GPUComputePipeline ptr)
declare sub SDL_ReleaseGPUShader(byval device as SDL_GPUDevice ptr, byval shader as SDL_GPUShader ptr)
declare sub SDL_ReleaseGPUGraphicsPipeline(byval device as SDL_GPUDevice ptr, byval graphics_pipeline as SDL_GPUGraphicsPipeline ptr)
declare function SDL_AcquireGPUCommandBuffer(byval device as SDL_GPUDevice ptr) as SDL_GPUCommandBuffer ptr
declare sub SDL_PushGPUVertexUniformData(byval command_buffer as SDL_GPUCommandBuffer ptr, byval slot_index as Uint32, byval data as const any ptr, byval length as Uint32)
declare sub SDL_PushGPUFragmentUniformData(byval command_buffer as SDL_GPUCommandBuffer ptr, byval slot_index as Uint32, byval data as const any ptr, byval length as Uint32)
declare sub SDL_PushGPUComputeUniformData(byval command_buffer as SDL_GPUCommandBuffer ptr, byval slot_index as Uint32, byval data as const any ptr, byval length as Uint32)
declare function SDL_BeginGPURenderPass(byval command_buffer as SDL_GPUCommandBuffer ptr, byval color_target_infos as const SDL_GPUColorTargetInfo ptr, byval num_color_targets as Uint32, byval depth_stencil_target_info as const SDL_GPUDepthStencilTargetInfo ptr) as SDL_GPURenderPass ptr
declare sub SDL_BindGPUGraphicsPipeline(byval render_pass as SDL_GPURenderPass ptr, byval graphics_pipeline as SDL_GPUGraphicsPipeline ptr)
declare sub SDL_SetGPUViewport(byval render_pass as SDL_GPURenderPass ptr, byval viewport as const SDL_GPUViewport ptr)
declare sub SDL_SetGPUScissor(byval render_pass as SDL_GPURenderPass ptr, byval scissor as const SDL_Rect ptr)
declare sub SDL_SetGPUBlendConstants(byval render_pass as SDL_GPURenderPass ptr, byval blend_constants as SDL_FColor)
declare sub SDL_SetGPUStencilReference(byval render_pass as SDL_GPURenderPass ptr, byval reference as Uint8)
declare sub SDL_BindGPUVertexBuffers(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval bindings as const SDL_GPUBufferBinding ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUIndexBuffer(byval render_pass as SDL_GPURenderPass ptr, byval binding as const SDL_GPUBufferBinding ptr, byval index_element_size as SDL_GPUIndexElementSize)
declare sub SDL_BindGPUVertexSamplers(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval texture_sampler_bindings as const SDL_GPUTextureSamplerBinding ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUVertexStorageTextures(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval storage_textures as SDL_GPUTexture const ptr ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUVertexStorageBuffers(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval storage_buffers as SDL_GPUBuffer const ptr ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUFragmentSamplers(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval texture_sampler_bindings as const SDL_GPUTextureSamplerBinding ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUFragmentStorageTextures(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval storage_textures as SDL_GPUTexture const ptr ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUFragmentStorageBuffers(byval render_pass as SDL_GPURenderPass ptr, byval first_slot as Uint32, byval storage_buffers as SDL_GPUBuffer const ptr ptr, byval num_bindings as Uint32)
declare sub SDL_DrawGPUIndexedPrimitives(byval render_pass as SDL_GPURenderPass ptr, byval num_indices as Uint32, byval num_instances as Uint32, byval first_index as Uint32, byval vertex_offset as Sint32, byval first_instance as Uint32)
declare sub SDL_DrawGPUPrimitives(byval render_pass as SDL_GPURenderPass ptr, byval num_vertices as Uint32, byval num_instances as Uint32, byval first_vertex as Uint32, byval first_instance as Uint32)
declare sub SDL_DrawGPUPrimitivesIndirect(byval render_pass as SDL_GPURenderPass ptr, byval buffer as SDL_GPUBuffer ptr, byval offset as Uint32, byval draw_count as Uint32)
declare sub SDL_DrawGPUIndexedPrimitivesIndirect(byval render_pass as SDL_GPURenderPass ptr, byval buffer as SDL_GPUBuffer ptr, byval offset as Uint32, byval draw_count as Uint32)
declare sub SDL_EndGPURenderPass(byval render_pass as SDL_GPURenderPass ptr)
declare function SDL_BeginGPUComputePass(byval command_buffer as SDL_GPUCommandBuffer ptr, byval storage_texture_bindings as const SDL_GPUStorageTextureReadWriteBinding ptr, byval num_storage_texture_bindings as Uint32, byval storage_buffer_bindings as const SDL_GPUStorageBufferReadWriteBinding ptr, byval num_storage_buffer_bindings as Uint32) as SDL_GPUComputePass ptr
declare sub SDL_BindGPUComputePipeline(byval compute_pass as SDL_GPUComputePass ptr, byval compute_pipeline as SDL_GPUComputePipeline ptr)
declare sub SDL_BindGPUComputeSamplers(byval compute_pass as SDL_GPUComputePass ptr, byval first_slot as Uint32, byval texture_sampler_bindings as const SDL_GPUTextureSamplerBinding ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUComputeStorageTextures(byval compute_pass as SDL_GPUComputePass ptr, byval first_slot as Uint32, byval storage_textures as SDL_GPUTexture const ptr ptr, byval num_bindings as Uint32)
declare sub SDL_BindGPUComputeStorageBuffers(byval compute_pass as SDL_GPUComputePass ptr, byval first_slot as Uint32, byval storage_buffers as SDL_GPUBuffer const ptr ptr, byval num_bindings as Uint32)
declare sub SDL_DispatchGPUCompute(byval compute_pass as SDL_GPUComputePass ptr, byval groupcount_x as Uint32, byval groupcount_y as Uint32, byval groupcount_z as Uint32)
declare sub SDL_DispatchGPUComputeIndirect(byval compute_pass as SDL_GPUComputePass ptr, byval buffer as SDL_GPUBuffer ptr, byval offset as Uint32)
declare sub SDL_EndGPUComputePass(byval compute_pass as SDL_GPUComputePass ptr)
declare function SDL_MapGPUTransferBuffer(byval device as SDL_GPUDevice ptr, byval transfer_buffer as SDL_GPUTransferBuffer ptr, byval cycle as boolean) as any ptr
declare sub SDL_UnmapGPUTransferBuffer(byval device as SDL_GPUDevice ptr, byval transfer_buffer as SDL_GPUTransferBuffer ptr)
declare function SDL_BeginGPUCopyPass(byval command_buffer as SDL_GPUCommandBuffer ptr) as SDL_GPUCopyPass ptr
declare sub SDL_UploadToGPUTexture(byval copy_pass as SDL_GPUCopyPass ptr, byval source as const SDL_GPUTextureTransferInfo ptr, byval destination as const SDL_GPUTextureRegion ptr, byval cycle as boolean)
declare sub SDL_UploadToGPUBuffer(byval copy_pass as SDL_GPUCopyPass ptr, byval source as const SDL_GPUTransferBufferLocation ptr, byval destination as const SDL_GPUBufferRegion ptr, byval cycle as boolean)
declare sub SDL_CopyGPUTextureToTexture(byval copy_pass as SDL_GPUCopyPass ptr, byval source as const SDL_GPUTextureLocation ptr, byval destination as const SDL_GPUTextureLocation ptr, byval w as Uint32, byval h as Uint32, byval d as Uint32, byval cycle as boolean)
declare sub SDL_CopyGPUBufferToBuffer(byval copy_pass as SDL_GPUCopyPass ptr, byval source as const SDL_GPUBufferLocation ptr, byval destination as const SDL_GPUBufferLocation ptr, byval size as Uint32, byval cycle as boolean)
declare sub SDL_DownloadFromGPUTexture(byval copy_pass as SDL_GPUCopyPass ptr, byval source as const SDL_GPUTextureRegion ptr, byval destination as const SDL_GPUTextureTransferInfo ptr)
declare sub SDL_DownloadFromGPUBuffer(byval copy_pass as SDL_GPUCopyPass ptr, byval source as const SDL_GPUBufferRegion ptr, byval destination as const SDL_GPUTransferBufferLocation ptr)
declare sub SDL_EndGPUCopyPass(byval copy_pass as SDL_GPUCopyPass ptr)
declare sub SDL_GenerateMipmapsForGPUTexture(byval command_buffer as SDL_GPUCommandBuffer ptr, byval texture as SDL_GPUTexture ptr)
declare sub SDL_BlitGPUTexture(byval command_buffer as SDL_GPUCommandBuffer ptr, byval info as const SDL_GPUBlitInfo ptr)
declare function SDL_WindowSupportsGPUSwapchainComposition(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr, byval swapchain_composition as SDL_GPUSwapchainComposition) as boolean
declare function SDL_WindowSupportsGPUPresentMode(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr, byval present_mode as SDL_GPUPresentMode) as boolean
declare function SDL_ClaimWindowForGPUDevice(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr) as boolean
declare sub SDL_ReleaseWindowFromGPUDevice(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr)
declare function SDL_SetGPUSwapchainParameters(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr, byval swapchain_composition as SDL_GPUSwapchainComposition, byval present_mode as SDL_GPUPresentMode) as boolean
declare function SDL_SetGPUAllowedFramesInFlight(byval device as SDL_GPUDevice ptr, byval allowed_frames_in_flight as Uint32) as boolean
declare function SDL_GetGPUSwapchainTextureFormat(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr) as SDL_GPUTextureFormat
declare function SDL_AcquireGPUSwapchainTexture(byval command_buffer as SDL_GPUCommandBuffer ptr, byval window as SDL_Window ptr, byval swapchain_texture as SDL_GPUTexture ptr ptr, byval swapchain_texture_width as Uint32 ptr, byval swapchain_texture_height as Uint32 ptr) as boolean
declare function SDL_WaitForGPUSwapchain(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr) as boolean
declare function SDL_WaitAndAcquireGPUSwapchainTexture(byval command_buffer as SDL_GPUCommandBuffer ptr, byval window as SDL_Window ptr, byval swapchain_texture as SDL_GPUTexture ptr ptr, byval swapchain_texture_width as Uint32 ptr, byval swapchain_texture_height as Uint32 ptr) as boolean
declare function SDL_SubmitGPUCommandBuffer(byval command_buffer as SDL_GPUCommandBuffer ptr) as boolean
declare function SDL_SubmitGPUCommandBufferAndAcquireFence(byval command_buffer as SDL_GPUCommandBuffer ptr) as SDL_GPUFence ptr
declare function SDL_CancelGPUCommandBuffer(byval command_buffer as SDL_GPUCommandBuffer ptr) as boolean
declare function SDL_WaitForGPUIdle(byval device as SDL_GPUDevice ptr) as boolean
declare function SDL_WaitForGPUFences(byval device as SDL_GPUDevice ptr, byval wait_all as boolean, byval fences as SDL_GPUFence const ptr ptr, byval num_fences as Uint32) as boolean
declare function SDL_QueryGPUFence(byval device as SDL_GPUDevice ptr, byval fence as SDL_GPUFence ptr) as boolean
declare sub SDL_ReleaseGPUFence(byval device as SDL_GPUDevice ptr, byval fence as SDL_GPUFence ptr)
declare function SDL_GPUTextureFormatTexelBlockSize(byval format as SDL_GPUTextureFormat) as Uint32
declare function SDL_GPUTextureSupportsFormat(byval device as SDL_GPUDevice ptr, byval format as SDL_GPUTextureFormat, byval type as SDL_GPUTextureType, byval usage as SDL_GPUTextureUsageFlags) as boolean
declare function SDL_GPUTextureSupportsSampleCount(byval device as SDL_GPUDevice ptr, byval format as SDL_GPUTextureFormat, byval sample_count as SDL_GPUSampleCount) as boolean
declare function SDL_CalculateGPUTextureFormatSize(byval format as SDL_GPUTextureFormat, byval width as Uint32, byval height as Uint32, byval depth_or_layer_count as Uint32) as Uint32
declare function SDL_GetPixelFormatFromGPUTextureFormat(byval format as SDL_GPUTextureFormat) as SDL_PixelFormat
declare function SDL_GetGPUTextureFormatFromPixelFormat(byval format as SDL_PixelFormat) as SDL_GPUTextureFormat

'' -------------------------------------------------------------------------
'' SDL_haptic.h
'' -------------------------------------------------------------------------
const SDL_HAPTIC_INFINITY = 4294967295u
type SDL_HapticEffectType as Uint16
const SDL_HAPTIC_CONSTANT = culng(1u shl 0)
const SDL_HAPTIC_SINE = culng(1u shl 1)
const SDL_HAPTIC_SQUARE = culng(1u shl 2)
const SDL_HAPTIC_TRIANGLE = culng(1u shl 3)
const SDL_HAPTIC_SAWTOOTHUP = culng(1u shl 4)
const SDL_HAPTIC_SAWTOOTHDOWN = culng(1u shl 5)
const SDL_HAPTIC_RAMP = culng(1u shl 6)
const SDL_HAPTIC_SPRING = culng(1u shl 7)
const SDL_HAPTIC_DAMPER = culng(1u shl 8)
const SDL_HAPTIC_INERTIA = culng(1u shl 9)
const SDL_HAPTIC_FRICTION = culng(1u shl 10)
const SDL_HAPTIC_LEFTRIGHT = culng(1u shl 11)
const SDL_HAPTIC_RESERVED1 = culng(1u shl 12)
const SDL_HAPTIC_RESERVED2 = culng(1u shl 13)
const SDL_HAPTIC_RESERVED3 = culng(1u shl 14)
const SDL_HAPTIC_CUSTOM = culng(1u shl 15)
const SDL_HAPTIC_GAIN = culng(1u shl 16)
const SDL_HAPTIC_AUTOCENTER = culng(1u shl 17)
const SDL_HAPTIC_STATUS = culng(1u shl 18)
const SDL_HAPTIC_PAUSE = culng(1u shl 19)
type SDL_HapticDirectionType as Uint8
const SDL_HAPTIC_POLAR = 0
const SDL_HAPTIC_CARTESIAN = 1
const SDL_HAPTIC_SPHERICAL = 2
const SDL_HAPTIC_STEERING_AXIS = 3
type SDL_HapticEffectID as long

type SDL_HapticDirection
	as SDL_HapticDirectionType type
	dir(0 to 2) as Sint32
end type

type SDL_HapticConstant
	as SDL_HapticEffectType type
	direction as SDL_HapticDirection
	length as Uint32
	delay as Uint16
	button as Uint16
	interval as Uint16
	level as Sint16
	attack_length as Uint16
	attack_level as Uint16
	fade_length as Uint16
	fade_level as Uint16
end type

type SDL_HapticPeriodic
	as SDL_HapticEffectType type
	direction as SDL_HapticDirection
	length as Uint32
	delay as Uint16
	button as Uint16
	interval as Uint16
	period as Uint16
	magnitude as Sint16
	offset as Sint16
	phase as Uint16
	attack_length as Uint16
	attack_level as Uint16
	fade_length as Uint16
	fade_level as Uint16
end type

type SDL_HapticCondition
	as SDL_HapticEffectType type
	direction as SDL_HapticDirection
	length as Uint32
	delay as Uint16
	button as Uint16
	interval as Uint16
	right_sat(0 to 2) as Uint16
	left_sat(0 to 2) as Uint16
	right_coeff(0 to 2) as Sint16
	left_coeff(0 to 2) as Sint16
	deadband(0 to 2) as Uint16
	center(0 to 2) as Sint16
end type

type SDL_HapticRamp
	as SDL_HapticEffectType type
	direction as SDL_HapticDirection
	length as Uint32
	delay as Uint16
	button as Uint16
	interval as Uint16
	start as Sint16
	as Sint16 end
	attack_length as Uint16
	attack_level as Uint16
	fade_length as Uint16
	fade_level as Uint16
end type

type SDL_HapticLeftRight
	as SDL_HapticEffectType type
	length as Uint32
	large_magnitude as Uint16
	small_magnitude as Uint16
end type

type SDL_HapticCustom
	as SDL_HapticEffectType type
	direction as SDL_HapticDirection
	length as Uint32
	delay as Uint16
	button as Uint16
	interval as Uint16
	channels as Uint8
	period as Uint16
	samples as Uint16
	data as Uint16 ptr
	attack_length as Uint16
	attack_level as Uint16
	fade_length as Uint16
	fade_level as Uint16
end type

union SDL_HapticEffect
	as SDL_HapticEffectType type
	constant as SDL_HapticConstant
	periodic as SDL_HapticPeriodic
	condition as SDL_HapticCondition
	ramp as SDL_HapticRamp
	leftright as SDL_HapticLeftRight
	custom as SDL_HapticCustom
end union

type SDL_HapticID as Uint32
declare function SDL_GetHaptics(byval count as long ptr) as SDL_HapticID ptr
declare function SDL_GetHapticNameForID(byval instance_id as SDL_HapticID) as const zstring ptr
declare function SDL_OpenHaptic(byval instance_id as SDL_HapticID) as SDL_Haptic ptr
declare function SDL_GetHapticFromID(byval instance_id as SDL_HapticID) as SDL_Haptic ptr
declare function SDL_GetHapticID(byval haptic as SDL_Haptic ptr) as SDL_HapticID
declare function SDL_GetHapticName(byval haptic as SDL_Haptic ptr) as const zstring ptr
declare function SDL_IsMouseHaptic() as boolean
declare function SDL_OpenHapticFromMouse() as SDL_Haptic ptr
declare function SDL_IsJoystickHaptic(byval joystick as SDL_Joystick ptr) as boolean
declare function SDL_OpenHapticFromJoystick(byval joystick as SDL_Joystick ptr) as SDL_Haptic ptr
declare sub SDL_CloseHaptic(byval haptic as SDL_Haptic ptr)
declare function SDL_GetMaxHapticEffects(byval haptic as SDL_Haptic ptr) as long
declare function SDL_GetMaxHapticEffectsPlaying(byval haptic as SDL_Haptic ptr) as long
declare function SDL_GetHapticFeatures(byval haptic as SDL_Haptic ptr) as Uint32
declare function SDL_GetNumHapticAxes(byval haptic as SDL_Haptic ptr) as long
declare function SDL_HapticEffectSupported(byval haptic as SDL_Haptic ptr, byval effect as const SDL_HapticEffect ptr) as boolean
declare function SDL_CreateHapticEffect(byval haptic as SDL_Haptic ptr, byval effect as const SDL_HapticEffect ptr) as SDL_HapticEffectID
declare function SDL_UpdateHapticEffect(byval haptic as SDL_Haptic ptr, byval effect as SDL_HapticEffectID, byval data as const SDL_HapticEffect ptr) as boolean
declare function SDL_RunHapticEffect(byval haptic as SDL_Haptic ptr, byval effect as SDL_HapticEffectID, byval iterations as Uint32) as boolean
declare function SDL_StopHapticEffect(byval haptic as SDL_Haptic ptr, byval effect as SDL_HapticEffectID) as boolean
declare sub SDL_DestroyHapticEffect(byval haptic as SDL_Haptic ptr, byval effect as SDL_HapticEffectID)
declare function SDL_GetHapticEffectStatus(byval haptic as SDL_Haptic ptr, byval effect as SDL_HapticEffectID) as boolean
declare function SDL_SetHapticGain(byval haptic as SDL_Haptic ptr, byval gain as long) as boolean
declare function SDL_SetHapticAutocenter(byval haptic as SDL_Haptic ptr, byval autocenter as long) as boolean
declare function SDL_PauseHaptic(byval haptic as SDL_Haptic ptr) as boolean
declare function SDL_ResumeHaptic(byval haptic as SDL_Haptic ptr) as boolean
declare function SDL_StopHapticEffects(byval haptic as SDL_Haptic ptr) as boolean
declare function SDL_HapticRumbleSupported(byval haptic as SDL_Haptic ptr) as boolean
declare function SDL_InitHapticRumble(byval haptic as SDL_Haptic ptr) as boolean
declare function SDL_PlayHapticRumble(byval haptic as SDL_Haptic ptr, byval strength as single, byval length as Uint32) as boolean
declare function SDL_StopHapticRumble(byval haptic as SDL_Haptic ptr) as boolean
'' -------------------------------------------------------------------------
'' SDL_hidapi.h
'' -------------------------------------------------------------------------
type SDL_hid_bus_type as long
enum
	SDL_HID_API_BUS_UNKNOWN = &h00
	SDL_HID_API_BUS_USB = &h01
	SDL_HID_API_BUS_BLUETOOTH = &h02
	SDL_HID_API_BUS_I2C = &h03
	SDL_HID_API_BUS_SPI = &h04
end enum

type SDL_hid_device_info
	path as zstring ptr
	vendor_id as ushort
	product_id as ushort
	serial_number as wstring ptr
	release_number as ushort
	manufacturer_string as wstring ptr
	product_string as wstring ptr
	usage_page as ushort
	usage as ushort
	interface_number as long
	interface_class as long
	interface_subclass as long
	interface_protocol as long
	bus_type as SDL_hid_bus_type
	next as SDL_hid_device_info ptr
end type

declare function SDL_hid_init() as long
declare function SDL_hid_exit() as long
declare function SDL_hid_device_change_count() as Uint32
declare function SDL_hid_enumerate(byval vendor_id as ushort, byval product_id as ushort) as SDL_hid_device_info ptr
declare sub SDL_hid_free_enumeration(byval devs as SDL_hid_device_info ptr)
declare function SDL_hid_open(byval vendor_id as ushort, byval product_id as ushort, byval serial_number as const wstring ptr) as SDL_hid_device ptr
declare function SDL_hid_open_path(byval path as const zstring ptr) as SDL_hid_device ptr
declare function SDL_hid_get_properties(byval dev as SDL_hid_device ptr) as SDL_PropertiesID
#define SDL_PROP_HIDAPI_LIBUSB_DEVICE_HANDLE_POINTER "SDL.hidapi.libusb.device.handle"
declare function SDL_hid_write(byval dev as SDL_hid_device ptr, byval data as const ubyte ptr, byval length as uinteger) as long
declare function SDL_hid_read_timeout(byval dev as SDL_hid_device ptr, byval data as ubyte ptr, byval length as uinteger, byval milliseconds as long) as long
declare function SDL_hid_read(byval dev as SDL_hid_device ptr, byval data as ubyte ptr, byval length as uinteger) as long
declare function SDL_hid_set_nonblocking(byval dev as SDL_hid_device ptr, byval nonblock as long) as long
declare function SDL_hid_send_feature_report(byval dev as SDL_hid_device ptr, byval data as const ubyte ptr, byval length as uinteger) as long
declare function SDL_hid_get_feature_report(byval dev as SDL_hid_device ptr, byval data as ubyte ptr, byval length as uinteger) as long
declare function SDL_hid_get_input_report(byval dev as SDL_hid_device ptr, byval data as ubyte ptr, byval length as uinteger) as long
declare function SDL_hid_close(byval dev as SDL_hid_device ptr) as long
declare function SDL_hid_get_manufacturer_string(byval dev as SDL_hid_device ptr, byval string as wstring ptr, byval maxlen as uinteger) as long
declare function SDL_hid_get_product_string(byval dev as SDL_hid_device ptr, byval string as wstring ptr, byval maxlen as uinteger) as long
declare function SDL_hid_get_serial_number_string(byval dev as SDL_hid_device ptr, byval string as wstring ptr, byval maxlen as uinteger) as long
declare function SDL_hid_get_indexed_string(byval dev as SDL_hid_device ptr, byval string_index as long, byval string as wstring ptr, byval maxlen as uinteger) as long
declare function SDL_hid_get_device_info(byval dev as SDL_hid_device ptr) as SDL_hid_device_info ptr
declare function SDL_hid_get_report_descriptor(byval dev as SDL_hid_device ptr, byval buf as ubyte ptr, byval buf_size as uinteger) as long
declare sub SDL_hid_ble_scan(byval active as boolean)

'' -------------------------------------------------------------------------
'' SDL_hints.h
'' -------------------------------------------------------------------------
#define SDL_HINT_ALLOW_ALT_TAB_WHILE_GRABBED "SDL_ALLOW_ALT_TAB_WHILE_GRABBED"
#define SDL_HINT_ANDROID_ALLOW_RECREATE_ACTIVITY "SDL_ANDROID_ALLOW_RECREATE_ACTIVITY"
#define SDL_HINT_ANDROID_BLOCK_ON_PAUSE "SDL_ANDROID_BLOCK_ON_PAUSE"
#define SDL_HINT_ANDROID_LOW_LATENCY_AUDIO "SDL_ANDROID_LOW_LATENCY_AUDIO"
#define SDL_HINT_ANDROID_AAUDIO_INPUT_PRESET "SDL_ANDROID_AAUDIO_INPUT_PRESET"
#define SDL_HINT_ANDROID_TRAP_BACK_BUTTON "SDL_ANDROID_TRAP_BACK_BUTTON"
#define SDL_HINT_APP_ID "SDL_APP_ID"
#define SDL_HINT_APP_NAME "SDL_APP_NAME"
#define SDL_HINT_APPLE_TV_CONTROLLER_UI_EVENTS "SDL_APPLE_TV_CONTROLLER_UI_EVENTS"
#define SDL_HINT_APPLE_TV_REMOTE_ALLOW_ROTATION "SDL_APPLE_TV_REMOTE_ALLOW_ROTATION"
#define SDL_HINT_AUDIO_ALSA_DEFAULT_DEVICE "SDL_AUDIO_ALSA_DEFAULT_DEVICE"
#define SDL_HINT_AUDIO_ALSA_DEFAULT_PLAYBACK_DEVICE "SDL_AUDIO_ALSA_DEFAULT_PLAYBACK_DEVICE"
#define SDL_HINT_AUDIO_ALSA_DEFAULT_RECORDING_DEVICE "SDL_AUDIO_ALSA_DEFAULT_RECORDING_DEVICE"
#define SDL_HINT_AUDIO_CATEGORY "SDL_AUDIO_CATEGORY"
#define SDL_HINT_AUDIO_CHANNELS "SDL_AUDIO_CHANNELS"
#define SDL_HINT_AUDIO_DEVICE_APP_ICON_NAME "SDL_AUDIO_DEVICE_APP_ICON_NAME"
#define SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES "SDL_AUDIO_DEVICE_SAMPLE_FRAMES"
#define SDL_HINT_AUDIO_DEVICE_STREAM_NAME "SDL_AUDIO_DEVICE_STREAM_NAME"
#define SDL_HINT_AUDIO_DEVICE_STREAM_ROLE "SDL_AUDIO_DEVICE_STREAM_ROLE"
#define SDL_HINT_AUDIO_DEVICE_RAW_STREAM "SDL_AUDIO_DEVICE_RAW_STREAM"
#define SDL_HINT_AUDIO_DISK_INPUT_FILE "SDL_AUDIO_DISK_INPUT_FILE"
#define SDL_HINT_AUDIO_DISK_OUTPUT_FILE "SDL_AUDIO_DISK_OUTPUT_FILE"
#define SDL_HINT_AUDIO_DISK_TIMESCALE "SDL_AUDIO_DISK_TIMESCALE"
#define SDL_HINT_AUDIO_DRIVER "SDL_AUDIO_DRIVER"
#define SDL_HINT_AUDIO_DUMMY_TIMESCALE "SDL_AUDIO_DUMMY_TIMESCALE"
#define SDL_HINT_AUDIO_FORMAT "SDL_AUDIO_FORMAT"
#define SDL_HINT_AUDIO_FREQUENCY "SDL_AUDIO_FREQUENCY"
#define SDL_HINT_AUDIO_INCLUDE_MONITORS "SDL_AUDIO_INCLUDE_MONITORS"
#define SDL_HINT_AUTO_UPDATE_JOYSTICKS "SDL_AUTO_UPDATE_JOYSTICKS"
#define SDL_HINT_AUTO_UPDATE_SENSORS "SDL_AUTO_UPDATE_SENSORS"
#define SDL_HINT_BMP_SAVE_LEGACY_FORMAT "SDL_BMP_SAVE_LEGACY_FORMAT"
#define SDL_HINT_CAMERA_DRIVER "SDL_CAMERA_DRIVER"
#define SDL_HINT_CPU_FEATURE_MASK "SDL_CPU_FEATURE_MASK"
#define SDL_HINT_JOYSTICK_DIRECTINPUT "SDL_JOYSTICK_DIRECTINPUT"
#define SDL_HINT_FILE_DIALOG_DRIVER "SDL_FILE_DIALOG_DRIVER"
#define SDL_HINT_DISPLAY_USABLE_BOUNDS "SDL_DISPLAY_USABLE_BOUNDS"
#define SDL_HINT_INVALID_PARAM_CHECKS "SDL_INVALID_PARAM_CHECKS"
#define SDL_HINT_EMSCRIPTEN_ASYNCIFY "SDL_EMSCRIPTEN_ASYNCIFY"
#define SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR "SDL_EMSCRIPTEN_CANVAS_SELECTOR"
#define SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT "SDL_EMSCRIPTEN_KEYBOARD_ELEMENT"
#define SDL_HINT_ENABLE_SCREEN_KEYBOARD "SDL_ENABLE_SCREEN_KEYBOARD"
#define SDL_HINT_ENABLE_STEAM_SCREEN_KEYBOARD "SDL_ENABLE_STEAM_SCREEN_KEYBOARD"
#define SDL_HINT_EVDEV_DEVICES "SDL_EVDEV_DEVICES"
#define SDL_HINT_EVENT_LOGGING "SDL_EVENT_LOGGING"
#define SDL_HINT_FORCE_RAISEWINDOW "SDL_FORCE_RAISEWINDOW"
#define SDL_HINT_FRAMEBUFFER_ACCELERATION "SDL_FRAMEBUFFER_ACCELERATION"
#define SDL_HINT_GAMECONTROLLERCONFIG "SDL_GAMECONTROLLERCONFIG"
#define SDL_HINT_GAMECONTROLLERCONFIG_FILE "SDL_GAMECONTROLLERCONFIG_FILE"
#define SDL_HINT_GAMECONTROLLERTYPE "SDL_GAMECONTROLLERTYPE"
#define SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES "SDL_GAMECONTROLLER_IGNORE_DEVICES"
#define SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT "SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT"
#define SDL_HINT_GAMECONTROLLER_SENSOR_FUSION "SDL_GAMECONTROLLER_SENSOR_FUSION"
#define SDL_HINT_GDK_TEXTINPUT_DEFAULT_TEXT "SDL_GDK_TEXTINPUT_DEFAULT_TEXT"
#define SDL_HINT_GDK_TEXTINPUT_DESCRIPTION "SDL_GDK_TEXTINPUT_DESCRIPTION"
#define SDL_HINT_GDK_TEXTINPUT_MAX_LENGTH "SDL_GDK_TEXTINPUT_MAX_LENGTH"
#define SDL_HINT_GDK_TEXTINPUT_SCOPE "SDL_GDK_TEXTINPUT_SCOPE"
#define SDL_HINT_GDK_TEXTINPUT_TITLE "SDL_GDK_TEXTINPUT_TITLE"
#define SDL_HINT_HIDAPI_LIBUSB "SDL_HIDAPI_LIBUSB"
#define SDL_HINT_HIDAPI_LIBUSB_GAMECUBE "SDL_HIDAPI_LIBUSB_GAMECUBE"
#define SDL_HINT_HIDAPI_LIBUSB_WHITELIST "SDL_HIDAPI_LIBUSB_WHITELIST"
#define SDL_HINT_HIDAPI_UDEV "SDL_HIDAPI_UDEV"
#define SDL_HINT_GPU_DRIVER "SDL_GPU_DRIVER"
#define SDL_HINT_HIDAPI_ENUMERATE_ONLY_CONTROLLERS "SDL_HIDAPI_ENUMERATE_ONLY_CONTROLLERS"
#define SDL_HINT_HIDAPI_IGNORE_DEVICES "SDL_HIDAPI_IGNORE_DEVICES"
#define SDL_HINT_IME_IMPLEMENTED_UI "SDL_IME_IMPLEMENTED_UI"
#define SDL_HINT_IOS_HIDE_HOME_INDICATOR "SDL_IOS_HIDE_HOME_INDICATOR"
#define SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS "SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS"
#define SDL_HINT_JOYSTICK_ARCADESTICK_DEVICES "SDL_JOYSTICK_ARCADESTICK_DEVICES"
#define SDL_HINT_JOYSTICK_ARCADESTICK_DEVICES_EXCLUDED "SDL_JOYSTICK_ARCADESTICK_DEVICES_EXCLUDED"
#define SDL_HINT_JOYSTICK_BLACKLIST_DEVICES "SDL_JOYSTICK_BLACKLIST_DEVICES"
#define SDL_HINT_JOYSTICK_BLACKLIST_DEVICES_EXCLUDED "SDL_JOYSTICK_BLACKLIST_DEVICES_EXCLUDED"
#define SDL_HINT_JOYSTICK_DEVICE "SDL_JOYSTICK_DEVICE"
#define SDL_HINT_JOYSTICK_ENHANCED_REPORTS "SDL_JOYSTICK_ENHANCED_REPORTS"
#define SDL_HINT_JOYSTICK_FLIGHTSTICK_DEVICES "SDL_JOYSTICK_FLIGHTSTICK_DEVICES"
#define SDL_HINT_JOYSTICK_FLIGHTSTICK_DEVICES_EXCLUDED "SDL_JOYSTICK_FLIGHTSTICK_DEVICES_EXCLUDED"
#define SDL_HINT_JOYSTICK_GAMEINPUT "SDL_JOYSTICK_GAMEINPUT"
#define SDL_HINT_JOYSTICK_GAMEINPUT_RAW "SDL_JOYSTICK_GAMEINPUT_RAW"
#define SDL_HINT_JOYSTICK_GAMECUBE_DEVICES "SDL_JOYSTICK_GAMECUBE_DEVICES"
#define SDL_HINT_JOYSTICK_GAMECUBE_DEVICES_EXCLUDED "SDL_JOYSTICK_GAMECUBE_DEVICES_EXCLUDED"
#define SDL_HINT_JOYSTICK_HIDAPI "SDL_JOYSTICK_HIDAPI"
#define SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS "SDL_JOYSTICK_HIDAPI_COMBINE_JOY_CONS"
#define SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE "SDL_JOYSTICK_HIDAPI_GAMECUBE"
#define SDL_HINT_JOYSTICK_HIDAPI_GAMECUBE_RUMBLE_BRAKE "SDL_JOYSTICK_HIDAPI_GAMECUBE_RUMBLE_BRAKE"
#define SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS "SDL_JOYSTICK_HIDAPI_JOY_CONS"
#define SDL_HINT_JOYSTICK_HIDAPI_JOYCON_HOME_LED "SDL_JOYSTICK_HIDAPI_JOYCON_HOME_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_LUNA "SDL_JOYSTICK_HIDAPI_LUNA"
#define SDL_HINT_JOYSTICK_HIDAPI_NINTENDO_CLASSIC "SDL_JOYSTICK_HIDAPI_NINTENDO_CLASSIC"
#define SDL_HINT_JOYSTICK_HIDAPI_PS3 "SDL_JOYSTICK_HIDAPI_PS3"
#define SDL_HINT_JOYSTICK_HIDAPI_PS3_SIXAXIS_DRIVER "SDL_JOYSTICK_HIDAPI_PS3_SIXAXIS_DRIVER"
#define SDL_HINT_JOYSTICK_HIDAPI_PS4 "SDL_JOYSTICK_HIDAPI_PS4"
#define SDL_HINT_JOYSTICK_HIDAPI_PS4_REPORT_INTERVAL "SDL_JOYSTICK_HIDAPI_PS4_REPORT_INTERVAL"
#define SDL_HINT_JOYSTICK_HIDAPI_PS5 "SDL_JOYSTICK_HIDAPI_PS5"
#define SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED "SDL_JOYSTICK_HIDAPI_PS5_PLAYER_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_SHIELD "SDL_JOYSTICK_HIDAPI_SHIELD"
#define SDL_HINT_JOYSTICK_HIDAPI_STADIA "SDL_JOYSTICK_HIDAPI_STADIA"
#define SDL_HINT_JOYSTICK_HIDAPI_STEAM "SDL_JOYSTICK_HIDAPI_STEAM"
#define SDL_HINT_JOYSTICK_HIDAPI_STEAM_HOME_LED "SDL_JOYSTICK_HIDAPI_STEAM_HOME_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK "SDL_JOYSTICK_HIDAPI_STEAMDECK"
#define SDL_HINT_JOYSTICK_HIDAPI_STEAM_HORI "SDL_JOYSTICK_HIDAPI_STEAM_HORI"
#define SDL_HINT_JOYSTICK_HIDAPI_LG4FF "SDL_JOYSTICK_HIDAPI_LG4FF"
#define SDL_HINT_JOYSTICK_HIDAPI_8BITDO "SDL_JOYSTICK_HIDAPI_8BITDO"
#define SDL_HINT_JOYSTICK_HIDAPI_SINPUT "SDL_JOYSTICK_HIDAPI_SINPUT"
#define SDL_HINT_JOYSTICK_HIDAPI_ZUIKI "SDL_JOYSTICK_HIDAPI_ZUIKI"
#define SDL_HINT_JOYSTICK_HIDAPI_FLYDIGI "SDL_JOYSTICK_HIDAPI_FLYDIGI"
#define SDL_HINT_JOYSTICK_HIDAPI_SWITCH "SDL_JOYSTICK_HIDAPI_SWITCH"
#define SDL_HINT_JOYSTICK_HIDAPI_SWITCH_HOME_LED "SDL_JOYSTICK_HIDAPI_SWITCH_HOME_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_SWITCH_PLAYER_LED "SDL_JOYSTICK_HIDAPI_SWITCH_PLAYER_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_SWITCH2 "SDL_JOYSTICK_HIDAPI_SWITCH2"
#define SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS "SDL_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS"
#define SDL_HINT_JOYSTICK_HIDAPI_WII "SDL_JOYSTICK_HIDAPI_WII"
#define SDL_HINT_JOYSTICK_HIDAPI_WII_PLAYER_LED "SDL_JOYSTICK_HIDAPI_WII_PLAYER_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_XBOX "SDL_JOYSTICK_HIDAPI_XBOX"
#define SDL_HINT_JOYSTICK_HIDAPI_XBOX_360 "SDL_JOYSTICK_HIDAPI_XBOX_360"
#define SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_PLAYER_LED "SDL_JOYSTICK_HIDAPI_XBOX_360_PLAYER_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_XBOX_360_WIRELESS "SDL_JOYSTICK_HIDAPI_XBOX_360_WIRELESS"
#define SDL_HINT_JOYSTICK_HIDAPI_XBOX_ONE "SDL_JOYSTICK_HIDAPI_XBOX_ONE"
#define SDL_HINT_JOYSTICK_HIDAPI_XBOX_ONE_HOME_LED "SDL_JOYSTICK_HIDAPI_XBOX_ONE_HOME_LED"
#define SDL_HINT_JOYSTICK_HIDAPI_GIP "SDL_JOYSTICK_HIDAPI_GIP"
#define SDL_HINT_JOYSTICK_HIDAPI_GIP_RESET_FOR_METADATA "SDL_JOYSTICK_HIDAPI_GIP_RESET_FOR_METADATA"
#define SDL_HINT_JOYSTICK_IOKIT "SDL_JOYSTICK_IOKIT"
#define SDL_HINT_JOYSTICK_LINUX_CLASSIC "SDL_JOYSTICK_LINUX_CLASSIC"
#define SDL_HINT_JOYSTICK_LINUX_DEADZONES "SDL_JOYSTICK_LINUX_DEADZONES"
#define SDL_HINT_JOYSTICK_LINUX_DIGITAL_HATS "SDL_JOYSTICK_LINUX_DIGITAL_HATS"
#define SDL_HINT_JOYSTICK_LINUX_HAT_DEADZONES "SDL_JOYSTICK_LINUX_HAT_DEADZONES"
#define SDL_HINT_JOYSTICK_MFI "SDL_JOYSTICK_MFI"
#define SDL_HINT_JOYSTICK_RAWINPUT "SDL_JOYSTICK_RAWINPUT"
#define SDL_HINT_JOYSTICK_RAWINPUT_CORRELATE_XINPUT "SDL_JOYSTICK_RAWINPUT_CORRELATE_XINPUT"
#define SDL_HINT_JOYSTICK_ROG_CHAKRAM "SDL_JOYSTICK_ROG_CHAKRAM"
#define SDL_HINT_JOYSTICK_THREAD "SDL_JOYSTICK_THREAD"
#define SDL_HINT_JOYSTICK_THROTTLE_DEVICES "SDL_JOYSTICK_THROTTLE_DEVICES"
#define SDL_HINT_JOYSTICK_THROTTLE_DEVICES_EXCLUDED "SDL_JOYSTICK_THROTTLE_DEVICES_EXCLUDED"
#define SDL_HINT_JOYSTICK_WGI "SDL_JOYSTICK_WGI"
#define SDL_HINT_JOYSTICK_WHEEL_DEVICES "SDL_JOYSTICK_WHEEL_DEVICES"
#define SDL_HINT_JOYSTICK_WHEEL_DEVICES_EXCLUDED "SDL_JOYSTICK_WHEEL_DEVICES_EXCLUDED"
#define SDL_HINT_JOYSTICK_ZERO_CENTERED_DEVICES "SDL_JOYSTICK_ZERO_CENTERED_DEVICES"
#define SDL_HINT_JOYSTICK_HAPTIC_AXES "SDL_JOYSTICK_HAPTIC_AXES"
#define SDL_HINT_KEYCODE_OPTIONS "SDL_KEYCODE_OPTIONS"
#define SDL_HINT_KMSDRM_DEVICE_INDEX "SDL_KMSDRM_DEVICE_INDEX"
#define SDL_HINT_KMSDRM_REQUIRE_DRM_MASTER "SDL_KMSDRM_REQUIRE_DRM_MASTER"
#define SDL_HINT_KMSDRM_ATOMIC "SDL_KMSDRM_ATOMIC"
#define SDL_HINT_LOGGING "SDL_LOGGING"
#define SDL_HINT_MAC_BACKGROUND_APP "SDL_MAC_BACKGROUND_APP"
#define SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK "SDL_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK"
#define SDL_HINT_MAC_OPENGL_ASYNC_DISPATCH "SDL_MAC_OPENGL_ASYNC_DISPATCH"
#define SDL_HINT_MAC_OPTION_AS_ALT "SDL_MAC_OPTION_AS_ALT"
#define SDL_HINT_MAC_SCROLL_MOMENTUM "SDL_MAC_SCROLL_MOMENTUM"
#define SDL_HINT_MAC_PRESS_AND_HOLD "SDL_MAC_PRESS_AND_HOLD"
#define SDL_HINT_MAIN_CALLBACK_RATE "SDL_MAIN_CALLBACK_RATE"
#define SDL_HINT_MOUSE_AUTO_CAPTURE "SDL_MOUSE_AUTO_CAPTURE"
#define SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS "SDL_MOUSE_DOUBLE_CLICK_RADIUS"
#define SDL_HINT_MOUSE_DOUBLE_CLICK_TIME "SDL_MOUSE_DOUBLE_CLICK_TIME"
#define SDL_HINT_MOUSE_DEFAULT_SYSTEM_CURSOR "SDL_MOUSE_DEFAULT_SYSTEM_CURSOR"
#define SDL_HINT_MOUSE_DPI_SCALE_CURSORS "SDL_MOUSE_DPI_SCALE_CURSORS"
#define SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE "SDL_MOUSE_EMULATE_WARP_WITH_RELATIVE"
#define SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH "SDL_MOUSE_FOCUS_CLICKTHROUGH"
#define SDL_HINT_MOUSE_NORMAL_SPEED_SCALE "SDL_MOUSE_NORMAL_SPEED_SCALE"
#define SDL_HINT_MOUSE_RELATIVE_MODE_CENTER "SDL_MOUSE_RELATIVE_MODE_CENTER"
#define SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE "SDL_MOUSE_RELATIVE_SPEED_SCALE"
#define SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE "SDL_MOUSE_RELATIVE_SYSTEM_SCALE"
#define SDL_HINT_MOUSE_RELATIVE_WARP_MOTION "SDL_MOUSE_RELATIVE_WARP_MOTION"
#define SDL_HINT_MOUSE_RELATIVE_CURSOR_VISIBLE "SDL_MOUSE_RELATIVE_CURSOR_VISIBLE"
#define SDL_HINT_MOUSE_TOUCH_EVENTS "SDL_MOUSE_TOUCH_EVENTS"
#define SDL_HINT_MUTE_CONSOLE_KEYBOARD "SDL_MUTE_CONSOLE_KEYBOARD"
#define SDL_HINT_NO_SIGNAL_HANDLERS "SDL_NO_SIGNAL_HANDLERS"
#define SDL_HINT_OPENGL_LIBRARY "SDL_OPENGL_LIBRARY"
#define SDL_HINT_EGL_LIBRARY "SDL_EGL_LIBRARY"
#define SDL_HINT_OPENGL_ES_DRIVER "SDL_OPENGL_ES_DRIVER"
#define SDL_HINT_OPENGL_FORCE_SRGB_FRAMEBUFFER "SDL_OPENGL_FORCE_SRGB_FRAMEBUFFER"
#define SDL_HINT_OPENVR_LIBRARY "SDL_OPENVR_LIBRARY"
#define SDL_HINT_ORIENTATIONS "SDL_ORIENTATIONS"
#define SDL_HINT_POLL_SENTINEL "SDL_POLL_SENTINEL"
#define SDL_HINT_PREFERRED_LOCALES "SDL_PREFERRED_LOCALES"
#define SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE "SDL_QUIT_ON_LAST_WINDOW_CLOSE"
#define SDL_HINT_RENDER_DIRECT3D_THREADSAFE "SDL_RENDER_DIRECT3D_THREADSAFE"
#define SDL_HINT_RENDER_DIRECT3D11_DEBUG "SDL_RENDER_DIRECT3D11_DEBUG"
#define SDL_HINT_RENDER_DIRECT3D11_WARP "SDL_RENDER_DIRECT3D11_WARP"
#define SDL_HINT_RENDER_VULKAN_DEBUG "SDL_RENDER_VULKAN_DEBUG"
#define SDL_HINT_RENDER_GPU_DEBUG "SDL_RENDER_GPU_DEBUG"
#define SDL_HINT_RENDER_GPU_LOW_POWER "SDL_RENDER_GPU_LOW_POWER"
#define SDL_HINT_RENDER_DRIVER "SDL_RENDER_DRIVER"
#define SDL_HINT_RENDER_LINE_METHOD "SDL_RENDER_LINE_METHOD"
#define SDL_HINT_RENDER_METAL_PREFER_LOW_POWER_DEVICE "SDL_RENDER_METAL_PREFER_LOW_POWER_DEVICE"
#define SDL_HINT_RENDER_VSYNC "SDL_RENDER_VSYNC"
#define SDL_HINT_RETURN_KEY_HIDES_IME "SDL_RETURN_KEY_HIDES_IME"
#define SDL_HINT_ROG_GAMEPAD_MICE "SDL_ROG_GAMEPAD_MICE"
#define SDL_HINT_ROG_GAMEPAD_MICE_EXCLUDED "SDL_ROG_GAMEPAD_MICE_EXCLUDED"
#define SDL_HINT_PS2_GS_WIDTH "SDL_PS2_GS_WIDTH"
#define SDL_HINT_PS2_GS_HEIGHT "SDL_PS2_GS_HEIGHT"
#define SDL_HINT_PS2_GS_PROGRESSIVE "SDL_PS2_GS_PROGRESSIVE"
#define SDL_HINT_PS2_GS_MODE "SDL_PS2_GS_MODE"
#define SDL_HINT_RPI_VIDEO_LAYER "SDL_RPI_VIDEO_LAYER"
#define SDL_HINT_SCREENSAVER_INHIBIT_ACTIVITY_NAME "SDL_SCREENSAVER_INHIBIT_ACTIVITY_NAME"
#define SDL_HINT_SHUTDOWN_DBUS_ON_QUIT "SDL_SHUTDOWN_DBUS_ON_QUIT"
#define SDL_HINT_STORAGE_TITLE_DRIVER "SDL_STORAGE_TITLE_DRIVER"
#define SDL_HINT_STORAGE_USER_DRIVER "SDL_STORAGE_USER_DRIVER"
#define SDL_HINT_THREAD_FORCE_REALTIME_TIME_CRITICAL "SDL_THREAD_FORCE_REALTIME_TIME_CRITICAL"
#define SDL_HINT_THREAD_PRIORITY_POLICY "SDL_THREAD_PRIORITY_POLICY"
#define SDL_HINT_TIMER_RESOLUTION "SDL_TIMER_RESOLUTION"
#define SDL_HINT_TOUCH_MOUSE_EVENTS "SDL_TOUCH_MOUSE_EVENTS"
#define SDL_HINT_TRACKPAD_IS_TOUCH_ONLY "SDL_TRACKPAD_IS_TOUCH_ONLY"
#define SDL_HINT_TV_REMOTE_AS_JOYSTICK "SDL_TV_REMOTE_AS_JOYSTICK"
#define SDL_HINT_VIDEO_ALLOW_SCREENSAVER "SDL_VIDEO_ALLOW_SCREENSAVER"
#define SDL_HINT_VIDEO_DISPLAY_PRIORITY "SDL_VIDEO_DISPLAY_PRIORITY"
#define SDL_HINT_VIDEO_DOUBLE_BUFFER "SDL_VIDEO_DOUBLE_BUFFER"
#define SDL_HINT_VIDEO_DRIVER "SDL_VIDEO_DRIVER"
#define SDL_HINT_VIDEO_DUMMY_SAVE_FRAMES "SDL_VIDEO_DUMMY_SAVE_FRAMES"
#define SDL_HINT_VIDEO_EGL_ALLOW_GETDISPLAY_FALLBACK "SDL_VIDEO_EGL_ALLOW_GETDISPLAY_FALLBACK"
#define SDL_HINT_VIDEO_FORCE_EGL "SDL_VIDEO_FORCE_EGL"
#define SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES "SDL_VIDEO_MAC_FULLSCREEN_SPACES"
#define SDL_HINT_VIDEO_MAC_FULLSCREEN_MENU_VISIBILITY "SDL_VIDEO_MAC_FULLSCREEN_MENU_VISIBILITY"
#define SDL_HINT_VIDEO_METAL_AUTO_RESIZE_DRAWABLE "SDL_VIDEO_METAL_AUTO_RESIZE_DRAWABLE"
#define SDL_HINT_VIDEO_MATCH_EXCLUSIVE_MODE_ON_MOVE "SDL_VIDEO_MATCH_EXCLUSIVE_MODE_ON_MOVE"
#define SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS "SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS"
#define SDL_HINT_VIDEO_OFFSCREEN_SAVE_FRAMES "SDL_VIDEO_OFFSCREEN_SAVE_FRAMES"
#define SDL_HINT_VIDEO_SYNC_WINDOW_OPERATIONS "SDL_VIDEO_SYNC_WINDOW_OPERATIONS"
#define SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR "SDL_VIDEO_WAYLAND_ALLOW_LIBDECOR"
#define SDL_HINT_VIDEO_WAYLAND_MODE_EMULATION "SDL_VIDEO_WAYLAND_MODE_EMULATION"
#define SDL_HINT_VIDEO_WAYLAND_MODE_SCALING "SDL_VIDEO_WAYLAND_MODE_SCALING"
#define SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR "SDL_VIDEO_WAYLAND_PREFER_LIBDECOR"
#define SDL_HINT_VIDEO_WAYLAND_SCALE_TO_DISPLAY "SDL_VIDEO_WAYLAND_SCALE_TO_DISPLAY"
#define SDL_HINT_VIDEO_WIN_D3DCOMPILER "SDL_VIDEO_WIN_D3DCOMPILER"
#define SDL_HINT_VIDEO_X11_ENABLE_XSYNC_EXT "SDL_VIDEO_X11_ENABLE_XSYNC_EXT"
#define SDL_HINT_VIDEO_X11_EXTERNAL_WINDOW_INPUT "SDL_VIDEO_X11_EXTERNAL_WINDOW_INPUT"
#define SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR "SDL_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR"
#define SDL_HINT_VIDEO_X11_NET_WM_PING "SDL_VIDEO_X11_NET_WM_PING"
#define SDL_HINT_VIDEO_X11_NODIRECTCOLOR "SDL_VIDEO_X11_NODIRECTCOLOR"
#define SDL_HINT_VIDEO_X11_SCALING_FACTOR "SDL_VIDEO_X11_SCALING_FACTOR"
#define SDL_HINT_VIDEO_X11_VISUALID "SDL_VIDEO_X11_VISUALID"
#define SDL_HINT_VIDEO_X11_WINDOW_VISUALID "SDL_VIDEO_X11_WINDOW_VISUALID"
#define SDL_HINT_VIDEO_X11_XRANDR "SDL_VIDEO_X11_XRANDR"
#define SDL_HINT_VITA_ENABLE_BACK_TOUCH "SDL_VITA_ENABLE_BACK_TOUCH"
#define SDL_HINT_VITA_ENABLE_FRONT_TOUCH "SDL_VITA_ENABLE_FRONT_TOUCH"
#define SDL_HINT_VITA_MODULE_PATH "SDL_VITA_MODULE_PATH"
#define SDL_HINT_VITA_PVR_INIT "SDL_VITA_PVR_INIT"
#define SDL_HINT_VITA_RESOLUTION "SDL_VITA_RESOLUTION"
#define SDL_HINT_VITA_PVR_OPENGL "SDL_VITA_PVR_OPENGL"
#define SDL_HINT_VITA_TOUCH_MOUSE_DEVICE "SDL_VITA_TOUCH_MOUSE_DEVICE"
#define SDL_HINT_VULKAN_DISPLAY "SDL_VULKAN_DISPLAY"
#define SDL_HINT_VULKAN_LIBRARY "SDL_VULKAN_LIBRARY"
#define SDL_HINT_WAVE_FACT_CHUNK "SDL_WAVE_FACT_CHUNK"
#define SDL_HINT_WAVE_CHUNK_LIMIT "SDL_WAVE_CHUNK_LIMIT"
#define SDL_HINT_WAVE_RIFF_CHUNK_SIZE "SDL_WAVE_RIFF_CHUNK_SIZE"
#define SDL_HINT_WAVE_TRUNCATION "SDL_WAVE_TRUNCATION"
#define SDL_HINT_WINDOW_ACTIVATE_WHEN_RAISED "SDL_WINDOW_ACTIVATE_WHEN_RAISED"
#define SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN "SDL_WINDOW_ACTIVATE_WHEN_SHOWN"
#define SDL_HINT_WINDOW_ALLOW_TOPMOST "SDL_WINDOW_ALLOW_TOPMOST"
#define SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN "SDL_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN"
#define SDL_HINT_WINDOWS_CLOSE_ON_ALT_F4 "SDL_WINDOWS_CLOSE_ON_ALT_F4"
#define SDL_HINT_WINDOWS_ENABLE_MENU_MNEMONICS "SDL_WINDOWS_ENABLE_MENU_MNEMONICS"
#define SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP "SDL_WINDOWS_ENABLE_MESSAGELOOP"
#define SDL_HINT_WINDOWS_GAMEINPUT "SDL_WINDOWS_GAMEINPUT"
#define SDL_HINT_WINDOWS_RAW_KEYBOARD "SDL_WINDOWS_RAW_KEYBOARD"
#define SDL_HINT_WINDOWS_RAW_KEYBOARD_EXCLUDE_HOTKEYS "SDL_WINDOWS_RAW_KEYBOARD_EXCLUDE_HOTKEYS"
#define SDL_HINT_WINDOWS_RAW_KEYBOARD_INPUTSINK "SDL_WINDOWS_RAW_KEYBOARD_INPUTSINK"
#define SDL_HINT_WINDOWS_FORCE_SEMAPHORE_KERNEL "SDL_WINDOWS_FORCE_SEMAPHORE_KERNEL"
#define SDL_HINT_WINDOWS_INTRESOURCE_ICON "SDL_WINDOWS_INTRESOURCE_ICON"
#define SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL "SDL_WINDOWS_INTRESOURCE_ICON_SMALL"
#define SDL_HINT_WINDOWS_USE_D3D9EX "SDL_WINDOWS_USE_D3D9EX"
#define SDL_HINT_WINDOWS_ERASE_BACKGROUND_MODE "SDL_WINDOWS_ERASE_BACKGROUND_MODE"
#define SDL_HINT_X11_FORCE_OVERRIDE_REDIRECT "SDL_X11_FORCE_OVERRIDE_REDIRECT"
#define SDL_HINT_X11_WINDOW_TYPE "SDL_X11_WINDOW_TYPE"
#define SDL_HINT_X11_XCB_LIBRARY "SDL_X11_XCB_LIBRARY"
#define SDL_HINT_XINPUT_ENABLED "SDL_XINPUT_ENABLED"
#define SDL_HINT_ASSERT "SDL_ASSERT"
#define SDL_HINT_PEN_MOUSE_EVENTS "SDL_PEN_MOUSE_EVENTS"
#define SDL_HINT_PEN_TOUCH_EVENTS "SDL_PEN_TOUCH_EVENTS"

type SDL_HintPriority as long
enum
	SDL_HINT_DEFAULT
	SDL_HINT_NORMAL
	SDL_HINT_OVERRIDE
end enum

declare function SDL_SetHintWithPriority(byval name as const zstring ptr, byval value as const zstring ptr, byval priority as SDL_HintPriority) as boolean
declare function SDL_SetHint(byval name as const zstring ptr, byval value as const zstring ptr) as boolean
declare function SDL_ResetHint(byval name as const zstring ptr) as boolean
declare sub SDL_ResetHints()
declare function SDL_GetHint(byval name as const zstring ptr) as const zstring ptr
declare function SDL_GetHintBoolean(byval name as const zstring ptr, byval default_value as boolean) as boolean
type SDL_HintCallback as sub(byval userdata as any ptr, byval name as const zstring ptr, byval oldValue as const zstring ptr, byval newValue as const zstring ptr)
declare function SDL_AddHintCallback(byval name as const zstring ptr, byval callback as SDL_HintCallback, byval userdata as any ptr) as boolean
declare sub SDL_RemoveHintCallback(byval name as const zstring ptr, byval callback as SDL_HintCallback, byval userdata as any ptr)
'' -------------------------------------------------------------------------
'' SDL_init.h
'' -------------------------------------------------------------------------
type SDL_InitFlags as Uint32

const SDL_INIT_AUDIO = &h00000010u
const SDL_INIT_VIDEO = &h00000020u
const SDL_INIT_JOYSTICK = &h00000200u
const SDL_INIT_HAPTIC = &h00001000u
const SDL_INIT_GAMEPAD = &h00002000u
const SDL_INIT_EVENTS = &h00004000u
const SDL_INIT_SENSOR = &h00008000u
const SDL_INIT_CAMERA = &h00010000u

type SDL_AppResult as long
enum
	SDL_APP_CONTINUE
	SDL_APP_SUCCESS
	SDL_APP_FAILURE
end enum

type SDL_AppInit_func as function(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
type SDL_AppIterate_func as function(byval appstate as any ptr) as SDL_AppResult
type SDL_AppEvent_func as function(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
type SDL_AppQuit_func as sub(byval appstate as any ptr, byval result as SDL_AppResult)

declare function SDL_Init(byval flags as SDL_InitFlags) as boolean
declare function SDL_InitSubSystem(byval flags as SDL_InitFlags) as boolean
declare sub SDL_QuitSubSystem(byval flags as SDL_InitFlags)
declare function SDL_WasInit(byval flags as SDL_InitFlags) as SDL_InitFlags
declare sub SDL_Quit()
declare function SDL_IsMainThread() as boolean
type SDL_MainThreadCallback as sub(byval userdata as any ptr)
declare function SDL_RunOnMainThread(byval callback as SDL_MainThreadCallback, byval userdata as any ptr, byval wait_complete as boolean) as boolean
declare function SDL_SetAppMetadata(byval appname as const zstring ptr, byval appversion as const zstring ptr, byval appidentifier as const zstring ptr) as boolean
declare function SDL_SetAppMetadataProperty(byval name as const zstring ptr, byval value as const zstring ptr) as boolean

#define SDL_PROP_APP_METADATA_NAME_STRING "SDL.app.metadata.name"
#define SDL_PROP_APP_METADATA_VERSION_STRING "SDL.app.metadata.version"
#define SDL_PROP_APP_METADATA_IDENTIFIER_STRING "SDL.app.metadata.identifier"
#define SDL_PROP_APP_METADATA_CREATOR_STRING "SDL.app.metadata.creator"
#define SDL_PROP_APP_METADATA_COPYRIGHT_STRING "SDL.app.metadata.copyright"
#define SDL_PROP_APP_METADATA_URL_STRING "SDL.app.metadata.url"
#define SDL_PROP_APP_METADATA_TYPE_STRING "SDL.app.metadata.type"
declare function SDL_GetAppMetadataProperty(byval name as const zstring ptr) as const zstring ptr
'' -------------------------------------------------------------------------
'' SDL_loadso.h
'' -------------------------------------------------------------------------
declare function SDL_LoadObject(byval sofile as const zstring ptr) as SDL_SharedObject ptr
declare function SDL_LoadFunction(byval handle as SDL_SharedObject ptr, byval name as const zstring ptr) as SDL_FunctionPointer
declare sub SDL_UnloadObject(byval handle as SDL_SharedObject ptr)
#define SDL_locale_h

type SDL_Locale
	language as const zstring ptr
	country as const zstring ptr
end type

declare function SDL_GetPreferredLocales(byval count as long ptr) as SDL_Locale ptr ptr
'' -------------------------------------------------------------------------
'' SDL_log.h
'' -------------------------------------------------------------------------
type SDL_LogCategory as long
enum
	SDL_LOG_CATEGORY_APPLICATION
	SDL_LOG_CATEGORY_ERROR
	SDL_LOG_CATEGORY_ASSERT
	SDL_LOG_CATEGORY_SYSTEM
	SDL_LOG_CATEGORY_AUDIO
	SDL_LOG_CATEGORY_VIDEO
	SDL_LOG_CATEGORY_RENDER
	SDL_LOG_CATEGORY_INPUT
	SDL_LOG_CATEGORY_TEST
	SDL_LOG_CATEGORY_GPU
	SDL_LOG_CATEGORY_RESERVED2
	SDL_LOG_CATEGORY_RESERVED3
	SDL_LOG_CATEGORY_RESERVED4
	SDL_LOG_CATEGORY_RESERVED5
	SDL_LOG_CATEGORY_RESERVED6
	SDL_LOG_CATEGORY_RESERVED7
	SDL_LOG_CATEGORY_RESERVED8
	SDL_LOG_CATEGORY_RESERVED9
	SDL_LOG_CATEGORY_RESERVED10
	SDL_LOG_CATEGORY_CUSTOM
end enum

type SDL_LogPriority as long
enum
	SDL_LOG_PRIORITY_INVALID
	SDL_LOG_PRIORITY_TRACE
	SDL_LOG_PRIORITY_VERBOSE
	SDL_LOG_PRIORITY_DEBUG
	SDL_LOG_PRIORITY_INFO
	SDL_LOG_PRIORITY_WARN
	SDL_LOG_PRIORITY_ERROR
	SDL_LOG_PRIORITY_CRITICAL
	SDL_LOG_PRIORITY_COUNT
end enum

declare sub SDL_SetLogPriorities(byval priority as SDL_LogPriority)
declare sub SDL_SetLogPriority(byval category as long, byval priority as SDL_LogPriority)
declare function SDL_GetLogPriority(byval category as long) as SDL_LogPriority
declare sub SDL_ResetLogPriorities()
declare function SDL_SetLogPriorityPrefix(byval priority as SDL_LogPriority, byval prefix as const zstring ptr) as boolean
declare sub SDL_Log_ alias "SDL_Log"(byval fmt as const zstring ptr, ...)
declare sub SDL_LogTrace(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogVerbose(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogDebug(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogInfo(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogWarn(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogError(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogCritical(byval category as long, byval fmt as const zstring ptr, ...)
declare sub SDL_LogMessage(byval category as long, byval priority as SDL_LogPriority, byval fmt as const zstring ptr, ...)
declare sub SDL_LogMessageV(byval category as long, byval priority as SDL_LogPriority, byval fmt as const zstring ptr, byval ap as va_list)
type SDL_LogOutputFunction as sub(byval userdata as any ptr, byval category as long, byval priority as SDL_LogPriority, byval message as const zstring ptr)
declare function SDL_GetDefaultLogOutputFunction() as SDL_LogOutputFunction
declare sub SDL_GetLogOutputFunction(byval callback as SDL_LogOutputFunction ptr, byval userdata as any ptr ptr)
declare sub SDL_SetLogOutputFunction(byval callback as SDL_LogOutputFunction, byval userdata as any ptr)
'' -------------------------------------------------------------------------
'' SDL_messagebox.h
'' -------------------------------------------------------------------------
type SDL_MessageBoxFlags as Uint32

const SDL_MESSAGEBOX_ERROR = &h00000010u
const SDL_MESSAGEBOX_WARNING = &h00000020u
const SDL_MESSAGEBOX_INFORMATION = &h00000040u
const SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT = &h00000080u
const SDL_MESSAGEBOX_BUTTONS_RIGHT_TO_LEFT = &h00000100u
type SDL_MessageBoxButtonFlags as Uint32
const SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT = &h00000001u
const SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT = &h00000002u

type SDL_MessageBoxButtonData
	flags as SDL_MessageBoxButtonFlags
	buttonID as long
	text as const zstring ptr
end type

type SDL_MessageBoxColor
	r as Uint8
	g as Uint8
	b as Uint8
end type

type SDL_MessageBoxColorType as long
enum
	SDL_MESSAGEBOX_COLOR_BACKGROUND
	SDL_MESSAGEBOX_COLOR_TEXT
	SDL_MESSAGEBOX_COLOR_BUTTON_BORDER
	SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND
	SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED
	SDL_MESSAGEBOX_COLOR_COUNT
end enum

type SDL_MessageBoxColorScheme
	colors(0 to SDL_MESSAGEBOX_COLOR_COUNT - 1) as SDL_MessageBoxColor
end type

type SDL_MessageBoxData
	flags as SDL_MessageBoxFlags
	window as SDL_Window ptr
	title as const zstring ptr
	message as const zstring ptr
	numbuttons as long
	buttons as const SDL_MessageBoxButtonData ptr
	colorScheme as const SDL_MessageBoxColorScheme ptr
end type

declare function SDL_ShowMessageBox(byval messageboxdata as const SDL_MessageBoxData ptr, byval buttonid as long ptr) as boolean
declare function SDL_ShowSimpleMessageBox(byval flags as SDL_MessageBoxFlags, byval title as const zstring ptr, byval message as const zstring ptr, byval window as SDL_Window ptr) as boolean
'' -------------------------------------------------------------------------
'' SDL_metal.h
'' -------------------------------------------------------------------------
type SDL_MetalView as any ptr
declare function SDL_Metal_CreateView(byval window as SDL_Window ptr) as SDL_MetalView
declare sub SDL_Metal_DestroyView(byval view as SDL_MetalView)
declare function SDL_Metal_GetLayer(byval view as SDL_MetalView) as any ptr
'' -------------------------------------------------------------------------
'' SDL_misc.h
'' -------------------------------------------------------------------------
declare function SDL_OpenURL(byval url as const zstring ptr) as boolean
'' -------------------------------------------------------------------------
'' SDL_platform.h
'' -------------------------------------------------------------------------
declare function SDL_GetPlatform() as const zstring ptr
'' -------------------------------------------------------------------------
'' SDL_process.h
'' -------------------------------------------------------------------------
declare function SDL_CreateProcess(byval args as const zstring const ptr ptr, byval pipe_stdio as boolean) as SDL_Process ptr

type SDL_ProcessIO as long
enum
	SDL_PROCESS_STDIO_INHERITED
	SDL_PROCESS_STDIO_NULL
	SDL_PROCESS_STDIO_APP
	SDL_PROCESS_STDIO_REDIRECT
end enum

declare function SDL_CreateProcessWithProperties(byval props as SDL_PropertiesID) as SDL_Process ptr
#define SDL_PROP_PROCESS_CREATE_ARGS_POINTER "SDL.process.create.args"
#define SDL_PROP_PROCESS_CREATE_ENVIRONMENT_POINTER "SDL.process.create.environment"
#define SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING "SDL.process.create.working_directory"
#define SDL_PROP_PROCESS_CREATE_STDIN_NUMBER "SDL.process.create.stdin_option"
#define SDL_PROP_PROCESS_CREATE_STDIN_POINTER "SDL.process.create.stdin_source"
#define SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER "SDL.process.create.stdout_option"
#define SDL_PROP_PROCESS_CREATE_STDOUT_POINTER "SDL.process.create.stdout_source"
#define SDL_PROP_PROCESS_CREATE_STDERR_NUMBER "SDL.process.create.stderr_option"
#define SDL_PROP_PROCESS_CREATE_STDERR_POINTER "SDL.process.create.stderr_source"
#define SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN "SDL.process.create.stderr_to_stdout"
#define SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN "SDL.process.create.background"
#define SDL_PROP_PROCESS_CREATE_CMDLINE_STRING "SDL.process.create.cmdline"
declare function SDL_GetProcessProperties(byval process as SDL_Process ptr) as SDL_PropertiesID
#define SDL_PROP_PROCESS_PID_NUMBER "SDL.process.pid"
#define SDL_PROP_PROCESS_STDIN_POINTER "SDL.process.stdin"
#define SDL_PROP_PROCESS_STDOUT_POINTER "SDL.process.stdout"
#define SDL_PROP_PROCESS_STDERR_POINTER "SDL.process.stderr"
#define SDL_PROP_PROCESS_BACKGROUND_BOOLEAN "SDL.process.background"

declare function SDL_ReadProcess(byval process as SDL_Process ptr, byval datasize as uinteger ptr, byval exitcode as long ptr) as any ptr
declare function SDL_GetProcessInput(byval process as SDL_Process ptr) as SDL_IOStream ptr
declare function SDL_GetProcessOutput(byval process as SDL_Process ptr) as SDL_IOStream ptr
declare function SDL_KillProcess(byval process as SDL_Process ptr, byval force as boolean) as boolean
declare function SDL_WaitProcess(byval process as SDL_Process ptr, byval block as boolean, byval exitcode as long ptr) as boolean
declare sub SDL_DestroyProcess(byval process as SDL_Process ptr)

'' -------------------------------------------------------------------------
'' SDL_render.h
'' -------------------------------------------------------------------------
#define SDL_SOFTWARE_RENDERER "software"
#define SDL_GPU_RENDERER "gpu"

type SDL_Vertex
	position as SDL_FPoint
	color as SDL_FColor
	tex_coord as SDL_FPoint
end type

type SDL_TextureAccess as long
enum
	SDL_TEXTUREACCESS_STATIC
	SDL_TEXTUREACCESS_STREAMING
	SDL_TEXTUREACCESS_TARGET
end enum

type SDL_TextureAddressMode as long
enum
	SDL_TEXTURE_ADDRESS_INVALID = -1
	SDL_TEXTURE_ADDRESS_AUTO
	SDL_TEXTURE_ADDRESS_CLAMP
	SDL_TEXTURE_ADDRESS_WRAP
end enum

type SDL_RendererLogicalPresentation as long
enum
	SDL_LOGICAL_PRESENTATION_DISABLED
	SDL_LOGICAL_PRESENTATION_STRETCH
	SDL_LOGICAL_PRESENTATION_LETTERBOX
	SDL_LOGICAL_PRESENTATION_OVERSCAN
	SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
end enum

type SDL_Texture
	format as SDL_PixelFormat
	w as long
	h as long
	refcount as long
end type

declare function SDL_GetNumRenderDrivers() as long
declare function SDL_GetRenderDriver(byval index as long) as const zstring ptr
declare function SDL_CreateWindowAndRenderer(byval title as const zstring ptr, byval width as long, byval height as long, byval window_flags as SDL_WindowFlags, byval window as SDL_Window ptr ptr, byval renderer as SDL_Renderer ptr ptr) as boolean
declare function SDL_CreateRenderer(byval window as SDL_Window ptr, byval name as const zstring ptr) as SDL_Renderer ptr
declare function SDL_CreateRendererWithProperties(byval props as SDL_PropertiesID) as SDL_Renderer ptr

#define SDL_PROP_RENDERER_CREATE_NAME_STRING "SDL.renderer.create.name"
#define SDL_PROP_RENDERER_CREATE_WINDOW_POINTER "SDL.renderer.create.window"
#define SDL_PROP_RENDERER_CREATE_SURFACE_POINTER "SDL.renderer.create.surface"
#define SDL_PROP_RENDERER_CREATE_OUTPUT_COLORSPACE_NUMBER "SDL.renderer.create.output_colorspace"
#define SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER "SDL.renderer.create.present_vsync"
#define SDL_PROP_RENDERER_CREATE_GPU_DEVICE_POINTER "SDL.renderer.create.gpu.device"
#define SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN "SDL.renderer.create.gpu.shaders_spirv"
#define SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN "SDL.renderer.create.gpu.shaders_dxil"
#define SDL_PROP_RENDERER_CREATE_GPU_SHADERS_MSL_BOOLEAN "SDL.renderer.create.gpu.shaders_msl"
#define SDL_PROP_RENDERER_CREATE_VULKAN_INSTANCE_POINTER "SDL.renderer.create.vulkan.instance"
#define SDL_PROP_RENDERER_CREATE_VULKAN_SURFACE_NUMBER "SDL.renderer.create.vulkan.surface"
#define SDL_PROP_RENDERER_CREATE_VULKAN_PHYSICAL_DEVICE_POINTER "SDL.renderer.create.vulkan.physical_device"
#define SDL_PROP_RENDERER_CREATE_VULKAN_DEVICE_POINTER "SDL.renderer.create.vulkan.device"
#define SDL_PROP_RENDERER_CREATE_VULKAN_GRAPHICS_QUEUE_FAMILY_INDEX_NUMBER "SDL.renderer.create.vulkan.graphics_queue_family_index"
#define SDL_PROP_RENDERER_CREATE_VULKAN_PRESENT_QUEUE_FAMILY_INDEX_NUMBER "SDL.renderer.create.vulkan.present_queue_family_index"

declare function SDL_CreateGPURenderer(byval device as SDL_GPUDevice ptr, byval window as SDL_Window ptr) as SDL_Renderer ptr
declare function SDL_GetGPURendererDevice(byval renderer as SDL_Renderer ptr) as SDL_GPUDevice ptr
declare function SDL_CreateSoftwareRenderer(byval surface as SDL_Surface ptr) as SDL_Renderer ptr
declare function SDL_GetRenderer(byval window as SDL_Window ptr) as SDL_Renderer ptr
declare function SDL_GetRenderWindow(byval renderer as SDL_Renderer ptr) as SDL_Window ptr
declare function SDL_GetRendererName(byval renderer as SDL_Renderer ptr) as const zstring ptr
declare function SDL_GetRendererProperties(byval renderer as SDL_Renderer ptr) as SDL_PropertiesID

#define SDL_PROP_RENDERER_NAME_STRING "SDL.renderer.name"
#define SDL_PROP_RENDERER_WINDOW_POINTER "SDL.renderer.window"
#define SDL_PROP_RENDERER_SURFACE_POINTER "SDL.renderer.surface"
#define SDL_PROP_RENDERER_VSYNC_NUMBER "SDL.renderer.vsync"
#define SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER "SDL.renderer.max_texture_size"
#define SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER "SDL.renderer.texture_formats"
#define SDL_PROP_RENDERER_TEXTURE_WRAPPING_BOOLEAN "SDL.renderer.texture_wrapping"
#define SDL_PROP_RENDERER_OUTPUT_COLORSPACE_NUMBER "SDL.renderer.output_colorspace"
#define SDL_PROP_RENDERER_HDR_ENABLED_BOOLEAN "SDL.renderer.HDR_enabled"
#define SDL_PROP_RENDERER_SDR_WHITE_POINT_FLOAT "SDL.renderer.SDR_white_point"
#define SDL_PROP_RENDERER_HDR_HEADROOM_FLOAT "SDL.renderer.HDR_headroom"
#define SDL_PROP_RENDERER_D3D9_DEVICE_POINTER "SDL.renderer.d3d9.device"
#define SDL_PROP_RENDERER_D3D11_DEVICE_POINTER "SDL.renderer.d3d11.device"
#define SDL_PROP_RENDERER_D3D11_SWAPCHAIN_POINTER "SDL.renderer.d3d11.swap_chain"
#define SDL_PROP_RENDERER_D3D12_DEVICE_POINTER "SDL.renderer.d3d12.device"
#define SDL_PROP_RENDERER_D3D12_SWAPCHAIN_POINTER "SDL.renderer.d3d12.swap_chain"
#define SDL_PROP_RENDERER_D3D12_COMMAND_QUEUE_POINTER "SDL.renderer.d3d12.command_queue"
#define SDL_PROP_RENDERER_VULKAN_INSTANCE_POINTER "SDL.renderer.vulkan.instance"
#define SDL_PROP_RENDERER_VULKAN_SURFACE_NUMBER "SDL.renderer.vulkan.surface"
#define SDL_PROP_RENDERER_VULKAN_PHYSICAL_DEVICE_POINTER "SDL.renderer.vulkan.physical_device"
#define SDL_PROP_RENDERER_VULKAN_DEVICE_POINTER "SDL.renderer.vulkan.device"
#define SDL_PROP_RENDERER_VULKAN_GRAPHICS_QUEUE_FAMILY_INDEX_NUMBER "SDL.renderer.vulkan.graphics_queue_family_index"
#define SDL_PROP_RENDERER_VULKAN_PRESENT_QUEUE_FAMILY_INDEX_NUMBER "SDL.renderer.vulkan.present_queue_family_index"
#define SDL_PROP_RENDERER_VULKAN_SWAPCHAIN_IMAGE_COUNT_NUMBER "SDL.renderer.vulkan.swapchain_image_count"
#define SDL_PROP_RENDERER_GPU_DEVICE_POINTER "SDL.renderer.gpu.device"

declare function SDL_GetRenderOutputSize(byval renderer as SDL_Renderer ptr, byval w as long ptr, byval h as long ptr) as boolean
declare function SDL_GetCurrentRenderOutputSize(byval renderer as SDL_Renderer ptr, byval w as long ptr, byval h as long ptr) as boolean
declare function SDL_CreateTexture(byval renderer as SDL_Renderer ptr, byval format as SDL_PixelFormat, byval access as SDL_TextureAccess, byval w as long, byval h as long) as SDL_Texture ptr
declare function SDL_CreateTextureFromSurface(byval renderer as SDL_Renderer ptr, byval surface as SDL_Surface ptr) as SDL_Texture ptr
declare function SDL_CreateTextureWithProperties(byval renderer as SDL_Renderer ptr, byval props as SDL_PropertiesID) as SDL_Texture ptr

#define SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER "SDL.texture.create.colorspace"
#define SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER "SDL.texture.create.format"
#define SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER "SDL.texture.create.access"
#define SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER "SDL.texture.create.width"
#define SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER "SDL.texture.create.height"
#define SDL_PROP_TEXTURE_CREATE_PALETTE_POINTER "SDL.texture.create.palette"
#define SDL_PROP_TEXTURE_CREATE_SDR_WHITE_POINT_FLOAT "SDL.texture.create.SDR_white_point"
#define SDL_PROP_TEXTURE_CREATE_HDR_HEADROOM_FLOAT "SDL.texture.create.HDR_headroom"
#define SDL_PROP_TEXTURE_CREATE_D3D11_TEXTURE_POINTER "SDL.texture.create.d3d11.texture"
#define SDL_PROP_TEXTURE_CREATE_D3D11_TEXTURE_U_POINTER "SDL.texture.create.d3d11.texture_u"
#define SDL_PROP_TEXTURE_CREATE_D3D11_TEXTURE_V_POINTER "SDL.texture.create.d3d11.texture_v"
#define SDL_PROP_TEXTURE_CREATE_D3D12_TEXTURE_POINTER "SDL.texture.create.d3d12.texture"
#define SDL_PROP_TEXTURE_CREATE_D3D12_TEXTURE_U_POINTER "SDL.texture.create.d3d12.texture_u"
#define SDL_PROP_TEXTURE_CREATE_D3D12_TEXTURE_V_POINTER "SDL.texture.create.d3d12.texture_v"
#define SDL_PROP_TEXTURE_CREATE_METAL_PIXELBUFFER_POINTER "SDL.texture.create.metal.pixelbuffer"
#define SDL_PROP_TEXTURE_CREATE_OPENGL_TEXTURE_NUMBER "SDL.texture.create.opengl.texture"
#define SDL_PROP_TEXTURE_CREATE_OPENGL_TEXTURE_UV_NUMBER "SDL.texture.create.opengl.texture_uv"
#define SDL_PROP_TEXTURE_CREATE_OPENGL_TEXTURE_U_NUMBER "SDL.texture.create.opengl.texture_u"
#define SDL_PROP_TEXTURE_CREATE_OPENGL_TEXTURE_V_NUMBER "SDL.texture.create.opengl.texture_v"
#define SDL_PROP_TEXTURE_CREATE_OPENGLES2_TEXTURE_NUMBER "SDL.texture.create.opengles2.texture"
#define SDL_PROP_TEXTURE_CREATE_OPENGLES2_TEXTURE_UV_NUMBER "SDL.texture.create.opengles2.texture_uv"
#define SDL_PROP_TEXTURE_CREATE_OPENGLES2_TEXTURE_U_NUMBER "SDL.texture.create.opengles2.texture_u"
#define SDL_PROP_TEXTURE_CREATE_OPENGLES2_TEXTURE_V_NUMBER "SDL.texture.create.opengles2.texture_v"
#define SDL_PROP_TEXTURE_CREATE_VULKAN_TEXTURE_NUMBER "SDL.texture.create.vulkan.texture"
#define SDL_PROP_TEXTURE_CREATE_VULKAN_LAYOUT_NUMBER "SDL.texture.create.vulkan.layout"
#define SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER "SDL.texture.create.gpu.texture"
#define SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_UV_POINTER "SDL.texture.create.gpu.texture_uv"
#define SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_U_POINTER "SDL.texture.create.gpu.texture_u"
#define SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_V_POINTER "SDL.texture.create.gpu.texture_v"
declare function SDL_GetTextureProperties(byval texture as SDL_Texture ptr) as SDL_PropertiesID
#define SDL_PROP_TEXTURE_COLORSPACE_NUMBER "SDL.texture.colorspace"
#define SDL_PROP_TEXTURE_FORMAT_NUMBER "SDL.texture.format"
#define SDL_PROP_TEXTURE_ACCESS_NUMBER "SDL.texture.access"
#define SDL_PROP_TEXTURE_WIDTH_NUMBER "SDL.texture.width"
#define SDL_PROP_TEXTURE_HEIGHT_NUMBER "SDL.texture.height"
#define SDL_PROP_TEXTURE_SDR_WHITE_POINT_FLOAT "SDL.texture.SDR_white_point"
#define SDL_PROP_TEXTURE_HDR_HEADROOM_FLOAT "SDL.texture.HDR_headroom"
#define SDL_PROP_TEXTURE_D3D11_TEXTURE_POINTER "SDL.texture.d3d11.texture"
#define SDL_PROP_TEXTURE_D3D11_TEXTURE_U_POINTER "SDL.texture.d3d11.texture_u"
#define SDL_PROP_TEXTURE_D3D11_TEXTURE_V_POINTER "SDL.texture.d3d11.texture_v"
#define SDL_PROP_TEXTURE_D3D12_TEXTURE_POINTER "SDL.texture.d3d12.texture"
#define SDL_PROP_TEXTURE_D3D12_TEXTURE_U_POINTER "SDL.texture.d3d12.texture_u"
#define SDL_PROP_TEXTURE_D3D12_TEXTURE_V_POINTER "SDL.texture.d3d12.texture_v"
#define SDL_PROP_TEXTURE_OPENGL_TEXTURE_NUMBER "SDL.texture.opengl.texture"
#define SDL_PROP_TEXTURE_OPENGL_TEXTURE_UV_NUMBER "SDL.texture.opengl.texture_uv"
#define SDL_PROP_TEXTURE_OPENGL_TEXTURE_U_NUMBER "SDL.texture.opengl.texture_u"
#define SDL_PROP_TEXTURE_OPENGL_TEXTURE_V_NUMBER "SDL.texture.opengl.texture_v"
#define SDL_PROP_TEXTURE_OPENGL_TEXTURE_TARGET_NUMBER "SDL.texture.opengl.target"
#define SDL_PROP_TEXTURE_OPENGL_TEX_W_FLOAT "SDL.texture.opengl.tex_w"
#define SDL_PROP_TEXTURE_OPENGL_TEX_H_FLOAT "SDL.texture.opengl.tex_h"
#define SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_NUMBER "SDL.texture.opengles2.texture"
#define SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_UV_NUMBER "SDL.texture.opengles2.texture_uv"
#define SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_U_NUMBER "SDL.texture.opengles2.texture_u"
#define SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_V_NUMBER "SDL.texture.opengles2.texture_v"
#define SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_TARGET_NUMBER "SDL.texture.opengles2.target"
#define SDL_PROP_TEXTURE_VULKAN_TEXTURE_NUMBER "SDL.texture.vulkan.texture"
#define SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER "SDL.texture.gpu.texture"
#define SDL_PROP_TEXTURE_GPU_TEXTURE_UV_POINTER "SDL.texture.gpu.texture_uv"
#define SDL_PROP_TEXTURE_GPU_TEXTURE_U_POINTER "SDL.texture.gpu.texture_u"
#define SDL_PROP_TEXTURE_GPU_TEXTURE_V_POINTER "SDL.texture.gpu.texture_v"

declare function SDL_GetRendererFromTexture(byval texture as SDL_Texture ptr) as SDL_Renderer ptr
declare function SDL_GetTextureSize(byval texture as SDL_Texture ptr, byval w as single ptr, byval h as single ptr) as boolean
declare function SDL_SetTexturePalette(byval texture as SDL_Texture ptr, byval palette as SDL_Palette ptr) as boolean
declare function SDL_GetTexturePalette(byval texture as SDL_Texture ptr) as SDL_Palette ptr
declare function SDL_SetTextureColorMod(byval texture as SDL_Texture ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8) as boolean
declare function SDL_SetTextureColorModFloat(byval texture as SDL_Texture ptr, byval r as single, byval g as single, byval b as single) as boolean
declare function SDL_GetTextureColorMod(byval texture as SDL_Texture ptr, byval r as Uint8 ptr, byval g as Uint8 ptr, byval b as Uint8 ptr) as boolean
declare function SDL_GetTextureColorModFloat(byval texture as SDL_Texture ptr, byval r as single ptr, byval g as single ptr, byval b as single ptr) as boolean
declare function SDL_SetTextureAlphaMod(byval texture as SDL_Texture ptr, byval alpha as Uint8) as boolean
declare function SDL_SetTextureAlphaModFloat(byval texture as SDL_Texture ptr, byval alpha as single) as boolean
declare function SDL_GetTextureAlphaMod(byval texture as SDL_Texture ptr, byval alpha as Uint8 ptr) as boolean
declare function SDL_GetTextureAlphaModFloat(byval texture as SDL_Texture ptr, byval alpha as single ptr) as boolean
declare function SDL_SetTextureBlendMode(byval texture as SDL_Texture ptr, byval blendMode as SDL_BlendMode) as boolean
declare function SDL_GetTextureBlendMode(byval texture as SDL_Texture ptr, byval blendMode as SDL_BlendMode ptr) as boolean
declare function SDL_SetTextureScaleMode(byval texture as SDL_Texture ptr, byval scaleMode as SDL_ScaleMode) as boolean
declare function SDL_GetTextureScaleMode(byval texture as SDL_Texture ptr, byval scaleMode as SDL_ScaleMode ptr) as boolean
declare function SDL_UpdateTexture(byval texture as SDL_Texture ptr, byval rect as const SDL_Rect ptr, byval pixels as const any ptr, byval pitch as long) as boolean
declare function SDL_UpdateYUVTexture(byval texture as SDL_Texture ptr, byval rect as const SDL_Rect ptr, byval Yplane as const Uint8 ptr, byval Ypitch as long, byval Uplane as const Uint8 ptr, byval Upitch as long, byval Vplane as const Uint8 ptr, byval Vpitch as long) as boolean
declare function SDL_UpdateNVTexture(byval texture as SDL_Texture ptr, byval rect as const SDL_Rect ptr, byval Yplane as const Uint8 ptr, byval Ypitch as long, byval UVplane as const Uint8 ptr, byval UVpitch as long) as boolean
declare function SDL_LockTexture(byval texture as SDL_Texture ptr, byval rect as const SDL_Rect ptr, byval pixels as any ptr ptr, byval pitch as long ptr) as boolean
declare function SDL_LockTextureToSurface(byval texture as SDL_Texture ptr, byval rect as const SDL_Rect ptr, byval surface as SDL_Surface ptr ptr) as boolean
declare sub SDL_UnlockTexture(byval texture as SDL_Texture ptr)
declare function SDL_SetRenderTarget(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr) as boolean
declare function SDL_GetRenderTarget(byval renderer as SDL_Renderer ptr) as SDL_Texture ptr
declare function SDL_SetRenderLogicalPresentation(byval renderer as SDL_Renderer ptr, byval w as long, byval h as long, byval mode as SDL_RendererLogicalPresentation) as boolean
declare function SDL_GetRenderLogicalPresentation(byval renderer as SDL_Renderer ptr, byval w as long ptr, byval h as long ptr, byval mode as SDL_RendererLogicalPresentation ptr) as boolean
declare function SDL_GetRenderLogicalPresentationRect(byval renderer as SDL_Renderer ptr, byval rect as SDL_FRect ptr) as boolean
declare function SDL_RenderCoordinatesFromWindow(byval renderer as SDL_Renderer ptr, byval window_x as single, byval window_y as single, byval x as single ptr, byval y as single ptr) as boolean
declare function SDL_RenderCoordinatesToWindow(byval renderer as SDL_Renderer ptr, byval x as single, byval y as single, byval window_x as single ptr, byval window_y as single ptr) as boolean
declare function SDL_ConvertEventToRenderCoordinates(byval renderer as SDL_Renderer ptr, byval event as SDL_Event ptr) as boolean
declare function SDL_SetRenderViewport(byval renderer as SDL_Renderer ptr, byval rect as const SDL_Rect ptr) as boolean
declare function SDL_GetRenderViewport(byval renderer as SDL_Renderer ptr, byval rect as SDL_Rect ptr) as boolean
declare function SDL_RenderViewportSet(byval renderer as SDL_Renderer ptr) as boolean
declare function SDL_GetRenderSafeArea(byval renderer as SDL_Renderer ptr, byval rect as SDL_Rect ptr) as boolean
declare function SDL_SetRenderClipRect(byval renderer as SDL_Renderer ptr, byval rect as const SDL_Rect ptr) as boolean
declare function SDL_GetRenderClipRect(byval renderer as SDL_Renderer ptr, byval rect as SDL_Rect ptr) as boolean
declare function SDL_RenderClipEnabled(byval renderer as SDL_Renderer ptr) as boolean
declare function SDL_SetRenderScale(byval renderer as SDL_Renderer ptr, byval scaleX as single, byval scaleY as single) as boolean
declare function SDL_GetRenderScale(byval renderer as SDL_Renderer ptr, byval scaleX as single ptr, byval scaleY as single ptr) as boolean
declare function SDL_SetRenderDrawColor(byval renderer as SDL_Renderer ptr, byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as boolean
declare function SDL_SetRenderDrawColorFloat(byval renderer as SDL_Renderer ptr, byval r as single, byval g as single, byval b as single, byval a as single) as boolean
declare function SDL_GetRenderDrawColor(byval renderer as SDL_Renderer ptr, byval r as Uint8 ptr, byval g as Uint8 ptr, byval b as Uint8 ptr, byval a as Uint8 ptr) as boolean
declare function SDL_GetRenderDrawColorFloat(byval renderer as SDL_Renderer ptr, byval r as single ptr, byval g as single ptr, byval b as single ptr, byval a as single ptr) as boolean
declare function SDL_SetRenderColorScale(byval renderer as SDL_Renderer ptr, byval scale as single) as boolean
declare function SDL_GetRenderColorScale(byval renderer as SDL_Renderer ptr, byval scale as single ptr) as boolean
declare function SDL_SetRenderDrawBlendMode(byval renderer as SDL_Renderer ptr, byval blendMode as SDL_BlendMode) as boolean
declare function SDL_GetRenderDrawBlendMode(byval renderer as SDL_Renderer ptr, byval blendMode as SDL_BlendMode ptr) as boolean
declare function SDL_RenderClear(byval renderer as SDL_Renderer ptr) as boolean
declare function SDL_RenderPoint(byval renderer as SDL_Renderer ptr, byval x as single, byval y as single) as boolean
declare function SDL_RenderPoints(byval renderer as SDL_Renderer ptr, byval points as const SDL_FPoint ptr, byval count as long) as boolean
declare function SDL_RenderLine(byval renderer as SDL_Renderer ptr, byval x1 as single, byval y1 as single, byval x2 as single, byval y2 as single) as boolean
declare function SDL_RenderLines(byval renderer as SDL_Renderer ptr, byval points as const SDL_FPoint ptr, byval count as long) as boolean
declare function SDL_RenderRect(byval renderer as SDL_Renderer ptr, byval rect as const SDL_FRect ptr) as boolean
declare function SDL_RenderRects(byval renderer as SDL_Renderer ptr, byval rects as const SDL_FRect ptr, byval count as long) as boolean
declare function SDL_RenderFillRect(byval renderer as SDL_Renderer ptr, byval rect as const SDL_FRect ptr) as boolean
declare function SDL_RenderFillRects(byval renderer as SDL_Renderer ptr, byval rects as const SDL_FRect ptr, byval count as long) as boolean
declare function SDL_RenderTexture(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval srcrect as const SDL_FRect ptr, byval dstrect as const SDL_FRect ptr) as boolean
declare function SDL_RenderTextureRotated(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval srcrect as const SDL_FRect ptr, byval dstrect as const SDL_FRect ptr, byval angle as double, byval center as const SDL_FPoint ptr, byval flip as SDL_FlipMode) as boolean
declare function SDL_RenderTextureAffine(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval srcrect as const SDL_FRect ptr, byval origin as const SDL_FPoint ptr, byval right as const SDL_FPoint ptr, byval down as const SDL_FPoint ptr) as boolean
declare function SDL_RenderTextureTiled(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval srcrect as const SDL_FRect ptr, byval scale as single, byval dstrect as const SDL_FRect ptr) as boolean
declare function SDL_RenderTexture9Grid(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval srcrect as const SDL_FRect ptr, byval left_width as single, byval right_width as single, byval top_height as single, byval bottom_height as single, byval scale as single, byval dstrect as const SDL_FRect ptr) as boolean
declare function SDL_RenderTexture9GridTiled(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval srcrect as const SDL_FRect ptr, byval left_width as single, byval right_width as single, byval top_height as single, byval bottom_height as single, byval scale as single, byval dstrect as const SDL_FRect ptr, byval tileScale as single) as boolean
declare function SDL_RenderGeometry(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval vertices as const SDL_Vertex ptr, byval num_vertices as long, byval indices as const long ptr, byval num_indices as long) as boolean
declare function SDL_RenderGeometryRaw(byval renderer as SDL_Renderer ptr, byval texture as SDL_Texture ptr, byval xy as const single ptr, byval xy_stride as long, byval color as const SDL_FColor ptr, byval color_stride as long, byval uv as const single ptr, byval uv_stride as long, byval num_vertices as long, byval indices as const any ptr, byval num_indices as long, byval size_indices as long) as boolean
declare function SDL_SetRenderTextureAddressMode(byval renderer as SDL_Renderer ptr, byval u_mode as SDL_TextureAddressMode, byval v_mode as SDL_TextureAddressMode) as boolean
declare function SDL_GetRenderTextureAddressMode(byval renderer as SDL_Renderer ptr, byval u_mode as SDL_TextureAddressMode ptr, byval v_mode as SDL_TextureAddressMode ptr) as boolean
declare function SDL_RenderReadPixels(byval renderer as SDL_Renderer ptr, byval rect as const SDL_Rect ptr) as SDL_Surface ptr
declare function SDL_RenderPresent(byval renderer as SDL_Renderer ptr) as boolean
declare sub SDL_DestroyTexture(byval texture as SDL_Texture ptr)
declare sub SDL_DestroyRenderer(byval renderer as SDL_Renderer ptr)
declare function SDL_FlushRenderer(byval renderer as SDL_Renderer ptr) as boolean
declare function SDL_GetRenderMetalLayer(byval renderer as SDL_Renderer ptr) as any ptr
declare function SDL_GetRenderMetalCommandEncoder(byval renderer as SDL_Renderer ptr) as any ptr
declare function SDL_AddVulkanRenderSemaphores(byval renderer as SDL_Renderer ptr, byval wait_stage_mask as Uint32, byval wait_semaphore as Sint64, byval signal_semaphore as Sint64) as boolean
declare function SDL_SetRenderVSync(byval renderer as SDL_Renderer ptr, byval vsync as long) as boolean
const SDL_RENDERER_VSYNC_DISABLED = 0
const SDL_RENDERER_VSYNC_ADAPTIVE = -1
declare function SDL_GetRenderVSync(byval renderer as SDL_Renderer ptr, byval vsync as long ptr) as boolean
const SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE = 8
declare function SDL_RenderDebugText(byval renderer as SDL_Renderer ptr, byval x as single, byval y as single, byval str as const zstring ptr) as boolean
declare function SDL_RenderDebugTextFormat(byval renderer as SDL_Renderer ptr, byval x as single, byval y as single, byval fmt as const zstring ptr, ...) as boolean
declare function SDL_SetDefaultTextureScaleMode(byval renderer as SDL_Renderer ptr, byval scale_mode as SDL_ScaleMode) as boolean
declare function SDL_GetDefaultTextureScaleMode(byval renderer as SDL_Renderer ptr, byval scale_mode as SDL_ScaleMode ptr) as boolean

type SDL_GPURenderStateCreateInfo
	fragment_shader as SDL_GPUShader ptr
	num_sampler_bindings as Sint32
	sampler_bindings as const SDL_GPUTextureSamplerBinding ptr
	num_storage_textures as Sint32
	storage_textures as SDL_GPUTexture const ptr ptr
	num_storage_buffers as Sint32
	storage_buffers as SDL_GPUBuffer const ptr ptr
	props as SDL_PropertiesID
end type

declare function SDL_CreateGPURenderState(byval renderer as SDL_Renderer ptr, byval createinfo as const SDL_GPURenderStateCreateInfo ptr) as SDL_GPURenderState ptr
declare function SDL_SetGPURenderStateFragmentUniforms(byval state as SDL_GPURenderState ptr, byval slot_index as Uint32, byval data as const any ptr, byval length as Uint32) as boolean
declare function SDL_SetGPURenderState(byval renderer as SDL_Renderer ptr, byval state as SDL_GPURenderState ptr) as boolean
declare sub SDL_DestroyGPURenderState(byval state as SDL_GPURenderState ptr)
'' -------------------------------------------------------------------------
'' SDL_storage.h
'' -------------------------------------------------------------------------
type SDL_StorageInterface
	version as Uint32
	close as function(byval userdata as any ptr) as boolean
	ready as function(byval userdata as any ptr) as boolean
	enumerate as function(byval userdata as any ptr, byval path as const zstring ptr, byval callback as SDL_EnumerateDirectoryCallback, byval callback_userdata as any ptr) as boolean
	info as function(byval userdata as any ptr, byval path as const zstring ptr, byval info as SDL_PathInfo ptr) as boolean
	read_file as function(byval userdata as any ptr, byval path as const zstring ptr, byval destination as any ptr, byval length as Uint64) as boolean
	write_file as function(byval userdata as any ptr, byval path as const zstring ptr, byval source as const any ptr, byval length as Uint64) as boolean
	mkdir as function(byval userdata as any ptr, byval path as const zstring ptr) as boolean
	remove as function(byval userdata as any ptr, byval path as const zstring ptr) as boolean
	rename as function(byval userdata as any ptr, byval oldpath as const zstring ptr, byval newpath as const zstring ptr) as boolean
	copy as function(byval userdata as any ptr, byval oldpath as const zstring ptr, byval newpath as const zstring ptr) as boolean
	space_remaining as function(byval userdata as any ptr) as Uint64
end type

declare function SDL_OpenTitleStorage(byval override as const zstring ptr, byval props as SDL_PropertiesID) as SDL_Storage ptr
declare function SDL_OpenUserStorage(byval org as const zstring ptr, byval app as const zstring ptr, byval props as SDL_PropertiesID) as SDL_Storage ptr
declare function SDL_OpenFileStorage(byval path as const zstring ptr) as SDL_Storage ptr
declare function SDL_OpenStorage(byval iface as const SDL_StorageInterface ptr, byval userdata as any ptr) as SDL_Storage ptr
declare function SDL_CloseStorage(byval storage as SDL_Storage ptr) as boolean
declare function SDL_StorageReady(byval storage as SDL_Storage ptr) as boolean
declare function SDL_GetStorageFileSize(byval storage as SDL_Storage ptr, byval path as const zstring ptr, byval length as Uint64 ptr) as boolean
declare function SDL_ReadStorageFile(byval storage as SDL_Storage ptr, byval path as const zstring ptr, byval destination as any ptr, byval length as Uint64) as boolean
declare function SDL_WriteStorageFile(byval storage as SDL_Storage ptr, byval path as const zstring ptr, byval source as const any ptr, byval length as Uint64) as boolean
declare function SDL_CreateStorageDirectory(byval storage as SDL_Storage ptr, byval path as const zstring ptr) as boolean
declare function SDL_EnumerateStorageDirectory(byval storage as SDL_Storage ptr, byval path as const zstring ptr, byval callback as SDL_EnumerateDirectoryCallback, byval userdata as any ptr) as boolean
declare function SDL_RemoveStoragePath(byval storage as SDL_Storage ptr, byval path as const zstring ptr) as boolean
declare function SDL_RenameStoragePath(byval storage as SDL_Storage ptr, byval oldpath as const zstring ptr, byval newpath as const zstring ptr) as boolean
declare function SDL_CopyStorageFile(byval storage as SDL_Storage ptr, byval oldpath as const zstring ptr, byval newpath as const zstring ptr) as boolean
declare function SDL_GetStoragePathInfo(byval storage as SDL_Storage ptr, byval path as const zstring ptr, byval info as SDL_PathInfo ptr) as boolean
declare function SDL_GetStorageSpaceRemaining(byval storage as SDL_Storage ptr) as Uint64
declare function SDL_GlobStorageDirectory(byval storage as SDL_Storage ptr, byval path as const zstring ptr, byval pattern as const zstring ptr, byval flags as SDL_GlobFlags, byval count as long ptr) as zstring ptr ptr
'' -------------------------------------------------------------------------
'' SDL_system.h
'' -------------------------------------------------------------------------
#ifdef __FB_WIN32__
	type MSG as tagMSG
	type SDL_WindowsMessageHook as function(byval userdata as any ptr, byval msg as MSG ptr) as boolean
	declare sub SDL_SetWindowsMessageHook(byval callback as SDL_WindowsMessageHook, byval userdata as any ptr)
	declare function SDL_GetDirect3D9AdapterIndex(byval displayID as SDL_DisplayID) as long
	declare function SDL_GetDXGIOutputInfo(byval displayID as SDL_DisplayID, byval adapterIndex as long ptr, byval outputIndex as long ptr) as boolean
#endif

type XEvent as _XEvent
type SDL_X11EventHook as function(byval userdata as any ptr, byval xevent as XEvent ptr) as boolean
declare sub SDL_SetX11EventHook(byval callback as SDL_X11EventHook, byval userdata as any ptr)

#ifdef __FB_LINUX__
	declare function SDL_SetLinuxThreadPriority(byval threadID as Sint64, byval priority as long) as boolean
	declare function SDL_SetLinuxThreadPriorityAndPolicy(byval threadID as Sint64, byval sdlPriority as long, byval schedPolicy as long) as boolean
#endif

declare function SDL_IsTablet() as boolean
declare function SDL_IsTV() as boolean

type SDL_Sandbox as long
enum
	SDL_SANDBOX_NONE = 0
	SDL_SANDBOX_UNKNOWN_CONTAINER
	SDL_SANDBOX_FLATPAK
	SDL_SANDBOX_SNAP
	SDL_SANDBOX_MACOS
end enum

declare function SDL_GetSandbox() as SDL_Sandbox
declare sub SDL_OnApplicationWillTerminate()
declare sub SDL_OnApplicationDidReceiveMemoryWarning()
declare sub SDL_OnApplicationWillEnterBackground()
declare sub SDL_OnApplicationDidEnterBackground()
declare sub SDL_OnApplicationWillEnterForeground()
declare sub SDL_OnApplicationDidEnterForeground()
'' -------------------------------------------------------------------------
'' SDL_time.h
'' -------------------------------------------------------------------------
type SDL_DateTime
	year as long
	month as long
	day as long
	hour as long
	minute as long
	second as long
	nanosecond as long
	day_of_week as long
	utc_offset as long
end type

type SDL_DateFormat as long
enum
	SDL_DATE_FORMAT_YYYYMMDD = 0
	SDL_DATE_FORMAT_DDMMYYYY = 1
	SDL_DATE_FORMAT_MMDDYYYY = 2
end enum

type SDL_TimeFormat as long
enum
	SDL_TIME_FORMAT_24HR = 0
	SDL_TIME_FORMAT_12HR = 1
end enum

declare function SDL_GetDateTimeLocalePreferences(byval dateFormat as SDL_DateFormat ptr, byval timeFormat as SDL_TimeFormat ptr) as boolean
declare function SDL_GetCurrentTime(byval ticks as SDL_Time ptr) as boolean
declare function SDL_TimeToDateTime(byval ticks as SDL_Time, byval dt as SDL_DateTime ptr, byval localTime as boolean) as boolean
declare function SDL_DateTimeToTime(byval dt as const SDL_DateTime ptr, byval ticks as SDL_Time ptr) as boolean
declare sub SDL_TimeToWindows(byval ticks as SDL_Time, byval dwLowDateTime as Uint32 ptr, byval dwHighDateTime as Uint32 ptr)
declare function SDL_TimeFromWindows(byval dwLowDateTime as Uint32, byval dwHighDateTime as Uint32) as SDL_Time
declare function SDL_GetDaysInMonth(byval year as long, byval month as long) as long
declare function SDL_GetDayOfYear(byval year as long, byval month as long, byval day as long) as long
declare function SDL_GetDayOfWeek(byval year as long, byval month as long, byval day as long) as long

'' -------------------------------------------------------------------------
'' SDL_timer.h
'' -------------------------------------------------------------------------
const SDL_MS_PER_SECOND as long = 1000
const SDL_US_PER_SECOND as long = 1000000
const SDL_NS_PER_SECOND = 1000000000ll
const SDL_NS_PER_MS as long = 1000000
const SDL_NS_PER_US as long = 1000
#define SDL_SECONDS_TO_NS(S) (cast(Uint64, (S)) * SDL_NS_PER_SECOND)
#define SDL_NS_TO_SECONDS(NS) ((NS) \ SDL_NS_PER_SECOND)
#define SDL_MS_TO_NS(MS) (cast(Uint64, (MS)) * SDL_NS_PER_MS)
#define SDL_NS_TO_MS(NS) ((NS) \ SDL_NS_PER_MS)
#define SDL_US_TO_NS(US) (cast(Uint64, (US)) * SDL_NS_PER_US)
#define SDL_NS_TO_US(NS) ((NS) \ SDL_NS_PER_US)

declare function SDL_GetTicks() as Uint64
declare function SDL_GetTicksNS() as Uint64
declare function SDL_GetPerformanceCounter() as Uint64
declare function SDL_GetPerformanceFrequency() as Uint64
declare sub SDL_Delay(byval ms as Uint32)
declare sub SDL_DelayNS(byval ns as Uint64)
declare sub SDL_DelayPrecise(byval ns as Uint64)
type SDL_TimerID as Uint32
type SDL_TimerCallback as function(byval userdata as any ptr, byval timerID as SDL_TimerID, byval interval as Uint32) as Uint32
declare function SDL_AddTimer(byval interval as Uint32, byval callback as SDL_TimerCallback, byval userdata as any ptr) as SDL_TimerID
type SDL_NSTimerCallback as function(byval userdata as any ptr, byval timerID as SDL_TimerID, byval interval as Uint64) as Uint64
declare function SDL_AddTimerNS(byval interval as Uint64, byval callback as SDL_NSTimerCallback, byval userdata as any ptr) as SDL_TimerID
declare function SDL_RemoveTimer(byval id as SDL_TimerID) as boolean
'' -------------------------------------------------------------------------
'' SDL_tray.h
'' -------------------------------------------------------------------------
type SDL_TrayEntryFlags as Uint32

const SDL_TRAYENTRY_BUTTON = &h00000001u
const SDL_TRAYENTRY_CHECKBOX = &h00000002u
const SDL_TRAYENTRY_SUBMENU = &h00000004u
const SDL_TRAYENTRY_DISABLED = &h80000000u
const SDL_TRAYENTRY_CHECKED = &h40000000u
type SDL_TrayCallback as sub(byval userdata as any ptr, byval entry as SDL_TrayEntry ptr)

declare function SDL_CreateTray(byval icon as SDL_Surface ptr, byval tooltip as const zstring ptr) as SDL_Tray ptr
declare sub SDL_SetTrayIcon(byval tray as SDL_Tray ptr, byval icon as SDL_Surface ptr)
declare sub SDL_SetTrayTooltip(byval tray as SDL_Tray ptr, byval tooltip as const zstring ptr)
declare function SDL_CreateTrayMenu(byval tray as SDL_Tray ptr) as SDL_TrayMenu ptr
declare function SDL_CreateTraySubmenu(byval entry as SDL_TrayEntry ptr) as SDL_TrayMenu ptr
declare function SDL_GetTrayMenu(byval tray as SDL_Tray ptr) as SDL_TrayMenu ptr
declare function SDL_GetTraySubmenu(byval entry as SDL_TrayEntry ptr) as SDL_TrayMenu ptr
declare function SDL_GetTrayEntries(byval menu as SDL_TrayMenu ptr, byval count as long ptr) as const SDL_TrayEntry ptr ptr
declare sub SDL_RemoveTrayEntry(byval entry as SDL_TrayEntry ptr)
declare function SDL_InsertTrayEntryAt(byval menu as SDL_TrayMenu ptr, byval pos as long, byval label as const zstring ptr, byval flags as SDL_TrayEntryFlags) as SDL_TrayEntry ptr
declare sub SDL_SetTrayEntryLabel(byval entry as SDL_TrayEntry ptr, byval label as const zstring ptr)
declare function SDL_GetTrayEntryLabel(byval entry as SDL_TrayEntry ptr) as const zstring ptr
declare sub SDL_SetTrayEntryChecked(byval entry as SDL_TrayEntry ptr, byval checked as boolean)
declare function SDL_GetTrayEntryChecked(byval entry as SDL_TrayEntry ptr) as boolean
declare sub SDL_SetTrayEntryEnabled(byval entry as SDL_TrayEntry ptr, byval enabled as boolean)
declare function SDL_GetTrayEntryEnabled(byval entry as SDL_TrayEntry ptr) as boolean
declare sub SDL_SetTrayEntryCallback(byval entry as SDL_TrayEntry ptr, byval callback as SDL_TrayCallback, byval userdata as any ptr)
declare sub SDL_ClickTrayEntry(byval entry as SDL_TrayEntry ptr)
declare sub SDL_DestroyTray(byval tray as SDL_Tray ptr)
declare function SDL_GetTrayEntryParent(byval entry as SDL_TrayEntry ptr) as SDL_TrayMenu ptr
declare function SDL_GetTrayMenuParentEntry(byval menu as SDL_TrayMenu ptr) as SDL_TrayEntry ptr
declare function SDL_GetTrayMenuParentTray(byval menu as SDL_TrayMenu ptr) as SDL_Tray ptr
declare sub SDL_UpdateTrays()

'' -------------------------------------------------------------------------
'' SDL_version.h
'' -------------------------------------------------------------------------
const SDL_MAJOR_VERSION as long = 3
const SDL_MINOR_VERSION as long = 4
const SDL_MICRO_VERSION as long = 18
#define SDL_VERSIONNUM(major, minor, patch) ((((major) * 1000000L) + ((minor) * 1000L)) + (patch))
#define SDL_VERSIONNUM_MAJOR(version) clng((version) \ 1000000L)
#define SDL_VERSIONNUM_MINOR(version) clng(((version) \ 1000L) mod 1000L)
#define SDL_VERSIONNUM_MICRO(version) clng((version) mod 1000L)
#define SDL_VERSION SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION)
#define SDL_VERSION_ATLEAST(X, Y, Z) (SDL_VERSION >= SDL_VERSIONNUM(X, Y, Z))
declare function SDL_GetVersion() as long
declare function SDL_GetRevision() as const zstring ptr
'' -------------------------------------------------------------------------
'' SDL_oldnames.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_main.h
'' -------------------------------------------------------------------------
#define SDLMAIN_DECLSPEC
type SDL_main_func as function(byval argc as long, byval argv as zstring ptr ptr) as long

declare function SDL_main(byval argc as long, byval argv as zstring ptr ptr) as long
declare sub SDL_SetMainReady()
declare function SDL_RunApp(byval argc as long, byval argv as zstring ptr ptr, byval mainFunction as SDL_main_func, byval reserved as any ptr) as long
declare function SDL_EnterAppMainCallbacks(byval argc as long, byval argv as zstring ptr ptr, byval appinit as SDL_AppInit_func, byval appiter as SDL_AppIterate_func, byval appevent as SDL_AppEvent_func, byval appquit as SDL_AppQuit_func) as long

#ifdef __FB_WIN32__
	declare function SDL_RegisterApp(byval name as const zstring ptr, byval style as Uint32, byval hInst as any ptr) as boolean
	declare sub SDL_UnregisterApp()
#endif

declare sub SDL_GDKSuspendComplete()

end extern

#include once "SDL_inline.bi"
#include once "SDL_platform_api.bi"

'' end of SDL.bi
