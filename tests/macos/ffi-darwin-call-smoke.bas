''
'' FreeBASIC macOS libffi tests
'' ----------------------------
''
'' File: ffi-darwin-call-smoke.bas
''
'' Purpose:
''
''     Exercise the system libffi call and closure paths through ffi.bi.
''
'' Responsibilities:
''
''     - prepare and invoke a native function through ffi_call
''     - allocate and invoke a callback through ffi_prep_closure_loc
''     - use sizeof(ffi_closure), which is architecture-dependent on Darwin
''     - return a diagnostic failure code for each failed stage
''
'' This file intentionally does NOT contain:
''
''     - deprecated ffi_prep_closure use
''     - raw or Java closure coverage
''     - non-Darwin fallback behavior
''

#include once "ffi.bi"

#ifndef __FB_DARWIN__
	#error "ffi-darwin-call-smoke.bas requires the Darwin target"
#endif

const PREP_CALL_FAILED = 1
const CALL_RESULT_FAILED = 2
const CLOSURE_ALLOC_FAILED = 3
const PREP_CLOSURE_FAILED = 4
const CLOSURE_RESULT_FAILED = 5
const LONG_DOUBLE_ALIAS_FAILED = 6

function add_values cdecl _
	( _
		byval lhs as long, _
		byval rhs as long _
	) as long

	function = lhs + rhs
end function

sub add_closure cdecl _
	( _
		byval cif as ffi_cif ptr, _
		byval result as any ptr, _
		byval arguments as any ptr ptr, _
		byval user_data as any ptr _
	)

	dim as long lhs = *cptr( long ptr, arguments[0] )
	dim as long rhs = *cptr( long ptr, arguments[1] )
	dim as long bias = *cptr( long ptr, user_data )

	*cptr( long ptr, result ) = lhs + rhs + bias
end sub

dim as ffi_type ptr argument_types(0 to 1) = _
	{ _
		@ffi_type_sint32, _
		@ffi_type_sint32 _
	}

#ifdef __FB_ARM__
	if( @ffi_type_longdouble <> @ffi_type_double ) then
		end LONG_DOUBLE_ALIAS_FAILED
	end if
	if( @ffi_type_complex_longdouble <> @ffi_type_complex_double ) then
		end LONG_DOUBLE_ALIAS_FAILED
	end if
#endif

dim as long lhs = 19
dim as long rhs = 23
dim as any ptr argument_values(0 to 1) = _
	{ _
		@lhs, _
		@rhs _
	}

dim as ffi_cif call_interface
if( ffi_prep_cif( _
	@call_interface, _
	FFI_DEFAULT_ABI, _
	2, _
	@ffi_type_sint32, _
	@argument_types(0) _
) <> FFI_OK ) then
	end PREP_CALL_FAILED
end if

dim as long call_result
ffi_call( _
	@call_interface, _
	FFI_FN( @add_values ), _
	@call_result, _
	@argument_values(0) _
)

if( call_result <> 42 ) then
	end CALL_RESULT_FAILED
end if

dim as function cdecl( byval as long, byval as long ) as long closure_entry
dim as ffi_closure ptr closure = ffi_closure_alloc( _
	sizeof( ffi_closure ), _
	cptr( any ptr ptr, @closure_entry ) _
)

if( closure = 0 ) then
	end CLOSURE_ALLOC_FAILED
end if

dim as long bias = 5
if( ffi_prep_closure_loc( _
	closure, _
	@call_interface, _
	@add_closure, _
	@bias, _
	cptr( any ptr, closure_entry ) _
) <> FFI_OK ) then
	ffi_closure_free closure
	end PREP_CLOSURE_FAILED
end if

dim as long closure_result = closure_entry( 17, 20 )
ffi_closure_free closure

if( closure_result <> 42 ) then
	end CLOSURE_RESULT_FAILED
end if

end 0

'' end of ffi-darwin-call-smoke.bas
