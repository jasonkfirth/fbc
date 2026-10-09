'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-tools.bas
''
'' Purpose:
''
''     Find and execute the external tools used by a compiler invocation.
''
'' Responsibilities:
''
''     - own the tool catalog and cached executable paths
''     - query compiler toolchains and resolve library files
''     - honor host execution rules and environment overrides
''
'' This file intentionally does NOT contain:
''
''     - command-line option parsing or backend emission
''

#include once "driver/fbc-private.bi"

'' must be same order as enum FBCTOOL
'' -------------------------------------------------------------------------
'' Tool catalog
'' -------------------------------------------------------------------------

dim shared as FBCTOOLINFO fbctoolTB(0 to FBCTOOL__COUNT-1) = _
{ _
	/' FBCTOOL_NONE    '/ ( ""       , ""       , FBCTOOLFLAG_INVALID  ), _
	/' FBCTOOL_AS      '/ ( "as"     , "AS"     , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_AR      '/ ( "ar"     , "AR"     , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_LD      '/ ( "ld"     , "LD"     , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_GCC     '/ ( "gcc"    , "GCC"    , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_LLC     '/ ( "llc"    , "LLC"    , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_CLANG   '/ ( "clang"  , "CLANG"  , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_DLLTOOL '/ ( "dlltool", "DLLTOOL", FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_GORC    '/ ( "GoRC"   , "GORC"   , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_WINDRES '/ ( "windres", "WINDRES", FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_CXBE    '/ ( "cxbe"   , "CXBE"   , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_DXEGEN  '/ ( "dxe3gen", "DXEGEN" , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_EMAS    '/ ( "emcc"   , "EMAS"   , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_EMAR    '/ ( "emar"   , "EMAR"   , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_EMLD    '/ ( "emcc"   , "EMLD"   , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_EMCC    '/ ( "emcc"   , "EMCC"   , FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_ELF2DOL '/ ( "elf2dol", "ELF2DOL", FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_ELF2AIF '/ ( "elf2aif", "ELF2AIF", FBCTOOLFLAG_DEFAULT  ), _
	/' FBCTOOL_ELF2HUNK'/ ( "elf2hunk", "ELF2HUNK", FBCTOOLFLAG_DEFAULT )  _
}

'' -------------------------------------------------------------------------
'' Toolchain queries and library discovery
'' -------------------------------------------------------------------------

function fbcDriverGet1stOutputLineFromCommand( byref cmd as string ) as string
	var f = freefile( )
	if( open pipe( cmd, for input, as f ) <> 0 ) then
		exit function
	end if

	dim ln as string
	line input #f, ln

	close f
	return ln
end function

function fbcDriverGetClangTargetOption( ) as string
	var platformoption = fbcPlatformGetClangTargetOption( )
	if( len( platformoption ) > 0 ) then
		return platformoption
	end if

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WIN32 ) then
		'' MSYS2 provides host-runnable Clang binaries. Their default CPU
		'' does not necessarily match the selected MinGW target, especially
		'' when LLVM assembly is produced for a different word size.
		select case fbGetCpuFamily( )
		case FB_CPUFAMILY_X86
			return "--target=i686-w64-mingw32 "
		case FB_CPUFAMILY_X86_64
			return "--target=x86_64-w64-mingw32 "
		case FB_CPUFAMILY_AARCH64
			return "--target=aarch64-w64-mingw32 "
		end select
	end if
	return ""
end function

private sub hAppendTargetCcQueryOptions( byref path as string )
	select case( fbGetCpuFamily( ) )
	case FB_CPUFAMILY_X86
		path += " -m32"
	case FB_CPUFAMILY_X86_64
		path += " -m64"
	case FB_CPUFAMILY_PPC
		path += " -m32"
	case FB_CPUFAMILY_PPC64, FB_CPUFAMILY_PPC64LE
		path += " -m64"
	case FB_CPUFAMILY_MIPS32, FB_CPUFAMILY_MIPS32EL
		path += " -mabi=32"
	case FB_CPUFAMILY_MIPS64, FB_CPUFAMILY_MIPS64EL
		path += " -mabi=64"
	end select

	fbcPlatformAddCcQueryOptions( path )

	if( fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG ) then
		path += " " + fbcDriverGetClangTargetOption( )
	end if
end sub

function fbcUseLldLinker( ) as integer
	'' win32-aarch64 builds from MSYS2 use ld.lld.exe as the practical linker.
	'' The package also provides ld.exe for compatibility, but this is actually
	'' the LLVM driver binary and does not accept all GNU ld linker-option forms
	'' that fbc currently emits.
	'' This is a target toolchain requirement, independent of the source backend.
	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WIN32) and _
		(fbGetCpuFamily( ) = FB_CPUFAMILY_AARCH64) ) then
		return TRUE
	end if

	function = FALSE
end function

'' Pass some arguments to gcc/clang and read the results. Returns an empty string on
'' an error.
function fbcQueryCC( byref options as string ) as string
	dim as string path

	select case( fbGetOption( FB_COMPOPT_BACKEND ) )
	case FB_BACKEND_CLANG
		fbcFindBin( FBCTOOL_CLANG, path )

	'' For gcc backend and all other backends assume we want to query gcc
	case else
		fbcFindBin( FBCTOOL_GCC, path )
	end select

	hAppendTargetCcQueryOptions( path )

	path += options

	dim as integer ff = freefile( )
	if( open pipe( path, for input, as ff ) <> 0 ) then
		exit function
	end if

	dim ret as string
	line input #ff, ret

	close ff

	return ret
end function

#if defined( __FB_HAIKU__ )
private function fbcUseHaikuSecondaryX86Tools( ) as integer
	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_HAIKU) and _
		(fbIs64Bit( ) = FALSE) and _
		(len( fbc.buildprefix ) = 0) and _
		(len( fbc.targetprefix ) = 0) ) then
		function = TRUE
	end if
end function

private function fbcFindHaikuSecondaryX86Tool _
	( _
		byval tool as integer, _
		byref path as string _
	) as integer

	if( fbcUseHaikuSecondaryX86Tools( ) = FALSE ) then
		exit function
	end if

	select case tool
	case FBCTOOL_GCC
		path = fbc.prefix + "develop" + FB_HOST_PATHDIV + _
			"tools" + FB_HOST_PATHDIV + "x86" + FB_HOST_PATHDIV + _
			"bin" + FB_HOST_PATHDIV + "gcc"
		function = hFileExists( path )

	case FBCTOOL_AS
		path = fbcQueryCC( " -print-prog-name=as" )
		function = (len( path ) > 0)

	case FBCTOOL_AR
		path = fbcQueryCC( " -print-prog-name=ar" )
		function = (len( path ) > 0)

	case FBCTOOL_LD
		path = fbcQueryCC( " -print-prog-name=ld" )
		function = (len( path ) > 0)
	end select
end function
#endif

''
'' Build the path to a certain file in our lib/ directory (or, in case of
'' non-standalone, somewhere in a system directory such as /usr/lib).
''
'' standalone: Will always return the path to lib/<target>/<file>, no matter
''             whether it exists or not - because that's where it should be.
''             This way the "file not found" errors will be prettier.
''
'' normal: Will check lib/ and query gcc if not found. Querying gcc may fail,
''         because of that an empty string may be returned.
''
private function fbcFindFreeBsdSystemLib( byval file as zstring ptr ) as string
	if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_FREEBSD ) then
		exit function
	end if

	''
	'' FreeBSD keeps the system startup objects in /usr/lib, but its compiler
	'' driver can return only the input filename for -print-file-name.  That
	'' response means "not found", so try the documented system directory.
	'' Cross-compilers must provide a sysroot to keep the lookup off the host.
	''
	#ifndef __FB_FREEBSD__
		if( len( fbc.sysroot ) = 0 ) then
			exit function
		end if
	#endif

	dim as string found = pathStripDiv( fbc.sysroot )
	found += FB_HOST_PATHDIV + "usr" + FB_HOST_PATHDIV + _
		"lib" + FB_HOST_PATHDIV + *file

	if( hFileExists( found ) ) then
		function = found
	end if
end function

function fbcBuildPathToLibFile( byval file as zstring ptr ) as string
	dim as string found

	''
	'' The Standalone build expects to have all needed files in its lib/,
	'' so it needs to do nothing but build up the path and use that.
	''
	'' Normal however wants to use the "system's" files (and only has few
	'' files in its own lib/).
	''
	'' Typically libgcc.a, crtbegin.o, crtend.o will be inside
	'' gcc's sub-directory in lib/gcc/target/version, i.e. Normal can only
	'' find them via 'gcc -print-file-name=foo' (except for hard-coding
	'' against a specific gcc target/version, but that's not a good option).
	''

	found = fbc.libpath + FB_HOST_PATHDIV + *file

	#ifdef ENABLE_STANDALONE
	'' running a standalone version of fbc to build with android
	'' ndk is not recommended, but if we are, try and query the
	'' sysroot if it wasn't already supplied.  Otherwise
	'' if with are running a standalone fbc, just return the
	'' computed library file name if we didn't explicitly pass
	'' in a -sysroot
	if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_ANDROID ) then
		if( len( fbc.sysroot ) = 0 ) then
			return found
		end if
	end if
	#endif

	'' Does it exist in our lib/?
	if( hFileExists( found ) ) then
		'' Overrides anything else
		return found
	end if

	'' Not found in our lib/, query the target-specific gcc
	dim as string path
	select case( fbGetOption( FB_COMPOPT_BACKEND ) )
	case FB_BACKEND_CLANG
		fbcFindBin( FBCTOOL_CLANG, path )

	'' For gcc backend and all other backends assume we want to query gcc
	case else
		fbcFindBin( FBCTOOL_GCC, path )
	end select

	hAppendTargetCcQueryOptions( path )

	if( len( fbc.sysroot ) ) then
		path += " --sysroot=" + fbc.sysroot
	end if

	path += " -print-file-name=" + *file

	found = fbcDriverGet1stOutputLineFromCommand( path )
	if( len( found ) = 0 ) then
		return fbcFindFreeBsdSystemLib( file )
	end if

	if( found = hStripPath( found ) ) then
		return fbcFindFreeBsdSystemLib( file )
	end if

	function = found
end function

'' gcc may have a default sysroot which we need to pass on to ld. Or it may not know the sysroot.
function fbcFindSysroot( ) as string
	'' Query the target-specific gcc
	dim as string path

	select case( fbGetOption( FB_COMPOPT_BACKEND ) )
	case FB_BACKEND_CLANG
		fbcFindBin( FBCTOOL_CLANG, path )

	'' For gcc backend and all other backends assume we want to query gcc
	case else
		fbcFindBin( FBCTOOL_GCC, path )
	end select

	hAppendTargetCcQueryOptions( path )
	path += " --print-sysroot"
	return fbcDriverGet1stOutputLineFromCommand( path )
end function

'' Retrieve the path to a library file, or an empty string if it can't be found.
function fbcFindLibFile( byval file as zstring ptr ) as string
	dim as string found
	found = fbcBuildPathToLibFile( file )
	if( len( found ) > 0 ) then
		if( hFileExists( found ) = FALSE ) then
			found = ""
		end if
	end if
	function = found
end function

sub fbcAddDefLibPath(byref path as string)
	strsetAdd(@fbc.finallibpaths, path, TRUE)
end sub

#ifndef ENABLE_STANDALONE
sub fbcAddLibPathFor( byval libname as zstring ptr )
	dim as string path
	path = hStripFilename( fbcBuildPathToLibFile( libname ) )
	path = pathStripDiv( path )
	if( len( path ) > 0 ) then
		fbcAddDefLibPath( path )
	end if
end sub
#endif

'' -------------------------------------------------------------------------
'' Executable discovery
'' -------------------------------------------------------------------------

sub fbcFindBin _
	( _
		byval tool as integer, _
		byref path as string _
	)

	'' Re-use path from last time if possible
	if( fbctoolGetFlags( tool, FBCTOOLFLAG_FOUND ) ) then
		path = fbctoolTB( tool ).path
		exit sub
	end if

	fbctoolUnsetFlags( tool, FBCTOOLFLAG_RELYING_ON_SYSTEM )

	'' a) Use the path from the corresponding environment variable if it's set
	if( (fbctoolTB(tool).flags and FBCTOOLFLAG_CAN_USE_ENVIRON) <> 0 ) then
		path = environ( fbctoolTB(tool).env_variable )
	end if

	if( len( path ) = 0 ) then
		#if defined( __FB_HAIKU__ )
			if( fbcFindHaikuSecondaryX86Tool( tool, path ) ) then
				fbctoolTB( tool ).path = path
				fbctoolSetFlags( tool, FBCTOOLFLAG_FOUND )
				exit sub
			end if
		#endif

		'' b) Try bin/ directory
		#ifndef ENABLE_STANDALONE
			'' normal build, the build/target prefix is already appended to binpath
			if( (tool = FBCTOOL_LD) and fbcUseLldLinker( ) ) then
				path = fbc.binpath + "ld.lld" + FB_HOST_EXEEXT
			else
				path = fbc.binpath + fbctoolTB(tool).name + FB_HOST_EXEEXT
			end if
		#else
			'' standalone build, we need to use insert it here
			if( (tool = FBCTOOL_LD) and fbcUseLldLinker( ) ) then
				path = fbc.binpath + fbc.buildprefix + "ld.lld" + FB_HOST_EXEEXT
			else
				path = fbc.binpath + fbc.buildprefix + fbctoolTB(tool).name + FB_HOST_EXEEXT
			end if
		#endif

		#ifndef ENABLE_STANDALONE
			if( hFileExists( path ) = FALSE ) then
				select case fbGetOption( FB_COMPOPT_BACKEND )
				case FB_BACKEND_GCC, FB_BACKEND_CLANG
					'' c) Ask GCC where it is, if applicable (GCC might have its
					'' own copy which we must use instead of the system one)
					if( fbcRiscosHostMayQueryCcForTool( ) ) then
						if( tool = FBCTOOL_AS ) then
							path = fbcQueryCC( " -print-prog-name=as" )
						elseif( tool = FBCTOOL_LD ) then
							path = fbcQueryCC( " -print-prog-name=ld" )
						end if
					end if
				case FB_BACKEND_GAS, FB_BACKEND_GAS64
					#if defined( __FB_FREEBSD__ )
						'' gas backend? and we are looking for the linker? and we're hosted freebsd?
						'' switch to ld.bfd instead...
						if( tool = FBCTOOL_LD ) then
							fbctoolTB(tool).name = "ld.bfd"
							path = fbc.binpath + fbctoolTB(tool).name + FB_HOST_EXEEXT
						end if
					#endif
				end select
			end if

			if( hFileExists( path ) = FALSE ) then
				'' d) Rely on PATH
				if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS ) then
					if( len(fbc.buildprefix) > 0 ) then
						path = fbc.buildprefix + fbctoolTB(tool).name + FB_HOST_EXEEXT
					else
						path = fbc.targetprefix + fbctoolTB(tool).name + FB_HOST_EXEEXT
					end if
				else
					path = fbctoolTB(tool).name
				end if
				fbctoolSetFlags( tool, FBCTOOLFLAG_RELYING_ON_SYSTEM )
			end if
		#endif
	end if

	fbctoolTB( tool ).path = path
	fbctoolSetFlags( tool, FBCTOOLFLAG_FOUND )
end sub

#if defined( __FB_CYGWIN__ )
extern "c"
declare function cygwin_conv_path( byval what as uinteger, byval from_path as const any ptr, byval to_path as any ptr, byval bytes as uinteger ) as integer
end extern

const CCP_POSIX_TO_WIN_A = 0

private function hCygwinExecPath( byref path as string ) as string
	dim as string result

	result = path
	hReplaceSlash( strptr( result ), asc( "/" ) )

	if( left( result, 1 ) = "/" ) then
		dim as integer bytes = cygwin_conv_path( CCP_POSIX_TO_WIN_A, strptr( result ), NULL, 0 )

		if( (bytes > 0) and (bytes < 32768) ) then
			dim as string converted = space( bytes )

			if( cygwin_conv_path( CCP_POSIX_TO_WIN_A, strptr( result ), strptr( converted ), bytes ) = 0 ) then
				dim as integer nulpos = instr( converted, chr( 0 ) )
				if( nulpos > 0 ) then
					converted = left( converted, nulpos - 1 )
				end if
				hReplaceSlash( strptr( converted ), asc( "/" ) )
				return converted
			end if
		end if
	end if

	function = result
end function
#endif

'' -------------------------------------------------------------------------
'' External process execution
'' -------------------------------------------------------------------------

function fbcRunBin _
	( _
		byval action as zstring ptr, _
		byval tool as integer, _
		byref ln as string _
	) as integer

	dim as integer result = any
	dim as string path

	fbcFindBin( tool, path )
#if defined( __FB_CYGWIN__ )
	path = hCygwinExecPath( path )
#endif

	if( fbc.verbose ) then
		print *action + ": ", path + " " + ln
	end if

	dim as integer was_handled
	result = fbcRiscosHostRunTool( tool, path, ln, was_handled )
	if( was_handled = FALSE ) then
		'' Always use exec() on Unix hosts or for standalone because
		'' - Unix exec() searches PATH on those hosts, so shell() isn't needed,
		'' - standalone doesn't use system-wide tools.
		#if defined( __FB_UNIX__ ) or defined( ENABLE_STANDALONE )
			result = exec( path, ln )
		#else
			'' Found at bin/?
			if( fbctoolGetFlags( tool, FBCTOOLFLAG_RELYING_ON_SYSTEM ) = FALSE ) then
				result = exec( path, ln )
			else
				'' System-provided tools must be resolved through the host PATH; Exec()
				'' cannot reproduce that platform-specific command lookup.
				result = shell( path + " " + ln )
			end if
		#endif
	end if

	if( *action = "linking" ) then fbSemanticLinkCompleted(result)
	if( result = 0 ) then
		function = TRUE
	elseif( result < 0 ) then
		errReportEx( FB_ERRMSG_EXEMISSING, path, -1, FB_ERRMSGOPT_ADDCOLON or FB_ERRMSGOPT_ADDQUOTES )
	else
		'' Report bad exit codes only in verbose mode; normally the
		'' program should already have shown an error message, and the
		'' exit code is only interesting for debugging purposes.
		if( fbc.verbose ) then
			print *action + " failed: '" + path + "' terminated with exit code " + str( result )
		end if
	end if
end function

'' end of driver/fbc-tools.bas
