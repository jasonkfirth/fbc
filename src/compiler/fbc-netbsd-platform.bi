''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-netbsd-platform.bi
''
'' Purpose:
''
''     Keep NetBSD target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add NetBSD library search paths
''     - add NetBSD gfx library dependencies
''     - add NetBSD default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - NetBSD ABI or code generation rules
''

#ifndef __FBC_NETBSD_PLATFORM_BI__
#define __FBC_NETBSD_PLATFORM_BI__

private function fbcNetbsdPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_NETBSD)
end function

private sub fbcNetbsdPlatformAddDefaultLibPaths( )
	if( fbcNetbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	fbcAddLibPathFor( "libX11.so" )
	fbcAddLibPathFor( "libXext.so" )
	fbcAddLibPathFor( "libXpm.so" )
	fbcAddLibPathFor( "libXrandr.so" )
	fbcAddLibPathFor( "libXrender.so" )
	fbcAddLibPathFor( "libXrender.so" )
	fbcAddLibPathFor( "libncurses.so" )
#endif
end sub

private sub fbcNetbsdPlatformAddGfxLibs( )
	if( fbcNetbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcNetbsdPlatformAddSfxLibs( )
	if( fbcNetbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "ossaudio" )
end sub

private sub fbcNetbsdPlatformAddDefaultLibs( )
	if( fbcNetbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLibPath( "/usr/pkg/lib/" )
	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "m" )
	fbcAddDefLib( "ncurses" )
end sub

private sub fbcNetbsdPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcNetbsdPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-netbsd-platform.bi
