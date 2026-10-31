/'
	Project: FreeBASIC semantic sidecar tests
	File: graphics-call-abi.bas
	Purpose: Expose selected typed graphics callbacks and their native contexts.
	Responsibilities: Adapted and directly compatible callback operands.
	This file intentionally does NOT contain: Runtime graphics initialization.
'/
#lang "fb"

private function TypedBlend cdecl _
	(byval source_pixel as ulong, byval destination_pixel as ulong, _
	 byval context as long ptr) as ulong
	return source_pixel xor destination_pixel xor culng(*context)
end function

private function ErasedBlend _
	(byval source_pixel as ulong, byval destination_pixel as ulong, _
	 byval context as any ptr) as ulong
	return source_pixel xor destination_pixel xor culng(*cptr(long ptr, context))
end function

sub GraphicsCalls(byval image as any ptr, byval context as long ptr)
	put (0, 0), image, custom, @TypedBlend, context
	put (0, 0), image, custom, @ErasedBlend, context
end sub

/' end of graphics-call-abi.bas '/
