'' Project: FreeBASIC SDL3 binding tests
'' File: binding-smoke.bas
'' Purpose: Exercise the new bindings against the pinned native libraries.
'' Responsibilities: Check calling conventions, ownership, and representative APIs.
'' This file intentionally does NOT contain: hardware tests or interactive examples.

#define SDL_ASSERT_LEVEL 2
#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_image.bi"
#include once "SDL3/SDL_mixer.bi"
#include once "SDL3/SDL_ttf.bi"
#include once "SDL3/SDL_textengine.bi"
#include once "SDL3/SDL_net.bi"
#include once "SDL3/SDL_sound.bi"

extern "C"
	declare function SDL3_test_boolean(byval value as boolean) as boolean
	type SDL3_TestCallback as function(byval value as boolean, byval userdata as any ptr) as boolean
	declare function SDL3_test_callback(byval callback as SDL3_TestCallback, byval userdata as any ptr) as boolean
	declare function SDL3_test_color(byval color as SDL_FColor) as SDL_FColor
	declare function SDL3_test_ns_to_seconds(byval value as Uint64) as Uint64
	declare function SDL3_test_ns_to_ms(byval value as Uint64) as Uint64
	declare function SDL3_test_ns_to_us(byval value as Uint64) as Uint64
	declare function SDL3_test_audio_bytes(byval format as SDL_AudioFormat) as long
	declare function SDL3_test_audio_bytes_width() as uinteger
	declare function SDL3_test_audio_frame_bytes(byval spec as const SDL_AudioSpec ptr) as long
	declare function SDL3_test_audio_frame_bytes_width() as uinteger
end extern

'' All diagnostics go through this check so a failure identifies the API and
'' the test still releases resources acquired by the other cases.
dim shared failures as long

private sub check(byval passed as boolean, byref description as const string)
	if passed then exit sub
	print "FAIL: "; description; " ("; *SDL_GetError(); ")"
	failures += 1
end sub

private function invert_boolean cdecl(byval value as boolean, byval userdata as any ptr) as boolean
	return not value
end function

private function thread_worker cdecl(byval userdata as any ptr) as long
	'' Only SDL's atomic operation touches shared state. SDL_WaitThread provides
	'' the synchronization needed before the main thread inspects the result.
	SDL_AddAtomicInt(cptr(SDL_AtomicInt ptr, userdata), 1)
	return 37
end function

'' -------------------------------------------------------------------------
'' Language and C ABI boundaries
'' -------------------------------------------------------------------------

check(SDL3_test_boolean(false) = false, "C false return")
check(SDL3_test_boolean(true) = true, "C true argument and return")
check(SDL3_test_callback(@invert_boolean, 0), "C callback booleans")
dim fcolor as SDL_FColor = (0.25, 0.5, 0.75, 1.0)
fcolor = SDL3_test_color(fcolor)
check(fcolor.r = 0.75 andalso fcolor.g = 0.5 andalso fcolor.b = 0.25 andalso fcolor.a = 1.0, "C structure argument and return")
check(SDL_Swap16(&h1234) = &h3412, "16-bit byte swap")
check(SDL_Swap32(&h12345678u) = &h78563412u, "32-bit byte swap")
check(SDL_Swap64(&h0123456789ABCDEFull) = &hEFCDAB8967452301ull, "64-bit byte swap")
check(SDL_MostSignificantBitIndex32(0) = -1, "zero bit index")
check(SDL_MostSignificantBitIndex32(&h80000000u) = 31, "high bit index")
dim formatted as zstring * 80
SDL_snprintf(@formatted, sizeof(formatted), strptr("%" SDL_PRIu64 "/%" SDL_PRIs64), _
             cast(Uint64, &hFEDCBA9876543210ull), cast(Sint64, -81985529216486895ll))
check(formatted = "18364758544493064720/-81985529216486895", "64-bit variadic formatting")
'' The C macros truncate integral durations. Fractions and values above the
'' exact range of Double expose accidental floating-point translations.
dim duration_ns(0 to 3) as Uint64 = {999ull, 1999999ull, 2999999999ull, &hFFFFFFFFFFFFFFFFull}
for duration_index as long = 0 to 3
	dim duration as Uint64 = duration_ns(duration_index)
	check(SDL_NS_TO_SECONDS(duration) = SDL3_test_ns_to_seconds(duration), "nanoseconds to seconds")
	check(SDL_NS_TO_MS(duration) = SDL3_test_ns_to_ms(duration), "nanoseconds to milliseconds")
	check(SDL_NS_TO_US(duration) = SDL3_test_ns_to_us(duration), "nanoseconds to microseconds")
next
dim audio_format as SDL_AudioFormat = SDL_AUDIO_F32
check(SDL_AUDIO_BYTESIZE(audio_format) = SDL3_test_audio_bytes(audio_format), "audio byte count")
check(sizeof(SDL_AUDIO_BYTESIZE(audio_format)) = SDL3_test_audio_bytes_width(), "audio byte count width")
dim stereo_spec as SDL_AudioSpec = (SDL_AUDIO_F32, 2, 8000)
check(SDL_AUDIO_FRAMESIZE(stereo_spec) = SDL3_test_audio_frame_bytes(@stereo_spec), "audio frame byte count")
check(sizeof(SDL_AUDIO_FRAMESIZE(stereo_spec)) = SDL3_test_audio_frame_bytes_width(), "audio frame byte count width")
dim version_number as long = SDL_VERSION
check(sizeof(SDL_VERSIONNUM_MAJOR(version_number)) = sizeof(long), "version major width")
check(sizeof(SDL_VERSIONNUM_MINOR(version_number)) = sizeof(long), "version minor width")
check(sizeof(SDL_VERSIONNUM_MICRO(version_number)) = sizeof(long), "version micro width")
dim checked_size as uinteger = 19
check(not SDL_size_mul_check_overflow(SDL_SIZE_MAX, 2, @checked_size), "size multiplication overflow")
check(checked_size = 19, "overflow leaves output untouched")
check(SDL_size_mul_check_overflow(7, 9, @checked_size) andalso checked_size = 63, "size multiplication")
dim rect as SDL_Rect = (10, 20, 30, 40)
dim point_value as SDL_Point = (40, 60)
check(not SDL_PointInRect(@point_value, @rect), "integer rectangle excludes boundary")
dim frect as SDL_FRect
SDL_RectToFRect(@rect, @frect)
dim fpoint as SDL_FPoint = (40, 60)
check(SDL_PointInRectFloat(@fpoint, @frect), "floating rectangle includes boundary")
SDL_assert_always(sizeof(SDL_Event) = 128)
SDL_COMPILE_TIME_ASSERT(event_padding, sizeof(SDL_Event) = 128)

#include once "inline-smoke.bi"

'' -------------------------------------------------------------------------
'' Core initialization, properties, I/O, threads, and audio
'' -------------------------------------------------------------------------

if not SDL_Init(SDL_INIT_AUDIO or SDL_INIT_EVENTS) then
	check(false, "SDL_Init")
	end 1
end if
check(SDL_GetVersion() = SDL_VERSION, "SDL3 runtime version")
dim properties as SDL_PropertiesID = SDL_CreateProperties()
check(properties <> 0, "properties allocation")
if properties <> 0 then
	check(SDL_SetBooleanProperty(properties, "test.boolean", true), "boolean property write")
	check(SDL_GetBooleanProperty(properties, "test.boolean", false), "boolean property read")
	check(SDL_SetNumberProperty(properties, "test.number", &h123456789ABCDEFll), "64-bit property write")
	check(SDL_GetNumberProperty(properties, "test.number", 0) = &h123456789ABCDEFll, "64-bit property read")
	SDL_DestroyProperties(properties)
end if
dim bytes(0 to 3) as Uint8 = {&h78, &h56, &h34, &h12}
dim io as SDL_IOStream ptr = SDL_IOFromConstMem(@bytes(0), SDL_arraysize(bytes))
check(io <> 0, "memory I/O allocation")
if io <> 0 then
	dim value as Uint32
	check(SDL_ReadU32LE(io, @value) andalso value = &h12345678u, "little endian I/O")
	check(SDL_CloseIO(io), "I/O close")
end if
dim atomic_count as SDL_AtomicInt
dim worker as SDL_Thread ptr = SDL_CreateThread(@thread_worker, "SDL3 binding test", @atomic_count)
check(worker <> 0, "thread creation")
if worker <> 0 then
	dim result as long
	SDL_WaitThread(worker, @result)
	check(result = 37 andalso SDL_GetAtomicInt(@atomic_count) = 1, "thread callback and join")
end if
dim spec as SDL_AudioSpec = (SDL_AUDIO_F32, 1, 8000)
dim stream as SDL_AudioStream ptr = SDL_CreateAudioStream(@spec, @spec)
check(stream <> 0, "audio stream allocation")
if stream <> 0 then
	dim samples(0 to 15) as single
	'' Sixteen 32-bit samples occupy 64 bytes in the C stream API.
	check(SDL_PutAudioStreamData(stream, @samples(0), 64), "audio stream queue")
	check(SDL_GetAudioStreamQueued(stream) = 64, "audio queue byte count")
	SDL_DestroyAudioStream(stream)
end if

'' -------------------------------------------------------------------------
'' Companion libraries
'' -------------------------------------------------------------------------

dim surface as SDL_Surface ptr = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32)
check(surface <> 0, "surface allocation")
if surface <> 0 then
	check(SDL_FillSurfaceRect(surface, 0, SDL_MapSurfaceRGBA(surface, 17, 34, 51, 255)), "surface fill")
	check(IMG_Version() = SDL_IMAGE_VERSION, "image runtime version")
	check(IMG_SavePNG(surface, command(2)), "PNG write")
	dim loaded as SDL_Surface ptr = IMG_Load(command(2))
	check(loaded <> 0, "PNG read")
	if loaded <> 0 then
		check(loaded->w = 2 andalso loaded->h = 2, "PNG dimensions")
		SDL_DestroySurface(loaded)
	end if
	SDL_DestroySurface(surface)
end if
check(MIX_Init(), "mixer initialization")
dim mixer_value as MIX_Mixer ptr = MIX_CreateMixer(@spec)
check(mixer_value <> 0, "offline mixer allocation")
if mixer_value <> 0 then
	dim audio as MIX_Audio ptr = MIX_LoadAudio(mixer_value, command(1), true)
	check(audio <> 0, "mixer WAV decoding")
	if audio <> 0 then
		check(MIX_GetAudioDuration(audio) > 0, "mixer decoded duration")
		MIX_DestroyAudio(audio)
	end if
	MIX_DestroyMixer(mixer_value)
end if
MIX_Quit()
check(Sound_Init() <> 0, "sound initialization")
dim decoded as Sound_Sample ptr = Sound_NewSampleFromFile(command(1), 0, 4096)
check(decoded <> 0, "sound sample allocation")
if decoded <> 0 then
	check(Sound_DecodeAll(decoded) > 0, "sound WAV decoding")
	check((decoded->flags and SOUND_SAMPLEFLAG_ERROR) = 0, "sound decode status")
	Sound_FreeSample(decoded)
end if
Sound_Quit()
check(TTF_Init(), "TTF initialization")
dim font as TTF_Font ptr = TTF_OpenFont(command(3), 16.0)
check(font <> 0, "font allocation")
if font <> 0 then
	dim foreground as SDL_Color = (255, 255, 255, 255)
	surface = TTF_RenderText_Blended(font, "FreeBASIC SDL3", 0, foreground)
	check(surface <> 0, "UTF-8 font rendering")
	if surface <> 0 then SDL_DestroySurface(surface)
	TTF_CloseFont(font)
end if
TTF_Quit()
check(NET_Init(), "network initialization")
dim address as NET_Address ptr = NET_ResolveHostname("localhost")
check(address <> 0, "localhost resolution allocation")
if address <> 0 then
	check(NET_WaitUntilResolved(address, 3000) = NET_SUCCESS, "asynchronous localhost resolution")
	NET_UnrefAddress(address)
end if
NET_Quit()
SDL_Quit()

if failures = 0 then print "SDL3 binding smoke passed"
if failures <> 0 then end 1
end 0

'' end of binding-smoke.bas
