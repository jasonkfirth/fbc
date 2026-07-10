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
''     - wrap GUI executables in a simple .app bundle
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
'' Directory attribute used by dir().
''
'' Keep this local to the compiler driver instead of depending on the public
'' dir.bi include.  Older bootstrap compilers know the dir() runtime function
'' but do not necessarily expose fbDirectory as a built-in name.
''
private const FBC_DIR_ATTR_DIRECTORY = &h10

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
'' Compiler-driver options
''

private function fbcDarwinPlatformGetSdkRoot( ) as string
	dim as string sdkroot = environ( "SDKROOT" )

	if( len( sdkroot ) > 0 ) then
		function = sdkroot
		exit function
	end if

	''
	'' Modern macOS systems usually keep startup objects such as crt1.10.5.o
	'' inside the Command Line Tools SDK instead of a default linker search
	'' directory.  Query xcrun here so an in-tree compiler behaves like the
	'' packaged wrapper even when SDKROOT was not exported by the shell.
	''
	sdkroot = hGet1stOutputLineFromCommand( "xcrun --sdk macosx --show-sdk-path 2>/dev/null" )
	if( len( sdkroot ) = 0 ) then
		exit function
	end if

	if( hFileExists( sdkroot + FB_HOST_PATHDIV + "usr" + FB_HOST_PATHDIV + _
		"lib" + FB_HOST_PATHDIV + "crt1.o" ) = FALSE ) then
		exit function
	end if

	function = sdkroot
end function

private sub fbcDarwinPlatformAddSdkRoot( byref ldcline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	if( len( fbc.sysroot ) > 0 ) then
		''
		'' An explicit -sysroot from the user is already emitted by the shared
		'' linker path.  Do not also inject the package/default SDK.
		''
		exit sub
	end if

	dim as string sdkroot = fbcDarwinPlatformGetSdkRoot( )
	if( len( sdkroot ) = 0 ) then
		exit sub
	end if

	''
	'' Packaged macOS builds carry an SDK because Homebrew GCC does not know
	'' where Apple's .tbd stubs live.  The SDK is only a compile/link-time
	'' search root; linked binaries still record normal /usr/lib and /System
	'' framework install names.
	''
	ldcline += " -Wl,-syslibroot," + QUOTE + sdkroot + QUOTE
end sub

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

private sub fbcDarwinPlatformAddCompilerDriverLinkerOptions( byref ldcline as string )
	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcDarwinPlatformAddSdkRoot( ldcline )

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

private function fbcDarwinPlatformXmlEscape( byref text as string ) as string
	dim as string result

	for i as integer = 1 to len( text )
		select case mid( text, i, 1 )
		case "&"
			result += "&amp;"
		case "<"
			result += "&lt;"
		case ">"
			result += "&gt;"
		case chr( 34 )
			result += "&quot;"
		case "'"
			result += "&apos;"
		case else
			result += mid( text, i, 1 )
		end select
	next

	function = result
end function

private function fbcDarwinPlatformBundleIdComponent( byref appname as string ) as string
	dim as string result

	for i as integer = 1 to len( appname )
		dim as integer ch = asc( mid( appname, i, 1 ) )

		if( ((ch >= asc( "a" )) and (ch <= asc( "z" ))) or _
		    ((ch >= asc( "A" )) and (ch <= asc( "Z" ))) or _
		    ((ch >= asc( "0" )) and (ch <= asc( "9" ))) ) then
			result += lcase( chr( ch ) )
		else
			if( (len( result ) > 0) andalso (right( result, 1 ) <> "-") ) then
				result += "-"
			end if
		end if
	next

	while( (len( result ) > 0) andalso (right( result, 1 ) = "-") )
		result = left( result, len( result ) - 1 )
	wend

	if( len( result ) = 0 ) then
		result = "program"
	end if

	function = result
end function

private function fbcDarwinPlatformEnsureDirectory( byref path as string ) as integer
	if( mkdir( path ) = 0 ) then
		return TRUE
	end if

	if( len( dir( path, FBC_DIR_ATTR_DIRECTORY ) ) > 0 ) then
		return TRUE
	end if

	errReportEx( FB_ERRMSG_FILEACCESSERROR, path, -1 )
	function = FALSE
end function

private function fbcDarwinPlatformCopyExecutable _
	( _
		byref source_path as string, _
		byref destination_path as string _
	) as integer

	const COPY_BLOCK_SIZE = 32768

	dim as integer source_file = freefile( )
	if( open( source_path, for binary, access read, as #source_file ) <> 0 ) then
		errReportEx( FB_ERRMSG_FILENOTFOUND, source_path, -1 )
		return FALSE
	end if

	if( kill( destination_path ) <> 0 ) then
	end if

	dim as integer destination_file = freefile( )
	if( open( destination_path, for binary, access write, as #destination_file ) <> 0 ) then
		close #source_file
		errReportEx( FB_ERRMSG_FILEACCESSERROR, destination_path, -1 )
		return FALSE
	end if

	dim as ulongint remaining = lof( source_file )

	while( remaining > 0 )
		dim as integer chunk_size = COPY_BLOCK_SIZE

		if( remaining < COPY_BLOCK_SIZE ) then
			chunk_size = cint( remaining )
		end if

		dim as string buffer = space( chunk_size )

		get #source_file, , buffer
		if( err( ) <> 0 ) then
			close #destination_file
			close #source_file
			errReportEx( FB_ERRMSG_FILEACCESSERROR, source_path, -1 )
			return FALSE
		end if

		put #destination_file, , buffer
		if( err( ) <> 0 ) then
			close #destination_file
			close #source_file
			errReportEx( FB_ERRMSG_FILEACCESSERROR, destination_path, -1 )
			return FALSE
		end if

		remaining -= chunk_size
	wend

	close #destination_file
	close #source_file

#if defined( __FB_UNIX__ )
	if( exec( "chmod", "+x " + QUOTE + destination_path + QUOTE ) <> 0 ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, destination_path, -1 )
		return FALSE
	end if
#endif

	function = TRUE
end function

private function fbcDarwinPlatformWriteInfoPlist _
	( _
		byref plist_path as string, _
		byref appname as string, _
		byref executable_name as string _
	) as integer

	dim as integer f = freefile( )

	if( open( plist_path, for output, as #f ) <> 0 ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, plist_path, -1 )
		return FALSE
	end if

	dim as string escaped_appname = fbcDarwinPlatformXmlEscape( appname )
	dim as string escaped_executable = fbcDarwinPlatformXmlEscape( executable_name )
	dim as string bundle_id = "org.freebasic." + fbcDarwinPlatformBundleIdComponent( appname )

	print #f, "<?xml version=""1.0"" encoding=""UTF-8""?>"
	print #f, "<!DOCTYPE plist PUBLIC ""-//Apple//DTD PLIST 1.0//EN"" ""http://www.apple.com/DTDs/PropertyList-1.0.dtd"">"
	print #f, "<plist version=""1.0"">"
	print #f, "<dict>"
	print #f, "    <key>CFBundleDevelopmentRegion</key>"
	print #f, "    <string>en</string>"
	print #f, "    <key>CFBundleExecutable</key>"
	print #f, "    <string>" + escaped_executable + "</string>"
	print #f, "    <key>CFBundleIdentifier</key>"
	print #f, "    <string>" + bundle_id + "</string>"
	print #f, "    <key>CFBundleName</key>"
	print #f, "    <string>" + escaped_appname + "</string>"
	print #f, "    <key>CFBundlePackageType</key>"
	print #f, "    <string>APPL</string>"
	print #f, "    <key>CFBundleSignature</key>"
	print #f, "    <string>????</string>"
	print #f, "    <key>NSHighResolutionCapable</key>"
	print #f, "    <true/>"
	print #f, "</dict>"
	print #f, "</plist>"

	close #f

	function = TRUE
end function

private function fbcDarwinPlatformWritePkgInfo( byref pkginfo_path as string ) as integer
	dim as integer f = freefile( )

	if( open( pkginfo_path, for output, as #f ) <> 0 ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, pkginfo_path, -1 )
		return FALSE
	end if

	print #f, "APPL????";
	close #f

	function = TRUE
end function

private function fbcDarwinPlatformBuildGuiAppBundle( ) as integer
	function = TRUE

	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit function
	end if

	if( fbGetOption( FB_COMPOPT_OUTTYPE ) <> FB_OUTTYPE_EXECUTABLE ) then
		exit function
	end if

	if( fbGetOption( FB_COMPOPT_MODEVIEW ) <> FB_MODEVIEW_GUI ) then
		exit function
	end if

	''
	'' A plain Mach-O executable has no equivalent to the Windows GUI
	'' subsystem flag.  Finder launches bare executables through Terminal,
	'' regardless of what the program does after it starts.
	''
	'' The macOS launch contract is the .app bundle.  Keep the linked
	'' executable where fbc normally puts it so command-line workflows still
	'' work, then copy it into a small bundle next to it for GUI launching.
	''
	dim as string executable_name = hStripPath( fbc.outname )
	dim as string appname = hStripExt( executable_name )

	if( len( appname ) = 0 ) then
		appname = executable_name
	end if

	dim as string bundle_path = hStripExt( fbc.outname ) + ".app"
	dim as string contents_path = bundle_path + FB_HOST_PATHDIV + "Contents"
	dim as string macos_path = contents_path + FB_HOST_PATHDIV + "MacOS"
	dim as string resources_path = contents_path + FB_HOST_PATHDIV + "Resources"
	dim as string plist_path = contents_path + FB_HOST_PATHDIV + "Info.plist"
	dim as string pkginfo_path = contents_path + FB_HOST_PATHDIV + "PkgInfo"
	dim as string bundled_executable = macos_path + FB_HOST_PATHDIV + executable_name

	if( fbc.verbose ) then
		print "creating app bundle: ", bundle_path
	end if

	if( fbcDarwinPlatformEnsureDirectory( bundle_path ) = FALSE ) then
		return FALSE
	end if

	if( fbcDarwinPlatformEnsureDirectory( contents_path ) = FALSE ) then
		return FALSE
	end if

	if( fbcDarwinPlatformEnsureDirectory( macos_path ) = FALSE ) then
		return FALSE
	end if

	if( fbcDarwinPlatformEnsureDirectory( resources_path ) = FALSE ) then
		return FALSE
	end if

	if( fbcDarwinPlatformCopyExecutable( fbc.outname, bundled_executable ) = FALSE ) then
		return FALSE
	end if

	if( fbcDarwinPlatformWriteInfoPlist( plist_path, appname, executable_name ) = FALSE ) then
		return FALSE
	end if

	if( fbcDarwinPlatformWritePkgInfo( pkginfo_path ) = FALSE ) then
		return FALSE
	end if
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

private function fbcDarwinPlatformGetFrameworkName _
	( _
		byref libname as string _
	) as string

	if( fbcDarwinPlatformIsSelected( ) = FALSE ) then
		exit function
	end if

	''
	'' FreeBASIC bindings record link dependencies with #inclib.  Apple ships
	'' these system APIs as frameworks instead of Unix-style lib*.dylib files.
	'' Keep the translation here so each binding can name the Apple framework
	'' while the shared linker path still owns the order of all dependencies.
	''
	select case libname
	case "OpenGL", "GLUT", "OpenAL", "Cocoa"
		function = libname
	end select
end function

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
