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
''     - define the standard hooks implemented by platform subdirectories
''     - include every platform file matching an FB_COMPTARGET entry
''     - run platform hooks in a predictable order
''     - choose target-specific linker tools
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
''         [target-id]/fbc-platform.bi
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
	getLinkerTool as function( ) as integer
	addDefaultLibPaths as sub( )
	addGfxLibs as sub( )
	addSfxLibs as sub( )
	addDefaultLibs as sub( )
	addLinkerFrameworks as sub( byref ldcline as string )
end type

private function fbcPlatformGetDefaultLinkerTool( ) as integer
	function = FBCTOOL_LD
end function

private sub fbcPlatformAddX11GfxLibs( )
	#if defined(__FB_LINUX__) or _
		defined(__FB_FREEBSD__) or _
		defined(__FB_DRAGONFLY__) or _
		defined(__FB_SOLARIS__) or _
		defined(__FB_ILLUMOS__) or _
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
#ifndef DISABLE_XPM
		fbcAddDefLib( "Xpm" )
#endif
		fbcAddDefLib( "Xrandr" )
		fbcAddDefLib( "Xrender" )
	#endif
end sub

#include once "win32/fbc-platform.bi"
#include once "cygwin/fbc-platform.bi"
#include once "linux/fbc-platform.bi"
#include once "android/fbc-platform.bi"
#include once "haiku/fbc-platform.bi"
#include once "dos/fbc-platform.bi"
#include once "xbox/fbc-platform.bi"
#include once "freebsd/fbc-platform.bi"
#include once "dragonfly/fbc-platform.bi"
#include once "solaris/fbc-platform.bi"
#include once "illumos/fbc-platform.bi"
#include once "openbsd/fbc-platform.bi"
#include once "darwin/fbc-platform.bi"
#include once "netbsd/fbc-platform.bi"
#include once "js/fbc-platform.bi"
#include once "wii/fbc-platform.bi"
#include once "nuttx/fbc-platform.bi"
#include once "riscos/fbc-platform.bi"

'' must be same order as enum FB_COMPTARGET
static shared as FBC_PLATFORM_HOOKS fbcplatforms(0 to FB_COMPTARGETS-1) = _
{ _
	( @fbcPlatformGetDefaultLinkerTool, @fbcWin32PlatformAddDefaultLibPaths, _
	  @fbcWin32PlatformAddGfxLibs, @fbcWin32PlatformAddSfxLibs, _
	  @fbcWin32PlatformAddDefaultLibs, @fbcWin32PlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcCygwinPlatformAddDefaultLibPaths, _
	  @fbcCygwinPlatformAddGfxLibs, @fbcCygwinPlatformAddSfxLibs, _
	  @fbcCygwinPlatformAddDefaultLibs, @fbcCygwinPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcLinuxPlatformAddDefaultLibPaths, _
	  @fbcLinuxPlatformAddGfxLibs, @fbcLinuxPlatformAddSfxLibs, _
	  @fbcLinuxPlatformAddDefaultLibs, @fbcLinuxPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcAndroidPlatformAddDefaultLibPaths, _
	  @fbcAndroidPlatformAddGfxLibs, @fbcAndroidPlatformAddSfxLibs, _
	  @fbcAndroidPlatformAddDefaultLibs, @fbcAndroidPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcHaikuPlatformAddDefaultLibPaths, _
	  @fbcHaikuPlatformAddGfxLibs, @fbcHaikuPlatformAddSfxLibs, _
	  @fbcHaikuPlatformAddDefaultLibs, @fbcHaikuPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcDosPlatformAddDefaultLibPaths, _
	  @fbcDosPlatformAddGfxLibs, @fbcDosPlatformAddSfxLibs, _
	  @fbcDosPlatformAddDefaultLibs, @fbcDosPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcXboxPlatformAddDefaultLibPaths, _
	  @fbcXboxPlatformAddGfxLibs, @fbcXboxPlatformAddSfxLibs, _
	  @fbcXboxPlatformAddDefaultLibs, @fbcXboxPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcFreebsdPlatformAddDefaultLibPaths, _
	  @fbcFreebsdPlatformAddGfxLibs, @fbcFreebsdPlatformAddSfxLibs, _
	  @fbcFreebsdPlatformAddDefaultLibs, @fbcFreebsdPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcDragonflyPlatformAddDefaultLibPaths, _
	  @fbcDragonflyPlatformAddGfxLibs, @fbcDragonflyPlatformAddSfxLibs, _
	  @fbcDragonflyPlatformAddDefaultLibs, @fbcDragonflyPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcSolarisPlatformAddDefaultLibPaths, _
	  @fbcSolarisPlatformAddGfxLibs, @fbcSolarisPlatformAddSfxLibs, _
	  @fbcSolarisPlatformAddDefaultLibs, @fbcSolarisPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcIllumosPlatformAddDefaultLibPaths, _
	  @fbcIllumosPlatformAddGfxLibs, @fbcIllumosPlatformAddSfxLibs, _
	  @fbcIllumosPlatformAddDefaultLibs, @fbcIllumosPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcOpenbsdPlatformAddDefaultLibPaths, _
	  @fbcOpenbsdPlatformAddGfxLibs, @fbcOpenbsdPlatformAddSfxLibs, _
	  @fbcOpenbsdPlatformAddDefaultLibs, @fbcOpenbsdPlatformAddLinkerFrameworks ), _
	( @fbcDarwinPlatformGetLinkerTool, @fbcDarwinPlatformAddDefaultLibPaths, _
	  @fbcDarwinPlatformAddGfxLibs, @fbcDarwinPlatformAddSfxLibs, _
	  @fbcDarwinPlatformAddDefaultLibs, @fbcDarwinPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcNetbsdPlatformAddDefaultLibPaths, _
	  @fbcNetbsdPlatformAddGfxLibs, @fbcNetbsdPlatformAddSfxLibs, _
	  @fbcNetbsdPlatformAddDefaultLibs, @fbcNetbsdPlatformAddLinkerFrameworks ), _
	( @fbcJsPlatformGetLinkerTool, @fbcJsPlatformAddDefaultLibPaths, _
	  @fbcJsPlatformAddGfxLibs, @fbcJsPlatformAddSfxLibs, _
	  @fbcJsPlatformAddDefaultLibs, @fbcJsPlatformAddLinkerFrameworks ), _
	( @fbcWiiPlatformGetLinkerTool, @fbcWiiPlatformAddDefaultLibPaths, _
	  @fbcWiiPlatformAddGfxLibs, @fbcWiiPlatformAddSfxLibs, _
	  @fbcWiiPlatformAddDefaultLibs, @fbcWiiPlatformAddLinkerFrameworks ), _
	( @fbcPlatformGetDefaultLinkerTool, @fbcNuttxPlatformAddDefaultLibPaths, _
	  @fbcNuttxPlatformAddGfxLibs, @fbcNuttxPlatformAddSfxLibs, _
	  @fbcNuttxPlatformAddDefaultLibs, @fbcNuttxPlatformAddLinkerFrameworks ), _
	( @fbcRiscosPlatformGetLinkerTool, @fbcRiscosPlatformAddDefaultLibPaths, _
	  @fbcRiscosPlatformAddGfxLibs, @fbcRiscosPlatformAddSfxLibs, _
	  @fbcRiscosPlatformAddDefaultLibs, @fbcRiscosPlatformAddLinkerFrameworks )  _
}

private function fbcPlatformGetLinkerTool( ) as integer
	select case as const fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_DARWIN
		return fbcDarwinPlatformGetLinkerTool( )
	case FB_COMPTARGET_JS
		return fbcJsPlatformGetLinkerTool( )
	case FB_COMPTARGET_WII
		return fbcWiiPlatformGetLinkerTool( )
	case FB_COMPTARGET_RISCOS
		return fbcRiscosPlatformGetLinkerTool( )
	case else
		'' Most targets link through the normal linker.  Keep this hook
		'' explicit instead of dispatching through the platform procptr table:
		'' the linker path must remain safe even while bootstrapping compiler
		'' changes that add or reorder platform targets.
		return FBCTOOL_LD
	end select
end function

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

private function fbcPlatformMapLibName( byref libname as string ) as string
	select case fbGetOption( FB_COMPOPT_TARGET )
	case FB_COMPTARGET_WIN32
		return fbcWin32PlatformMapLibName( libname )
	case FB_COMPTARGET_OPENBSD
		if( libname = "stdc++" ) then
			return "estdc++"
		end if
	end select

	function = libname
end function

private sub fbcPlatformAddLinkerFrameworks( byref ldcline as string )
	for i as integer = 0 to ubound( fbcplatforms )
		fbcplatforms(i).addLinkerFrameworks( ldcline )
	next
end sub

#endif

'' end of fbc-platform.bi
