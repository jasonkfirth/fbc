'' Project: FreeBASIC SDL3 examples
'' File: points.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/renderer/04-points/points.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This example creates an SDL window and renderer, and then draws some points
'' to it every frame.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define NUM_POINTS 500
#define MIN_PIXELS_PER_SECOND 30
#define MAX_PIXELS_PER_SECOND 60

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared last_time as Uint64 = 0
'' move at least this many pixels per second.
'' move this many pixels per second at most.
'' (track everything as parallel arrays instead of a array of structs,
'' so we can pass the coordinates to the renderer in a single function call.)
'' Points are plotted as a set of X and Y coordinates.
'' (0, 0) is the top left of the window, and larger numbers go down
'' and to the right. This isn't how geometry works, but this is pretty
'' standard in 2D graphics.
dim shared points(0 to 499) as SDL_FPoint
dim shared point_speeds(0 to 499) as single

declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim i as long
		SDL_SetAppMetadata(strptr("Example Renderer Points"), strptr("1.0"), strptr("com.example.renderer-points"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/renderer/points"), 640, 480, SDL_WINDOW_RESIZABLE, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(renderer, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX)
		'' set up the data for a bunch of points.
		scope
			i = 0
			do while (i < (((sizeof(SDL_FPoint) * 500) \ sizeof((points(0))))))
				scope
					points(i).x = (SDL_randf() * (cast(single, 640)))
					points(i).y = (SDL_randf() * (cast(single, 480)))
					point_speeds(i) = (30 + ((SDL_randf() * ((60 - 30)))))
				end scope
				i += 1
			loop
		end scope
		last_time = SDL_GetTicks()
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
		end if
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim now as Uint64 = SDL_GetTicks()
		dim elapsed as single = ((cast(single, ((now - last_time)))) / 1000.0f)
		'' seconds since last iteration
		dim i as long
		'' let's move all our points a little for a new frame.
		scope
			i = 0
			do while (i < (((sizeof(SDL_FPoint) * 500) \ sizeof((points(0))))))
				scope
					dim distance as single = (elapsed * point_speeds(i))
					points(i).x += distance
					points(i).y += distance
					if (((points(i).x >= 640)) orelse ((points(i).y >= 480))) then
						scope
							'' off the screen; restart it elsewhere!
							if SDL_rand(2) then
								scope
									points(i).x = (SDL_randf() * (cast(single, 640)))
									points(i).y = 0.0f
								end scope
							else
								scope
									points(i).x = 0.0f
									points(i).y = (SDL_randf() * (cast(single, 480)))
								end scope
							end if
							point_speeds(i) = (30 + ((SDL_randf() * ((60 - 30)))))
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		last_time = now
		'' as you can see from this, rendering draws over whatever was drawn before it.
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		'' black, full alpha
		SDL_RenderClear(renderer)
		'' start with a blank canvas.
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)
		'' white, full alpha
		SDL_RenderPoints(renderer, @points(0), (((sizeof(SDL_FPoint) * 500) \ sizeof((points(0))))))
		'' draw all the points!
		'' You can also draw single points with SDL_RenderPoint(), but it's
		'' cheaper (sometimes significantly so) to do them all at once.
		SDL_RenderPresent(renderer)
		'' put it all on the screen!
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
	end scope
end sub

#include once "callback-main.bi"

'' end of points.bas
