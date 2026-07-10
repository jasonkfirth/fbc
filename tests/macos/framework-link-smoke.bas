'' FreeBASIC macOS system framework smoke test
'' -------------------------------------------
''
'' File: framework-link-smoke.bas
''
'' Purpose:
''
''     Verify that bindings for APIs supplied by macOS system frameworks
''     compile, link, and resolve representative framework symbols.
''
'' Responsibilities:
''
''     - request the OpenGL, GLUT, OpenAL, and Cocoa frameworks
''     - retain one symbol reference from each framework
''     - run without creating a window or opening an audio device
''
'' This file intentionally does NOT contain:
''
''     - graphics context creation
''     - window-system event handling
''     - audio device initialization
''

#include once "GL/gl.bi"
#include once "GL/glu.bi"
#include once "GL/glut.bi"
#include once "AL/al.bi"
#include once "AL/alc.bi"

#inclib "Cocoa"

extern "C"
	extern CocoaVersionNumber as double
end extern

''
'' Taking the addresses keeps the smoke test independent of a display server,
'' an OpenGL context, and an audio device while still requiring the linker to
'' resolve a symbol from every requested API.
''
if( procptr( glGetError ) = 0 ) then
	end 1
end if

if( procptr( gluErrorString ) = 0 ) then
	end 2
end if

if( procptr( glutInit ) = 0 ) then
	end 3
end if

if( procptr( alGetError ) = 0 ) then
	end 4
end if

if( procptr( alcGetString ) = 0 ) then
	end 5
end if

if( CocoaVersionNumber <= 0.0 ) then
	end 6
end if

print "framework-link-smoke: system framework symbols resolved"

'' end of framework-link-smoke.bas
