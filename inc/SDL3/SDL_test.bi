'' Project: FreeBASIC SDL3 bindings
'' File: SDL_test.bi
'' Purpose: Declare the SDL3-3.4.18 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''
'' Simple DirectMedia Layer
''   Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
''
''   This software is provided 'as-is', without any express or implied
''   warranty.  In no event will the authors be held liable for any damages
''   arising from the use of this software.
''
''   Permission is granted to anyone to use this software for any purpose,
''   including commercial applications, and to alter it and redistribute it
''   freely, subject to the following restrictions:
''
''   1. The origin of this software must not be misrepresented; you must not
''      claim that you wrote the original software. If you use this software
''      in a product, an acknowledgment in the product documentation would be
''      appreciated but is not required.
''   2. Altered source versions must be plainly marked as such, and must not be
''      misrepresented as being the original software.
''   3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL3_test"

#include once "SDL.bi"

extern "C"

'' -------------------------------------------------------------------------
'' SDL_test.h
'' -------------------------------------------------------------------------
'' -------------------------------------------------------------------------
'' SDL_test_assert.h
'' -------------------------------------------------------------------------
const ASSERT_FAIL = 0
const ASSERT_PASS = 1

declare sub SDLTest_Assert(byval assertCondition as long, byval assertDescription as const zstring ptr, ...)
declare function SDLTest_AssertCheck(byval assertCondition as long, byval assertDescription as const zstring ptr, ...) as long
declare sub SDLTest_AssertPass(byval assertDescription as const zstring ptr, ...)
declare sub SDLTest_ResetAssertSummary()
declare sub SDLTest_LogAssertSummary()
declare function SDLTest_AssertSummaryToTestResult() as long

'' -------------------------------------------------------------------------
'' SDL_test_common.h
'' -------------------------------------------------------------------------
const DEFAULT_WINDOW_WIDTH = 640
const DEFAULT_WINDOW_HEIGHT = 480
type SDLTest_VerboseFlags as Uint32
const VERBOSE_VIDEO = &h00000001
const VERBOSE_MODES = &h00000002
const VERBOSE_RENDER = &h00000004
const VERBOSE_EVENT = &h00000008
const VERBOSE_AUDIO = &h00000010
const VERBOSE_MOTION = &h00000020
type SDLTest_ParseArgumentsFp as function(byval data as any ptr, byval argv as zstring ptr ptr, byval index as long) as long
type SDLTest_FinalizeArgumentParserFp as sub(byval arg as any ptr)

type SDLTest_ArgumentParser
	parse_arguments as SDLTest_ParseArgumentsFp
	finalize as SDLTest_FinalizeArgumentParserFp
	usage as const zstring ptr ptr
	data as any ptr
	next as SDLTest_ArgumentParser ptr
end type

type SDLTest_CommonState
	argv as zstring ptr ptr
	flags as SDL_InitFlags
	verbose as SDLTest_VerboseFlags
	videodriver as const zstring ptr
	display_index as long
	displayID as SDL_DisplayID
	window_title as const zstring ptr
	window_icon as const zstring ptr
	window_flags as SDL_WindowFlags
	flash_on_focus_loss as boolean
	window_x as long
	window_y as long
	window_w as long
	window_h as long
	window_minW as long
	window_minH as long
	window_maxW as long
	window_maxH as long
	window_min_aspect as single
	window_max_aspect as single
	logical_w as long
	logical_h as long
	auto_scale_content as boolean
	logical_presentation as SDL_RendererLogicalPresentation
	scale as single
	depth as long
	refresh_rate as single
	fill_usable_bounds as boolean
	fullscreen_exclusive as boolean
	fullscreen_mode as SDL_DisplayMode
	num_windows as long
	windows as SDL_Window ptr ptr
	gpudriver as const zstring ptr
	renderdriver as const zstring ptr
	render_vsync as long
	skip_renderer as boolean
	renderers as SDL_Renderer ptr ptr
	targets as SDL_Texture ptr ptr
	audiodriver as const zstring ptr
	audio_format as SDL_AudioFormat
	audio_channels as long
	audio_freq as long
	audio_id as SDL_AudioDeviceID
	gl_red_size as long
	gl_green_size as long
	gl_blue_size as long
	gl_alpha_size as long
	gl_buffer_size as long
	gl_depth_size as long
	gl_stencil_size as long
	gl_double_buffer as long
	gl_accum_red_size as long
	gl_accum_green_size as long
	gl_accum_blue_size as long
	gl_accum_alpha_size as long
	gl_stereo as long
	gl_release_behavior as long
	gl_multisamplebuffers as long
	gl_multisamplesamples as long
	gl_retained_backing as long
	gl_accelerated as long
	gl_major_version as long
	gl_minor_version as long
	gl_debug as long
	gl_profile_mask as long
	confine as SDL_Rect
	hide_cursor as boolean
	quit_after_ms_interval as long
	quit_after_ms_timer as SDL_TimerID
	common_argparser as SDLTest_ArgumentParser
	video_argparser as SDLTest_ArgumentParser
	audio_argparser as SDLTest_ArgumentParser
	argparser as SDLTest_ArgumentParser ptr
end type

declare function SDLTest_CommonCreateState(byval argv as zstring ptr ptr, byval flags as SDL_InitFlags) as SDLTest_CommonState ptr
declare sub SDLTest_CommonDestroyState(byval state as SDLTest_CommonState ptr)
declare function SDLTest_CommonArg(byval state as SDLTest_CommonState ptr, byval index as long) as long
declare sub SDLTest_CommonLogUsage(byval state as SDLTest_CommonState ptr, byval argv0 as const zstring ptr, byval options as const zstring ptr ptr)
declare function SDLTest_CommonInit(byval state as SDLTest_CommonState ptr) as boolean
declare function SDLTest_CommonDefaultArgs(byval state as SDLTest_CommonState ptr, byval argc as long, byval argv as zstring ptr ptr) as boolean
declare sub SDLTest_PrintEvent(byval event as const SDL_Event ptr)
declare sub SDLTest_CommonEvent(byval state as SDLTest_CommonState ptr, byval event as SDL_Event ptr, byval done as long ptr)
declare function SDLTest_CommonEventMainCallbacks(byval state as SDLTest_CommonState ptr, byval event as const SDL_Event ptr) as SDL_AppResult
declare sub SDLTest_CommonQuit(byval state as SDLTest_CommonState ptr)
declare sub SDLTest_CommonDrawWindowInfo(byval renderer as SDL_Renderer ptr, byval window as SDL_Window ptr, byval usedHeight as single ptr)
'' -------------------------------------------------------------------------
'' SDL_test_compare.h
'' -------------------------------------------------------------------------
declare function SDLTest_CompareSurfaces(byval surface as SDL_Surface ptr, byval referenceSurface as SDL_Surface ptr, byval allowable_error as long) as long
declare function SDLTest_CompareSurfacesIgnoreTransparentPixels(byval surface as SDL_Surface ptr, byval referenceSurface as SDL_Surface ptr, byval allowable_error as long) as long
declare function SDLTest_CompareMemory(byval actual as const any ptr, byval size_actual as uinteger, byval reference as const any ptr, byval size_reference as uinteger) as long
'' -------------------------------------------------------------------------
'' SDL_test_crc32.h
'' -------------------------------------------------------------------------
type CrcUint32 as ulong
type CrcUint8 as ubyte
const CRC32_POLY = &hEDB88320

type SDLTest_Crc32Context
	crc32_table(0 to 255) as ulong
end type

declare function SDLTest_Crc32Init(byval crcContext as SDLTest_Crc32Context ptr) as boolean
declare function SDLTest_Crc32Calc(byval crcContext as SDLTest_Crc32Context ptr, byval inBuf as ubyte ptr, byval inLen as ulong, byval crc32 as ulong ptr) as boolean
declare function SDLTest_Crc32CalcStart(byval crcContext as SDLTest_Crc32Context ptr, byval crc32 as ulong ptr) as boolean
declare function SDLTest_Crc32CalcEnd(byval crcContext as SDLTest_Crc32Context ptr, byval crc32 as ulong ptr) as boolean
declare function SDLTest_Crc32CalcBuffer(byval crcContext as SDLTest_Crc32Context ptr, byval inBuf as ubyte ptr, byval inLen as ulong, byval crc32 as ulong ptr) as boolean
declare function SDLTest_Crc32Done(byval crcContext as SDLTest_Crc32Context ptr) as boolean
'' -------------------------------------------------------------------------
'' SDL_test_font.h
'' -------------------------------------------------------------------------
extern FONT_CHARACTER_SIZE as long
#define FONT_LINE_HEIGHT (FONT_CHARACTER_SIZE + 2)
declare function SDLTest_DrawCharacter(byval renderer as SDL_Renderer ptr, byval x as single, byval y as single, byval c as Uint32) as boolean
declare function SDLTest_DrawString(byval renderer as SDL_Renderer ptr, byval x as single, byval y as single, byval s as const zstring ptr) as boolean

type SDLTest_TextWindow
	rect as SDL_FRect
	current as long
	numlines as long
	lines as zstring ptr ptr
end type

declare function SDLTest_TextWindowCreate(byval x as single, byval y as single, byval w as single, byval h as single) as SDLTest_TextWindow ptr
declare sub SDLTest_TextWindowDisplay(byval textwin as SDLTest_TextWindow ptr, byval renderer as SDL_Renderer ptr)
declare sub SDLTest_TextWindowAddText(byval textwin as SDLTest_TextWindow ptr, byval fmt as const zstring ptr, ...)
declare sub SDLTest_TextWindowAddTextWithLength(byval textwin as SDLTest_TextWindow ptr, byval text as const zstring ptr, byval len as uinteger)
declare sub SDLTest_TextWindowClear(byval textwin as SDLTest_TextWindow ptr)
declare sub SDLTest_TextWindowDestroy(byval textwin as SDLTest_TextWindow ptr)
declare sub SDLTest_CleanupTextDrawing()
'' -------------------------------------------------------------------------
'' SDL_test_fuzzer.h
'' -------------------------------------------------------------------------
declare sub SDLTest_FuzzerInit(byval execKey as Uint64)
declare function SDLTest_RandomUint8() as Uint8
declare function SDLTest_RandomSint8() as Sint8
declare function SDLTest_RandomUint16() as Uint16
declare function SDLTest_RandomSint16() as Sint16
declare function SDLTest_RandomSint32() as Sint32
declare function SDLTest_RandomUint32() as Uint32
declare function SDLTest_RandomUint64() as Uint64
declare function SDLTest_RandomSint64() as Sint64
declare function SDLTest_RandomUnitFloat() as single
declare function SDLTest_RandomUnitDouble() as double
declare function SDLTest_RandomFloat() as single
declare function SDLTest_RandomDouble() as double
declare function SDLTest_RandomUint8BoundaryValue(byval boundary1 as Uint8, byval boundary2 as Uint8, byval validDomain as boolean) as Uint8
declare function SDLTest_RandomUint16BoundaryValue(byval boundary1 as Uint16, byval boundary2 as Uint16, byval validDomain as boolean) as Uint16
declare function SDLTest_RandomUint32BoundaryValue(byval boundary1 as Uint32, byval boundary2 as Uint32, byval validDomain as boolean) as Uint32
declare function SDLTest_RandomUint64BoundaryValue(byval boundary1 as Uint64, byval boundary2 as Uint64, byval validDomain as boolean) as Uint64
declare function SDLTest_RandomSint8BoundaryValue(byval boundary1 as Sint8, byval boundary2 as Sint8, byval validDomain as boolean) as Sint8
declare function SDLTest_RandomSint16BoundaryValue(byval boundary1 as Sint16, byval boundary2 as Sint16, byval validDomain as boolean) as Sint16
declare function SDLTest_RandomSint32BoundaryValue(byval boundary1 as Sint32, byval boundary2 as Sint32, byval validDomain as boolean) as Sint32
declare function SDLTest_RandomSint64BoundaryValue(byval boundary1 as Sint64, byval boundary2 as Sint64, byval validDomain as boolean) as Sint64
declare function SDLTest_RandomIntegerInRange(byval min as Sint32, byval max as Sint32) as Sint32
declare function SDLTest_RandomAsciiString() as zstring ptr
declare function SDLTest_RandomAsciiStringWithMaximumLength(byval maxLength as long) as zstring ptr
declare function SDLTest_RandomAsciiStringOfSize(byval size as long) as zstring ptr
declare function SDLTest_GetFuzzerInvocationCount() as long

#define SDL_test_h_arness_h
const TEST_ENABLED = 1
const TEST_DISABLED = 0
const TEST_ABORTED = -1
const TEST_STARTED = 0
const TEST_COMPLETED = 1
const TEST_SKIPPED = 2
const TEST_RESULT_PASSED = 0
const TEST_RESULT_FAILED = 1
const TEST_RESULT_NO_ASSERT = 2
const TEST_RESULT_SKIPPED = 3
const TEST_RESULT_SETUP_FAILURE = 4

type SDLTest_TestCaseSetUpFp as sub(byval arg as any ptr ptr)
type SDLTest_TestCaseFp as function(byval arg as any ptr) as long
type SDLTest_TestCaseTearDownFp as sub(byval arg as any ptr)

type SDLTest_TestCaseReference
	testCase as SDLTest_TestCaseFp
	name as const zstring ptr
	description as const zstring ptr
	enabled as long
end type

type SDLTest_TestSuiteReference
	name as const zstring ptr
	testSetUp as SDLTest_TestCaseSetUpFp
	testCases as const SDLTest_TestCaseReference ptr ptr
	testTearDown as SDLTest_TestCaseTearDownFp
end type

declare function SDLTest_GenerateRunSeed(byval buffer as zstring ptr, byval length as long) as zstring ptr
declare function SDLTest_CreateTestSuiteRunner(byval state as SDLTest_CommonState ptr, byval testSuites as SDLTest_TestSuiteReference ptr ptr) as SDLTest_TestSuiteRunner ptr
declare sub SDLTest_DestroyTestSuiteRunner(byval runner as SDLTest_TestSuiteRunner ptr)
declare function SDLTest_ExecuteTestSuiteRunner(byval runner as SDLTest_TestSuiteRunner ptr) as long
'' -------------------------------------------------------------------------
'' SDL_test_log.h
'' -------------------------------------------------------------------------
declare sub SDLTest_LogMessage(byval priority as SDL_LogPriority, byval fmt as const zstring ptr, ...)
declare sub SDLTest_Log(byval fmt as const zstring ptr, ...)
declare sub SDLTest_LogEscapedString(byval prefix as const zstring ptr, byval buffer as const any ptr, byval size as uinteger)
declare sub SDLTest_LogError(byval fmt as const zstring ptr, ...)
'' -------------------------------------------------------------------------
'' SDL_test_md5.h
'' -------------------------------------------------------------------------
type MD5UINT4 as Uint32

type SDLTest_Md5Context
	i(0 to 1) as MD5UINT4
	buf(0 to 3) as MD5UINT4
	in(0 to 63) as ubyte
	digest(0 to 15) as ubyte
end type

declare sub SDLTest_Md5Init(byval mdContext as SDLTest_Md5Context ptr)
declare sub SDLTest_Md5Update(byval mdContext as SDLTest_Md5Context ptr, byval inBuf as ubyte ptr, byval inLen as ulong)
declare sub SDLTest_Md5Final(byval mdContext as SDLTest_Md5Context ptr)
'' -------------------------------------------------------------------------
'' SDL_test_memory.h
'' -------------------------------------------------------------------------
declare sub SDLTest_TrackAllocations()
declare sub SDLTest_RandFillAllocations()
declare sub SDLTest_LogAllocations()
const SDLTEST_MAX_LOGMESSAGE_LENGTH = 3584

end extern

'' end of SDL_test.bi
