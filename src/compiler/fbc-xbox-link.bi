''
'' FreeBASIC compiler driver
''
'' File: fbc-xbox-link.bi
''
'' Purpose:
''
''     Keep the nxdk-specific Xbox link command helpers out of fbc.bas.
''
'' Responsibilities:
''
''     - add FreeBASIC Xbox archives as concrete linker input files
''     - locate the package-local nxdk tree
''     - add the nxdk import/static libraries expected by nxdk-link
''
'' This file intentionally does NOT contain:
''
''     - command-line option parsing
''     - generic linker command construction
''     - cxbe invocation
''

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

'' end of fbc-xbox-link.bi
