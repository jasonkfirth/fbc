''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: win32/fbc-platform.bi
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

private function fbcWin32PlatformUsesClangArm64Runtime( ) as integer
	function = fbcWin32PlatformIsSelected( ) andalso _
		(fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG) andalso _
		(fbGetCpuFamily( ) = FB_CPUFAMILY_AARCH64)
end function

private sub fbcWin32PlatformAddDefaultLibPaths( )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	if( fbcWin32PlatformUsesClangArm64Runtime( ) ) then
		fbcAddLibPathFor( "libclang_rt.builtins-aarch64.a" )
		fbcAddLibPathFor( "libc++.a" )
	end if

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

	if( fbcWin32PlatformUsesClangArm64Runtime( ) ) then
		fbcAddDefLib( "clang_rt.builtins-aarch64" )
		fbcAddDefLib( "unwind" )
	else
		fbcAddDefLib( "gcc" )
	end if

	fbcAddDefLib( "msvcrt" )
	fbcAddDefLib( "kernel32" )
	fbcAddDefLib( "user32" )
	fbcAddDefLib( "advapi32" )
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

private function fbcWin32PlatformMapLibName( byref libname as string ) as string
	if( fbcWin32PlatformUsesClangArm64Runtime( ) ) then
		if( libname = "stdc++" ) then
			return "c++"
		end if
	end if

	function = libname
end function

private sub fbcWin32PlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcWin32PlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of win32/fbc-platform.bi
