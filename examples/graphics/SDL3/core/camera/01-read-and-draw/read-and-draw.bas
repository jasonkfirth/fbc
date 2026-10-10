'' Project: FreeBASIC SDL3 examples
'' File: read-and-draw.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/camera/01-read-and-draw/read-and-draw.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example code reads frames from a camera and draws it to the screen.
''
'' This is a very simple approach that is often Good Enough. You can get
'' fancier with this: multiple cameras, front/back facing cameras on phones,
'' color spaces, choosing formats and framerates...this just requests
'' _anything_ and goes with what it is handed.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared camera as SDL_Camera ptr = cptr(SDL_Camera ptr, 0)
dim shared texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
dim shared smoke_camera as boolean

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim devices as SDL_CameraID ptr = cptr(SDL_CameraID ptr, 0)
		dim devcount as long = 0
		SDL_SetAppMetadata(strptr("Example Camera Read and Draw"), strptr("1.0"), strptr("com.example.camera-read-and-draw"))
		smoke_camera = environ("FB_SDL3_SMOKE_CAMERA") <> ""
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_CAMERA)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/camera/read-and-draw"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		devices = SDL_GetCameras(@(devcount))
		if (devices = cptr(SDL_CameraID ptr, (cptr(any ptr, 0)))) then
			scope
				SDL_Log_(strptr("Couldn't enumerate camera devices: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		else
			if (devcount = 0) then
				scope
					SDL_Log_(strptr("Couldn't find any camera devices! Please connect a camera and try again."))
					SDL_free(cptr(any ptr, devices))
					return SDL_APP_FAILURE
				end scope
			end if
		end if
		camera = SDL_OpenCamera(devices[0], cptr(const SDL_CameraSpec ptr, 0))
		'' just take the first thing we see in any format it wants.
		SDL_free(cptr(any ptr, devices))
		if (camera = cptr(SDL_Camera ptr, (cptr(any ptr, 0)))) then
			scope
				SDL_Log_(strptr("Couldn't open camera: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
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
			if (event->type = SDL_EVENT_CAMERA_DEVICE_APPROVED) then
				scope
					SDL_Log_(strptr("Camera use approved by user!"))
				end scope
			else
				if (event->type = SDL_EVENT_CAMERA_DEVICE_DENIED) then
					scope
						SDL_Log_(strptr("Camera use denied by user!"))
						return SDL_APP_FAILURE
					end scope
				end if
			end if
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim timestampNS as Uint64 = 0
		dim frame as SDL_Surface ptr = SDL_AcquireCameraFrame(camera, @(timestampNS))
		if (frame <> cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) then
			scope
				'' Some platforms (like Emscripten) don't know _what_ the camera offers
				'' until the user gives permission, so we build the texture and resize
				'' the window when we get a first frame from the camera.
				if (texture = 0) then
					scope
						'' The runner checks capture as well as initialization. Only
						'' dimensions are recorded; camera pixels stay in the window.
						if smoke_camera then SDL_Log_("SDL3 camera frame: %dx%d", frame->w, frame->h)
						SDL_SetWindowSize(window_, frame->w, frame->h)
						'' Resize the window to match
						SDL_SetRenderLogicalPresentation(renderer, frame->w, frame->h, SDL_LOGICAL_PRESENTATION_LETTERBOX)
						texture = SDL_CreateTexture(renderer, frame->format, SDL_TEXTUREACCESS_STREAMING, frame->w, frame->h)
					end scope
				end if
				if texture then
					scope
						SDL_UpdateTexture(texture, cptr(const SDL_Rect ptr, 0), frame->pixels, frame->pitch)
					end scope
				end if
				SDL_ReleaseCameraFrame(camera, frame)
			end scope
		end if
		SDL_SetRenderDrawColor(renderer, 153, 153, 153, SDL_ALPHA_OPAQUE)
		SDL_RenderClear(renderer)
		if texture then
			scope
				'' draw the latest camera frame, if available.
				SDL_RenderTexture(renderer, texture, cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
			end scope
		end if
		SDL_RenderPresent(renderer)
		if smoke_camera andalso (texture <> 0) then
			'' A captured frame has now been presented. Finish through the
			'' ordinary quit-event path instead of depending on rendering speed.
			dim quit_event as SDL_Event
			quit_event.type = SDL_EVENT_QUIT
			SDL_PushEvent(@quit_event)
		end if
		'' Let capture deliver a frame before a bounded unattended run finishes.
		if smoke_camera then SDL_Delay(10)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_CloseCamera(camera)
		SDL_DestroyTexture(texture)
	end scope
end sub

#include once "callback-main.bi"

'' end of read-and-draw.bas
