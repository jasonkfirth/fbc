'' Project: FreeBASIC compiler driver
'' -----------------------------------------
''
'' File: driver/fbc-link.bas
''
'' Purpose:
''
''     Build link commands and combine module library requirements.
''
'' Responsibilities:
''
''     - collect embedded object metadata and default libraries
''     - select startup objects, library order, and linker flags
''     - link programs and finish platform output formats
''
'' This file intentionally does NOT contain:
''
''     - BASIC parsing or source-to-object compilation
''
'' Resource ownership:
''     Generated definition files are registered with the driver cleanup set.
''     Final program and library outputs are retained for the caller.
''

#include once "driver/fbc-private.bi"

declare sub hAddDarwinFrameworks( byref ldcline as string )

'' -------------------------------------------------------------------------
'' Import libraries
'' -------------------------------------------------------------------------

private function clearDefList(byref deffile as string) as integer
	dim as integer fi = freefile()
	if (open(deffile, for input, as #fi)) then
		return FALSE
	end if

	dim as string cleaned = hStripExt(deffile) + ".clean.def"
	dim as integer fo = freefile()
	if (open(cleaned, for output, as #fo)) then
		close #fi
		return FALSE
	end if

	dim as string ln
	while (eof(fi) = FALSE)
		line input #fi, ln

		if (right(ln, 4) = "DATA") then
			ln = left(ln, len(ln) - 4)
		end if

		print #fo, ln
	wend

	close #fo
	close #fi

	kill(deffile)
	return (name(cleaned, deffile) = 0)
end function

private function hGenerateEmptyDefFile( byref deffile as string ) as integer
	var f = freefile( )
	if( open( deffile, for output, as #f ) ) then
		exit function
	end if

	print #f, "EXPORTS"

	close #f
	function = TRUE
end function

private function makeImpLib _
	( _
		byref dllname as string, _
		byref deffile as string _
	) as integer

	'' for some weird reason, LD will declare all functions exported as if they were
	'' from DATA segment, causing an exception (UPPERCASE'd symbols assumption??)
	if( clearDefList( deffile ) = FALSE ) then
		exit function
	end if

	'' If the .def file is empty (happens if there were no EXPORTs),
	'' then add a single "EXPORTS" line, otherwise dlltool will complain
	'' about a syntax error. (ld --output-def should probably do this
	'' automatically, or dlltool should be fixed, but oh well)
	if( filelen( deffile ) = 0 ) then
		if( hGenerateEmptyDefFile( deffile ) = FALSE ) then
			exit function
		end if
	end if

	dim as string ln
	ln += "--def """ + deffile + """"
	ln += " --dllname """ + hStripPath( fbc.outname ) + """"
	ln += " --output-lib """ + hStripFilename( fbc.outname ) + "lib" + dllname + ".dll.a"""

	if( fbcRunBin( "creating import library", FBCTOOL_DLLTOOL, ln ) = FALSE ) then
		exit function
	end if

	'' Clean up the .def file if -R wasn't given
	if( fbc.keepasm = FALSE ) then
		fbcAddTemp( deffile )
	end if

	function = TRUE
end function

'' Find a library file and wrap it into ""'s for passing it on the ld command
'' line. Or if it couldn't be found, show an error.
private function hFindLib( byval file as zstring ptr ) as string
	dim as string found = fbcBuildPathToLibFile( file )
	if( len( found ) > 0 ) then
		function = " """ + found + """"
	else
		errReportEx( FB_ERRMSG_FILENOTFOUND, file, -1 )
	end if
end function

'' -------------------------------------------------------------------------
'' Linker and target selection
'' -------------------------------------------------------------------------

private function fbcLinkerIsGold( ) as integer
	'' This is needed otherwise it will wrongly pass --version into the linker on Solaris
	'' caused the linker version to be printed everytime we compile with fbc
	if( (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_SOLARIS) or _
	    (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_ILLUMOS) ) then
		return FALSE
	else
		dim ldcmd as string
		fbcFindBin( FBCTOOL_LD, ldcmd )
		ldcmd += " --version"
		return (instr( fbcDriverGet1stOutputLineFromCommand( ldcmd ), "GNU gold" ) > 0)
	end if
end function

'' Check whether we're using the gold linker.
private function fbcIsUsingGoldLinker( ) as integer
	'' gold only supports ELF, we only need to check for it when targetting ELF.
	if( fbTargetSupportsELF( ) ) then
		return fbcLinkerIsGold( )
	end if
	return FALSE
end function

private sub hPrepareLinkTarget _
	( _
		byref ldcline as string, _
		byref xbox_xbe_outname as string, _
		byref wii_dol_outname as string _
	)

	fbcDriverSetOutName( )

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
		if( lcase( right( fbc.outname, 4 ) ) = ".xbe" ) then
			xbox_xbe_outname = fbc.outname
			fbc.outname = hStripExt( fbc.outname ) + ".exe"
		end if
	end if

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_WII ) then
		if( lcase( right( fbc.outname, 4 ) ) = ".dol" ) then
			wii_dol_outname = fbc.outname
			fbc.outname = hStripExt( fbc.outname ) + ".elf"
		end if
	end if

	select case( fbGetOption( FB_COMPOPT_TARGET ) )
	case FB_COMPTARGET_WIN32
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86
			ldcline += "-m i386pe "
		case FB_CPUFAMILY_X86_64
			ldcline += "-m i386pep "
		case FB_CPUFAMILY_AARCH64
			ldcline += "-m arm64pe "
		end select
	case FB_COMPTARGET_LINUX
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86
			ldcline += "-m elf_i386 "
		case FB_CPUFAMILY_X86_64
			ldcline += "-m elf_x86_64 "
		case FB_CPUFAMILY_ARM
			ldcline += "-m armelf_linux_eabi "
		end select
	case FB_COMPTARGET_SOLARIS
		if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86_64 ) then
			'' Solaris ld does not infer the output ELF class from startup
			'' objects or library paths.
			ldcline += "-64 "
		end if
	case FB_COMPTARGET_ILLUMOS
		if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86_64 ) then
			'' illumos uses the Solaris link editor and likewise requires
			'' its 64-bit output mode to be selected explicitly.
			ldcline += "-64 "
		end if
	case FB_COMPTARGET_HAIKU
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86
			ldcline += "-m elf_i386_haiku "
		case FB_CPUFAMILY_X86_64
			ldcline += "-m elf_x86_64_haiku "
		case FB_CPUFAMILY_ARM
			ldcline += "-m armelf_linux_eabi "
		end select
	case FB_COMPTARGET_ANDROID
		if( (len( fbc.sysroot ) = 0) and _
		    (fbGetOption( FB_COMPOPT_BACKEND ) = FB_BACKEND_GCC) ) then
			'' Newer NDKs use clang, which does not support -print-sysroot.
			'' Older GCC toolchains need their link-time sysroot discovered.
			fbc.sysroot = fbcFindSysroot( )
			if( left( fbc.sysroot, 5 ) = "/tmp/" ) then
				errReportWarnEx( FB_WARNINGMSG_MISSINGANDROIDSYSROOT, , 0 )
			end if
		end if

		'' Query the compiler driver because the builtins archive name varies
		'' with the Android target and toolchain.
		var args = ""
#ifndef ENABLE_STANDALONE
		if( len( fbc.target ) > 0 ) then
			args = " -target " & fbc.target
		end if
#endif
		args &= " -print-libgcc-file-name"
		ldcline &= fbcQueryCC( args ) & " "

		if( fbGetOption( FB_COMPOPT_CPUTYPE ) = FB_CPUTYPE_ARMV7A ) then
			ldcline += "--fix-cortex-a8 "
		end if
	case FB_COMPTARGET_DARWIN
		'' The compiler driver supplies native architecture and startup defaults.
		exit select
	case FB_COMPTARGET_WII
		'' The devkitPPC driver supplies newlib and libgcc startup support.
		exit select
	case FB_COMPTARGET_RISCOS
		'' GCCSDK's driver supplies the RISC OS ELF startup objects, UnixLib,
		'' libgcc, and the correct armelf_riscos linker emulation.
		exit select
	case FB_COMPTARGET_AROS
		'' The generated AROS GCC specs supply startup objects, collect-aros,
		'' system libraries, and the architecture's linker emulation.
		exit select
	case FB_COMPTARGET_AMIGA
		'' The Amiga GCC driver selects its native startup and Hunk linker.
		exit select
	case FB_COMPTARGET_WINCE
		'' CeGCC's driver supplies the Windows CE startup object, import
		'' libraries, and PE/COFF linker emulation.
		exit select
	case FB_COMPTARGET_NETBSD
		ldcline += " -rpath /usr/X11R7/lib/ "
		ldcline += " -rpath /usr/pkg/lib/ "
	end select

	'' Set executable name
	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
		ldcline += "-out:" + QUOTE + fbc.outname + QUOTE
	else
		ldcline += "-o " + QUOTE + fbc.outname + QUOTE
	end if
end sub

private function hLinkDosDxe _
	( _
		byref ldcline as string, _
		byref handled as integer _
	) as integer

	handled = FALSE
	if( (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DOS) or _
	    (fbGetOption( FB_COMPOPT_OUTTYPE ) <> FB_OUTTYPE_DYNAMICLIB) ) then
		return TRUE
	end if

	handled = TRUE

	'' DXE modules use an import archive for libfb and leave the remaining
	'' references for the loader to resolve when the module is loaded.
	ldcline += " -I ""lib" + hStripExt( fbc.outname ) + "_il.a"""
	ldcline += " -U"

	scope
		dim as DZSTRING args
		dim as string ptr objfile = listGetHead( @fbc.objlist )
		DZstrZero( args )
		while( objfile )
			DZstrConcatAssign( args, " """ + *objfile + """" )
			objfile = listGetNext( objfile )
		wend
		if( args.data <> NULL ) then
			ldcline += *args.data
		end if
		DZstrAllocate( args, 0 )
	end scope

	scope
		dim as DZSTRING args
		dim as string ptr libfile = listGetHead( @fbc.libfiles )
		DZstrZero( args )
		if( libfile ) then
			ldcline += " -lc"
		end if
		while( libfile )
			DZstrConcatAssign( args, " """ + *libfile + """" )
			libfile = listGetNext( libfile )
		wend
		if( args.data <> NULL ) then
			ldcline += *args.data
		end if
		DZstrAllocate( args, 0 )
	end scope

#ifdef __FB_DOS__
	'' DOS dxe3gen accepts the response-file form used to avoid its short
	'' process command line. Windows-hosted versions do not accept it.
	if( fbcDriverPutLdArgsIntoFile( ldcline ) = FALSE ) then
		return FALSE
	end if
#endif

#ifdef ENABLE_STANDALONE
	dim as string dxepath = environ( "DXE_LD_LIBRARY_PATH" )
	if( dxepath = "" ) then
		setenviron "DXE_LD_LIBRARY_PATH=" + fbc.libpath + FB_HOST_PATHDIV
		if( fbc.verbose ) then
			print "DXE_LD_LIBRARY_PATH=" + fbc.libpath + FB_HOST_PATHDIV
		end if
	end if
#endif

	function = fbcRunBin( "making DXE", FBCTOOL_DXEGEN, ldcline )
end function

private sub hAddUnixDynamicLinker( byref ldcline as string )
	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_FREEBSD
		ldcline += " -dynamic-linker /libexec/ld-elf.so.1"
	case FB_COMPTARGET_DRAGONFLY
		ldcline += " -dynamic-linker /libexec/ld-elf.so.2"
	case FB_COMPTARGET_SOLARIS, FB_COMPTARGET_ILLUMOS
		if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86_64 ) then
			ldcline += " -I /lib/64/ld.so.1"
		else
			ldcline += " -I /lib/ld.so.1"
		end if
	case FB_COMPTARGET_LINUX
		select case( fbGetCpuFamily( ) )
		case FB_CPUFAMILY_X86
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-i386.so.1"
#else
			ldcline += " -dynamic-linker /lib/ld-linux.so.2"
#endif
		case FB_CPUFAMILY_X86_64
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-x86_64.so.1"
#else
			ldcline += " -dynamic-linker /lib64/ld-linux-x86-64.so.2"
#endif
		case FB_CPUFAMILY_ARM
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			if( fbcLinuxPlatformArmUsesHardFloatAbi( ) ) then
				ldcline += " -dynamic-linker /lib/ld-musl-armhf.so.1"
			else
				ldcline += " -dynamic-linker /lib/ld-musl-arm.so.1"
			end if
#else
			if( fbcLinuxPlatformArmUsesHardFloatAbi( ) ) then
				ldcline += " -dynamic-linker /lib/ld-linux-armhf.so.3"
			else
				ldcline += " -dynamic-linker /lib/ld-linux.so.3"
			end if
#endif
		case FB_CPUFAMILY_AARCH64
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-aarch64.so.1"
#else
			ldcline += " -dynamic-linker /lib/ld-linux-aarch64.so.1"
#endif
		case FB_CPUFAMILY_PPC
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-powerpc.so.1"
#else
			ldcline += " -dynamic-linker /lib/ld.so.1"
#endif
		case FB_CPUFAMILY_PPC64
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-powerpc64.so.1"
#else
			ldcline += " -dynamic-linker /lib64/ld64.so.1"
#endif
		case FB_CPUFAMILY_PPC64LE
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-powerpc64le.so.1"
#else
			ldcline += " -dynamic-linker /lib64/ld64.so.2"
#endif
		case FB_CPUFAMILY_RISCV32
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-riscv32.so.1"
#else
			ldcline += " -dynamic-linker /lib/ld-linux-riscv32-ilp32.so.1"
#endif
		case FB_CPUFAMILY_RISCV64
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-riscv64.so.1"
#else
			ldcline += " -dynamic-linker /lib/ld-linux-riscv64-lp64d.so.1"
#endif
		case FB_CPUFAMILY_S390X
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-s390x.so.1"
#else
			ldcline += " -dynamic-linker /lib/ld64.so.1"
#endif
		case FB_CPUFAMILY_LOONGARCH64
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			ldcline += " -dynamic-linker /lib/ld-musl-loongarch64.so.1"
#else
			ldcline += " -dynamic-linker /lib64/ld-linux-loongarch-lp64d.so.1"
#endif
		case FB_CPUFAMILY_MIPS32, FB_CPUFAMILY_MIPS32EL
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			if( fbGetCpuFamily( ) = FB_CPUFAMILY_MIPS32EL ) then
				ldcline += " -dynamic-linker /lib/ld-musl-mipsel.so.1"
			else
				ldcline += " -dynamic-linker /lib/ld-musl-mips.so.1"
			end if
#else
			ldcline += " -dynamic-linker /lib/ld.so.1"
#endif
		case FB_CPUFAMILY_MIPS64, FB_CPUFAMILY_MIPS64EL
#ifdef ENABLE_MUSL_DYNAMIC_LINKER
			if( fbGetCpuFamily( ) = FB_CPUFAMILY_MIPS64EL ) then
				ldcline += " -dynamic-linker /lib/ld-musl-mips64el.so.1"
			else
				ldcline += " -dynamic-linker /lib/ld-musl-mips64.so.1"
			end if
#else
			ldcline += " -dynamic-linker /lib64/ld.so.1"
#endif
		end select
	case FB_COMPTARGET_HAIKU
		'' Haiku executables use the shared-object path. Bind definitions
		'' locally so RIP-relative inline assembly references remain valid.
		ldcline += " -shared -no-undefined -Bsymbolic"
	case FB_COMPTARGET_NETBSD
		ldcline += " -dynamic-linker /usr/libexec/ld.elf_so"
	case FB_COMPTARGET_OPENBSD
		ldcline += " -dynamic-linker /usr/libexec/ld.so"
	case FB_COMPTARGET_ANDROID
		ldcline += " -dynamic-linker /system/bin/linker"
	end select
end sub

private sub hAddUnixLinkOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	)

	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
		dllname = hStripPath( hStripExt( fbc.outname ) )
		ldcline += " -shared -h" + hStripPath( fbc.outname )

		'' Normalize libfoo to foo so the library list can avoid linking a
		'' shared library against its own output.
		if( left( dllname, 3 ) = "lib" ) then
			dllname = right( dllname, len( dllname ) - 3 )
		end if
	else
		hAddUnixDynamicLinker( ldcline )
	end if

	'' Solaris-family linkers do not support GNU --export-dynamic.
	if( ((fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB) or _
	     fbGetOption( FB_COMPOPT_EXPORT )) and _
	    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_SOLARIS) and _
	    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_ILLUMOS) and _
	    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN) ) then
		ldcline += " --export-dynamic"
	end if
	fbcDarwinPlatformAddExportDynamic( ldcline )
end sub

private sub hAddJsLinkOptions( byref ldcline as string )
	dim as integer js_link_optimize = fbGetOption( FB_COMPOPT_OPTIMIZELEVEL )
	if( js_link_optimize < 2 ) then
		'' Asyncify expands reachable control flow. Keeping the final wasm
		'' link at O2 prevents large QB-era programs from exceeding browser
		'' function limits even when BASIC compilation itself uses -O0.
		js_link_optimize = 2
	end if
	ldcline += " -O" + str( js_link_optimize )

	static as zstring*32 emscripten_options(...) = _
	{ _
		"CASE_INSENSITIVE_FS=1", _
		"TOTAL_MEMORY=67108864", _
		"ALLOW_MEMORY_GROWTH=1", _
		"RETAIN_COMPILER_SETTINGS=1", _
		"ASYNCIFY=1", _
		"ASYNCIFY_STACK_SIZE=65536" _
	}

	ldcline += " -Wno-warn-absolute-paths"
	dim as DZSTRING emscripten_args
	DZstrZero( emscripten_args )
	for i as integer = 0 to ubound( emscripten_options )
		DZstrConcatAssign( emscripten_args, " -s " + emscripten_options(i) )
	next
	ldcline += *emscripten_args.data
	DZstrAllocate( emscripten_args, 0 )

	''
	'' Emscripten only consumes a shell template when it is producing HTML.
	'' Supplying one for a JavaScript output is ignored and emits a warning.
	''
	if( hGetFileExt( fbc.outname ) = "html" ) then
		ldcline += " --shell-file" + hFindLib( "fb_shell.html" )
	end if
	ldcline += " --post-js" + hFindLib( "fb_rtlib.js" )
	if( ((len( fbc.subsystem ) = 0) and _
	     (fbGetOption( FB_COMPOPT_MODEVIEW ) = FB_MODEVIEW_CONSOLE)) or _
	    (fbc.subsystem = "console") ) then
		ldcline += " --post-js" + hFindLib( "termlib_min.js" )
	end if
end sub

private sub hAddRiscosLinkOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	)
	''
	'' RISC OS links through GCCSDK's GCC driver.  Options intended for GNU ld
	'' therefore need the -Wl, prefix here.  Shared objects use the ELFLoader and
	'' SOManager model; the final leaf name is also their soname.
	''
	if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
		dllname = hStripPath( hStripExt( fbc.outname ) )
		ldcline += " -shared -Wl,-soname," + hStripPath( fbc.outname )

		'' Keep the library's own name out of the final dependency list.
		if( left( dllname, 3 ) = "lib" ) then
			dllname = right( dllname, len( dllname ) - 3 )
		end if
	end if

	if( (fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB) or _
	    fbGetOption( FB_COMPOPT_EXPORT ) ) then
		ldcline += " -Wl,--export-dynamic"
	end if
end sub

private function hAddPlatformLinkOptions _
	( _
		byref ldcline as string, _
		byref dllname as string _
	) as integer

	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_CYGWIN
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			dllname = hStripPath( hStripExt( fbc.outname ) )
			ldcline += " --shared -e _cygwin_dll_entry --enable-auto-image-base --dll-search-prefix=cyg"
		else
			if( len( fbc.subsystem ) = 0 ) then
				if( fbGetOption( FB_COMPOPT_MODEVIEW ) = FB_MODEVIEW_GUI ) then
					fbc.subsystem = "windows"
				else
					fbc.subsystem = "console"
				end if
			elseif( fbc.subsystem = "gui" ) then
				fbc.subsystem = "windows"
			end if
			ldcline += " -subsystem " + fbc.subsystem
		end if

	case FB_COMPTARGET_WIN32
		if( len( fbc.subsystem ) = 0 ) then
			if( fbGetOption( FB_COMPOPT_MODEVIEW ) = FB_MODEVIEW_GUI ) then
				fbc.subsystem = "windows"
			else
				fbc.subsystem = "console"
			end if
		elseif( fbc.subsystem = "gui" ) then
			fbc.subsystem = "windows"
		end if
		ldcline += " -subsystem " + fbc.subsystem

		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			dllname = hStripPath( hStripExt( fbc.outname ) )
			ldcline += " --dll --enable-stdcall-fixup"
			if( fbGetCpuFamily( ) = FB_CPUFAMILY_X86 ) then
				ldcline += " -e _DllMainCRTStartup@12"
			else
				ldcline += " -e DllMainCRTStartup"
			end if
		end if

	case FB_COMPTARGET_DARWIN
		if( fbcDarwinPlatformAddDynamicLibOptions( ldcline, dllname ) = FALSE ) then
			return FALSE
		end if

	case FB_COMPTARGET_LINUX, FB_COMPTARGET_HAIKU, _
	     FB_COMPTARGET_FREEBSD, FB_COMPTARGET_OPENBSD, _
	     FB_COMPTARGET_NETBSD, FB_COMPTARGET_DRAGONFLY, _
	     FB_COMPTARGET_SOLARIS, FB_COMPTARGET_ILLUMOS, _
	     FB_COMPTARGET_ANDROID
		hAddUnixLinkOptions( ldcline, dllname )

	case FB_COMPTARGET_XBOX
		'' nxdk-link uses lld's MSVC-compatible frontend.
		ldcline += " -safeseh:no"
		ldcline += " -include:_automount_d_drive"

	case FB_COMPTARGET_JS
		hAddJsLinkOptions( ldcline )

	case FB_COMPTARGET_RISCOS
		hAddRiscosLinkOptions( ldcline, dllname )

	case FB_COMPTARGET_WINCE
		fbcWincePlatformAddLinkOptions( ldcline, dllname )
	case FB_COMPTARGET_AMIGA
		fbcAmigaPlatformAddLinkOptions( ldcline )
	end select

	function = TRUE
end function

'' -------------------------------------------------------------------------
'' Link command construction
'' -------------------------------------------------------------------------

private sub hAddGeneralLinkOptions _
	( _
		byref ldcline as string, _
		byref deffile as string _
	)

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS ) then
		'' DJGPP needs fbc's script to order runtime constructors and
		'' destructors around the program's own initialization.
		ldcline += " -T """ + fbc.libpath + (FB_HOST_PATHDIV + "i386go32.x""")
	else
		'' GNU ld can discard object-info metadata through a supplementary
		'' script. Gold, lld, and the non-GNU target linkers cannot use it.
		if( fbGetOption( FB_COMPOPT_OBJINFO ) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_SOLARIS) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_ILLUMOS) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_WII) and _
		    fbcPlatformSupportsSupplementaryLinkerScript( ) and _
		    (fbcPlatformGetLinkerTool( ) = FBCTOOL_LD) and _
		    (not fbcUseLldLinker( )) and _
		    (not fbcIsUsingGoldLinker( )) ) then
			ldcline += " -T """ + fbc.libpath + (FB_HOST_PATHDIV + "fbextra.x""")
		end if
	end if

	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_CYGWIN, FB_COMPTARGET_WIN32
		dim as integer stacksize = fbGetOption( FB_COMPOPT_STACKSIZE )
		ldcline += " --stack " + str( stacksize ) + "," + str( stacksize )

		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			'' The generated definition file is converted to the import
			'' archive after the DLL has linked successfully.
			deffile = hStripExt( fbc.outname ) + ".def"
			ldcline += " --output-def """ + deffile + """"
		end if

	case FB_COMPTARGET_XBOX
		ldcline += " -entry:WinMainCRTStartup"
		'' nxdk-link's 64 KiB default is too small for programs with large
		'' local buffers, so use the compiler's normal stack setting.
		ldcline += " -stack:" + str( fbGetOption( FB_COMPOPT_STACKSIZE ) )

	case FB_COMPTARGET_OPENBSD
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE ) then
			ldcline += " -pie -e __start"
		end if
	end select

	if( fbc.staticlink and _
	    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX) ) then
		if( fbcPlatformGetLinkerTool( ) = FBCTOOL_GCC ) then
			'' These commands are passed to the target GCC driver, not raw ld.
			ldcline += " -static"
		else
			ldcline += " -Bstatic"
		end if
	end if

	if( fbGetOption( FB_COMPOPT_PIC ) and _
	    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX) ) then
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE ) then
			ldcline += " -pie"
		end if
	end if

	if( len( fbc.mapfile ) > 0 ) then
		if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
			ldcline += " -map:" + fbc.mapfile
		elseif( fbcPlatformGetLinkerTool( ) = FBCTOOL_GCC ) then
			ldcline += " -Wl,-Map," + fbc.mapfile
		else
			ldcline += " -Map " + fbc.mapfile
		end if
	end if

	if( fbGetOption( FB_COMPOPT_DEBUGINFO ) = FALSE ) then
		if( fbGetOption( FB_COMPOPT_PROFILE ) <> FB_PROFILE_OPT_GMON ) then
			if( (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN) and _
			    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS) ) then
				if( fbc.stripsymbols and _
				    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX) ) then
					ldcline += " -s"
				end if
			end if
		end if
	end if
end sub

private sub hAddLinkSearchPaths( byref ldcline as string )
	scope
		dim as DZSTRING args
		dim as TSTRSETITEM ptr i = listGetHead( @fbc.finallibpaths.list )
		DZstrZero( args )

		dim as string option_prefix
		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS ) then
			option_prefix = " -L """
		else
			option_prefix = " -L"""
		end if

		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX ) then
			while( i )
				DZstrConcatAssign( args, option_prefix + i->s + """" )
				i = listGetNext( i )
			wend
		end if
		if( args.data <> NULL ) then
			ldcline += *args.data
		end if
		DZstrAllocate( args, 0 )
	end scope

	if( len( fbc.sysroot ) ) then
		ldcline += " --sysroot=" + fbc.sysroot
	end if
end sub

private sub hAddCrtBeginObjects( byref ldcline as string )
	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_CYGWIN
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			ldcline += hFindLib( "crtbeginS.o" )
		else
			'' Cygwin needs crt0.o for normal program initialization.
			ldcline += hFindLib( "crt0.o" )
			if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
				ldcline += hFindLib( "gcrt0.o" )
			end if
		end if

	case FB_COMPTARGET_WIN32
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			ldcline += hFindLib( "dllcrt2.o" )
		else
			ldcline += hFindLib( "crt2.o" )
			if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
				ldcline += hFindLib( "gcrt2.o" )
			end if
		end if
		ldcline += hFindLib( "crtbegin.o" )

	case FB_COMPTARGET_DOS
		if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
			ldcline += hFindLib( "gcrt0.o" )
		else
			ldcline += hFindLib( "crt0.o" )
		end if

	case FB_COMPTARGET_LINUX, FB_COMPTARGET_HAIKU, FB_COMPTARGET_DARWIN, _
	     FB_COMPTARGET_FREEBSD, FB_COMPTARGET_OPENBSD, _
	     FB_COMPTARGET_NETBSD, FB_COMPTARGET_DRAGONFLY, _
	     FB_COMPTARGET_SOLARIS, FB_COMPTARGET_ILLUMOS

		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE ) then
			if( fbGetOption( FB_COMPOPT_PROFILE ) ) then
				select case as const fbGetOption( FB_COMPOPT_TARGET )
				case FB_COMPTARGET_OPENBSD, FB_COMPTARGET_NETBSD
					ldcline += hFindLib( "gcrt0.o" )
				case FB_COMPTARGET_DARWIN
					'' The Darwin compiler driver supplies startup objects.
					exit select
				case FB_COMPTARGET_HAIKU
					'' Haiku has no gcrt1.o.
					exit select
				case else
					ldcline += hFindLib( "gcrt1.o" )
				end select
			else
				select case as const fbGetOption( FB_COMPOPT_TARGET )
				case FB_COMPTARGET_OPENBSD, FB_COMPTARGET_NETBSD
					ldcline += hFindLib( "crt0.o" )
				case FB_COMPTARGET_DARWIN
					'' The Darwin compiler driver supplies startup objects.
					exit select
				case FB_COMPTARGET_HAIKU
					'' Haiku has no crt1.o.
					exit select
				case else
					ldcline += hFindLib( "crt1.o" )
				end select
			end if
		end if

		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN ) then
			if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_OPENBSD ) then
				ldcline += hFindLib( "crti.o" )
			end if

			select case as const fbGetOption( FB_COMPOPT_TARGET )
			case FB_COMPTARGET_OPENBSD
				ldcline += hFindLib( "crtbegin.o" )
			case FB_COMPTARGET_HAIKU
				ldcline += hFindLib( "crtbeginS.o" )
				ldcline += hFindLib( "start_dyn.o" )
				ldcline += hFindLib( "init_term_dyn.o" )
			case else
				if( fbGetOption( FB_COMPOPT_PIC ) ) then
					ldcline += hFindLib( "crtbeginS.o" )
				else
					ldcline += hFindLib( "crtbegin.o" )
				end if
			end select
		end if

	case FB_COMPTARGET_ANDROID
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE ) then
			if( fbc.staticlink ) then
				ldcline += hFindLib( "crtbegin_static.o" )
			else
				ldcline += hFindLib( "crtbegin_dynamic.o" )
			end if
		else
			ldcline += hFindLib( "crtbegin_so.o" )
		end if

	case FB_COMPTARGET_XBOX
		'' nxdk's CRT startup object is supplied by libpdclib.lib.
		exit select

	case FB_COMPTARGET_WINCE
		fbcWincePlatformAddCrtBeginObjects( ldcline )
	end select
end sub

private sub hAddRuntimeAndObjectFiles( byref ldcline as string )
	if( fbc.nofbrt0 = FALSE ) then
		'' Emscripten does not use fbrt0 because it does not support the
		'' constructor/destructor scheme used by the native runtimes.
		if( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS ) then
			ldcline += " """ + fbc.libpath + FB_HOST_PATHDIV
			select case fbGetOption( FB_COMPOPT_PROFILE )
			case FB_PROFILE_OPT_CALLS
				if( fbGetOption( FB_COMPOPT_PIC ) ) then
					ldcline += "fbrt1pic.o"
				else
					ldcline += "fbrt1.o"
				end if
			case FB_PROFILE_OPT_CYCLES
				if( fbGetOption( FB_COMPOPT_PIC ) ) then
					ldcline += "fbrt2pic.o"
				else
					ldcline += "fbrt2.o"
				end if
			case else
				if( fbGetOption( FB_COMPOPT_PIC ) ) then
					ldcline += "fbrt0pic.o"
				else
					ldcline += "fbrt0.o"
				end if
			end select
			ldcline += """"
		end if
	end if

	scope
		dim as DZSTRING args
		dim as string ptr objfile = listGetHead( @fbc.objlist )
		DZstrZero( args )
		while( objfile )
			DZstrConcatAssign( args, " """ + *objfile + """" )
			objfile = listGetNext( objfile )
		wend
		if( args.data <> NULL ) then
			ldcline += *args.data
		end if
		DZstrAllocate( args, 0 )
	end scope
end sub

private function hRunLinkCommand( byref ldcline as string ) as integer
	'' DOS process command lines are limited to 127 characters. Windows
	'' also needs a response file when cmd.exe's 2047-character legacy
	'' limit may be reached, or when DOS/JS cross tools require one.
#ifdef __FB_DOS__
	if( fbcDriverPutLdArgsIntoFile( ldcline ) = FALSE ) then
		return FALSE
	end if
#elseif defined( __FB_WIN32__ )
	dim as long forcefile
	dim as ulong targetprefixlen
#ifdef ENABLE_STANDALONE
	select case fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_DOS, FB_COMPTARGET_JS
		forcefile = TRUE
	end select
#else
	targetprefixlen = len( fbc.targetprefix )
#endif
	dim as integer toolnamelen = len( "ld.exe " ) + _
		iif( targetprefixlen > len( fbc.buildprefix ), _
		     targetprefixlen, len( fbc.buildprefix ) )
	if( forcefile or _
	    (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS) or _
	    (len( ldcline ) > (2047 - toolnamelen)) ) then
		if( fbcDriverPutLdArgsIntoFile( ldcline ) = FALSE ) then
			return FALSE
		end if
	end if
#endif

	var ld = fbcPlatformGetLinkerTool( )
	if( fbcRunBin( "linking", ld, ldcline ) = FALSE ) then
		return FALSE
	end if

	function = fbcDarwinPlatformBuildGuiAppBundle( )
end function

private function hFinishLinkedOutput _
	( _
		byref dllname as string, _
		byref deffile as string, _
		byref xbox_xbe_outname as string, _
		byref wii_dol_outname as string _
	) as integer

	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_DOS
		'' DJGPP stores the requested stack size in the executable header.
		dim as integer f = freefile( )
		if( open( fbc.outname, for binary, access read write, as #f ) <> 0 ) then
			return FALSE
		end if

		dim as long value = clng( fbGetOption( FB_COMPOPT_STACKSIZE ) )
		put #f, 533, value
		close #f

	case FB_COMPTARGET_CYGWIN, FB_COMPTARGET_WIN32
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			if( makeImpLib( dllname, deffile ) = FALSE ) then
				return FALSE
			end if
		end if

	case FB_COMPTARGET_XBOX
		'' cxbe converts the linked PE executable into the Xbox XBE image.
		dim as string cxbepath, cxbecline
		if( len( xbox_xbe_outname ) = 0 ) then
			xbox_xbe_outname = hStripExt( fbc.outname ) + ".xbe"
		end if
		if( len( fbc.xbe_title ) = 0 ) then
			fbc.xbe_title = hStripPath( hStripExt( xbox_xbe_outname ) )
		end if

		cxbecline = "-TITLE:" + QUOTE + fbc.xbe_title + (QUOTE + " ")
		if( fbGetOption( FB_COMPOPT_DEBUGINFO ) ) then
			cxbecline += "-DUMPINFO:" + QUOTE + _
				hStripExt( xbox_xbe_outname ) + (".cxbe" + QUOTE)
		end if
		cxbecline += " -OUT:" + QUOTE + xbox_xbe_outname + QUOTE
		cxbecline += " " + QUOTE + fbc.outname + QUOTE
		if( fbc.verbose = FALSE ) then
			cxbecline += " >nul"
		end if
		if( fbc.verbose ) then
			print "cxbe: ", cxbecline
		end if

		fbcFindBin( FBCTOOL_CXBE, cxbepath )
		'' Shell redirection suppresses cxbe output in non-verbose mode.
		dim as integer res = shell( cxbepath + " " + cxbecline )
		if( res <> 0 ) then
			if( fbc.verbose ) then
				print "cxbe failed: exit code " & res
			end if
			return FALSE
		end if
		if( kill( fbc.outname ) <> 0 ) then
			return FALSE
		end if

	case FB_COMPTARGET_WII
		'' elf2dol converts the linked ELF into the bootable Wii image.
		if( len( wii_dol_outname ) = 0 ) then
			wii_dol_outname = hStripExt( fbc.outname ) + ".dol"
		end if
		if( fbcRunBin( "making DOL", FBCTOOL_ELF2DOL, _
		               QUOTE + fbc.outname + QUOTE + " " + _
		               QUOTE + wii_dol_outname + QUOTE ) = FALSE ) then
			return FALSE
		end if
		if( kill( fbc.outname ) <> 0 ) then
			return FALSE
		end if

	case FB_COMPTARGET_RISCOS
		if( fbcRiscosHostFinishExecutable( ) = FALSE ) then
			return FALSE
		end if

	case FB_COMPTARGET_AROS
		if( fbcArosHostFinishExecutable( ) = FALSE ) then
			return FALSE
		end if
	end select

	function = TRUE
end function

'' Link command construction is ordered and shares target state throughout.
''
'' -------------------------------------------------------------------------
'' Link phase
'' -------------------------------------------------------------------------

function fbcDriverLinkFiles( ) as integer
	dim as string ldcline, dllname, deffile
	dim as string xbox_xbe_outname
	dim as string wii_dol_outname

	function = FALSE

	hPrepareLinkTarget( ldcline, xbox_xbe_outname, wii_dol_outname )

	dim as integer link_was_handled = any
	if( hLinkDosDxe( ldcline, link_was_handled ) = FALSE ) then
		exit function
	end if
	if( link_was_handled ) then
		function = TRUE
		exit function
	end if

	if( hAddPlatformLinkOptions( ldcline, dllname ) = FALSE ) then
		exit function
	end if

	hAddGeneralLinkOptions( ldcline, deffile )

	hAddLinkSearchPaths( ldcline )

	hAddCrtBeginObjects( ldcline )

	hAddRuntimeAndObjectFiles( ldcline )

	dim as integer addsolarislibmearly = FALSE
	if( fbc.nodeflibs = FALSE ) then
		select case as const fbGetOption( FB_COMPOPT_TARGET )
		case FB_COMPTARGET_SOLARIS, FB_COMPTARGET_ILLUMOS
			'' Solaris libc exports some libm entry points too.  If a user
			'' library appears before -lm and causes libc to be loaded first,
			'' calls such as pow() can bind to libc's older entry point instead
			'' of libm's.  Keep the default math library early in the group so
			'' the runtime behavior matches GCC's normal Solaris link order.
			scope
				dim as TSTRSETITEM ptr i = listGetHead(@fbc.finallibs.list)
				while (i)
					if( i->s = "m" ) then
						addsolarislibmearly = TRUE
						exit while
					end if
					i = listGetNext(i)
				wend
			end scope
		end select
	end if

	'' Begin of lib group
	'' All libraries are passed inside -( -) so we don't need to worry as
	'' much about their order and/or listing them repeatedly. (Not supported by Darwin ld)
	if ( fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN ) then
		if( (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX) ) then
			if( fbcPlatformGetLinkerTool( ) = FBCTOOL_GCC ) then
				ldcline += " -Wl,--start-group"
			elseif( fbcUseLldLinker( ) ) then
				ldcline += " --start-group"
			else
				ldcline += " ""-("""
			end if
		end if
	end if

	if( addsolarislibmearly ) then
		ldcline += " -lm"
	end if

	'' Add libraries passed by file name
	scope
		dim as DZSTRING args
		dim as string ptr libfile = listGetHead(@fbc.libfiles)
		DZstrZero( args )
		while (libfile)
			DZstrConcatAssign( args, " """ + *libfile + """" )
			libfile = listGetNext(libfile)
		wend
		if( args.data <> NULL ) then
			ldcline += *args.data
		end if
		DZstrAllocate( args, 0 )
	end scope

	'' Add libraries from command-line, those found during parsing, and
	'' the default ones
	scope
		dim as DZSTRING args
		dim as TSTRSETITEM ptr i = listGetHead(@fbc.finallibs.list)
		dim as integer checkdllname = (fbGetOption(FB_COMPOPT_OUTTYPE) = FB_OUTTYPE_DYNAMICLIB)
		DZstrZero( args )
		while (i)
			'' Prevent linking DLLs against their own import library,
			'' or .so's against themselves (ld will fail to read in
			'' its output file...)
			if ((checkdllname = FALSE) orelse (i->s <> dllname)) then
				if( addsolarislibmearly andalso (i->s = "m") ) then
					'' Already emitted before user libraries, see above.
				elseif( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
					fbcDriverAddXboxLibArchive( ldcline, i->s )
				else
					dim as string libname = i->s
					dim as string frameworkname = _
						fbcDarwinPlatformGetFrameworkName( libname )

					if( len( frameworkname ) > 0 ) then
						DZstrConcatAssign( args, " -framework " + frameworkname )
					else
						libname = fbcPlatformMapLibName( libname )
						DZstrConcatAssign( args, " -l" + libname )
					end if
				end if
			end if
			i = listGetNext(i)
		wend
		if( args.data <> NULL ) then
			ldcline += *args.data
		end if
		DZstrAllocate( args, 0 )
	end scope

	hAddDarwinFrameworks( ldcline )
	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX ) then
		fbcDriverAddXboxNxdkLibs( ldcline )
	end if

	if (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_DARWIN) then
		if( (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_JS) and _
		    (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_XBOX) ) then
			'' End of lib group
			if( fbcPlatformGetLinkerTool( ) = FBCTOOL_GCC ) then
				ldcline += " -Wl,--end-group"
			elseif( fbcUseLldLinker( ) ) then
				ldcline += " --end-group"
			else
				ldcline += " ""-)"""
			end if
		else
			if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS ) then
				ldcline += " -lfb"
			end if
		end if
	end if

	'' crt end
	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_CYGWIN
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_DYNAMICLIB ) then
			ldcline += hFindLib( "crtend.o" )
		end if

	case FB_COMPTARGET_LINUX, FB_COMPTARGET_FREEBSD, _
		FB_COMPTARGET_OPENBSD, FB_COMPTARGET_NETBSD, _
		FB_COMPTARGET_DRAGONFLY, FB_COMPTARGET_SOLARIS, _
		FB_COMPTARGET_ILLUMOS
		if( fbGetOption( FB_COMPOPT_PIC ) ) then
			ldcline += hFindLib( "crtendS.o" )
		else
			ldcline += hFindLib( "crtend.o" )
		end if
		if (fbGetOption( FB_COMPOPT_TARGET ) <> FB_COMPTARGET_OPENBSD) then
			ldcline += hFindLib( "crtn.o" )
		end if

	case FB_COMPTARGET_ANDROID
		if( fbGetOption( FB_COMPOPT_OUTTYPE ) = FB_OUTTYPE_EXECUTABLE) then
			ldcline += hFindLib( "crtend_android.o" )
		else
			'' FB_OUTTYPE_DYNAMICLIB
			ldcline += hFindLib( "crtend_so.o" )
		end if

	case FB_COMPTARGET_HAIKU
		ldcline += hFindLib( "crtn.o" )
		ldcline += hFindLib( "crtendS.o" )

	case FB_COMPTARGET_WIN32
		ldcline += hFindLib( "crtend.o" )

	end select

	fbcDarwinPlatformAddCompilerDriverLinkerOptions( ldcline )

	'' This is required for 64-bit modules on *nix-y platforms
	'' for the unwind tables to have any effect
	'' Windows doesn't need this option
	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_LINUX, FB_COMPTARGET_HAIKU, FB_COMPTARGET_FREEBSD, _
		FB_COMPTARGET_OPENBSD, FB_COMPTARGET_NETBSD, _
		FB_COMPTARGET_DRAGONFLY
		dim as long outtype = fbGetOption( FB_COMPOPT_OUTTYPE )
		if outtype = FB_OUTTYPE_EXECUTABLE OrElse outtype = FB_OUTTYPE_DYNAMICLIB Then
			dim as long cpufamily = fbGetCpuFamily( )
			if cpufamily = FB_CPUFAMILY_X86_64 OrElse cpufamily = FB_CPUFAMILY_AARCH64 OrElse _
				cpuFamily = FB_CPUFAMILY_PPC64 OrElse cpufamily = FB_CPUFAMILY_PPC64LE OrElse _
				cpufamily = FB_CPUFAMILY_MIPS64 OrElse cpufamily = FB_CPUFAMILY_MIPS64EL Then
				ldcline += " --eh-frame-hdr"
			end if
		end if
	end select

	if( fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS ) then
		''
		'' Keep the wasm link optimized, but do not ask Emscripten to run
		'' its optional HTML/JavaScript minifier afterwards.  The current
		'' Emscripten setting is used instead of --minify 0 because that
		'' option selects debug level 1 rather than disabling HTML minification.
		''
		ldcline += " -s MINIFY_HTML=0"
	end if

	'' extra options
	ldcline += " " + fbc.extopt.ld

	if( hRunLinkCommand( ldcline ) = FALSE ) then
		exit function
	end if

	if( hFinishLinkedOutput( dllname, deffile, xbox_xbe_outname, _
	                         wii_dol_outname ) = FALSE ) then
		exit function
	end if

	function = TRUE

end function

'' -------------------------------------------------------------------------
'' Module object metadata
'' -------------------------------------------------------------------------

private sub hReadObjinfo( )
	dim as string dat
	dim as integer lang = any

	#macro hReportErr( num )
		errReportWarnEx( num, objinfoGetFilename( ), -1 )
	#endmacro

	do
		select case as const( objinfoReadNext( dat ) )
		case OBJINFO_LIB
			strsetAdd( @fbc.finallibs, dat, FALSE )
			if( dat = "sfx" OR dat = "sfxpic" OR dat = "sfxmt" OR dat = "sfxmtpic" ) then
				fbSetOption( FB_COMPOPT_FBSFX, TRUE )
			end if

		case OBJINFO_LIBPATH
			strsetAdd( @fbc.finallibpaths, dat, FALSE )

		case OBJINFO_MT
			if( fbc.objinf.mt = FALSE ) then
				hReportErr( FB_WARNINGMSG_MIXINGMTMODES )

				fbc.objinf.mt = TRUE
				fbSetOption( FB_COMPOPT_MULTITHREADED, TRUE )
			end if

		case OBJINFO_GFX
			if( fbGetOption( FB_COMPOPT_FBGFX ) = FB_GFXLIB_NONE ) then
				fbSetOption( FB_COMPOPT_FBGFX, FB_GFXLIB_DEFAULT )
			end if

		case OBJINFO_GFX3
			fbSetOption( FB_COMPOPT_FBGFX, FB_GFXLIB_GFX3 )

		case OBJINFO_LANG
			lang = fbGetLangId( dat )

			'' bad objinfo value?
			if( lang = FB_LANG_INVALID ) then
				lang = FB_LANG_FB
			end if

			if( lang <> fbc.objinf.lang ) then
				hReportErr( FB_WARNINGMSG_MIXINGLANGMODES )
				fbc.objinf.lang = lang
				fbSetOption( FB_COMPOPT_LANG, lang )
			end if

		case OBJINFO_SFX
			fbSetOption( FB_COMPOPT_FBSFX, TRUE )

		case else
			exit do
		end select
	loop

	objinfoReadEnd( )
end sub

sub fbcDriverCollectObjinfo( )
	dim as string ptr s = any
	dim as TSTRSETITEM ptr i = any

	'' for each object passed in the cmd-line
	s = listGetHead( @fbc.objlist )
	while( s )
		objinfoReadObj( *s )
		hReadObjinfo( )
		s = listGetNext( s )
	wend

	'' for each library found (must be done after processing all objects)
	i = listGetHead( @fbc.finallibs.list )
	while( i )
		'' Not default?
		if( i->userdata = FALSE ) then
			objinfoReadLib( i->s, @fbc.finallibpaths.list )
			hReadObjinfo( )
		end if
		i = listGetNext( i )
	wend

	'' Search libs given as *.a input files instead of -l or #inclib
	s = listGetHead( @fbc.libfiles )
	while( s )
		'' Compile-time metadata is stored in static/import archives.  A
		'' dynamically linked library is a linker input only and is not an
		'' ar archive that can contain .fbctinf data.
		if( hGetFileExt( *s ) = "a" ) then
			objinfoReadLibfile( *s )
		end if
		hReadObjinfo( )
		s = listGetNext( s )
	wend
end sub

'' -------------------------------------------------------------------------
'' Default libraries and search paths
'' -------------------------------------------------------------------------

sub fbcDriverSetDefaultLibPaths( )
	'' compiler's lib/
	fbcAddDefLibPath( fbc.libpath )

	'' and the current path
	fbcAddDefLibPath( "." )

#ifndef ENABLE_STANDALONE
	'' Add gcc's private lib directory, to find libgcc
	'' This is for installing into Unix-like systems, and not for
	'' standalone, which has libgcc in the main lib/.
	select case fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_ANDROID
		'' We don't pass -lgcc to ld (it's probably not called libgcc)
		exit select
	case FB_COMPTARGET_JS
		'' We let emcc handle linking
		exit select
	case else
		fbcAddLibPathFor( "libgcc.a" )

		#ifndef DISABLE_STDCXX_PATH
			'' we don't specifically need c++, but for some users that do want to
			'' interop with c++ and to allow some of the tests/cpp tests to pass
			'' it's helpful to also query for a c++ library.
			select case fbGetOption( FB_COMPOPT_TARGET )
			case FB_COMPTARGET_FREEBSD
				fbcAddLibPathFor( "libc++.so" )
			case FB_COMPTARGET_DOS
				fbcAddLibPathFor( "libstdcxx.a" )
			case else
				fbcAddLibPathFor( "libstdc++.so" )
			end select
		#endif
	end select

	fbcPlatformAddDefaultLibPaths( )
#endif
end sub

sub fbcAddDefLib(byval libname as zstring ptr)
	strsetAdd(@fbc.finallibs, *libname, TRUE)
end sub

private function hGetFbLibNameSuffix( ) as string
	dim s as string
	dim as integer target = fbGetOption( FB_COMPOPT_TARGET )
	if( fbGetOption( FB_COMPOPT_MULTITHREADED ) ) then
		s += "mt"
	end if
	if( target = FB_COMPTARGET_OPENBSD ) then
		s += "pic"
	elseif( fbGetOption( FB_COMPOPT_PIC ) ) then
		s += "pic"
	end if
	function = s
end function

private sub hAddDarwinFrameworks( byref ldcline as string )
	fbcPlatformAddLinkerFrameworks( ldcline )
end sub

sub fbcDriverAddDefaultLibs( )
	'' select the right FB rtlib
	if( fbGetOption( FB_COMPOPT_FBRT ) ) then
		fbcAddDefLib( "fbrt" + hGetFbLibNameSuffix( ) )
	else
		fbcAddDefLib( "fb" + hGetFbLibNameSuffix( ) )
	end if

	'' and the gfxlib, if gfx functions were used
	if( fbGetOption( FB_COMPOPT_FBGFX ) ) then
		if( fbGetOption( FB_COMPOPT_FBGFX ) = FB_GFXLIB_GFX3 ) then
			fbcAddDefLib( "fbgfx3" + hGetFbLibNameSuffix( ) )
		else
			fbcAddDefLib( "fbgfx" + hGetFbLibNameSuffix( ) )
		end if
		fbcPlatformAddGfxLibs( )
	end if

	'' and the sound runtime, if sound functions were used
	if( fbGetOption( FB_COMPOPT_FBSFX ) ) then
		fbcAddDefLib( "sfx" + hGetFbLibNameSuffix( ) )
		fbcPlatformAddSfxLibs( )
	end if

	fbcPlatformAddDefaultLibs( )

end sub

sub fbcDriverExcludeLibsFromLink( )
	'' Remove any excluded libs from fbc.finallibs list
	dim as TSTRSETITEM ptr i = listGetHead(@fbc.excludedlibs.list)
	while i
		select case i->s
		case "fbrt0.o", "fbrt0pic.o", "fbrt1.o", "fbrt1pic.o", "fbrt2.o", "fbrt2pic.o"
			fbc.nofbrt0 = TRUE
		case else
			strsetDel(@fbc.finallibs, i->s)
		end select
		i = listGetNext(i)
	wend
end sub

'' end of driver/fbc-link.bas
