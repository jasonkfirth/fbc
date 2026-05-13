''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-freebsd-platform.bi
''
'' Purpose:
''
''     Keep FreeBSD target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add FreeBSD gfx library dependencies
''     - add FreeBSD default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - FreeBSD ABI or code generation rules
''

#ifndef __FBC_FREEBSD_PLATFORM_BI__
#define __FBC_FREEBSD_PLATFORM_BI__

private function fbcFreebsdPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_FREEBSD)
end function

private sub fbcFreebsdPlatformAddDefaultLibPaths( )
	if( fbcFreebsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcFreebsdPlatformAddGfxLibs( )
	if( fbcFreebsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcFreebsdPlatformAddSfxLibs( )
	if( fbcFreebsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcFreebsdPlatformAddDefaultLibs( )
	if( fbcFreebsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "m" )
	fbcAddDefLib( "ncurses" )
end sub

private sub fbcFreebsdPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcFreebsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-freebsd-platform.bi
