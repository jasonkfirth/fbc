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

private function fbcRiscosPlatformGetLinkerTool( ) as integer
	if( fbcRiscosPlatformIsSelected( ) = FALSE ) then
		function = FBCTOOL_LD
		exit function
	end if

	function = FBCTOOL_GCC
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
		#endif
	#endif

	return 0
end function

private function fbcRiscosHostFinishExecutable( ) as integer
	#ifdef __FB_RISCOS__
		if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_RISCOS) and _
		    (fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE) ) then
			return fbcRunBin( "making AIF", FBCTOOL_ELF2AIF, _
			                  QUOTE + fbc.outname + QUOTE )
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
