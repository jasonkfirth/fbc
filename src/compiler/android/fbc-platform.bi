''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: android/fbc-platform.bi
''
'' Purpose:
''
''     Keep Android target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add Android gfx library dependencies
''     - add Android default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Android ABI or code generation rules
''

#ifndef __FBC_ANDROID_PLATFORM_BI__
#define __FBC_ANDROID_PLATFORM_BI__

private function fbcAndroidPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ANDROID)
end function

private sub fbcAndroidPlatformAddDefaultLibPaths( )
	if( fbcAndroidPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcAndroidPlatformAddGfxLibs( )
	if( fbcAndroidPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "android" )
	fbcAddDefLib( "log" )
	fbcAddDefLib( "EGL" )
	fbcAddDefLib( "GLESv2" )
end sub

private sub fbcAndroidPlatformAddSfxLibs( )
	if( fbcAndroidPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcAndroidPlatformAddDefaultLibs( )
	if( fbcAndroidPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "m" )
	fbcAddDefLib( "dl" )
	fbcAddDefLib( "c" )
	'' On Android we don't even know what the runtime support library
	'' is called (it won't be libgcc when compiling with clang, unlike
	'' on Linux); we query gcc/clang later while building the link line.
end sub

private sub fbcAndroidPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcAndroidPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of android/fbc-platform.bi
