'' Project: FreeBASIC SDL3 examples
'' File: metadata.bas
'' Purpose: Port upstream SDL3_mixer-3.2.4/examples/basics/04-metadata/metadata.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code loads a single sound, and displays metadata.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_mixer.bi"

'' use the callbacks instead of main()
dim shared audio as MIX_Audio ptr = cptr(MIX_Audio ptr, 0)
dim shared path as zstring ptr = cptr(zstring ptr, 0)
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare sub RenderMetadata cdecl(byval userdata as any ptr, byval props as SDL_PropertiesID, byval name_ as const zstring ptr)
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		SDL_SetAppMetadata(strptr("Example Metadata"), strptr("1.0"), strptr("com.example.metadata"))
		'' this doesn't have to run very much, so give up tons of CPU time between iterations. Optional!
		SDL_SetHint(strptr(SDL_HINT_MAIN_CALLBACK_RATE), strptr("5"))
		'' we don't need video, but we'll make a window for smooth operation.
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/basic/metadata"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (MIX_Init() = 0) then
			scope
				SDL_Log_(strptr("Couldn't init SDL_mixer library: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_asprintf(@(path), strptr("%smusic.mp3"), SDL_GetBasePath())
		'' allocate a string of the full file path
		audio = MIX_LoadAudio(cptr(MIX_Mixer ptr, 0), path, false)
		'' MIX_Audios are shared between mixers, so you can pass a NULL mixer here; non-NULL just lets it optimize for a specific output.
		'' if MIX_LoadAudio failed, it's okay, the user can drop a new file on the window.
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs when a new event (mouse input, keypresses, etc) occurs.
function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		if (event->type = SDL_EVENT_QUIT) then
			scope
				return SDL_APP_SUCCESS
			end scope
		else
			if (event->type = SDL_EVENT_DROP_FILE) then
				scope
					MIX_DestroyAudio(audio)
					SDL_free(cptr(any ptr, path))
					path = SDL_strdup(event->drop.data)
					audio = MIX_LoadAudio(cptr(MIX_Mixer ptr, 0), path, false)
				end scope
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

sub RenderMetadata cdecl(byval userdata as any ptr, byval props as SDL_PropertiesID, byval name_ as const zstring ptr)
	scope
		dim y as single ptr = cptr(single ptr, userdata)
		select case SDL_GetPropertyType(props, name_)
			case SDL_PROPERTY_TYPE_INVALID
				goto switch_case_1
			case SDL_PROPERTY_TYPE_POINTER
				goto switch_case_2
			case SDL_PROPERTY_TYPE_STRING
				goto switch_case_3
			case SDL_PROPERTY_TYPE_NUMBER
				goto switch_case_4
			case SDL_PROPERTY_TYPE_FLOAT
				goto switch_case_5
			case SDL_PROPERTY_TYPE_BOOLEAN
				goto switch_case_6
			case else
				goto switch_case_7
		end select
		switch_case_1:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(" - %s [invalid type]"), name_)
			goto switch_done_8
		end scope
		switch_case_2:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(" - %s [pointer=%p]"), name_, SDL_GetPointerProperty(props, name_, (cptr(any ptr, 0))))
			goto switch_done_8
		end scope
		switch_case_3:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(!" - %s [string=\"%s\"]"), name_, SDL_GetStringProperty(props, name_, strptr("")))
			goto switch_done_8
		end scope
		switch_case_4:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(" - %s [number=%" SDL_PRIs64 "]"), name_, cast(Sint64, SDL_GetNumberProperty(props, name_, 0)))
			goto switch_done_8
		end scope
		switch_case_5:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(" - %s [float=%f]"), name_, cast(double, cast(double, SDL_GetFloatProperty(props, name_, 0.0f))))
			goto switch_done_8
		end scope
		switch_case_6:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(" - %s [boolean=%s]"), name_, iif(SDL_GetBooleanProperty(props, name_, false), strptr("true"), strptr("false")))
			goto switch_done_8
		end scope
		switch_case_7:
		scope
			SDL_RenderDebugTextFormat(renderer, 0.0f, (*y), strptr(" - %s [unknown type]"), name_)
			goto switch_done_8
		end scope
		switch_done_8:
		(*y) += (SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE + 2)
	end scope
end sub

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		'' audio file metadata is stored in an SDL Properties group; you can also hang any app-specific metadata on here, too!
		dim props as SDL_PropertiesID = MIX_GetAudioProperties(audio)
		dim audiospec as SDL_AudioSpec
		dim y as single = 0.0f
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		'' black
		SDL_RenderClear(renderer)
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		'' white
		MIX_GetAudioFormat(audio, @(audiospec))
		SDL_RenderDebugText(renderer, 0.0f, y, strptr("== Drop a file on this window to get its metadata. =="))
		y += (cast(single, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) * 2.0f)
		if audio then
			scope
				SDL_RenderDebugText(renderer, 0.0f, y, path)
				y += (cast(single, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) + 2.0f)
				SDL_RenderDebugTextFormat(renderer, 0.0f, y, strptr("%s, %d channel%s, %d freq"), SDL_GetAudioFormatName(audiospec.format), cast(long, audiospec.channels), iif(((audiospec.channels = 1)), strptr(""), strptr("s")), cast(long, audiospec.freq))
				y += (cast(single, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) * 2.0f)
				'' now draw all the metadata. It happens in the RenderMetadata function, once per metadata item.
				SDL_EnumerateProperties(props, @RenderMetadata, cptr(any ptr, @(y)))
			end scope
		end if
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		'' SDL will clean up the window/renderer for us, MIX_Quit() destroys any mixer objects we made.
		MIX_Quit()
		SDL_free(cptr(any ptr, path))
	end scope
end sub

#include once "callback-main.bi"

'' end of metadata.bas
