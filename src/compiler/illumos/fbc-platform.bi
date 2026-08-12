''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: illumos/fbc-platform.bi
''
'' Purpose:
''
''     Keep illumos target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add illumos gfx library dependencies
''     - add illumos default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - illumos ABI or code generation rules
''

#ifndef __FBC_ILLUMOS_PLATFORM_BI__
#define __FBC_ILLUMOS_PLATFORM_BI__

private function fbcIllumosPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ILLUMOS)
end function

private function fbcIllumosPlatformGetOoCeLibDir( ) as string
	select case as const fbGetCpuFamily( )
	case FB_CPUFAMILY_X86_64
		function = "/opt/ooce/lib/amd64"
	case else
		function = "/opt/ooce/lib"
	end select
end function

private sub fbcIllumosPlatformAddDefaultLibPaths( )
	if( fbcIllumosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLibPath( fbcIllumosPlatformGetOoCeLibDir( ) )
end sub

private sub fbcIllumosPlatformAddGfxLibs( )
	if( fbcIllumosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcIllumosPlatformAddSfxLibs( )
	if( fbcIllumosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcIllumosPlatformAddDefaultLibs( )
	if( fbcIllumosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "m" )
	fbcAddDefLib( "c" )
#ifndef DISABLE_TCP
	fbcAddDefLib( "socket" )
	fbcAddDefLib( "nsl" )
#endif
	fbcAddDefLib( "ncurses" )
end sub

private sub fbcIllumosPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcIllumosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	ldcline += " -rpath " + fbcIllumosPlatformGetOoCeLibDir( ) + " "
end sub

#endif

'' end of illumos/fbc-platform.bi
