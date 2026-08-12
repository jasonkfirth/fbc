''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: wii/fbc-platform.bi
''
'' Purpose:
''
''     Keep Nintendo Wii target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - choose the GCC driver for Wii linking
''     - locate devkitPro/libogc library directories
''     - add libogc default libraries expected by normal Wii homebrew
''     - add devkitPPC machine flags to the linker command
''
'' This file intentionally does NOT contain:
''
''     - command-line option parsing
''     - generic linker command construction
''     - elf-to-dol conversion
''

#ifndef __FBC_WII_PLATFORM_BI__
#define __FBC_WII_PLATFORM_BI__

private function fbcWiiPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WII)
end function

private function fbcWiiPlatformGetLinkerTool( ) as integer
	if( fbcWiiPlatformIsSelected( ) = FALSE ) then
		function = FBCTOOL_LD
		exit function
	end if

	''
	'' devkitPPC's Wii rules link through the GCC driver.  That driver adds the
	'' correct libgcc/newlib startup support around the libogc archives, while
	'' raw ld would require duplicating more SDK internals here.
	''
	function = FBCTOOL_GCC
end function

private function hGetWiiDevkitProDir( ) as string
	dim as string devkitpro = environ( "DEVKITPRO" )

	if( len( devkitpro ) = 0 ) then
		devkitpro = "/opt/devkitpro"
	end if

	function = pathStripDiv( devkitpro )
end function

private sub fbcWiiPlatformAddDefaultLibPaths( )
	if( fbcWiiPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLibPath( hGetWiiDevkitProDir( ) + "/libogc/lib/wii" )
end sub

private sub fbcWiiPlatformAddGfxLibs( )
	if( fbcWiiPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcWiiPlatformAddSfxLibs( )
	if( fbcWiiPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcWiiPlatformAddDefaultLibs( )
	if( fbcWiiPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	''
	'' These are the libogc-side libraries used by ordinary Wii homebrew that
	'' touches files, video, controllers, or sound.  Keep them ordered with the
	'' higher level convenience libraries first, then libogc/newlib/libgcc last.
	''
	fbcAddDefLib( "fat" )
	fbcAddDefLib( "wiiuse" )
	fbcAddDefLib( "bte" )
	fbcAddDefLib( "asnd" )
	fbcAddDefLib( "ogc" )
	fbcAddDefLib( "m" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "gcc" )
end sub

private sub fbcWiiPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcWiiPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	''
	'' devkitPro exposes these as MACHDEP in devkitPPC/wii_rules.  They select
	'' the Broadway/Gekko ABI and are required consistently at compile and link
	'' time.
	''
	ldcline += " -mrvl -mcpu=750 -meabi -mhard-float"
end sub

#endif

'' end of wii/fbc-platform.bi
