' TEST_MODE : COMPILE_AND_RUN_OK

'' On 32-bit x86, cdecl arguments must be removed by the caller. Keep the
'' deallocation in a frameless procedure so a missing cleanup cannot be hidden
'' by restoring ESP from EBP in the procedure epilogue.

dim shared as any ptr buffer

private sub resetBuffer( )
	buffer = 0
end sub

private sub releaseBuffer( )
	if( buffer <> 0 ) then
		deallocate( buffer )
	end if

	resetBuffer( )
end sub

buffer = allocate( 32 )
assert( buffer <> 0 )

releaseBuffer( )
assert( buffer = 0 )
