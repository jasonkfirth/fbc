'' Project: FreeBASIC SDL addon examples
'' File: bgi2.bas
'' Purpose: Draw Borland-style primitives and text with SDL2_bgi.
'' Responsibilities: Check initialization, draw a scene, and release the BGI window.
'' This file intentionally does NOT contain: direct framebuffer writes or input threads.

#define SDL_ADDON_API 2
#include once "../../SDL_common/example-common.bi"
#include once "SDL2/SDL_bgi.bi"

function main() as integer
	'' BGI's auto mode renders from a timer thread. This example presents
	'' explicitly, so select compatible mode before the window is created.
	if SDL_setenv("SDL_BGI_RATE", "compatible", 1) <> 0 then
		example_error("could not select BGI refresh mode")
		return 1
	end if
	if bgi.initwindow(640, 480, strptr("SDL2 BGI")) < 0 then
		example_error("initwindow failed")
		return 1
	end if
	'' Batch drawing calls and present the completed scene once.
	bgi.sdlbgifast()
	bgi.setbkcolor(bgi.BLACK)
	bgi.cleardevice()
	bgi.setcolor(bgi.YELLOW)
	bgi.circle(320, 220, 100)
	bgi.setcolor(bgi.LIGHTCYAN)
	bgi.rectangle(170, 70, 470, 370)
	bgi.setcolor(bgi.WHITE)
	bgi.outtextxy(180, 410, strptr("FreeBASIC SDL2_bgi"))
	bgi.refresh()
	do
	loop until example_done()
	bgi.closegraph()
	return 0
end function

end main()

'' End of bgi2.bas
