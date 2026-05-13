''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-dos-platform.bi
''
'' Purpose:
''
''     Keep DOS target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add DJGPP library search paths
''     - add DOS default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - DOS ABI or code generation rules
''

#ifndef __FBC_DOS_PLATFORM_BI__
#define __FBC_DOS_PLATFORM_BI__

private function fbcDosPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS)
end function

private sub fbcDosPlatformAddDefaultLibPaths( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	'' Help out the DJGPP linker to find DJGPP's lib/ dir.
	'' It doesn't seem to add it by default like on other systems.
	'' Note: Can't use libc here, we have a fixed copy of that in
	'' the compiler's lib/ dir.
	fbcAddLibPathFor( "libm.a" )
#endif
end sub

private sub fbcDosPlatformAddGfxLibs( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcDosPlatformAddSfxLibs( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcDosPlatformAddDefaultLibs( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "m" )
end sub

private sub fbcDosPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-dos-platform.bi
