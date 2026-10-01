'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-files.bas
''
'' Purpose:
''
''     Manage invocation paths, output names, and temporary files.
''
'' Responsibilities:
''
''     - choose compiler, include, library, and output paths
''     - keep temporary files unique and record their cleanup
''     - write host-compatible linker response files
''
'' This file intentionally does NOT contain:
''
''     - language parsing or linker option policy
''

#include once "driver/fbc-private.bi"

#ifdef __FB_WIN32__
extern "windows"
	declare function GetCurrentProcessId( ) as ulong
end extern
#else
extern "c"
	declare function getpid( ) as long
end extern
#endif

'' -------------------------------------------------------------------------
'' Output names and cleanup records
'' -------------------------------------------------------------------------

sub fbcDriverSetOutName( )
	'' Determine the output binary/archive's name if not given via -x
	if( len( fbc.outname ) > 0 ) then
		exit sub
	end if

	'' Creating a static lib?
	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_STATICLIB ) then
		fbc.outname = hStripFilename( fbc.mainname ) + _
			"lib" + hStripPath( fbc.mainname ) + ".a"
		exit sub
	end if

	'' Otherwise, we're creating an .exe or DLL/shared lib
	fbc.outname = fbc.mainname

	select case( fbGetOption( FB_COMPOPT_OUTTYPE ) )
	case FB_OUTTYPE_EXECUTABLE
		select case( fbGetOption( FB_COMPOPT_TARGET ) )
		case FB_COMPTARGET_DOS, FB_COMPTARGET_CYGWIN, _
			FB_COMPTARGET_WIN32, FB_COMPTARGET_XBOX
			'' Note: XBox target creates an .exe first,
			'' then uses cxbe to turn it into an .xbe later
			fbc.outname += ".exe"
		case FB_COMPTARGET_JS
			fbc.outname += ".html"
		case FB_COMPTARGET_WII
			fbc.outname += ".dol"
		end select
	case FB_OUTTYPE_DYNAMICLIB
		select case( fbGetOption( FB_COMPOPT_TARGET ) )
		case FB_COMPTARGET_CYGWIN, FB_COMPTARGET_WIN32
			fbc.outname += ".dll"
		case FB_COMPTARGET_LINUX, FB_COMPTARGET_DARWIN, _
		     FB_COMPTARGET_FREEBSD, FB_COMPTARGET_OPENBSD, _
		     FB_COMPTARGET_NETBSD, FB_COMPTARGET_DRAGONFLY, _
		     FB_COMPTARGET_SOLARIS, FB_COMPTARGET_ILLUMOS, _
		     FB_COMPTARGET_ANDROID
			fbc.outname = hStripFilename( fbc.outname ) + _
				"lib" + hStripPath( fbc.outname ) + ".so"
		case FB_COMPTARGET_DOS
			fbc.outname += ".dxe"
		end select
	end select
end sub

sub fbcAddTemp(byref file as string)
	strsetAdd(@fbc.temps, file, 0)
end sub

sub fbcRemoveTemp(byref file as string)
	strsetDel(@fbc.temps, file)
end sub

function fbcAddObj( byref file as string ) as string ptr
	'' .o's should be linked/archived in the order they were found on
	'' command line, so callers of this function must take care to preserve
	'' the order...
	dim as string ptr s = listNewNode( @fbc.objlist )
	*s = file
	function = s
end function

'' -------------------------------------------------------------------------
'' Invocation-private temporary paths
'' -------------------------------------------------------------------------

function fbcDriverGetTempFileTag( ) as string
	static as string tag

	if( len( tag ) = 0 ) then
		dim as uinteger processid
		#ifdef __FB_WIN32__
			processid = GetCurrentProcessId( )
		#else
			processid = culng( getpid( ) )
		#endif

		''
		'' Intermediate and response files are deleted automatically unless
		'' the user asked to keep them.  The process id prevents simultaneous
		'' compiler invocations started during the same timer tick from choosing
		'' the same names.  The timer prevents a later process which reuses an id
		'' from selecting a stale file left by an interrupted compilation.
		''
		tag = ".fbc-" + hex( processid, 8 ) + "-" + _
			hex( cuint( timer( ) * 1000.0 ), 8 )
	end if

	function = tag
end function

#ifdef __FB_DOS__
function fbcDriverGetDosTempFileStem _
	( _
		byref reference as string, _
		byref key as string _
	) as string

	''
	'' Plain DOS does not necessarily provide long filename services.  Keep
	'' compiler-owned files within the 8.3 limit while retaining the full path
	'' and per-invocation tag as inputs to the name.
	''
	dim as string hashinput = reference + key + fbcDriverGetTempFileTag( )
	function = hStripFilename( reference ) + _
		hex( hashHash( strptr( hashinput ) ), 8 )
end function
#endif

function fbcDriverCreateFbctinfDirectory( ) as integer
	if( len( fbc.fbctinfdir ) > 0 ) then
		return TRUE
	end if

	'' GNU ar stores the basename of a path as the archive member name.  A
	'' private directory therefore isolates concurrent compilers while keeping
	'' the required __fb_ct.inf member spelling intact.  Keep using the current
	'' directory, where the historical fixed-name metadata was created.
	#ifndef __FB_DOS__
		dim as string candidatebase = fbcDriverGetTempFileTag( ) + "-ct"
	#endif
	dim as string candidate
	for attempt as integer = 0 to 255
		#ifdef __FB_DOS__
			dim as string key = "fbctinf" + ltrim( str( attempt ) )
			candidate = hStripPath( _
				fbcDriverGetDosTempFileStem( fbc.outname, key ) _
			)
		#else
			candidate = candidatebase
			if( attempt > 0 ) then
				candidate = candidatebase + "-" + ltrim( str( attempt ) )
			end if
		#endif

		if( mkdir( candidate ) = 0 ) then
			fbc.fbctinfdir = candidate
			return TRUE
		end if
	next

	errReportEx( FB_ERRMSG_FILEACCESSERROR, candidate, -1 )
	function = FALSE
end function

private sub hUseTemporaryObjfile _
	( _
		byval module as FBCIOFILE ptr, _
		byval sequence as integer _
	)

	if( module->is_custom_objfile ) then
		exit sub
	end if

	dim as string extension = hGetFileExt( *module->objfile )
	#ifdef __FB_DOS__
		*module->objfile = fbcDriverGetDosTempFileStem( _
			*module->objfile, "object" + ltrim( str( sequence ) ) _
		) + "." + extension
	#else
		*module->objfile = hStripExt( *module->objfile ) + _
			fbcDriverGetTempFileTag( ) + "." + extension
	#endif
end sub

sub fbcDriverUseTemporaryObjfiles( )
	'' -c, -C and final assembly output expose the traditional object-derived
	'' names to the caller.  Only objects which are internal to a link/archive
	'' operation can be renamed without changing the command-line contract.
	if( fbc.keepobj or fbc.keepfinalasm or fbc.emitfinalasmonly or _
		fbc.emitasmonly or _
		(fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_OBJECT) ) then
		exit sub
	end if

	dim as integer sequence = 0
	dim as FBCIOFILE ptr module = listGetHead( @fbc.modules )
	while( module )
		hUseTemporaryObjfile( module, sequence )
		sequence += 1
		module = listGetNext( module )
	wend

	dim as FBCIOFILE ptr rc = listGetHead( @fbc.rcs )
	while( rc )
		hUseTemporaryObjfile( rc, sequence )
		sequence += 1
		rc = listGetNext( rc )
	wend

	if( len( fbc.xpm.srcfile ) > 0 ) then
		hUseTemporaryObjfile( @fbc.xpm, sequence )
	end if
end sub

#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
function fbcDriverPutLdArgsIntoFile( byref ldcline as string ) as integer
	dim as string argsfile, ln
	dim as integer f = any

#ifdef __FB_DOS__
	'' The DOS stem hashes the output, command, and per-invocation PID/timer tag.
	'' fblint: disable-next-line FBL760
	argsfile = fbcDriverGetDosTempFileStem( fbc.outname, ldcline ) + ".tmp"
#else
	'' The per-invocation PID/timer tag separates concurrent and stale files.
	'' fblint: disable-next-line FBL760
	argsfile = hStripFilename( fbc.outname ) + _
		"ldopt" + fbcDriverGetTempFileTag( ) + ".tmp" '' fblint: disable-line FBL760
#endif

	f = freefile( )
	if( open( argsfile, for output, as #f ) ) then
		exit function
	end if

	''
	'' MinGW ld (including the MinGW-to-DJGPP cross-compiling ld) treats \
	'' backslashes in @files (response files) as escape sequence, so \ must
	'' be escaped as \\. ld seems to behave pretty much like Unixish shells
	'' would: all \'s indicate an escape sequence. (For reference,
	'' binutils/libiberty source code: expandargv(), buildargv())
	''
	'' With DJGPP ld however, \ chars do not seem to indicate escape
	'' sequences, despite the DJGPP FAQ (http://www.delorie.com/djgpp/v2faq/faq16_3.html)
	'' which says that \ is special in some cases such as \" or \\.
	'' Thus we mustn't (and don't need to) use \\ when using DJGPP ld.
	''
	ln = ldcline
	#ifdef __FB_WIN32__
		ln = hReplace( ln, $"\", $"\\" )
	#endif

	print #f, ln

	close #f

	'' Clean up the @file if -R wasn't given
	if( fbc.keepasm = FALSE ) then
		fbcAddTemp( argsfile )
	end if

	if( fbc.verbose ) then
		print "ld options in '" & argsfile & "': ", ldcline
	end if

	ldcline = "@" + argsfile
	function = TRUE
end function
#endif

'' -------------------------------------------------------------------------
'' Compiler installation paths
'' -------------------------------------------------------------------------

sub fbcDeterminePrefix( )
	'' Not already set from -prefix command line option?
	if( len( fbc.prefix ) = 0 ) then
		'' Then default to exepath() or the hard-coded prefix
		#ifdef ENABLE_PREFIX
			fbc.prefix = ENABLE_PREFIX + FB_HOST_PATHDIV
		#else
			fbc.prefix = pathStripDiv( exepath( ) ) + FB_HOST_PATHDIV
			#ifndef ENABLE_STANDALONE
				'' Non-standalone fbc is in prefix/bin,
				'' just add '..' to get to prefix
				fbc.prefix += ".." + FB_HOST_PATHDIV
			#endif
		#endif
	else
		fbc.prefix = pathStripDiv( fbc.prefix ) + FB_HOST_PATHDIV
	end if

	fbc.prefix = pathStripDiv( pathNormalizeHost( fbc.prefix ) ) + FB_HOST_PATHDIV
end sub

sub fbcSetupCompilerPaths( )
	''
	'' Standalone (classic FB):
	''
	''    bin/os[-arch]/
	''    inc/
	''    lib/os[-arch]/
	''
	'' Normal (unix-style):
	''
	''    bin/[target-]
	''    include/freebasic[suffix]/
	''    lib/freebasic[suffix]/{target | os[-arch]}/
	''
	'' x86 standalone traditionally uses the win32/dos/linux subdirs in bin/
	'' and lib/, named after the target OS. For other architectures, the
	'' arch name needs to be added to distinguish the subdir from the x86
	'' version. (especially for cross-compiling)
	''
	'' Normal has additional support for gcc targets (e.g. i686-pc-mingw32),
	'' which have to be prefixed to the executable names of cross-compiling
	'' tools in the bin/ directory (e.g. bin/i686-pc-mingw32-ld). However,
	'' for native compilation, no target is prefixed to bin/ tools at all.
	''
	'' Normal uses include/freebasic/ and lib/freebasic/ to hold FB includes
	'' and libraries, to stay out of the way of the C ones in include/ and
	'' lib/ and to conform to Linux distro packaging standards.
	''
	'' With ENABLE_LIB64, we put 64bit libs into
	''    lib64/freebasic/<target>/
	'' instead of the default
	''    lib/freebasic/<target>/
	'' lib/ is then used for 32bit libs only. This is useful for some Linux
	'' distros which use lib/ and lib64/ this way.
	''
	'' - The paths are not terminated with [back]slashes here,
	''   except for the bin/ path. fbcFindBin() expects to only have to
	''   append the file name, for example:
	''     "prefix/bin/win32/" + "as.exe"
	''     "prefix/bin/" + "ld"
	''     "prefix/bin/i686-w64-mingw32-" + "ld"
	''
	''
	'' NOTE: Android armeabi and armeabi-v7a ABIs have the same FB target id,
	'' arm-android.  We could have a separate target id called armv7a-android,
	'' but the improvements to performance of libfb would be pretty minor
	'' anyway (there is very little use of floating point), so it's simpler to
	'' just use libraries built for armeabi for both abis. libfbgfx and other
	'' libraries would be a different matter, so in future maybe change this.

	dim as string targetid = fbGetTargetId( )

	#if defined( __FB_OPENBSD__ )
		#ifndef ENABLE_STANDALONE
			if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_OPENBSD) and _
				(len( fbc.targetprefix ) = 0) and _
				(len( fbc.buildprefix ) = 0) ) then
				fbctoolTB(FBCTOOL_GCC).name = "egcc"
			end if
		#endif
	#endif

#ifdef ENABLE_STANDALONE
	'' Use default target name
	fbc.binpath = fbc.prefix + "bin" + FB_HOST_PATHDIV + targetid + FB_HOST_PATHDIV
	fbc.incpath = fbc.prefix + "inc"
	fbc.libpath = fbc.prefix + "lib" + FB_HOST_PATHDIV + targetid
#else
	dim as string fbname
	#ifdef __FB_DOS__
		'' Our subdirectory in include/ and lib/ is usually called
		'' freebasic/, but on DOS that's too long... of course almost
		'' no targetid or suffix can be used either.
		fbname = "freebas"
	#else
		fbname = "freebasic"
	#endif
	#ifdef ENABLE_SUFFIX
		fbname += ENABLE_SUFFIX
	#endif

	dim libdirname as string = "lib"
	#ifdef ENABLE_LIB64
		if( fbIs64Bit( ) ) then
			libdirname = "lib64"
		end if
	#endif

	if( len(fbc.buildprefix) > 0 ) then
		fbc.binpath = fbc.prefix + "bin"     + FB_HOST_PATHDIV + fbc.buildprefix
	else
		fbc.binpath = fbc.prefix + "bin"     + FB_HOST_PATHDIV + fbc.targetprefix
	end if

	#if defined( __FB_HAIKU__ )
		if( (fbIs64Bit( ) = FALSE) and _
			(fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_HAIKU) and _
			(len( fbc.buildprefix ) = 0) and _
			(len( fbc.targetprefix ) = 0) ) then
			dim as string secondarybin = fbc.prefix + "develop" + _
				FB_HOST_PATHDIV + "tools" + FB_HOST_PATHDIV + _
				"x86" + FB_HOST_PATHDIV + "bin" + FB_HOST_PATHDIV
			if( hFileExists( secondarybin + fbctoolTB(FBCTOOL_LD).name + FB_HOST_EXEEXT ) ) then
				fbc.binpath = secondarybin
			end if
		end if
	#endif

	''
	'' Haiku keeps development headers in /develop/headers instead of the
	'' more typical /include tree used by the other hosted Unix targets.
	''
	'' Keep the compiler's built-in default include path aligned with the
	'' install layout from mk/layout.mk, otherwise installed core headers
	'' such as fbgfx.bi, crt.bi, vbcompat.bi, and fbthread.bi appear to be
	'' missing even though the package installed them correctly.
	''
	#ifdef __FB_HAIKU__
		fbc.incpath = fbc.prefix + "develop" + FB_HOST_PATHDIV + "headers" + FB_HOST_PATHDIV + fbname
	#else
		fbc.incpath = fbc.prefix + "include" + FB_HOST_PATHDIV + fbname
	#endif
	fbc.libpath = fbc.prefix + libdirname + FB_HOST_PATHDIV + fbname + FB_HOST_PATHDIV + targetid
#endif
end sub

sub fbcPrintTargetInfo( )
	var s = fbGetTargetId( )
	s += ", " + *fbGetFbcArch( )
	s += ", " & fbGetBits( ) & "bit"
	#ifndef ENABLE_STANDALONE
		if( len( fbc.target ) > 0 ) then
			s += " (" + fbc.target + ")"
		end if
	#endif
	print "target:", s
	print "backend:", fbGetBackendName( fbGetOption( FB_COMPOPT_BACKEND ) )
end sub

sub fbcDetermineMainName( )
	'' Determine the main module path/name if not given via -m
	if (len(fbc.mainname) = 0) then
		'' 1) First input .bas module
		dim as FBCIOFILE ptr m = listGetHead( @fbc.modules )
		if( m ) then
			fbc.mainname = m->srcfile
		else
			'' 2) First input .o
			dim as string ptr objf = listGetHead( @fbc.objlist )
			if( objf <> NULL ) then
				fbc.mainname = *objf
			else
				'' 3) Neither input .bas nor .o, that is rare,
				'' but happens in this case:
				''      $ fbc a.bas -lib -m a
				''      $ fbc b.bas -lib
				''      $ fbc -l a -l b
				'' Usually -x is used too though, so this
				'' fallback name won't be seen often.
				'' This name should be 8.3 compatible (for DOS)
				fbc.mainname = "unnamed"
			end if
		end if
		fbc.mainname = hStripExt(fbc.mainname)
	end if
end sub

'' end of driver/fbc-files.bas
