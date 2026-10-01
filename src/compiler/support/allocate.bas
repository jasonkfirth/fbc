'' Project: FreeBASIC compiler support
'' -----------------------------------------
''
'' File: support/allocate.bas
''
'' Purpose:
''
''     Check compiler storage sizes and stop safely when allocation fails.
''
'' Responsibilities:
''
''     - check additions, products, alignment, and block growth
''     - wrap runtime allocation with one failure policy
''
'' This file intentionally does NOT contain:
''
''     - container links, parser state, or payload cleanup
''
'' Resource ownership:
''     Successful allocation results belong to the caller. The caller releases
''     them with deallocate(); these helpers retain no allocated blocks.
''

#include once "support/common.bi"

private sub fatalOutOfMemory()
	'' Size overflow and exhausted storage have the same compiler failure
	'' policy. Continuing after either would expose an undersized buffer.
	error(4)
end sub

'' -------------------------------------------------------------------------
'' Checked storage arithmetic
'' -------------------------------------------------------------------------

function xcheckedAdd(byval a as integer, byval b as integer) as integer
	if( (a < 0) or (b < 0) ) then
		fatalOutOfMemory()
	end if
	if( a > XALLOC_MAX_SIZE - b ) then
		fatalOutOfMemory()
	end if
	return a + b
end function

function xcheckedMultiply(byval count as integer, byval size as integer) as integer
	if( (count < 0) or (size < 0) ) then
		fatalOutOfMemory()
	end if
	if( size <> 0 ) then
		if( count > XALLOC_MAX_SIZE \ size ) then
			fatalOutOfMemory()
		end if
	end if
	return count * size
end function

function xcheckedAlign(byval size as integer, byval alignment as integer) as integer
	if( alignment <= 0 ) then
		fatalOutOfMemory()
	end if
	if( (alignment and (alignment - 1)) <> 0 ) then
		fatalOutOfMemory()
	end if
	return xcheckedAdd(size, alignment - 1) and (not (alignment - 1))
end function

function xgrowthCount(byval count as integer, byval divisor as integer) as integer
	if( (count < 0) or (divisor <= 0) ) then
		fatalOutOfMemory()
	end if
	var growth = count \ divisor
	'' Fractional growth rounds to zero for small initial pools. At least one
	'' node is needed before the caller can link or return a new payload.
	if( growth = 0 ) then
		growth = 1
	end if
	return growth
end function

'' -------------------------------------------------------------------------
'' Runtime allocation
'' -------------------------------------------------------------------------

function xallocate(byval size as integer) as any ptr
	if( size <= 0 ) then
		fatalOutOfMemory()
	end if
	dim as any ptr p = allocate(size)
	if( p = NULL ) then
		fatalOutOfMemory()
	end if
	return p
end function

function xcallocate(byval size as integer) as any ptr
	if( size <= 0 ) then
		fatalOutOfMemory()
	end if
	dim as any ptr p = callocate(size)
	if( p = NULL ) then
		fatalOutOfMemory()
	end if
	return p
end function

function xreallocate(byval old as any ptr, byval size as integer) as any ptr
	'' Zero-byte realloc may free the old allocation. Compiler buffers release
	'' explicitly, so reject it before calling the external allocator.
	if( size <= 0 ) then
		fatalOutOfMemory()
	end if
	dim as any ptr p = reallocate(old, size)
	if( p = NULL ) then
		fatalOutOfMemory()
	end if
	return p
end function

'' end of support/allocate.bas
