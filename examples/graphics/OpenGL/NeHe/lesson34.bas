''
'' Project: FreeBASIC OpenGL examples
'' File: lesson34.bas
''
'' Purpose:
''     Render a fixed-size raw terrain height map as OpenGL quads or lines.
''
'' Ownership:
''     The module owns the fixed height-map array for its one render loop.
''     LoadRawFile owns its file unit only after a successful binary open.
''
'' Raw file format:
''     Terrain.raw contains exactly MAP_SIZE by MAP_SIZE unsigned height bytes
''     without a header, stride, palette, or compression.
''
'' This file intentionally does NOT contain:
''     - a general terrain file loader
''     - dynamically sized terrain storage
''
''  This Code Was Created By Ben Humphrey 2001
''  If You've Found This Code Useful, Please Let Me Know.
''  Visit NeHe Productions At http://nehe.gamedev.net
'' Setup our booleans
const null = 0


#include once "GL/gl.bi"
#include once "GL/glu.bi"
#include once "fbgfx.bi"                       '' for Scan code constants


const MAP_SIZE = 512                           '' Size Of Our .RAW Height Map (NEW)
const STEP_SIZE = 32                           '' Width And Height Of Each Quad (NEW)
const HEIGHT_RATIO = 1.5f                      '' Ratio That The Y Is Scaled According To The X And Z (NEW)


declare sub RenderHeightMap (byval pHeightMap as ubyte ptr)
declare sub SetVertexColor (byval pHeightMap as ubyte ptr, byval x as integer, byval y as integer)
declare function Height (byval pHeightMap as ubyte ptr, byval X as integer, byval Y as integer) as integer
declare function LoadRawFile (byref strName as string, byval nSize as integer, HeightMap() as ubyte) as integer


'' The render loop and terrain helpers share only this module's height-map state.
'' FB-LINTER: DISABLE-NEXT-LINE FBL301
dim shared as integer bRender = TRUE, rdown = FALSE         '' Polygon Flag Set To TRUE By Default (NEW)
dim shared as ubyte g_HeightMap(0 to MAP_SIZE * MAP_SIZE - 1)  '' Holds The Height Map Data (NEW)
dim shared as single scaleValue = 0.30f                     '' Scale Value For The Terrain (NEW)

	windowtitle "NeHe & Ben Humphrey's Height Map Tutorial" '' Set window title
	screen 18, 32, , 2

	'' Initialize The GL Window
	glViewport 0, 0, 640, 480                  '' Reset The Current Viewport
	glMatrixMode GL_PROJECTION                 '' Select The Projection Matrix
	glLoadIdentity                             '' Reset The Projection Matrix
	'' Calculate The Aspect Ratio Of The Window.  Farthest Distance Changed To 500.0f (NEW)
	gluPerspective 45.0, 640/480, 0.1, 500.0
	glMatrixMode GL_MODELVIEW                  '' Select The Modelview Matrix
	glLoadIdentity                             '' Reset The Projection Matrix

	'' All Setup For OpenGL Goes Here
	glShadeModel (GL_SMOOTH)                                '' Enable Smooth Shading
	glClearColor (0.0f, 0.0f, 0.0f, 0.5f)                '' Black Background
	glClearDepth (1.0f)                                     '' Depth Buffer Setup
	glEnable (GL_DEPTH_TEST)                                '' Enables Depth Testing
	glDepthFunc (GL_LEQUAL)                                 '' The Type Of Depth Testing To Do
	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST)     '' Really Nice Perspective Calculations
	if LoadRawFile (exepath + "/data/Terrain.raw", MAP_SIZE * MAP_SIZE, g_HeightMap()) = false then end 1

	do

		glClear (GL_COLOR_BUFFER_BIT  or  GL_DEPTH_BUFFER_BIT)        '' Clear The Screen And The Depth Buffer
		glLoadIdentity ()                                             '' Reset The Matrix
		''         Position         View             Up Vector
		gluLookAt (212, 60, 194, 186, 55, 171, 0, 1, 0)       '' This Determines Where The Camera's Position And View Is
		glScalef (scaleValue, scaleValue * HEIGHT_RATIO, scaleValue)
		RenderHeightMap (@g_HeightMap(0))                             '' Render The Height Map

		'' Is the UP ARROW key Being Pressed?
		if MULTIKEY(FB.SC_UP) then                          '' Increase the scale value to zoom in
			scaleValue + = 0.001f
		end if
		'' Is the DOWN ARROW key Being Pressed?
		if MULTIKEY(FB.SC_DOWN) then                        '' Decrease the scale value to zoom out
			scaleValue - = 0.001f
		end if
		'' Is the R key Being Pressed?
		if MULTIKEY(FB.SC_R) and not rdown then             '' toggle Render Polygons/lines
			bRender = not bRender
			rdown = true
		end if
	if MULTIKEY(FB.SC_R) = 0 then rdown = false

		flip
		if inkey = chr(255)+"k" then exit do           '' exit if close box is clicked
	loop while MULTIKEY(FB.SC_ESCAPE) = 0

	'' Empty keyboard buffer
	while inkey <> "": wend

	end


'----------------------------------------------------------
'' Loads the documented headerless RAW file into one exact caller-owned array.
function LoadRawFile (byref strName as string, byval nSize as integer, HeightMap() as ubyte) as integer
	dim pFile as integer

	if nSize <= 0 then exit function
	if ubound(HeightMap) < lbound(HeightMap) then exit function
	if ubound(HeightMap) - lbound(HeightMap) + 1 <> nSize then exit function

	'' Open The File In Read / Binary Mode.
	pFile = freefile
	open strName for binary access read as #pFile
	if err <> 0 then exit function

	'' The routine header documents this exact raw transfer and its byte count.
	'' FB-LINTER: DISABLE-NEXT-LINE FBL-DOC-BIN-003
	get #pFile,, HeightMap()
	if err <> 0 then
		close #pFile
		exit function
	end if

	'' Close The File.
	close #pFile
	LoadRawFile = true
end function

'----------------------------------------------------------
function Height (byval pHeightMap as ubyte ptr, byval XX as integer, byval YY as integer) as integer        '' This Returns The Height From A Height Map Index
	dim x as integer                                    '' Error Check Our x Value
	x = XX mod MAP_SIZE
	dim y as integer                                    '' Error Check Our y Value
	y = YY mod MAP_SIZE

	if pHeightMap = 0 then                              '' Make Sure Our Data Is Valid
		return 0
	end if
	return pHeightMap[x + (y * MAP_SIZE)]               '' Index Into Our Height Array And Return The Height
end function

'----------------------------------------------------------
'' Sets The Color Value For A Particular Index, Depending On The Height Index
sub SetVertexColor (byval pHeightMap as ubyte ptr, byval x as integer, byval y as integer)
	if pHeightMap = 0 then                             '' Make Sure Our Height Data Is Valid
		exit sub
	end if
	dim fColor as single

	fColor = - 0.15f + (Height (pHeightMap, x, y) / 256.0f)

	'' Assign This Blue Shade To The Current Vertex
	glColor3f (0, 0, fColor)
end sub

'----------------------------------------------------------
'' This Renders The Height Map As Quads
sub RenderHeightMap (byval pHeightMap as ubyte ptr)
	dim as integer XX, YY                              '' Create Some Variables To Walk The Array With.
	dim as integer x, y, z                             '' Create Some Variables For Readability

	if pHeightMap = 0 then                             '' Make Sure Our Height Data Is Valid
		exit sub
	end if
	'' What We Want To Render
	if bRender <> 0 then                               '' Render Polygons
		glBegin (GL_QUADS)
	else                                               '' Render Lines Instead
		glBegin (GL_LINES)
	end if

	for XX = 0 to MAP_SIZE-1 step STEP_SIZE
		for YY = 0 to MAP_SIZE-1 step STEP_SIZE
			'' Get The (X, Y, Z) Value For The Bottom Left Vertex
			x = XX
			y = Height (pHeightMap, XX, YY)
			z = YY

			'' Set The Color Value Of The Current Vertex
			SetVertexColor (pHeightMap, x, z)

			glVertex3i (x, y, z)                     '' Send This Vertex To OpenGL To Be Rendered (Integer Points Are Faster)

			'' Get The (X, Y, Z) Value For The Top Left Vertex
			x = XX
			y = Height (pHeightMap, XX, YY + STEP_SIZE)
			z = YY + STEP_SIZE
			'' Set The Color Value Of The Current Vertex
			SetVertexColor (pHeightMap, x, z)

			glVertex3i (x, y, z)                     '' Send This Vertex To OpenGL To Be Rendered

			'' Get The (X, Y, Z) Value For The Top Right Vertex
			x = XX + STEP_SIZE
			y = Height (pHeightMap, XX + STEP_SIZE, YY + STEP_SIZE)
			z = YY + STEP_SIZE

			'' Set The Color Value Of The Current Vertex
			SetVertexColor (pHeightMap, x, z)
			glVertex3i (x, y, z)                     '' Send This Vertex To OpenGL To Be Rendered

			'' Get The (X, Y, Z) Value For The Bottom Right Vertex
			x = XX + STEP_SIZE
			y = Height (pHeightMap, XX + STEP_SIZE, YY)
			z = YY

			'' Set The Color Value Of The Current Vertex
			SetVertexColor (pHeightMap, x, z)

			glVertex3i (x, y, z)                     '' Send This Vertex To OpenGL To Be Rendered
		next
	next
	glEnd ()
	glColor4f (1.0f, 1.0f, 1.0f, 1.0f)              '' Reset The Color
end sub

'' End of lesson34.bas
