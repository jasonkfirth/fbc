'' Project: FreeBASIC compiler tests
'' File: c-emitted-prototypes.bas
'' Purpose: Exercise public C definitions that have no BASIC callers.
'' Responsibilities: Cover declarations, namespaces, pointer and UDT parameters,
''     calling conventions and termination when linked to a separate C caller.
'' This file intentionally does NOT require that caller in the normal test suite.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

declare function declared_value cdecl alias "fb_audit_declared_value"( byval value as long ) as long
declare function unused_external cdecl alias "fb_audit_unused_external"( ) as long

type exported_pair alias "fb_audit_pair"

	left_value as long
	right_value as long

end type

function exported_value cdecl alias "fb_audit_exported_value"( byval value as long ) as long

	return value + 1

end function

function declared_value cdecl( byval value as long ) as long

	return value + 2

end function

function pair_sum cdecl alias "fb_audit_pair_sum"( byval value as exported_pair ptr ) as long

	if( value = 0 ) then
		return 0
	end if

	return value->left_value + value->right_value

end function

function exported_pointer cdecl alias "fb_audit_pointer"( byval value as const long ptr ) as const long ptr

	return value

end function

namespace prototype_case

	extern "C"

		function namespace_value cdecl alias "fb_audit_namespace_value"( byval value as long ) as long

			return value + 3

		end function

	end extern

end namespace

function stdcall_value stdcall alias "fb_audit_stdcall_value"( byval value as long ) as long

	return value + 4

end function

sub exported_end cdecl alias "fb_audit_end"( )

	end 37

end sub

private function unused_private cdecl alias "fb_audit_unused_private"( ) as long

	return 99

end function

#ifdef C_EXPORT_DRIVER

	extern "C"
		declare function fb_audit_check_exports( byval terminate as long ) as long
	end extern

	end fb_audit_check_exports( clng( len( command( 1 ) ) <> 0 ) )

#else

	end 0

#endif

'' end of c-emitted-prototypes.bas
