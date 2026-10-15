'' TEST_MODE : COMPILE_ONLY
'' Project: FreeBASIC compiler graphics argument tests
'' File: gfx-array-descriptor-load.bas
'' Purpose: Exercise GET through array parameters with nonzero lower bounds.
'' Responsibilities: Check static, dynamic and forwarded array addresses.
'' This file intentionally does NOT create a display or validate image pixels.
'' Native qualification links the companion C observer; ordinary tests compile only.

extern "C"
	declare function compiler_test_gfx_destination( ) as any ptr
	declare function compiler_test_gfx_lower_bound( ) as longint
	declare function compiler_test_gfx_calls( ) as ulong
	declare function compiler_test_gfx_valid( ) as long
end extern

private sub CaptureByDescriptor( pixels( ) as ulong )
	get (0, 0)-(0, 0), pixels
end sub

private sub ForwardByDescriptor( pixels( ) as ulong )
	CaptureByDescriptor( pixels( ) )
end sub

const static_lower = -3
const dynamic_lower = 7
dim fixed_pixels(static_lower to static_lower + 4) as ulong
redim dynamic_pixels(dynamic_lower to dynamic_lower + 4) as ulong

CaptureByDescriptor( fixed_pixels( ) )
if( compiler_test_gfx_destination( ) <> @fixed_pixels(static_lower) ) then end 1
if( compiler_test_gfx_lower_bound( ) <> static_lower ) then end 1
if( compiler_test_gfx_valid( ) = 0 ) then end 1
if( compiler_test_gfx_calls( ) <> 1 ) then end 1

CaptureByDescriptor( dynamic_pixels( ) )
if( compiler_test_gfx_destination( ) <> @dynamic_pixels(dynamic_lower) ) then end 1
if( compiler_test_gfx_lower_bound( ) <> dynamic_lower ) then end 1
if( compiler_test_gfx_valid( ) = 0 ) then end 1
if( compiler_test_gfx_calls( ) <> 2 ) then end 1

ForwardByDescriptor( fixed_pixels( ) )
if( compiler_test_gfx_destination( ) <> @fixed_pixels(static_lower) ) then end 1
if( compiler_test_gfx_lower_bound( ) <> static_lower ) then end 1
if( compiler_test_gfx_valid( ) = 0 ) then end 1
if( compiler_test_gfx_calls( ) <> 3 ) then end 1

'' end of gfx-array-descriptor-load.bas
