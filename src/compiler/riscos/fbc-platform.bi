''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: riscos/fbc-platform.bi
''
'' Purpose:
''
''     Keep RISC OS linker-driver policy in the RISC OS replacement tree.
''
'' Responsibilities:
''
''     - place the RISC OS declaration overlay before shared headers
''     - select GCCSDK's GCC driver for the final link
''     - select static executable linking while the port ships static runtimes
''     - provide the platform hook entry points used by fbc-platform.bi
''     - document which link responsibilities belong to GCCSDK
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - ARM code generation or ABI selection
''
'' GCCSDK linker ownership:
''
''     The GCC driver specs select armelf_riscos, the software floating-point
''     ABI, and the RISC OS startup objects. They also place UnixLib and libgcc
''     correctly around user libraries. Repeating that policy here would make
''     fbc depend on GCCSDK internals.
''

#ifndef __FBC_RISCOS_PLATFORM_BI__
#define __FBC_RISCOS_PLATFORM_BI__

private function fbcRiscosPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_RISCOS)
end function

private sub fbcRiscosPlatformAddDefaultIncludePaths( byref incpath as string )
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' RISC OS headers are complete replacements for generic headers.
	fbAddIncludePath( incpath + FB_HOST_PATHDIV + "riscos" )
end sub

private function fbcRiscosPlatformGetToolPrefix( byval cputype as integer ) as string
	if( cputype = FB_CPUTYPE_ARMV4 ) then
		return "arm-unknown-riscos"
	end if

	function = ""
end function

private sub fbcRiscosPlatformAdjustParsedCpuType _
	( _
		byref arch as string, _
		byref cputype as integer _
	)
	if( arch = "arm" ) then
		'' GCCSDK's supported baseline remains compatible with StrongARM RiscPCs.
		cputype = FB_CPUTYPE_ARMV4
	end if
end sub

private function fbcRiscosPlatformGetLinkerTool( ) as integer
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		function = FBCTOOL_LD
		exit function
	end if

	function = FBCTOOL_GCC
end function

private function fbcRiscosPlatformSupportsSupplementaryLinkerScript( ) as integer
	'' GCCSDK's driver owns its complete linker script.  An INSERT fragment is
	'' neither required nor portable to the older RISC OS linker releases.
	function = FALSE
end function

'' A native UnixLib compiler must let !GCC's Run$Path resolve system tools.
'' Cross-host compilers continue to query GCC for the exact assembler/linker.
private function fbcRiscosHostMayQueryCcForTool( ) as integer
	#ifdef __FB_RISCOS__
		return FALSE
	#else
		return TRUE
	#endif
end function

private function fbcRiscosHostRunTool _
	( _
		byval tool as integer, _
		byref path as string, _
		byref arguments as string, _
		byref was_handled as integer _
	) as integer

	was_handled = FALSE

	#ifdef __FB_RISCOS__
		#ifndef ENABLE_STANDALONE
			if( fbctoolGetFlags( tool, FBCTOOLFLAG_RELYING_ON_SYSTEM ) ) then
				was_handled = TRUE
				return shell( path + " " + arguments )
			end if

			if( tool = FBCTOOL_ELF2AIF ) then
				'' UnixLib's exec() cannot start the bundled &FF8 converter through
				'' its Unix-style absolute path.  !FreeBASIC registers bin/ on
				'' Run$Path, so let the RISC OS command resolver load elf2aif by
				'' its native file type, just as a user starts it at the CLI.
				was_handled = TRUE
				return shell( "elf2aif " + arguments )
			end if
		#endif
	#endif

	return 0
end function

private function fbcRiscosHostFinishExecutable( ) as integer
	#ifdef __FB_RISCOS__
		if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_RISCOS) and _
		    (fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE) ) then
			dim as string aifname = fbc.outname + "-aif"

			'' elf2aif's one-path mode converts through its own temporary name.
			'' On UnixLib, that replacement can leave the original ELF behind
			'' while still returning success.  Use its documented input/output
			'' form, then replace the ELF only after a separate AIF exists.
			'' The hyphenated temporary leaf deliberately avoids a new UnixLib
			'' suffix mapping and therefore keeps the final file type as &FF8.
			safeKill( aifname )
			if( fbcRunBin( "making AIF", FBCTOOL_ELF2AIF, _
			               QUOTE + fbc.outname + QUOTE + " " + _
			               QUOTE + aifname + QUOTE ) = FALSE ) then
				return FALSE
			end if

			if( kill( fbc.outname ) <> 0 ) then
				safeKill( aifname )
				return FALSE
			end if
			name aifname as fbc.outname
		end if
	#endif

	return TRUE
end function

'' The generic driver already adds the FreeBASIC runtime, gfxlib2, and sfxlib.
'' No additional RISC OS search directories or platform libraries are needed
'' here because GCCSDK owns the C runtime paths and default libraries.
private sub fbcRiscosPlatformAddDefaultLibPaths( )
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcRiscosPlatformAddGfxLibs( )
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcRiscosPlatformAddSfxLibs( )
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcRiscosPlatformAddDefaultLibs( )
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcRiscosPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' The current port deliberately ships static runtime libraries only.
	'' Asking GCCSDK's legacy linker for its dynamic default leaves it with
	'' static inputs and reaches a broken dynamic-section path in BFD.
	if( (fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE) and _
	    (fbc.staticlink = FALSE) ) then
		ldcline += " -static"
	end if
end sub

#endif

'' end of riscos/fbc-platform.bi
''
