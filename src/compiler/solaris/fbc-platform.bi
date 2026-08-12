''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: solaris/fbc-platform.bi
''
'' Purpose:
''
''     Keep Solaris target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add Solaris gfx library dependencies
''     - add Solaris default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Solaris ABI or code generation rules
''

#ifndef __FBC_SOLARIS_PLATFORM_BI__
#define __FBC_SOLARIS_PLATFORM_BI__

private function fbcSolarisPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_SOLARIS)
end function

private sub fbcSolarisPlatformAddDefaultLibPaths( )
	if( fbcSolarisPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcSolarisPlatformAddGfxLibs( )
	if( fbcSolarisPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcSolarisPlatformAddSfxLibs( )
	if( fbcSolarisPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcSolarisPlatformAddDefaultLibs( )
	if( fbcSolarisPlatformIsSelected( ) = FALSE ) then
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

private sub fbcSolarisPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcSolarisPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of solaris/fbc-platform.bi
