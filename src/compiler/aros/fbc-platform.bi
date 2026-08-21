''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: aros/fbc-platform.bi
''
'' Purpose:
''
''     Keep AROS compiler-driver and architecture baseline policy in the AROS
''     replacement tree.
''
'' Responsibilities:
''
''     - map friendly AROS targets to the canonical SDK tool prefixes
''     - select the AROS GCC driver for assembly and final linking
''     - apply the SDK's per-port ISA and floating-point contract
''     - allocate BASIC COMMON symbols before AROS LoadSeg sees the executable
''     - link the native pthread provider required by the AROS runtime
''     - derive native package paths without unsupported parent traversal
''     - place the AROS declaration overlay before shared headers
''     - convert native-hosted m68k executable output from ELF to Hunk
''
'' This file intentionally does NOT contain:
''
''     - generic m68k identity, endianness, or ELF handling
''     - command-line option parsing
''     - Intuition graphics or AHI sound implementation
''
'' Toolchain ownership:
''
''     The generated AROS GCC specs own startup objects, collect-aros, the
''     system library set, and target library paths.  Duplicating that list in
''     fbc would couple packages to one SDK build directory and specs revision.
''

#ifndef __FBC_AROS_PLATFORM_BI__
#define __FBC_AROS_PLATFORM_BI__

private function fbcArosPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_AROS)
end function

private sub fbcArosPlatformAdjustPrefix( byref prefix as string )
	#if defined( __FB_AROS__ )
		if( fbcArosPlatformIsSelected( ) = FALSE ) then
			exit sub
		end if

		'' Native packages install fbc below <root>/bin.  AmigaDOS accepts
		'' slashes below a volume or assign, but does not resolve the Unix-style
		'' bin/../lib path produced by the generic non-standalone layout.  Strip
		'' the bin component explicitly so every later tool, include, and library
		'' path begins at the canonical package root.
		dim as string bindir = pathStripDiv( exepath( ) )
		dim as string root = hStripFilename( bindir )
		if( len( root ) > 0 ) then
			prefix = root
		end if
	#endif
end sub

private sub fbcArosPlatformAddDefaultIncludePaths( byref incpath as string )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' AROS headers replace only declarations whose POSIXC ABI differs.
	fbAddIncludePath( incpath + FB_HOST_PATHDIV + "aros" )
end sub

private function fbcArosPlatformGetToolPrefix( byval cputype as integer ) as string
	select case cputype
	case FB_CPUTYPE_M68K
		return "m68k-aros"
	case FB_CPUTYPE_ARMV4, FB_CPUTYPE_ARMV5TE, FB_CPUTYPE_ARMV6, _
	     FB_CPUTYPE_ARMV6_FP, FB_CPUTYPE_ARMV7A, FB_CPUTYPE_ARMV7A_FP
		return "arm-aros"
	case FB_CPUTYPE_X86_64
		return "x86_64-aros"
	end select

	function = ""
end function

private sub fbcArosPlatformAdjustParsedCpuType _
	( _
		byref arch as string, _
		byref cputype as integer _
	)
	if( arch = "arm" ) then
		'' raspi-armhf uses the ARMv7-A hard-float ABI even though its canonical
		'' GNU architecture component is the unqualified name "arm".
		cputype = FB_CPUTYPE_ARMV7A_FP
	end if
end sub

private function fbcArosPlatformGetLinkerTool( ) as integer
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		return FBCTOOL_LD
	end if

	return FBCTOOL_GCC
end function

private function fbcArosPlatformUsesCompilerDriverAssembler( ) as integer
	function = fbcArosPlatformIsSelected( )
end function

private function fbcArosHostFinishExecutable( ) as integer
	#if defined( __FB_AROS__ ) and defined( __FB_M68K__ )
		if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_AROS) and _
		    (fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE) ) then
			dim as string hunkname = fbc.outname + ".hunk"

			'' The m68k GCC driver deliberately leaves a relocatable ELF file.
			'' AROS's classic LoadSeg loader needs a Hunk executable, so perform
			'' the format conversion only when fbc itself is hosted on AROS m68k.
			'' Cross hosts retain ELF until their packaging or emulator boundary.
			safeKill( hunkname )
			if( fbcRunBin( "making Hunk", FBCTOOL_ELF2HUNK, _
			               QUOTE + fbc.outname + QUOTE + " " + _
			               QUOTE + hunkname + QUOTE ) = FALSE ) then
				return FALSE
			end if

			if( kill( fbc.outname ) <> 0 ) then
				safeKill( hunkname )
				return FALSE
			end if
			name hunkname as fbc.outname
		end if
	#endif

	function = TRUE
end function

private sub fbcArosPlatformAddCcQueryOptions( byref path as string )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' GCC's file queries must select the same multilib as compilation and
	'' linking.  Otherwise ARM hard-float programs receive the default soft-
	'' float libgcc archive when a generated operation needs a helper routine.
	select case fbGetCpuFamily( )
	case FB_CPUFAMILY_M68K
		path += " -march=68000 -msoft-float"
	case FB_CPUFAMILY_ARM
		path += " -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4"
	case FB_CPUFAMILY_X86_64
		path += " -m64 -mcmodel=large -mno-red-zone"
	end select
end sub

private function fbcArosPlatformAddCCompilerCpuOptions( byref ccline as string ) as integer
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		return FALSE
	end if

	'' AROS LoadSeg rejects ELF SHN_COMMON symbols.  This is an AROS object
	'' contract for every CPU family, not part of a generic CPU baseline.
	ccline += "-fno-common "

	select case fbGetCpuFamily( )
	case FB_CPUFAMILY_M68K
		'' amiga-m68k is configured for every 68000-class machine and uses the
		'' software floating-point libraries selected by this exact multilib.
		ccline += "-march=68000 -msoft-float "
	case FB_CPUFAMILY_ARM
		'' raspi-armhf is the ARMv7-A hard-float port despite the arm-aros
		'' compiler prefix not carrying an hf suffix.
		ccline += "-march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4 "
	case FB_CPUFAMILY_X86_64
		'' AROS uses a large kernel-oriented address model without a red zone.
		ccline += "-m64 -mcmodel=large -mno-red-zone "
	case else
		return FALSE
	end select

	function = TRUE
end function

private sub fbcArosPlatformAddAssemblerOptions( byref ascline as string )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	select case fbGetCpuFamily( )
	case FB_CPUFAMILY_M68K
		ascline += "-march=68000 -msoft-float "
	case FB_CPUFAMILY_ARM
		ascline += "-march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4 "
	case FB_CPUFAMILY_X86_64
		ascline += "-m64 -mcmodel=large -mno-red-zone "
	end select
end sub

private sub fbcArosPlatformAddDefaultLibPaths( )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	#if defined( __FB_AROS__ )
		'' Native GCC's Unix-configured default library root is not directly
		'' usable through AmigaDOS.  Add the native path here so -L precedes
		'' every -l option; the m68k GNU linker resolves archives in order.
		dim as string native_link_prefix = _
			environ( "FBC_AROS_NATIVE_LINK_PREFIX" )
		if( len( native_link_prefix ) = 0 ) then
			native_link_prefix = "Developer:lib/"
		end if
		'' GNU ld inserts its own separator before each archive name.  A doubled
		'' slash means "parent directory" to AmigaDOS, so keep -L unambiguously
		'' directory-valued while retaining the trailing slash required by -B.
		fbcAddDefLibPath( pathStripDiv( native_link_prefix ) )
	#endif
end sub

private sub fbcArosPlatformAddGfxLibs( )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' CyberGraphX, Intuition, and Graphics are AROS GCC default libraries.
end sub

private sub fbcArosPlatformAddSfxLibs( )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' sfxlib owns an internal AHI feeder even when the BASIC program does not
	'' request the public multithreaded runtime.
	fbcAddDefLib( "pthread" )
end sub

private sub fbcArosPlatformAddDefaultLibs( )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' The normal runtime uses pthread mutexes for console state and may start
	'' its background input service.  This dependency applies independently of
	'' the public -mt language option.
	fbcAddDefLib( "pthread" )

	if( fbGetCpuFamily( ) = FB_CPUFAMILY_M68K ) then
		'' The AROS m68k runtime wraps selected soft-float helpers to repair
		'' special-value behavior.  Keep libgcc in fbc's rescan group so GNU ld
		'' can resolve the wrappers' __real_* references even when the wrapper
		'' archive member is pulled in only during a later group pass.
		fbcAddDefLib( "gcc" )
	end if
end sub

private sub fbcArosPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcArosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	#if defined( __FB_AROS__ )
		'' Native GCC is installed below the conventional Developer: assign.
		'' Its Unix-configured /Developer start-file prefix cannot be opened by
		'' AmigaDOS, while -B accepts native assign syntax for startup-object and
		'' compiler-component lookup.  The default-library-path hook separately
		'' places the same prefix before all -l inputs.  Emulator qualification
		'' can point FBC_AROS_NATIVE_LINK_PREFIX at a RAM copy when its host-volume
		'' transport cannot seek reliably inside ELF archives.
		dim as string native_link_prefix = _
			environ( "FBC_AROS_NATIVE_LINK_PREFIX" )
		if( len( native_link_prefix ) = 0 ) then
			native_link_prefix = "Developer:lib/"
		end if
		ldcline += " -B""" + native_link_prefix + """"
	#endif

	'' The GCC driver uses these options to select the same architecture and
	'' floating-point multilib that compiled every object.  This is essential
	'' on ARM once a program needs a libgcc helper such as 64-bit division.
	select case fbGetCpuFamily( )
	case FB_CPUFAMILY_M68K
		'' GCC 6.5's AROS m68k __truncdfsf2 wraps an overflowing binary64
		'' exponent instead of returning binary32 infinity, and __extendsfdf2
		'' treats binary32 infinity as an ordinary exponent.  Both helpers share
		'' one archive object with other required operations, so normal symbol
		'' overrides would cause multiple definitions.  GNU ld wrapping redirects
		'' generated conversion references to the AROS runtime corrections.
		ldcline += " -march=68000 -msoft-float"
		ldcline += " -Wl,--wrap=__truncdfsf2"
		ldcline += " -Wl,--wrap=__extendsfdf2"
		ldcline += " -Wl,--wrap=__divdf3"
		ldcline += " -Wl,--wrap=__divsf3"
	case FB_CPUFAMILY_ARM
		ldcline += " -march=armv7-a -marm -mfloat-abi=hard -mfpu=neon-vfpv4"
	case FB_CPUFAMILY_X86_64
		ldcline += " -m64 -mcmodel=large -mno-red-zone"
	end select

	'' AROS installs archive libraries, but -static is not merely a format
	'' selection in its GCC specs.  It selects the reduced kernel-style stdc
	'' library set and omits posixc.  Normal application linking must therefore
	'' remain in the driver's default mode even though the resulting executable
	'' is self-contained.

	'' BASIC COMMON declarations intentionally produce ELF SHN_COMMON symbols,
	'' independently of GCC's -fno-common policy for tentative C definitions.
	'' AROS LoadSeg rejects SHN_COMMON in its final relocatable executable, so
	'' ask the GNU linker to allocate these language-level symbols into BSS.
	ldcline += " -Wl,-d"
end sub

#endif

'' end of aros/fbc-platform.bi
