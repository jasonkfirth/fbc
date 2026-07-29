''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: gl-interop-smoke.bas
''
'' Purpose:
''
''     Prove that gfxlib3 exposes live OpenGL entry points only while an
''     ordered callback owns its renderer thread.
''
'' Responsibilities:
''
''     - reject SCREENGLPROC from the BASIC application thread
''     - resolve and call glGetString from a render-thread callback
''     - verify that the callback returns only after its GL work is complete
''
'' This file intentionally does NOT contain:
''
''     - raw OpenGL drawing or persistent OpenGL object ownership
''     - calls to normal graphics APIs from inside the callback
''     - a dependency on a driver-specific GL version string
''

#include once "fbgfx3.bi"

const GL_VERSION = &h1F02
const GL_VENDOR = &h1F00
const GL_RENDERER = &h1F01

dim shared as integer callback_called
dim shared as integer callback_found_proc
dim shared as integer callback_found_version
dim shared as string callback_vendor
dim shared as string callback_renderer

sub gl_callback cdecl( byval user_data as any ptr )
	dim get_string as function stdcall( byval name as ulong ) as const ubyte ptr
	dim version as const ubyte ptr
	dim vendor as const ubyte ptr
	dim renderer as const ubyte ptr

	callback_called += 1
	get_string = cptr( function stdcall( byval name as ulong ) as const ubyte ptr, _
		screenglproc( "glGetString" ) )
	if get_string = 0 then exit sub
	callback_found_proc = true
	version = get_string( GL_VERSION )
	if version <> 0 then callback_found_version = true
	vendor = get_string( GL_VENDOR )
	renderer = get_string( GL_RENDERER )
	if vendor <> 0 then callback_vendor = *cptr( zstring ptr, vendor )
	if renderer <> 0 then callback_renderer = *cptr( zstring ptr, renderer )
end sub

if screenres( 96, 80, 32, 1, fb.GFX_OPENGL ) <> 0 then
	print "GFX3_GL_INTEROP_FAIL screenres"
	end 1
end if

if screenglproc( "glGetString" ) <> 0 then
	print "GFX3_GL_INTEROP_FAIL public-proc"
	screen 0
	end 2
end if
if fb.Gfx3RunOnRenderThread( cptr( fb.Gfx3RenderCallback ptr, _
	@gl_callback ) ) <> 0 then
	print "GFX3_GL_INTEROP_FAIL callback"
	screen 0
	end 3
end if
screen 0

if callback_called <> 1 orelse callback_found_proc = false orelse _
	callback_found_version = false then
	print "GFX3_GL_INTEROP_FAIL result " & callback_called & "," & _
		callback_found_proc & "," & callback_found_version
	end 4
end if
print "GFX3_GL_ADAPTER vendor=" & callback_vendor & _
	" renderer=" & callback_renderer

'' Null and Vulkan intentionally do not expose an OpenGL callback boundary.
callback_called = 0
if screenres( 32, 24, 32, 1, fb.GFX_NULL ) <> 0 then
	print "GFX3_GL_INTEROP_FAIL null-screenres"
	end 5
end if
if fb.Gfx3RunOnRenderThread( cptr( fb.Gfx3RenderCallback ptr, _
	@gl_callback ) ) = 0 then
	print "GFX3_GL_INTEROP_FAIL null-callback"
	screen 0
	end 6
end if
screen 0
if callback_called <> 0 then
	print "GFX3_GL_INTEROP_FAIL null-called"
	end 7
end if

print "GFX3_GL_INTEROP_PASS"
end 0

'' end of gl-interop-smoke.bas
