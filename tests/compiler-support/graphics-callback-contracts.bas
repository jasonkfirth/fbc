'' Project: FreeBASIC C backend regression tests
'' File: compiler-support/graphics-callback-contracts.bas
'' Purpose: Check graphics putter and blender calling contracts without a window.
'' Responsibilities: Exercise PUT modes, a stored callback and caller-owned data.
'' This file intentionally does NOT contain: window creation or timing checks.

#lang "fb"
#include once "fbgfx.bi"

type CallbackData
	calls as integer
	mask as ulong
	parameter_reads as integer
end type

private sub require( byval condition as boolean, byval description as const zstring ptr )
	if( condition = FALSE ) then
		print "GRAPHICS_CALLBACK_FAILURE: "; *description
		end 1
	end if
end sub

private function blend _
	( byval source_pixel as ulong, byval destination_pixel as ulong, _
	  byval parameter as any ptr ) as ulong

	var context = cptr(CallbackData ptr, parameter)
	if( context = 0 ) then return 0
	context->calls += 1
	return source_pixel xor destination_pixel xor context->mask
end function

private function typed_blend cdecl _
	( byval source_pixel as ulong, byval destination_pixel as ulong, _
	  byval context as CallbackData ptr ) as ulong

	if( context = 0 ) then return 0
	context->calls += 1
	return source_pixel xor destination_pixel xor context->mask
end function

private function get_parameter( byval context as CallbackData ptr ) as CallbackData ptr
	context->parameter_reads += 1
	return context
end function

const SOURCE_COLOUR = &hFF112233ul
const DESTINATION_COLOUR = &hFF445566ul
require( screenres(8, 8, 32, 1, fb.GFX_NULL) = 0, @"null graphics driver" )
var source_image = imagecreate( 2, 2, SOURCE_COLOUR, 32 )
var destination_image = imagecreate( 4, 4, DESTINATION_COLOUR, 32 )
if( source_image = 0 orelse destination_image = 0 ) then
	if( source_image <> 0 ) then imagedestroy( source_image )
	if( destination_image <> 0 ) then imagedestroy( destination_image )
	screen 0
	print "GRAPHICS_CALLBACK_ALLOCATION_FAILURE"
	end 1
end if

put destination_image, (1, 1), source_image, pset
require( culng(point(1, 1, destination_image)) = SOURCE_COLOUR, @"PSET pixels" )
print "PSET "; hex(culng(point(1, 1, destination_image)), 8)

put destination_image, (1, 1), source_image, preset
print "PRESET "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, trans
print "TRANS "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, and
print "AND "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, or
print "OR "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, xor
print "XOR "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, alpha
print "ALPHA "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, alpha, 128
print "BLEND "; hex(culng(point(1, 1, destination_image)), 8)
put destination_image, (1, 1), source_image, add, 128
print "ADD "; hex(culng(point(1, 1, destination_image)), 8)

line destination_image, (0, 0)-(3, 3), DESTINATION_COLOUR, bf
dim as CallbackData context = ( 0, &hFul, 0 )
dim as function( byval as ulong, byval as ulong, byval as any ptr ) as ulong callback = @blend
put destination_image, (1, 1), source_image, custom, callback, @context
require( context.calls = 4, @"one callback per source pixel" )
require( culng(point(1, 1, destination_image)) = _
	(SOURCE_COLOUR xor DESTINATION_COLOUR xor context.mask), @"CUSTOM pixels and user data" )
print "CUSTOM "; hex(culng(point(1, 1, destination_image)), 8)

line destination_image, (0, 0)-(3, 3), DESTINATION_COLOUR, bf
context.calls = 0
put destination_image, (1, 1), source_image, custom, @typed_blend, get_parameter(@context)
require( context.calls = 4, @"typed user-data callback" )
require( context.parameter_reads = 1, @"user-data expression evaluated once" )
require( culng(point(1, 1, destination_image)) = _
	(SOURCE_COLOUR xor DESTINATION_COLOUR xor context.mask), @"typed callback pixels" )
print "TYPED CUSTOM "; hex(culng(point(1, 1, destination_image)), 8)

'' Both text entry points use the same putter contract and a C int result.
draw string destination_image, (0, 0), "A", SOURCE_COLOUR
draw string destination_image, (0, 0), wstr("B"), SOURCE_COLOUR

imagedestroy( source_image )
imagedestroy( destination_image )
screen 0
print "GRAPHICS_CALLBACK_CONTRACTS_PASS"

'' end of compiler-support/graphics-callback-contracts.bas
