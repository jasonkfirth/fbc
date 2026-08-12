''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: haiku/fbc-platform.bi
''
'' Purpose:
''
''     Keep Haiku target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add Haiku development library search paths
''     - add Haiku sound library dependencies
''     - add Haiku default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Haiku ABI or code generation rules
''

#ifndef __FBC_HAIKU_PLATFORM_BI__
#define __FBC_HAIKU_PLATFORM_BI__

private function fbcHaikuPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_HAIKU)
end function

private sub fbcHaikuPlatformAddDefaultLibPaths( )
	if( fbcHaikuPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	''
	'' Haiku keeps some development-time libraries in /boot/system/develop/lib
	'' instead of the runtime library locations that ld searches by default.
	''
	'' Add the directories for a few representative libraries used by the
	'' default Haiku link set so direct ld linking can resolve -lnetwork,
	'' -lncurses and related libraries without relying on user-local symlinks.
	''
	fbcAddLibPathFor( "libnetwork.so" )
	fbcAddLibPathFor( "libncurses.so" )
#endif
end sub

private sub fbcHaikuPlatformAddGfxLibs( )
	if( fbcHaikuPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	''
	'' Haiku exposes BJoystick through libdevice.  The gfx runtime uses
	'' BJoystick for GETJOYSTICK and GETXPAD, so programs that pull in
	'' libfbgfx must link libdevice too.
	''
	fbcAddDefLib( "device" )
	fbcAddDefLib( "GL" )
end sub

private sub fbcHaikuPlatformAddSfxLibs( )
	if( fbcHaikuPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "media" )
	fbcAddDefLib( "midi" )
end sub

private sub fbcHaikuPlatformAddDefaultLibs( )
	if( fbcHaikuPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "root" )
	fbcAddDefLib( "bsd" )
#ifndef DISABLE_TCP
	fbcAddDefLib( "network" )
#endif
	fbcAddDefLib( "ncurses" )
	fbcAddDefLib( "be" )
	fbcAddDefLib( "game" )
	fbcAddDefLib( "stdc++" )
	fbcAddDefLib( "gcc_s" )
end sub

private sub fbcHaikuPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcHaikuPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of haiku/fbc-platform.bi
