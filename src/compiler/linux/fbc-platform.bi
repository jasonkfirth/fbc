''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: linux/fbc-platform.bi
''
'' Purpose:
''
''     Keep Linux target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add Linux gfx and sound library dependencies
''     - choose ncurses or tinfo for the runtime library dependency
''     - add Linux default system libraries
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Linux ABI or code generation rules
''

#ifndef __FBC_LINUX_PLATFORM_BI__
#define __FBC_LINUX_PLATFORM_BI__

private function fbcLinuxPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_LINUX)
end function

private sub fbcLinuxPlatformAddDefaultLibPaths( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcLinuxPlatformAddGfxLibs( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcPlatformAddX11GfxLibs( )
end sub

private sub fbcLinuxPlatformAddSfxLibs( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	fbcAddDefLib( "asound" )
	fbcAddDefLib( "pulse-simple" )
	fbcAddDefLib( "pulse" )
end sub

private sub fbcLinuxPlatformAddDefaultLibs( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	''
	'' Notes:
	''
	'' When linking statically, -lpthread apparently should be
	'' linked before -lc. Otherwise there can be errors due to
	'' -lpthread/-lc containing overlapping symbols (but the pthread
	'' ones should be used). This is confirmed by minimal testing,
	'' searching the web and 'gcc -pthread' behavior.
	''
	'' libncurses and libtinfo: FB's rtlib depends on the libtinfo
	'' part of ncurses, which sometimes is included in libncurses
	'' and sometimes separate (depending on how ncurses was built).

	'' Prefer libtinfo over libncurses
	if( (len( fbcFindLibFile( "libtinfo.a"  ) ) > 0) or _
		(len( fbcFindLibFile( "libtinfo.so" ) ) > 0) ) then
		fbcAddDefLib( "tinfo" )
	else
		fbcAddDefLib( "ncurses" )
	end if

	fbcAddDefLib( "m" )
	fbcAddDefLib( "dl" )
	fbcAddDefLib( "pthread" )
	fbcAddDefLib( "gcc" )

	'' Link libgcc_eh if it exists (it depends on the gcc build)
	if( (len( fbcFindLibFile( "libgcc_eh.a"  ) ) > 0) or _
		(len( fbcFindLibFile( "libgcc_eh.so" ) ) > 0) ) then
		fbcAddDefLib( "gcc_eh" )
	end if

	fbcAddDefLib( "c" )
end sub

private sub fbcLinuxPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of linux/fbc-platform.bi
