'' Project: FreeBASIC SDL examples
'' File: image_test1.bas
'' Purpose:
''     Load JPEG, GIF, and TGA images using SDL1_image.
'' Responsibilities:
''     Validate image streams, display images, and release surfaces.
'' This file intentionally does NOT contain:
''     Image editing or application-owned video memory.
''
' SDL_image example written by Edmond Leung (leung.edmond@gmail.com)
'
' free.jpg, basic.gif and horse.tga are taken from the official freeBasic
' website.

#include  "SDL\SDL.bi"
#include  "SDL\SDL_image.bi"

declare sub blitImage _
   (byval img as SDL_Surface ptr, byval x as integer, byval y as integer)
declare sub releaseImages _
   (byval freeImg as SDL_Surface ptr, byval basicImg as SDL_Surface ptr, byval horseImg as SDL_Surface ptr)

	'' The image blitter reads the video surface initialized by this module.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL301
	dim shared video as SDL_Surface ptr

	dim freeImg as SDL_Surface ptr, basicImg as SDL_Surface ptr, horseImg as SDL_Surface ptr

	dim version as const SDL_version ptr
	version = IMG_Linked_Version()

	' display the version number of the SDL_image being used
	print "Using SDL_image version number: "; SDL_VERSIONNUM(version->major, _
   	version->minor, version->patch)

	' initialise sdl with video support
	if (SDL_Init(SDL_INIT_VIDEO) < 0) then
		print "Couldn't initialise SDL: "; *SDL_GetError()
		end 1
	end if

	' check to see if the images are in the correct formats
	dim freeRw as SDL_RWops ptr, basicRw as SDL_RWops ptr
	freeRw = SDL_RWFromFile("data/free.jpg", "rb")
	if freeRw = NULL then
		print "Couldn't open free.jpg: "; *SDL_GetError()
		SDL_Quit
		end 1
	end if
	if (IMG_isJPG(freeRw) = 0) then
		print "The image (free.jpg) is not a jpg file."
		SDL_RWclose(freeRw)
		SDL_Quit
		end 1
	end if
	basicRw = SDL_RWFromFile("data/basic.gif", "rb")
	if basicRw = NULL then
		print "Couldn't open basic.gif: "; *SDL_GetError()
		SDL_RWclose(freeRw)
		SDL_Quit
		end 1
	end if
	if (IMG_isGIF(basicRw) = 0) then
		print "The image (basic.gif) is not a gif file."
		SDL_RWclose(freeRw)
		SDL_RWclose(basicRw)
		SDL_Quit
		end 1
	end if

	' set the video mode to 640x480x32bpp
	video = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE or SDL_DOUBLEBUF)
	if (video = NULL) then
		print "Couldn't set video mode: "; *SDL_GetError()
		SDL_RWclose(freeRw)
		SDL_RWclose(basicRw)
		SDL_Quit
		end 1
	end if

	' load the images onto an SDL_Surface using three different functions available
	' in the SDL_image library
	freeImg = IMG_LoadJPG_RW(freeRw)
	' IMG_LoadJPG_RW leaves its stream open. IMG_LoadTyped_RW owns the GIF
	' stream because its freesrc argument is one, even if decoding fails.
	SDL_RWclose(freeRw)
	horseImg = IMG_Load("data/horse.tga")
	basicImg = IMG_LoadTyped_RW(basicRw, 1, "gif")
	if freeImg = NULL or basicImg = NULL or horseImg = NULL then
		print "Couldn't load images: "; *IMG_GetError()
		releaseImages freeImg, basicImg, horseImg
		SDL_Quit
		end 1
	end if

	dim done as integer
	done = 0

	do while (done = 0)
		dim event as SDL_Event

		do while (SDL_PollEvent(@event))
			if (event.type = SDL_QUIT_) then done = 1
			if (event.type = SDL_KEYDOWN) then
				if (event.key.keysym.sym = SDLK_ESCAPE) then done = 1
			end if
		loop

		dim destrect as SDL_Rect
		destrect.w = video->w
		destrect.h = video->h

		' clear the screen with the colour white
		SDL_FillRect(video, @destrect, SDL_MapRGB(video->format, 255, 255, 255))

		' draw the images onto the screen
		blitImage freeImg, 245, 205
		blitImage basicImg, 250, 230
		blitImage horseImg, 145, 200

		SDL_Flip(video)
	loop

	releaseImages freeImg, basicImg, horseImg
	SDL_Quit

' sub-routine used to help with blitting the images onto the screen
sub blitImage _
   (byval img as SDL_Surface ptr, byval x as integer, byval y as integer)
	dim dest as SDL_Rect
	dest.x = x
	dest.y = y
	SDL_BlitSurface(img, 0, video, @dest)
end sub

sub releaseImages _
   (byval freeImg as SDL_Surface ptr, byval basicImg as SDL_Surface ptr, byval horseImg as SDL_Surface ptr)
	if freeImg <> NULL then SDL_FreeSurface(freeImg)
	if basicImg <> NULL then SDL_FreeSurface(basicImg)
	if horseImg <> NULL then SDL_FreeSurface(horseImg)
end sub

'' End of image_test1.bas
