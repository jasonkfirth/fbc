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
''     - add Linux gfx and sound dependencies present in the target sysroot
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

private function fbcLinuxPlatformHasLibrary( byval libname as zstring ptr ) as integer
	dim as string filename

	filename = "lib" + *libname + ".a"
	if( len( fbcFindLibFile( strptr( filename ) ) ) > 0 ) then
		return TRUE
	end if

	filename = "lib" + *libname + ".so"
	function = (len( fbcFindLibFile( strptr( filename ) ) ) > 0)
end function

private sub fbcLinuxPlatformAddLibraryIfPresent( byval libname as zstring ptr )
	if( fbcLinuxPlatformHasLibrary( libname ) ) then
		fbcAddDefLib( libname )
	end if
end sub

private sub fbcLinuxPlatformAddDefaultLibPaths( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcLinuxPlatformAddGfxLibs( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' Cross sysroots are commonly framebuffer-only.  Query the target compiler
	'' instead of making a host-side X11 installation part of the target ABI.
	if( fbcLinuxPlatformHasLibrary( "X11" ) ) then
		fbcAddDefLibPath( "/usr/X11R6/lib" )
		fbcLinuxPlatformAddLibraryIfPresent( "X11" )
		fbcLinuxPlatformAddLibraryIfPresent( "Xext" )
		fbcLinuxPlatformAddLibraryIfPresent( "Xpm" )
		fbcLinuxPlatformAddLibraryIfPresent( "Xrandr" )
		fbcLinuxPlatformAddLibraryIfPresent( "Xrender" )
	end if
end sub

private sub fbcLinuxPlatformAddSfxLibs( )
	if( fbcLinuxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' A headless cross package still provides the mixer and null driver.  Add
	'' native transports only when their target development libraries exist.
	fbcLinuxPlatformAddLibraryIfPresent( "asound" )
	fbcLinuxPlatformAddLibraryIfPresent( "pulse-simple" )
	fbcLinuxPlatformAddLibraryIfPresent( "pulse" )
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
