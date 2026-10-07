'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-help.bas
''
'' Purpose:
''
''     Present compiler usage, version, and build information.
''
'' Responsibilities:
''
''     - print option descriptions with native line endings
''     - report version and compiler configuration
''
'' This file intentionally does NOT contain:
''
''     - option parsing or changes to invocation state
''

#include once "driver/fbc-private.bi"

sub fbcDriverPrintOptions( byval verbose as integer )
	'' Note: must print each line separately to let the rtlib print the
	'' proper line endings even if redirected to file/pipe, hard-coding \n
	'' here isn't enough for DOS/Windows.

	print "usage: fbc [options] <input files>"
	print "input files:"
	print "  *.a = static library, *.so/*.dylib = dynamic library, *.o = object file, *.bas = source"
	print "  *.rc = resource script, *.res = compiled resource (win32)"
	print "  *.xpm = icon resource (*nix/*bsd)"
	print "options:"
	print "  @<file>          Read more command line arguments from a file"
	print "  -a <file>        Treat file as .o/.a input file"
	print "  -arch <type>     Set target architecture (default: 686)"
	print "  -asm att|intel   Set asm format (-gen gcc|llvm, x86 or x86_64 only)"
	print "  -b <file>        Treat file as .bas input file"
	if( verbose ) then
	print "  -buildprefix <name>  specify prefix on tool names (as, ar, ld)"
	end if
	print "  -c               Compile only, do not link"
	print "  -C               Preserve temporary .o files"
	print "  -d <name>[=<val>]  Add a global #define"
	print "  -dll             Same as -dylib"
	print "  -dylib           Create a DLL (win32) or shared library (*nix/*BSD)"
	print "  -e               Enable runtime error checking"

	if( verbose ) then
	print "  -earray          Enable array bounds checking"
	print "  -earraydims      Enable array dimensions checking"
	print "  -eassert         Enable assert() and assertwarn() checking"
	print "  -edebug          Enable __FB_DEBUG__"
	print "  -edebuginfo      Add debug info"
	print "  -elocation       Enable error location reporting"
	print "  -enullptr        Enable null-pointer checking"
	print "  -eunwind         Enable call stack unwind information"
	print "  -entry <name>    Change the entry point of the program from main()"
	end if

	print "  -ex              -e plus RESUME support"
	print "  -exx             -ex plus array bounds/null-pointer checking"
	print "  -export          Export symbols for dynamic linkage"
	print "  -semantic-model <file>  Write a versioned compiler semantic model"
	print "  -semantic-model-bindings <file>  Write bindings and selected implicit calls"
	print "  -semantic-model-compact  Omit verbose macro-expansion provenance from the semantic model"
	print "  -semantic-model-expressions <file>  Write only typed expression ranges"
	print "  -semantic-diagnostics <file>  Write structured diagnostics, including rejected modules"
	if( verbose ) then
	print "  -fbgfx           Link to the appropriate libfbgfx variant (normally automatic)"
	end if
	print "  -gfx3            Select gfxlib3 and define __FB_GFXLIB3__"
	print "  -forcelang <name>  Override #lang statements in source code"
	if( verbose ) then
	print "  -fpmode fast|precise  Select floating-point math accuracy/speed"
	print "  -fpu x87|sse|neon  Set target FPU"
	end if
	print "  -g               Add debug info, enable __FB_DEBUG__, and enable assert()"

	if( verbose ) then
	print "  -gen gas         Select GNU gas 32-bit assembler backend"
	print "  -gen gas64       Select GNU gas 64-bit assembler backend"
	print "  -gen gcc         Select GNU gcc C backend"
	print "  -gen llvm        Select LLVM backend"
	print "  -gen clang       Select clang C backend"
	else
	print "  -gen <backend>   Select code generation backend (gas|gas64|gcc|llvm|clang)"
	end if

	print "  [-]-help         Show this help output; use '-help -v' to show verbose help"
	print "  -i <path>        Add an include file search path"
	print "  -include <file>  Pre-#include a file for each input .bas"
	print "  -l <name>        Link in a library"
	print "  -lang <name>     Select FB dialect: fb, deprecated, fblite, qb"
	print "  -lib             Create a static library"
	print "  -m <name>        Specify main module (default if not -c: first input .bas)"
	print "  -map <file>      Save linking map to file"
	print "  -maxerr <n>      Only show <n> errors"
	print "  -mt              Use thread-safe FB runtime"
	print "  -dos-threads pdmlwp  Enable the optional native DOS thread provider (x87)"
	print "  -nodeflibs       Do not include the default libraries when linking"
	print "  -noerrline       Do not show source context in error messages"
	print "  -nolib <a,b,c>   Do not include the specified libraries when linking"
	print "  -noobjinfo       Do not read/write compile-time info from/to .o and .a files"
	print "  -nostrip         Do not strip symbol information from the output file"
	print "  -o <file>        Set .o (or -pp .bas) file name for prev/next input file"
	print "  -O <value>       Optimization level (default: 0)"
	print "  -p <path>        Add a library search path"
	print "  -pic             Generate position-independent code (Haiku executables, Unix shared libs)"
	print "  -pp              Write out preprocessed input file (.pp.bas) only"
	print "  -prefix <path>   Set the compiler prefix path"
	print "  -print host|target  Display host/target system name"
	print "  -print fblibdir  Display the compiler's lib/ path"
	print "  -print x         Display output binary/library file name (if known)"
	if( verbose ) then
	print "  -print fork-id   Display compiler's fork identifier (if set)"
	print "  -print sha-1     Display compiler's source code commit sha-1 (if known)"
	end if
	print "  -profile         Enable function profiling"
	print "  -profgen         Set the profiling code generation type (gmon|fb|cycles)"
	print "  -r               Write out .asm/.c/.ll (-gen gas/gcc/llvm) only"
	print "  -rr              Write out the final .asm only"
	print "  -R               Preserve temporary .asm/.c/.ll/.def files"
	print "  -RR              Preserve the final .asm file"
	print "  -s console|gui   Select application subsystem"
	print "  -showincludes    Display a tree of file names of #included files"
	print "  -static          Prefer static libraries over dynamic ones when linking"
	print "  -strip           Omit all symbol information from the output file"
	print "  -sysroot <path>  Linker sysroot, needed by some cross-compiling toolchains"
	print "  -t <value>       Set .exe stack size in kbytes, default: 1024 (win32/dos/xbox)"
	if( verbose ) then
	print "  -target <name>   Set cross-compilation target"
	print "                   Examples: win64, linux-x86_64, android, nuttx, riscos, aros-m68k"
	else
	print "  -target <name>   Set cross-compilation target"
	end if
	print "  -title <name>    Set XBE display title (xbox)"
	print "  -v               Be verbose"
	print "  -vec <n>         Automatic vectorization level (default: 0)"
	print "  [-]-version      Show compiler version"
	print "  -w all|pedantic|<n>  Set min warning level: all, pedantic or a value"
	if( verbose ) then
	print "  -w all           Enable all warnings"
	print "  -w none          Disable all warnings"
	print "  -w param         Enable parameter warnings"
	print "  -w escape        Enable string escape sequence warnings"
	print "  -w next          Enable next statement warnings"
	print "  -w signedness    Enable type signedness warnings"
	print "  -w constness     Enable const type warnings"
	print "  -w suffix        Enable invalid suffix warnings"
	print "  -w error         Report warnings as errors"
	print "  -w upcast        Enable warning when up-casting discards initializers"
	end if
	print "  -Wa <a,b,c>      Pass options to 'as'"
	print "  -Wc <a,b,c>      Pass options to 'gcc' (-gen gcc) or 'llc' (-gen llvm)"
	print "  -Wl <a,b,c>      Pass options to 'ld'"
	print "  -x <file>        Set output executable/library file name"

	if( verbose ) then
	print "  -z fbrt          Link with 'fbrt' instead of 'fb' runtime library"
	print "  -z gosub-setjmp  Use setjmp/longjmp to implement GOSUB"
	print "  -z no-thiscall   Don't use '__thiscall' calling convention"
	print "  -z no-fastcall   Don't use '__fastcall' calling convention"
	print "  -z nobuiltins    Disable all non-required builtin procedure definitions"
	print "  -z nocmdline     Disable #cmdline source directives"
	print "  -z optabstract   Only supports optimizing purely abstract types"
	print "  -z retinflts     Enable returning some types in floating point registers"
	print "  -z valist-as-ptr Use pointer expressions to implement CVA_*() macros"
	else
	print "  -z <option>      Extended options (see fbc -help -v)"
	end if

end sub

private sub hAppendConfigInfo( byref config as string, byval info as zstring ptr )
	if( len( config ) > 0 ) then
		config += ", "
	end if
	config += *info
end sub

sub fbcDriverPrintVersion( byval verbose as integer )
	dim as string config
	dim as string version = FB_VERSION
	if( FB_REV > 0 ) then
		version += "-" + str(FB_REV)
	end if

	print "FreeBASIC Compiler - Version " + version + _
		" (" + FB_BUILD_DATE_ISO + "), built for " + fbGetHostId( ) + " (" & fbGetHostBits( ) & "bit)"
	print "Copyright (C) 2004-2025 The FreeBASIC development team."

	#ifdef ENABLE_STANDALONE
		hAppendConfigInfo( config, "standalone" )
	#endif

	#ifdef ENABLE_PREFIX
		hAppendConfigInfo( config, "prefix: '" + ENABLE_PREFIX + "'" )
	#endif

	if( len( config ) > 0 ) then
		print config
	end if

	if( verbose ) then
		fbcPrintTargetInfo( )
		if( FB_BUILD_SHA1 > "" ) then
			print "source sha-1: " & FB_BUILD_SHA1
		end if
		if( FB_BUILD_FORK_ID > "" ) then
			print "fbc fork id:  " & FB_BUILD_FORK_ID
		end if
	end if
end sub

'' end of driver/fbc-help.bas
