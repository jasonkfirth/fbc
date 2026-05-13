''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-dragonfly-platform.bi
''
'' Purpose:
''
''     Keep DragonFly target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add DragonFly library search paths
''     - add DragonFly gfx library dependencies
''     - add DragonFly default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - DragonFly ABI or code generation rules
''

#ifndef __FBC_DRAGONFLY_PLATFORM_BI__
#define __FBC_DRAGONFLY_PLATFORM_BI__

private function fbcDragonflyPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DRAGONFLY)
end function

private sub fbcDragonflyPlatformAddDefaultLibPaths( )
	if( fbcDragonflyPlatformIsSelected( ) = FALSE ) then
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

private sub fbcDragonflyPlatformAddGfxLibs( )
	if( fbcDragonflyPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcDragonflyPlatformAddSfxLibs( )
	if( fbcDragonflyPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcDragonflyPlatformAddDefaultLibs( )
	if( fbcDragonflyPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLibPath( "/usr/local/lib/" )
	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "m" )
	fbcAddDefLib( "ncurses" )
end sub

private sub fbcDragonflyPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcDragonflyPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-dragonfly-platform.bi
