''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: wince/fbc-platform.bi
''
'' Purpose:
''
''     Keep Windows CE compiler-driver and CPU baseline policy in the WinCE
''     replacement tree.
''
'' Responsibilities:
''
''     - place the Windows CE declaration overlay before shared headers
''     - select the maintained ARM and MIPS tool prefixes
''     - preserve the ARMv4T and MIPS III software floating-point baselines
''     - select the appropriate compiler-driver or raw PE linker pipeline
''     - provide Windows CE dynamic-library options
''     - expose the standard platform hook surface
''
'' This file intentionally does NOT contain:
''
''     - generic PE/COFF emission rules
''     - Windows desktop library policy
''     - runtime, graphics, or sound implementation
''     - emulator and package orchestration
''
'' Toolchain ownership:
''
''     CeGCC's ARM specs select crt3.o, the CE old-name compatibility library,
''     MinGW support libraries, libgcc, and COREDLL.  MIPS uses Clang for
''     direct COFF object generation and GNU PE ld with the package's small,
''     target-specific startup object and import archives.
''

#ifndef __FBC_WINCE_PLATFORM_BI__
#define __FBC_WINCE_PLATFORM_BI__

private function fbcWincePlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WINCE)
end function

private sub fbcWincePlatformAddDefaultIncludePaths( byref incpath as string )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' COREDLL differs from desktop MSVCRT.  Its declarations must replace the
	'' shared CRT umbrella before individual includes are resolved.
	fbAddIncludePath( incpath + FB_HOST_PATHDIV + "wince" )
end sub

private function fbcWincePlatformGetToolPrefix( byval cputype as integer ) as string
	select case cputype
	case FB_CPUTYPE_ARMV4
		return "arm-mingw32ce"
	case FB_CPUTYPE_MIPS32, FB_CPUTYPE_MIPS32EL
		return "mips-wince-pe"
	end select

	function = ""
end function

private sub fbcWincePlatformAdjustParsedCpuType _
	( _
		byref arch as string, _
		byref cputype as integer _
	)
	if( arch = "arm" ) then
		cputype = FB_CPUTYPE_ARMV4
	elseif( arch = "mips" ) then
		'' Windows CE supported little-endian MIPS processors.  The historical
		'' tool prefix omits the endian suffix, so correct the generic GNU arch
		'' parser before it chooses target library directories and ABI metadata.
		cputype = FB_CPUTYPE_MIPS32EL
	end if
end sub

private function fbcWincePlatformGetLinkerTool( ) as integer
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		return FBCTOOL_LD
	end if

	select case fbGetOption( FB_COMPOPT_CPUTYPE )
	case FB_CPUTYPE_MIPS32, FB_CPUTYPE_MIPS32EL
		return FBCTOOL_LD
	case else
		return FBCTOOL_GCC
	end select
end function

private function fbcWincePlatformCompilesDirectlyToObject( ) as integer
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		return FALSE
	end if

	select case fbGetOption( FB_COMPOPT_CPUTYPE )
	case FB_CPUTYPE_MIPS32, FB_CPUTYPE_MIPS32EL
		'' LLVM's integrated assembler is the maintained producer of MIPS PE
		'' COFF objects.  GNU as from the recovered PE toolchain is intentionally
		'' not part of this pipeline.
		return (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG)
	end select

	function = FALSE
end function

private function fbcWincePlatformGetClangTargetOption( ) as string
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		return ""
	end if

	select case fbGetOption( FB_COMPOPT_CPUTYPE )
	case FB_CPUTYPE_MIPS32, FB_CPUTYPE_MIPS32EL
		return "--target=mipsel-pc-windows-msvc "
	end select

	function = ""
end function

private function fbcWincePlatformSupportsSupplementaryLinkerScript( ) as integer
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		return TRUE
	end if

	'' The recovered MIPS PE linker predates GNU ld's INSERT command.  Object
	'' metadata remains harmless in development builds and release stripping
	'' removes it, so do not feed the ELF-oriented fbextra.x fragment to it.
	function = (fbcWincePlatformGetLinkerTool( ) <> FBCTOOL_LD)
end function

private function fbcWincePlatformAddCCompilerCpuOptions _
	( _
		byref ccline as string _
	) as integer

	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		return FALSE
	end if

	select case fbGetOption( FB_COMPOPT_CPUTYPE )
	case FB_CPUTYPE_ARMV4
		'' CeGCC does not publish the customary SDK version macro itself.
		'' Defining the Windows CE 5.0 API baseline makes third-party headers
		'' and the bundled libffi agree with the runtime's supported surface.
		ccline += "-march=armv4t -mfloat-abi=soft -D_WIN32_WCE=0x0500 "
		return TRUE
	case FB_CPUTYPE_MIPS32, FB_CPUTYPE_MIPS32EL
		'' MIPS III is the common baseline for CERF's CE 2.0 through CE 3.0
		'' devices.  Software floating point also avoids an accidental MIPS IV
		'' FPU dependency when compiling on a modern LLVM host.
		ccline += "-mcpu=mips3 -mabi=32 -msoft-float "
		ccline += "-ffreestanding -fno-builtin "
		ccline += "-D__MINGW32CE__ -D__MINGW32__ -D__COREDLL__ "
		ccline += "-D_WIN32_WCE=0x0500 -D_M_MRX000=4000 -DMIPS "
		return TRUE
	end select

	function = FALSE
end function

private function fbcWincePlatformUsesCompilerDriverAssembler( ) as integer
	function = fbcWincePlatformIsSelected( )
end function

private sub fbcWincePlatformAddAssemblerOptions( byref ascline as string )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	select case fbGetOption( FB_COMPOPT_CPUTYPE )
	case FB_CPUTYPE_ARMV4
		'' CeGCC's assembler defaults to plain ARMv4.  GCC-generated code uses
		'' BX for ARM/Thumb interworking, so the assembler must see the same
		'' ARMv4T baseline selected during C generation.
		ascline += "-march=armv4t -mfloat-abi=soft "
	case FB_CPUTYPE_MIPS32, FB_CPUTYPE_MIPS32EL
		ascline += "--target=mipsel-pc-windows-msvc -mcpu=mips3 "
		ascline += "-mabi=32 -msoft-float "
	end select
end sub

private sub fbcWincePlatformAddLinkOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	)
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
		dllname = hStripPath( hStripExt( fbc.outname ) )
		if( fbcWincePlatformGetLinkerTool( ) = FBCTOOL_LD ) then
			ldcline += " --dll --enable-stdcall-fixup"
			ldcline += " --subsystem wince:5.0 --entry DllMainCRTStartup"
		else
			ldcline += " -shared"
		end if
	elseif( fbcWincePlatformGetLinkerTool( ) = FBCTOOL_LD ) then
		ldcline += " --subsystem wince:5.0 --entry WinMainCRTStartup"
		ldcline += " --stack 1048576,4096 --heap 1048576,4096"
	end if
end sub

private sub fbcWincePlatformAddCrtBeginObjects( byref ldcline as string )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	if( fbcWincePlatformGetLinkerTool( ) = FBCTOOL_LD ) then
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			ldcline += " """ + fbc.libpath + FB_HOST_PATHDIV + "dllcrt0.o"""
		else
			ldcline += " """ + fbc.libpath + FB_HOST_PATHDIV + "crt0.o"""
		end if
	end if
end sub

'' CeGCC's specs own the system library paths and default import libraries.
private sub fbcWincePlatformAddDefaultLibPaths( )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcWincePlatformAddGfxLibs( )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcWincePlatformAddSfxLibs( )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcWincePlatformAddDefaultLibs( )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef DISABLE_TCP
	'' Classic Windows CE images expose their IPv4 socket surface through the
	'' original WinSock import library.  The WinCE runtime intentionally avoids
	'' WS2-only address-resolution entry points so ARMv4 packages remain usable
	'' on older devices.
	fbcAddDefLib( "winsock" )
#endif

	if( fbcWincePlatformGetLinkerTool( ) = FBCTOOL_LD ) then
		'' Clang lowers MIPS software floating-point and wide-integer operations
		'' to compiler-rt helpers.  Raw GNU PE ld also needs an explicit COREDLL
		'' dependency; ARM receives its corresponding libraries from CeGCC's
		'' GCC specs.
		fbcAddDefLib( "clang_rt.builtins-mips" )
		fbcAddDefLib( "coredll" )
	end if
end sub

private sub fbcWincePlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcWincePlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of wince/fbc-platform.bi
