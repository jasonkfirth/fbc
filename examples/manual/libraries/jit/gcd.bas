'' examples/manual/libraries/jit/gcd.bas
''
'' Example extracted from the FreeBASIC Manual
'' from topic 'libjit'
'' Purpose: Compile a recursive greatest-common-divisor function.
'' Responsibilities: Positive unsigned inputs, build locking and context cleanup.
'' This example contains no arbitrary-input parser or native-float ABI calls.
''
'' See Also: https://www.freebasic.net/wiki/wikka.php?wakka=ExtLibJit
'' --------

'' GCD calculation example

#include "jit.bi"

' The caller owns the build lock and releases the context on failure.
Function emit_gcd(ByVal gcd As jit_function_t) As Integer
	' build function
	' check x = y
	Dim As jit_value_t x, y, x_eq_y
	x = jit_value_get_param(gcd, 0)
	y = jit_value_get_param(gcd, 1)
	If x = 0 OrElse y = 0 Then Return 0
	x_eq_y = jit_insn_eq(gcd, x, y)
	If x_eq_y = 0 Then Return 0

	' if x = y, return x
	Dim label_x_ne_y As jit_label_t = jit_label_undefined
	If jit_insn_branch_if_not(gcd, x_eq_y, @label_x_ne_y) = 0 Then Return 0
	If jit_insn_return(gcd, x) = 0 Then Return 0

	' else if...
	If jit_insn_label(gcd, @label_x_ne_y) = 0 Then Return 0

	' check x < y
	Dim As jit_value_t x_lt_y
	Dim label_x_gte_y As jit_label_t = jit_label_undefined
	x_lt_y = jit_insn_lt(gcd, x, y)
	If x_lt_y = 0 Then Return 0
	If jit_insn_branch_if_not(gcd, x_lt_y, @label_x_gte_y) = 0 Then Return 0

	' if x < y, return gcd(x, y-x)
	Dim As jit_value_t gcd_args(0 To 1), gcd_result
	gcd_args(0) = x
	gcd_args(1) = jit_insn_sub(gcd, y, x)
	If gcd_args(1) = 0 Then Return 0
	gcd_result = jit_insn_call( _
		gcd,          _ ' where we are calling from
		"gcd",        _ ' function name
		gcd,          _ ' function reference
		0,            _ ' signature = auto
		@gcd_args(0), _ ' arguments
		2,            _ ' argument count
		0             _ ' flags = nothing special
	)
	If gcd_result = 0 Then Return 0
	If jit_insn_return(gcd, gcd_result) = 0 Then Return 0

	' else...
	If jit_insn_label(gcd, @label_x_gte_y) = 0 Then Return 0

	' return gcd(x-y, y)
	gcd_args(0) = jit_insn_sub(gcd, x, y)
	If gcd_args(0) = 0 Then Return 0
	gcd_args(1) = y
	gcd_result = jit_insn_call( _
		gcd,          _ ' where we are calling from
		"gcd",        _ ' function name
		gcd,          _ ' function reference
		0,            _ ' signature = auto
		@gcd_args(0), _ ' arguments
		2,            _ ' argument count
		0             _ ' flags = nothing special
	)
	If gcd_result = 0 Then Return 0
	If jit_insn_return(gcd, gcd_result) = 0 Then Return 0

	' compile function
	Return jit_function_compile(gcd)
End Function

' initialize libjit
Dim context As jit_context_t = jit_context_create()
If context = 0 Then End 1
jit_context_build_start(context)

' define function gcd(x as jit_uint, y as jit_uint) as jit_uint
Dim params(0 To 1) As jit_type_t = {jit_type_uint, jit_type_uint}
Dim signature As jit_type_t = jit_type_create_signature( _
	jit_abi_cdecl,  _ ' C-style function
	jit_type_uint,  _ ' Return type
	@params(0),     _ ' Parameter array
	2,              _ ' Number of components
	1               _ ' Count references?
)
If signature = 0 Then
	jit_context_build_end(context)
	jit_context_destroy(context)
	End 1
End If
Dim gcd As jit_function_t = jit_function_create(context, signature)
'' The function retains its own signature reference.
jit_type_free(signature)
If gcd = 0 Then
	jit_context_build_end(context)
	jit_context_destroy(context)
	End 1
End If

Dim As Integer build_ok = emit_gcd(gcd)
jit_context_build_end(context)
If build_ok = 0 Then
	jit_context_destroy(context)
	End 1
End If

' call function
Dim As jit_uint a = 21
Dim As jit_uint b = 14
Dim As jit_uint result
Dim As jit_uint Ptr args(0 To 1) = {@a, @b}
'' The subtraction algorithm needs positive inputs to make progress.
If a = 0 OrElse b = 0 Then
	jit_context_destroy(context)
	End 1
End If
If jit_function_apply(gcd, @args(0), @result) = 0 Then
	jit_context_destroy(context)
	End 1
End If
Print Using "gcd(&, &) = &"; a; b; result

' clean up libjit
jit_context_destroy(context)
'' end of gcd.bas
