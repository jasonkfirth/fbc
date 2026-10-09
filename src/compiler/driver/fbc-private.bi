'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-private.bi
''
'' Purpose:
''
''     Define the internal contracts shared by driver implementation modules.
''
'' Responsibilities:
''
''     - describe invocation-owned state and external tool identities
''     - declare the interfaces between driver phases
''     - document state ownership and serial execution
''
'' This file intentionally does NOT contain:
''
''     - parser contracts, target hook bodies, or executable initialization
''

#ifndef __FBC_PRIVATE_BI__
#define __FBC_PRIVATE_BI__

#include once "core/fb.bi"
#include once "support/hlp.bi"
#include once "support/containers/hash.bi"
#include once "support/containers/list.bi"
#include once "driver/objinfo.bi"
#include once "support/strings/dstr.bi"

#include once "file.bi"

#if defined( ENABLE_STANDALONE ) and defined( __FB_WIN32__ )
	#define ENABLE_GORC
#endif

enum
	PRINT_HOST
	PRINT_TARGET
	PRINT_X
	PRINT_FBLIBDIR
	PRINT_SHA1
	PRINT_FORK_ID
end enum

type FBC_EXTOPT
	gas         as string
	ld          as string
	gcc         as string
end type

type FBCIOFILE
	'' Input file name (usually *.bas, but also *.rc, *.res, *.xpm)
	srcfile         as string     '' input file

	'' Stage-one output (.asm/.c/.ll), retained across parser restarts
	asmfile         as string

	'' Output .o file
	'' - for modules from the command line this points to a node from
	''   fbc.objlist, see also fbcAddObj()
	'' - for example in hCompileFbctinf(), add temporary FBCIOFILE is used,
	''   with objfile pointing to a string var on stack
	objfile         as string ptr

	'' Whether -o was used to override the default .o file name
	is_custom_objfile   as integer
end type

type FBC_OBJINF
	lang        as FB_LANG
	mt          as integer
end type

type FBCCTX
	'' For command line parsing
	optid               as integer     '' Current option
	lastmodule          as FBCIOFILE ptr '' module for last input file, so the default .o name can be overwritten with a following -o filename
	objfile             as string      '' -o filename waiting for next input file
	backend             as integer     '' FB_BACKEND_* given via -gen, or -1 if -gen wasn't given
	cputype             as integer     '' FB_CPUTYPE_* (-arch's argument), or -1
	cputype_is_native   as integer     '' Whether -arch native was used
	asmsyntax           as integer     '' FB_ASMSYNTAX_* from -asm, or -1 if not given

	emitasmonly         as integer     '' write out FB backend output file only (.asm/.c)
	keepasm             as integer     '' preserve FB backend output file (.asm/.c)
	emitfinalasmonly    as integer     '' write out final .asm file only
	keepfinalasm        as integer     '' preserve final .asm
	keepobj             as integer
	verbose             as integer
	sourcecmdline       as string      '' accepted options from source #cmdline directives
	showversion         as integer
	showhelp            as integer
	print               as integer     '' PRINT_* (-print option)

	'' Command line input
	modules             as TLIST    '' FBCIOFILE's for input .bas files
	rcs                 as TLIST    '' FBCIOFILE's for input .rc/.res files
	xpm                 as FBCIOFILE '' .xpm input file
	temps               as TSTRSET  '' Temporary files to delete at shutdown
	objlist             as TLIST    '' Objects from command line and from compilation
	libfiles            as TLIST
	libs                as TSTRSET
	libpaths            as TSTRSET
	excludedlibs        as TSTRSET  '' lib names explicitly excluded via -nodeflib option(s)

	'' Final list of libs and paths for linking
	'' (each module can have #inclibs and #libpaths and add more, and for
	'' objinfo emitting only the module-specific libs are wanted, so there
	'' are multiple lists necessary to allow each module to start fresh
	'' with the same input libs)
	finallibs           as TSTRSET
	finallibpaths       as TSTRSET

	outname             as zstring * FB_MAXPATHLEN+1
	mainname            as zstring * FB_MAXPATHLEN+1
	entry               as zstring * FB_MAXNAMELEN+1
	mainset             as integer
	mapfile             as zstring * FB_MAXPATHLEN+1
	subsystem           as zstring * FB_MAXNAMELEN+1
	extopt              as FBC_EXTOPT
#ifndef ENABLE_STANDALONE
	target              as zstring * FB_MAXNAMELEN+1  '' Target system identifier (e.g. a name like "win32", or a GNU triplet) to prefix in front of cross-compiling tool names
	targetprefix        as zstring * FB_MAXNAMELEN+1  '' same, but with "-" appended, if there was a target id given; otherwise empty.
#endif
	sysroot             as zstring * FB_MAXPATHLEN+1
	xbe_title           as zstring * FB_MAXNAMELEN+1  '' For the '-title <title>' xbox option
	dos_threads         as integer     '' Explicit native DOS provider selection
	fbctinfdir          as string      '' Private directory for temporary archive metadata
	stacksize_set       as integer
	nodeflibs           as integer
	nofbrt0             as integer  '' If we should exclude fbrt0.o or fbrt0pic.o (implied by nodeflibs, and optional by -nolib fbrt0.o,fbrt0pic.o)
	staticlink          as integer
	stripsymbols        as integer
	semanticmodel       as string      '' Optional compiler-owned semantic model output
	semanticmodel_expressions as integer '' Emit only module and expression records
	semanticmodel_bindings as integer '' Emit token identities and implicit calls without AST facts
	semanticmodel_compact as integer '' Omit verbose macro-expansion provenance

	'' Compiler paths
	prefix              as zstring * FB_MAXPATHLEN+1  '' Path from -prefix or empty
	binpath             as zstring * FB_MAXPATHLEN+1  '' standalone=prefix/bin/target/  normal=prefix/<fbname>/(target|build)prefix
	incpath             as zstring * FB_MAXPATHLEN+1  '' standalone=prefix/inc          normal=prefix/include/<fbname>
	libpath             as zstring * FB_MAXPATHLEN+1  '' standalone=prefix/lib/target   normal=prefix/lib[64]/<fbname>/target

	'' Tool prefix
	buildprefix         as zstring * FB_MAXPATHLEN+1  '' command line option to override target prefix (affects tool names executed)

	objinf              as FBC_OBJINF
	semanticdiagnostics as string      '' Independent diagnostics, including rejected modules
end type

enum FBCTOOL
	FBCTOOL_NONE = 0
	FBCTOOL_AS
	FBCTOOL_AR
	FBCTOOL_LD
	FBCTOOL_GCC
	FBCTOOL_LLC
	FBCTOOL_CLANG
	FBCTOOL_DLLTOOL
	FBCTOOL_GORC
	FBCTOOL_WINDRES
	FBCTOOL_CXBE
	FBCTOOL_DXEGEN
	FBCTOOL_EMAS
	FBCTOOL_EMAR
	FBCTOOL_EMLD
	FBCTOOL_EMCC
	FBCTOOL_ELF2DOL
	FBCTOOL_ELF2AIF
	FBCTOOL_ELF2HUNK
	FBCTOOL__COUNT
end enum

enum FBCTOOLFLAG
	FBCTOOLFLAG_INVALID            = 0  '' tool is disabled
	FBCTOOLFLAG_ASSUME_EXISTS      = 1  '' assume the tool exists
	FBCTOOLFLAG_CAN_USE_ENVIRON    = 2  '' allow path to tool to specified by environment variable
	FBCTOOLFLAG_FOUND              = 4  '' tool was checked for
	FBCTOOLFLAG_RELYING_ON_SYSTEM  = 8  '' tool is expected to be on system PATH

	FBCTOOLFLAG_DEFAULT = FBCTOOLFLAG_ASSUME_EXISTS or FBCTOOLFLAG_CAN_USE_ENVIRON
end enum

type FBCTOOLINFO
	name as zstring * 16                  '' default name of tool to invoke
	env_variable as zstring * 16          '' environment variable to override
	flags as FBCTOOLFLAG
	path as zstring * (FB_MAXPATHLEN + 1) '' cached tool path and name
end type

#define fbctoolGetFlags( tool, f )   ((fbctoolTB( tool ).flags and (f)) <> 0)
#define fbctoolSetFlags( tool, f )   fbctoolTB( tool ).flags or= f
#define fbctoolUnsetFlags( tool, f ) fbctoolTB( tool ).flags and= not f

'' One invocation is active per process. fbc.bas owns this context until
'' fbcEnd() removes temporary files and terminates the process. Invocation
'' pools have process lifetime; module pools have separate Init/End pairs.
'' These modules are not reentrant; parallel builds use separate processes.
extern fbc as FBCCTX

'' Tool names and paths are initialized and owned by fbc-tools.bas.
extern fbctoolTB(0 to FBCTOOL__COUNT-1) as FBCTOOLINFO

#macro safeKill(f)
	if( kill( f ) <> 0 ) then
	end if
#endmacro

'' -------------------------------------------------------------------------
'' fbc.bas
'' -------------------------------------------------------------------------

declare sub fbcEnd( byval errnum as integer )

'' -------------------------------------------------------------------------
'' fbc-files.bas
'' -------------------------------------------------------------------------

declare sub fbcDriverSetOutName( )

declare sub fbcAddTemp(byref file as string)

declare sub fbcRemoveTemp(byref file as string)

declare function fbcAddObj( byref file as string ) as string ptr

declare function fbcDriverGetTempFileTag( ) as string

declare function fbcDriverGetDosTempFileStem _
	( _
		byref reference as string, _
		byref key as string _
	) as string

declare function fbcDriverCreateFbctinfDirectory( ) as integer

declare sub fbcDriverUseTemporaryObjfiles( )

declare function fbcDriverPutLdArgsIntoFile( byref ldcline as string ) as integer

declare sub fbcDeterminePrefix( )

declare sub fbcSetupCompilerPaths( )

declare sub fbcPrintTargetInfo( )

declare sub fbcDetermineMainName( )

'' -------------------------------------------------------------------------
'' fbc-tools.bas
'' -------------------------------------------------------------------------

declare function fbcDriverGet1stOutputLineFromCommand( byref cmd as string ) as string

declare function fbcDriverGetClangTargetOption( ) as string

declare function fbcUseLldLinker( ) as integer

declare function fbcQueryCC( byref options as string ) as string

declare function fbcBuildPathToLibFile( byval file as zstring ptr ) as string

declare function fbcFindSysroot( ) as string

declare function fbcFindLibFile( byval file as zstring ptr ) as string

declare sub fbcAddDefLibPath(byref path as string)

declare sub fbcAddLibPathFor( byval libname as zstring ptr )

declare sub fbcFindBin _
	( _
		byval tool as integer, _
		byref path as string _
	)

declare function fbcRunBin _
	( _
		byval action as zstring ptr, _
		byval tool as integer, _
		byref ln as string _
	) as integer

'' -------------------------------------------------------------------------
'' fbc-link.bas
'' -------------------------------------------------------------------------

declare function fbcDriverLinkFiles( ) as integer

declare sub fbcDriverCollectObjinfo( )

declare sub fbcDriverSetDefaultLibPaths( )

declare sub fbcAddDefLib(byval libname as zstring ptr)

declare sub fbcDriverAddDefaultLibs( )

declare sub fbcDriverExcludeLibsFromLink( )

'' -------------------------------------------------------------------------
'' fbc-options.bas
'' -------------------------------------------------------------------------

declare sub fbcDriverParseArgs( byval argc as integer, byval argv as zstring ptr ptr )

declare sub fbcDriverCheckArgs()

'' -------------------------------------------------------------------------
'' fbc-compile.bas
'' -------------------------------------------------------------------------

declare sub fbcDriverCompileModules( )

declare function fbcDriverCompileXpm( ) as integer

declare sub fbcDriverCompileStage2Modules( )

declare sub fbcDriverAssembleModules( )

declare sub fbcDriverAssembleRcs( )

declare sub fbcDriverAssembleXpm( )

declare function fbcDriverArchiveFiles( ) as integer

'' -------------------------------------------------------------------------
'' fbc-help.bas
'' -------------------------------------------------------------------------

declare sub fbcDriverPrintOptions( byval verbose as integer )

declare sub fbcDriverPrintVersion( byval verbose as integer )

'' -------------------------------------------------------------------------
'' fbc-platform.bas
'' -------------------------------------------------------------------------

declare sub fbcPlatformAdjustPrefix( byref prefix as string )

declare function fbcPlatformGetLinkerTool( ) as integer

declare function fbcPlatformGetToolPrefix( byval cputype as integer ) as string

declare sub fbcPlatformAddDefaultIncludePaths( byref incpath as string )

declare function fbcPlatformCompilesDirectlyToObject( ) as integer

declare function fbcPlatformGetClangTargetOption( ) as string

declare function fbcPlatformSupportsSupplementaryLinkerScript( ) as integer

declare sub fbcPlatformAdjustParsedCpuType _
	( _
		byval os as integer, _
		byref arch as string, _
		byref cputype as integer _
	)

declare sub fbcPlatformAddCcQueryOptions( byref path as string )

declare function fbcPlatformAddCCompilerCpuOptions _
	( _
		byref ccline as string _
	) as integer

declare function fbcLinuxPlatformArmUsesHardFloatAbi( ) as integer

declare function fbcLinuxPlatformGetArmLlvmTargetTriple _
	( _
		byref targettriple as string _
	) as integer

declare function fbcPlatformUsesCompilerDriverAssembler( ) as integer

declare sub fbcPlatformAddAssemblerOptions( byref ascline as string )

declare sub fbcPlatformAddDefaultLibPaths( )

declare sub fbcPlatformAddGfxLibs( )

declare sub fbcPlatformAddSfxLibs( )

declare sub fbcPlatformAddDefaultLibs( )

declare function fbcPlatformMapLibName( byref libname as string ) as string

declare sub fbcPlatformAddLinkerFrameworks( byref ldcline as string )

declare function fbcArosHostFinishExecutable( ) as integer
declare sub fbcAmigaPlatformAddLinkOptions( byref ldcline as string )
declare sub fbcAmigaPlatformValidateOptions( )

declare sub fbcDarwinPlatformAddCCompilerOptions( byref ccline as string )

declare sub fbcDarwinPlatformAddAssemblerOptions( byref ascline as string )

declare sub fbcDarwinPlatformAddCompilerDriverLinkerOptions( byref ldcline as string )

declare function fbcDarwinPlatformAddDynamicLibOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	) as integer

declare function fbcDarwinPlatformBuildGuiAppBundle( ) as integer

declare sub fbcDarwinPlatformAddExportDynamic( byref ldcline as string )

declare function fbcDarwinPlatformGetFrameworkName _
	( _
		byref libname as string _
	) as string

declare function fbcRiscosHostMayQueryCcForTool( ) as integer

declare function fbcRiscosHostRunTool _
	( _
		byval tool as integer, _
		byref path as string, _
		byref arguments as string, _
		byref was_handled as integer _
	) as integer

declare function fbcRiscosHostFinishExecutable( ) as integer

declare sub fbcWincePlatformAddLinkOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	)

declare sub fbcWincePlatformAddCrtBeginObjects( byref ldcline as string )

declare sub fbcDriverAddXboxLibArchive( byref ldcline as string, byref libname as string )

declare sub fbcDriverAddXboxNxdkLibs( byref ldcline as string )

declare function fbSemanticModelBegin(byref filename as string, byval expressions_only as integer, byval bindings_only as integer, byval macros_enabled as integer) as integer
declare function fbSemanticModelEnd(byval succeeded as integer) as integer
declare sub fbSemanticModelProtectFile(byref filename as const string)
declare sub fbSemanticModelFinishModule(byval commit as integer)
declare sub fbSemanticModelFinishRecoveryModule( )
#include once "tooling/semantic-diagnostics.bi"
#include once "tooling/semantic-link.bi"

#endif

'' end of driver/fbc-private.bi
