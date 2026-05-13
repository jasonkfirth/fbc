''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-darwin-platform.bi
''
'' Purpose:
''
''     Keep Darwin target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add Darwin gfx library dependencies
''     - add Darwin framework linker options
''     - add Darwin default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Darwin ABI or code generation rules
''

#ifndef __FBC_DARWIN_PLATFORM_BI__
#define __FBC_DARWIN_PLATFORM_BI__

private function fbcDarwinPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN)
end function

private sub fbcDarwinPlatformAddDefaultLibPaths( )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcDarwinPlatformAddGfxLibs( )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcDarwinPlatformAddSfxLibs( )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcDarwinPlatformAddDefaultLibs( )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "ffi" )
	fbcAddDefLib( "ncurses" )
	fbcAddDefLib( "m" )
end sub

private sub fbcDarwinPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	if( fbGetOption( FB_COMPOPT_FBGFX ) ) then
		ldcline += " -lobjc"
		ldcline += " -framework AppKit"
		ldcline += " -framework Foundation"
		ldcline += " -framework CoreGraphics"
	end if

	if( fbGetOption( FB_COMPOPT_FBSFX ) ) then
		ldcline += " -framework AudioToolbox"
		ldcline += " -framework CoreAudio"
		ldcline += " -framework CoreFoundation"
		ldcline += " -framework CoreMIDI"
	end if
end sub

#endif

'' end of fbc-darwin-platform.bi
