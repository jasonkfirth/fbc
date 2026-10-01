'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-options.bas
''
'' Purpose:
''
''     Parse and validate compiler command-line options.
''
'' Responsibilities:
''
''     - own option identities, spellings, and target alias tables
''     - parse command lines, source directives, and response files
''     - validate combinations before the compilation phases
''
'' This file intentionally does NOT contain:
''
''     - tool execution, backend emission, or help presentation
''

#include once "driver/fbc-private.bi"

'' -------------------------------------------------------------------------
'' Input files and pending option arguments
'' -------------------------------------------------------------------------

private sub hFatalInvalidOption _
	( _
		byref arg as string, _
		byval is_source as integer _
	)
	'' if 'is_source' then show a line number, otherwise it's the actual command line and line number is undefined
	errReportEx( FB_ERRMSG_INVALIDCMDOPTION, QUOTE + arg + QUOTE, iif( is_source, 0, -1 ) )
	fbcEnd( 1 )
end sub

private sub hCheckWaitingObjfile( )
	if( len( fbc.objfile ) > 0 ) then
		errReportEx( FB_ERRMSG_OBJFILEWITHOUTINPUTFILE, "-o " & fbc.objfile, -1 )
		fbc.objfile = ""
	end if
end sub

private sub hSetIofile _
	( _
		byval module as FBCIOFILE ptr, _
		byref srcfile as string, _
		byval is_rc as integer _
	)

	module->srcfile = srcfile

	'' No objfile name set yet (from the -o <file> option)?
	if( len( fbc.objfile ) = 0 ) then
		module->is_custom_objfile = FALSE

		'' Choose default *.o name based on input file name
		if( is_rc ) then
#ifdef ENABLE_GORC
			'' GoRC only accepts *.obj
			'' foo.rc -> foo.obj, so there is no collision with foo.bas' foo.o
			fbc.objfile = hStripExt( srcfile ) + ".obj"
#else
			'' windres doesn't care, so we use the default *.o
			'' foo.rc -> foo.rc.o to avoid collision with foo.bas' foo.o
			fbc.objfile = srcfile + ".o"
#endif
		else
			'' foo.bas -> foo.o
			fbc.objfile = hStripExt( srcfile ) + ".o"
		end if

		'' Since there was no preceding -o for this module, allow
		'' -o <file> to follow later
		fbc.lastmodule = module
	else
		module->is_custom_objfile = TRUE
	end if

	module->objfile = fbcAddObj( fbc.objfile )
	fbc.objfile = ""

end sub

private sub hAddBas( byref basfile as string )
	hSetIofile( listNewNode( @fbc.modules ), basfile, FALSE )
end sub

'' -------------------------------------------------------------------------
'' Target aliases and GNU triplet parsing
'' -------------------------------------------------------------------------

type FBGNUOSINFO
	gnuid       as zstring ptr  '' Part of GNU triplet identifying a certain OS
	os          as integer      '' Corresponding FB_COMPTARGET_*
end type

type FBGNUARCHINFO
	gnuid       as zstring ptr  '' Part of GNU triplet identifying a certain architecture
	cputype     as integer      '' Corresponding FB_CPUTYPE_*
end type

'' OS name strings recognized when parsing GNU triplets (-target option)
dim shared as FBGNUOSINFO gnuosmap(0 to ...) => _
{ _
	(@"android"    , FB_COMPTARGET_ANDROID  ), _ '' Must appear before linux
	(@"mingw32ce"  , FB_COMPTARGET_WINCE    ), _ '' Must appear before mingw
	(@"nuttx"      , FB_COMPTARGET_NUTTX    ), _
	(@"riscos"     , FB_COMPTARGET_RISCOS   ), _
	(@"aros"       , FB_COMPTARGET_AROS     ), _
	(@"amigaos"    , FB_COMPTARGET_AMIGA    ), _
	(@"amiga"      , FB_COMPTARGET_AMIGA    ), _
	(@"linux"      , FB_COMPTARGET_LINUX    ), _
	(@"haiku"      , FB_COMPTARGET_HAIKU    ), _
	(@"mingw"      , FB_COMPTARGET_WIN32    ), _
	(@"djgpp"      , FB_COMPTARGET_DOS      ), _
	(@"msdosdjgpp" , FB_COMPTARGET_DOS      ), _
	(@"cygwin"     , FB_COMPTARGET_CYGWIN   ), _
	(@"darwin"     , FB_COMPTARGET_DARWIN   ), _
	(@"freebsd"    , FB_COMPTARGET_FREEBSD  ), _
	(@"dragonfly"  , FB_COMPTARGET_DRAGONFLY), _
	(@"solaris"    , FB_COMPTARGET_SOLARIS  ), _
	(@"illumos"    , FB_COMPTARGET_ILLUMOS  ), _
	(@"sunos"      , FB_COMPTARGET_ILLUMOS  ), _
	(@"netbsd"     , FB_COMPTARGET_NETBSD   ), _
	(@"openbsd"    , FB_COMPTARGET_OPENBSD  ), _
	(@"xbox"       , FB_COMPTARGET_XBOX     ), _
	(@"wii"        , FB_COMPTARGET_WII      )  _
}

'' Architectures recognized when parsing GNU triplets (-target option)
dim shared as FBGNUARCHINFO gnuarchmap(0 to ...) => _
{ _
	(@"i386"       , FB_CPUTYPE_386            ), _
	(@"i486"       , FB_CPUTYPE_486            ), _
	(@"i586"       , FB_CPUTYPE_586            ), _
	(@"i686"       , FB_CPUTYPE_686            ), _
	(@"x86"        , FB_DEFAULT_CPUTYPE_X86    ), _
	(@"x86_64"     , FB_DEFAULT_CPUTYPE_X86_64 ), _
	(@"amd64"      , FB_DEFAULT_CPUTYPE_X86_64 ), _
	(@"armv4"      , FB_CPUTYPE_ARMV4          ), _
	(@"armv4l"     , FB_CPUTYPE_ARMV4          ), _
	(@"armv5te"    , FB_CPUTYPE_ARMV5TE        ), _
	(@"armv6"      , FB_CPUTYPE_ARMV6          ), _
	(@"armv6+fp"   , FB_CPUTYPE_ARMV6_FP       ), _
	(@"armv7a"     , FB_CPUTYPE_ARMV7A         ), _
	(@"armv7a+fp"  , FB_CPUTYPE_ARMV7A_FP      ), _
	(@"arm"        , FB_DEFAULT_CPUTYPE_ARM    ), _
	(@"aarch64"    , FB_DEFAULT_CPUTYPE_AARCH64), _
	(@"arm64"      , FB_DEFAULT_CPUTYPE_AARCH64), _
	(@"ppc"        , FB_DEFAULT_CPUTYPE_PPC    ), _
	(@"powerpc"    , FB_DEFAULT_CPUTYPE_PPC    ), _
	(@"ppc64"      , FB_DEFAULT_CPUTYPE_PPC64  ), _
	(@"powerpc64"  , FB_DEFAULT_CPUTYPE_PPC64  ),  _
	(@"ppc64le"    , FB_DEFAULT_CPUTYPE_PPC64LE), _
	(@"powerpc64le", FB_DEFAULT_CPUTYPE_PPC64LE), _
	(@"riscv32"    , FB_DEFAULT_CPUTYPE_RISCV32), _
	(@"rv32"       , FB_DEFAULT_CPUTYPE_RISCV32), _
	(@"rv32imac"   , FB_DEFAULT_CPUTYPE_RISCV32), _
	(@"riscv64"    , FB_DEFAULT_CPUTYPE_RISCV64), _
	(@"rv64"       , FB_DEFAULT_CPUTYPE_RISCV64), _
	(@"rv64gc"     , FB_DEFAULT_CPUTYPE_RISCV64), _
	(@"s390x"      , FB_DEFAULT_CPUTYPE_S390X  ), _
	(@"loongarch64", FB_DEFAULT_CPUTYPE_LOONGARCH64), _
	(@"m68k"       , FB_DEFAULT_CPUTYPE_M68K   ), _
	(@"m68000"     , FB_DEFAULT_CPUTYPE_M68K   ), _
	(@"mips"       , FB_DEFAULT_CPUTYPE_MIPS32 ), _
	(@"mips32"     , FB_DEFAULT_CPUTYPE_MIPS32 ), _
	(@"mipsisa32"  , FB_DEFAULT_CPUTYPE_MIPS32 ), _
	(@"mipsel"     , FB_DEFAULT_CPUTYPE_MIPS32EL), _
	(@"mips32el"   , FB_DEFAULT_CPUTYPE_MIPS32EL), _
	(@"mipsisa32el", FB_DEFAULT_CPUTYPE_MIPS32EL), _
	(@"mips64"     , FB_DEFAULT_CPUTYPE_MIPS64 ), _
	(@"mipsisa64"  , FB_DEFAULT_CPUTYPE_MIPS64 ), _
	(@"mips64el"   , FB_DEFAULT_CPUTYPE_MIPS64EL), _
	(@"mipsisa64el", FB_DEFAULT_CPUTYPE_MIPS64EL)  _
}

'' Identify OS (FB_COMPTARGET_*) and architecture (FB_CPUTYPE_*) in a GNU
'' triplet string (gcc toolchain target name).
private sub hParseGnuTriplet _
	( _
		byref arg as string, _
		byval separator as integer, _
		byref os as integer, _
		byref cputype as integer _
	)
	dim arch as string

	'' Search for OS, it be anywere in the triplet:
	''    mingw32              -> mingw
	''    arm-linux-gnueabihf  -> linux
	''    arm-linux-androideabi-> android
	''    i686-w64-mingw32     -> mingw
	''    i686-pc-linux-gnu    -> linux
	''    i386-pc-msdosdjgpp   -> dos386
	''    i486-pc-msdosdjgpp   -> dos486
	''    i586-pc-msdosdjgpp   -> dos586
	for i as integer = 0 to ubound( gnuosmap )
		if( instr( arg, *gnuosmap(i).gnuid ) > 0 ) then
			os = gnuosmap(i).os
			exit for
		end if
	next

	'' If the triplet has at least two components (<arch>-<...>),
	'' extract the first (the architecture) and try to identify it.
	if( separator > 0 ) then
		arch = left( arg, separator - 1 )
		for i as integer = 0 to ubound( gnuarchmap )
			if( arch = *gnuarchmap(i).gnuid ) then
				cputype = gnuarchmap(i).cputype
				exit for
			end if
		next

		fbcPlatformAdjustParsedCpuType( os, arch, cputype )
	end if

end sub

function fbCpuTypeFromGNUArchInfo( byref arch as string ) as integer
	for i as integer = 0 to ubound( gnuarchmap )
		if( arch = *gnuarchmap(i).gnuid ) then
			return gnuarchmap(i).cputype
		end if
	next
	return -1
end function

type FBOSARCHINFO
	targetid    as zstring ptr  '' -target option argument
	os          as integer      '' FB_COMPTARGET_*
	cputype     as integer      '' FB_CPUTYPE_*
end type

'' Simple free-form arguments accepted by -target option
dim shared as FBOSARCHINFO fbosarchmap(0 to ...) => _
{ _
	_ '' win32/win64 refer to specific OS/arch combinations
	(@"win32"  , FB_COMPTARGET_WIN32  , FB_DEFAULT_CPUTYPE_X86   ), _
	_ '' dos-hx is a 32-bit Win32 PE image for the HX DOS extender
	(@"dos-hx"  , FB_COMPTARGET_WIN32  , FB_DEFAULT_CPUTYPE_X86   ), _
	(@"win64"  , FB_COMPTARGET_WIN32  , FB_DEFAULT_CPUTYPE_X86_64), _
	(@"win32-aarch64", FB_COMPTARGET_WIN32, FB_DEFAULT_CPUTYPE_AARCH64), _
	(@"wince-arm", FB_COMPTARGET_WINCE, FB_CPUTYPE_ARMV4), _
	(@"wince-mips", FB_COMPTARGET_WINCE, FB_DEFAULT_CPUTYPE_MIPS32EL), _
	(@"wince-mipsel", FB_COMPTARGET_WINCE, FB_DEFAULT_CPUTYPE_MIPS32EL), _
	(@"wince-mips32el", FB_COMPTARGET_WINCE, FB_DEFAULT_CPUTYPE_MIPS32EL), _
	_ '' dragonfly is 64 bit only
	(@"dragonfly", FB_COMPTARGET_DRAGONFLY, FB_DEFAULT_CPUTYPE_X86_64), _
	_ '' solaris is 64 bit only
	(@"solaris", FB_COMPTARGET_SOLARIS, FB_DEFAULT_CPUTYPE_X86_64), _
	_ '' illumos is 64 bit only
	(@"illumos", FB_COMPTARGET_ILLUMOS, FB_DEFAULT_CPUTYPE_X86_64), _
	_
	_ '' OS given without arch, using the default arch, except for dos/xbox
	_ ''  which only work with x86, so we can always default to x86 for them.
	_ '' (these are supported for backwards compatibility with x86-only FB)
	_ '' When targetting android assume cross-compiling.
	_ '' armv7a is the default arch for android ndk r11 and later
	(@"dos"    , FB_COMPTARGET_DOS    , FB_DEFAULT_CPUTYPE_X86   ), _
	(@"xbox"   , FB_COMPTARGET_XBOX   , FB_DEFAULT_CPUTYPE_X86   ), _
	(@"cygwin" , FB_COMPTARGET_CYGWIN , FB_DEFAULT_CPUTYPE       ), _
	(@"darwin" , FB_COMPTARGET_DARWIN , FB_DEFAULT_CPUTYPE       ), _
	(@"freebsd", FB_COMPTARGET_FREEBSD, FB_DEFAULT_CPUTYPE       ), _
	(@"linux"  , FB_COMPTARGET_LINUX  , FB_DEFAULT_CPUTYPE       ), _
	(@"haiku"  , FB_COMPTARGET_HAIKU  , FB_DEFAULT_CPUTYPE       ), _
	(@"android", FB_COMPTARGET_ANDROID, FB_CPUTYPE_ARMV7A        ), _
	(@"netbsd" , FB_COMPTARGET_NETBSD , FB_DEFAULT_CPUTYPE       ), _
	(@"openbsd", FB_COMPTARGET_OPENBSD, FB_DEFAULT_CPUTYPE       ), _
	(@"wii"    , FB_COMPTARGET_WII    , FB_DEFAULT_CPUTYPE_PPC   ), _
	(@"nuttx"  , FB_COMPTARGET_NUTTX  , FB_DEFAULT_CPUTYPE_RISCV32), _
	(@"riscos" , FB_COMPTARGET_RISCOS , FB_CPUTYPE_ARMV4         ), _
	(@"aros"   , FB_COMPTARGET_AROS   , FB_DEFAULT_CPUTYPE       ), _
	(@"amiga"  , FB_COMPTARGET_AMIGA  , FB_DEFAULT_CPUTYPE_M68K  ), _
	(@"amigaos", FB_COMPTARGET_AMIGA  , FB_DEFAULT_CPUTYPE_M68K  )  _
}

''
'' Parse the -target option's argument.
''
'' Examples:
''    -target win32           ->    Windows + default x86 arch
''    -target win64           ->    Windows + x86_64
''    -target win32-aarch64   ->    Windows + AArch64
''    -target dos-hx          ->    Win32 PE for HX under DOS + x86
''    -target dos             ->    DOS + x86
''    -target linux           ->    Linux + default arch
''    -target linux-x86       ->    Linux + default x86 arch
''    -target linux-x86_64    ->    Linux + x86_64
''    -target android         ->    Android + ARMv7a (the default ARM)
''    ...
''
'' The normal (non-standalone) build also accepts GNU triplets:
'' (the rough format is <arch>-<vendor>-<os> but it can vary a lot)
''    -target i686-pc-linux-gnu        ->    Linux + i686
''    -target arm-linux-gnueabihf      ->    Linux + default ARM arch
''    -target arm-android              ->    android-arm, armv7-a, 32bit
''    -target android -arch armv5      ->    android-arm, armv5te, 32bit
''    -target armv5te-linux-android    ->    android-arm, armv5te, 32bit
''    -target android -arch armv7      ->    android-arm, armv7-a, 32bit
''    -target arm-linux-android        ->    android-arm, armv7-a, 32bit
''    -target armv7a-linux-android     ->    android-arm, armv7-a, 32bit
''    -target armv7a-linux-androideabi ->    android-arm, armv7-a, 32bit
''    -target i686-linux-android       ->    android-x86, 686, 32bit
''    -target x86_64-w64-mingw32       ->    Windows + x86_64
''    ...
''
'' The normal build uses the -target argument as prefix for binutils/gcc tools.
'' This allows fbc to work well with gcc/binutils cross-compiling toolchains,
'' for example: -target i686-pc-mingw32 causes it to use i686-pc-mingw32-ld
'' instead of the native ld.
''
'' Something like -target win32 is mostly useful for the standalone build, where
'' binutils/gcc tools are arranged into the bin/win32/ld.exe etc. directory
'' layout. -target win32 is less useful for the normal build, because typically
'' binutils/gcc toolchains for cross-compiling to Windows are named something
'' like "i686-pc-mingw32", not just "win32". Nevertheless, even the normal build
'' should accept these options -- it can be useful for debugging purposes or
'' with the -print option. Furthermore, people (unnecessarily) specify -target
'' for native compilation (e.g. using -target linux on Linux), and this supports
'' that.
''
'' This function should just do parsing, without any validation. It would be
'' nice if
''    -target linux-x86_64
'' would just be the same as
''    -target linux -arch x86_64
'' Thus, any validation should be done later when the command line has been
'' parsed completely.
''
'' It's up to the caller to report an error if only one of OS/arch (but not
'' both) could be identified.
''
private sub hParseTargetArg _
	( _
		byref arg as string, _
		byref os as integer, _
		byref cputype as integer, _
		byref is_gnu_triplet as integer _
	)

	os = -1
	cputype = -1
	is_gnu_triplet = FALSE

	'' Case-insensitive so "-target WIN32" etc. works too
	var lcasearg = lcase( arg )

	'' Check for simple arguments (dos, linux, win32, etc.)
	for i as integer = 0 to ubound( fbosarchmap )
		if( lcasearg = *fbosarchmap(i).targetid ) then
			os = fbosarchmap(i).os
			cputype = fbosarchmap(i).cputype
			exit sub
		end if
	next

	'' <os>-<cpufamily>
	var separator = instr( arg, "-" )
	if( separator > 0 ) then
		var arch = right( lcasearg, len( lcasearg ) - separator )
		os = fbIdentifyOs( left( lcasearg, separator - 1 ) )
		cputype = fbDefaultCpuTypeFromCpuFamilyId( os, arch )

		fbcPlatformAdjustParsedCpuType( os, arch, cputype )

		'' allow normalizing on gnu arch types to determine the standalone targetid
		#ifdef ENABLE_STANDALONE
			if( (os < 0) and (cputype < 0) ) then
				cputype = fbCpuTypeFromGNUArchInfo( arch )
			end if
		#endif
	end if

	'' Normal build: Check for GNU triplets, if the above checks failed.
	#ifndef ENABLE_STANDALONE
		if( (os < 0) and (cputype < 0) ) then
			hParseGnuTriplet( arg, separator, os, cputype )
			is_gnu_triplet = TRUE
		end if
	#else
		if( (os < 0) and (cputype < 0) ) then
			hParseGnuTriplet( arg, separator, os, cputype )
		end if
	#endif
end sub

enum
	OPT_A = 0
	OPT_ARCH
	OPT_ASM
	OPT_B
	OPT_BUILDPREFIX
	OPT_C
	OPT_CKEEPOBJ
	OPT_D
	OPT_DLL
	OPT_DYLIB
	OPT_E
	OPT_EARRAY
	OPT_EARRAYDIMS
	OPT_EASSERT
	OPT_EDEBUG
	OPT_EDEBUGINFO
	OPT_ELOCATION
	OPT_ENULLPTR
	OPT_EUNWIND
	OPT_ENTRY
	OPT_EX
	OPT_EXX
	OPT_EXPORT
	OPT_FBGFX
	OPT_FORCELANG
	OPT_FPMODE
	OPT_FPU
	OPT_G
	OPT_GFX3
	OPT_GEN
	OPT_HELP
	OPT_I
	OPT_INCLUDE
	OPT_L
	OPT_LANG
	OPT_LIB
	OPT_M
	OPT_MAP
	OPT_MAXERR
	OPT_MT
	OPT_DOS_THREADS
	OPT_NODEFLIBS
	OPT_NOERRLINE
	OPT_NOLIB
	OPT_NOOBJINFO
	OPT_NOSTRIP
	OPT_O
	OPT_OPTIMIZE
	OPT_P
	OPT_PIC
	OPT_PP
	OPT_PREFIX
	OPT_PRINT
	OPT_PROFGEN
	OPT_PROFILE
	OPT_R
	OPT_RKEEPASM
	OPT_RR
	OPT_RRKEEPASM
	OPT_S
	OPT_SEMANTIC_MODEL
	OPT_SEMANTIC_EXPRESSIONS
	OPT_SHOWINCLUDES
	OPT_STATIC
	OPT_STRIP
	OPT_SYSROOT
	OPT_T
	OPT_TARGET
	OPT_TITLE
	OPT_V
	OPT_VEC
	OPT_VERSION
	OPT_W
	OPT_WA
	OPT_WC
	OPT_WL
	OPT_X
	OPT_Z
	OPT__COUNT
end enum

type FBC_CMDLINE_OPTION
	takes_argument as boolean          '' true = option requires argument
	allowed_in_source as boolean       '' true = can be used with #cmdline directive
	parser_restart as boolean          '' true = restart of parser is required when used with #cmdline directive
	fbc_restart as integer             '' true = major restart of fbc required
end type

dim shared as FBC_CMDLINE_OPTION cmdlineOptionTB(0 to (OPT__COUNT - 1)) = _
{ _
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_A            add files to link, affects link
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_ARCH         affects major initialization
	( TRUE , TRUE , FALSE, TRUE  ), _ '' OPT_ASM          affects major initialization,affects second stage compile
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_B            adds files to compile
	( TRUE , TRUE , FALSE, TRUE  ), _ '' OPT_BUILDPREFIX  affects tools executed (fbcSetupCompilerPaths(), so restart is required)
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_C            affects code generation / compile / assemble /link process
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_CKEEPOBJ     affects removal of temporary files
	( TRUE , TRUE , FALSE, TRUE  ), _ '' OPT_D            add symbols to current source also, not just the preDefines, affects global defines
	( FALSE, TRUE , TRUE , TRUE  ), _ '' OPT_DLL          affects major initialization, affects output format
	( FALSE, TRUE , TRUE , TRUE  ), _ '' OPT_DYLIB        affects major initialization, affects output format
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_E            affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EARRAY       affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EARRAYDIMS   affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EASSERT      affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EDEBUG       affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EDEBUGINFO   affects code generation, affects link
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_ELOCATION    affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_ENULLPTR     affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EUNWIND      affects code generation
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_ENTRY        affects major initialization, affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EX           affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EXX          affects code generation
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_EXPORT       affects code generation
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_FBGFX        affects link
	( TRUE , TRUE , TRUE , FALSE ), _ '' OPT_FORCELANG    never allow, command line only
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_FPMODE       affects major initialization, affects code generation
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_FPU          affects major initialization,affects code generation, affects second stage compile, affects link
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_G            affects code generation, affects link
	( FALSE, TRUE , FALSE, TRUE  ), _ '' OPT_GFX3         selects gfxlib3 and adds a global define
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_GEN          affects major initialization
	( FALSE, FALSE, FALSE, FALSE ), _ '' OPT_HELP         never allow, real command line only, makes no sense to have in source
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_I            add include path before the default one
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_INCLUDE      restart required to inject preInclude
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_L            affects link, same as #inclib
	( TRUE , TRUE , TRUE , FALSE ), _ '' OPT_LANG         affects code generation, affects initialization
	( FALSE, TRUE , TRUE , TRUE  ), _ '' OPT_LIB          affects major initialization, affects output format
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_M            affects major initialization for all modules
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_MAP          affects output files
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_MAXERR       affects compile process
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_MT           affects link, __FB_MT__
	( TRUE , FALSE, FALSE, FALSE ), _ '' OPT_DOS_THREADS  command-line provider selection
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_NODEFLIBS    affects link
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_NOERRLINE    affects compiler output display
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_NOLIB        affects link
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_NOOBJINFO    affects post compile process
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_NOSTRIP      affects link
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_O            affects output file naming with initialization before compile
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_OPTIMIZE     affects link
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_P            affects link, same as #libpath
	( FALSE, TRUE , FALSE, TRUE  ), _ '' OPT_PIC          affects major initialization, affects link
	( FALSE, TRUE , FALSE, TRUE  ), _ '' OPT_PP           affects major initialization
	( TRUE , TRUE , FALSE, TRUE  ), _ '' OPT_PREFIX       affects major initialization
	( TRUE , FALSE, FALSE, FALSE ), _ '' OPT_PRINT        never allow, makes no sense to have in source
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_PROFGEN      affects major initialization, affects initialization, affects code generation, affects link
	( FALSE, TRUE , TRUE , TRUE  ), _ '' OPT_PROFILE      affects major initialization, affects initialization, affects code generation, affects link
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_R            affects compile / assemble / link process, removal of temporary files
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_RKEEPASM     affects removal of temporary files
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_RR           affects compile / assemble / link process, removal of temporary files
	( FALSE, TRUE , TRUE , FALSE ), _ '' OPT_RRKEEPASM    affects removal of temporary files
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_S            affects link
	( TRUE , FALSE, FALSE, FALSE ), _ '' OPT_SEMANTIC_MODEL compiler-owned semantic model output
	( TRUE , FALSE, FALSE, FALSE ), _ '' OPT_SEMANTIC_EXPRESSIONS expression-only semantic output
	( FALSE, TRUE , FALSE, TRUE  ), _ '' OPT_SHOWINCLUDES affects compiler output display
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_STATIC       affects link
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_STRIP        affects link
	( TRUE,  TRUE , FALSE, FALSE ), _ '' OPT_SYSROOT      affects link
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_T            affects link
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_TARGET       affects major initialization
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_TITLE        affects link
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_V            affects nothing
	( TRUE , TRUE , TRUE , TRUE  ), _ '' OPT_VEC          affects major initialization, affects code generation
	( FALSE, TRUE , FALSE, FALSE ), _ '' OPT_VERSION      print version information
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_W            affects compiler display output
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_WA           affects assembly
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_WC           affects second stage compile
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_WL           affects link
	( TRUE , TRUE , FALSE, FALSE ), _ '' OPT_X            affects output file
	( TRUE , TRUE , TRUE , TRUE  )  _ '' OPT_Z            affects various - code generation
}

'' -------------------------------------------------------------------------
'' Option handlers
'' -------------------------------------------------------------------------

private sub hHandleOptCompileSetup _
	( _
		byval optid as integer, _
		byref arg as string, _
		byval is_source as integer _
	)

	select case as const optid
	case OPT_A
		fbcAddObj( arg )

	case OPT_ARCH
		'' Set cputype later, so it overrides -target.
		fbc.cputype_is_native = (arg = "native")
		fbc.cputype = fbIdentifyFbcArch( arg )
		if( fbc.cputype < 0 ) then
			hFatalInvalidOption( "-arch " + arg, is_source )
		end if

	case OPT_ASM
		select case arg
		case "att"
			fbc.asmsyntax = FB_ASMSYNTAX_ATT
		case "intel"
			fbc.asmsyntax = FB_ASMSYNTAX_INTEL
		case else
			hFatalInvalidOption( arg, is_source )
		end select

	case OPT_B
		hAddBas( arg )

	case OPT_BUILDPREFIX
		fbc.buildprefix = arg

	case OPT_C
		fbSetOption( FB_COMPOPT_OUTTYPE, FB_OUTTYPE_OBJECT )
		fbc.keepobj = TRUE

	case OPT_CKEEPOBJ
		fbc.keepobj = TRUE

	case OPT_D
		fbAddPreDefine( arg )

	case OPT_DLL, OPT_DYLIB
		fbSetOption( FB_COMPOPT_OUTTYPE, FB_OUTTYPE_DYNAMICLIB )

	case OPT_E
		fbSetOption( FB_COMPOPT_ERRORCHECK, TRUE )
		fbSetOption( FB_COMPOPT_UNWINDINFO, TRUE )

	case OPT_EARRAY
		fbSetOption( FB_COMPOPT_ARRAYBOUNDCHECK, TRUE )

	case OPT_EARRAYDIMS
		fbSetOption( FB_COMPOPT_ARRAYDIMSCHECK, TRUE )

	case OPT_EASSERT
		fbSetOption( FB_COMPOPT_ASSERTIONS, TRUE )

	case OPT_EDEBUG
		fbSetOption( FB_COMPOPT_DEBUG, TRUE )

	case OPT_EDEBUGINFO
		fbSetOption( FB_COMPOPT_DEBUGINFO, TRUE )

	case OPT_ELOCATION
		fbSetOption( FB_COMPOPT_ERRLOCATION, TRUE )

	case OPT_ENULLPTR
		fbSetOption( FB_COMPOPT_NULLPTRCHECK, TRUE )

	case OPT_EUNWIND
		fbSetOption( FB_COMPOPT_UNWINDINFO, TRUE )

	case OPT_ENTRY
		fbc.entry = arg

	case OPT_EX
		fbSetOption( FB_COMPOPT_ERRORCHECK, TRUE )
		fbSetOption( FB_COMPOPT_RESUMEERROR, TRUE )
		fbSetOption( FB_COMPOPT_UNWINDINFO, TRUE )

	case OPT_EXX
		fbSetOption( FB_COMPOPT_ERRORCHECK, TRUE )
		fbSetOption( FB_COMPOPT_RESUMEERROR, TRUE )
		fbSetOption( FB_COMPOPT_EXTRAERRCHECK, TRUE )
		fbSetOption( FB_COMPOPT_ERRLOCATION, TRUE )
		fbSetOption( FB_COMPOPT_ARRAYBOUNDCHECK, TRUE )
		fbSetOption( FB_COMPOPT_ARRAYDIMSCHECK, TRUE )
		fbSetOption( FB_COMPOPT_NULLPTRCHECK, TRUE )
		fbSetOption( FB_COMPOPT_UNWINDINFO, TRUE )

	case OPT_EXPORT
		fbSetOption( FB_COMPOPT_EXPORT, TRUE )

	case OPT_FBGFX
		fbSetOption( FB_COMPOPT_FBGFX, TRUE )

	case OPT_SEMANTIC_MODEL
		fbc.semanticmodel = arg
		fbc.semanticmodel_expressions = FALSE
		'' The parser emits AST line markers independently of debug-info mode.
		'' Keep this tooling option from changing source-visible __FB_ERR__.

	case OPT_SEMANTIC_EXPRESSIONS
		fbc.semanticmodel = arg
		fbc.semanticmodel_expressions = TRUE
		'' The compact mode retains typed source ranges without symbol or AST dumps.

	case OPT_GFX3
		'' The preinclude uses the same empty define spelling as source code,
		'' avoiding a conflicting redefinition when both forms are present.
		fbSetOption( FB_COMPOPT_FBGFX, FB_GFXLIB_GFX3 )
		fbAddPreInclude( "fbgfx3-option.bi" )

	case OPT_FORCELANG
		dim as integer value = fbGetLangId( strptr( arg ) )
		if( value = FB_LANG_INVALID ) then
			hFatalInvalidOption( arg, is_source )
		end if

		if( is_source and fbGetOption( FB_COMPOPT_FORCELANG ) ) then
			errReportWarn( FB_WARNINGMSG_CMDLINEOVERRIDES )
		else
			fbSetOption( FB_COMPOPT_LANG, value )
			fbSetOption( FB_COMPOPT_FORCELANG, TRUE )
			fbc.objinf.lang = value
			if( is_source ) then
				fbSetOption( FB_COMPOPT_RESTART_LANG, value )
			end if
		end if

	case OPT_FPMODE
		dim as integer value = FB_FPMODE_PRECISE
		select case ucase( arg )
		case "PRECISE"
			value = FB_FPMODE_PRECISE
		case "FAST"
			value = FB_FPMODE_FAST
		case else
			hFatalInvalidOption( arg, is_source )
		end select
		fbSetOption( FB_COMPOPT_FPMODE, value )

	case OPT_FPU
		dim as integer value = FB_FPUTYPE_FPU
		select case ucase( arg )
		case "X87", "FPU"
			value = FB_FPUTYPE_FPU
		case "SSE"
			value = FB_FPUTYPE_SSE
		case "NEON"
			value = FB_FPUTYPE_NEON
		case else
			hFatalInvalidOption( arg, is_source )
		end select
		fbSetOption( FB_COMPOPT_FPUTYPE, value )
	end select
end sub

private sub hHandleOptFilesAndOutput _
	( _
		byval optid as integer, _
		byref arg as string, _
		byval is_source as integer _
	)

	select case as const optid
	case OPT_G
		fbSetOption( FB_COMPOPT_DEBUG, TRUE )
		fbSetOption( FB_COMPOPT_DEBUGINFO, TRUE )
		fbSetOption( FB_COMPOPT_ASSERTIONS, TRUE )

	case OPT_GEN
		select case lcase( arg )
		case "gas"
			fbc.backend = FB_BACKEND_GAS
		case "gcc"
			fbc.backend = FB_BACKEND_GCC
		case "clang"
			fbc.backend = FB_BACKEND_CLANG
		case "llvm"
			fbc.backend = FB_BACKEND_LLVM
		case "gas64"
			fbc.backend = FB_BACKEND_GAS64
		case else
			hFatalInvalidOption( arg, is_source )
		end select

	case OPT_HELP
		fbc.showhelp = TRUE

	case OPT_I
		fbAddIncludePath( arg )

	case OPT_INCLUDE
		fbAddPreInclude( arg )

	case OPT_L
		strsetAdd( @fbc.libs, arg, FALSE )

	case OPT_LANG
		dim as integer value = fbGetLangId( strptr( arg ) )
		if( value = FB_LANG_INVALID ) then
			hFatalInvalidOption( arg, is_source )
		end if

		'' A real-command-line -forcelang takes precedence over source
		'' #cmdline language selections.
		if( fbGetOption( FB_COMPOPT_FORCELANG ) = FALSE ) then
			fbSetOption( FB_COMPOPT_LANG, value )
			fbc.objinf.lang = value
			if( is_source ) then
				fbSetOption( FB_COMPOPT_RESTART_LANG, value )
			end if
		end if

	case OPT_LIB
		fbSetOption( FB_COMPOPT_OUTTYPE, FB_OUTTYPE_STATICLIB )

	case OPT_M
		fbc.mainname = arg
		fbc.mainset = TRUE

	case OPT_MAP
		fbc.mapfile = arg

	case OPT_MAXERR
		dim as integer value = FB_ERR_INFINITE
		if( arg <> "inf" ) then
			value = clng( arg )
			if( value <= 0 ) then
				hFatalInvalidOption( arg, is_source )
			end if
		end if
		fbSetOption( FB_COMPOPT_MAXERRORS, value )

	case OPT_DOS_THREADS
		if( lcase( arg ) <> "pdmlwp" ) then
			hFatalInvalidOption( arg, is_source )
		end if
		fbc.dos_threads = TRUE
		fbSetOption( FB_COMPOPT_MULTITHREADED, TRUE )
		fbc.objinf.mt = TRUE

	case OPT_MT
		fbSetOption( FB_COMPOPT_MULTITHREADED, TRUE )
		fbc.objinf.mt = TRUE

	case OPT_NODEFLIBS
		fbc.nodeflibs = TRUE
		fbc.nofbrt0 = TRUE

	case OPT_NOERRLINE
		fbSetOption( FB_COMPOPT_SHOWERROR, FALSE )

	case OPT_NOLIB
		dim libs() as string
		var libcount = hSplitStr( arg, ",", libs() )
		for i as integer = 0 to libcount - 1
			if( len( libs(i) ) > 0 ) then
				strsetAdd( @fbc.excludedlibs, libs(i), 0 /'unused userdata'/ )
			end if
		next

	case OPT_NOOBJINFO
		fbSetOption( FB_COMPOPT_OBJINFO, FALSE )

	case OPT_NOSTRIP
		fbc.stripsymbols = FALSE

	case OPT_O
		'' Bind -o to the last module when possible, otherwise hold it for
		'' the next module encountered by command-line parsing.
		hCheckWaitingObjfile( )
		if( fbc.lastmodule ) then
			*fbc.lastmodule->objfile = arg
			fbc.lastmodule->is_custom_objfile = TRUE
		else
			fbc.objfile = arg
		end if
	end select
end sub

private sub hHandleOptPipeline _
	( _
		byval optid as integer, _
		byref arg as string, _
		byval is_source as integer _
	)

	select case as const optid
	case OPT_OPTIMIZE
		dim as integer value = 0
		if( arg = "max" ) then
			value = 3
		else
			value = clng( arg )
			if( value < 0 ) then
				value = 0
			elseif( value > 3 ) then
				value = 3
			end if
		end if
		fbSetOption( FB_COMPOPT_OPTIMIZELEVEL, value )

	case OPT_P
		strsetAdd( @fbc.libpaths, _
		           pathStripDiv( pathNormalizeHost( arg ) ), FALSE )

	case OPT_PIC
		fbSetOption( FB_COMPOPT_PIC, TRUE )

	case OPT_PP
		'' Preprocessing stops before code generation without changing the
		'' output type selected for the module.
		fbSetOption( FB_COMPOPT_PPONLY, TRUE )
		fbc.emitasmonly = TRUE

	case OPT_PREFIX
		fbc.prefix = pathStripDiv( pathNormalizeHost( arg ) )
		hReplaceSlash( fbc.prefix, asc( FB_HOST_PATHDIV ) )

	case OPT_PRINT
		select case arg
		case "host"     : fbc.print = PRINT_HOST
		case "target"   : fbc.print = PRINT_TARGET
		case "x"        : fbc.print = PRINT_X
		case "fblibdir" : fbc.print = PRINT_FBLIBDIR
		case "sha-1"    : fbc.print = PRINT_SHA1
		case "fork-id"  : fbc.print = PRINT_FORK_ID
		case else
			hFatalInvalidOption( arg, is_source )
		end select

	case OPT_PROFILE
		fbSetOption( FB_COMPOPT_PROFILE, FB_PROFILE_OPT_GMON )

	case OPT_PROFGEN
		select case arg
		case "default", "gmon"
			fbSetOption( FB_COMPOPT_PROFILE, FB_PROFILE_OPT_GMON )
		case "fb"
			fbSetOption( FB_COMPOPT_PROFILE, FB_PROFILE_OPT_CALLS )
		case "cycles"
			fbSetOption( FB_COMPOPT_PROFILE, FB_PROFILE_OPT_CYCLES )
		case else
			hFatalInvalidOption( arg, is_source )
		end select

	case OPT_R
		fbSetOption( FB_COMPOPT_OUTTYPE, FB_OUTTYPE_OBJECT )
		fbc.emitasmonly = TRUE
		fbc.keepasm = TRUE

	case OPT_RKEEPASM
		fbc.keepasm = TRUE

	case OPT_RR
		fbSetOption( FB_COMPOPT_OUTTYPE, FB_OUTTYPE_OBJECT )
		fbc.emitfinalasmonly = TRUE
		fbc.keepfinalasm = TRUE

	case OPT_RRKEEPASM
		fbc.keepfinalasm = TRUE

	case OPT_S
		fbc.subsystem = arg
		select case lcase( arg )
		case "gui", "windows"
			fbSetOption( FB_COMPOPT_MODEVIEW, FB_MODEVIEW_GUI )
		case "console"
			fbSetOption( FB_COMPOPT_MODEVIEW, FB_MODEVIEW_CONSOLE )
		end select

	case OPT_SHOWINCLUDES
		fbSetOption( FB_COMPOPT_SHOWINCLUDES, TRUE )

	case OPT_STATIC
		fbc.staticlink = TRUE

	case OPT_STRIP
		fbc.stripsymbols = TRUE

	case OPT_SYSROOT
		fbc.sysroot = arg

	case OPT_T
		fbSetOption( FB_COMPOPT_STACKSIZE, clng( arg ) * 1024 )
		fbc.stacksize_set = TRUE

	case OPT_TARGET
		dim as integer os, cputype, is_gnu_triplet
		hParseTargetArg( arg, os, cputype, is_gnu_triplet )
		if( (os < 0) or (cputype < 0) ) then
			hFatalInvalidOption( arg, is_source )
		end if

		fbSetOption( FB_COMPOPT_TARGET, os )
		fbSetOption( FB_COMPOPT_CPUTYPE, cputype )

#ifndef ENABLE_STANDALONE
		'' Preserve GNU triplets as tool prefixes for cross compilation.
		if( (os <> FB_DEFAULT_TARGET) or _
		    (cputype <> FB_DEFAULT_CPUTYPE) or _
		    is_gnu_triplet ) then
			'' Friendly platform aliases resolve through their platform module;
			'' GNU triplets remain exact user-supplied tool prefixes.
			var platformprefix = fbcPlatformGetToolPrefix( cputype )
			if( (is_gnu_triplet = FALSE) and (len( platformprefix ) > 0) ) then
				fbc.target = platformprefix
			else
				fbc.target = arg
			end if
			fbc.targetprefix = fbc.target + "-"
		end if
#endif
	end select
end sub

private sub hHandleWarningOption( byref arg as string )
	dim as integer value = FB_WARNINGMSGS_LOWEST_LEVEL - 1

	select case arg
	case "all"
		value = FB_WARNINGMSGS_LOWEST_LEVEL
	case "none"
		value = FB_WARNINGMSGS_HIGHEST_LEVEL + 1
	case "param"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_PARAMMODE )
	case "escape"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_ESCSEQ )
	case "next"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_NEXTVAR )
	case "signedness"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_SIGNEDNESS )
	case "constness"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_CONSTNESS )
		value = FB_WARNINGMSGS_LOWEST_LEVEL
	case "funcptr"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_CASTFUNCPTR )
		value = FB_WARNINGMSGS_LOWEST_LEVEL
	case "suffix"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_SUFFIX )
	case "pedantic"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, FB_PDCHECK_DEFAULT )
		if( value > FB_WARNINGMSGS_DEFAULT_LEVEL ) then
			value = FB_WARNINGMSGS_DEFAULT_LEVEL
		end if
	case "error"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_ERROR )
	case "upcast"
		fbSetOption( FB_COMPOPT_PEDANTICCHK, _
			fbGetOption( FB_COMPOPT_PEDANTICCHK ) or FB_PDCHECK_UPCAST )
	case else
		value = clng( arg )
	end select

	if( value >= FB_WARNINGMSGS_LOWEST_LEVEL ) then
		fbSetOption( FB_COMPOPT_WARNINGLEVEL, value )
	end if
end sub

private sub hHandleOptDiagnostics _
	( _
		byval optid as integer, _
		byref arg as string, _
		byval is_source as integer _
	)

	select case as const optid
	case OPT_TITLE
		fbc.xbe_title = arg

	case OPT_V
		fbc.verbose = TRUE

	case OPT_VEC
		dim as integer value = FB_VECTORIZE_NONE
		select case ucase( arg )
		case "NONE", "0"
			value = FB_VECTORIZE_NONE
		case "1"
			value = FB_VECTORIZE_NORMAL
		case "2"
			value = FB_VECTORIZE_INTRATREE
		case else
			hFatalInvalidOption( arg, is_source )
		end select
		fbSetOption( FB_COMPOPT_VECTORIZE, value )

	case OPT_VERSION
		if( is_source ) then
			if( fbc.showversion = FALSE ) then
				fbcDriverPrintVersion( fbc.verbose )
			end if
		end if
		fbc.showversion = TRUE

	case OPT_W
		hHandleWarningOption( arg )

	case OPT_WA
		fbc.extopt.gas += " " + hReplace( arg, ",", " " ) + " "

	case OPT_WC
		fbc.extopt.gcc += " " + hReplace( arg, ",", " " ) + " "

	case OPT_WL
		fbc.extopt.ld += " " + hReplace( arg, ",", " " ) + " "

	case OPT_X
		fbc.outname = arg

	case OPT_Z
		select case lcase( arg )
		case "gosub-setjmp"
			fbSetOption( FB_COMPOPT_GOSUBSETJMP, TRUE )
		case "valist-as-ptr"
			fbSetOption( FB_COMPOPT_VALISTASPTR, TRUE )
		case "no-thiscall"
			fbSetOption( FB_COMPOPT_NOTHISCALL, TRUE )
		case "no-fastcall"
			fbSetOption( FB_COMPOPT_NOFASTCALL, TRUE )
		case "fbrt"
			fbSetOption( FB_COMPOPT_FBRT, TRUE )
		case "nocmdline"
			fbSetOption( FB_COMPOPT_NOCMDLINE, TRUE )
		case "retinflts"
			fbSetOption( FB_COMPOPT_RETURNINFLTS, TRUE )
		case "nobuiltins"
			fbSetOption( FB_COMPOPT_NOBUILTINS, TRUE )
		case "optabstract"
			fbSetOption( FB_COMPOPT_OPTABSTRACT, TRUE )
		case else
			hFatalInvalidOption( arg, is_source )
		end select
	end select
end sub

'' Command-line options form a closed dispatcher with option-specific parsing.
''
private sub handleOpt _
	( _
		byval optid as integer, _
		byref arg as string, _
		byval is_source as integer _
	)

	select case as const optid
	case OPT_A to OPT_FPU, OPT_GFX3, OPT_SEMANTIC_MODEL, OPT_SEMANTIC_EXPRESSIONS
		hHandleOptCompileSetup( optid, arg, is_source )

	case OPT_G, OPT_GEN to OPT_O
		hHandleOptFilesAndOutput( optid, arg, is_source )

	case OPT_OPTIMIZE to OPT_S, OPT_SHOWINCLUDES to OPT_TARGET
		hHandleOptPipeline( optid, arg, is_source )

	case OPT_TITLE to OPT_Z
		hHandleOptDiagnostics( optid, arg, is_source )

	end select
end sub

'' -------------------------------------------------------------------------
'' Command-line and response file scanning
'' -------------------------------------------------------------------------

private function parseOption(byval opt as zstring ptr) as integer
	#macro CHECK(opttext, optid)
		if (*opt = opttext) then
			return optid
		end if
	#endmacro

	#macro ONECHAR(optid)
		if (len(*opt) = 1) then
			return optid
		end if
	#endmacro

	select case as const asc(*opt)
	case asc("a")
		ONECHAR(OPT_A)
		CHECK("arch", OPT_ARCH)
		CHECK("asm", OPT_ASM)

	case asc("b")
		ONECHAR(OPT_B)
		CHECK("buildprefix", OPT_BUILDPREFIX)

	case asc("c")
		ONECHAR(OPT_C)

	case asc("C")
		ONECHAR(OPT_CKEEPOBJ)

	case asc("d")
		ONECHAR(OPT_D)
		CHECK("dos-threads", OPT_DOS_THREADS)
		CHECK("dll", OPT_DLL)
		CHECK("dylib", OPT_DYLIB)

	case asc("e")
		ONECHAR(OPT_E)
		CHECK("ex", OPT_EX)
		CHECK("earray", OPT_EARRAY)
		CHECK("earraydims", OPT_EARRAYDIMS)
		CHECK("eassert", OPT_EASSERT)
		CHECK("edebug", OPT_EDEBUG)
		CHECK("edebuginfo", OPT_EDEBUGINFO)
		CHECK("elocation", OPT_ELOCATION)
		CHECK("enullptr", OPT_ENULLPTR)
		CHECK("eunwind", OPT_EUNWIND)
		CHECK("entry", OPT_ENTRY)
		CHECK("exx", OPT_EXX)
		CHECK("export", OPT_EXPORT)

	case asc("f")
		CHECK("fbgfx", OPT_FBGFX)
		CHECK("forcelang", OPT_FORCELANG)
		CHECK("fpmode", OPT_FPMODE)
		CHECK("fpu", OPT_FPU)

	case asc("g")
		ONECHAR(OPT_G)
		CHECK("gfx3", OPT_GFX3)
		CHECK("gen", OPT_GEN)

	case asc( "h" )
		CHECK( "help", OPT_HELP )

	case asc("i")
		ONECHAR(OPT_I)
		CHECK("include", OPT_INCLUDE)

	case asc("l")
		ONECHAR(OPT_L)
		CHECK("lang", OPT_LANG)
		CHECK("lib", OPT_LIB)

	case asc("m")
		ONECHAR(OPT_M)
		CHECK("map", OPT_MAP)
		CHECK("maxerr", OPT_MAXERR)
		CHECK("mt", OPT_MT)

	case asc("n")
		CHECK("noerrline", OPT_NOERRLINE)
		CHECK("nodeflibs", OPT_NODEFLIBS)
		CHECK("nolib", OPT_NOLIB)
		CHECK("noobjinfo", OPT_NOOBJINFO)
		CHECK("nostrip", OPT_NOSTRIP)

	case asc("o")
		ONECHAR(OPT_O)

	case asc("O")
		ONECHAR(OPT_OPTIMIZE)

	case asc("p")
		ONECHAR(OPT_P)
		CHECK("pic", OPT_PIC)
		CHECK("pp", OPT_PP)
		CHECK("prefix", OPT_PREFIX)
		CHECK("print", OPT_PRINT)
		CHECK("profile", OPT_PROFILE)
		CHECK("profgen", OPT_PROFGEN)

	case asc("r")
		ONECHAR(OPT_R)
		CHECK("rr", OPT_RR)

	case asc("R")
		ONECHAR(OPT_RKEEPASM)
		CHECK("RR", OPT_RRKEEPASM)

	case asc("s")
		ONECHAR(OPT_S)
		CHECK("semantic-model", OPT_SEMANTIC_MODEL)
		CHECK("semantic-model-expressions", OPT_SEMANTIC_EXPRESSIONS)
		CHECK("showincludes", OPT_SHOWINCLUDES)
		CHECK("static", OPT_STATIC)
		CHECK("strip", OPT_STRIP)
		CHECK("sysroot", OPT_SYSROOT)

	case asc("t")
		ONECHAR(OPT_T)
		CHECK("target", OPT_TARGET)
		CHECK("title", OPT_TITLE)

	case asc("v")
		ONECHAR(OPT_V)
		CHECK("vec", OPT_VEC)
		CHECK("version", OPT_VERSION)

	case asc("w")
		ONECHAR(OPT_W)

	case asc("W")
		CHECK("Wa", OPT_WA)
		CHECK("Wl", OPT_WL)
		CHECK("Wc", OPT_WC)

	case asc("x")
		ONECHAR(OPT_X)

	case asc("z")
		ONECHAR(OPT_Z)

	case asc( "-" )
		CHECK( "-version", OPT_VERSION )
		CHECK( "-help", OPT_HELP )

	end select

	return -1
end function

declare sub parseArgsFromFile _
	( _
		byref filename as string, _
		byval is_source as integer _
	)

private sub handleArg _
	( _
		byref arg as string, _
		byval is_source as integer, _
		byval is_file as integer _
	)
	'' If the previous option wants this argument as parameter,
	'' call the handler with it, now that it's known.
	'' Note: Anything is accepted, even if it starts with '-' or '@'.
	if( fbc.optid >= 0 ) then
		'' Complain about empty next argument
		if (len(arg) = 0) then
			hFatalInvalidOption( arg, is_source )
		end if

		handleOpt( fbc.optid, arg, is_source )
		fbc.optid = -1
		return
	end if

	if (len(arg) = 0) then
		'' Ignore empty argument
		return
	end if

	select case asc(arg)
	case asc("-")
		dim as zstring ptr opt = strptr(arg) + 1

		'' Complain about '-' only
		if (len(arg) = 1) then
			'' Incomplete command line option
			hFatalInvalidOption( arg, is_source )
		end if

		'' Parse the option after the '-'
		dim as integer optid = parseOption(opt)
		if (optid < 0) then
			'' Unrecognized command line option
			hFatalInvalidOption( arg, is_source )
		end if

		'' Are we in source and option not allowed in source?
		if( is_source ) then
			if( not cmdlineOptionTB( optid ).allowed_in_source ) then
				hFatalInvalidOption( arg, is_source )
			endif
		end if

		'' Does this option take a parameter?
		if( cmdlineOptionTB( optid ).takes_argument ) then
			'' Delay handling it, until the next argument is known.
			fbc.optid = optid
		else
			'' Handle this option now
			handleOpt( optid, arg, is_source )
		end if

		'' even if the handling of the option is delayed, check the restart options here
		if( is_source ) then
			if( cmdlineOptionTB( optid ).parser_restart ) then
				fbRestartBeginRequest( FB_RESTART_PARSER_CMDLINE )
			end if

			if( cmdlineOptionTB( optid ).fbc_restart ) then
				fbRestartBeginRequest( FB_RESTART_FBC_CMDLINE )
			end if
		end if

	case asc("@")
		'' Maximum nesting/recursion level
		const MAX_LEVELS = 128
		static as integer reclevel = 0

		if (reclevel > MAX_LEVELS) then
			'' Options file nesting level too deep (recursion?)
			errReportEx( FB_ERRMSG_RECLEVELTOODEEP, arg, -1 )
			fbcEnd(1)
		end if

		'' Cut off the '@' at the front to get just the file name
		arg = right(arg, len(arg) - 1)

		'' Complain about '@' only
		if (len(arg) = 0) then
			'' Missing file name after '@'
			hFatalInvalidOption( arg, is_source )
		end if

		'' Recursively read in the additional options from the file
		reclevel += 1
		parseArgsFromFile( arg, is_source )
		reclevel -= 1

	case else
		'' Input file, get its extension to determine what it is
		dim as string ext = hGetFileExt(arg)

		#if defined(__FB_WIN32__) or defined(__FB_DOS__) or defined(__FB_CYGWIN__)
			'' For case in-sensitive file systems
			ext = lcase(ext)
		#endif

		select case (ext)
		case "bas"
			hAddBas( arg )

		case "o"
			fbcAddObj( arg )

		case "a"
			strlistAppend( @fbc.libfiles, arg )

		case "so", "dylib"
			'' Dynamic libraries are passed to the linker by their full path.
			'' Unlike .a files, they do not contain FreeBASIC object metadata.
			strlistAppend( @fbc.libfiles, arg )

		case "rc", "res"
			hSetIofile( listNewNode( @fbc.rcs ), arg, TRUE )

		case "xpm"
			'' Can have only one .xpm, or the fb_program_icon
			'' symbol will be duplicated
			if( len( fbc.xpm.srcfile ) > 0 ) then
				hFatalInvalidOption( arg, is_source )
			end if

			hSetIofile( @fbc.xpm, arg, TRUE )

		case else
			'' Input file without or with unknown extension
			hFatalInvalidOption( arg, is_source )

		end select
	end select
end sub

sub fbcParseArgsFromString _
	( _
		byval args_in as zstring ptr, _
		byval is_source as integer, _
		byval is_file as integer _
	)

	dim as string args = *args_in
	dim as string arg

	'' Parse the line containing command line arguments,
	'' separated by spaces. Double- and single-quoted strings
	'' are handled too, but nothing else.
	do
		dim as integer length = len(args)
		if (length = 0) then
			exit do
		end if

		dim as integer i = 0
		dim as integer quotech = 0

		while (i < length)
			dim as integer ch = args[i]

			select case as const (ch)
			case asc(" ")
				if (quotech = 0) then
					exit while
				end if

			case asc(""""), asc("'")
				if (quotech = ch) then
					'' String closed
					quotech = 0
				elseif (quotech = 0) then
					'' String opened
					quotech = ch
				end if

			end select

			i += 1
		wend

		if (i = 0) then
			'' Just space, skip it
			i = 1
		else
			arg = left(args, i)
			arg = trim(arg)
			arg = strUnquote(arg)
			handleArg( arg, is_source, is_file )
		end if

		args = right(args, length - i)
	loop

end sub

private sub parseArgsFromFile _
	( _
		byref filename as string, _
		byval is_source as integer _
	)
	dim as integer f = freefile()
	if (open(filename, for input, as #f)) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, filename, -1 )
		fbcEnd(1)
	end if

	dim as string args

	while (eof(f) = FALSE)
		line input #f, args
		args = trim(args)
		fbcParseArgsFromString( strptr( args ), is_source, TRUE )
	wend

	close #f
end sub

'' Whether a target needs shared libraries to be built with PIC.
'' (Note: Android 5.0+ also need executables to be built with PIC (gcc -pie argument),
'' but Android <4.1 didn't support PIE executables. We assume 4.1+.)
private function hTargetNeedsPIC( ) as integer
	function = FALSE
	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_OPENBSD ) then
		function = TRUE
		exit function
	end if
	if( fbGetCpuFamily( ) <> FB_CPUFAMILY_X86 ) then
		select case as const( fbGetOption( FB_COMPOPT_TARGET ) )
		case FB_COMPTARGET_LINUX, FB_COMPTARGET_FREEBSD, _
		     FB_COMPTARGET_NETBSD, _
		     FB_COMPTARGET_DRAGONFLY, FB_COMPTARGET_SOLARIS, _
		     FB_COMPTARGET_ILLUMOS, _
		     FB_COMPTARGET_ANDROID, FB_COMPTARGET_RISCOS
			function = TRUE
		end select
	else
		'' On android-x86, PIC is necessary even to access globals in dynamic
		'' libraries, because the runtime linker doesn't support usual relocation types.
		'' GCC defaults to -fPIC anyway, but we need to be aware of whether PIC is used.
		if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ANDROID ) then
			function = TRUE
		end if
	end if
end function

sub fbcDriverParseArgs( byval argc as integer, byval argv as zstring ptr ptr )
	fbc.optid = -1

	'' Note: ignoring argv[0], assuming it's the path used to run fbc
	dim as string arg
	for i as integer = 1 to (argc - 1)
		arg = *argv[i]
		handleArg( arg, FALSE, FALSE )
	next

	'' Waiting for argument to an option? If the user did something like
	'' 'fbc foo.bas -o' this shows the error.
	if (fbc.optid >= 0) then
		'' Missing argument for command line option
		hFatalInvalidOption( *argv[argc - 1], FALSE )
	end if
end sub

'' -------------------------------------------------------------------------
'' Invocation validation
'' -------------------------------------------------------------------------

sub fbcDriverCheckArgs()
	'' In case there was an '-o <file>', but no corresponding input file,
	'' this will report the error.
	hCheckWaitingObjfile( )

	''
	'' Check for incompatible options etc.
	''
	select case( fbGetOption( FB_COMPOPT_FPUTYPE ) )
	case FB_FPUTYPE_FPU
		if( fbGetOption( FB_COMPOPT_VECTORIZE ) >= FB_VECTORIZE_NORMAL ) then
			errReportEx( FB_ERRMSG_OPTIONREQUIRESSSE, "", -1 )
			fbcEnd( 1 )
		end if
	case FB_FPUTYPE_SSE
		if( (fbGetCpuFamily( ) <> FB_CPUFAMILY_X86) and _
		    (fbGetCpuFamily( ) <> FB_CPUFAMILY_X86_64) ) then
			errReportEx( FB_ERRMSG_SSEREQUIRESX86, "", -1 )
			fbcEnd( 1 )
		end if
	case FB_FPUTYPE_NEON
		if( (fbGetCpuFamily( ) <> FB_CPUFAMILY_ARM) and _
		    (fbGetCpuFamily( ) <> FB_CPUFAMILY_AARCH64) ) then
			errReportEx( FB_ERRMSG_NEONREQUIRESARM, "", -1 )
			fbcEnd( 1 )
		end if
	end select

	'' 1. The compiler (fb.bas) starts with default target settings for
	''    native compilation.

	'' 2. -target option handling has already switched the target if given.

	'' 3. -arch overrides any other arch settings.
	if( fbc.cputype >= 0 ) then
		fbSetOption( FB_COMPOPT_CPUTYPE, fbc.cputype )
	end if

	'' NEON implies at least armv7-a
	if( (fbGetOption( FB_COMPOPT_FPUTYPE ) = FB_FPUTYPE_NEON) and _
	    (fbGetOption( FB_COMPOPT_CPUTYPE ) < FB_CPUTYPE_ARMV7A) ) then
		fbSetOption( FB_COMPOPT_CPUTYPE, FB_CPUTYPE_ARMV7A )
	end if

	'' 4. Check for target/arch conflicts, e.g. dos and non-x86
	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS) and _
		(fbGetCpuFamily( ) <> FB_CPUFAMILY_X86) ) then
		errReportEx( FB_ERRMSG_DOSWITHNONX86, fbGetFbcArch( ), -1 )
		fbcEnd( 1 )
	end if

	if( fbc.dos_threads ) then
		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DOS ) then
			errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "-dos-threads requires -target dos", -1 )
			fbcEnd( 1 )
		end if
		'' The native scheduler preserves x87 state. Do not silently accept a
		'' code generation mode whose SIMD registers it cannot switch.
		if( fbGetOption( FB_COMPOPT_FPUTYPE ) <> FB_FPUTYPE_FPU ) then
			errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "-dos-threads requires -fpu x87", -1 )
			fbcEnd( 1 )
		end if
	elseif( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS) and _
		fbGetOption( FB_COMPOPT_MULTITHREADED ) ) then
		errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "-mt requires -dos-threads pdmlwp on DOS", -1 )
		fbcEnd( 1 )
	end if

	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS) and _
		fbGetOption( FB_COMPOPT_MULTITHREADED ) ) then
		errReportEx( FB_ERRMSG_INVALIDCMDOPTION, "-mt", -1 )
		fbcEnd( 1 )
	end if

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
		'' The nxdk Xbox package uses worker-backed graphics and sound
		'' services.  Prefer the thread-safe runtime by default so Xbox
		'' programs do not need to pass -mt just to use the normal package
		'' backends safely.
		fbSetOption( FB_COMPOPT_MULTITHREADED, TRUE )
		fbc.objinf.mt = TRUE
	end if

	'' 4.5. Enable -pic automatically when building a Unix shared library,
	''      an OpenBSD executable, or an Android executable.
	if( (fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB) or _
	    (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_OPENBSD) or _
	    (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ANDROID) ) then
		if( hTargetNeedsPIC( ) ) then
			fbSetOption( FB_COMPOPT_PIC, TRUE )
		end if
	end if

	'' Complain if -pic was given in cases where it's not needed/supported
	if( fbGetOption( FB_COMPOPT_PIC ) ) then
		if( hTargetNeedsPIC( ) = FALSE ) then
			errReportEx( FB_ERRMSG_PICNOTSUPPORTEDFORTARGET, "", -1 )
		end if
	end if

	'' 5. Select default backend based on selected arch, e.g. when compiling
	''    for x86-64 or ARM, we shouldn't default to -gen gas anymore (as
	''    long as it doesn't support it).
	''
	'' This should be done no matter whether compiling for the native system
	'' or cross-compiling. Even on a 64bit x86_64 host where
	'' FB_DEFAULT_BACKEND is -gen gcc, we still prefer using -gen gas when
	'' cross-compiling to 32bit x86.
	'' (Apple gas assembler has such broken support for intel syntax
	'' (see https://discussions.apple.com/message/10163960#10163960)
	'' that it can't work for non-trivial programs, so default to -gen gcc.)
	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
		'' nxdk is clang/LLVM based and does not provide the old xbox-as
		'' toolchain expected by the GAS backend.
		fbSetOption( FB_COMPOPT_BACKEND, FB_BACKEND_CLANG )
	elseif( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WIN32) and _
		(fbGetCpuFamily( ) = FB_CPUFAMILY_AARCH64) ) then
		'' Windows ARM64 toolchains are LLVM/Clang based in the supported
		'' MSYS2 environment.  Prefer clang so assembly is handled by the
		'' compiler driver instead of assuming a GNU as driver exists.
		fbSetOption( FB_COMPOPT_BACKEND, FB_BACKEND_CLANG )
	elseif( (fbGetCpuFamily( ) = FB_CPUFAMILY_X86) and _
		(fbGetOption(FB_COMPOPT_TARGET) <> FB_COMPTARGET_DARWIN) ) then
		fbSetOption( FB_COMPOPT_BACKEND, FB_BACKEND_GAS )
	else
		fbSetOption( FB_COMPOPT_BACKEND, FB_BACKEND_GCC )
	end if

	'' gas doesn't currently support PIC
	if( (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GAS) and _
	    fbGetOption( FB_COMPOPT_PIC ) ) then
		fbSetOption( FB_COMPOPT_BACKEND, FB_BACKEND_GCC )
	end if

	'' 6. -gen overrides any other backend setting.
	if( fbc.backend >= 0 ) then
		fbSetOption( FB_COMPOPT_BACKEND, fbc.backend )
	end if

	'' 7. Check whether backend supports the target/arch.
	'' -gen gas with non-x86 arch or with PIC isn't possible.
	'' -gen gas64 with non-x86_64 or with PIC isn't possible.
	if( ((fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GAS) and _
	    (fbGetCpuFamily( ) <> FB_CPUFAMILY_X86)) or _
	    ((fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GAS64) and _
	    (fbGetCpuFamily( ) <> FB_CPUFAMILY_X86_64)) ) then
		errReportEx( FB_ERRMSG_GENGASWITHNONX86, fbGetFbcArch( ), -1 )
		fbcEnd( 1 )
	end if

	if( (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GAS) and _
	    fbGetOption( FB_COMPOPT_PIC ) ) then
		errReportEx( FB_ERRMSG_GENGASWITHPIC, "", -1 )
		fbcEnd( 1 )
	end if

	'' Resource scripts are only allowed for win32 & co,
	select case as const (fbGetOption(FB_COMPOPT_TARGET))
	case FB_COMPTARGET_WIN32, FB_COMPTARGET_CYGWIN, FB_COMPTARGET_XBOX
		exit select

	case else
		dim as FBCIOFILE ptr rc = listGetHead(@fbc.rcs)
		if (rc) then
			errReportEx(FB_ERRMSG_RCFILEWRONGTARGET, rc->srcfile, -1)
			fbcEnd(1)
		end if
	end select

	'' The embedded .xpm is only useful for the X11 gfxlib
	select case as const (fbGetOption(FB_COMPOPT_TARGET))
	case FB_COMPTARGET_LINUX, FB_COMPTARGET_DARWIN, _
		FB_COMPTARGET_FREEBSD, FB_COMPTARGET_OPENBSD, _
		FB_COMPTARGET_NETBSD, FB_COMPTARGET_DRAGONFLY, _
		FB_COMPTARGET_SOLARIS, FB_COMPTARGET_ILLUMOS

	case else
		if (len(fbc.xpm.srcfile) > 0) then
			errReportEx(FB_ERRMSG_RCFILEWRONGTARGET, fbc.xpm.srcfile, -1)
			fbcEnd(1)
		end if
	end select

	'' On darwin need to change the default asm syntax when using gen gcc because
	'' most C compilers on OSX seem to be configured without intel syntax support;
	'' probably because Apple as and llvm-mc have horribly broken intel support.
	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN) and _
		(fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS) ) then
		fbSetOption( FB_COMPOPT_ASMSYNTAX, FB_ASMSYNTAX_ATT )
	end if

	if( fbc.asmsyntax >= 0 ) then
		'' -asm only applies to x86 and x86_64
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
			exit select
		case else
			errReportEx( FB_ERRMSG_ASMOPTIONGIVENFORNONX86, fbGetTargetId( ), -1 )
		end select

		'' -gen gas only supports -asm intel
		select case fbGetOption( FB_COMPOPT_BACKEND )
		case FB_BACKEND_GAS, FB_BACKEND_GAS64
			if( fbc.asmsyntax <> FB_ASMSYNTAX_INTEL ) then
				errReportEx( FB_ERRMSG_GENGASWITHOUTINTEL, "", -1 )
			end if
		end select

		'' -asm overrides the target's default
		fbSetOption( FB_COMPOPT_ASMSYNTAX, fbc.asmsyntax )
	end if

	'' Update the stacksize for the current target options if
	'' stacksize was never set yet by passing a negative stacksize
	fbSetOption( FB_COMPOPT_STACKSIZE, -1 )

	if( len( fbc.subsystem ) > 0 ) then
		select case fbGetOption( FB_COMPOPT_TARGET )
		case FB_COMPTARGET_CYGWIN, FB_COMPTARGET_WIN32, FB_COMPTARGET_JS
			exit select
		case else
			hFatalInvalidOption( "-s " + fbc.subsystem, FALSE )
		end select
	end if

	if( fbc.stacksize_set ) then
		select case fbGetOption( FB_COMPOPT_TARGET )
		case FB_COMPTARGET_CYGWIN, FB_COMPTARGET_WIN32, _
		     FB_COMPTARGET_XBOX, FB_COMPTARGET_DOS
		case else
			hFatalInvalidOption( "-t", FALSE )
		end select
	end if

	if( len( fbc.xbe_title ) > 0 ) then
		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX ) then
			hFatalInvalidOption( "-title " + fbc.xbe_title, FALSE )
		end if
	end if
end sub

'' Determine base/prefix path

'' end of driver/fbc-options.bas
