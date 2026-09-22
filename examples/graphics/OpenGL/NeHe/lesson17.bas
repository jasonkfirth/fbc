''
'' Project: FreeBASIC OpenGL examples
'' File: lesson17.bas
''
'' Purpose:
''     Demonstrate textured OpenGL display-list fonts over rotating geometry.
''
'' Ownership:
''     The render loop owns the two texture names and the font display-list
''     base.  BuildFont and glPrint share those names only within this module.
''
'' This file intentionally does NOT contain:
''     - a general text layout engine
''     - reusable texture or display-list ownership outside this lesson
''
'' This Code Was Created By Jeff Molofee 2000
'' A HUGE Thanks To Fredric Echols For Cleaning Up
'' And Optimizing The Base Code, Making It More Flexible!
'' If You've Found This Code Useful, Please Let Me Know.
'' Visit My Site At nehe.gamedev.net
''
''------------------------------------------------------------------------------
'' Use ESC key to quit
''
''------------------------------------------------------------------------------

'' compile as: fbc -s gui lesson17.bas



#include once "GL/gl.bi"
#include once "GL/glu.bi"
#include once "fbgfx.bi"                   '' for Scan code constants
#include once "createtex.bi"

'' Setup our booleans

declare sub BuildFont()
declare sub glPrint(byval x as integer, byval y as integer, byref glstring as string, byval gset as integer)


'' The one render loop and its two helpers share the OpenGL font resources.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
dim shared gbase as uinteger                      '' Base Display List For The Font
dim shared texture(0 to 1) as GLuint            '' Storage For Our Font Texture

	dim cnt1 as single                            '' 1st Counter Used To Move Text & For Coloring
	dim cnt2 as single                            '' 2nd Counter Used To Move Text & For Coloring

	screen 18, 16, , 2

	'' ReSizeGLScene
	glViewport 0, 0, 640, 480                      '' Reset The Current Viewport
	glMatrixMode GL_PROJECTION                     '' Select The Projection Matrix
	glLoadIdentity                                 '' Reset The Projection Matrix
	gluPerspective 45.0, 640.0/480.0, 0.1, 100.0   '' Calculate The Aspect Ratio Of The Window
	glMatrixMode GL_MODELVIEW                      '' Select The Modelview Matrix
	glLoadIdentity                                 '' Reset The Modelview Matrix

	'' Use BLOAD to load the bitmaps.
	redim buffer(0 to 256*256*4+4) as ubyte  '' Size = Width x Height x 4 bytes per pixel + 4 bytes for header
	if ubound(buffer) < lbound(buffer) then end 1
	bload exepath + "/data/Font.bmp", @buffer(lbound(buffer))        '' BLOAD the bitmap
	'' BLOAD reports a failed file transfer through FreeBASIC's immediate Err value.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL613
	if err <> 0 then end 1
	texture(0) = CreateTexture(@buffer(lbound(buffer)))   '' Linear Texture
	bload exepath + "/data/Bumps.bmp", @buffer(lbound(buffer))       '' BLOAD the bitmap
	'' FB-LINTER: DISABLE-NEXT-LINE FBL613
	if err <> 0 then
		glDeleteTextures(2, @texture(0))
		end 1
	end if
	texture(1) = CreateTexture(@buffer(lbound(buffer)))   '' Linear Texture

	'' Exit if error loading textures
	if texture(0) = 0 or texture(1) = 0 then
		glDeleteTextures(2, @texture(0))
		end 1
	end if

	'' All Setup For OpenGL Goes Here
	BuildFont                                '' Build The Font
	if gbase = 0 then
		glDeleteTextures(2, @texture(0))
		end 1
	end if
	glClearColor 0.0, 0.0, 0.0, 0.0          '' Clear The Background Color To Black
	glClearDepth 1.0                         '' Enables Clearing Of The Depth Buffer
	glDepthFunc GL_LEQUAL                    '' The Type Of Depth Test To Do
	glBlendFunc GL_SRC_ALPHA, GL_ONE         '' Select The Type Of Blending
	glShadeModel GL_SMOOTH                   '' Enables Smooth Color Shading
	glEnable GL_TEXTURE_2D                   '' Enable 2D Texture Mapping

	do
		glClear GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT     '' Clear The Screen And The Depth Buffer
		glLoadIdentity                                         '' Reset The Modelview Matrix
		glBindTexture GL_TEXTURE_2D, texture(1)                '' Select Our Second Texture
		glTranslatef 0.0, 0.0, -5.0                            '' Move Into The Screen 5 Units
		glRotatef 45.0, 0.0, 0.0, 1.0                          '' Rotate On The Z Axis 45 Degrees (Clockwise)
		glRotatef cnt1*30.0, 1.0, 1.0, 0.0                     '' Rotate On The X & Y Axis By cnt1 (Left To Right)
		glDisable GL_BLEND                                     '' Disable Blending Before We Draw In 3D
		glColor3f 1.0, 1.0, 1.0                                  '' Bright White
		glBegin GL_QUADS                                       '' Draw Our First Texture Mapped Quad
			glTexCoord2d 0.0, 0.0                              '' First Texture Coord
			glVertex2f -1.0, 1.0                               '' First Vertex
			glTexCoord2d 1.0, 0.0                              '' Second Texture Coord
			glVertex2f  1.0, 1.0                               '' Second Vertex
			glTexCoord2d 1.0, 1.0                              '' Third Texture Coord
			glVertex2f  1.0, -1.0                              '' Third Vertex
			glTexCoord2d 0.0, 1.0                              '' Fourth Texture Coord
			glVertex2f -1.0, -1.0                              '' Fourth Vertex
		glEnd                                                  '' Done Drawing The First Quad
		glRotatef 90.0, 1.0, 1.0, 0.0                          '' Rotate On The X & Y Axis By 90 Degrees (Left To Right)
		glBegin GL_QUADS                                       '' Draw Our Second Texture Mapped Quad
			glTexCoord2d 0.0, 0.0                              '' First Texture Coord
			glVertex2f -1.0, 1.0                               '' First Vertex
			glTexCoord2d 1.0, 0.0                              '' Second Texture Coord
			glVertex2f  1.0, 1.0                               '' Second Vertex
			glTexCoord2d 1.0, 1.0                              '' Third Texture Coord
			glVertex2f  1.0, -1.0                              '' Third Vertex
			glTexCoord2d 0.0, 1.0                              '' Fourth Texture Coord
			glVertex2f -1.0, -1.0                              '' Fourth Vertex
		glEnd                                                  '' Done Drawing Our Second Quad
		glEnable GL_BLEND                                      '' Enable Blending

		glLoadIdentity                                         '' Reset The View
		'' Pulsing Colors Based On Text Position
		glColor3f 1.0*(cos(cnt1)), 1.0*(sin(cnt2)), 1.0-0.5*(cos(cnt1+cnt2))
		glPrint 280+250*cos(cnt1), 235+200*sin(cnt2), "NeHe", 0    '' Print GL Text To The Screen

		glColor3f 1.0*(sin(cnt2)), 1.0-0.5*(cos(cnt1+cnt2)), 1.0*(cos(cnt1))
		glPrint 280+230*cos(cnt2), 235+200*sin(cnt1), "OpenGL", 1  '' Print GL Text To The Screen

		glColor3f 0.0, 0.0, 1.0                                  '' Set Color To Blue
		glPrint 240+200*cos((cnt2+cnt1)/5.0), 2, "Giuseppe D'Agata", 0

		glColor3f 1.0, 1.0, 1.0                                  '' Set Color To White
		glPrint 242+200*cos((cnt2+cnt1)/5.0), 2, "Giuseppe D'Agata", 0

		cnt1 = cnt1 + 0.01                                     '' Increase The First Counter
		cnt2 = cnt2 + 0.0081                                   '' Increase The Second Counter

		flip                                      '' filp or crash
		if INKEY = CHR(255)+"k" then exit do    '' exit if close box is clicked
	loop while MULTIKEY(FB.SC_ESCAPE) = 0            '' exit if EXC is pressed
	'' Empty keyboard buffer
	while INKEY <> "": wend
	glDeleteLists gbase, 256                      '' Delete All 256 Display Lists
	glDeleteTextures(2, @texture(0))
	end

'------------------------------------------------------------------------
sub BuildFont()                                  '' Build Our Font Display List

	dim cx as single                             '' Holds Our X Character Coord
	dim cy as single                             '' Holds Our Y Character Coord
	dim gloop as integer

	gbase = glGenLists(256)                      '' Creating 256 Display Lists
	glBindTexture GL_TEXTURE_2D, texture(0)      '' Select Our Font Texture
	for gloop = 0 to 255                         '' Loop Through All 256 Lists

		cx = (gloop mod 16)/16.0                 '' X Position Of Current Character
		'' Each row contains 16 glyphs, so this deliberately uses integer division.
		'' FB-LINTER: DISABLE-NEXT-LINE FBL405 FBL-NUM-017
		cy = (gloop\16)/16.0                     '' Y Position Of Current Character

		glNewList gbase+gloop, GL_COMPILE        '' Start Building A List
		glBegin GL_QUADS                         '' Use A Quad For Each Character
			glTexCoord2f cx, 1-cy-0.0625         '' Texture Coord (Bottom Left)
			glVertex2i 0, 0                      '' Vertex Coord (Bottom Left)
			glTexCoord2f cx+0.0625, 1-cy-0.0625  '' Texture Coord (Bottom Right)
			glVertex2i 16, 0                      '' Vertex Coord (Bottom Right)
			glTexCoord2f cx+0.0625, 1-cy         '' Texture Coord (Top Right)
			glVertex2i 16, 16                    '' Vertex Coord (Top Right)
			glTexCoord2f cx, 1-cy                 '' Texture Coord (Top Left)
			glVertex2i 0, 16                     '' Vertex Coord (Top Left)
		glEnd                                    '' Done Building Our Quad (Character)
		glTranslated 10, 0, 0                    '' Move To The Right Of The Character
		glEndList                                '' Done Building The Display List
	next                                         '' Loop Until All 256 Are Built
end sub

'------------------------------------------------------------------------
'' Where The Printing Happens
sub glPrint(byval x as integer, byval y as integer, byref glstring as string, byval gset as integer)

	if gset>1 then gset=1

	glBindTexture GL_TEXTURE_2D, texture(0)                         '' Select Our Font Texture
	glDisable GL_DEPTH_TEST                                         '' Disables Depth Testing
	glMatrixMode GL_PROJECTION                                      '' Select The Projection Matrix
	glPushMatrix                                                    '' Store The Projection Matrix
		glLoadIdentity                                              '' Reset The Projection Matrix
		glOrtho 0, 640, 0, 480, -1, 1                                '' Set Up An Ortho Screen
		glMatrixMode GL_MODELVIEW                                   '' Select The Modelview Matrix
		glPushMatrix                                                '' Store The Modelview Matrix
			glLoadIdentity                                          '' Reset The Modelview Matrix
			glTranslated x, y, 0                                    '' Position The Text (0,0 - Bottom Left)
			glListBase gbase-32+(128*gset)                          '' Choose The Font Set (0 or 1)
			glCallLists len(glstring), GL_BYTE, strptr(glstring)     '' Write The Text To The Screen
			glMatrixMode GL_PROJECTION                              '' Select The Projection Matrix
		glPopMatrix                                                 '' Restore The Old Projection Matrix
		glMatrixMode GL_MODELVIEW                                   '' Select The Modelview Matrix
	glPopMatrix                                                     '' Restore The Old Projection Matrix
	glEnable GL_DEPTH_TEST                                          '' Enables Depth Testing
end sub

'' End of lesson17.bas
