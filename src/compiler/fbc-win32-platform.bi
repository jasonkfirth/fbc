''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-win32-platform.bi
''
'' Purpose:
''
''     Keep Win32 target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add MinGW library search paths
''     - add Win32 gfx and sound library dependencies
''     - add Win32 default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Win32 ABI or code generation rules
''

#ifndef __FBC_WIN32_PLATFORM_BI__
#define __FBC_WIN32_PLATFORM_BI__

private function fbcWin32PlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WIN32)
end function

private sub fbcWin32PlatformAddDefaultLibPaths( )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	'' Help the MinGW linker to find MinGW's lib/ dir, allowing
	'' the C:\MinGW dir to be renamed and linking to still work.
	fbcAddLibPathFor( "libmingw32.a" )
#endif
end sub

private sub fbcWin32PlatformAddGfxLibs( )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gdi32" )
	fbcAddDefLib( "winmm" )
end sub

private sub fbcWin32PlatformAddSfxLibs( )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "winmm" )
	fbcAddDefLib( "ole32" )
	fbcAddDefLib( "uuid" )
end sub

private sub fbcWin32PlatformAddDefaultLibs( )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "msvcrt" )
	fbcAddDefLib( "kernel32" )
	fbcAddDefLib( "user32" )
#ifndef DISABLE_TCP
	fbcAddDefLib( "ws2_32" )
#endif
	fbcAddDefLib( "mingw32" )
	fbcAddDefLib( "mingwex" )
	fbcAddDefLib( "moldname" )

	'' Link libgcc_eh if it exists
	if( (len( fbcFindLibFile( "libgcc_eh.a"     ) ) > 0) or _
		(len( fbcFindLibFile( "libgcc_eh.dll.a" ) ) > 0) ) then
		'' Needed by mingw.org toolchain, but not TDM-GCC
		fbcAddDefLib( "gcc_eh" )
	end if

	'' profiling?
	if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
		fbcAddDefLib( "gmon" )
	end if
end sub

private sub fbcWin32PlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-win32-platform.bi
