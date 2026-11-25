'' Project: FreeBASIC SDL3 examples
'' File: callback-main.bi
'' Purpose: Start the translated examples through SDL's callback main loop.
'' Responsibilities: Pass BASIC's arguments and support bounded smoke runs.
'' This file intentionally does NOT contain: rendering, input, or application state.

#pragma once

'' SDL_RunApp handles platform startup. SDL_EnterAppMainCallbacks owns event
'' dispatch and calls SDL_AppQuit before shutting SDL down. The four application
'' callbacks are defined by the example before this file is included.
'' The browser calls these functions on its main thread after startup returns.
dim shared SDL3_ExampleFrameLimit as integer

private function SDL3_ExampleIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	static frames as ulong
	dim result as SDL_AppResult = SDL_AppIterate(appstate)
	if result <> SDL_APP_CONTINUE then return result
	if SDL3_ExampleFrameLimit > 0 then
		frames += 1
		'' Sixteen iterations exercise initialization, drawing, and cleanup while
		'' keeping unattended examples independent of keyboard or mouse input.
		if frames >= cuint(SDL3_ExampleFrameLimit) then return SDL_APP_SUCCESS
	end if
	return SDL_APP_CONTINUE
end function

#ifdef __FB_JS__
	extern "C"
		declare sub emscripten_async_call(byval callback as sub cdecl(byval as any ptr), _
			byval context as any ptr, byval milliseconds as long)
		declare sub emscripten_force_exit(byval status as long)
	end extern

	private sub SDL3_ExampleExit cdecl(byval context as any ptr)
		emscripten_force_exit(clng(cuint(context)))
	end sub
#endif

private sub SDL3_ExampleQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	SDL_AppQuit(appstate, result)
	#ifdef __FB_JS__
		if SDL3_ExampleFrameLimit > 0 then
			'' Report a bounded run's exit after SDL finishes its own
			'' callback shutdown and releases the browser main loop.
			dim status as ulong = 0
			if result = SDL_APP_FAILURE then status = 1
			emscripten_async_call(@SDL3_ExampleExit, cptr(any ptr, status), 0)
		end if
	#endif
end sub

private function SDL3_ExampleMain cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	SDL3_ExampleFrameLimit = valint(environ("FB_SDL3_SMOKE_FRAMES"))
	if argc > 1 andalso argv <> 0 andalso argv[1] <> 0 then
		if *argv[1] = "--smoke" then SDL3_ExampleFrameLimit = 16
	end if
	return SDL_EnterAppMainCallbacks(argc, argv, @SDL_AppInit, @SDL3_ExampleIterate, @SDL_AppEvent, @SDL3_ExampleQuit)
end function

#ifdef __FB_JS__
	'' SDL_RunApp returns after registering the web loop. SDL owns shutdown
	'' when a later frame finishes. END here would exit the BASIC runtime
	'' while the browser still has pending callbacks.
	dim as long sdl3_start_status = SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @SDL3_ExampleMain, 0)
	if sdl3_start_status <> 0 then end sdl3_start_status
#else
end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @SDL3_ExampleMain, 0)
#endif

'' end of callback-main.bi
