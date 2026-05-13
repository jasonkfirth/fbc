''
'' FreeBASIC compiler driver
'' -------------------------
''
'' File: fbc-platform.bi
''
'' Purpose:
''
''     Define the compiler driver's per-target platform hook interface and
''     keep the list of platform modules in one place.
''
'' Responsibilities:
''
''     - define the standard hooks implemented by fbc-*-platform.bi files
''     - include every platform file matching an FB_COMPTARGET entry
''     - run platform hooks in a predictable order
''     - keep shared platform helpers out of fbc.bas
''
'' This file intentionally does NOT contain:
''
''     - command-line option parsing
''     - ABI layout or code generation policy
''     - target table data from fb.bas
''     - large platform implementations
''
'' Platform file standard
''
''     One OS compile target maps to one file named:
''
''         fbc-[target-id]-platform.bi
''
''     The [target-id] part must match the target id from fb.bas:targetinfo().
''     Each platform file exports the hook names listed in FBC_PLATFORM_HOOKS
''     for its target.  Hooks must begin by checking whether their platform is
''     the selected FB_COMPTARGET and return immediately if it is not.
''
''     Platform files own behavior that is primarily about the selected OS
''     target, especially:
''
''         - default link libraries and library search paths
''         - target-specific linker flags and startup/shutdown objects
''         - target-specific resource, package, or post-link conversion steps
''         - target toolchain quirks
''         - target validation that depends on OS rules
''
''     Platform files should not own parser behavior, AST behavior, ABI layout,
''     or backend emission rules unless a future hook is explicitly added for
''     that subsystem.  Those areas can still consult env.target and
''     FB_COMPTARGET directly when the decision is genuinely local.
''

#ifndef __FBC_PLATFORM_BI__
#define __FBC_PLATFORM_BI__

type FBC_PLATFORM_HOOKS
	addDefaultLibPaths as sub( )
	addGfxLibs as sub( )
	addSfxLibs as sub( )
	addDefaultLibs as sub( )
	addLinkerFrameworks as sub( byref ldcline as string )
end type

private sub fbcPlatformAddX11GfxLibs( )
	#if defined(__FB_LINUX__) or _
		defined(__FB_FREEBSD__) or _
		defined(__FB_DRAGONFLY__) or _
		defined(__FB_SOLARIS__) or _
		defined(__FB_OPENBSD__) or _
		defined(__FB_NETBSD__)
		fbcAddDefLibPath( "/usr/X11R6/lib" )
	#endif

	#if defined(__FB_DRAGONFLY__)
		fbcAddDefLibPath( "/usr/local/lib/" )
	#endif

	#if defined(__FB_NETBSD__)
		fbcAddDefLibPath( "/usr/X11R7/lib/" )
	#endif

	#if defined(__FB_DARWIN__) and defined(ENABLE_XQUARTZ)
		fbcAddDefLibPath( "/opt/X11/lib" )
	#endif

	#if (not defined(__FB_DARWIN__)) or defined(ENABLE_XQUARTZ)
		fbcAddDefLib( "X11" )
		fbcAddDefLib( "Xext" )
		fbcAddDefLib( "Xpm" )
		fbcAddDefLib( "Xrandr" )
		fbcAddDefLib( "Xrender" )
	#endif
end sub

#include once "fbc-win32-platform.bi"
#include once "fbc-cygwin-platform.bi"
#include once "fbc-linux-platform.bi"
#include once "fbc-android-platform.bi"
#include once "fbc-haiku-platform.bi"
#include once "fbc-dos-platform.bi"
#include once "fbc-xbox-platform.bi"
#include once "fbc-freebsd-platform.bi"
#include once "fbc-dragonfly-platform.bi"
#include once "fbc-solaris-platform.bi"
#include once "fbc-openbsd-platform.bi"
#include once "fbc-darwin-platform.bi"
#include once "fbc-netbsd-platform.bi"
#include once "fbc-js-platform.bi"

'' must be same order as enum FB_COMPTARGET
static shared as FBC_PLATFORM_HOOKS fbcplatforms(0 to FB_COMPTARGETS-1) = _
{ _
	( @fbcWin32PlatformAddDefaultLibPaths,     @fbcWin32PlatformAddGfxLibs,     @fbcWin32PlatformAddSfxLibs,     @fbcWin32PlatformAddDefaultLibs,     @fbcWin32PlatformAddLinkerFrameworks     ), _
	( @fbcCygwinPlatformAddDefaultLibPaths,    @fbcCygwinPlatformAddGfxLibs,    @fbcCygwinPlatformAddSfxLibs,    @fbcCygwinPlatformAddDefaultLibs,    @fbcCygwinPlatformAddLinkerFrameworks    ), _
	( @fbcLinuxPlatformAddDefaultLibPaths,     @fbcLinuxPlatformAddGfxLibs,     @fbcLinuxPlatformAddSfxLibs,     @fbcLinuxPlatformAddDefaultLibs,     @fbcLinuxPlatformAddLinkerFrameworks     ), _
	( @fbcAndroidPlatformAddDefaultLibPaths,   @fbcAndroidPlatformAddGfxLibs,   @fbcAndroidPlatformAddSfxLibs,   @fbcAndroidPlatformAddDefaultLibs,   @fbcAndroidPlatformAddLinkerFrameworks   ), _
	( @fbcHaikuPlatformAddDefaultLibPaths,     @fbcHaikuPlatformAddGfxLibs,     @fbcHaikuPlatformAddSfxLibs,     @fbcHaikuPlatformAddDefaultLibs,     @fbcHaikuPlatformAddLinkerFrameworks     ), _
	( @fbcDosPlatformAddDefaultLibPaths,       @fbcDosPlatformAddGfxLibs,       @fbcDosPlatformAddSfxLibs,       @fbcDosPlatformAddDefaultLibs,       @fbcDosPlatformAddLinkerFrameworks       ), _
	( @fbcXboxPlatformAddDefaultLibPaths,      @fbcXboxPlatformAddGfxLibs,      @fbcXboxPlatformAddSfxLibs,      @fbcXboxPlatformAddDefaultLibs,      @fbcXboxPlatformAddLinkerFrameworks      ), _
	( @fbcFreebsdPlatformAddDefaultLibPaths,   @fbcFreebsdPlatformAddGfxLibs,   @fbcFreebsdPlatformAddSfxLibs,   @fbcFreebsdPlatformAddDefaultLibs,   @fbcFreebsdPlatformAddLinkerFrameworks   ), _
	( @fbcDragonflyPlatformAddDefaultLibPaths, @fbcDragonflyPlatformAddGfxLibs, @fbcDragonflyPlatformAddSfxLibs, @fbcDragonflyPlatformAddDefaultLibs, @fbcDragonflyPlatformAddLinkerFrameworks ), _
	( @fbcSolarisPlatformAddDefaultLibPaths,   @fbcSolarisPlatformAddGfxLibs,   @fbcSolarisPlatformAddSfxLibs,   @fbcSolarisPlatformAddDefaultLibs,   @fbcSolarisPlatformAddLinkerFrameworks   ), _
	( @fbcOpenbsdPlatformAddDefaultLibPaths,   @fbcOpenbsdPlatformAddGfxLibs,   @fbcOpenbsdPlatformAddSfxLibs,   @fbcOpenbsdPlatformAddDefaultLibs,   @fbcOpenbsdPlatformAddLinkerFrameworks   ), _
	( @fbcDarwinPlatformAddDefaultLibPaths,    @fbcDarwinPlatformAddGfxLibs,    @fbcDarwinPlatformAddSfxLibs,    @fbcDarwinPlatformAddDefaultLibs,    @fbcDarwinPlatformAddLinkerFrameworks    ), _
	( @fbcNetbsdPlatformAddDefaultLibPaths,    @fbcNetbsdPlatformAddGfxLibs,    @fbcNetbsdPlatformAddSfxLibs,    @fbcNetbsdPlatformAddDefaultLibs,    @fbcNetbsdPlatformAddLinkerFrameworks    ), _
	( @fbcJsPlatformAddDefaultLibPaths,        @fbcJsPlatformAddGfxLibs,        @fbcJsPlatformAddSfxLibs,        @fbcJsPlatformAddDefaultLibs,        @fbcJsPlatformAddLinkerFrameworks        )  _
}

private sub fbcPlatformAddDefaultLibPaths( )
	for i as integer = 0 to ubound( fbcplatforms )
		fbcplatforms(i).addDefaultLibPaths( )
	next
end sub

private sub fbcPlatformAddGfxLibs( )
	for i as integer = 0 to ubound( fbcplatforms )
		fbcplatforms(i).addGfxLibs( )
	next
end sub

private sub fbcPlatformAddSfxLibs( )
	for i as integer = 0 to ubound( fbcplatforms )
		fbcplatforms(i).addSfxLibs( )
	next
end sub

private sub fbcPlatformAddDefaultLibs( )
	for i as integer = 0 to ubound( fbcplatforms )
		fbcplatforms(i).addDefaultLibs( )
	next
end sub

private sub fbcPlatformAddLinkerFrameworks( byref ldcline as string )
	for i as integer = 0 to ubound( fbcplatforms )
		fbcplatforms(i).addLinkerFrameworks( ldcline )
	next
end sub

#endif

'' end of fbc-platform.bi
