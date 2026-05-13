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
''     - choose the compiler driver for Darwin link jobs
''     - add Darwin compiler/assembler options
''     - add Darwin gfx library dependencies
''     - add Darwin compiler-driver linker options
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

#ifndef FB_DARWIN_DEFAULT_DEPLOYMENT_TARGET
#define FB_DARWIN_DEFAULT_DEPLOYMENT_TARGET ""
#endif

''
'' Target selection
''

private function fbcDarwinPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN)
end function

private function fbcDarwinPlatformGetLinkerTool( ) as integer
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		function = FBCTOOL_LD
		exit function
	end if

	''
	'' Darwin link lines use compiler-driver options such as
	'' -mmacosx-version-min=... and depend on the driver for the platform
	'' runtime defaults.  Apple ld64 does not accept those options directly.
	''
	function = FBCTOOL_GCC
end function

''
'' Compiler-driver linker options
''

private function fbcDarwinPlatformGetDeploymentTarget( ) as string
	dim as string version = environ( "MACOSX_DEPLOYMENT_TARGET" )

	if( len( version ) > 0 ) then
		function = version
		exit function
	end if

	version = FB_DARWIN_DEFAULT_DEPLOYMENT_TARGET
	if( len( version ) > 0 ) then
		''
		'' The makefiles may raise the default deployment target when the
		'' compiler is linked against Homebrew dylibs built for a newer macOS.
		'' User-supplied MACOSX_DEPLOYMENT_TARGET still wins above.
		''
		function = version
		exit function
	end if

	''
	'' Apple Silicon first shipped with macOS 11.  Older deployment targets
	'' are valid for Intel builds, but arm64 objects cannot be linked for the
	'' pre-11.0 macOS ABI.
	''
	select case as const fbGetCpuFamily( )
	case FB_CPUFAMILY_AARCH64
		function = "11.0"
	case else
		''
		'' macOS 10.5 keeps Intel builds on an old deployment floor without
		'' tripping modern ld64 warnings about pre-10.5 dynamic symbol flags.
		''
		function = "10.5"
	end select
end function

private sub fbcDarwinPlatformAddCCompilerOptions( byref ccline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	dim as string version = fbcDarwinPlatformGetDeploymentTarget( )
	if( len( version ) > 0 ) then
		ccline += "-mmacosx-version-min=" + version + " "
	end if

	''
	'' The C backend emits prototypes for a few C runtime entry points because
	'' the generated C is compiled with -nostdinc.  Apple clang still knows
	'' about C library builtins and can warn when FreeBASIC declarations are
	'' not textually identical to the SDK typedefs.
	''
	ccline += "-fno-builtin "
end sub

private sub fbcDarwinPlatformAddAssemblerOptions( byref ascline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	dim as string version = fbcDarwinPlatformGetDeploymentTarget( )
	if( len( version ) = 0 ) then
		exit sub
	end if

	''
	'' Assembly produced by the C compiler normally carries its own minimum
	'' version directive. This option covers direct GAS output and keeps object
	'' metadata aligned with the final link target.
	''
	ascline += "-mmacosx-version-min=" + version + " "
end sub

private sub fbcDarwinPlatformAddDeploymentTarget( byref ldcline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	dim as string version = fbcDarwinPlatformGetDeploymentTarget( )
	if( len( version ) = 0 ) then
		exit sub
	end if

	''
	'' The Darwin build invokes the compiler driver as the linker so that it
	'' can provide libgcc/compiler-rt and the modern Apple startup defaults.
	'' Use the driver option here, not the raw ld64 -macosx_version_min form.
	''
	ldcline += " -mmacosx-version-min=" + version
end sub

private function fbcDarwinPlatformAddDynamicLibOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	) as integer

	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		return FALSE
	end if

	if( fbGetOption( FB_COMPOPT_OUTTYPE ) <> FB_OUTTYPE_DYNAMICLIB ) then
		return TRUE
	end if

	dllname = hStripPath( hStripExt( fbc.outname ) )

	''
	'' Darwin dylibs are built through ld64 via the compiler driver.  The ELF
	'' -shared/-h pair is not accepted there; -dynamiclib selects a Mach-O
	'' dynamic library and -install_name records the runtime library identity.
	''
	ldcline += " -dynamiclib"
	ldcline += " -install_name " + QUOTE + hStripPath( fbc.outname ) + QUOTE

	'' Turn libfoo into foo, so it can be checked against -l foo below.
	if( left( dllname, 3 ) = "lib" ) then
		dllname = right( dllname, len( dllname ) - 3 )
	end if

	return TRUE
end function

private sub fbcDarwinPlatformAddExportDynamic( byref ldcline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	if( fbGetOption( FB_COMPOPT_EXPORT ) = FALSE ) then
		exit sub
	end if

	ldcline += " -Wl,-export_dynamic"
end sub

''
'' Default library policy
''

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

''
'' Frameworks
''

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
