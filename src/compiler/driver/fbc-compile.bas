'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-compile.bas
''
'' Purpose:
''
''     Compile source modules and prepare objects and archives.
''
'' Responsibilities:
''
''     - coordinate parser restarts and source metadata
''     - run stage-two compilers and assemblers
''     - compile resources, archive objects, and emit archive metadata
''
'' This file intentionally does NOT contain:
''
''     - command-line parsing or executable linker policy
''
'' Resource ownership:
''     Intermediate paths are registered with the driver cleanup set unless a
''     keep option preserves them. Output objects and archives remain owned by
''     the invocation and are not removed as temporary files.
''

#include once "driver/fbc-private.bi"

'' -------------------------------------------------------------------------
'' Source compilation and parser restarts
'' -------------------------------------------------------------------------

private function hCompileStage2DirectlyToObj( ) as integer
	if( fbcPlatformCompilesDirectlyToObject( ) ) then
		return TRUE
	end if

	select case( fbGetOption( FB_COMPOPT_TARGET ) )
	case FB_COMPTARGET_JS, FB_COMPTARGET_XBOX
		function = TRUE
	case FB_COMPTARGET_WIN32
		if( ((fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG) or _
		     (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GCC)) and _
		    (fbGetCpuFamily( ) = FB_CPUFAMILY_AARCH64) ) then
			''
			'' The Windows ARM64 toolchain is clang/LLVM based.  Let the C
			'' compiler produce COFF objects directly instead of sending C
			'' backend assembly through a GNU as style stage.
			''
			function = TRUE
		end if
	case else
		function = FALSE
	end select
end function

'' Build the intermediate file name for the given module and step
private function hGetAsmName _
	( _
		byval module as FBCIOFILE ptr, _
		byval stage as integer _
	) as string

	dim as zstring ptr ext = any
	dim as string asmfile

	'' Based on the objfile name so it's also affected by -o
	asmfile = hStripExt( *module->objfile )

	if( stage = 1 ) then
		if( (fbc.keepasm = FALSE) and (fbc.emitasmonly = FALSE) and _
			(((fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS) and _
			  (fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS64)) or _
			 ((fbc.keepfinalasm = FALSE) and _
			  (fbc.emitfinalasmonly = FALSE))) ) then
#ifdef __FB_DOS__
			asmfile = fbcDriverGetDosTempFileStem( *module->objfile, "module" )
#else
			asmfile += fbcDriverGetTempFileTag( )
#endif
		end if
	elseif( hCompileStage2DirectlyToObj( ) = FALSE ) then
		if( (fbc.keepfinalasm = FALSE) and (fbc.emitfinalasmonly = FALSE) ) then
#ifdef __FB_DOS__
			asmfile = fbcDriverGetDosTempFileStem( *module->objfile, "module" )
#else
			asmfile += fbcDriverGetTempFileTag( )
#endif
		end if
	end if

	if( hCompileStage2DirectlyToObj( ) = FALSE ) then
		ext = @".asm"
	else
		ext = @".o"
	end if
	if( stage = 1 ) then
		select case( fbGetOption( FB_COMPOPT_BACKEND ) )
		case FB_BACKEND_GCC, FB_BACKEND_CLANG
			ext = @".c"
		case FB_BACKEND_LLVM
			ext = @".ll"
		end select
	end if

	asmfile += *ext

	function = asmfile
end function

private sub hCompileBas _
	( _
		byval module as FBCIOFILE ptr, _
		byval is_main as integer, _
		byval is_fbctinf as integer, _
		byval module_count as integer _
	)

	dim as integer prevlang = any, prevouttype = any
	dim as string asmfile, pponlyfile

	asmfile = hGetAsmName( module, 1 )
	'' A #cmdline directive can change -R after the first parse.  Keep the
	'' name selected before that restart so stage two consumes the file that
	'' the parser actually emitted.
	module->asmfile = asmfile

	'' -pp?
	if( fbGetOption( FB_COMPOPT_PPONLY ) ) then
		'' Re-use the full -o path/filename for the -pp output file,
		'' since no .o will be generated anyways (if -o was given)
		pponlyfile = *module->objfile
		if( module->is_custom_objfile = FALSE ) then
			'' Otherwise, use a default file name
			pponlyfile = hStripExt( pponlyfile ) + ".pp.bas"
		end if
	end if

	if( fbc.verbose ) then
		print "compiling: ", module->srcfile; " -o "; asmfile;
		if( fbGetOption( FB_COMPOPT_PPONLY ) ) then
			print " -pp " + pponlyfile;
		end if
		if( is_main ) then
			print " (main module)";
		elseif( is_fbctinf ) then
			print " (FB compile-time info)";
		end if
		print
	end if

	'' Restarting with a new lang option?
	'' We need to initialize with the restart lang
	if( fbGetOption( FB_COMPOPT_RESTART_LANG ) <> FB_LANG_INVALID ) then
		fbSetOption( FB_COMPOPT_LANG, fbGetOption( FB_COMPOPT_RESTART_LANG ) )
	end if

	'' preserve orginal values that might have to restored
	'' (e.g. -lang mode could be overwritten while parsing due to #lang,
	'' but that shouldn't affect other modules)
	prevlang = fbGetOption( FB_COMPOPT_LANG )
	prevouttype = fbGetOption( FB_COMPOPT_OUTTYPE )

	if( is_fbctinf ) then
		'' Switch to -c mode temporarily to get the compiler to write objinfo
		fbSetOption( FB_COMPOPT_OUTTYPE, FB_OUTTYPE_OBJECT )
	end if

	do
		'' Clean up stage 1 output (FB backend's output, *.asm/*.c/*.ll),
		'' unless -R was given, and additionally in case of -gen gas, unless -RR
		'' was given (because for -gen gas, the FB backend's .asm output is also
		'' the final .asm which -RR is supposed to preserve).
		if( (not fbc.keepasm) and _
			(((fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS) and (fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS64) ) or _
			(not fbc.keepfinalasm)) ) then
			fbcAddTemp( asmfile )

		'' first module? handle side effects of #cmdline
		elseif( module_count = 1 ) then
			'' Keep the asm file.  If the keep option was in a #cmdline then
			'' the temporary file probably was already added to the fbc.temps
			'' list on the first pass in to the parser (unless real command
			'' line also had an option to keep the asm file).

			if( fbRestartGetCount() > 0 ) then
				fbcRemoveTemp( asmfile )
			end if
		end if

		'' init the parser (note: initializes env)
		fbInit( is_main, fbc.entry, module_count )

		if( is_fbctinf ) then
			'' Let the compiler know about all libs collected so far,
			'' so the fbctinf module represents all the other modules
			'' compiled/included in this fbc invocation.
			fbSetLibs( @fbc.finallibs, @fbc.finallibpaths )
		else
			'' Add only the libs and paths passed on the command line,
			'' so this module will only include objinfo for those libs
			'' and the ones found while parsing it, but not unrelated
			'' libs from other modules.
			fbSetLibs( @fbc.libs, @fbc.libpaths )
		end if

		fbCompile( module->srcfile, asmfile, pponlyfile, is_main )

		'' If there were any errors during parsing, just exit without
		'' doing anything else.
		if( errGetCount( ) > 0 ) then
			fbSemanticModelFinishRecoveryModule( )
			fbcEnd( 1 )
		end if

		'' Don't restart unless asked for
		if( fbShouldRestart( ) = FALSE ) then
			fbSemanticModelFinishModule( TRUE )
			exit do
		end if

		fbSemanticModelFinishModule( FALSE )

		'' Close the request to restart the parser
		fbRestartEndRequest( FB_RESTART_PARSER )

		'' Shutdown the parser before restarting
		fbEnd( )

		'' Still have restart set?  It must be a request to restart fbc
		if( fbShouldRestart( ) ) then
			'' Restore original #lang? only if we didn't set a new lang to restart with
			if( fbGetOption( FB_COMPOPT_RESTART_LANG ) = FB_LANG_INVALID ) then
				fbSetOption( FB_COMPOPT_LANG, prevlang )
			end if
			exit sub
		end if
	loop

	'' (unnecessary for the empty fbctinf module, it won't add anything new)
	if( is_fbctinf = FALSE ) then
		'' Update the list of libs and paths with the ones found when parsing
		fbGetLibs( @fbc.finallibs, @fbc.finallibpaths )
	end if

	'' Shutdown the parser
	fbEnd( )

	'' Restore original options
	if( is_fbctinf ) then
		fbSetOption( FB_COMPOPT_OUTTYPE, prevouttype )
	end if
	fbSetOption( FB_COMPOPT_LANG, prevlang )
end sub

sub fbcDriverCompileModules( )
	dim as integer ismain = any, checkmain = any
	dim as string mainfile
	dim as FBCIOFILE ptr module = any

	ismain = FALSE

	select case fbGetOption( FB_COMPOPT_OUTTYPE )
	case FB_OUTTYPE_EXECUTABLE, FB_OUTTYPE_DYNAMICLIB
		checkmain = TRUE
	case else
		'' When building an object or a library (-c/-r, -lib), nothing
		'' is compiled with ismain = TRUE until -m was given for it.
		'' This makes sense because -c is usually used to compile
		'' single modules of which only a very specific one is the
		'' main one (nobody would want -c to include main() everywhere),
		'' and because -lib is for making libraries which generally
		'' don't include a main module for programs to use.
		checkmain = fbc.mainset
	end select

	if( checkmain ) then
		'' Note: This causes the path given with -m to be ignored in
		'' the ismain check below. This is good because -m is easier
		'' to use that way (e.g. fbc ../../main.bas -m main), and bad
		'' because then modules with the same name but in different
		'' directories will both be seen as the main one.
		mainfile = hStripPath( fbc.mainname )
	end if

	module = listGetHead( @fbc.modules )

	if( module = NULL ) then
		'' No input .bas files to compile - make sure to add the libs
		'' from the command line to the final lists anyways.
		strsetCopy( @fbc.finallibs, @fbc.libs )
		strsetCopy( @fbc.finallibpaths, @fbc.libpaths )
		exit sub
	end if

	'' We have input .bas files to compile - hCompileBas() will take care of
	'' copying the command line libs into the final lists:
	'' 1. into the compiler
	''    (fbc.libs -> fbSetLibs() -> compiler)
	'' 2. compiler collects additional #inclibs etc...
	'' 3. and copy back into final lists
	''    (compiler -> fbGetLibs() -> fbc.finallibs)

	dim as integer module_count = 0
	do
		if( checkmain ) then
			ismain = (mainfile = hStripPath( hStripExt( module->srcfile ) ))
			'' Note: checking continues for all modules, because
			'' "the" main module could be passed multiple times,
			'' and it makes sense to always treat it the same,
			'' so that <fbc 1.bas 1.bas -c> generates the same 1.o
			'' twice and <fbc 1.bas 1.bas> causes a duplicated
			'' definition of main().
			/'checkmain = not ismain'/
		end if

		module_count += 1
		hCompileBas( module, ismain, FALSE, module_count )

		if( fbShouldRestart( ) ) then
			exit sub
		end if

		module = listGetNext( module )
	loop while( module )
end sub

'' -------------------------------------------------------------------------
'' Icon resource generation
'' -------------------------------------------------------------------------

private function hParseXpm _
	( _
		byref xpmfile as string, _
		byref code as string _
	) as integer

	code += !"\ndim shared as zstring ptr "
	code += "fb_program_icon_data"
	code += !"(0 to ...) = _\n{ _\n"

	dim as integer f = freefile( )
	if( open( xpmfile, for input, as #f ) ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, xpmfile, -1 )
		exit function
	end if

	dim as string ln

	'' Check for the header line
	line input #f, ln
	if( ucase( ln ) <> "/* XPM */" ) then
		'' Invalid XPM header
		close #f
		errReportEx( FB_ERRMSG_INVALIDXPMFILE, xpmfile, -1 )
		exit function
	end if

	'' Check for lines containing strings (color and pixel lines)
	'' Other lines (declaration line, empty lines, C comments, ...) aren't
	'' explicitely handled, but should automatically be ignored, as long as
	'' they don't contain strings.
	dim as integer saw_rows = FALSE
	dim as DZSTRING rows
	DZstrZero( rows )
	while( eof( f ) = FALSE )
		line input #f, ln

		'' Strip everything in front of the first '"'
		ln = right( ln, len( ln ) - (instr( ln, """" ) - 1) )

		'' Strip everything behind the second '"'
		ln = left( ln, instr( 2, ln, """" ) )

		'' Got something left?
		if( len( ln ) > 0 ) then
			'' Add an entry to the array, in a new line,
			'' separated by a comma, if it's not the first one.
			if( saw_rows ) then
				DZstrConcatAssign( rows, !", _\n" )
			end if
			DZstrConcatAssign( rows, !"\t@" + ln )
			saw_rows = TRUE
		end if
	wend

	close #f

	if( saw_rows = FALSE ) then
		'' No image data found
		DZstrAllocate( rows, 0 )
		errReportEx( FB_ERRMSG_INVALIDXPMFILE, xpmfile, -1 )
		exit function
	end if

	code += *rows.data
	DZstrAllocate( rows, 0 )

	'' Line break after the last entry
	code += !" _ \n"

	code += !"}\n\n"

	'' Symbol for the gfxlib
	code += !"extern as zstring ptr ptr fb_program_icon alias ""fb_program_icon""\n"
	code += "dim shared as zstring ptr ptr fb_program_icon = " & _
					!"@fb_program_icon_data(0)\n"

	function = TRUE
end function

'' Turns the .xpm icon resource into a .bas file,
'' then compiles that using the normal FB compilation process.
function fbcDriverCompileXpm( ) as integer
	dim as string xpmfile, code
	dim as integer fo = any

	if( len( fbc.xpm.srcfile ) = 0 ) then
		return TRUE
	end if

	'' Remember *.xpm file name
	xpmfile = fbc.xpm.srcfile

	'' Set *.bas name based on input file name or -o <file>:
	if( len( *fbc.xpm.objfile ) > 0 ) then
		fbc.xpm.srcfile = hStripExt( *fbc.xpm.objfile )
	end if

	'' foo.xpm -> foo.xpm.bas to avoid collision with foo.bas
	fbc.xpm.srcfile &= ".bas"

	if( fbc.verbose ) then
		print "parsing xpm: ", xpmfile & " -o " & fbc.xpm.srcfile
	end if

	if( hParseXpm( xpmfile, code ) = FALSE ) then
		exit function
	end if

	fo = freefile( )
	if( open( fbc.xpm.srcfile, for output, as #fo ) ) then
		errReportEx( FB_ERRMSG_FILEACCESSERROR, fbc.xpm.srcfile, -1 )
		exit function
	end if
	print #fo, code;
	close #fo

	'' Clean up the temp .bas if -R wasn't given
	if( fbc.keepasm = FALSE ) then
		fbcAddTemp( fbc.xpm.srcfile )
	end if

	hCompileBas( @fbc.xpm, FALSE, FALSE, -1 )
	function = TRUE
end function

'' A module's stage-two lifecycle is kept in one cleanup-controlled routine.
''
'' -------------------------------------------------------------------------
'' External C and LLVM compilation
'' -------------------------------------------------------------------------

private function hCompileStage2Module( byval module as FBCIOFILE ptr ) as integer
	dim as string ln, asmfile
	dim as integer directtoobj = hCompileStage2DirectlyToObj( )

	asmfile = hGetAsmName( module, 2 )
	if( directtoobj ) then
		asmfile = *module->objfile
	end if

	'' Clean up stage 2 output (the final .asm for -gen gcc/llvm) unless
	'' -RR was given.
	if( (not fbc.keepfinalasm) and _
		((directtoobj = FALSE) or _
		(not fbc.keepobj)) ) then
		fbcAddTemp( asmfile )
	end if

	select case( fbGetOption( FB_COMPOPT_BACKEND ) )
	case FB_BACKEND_GCC, FB_BACKEND_CLANG
		dim as boolean ism64target = false

		if( fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG ) then
			ln += fbcDriverGetClangTargetOption( )
		end if

		dim as integer platformcpuoptions = _
			fbcPlatformAddCCompilerCpuOptions( ln )

		if( platformcpuoptions = FALSE ) then
			select case( fbGetCpuFamily( ) )
			case FB_CPUFAMILY_X86
				ln += "-m32 "
			case FB_CPUFAMILY_X86_64
				ln += "-m64 "
				ism64Target = True
			case FB_CPUFAMILY_AARCH64, FB_CPUFAMILY_PPC64, _
			     FB_CPUFAMILY_PPC64LE, FB_CPUFAMILY_RISCV64, _
			     FB_CPUFAMILY_S390X, FB_CPUFAMILY_LOONGARCH64, _
			     FB_CPUFAMILY_MIPS64, FB_CPUFAMILY_MIPS64EL
				ism64Target = True
			end select

			if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS ) then
				'' GCC doesn't recognize the -march option and PowerPC combination
				'' and recommends the -mcpu option be used for PowerPC.
				select case fbGetCpuFamily( )
				case FB_CPUFAMILY_PPC, FB_CPUFAMILY_PPC64, FB_CPUFAMILY_PPC64LE
					if( fbc.cputype_is_native ) then
						ln += "-mcpu=native "
					else
						ln += "-mcpu=" + *fbGetGccArch( ) + " "
					end if
				case FB_CPUFAMILY_M68K
					'' The m68k family describes the reusable ABI, not a processor
					'' baseline. Platform replacements select a baseline when their
					'' SDK requires one; otherwise GCC's configured default is used.
					if( fbc.cputype_is_native ) then
						ln += "-march=native "
					end if
				case else
					if( fbc.cputype_is_native ) then
						ln += "-march=native "
					else
						ln += "-march=" + *fbGetGccArch( ) + " "
					end if
				end select

				if( fbGetCpuFamily( ) = FB_CPUFAMILY_RISCV32 ) then
					ln += "-mabi=ilp32 "
				elseif( (fbGetCpuFamily( ) = FB_CPUFAMILY_MIPS32) or _
				        (fbGetCpuFamily( ) = FB_CPUFAMILY_MIPS32EL) ) then
					ln += "-mabi=32 "
				elseif( (fbGetCpuFamily( ) = FB_CPUFAMILY_MIPS64) or _
				        (fbGetCpuFamily( ) = FB_CPUFAMILY_MIPS64EL) ) then
					ln += "-mabi=64 "
				end if
			end if
		elseif( fbIs64bit( ) ) then
			ism64Target = TRUE
		end if

		if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ANDROID) and _
		    (fbGetOption( FB_COMPOPT_CPUTYPE ) = FB_CPUTYPE_ARMV7A) ) then
			'' The following options enforce the androideabi-v7a ABI
			'' From https://developer.android.com/ndk/guides/standalone_toolchain.html
			ln += "-mfloat-abi=softfp -mfpu=vfpv3-d16 "
		end if

		fbcDarwinPlatformAddCCompilerOptions( ln )

		if( fbGetOption( FB_COMPOPT_PIC ) ) then
			ln += "-fPIC "
		end if

		if( directtoobj = FALSE ) then

			if( fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG ) then
				if( ((fbGetCpuFamily() = FB_CPUFAMILY_X86) or _
				     (fbGetCpuFamily() = FB_CPUFAMILY_X86_64)) and _
				    (fbGetOption(FB_COMPOPT_ASMSYNTAX) = FB_ASMSYNTAX_INTEL) ) then
					'' Clang's raw inline-asm output restores AT&T syntax even
					'' inside an Intel assembly file. Parse the instructions so
					'' its assembly printer keeps one dialect throughout. GNU as
					'' does not need Clang's address-significance directive.
					ln += "-fno-addrsig "
				else
					'' Other targets retain the external assembler's syntax and
					'' avoid Clang-only assembly directives.
					ln += "-fno-integrated-as "
				end if
			end if

			'' generate assembly
			ln += "-S "

			'' don't use any standard libraries or includes
			ln += "-nostdlib -nostdinc "

			'' enable all warnings
			ln += "-Wall "

			'' -Wno-unused-but-set-variable and the warning it suppresses were introduced
			'' in GCC 4.6. Don't pass that flag to avoid an error on earlier GCC. As a
			'' result, to disable the warning on 4.6+ need to disable all unused warnings...
			'' ln += "-Wno-unused-label -Wno-unused-function -Wno-unused-variable "
			'' ln += "-Wno-unused-but-set-variable "
			ln += "-Wno-unused "

		else
			'' Some clang-based targets do not have a useful external assembler
			'' stage here.  Compile the generated C directly to object code and
			'' skip hAssembleModule() for them.
			ln += "-c -nostdlib -nostdinc -Wall -Wno-unused-label " + _
				"-Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable "
			if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS ) then
				ln += "-Wno-warn-absolute-paths "
			end if

		end if

		'' Don't warn about non-standard main() signature
		'' (we emit "ubyte **argv" instead of "char **argv")
		ln += "-Wno-main "

		'' helps finding ir-hlc bugs
		ln += "-Werror-implicit-function-declaration "

		ln += "-O" + str( fbGetOption( FB_COMPOPT_OPTIMIZELEVEL ) ) + " "

		'' Do not let gcc make assumptions about pointers; FB isn't strict about it.
		ln += "-fno-strict-aliasing "

		'' Ignore .ident directives on win32 targets to prevent identification strings
		'' from accumulating in the final binary (each .ident string from every object
		'' module is added to the final binary even when strings are identical).
		select case as const( fbGetOption( FB_COMPOPT_TARGET ) )
		case FB_COMPTARGET_WIN32
			ln += "-fno-ident "
		end select

		'' The rtlib sets its own rounding mode, don't let gcc make assumptions.
		'' (Should this be skipped for ARM rather than Android?)
		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS andalso _
		    fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_ANDROID ) then
			ln += "-frounding-math "
		end if

		'' ?
		ln += "-fno-math-errno "

		'' Note: we shouldn't use some options like e.g. -ffast-math, because
		'' they cause incompatibilities with the ASM backend. For example:
		''    dim as double d = INF
		''    print d - d
		'' prints -NaN (IND) under the ASM backend because the FPU does the
		'' subtraction, however with the C backend with, gcc -ffast-math
		'' optimizes out the subtraction (even under -O0) and inserts 0 instead.

		'' Define signed integer overflow
		ln += "-fwrapv "

		'' Avoid gcc exception handling bloat
		ln += "-fno-exceptions -fno-asynchronous-unwind-tables "

		'' But enable unwind-tables on x64, these GREATLY increase the accuracy of
		'' debuggers and crash tools across platforms for minimal overhead
		if( (ism64Target = TRUE) or (fbGetOption( FB_COMPOPT_UNWINDINFO ) = TRUE) ) then
			ln += "-funwind-tables "
		else
			ln += "-fno-unwind-tables "
		end if

		'' Newer GCC versions diagnose some const-qualified pointer helper calls
		'' more strictly.  The generated C is an intermediate representation owned
		'' by fbc, so keep that compatibility detail out of user programs.
		ln += "-Wno-incompatible-pointer-types "

		if( fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GCC ) then
			'' GCC 6 added -Wmisleading-indentation to -Wall.  The check is not
			'' useful for machine-generated C and has severe scaling costs in GCC 9
			'' and older when the generated translation unit is large.
			ln += "-Wno-misleading-indentation "
		end if

		if( fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG ) then
			ln += "-Wno-builtin-requires-header "
			'' Clang's uninitialized-variable analysis follows computed error
			'' jumps through generated temporaries. Large BASIC procedures can
			'' spend minutes in this C warning pass. BASIC initialization and ANY
			'' storage are handled before C emission; disable this costly pass.
			'' An explicit -Wc -Wuninitialized can enable it for emitter debugging.
			ln += "-Wno-uninitialized "
		end if

		if( fbGetOption( FB_COMPOPT_DEBUGINFO ) ) then
			ln += "-g "
		end if

		if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
			ln += "-pg "
		end if

		if( fbGetOption( FB_COMPOPT_FPUTYPE ) = FB_FPUTYPE_SSE ) then
			if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ANDROID ) then
				'' Guaranteed to be present
				ln += "-mfpmath=sse -mssse3 "
			else
				ln += "-mfpmath=sse -msse2 "
			end if
		elseif( fbGetOption( FB_COMPOPT_FPUTYPE ) = FB_FPUTYPE_NEON ) then
			'' NEON is not IEEE 754-compliant (except in armv8+), so
			'' gcc will not use it without this.
			ln += "-mfpu=neon -funsafe-math-optimizations "
		end if

		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
			if( fbGetOption( FB_COMPOPT_ASMSYNTAX ) = FB_ASMSYNTAX_INTEL ) then
				ln += "-masm=intel "
			end if
		end select

	case FB_BACKEND_LLVM
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86
			ln += "-march=x86 "
		case FB_CPUFAMILY_X86_64
			ln += "-march=x86-64 "
		case FB_CPUFAMILY_ARM
			ln += "-march=arm "
		case FB_CPUFAMILY_AARCH64
			'' llc selects a target backend here, unlike GCC's instruction
			'' baseline option. Its 64-bit ARM backend is named aarch64.
			ln += "-march=aarch64 "
		case FB_CPUFAMILY_PPC
			ln += "-mcpu=powerpc "
		case FB_CPUFAMILY_PPC64
			ln += "-mcpu=powerpc64 "
		case FB_CPUFAMILY_PPC64LE
			ln += "-mcpu=powerpc64le "
		end select

		if( fbGetOption( FB_COMPOPT_PIC ) ) then
			ln += "-relocation-model=pic "
		end if

		ln += "-O" + str( fbGetOption( FB_COMPOPT_OPTIMIZELEVEL ) ) + " "

		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
			if( fbGetOption( FB_COMPOPT_ASMSYNTAX ) = FB_ASMSYNTAX_INTEL ) then
				ln += "--x86-asm-syntax=intel "
			end if
		end select

	end select

	'' hCompileBas() records the stage-one name before parsing.  Reusing it is
	'' required when a source-level #cmdline changes temporary-file retention.
	if( len( module->asmfile ) > 0 ) then
		ln += """" + module->asmfile + """ "
	else
		ln += """" + hGetAsmName( module, 1 ) + """ "
	end if
	ln += "-o """ + asmfile + """"
	ln += fbc.extopt.gcc

	dim as FBCTOOL ccompiler = FBCTOOL_NONE

	select case( fbGetOption( FB_COMPOPT_BACKEND ) )
	case FB_BACKEND_GCC
		ccompiler = FBCTOOL_GCC
		if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS ) then
			ccompiler = FBCTOOL_EMCC
		end if
		function = fbcRunBin( "compiling C", ccompiler, ln )
	case FB_BACKEND_CLANG
		ccompiler = FBCTOOL_CLANG
		function = fbcRunBin( "compiling C", ccompiler, ln )
	case FB_BACKEND_LLVM
		ccompiler = FBCTOOL_LLC
		function = fbcRunBin( "compiling LLVM IR", ccompiler, ln )
	end select
end function

sub fbcDriverCompileStage2Modules( )
	dim as FBCIOFILE ptr module = listGetHead( @fbc.modules )
	while( module )
		if( hCompileStage2Module( module ) = FALSE ) then
			fbcEnd( 1 )
		end if
		module = listGetNext( module )
	wend
end sub

'' -------------------------------------------------------------------------
'' Assembly and resource compilation
'' -------------------------------------------------------------------------

private function hAssembleModule( byval module as FBCIOFILE ptr ) as integer
	dim as string ln

	dim as FBCTOOL assembler = FBCTOOL_NONE

	if( hCompileStage2DirectlyToObj( ) ) then
		function = TRUE
		exit function
	end if

#ifdef ENABLE_STANDALONE
	if( assembler = FBCTOOL_NONE ) then
		if( fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG ) then
			assembler = FBCTOOL_CLANG
		end if
	end if
#endif

	'' LLVM's Windows assembly can contain unwind directives or unique ctor
	'' sections unavailable in GNU as. Use the producer's integrated assembler.
	if( ((fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_CLANG) or _
	     (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_LLVM)) and _
	    (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WIN32) ) then
		assembler = FBCTOOL_CLANG
	end if

	if( assembler = FBCTOOL_NONE ) then
		select case fbGetOption( FB_COMPOPT_TARGET )
		case FB_COMPTARGET_ANDROID
			'' Current NDK toolchains are clang/LLVM based and do not provide
			'' the old GNU as driver expected by the generic Unix path.
			assembler = FBCTOOL_CLANG
		case FB_COMPTARGET_JS
			'' We will skip assemble stage, since it is
			'' already performed by Emscripten
			'' to re-enable assembly, change to FBCTOOL_EMAS
			assembler = FBCTOOL_NONE
		case else
			'' Some targets require the compiler driver to apply its assembler
			'' specs and platform architecture options.
			if( fbcPlatformUsesCompilerDriverAssembler( ) ) then
				assembler = FBCTOOL_GCC
			else
				assembler = FBCTOOL_AS
			end if
		end select
	end if

	'' Still no assembler selected? Then we are skipping the assembly
	'' and letting the ccompiler generate the object file directly
	if( assembler = FBCTOOL_NONE ) then
		function = TRUE
		exit function
	end if

	select case assembler
	case FBCTOOL_CLANG
		ln += fbcDriverGetClangTargetOption( )
		ln += "-x assembler -c "
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86, FB_CPUFAMILY_X86_64
			if( fbGetOption( FB_COMPOPT_ASMSYNTAX ) = FB_ASMSYNTAX_INTEL ) then
				ln += "-masm=intel "
			end if
		end select
	case FBCTOOL_GCC
		'' The compiler uses an .asm suffix for all stage-two assembly.  GCC
		'' does not infer GNU assembly input from that suffix, so name it.
		ln += "-x assembler -c "
		fbcPlatformAddAssemblerOptions( ln )
	case else
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86
			if (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN) then
				ln += "-arch i386 "
			else
				ln += "--32 "
			endif
		case FB_CPUFAMILY_X86_64
			if (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN) then
				ln += "-arch x86_64 "
			else
				ln += "--64 "
			endif
		end select

		if( fbGetOption( FB_COMPOPT_DEBUGINFO ) = FALSE ) then
			if (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN) then
				if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS ) then
					ln += "--strip-local-absolute "
				end if
			endif
		end if
	end select

	fbcDarwinPlatformAddAssemblerOptions( ln )

	'' GAS consumes the first-stage assembly directly.  Reuse the name chosen
	'' before parsing, because invocation-private object names already contain
	'' the temporary tag and recomputing a stage-two name would append it again.
	'' Other backends create a second-stage assembly file which is the input to
	'' the assembler, so they must retain the traditional stage-two lookup.
	if( (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GAS) or _
		(fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GAS64) ) then
		if( len( module->asmfile ) > 0 ) then
			ln += """" + module->asmfile + """ "
		else
			ln += """" + hGetAsmName( module, 1 ) + """ "
		end if
	else
		ln += """" + hGetAsmName( module, 2 ) + """ "
	end if
	ln += "-o """ + *module->objfile + """"
	ln += fbc.extopt.gas

	if( fbcRunBin( "assembling", assembler, ln ) = FALSE ) then
		exit function
	end if

	'' Clean up the .o if -C wasn't given
	if( fbc.keepobj = FALSE ) then
		fbcAddTemp( *module->objfile )
	end if

	function = TRUE
end function

sub fbcDriverAssembleModules( )
	dim as FBCIOFILE ptr module = listGetHead( @fbc.modules )
	while( module )
		if( hAssembleModule( module ) = FALSE ) then
			fbcEnd( 1 )
		end if
		module = listGetNext( module )
	wend
end sub

private function hAssembleRc( byval rc as FBCIOFILE ptr ) as integer
#ifdef ENABLE_GORC
	'' Using GoRC for the classical native win32 standalone build
	'' Note: GoRC /fo doesn't accept anything except *.obj, not even *.o,
	'' so we need to make it *.obj and then rename it afterwards.

	dim as integer need_rename = FALSE

	'' Ensure to use *.obj so GoRC accepts it
	if( hGetFileExt( *rc->objfile ) <> "obj" ) then
		need_rename = TRUE
		*rc->objfile += ".obj"
	end if

	'' Change the include env var to point to the (hopefully present)
	'' win/rc/*.h headers.
	dim as string oldinclude = trim( environ( "INCLUDE" ) )
	setenviron "INCLUDE=" + fbc.incpath + _
			(FB_HOST_PATHDIV + "win" + FB_HOST_PATHDIV + "rc")

	dim as string ln = "/ni /nw /o "

	if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86_64 ) then
		ln += "/machine X64 "
	end if

	ln &= "/fo """ & *rc->objfile & """"
	ln &= " """ & rc->srcfile & """"

	if( fbcRunBin( "compiling rc", FBCTOOL_GORC, ln ) = FALSE ) then
		exit function
	end if

	'' restore the include env var
	if( len( oldinclude ) > 0 ) then
		setenviron "INCLUDE=" + oldinclude
	end if

	if( need_rename ) then
		dim as string badname = *rc->objfile
		*rc->objfile = hStripExt( *rc->objfile )
		'' Rename back so it will be found by ld/the user
		function = (name( badname, *rc->objfile ) = 0)
	else
		function = TRUE
	end if
#else
	'' Using binutils' windres for all other setups (e.g. cross-compiling
	'' linux -> win32)
	'' Note: windres uses gcc -E to preprocess the .rc by default,
	'' that may not be 100% compatible to GoRC.

	dim as string ln = "--output-format=coff --include-dir=."
	ln += " """ + rc->srcfile + """"
	ln += " """ + *rc->objfile + """"

	function = fbcRunBin( "compiling rc", FBCTOOL_WINDRES, ln )
#endif

	'' Clean up the .o if -C wasn't given
	if( fbc.keepobj = FALSE ) then
		fbcAddTemp( *rc->objfile )
	end if
end function

sub fbcDriverAssembleRcs( )
	'' Compile .rc/.res files
	dim as FBCIOFILE ptr rc = listGetHead( @fbc.rcs )
	while( rc )
		if( hAssembleRc( rc ) = FALSE ) then
			fbcEnd( 1 )
		end if
		rc = listGetNext( rc )
	wend
end sub

sub fbcDriverAssembleXpm( )
	if( len( fbc.xpm.srcfile ) > 0 ) then
		if( fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS ) then
			hCompileStage2Module( @fbc.xpm )
		end if
		if( hAssembleModule( @fbc.xpm ) = FALSE ) then
			fbcEnd( 1 )
		end if
	end if
end sub

'' -------------------------------------------------------------------------
'' Archives and compiler metadata
'' -------------------------------------------------------------------------

private function hGetFbctinfAnchorName( ) as string
	dim as ulongint hash = 2166136261ull

	for i as integer = 1 to len( fbc.outname )
		hash xor= culngint( asc( mid( fbc.outname, i, 1 ) ) )
		hash *= 16777619ull
		hash and= &hFFFFFFFFull
	next

	function = "__fb_ctinf_anchor_" + lcase( hex( cuint( hash ), 8 ) )
end function

private function hCompileFbctinf( byref archiveobjfile as string ) as integer
	dim as FBCIOFILE fbctinf
	dim as string objfile
	dim as integer fo = any

	'' Compile an empty .bas into the fbctinf object file
	'' (it will contain only objinfo)
	if( (fbc.keepasm = FALSE) and (fbc.keepfinalasm = FALSE) ) then
		if( fbcDriverCreateFbctinfDirectory( ) = FALSE ) then
			exit function
		end if
		fbctinf.srcfile = fbc.fbctinfdir + FB_HOST_PATHDIV + FB_INFOSEC_BASNAME
		objfile = fbc.fbctinfdir + FB_HOST_PATHDIV + FB_INFOSEC_OBJNAME
	else
		'' Preserve the historical names when the user requested intermediates.
		fbctinf.srcfile = FB_INFOSEC_BASNAME
		objfile = FB_INFOSEC_OBJNAME
	end if
	fbctinf.objfile = @objfile
	archiveobjfile = objfile

	if( fbc.verbose ) then
		print "creating: ", fbctinf.srcfile
	end if

	'' Create the empty .bas file
	fo = freefile( )
	if( open( fbctinf.srcfile, for output, as #fo ) ) then
		exit function
	end if
	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DARWIN ) then
		''
		'' Apple ar/ranlib warns about archive members with no symbols.
		'' The objinfo member is intentionally metadata-only, so add a
		'' unique, unreferenced symbol to keep Darwin archives quiet.
		''
		print #fo, "sub " + hGetFbctinfAnchorName( ) + "()"
		print #fo, "end sub"
	end if
	close #fo

	'' Clean up the temp .bas if -R wasn't given
	if( fbc.keepasm = FALSE ) then
		fbcAddTemp( fbctinf.srcfile )
	end if

	hCompileBas( @fbctinf, FALSE, TRUE, -1 )
	if( fbGetOption( FB_COMPOPT_BACKEND ) <> FB_BACKEND_GAS ) then
		hCompileStage2Module( @fbctinf )
	end if
	function = hAssembleModule( @fbctinf )
end function

function fbcDriverArchiveFiles( ) as integer
	fbcDriverSetOutName( )

	'' Remove lib*.a if it already exists, because ar doesn't overwrite
	safeKill( fbc.outname )

	dim as string ln = "-rsc " + QUOTE + fbc.outname + (QUOTE + " ")

	if( fbGetOption( FB_COMPOPT_OBJINFO ) and _
		(fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS) and _
		(not fbIsCrossComp( )) ) then
		dim as string archiveobjfile
		dim as integer compiled = hCompileFbctinf( archiveobjfile )
		if( len( archiveobjfile ) > 0 ) then
			'' Also remove a partial object left by a failed assembler.
			fbcAddTemp( archiveobjfile )
		end if
		if( compiled ) then
			'' The objinfo reader expects the fbctinf object to be
			'' the first object file in libraries, so it must be
			'' specified first on the archiver command line:
			ln += QUOTE + archiveobjfile + QUOTE + " "
		end if
	end if

	dim as DZSTRING objects
	DZstrZero( objects )
	dim as string ptr objfile = listGetHead( @fbc.objlist )
	while( objfile )
		DZstrConcatAssign( objects, """" + *objfile + """ " )
		objfile = listGetNext( objfile )
	wend
	if( objects.data <> NULL ) then
		ln += *objects.data
	end if
	DZstrAllocate( objects, 0 )

	var ar = FBCTOOL_AR
	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS ) then
		ar = FBCTOOL_EMAR
	end if

'' See comment in fbcDriverLinkFiles about command line lengths
#if defined( __FB_WIN32__ ) or defined( __FB_DOS__ )
	dim targetprefixlen as ulong
	#ifndef ENABLE_STANDALONE
		targetprefixlen = len( fbc.targetprefix )
	#endif
	dim toolnamelen as integer = len( fbctoolTB( ar ).name + ".exe" ) + _
		iif( targetprefixlen > len( fbc.buildprefix ), targetprefixlen, len( fbc.buildprefix ) )
	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS) or _
		(len( ln ) > (2047 - toolnamelen)) ) then
		if( fbcDriverPutLdArgsIntoFile( ln ) = FALSE ) then
			exit function
		end if
	end if
#endif

	'' invoke ar
	function = fbcRunBin( "archiving", ar, ln )
end function

'' end of driver/fbc-compile.bas
