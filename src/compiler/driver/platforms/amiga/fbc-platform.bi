'' Project: FreeBASIC compiler driver
'' -------------------------
''
'' File: driver/platforms/amiga/fbc-platform.bi
''
'' Purpose:
''     Select the classic AmigaOS 3.x toolchain and native library contract.
''
'' Responsibilities:
''     - use the m68k-amigaos GCC driver for compilation and Hunk linking
''     - keep the 68020 software floating-point baseline consistent
''     - place Amiga C runtime declarations before shared include files
''
'' This file intentionally does NOT contain:
''     - generic m68k alignment or byte-order rules
''     - BASIC grammar, graphics, sound, or emulator transport
''

#ifndef __FBC_AMIGA_PLATFORM_BI__
#define __FBC_AMIGA_PLATFORM_BI__

private function fbcAmigaPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_AMIGA)
end function

sub fbcAmigaPlatformValidateOptions( )
	if( fbcAmigaPlatformIsSelected( ) = FALSE ) then exit sub
	if( fbGetCpuFamily( ) <> FB_CPUFAMILY_M68K ) then
		errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "AmigaOS requires -arch m68k", -1 )
		fbcEnd( 1 )
	end if
	if( fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GCC ) then
		errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "AmigaOS requires -gen gcc", -1 )
		fbcEnd( 1 )
	end if
	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
		errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "AmigaOS does not support -dll", -1 )
		fbcEnd( 1 )
	end if
end sub

private function fbcAmigaPlatformGetLinkerTool( ) as integer
	if( fbcAmigaPlatformIsSelected( ) ) then
		return FBCTOOL_GCC
	end if
	function = FBCTOOL_LD
end function

private function fbcAmigaPlatformGetToolPrefix( byval cputype as integer ) as string
	if( cputype = FB_CPUTYPE_M68K ) then
		return "m68k-amigaos"
	end if
	function = ""
end function

private sub fbcAmigaPlatformAddDefaultIncludePaths( byref incpath as string )
	if( fbcAmigaPlatformIsSelected( ) ) then
		fbAddIncludePath( incpath + FB_HOST_PATHDIV + "amiga" )
	end if
end sub

private sub fbcAmigaPlatformAddCcQueryOptions( byref path as string )
	if( fbcAmigaPlatformIsSelected( ) ) then
		path += " -m68020 -msoft-float"
	end if
end sub

private function fbcAmigaPlatformAddCCompilerCpuOptions( byref ccline as string ) as integer
	if( fbcAmigaPlatformIsSelected( ) = FALSE ) then
		return FALSE
	end if

	'' The compiler and every runtime archive must use the same soft-float
	'' multilib. The Amiga GCC driver supplies the SDK's default newlib CRT.
	ccline += "-m68020 -msoft-float -fno-common "
	function = TRUE
end function

private sub fbcAmigaPlatformAddAssemblerOptions( byref ascline as string )
	if( fbcAmigaPlatformIsSelected( ) ) then
		ascline += "-m68020 -msoft-float "
	end if
end sub

private sub fbcAmigaPlatformAddDefaultLibPaths( )
	if( fbcAmigaPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
	'' GCC specs own the SDK and multilib paths, including native startup.
end sub

private sub fbcAmigaPlatformAddGfxLibs( )
	if( fbcAmigaPlatformIsSelected( ) ) then
		fbcAddDefLib( "amiga" )
	end if
end sub

private sub fbcAmigaPlatformAddSfxLibs( )
	if( fbcAmigaPlatformIsSelected( ) ) then
		fbcAddDefLib( "amiga" )
		fbcAddDefLib( "pthread" )
	end if
end sub

private sub fbcAmigaPlatformAddDefaultLibs( )
	if( fbcAmigaPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' libamiga supplies tag-list call helpers. GCC specs own the CRT startup,
	'' libc, libm, and libgcc order; do not substitute a bare Unix ld command.
	'' The SDK also defines float helpers in libc through disk-based Amiga
	'' math libraries. Resolve them from GCC's soft-float archive first.
	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "amiga" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "m" )
	'' Hunk objects do not carry the ELF library-dependency metadata emitted
	'' by separately compiled THREADCALL users. Keep its static provider in
	'' the default group; the linker selects members only when needed.
	fbcAddDefLib( "ffi" )
end sub

private sub fbcAmigaPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcAmigaPlatformIsSelected( ) ) then
		ldcline += " -m68020 -msoft-float"
	end if
end sub

sub fbcAmigaPlatformAddLinkOptions( byref ldcline as string )
	if( fbcAmigaPlatformIsSelected( ) and (fbc.nodeflibs = FALSE) ) then
		'' Load the complete CPU helper provider before other archives. Hunk
		'' ld otherwise selects newlib's indirect aliases or duplicate GCC
		'' conversion members before it has seen the software definitions.
		ldcline += " -Wl,--whole-archive -lfbsoftfloat -Wl,--no-whole-archive"
	end if
end sub

#endif

'' end of driver/platforms/amiga/fbc-platform.bi
