'' Project: FreeBASIC SDL3 examples
'' File: callback-main.bi
'' Purpose: Start the translated examples through SDL's callback main loop.
'' Responsibilities: Pass BASIC's arguments and support bounded smoke runs.
'' This file intentionally does NOT contain: rendering, input, or application state.

#pragma once

'' SDL_RunApp handles platform startup. SDL_EnterAppMainCallbacks owns event
'' dispatch and calls SDL_AppQuit before shutting SDL down. The four application
'' callbacks are defined by the example before this file is included.
private function SDL3_ExampleIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	static frames as ulong
	static frame_limit as integer
	static initialized as boolean
	if not initialized then
		frame_limit = valint(environ("FB_SDL3_SMOKE_FRAMES"))
		initialized = true
	end if
	dim result as SDL_AppResult = SDL_AppIterate(appstate)
	if result <> SDL_APP_CONTINUE then return result
	if command(1) = "--smoke" then frame_limit = 16
	if frame_limit > 0 then
		frames += 1
		'' Sixteen iterations exercise initialization, drawing, and cleanup while
		'' keeping unattended examples independent of keyboard or mouse input.
		if frames >= cuint(frame_limit) then return SDL_APP_SUCCESS
	end if
	return SDL_APP_CONTINUE
end function

private function SDL3_ExampleMain cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	return SDL_EnterAppMainCallbacks(argc, argv, @SDL_AppInit, @SDL3_ExampleIterate, @SDL_AppEvent, @SDL_AppQuit)
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @SDL3_ExampleMain, 0)

'' end of callback-main.bi
