''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-xbox-platform.bi
''
'' Purpose:
''
''     Keep Xbox target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - add FreeBASIC Xbox archives as concrete linker input files
''     - locate the package-local nxdk tree
''     - add the nxdk import/static libraries expected by nxdk-link
''     - add Xbox default library metadata
''
'' This file intentionally does NOT contain:
''
''     - command-line option parsing
''     - generic linker command construction
''     - cxbe invocation
''

#ifndef __FBC_XBOX_PLATFORM_BI__
#define __FBC_XBOX_PLATFORM_BI__

private function fbcXboxPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_XBOX)
end function

private sub hAddXboxLinkFile( byref ldcline as string, byref file as string )
	ldcline += " " + QUOTE + file + QUOTE
end sub

private sub hAddXboxLibArchive( byref ldcline as string, byref libname as string )
	dim as string filename, found

	filename = "lib" + libname + ".a"
	found = fbcFindLibFile( strptr( filename ) )
	if( len( found ) = 0 ) then
		filename = libname + ".lib"
		found = fbcFindLibFile( strptr( filename ) )
	end if

	if( len( found ) > 0 ) then
		hAddXboxLinkFile( ldcline, found )
	else
		errReportEx( FB_ERRMSG_FILENOTFOUND, filename, -1 )
	end if
end sub

private function hGetXboxNxdkDir( ) as string
	dim as string nxdkdir = environ( "NXDK_DIR" )

	if( len( nxdkdir ) = 0 ) then
		nxdkdir = fbc.prefix + "nxdk"
	end if

	function = pathStripDiv( nxdkdir )
end function

private sub hAddXboxNxdkLib( byref ldcline as string, byref nxdkdir as string, byval relpath as zstring ptr )
	dim as string file = nxdkdir + "/" + *relpath

	hAddXboxLinkFile( ldcline, file )
end sub

private sub hAddXboxNxdkLibs( byref ldcline as string )
	dim as string nxdkdir = hGetXboxNxdkDir( )

	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libpdclib.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libwinapi.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/winmm.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libnxdk_hal.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libnxdk.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libnxdk_automount_d.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libpbkit.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/nxdk_usb.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/libxboxrt.lib" )
	hAddXboxNxdkLib( ldcline, nxdkdir, @"lib/xboxkrnl/libxboxkrnl.lib" )
end sub

private sub fbcXboxPlatformAddDefaultLibPaths( )
	if( fbcXboxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcXboxPlatformAddGfxLibs( )
	if( fbcXboxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcXboxPlatformAddSfxLibs( )
	if( fbcXboxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcXboxPlatformAddDefaultLibs( )
	if( fbcXboxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if

	'' nxdk libraries are added as concrete .lib files in hLinkFiles()
	'' because nxdk-link uses the MSVC-style linker frontend instead of
	'' GNU ld's -L/-l archive lookup.

	'' profiling?
	if( fbGetOption( FB_COMPOPT_PROFILE ) = FB_PROFILE_OPT_GMON ) then
		fbcAddDefLib( "gmon" )
	end if
end sub

private sub fbcXboxPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcXboxPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-xbox-platform.bi
