'' Project: FreeBASIC C error-frame qualification
'' File: rtlib/error-frame-bindings.bas
'' Purpose: Exercise typed runtime callbacks through real BASIC ERROR statements.
'' Responsibilities: Check CDECL callback signatures, owner data, retry/next and
''     legacy restoration without replacing the installed runtime.
'' This file intentionally does NOT stand in for automatic ON ERROR outlining.
#lang "fblite"

type CErrorFrame
	previous as CErrorFrame ptr
	context as any ptr
	cleanup as sub cdecl( byval as any ptr )
	owner as any ptr
end type

type OwnerData
	frame as CErrorFrame
	values(0 to 31) as integer
	calls as long
	retry_first as long
	cleanup_calls as long
	completed as long
end type

private sub owner_cleanup cdecl( byval opaque as any ptr )
	dim as OwnerData ptr owner = cptr( OwnerData ptr, opaque )
	owner->cleanup_calls += 1
end sub

private function owner_handler cdecl( byval opaque as any ptr ) as long
	dim as OwnerData ptr owner = cptr( OwnerData ptr, opaque )
	if err <> 24 then end 41
	if owner->values(3) <> 103 then end 42
	owner->calls += 1
	if owner->retry_first andalso owner->calls = 1 then return 1 '' FB_ERROR_FRAME_RETRY
	return 2 '' FB_ERROR_FRAME_NEXT: continue in the worker's live activation.
end function

private sub worker( byval owner as OwnerData ptr )
	dim worker_values(0 to 31) as integer
	for index as integer = 0 to 31
		worker_values(index) = 300 + index
	next
	error 24
	if worker_values(3) <> 303 then end 43
	owner->completed += 1
end sub

for retry_first as long = 0 to 1
	'' Keep mutable owner data outside the worker's C activation. The runtime
	'' owns only its registration metadata; this BASIC scope owns this block.
	dim as OwnerData ptr owner = callocate( sizeof( OwnerData ) )
	if owner = 0 then end 44
	for index as integer = 0 to 31
		owner->values(index) = 100 + index
	next
	owner->retry_first = retry_first
	if fb_ErrorFrameEnter( @owner->frame, owner, @owner_cleanup ) <> 0 then end 45
	if fb_ErrorFrameBuffer( @owner->frame ) = 0 then end 46
	'' A checkpoint can only be marked on live storage, and an owner target
	'' can only be selected during its callback. Exercise the remaining typed
	'' entry points without pretending that this fixture established setjmp.
	if fb_ErrorFrameReady( 0 ) <> 1 then end 53
	if fb_ErrorFrameSetTarget( @owner->frame, 1 ) <> 1 then end 54
	if fb_ErrorFrameGetTarget( @owner->frame ) <> 0 then end 55
	dim as any ptr previous
	if fb_ErrorFrameSetHandler( @owner->frame, @owner_handler, @previous ) <> 0 then end 47
	if previous <> 0 then end 48
	worker( owner )
	if owner->calls <> 1 + retry_first or owner->completed <> 1 then end 49
	if fb_ErrorFrameLeave( @owner->frame ) <> 0 then end 50
	if owner->cleanup_calls <> 0 then end 51
	if fb_ErrorFrameRestore( previous ) <> 0 then end 52
	deallocate owner
next

'' Observable success marker consumed by the external regression runner.
print "error-frame-bindings-ok"

'' end of rtlib/error-frame-bindings.bas
