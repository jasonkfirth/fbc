'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc.bas
''
'' Purpose:
''
''     Own one compiler invocation and coordinate its phases.
''
'' Responsibilities:
''
''     - initialize and release invocation state
''     - sequence argument checking, compilation, and linking
''     - expose invocation observations to compiler subsystems
''
'' This file intentionally does NOT contain:
''
''     - command-line parsing, target hooks, or external tool implementations
''

#include once "driver/fbc-private.bi"

'' Compiler invocation entry point
''
'' Ownership: The driver owns its shared context, command-line temporary state,
'' and generated temporary files until fbcEnd() releases them.
''
'' chng: sep/2004 written [v1ctor]
''       dec/2004 linux support added [lillo]
''       jan/2005 dos support added [DrV]

dim shared as FBCCTX fbc

function fbcGetVerbose() as integer
	function = fbc.verbose
end function

function fbcGetSourceCmdline() as string
	function = fbc.sourcecmdline
end function

function fbcGetEntry() as string
	function = fbc.entry
end function

'' Keep accepted source #cmdline text for the __CMDLINE__ intrinsic define.
'' Preserve each directive's text and separate directives with one space.
sub fbcAddSourceCmdline(byval args as zstring ptr)
	if( args = NULL ) then
		exit sub
	end if

	if( len( *args ) = 0 ) then
		exit sub
	end if

	if( len( fbc.sourcecmdline ) > 0 ) then
		fbc.sourcecmdline += " "
	end if

	fbc.sourcecmdline += *args
end sub

private sub fbcInit( )
	const FBC_INITFILES = 64

	fbc.backend = -1
	fbc.cputype = -1
	fbc.asmsyntax = -1
	fbc.sourcecmdline = ""
	fbc.entry = ""

	listInit( @fbc.modules, FBC_INITFILES, sizeof(FBCIOFILE) )
	listInit( @fbc.rcs, FBC_INITFILES\4, sizeof(FBCIOFILE) )
	strsetInit( @fbc.temps, FBC_INITFILES\4 )
	strlistInit( @fbc.objlist, FBC_INITFILES )
	strlistInit( @fbc.libfiles, FBC_INITFILES\4 )
	strsetInit( @fbc.libs, FBC_INITFILES\4 )
	strsetInit( @fbc.libpaths, FBC_INITFILES\4 )
	strsetInit( @fbc.excludedlibs, FBC_INITFILES\4 )

	strsetInit(@fbc.finallibs, FBC_INITFILES\2)
	strsetInit(@fbc.finallibpaths, FBC_INITFILES\2)

	fbGlobalInit()

#ifdef ENABLE_STRIPALL
	fbc.stripsymbols = TRUE
#endif

	fbc.objinf.lang = fbGetOption( FB_COMPOPT_LANG )

	fbc.print = -1
end sub

sub fbcEnd( byval errnum as integer )
	'' Inputs and requested artifacts remain protected through final publication.
	dim as FBCIOFILE ptr module = listGetHead(@fbc.modules)
	while( module <> NULL )
		fbSemanticModelProtectFile(module->srcfile)
		fbSemanticModelProtectFile(module->asmfile)
		if( module->objfile <> NULL ) then fbSemanticModelProtectFile(*module->objfile)
		module = listGetNext(module)
	wend
	dim as string ptr artifact = listGetHead(@fbc.objlist)
	while( artifact <> NULL )
		fbSemanticModelProtectFile(*artifact)
		artifact = listGetNext(artifact)
	wend
	artifact = listGetHead(@fbc.libfiles)
	while( artifact <> NULL )
		fbSemanticModelProtectFile(*artifact)
		artifact = listGetNext(artifact)
	wend
	fbSemanticModelProtectFile(fbc.outname)
	fbSemanticModelProtectFile(fbc.mapfile)
	if( fbSemanticModelEnd( errnum = 0 ) = FALSE ) then
		print "error: could not write semantic model: "; fbc.semanticmodel
		errnum = 1
	end if

	'' Clean up temporary files
	dim as TSTRSETITEM ptr file = listGetHead(@fbc.temps.list)
	while( file )
		safeKill( file->s )
		file = listGetNext( file )
	wend

	'' The directory is created only for compiler-owned archive metadata.
	'' All known files above must be removed before attempting to remove it.
	if( len( fbc.fbctinfdir ) > 0 ) then
		if( rmdir( fbc.fbctinfdir ) <> 0 ) then
			if( fbc.verbose ) then
				print "warning: could not remove temporary directory: ", _
					fbc.fbctinfdir
			end if
		end if
	end if

	end errnum
end sub

	fbcInit( )

	if( __FB_ARGC__ = 1 ) then
		fbcDriverPrintOptions( FALSE )
		fbcEnd( 1 )
	end if

	fbcDriverParseArgs( __FB_ARGC__, __FB_ARGV__ )

	fbcDriverCheckArgs( )

	if( fbc.showversion ) then
		fbcDriverPrintVersion( fbc.verbose )
		fbcEnd( 0 )
	end if

	if( fbc.verbose ) then
		fbc.showversion = TRUE
		fbcDriverPrintVersion( FALSE )
	end if

	'' Show help if -help was given
	if( fbc.showhelp ) then
		fbcDriverPrintOptions( fbc.verbose )
		fbcEnd( 1 )
	end if

	do
		fbcDeterminePrefix( )
		fbcPlatformAdjustPrefix( fbc.prefix )
		fbcSetupCompilerPaths( )

		if( fbc.verbose ) then
			fbcPrintTargetInfo( )
		end if

		'' Tell the compiler about the default include paths (added after
		'' the command line ones, so those will be searched first).
		fbcPlatformAddDefaultIncludePaths( fbc.incpath )
		fbAddIncludePath( fbc.incpath )

		var have_input_files = (listGetHead( @fbc.modules   ) <> NULL) or _
			(listGetHead( @fbc.objlist   ) <> NULL) or _
			(listGetHead( @fbc.libs.list ) <> NULL) or _
			(listGetHead( @fbc.libfiles  ) <> NULL)

		'' Answer -print query, if any, and stop
		'' The -print option is intended to allow shell scripts, makefiles, etc.
		'' to query information from fbc.
		if( fbc.print >= 0 ) then
			select case( fbc.print )
			case PRINT_HOST
				print fbGetHostId( )
			case PRINT_TARGET
				print fbGetTargetId( )
			case PRINT_X
				'' If we have input files, -print x should give the output name that we'd normally get.
				'' However, a plain "fbc -print x" without input files should just give the .exe extension.
				if( have_input_files ) then
					fbcDetermineMainName( )
				end if
				fbcDriverSetOutName( )
				print fbc.outname
			case PRINT_FBLIBDIR
				print fbc.libpath
			case PRINT_SHA1
				print FB_BUILD_SHA1
			case PRINT_FORK_ID
				print FB_BUILD_FORK_ID
			end select
			fbcEnd( 0 )
		end if

		fbcDetermineMainName( )

		'' Show help if there are no input files
		if( have_input_files = FALSE ) then
			fbcDriverPrintOptions( fbc.verbose )
			fbcEnd( 1 )
		end if

		if( len( fbc.semanticmodel ) > 0 ) then
			if( fbSemanticModelBegin( fbc.semanticmodel, fbc.semanticmodel_expressions ) = FALSE ) then
				print "error: could not write semantic model: "; fbc.semanticmodel
				fbcEnd( 1 )
			end if
		end if

		''
		'' Compile .bas modules
		''
		fbcDriverCompileModules( )

		if( fbShouldRestart( ) = FALSE ) then
			exit do
		end if

		fbRestartEndRequest( FB_RESTART_FBC_CMDLINE )

		'' we are restarting, so show errors again
		errPreInit( )

		'' command line arguments have changed, check them again
		fbcDriverCheckArgs( )

		if( fbc.verbose ) then
			print "Restarting fbc ..."
		end if
	loop

	'' Object files which are consumed by this invocation are implementation
	'' details.  Give them invocation-private names before the backend creates
	'' them so parallel compilers cannot link or delete one another's objects.
	fbcDriverUseTemporaryObjfiles( )

	if( fbcDriverCompileXpm( ) = FALSE ) then
		fbcEnd( 1 )
	end if

	if( fbc.emitasmonly ) then
		fbcEnd( 0 )
	end if

	if( (fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS)  and _
		fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS64 ) then
		''
		'' Compile intermediate .c modules produced by -gen gcc
		''
		fbcDriverCompileStage2Modules( )
	end if

	if( fbc.emitfinalasmonly ) then
		fbcEnd( 0 )
	end if

	''
	'' Assemble into .o files
	''
	fbcDriverAssembleModules( )
	fbcDriverAssembleRcs( )
	fbcDriverAssembleXpm( )

	'' Stop for -c
	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_OBJECT ) then
		fbcEnd( 0 )
	end if

	'' Set the default lib paths before scanning for other libs
	fbcDriverSetDefaultLibPaths( )

	'' Scan objects and libraries for more libraries and paths,
	'' before adding the default libs, which don't need to be searched,
	'' because they don't contain objinfo anyways.
	if( fbGetOption( FB_COMPOPT_OBJINFO ) and _
		(fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS) and _
		(not fbIsCrossComp( )) ) then
		fbcDriverCollectObjinfo( )
	end if

	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_STATICLIB ) then
		if( fbcDriverArchiveFiles( ) = FALSE ) then
			fbcEnd( 1 )
		end if
		fbcEnd( 0 )
	end if

	'' Link

	'' Add default libs for linking, unless -nodeflibs was given
	'' Note: These aren't added into objinfo sections of objects or
	'' static libraries. Only the non-default libs are needed there.
	if( fbc.nodeflibs = FALSE ) then
		fbcDriverAddDefaultLibs( )
	end if

	fbcDriverExcludeLibsFromLink( )

	if( fbcDriverLinkFiles( ) = FALSE ) then
		fbcEnd( 1 )
	end if

	fbcEnd( 0 )

'' end of driver/fbc.bas
