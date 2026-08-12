''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: nuttx/fbc-platform.bi
''
'' Purpose:
''
''     Keep NuttX target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - provide the platform hook entry points required by fbc-platform.bi
''     - keep NuttX-specific linker behavior in one target-owned file
''     - leave shared compiler driver behavior in fbc.bas
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - NuttX ABI or code generation rules
''
'' NuttX target hooks
''
''     The NuttX port is still small and does not currently need extra
''     default system libraries, graphics libraries, sound libraries, or
''     linker frameworks from the compiler driver.  These hooks are kept
''     explicit so FB_COMPTARGET_NUTTX has a complete platform table entry.
''

#ifndef __FBC_NUTTX_PLATFORM_BI__
#define __FBC_NUTTX_PLATFORM_BI__

private function fbcNuttxPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_NUTTX)
end function

private sub fbcNuttxPlatformAddDefaultLibPaths( )
	if( fbcNuttxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcNuttxPlatformAddGfxLibs( )
	if( fbcNuttxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcNuttxPlatformAddSfxLibs( )
	if( fbcNuttxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcNuttxPlatformAddDefaultLibs( )
	if( fbcNuttxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcNuttxPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcNuttxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of nuttx/fbc-platform.bi
