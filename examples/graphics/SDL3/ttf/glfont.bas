'' Project: FreeBASIC SDL3 examples
'' File: glfont.bas
'' Purpose: Port upstream SDL3_ttf-3.2.2/examples/glfont.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' glfont:  An example of using the SDL_ttf library with OpenGL.
'' Copyright (C) 2001-2025 Sam Lantinga <slouken@libsdl.org>
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
'' claim that you wrote the original software. If you use this software
'' in a product, an acknowledgment in the product documentation would be
'' appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
'' misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#include once "SDL3/SDL.bi"
#include once "smoke.bi"
#include once "SDL3/SDL_ttf.bi"
#include once "crt.bi"
#include once "GL/gl.bi"
#include once "GL/glext.bi"

#define DEFAULT_PTSIZE 18.0f
#define DEFAULT_TEXT "The quick brown fox jumped over the lazy dog"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define TTF_GLFONT_USAGE !"Usage: %s [-b] [-i] [-u] [--fgcol r,g,b] [--bgcol r,g,b] <font>.ttf [ptsize] [text]\n"

declare sub SDL_GL_Enter2DMode cdecl(byval width_ as long, byval height as long)
declare sub SDL_GL_Leave2DMode cdecl()
declare function power_of_two cdecl(byval input_ as long) as long
declare function SDL_GL_LoadTexture cdecl(byval surface as SDL_Surface ptr, byval texcoord as GLfloat ptr) as GLuint
declare sub cleanup cdecl(byval exitcode as long)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

'' A simple program to test the text rendering feature of the TTF library
'' quiet windows compiler warnings
sub SDL_GL_Enter2DMode cdecl(byval width_ as long, byval height as long)
	scope
		'' Note, there may be other things you need to change,
		'' depending on how you have your OpenGL state set up.
		glPushAttrib(8192)
		glDisable(2929)
		glDisable(2884)
		glEnable(3553)
		'' This allows alpha blending of 2D textures with the scene
		glEnable(3042)
		glBlendFunc(770, 771)
		glViewport(0, 0, width_, height)
		glMatrixMode(5889)
		glPushMatrix()
		glLoadIdentity()
		glOrtho(0.0, cast(GLdouble, width_), cast(GLdouble, height), 0.0, 0.0, 1.0)
		glMatrixMode(5888)
		glPushMatrix()
		glLoadIdentity()
		glTexEnvf(8960, 8704, 8448)
	end scope
end sub

sub SDL_GL_Leave2DMode cdecl()
	scope
		glMatrixMode(5888)
		glPopMatrix()
		glMatrixMode(5889)
		glPopMatrix()
		glPopAttrib()
	end scope
end sub

'' Quick utility function for texture creation
function power_of_two cdecl(byval input_ as long) as long
	scope
		dim value as long = 1
		do
			if ((value < input_)) = 0 then exit do
			scope
				value shl= 1
			end scope
		loop
		return value
	end scope
end function

function SDL_GL_LoadTexture cdecl(byval surface as SDL_Surface ptr, byval texcoord as GLfloat ptr) as GLuint
	scope
		dim texture as GLuint
		dim w as long
		dim h as long
		dim image as SDL_Surface ptr
		dim area as SDL_Rect
		dim saved_alpha as Uint8
		dim saved_mode as SDL_BlendMode
		'' Use the surface width and height expanded to powers of 2
		w = power_of_two(surface->w)
		h = power_of_two(surface->h)
		texcoord[0] = 0.0f
		'' Min X
		texcoord[1] = 0.0f
		'' Min Y
		texcoord[2] = (cast(GLfloat, surface->w) / w)
		'' Max X
		texcoord[3] = (cast(GLfloat, surface->h) / h)
		'' Max Y
		image = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32)
		if (image = cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) then
			scope
				return 0
			end scope
		end if
		'' Save the alpha blending attributes
		SDL_GetSurfaceAlphaMod(surface, @(saved_alpha))
		SDL_SetSurfaceAlphaMod(surface, 255)
		SDL_GetSurfaceBlendMode(surface, @(saved_mode))
		SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE)
		'' Copy the surface into the GL texture image
		area.x = 0
		area.y = 0
		area.w = surface->w
		area.h = surface->h
		SDL_BlitSurface(surface, @(area), image, @(area))
		'' Restore the alpha blending attributes
		SDL_SetSurfaceAlphaMod(surface, saved_alpha)
		SDL_SetSurfaceBlendMode(surface, saved_mode)
		'' Create an OpenGL texture for the image
		glGenTextures(1, @(texture))
		glBindTexture(3553, texture)
		glTexParameteri(3553, 10240, 9728)
		glTexParameteri(3553, 10241, 9728)
		glTexImage2D(3553, 0, 6408, w, h, 0, 6408, 5121, image->pixels)
		SDL_DestroySurface(image)
		'' No longer needed
		return texture
	end scope
end function

sub cleanup cdecl(byval exitcode as long)
	scope
		TTF_Quit()
		SDL_Quit()
		exit_(exitcode)
	end scope
end sub

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim argv0 as zstring ptr = argv[0]
		dim window_ as SDL_Window ptr
		dim context as SDL_GLContext
		dim font as TTF_Font ptr
		dim text as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		dim ptsize as single
		dim i as long
		dim done as long
		dim white as SDL_Color = type<SDL_Color>(255, 255, 255, SDL_ALPHA_OPAQUE)
		dim black as SDL_Color = type<SDL_Color>(0, 0, 0, SDL_ALPHA_OPAQUE)
		dim forecol as SDL_Color ptr
		dim backcol as SDL_Color ptr
		dim gl_error as GLenum
		dim texture as GLuint
		dim x as long
		dim y as long
		dim w as long
		dim h as long
		dim texcoord(0 to 3) as GLfloat
		dim texMinX as GLfloat
		dim texMinY as GLfloat
		dim texMaxX as GLfloat
		dim texMaxY as GLfloat
		dim color_(0 to 7, 0 to 2) as single = {{cast(single, 1.0), cast(single, 1.0), cast(single, 0.0)}, {cast(single, 1.0), cast(single, 0.0), cast(single, 0.0)}, {cast(single, 0.0), cast(single, 0.0), cast(single, 0.0)}, {cast(single, 0.0), cast(single, 1.0), cast(single, 0.0)}, {cast(single, 0.0), cast(single, 1.0), cast(single, 1.0)}, {cast(single, 1.0), cast(single, 1.0), cast(single, 1.0)}, {cast(single, 1.0), cast(single, 0.0), cast(single, 1.0)}, {cast(single, 0.0), cast(single, 0.0), cast(single, 1.0)}}
		dim cube(0 to 7, 0 to 2) as single = {{cast(single, 0.5), cast(single, 0.5), cast(single, (-0.5))}, {cast(single, 0.5), cast(single, (-0.5)), cast(single, (-0.5))}, {cast(single, (-0.5)), cast(single, (-0.5)), cast(single, (-0.5))}, {cast(single, (-0.5)), cast(single, 0.5), cast(single, (-0.5))}, {cast(single, (-0.5)), cast(single, 0.5), cast(single, 0.5)}, {cast(single, 0.5), cast(single, 0.5), cast(single, 0.5)}, {cast(single, 0.5), cast(single, (-0.5)), cast(single, 0.5)}, {cast(single, (-0.5)), cast(single, (-0.5)), cast(single, 0.5)}}
		dim event as SDL_Event
		dim renderstyle as long
		dim dump as long
		dim message as zstring ptr
		'' Look for special execution mode
		dump = 0
		'' Look for special rendering types
		renderstyle = TTF_STYLE_NORMAL
		'' Default is black and white
		forecol = @(black)
		backcol = @(white)
		scope
			i = 1
			do while (argv[i] andalso (cptr(byte ptr, argv[i])[0] = 45))
				scope
					if (SDL_strcmp(argv[i], strptr("-b")) = 0) then
						scope
							renderstyle or= TTF_STYLE_BOLD
						end scope
					else
						if (SDL_strcmp(argv[i], strptr("-i")) = 0) then
							scope
								renderstyle or= TTF_STYLE_ITALIC
							end scope
						else
							if (SDL_strcmp(argv[i], strptr("-u")) = 0) then
								scope
									renderstyle or= TTF_STYLE_UNDERLINE
								end scope
							else
								if (SDL_strcmp(argv[i], strptr("--dump")) = 0) then
									scope
										dump = 1
									end scope
								else
									if (SDL_strcmp(argv[i], strptr("--fgcol")) = 0) then
										scope
											dim r as long
											dim g as long
											dim b as long
											i += 1
											if (sscanf(argv[i], strptr("%d,%d,%d"), @(r), @(g), @(b)) <> 3) then
												scope
													fprintf(stderr, strptr(TTF_GLFONT_USAGE), argv0)
													return (1)
												end scope
											end if
											forecol->r = cast(Uint8, r)
											forecol->g = cast(Uint8, g)
											forecol->b = cast(Uint8, b)
										end scope
									else
										if (SDL_strcmp(argv[i], strptr("--bgcol")) = 0) then
											scope
												dim r as long
												dim g as long
												dim b as long
												i += 1
												if (sscanf(argv[i], strptr("%d,%d,%d"), @(r), @(g), @(b)) <> 3) then
													scope
														fprintf(stderr, strptr(TTF_GLFONT_USAGE), argv0)
														return (1)
													end scope
												end if
												backcol->r = cast(Uint8, r)
												backcol->g = cast(Uint8, g)
												backcol->b = cast(Uint8, b)
											end scope
										else
											scope
												fprintf(stderr, strptr(TTF_GLFONT_USAGE), argv0)
												return (1)
											end scope
										end if
									end if
								end if
							end if
						end if
					end if
				end scope
				i += 1
			loop
		end scope
		argv += i
		argc -= i
		'' Check usage
		if (argv[0] = 0) then
			scope
				fprintf(stderr, strptr(TTF_GLFONT_USAGE), argv0)
				return (1)
			end scope
		end if
		'' Initialize the TTF library
		if (TTF_Init() = 0) then
			scope
				fprintf(stderr, strptr(!"Couldn't initialize TTF: %s\n"), SDL_GetError())
				SDL_Quit()
				return (2)
			end scope
		end if
		'' Open the font file with the requested point size
		ptsize = 0.0f
		if (argc > 1) then
			scope
				ptsize = cast(single, SDL_atof(argv[1]))
			end scope
		end if
		if (ptsize = 0.0f) then
			scope
				i = 2
				ptsize = 18.0f
			end scope
		else
			scope
				i = 3
			end scope
		end if
		font = TTF_OpenFont(argv[0], ptsize)
		if (font = cptr(TTF_Font ptr, (cptr(any ptr, 0)))) then
			scope
				fprintf(stderr, strptr(!"Couldn't load %g pt font from %s: %s\n"), cast(double, cast(double, ptsize)), argv[0], SDL_GetError())
				cleanup(2)
			end scope
		end if
		TTF_SetFontStyle(font, renderstyle)
		if dump then
			scope
				scope
					i = 48
					do while (i < 123)
						scope
							dim glyph as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
							glyph = TTF_RenderGlyph_Shaded(font, i, (*forecol), (*backcol))
							if glyph then
								scope
									dim outname(0 to 63) as byte
									sprintf(@outname(0), strptr("glyph-%d.bmp"), cast(long, i))
									SDL_SaveBMP(glyph, @outname(0))
								end scope
							end if
						end scope
						i += 1
					loop
				end scope
				cleanup(0)
			end scope
		end if
		'' Set a 640x480 video mode
		window_ = SDL_CreateWindow(strptr("glfont"), 640, 480, SDL_WINDOW_OPENGL)
		if (window_ = cptr(SDL_Window ptr, (cptr(any ptr, 0)))) then
			scope
				fprintf(stderr, strptr(!"Couldn't create window: %s\n"), SDL_GetError())
				cleanup(2)
			end scope
		end if
		context = SDL_GL_CreateContext(window_)
		if (context = cast(SDL_GLContext, (cptr(any ptr, 0)))) then
			scope
				fprintf(stderr, strptr(!"Couldn't create OpenGL context: %s\n"), SDL_GetError())
				cleanup(2)
			end scope
		end if
		'' Render and center the message
		if (argc > 2) then
			scope
				message = argv[2]
			end scope
		else
			scope
				message = strptr("The quick brown fox jumped over the lazy dog")
			end scope
		end if
		text = TTF_RenderText_Blended(font, message, 0, (*forecol))
		if (text = cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) then
			scope
				fprintf(stderr, strptr(!"Couldn't render text: %s\n"), SDL_GetError())
				TTF_CloseFont(font)
				cleanup(2)
			end scope
		end if
		x = (((640 - text->w)) \ 2)
		y = (((480 - text->h)) \ 2)
		w = text->w
		h = text->h
		printf(strptr(!"Font is generally %d big, and string is %d big\n"), cast(long, TTF_GetFontHeight(font)), cast(long, text->h))
		'' Convert the text into an OpenGL texture
		glGetError()
		texture = SDL_GL_LoadTexture(text, @texcoord(0))
		gl_error = glGetError()
		if ((gl_error) <> 0) then
			scope
				'' If this failed, the text may exceed texture size limits
				printf(strptr(!"Warning: Couldn't create texture: 0x%x\n"), cast(GLenum, gl_error))
			end scope
		end if
		'' Make texture coordinates easy to understand
		texMinX = texcoord(0)
		texMinY = texcoord(1)
		texMaxX = texcoord(2)
		texMaxY = texcoord(3)
		'' We don't need the original text surface anymore
		SDL_DestroySurface(text)
		'' Initialize the GL state
		glViewport(0, 0, 640, 480)
		glMatrixMode(5889)
		glLoadIdentity()
		glOrtho((-2.0), 2.0, (-2.0), 2.0, (-20.0), 20.0)
		glMatrixMode(5888)
		glLoadIdentity()
		glEnable(2929)
		glDepthFunc(513)
		glShadeModel(7425)
		'' Wait for a keystroke, and blit text on mouse press
		done = 0
		do
			if ((done = 0)) = 0 then exit do
			scope
				do
					if (SDL_PollEvent(@(event))) = 0 then exit do
					scope
						select case event.type
							case SDL_EVENT_MOUSE_MOTION
								goto switch_case_6
							case SDL_EVENT_KEY_DOWN, SDL_EVENT_QUIT
								goto switch_case_7
							case else
								goto switch_case_8
						end select
						switch_case_6:
						scope
							x = cast(long, ((event.motion.x - (w \ 2))))
							y = cast(long, ((event.motion.y - (h \ 2))))
							goto switch_done_9
						end scope
						switch_case_7:
						scope
							done = 1
							goto switch_done_9
						end scope
						switch_case_8:
						scope
							goto switch_done_9
						end scope
						switch_done_9:
					end scope
				loop
				'' Clear the screen
				glClearColor(cast(GLclampf, 1.0), cast(GLclampf, 1.0), cast(GLclampf, 1.0), cast(GLclampf, 1.0))
				glClear((16384 or 256))
				'' Draw the spinning cube
				glBegin(7)
				glColor3fv((@color_(0, 0) + (0) * 3))
				glVertex3fv((@cube(0, 0) + (0) * 3))
				glColor3fv((@color_(0, 0) + (1) * 3))
				glVertex3fv((@cube(0, 0) + (1) * 3))
				glColor3fv((@color_(0, 0) + (2) * 3))
				glVertex3fv((@cube(0, 0) + (2) * 3))
				glColor3fv((@color_(0, 0) + (3) * 3))
				glVertex3fv((@cube(0, 0) + (3) * 3))
				glColor3fv((@color_(0, 0) + (3) * 3))
				glVertex3fv((@cube(0, 0) + (3) * 3))
				glColor3fv((@color_(0, 0) + (4) * 3))
				glVertex3fv((@cube(0, 0) + (4) * 3))
				glColor3fv((@color_(0, 0) + (7) * 3))
				glVertex3fv((@cube(0, 0) + (7) * 3))
				glColor3fv((@color_(0, 0) + (2) * 3))
				glVertex3fv((@cube(0, 0) + (2) * 3))
				glColor3fv((@color_(0, 0) + (0) * 3))
				glVertex3fv((@cube(0, 0) + (0) * 3))
				glColor3fv((@color_(0, 0) + (5) * 3))
				glVertex3fv((@cube(0, 0) + (5) * 3))
				glColor3fv((@color_(0, 0) + (6) * 3))
				glVertex3fv((@cube(0, 0) + (6) * 3))
				glColor3fv((@color_(0, 0) + (1) * 3))
				glVertex3fv((@cube(0, 0) + (1) * 3))
				glColor3fv((@color_(0, 0) + (5) * 3))
				glVertex3fv((@cube(0, 0) + (5) * 3))
				glColor3fv((@color_(0, 0) + (4) * 3))
				glVertex3fv((@cube(0, 0) + (4) * 3))
				glColor3fv((@color_(0, 0) + (7) * 3))
				glVertex3fv((@cube(0, 0) + (7) * 3))
				glColor3fv((@color_(0, 0) + (6) * 3))
				glVertex3fv((@cube(0, 0) + (6) * 3))
				glColor3fv((@color_(0, 0) + (5) * 3))
				glVertex3fv((@cube(0, 0) + (5) * 3))
				glColor3fv((@color_(0, 0) + (0) * 3))
				glVertex3fv((@cube(0, 0) + (0) * 3))
				glColor3fv((@color_(0, 0) + (3) * 3))
				glVertex3fv((@cube(0, 0) + (3) * 3))
				glColor3fv((@color_(0, 0) + (4) * 3))
				glVertex3fv((@cube(0, 0) + (4) * 3))
				glColor3fv((@color_(0, 0) + (6) * 3))
				glVertex3fv((@cube(0, 0) + (6) * 3))
				glColor3fv((@color_(0, 0) + (1) * 3))
				glVertex3fv((@cube(0, 0) + (1) * 3))
				glColor3fv((@color_(0, 0) + (2) * 3))
				glVertex3fv((@cube(0, 0) + (2) * 3))
				glColor3fv((@color_(0, 0) + (7) * 3))
				glVertex3fv((@cube(0, 0) + (7) * 3))
				glEnd()
				'' Rotate the cube
				glMatrixMode(5888)
				glRotatef(cast(GLfloat, 5.0), cast(GLfloat, 1.0), cast(GLfloat, 1.0), cast(GLfloat, 1.0))
				'' Show the text on the screen
				SDL_GL_Enter2DMode(640, 480)
				glBindTexture(3553, texture)
				glBegin(5)
				glTexCoord2f(texMinX, texMinY)
				glVertex2i(x, y)
				glTexCoord2f(texMaxX, texMinY)
				glVertex2i((x + w), y)
				glTexCoord2f(texMinX, texMaxY)
				glVertex2i(x, (y + h))
				glTexCoord2f(texMaxX, texMaxY)
				glVertex2i((x + w), (y + h))
				glEnd()
				SDL_GL_Leave2DMode()
				'' Swap the buffers so everything is visible
				SDL_GL_SwapWindow(window_)
				SDL3_ExampleSmokeFrame()
			end scope
		loop
		SDL_GL_DestroyContext(context)
		TTF_CloseFont(font)
		cleanup(0)
		'' Not reached, but fixes compiler warnings
		return 0
	end scope
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of glfont.bas
