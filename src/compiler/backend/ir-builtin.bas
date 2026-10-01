'' Project: FreeBASIC compiler - shared backend builtin policy
'' -----------------------------------------
''
'' File: backend/ir-builtin.bas
''
'' Purpose:
''
''     Identify compiler builtins whose signatures match a C library symbol.
''
'' Responsibilities:
''
''     - share the libc mapping used by LLVM and Clang emission
''     - keep calls and procedure addresses on the same external ABI
''
'' This file intentionally does NOT contain:
''
''     - instruction emission or mappings for intrinsic-only builtins
''

#include once "core/fb.bi"
#include once "core/fbint.bi"
#include once "backend/ir.bi"

function irGetBuiltinLibcName( byval proc as FBSYMBOL ptr ) as string

	if( proc = NULL ) then return ""
	if( not symbIsProc( proc ) ) then return ""
	if( symbGetIsParsed( proc ) ) then return ""
	if( proc->id.alias = NULL ) then return ""
	if( symbGetProcMode( proc ) <> FB_FUNCMODE_CDECL ) then return ""

	'' Inspect the source alias before platform decoration. These declarations
	'' have libc's ABI; intrinsic-only operations have no addressable C symbol.
	var aliasname = *proc->id.alias
	if( left( aliasname, 10 ) <> "__builtin_" ) then return ""
	var libcname = mid( aliasname, 11 )
	select case( libcname )
	case "memchr", "memcmp", "memcpy", "memmove", "memset", _
	     "strlen", "strcmp", "strncmp", "strcpy", "strncpy", _
	     "strcat", "strncat", "strchr", "strrchr", "strstr", _
	     "strpbrk", "strspn", "strcspn"
		return libcname
	end select
	return ""

end function

'' end of backend/ir-builtin.bas
