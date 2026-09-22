''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: dos/fbc-platform.bi
''
'' Purpose:
''
''     Keep DOS target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add DJGPP library search paths
''     - add DOS default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - DOS ABI or code generation rules
''

#ifndef __FBC_DOS_PLATFORM_BI__
#define __FBC_DOS_PLATFORM_BI__

private function fbcDosPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_DOS)
end function

private sub fbcDosPlatformAddDefaultLibPaths( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

#ifndef ENABLE_STANDALONE
	'' Help out the DJGPP linker to find DJGPP's lib/ dir.
	'' It doesn't seem to add it by default like on other systems.
	'' Note: Can't use libc here, we have a fixed copy of that in
	'' the compiler's lib/ dir.
	fbcAddLibPathFor( "libm.a" )
#ifdef FB_DOS_WATT32
	'' The opt-in provider is installed beside the DJGPP runtime archives.
	fbcAddLibPathFor( "libwatt.a" )
#endif
#endif
end sub

private sub fbcDosPlatformAddGfxLibs( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private function fbcDosPlatformLinksWatt32( ) as integer
	dim as TSTRSETITEM ptr item

	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		function = FALSE
		exit function
	end if

	item = listGetHead( @fbc.finallibs.list )
	while( item )
		if( lcase( item->s ) = "watt" ) then
			function = TRUE
			exit function
		end if
		item = listGetNext( item )
	wend

	function = FALSE
end function

private sub fbcDosPlatformAddSfxLibs( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcDosPlatformAddDefaultLibs( )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "gcc" )
	fbcAddDefLib( "c" )
	fbcAddDefLib( "m" )
	if( fbc.dos_threads ) then
		fbcAddDefLib( "fbpdmlwp" )
	end if
#ifdef FB_DOS_WATT32
	fbcAddDefLib( "watt" )
#endif
	'' Default DOS has no socket library. A cross compiler's host TCP support
	'' must not introduce a nonexistent DOS -lsocket dependency.
end sub

private sub fbcDosPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcDosPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
	if( fbcDosPlatformLinksWatt32( ) ) then
		'' Watt-32's neterr object deliberately replaces DJGPP's strerror() so
		'' socket-specific errno values retain their messages. The runtime
		'' archives are linked as a group, requiring GNU ld to accept that
		'' documented duplicate definition.
		ldcline += " --allow-multiple-definition"
	end if
	if( fbc.dos_threads ) then
		'' DJGPP ld handles COFF's leading underscore itself. Pass C names
		'' so allocations from precompiled libraries also use the wrappers.
		ldcline += " --wrap=malloc --wrap=calloc --wrap=realloc --wrap=free"
		'' BIOS delay() otherwise disables the RTC used for preemption.
		ldcline += " --wrap=delay"
		'' DOS/BIOS calls share a nonreentrant real-mode stack.
		ldcline += " --wrap=__dpmi_int"
		'' Concurrent PIT reads share DJGPP's midnight rollover state.
		ldcline += " --wrap=uclock"
	end if
end sub

#endif

'' end of dos/fbc-platform.bi
