''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: cygwin/fbc-platform.bi
''
'' Purpose:
''
''     Keep Cygwin target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add Cygwin gfx and sound library dependencies
''     - add Cygwin default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Cygwin ABI or code generation rules
''

#ifndef __FBC_CYGWIN_PLATFORM_BI__
#define __FBC_CYGWIN_PLATFORM_BI__

private function fbcCygwinPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_CYGWIN)
end function

private sub fbcCygwinPlatformAddDefaultLibPaths( )
	if( fbcCygwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcCygwinPlatformAddGfxLibs( )
	if( fbcCygwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gdi32" )
	fbcAddDefLib( "winmm" )
end sub

private sub fbcCygwinPlatformAddSfxLibs( )
	if( fbcCygwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "winmm" )
	fbcAddDefLib( "ole32" )
	fbcAddDefLib( "uuid" )
end sub

private sub fbcCygwinPlatformAddDefaultLibs( )
	if( fbcCygwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "cygwin" )
	fbcAddDefLib( "kernel32" )
	fbcAddDefLib( "user32" )

	'' profiling?
	if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
		fbcAddDefLib( "gmon" )
	end if
end sub

private sub fbcCygwinPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcCygwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of cygwin/fbc-platform.bi
