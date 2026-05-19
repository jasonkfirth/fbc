''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-openbsd-platform.bi
''
'' Purpose:
''
''     Keep OpenBSD target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add OpenBSD library search paths
''     - add OpenBSD gfx library dependencies
''     - add OpenBSD default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - OpenBSD ABI or code generation rules
''

#ifndef __FBC_OPENBSD_PLATFORM_BI__
#define __FBC_OPENBSD_PLATFORM_BI__

private function fbcOpenbsdPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_OPENBSD)
end function

private sub fbcOpenbsdPlatformAddDefaultLibPaths( )
	if( fbcOpenbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	fbcAddDefLibPath( "/usr/local/lib/" )
	fbcAddLibPathFor( "libX11.a" )
	fbcAddLibPathFor( "libm.a" )
#endif
end sub

private sub fbcOpenbsdPlatformAddGfxLibs( )
	if( fbcOpenbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
	fbcAddDefLib( "usbhid" )
end sub

private sub fbcOpenbsdPlatformAddSfxLibs( )
	if( fbcOpenbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "sndio" )
end sub

private sub fbcOpenbsdPlatformAddDefaultLibs( )
	if( fbcOpenbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "m" )
	fbcAddDefLib( "ncurses" )
end sub

private sub fbcOpenbsdPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcOpenbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-openbsd-platform.bi
