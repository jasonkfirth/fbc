'' Project: FreeBASIC SDL3 examples
'' File: smoke.bi
'' Purpose: Bound unattended runs of examples that normally wait for input.
'' Responsibilities: Post a local quit event after the requested frame count.
'' This file intentionally does NOT contain: rendering, assets, or application state.

#pragma once

'' FB_SDL3_SMOKE_FRAMES is set by the example test runner. Without it, the
'' examples retain their normal interactive lifecycle. SDL_PushEvent wakes
'' the same event loop that handles an ordinary request to close the window.
private sub SDL3_ExampleSmokeFrame()
	static frame_limit as integer
	static initialized as boolean
	static frames as integer
	if not initialized then
		frame_limit = valint(environ("FB_SDL3_SMOKE_FRAMES"))
		initialized = true
	end if
	if frame_limit <= 0 then exit sub
	if frames >= frame_limit then exit sub
	frames += 1
	if frames = frame_limit then
		dim quit_event as SDL_Event
		quit_event.type = SDL_EVENT_QUIT
		SDL_PushEvent(@quit_event)
	end if
end sub

'' end of smoke.bi
