''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-js-platform.bi
''
'' Purpose:
''
''     Keep JavaScript/Emscripten target driver behavior out of fbc.bas.
''
'' Responsibilities:
''
''     - provide standard platform hooks for the JS target
''     - reserve a clear place for future Emscripten link/library behavior
''
'' This file intentionally does NOT contain:
''
''     - generic linker command construction
''     - command-line option parsing
''     - Emscripten ABI or code generation rules
''

#ifndef __FBC_JS_PLATFORM_BI__
#define __FBC_JS_PLATFORM_BI__

private function fbcJsPlatformIsSelected( ) as integer
	function = (fbGetOption( FB_COMPOPT_TARGET ) = FB_COMPTARGET_JS)
end function

private sub fbcJsPlatformAddDefaultLibPaths( )
	if( fbcJsPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcJsPlatformAddGfxLibs( )
	if( fbcJsPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcJsPlatformAddSfxLibs( )
	if( fbcJsPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcJsPlatformAddDefaultLibs( )
	if( fbcJsPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

private sub fbcJsPlatformAddLinkerFrameworks( byref ldcline as string )
	if( fbcJsPlatformIsSelected( ) = FALSE ) then
		exit sub
	end if
end sub

#endif

'' end of fbc-js-platform.bi
